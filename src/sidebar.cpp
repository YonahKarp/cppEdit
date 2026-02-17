#include "sidebar.h"
#include "editor.h"
#include "input.h"
#include "icons.h"
#include <cstdio>
#include <cstring>
#include <fstream>
#include <dirent.h>
#include <algorithm>
#include <sys/stat.h>
#include <ctime>

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#include "nuklear.h"

#include "nuklear_sdl_renderer.h"
#include "theme.h"

void scan_txt_files(EditorState& state) {
    state.sidebar.file_list.clear();
    DIR* dir = opendir(state.user_files_dir.c_str());
    if (!dir) return;

    struct dirent* entry;
    std::vector<std::pair<std::string, time_t>> files_with_times;
    
    while ((entry = readdir(dir)) != nullptr) {
        std::string name = entry->d_name;
        if (name.size() > 4 && name.substr(name.size() - 4) == ".txt") {
            std::string full_path = state.user_files_dir + "/" + name;
            struct stat file_stat;
            time_t mod_time = 0;
            if (stat(full_path.c_str(), &file_stat) == 0) {
                mod_time = file_stat.st_mtime;
            }
            files_with_times.push_back({name, mod_time});
        }
    }
    closedir(dir);

    std::sort(files_with_times.begin(), files_with_times.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });
    
    for (const auto& file : files_with_times) {
        state.sidebar.file_list.push_back(file.first);
    }

    state.sidebar.selected_index = 0;
    state.sidebar.new_file_selected = false;

    std::string current_filename;
    size_t last_slash = state.current_file_path.find_last_of('/');
    if (last_slash != std::string::npos) {
        current_filename = state.current_file_path.substr(last_slash + 1);
    } else {
        current_filename = state.current_file_path;
    }

    for (size_t i = 0; i < state.sidebar.file_list.size(); ++i) {
        if (state.sidebar.file_list[i] == current_filename) {
            state.sidebar.selected_index = static_cast<int>(i);
            break;
        }
    }
    
    state.sidebar.filtered_file_list = state.sidebar.file_list;
}

void toggle_sidebar(EditorState& state) {
    if (state.sidebar.visible) {
        state.sidebar.anim_closing = true;
        state.sidebar.anim_progress = 0.0f;
        state.sidebar.anim_start_time = SDL_GetTicks();
    } else {
        state.sidebar.visible = true;
        state.sidebar.anim_closing = false;
        scan_txt_files(state);
        state.sidebar.renaming = false;
        state.sidebar.rename_pending = false;
        state.sidebar.searching = false;
        state.sidebar.search_buffer[0] = '\0';
        state.sidebar.search_len = 0;
        state.sidebar.scroll_offset = 0;
        state.sidebar.anim_progress = 0.0f;
        state.sidebar.anim_start_time = SDL_GetTicks();
    }
}

void switch_to_file(EditorState& state, const std::string& new_file_path) {
    if (state.content_pending_save) {
        safe_save(state);
        state.content_pending_save = false;
    }
    if (state.state_pending_save) {
        save_state(state, state.pending_cursor_pos, state.pending_scroll_y);
        state.state_pending_save = false;
    }

    state.current_file_path = new_file_path;
    state.temp_file_path = new_file_path + ".tmp";

    std::ifstream infile(new_file_path);
    if (infile) {
        std::string content((std::istreambuf_iterator<char>(infile)),
                            std::istreambuf_iterator<char>());
        infile.close();
        size_t copy_len = content.size() < sizeof(state.text_buffer) - 1
                              ? content.size()
                              : sizeof(state.text_buffer) - 1;
        std::memcpy(state.text_buffer, content.c_str(), copy_len);
        state.text_buffer[copy_len] = '\0';
        state.text_len = static_cast<int>(copy_len);
    } else {
        state.text_buffer[0] = '\0';
        state.text_len = 0;
    }

    state.prev_text_len = state.text_len;
    std::memcpy(state.prev_text_buffer, state.text_buffer, state.text_len);
    state.cached_word_count = count_words(state.text_buffer, state.text_len);

    state.saved_cursor_pos = 0;
    state.saved_scroll_y = 0;
    state.last_cursor_pos = 0;
    state.last_scroll_y = 0;

    state.first_frame = true;
    state.second_frame = false;
}

