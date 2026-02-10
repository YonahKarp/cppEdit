#include "input.h"
#include "editor.h"
#include "sidebar.h"
#include "search.h"

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#include "nuklear.h"

#include "nuklear_sdl_renderer.h"

InputState process_events(nk_context* ctx, EditorState& state, SDL_Event* first_event) {
    InputState input;

    nk_input_begin(ctx);

    auto handle_event = [&](SDL_Event& e) {
        if (e.type == SDL_QUIT) {
            input.quit = true;
        }

        if (e.type == SDL_KEYDOWN || e.type == SDL_TEXTINPUT) {
            input.had_input = true;
        }

        // Suppress text input when Ctrl or Cmd is held - this prevents shortcuts
        // from also typing characters (e.g., Ctrl+A typing 'a' on Linux)
        if (e.type == SDL_TEXTINPUT) {
            const Uint8* keyboard_state = SDL_GetKeyboardState(NULL);
            bool ctrl_held = keyboard_state[SDL_SCANCODE_LCTRL] || keyboard_state[SDL_SCANCODE_RCTRL];
            bool gui_held = keyboard_state[SDL_SCANCODE_LGUI] || keyboard_state[SDL_SCANCODE_RGUI];
            if (ctrl_held || gui_held) {
                return;
            }
        }

        if (e.type == SDL_TEXTINPUT && state.search.active) {
            size_t input_len = strlen(e.text.text);
            if (state.search.query_len + input_len < SEARCH_BUFFER_SIZE - 1) {
                memcpy(state.search.query + state.search.query_len, e.text.text, input_len);
                state.search.query_len += input_len;
                state.search.query[state.search.query_len] = '\0';
                perform_search(state.search, state.text_buffer, state.text_len);
                int match_pos = get_current_match_start(state.search);
                if (match_pos >= 0) {
                    state.pending_navigate_to_pos = match_pos;
                }
            }
            return;
        }

        if (e.type == SDL_TEXTINPUT && state.sidebar.visible && state.sidebar.searching) {
            size_t input_len = strlen(e.text.text);
            if (state.sidebar.search_len + input_len < sizeof(state.sidebar.search_buffer) - 1) {
                memcpy(state.sidebar.search_buffer + state.sidebar.search_len, e.text.text, input_len);
                state.sidebar.search_len += input_len;
                state.sidebar.search_buffer[state.sidebar.search_len] = '\0';
                filter_sidebar_files(state);
            }
            return;
        }

        if (e.type == SDL_TEXTINPUT && state.sidebar.visible &&
            !state.sidebar.confirm_delete && !state.sidebar.new_file_selected && !state.sidebar.searching) {
            handle_sidebar_input(state, &e);
            return;
        }

        if (e.type == SDL_KEYDOWN) {
            if (e.key.keysym.sym == SDLK_LGUI || e.key.keysym.sym == SDLK_RGUI) {
                input.meta = true;
                return;
            }

            if (e.key.keysym.sym == SDLK_ESCAPE &&
                (e.key.keysym.mod & KMOD_CTRL) &&
                (e.key.keysym.mod & KMOD_SHIFT) &&
                (e.key.keysym.mod & KMOD_ALT)) {
                input.quit = true;
                return;
            }

            if (state.search.active) {
                if (e.key.keysym.sym == SDLK_ESCAPE) {
                    close_search(state.search);
                    return;
                } else if (e.key.keysym.sym == SDLK_RETURN || e.key.keysym.sym == SDLK_KP_ENTER) {
                    if (e.key.keysym.mod & KMOD_SHIFT) {
                        input.search_prev = true;
                    } else {
                        input.search_next = true;
                    }
                    return;
                } else if (e.key.keysym.sym == SDLK_BACKSPACE) {
                    if (state.search.query_len > 0) {
                        state.search.query_len--;
                        state.search.query[state.search.query_len] = '\0';
                        perform_search(state.search, state.text_buffer, state.text_len);
                        int match_pos = get_current_match_start(state.search);
                        if (match_pos >= 0) {
                            state.pending_navigate_to_pos = match_pos;
                        }
                    }
                    return;
                }
            }

            if (state.sidebar.visible) {
                if (state.sidebar.searching) {
                    if (e.key.keysym.sym == SDLK_ESCAPE) {
                        state.sidebar.searching = false;
                        state.sidebar.search_buffer[0] = '\0';
                        state.sidebar.search_len = 0;
                        filter_sidebar_files(state);
                        return;
                    } else if (e.key.keysym.sym == SDLK_BACKSPACE) {
                        if (state.sidebar.search_len > 0) {
                            state.sidebar.search_len--;
                            state.sidebar.search_buffer[state.sidebar.search_len] = '\0';
                            filter_sidebar_files(state);
                        }
                        return;
                    } else if (e.key.keysym.sym == SDLK_UP) {
                        input.up = true;
                        return;
                    } else if (e.key.keysym.sym == SDLK_DOWN) {
                        input.down = true;
                        return;
                    } else if (e.key.keysym.sym == SDLK_RETURN || e.key.keysym.sym == SDLK_KP_ENTER) {
                        input.enter = true;
                        return;
                    }
                }
                if (e.key.keysym.sym == SDLK_UP) {
                    input.up = true;
                    return;
                } else if (e.key.keysym.sym == SDLK_DOWN) {
                    input.down = true;
                    return;
                } else if (e.key.keysym.sym == SDLK_LEFT) {
                    input.left = true;
                    return;
                } else if (e.key.keysym.sym == SDLK_RIGHT) {
                    input.right = true;
                    return;
                } else if (e.key.keysym.sym == SDLK_RETURN || e.key.keysym.sym == SDLK_KP_ENTER) {
                    input.enter = true;
                    return;
                } else if (e.key.keysym.sym == SDLK_ESCAPE) {
                    input.escape = true;
                    return;
                } else if (e.key.keysym.sym == SDLK_BACKSPACE || e.key.keysym.sym == SDLK_DELETE) {
                    if (state.sidebar.renaming) {
                        handle_sidebar_input(state, &e);
                        return;
                    }
                    input.delete_key = true;
                    return;
                }
            }

            if (e.key.keysym.mod & KMOD_CTRL) {
                bool shift_held = (e.key.keysym.mod & KMOD_SHIFT) != 0;
                if (e.key.keysym.sym == SDLK_f) {
                    if (state.sidebar.visible && !state.sidebar.confirm_delete) {
                        toggle_sidebar_search(state);
                    } else {
                        input.toggle_search = true;
                    }
                    input.had_input = true;
                    return;
                } else if (e.key.keysym.sym == SDLK_UP) {
                    state.pending_paragraph_move = -1;
                    state.pending_paragraph_extend_selection = shift_held;
                    input.had_input = true;
                    return;
                } else if (e.key.keysym.sym == SDLK_DOWN) {
                    state.pending_paragraph_move = 1;
                    state.pending_paragraph_extend_selection = shift_held;
                    input.had_input = true;
                    return;
                } else if (e.key.keysym.sym == SDLK_BACKSPACE) {
                    state.pending_delete_word = true;
                    input.had_input = true;
                    return;
                } else if (e.key.keysym.sym == SDLK_a) {
                    state.pending_select_all = true;
                    input.had_input = true;
                    return;
                } else if (e.key.keysym.sym == SDLK_EQUALS || e.key.keysym.sym == SDLK_PLUS || e.key.keysym.sym == SDLK_KP_PLUS) {
                    input.increase_font_size = true;
                    return;
                } else if (e.key.keysym.sym == SDLK_MINUS || e.key.keysym.sym == SDLK_KP_MINUS) {
                    input.decrease_font_size = true;
                    return;
                } else if (e.key.keysym.sym == SDLK_t) {
                    input.toggle_theme = true;
                    return;
                } else if (e.key.keysym.sym == SDLK_z) {
                    state.pending_undo = true;
                    input.had_input = true;
                    return;
                }
            }

            if (e.key.keysym.mod & (KMOD_LALT | KMOD_RALT)) {
                if (e.key.keysym.sym == SDLK_UP) {
                    state.pending_jump_to_end = -1;
                    input.had_input = true;
                    return;
                } else if (e.key.keysym.sym == SDLK_DOWN) {
                    state.pending_jump_to_end = 1;
                    input.had_input = true;
                    return;
                }
            }
        }

        if (e.type == SDL_TEXTINPUT && !state.search.active && !state.sidebar.visible) {
            state.last_typed_char = e.text.text[0];
        }

        nk_sdl_handle_event(&e);
    };

    if (first_event) {
        handle_event(*first_event);
    }

    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        handle_event(e);
    }

    nk_input_end(ctx);

    return input;
}
