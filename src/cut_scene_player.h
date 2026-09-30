#pragma once

#include "entity_base_vhandle.h"
#include "float.hpp"
#include "game_button.h"
#include "mvector.h"

#include <vector.hpp>

struct entity_base;
struct cut_scene;
struct nalStreamInstance;
struct cut_scene_segment;

struct cut_scene_player {
    cut_scene *current_cut_scene;
    mVector<cut_scene_segment>::iterator current_segment;
    int field_8;
    int field_C;
    char field_10;
    int field_14;
    int field_18;
    int field_1C;
    int field_20;
    int field_24;
    int field_28;
    int field_2C;
    int field_30;
    int field_34;
    int field_38;
    int field_3C;
    int field_40;
    int field_44;
    _std::vector<nalStreamInstance *> field_48;
    _std::vector<nalStreamInstance *> field_58;
    int field_68[5];
    void *field_7C;
    int field_80;
    int field_84;

    int field_88[12];

    int field_B8;
    _std::vector<entity_base_vhandle> field_BC;
    entity_base *field_CC;
    entity_base *field_D0;
    bool field_D4;
    bool field_D5;
    bool field_D6;
    bool field_D7;
    bool m_traffic_enabled;
    bool m_peds_enabled;
    int field_DC;
    bool field_E0;
    bool field_E1;
    bool field_E2;
    game_button field_E4;
    game_button field_118;
    int field_14C;
    int field_150;
    float field_154;
    float field_158;
    int field_15C;
    int field_160;
    int field_164;

    //0x0073F170
    cut_scene_player();

    //0x00737F60
    void advance_lip_syncing(Float a2);

    //0x007382E0
    bool advance_panel_anims(Float a2);

    //0x0073FFB0
    void clean_up_finished_segment();

    //0x00742190
    void play(cut_scene *a2);

    //0x007414E0
    void play_current_segment();

    bool frame_advance_lite(Float a2);

    //0x00741EC0
    void frame_advance(Float a2);

    //0x00502B00
    bool is_playing();

    //0x00740660
    void stop(cut_scene *a2);

    //0x0086DE50
    static void finalize();
};

//0x007411C0
extern cut_scene_player *g_cut_scene_player();

extern void cut_scene_player_patch();

// Installs only the V10 tracking-panel creation compatibility hook.
extern void cut_scene_player_xbpack_v10_patch();