void create_new_file(EditorState& state, const std::string& filename) {
    std::string full_filename = filename;
    if (full_filename.size() < 4 || full_filename.substr(full_filename.size() - 4) != ".txt") {
        full_filename += ".txt";
    }

    std::string new_path = state.user_files_dir + "/" + full_filename;

    std::ofstream outfile(new_path);
    outfile.close();

    switch_to_file(state, new_path);
}

void create_new_file_auto(EditorState& state) {
    int counter = 1;
    std::string filename;
    std::string new_path;

    while (true) {
        filename = "file " + std::to_string(counter) + ".txt";
        new_path = state.user_files_dir + "/" + filename;

        std::ifstream test(new_path);
        if (!test.good()) {
            break;
        }
        test.close();
        counter++;
    }

    std::ofstream outfile(new_path);
    outfile.close();

    switch_to_file(state, new_path);
}

void delete_file(EditorState& state, int file_index) {
    if (file_index < 0 || file_index >= (int)state.sidebar.file_list.size()) {
        return;
    }

    std::string filename = state.sidebar.file_list[file_index];
    std::string file_path = state.user_files_dir + "/" + filename;

    std::string previous_path = state.sidebar.previous_file_path;

    std::remove(file_path.c_str());

    scan_txt_files(state);

    if (state.sidebar.file_list.empty()) {
        create_new_file_auto(state);
        scan_txt_files(state);
    } else {
        std::string previous_filename;
        size_t last_slash = previous_path.find_last_of('/');
        if (last_slash != std::string::npos) {
            previous_filename = previous_path.substr(last_slash + 1);
        } else {
            previous_filename = previous_path;
        }

        bool found_previous = false;
        for (size_t i = 0; i < state.sidebar.file_list.size(); ++i) {
            if (state.sidebar.file_list[i] == previous_filename) {
                state.sidebar.selected_index = static_cast<int>(i);
                switch_to_file(state, previous_path);
                found_previous = true;
                break;
            }
        }

        if (!found_previous) {
            state.sidebar.selected_index = 0;
            std::string first_file = state.sidebar.file_list[0];
            std::string first_path = state.user_files_dir + "/" + first_file;
            switch_to_file(state, first_path);
        }
    }

    state.sidebar.previous_file_path.clear();
}

