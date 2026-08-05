#pragma once

#include "entity_base_vtable.h"

#include "entity_base_vhandle.h"
#include "string_hash.h"
#include "vector3d.h"

#include "config.h"

#include <cstdint>
#include <filesystem>
#include <vector>

struct po;
struct event;
struct conglomerate;
struct generic_mash_header;
struct generic_mash_data_ptrs;
struct mString;
struct time_interface;
struct sound_and_pfx_interface;
struct chunk_file;
struct damage_interface;
struct facial_expression_interface;
struct physical_interface;
struct skeleton_interface;
struct animation_interface;
struct script_data_interface;
struct decal_data_interface;
struct resource_key;
struct motion_effect_struct;

namespace ai {
struct ai_core;
}



// @todo: complete flags
enum EntityFlags : unsigned __int32
{
    FLAG_HAS_COLLISION                      = 0x2,
    FLAG_MODEL_DEPENDENT_PO                 = 0x4,
    FLAG_DISABLE_REGION_LINK                = 0x8,
    FLAG_FADE_DISTANCE_MASK                 = 0xF,
    FLAG_HAS_CHARACTER_COLLISION            = 0x10,
    FLAG_HAS_TERRAIN_COLLISION              = 0x20,
    FLAG_DIRTY                              = 0x40,
    FLAG_CAN_WALK_ON                        = 0x80,
    FLAG_RENDERABLE                         = 0x100,
    FLAG_IS_VISIBLE                         = 0x200,
    FLAG_HAS_BOX_TRIGGER                    = 0x400,
    FLAG_SCALE_VISUAL_RADIUS                = 0x800,
    FLAG_IS_ACTIVE                          = 0x2000,
    FLAG_COLLISION_ACTIVE                   = 0x4000,
    FLAG_CONGLOMERATE_MEMBER                = 0x8000,
    FLAG_TRAFFIC_LIGHT_0                    = 0x20000,
    FLAG_MISC_CAST_SHADOW                   = 0x40000,
    FLAG_ACTIVE_DYNAMIC_ENTITY              = 0x80000,
    FLAG_NO_COLLISION_GEOMETRY              = 0x200000,
    FLAG_CAMERA_FROZEN                      = 0x400000,
    FLAG_MISC_HIRES_SHADOW                  = 0x8000000,
    FLAG_IN_REGION_SYSTEM                   = 0x10000000,
    FLAG_HAS_COLLISION_GEO                  = 0x20000000,
};
enum EntityExtFlags : unsigned __int32
{
    EXTFLAG_NONE                            = 0x0,
    EXTFLAG_VISUAL_RADIUS_REDUCED           = 0x20,
    EXTFLAG_DIRTY                           = 0x40,
    EXTFLAG_FORCE_UPDATE                    = 0x100,
    EXTFLAG_UPDATE_VIA_REGIONLINK           = 0x2000,
    EXTFLAG_PHYSICS_SUSPENDED               = 0x4000,
    EXTFLAG_SCENE_OWNED                     = 0x8000,
    EXTFLAG_SIGNALLER_ONLY                  = 0x20000,
    EXTFLAG_NO_ZERO_VELOCITY                = 0x40000,
    EXTFLAG_DYNAMIC_COLLISION_ACTOR         = 0x80000,
    EXTFLAG_ALREADY_MOVED                   = 0x800000,
    EXTFLAG_VISUAL_RADIUS_MINIMAL           = 0x1000000,
    EXTFLAG_PHYSICS_MOVED                   = 0x2000000,
    EXTFLAG_MODEL_REL_PO                    = 0x8000000,
    EXTFLAG_DIRTY_ABS_PO                    = 0x10000000,
    EXTFLAG_DOES_NOT_MANAGE_ABS_PO          = 0x40000000,
    EXTFLAG_ACTOR_DELUXE                    = 0x80000000,

};
enum PhysicalFlags : unsigned __int32
{
    PHYSICAL_FLAG_NONE                      = 0x0,
    PHYSICAL_FLAG_ENABLED                   = 0x1,
    PHYSICAL_FLAG_HAS_GRAVITY               = 0x4,
    PHYSICAL_FLAG_NO_BOUNCE                 = 0x80,
    PHYSICAL_FLAG_TERRAIN_GROUNDED          = 0x200,
    PHYSICAL_FLAG_FORCE_FLOOR_EVAL          = 0x400,
    PHYSICAL_FLAG_KINEMATIC_OVERRIDE        = 0x800,
    PHYSICAL_FLAG_MANAGE_STANDING           = 0x1000,
    PHYSICAL_FLAG_IS_STANDING               = 0x8000,
    PHYSICAL_FLAG_BIPED_PHYSICS             = 0x80000,
    PHYSICAL_FLAG_SCENE_SYNC_A              = 0x10000000,
    PHYSICAL_FLAG_SCENE_SYNC_B              = 0x20000000,
    PHYSICAL_FLAG_NO_BOUNCE_RESPONSE        = 0x80000000,
};

