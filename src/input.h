#pragma once

#include <SDL.h>

struct nk_context;
struct EditorState;

struct InputState {
    bool up = false;
    bool down = false;
    bool left = false;
    bool right = false;
    bool enter = false;
    bool escape = false;
    bool delete_key = false;
    bool meta = false;
    bool had_input = false;
    bool quit = false;
    bool increase_font_size = false;
    bool decrease_font_size = false;
    bool toggle_theme = false;
    bool toggle_search = false;
    bool search_next = false;
    bool search_prev = false;
};

InputState process_events(nk_context* ctx, EditorState& state, SDL_Event* first_event = nullptr);