bool render_sidebar(struct nk_context* ctx, EditorState& state,
                    int window_width, int window_height, struct nk_font* font,
                    struct nk_font* sidebar_font, struct nk_font* small_font) {
    if (!state.sidebar.visible) return false;

    (void)font;
    
    const Theme& theme = get_theme(state.dark_theme);

    const int sidebar_width = 250;
    const int item_height = 60;

    if (state.sidebar.anim_progress < 1.0f) {
        Uint32 elapsed = SDL_GetTicks() - state.sidebar.anim_start_time;
        state.sidebar.anim_progress = (float)elapsed / (float)SidebarState::ANIM_DURATION_MS;
        if (state.sidebar.anim_progress > 1.0f) {
            state.sidebar.anim_progress = 1.0f;
        }
    }

    if (state.sidebar.anim_closing && state.sidebar.anim_progress >= 1.0f) {
        state.sidebar.visible = false;
        state.sidebar.anim_closing = false;
        return false;
    }

    float t = state.sidebar.anim_closing ? (1.0f - state.sidebar.anim_progress) : state.sidebar.anim_progress;
    int sidebar_x = (int)((t - 1.0f) * sidebar_width);

    static struct nk_style_button style;
    static struct nk_style_button selected_style;
    static struct nk_style_button new_file_style;
    static struct nk_style_button new_file_selected_style;
    static struct nk_style_button delete_style;
    static struct nk_style_button cancel_style;
    static struct nk_style_button delete_selected_style;
    static struct nk_style_button cancel_selected_style;
    static bool sidebar_styles_initialized = false;
    
    if (theme_changed(state.dark_theme) || !sidebar_styles_initialized) {
        sidebar_styles_initialized = true;
        
        style = ctx->style.button;
        style.normal = nk_style_item_color(theme.sidebar_btn_normal);
        style.hover = nk_style_item_color(theme.sidebar_btn_hover);
        style.active = nk_style_item_color(theme.sidebar_btn_active);
        style.text_normal = theme.sidebar_btn_text;
        style.text_hover = theme.sidebar_btn_text_hover;
        style.text_active = theme.sidebar_btn_text_hover;
        style.border = 0;
        style.rounding = 0;
        style.text_alignment = NK_TEXT_LEFT;
        style.padding = nk_vec2(10, 0);

        selected_style = style;
        selected_style.normal = nk_style_item_color(theme.sidebar_selected_normal);
        selected_style.hover = nk_style_item_color(theme.sidebar_selected_hover);
        selected_style.active = nk_style_item_color(theme.sidebar_selected_active);

        new_file_style = style;
        new_file_style.normal = nk_style_item_color(theme.sidebar_new_file_normal);
        new_file_style.hover = nk_style_item_color(theme.sidebar_new_file_hover);
        new_file_style.active = nk_style_item_color(theme.sidebar_new_file_active);

        new_file_selected_style = new_file_style;
        new_file_selected_style.normal = nk_style_item_color(theme.sidebar_new_file_selected_normal);
        new_file_selected_style.hover = nk_style_item_color(theme.sidebar_new_file_selected_hover);
        new_file_selected_style.active = nk_style_item_color(theme.sidebar_new_file_selected_active);

        delete_style = style;
        delete_style.normal = nk_style_item_color(theme.sidebar_delete_normal);
        delete_style.hover = nk_style_item_color(theme.sidebar_delete_hover);
        delete_style.active = nk_style_item_color(theme.sidebar_delete_active);
        delete_style.text_alignment = NK_TEXT_CENTERED;

        cancel_style = style;
        cancel_style.normal = nk_style_item_color(theme.sidebar_cancel_normal);
        cancel_style.hover = nk_style_item_color(theme.sidebar_cancel_hover);
        cancel_style.active = nk_style_item_color(theme.sidebar_cancel_active);
        cancel_style.text_alignment = NK_TEXT_CENTERED;

        delete_selected_style = delete_style;
        delete_selected_style.normal = nk_style_item_color(nk_rgb(160, 80, 80));
        delete_selected_style.hover = nk_style_item_color(nk_rgb(180, 90, 90));
        delete_selected_style.border_color = nk_rgb(255, 255, 255);
        delete_selected_style.border = 2;

        cancel_selected_style = cancel_style;
        cancel_selected_style.normal = nk_style_item_color(nk_rgb(110, 110, 110));
        cancel_selected_style.hover = nk_style_item_color(nk_rgb(130, 130, 130));
        cancel_selected_style.border_color = nk_rgb(255, 255, 255);
        cancel_selected_style.border = 2;
    }

    if (state.sidebar.confirm_delete) {
        const int dialog_width = 280;
        const int dialog_height = 120;
        int dialog_x = (window_width - dialog_width) / 2;
        int dialog_y = (window_height - dialog_height) / 2;

        if (nk_begin(ctx, "Delete Confirmation",
                     nk_rect(dialog_x, dialog_y, dialog_width, dialog_height),
                     NK_WINDOW_BORDER | NK_WINDOW_NO_SCROLLBAR)) {

            nk_style_push_font(ctx, &sidebar_font->handle);
            nk_layout_row_dynamic(ctx, 30, 1);
            std::string filename = state.sidebar.file_list[state.sidebar.delete_index];
            if (filename.size() > 4 && filename.substr(filename.size() - 4) == ".txt") {
                filename = filename.substr(0, filename.size() - 4);
            }
            std::string msg = "Delete file: " + filename + " ?";
            nk_label(ctx, msg.c_str(), NK_TEXT_CENTERED);

            nk_layout_row_dynamic(ctx, 10, 1);
            nk_spacing(ctx, 1);

            nk_layout_row_dynamic(ctx, 35, 2);

            struct nk_style_button* del_btn = (state.sidebar.dialog_selection == 0)
                                                  ? &delete_selected_style
                                                  : &delete_style;
            struct nk_style_button* cancel_btn = (state.sidebar.dialog_selection == 1)
                                                     ? &cancel_selected_style
                                                     : &cancel_style;

            if (nk_button_label_styled(ctx, del_btn, "Delete")) {
                delete_file(state, state.sidebar.delete_index);
                state.sidebar.confirm_delete = false;
                state.sidebar.delete_index = -1;
            }
            if (nk_button_label_styled(ctx, cancel_btn, "Cancel")) {
                if (!state.sidebar.previous_file_path.empty() &&
                    state.sidebar.previous_file_path != state.current_file_path) {
                    switch_to_file(state, state.sidebar.previous_file_path);
                }
                state.sidebar.confirm_delete = false;
                state.sidebar.delete_index = -1;
                state.sidebar.previous_file_path.clear();
            }
            nk_style_pop_font(ctx);
        }
        nk_end(ctx);
        return true;
    }

    if (nk_begin(ctx, "Sidebar", nk_rect(sidebar_x, 0, sidebar_width, window_height),
                 NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_BORDER)) {

        const int vertical_padding = 6;
        const int search_height = 35;
        nk_layout_row_dynamic(ctx, vertical_padding, 1);
        nk_spacing(ctx, 1);

        if (state.sidebar.searching) {
            nk_layout_row_dynamic(ctx, search_height, 1);
            
            struct nk_rect search_bounds;
            nk_widget(&search_bounds, ctx);
            struct nk_command_buffer* canvas = nk_window_get_canvas(ctx);
            
            nk_fill_rect(canvas, search_bounds, 6, theme.sidebar_btn_normal);
            
            float icon_size = 14;
            float icon_x = search_bounds.x + 10;
            float icon_y = search_bounds.y + (search_bounds.h - icon_size) / 2;
            draw_magnifying_glass_icon(canvas, icon_x, icon_y, icon_size, theme.sidebar_btn_text);
            
            float text_x = search_bounds.x + 10 + icon_size + 6;
            float text_y = search_bounds.y + (search_bounds.h - sidebar_font->handle.height) / 2;
            
            std::string display_text = state.sidebar.search_len > 0 
                ? std::string(state.sidebar.search_buffer, state.sidebar.search_len)
                : "Search...";
            
            nk_draw_text(canvas, nk_rect(text_x, text_y, search_bounds.w - icon_size - 20, sidebar_font->handle.height),
                        display_text.c_str(), display_text.length(), &sidebar_font->handle,
                        theme.sidebar_btn_normal, theme.sidebar_btn_text);
        nk_layout_row_dynamic(ctx, 6, 1);
        nk_spacing(ctx, 1);
        } else {
            nk_layout_row_dynamic(ctx, item_height, 1);
            nk_style_push_font(ctx, &sidebar_font->handle);
            struct nk_style_button* btn_style = state.sidebar.new_file_selected
                                                    ? &new_file_selected_style
                                                    : &new_file_style;
            if (nk_button_label_styled(ctx, btn_style, "+ New File")) {
                state.sidebar.new_file_selected = true;
                state.sidebar.selected_index = -1;
            }
            nk_style_pop_font(ctx);
        }

        nk_layout_row_dynamic(ctx, vertical_padding / 2, 1);
        nk_spacing(ctx, 1);

        int file_list_height = window_height - item_height - vertical_padding * 3 - 20;
        nk_layout_row_dynamic(ctx, file_list_height, 1);
        
        state.sidebar.item_height = item_height;
        
        ctx->style.window.group_padding = nk_vec2(0, 0);
        ctx->style.scrollv.normal = nk_style_item_color(theme.scrollbar_bg);
        ctx->style.scrollv.hover = nk_style_item_color(theme.scrollbar_bg);
        ctx->style.scrollv.active = nk_style_item_color(theme.scrollbar_bg);
        ctx->style.scrollv.cursor_normal = nk_style_item_color(theme.scrollbar_cursor);
        ctx->style.scrollv.cursor_hover = nk_style_item_color(theme.scrollbar_cursor_hover);
        ctx->style.scrollv.cursor_active = nk_style_item_color(theme.scrollbar_cursor_hover);
        ctx->style.scrollv.border = 0;
        ctx->style.scrollv.rounding = 4;
        ctx->style.scrollv.rounding_cursor = 4;
        
        nk_uint x_offset = 0;
        nk_uint y_offset = state.sidebar.scroll_offset;
        if (nk_group_scrolled_offset_begin(ctx, &x_offset, &y_offset, "FileList", 0)) {
            state.sidebar.visible_height = file_list_height;
            
            nk_style_push_font(ctx, &sidebar_font->handle);
            
            const std::vector<std::string>& display_list = state.sidebar.searching 
                ? state.sidebar.filtered_file_list 
                : state.sidebar.file_list;
            
            for (size_t i = 0; i < display_list.size(); ++i) {
                nk_layout_row_dynamic(ctx, item_height, 1);

                bool is_selected = !state.sidebar.new_file_selected &&
                                   (static_cast<int>(i) == state.sidebar.selected_index);
                struct nk_style_button* file_btn_style = is_selected ? &selected_style : &style;

                std::string display_name;
                if (!state.sidebar.searching && state.sidebar.renaming && static_cast<int>(i) == state.sidebar.rename_file_index) {
                    display_name = std::string(state.sidebar.rename_buffer, state.sidebar.rename_len);
                } else {
                    display_name = display_list[i];
                    if (display_name.size() > 4 && display_name.substr(display_name.size() - 4) == ".txt") {
                        display_name = display_name.substr(0, display_name.size() - 4);
                    }
                }
                
                std::string file_path = state.user_files_dir + "/" + display_list[i];
                
                struct stat file_stat;
                std::string date_str;
                if (stat(file_path.c_str(), &file_stat) == 0) {
                    time_t mod_time = file_stat.st_mtime;
                    struct tm* time_info = localtime(&mod_time);
                    char date_buf[32];
                    strftime(date_buf, sizeof(date_buf), "%m/%d/%y", time_info);
                    date_str = date_buf;
                }
                
                std::string snippet;
                std::ifstream file(file_path);
                if (file) {
                    std::string first_line;
                    std::getline(file, first_line);
                    file.close();
                    snippet = first_line;
                }
                
                struct nk_rect item_bounds;
                nk_widget(&item_bounds, ctx);
                struct nk_command_buffer* canvas = nk_window_get_canvas(ctx);
                
                struct nk_color bg_color = is_selected ? theme.sidebar_selected_normal : theme.sidebar_btn_normal;
                struct nk_input* input_ctx = &ctx->input;
                bool hovered = nk_input_is_mouse_hovering_rect(input_ctx, item_bounds);
                if (hovered && !is_selected) {
                    bg_color = theme.sidebar_btn_hover;
                }
                nk_fill_rect(canvas, item_bounds, 0, bg_color);
                
                float title_y = item_bounds.y + 8;
                nk_draw_text(canvas, nk_rect(item_bounds.x + 10, title_y, item_bounds.w - 20, sidebar_font->handle.height),
                            display_name.c_str(), display_name.length(), &sidebar_font->handle,
                            bg_color, theme.sidebar_btn_text);
                
                float subtitle_y = title_y + sidebar_font->handle.height + 4;
                struct nk_color subtitle_color = theme.sidebar_btn_text;
                subtitle_color.a = 140;
                
                float content_width = item_bounds.w - 20;
                float date_width = 65;
                float snippet_area_width = content_width - date_width - 10;
                
                nk_draw_text(canvas, nk_rect(item_bounds.x + 10, subtitle_y, date_width, small_font->handle.height),
                            date_str.c_str(), date_str.length(), &small_font->handle,
                            bg_color, subtitle_color);
                
                if (!snippet.empty()) {
                    float snippet_text_width = small_font->handle.width(small_font->handle.userdata, small_font->handle.height, snippet.c_str(), snippet.length());
                    float snippet_area_x = item_bounds.x + 10 + date_width + 10;
                    
                    std::string display_snippet = snippet;
                    if (snippet_text_width > snippet_area_width) {
                        std::string ellipsis = "...";
                        float ellipsis_width = small_font->handle.width(small_font->handle.userdata, small_font->handle.height, ellipsis.c_str(), ellipsis.length());
                        float available_width = snippet_area_width - ellipsis_width;
                        
                        size_t fit_chars = 0;
                        float current_width = 0;
                        for (size_t j = 0; j < snippet.length(); ++j) {
                            float char_width = small_font->handle.width(small_font->handle.userdata, small_font->handle.height, snippet.c_str() + j, 1);
                            if (current_width + char_width > available_width) break;
                            current_width += char_width;
                            fit_chars = j + 1;
                        }
                        display_snippet = snippet.substr(0, fit_chars) + ellipsis;
                        snippet_text_width = small_font->handle.width(small_font->handle.userdata, small_font->handle.height, display_snippet.c_str(), display_snippet.length());
                    }
                    
                    float snippet_x = snippet_area_x + snippet_area_width - snippet_text_width;
                    if (snippet_x < snippet_area_x) snippet_x = snippet_area_x;
                    nk_draw_text(canvas, nk_rect(snippet_x, subtitle_y, snippet_text_width, small_font->handle.height),
                                display_snippet.c_str(), display_snippet.length(), &small_font->handle,
                                bg_color, subtitle_color);
                }
                
                if (hovered && nk_input_is_mouse_pressed(input_ctx, NK_BUTTON_LEFT)) {
                    if (!state.sidebar.searching && state.sidebar.renaming && state.sidebar.rename_file_index != static_cast<int>(i)) {
                        state.sidebar.renaming = false;
                        state.sidebar.rename_pending = false;
                    }
                    state.sidebar.selected_index = static_cast<int>(i);
                    state.sidebar.new_file_selected = false;
                }
            }
            
            nk_style_pop_font(ctx);
            nk_group_scrolled_end(ctx);
            state.sidebar.scroll_offset = y_offset;
        }
    }
    nk_end(ctx);

    return true;
}