enum CollisionGeomFlags : unsigned __int32
{
    COLGEOM_FLAG_NONE                       = 0x0,
    COLGEOM_FLAG_BLOCKS_MOVEMENT            = 0x1,
    COLGEOM_FLAG_BLOCKS_PROJECTILES         = 0x2,
    COLGEOM_FLAG_UNKNOWN_4                  = 0x4,
    COLGEOM_FLAG_UNKNOWN_8                  = 0x8,
    COLGEOM_FLAG_HAS_CHARACTER_COLLISION    = 0x10,
    COLGEOM_FLAG_UNKNOWN_20                 = 0x20,
    COLGEOM_FLAG_UNKNOWN_40                 = 0x40,
    COLGEOM_FLAG_UNKNOWN_80                 = 0x80,
    COLGEOM_FLAG_VISIBLE_FOR_DEBUG_DRAW     = 0x100,
    COLGEOM_FLAG_CUSTOM_MASK_200            = 0x200,
    COLGEOM_FLAG_CUSTOM_MASK_400            = 0x400,
    COLGEOM_SETUP_CAPSULE                   = 0x80000,
};
enum entity_flag_t {
    EFLAG_GRAPHICS_VISIBLE  = 0x200,
    EFLAG_MISC_CAST_SHADOW  = 0x40000,
    EFLAG_MISC_HIRES_SHADOW = 0x8000000,
};

enum entity_ext_flag_t {
    EFLAG_EXT_SIGNALLER_ONLY = 0x20000u,
    EFLAG_EXT_DOES_NOT_MANAGE_ABS_PO = 0x40000000u,
};

struct entity_base : entity_base_vtable {
    uint32_t field_4;
    uint32_t field_8;
    po *my_rel_po;
    string_hash field_10;
    po *my_abs_po;
    motion_effect_struct *field_18;
    entity_base_vhandle my_handle;
    entity_base *m_parent;
    entity_base *m_child;
    entity_base *field_28;
    int16_t proximity_map_cell_reference_count;
    uint8_t m_timer;
    _std::vector<entity_base *> *adopted_children;
    conglomerate *my_conglom_root;
    sound_and_pfx_interface *my_sound_and_pfx_interface;
    int16_t field_3C;
    int16_t field_3E;
    int8_t field_40;
    int8_t field_41;
    int8_t rel_po_idx;
    int8_t proximity_map_reference_count;

    entity_base() = default;

    //0x004F32C0
    entity_base(bool a2);

    //0x004F3400
    entity_base(const string_hash &a2, uint32_t a3, bool a4);

    bool has_region_idx() const;

    uint16_t get_region_idx() const
    {
        return this->field_3C;
    }

    uint16_t get_bone_idx() const;

    float sub_57CB80();

    bool has_model_po() const;

    po *get_model_po() const;

    ////// VIRTUAL FUNCTIONS: START //////

    //0x004F8FA0
    /* virtual */ ~entity_base();

    /* virtual */ int get_entity_size();

    bool has_sound_and_pfx_ifc();

    sound_and_pfx_interface *sound_and_pfx_ifc();

    //0x004F9020
    void release_mem();

    void load(chunk_file &a2);

    bool handle_load_chunk(chunk_file &, mString &);

    void read_enx(chunk_file &a1);

    bool handle_enx_chunk(chunk_file &, mString &);

