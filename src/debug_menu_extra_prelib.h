#pragma once

struct debug_menu;
struct debug_menu_entry;

extern void create_gamefile_menu_prelib(debug_menu *parent);

extern void create_warp_menu_prelib(debug_menu *parent);

extern void create_debug_render_menu_prelib(debug_menu *parent);

extern void create_debug_district_variants_menu_prelib(debug_menu *parent);

extern void create_camera_menu_items_prelib(debug_menu *parent);

extern void game_flags_handler_prelib(debug_menu_entry *a1);
