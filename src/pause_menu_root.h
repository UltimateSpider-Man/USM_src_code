#pragma once

#include "femenu.h"

#include "float.hpp"
#include "panelanim.h"
#include "panelanimfile.h"
#include "string_hash.h"
#include "panelquad.h"

#include "variable.h"

#include "func_wrapper.h"

#include "script.h"

#include "wds.h"

#include "ai_player_controller.h"

#include "cursor.h"

struct FEMenuSystem;
struct FEMenuSystem2;
struct PanelQuad;
struct FEText;




struct pause_menu_root : FEMenu {
    bool field_2C;
    char field_2D;
    int field_30;
    int field_34;
    int field_38;
    PanelQuad *field_3C[9];
    PanelQuad *field_60;
    PanelQuad *field_64;
    PanelQuad *field_68;
    PanelQuad *field_6C;
    PanelQuad *field_70;
    PanelQuad *field_74;
    FEText *field_78[9];
	FEText *field_9C;
    FEText *field_A0;
    FEText *field_A4;
    FEText *field_A8;
    FEMenuSystem *field_AC;
    int field_B0;
    int field_B4;
    float field_B8[4];
    float field_C8[4];
    float field_D8[4];
    float field_E8[4];
	bool field_F8;
    int field_F9;

    //0x0060E590
    pause_menu_root(FEMenuSystem *a2, int a3, int a4);
	



    //0x0063B2E0
    //virtual
    void _Load();

    //0x00641BF0
    void update_switching_heroes();


    //0x006490A0
    //virtual
	
    void Update(Float a2);
	
	void OnCross(int a2);
	
	void sub_61C520();
	
	void run_script(const char* func_name);
	
	void OnStart(int a2);
	
	
	void handle_objectives(float* a1);

    static string_hash get_sound_hash(int flag, string_hash &hash, const char *name);

    void set_menu_state(int state);

    void transition_to_submenu(int target_state);
	
    void custom_pause_awards_menu(int a2);
	
    void activate_menu(int mode);
	
    void sub_61C610();
	
	void play_accept_sound(int flag, string_hash &hash); 
	
	void reset_widget_state(int widget_ptr); 
	
	void handle_restart_mission(Float a2); 
	
	void handle_skip_cutscene(Float a2); 

    void handle_switch_hero(Float a2); 

    void handle_confirmation_state(Float a2, int a3); 

    void handle_skip_confirmation();

    void handle_hero_toggle(); 

    void finalize_confirmation(); 

    void setup_confirmation_dialog(); 

    int *get_current_widget(); 

};

extern void pause_menu_root_patch();

extern pause_menu_root *& pause_menu_root_ptr;

struct pause_menu_root2 : FEMenu {
    bool field_2C;
    char field_2D;
    int field_30;
    int field_34;
    int field_38;
    PanelQuad *field_3C[9];
    PanelQuad *field_60;
    PanelQuad *field_64;
    PanelQuad *field_68;
    PanelQuad *field_6C;
    PanelQuad *field_70;
    PanelQuad *field_74;
    FEText *field_78[10];
    FEText *field_A0;
    FEText *field_A4;
    FEText *field_A8;
    FEMenuSystem2 *field_AC;
    int field_B0;
    int field_B4;
    float field_B8[4];
    float field_C8[4];
    float field_D8[4];
    float field_E8[4];
    bool field_F8;
	int field_F9;
    int field_FC;

    void OnUp(int a2);
	
	void _Load();

    void OnDown(int a2);
	
	void update_selected();

    void OnActivate();
	
	void sub_62A840();
	
	void OnDeactivate(FEMenu *a2);
	
	void Draw();
	
};


extern pause_menu_root2 *& pause_menu_root2_ptr;