void handle_sidebar_input(EditorState& state, SDL_Event* event) {
    if (!state.sidebar.visible || state.sidebar.confirm_delete) return;
    if (state.sidebar.new_file_selected) return;
    if (state.sidebar.selected_index < 0 || 
        state.sidebar.selected_index >= (int)state.sidebar.file_list.size()) return;

    if (event->type == SDL_TEXTINPUT) {
        char c = event->text.text[0];
        if (c >= 32 && c < 127) {
            if (!state.sidebar.renaming) {
                state.sidebar.renaming = true;
                state.sidebar.rename_buffer[0] = c;
                state.sidebar.rename_buffer[1] = '\0';
                state.sidebar.rename_len = 1;
                state.sidebar.rename_file_index = state.sidebar.selected_index;
            } else {
                if (state.sidebar.rename_len < (int)sizeof(state.sidebar.rename_buffer) - 1) {
                    state.sidebar.rename_buffer[state.sidebar.rename_len] = c;
                    state.sidebar.rename_len++;
                    state.sidebar.rename_buffer[state.sidebar.rename_len] = '\0';
                }
            }
            state.sidebar.rename_pending = true;
            state.sidebar.rename_change_time = SDL_GetTicks();
        }
    } else if (event->type == SDL_KEYDOWN && state.sidebar.renaming) {
        if (event->key.keysym.sym == SDLK_BACKSPACE) {
            if (state.sidebar.rename_len > 0) {
                state.sidebar.rename_len--;
                state.sidebar.rename_buffer[state.sidebar.rename_len] = '\0';
                state.sidebar.rename_pending = true;
                state.sidebar.rename_change_time = SDL_GetTicks();
            }
            if (state.sidebar.rename_len == 0) {
                state.sidebar.renaming = false;
                state.sidebar.rename_pending = false;
            }
        }
    }
}

