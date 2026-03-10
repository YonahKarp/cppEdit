#pragma once

#include <SDL.h>

struct nk_context;
struct nk_font;
struct nk_font_atlas;

struct App {
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    nk_context* ctx = nullptr;
    nk_font* font = nullptr;
    nk_font* status_font = nullptr;
    nk_font* sidebar_font = nullptr;

    int window_width = 1024;
    int window_height = 600;

    float font_size = 30.0f;
    static constexpr float MIN_FONT_SIZE = 20.0f;
    static constexpr float MAX_FONT_SIZE = 54.0f;
    static constexpr float FONT_SIZE_STEP = 2.0f;
    static constexpr float SIDEBAR_FONT_SIZE = 24.0f;
    static constexpr float STATUS_FONT_SIZE = 22.0f;
};

bool init_app(App& app);
void shutdown_app(App& app);
void reload_fonts(App& app, float new_font_size);
