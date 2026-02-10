#include "editor.h"
#include "search.h"
#include "icons.h"
#include <cstdio>
#include <cstring>
#include <fstream>
#include <cmath>

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#include "nuklear.h"

#include "nuklear_sdl_renderer.h"
#include "theme.h"

static const Uint32 BLINK_INTERVAL = 530;

static inline uint64_t fnv1a_hash(const char* data, int len) {
    uint64_t hash = 14695981039346656037ULL;
    for (int i = 0; i < len; i++) {
        hash ^= static_cast<uint64_t>(static_cast<unsigned char>(data[i]));
        hash *= 1099511628211ULL;
    }
    return hash;
}

constexpr int HASH_CHANGE_DETECTION_THRESHOLD = 4096;

int count_words(const char* text, int text_len) {
    int count = 0;
    bool in_word = false;
    for (int i = 0; i < text_len; i++) {
        char c = text[i];
        bool is_whitespace = (c == ' ' || c == '\n' || c == '\t' || c == '\r');
        if (is_whitespace) {
            in_word = false;
        } else if (!in_word) {
            in_word = true;
            count++;
        }
    }
    return count;
}

void init_editor_state(EditorState& state, const char* file_path, const char* state_file_path,
                       int saved_cursor_pos, float saved_scroll_y) {
    state.current_file_path = file_path;
    state.temp_file_path = std::string(file_path) + ".tmp";
    state.state_file_path = state_file_path;
    state.saved_cursor_pos = saved_cursor_pos;
    state.saved_scroll_y = saved_scroll_y;
    state.last_cursor_pos = saved_cursor_pos;
    state.last_scroll_y = saved_scroll_y;
    state.last_blink_time = SDL_GetTicks();

    std::ifstream infile(file_path);
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
    }

    state.prev_text_len = state.text_len;
    std::memcpy(state.prev_text_buffer, state.text_buffer, state.text_len);
    if (state.text_len >= HASH_CHANGE_DETECTION_THRESHOLD) {
        state.prev_text_hash = fnv1a_hash(state.text_buffer, state.text_len);
    }
    state.cached_word_count = count_words(state.text_buffer, state.text_len);
}

void safe_save(EditorState& state) {
    std::ofstream outfile(state.temp_file_path, std::ios::binary | std::ios::trunc);
    if (outfile) {
        outfile.write(state.text_buffer, state.text_len);
        outfile.close();
        if (outfile.good()) {
            std::rename(state.temp_file_path.c_str(), state.current_file_path.c_str());
        }
    }
}

void save_state(EditorState& state, int cursor_pos, float scroll_y) {
    std::string temp_state_path = std::string(state.state_file_path) + ".tmp";
    std::ofstream state_out(temp_state_path);
    if (state_out) {
        state_out << state.current_file_path << "\n";
        state_out << cursor_pos << " " << scroll_y << "\n";
        state_out << state.font_size << "\n";
        state_out << (state.dark_theme ? 1 : 0) << "\n";
        state_out.close();
        if (state_out.good()) {
            std::rename(temp_state_path.c_str(), state.state_file_path);
        }
    }
}

void handle_cursor_blink(EditorState& state, bool had_input) {
    if (had_input) {
        state.cursor_visible = true;
        state.last_blink_time = SDL_GetTicks();
    }

    Uint32 current_time = SDL_GetTicks();
    if (current_time - state.last_blink_time >= BLINK_INTERVAL) {
        state.cursor_visible = !state.cursor_visible;
        state.last_blink_time = current_time;
    }
}

