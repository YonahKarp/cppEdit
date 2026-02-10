#pragma once

// NOTE: This header must be included AFTER nuklear.h

struct Theme {
    struct nk_color background;
    struct nk_color text;
    struct nk_color text_dim;
    struct nk_color edit_bg;
    struct nk_color edit_text;
    struct nk_color cursor;
    struct nk_color selection;
    
    struct nk_color sidebar_bg;
    struct nk_color sidebar_btn_normal;
    struct nk_color sidebar_btn_hover;
    struct nk_color sidebar_btn_active;
    struct nk_color sidebar_btn_text;
    struct nk_color sidebar_btn_text_hover;
    struct nk_color sidebar_selected_normal;
    struct nk_color sidebar_selected_hover;
    struct nk_color sidebar_selected_active;
    struct nk_color sidebar_new_file_normal;
    struct nk_color sidebar_new_file_hover;
    struct nk_color sidebar_new_file_active;
    struct nk_color sidebar_new_file_selected_normal;
    struct nk_color sidebar_new_file_selected_hover;
    struct nk_color sidebar_new_file_selected_active;
    struct nk_color sidebar_delete_normal;
    struct nk_color sidebar_delete_hover;
    struct nk_color sidebar_delete_active;
    struct nk_color sidebar_cancel_normal;
    struct nk_color sidebar_cancel_hover;
    struct nk_color sidebar_cancel_active;

    struct nk_color battery_border;
    struct nk_color battery_good;
    struct nk_color battery_low;
    
    struct nk_color scrollbar_bg;
    struct nk_color scrollbar_cursor;
    struct nk_color scrollbar_cursor_hover;
    
    struct nk_color search_highlight;
    struct nk_color search_current_highlight;
    struct nk_color search_input_bg;
    struct nk_color search_input_text;
};

inline Theme get_dark_theme() {
    Theme t;
    t.background = nk_rgb(40, 40, 40);
    t.text = nk_rgb(200, 200, 200);
    t.text_dim = nk_rgb(120, 120, 120);
    t.edit_bg = nk_rgb(30, 30, 30);
    t.edit_text = nk_rgb(200, 200, 200);
    t.cursor = nk_rgb(180, 180, 180);
    t.selection = nk_rgb(70, 100, 150);
    
    t.sidebar_bg = nk_rgb(40, 40, 40);
    t.sidebar_btn_normal = nk_rgb(50, 50, 50);
    t.sidebar_btn_hover = nk_rgb(70, 70, 70);
    t.sidebar_btn_active = nk_rgb(90, 90, 90);
    t.sidebar_btn_text = nk_rgb(200, 200, 200);
    t.sidebar_btn_text_hover = nk_rgb(255, 255, 255);
    t.sidebar_selected_normal = nk_rgb(80, 100, 120);
    t.sidebar_selected_hover = nk_rgb(90, 110, 130);
    t.sidebar_selected_active = nk_rgb(100, 120, 140);
    t.sidebar_new_file_normal = nk_rgb(60, 70, 60);
    t.sidebar_new_file_hover = nk_rgb(70, 85, 70);
    t.sidebar_new_file_active = nk_rgb(80, 100, 80);
    t.sidebar_new_file_selected_normal = nk_rgb(80, 120, 80);
    t.sidebar_new_file_selected_hover = nk_rgb(90, 130, 90);
    t.sidebar_new_file_selected_active = nk_rgb(100, 140, 100);
    t.sidebar_delete_normal = nk_rgb(120, 60, 60);
    t.sidebar_delete_hover = nk_rgb(140, 70, 70);
    t.sidebar_delete_active = nk_rgb(160, 80, 80);
    t.sidebar_cancel_normal = nk_rgb(70, 70, 70);
    t.sidebar_cancel_hover = nk_rgb(90, 90, 90);
    t.sidebar_cancel_active = nk_rgb(110, 110, 110);

    t.battery_border = nk_rgb(120, 120, 120);
    t.battery_good = nk_rgb(100, 180, 100);
    t.battery_low = nk_rgb(200, 80, 80);
    
    t.scrollbar_bg = nk_rgb(40, 40, 40);
    t.scrollbar_cursor = nk_rgb(80, 80, 80);
    t.scrollbar_cursor_hover = nk_rgb(100, 100, 100);
    
    t.search_highlight = nk_rgba(100, 100, 50, 100);
    t.search_current_highlight = nk_rgba(180, 140, 50, 100);
    t.search_input_bg = nk_rgb(50, 50, 50);
    t.search_input_text = nk_rgb(220, 220, 220);
    
    return t;
}

