#pragma once


#include "config.h"
#include "ai_std_hero.h"
#include "anchor_storage_class.h"
#include "float.hpp"
#include "game_axis.h"
#include "game_button.h"
#include "vector3d.h"

#include <cstdint>

struct actor;
struct conglomerate;

enum eHeroLocoMode {
    RUNNING = 1,
    CRAWLING = 2,
    SWINGING = 3,
    FALLING = 5,
    WEB_ZIP = 9,
};

struct ai_player_controller {
    std::intptr_t m_vtbl = 0x0;

    conglomerate *field_4[2];
    eHeroLocoMode m_spidey_loco_mode;
    eHeroLocoMode m_prev_spidey_loco_mode;
    int field_14;
    game_button gb_jump;
    game_button gb_swing_raw;
    game_button gb_attack;
    game_button gb_attack_secondary;
    game_button gb_grab;
    game_button gb_range;
    game_button field_150;
    game_button gb_camera_center;
    game_button field_1B8;
    game_button gb_swing;
    game_button field_220;
    game_button field_254;
    game_button field_288;

    game_axis field_2BC[6];
    bool field_3DC;
    bool field_3DD;
    vector3d field_3E0;
    vector3d field_3EC;
    float field_3F8;
    float field_3FC;
    vector3d field_400;
    float field_40C;
    int field_410;
    int field_414;
    int field_418;
    int field_41C;
    // Native gameplay family; use find_hero_type() for the distinct UI identity.
    hero_type_enum m_hero_type;

    //0x004728D0
    ai_player_controller(actor *a2);

    void lock_controls(bool a2);

    void unlock_controls(bool a2);

    void set_spidey_loco_mode(eHeroLocoMode a2);

    void force_always_camera_relative(bool a2) {
        this->field_3DD = a2;
    }

    //0x00449390
    hero_type_enum find_hero_type() const;
	
    // Constructor hook: preserves retail Venom missions/tokens for Carnage.
    hero_type_enum find_native_hero_type() const;


    //0x00449A90
    anchor_storage_class get_poleswing_anchor() const;

    //0x00449940
    int get_spidey_loco_mode() const;

    eHeroLocoMode get_prev_spidey_loco_mode() const {
        return this->m_prev_spidey_loco_mode;
    }

    //0x004696A0
    void set_player_num(int a2);

    void clear_controls();

    //0x00468FE0
    void remap_controls();

    //0x00449AD0
    game_button *get_gb_jump();

    //0x00449B10
    game_button *get_gb_attack();

    //0x00449B20
    game_button *get_gb_attack_secondary();

    //0x00449AF0
    game_button *get_gb_grab();

    //0x00449B00
    game_button *get_gb_range();

    //0x00449B30
    game_button *get_gb_camera_center();

    //0x00449B50
    game_button & get_gb_swing_raw();

    //0x00468E80
    void frame_advance(Float a2);

    //0x004495D0
    vector3d compute_left_stick_from_camera();

    //0x00457C20
    vector3d convert_left_stick_from_camera_space_to_world_space(bool a3);

    //0x00457930
    //virtual
    void update_controls(Float a2, bool a3);

    //virtual
    float get_motion_force();

    static bool is_a_controllable_mode(eHeroLocoMode a1)
    {
        return a1 != 14;
    }
};

extern void ai_player_controller_patch();
