/*
 * Nuklear SDL2 Renderer Backend
 * 
 * This file provides the implementation for rendering Nuklear UI with SDL2 Renderer.
 */

#ifndef NK_SDL_RENDERER_H_
#define NK_SDL_RENDERER_H_

#include <SDL.h>

NK_API struct nk_context* nk_sdl_init(SDL_Window *win, SDL_Renderer *renderer);
NK_API void nk_sdl_font_stash_begin(struct nk_font_atlas **atlas);
NK_API void nk_sdl_font_stash_end(void);
NK_API int nk_sdl_handle_event(SDL_Event *evt);
NK_API void nk_sdl_render(enum nk_anti_aliasing AA);
NK_API void nk_sdl_shutdown(void);

#endif /* NK_SDL_RENDERER_H_ */

#ifdef NK_SDL_RENDERER_IMPLEMENTATION

#include <string.h>

struct nk_sdl_device {
    struct nk_buffer cmds;
    SDL_Texture *font_tex;
    int font_tex_width;
    int font_tex_height;
};

struct nk_sdl {
    SDL_Window *win;
    SDL_Renderer *renderer;
    struct nk_sdl_device ogl;
    struct nk_context ctx;
    struct nk_font_atlas atlas;
};

static struct nk_sdl sdl;

NK_INTERN void
nk_sdl_device_upload_atlas(const void *image, int width, int height)
{
    struct nk_sdl_device *dev = &sdl.ogl;
    SDL_Texture *tex = SDL_CreateTexture(sdl.renderer,
        SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, width, height);
    
    SDL_UpdateTexture(tex, NULL, image, width * 4);
    SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
    dev->font_tex = tex;
    dev->font_tex_width = width;
    dev->font_tex_height = height;
}

NK_INTERN void
nk_sdl_clipbard_paste(nk_handle usr, struct nk_text_edit *edit)
{
    (void)usr;
    char *text = SDL_GetClipboardText();
    if (text) nk_textedit_paste(edit, text, nk_strlen(text));
    SDL_free(text);
}

NK_INTERN void
nk_sdl_clipbard_copy(nk_handle usr, const char *text, int len)
{
    char *str = 0;
    (void)usr;
    if (!len) return;
    str = (char*)malloc((size_t)len+1);
    if (!str) return;
    memcpy(str, text, (size_t)len);
    str[len] = '\0';
    SDL_SetClipboardText(str);
    free(str);
}

NK_API struct nk_context*
nk_sdl_init(SDL_Window *win, SDL_Renderer *renderer)
{
    sdl.win = win;
    sdl.renderer = renderer;
    nk_init_default(&sdl.ctx, 0);
    sdl.ctx.clip.copy = nk_sdl_clipbard_copy;
    sdl.ctx.clip.paste = nk_sdl_clipbard_paste;
    sdl.ctx.clip.userdata = nk_handle_ptr(0);
    nk_buffer_init_default(&sdl.ogl.cmds);
    return &sdl.ctx;
}

NK_API void
nk_sdl_font_stash_begin(struct nk_font_atlas **atlas)
{
    nk_font_atlas_init_default(&sdl.atlas);
    nk_font_atlas_begin(&sdl.atlas);
    *atlas = &sdl.atlas;
}

NK_API void
nk_sdl_font_stash_end(void)
{
    const void *image;
    int w, h;
    image = nk_font_atlas_bake(&sdl.atlas, &w, &h, NK_FONT_ATLAS_RGBA32);
    nk_sdl_device_upload_atlas(image, w, h);
    nk_font_atlas_end(&sdl.atlas, nk_handle_ptr(sdl.ogl.font_tex), 0);
    if (sdl.atlas.default_font)
        nk_style_set_font(&sdl.ctx, &sdl.atlas.default_font->handle);
}