inline Theme get_light_theme() {
    Theme t;
    t.background = nk_rgb(245, 245, 245);
    t.text = nk_rgb(40, 40, 40);
    t.text_dim = nk_rgb(120, 120, 120);
    t.edit_bg = nk_rgb(255, 255, 255);
    t.edit_text = nk_rgb(30, 30, 30);
    t.cursor = nk_rgb(50, 50, 50);
    t.selection = nk_rgb(180, 210, 255);
    
    t.sidebar_bg = nk_rgb(235, 235, 235);
    t.sidebar_btn_normal = nk_rgb(225, 225, 225);
    t.sidebar_btn_hover = nk_rgb(210, 210, 210);
    t.sidebar_btn_active = nk_rgb(195, 195, 195);
    t.sidebar_btn_text = nk_rgb(50, 50, 50);
    t.sidebar_btn_text_hover = nk_rgb(20, 20, 20);
    t.sidebar_selected_normal = nk_rgb(180, 200, 220);
    t.sidebar_selected_hover = nk_rgb(170, 190, 210);
    t.sidebar_selected_active = nk_rgb(160, 180, 200);
    t.sidebar_new_file_normal = nk_rgb(200, 220, 200);
    t.sidebar_new_file_hover = nk_rgb(190, 210, 190);
    t.sidebar_new_file_active = nk_rgb(180, 200, 180);
    t.sidebar_new_file_selected_normal = nk_rgb(170, 210, 170);
    t.sidebar_new_file_selected_hover = nk_rgb(160, 200, 160);
    t.sidebar_new_file_selected_active = nk_rgb(150, 190, 150);
    t.sidebar_delete_normal = nk_rgb(220, 160, 160);
    t.sidebar_delete_hover = nk_rgb(210, 140, 140);
    t.sidebar_delete_active = nk_rgb(200, 120, 120);
    t.sidebar_cancel_normal = nk_rgb(200, 200, 200);
    t.sidebar_cancel_hover = nk_rgb(180, 180, 180);
    t.sidebar_cancel_active = nk_rgb(160, 160, 160);

    t.battery_border = nk_rgb(100, 100, 100);
    t.battery_good = nk_rgb(80, 160, 80);
    t.battery_low = nk_rgb(200, 80, 80);
    
    t.scrollbar_bg = nk_rgb(230, 230, 230);
    t.scrollbar_cursor = nk_rgb(180, 180, 180);
    t.scrollbar_cursor_hover = nk_rgb(160, 160, 160);
    
    t.search_highlight = nk_rgb(255, 255, 150);
    t.search_current_highlight = nk_rgb(255, 200, 100);
    t.search_input_bg = nk_rgb(255, 255, 255);
    t.search_input_text = nk_rgb(30, 30, 30);
    
    return t;
}

inline const Theme& get_theme(bool dark_theme) {
    static Theme cached_theme;
    static bool cached_dark_theme = !dark_theme;
    
    if (cached_dark_theme != dark_theme) {
        cached_dark_theme = dark_theme;
        cached_theme = dark_theme ? get_dark_theme() : get_light_theme();
    }
    
    return cached_theme;
}

inline bool theme_changed(bool dark_theme) {
    static bool last_dark_theme = !dark_theme;
    if (last_dark_theme != dark_theme) {
        last_dark_theme = dark_theme;
        return true;
    }
    return false;
}
