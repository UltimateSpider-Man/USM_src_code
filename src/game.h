#pragma once

#define DEBUG_MENU_REIMPL 1
#define ENABLE_DEBUG_MENU 1
#define WINDOWED_MODE_WND_FIX 1
#define FORCE_MIPS          0
#define MOD_MESH_SUPPORT            1
#define MOD_MESH_DBG_REPLACE_ALL    0

// 60 FPS patch master switch.
// Set to 0 to compile the patch out entirely (binary identical to stock).
// Set to 1 to compile it in; runtime behaviour is then gated by
// g_config.EnableHighFPS so it can still be toggled without a rebuild.
#define FPS_UNLOCK_60       1

#include "color32.h"
#include "float.hpp"
#include "game_button.h"
#include "limited_timer.h"
#include "mstring.h"
#include "rumble_struct.h"
#include "vector3d.h"
#include "fixedstring.h"

#include <vector.hpp>

struct game_settings;
struct message_board;
struct world_dynamics_system;
struct entity_base;
struct localized_string_table;
struct game_process;
struct camera;
struct input_mgr;
struct mic;
struct nglMesh;
struct vector2di;
struct resource_key;
struct level_descriptor_t;

enum class game_state
{
    LEGAL = 1u,
    SPLASH_SCREENS = 2u,
    LOADING_SCREENS = 3u,
    WAIT_LINK = 4u,
    LOAD_LEVEL = 5u,
    RUNNING = 6u,
    PAUSED = 7u,
    TAKE_SCREENSHOT = 8u,
    SAVE_SCREENSHOT = 9u,
    UNHANDLED_UNK10 = 10u,
    PAUSE_AND_GO_NEXT_STATE = 11u,
    GO_NEXT_STATE = 12u,
    GO_NEXT_STATE2 = 13u,
    POP_PROCESS = 14u,
};



struct game;

//0x006063C0
extern void game__setup_input_registrations(game *a1);

//0x00605950
extern void game__setup_inputs(game *a1);

struct game {
    struct level_load_stuff {
        level_descriptor_t *descriptor;
        mString name_mission_table;
        mString field_14;
        vector3d field_24;
        int field_30;
        limited_timer_base field_34;
        bool load_widgets_created;
        bool load_completed;
        bool field_3A;
        bool field_3B;

        //0x00561EA0
        level_load_stuff();

        void reset_level_load_data();

        //0x0050B5F0
        bool wait_for_mem_check();

        //0x0055A1C0
        void look_up_level_descriptor();

        //0x0051D260
        void construct_loading_widgets();

        //0x0050B560
        void destroy_loading_widgets();
    };

    struct flag_t {
        bool level_is_loaded;
        bool single_step;
        bool physics_enabled;
        bool field_3;
        bool game_paused;
    };

    char field_0;
    char field_1;
    char field_2;
    mString field_4;
    level_load_stuff level;
    world_dynamics_system *the_world;
    int field_54;
    int field_58;
    camera *field_5C;
    entity_base *current_game_camera;
    mic *field_64;
    message_board *mb;
    _std::vector<game_process> process_stack;
    localized_string_table *field_7C;
    game_button field_80;
    nglMesh *field_B4;
    nglMesh *field_B8;
    char field_BC;
    char empty3[3];
    game_settings *gamefile;
    int empty4[30];
    float field_13C;
    float field_140;
    int field_144;
    char field_148;
    char field_149;
    char field_14A;
    float field_14C;
    float field_150;
    int field_154;
    bool field_158;
    bool field_159;
    bool field_15A;
    bool field_15B;
    bool field_15C;
    bool field_15D;
    bool field_15E;
    bool field_15F;
    bool field_160;
    bool field_161;
    bool field_162;
    bool field_163;
    bool field_164;
    bool field_165;
    bool field_166;
    bool field_167;
    flag_t flag;
    bool field_16D;
    bool field_16E;
    bool m_hero_start_enabled;
    bool field_170;
    bool field_171;
    bool m_user_camera_enabled;
    bool field_173;
    vector3d field_174;
    vector3d field_180[10];
    vector3d field_1F8[10];
    int field_270;
    float field_274;
    float field_278;
    int field_27C;
    int field_280;
    float field_284;
    int field_288;
    float field_28C;
    rumble_struct field_290;
    bool field_2B4;
    bool field_2B5;
    int field_2B8;
    int field_2BC;
    limited_timer_base field_2C0;

    //0x00557610
    game();

