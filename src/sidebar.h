#pragma once

#include <SDL.h>
#include <string>
#include <vector>

struct nk_context;
struct nk_font;
struct InputState;

struct SidebarState {
    bool visible = false;
    std::vector<std::string> file_list;
    std::vector<std::string> filtered_file_list;
    std::vector<std::string> deleted_file_list;
    std::vector<std::string> filtered_deleted_list;
    int selected_index = 0;
    bool new_file_selected = false;
    bool confirm_delete = false;
    int delete_index = -1;
    int dialog_selection = 1;  // 0 = Delete, 1 = Cancel (default)
    std::string previous_file_path;  // File to revert to after delete/cancel

    bool confirm_restore = false;
    int restore_index = -1;
    
    bool renaming = false;
    char rename_buffer[256] = "";
    int rename_len = 0;
    bool rename_pending = false;
    Uint32 rename_change_time = 0;
    int rename_file_index = -1;
    
    bool searching = false;
    char search_buffer[256] = "";
    int search_len = 0;
    
    unsigned int scroll_offset = 0;
    int item_height = 35;
    int visible_height = 0;
    
    float anim_progress = 0.0f;
    Uint32 anim_start_time = 0;
    bool anim_closing = false;
    static constexpr Uint32 ANIM_DURATION_MS = 100;
};

struct EditorState;

void scan_txt_files(EditorState& state);

void toggle_sidebar(EditorState& state);

void switch_to_file(EditorState& state, const std::string& new_file_path);

void create_new_file(EditorState& state, const std::string& filename);

void create_new_file_auto(EditorState& state);

void delete_file(EditorState& state, int file_index);

bool render_sidebar(struct nk_context* ctx, EditorState& state,
                    int window_width, int window_height, struct nk_font* font,
                    struct nk_font* sidebar_font, struct nk_font* small_font);

void handle_sidebar_input(EditorState& state, SDL_Event* event);

void handle_sidebar_keyboard(EditorState& state, const InputState& input);

void process_rename_save(EditorState& state, Uint32 debounce_delay);

void filter_sidebar_files(EditorState& state);

void toggle_sidebar_search(EditorState& state);

void scan_deleted_files(EditorState& state);

void cleanup_old_deleted_files(EditorState& state);

void restore_file(EditorState& state, int deleted_index);