void handle_sidebar_keyboard(EditorState& state, const InputState& input) {
    if (!state.sidebar.visible) return;

    if (state.sidebar.confirm_delete) {
        if (input.escape) {
            if (!state.sidebar.previous_file_path.empty() &&
                state.sidebar.previous_file_path != state.current_file_path) {
                switch_to_file(state, state.sidebar.previous_file_path);
            }
            state.sidebar.confirm_delete = false;
            state.sidebar.delete_index = -1;
            state.sidebar.previous_file_path.clear();
        }
        if (input.left) {
            state.sidebar.dialog_selection = 0;
        }
        if (input.right) {
            state.sidebar.dialog_selection = 1;
        }
        if (input.enter) {
            if (state.sidebar.dialog_selection == 0) {
                delete_file(state, state.sidebar.delete_index);
            } else {
                if (!state.sidebar.previous_file_path.empty() &&
                    state.sidebar.previous_file_path != state.current_file_path) {
                    switch_to_file(state, state.sidebar.previous_file_path);
                }
                state.sidebar.previous_file_path.clear();
            }
            state.sidebar.confirm_delete = false;
            state.sidebar.delete_index = -1;
        }
        return;
    }

    if (input.up) {
        if (state.sidebar.renaming) {
            state.sidebar.renaming = false;
            state.sidebar.rename_pending = false;
        }
        if (state.sidebar.new_file_selected) {
            // Already at top, do nothing
        } else if (state.sidebar.selected_index == 0) {
            state.sidebar.new_file_selected = true;
            state.sidebar.scroll_offset = 0;
        } else if (state.sidebar.selected_index > 0) {
            state.sidebar.selected_index--;
            int row_height = state.sidebar.item_height + 4; // item + spacing
            int item_top = state.sidebar.selected_index * row_height;
            if (item_top < (int)state.sidebar.scroll_offset) {
                state.sidebar.scroll_offset = item_top;
            }
        }
    }

    if (input.down) {
        if (state.sidebar.renaming) {
            state.sidebar.renaming = false;
            state.sidebar.rename_pending = false;
        }
        
        const std::vector<std::string>& display_list = state.sidebar.searching 
            ? state.sidebar.filtered_file_list 
            : state.sidebar.file_list;
            
        if (state.sidebar.new_file_selected) {
            state.sidebar.new_file_selected = false;
            state.sidebar.selected_index = 0;
            state.sidebar.scroll_offset = 0;
        } else if (state.sidebar.selected_index < (int)display_list.size() - 1) {
            state.sidebar.selected_index++;
            int row_height = state.sidebar.item_height + 4; // item + spacing
            int item_bottom = (state.sidebar.selected_index + 1) * row_height;
            int visible_bottom = state.sidebar.scroll_offset + state.sidebar.visible_height;
            if (item_bottom > visible_bottom) {
                state.sidebar.scroll_offset = item_bottom - state.sidebar.visible_height + 8;
            }
        }
    }

    if (input.escape) {
        if (state.sidebar.renaming) {
            state.sidebar.renaming = false;
            state.sidebar.rename_pending = false;
        } else {
            state.sidebar.visible = false;
        }
    }

    if (input.delete_key && !state.sidebar.new_file_selected) {
        state.sidebar.confirm_delete = true;
        state.sidebar.delete_index = state.sidebar.selected_index;
        state.sidebar.dialog_selection = 1;
        state.sidebar.previous_file_path = state.current_file_path;

        std::string file_to_delete = state.sidebar.file_list[state.sidebar.selected_index];
        std::string delete_path = state.user_files_dir + "/" + file_to_delete;
        if (delete_path != state.current_file_path) {
            switch_to_file(state, delete_path);
        }
    }

    if (input.enter) {
        if (state.sidebar.renaming) {
            state.sidebar.renaming = false;
            state.sidebar.rename_pending = false;
        }
        if (state.sidebar.new_file_selected) {
            create_new_file_auto(state);
            state.sidebar.visible = false;
        } else {
            const std::vector<std::string>& display_list = state.sidebar.searching 
                ? state.sidebar.filtered_file_list 
                : state.sidebar.file_list;
                
            if (!display_list.empty() && state.sidebar.selected_index < (int)display_list.size()) {
                std::string selected_file = display_list[state.sidebar.selected_index];
                std::string new_path = state.user_files_dir + "/" + selected_file;

                std::string current_filename;
                size_t last_slash = state.current_file_path.find_last_of('/');
                if (last_slash != std::string::npos) {
                    current_filename = state.current_file_path.substr(last_slash + 1);
                } else {
                    current_filename = state.current_file_path;
                }

                if (selected_file == current_filename) {
                    state.sidebar.visible = false;
                } else {
                    switch_to_file(state, new_path);
                    state.sidebar.visible = false;
                }
            }
        }
    }
}