NK_API int
nk_sdl_handle_event(SDL_Event *evt)
{
    struct nk_context *ctx = &sdl.ctx;

    if (evt->type == SDL_KEYUP || evt->type == SDL_KEYDOWN) {
        int down = evt->type == SDL_KEYDOWN;
        const Uint8* state = SDL_GetKeyboardState(0);
        SDL_Keycode sym = evt->key.keysym.sym;

        if (sym == SDLK_RSHIFT || sym == SDLK_LSHIFT)
            nk_input_key(ctx, NK_KEY_SHIFT, down);
        else if (sym == SDLK_DELETE)
            nk_input_key(ctx, NK_KEY_DEL, down);
        else if (sym == SDLK_RETURN)
            nk_input_key(ctx, NK_KEY_ENTER, down);
        else if (sym == SDLK_TAB)
            nk_input_key(ctx, NK_KEY_TAB, down);
        else if (sym == SDLK_BACKSPACE)
            nk_input_key(ctx, NK_KEY_BACKSPACE, down);
        else if (sym == SDLK_HOME) {
            nk_input_key(ctx, NK_KEY_TEXT_START, down);
            nk_input_key(ctx, NK_KEY_SCROLL_START, down);
        } else if (sym == SDLK_END) {
            nk_input_key(ctx, NK_KEY_TEXT_END, down);
            nk_input_key(ctx, NK_KEY_SCROLL_END, down);
        } else if (sym == SDLK_PAGEDOWN) {
            nk_input_key(ctx, NK_KEY_SCROLL_DOWN, down);
        } else if (sym == SDLK_PAGEUP) {
            nk_input_key(ctx, NK_KEY_SCROLL_UP, down);
        } else if (sym == SDLK_z)
            nk_input_key(ctx, NK_KEY_TEXT_UNDO, down && state[SDL_SCANCODE_LCTRL]);
        else if (sym == SDLK_r)
            nk_input_key(ctx, NK_KEY_TEXT_REDO, down && state[SDL_SCANCODE_LCTRL]);
        else if (sym == SDLK_c)
            nk_input_key(ctx, NK_KEY_COPY, down && state[SDL_SCANCODE_LCTRL]);
        else if (sym == SDLK_v)
            nk_input_key(ctx, NK_KEY_PASTE, down && state[SDL_SCANCODE_LCTRL]);
        else if (sym == SDLK_x)
            nk_input_key(ctx, NK_KEY_CUT, down && state[SDL_SCANCODE_LCTRL]);
        else if (sym == SDLK_b)
            nk_input_key(ctx, NK_KEY_TEXT_LINE_START, down && state[SDL_SCANCODE_LCTRL]);
        else if (sym == SDLK_e)
            nk_input_key(ctx, NK_KEY_TEXT_LINE_END, down && state[SDL_SCANCODE_LCTRL]);
        else if (sym == SDLK_UP)
            nk_input_key(ctx, NK_KEY_UP, down);
        else if (sym == SDLK_DOWN)
            nk_input_key(ctx, NK_KEY_DOWN, down);
        else if (sym == SDLK_LEFT) {
            if (state[SDL_SCANCODE_LCTRL] || state[SDL_SCANCODE_RCTRL])
                nk_input_key(ctx, NK_KEY_TEXT_WORD_LEFT, down);
            else nk_input_key(ctx, NK_KEY_LEFT, down);
        } else if (sym == SDLK_RIGHT) {
            if (state[SDL_SCANCODE_LCTRL] || state[SDL_SCANCODE_RCTRL])
                nk_input_key(ctx, NK_KEY_TEXT_WORD_RIGHT, down);
            else nk_input_key(ctx, NK_KEY_RIGHT, down);
        } else return 0;
        return 1;
    } else if (evt->type == SDL_MOUSEBUTTONDOWN || evt->type == SDL_MOUSEBUTTONUP) {
        int down = evt->type == SDL_MOUSEBUTTONDOWN;
        const int x = evt->button.x, y = evt->button.y;
        if (evt->button.button == SDL_BUTTON_LEFT) {
            if (evt->button.clicks > 1)
                nk_input_button(ctx, NK_BUTTON_DOUBLE, x, y, down);
            nk_input_button(ctx, NK_BUTTON_LEFT, x, y, down);
        } else if (evt->button.button == SDL_BUTTON_MIDDLE)
            nk_input_button(ctx, NK_BUTTON_MIDDLE, x, y, down);
        else if (evt->button.button == SDL_BUTTON_RIGHT)
            nk_input_button(ctx, NK_BUTTON_RIGHT, x, y, down);
        return 1;
    } else if (evt->type == SDL_MOUSEMOTION) {
        if (ctx->input.mouse.grabbed) {
            int x = (int)ctx->input.mouse.prev.x, y = (int)ctx->input.mouse.prev.y;
            nk_input_motion(ctx, x + evt->motion.xrel, y + evt->motion.yrel);
        } else nk_input_motion(ctx, evt->motion.x, evt->motion.y);
        return 1;
    } else if (evt->type == SDL_TEXTINPUT) {
        nk_glyph glyph;
        memcpy(glyph, evt->text.text, NK_UTF_SIZE);
        nk_input_glyph(ctx, glyph);
        return 1;
    } else if (evt->type == SDL_MOUSEWHEEL) {
        nk_input_scroll(ctx, nk_vec2((float)evt->wheel.x, (float)evt->wheel.y));
        return 1;
    }
    return 0;
}

