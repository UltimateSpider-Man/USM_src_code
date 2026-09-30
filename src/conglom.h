#pragma once

#include "actor.h"
#ifdef OPENUSM_XBPACK_V10
#include "exe_allocator.h"
#endif
#include "mashable_vector.h"
#include "variable.h"

struct animation_interface;
struct skeleton_interface;
struct script_data_interface;
struct tentacle_interface;
struct decal_data_interface;
struct variant_interface;
struct als_res_data;
struct light_manager;
struct light_source;

namespace als {
struct animation_logic_system;
}

struct conglomerate : actor {
#ifdef OPENUSM_XBPACK_V10
    using actor_list_t = _std::list<actor *, exe_allocator<actor *>>;
    using light_list_t = _std::list<light_source *, exe_allocator<light_source *>>;
#else
    using actor_list_t = _std::list<actor *>;
    using light_list_t = _std::list<light_source *>;
#endif

    mashable_vector<entity_base *> members;
    mashable_vector<entity_base *> skin_bones;
    mashable_vector<po> member_abs_po;
    mashable_vector<po> all_rel_po;
    mashable_vector<po> all_model_po;
    mashable_vector<int8_t> field_E8;
    mashable_vector<int8_t> field_F0;
    light_manager *field_F8;
    actor_list_t *field_FC;
    light_list_t *field_100;
    int field_104;
    float field_108;
    float field_10C;
    uint32_t field_110;
    als_res_data *field_114;
    skeleton_interface *skeleton_ifc;
    animation_interface *field_11C;
    script_data_interface *script_data_ifc;
    tentacle_interface *field_124;
    decal_data_interface *my_decal_data_interface;
    variant_interface *m_variant_interface;

    conglomerate(const string_hash &a2, unsigned int a3);

    //0x004E5300
    bool render_complex_shadow(Float camera_distance);

    //0x004E4D80
    void render_simple_shadow(Float arg0, Float arg4);

    void debug_render();

    void create_parentage_tree();

    //0x004D2610
    void create_variant_ifc();

	void remove_member_lights_from_region(region *a2);

    //0x004D0590
    entity_base *get_member(const string_hash &a2, bool a3);

    //0x004CCC90
    entity_base *get_bone(const string_hash &a2, bool a3);

    void sub_4D0E00();

    als::animation_logic_system *get_my_als();

    //0x004F9F10
    //virtual
    void release_mem();

    //0x004FC830
    //virtual
    void _un_mash(generic_mash_header *a2, void *a3, generic_mash_data_ptrs *a4);

    //0x004F9930
    //virtual
    void _render(Float a2);

    //0x004D2670
    //virtual
    float get_colgeom_radius();

    //0x00509F10
    //virtual
    vector3d get_colgeom_center();

    //0x004C1970
    //virtual
    void radius_changed(bool a2);

    //virtual
    light_manager *get_light_set() const;

    bool has_tentacle_ifc();

    tentacle_interface *tentacle_ifc();

    bool has_variant_ifc();

    variant_interface *variant_ifc();

    void init_member_data();

    bool is_cloned_conglomerate() const
    {
        return (this->field_110 & 0x40) != 0;
    }
};

inline Var<actor *> global_transfer_variable_the_conglom {0x00959570};

extern void conglomerate_patch();
extern void conglomerate_xbpack_patch();