static bool case_insensitive_contains(const std::string& str, const std::string& substr) {
    if (substr.empty()) return true;
    std::string str_lower = str;
    std::string substr_lower = substr;
    for (auto& c : str_lower) c = std::tolower(c);
    for (auto& c : substr_lower) c = std::tolower(c);
    return str_lower.find(substr_lower) != std::string::npos;
}

void filter_sidebar_files(EditorState& state) {
    state.sidebar.filtered_file_list.clear();
    std::string query(state.sidebar.search_buffer, state.sidebar.search_len);
    
    for (const auto& filename : state.sidebar.file_list) {
        if (case_insensitive_contains(filename, query)) {
            state.sidebar.filtered_file_list.push_back(filename);
        }
    }
    
    state.sidebar.selected_index = 0;
    state.sidebar.new_file_selected = false;
    state.sidebar.scroll_offset = 0;
}

void toggle_sidebar_search(EditorState& state) {
    if (state.sidebar.searching) {
        state.sidebar.searching = false;
        state.sidebar.search_buffer[0] = '\0';
        state.sidebar.search_len = 0;
        filter_sidebar_files(state);
    } else {
        state.sidebar.searching = true;
        state.sidebar.search_buffer[0] = '\0';
        state.sidebar.search_len = 0;
        state.sidebar.renaming = false;
        state.sidebar.rename_pending = false;
        filter_sidebar_files(state);
    }
}