    /* virtual */ vector3d get_last_position();

    float get_visual_radius();

    /* virtual */ vector3d get_visual_center();

    vector3d get_velocity();

    //0x004C0000
    void po_changed();

    void set_ext_flag_recursive_internal(entity_ext_flag_t f, bool a3);

    void set_flag_recursive(entity_flag_t a2, bool a3);

    void set_ext_flag_recursive(entity_ext_flag_t a2, bool a3);

    void set_active(bool a2);

    void set_visible(bool a2, bool a3);

    ai::ai_core *get_ai_core();

    bool is_hero();

    bool is_alive();

    /* virtual */ int get_flavor();

    /* virtual */ bool is_an_entity_base();

    bool is_a_signaller();

    bool is_an_entity();

    bool is_an_actor() const;

    bool is_a_conglomerate_clone();

    bool is_a_camera();

    bool is_a_station_camera();

    bool is_a_game_camera();

    bool is_a_glam_camera();

    bool is_a_marky_camera();

    bool is_a_mouselook_camera();

    bool is_a_sniper_camera();

    bool is_an_ai_camera();

    bool is_a_spiderman_camera();

    bool is_a_light_source();

    bool is_a_neolight();

    bool is_a_marker();

    bool is_a_parking_marker();

    bool is_a_water_exit_marker();

    bool is_a_rectangle_marker();

    bool is_a_cube_marker();

    bool is_a_crawl_marker();

    bool is_an_anchor_marker();

    bool is_a_line_marker_base();

    bool is_a_line_anchor();

    bool is_a_pole();

    bool is_an_ai_cover_marker();

    bool is_a_lensflare();

    bool is_a_manip_obj();

    bool is_a_switch_obj();

    bool is_a_mic();

    bool is_a_pfx_entity() const;

    bool is_an_item();

    bool is_a_handheld_item();

    bool is_a_gun();

    bool is_a_thrown_item();

    bool is_a_melee_item();

    bool is_a_visual_item();

    bool is_a_grenade();

    bool is_a_rocket();

    bool is_a_sky();

    bool is_a_beam();

    bool is_a_polytube();

    bool is_a_trigger();

    bool is_material_switching();

    bool has_time_ifc();

    time_interface *time_ifc();

    bool has_damage_ifc();

    damage_interface *damage_ifc();

    bool has_facial_expression_ifc();

    facial_expression_interface *facial_expression_ifc();

    bool has_physical_ifc();

    physical_interface *physical_ifc();

    bool has_skeleton_ifc();

    skeleton_interface *skeleton_ifc();

    bool has_animation_ifc();

    animation_interface *animation_ifc();

    bool has_script_data_ifc();

    script_data_interface *script_data_ifc();

    bool has_decal_data_ifc();

    decal_data_interface *decal_data_ifc();

    bool get_ifc_num(const resource_key &, float &, bool);

    bool set_ifc_num(const resource_key &, float, bool);

    bool get_ifc_vec(const resource_key &, vector3d &, bool);

    bool set_ifc_vec(const resource_key &, const vector3d &, bool);

    bool get_ifc_str(const resource_key &, mString &, bool);

    bool set_ifc_str(const resource_key &, const mString &, bool);

    void _un_mash(generic_mash_header *a1, void *a2, generic_mash_data_ptrs *a3);

    //0x004CB2F0
    /* virtual */ void un_mash(generic_mash_header *a1, void *a2, generic_mash_data_ptrs *a3);

    ////// VIRTUAL FUNCTIONS: END //////

    //0x004F36B0
    void raise_event(string_hash a2);

    //0x004E0E50
    void set_parent(entity_base *parent);

    //0x004BFF60
    bool is_a_parent(entity_base *a2);

    _std::vector<entity_base *> *get_adopted_children();

    bool empty_adopted_children() const;

    po &get_rel_po();

    auto *get_parent() {
        return this->m_parent;
    }

    auto get_my_handle() {
        return this->my_handle;
    }

    bool is_flagged(uint32_t a2) const;

    bool is_ext_flagged(uint32_t a2) const;

    bool is_flagged_in_the_moved_list() const;

    bool is_dynamic() const;

