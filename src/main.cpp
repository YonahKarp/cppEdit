#include "app.h"
#include "editor.h"
#include "sidebar.h"
#include "input.h"
#include "search.h"

#include <fstream>
#include <string>
#include <sys/stat.h>

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#include "nuklear.h"

#include "nuklear_sdl_renderer.h"
#include "theme.h"

static void load_editor_state(EditorState& state, App& app, const char* state_file_path, 
                               const char* default_file_path, const char* user_files_dir) {
    std::string current_file_path = default_file_path;
    int saved_cursor_pos = -1;
    float saved_scroll_y = -1.0f;
    float saved_font_size = 30.0f;
    int saved_dark_theme = 1;

    std::ifstream state_in(state_file_path);
    if (state_in) {
        std::getline(state_in, current_file_path);
        state_in >> saved_cursor_pos >> saved_scroll_y;
        if (state_in >> saved_font_size) {
            if (saved_font_size < App::MIN_FONT_SIZE || saved_font_size > App::MAX_FONT_SIZE) {
                saved_font_size = 30.0f;
            }
        }
        state_in >> saved_dark_theme;
        state_in.close();
        if (current_file_path.empty()) {
            current_file_path = default_file_path;
        }
    }

    struct stat st;
    // Create file if it doesn't exist
    if (stat(current_file_path.c_str(), &st) != 0) {
        std::ofstream new_file(current_file_path);
        new_file.close();
    }

    init_editor_state(state, current_file_path.c_str(), state_file_path, 
                      saved_cursor_pos, saved_scroll_y);
    state.user_files_dir = user_files_dir;
    state.font_size = saved_font_size;
    state.dark_theme = (saved_dark_theme != 0);

    if (saved_font_size != 30.0f) {
        reload_fonts(app, saved_font_size);
    }
}

static void flush_pending_saves(EditorState& state) {
    if (state.content_pending_save) {
        safe_save(state);
    }
    if (state.state_pending_save) {
        save_state(state, state.pending_cursor_pos, state.pending_scroll_y);
    }
}

int main(int argc, char** argv) {
    (void)argc; (void)argv;

    App app;
    if (!init_app(app)) {
        return 1;
    }

    const char* user_files_dir = "../user_files";
     const char* recently_deleted_dir = "../user_files/recently_deleted";
    const char* state_file_path = "../user_files/.editor_state";
    const char* default_file_path = "../user_files/file 1.txt";

    struct stat st;
    if (stat(user_files_dir, &st) != 0) {
        #ifdef _WIN32
        mkdir(user_files_dir);
        #else
        mkdir(user_files_dir, 0755);
        #endif
    }

    if (stat(recently_deleted_dir, &st) != 0) {
        #ifdef _WIN32
        mkdir(recently_deleted_dir);
        #else
        mkdir(recently_deleted_dir, 0755);
        #endif
    }

    EditorState editor;
    load_editor_state(editor, app, state_file_path, default_file_path, user_files_dir);
    editor.recently_deleted_dir = recently_deleted_dir;
    
    cleanup_old_deleted_files(editor);

    const Uint32 DEBOUNCE_DELAY = 500;
    const Uint32 RENAME_DEBOUNCE_DELAY = 200;
    const Uint32 BLINK_INTERVAL = 530;

    bool needs_redraw = true;

    bool running = true;
    while (running) {
        bool has_selection = (editor.prev_sel_start != editor.prev_sel_end);
        
        bool sidebar_animating = false;
        if (editor.sidebar.anim_start_time > 0) {
            Uint32 elapsed = SDL_GetTicks() - editor.sidebar.anim_start_time;
            sidebar_animating = (elapsed < SidebarState::ANIM_DURATION_MS + 50);
        }

        Uint32 wait_time;
        if (sidebar_animating) {
            wait_time = 1;
        } else if (!has_selection) {
            Uint32 time_since_blink = SDL_GetTicks() - editor.last_blink_time;
            Uint32 time_to_next_blink = time_since_blink >= BLINK_INTERVAL ? 0 : BLINK_INTERVAL - time_since_blink;
            wait_time = time_to_next_blink < 50 ? 1 : 50;
        } else {
            wait_time = 50;
        }

        SDL_Event wait_event;
        bool had_event = SDL_WaitEventTimeout(&wait_event, wait_time);

        InputState input = {};
        if (had_event) {
            input = process_events(app.ctx, editor, &wait_event);
        } else {
            input = process_events(app.ctx, editor, nullptr);
        }

        if (input.quit) {
            running = false;
        }

        if (input.meta) {
            toggle_sidebar(editor);
            if (editor.sidebar.visible) {
                close_search(editor.search);
            }
            needs_redraw = true;
        }

        if (input.increase_font_size || input.decrease_font_size) {
            float new_size = app.font_size + (input.increase_font_size ? App::FONT_SIZE_STEP : -App::FONT_SIZE_STEP);
            reload_fonts(app, new_size);
            editor.font_size = app.font_size;
            editor.state_pending_save = true;
            editor.state_change_time = SDL_GetTicks();
            needs_redraw = true;
        }

        if (input.toggle_theme) {
            editor.dark_theme = !editor.dark_theme;
            editor.state_pending_save = true;
            editor.state_change_time = SDL_GetTicks();
            needs_redraw = true;
        }

        if (input.toggle_search && !editor.sidebar.visible) {
            toggle_search(editor.search);
            needs_redraw = true;
        }

        if (input.search_next && editor.search.active) {
            int pos = navigate_to_next_match(editor.search);
            if (pos >= 0) {
                editor.pending_navigate_to_pos = pos;
                needs_redraw = true;
            }
        }

        if (input.search_prev && editor.search.active) {
            int pos = navigate_to_prev_match(editor.search);
            if (pos >= 0) {
                editor.pending_navigate_to_pos = pos;
                needs_redraw = true;
            }
        }

        handle_sidebar_keyboard(editor, input);

        bool prev_cursor_visible = editor.cursor_visible;
        handle_cursor_blink(editor, input.had_input);
        bool cursor_changed = (prev_cursor_visible != editor.cursor_visible);

        if (input.had_input) {
            needs_redraw = true;
        }
        if (cursor_changed && !has_selection) {
            needs_redraw = true;
        }
        if (sidebar_animating) {
            needs_redraw = true;
        }
        if (editor.first_frame || editor.second_frame) {
            needs_redraw = true;
        }
        if (editor.pending_select_all || editor.pending_paragraph_move != 0 || 
            editor.pending_jump_to_end != 0 || editor.pending_delete_word ||
            editor.pending_navigate_to_pos >= 0 || editor.pending_undo) {
            needs_redraw = true;
        }

        if (needs_redraw) {
            render_editor(app.ctx, editor, app.window_width, app.window_height, 
                          app.font, app.status_font);
            render_sidebar(app.ctx, editor, app.window_width, app.window_height, app.font, app.sidebar_font, app.status_font);

            const Theme& theme = get_theme(editor.dark_theme);
            SDL_SetRenderDrawColor(app.renderer, theme.background.r, theme.background.g, theme.background.b, 255);
            SDL_RenderClear(app.renderer);
            nk_sdl_render(NK_ANTI_ALIASING_ON);
            SDL_RenderPresent(app.renderer);

            if (editor.processed_pending_action) {
                editor.processed_pending_action = false;
                needs_redraw = true;
            } else {
                needs_redraw = false;
            }
        }

        process_pending_saves(editor, DEBOUNCE_DELAY);
        process_rename_save(editor, RENAME_DEBOUNCE_DELAY);
    }

    flush_pending_saves(editor);
    shutdown_app(app);

    return 0;
}