void render_editor(struct nk_context* ctx, EditorState& state,
                   int window_width, int window_height, struct nk_font* font,
                   struct nk_font* status_font) {
    const Theme& theme = get_theme(state.dark_theme);
    
    struct nk_color edit_bg = theme.edit_bg;
    struct nk_color cursor_on = theme.cursor;
    bool show_cursor = state.cursor_visible && !state.sidebar.confirm_delete && !state.search.active;
    struct nk_color cursor_color = show_cursor ? cursor_on : edit_bg;
    ctx->style.edit.cursor_normal = cursor_color;
    ctx->style.edit.cursor_hover = cursor_color;
    ctx->style.edit.cursor_text_normal = edit_bg;
    ctx->style.edit.cursor_text_hover = edit_bg;
    
    static bool editor_styles_initialized = false;
    if (theme_changed(state.dark_theme) || !editor_styles_initialized) {
        editor_styles_initialized = true;
        ctx->style.edit.cursor_size = 1;
        
        ctx->style.edit.normal = nk_style_item_color(edit_bg);
        ctx->style.edit.hover = nk_style_item_color(edit_bg);
        ctx->style.edit.active = nk_style_item_color(edit_bg);
        ctx->style.edit.text_normal = theme.edit_text;
        ctx->style.edit.text_hover = theme.edit_text;
        ctx->style.edit.text_active = theme.edit_text;
        ctx->style.edit.selected_normal = theme.selection;
        ctx->style.edit.selected_hover = theme.selection;
        ctx->style.edit.selected_text_normal = theme.edit_text;
        ctx->style.edit.selected_text_hover = theme.edit_text;
        
        ctx->style.edit.scrollbar.normal = nk_style_item_color(theme.scrollbar_bg);
        ctx->style.edit.scrollbar.hover = nk_style_item_color(theme.scrollbar_bg);
        ctx->style.edit.scrollbar.active = nk_style_item_color(theme.scrollbar_bg);
        ctx->style.edit.scrollbar.cursor_normal = nk_style_item_color(theme.scrollbar_cursor);
        ctx->style.edit.scrollbar.cursor_hover = nk_style_item_color(theme.scrollbar_cursor_hover);
        ctx->style.edit.scrollbar.cursor_active = nk_style_item_color(theme.scrollbar_cursor_hover);
        
        ctx->style.window.fixed_background = nk_style_item_color(theme.background);
    }


    const int status_bar_height = 30;

    nk_flags editor_flags = NK_WINDOW_NO_SCROLLBAR;
    if (state.sidebar.visible) {
        // This introduces a bug where once the user clicks on the menu, they can no longer change file or input will stop working
        editor_flags |= NK_WINDOW_BACKGROUND;
    }

    if (nk_begin(ctx, "Editor", nk_rect(10, 10, window_width - 20, window_height - 20),
                 editor_flags)) {

        nk_style_push_font(ctx, &status_font->handle);
        struct nk_color saved_text_color = ctx->style.text.color;
        ctx->style.text.color = theme.text_dim;

        int num_columns = state.search.active ? 4 : 3;
        nk_layout_row_begin(ctx, NK_DYNAMIC, status_bar_height, num_columns);

        float filename_width = state.search.active ? 0.35f : 0.70f;
        nk_layout_row_push(ctx, filename_width);
        std::string display_name;
        size_t last_slash = state.current_file_path.find_last_of('/');
        if (last_slash != std::string::npos) {
            display_name = state.current_file_path.substr(last_slash + 1);
        } else {
            display_name = state.current_file_path;
        }
        if (display_name.size() > 4 && display_name.substr(display_name.size() - 4) == ".txt") {
            display_name = display_name.substr(0, display_name.size() - 4);
        }
        nk_label(ctx, display_name.c_str(), NK_TEXT_LEFT);

        if (!state.search.active) {
            nk_layout_row_push(ctx, 0.15f);
            char word_count_str[32];
            snprintf(word_count_str, sizeof(word_count_str), "%d words", state.cached_word_count);
            nk_label(ctx, word_count_str, NK_TEXT_RIGHT);
        }

        if (state.search.active) {
            nk_layout_row_push(ctx, 0.40f);
            struct nk_rect search_bounds;
            nk_widget(&search_bounds, ctx);
            struct nk_command_buffer* canvas = nk_window_get_canvas(ctx);
            nk_fill_rect(canvas, search_bounds, 3, theme.search_input_bg);
            
            float icon_size = 14;
            float icon_x = search_bounds.x + 8;
            float icon_y = search_bounds.y + (search_bounds.h - icon_size) / 2;
            draw_magnifying_glass_icon(canvas, icon_x, icon_y, icon_size, theme.edit_text);
            
            float text_x = search_bounds.x + 8 + icon_size + 4;
            float text_y = search_bounds.y + (search_bounds.h - status_font->handle.height) / 2;
            
            const char* display_text = state.search.query_len > 0 ? state.search.query : "Find...";
            int display_len = state.search.query_len > 0 ? state.search.query_len : 7;
            
            nk_draw_text(canvas, nk_rect(text_x, text_y, search_bounds.w - icon_size - 16, status_font->handle.height),
                        display_text, display_len, &status_font->handle,
                        theme.search_input_bg, theme.search_input_text);
            
            nk_layout_row_push(ctx, 0.10f);
            char match_info[32];
            if (state.search.matches.empty()) {
                if (state.search.query_len > 0) {
                    snprintf(match_info, sizeof(match_info), "0/0");
                } else {
                    match_info[0] = '\0';
                }
            } else {
                snprintf(match_info, sizeof(match_info), "%d/%d",
                        state.search.current_match_index + 1,
                        (int)state.search.matches.size());
            }
            nk_label(ctx, match_info, NK_TEXT_LEFT);
        }

        nk_layout_row_push(ctx, 0.15f);
        struct nk_command_buffer* canvas = nk_window_get_canvas(ctx);
        struct nk_rect battery_bounds;
        nk_widget(&battery_bounds, ctx);

        const int batt_width = 30;
        const int batt_height = 14;
        const int batt_tip_width = 3;

        int batt_x = battery_bounds.x + battery_bounds.w - batt_width - batt_tip_width - 5;
        int batt_y = battery_bounds.y + (battery_bounds.h - batt_height) / 2;

        struct nk_color fill_color;
        if (state.battery_percent > 20) {
            fill_color = theme.battery_good;
        } else {
            fill_color = theme.battery_low;
        }

        draw_battery_icon(canvas, batt_x, batt_y, state.battery_percent, 
                          theme.battery_border, fill_color);

        nk_layout_row_end(ctx);

        ctx->style.text.color = saved_text_color;
        nk_style_pop_font(ctx);


        // here
        nk_layout_row_dynamic(ctx, window_height - font->handle.height - status_bar_height, 1);
        nk_flags edit_flags = NK_EDIT_ALWAYS_INSERT_MODE | NK_EDIT_SELECTABLE |
                              NK_EDIT_MULTILINE | NK_EDIT_ALLOW_TAB |
                              NK_EDIT_CLIPBOARD | NK_EDIT_NO_HORIZONTAL_SCROLL;
        if (state.first_frame) {
            nk_edit_focus(ctx, edit_flags);
        }
        nk_edit_string(ctx, edit_flags, state.text_buffer, &state.text_len,
                       sizeof(state.text_buffer) - 1, nk_filter_default);

        if (state.search.active && state.search.current_match_index >= 0 && 
            state.search.current_match_index < (int)state.search.matches.size()) {
            const SearchMatch& match = state.search.matches[state.search.current_match_index];
            
            struct nk_rect area = ctx->text_edit.area;
            float scroll_y = ctx->current->edit.scrollbar.y;
            float row_height = ctx->text_edit.row_height;
            float wrap_width = ctx->text_edit.wrap_width;
            
            struct nk_vec2 match_pos = nk_edit_text_pos(state.text_buffer, state.text_len,
                                                        match.start, &font->handle, 
                                                        row_height, wrap_width);
            
            float match_text_width = font->handle.width(font->handle.userdata, font->handle.height,
                                                        state.text_buffer + match.start, 
                                                        match.end - match.start);
            
            float highlight_x = area.x + match_pos.x;
            float highlight_y = area.y + match_pos.y - scroll_y;
            
            if (highlight_y >= area.y && highlight_y < area.y + area.h - row_height) {
                struct nk_command_buffer* edit_canvas = nk_window_get_canvas(ctx);
                struct nk_color highlight_color = nk_rgba(255, 255, 0, 80);
                nk_fill_rect(edit_canvas, nk_rect(highlight_x, highlight_y, match_text_width, row_height), 
                            0, highlight_color);
            }
        }

        if (state.pending_undo) {
            perform_undo(state, ctx);
            state.pending_undo = false;
            state.cached_word_count = count_words(state.text_buffer, state.text_len);
        }

        bool content_changed = false;
        if (state.text_len != state.prev_text_len) {
            content_changed = true;
        } else if (state.text_len > 0) {
            if (state.text_len >= HASH_CHANGE_DETECTION_THRESHOLD) {
                uint64_t current_hash = fnv1a_hash(state.text_buffer, state.text_len);
                content_changed = (current_hash != state.prev_text_hash);
            } else {
                content_changed = std::memcmp(state.text_buffer, state.prev_text_buffer, state.text_len) != 0;
            }
        }
        if (content_changed) {
            bool had_selection = (state.prev_sel_start != state.prev_sel_end);
            int chars_added = state.text_len - state.prev_text_len;
            bool is_paste = (chars_added > 1);
            bool is_large_delete = (chars_added < -1) || (had_selection && chars_added < 0);
            
            if (had_selection || is_paste || is_large_delete) {
                if (!state.undo_state.text_buffer) {
                    state.undo_state.text_buffer = new char[TEXT_BUFFER_SIZE];
                }
                std::memcpy(state.undo_state.text_buffer, state.prev_text_buffer, state.prev_text_len);
                state.undo_state.text_len = state.prev_text_len;
                state.undo_state.cursor_pos = state.last_cursor_pos >= 0 ? state.last_cursor_pos : 0;
                state.undo_state.sel_start = state.prev_sel_start;
                state.undo_state.sel_end = state.prev_sel_end;
            } else if (state.undo_state.text_buffer) {
                delete[] state.undo_state.text_buffer;
                state.undo_state.text_buffer = nullptr;
            }
            state.content_pending_save = true;
            state.content_change_time = SDL_GetTicks();
            
            bool should_recount = false;
            if (is_paste || is_large_delete) {
                should_recount = true;
            } else if (state.text_len > state.prev_text_len) {
                char c = state.last_typed_char;
                if (c == ' ' || c == '\n' || c == '\t' || c == '\r') {
                    should_recount = true;
                }
            } else if (state.text_len < state.prev_text_len) {
                should_recount = true;
            }
            
            if (should_recount) {
                state.cached_word_count = count_words(state.text_buffer, state.text_len);
            }
            
            state.prev_text_len = state.text_len;
            std::memcpy(state.prev_text_buffer, state.text_buffer, state.text_len);
            if (state.text_len >= HASH_CHANGE_DETECTION_THRESHOLD) {
                state.prev_text_hash = fnv1a_hash(state.text_buffer, state.text_len);
            }
            state.last_typed_char = 0;
        }

        // hack to get the cursor to the end of the text on the first frame
        if (state.first_frame) {
            if (state.saved_cursor_pos >= 0 &&
                state.saved_cursor_pos <= ctx->text_edit.string.len) {
                ctx->current->edit.cursor = state.saved_cursor_pos;
            } else {
                ctx->current->edit.cursor = ctx->text_edit.string.len;
            }
            if (state.saved_scroll_y >= 0) {
                ctx->current->edit.scrollbar.y = state.saved_scroll_y;
            } else {
                ctx->current->edit.scrollbar.y = 1000000;
            }
            state.first_frame = false;
            state.second_frame = true;

            // reset the cursor position again to ensure it stays at the end after the scroll adjustment
        } else if (state.second_frame) {
            if (state.saved_cursor_pos >= 0 &&
                state.saved_cursor_pos <= ctx->text_edit.string.len) {
                ctx->current->edit.cursor = state.saved_cursor_pos;
            } else {
                ctx->current->edit.cursor = ctx->text_edit.string.len;
            }
            state.second_frame = false;
        }

        int current_cursor = ctx->current->edit.cursor;
        float current_scroll = ctx->current->edit.scrollbar.y;

        if (state.pending_scroll_to_cursor) {
            ctx->current->edit.scrollbar.y = state.target_scroll_y;
            current_scroll = state.target_scroll_y;
            state.pending_scroll_to_cursor = false;
        }

        if (state.pending_paragraph_move != 0) {
            int direction = state.pending_paragraph_move;

            int selection_anchor = ctx->current->edit.sel_start;
            bool has_selection = (ctx->current->edit.sel_start != ctx->current->edit.sel_end);
            if (state.pending_paragraph_extend_selection && has_selection) {
                int sel_min = NK_MIN(ctx->current->edit.sel_start, ctx->current->edit.sel_end);
                int sel_max = NK_MAX(ctx->current->edit.sel_start, ctx->current->edit.sel_end);
                if (current_cursor == sel_min) {
                    selection_anchor = sel_max;
                } else {
                    selection_anchor = sel_min;
                }
            }

            int new_cursor;
            if (direction < 0) {
                new_cursor = find_prev_paragraph(state.text_buffer, state.text_len, current_cursor);
            } else {
                new_cursor = find_next_paragraph(state.text_buffer, state.text_len, current_cursor);
            }

            if (state.pending_paragraph_extend_selection) {
                ctx->current->edit.sel_start = selection_anchor;
                ctx->current->edit.sel_end = new_cursor;
            } else {
                ctx->current->edit.sel_start = new_cursor;
                ctx->current->edit.sel_end = new_cursor;
            }
            current_cursor = new_cursor;
            ctx->current->edit.cursor = current_cursor;

            float edit_padding = ctx->style.edit.padding.x;
            float edit_border = ctx->style.edit.border;
            float scrollbar_size = ctx->style.edit.scrollbar_size.x;
            float cursor_size = ctx->style.edit.cursor_size;
            struct nk_rect bounds = ctx->current->layout->clip;
            float area_w = bounds.w - (2.0f * edit_padding + 2 * edit_border) - scrollbar_size;
            float wrap_width = area_w - cursor_size;

            float row_padding = ctx->style.edit.row_padding;
            float row_height = font->handle.height + row_padding;
            float cursor_y = nk_edit_cursor_y(state.text_buffer, state.text_len, 
                                               current_cursor, &font->handle, row_height, wrap_width);

            float new_scroll = NK_MAX(0.0f, cursor_y - row_height * 2);
            ctx->current->edit.scrollbar.y = new_scroll;
            current_scroll = new_scroll;
            state.target_scroll_y = new_scroll;
            state.pending_scroll_to_cursor = true;

            state.pending_paragraph_move = 0;
            state.pending_paragraph_extend_selection = false;
            state.processed_pending_action = true;
        }

        if (state.pending_jump_to_end != 0) {
            ctx->current->edit.sel_start = ctx->current->edit.cursor;
            ctx->current->edit.sel_end = ctx->current->edit.cursor;

            if (state.pending_jump_to_end < 0) {
                current_cursor = 0;
                ctx->current->edit.scrollbar.y = 0;
                current_scroll = 0;
            } else {
                current_cursor = state.text_len;
                ctx->current->edit.scrollbar.y = 1000000;
                current_scroll = ctx->current->edit.scrollbar.y;
            }
            ctx->current->edit.cursor = current_cursor;
            ctx->current->edit.sel_start = current_cursor;
            ctx->current->edit.sel_end = current_cursor;
            state.pending_jump_to_end = 0;
            state.processed_pending_action = true;
        }

        if (state.pending_delete_word) {
            int word_start = find_word_start(state.text_buffer, current_cursor);
            if (word_start < current_cursor) {
                int chars_to_delete = current_cursor - word_start;
                std::memmove(state.text_buffer + word_start, 
                             state.text_buffer + current_cursor,
                             state.text_len - current_cursor);
                state.text_len -= chars_to_delete;
                state.text_buffer[state.text_len] = '\0';
                current_cursor = word_start;
                ctx->current->edit.cursor = current_cursor;
                ctx->current->edit.sel_start = current_cursor;
                ctx->current->edit.sel_end = current_cursor;
                ctx->text_edit.string.len = state.text_len;
            }
            state.pending_delete_word = false;
            state.processed_pending_action = true;
        }

        if (state.pending_select_all) {
            ctx->current->edit.sel_start = 0;
            ctx->current->edit.sel_end = state.text_len;
            ctx->current->edit.cursor = state.text_len;
            current_cursor = state.text_len;
            state.pending_select_all = false;
            state.processed_pending_action = true;
        }

        if (state.pending_navigate_to_pos >= 0) {
            int target_pos = state.pending_navigate_to_pos;
            if (target_pos > state.text_len) {
                target_pos = state.text_len;
            }
            ctx->current->edit.cursor = target_pos;
            ctx->current->edit.sel_start = target_pos;
            ctx->current->edit.sel_end = target_pos;
            current_cursor = target_pos;

            float edit_padding = ctx->style.edit.padding.x;
            float edit_border = ctx->style.edit.border;
            float scrollbar_size = ctx->style.edit.scrollbar_size.x;
            float cursor_size = ctx->style.edit.cursor_size;
            struct nk_rect bounds = ctx->current->layout->clip;
            float area_w = bounds.w - (2.0f * edit_padding + 2 * edit_border) - scrollbar_size;
            float wrap_width = area_w - cursor_size;

            float row_padding = ctx->style.edit.row_padding;
            float row_height = font->handle.height + row_padding;
            float cursor_y = nk_edit_cursor_y(state.text_buffer, state.text_len,
                                               target_pos, &font->handle, row_height, wrap_width);

            float new_scroll = NK_MAX(0.0f, cursor_y - row_height * 2);
            ctx->current->edit.scrollbar.y = new_scroll;
            current_scroll = new_scroll;
            state.target_scroll_y = new_scroll;
            state.pending_scroll_to_cursor = true;

            state.pending_navigate_to_pos = -1;
        }

        if (current_cursor != state.last_cursor_pos || current_scroll != state.last_scroll_y) {
            state.state_pending_save = true;
            state.state_change_time = SDL_GetTicks();
            state.pending_cursor_pos = current_cursor;
            state.pending_scroll_y = current_scroll;
            state.last_cursor_pos = current_cursor;
            state.last_scroll_y = current_scroll;
        }
        
        state.prev_sel_start = ctx->current->edit.sel_start;
        state.prev_sel_end = ctx->current->edit.sel_end;
    }
    nk_end(ctx);
}