NK_API void
nk_sdl_render(enum nk_anti_aliasing AA)
{
    (void)AA;
    
    struct nk_sdl_device *dev = &sdl.ogl;
    struct nk_context *ctx = &sdl.ctx;
    const struct nk_command *cmd;

    SDL_Rect saved_clip;
    SDL_bool clipping_enabled;
    int clip_set = 0;

    clipping_enabled = SDL_RenderIsClipEnabled(sdl.renderer);
    SDL_RenderGetClipRect(sdl.renderer, &saved_clip);

    nk_foreach(cmd, ctx) {
        switch (cmd->type) {
        case NK_COMMAND_NOP: break;
        case NK_COMMAND_SCISSOR: {
            const struct nk_command_scissor *s = (const struct nk_command_scissor*)cmd;
            SDL_Rect r;
            r.x = s->x;
            r.y = s->y;
            r.w = s->w;
            r.h = s->h;
            SDL_RenderSetClipRect(sdl.renderer, &r);
            clip_set = 1;
        } break;
        case NK_COMMAND_LINE: {
            const struct nk_command_line *l = (const struct nk_command_line*)cmd;
            SDL_SetRenderDrawColor(sdl.renderer, l->color.r, l->color.g, l->color.b, l->color.a);
            SDL_RenderDrawLine(sdl.renderer, l->begin.x, l->begin.y, l->end.x, l->end.y);
        } break;
        case NK_COMMAND_RECT: {
            const struct nk_command_rect *r = (const struct nk_command_rect*)cmd;
            SDL_SetRenderDrawColor(sdl.renderer, r->color.r, r->color.g, r->color.b, r->color.a);
            SDL_Rect rect = {r->x, r->y, (int)r->w, (int)r->h};
            SDL_RenderDrawRect(sdl.renderer, &rect);
        } break;
        case NK_COMMAND_RECT_FILLED: {
            const struct nk_command_rect_filled *r = (const struct nk_command_rect_filled*)cmd;
            int rad = (int)r->rounding;
            if (rad <= 0) {
                SDL_SetRenderDrawBlendMode(sdl.renderer, SDL_BLENDMODE_BLEND);
                SDL_SetRenderDrawColor(sdl.renderer, r->color.r, r->color.g, r->color.b, r->color.a);
                SDL_Rect rect = {r->x, r->y, (int)r->w, (int)r->h};
                SDL_RenderFillRect(sdl.renderer, &rect);
            } else {
                int x = r->x, y = r->y, w = (int)r->w, h = (int)r->h;
                if (rad > w/2) rad = w/2;
                if (rad > h/2) rad = h/2;
                SDL_SetRenderDrawColor(sdl.renderer, r->color.r, r->color.g, r->color.b, r->color.a);
                SDL_Rect rects[3] = {
                    {x + rad, y, w - 2*rad, h},
                    {x, y + rad, rad, h - 2*rad},
                    {x + w - rad, y + rad, rad, h - 2*rad}
                };
                SDL_RenderFillRects(sdl.renderer, rects, 3);
                float radf = (float)rad;
                for (int corner = 0; corner < 4; corner++) {
                    int cx = (corner == 0 || corner == 3) ? x + rad : x + w - rad - 1;
                    int cy = (corner < 2) ? y + rad : y + h - rad - 1;
                    for (int dy = -rad; dy <= 0; dy++) {
                        for (int dx = -rad; dx <= 0; dx++) {
                            float dist = SDL_sqrtf((float)(dx*dx + dy*dy));
                            if (dist <= radf + 0.5f) {
                                int px = (corner == 0 || corner == 3) ? cx + dx : cx - dx;
                                int py = (corner < 2) ? cy + dy : cy - dy;
                                if (dist > radf - 0.5f) {
                                    float alpha = (radf + 0.5f - dist);
                                    Uint8 a = (Uint8)(r->color.a * alpha);
                                    SDL_SetRenderDrawColor(sdl.renderer, r->color.r, r->color.g, r->color.b, a);
                                } else {
                                    SDL_SetRenderDrawColor(sdl.renderer, r->color.r, r->color.g, r->color.b, r->color.a);
                                }
                                SDL_RenderDrawPoint(sdl.renderer, px, py);
                            }
                        }
                    }
                }
            }
        } break;
        case NK_COMMAND_CIRCLE: {
            const struct nk_command_circle *c = (const struct nk_command_circle*)cmd;
            SDL_SetRenderDrawColor(sdl.renderer, c->color.r, c->color.g, c->color.b, c->color.a);
            int xc = c->x + c->w / 2;
            int yc = c->y + c->h / 2;
            int rx = c->w / 2;
            int ry = c->h / 2;
            for (int i = 0; i < 360; i++) {
                double rad = i * 3.14159265 / 180.0;
                int px = xc + (int)(rx * SDL_cos(rad));
                int py = yc + (int)(ry * SDL_sin(rad));
                SDL_RenderDrawPoint(sdl.renderer, px, py);
            }
        } break;
        case NK_COMMAND_CIRCLE_FILLED: {
            const struct nk_command_circle_filled *c = (const struct nk_command_circle_filled*)cmd;
            SDL_SetRenderDrawColor(sdl.renderer, c->color.r, c->color.g, c->color.b, c->color.a);
            int xc = c->x + c->w / 2;
            int yc = c->y + c->h / 2;
            int rx = c->w / 2;
            int ry = c->h / 2;
            for (int y = -ry; y <= ry; y++) {
                for (int x = -rx; x <= rx; x++) {
                    if ((x*x*ry*ry + y*y*rx*rx) <= (rx*rx*ry*ry))
                        SDL_RenderDrawPoint(sdl.renderer, xc + x, yc + y);
                }
            }
        } break;
        case NK_COMMAND_TRIANGLE: {
            const struct nk_command_triangle *t = (const struct nk_command_triangle*)cmd;
            SDL_SetRenderDrawColor(sdl.renderer, t->color.r, t->color.g, t->color.b, t->color.a);
            SDL_RenderDrawLine(sdl.renderer, t->a.x, t->a.y, t->b.x, t->b.y);
            SDL_RenderDrawLine(sdl.renderer, t->b.x, t->b.y, t->c.x, t->c.y);
            SDL_RenderDrawLine(sdl.renderer, t->c.x, t->c.y, t->a.x, t->a.y);
        } break;
        case NK_COMMAND_TRIANGLE_FILLED: {
            const struct nk_command_triangle_filled *t = (const struct nk_command_triangle_filled*)cmd;
            SDL_SetRenderDrawColor(sdl.renderer, t->color.r, t->color.g, t->color.b, t->color.a);
            SDL_Vertex verts[3] = {
                {{(float)t->a.x, (float)t->a.y}, {t->color.r, t->color.g, t->color.b, t->color.a}, {0, 0}},
                {{(float)t->b.x, (float)t->b.y}, {t->color.r, t->color.g, t->color.b, t->color.a}, {0, 0}},
                {{(float)t->c.x, (float)t->c.y}, {t->color.r, t->color.g, t->color.b, t->color.a}, {0, 0}}
            };
            SDL_RenderGeometry(sdl.renderer, NULL, verts, 3, NULL, 0);
        } break;
        case NK_COMMAND_TEXT: {
            const struct nk_command_text *t = (const struct nk_command_text*)cmd;
            if (!dev->font_tex || !t->font) break;
            
            struct nk_font *font = (struct nk_font*)t->font->userdata.ptr;
            if (!font) break;
            
            float x = (float)t->x;
            float y = (float)t->y;
            
            for (int i = 0; i < t->length;) {
                nk_rune unicode;
                int glyph_len = nk_utf_decode(t->string + i, &unicode, t->length - i);
                if (!glyph_len) break;
                
                const struct nk_font_glyph *g = nk_font_find_glyph(font, unicode);
                if (g) {
                    SDL_Rect src = {
                        (int)(g->u0 * dev->font_tex_width),
                        (int)(g->v0 * dev->font_tex_height),
                        (int)((g->u1 - g->u0) * dev->font_tex_width),
                        (int)((g->v1 - g->v0) * dev->font_tex_height)
                    };
                    SDL_FRect dst = {
                        x + g->x0,
                        y + g->y0,
                        g->x1 - g->x0,
                        g->y1 - g->y0
                    };
                    SDL_SetTextureColorMod(dev->font_tex, t->foreground.r, t->foreground.g, t->foreground.b);
                    SDL_SetTextureAlphaMod(dev->font_tex, t->foreground.a);
                    SDL_RenderCopyF(sdl.renderer, dev->font_tex, &src, &dst);
                    x += g->xadvance;
                }
                i += glyph_len;
            }
        } break;
        case NK_COMMAND_CURVE:
        case NK_COMMAND_RECT_MULTI_COLOR:
        case NK_COMMAND_ARC:
        case NK_COMMAND_ARC_FILLED:
        case NK_COMMAND_POLYGON:
        case NK_COMMAND_POLYGON_FILLED:
        case NK_COMMAND_POLYLINE:
        case NK_COMMAND_IMAGE:
        case NK_COMMAND_CUSTOM:
        default: break;
        }
    }
    
    if (clip_set) {
        if (clipping_enabled)
            SDL_RenderSetClipRect(sdl.renderer, &saved_clip);
        else
            SDL_RenderSetClipRect(sdl.renderer, NULL);
    }
    
    nk_clear(ctx);
}

NK_API void
nk_sdl_shutdown(void)
{
    struct nk_sdl_device *dev = &sdl.ogl;
    nk_font_atlas_clear(&sdl.atlas);
    nk_free(&sdl.ctx);
    if (dev->font_tex)
        SDL_DestroyTexture(dev->font_tex);
    nk_buffer_free(&dev->cmds);
    memset(&sdl, 0, sizeof(sdl));
}

#endif /* NK_SDL_RENDERER_IMPLEMENTATION */