    //0x00559D10
    ~game();

    void enable_user_camera(bool a2)
    {
        this->m_user_camera_enabled = a2;
    }

    bool is_user_camera_enabled() const {
        return m_user_camera_enabled;
    }

    game_settings *get_game_settings();




    void go_next_state();

    game_state get_cur_state() const;

	    void handle_frame_locking(float *a1);

    float handle_frame_locking2(float time_inc);

	float handle_frame_locking_advanced(float time_inc);

	float handle_frame_locking_simple(float time_inc);


    //0x00548C10
    void begin_hires_screenshot(int a2, int a3);

    //0x00515230
    void enable_physics(bool a2);

    //0x0054E1B0
    void render_world();

    //0x00558100
    void advance_state_legal(Float a2);

    //0x00545B00
    void pop_process();

    //0x00514FD0
    void set_current_camera(camera *a2, bool a3);

    //0x00552E50
    void one_time_init_stuff();

    bool level_is_loaded() const;

    bool is_paused() const;

    bool is_physics_enabled() const;

    bool is_marky_cam_enabled() const;

    //0x0054FBE0
    void pause();

    //0x0053A880
    void unpause();

    //0x00558220
    void advance_state_paused(Float a1);

    //0x00524100
    void one_time_deinit_stuff();

    //0x00515140
    void clear_screen();

    //0x00515010
    void frame_advance_game_overlays(Float a1);

    //0x0054F860
    void message_board_init();

    void message_board_clear();

    //0x00559FA0
    void advance_state_running(Float a2);

    //0x00536410
    void advance_state_wait_link(Float a2);

    //0x00552F50
    void handle_cameras(input_mgr *a2, const Float &a3);

    //0x0055D510
    void handle_game_states(const Float &a2);

    //0x00514AB0
    void init_motion_blur();

    //0x00514F00
    void freeze_hero(bool a2);

    //0x0055C6E0
    void load_this_level();

    //0x0055D780
    void frame_advance(Float a2);

    //0x0055D650
    void frame_advance_level(Float time_inc);

    //0x00557E10
    void unload_current_level();

    //0x00514A50
    camera *get_current_view_camera(int a2);

private:
    void _load_new_level(const mString &a2);

public:
    //0x00514C40
    void load_new_level(const mString &a2, const vector3d &a3);

    //0x00514C70
    void load_new_level(const mString &a1, int a2);

    //0x0055D3A0
    void advance_state_load_level(Float a2);

    //0x0054F8C0
    void set_camera(int a2);

    //0x0055A420
    void load_hero_packfile(const char *str, bool a3);

    //0x00514EB0
    void setup_input_registrations();

    //0x005241E0
    void enable_marky_cam(bool a2, bool a3, Float a4, Float a5);

    //0x00548C70
    void soft_reset_process();

    //0x00514AA0
    static void load_complete();

    void push_lores();

    void push_process(game_process &process);

	void render_bar_of_shame();

    //0x00557B80
    void render_intros();

    void setup_inputs();

    //0x00558320
    void unload_hero_packfile();

    //0x00521610
    void render_motion_blur();

    //0x005244E0
    float get_script_game_clock_timer() const;

    //0x00524290
    void render_interface();

    //0x005242D0
    mString get_hero_info() const;

    //0x0052E0D0
    mString get_camera_info() const;

    mString get_analyzer_info() const;

    void show_debug_info() const;

    // Renders each loaded region's district name as a 3D billboard
    // at its OBB centre. Active when SHOW_DISTRICTS is set via the
    // "Show Districts" entry in the Game flags menu.
    void render_district_labels();

    //0x00514D00
    void show_max_velocity();

    //0x005244B0
    void reset_control_mappings();

    //0x0052B250
    void render_ui();

    //0x0051D1C0
    bool is_button_pressed(int a4) const;

    void sub_5580F0();

    void sub_524170();

    void sub_559F50(Float *a1);

    void sub_5241D0(Float a1);

    //0x00510780
    static void render_empty_list();

    //0x0095C8F8
    static inline auto & setup_input_registrations_p = var<int (*)(game *)>(0x0095C8F8);

    //0x0095C8FC
    static inline void (* setup_inputs_p)(game *) = game__setup_inputs;
};

extern game *& g_game_ptr;

//0x00581B40
extern void system_idle();

//0x0054F6D0
extern void game_packs_modified_callback(_std::vector<resource_key> &a1);

extern void game_patch();
