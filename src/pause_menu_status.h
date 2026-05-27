#pragma once

#include "femenu.h"

struct PanelQuad;
struct FEText;
struct PauseMenuSystem;
struct pause_menu_awards;
struct pause_menu_game;
struct pause_menu_goals;
struct mString;

struct pause_menu_status : FEMenu {
    PanelQuad *field_2C[25];
    PanelQuad *field_90[5];
    FEText *field_A4[14];
    PanelQuad *field_DC;
    PanelQuad *field_E0;
    FEText *field_E4;
    PauseMenuSystem *field_E8;
    int field_EC;
    pause_menu_awards *field_F0;
    pause_menu_game *field_F4;
    pause_menu_goals *field_F8;
    int field_FC;
    int field_100;
    int field_104;
    int field_108;
    int field_10C;
    int field_110;
    int field_114;
    float field_118;
    float field_11C;
    float field_120;
    float field_124;
    int field_128;
    bool field_12C;
    bool field_12D;

    //0x0060FF30
    pause_menu_status(FEMenuSystem *a2, int a3, int a4);

    //0x00610290
    mString *get_element_desc(mString *out, int a3);
	
	
	 const mString *get_element_value(int index);
	
	mString *get_element_value_0(mString *out, int index);


    //0x0061CC30
    void update_selected();

    //0x0061D3F0
    //virtual
    void OnTriangle(int a2);

    //0x0063B890
    //virtual
    void _Load();
	
	void OnUp(int a2);
	
	void OnDown(int a2);
	
	void Down();
	
	char OnMouseEvent(int msg, int a3, int a4);


	    void Init();
};

extern void pause_menu_status_patch();

