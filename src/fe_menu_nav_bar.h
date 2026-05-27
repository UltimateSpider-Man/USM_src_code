#pragma once

#include "femenu.h"
#include "mstring.h"

struct FEText;
struct PanelQuad;

struct menu_nav_bar {
    struct button_type {
        int field_0;
    };

    int field_0;
    mString field_4;
    FEText *text_box;
    PanelQuad *background_a;
    PanelQuad *field_1C;
    FEText *field_20;
    FEText *field_24;
    bool field_28;
    char empty[3];

    menu_nav_bar();

    //0x00612080
    void Load();

	void Draw(){
	    };

    //0x006121C0
    void AddButtons(menu_nav_bar::button_type a2, menu_nav_bar::button_type a3, global_text_enum a4);

    //0x006122B0
    void Reformat();
};

extern void menu_nav_bar_patch();
