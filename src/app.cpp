#include "app.h"
#include <cstdio>

#define NK_IMPLEMENTATION
#include "nk_common.h"

#define NK_SDL_RENDERER_IMPLEMENTATION
#include "nuklear_sdl_renderer.h"

bool init_app(App& app) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return false;
    }

    app.window = SDL_CreateWindow(
        "JustType",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        app.window_width, app.window_height,
        SDL_WINDOW_SHOWN
    );

    if (!app.window) {
        std::fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        return false;
    }

    app.renderer = SDL_CreateRenderer(
        app.window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );
    if (!app.renderer) {
        std::fprintf(stderr, "SDL_CreateRenderer (accelerated) failed: %s\n", SDL_GetError());
        app.renderer = SDL_CreateRenderer(app.window, -1, SDL_RENDERER_SOFTWARE);
        if (!app.renderer) {
            std::fprintf(stderr, "SDL_CreateRenderer (software) failed: %s\n", SDL_GetError());
            SDL_DestroyWindow(app.window);
            SDL_Quit();
            return false;
        }
    }

    app.ctx = nk_sdl_init(app.window, app.renderer);

    SDL_ShowCursor(SDL_DISABLE);

    struct nk_font_atlas* atlas;
    nk_sdl_font_stash_begin(&atlas);
    app.font = nk_font_atlas_add_from_file(atlas, "../fonts/OpenSans-Regular.ttf", 30, 0);
    if (!app.font) {
        app.font = nk_font_atlas_add_default(atlas, 30, 0);
    }
    app.status_font = nk_font_atlas_add_from_file(atlas, "../fonts/OpenSans-Regular.ttf", App::STATUS_FONT_SIZE, 0);
    if (!app.status_font) {
        app.status_font = nk_font_atlas_add_default(atlas, App::STATUS_FONT_SIZE, 0);
    }
    app.sidebar_font = nk_font_atlas_add_from_file(atlas, "../fonts/OpenSans-Regular.ttf", App::SIDEBAR_FONT_SIZE, 0);
    if (!app.sidebar_font) {
        app.sidebar_font = nk_font_atlas_add_default(atlas, App::SIDEBAR_FONT_SIZE, 0);
    }
    nk_sdl_font_stash_end();
    nk_style_set_font(app.ctx, &app.font->handle);

    return true;
}

void shutdown_app(App& app) {
    nk_sdl_shutdown();
    if (app.renderer) {
        SDL_DestroyRenderer(app.renderer);
    }
    if (app.window) {
        SDL_DestroyWindow(app.window);
    }
    SDL_Quit();
}

void reload_fonts(App& app, float new_font_size) {
    if (new_font_size < App::MIN_FONT_SIZE || new_font_size > App::MAX_FONT_SIZE) {
        return;
    }

    app.font_size = new_font_size;

    struct nk_font_atlas* atlas;
    nk_sdl_font_stash_begin(&atlas);
    app.font = nk_font_atlas_add_from_file(atlas, "../fonts/OpenSans-Regular.ttf", new_font_size, 0);
    if (!app.font) {
        app.font = nk_font_atlas_add_default(atlas, new_font_size, 0);
    }
    app.status_font = nk_font_atlas_add_from_file(atlas, "../fonts/OpenSans-Regular.ttf", App::STATUS_FONT_SIZE, 0);
    if (!app.status_font) {
        app.status_font = nk_font_atlas_add_default(atlas, App::STATUS_FONT_SIZE, 0);
    }
    app.sidebar_font = nk_font_atlas_add_from_file(atlas, "../fonts/OpenSans-Regular.ttf", App::SIDEBAR_FONT_SIZE, 0);
    if (!app.sidebar_font) {
        app.sidebar_font = nk_font_atlas_add_default(atlas, App::SIDEBAR_FONT_SIZE, 0);
    }
    nk_sdl_font_stash_end();
    nk_style_set_font(app.ctx, &app.font->handle);
}