    bool is_walkable() const;

    bool playing_scene_anim() const;

    bool manage_abs_po();

    bool is_mashed_member();

    bool are_collisions_active() const;

    bool is_conglom_member() const {
        return ((this->field_4 & 0x8000) != 0);
    }

    bool is_a_conglomerate() const {
        return ((this->field_4 & 4) != 0);
    }

    //0x0048AC20
    po &get_abs_po();

    bool is_rel_po_dirty() const;

    void un_mash_start(generic_mash_header *a2, void *a3, generic_mash_data_ptrs *a4, void *a5);

    //0x004F3700
    int add_callback(string_hash a2,
                     void (*callback)(event *, entity_base_vhandle, void *),
                     void *a4,
                     bool a5);

    //0x004DB740
    void add_adopted_child(entity_base *child_arg);

    //0x004F3550
    void common_destruct();

    bool is_visible() const;

    //0x0040FCA0
    void set_fade_distance(Float a1);

    //0x0040FC80
    void on_fade_distance_changed(int value);

    //0x004CB450
    void on_fade_distance_changed_internal(int a2);

    //0x004DB590
    void update_abs_po(bool a2);

    void sub_4D3F60(entity_base *a2);

    void sub_4E0DD0();

    //0x004D3FB0
    void clear_parent(bool a1);

    //0x004D6050
    unsigned int compute_rel_po_from_model();

    entity_base_vhandle get_my_vhandle();

    //0x004F3800
    bool event_raised_last_frame(string_hash a2);

    //0x0048C5C0
    void set_abs_po(const po &the_po);

    const vector3d &get_rel_position();

    //0x0048AC00
    const vector3d &get_abs_position();

    entity_base *get_first_child();

    void enter_limbo();

    //0x004F3B80
    void exit_limbo();

    //0x004BFED0
    void dirty_family(bool a2);

    //0x004BFF20
    void dirty_model_po_family();

    inline auto &get_id() const {
        return this->field_10;
    }

    //0x004E09F0
    void look_at(const vector3d &a1);

    //0x00502B50
    entity_base *get_conglom_owner() const;

    void set_abs_position(const vector3d &pos);

    void set_timer(int new_timer);

    auto get_fade_group() const {
        return this->field_3E;
    }
};

extern void entity_teleport_abs_position(entity_base *a2, const vector3d &a3, bool a4);

extern void check_po(entity_base *);

extern int DEBUG_foster_conglom_warning;

// ---------------------------------------------------------------------------
// .ENT mods (entity_base.cpp)
//
// A .ent dropped under the mod root is a packer-produced entity mash image
// (generic_mash_header + object image, the same bytes a retail pack serves
// for RESOURCE_KEY_TYPE_ENTITY). enumerate_mods() validates+registers them
// via modEntRegister under to_hash(stem) — the stem is the entity class
// name, e.g. extra/GOBLIN.ent overrides the "goblin" template — and
// resource_manager::get_resource swaps the retail bytes for the mod image
// via modEntGetOverride at fetch time.
// ---------------------------------------------------------------------------

// Structural gate: header safety key, entity class_id, mash-data offset.
extern bool modEntImageUsable(const uint8_t *bytes, size_t size);

// Validate + register one .ent under to_hash(stem) (and its literal-hash
// stem, if the name parses as one). Called by enumerate_mods().
extern bool modEntRegister(const std::filesystem::path &path,
                           std::vector<uint8_t> &&fileData);

// Immortal writable image for the entity class hash, or nullptr. The engine
// mutates the image in place (IN_USE flag, vtable words) and clones it for
// repeat spawns, so the same copy is served for the mod's whole lifetime.
extern uint8_t *modEntGetOverride(uint32_t classHash, int *sizeOut);

//0x004E1290
extern void entity_set_abs_parent(entity_base *me, entity_base *parent);

//0x004E1490
extern void entity_report(entity_base *a1, const mString &a2, bool a3);

//0x004E1230
extern void entity_set_abs_position(entity_base *ent, const vector3d &pos);

//0x004E1100
extern void entity_set_abs_po(entity_base *ent, const po &the_po);

extern void entity_base_patch();
