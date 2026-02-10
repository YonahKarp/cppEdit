#include "icons.h"
#include <cmath>
#include <cstdarg>

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#include "nuklear.h"

void draw_magnifying_glass_icon(struct nk_command_buffer* canvas, float x, float y, 
                                 float size, struct nk_color color) {
    float circle_radius = size * 0.35f;
    float circle_cx = x + circle_radius + 1;
    float circle_cy = y + circle_radius + 1;
    
    nk_stroke_circle(canvas, nk_rect(circle_cx - circle_radius, circle_cy - circle_radius, 
                    circle_radius * 2, circle_radius * 2), 2.0f, color);
    
    float handle_start_x = circle_cx + circle_radius * 0.5f;
    float handle_start_y = circle_cy + circle_radius * 0.5f;
    float handle_end_x = x + size - 1;
    float handle_end_y = y + size - 1;
    float handle_thickness = 2.5f;
    
    float dx = handle_end_x - handle_start_x;
    float dy = handle_end_y - handle_start_y;
    float len = sqrtf(dx*dx + dy*dy);
    float nx = -dy / len * handle_thickness * 0.5f;
    float ny = dx / len * handle_thickness * 0.5f;
    
    nk_fill_triangle(canvas, 
        handle_start_x + nx, handle_start_y + ny,
        handle_start_x - nx, handle_start_y - ny,
        handle_end_x + nx, handle_end_y + ny, color);
    nk_fill_triangle(canvas,
        handle_start_x - nx, handle_start_y - ny,
        handle_end_x - nx, handle_end_y - ny,
        handle_end_x + nx, handle_end_y + ny, color);
}

void draw_battery_icon(struct nk_command_buffer* canvas, float x, float y,
                       int percent, struct nk_color border_color, 
                       struct nk_color fill_color) {
    const int batt_width = 30;
    const int batt_height = 14;
    const int batt_tip_width = 3;
    const int batt_tip_height = 6;

    nk_stroke_rect(canvas, nk_rect(x, y, batt_width, batt_height), 2, 1, border_color);

    nk_fill_rect(canvas, nk_rect(x + batt_width, y + (batt_height - batt_tip_height) / 2, 
                 batt_tip_width, batt_tip_height), 0, border_color);

    int fill_width = (batt_width - 4) * percent / 100;
    if (fill_width > 0) {
        nk_fill_rect(canvas, nk_rect(x + 2, y + 2, fill_width, batt_height - 4), 0, fill_color);
    }
}
