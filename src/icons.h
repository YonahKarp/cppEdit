#pragma once

struct nk_command_buffer;
struct nk_color;
struct nk_rect;

void draw_magnifying_glass_icon(struct nk_command_buffer* canvas, float x, float y, 
                                 float size, struct nk_color color);

void draw_battery_icon(struct nk_command_buffer* canvas, float x, float y,
                       int percent, struct nk_color border_color, 
                       struct nk_color fill_color);