void process_rename_save(EditorState& state, Uint32 debounce_delay) {
    if (!state.sidebar.rename_pending) return;

    Uint32 now = SDL_GetTicks();
    if (now - state.sidebar.rename_change_time < debounce_delay) return;

    state.sidebar.rename_pending = false;

    if (!state.sidebar.renaming || state.sidebar.rename_len == 0) return;
    if (state.sidebar.rename_file_index < 0 || 
        state.sidebar.rename_file_index >= (int)state.sidebar.file_list.size()) return;

    std::string old_filename = state.sidebar.file_list[state.sidebar.rename_file_index];
    std::string old_path = state.user_files_dir + "/" + old_filename;

    std::string new_filename = std::string(state.sidebar.rename_buffer, state.sidebar.rename_len);
    if (new_filename.size() < 4 || new_filename.substr(new_filename.size() - 4) != ".txt") {
        new_filename += ".txt";
    }
    std::string new_path = state.user_files_dir + "/" + new_filename;

    if (old_path == new_path) return;

    std::ifstream test(new_path);
    if (test.good()) {
        test.close();
        return;
    }

    std::rename(old_path.c_str(), new_path.c_str());

    bool was_current_file = (state.current_file_path == old_path);

    state.sidebar.file_list[state.sidebar.rename_file_index] = new_filename;

    if (was_current_file) {
        state.current_file_path = new_path;
        state.temp_file_path = new_path + ".tmp";
    }
}
