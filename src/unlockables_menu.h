#pragma once

#include "femenu.h"
#include "panelfile.h"

struct FEMenuSystem;
struct FEText;
struct PanelQuad;
struct nglTexture;
struct menu_nav_bar;


struct unlockable_item {
    PanelQuad *icon;                  // +0x00
    PanelQuad *icon_highlight;        // +0x04
    nglTexture *full_icon_texture;    // +0x08
    nglTexture *big_icon_texture;     // +0x0C
};

// Total size: 0x100, layout matches USM.exe (0x614020 ctor, 0x645C20 init).
struct unlockables_menu : FEMenu {
    FEMenuSystem *field_2C;            // 0x2C  (this+44)
    PanelFile *panel_file;             // 0x30  (this+48)
    menu_nav_bar *field_34;            // 0x34  (this+52)
    PanelQuad *field_38[14];           // 0x38..0x6F (14 background panel quads)
    int field_70;                      // 0x70  (unused/padding in IDA dump)
    unlockable_item items[7];          // 0x74..0xE3 (7 * 16 bytes)
    int field_E4;                      // 0xE4  vertical delta between adjacent icons
    int field_E8;                      // 0xE8  total span = field_E4 * (n - 1)
    FEText *field_EC;                  // 0xEC  "u_mm_unlockables_text"
    FEText *field_F0;                  // 0xF0  "u_mm_title_box_text"
    int field_F4;                      // 0xF4  current selection index
    int field_F8;                      // 0xF8  saved selection (last visible)
    int field_FC;                      // 0xFC  trailing field

    //0x00614020
    unlockables_menu(FEMenuSystem *a2, int a3, int a4);

    //0x0064BF80 - load icon + highlight + full + big textures for items[a2-1]
    void sub_64BF80(PanelFile *a1, int a2);

    //0x00645C20 - virtual Init: builds whole menu from "unlock_menu" PanelFile
    void Init();

    //0x00614110 - virtual Load: TurnOn(true) on all 14 background quads
    void _Load();

    //0x0062DB20 - virtual OnCross: dispatch on field_F4
    void OnCross(int a2);

    //0x006253C0 - virtual OnTriangle: pop pack + back to main menu
    void OnTriangle(int a2);

    //0x0062D510 - virtual OnActivate
    void OnActivate();

    //0x00614140 - virtual OnDeactivate
    void OnDeactivate(FEMenu *a2);

    //0x0062D6D0 - scroll/redraw helper used by Init + OnUp/OnDown
    void sub_62D6D0(int a2);
};

extern void unlockables_menu_patch();