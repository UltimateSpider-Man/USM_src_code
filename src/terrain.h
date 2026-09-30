#pragma once

#include "eligible_pack_streamer.h"
#include "fixedstring.h"
#include "ideal_pack_info.h"
#include "resource_pack_slot.h"
#include "avltree.h"
#include "vector3d.h"
#include "region_lookup_entry.h"

#include <list.hpp>
#include <vector.hpp>

struct actor;
struct entity;
struct limited_timer;
struct mString;
struct region;
struct vector3d;
struct proximity_map;
struct stack_allocator;
struct subdivision_node_obb_base;
struct traffic_path_graph;
struct traffic_path_brew;

struct pack_switch_info_t {
	eligible_pack *field_0;
	resource_pack_slot *field_4;
	int field_8;
};

struct terrain {
    region **regions;
    proximity_map *region_map;
    stack_allocator *region_proximity_map_allocator;
    int total_regions;
    struct {
        char field_0[34];
        short field_22;
        int field_24;
    } *strips;
    int total_strips;
    vector3d field_18;
    eligible_pack_streamer field_24;
    AvlTree<region_lookup_entry> field_5C;
    region *field_68;
    traffic_path_graph *traffic_ptr;
    _std::vector<pack_switch_info_t> field_70;
    _std::list<region *> field_80;

    //0x00559920
    terrain(const mString &a2);

    //0x0054E990
    ~terrain();

    void init_region_proximity_map();

    //0x0053FD90
    vector3d get_elevation_adv(
        vector3d &a1,
        vector3d &a4,
        actor *a5,
        entity **a6,
        subdivision_node_obb_base **a7,
        Float a8);

    //0x00544430
    float get_elevation(
        vector3d &a2,
        vector3d &a4,
        actor *exclude_self,
        entity **a6,
        subdivision_node_obb_base **a7,
        Float a8);

    //0x0054F380
    void update_region_pack_info();

    //0x00552C50
    void find_ideal_terrain_packs(_std::vector<ideal_pack_info> *a2);

    void unload_all_districts_immediate();

    //0x005571B0
    void unload_district_immediate(int a2);

    //0x0054EC50
    void find_ideal_terrain_packs_internal(const vector3d &a2,
                                           region *a3,
                                           _std::vector<region *> *a4,
                                           _std::vector<ideal_pack_info> *a5);

    //0x0055C3F0
    void start_streaming(void (*callback)(void));

    //0x00557480
    void set_district_variant(int a2, int a3, bool a4);

    void un_mash_audio_boxes(char *a1, int *a2, region *a3);

    //0x00514480
    void un_mash_region_paths(char *a1, int *a2, region *a3);

    //0x00514410
    bool un_mash_traffic_paths(char *a2, int *a3, region *reg, traffic_path_brew &a5);

    void force_streamer_refresh();

    void sub_557130();

    //0x00524030
    region *get_district(int a1);

    //0x005444F0
    void find_regions(const vector3d &a2, _std::vector<region *> *regions) const;

    //0x00523E70
    int add_strip(const mString &a2);

    //0x005148A0
    void unlock_district(int a2);

    region *get_region(int idx);

    auto get_num_regions() const {
        return total_regions;
    }

    //0x005145A0
    int find_strip(const mString &a2) const;

    //0x0054F670
    int get_region_index_by_name(const fixedstring<4> &a2) const;

    //0x0053FFA0
    void unlock_district_pack_slot(int slot_idx);

    bool is_district_pack_slot_locked(int slot_idx) const;

    //0x00556FF0
    void frame_advance(Float a2);

    _std::vector<region *> * get_region_info_for_point(vector3d a2);

    //0x0052DFF0
    region *find_region(const vector3d &a2, region *a3) const;

    //0x00534920
    region *find_region(string_hash a2) const;

    //0x00534890
    region *find_innermost_region(const vector3d &a1) const;

    //0x00523EC0
    region *find_outermost_region(const vector3d &a2) const;

    //0x0053FC90
    void register_region_change_callback(void (*a3)(bool, region *));

    void show_obbs();

    //0x00514310
    void un_mash_obb(char *a2, int *a3, region *reg);

    //0x00514380
    void un_mash_texture_to_frame(char *a2, int *a3, region *reg);

    //0x0055BFA0
    static bool district_construct_callback(resource_pack_slot::callback_enum reason,
                                            resource_pack_streamer *a2,
                                            resource_pack_slot *which_pack_slot,
                                            limited_timer *a4);

	static bool district_load_started_callback(resource_pack_slot::callback_enum a1,
											 resource_pack_streamer *a2,
											 resource_pack_slot *a3);

	static bool district_pre_destruct_callback(
		resource_pack_slot::callback_enum reason,
        resource_pack_streamer *a2,
        resource_pack_slot *which_pack_slot);


    //0x0055C350
    static bool district_load_callback(resource_pack_slot::callback_enum a1,
                                       resource_pack_streamer *a2,
                                       resource_pack_slot *a3,
                                       limited_timer *a4);

    //0x00552A20
    static bool district_destruct_callback(resource_pack_slot::callback_enum reason,
                                           resource_pack_streamer *a2,
                                           resource_pack_slot *which_pack_slot,
                                           limited_timer *a4);

    static inline constexpr auto STREAMING_BUBBLE_RADIUS2 = 16.0f;

    static inline auto & regions_for_point = var<_std::vector<region *> *>(0x0095C8D0);

    static inline auto & load_complete_callback = var<void (*)(void)>(0x0095C8C8);

    static inline auto & region_change_callbacks = var<_std::list<void (*)(bool, region *)> *>(0x0095C8CC);

    static inline float & MAX_STREAMING_DISTANCE = var<float>(0x00921DA4);
};

//0x
extern void find_ideal_terrain_packs_callback(_std::vector<ideal_pack_info> *a1);

extern void terrain_patch();

#ifdef OPENUSM_XBPACK_MODE
extern void terrain_xbpack_patch();
#endif
