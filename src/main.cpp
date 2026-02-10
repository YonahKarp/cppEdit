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

    EditorState editor;
    load_editor_state(editor, app, state_file_path, default_file_path, user_files_dir);

    const Uint32 DEBOUNCE_DELAY = 500;
    const Uint32 RENAME_DEBOUNCE_DELAY = 200;
    const Uint32 FRAME_RATE = 60;


    bool running = true;
    while (running) {
        InputState input = process_events(app.ctx, editor);

        if (input.quit) {
            running = false;
        }

        if (input.meta) {
            toggle_sidebar(editor);
            if (editor.sidebar.visible) {
                close_search(editor.search);
            }
        }

        if (input.increase_font_size || input.decrease_font_size  ) {
            float new_size = app.font_size + (input.increase_font_size ? App::FONT_SIZE_STEP : -App::FONT_SIZE_STEP);
            reload_fonts(app, new_size);
            editor.font_size = app.font_size;
            editor.state_pending_save = true;
            editor.state_change_time = SDL_GetTicks();
        }

        if (input.toggle_theme) {
            editor.dark_theme = !editor.dark_theme;
            editor.state_pending_save = true;
            editor.state_change_time = SDL_GetTicks();
        }

        if (input.toggle_search && !editor.sidebar.visible) {
            toggle_search(editor.search);
        }

        if (input.search_next && editor.search.active) {
            int pos = navigate_to_next_match(editor.search);
            if (pos >= 0) {
                editor.pending_navigate_to_pos = pos;
            }
        }

        if (input.search_prev && editor.search.active) {
            int pos = navigate_to_prev_match(editor.search);
            if (pos >= 0) {
                editor.pending_navigate_to_pos = pos;
            }
        }

        handle_sidebar_keyboard(editor, input);
        handle_cursor_blink(editor, input.had_input);

        render_editor(app.ctx, editor, app.window_width, app.window_height, 
                      app.font, app.status_font);
        render_sidebar(app.ctx, editor, app.window_width, app.window_height, app.font, app.sidebar_font);

        process_pending_saves(editor, DEBOUNCE_DELAY);
        process_rename_save(editor, RENAME_DEBOUNCE_DELAY);

        Theme theme = get_theme(editor.dark_theme);
        SDL_SetRenderDrawColor(app.renderer, theme.background.r, theme.background.g, theme.background.b, 255);
        SDL_RenderClear(app.renderer);
        nk_sdl_render(NK_ANTI_ALIASING_ON);
        SDL_RenderPresent(app.renderer);

        SDL_Delay(1000/ FRAME_RATE);
    }

    flush_pending_saves(editor);
    shutdown_app(app);

    return 0;
}
