#pragma once

#include "sidebar.h"
#include "search.h"
#include <SDL.h>
#include <string>
#include <cstdint>

struct nk_context;
struct nk_font;

constexpr int TEXT_BUFFER_SIZE = 512 * 1024;  // 512KB - enough for ~80k words

enum class ParagraphDirection { None = 0, Previous = -1, Next = 1 };
enum class JumpDirection { None = 0, Top = -1, Bottom = 1 };

struct UndoState {
    ~UndoState() { delete[] text_buffer; }
    
    UndoState() = default;
    UndoState(const UndoState&) = delete;
    UndoState& operator=(const UndoState&) = delete;
    UndoState(UndoState&& other) noexcept 
        : text_buffer(other.text_buffer), text_len(other.text_len),
          cursor_pos(other.cursor_pos), sel_start(other.sel_start), sel_end(other.sel_end) {
        other.text_buffer = nullptr;
    }
    UndoState& operator=(UndoState&& other) noexcept {
        if (this != &other) {
            delete[] text_buffer;
            text_buffer = other.text_buffer;
            text_len = other.text_len;
            cursor_pos = other.cursor_pos;
            sel_start = other.sel_start;
            sel_end = other.sel_end;
            other.text_buffer = nullptr;
        }
        return *this;
    }

    char* text_buffer = nullptr;
    int text_len = 0;
    int cursor_pos = 0;
    int sel_start = 0;
    int sel_end = 0;
};

struct EditorState {
    char text_buffer[TEXT_BUFFER_SIZE] = "";
    int text_len = 0;
    char prev_text_buffer[TEXT_BUFFER_SIZE];
    int prev_text_len = 0;
    uint64_t prev_text_hash = 0;

    std::string current_file_path;
    std::string temp_file_path;
    std::string state_file_path;

    int saved_cursor_pos = -1;
    float saved_scroll_y = -1.0f;
    int last_cursor_pos = -1;
    float last_scroll_y = -1.0f;

    bool content_pending_save = false;
    Uint32 content_change_time = 0;
    bool state_pending_save = false;
    Uint32 state_change_time = 0;
    int pending_cursor_pos = 0;
    float pending_scroll_y = 0;

    Uint32 last_blink_time = 0;
    bool cursor_visible = true;

    bool first_frame = true;
    bool second_frame = false;

    ParagraphDirection pending_paragraph_move = ParagraphDirection::None;
    bool pending_paragraph_extend_selection = false;
    bool pending_select_all = false;
    JumpDirection pending_jump_to_end = JumpDirection::None;
    bool pending_delete_word = false;
    bool pending_scroll_to_cursor = false;
    int pending_navigate_to_pos = -1;
    float target_scroll_y = 0;
    bool processed_pending_action = false;

    UndoState undo_state;
    bool pending_undo = false;
    int prev_sel_start = 0;
    int prev_sel_end = 0;

    SidebarState sidebar;
    SearchState search;
    std::string user_files_dir;
    std::string recently_deleted_dir;

    float font_size = 30.0f;
    bool dark_theme = true;

    int battery_percent = 100;
    bool battery_charging = false;

    int cached_word_count = 0;
    char last_typed_char = 0;
};

void init_editor_state(EditorState& state, const char* file_path, const char* state_file_path,
                       int saved_cursor_pos, float saved_scroll_y);

void safe_save(EditorState& state);

void save_state(EditorState& state, int cursor_pos, float scroll_y);

void handle_cursor_blink(EditorState& state, bool had_input);

void render_editor(struct nk_context* ctx, EditorState& state,
                   int window_width, int window_height, struct nk_font* font,
                   struct nk_font* status_font);

void process_pending_saves(EditorState& state, Uint32 debounce_delay);

int find_prev_paragraph(const char* text, int text_len, int cursor);
int find_next_paragraph(const char* text, int text_len, int cursor);
int find_word_start(const char* text, int cursor);

void save_undo_state(EditorState& state, int cursor_pos, int sel_start, int sel_end);
void perform_undo(EditorState& state, struct nk_context* ctx);
int count_words(const char* text, int text_len);