void process_pending_saves(EditorState& state, Uint32 debounce_delay) {
    Uint32 now = SDL_GetTicks();
    if (state.content_pending_save && (now - state.content_change_time >= debounce_delay)) {
        safe_save(state);
        state.content_pending_save = false;
    }
    if (state.state_pending_save && (now - state.state_change_time >= debounce_delay)) {
        save_state(state, state.pending_cursor_pos, state.pending_scroll_y);
        state.state_pending_save = false;
    }
}

int find_prev_paragraph(const char* text, int text_len, int cursor) {
    if (cursor <= 0) return 0;

    int pos = cursor;

    while (pos > 0 && text[pos - 1] == '\n') {
        pos--;
    }

    while (pos > 0 && text[pos - 1] != '\n') {
        pos--;
    }

    return pos;
}

int find_next_paragraph(const char* text, int text_len, int cursor) {
    if (cursor >= text_len) return text_len;

    int pos = cursor;

    while (pos < text_len && text[pos] != '\n') {
        pos++;
    }

    while (pos < text_len && text[pos] == '\n') {
        pos++;
    }

    return pos;
}

int find_word_start(const char* text, int cursor) {
    if (cursor <= 0) return 0;

    int pos = cursor;

    while (pos > 0 && (text[pos - 1] == ' ' || text[pos - 1] == '\n' || text[pos - 1] == '\t')) {
        pos--;
    }

    while (pos > 0 && text[pos - 1] != ' ' && text[pos - 1] != '\n' && text[pos - 1] != '\t') {
        pos--;
    }

    return pos;
}

void perform_undo(EditorState& state, struct nk_context* ctx) {
    if (!state.undo_state.text_buffer) return;
    
    std::memcpy(state.text_buffer, state.undo_state.text_buffer, state.undo_state.text_len);
    state.text_len = state.undo_state.text_len;
    state.text_buffer[state.text_len] = '\0';
    
    ctx->current->edit.cursor = state.undo_state.cursor_pos;
    ctx->current->edit.sel_start = state.undo_state.sel_start;
    ctx->current->edit.sel_end = state.undo_state.sel_end;
    ctx->text_edit.string.len = state.text_len;
    
    state.prev_text_len = state.text_len;
    std::memcpy(state.prev_text_buffer, state.text_buffer, state.text_len);
    if (state.text_len >= HASH_CHANGE_DETECTION_THRESHOLD) {
        state.prev_text_hash = fnv1a_hash(state.text_buffer, state.text_len);
    }
    
    delete[] state.undo_state.text_buffer;
    state.undo_state.text_buffer = nullptr;
}
