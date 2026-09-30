#pragma once

#include "animation_controller.h"
#include "cached_special_effect.h"
#include "entity_base_vhandle.h"
#include "mission_manager.h"
#include "mission_stack_manager.h"
#include "mstring.h"
#include "patrol_manager.h"
#include "slot_pool.h"
#include "timed_progress.h"
#include "wds_ai_manager.h"
#include "wds_camera_manager.h"
#include "wds_entity_manager.h"
#include "wds_patrol_def_manager.h"
#include "wds_render_manager.h"
#include "wds_script_manager.h"
#include "wds_time_manager.h"
#include "wds_token_manager.h"

#include <vector.hpp>

struct box_trigger;
struct game_camera;
struct terrain;
struct camera;
struct entity;
struct nal_anim_control;
struct region;
struct worldly_pack_slot;
struct limited_timer;
struct force_generator;
struct item;

struct scene_brew;
struct scene_entity_brew;
struct scene_spline_path_brew;

inline constexpr auto MAX_GAME_PLAYERS = 1u;

struct world_dynamics_system {
    slot_pool<nal_anim_control *, uint32_t> *field_0;
    _std::vector<animation_controller *> field_4;
    wds_ai_manager field_14;
    int field_24;
    wds_camera_manager field_28;
    wds_entity_manager ent_mgr;
    wds_render_manager field_A0;
    int empty3[1];
    wds_script_manager field_140;
    wds_time_manager field_158;
    wds_patrol_def_manager field_178;
    wds_token_manager field_188;
    terrain *the_terrain;
    cached_special_effect field_1B0;
    cached_special_effect field_1F0;


private:
    entity *field_230[MAX_GAME_PLAYERS];
    camera *field_234[MAX_GAME_PLAYERS];

public:
    int num_players;
    _std::list<entity *> field_23C;
    _std::list<unsigned int> field_248;
    _std::list<unsigned int> field_254;
    _std::vector<force_generator *> field_260;
    int field_270;
    int field_274;
    int field_278;
    int field_27C;

    _std::vector<scene_brew> scene_loads;

    int field_290;
    char field_294;
    bool m_loading_from_scn_file;
    int field_298;
    mission_stack_manager field_29C;
    mission_manager field_2A8;
    patrol_manager field_3A8;
    mString field_3E0;
    bool field_3F0;
    int empty6[2];
    int field_3FC;

    //0x005554D0
    world_dynamics_system();

    //0x00555750
    ~world_dynamics_system();

    //0x00530460
    void malor_point(const vector3d &a2, int a3, bool a4);

    //0x005421B0
    int add_generator(force_generator *generator);

    //0x00537170
    void advance_entity_animations(Float a3);

    //0x00558370
    void frame_advance(Float a2);

    //0x0053CC90
    void register_water_exclusion_trigger(box_trigger *trig);

    //0x0055A680
    bool un_mash_scene_entities(const resource_key &a2, region *, worldly_pack_slot *, bool a5, scene_entity_brew &);

    //0x005507F0
    bool un_mash_scene_box_triggers(const resource_key &a1, region *a2, worldly_pack_slot *a3, timed_progress *a4);

    //0x0054A1C0
    bool un_mash_box_triggers(
        int parse_code,
        char *a3,
        _std::vector<box_trigger *> *box_trigger_vec_ptr,
        int *a5);

    //0x0052FC90
    bool un_mash_scene_spline_paths(const resource_key &a2,
                                    region *a3,
                                    worldly_pack_slot *a4,
                                    scene_spline_path_brew &brew);

    //0x0053CB50
    bool un_mash_scene_audio_boxes(const resource_key &key_id,
                                   region *reg,
                                   worldly_pack_slot *slot_ptr,
                                   timed_progress &a4);

    //0x0053CAC0
    bool un_mash_scene_quad_paths(const resource_key &key_id,
                                  region *reg,
                                  worldly_pack_slot *slot_ptr,
                                  timed_progress &a4);

    //0x0055B160
    bool load_scene(resource_key &a2,
                    bool a3,
                    const char *a4,
                    region *a5,
                    worldly_pack_slot *a6,
                    limited_timer *a7);

    auto *get_the_terrain() {
        return this->the_terrain;
    }

    int get_num_players() {
        return this->num_players;
    }

    bool is_entity_in_water(vhandle_type<entity> a1);

    void entity_sinks(vhandle_type<entity> a2);

    //0x0055B100
    terrain *create_terrain(const mString &a2);

    void set_chase_cam_ptr(int index, game_camera *a3);

    //0x0050D1F0
    camera *get_chase_cam_ptr(int a2);

    //0x0055B400
    int add_player(const mString &a2);

    //0x0050D320
    bool is_loading_from_scn_file();

    //0x00558550
    int remove_player(int player_num);

    //0x0054A430
    void create_water_kill_trigger();

    //0x0050D310
    entity *get_hero_or_marky_cam_ptr();

    //0x0054ABE0
    void activate_web_splats();

    //0x0054AD20
    void deactivate_web_splats();

    //0x0054AC80
    void activate_corner_web_splats();

    //0x0054AD80
    void deactivate_corner_web_splats();

    //0x00530100
    void update_ai_and_visibility_proximity_maps_for_moved_entities(Float a1);

    //0x0054A610
    void update_collision_proximity_maps_for_moved_entities(Float a1);

    //0x00529CC0
    void update_light_proximity_maps_for_moved_entities(Float a1);

    //0x00542160
    void add_anim_ctrl(animation_controller *a2);

    nal_anim_control *get_anim_ctrl(uint32_t a1);

    void sub_54A3B0();

    void sub_530460(const vector3d &a2, int visited_regions, bool a4);

    entity *get_hero_ptr(int index);
};

extern int get_hero_type_helper();

inline constexpr auto MAX_REGIONS_IN_ARRAY = 30u;

struct region_array {
    region *m_data[MAX_REGIONS_IN_ARRAY];
    int count;

    auto &operator[](int idx) {
        return m_data[idx];
    }

    bool contains(region *a2) const;

    void push_back(region *a2);

};

extern void build_region_list_radius(region_array *arr, region *reg, const vector3d &a3, Float a4, bool a5);

//0x00605470
extern void collide_all_moved_entities(Float a1);

//0x004D1A30
extern void zero_xz_velocity_for_effectively_standing_physical_interfaces();

//0x004F28B0
extern void manage_standing_for_all_physical_interfaces(Float a1);

//0x004D00E0
extern void cleanup_actor_scene_anim_state_hash();

extern world_dynamics_system *& g_world_ptr;

extern bool wds_xbpack_patch();
extern void world_dynamics_system_patch();

// Install only the duplicate/invalid player-removal guard.  The complete WDS
// patch set remains optional because it replaces several unrelated systems.
extern void world_dynamics_system_remove_player_patch();
