#include "actor.h"

#ifdef OPENUSM_XBPACK_MODE
#include "actor_xbpack.h"
#endif

#include "advanced_entity_ptrs.h"
#include "ai_player_controller.h"
#include "als_animation_logic_system.h"
#include "als_res_data.h"
#include "base_ai_core.h"
#include "base_ai_data.h"
#include "camera_anim_controller.h"
#include "character_anim_controller.h"
#include "colgeom_alter_sys.h"
#include "collision_capsule.h"
#include "colmesh.h"
#include "color.h"
#include "common.h"
#include "conglom.h"
#include "custom_math.h"
#include "damage_interface.h"
#include "entity_mash.h"
#include "exe_allocator.h"
#include "facial_expression_interface.h"
#include "func_wrapper.h"
#include "generic_anim_controller.h"
#include "item.h"
#include "interactable_interface.h"
#include "intraframe_trajectory.h"
#include "lego_map.h"
#include "memory.h"
#include "moved_entities.h"
#include "nal_skeleton.h"
#include "nal_system.h"
#include "ngl.h"
#include "ngl_mesh.h"
#include "ngl_support.h"
#include "oldmath_po.h"
#include "parse_generic_mash.h"
#include "ped_anim_controller.h"
#include "physical_interface.h"
#include "resource_manager.h"
#include "string_hash.h"
#include "trace.h"
#include "utility.h"
#include "variables.h"
#include "vtbl.h"

#include <list.hpp>

VALIDATE_SIZE(actor, 0xC0u);
VALIDATE_OFFSET(actor, adv_ptrs, 0x78);

static collision_free_state *& collision_free_states = var<collision_free_state *>(0x00968504);

actor::actor(const string_hash &a2, uint32_t a3) : entity(a2, a3)
{
    this->field_64 = 0;
    this->field_60 = 0;
    this->field_5C = 0;
    this->regions[0] = nullptr;
    this->regions[1] = nullptr;
    this->extended_regions = nullptr;
    this->field_58 = nullptr;
    this->field_7C = nullptr;

    this->set_colgeom(nullptr);

    this->common_construct();

}

actor::actor(int) : entity()
{
    this->field_10 = {};

    this->field_90.field_4 = 1;
    this->field_90.field_5 = 1;
    this->field_90.field_0 = nullptr;
    this->field_90.field_6 = -1;
    this->field_90.active_client_count = 0;
}

actor::~actor() {
    THISCALL(0x004F93A0, this);
}

void actor::common_construct()
{
    this->m_facial_expression_interface = nullptr;
    this->m_damage_interface = nullptr;
    this->m_physical_interface = nullptr;
    this->m_traffic_light_interface = nullptr;
    this->field_88 = 0;
    this->adv_ptrs = nullptr;
    this->anim_ctrl = nullptr;
    this->m_skeleton= nullptr;
    this->field_A4 = 0;
    this->m_resource_context = nullptr;
    this->field_A8[0] = 0;
    this->field_AC = {};
    this->field_B8 = 0;
}

collision_free_state *actor::get_last_collision_free_state() const
{
    auto v1 = this->field_A4;
    if ( v1 != 0 ) {
        return collision_free_states + v1;
    }

    return nullptr;
}

void actor::set_colgeom(collision_geometry *a2)
{
    this->colgeom = a2;
    this->set_flag_recursive(static_cast<entity_flag_t>(2), this->colgeom != nullptr);
}

int actor::get_entity_size() {
    return 192;
}

void actor::release_mem() {
    THISCALL(0x004F9410, this);
}

vector3d actor::get_velocity() {
    vector3d a2;
    this->get_velocity(&a2);

    return a2;
}

bool actor::has_traffic_light_ifc() {
    return this->m_traffic_light_interface != nullptr;
}

traffic_light_interface *actor::traffic_light_ifc() {
    return this->m_traffic_light_interface;
}

bool actor::has_skeleton_ifc() const {
    bool (__fastcall *func)(const void *) = CAST(func, get_vfunc(m_vtbl, 0x12C));
    return func(this);
}

color32 actor::_get_render_color() const
{
    TRACE("actor::get_render_color");

    color32 result = (this->adv_ptrs != nullptr && this->adv_ptrs->field_8 != nullptr
                        ? this->adv_ptrs->field_8->field_0
                        : color32 {255, 255, 255, 255}
                    );

    auto c = result.to_color();
    sp_log("result = %f %f %f %f", c.r, c.g, c.b, c.a);
    return result;
}

color32 * __fastcall actor_get_render_color(const actor *self, void *, color32 *out)
{
    *out = self->_get_render_color();
    return out;
}

float actor::_get_render_alpha_mod() const
{
    TRACE("actor::get_render_alpha_mod");

    float alpha_mod =  ( this->adv_ptrs != nullptr && this->adv_ptrs->field_8 != nullptr
                            ? this->adv_ptrs->field_8->field_14
                            : 1.0f
                        );

    sp_log("alpha_mod = %f", alpha_mod);
    return alpha_mod;
}

void actor::set_render_scale(const vector3d &s)
{
	assert(s.is_valid());

	this->create_adv_ptrs();
	if ( this->adv_ptrs->field_8 == nullptr )
	{
		auto *mem = mem_alloc(sizeof(advanced_entity_ptrs::render_data));
		this->adv_ptrs->field_8 = new (mem) advanced_entity_ptrs::render_data {};
	}

	this->adv_ptrs->field_8->m_scale = s;
}

vector3d actor::get_render_scale() const
{
	bool v1 = (this->adv_ptrs != nullptr
				&& this->adv_ptrs->field_8 != nullptr);
	return (v1
			? this->adv_ptrs->field_8->m_scale
			: vector3d {1.0, 1.0, 1.0}
			);
}

void actor::ifl_lock(int a2)
{
    bool (__fastcall *func)(void *, void *, int) = CAST(func, get_vfunc(m_vtbl, 0x268));
    func(this, nullptr, a2);
}

nal_anim_controller *actor::select_and_new_anim_controller(
        nalBaseSkeleton *the_skeleton,
        unsigned int a3)
{
    TRACE("actor::select_and_new_anim_controller");

    assert(the_skeleton != nullptr && "Skeleton passed to select_and_new_anim_controller should never be NULL.");

    if constexpr (0)
    {
        als::als_meta_anim_table_shared *a5 = nullptr;
        if ( this->is_a_conglomerate() )
        {
            auto *v5 = bit_cast<conglomerate *>(this)->get_my_als();
            if ( v5 != nullptr ) {
                a5 = v5->get_meta_anim_table();
            }
        }

        nal_anim_controller *result = nullptr;

        if ( the_skeleton->GetAnimTypeName() == tlFixedString {"Character"})
        {
            result = new character_anim_controller { 
                                  this,
                                  the_skeleton,
                                  a3,
                                  a5};
            this->anim_ctrl = result;
        }
        else if ( the_skeleton->GetAnimTypeName() == tlFixedString {"Ped"} )
        {
            result = new ped_anim_controller {this, the_skeleton, a3, a5};
            this->anim_ctrl = result;
        }
        else if ( the_skeleton->GetAnimTypeName() == tlFixedString {"Camera"} )
        {
            result = new camera_anim_controller {this, the_skeleton, a3, a5};
            this->anim_ctrl = result;
        }
        else
        {
            result = new generic_anim_controller {
                                this,
                                the_skeleton,
                                a3,
                                a5};

            this->anim_ctrl = result;
        }

        return result;
    }
    else
    {
        return (generic_anim_controller *) THISCALL(0x004CC470, this, the_skeleton, a3);
    }
}

void actor::allocate_anim_controller(unsigned int a2, nalBaseSkeleton *a3) {
    TRACE("actor::allocate_anim_controller");

    if constexpr (0) {
        if ( this->anim_ctrl == nullptr ) {
            if ( a3 != nullptr ) {
                this->select_and_new_anim_controller(a3, a2);
            } else {
                if ( this->m_skeleton == nullptr ) {
                    this->m_skeleton = nalGetSkeleton(tlFixedString {"entity"});
                }

                this->select_and_new_anim_controller(this->m_skeleton, a2);
            }
        }
    } else {
        THISCALL(0x004CC630, this, a2, a3);
    }
}

#include "resource_pack_slot.h"
#include "resource_directory.h"
#include "tlresource_location.h"

animation_controller::anim_ctrl_handle actor::play_anim(const string_hash &a3)
{
    TRACE("actor::play_anim");

    if ( this->anim_ctrl == nullptr ) {
        this->allocate_anim_controller(0u, nullptr);
    }

    assert(anim_ctrl != nullptr);
    this->anim_ctrl->play_base_layer_anim(a3, 0.0, 32u, true);

    {
        animation_controller::anim_ctrl_handle result{};
        result.field_0 = true;
        result.field_8 = this->anim_ctrl;
        return result;
    }
}

void actor::bind_to_scene_anim() {
    THISCALL(0x004EF400, this);
}

void actor::unbind_from_scene_anim(string_hash a3, string_hash a4) {
    THISCALL(0x004E2750, this, a3, a4);
}

float actor::get_floor_offset() {
    float __fastcall (*func)(void *) = bit_cast<decltype(func)>(0x004C0D90);

    return func(this);
}

bool actor::anim_finished(int) {
    return true;
}

void actor::invalidate_frame_delta() {
    THISCALL(0x004E3880, this);
}

void actor::set_frame_delta_no_update(const po &a2, Float a3) {
    THISCALL(0x004D6B60, this, &a2, a3);
}

bool actor::get_allow_tunnelling_into_next_frame() {
    return (bool) THISCALL(0x004CC940, this);
}

void *actor::find_like_item(vhandle_type<item> a2) {
    return (void *) THISCALL(0x004D2100, this, a2);
}

void actor::common_destruct() {
    THISCALL(0x004F5720, this);
}

void actor::cancel_animated_movement(const vector3d &a2, Float a3)
{
    TRACE("actor::cancel_animated_movement");

    if constexpr (1) {
        if (this->is_frame_delta_valid()) {
            auto &v4 = this->adv_ptrs->mi->field_0;

            vector3d pos = v4.get_position();

            if (a2 != ZEROVEC) {
                auto local_dot = dot(a2, pos);
                if (local_dot <= 0.0f) {
                    pos = a2 * (local_dot - a3);

                    if (pos.length2() > v4.get_position().length2()) {
                        pos = v4.get_position();
                    }
                }
            }

            vector3d v17 = this->get_abs_position() - pos;

            entity_set_abs_position(this, v17);

            auto tmp = v4.get_position() - pos;
            v4.set_position(tmp);
        } else {
            THISCALL(0x004E3970, this, &a2, a3);
        }
    }
}

void actor::get_velocity(vector3d *a2) {
    if constexpr (0) {
        vector3d *v5;

        auto *v3 = (actor *) this->m_parent;
        if (v3 != nullptr) {
            if (this->has_physical_ifc()) {
                auto *v4 = this->physical_ifc();
                v5 = a2;
                *a2 = v4->get_velocity();
                auto *v6 = this->adv_ptrs;
                if (v6 != nullptr) {
                    auto *v7 = v6->mi;
                    if (v7 != nullptr) {
                        if (v7->field_54) {
                            auto *v8 = this->physical_ifc();
                            v8->field_84.get_volatile_ptr();
                            auto *v9 = this->physical_ifc();
                            if (v9->field_84.get_volatile_ptr() != nullptr) {
                                auto *v10 = this->physical_ifc();
                                if (v10->field_84.get_volatile_ptr() == (entity *) v3) {
                                    auto *v11 = (const vector3d *) this->adv_ptrs->mi;
                                    auto v31 = 1.f / v11[5][1];
                                    auto v12 = v11[4] * v31;
                                    *a2 += v12;
                                }
                            }
                        }
                    }
                }
            } else {
                v5 = a2;
                *a2 = ZEROVEC;
            }

            po v35 = this->get_rel_po();
            for (; v3->m_parent != nullptr; v3 = (actor *) v3->m_parent) {
                v35 = v35.sub_4BAB00(v3->get_abs_po());
            }

            if (v3->is_an_actor()) {
                auto a3 = ZEROVEC;
                v3->get_velocity(&a3);

                vector3d v33 = ZEROVEC;

                if (v3->has_physical_ifc()) {
                    auto &v14 = v3->physical_ifc()->field_2C;

                    v33 = v14;
                }

                auto v18 = vector3d::cross(v35.get_position(), v33);

                *v5 += a3 + v18;
            }

        } else {
            if (this->has_physical_ifc()) {
                auto *v21 = this->physical_ifc();
                auto v22 = v21->get_velocity();

                *a2 = v22;

            } else {
                *a2 = ZEROVEC;
            }

            auto *v26 = this->adv_ptrs;
            if (v26 != nullptr) {
                auto *v27 = v26->mi;
                if (v27 != nullptr) {
                    if (v27->field_54) {
                        auto *v28 = this->adv_ptrs->mi;
                        auto v29 = 1.f / v28->field_40;

                        *a2 += v28->field_0.get_position() * v29;
                    }
                }
            }
        }

    } else {
        THISCALL(0x004E2EE0, this, a2);
    }
}

void actor::process_extra_scene_flags(unsigned int a2) {
    THISCALL(0x004FB960, this, a2);
}

bool actor::has_camera_collision() const
{
    auto *v1 = this->colgeom;
    return (v1 != nullptr) && this->are_collisions_active() && (v1->field_C & 0x10) != 0;
}

bool actor::has_entity_collision() {
    auto *v1 = this->colgeom;
    return v1 && this->are_collisions_active() && (v1->field_C & 2) != 0;
}

void actor::kill_interact_anim()
{
    if constexpr (1)
    {
        auto *v2 = this->anim_ctrl;
        if (v2 != nullptr && (v2->field_10 & 1) != 0)
        {
            void (__fastcall *finalize)(nal_anim_controller *, void *, bool) = CAST(finalize, get_vfunc(v2->m_vtbl, 0x0));
            finalize(v2, nullptr, true);
            this->anim_ctrl = nullptr;
        }

        if (this->is_a_conglomerate())
        {
            conglomerate *self = CAST(self, this);

            auto *v3 = self->field_114;
            if (v3 != nullptr)
            {
                auto *v4 = v3->field_8;
                if (v4 != nullptr) {
                    void (__fastcall *suspend_logic_system)(void *, void *, int) = CAST(suspend_logic_system, get_vfunc(v4->m_vtbl, 0x8));
                    suspend_logic_system(v4, nullptr, 0);
                }
            }
        }

    } else {
        THISCALL(0x004CC740, this);
    }
}

void actor::create_adv_ptrs()
{
	if ( this->adv_ptrs == nullptr )
	{
		auto *mem = mem_alloc(sizeof(advanced_entity_ptrs));
		this->adv_ptrs = new (mem) advanced_entity_ptrs {};

		this->set_ext_flag_recursive_internal(static_cast<entity_ext_flag_t>(0x2000000u), true);
	}
}

physical_interface *actor::physical_ifc() {
    physical_interface * (__fastcall *func)(actor *) = CAST(func, get_vfunc(m_vtbl, 0x128));

    return func(this);
}

void actor::create_physical_ifc() {
    this->m_physical_interface = new physical_interface(this);
}

void actor::destroy_player_controller()
{
    if constexpr (1)
    {
        auto *v2 = this->m_player_controller;
        if (v2 != nullptr)
        {
            void (__fastcall *finalize)(ai_player_controller *, void *, bool) = CAST(finalize, get_vfunc(v2->m_vtbl, 0x0));
            finalize(v2, nullptr, true);
        }

        this->m_player_controller = nullptr;

    } else {
        THISCALL(0x004C0D40, this);
    }
}

constexpr auto MASH_SYNC_TEST_VAL5 = 0x5BADF00D;

void actor::_un_mash(generic_mash_header *a3, void *a4, generic_mash_data_ptrs *a5)
{
    TRACE("actor::un_mash");

    if constexpr (0)
    {
        auto &v4 = a5;
        auto &v5 = a3;
        entity::un_mash(a3, a4, a5);
        this->common_construct();

        if ( this->is_a_conglomerate() ) {
            bit_cast<conglomerate *>(this)->create_parentage_tree();
        }

        if ( a3->is_flagged(1) ) {
            this->adv_ptrs = v4->get<advanced_entity_ptrs>();
            this->adv_ptrs->un_mash(a3, this, this->adv_ptrs, v4);
        } else {
            this->adv_ptrs = nullptr;
        }

        this->field_A4 = 0;

        this->set_ext_flag_recursive_internal(static_cast<entity_ext_flag_t>(0x2000000u), false);
        this->set_ext_flag_recursive_internal(static_cast<entity_ext_flag_t>(0x800000u), false);

        rebase(v4->field_4, 8u);

        auto *skel_name_id_ptr = (resource_key *)v4->field_4;
        resource_key skel_name_id {};
        skel_name_id = *v4->get_from_shared<resource_key>();

        assert(*skel_name_id_ptr == skel_name_id);

        if ( skel_name_id.is_set() ) {
            auto *context = resource_manager::get_resource_context();
            assert(context != nullptr);

            auto v12 = skel_name_id.m_hash;
            auto &res_dir = context->get_resource_directory();
            auto *resource = res_dir.get_tlresource(
               v12.source_hash_code,
               TLRESOURCE_TYPE_SKELETON);
            this->m_skeleton = bit_cast<nalBaseSkeleton *>(resource);
        } else {
            this->m_skeleton = nullptr;
        }

        if ( v5->is_flagged(2u) ) {
            rebase(v4->field_0, 4u);

            auto *v15 = v4->get<collision_capsule>();

            v15->m_vtbl = collision_capsule_v_table;
            assert(((int)a3) % 4 == 0);

            v15->un_mash(a3, v15, v4);
            this->set_colgeom(v15);
        } else if ( v5->is_flagged(4u) ) {
            rebase(v4->field_0, 4u);

            auto *v19 = v4->get<cg_mesh>();
            v19->m_vtbl = collision_mesh_v_table();
            assert(((int)a3) % 4 == 0);

            v19->un_mash(a3, v19, v4);

            this->set_colgeom(v19);
        } else {
            this->set_colgeom(nullptr);
        }

        if ( colgeom != nullptr ) {
            colgeom->owner = this;
        }

        if ( v5->is_flagged(8u) ) {
            tlFixedString a1 {};

            rebase(v4->field_4, 8u);

            a1 = *v4->get_from_shared<tlFixedString>();
            auto &v28 = this->field_90;
            auto *Mesh = nglGetMesh(a1, false);
            v28.set_mesh(Mesh);

            auto func = [](auto &v28) -> nglMesh ** {

                if ( v28.field_5 <= 1u ) {
                    return v28.field_0;
                }

                return (nglMesh **)v28->field_0[v28.field_4];
            };

            if ( func(v28) == nullptr ) {
                auto *__old_context = resource_manager::get_and_push_resource_context(RESOURCE_PARTITION_HERO);
                auto *v32 = nglGetMesh(a1, false);
                this->field_90.set_mesh(v32);
                resource_manager::pop_resource_context();

                assert(resource_manager::get_resource_context() == __old_context);

                if ( func(v28) == nullptr ) {
                    auto *v10 = a1.to_string();
                    printf("Mesh '%s' not found in packfile!", v10);
                }
            }
        } else {
            this->field_90.set_mesh(nullptr);
        }

        this->get_lego_map_root();

        auto sync_check = *v4->get<uint32_t>();
        assert(sync_check == MASH_SYNC_TEST_VAL5);

        auto v34 = *v4->get<bool>();
        rebase(v4->field_0, 4u);

        if ( v34 ) {
            auto v38 = *v4->get<int>();

            rebase(v4->field_0, 16u);

            rebase(v4->field_0, 4u);

            auto *v42 = v4->field_0;
            v4->field_0 += v38;

            global_transfer_variable_the_actor = this;

#if XBOX_RELEASE
            mash_info_struct a1 {2, v42, v38, true};
#else
            mash_info_struct a1 {v42, v38};
#endif
            a1.unmash_class(this->field_7C, nullptr);
            a1.construct_class(this->field_7C);
        }

        auto v61 = *v4->get<bool>();
        rebase(v4->field_0, 4u);

        if ( v61 ) {
            auto v44 = *v4->get<int>();

            rebase(v4->field_0, 16u);

            rebase(v4->field_0, 4u);

            auto *v48 = v4->field_0;
            v4->field_0 += v44;

            mash_info_struct a1 {v48, v44};

            a1.unmash_class(this->m_interactable_ifc, nullptr);
            a1.construct_class(this->m_interactable_ifc);

        }

        if (this->m_interactable_ifc != nullptr) {
            auto &v50 = this->m_interactable_ifc;
            v50->field_0 = this;
            v50->update_registrations();
        }

        auto a4a = *v4->get<bool>();
        rebase(v4->field_4, 4u);

        if ( a4a ) {
            rebase(v4->field_4, 4u);

            if ( (a3->field_E & 0x8000) != 0 ) {
                rebase(v4->field_4, 4u);

                this->m_facial_expression_interface = v4->get<facial_expression_interface>();
                this->m_facial_expression_interface->m_vtbl = ifc_v_table_lookup()[4];
                this->m_facial_expression_interface->un_mash(
                    a3,
                    this,
                    this->m_facial_expression_interface,
                    v4);
            } else {
                this->m_facial_expression_interface = nullptr;
            }
        }
#if 0

        v53 = *v4->field_0;
        v54 = v4->field_0 + 1;
        v55 = 4 - ((LOBYTE(v4->field_0) + 1) & 3);
        v4->field_0 = v54;

        if ( v55 < 4 )
            v4->field_0 = &v54[v55];

        if ( v53 )
        {
            if ( !this->base.base.base.m_vtbl->field_20C(this) )
                sub_4C0BA0(this);

            v56 = *(_DWORD *)v4->field_0;
            v4->field_0 += 4;
            this->base.base.base.m_vtbl->traffic_light_ifc(this)->field_C = v56;
        }

        v57 = 16 - ((int)v4->field_0 & 0xF);
        if ( v57 < 0x10 )
            v4->field_0 += v57;

        if ( v93 )
        {
            v58 = *(_DWORD *)v4->field_0;
            v59 = (int)(v4->field_0 + 4);
            v60 = 16 - ((LOBYTE(v4->field_0) + 4) & 0xF);
            v4->field_0 = (unsigned __int8 *)v59;
            if ( v60 < 0x10 )
                v4->field_0 = (unsigned __int8 *)(v60 + v59);

            v61 = 4 - ((int)v4->field_0 & 3);
            if ( v61 < 4 )
                v4->field_0 += v61;

            v62 = &v4->field_0[v58];
            a1.m_hash = (unsigned int)v4->field_0;
            v4->field_0 = v62;
            *(_DWORD *)a1.field_4 = 0;
            *(_DWORD *)&a1.field_4[4] = v58;
            *(_DWORD *)&a1.field_4[8] = 0;
            v63 = mash_info_struct::read_from_buffer((mash_info_struct *)&a1, 32, 4);
            this->field_88 = (web_interface *)v63;
            nullsub_1((mAvlTree__string_hash_entry *)v63, (mash_info_struct *)&a1, (int)v63);
            sub_5073A0(v63, (mash_info_struct *)&a1, (int)v63);
            sub_508320((int *)&this->field_88);
            v64 = this->field_88;
            v65 = v64->field_8;
            v64->field_14 = (int *)this;
            if ( v65 )
                v66 = &v65[v64->field_4];
            else
                v66 = 0;

            for ( ; v65 != v66; *v67 = this )
                v67 = (actor **)*v65++;

            v68 = this->field_88;
            mVector<interaction>::push_back((mVector__interaction *)&web_interface::m_all_web_interfaces, (trigger_region *)v68);
            if ( (v68->field_1C & 4) != 0 )
            {
                ai::player_web_target_inode::add_to_web_targets_list(v68->field_14[7]);
                LOBYTE(v68->field_1C) |= 2u;
            }
        }

        v69 = (int)(v4->field_0 + 4);
        v4->field_0 = (unsigned __int8 *)v69;
        v70 = a3->field_E;
        if ( (v70 & 0x1B) != 0 )
        {
            if ( (v70 & 0x10) != 0 )
            {
                v71 = 4 - (v69 & 3);
                if ( v71 < 4 )
                    v4->field_0 = (unsigned __int8 *)(v71 + v69);

                this->my_physical_interface = (physical_interface *)v4->field_0;
                v4->field_0 += 432;
                this->my_physical_interface->m_vtbl = (physical_interface__vtbl *)ifc_v_table_lookup[2];
                ((void (__stdcall *)(generic_mash_header *, actor *, physical_interface *, generic_mash_data_ptrs *))this->my_physical_interface->m_vtbl->field_1C)(
                a3,
                this,
                this->m_physical_interface,
                v4);
            }
            else
            {
                this->m_physical_interface = nullptr;
            }

            rebase(v4->field_0, 8u);

            if ( (a3->field_E & 8) != 0 ) {

                rebase(v4->field_0, 4u);

                this->m_damage_interface = (damage_interface *)v4->field_0;
                v4->field_0 += sizeof(damage_interface);
                this->m_damage_interface->m_vtbl = ifc_v_table_lookup[1];
                this->m_damage_interface->un_mash(
                    a3,
                    this,
                    this->m_damage_interface,
                    v4);
            }
            else
            {
                this->m_damage_interface = nullptr;
            }
        }

        if ( this->base.base.base.m_vtbl->has_physical_ifc(this) )
            this->base.base.base.field_4 |= 0x40u;

        v7 = this->base.base.base.m_vtbl->get_ai_core(this) == 0;
        v74 = this->base.base.base.field_4;
        if ( v7 )
            v75 = v74 & 0xFFFFFFFE;
        else
            v75 = v74 | 1;

        this->base.base.base.field_4 = v75;
        if ( this->base.colgeom )
            this->base.base.base.m_vtbl->set_collisions_active(this, 1, 1);

        if ( ((unsigned __int8 (__fastcall *)(actor *))this->base.base.base.m_vtbl->field_134)(this) )
            ((void (__fastcall *)(actor *, void *, DWORD, DWORD))this->base.base.base.m_vtbl->field_214)(this, 0, 0);

        if ( resource_context_stack.m_first && resource_context_stack.m_last - resource_context_stack.m_first )
            v76 = (resource_pack_slot *)*((_DWORD *)resource_context_stack.m_last - 1);
        else
            v76 = 0;
        v77 = this->field_7C;
        this->field_BC = v76;
        if ( v77 && (this->base.base.base.field_4 & 4) == 0 ) {
            base_ai_data::post_entity_mash(v77);
        }

        if ( this->base.base.base.m_vtbl->field_20C(this) )
        {
            v78 = this->base.base.base.m_vtbl->traffic_light_ifc(this);
            traffic_signal_mgr::add_traffic_light(&this->base, v78->field_C != 0);
        }

        if ( this->base.base.base.m_vtbl->is_renderable(this) && (this->base.base.base.field_4 & 0x208000) == 0 )
        {
            a3a = this->base.base.base.m_vtbl->get_visual_radius(this);
            v79 = distance_fader::estimate_fade_index_for_bounding_sphere(a3a);
            entity_base::on_fade_distance_changed(&this->base.base.base, v79);
        }

        this->field_A8[0] = *(_WORD *)v4->field_0;
        v80 = (__int16 *)(v4->field_0 + 2);
        v4->field_0 = (unsigned __int8 *)v80;
        if ( this->field_A8[0] )
        {
            this->field_A8[1] = *v80;
            v81 = v4->field_0 + 2;
            v4->field_0 = v81;
            LOWORD(this->field_AC.arr[0]) = *(_WORD *)v81;
            v82 = v4->field_0 + 2;
            v4->field_0 = v82;
            HIWORD(this->field_AC.arr[0]) = *(_WORD *)v82;
            v83 = v4->field_0 + 2;
            v4->field_0 = v83;
            LOWORD(this->field_AC.arr[1]) = *(_WORD *)v83;
            v84 = v4->field_0 + 2;
            v4->field_0 = v84;
            HIWORD(this->field_AC.arr[1]) = *(_WORD *)v84;
            v85 = v4->field_0 + 2;
            v4->field_0 = v85;
            LOWORD(this->field_AC.arr[2]) = *(_WORD *)v85;
            v86 = v4->field_0 + 2;
            v4->field_0 = v86;
            HIWORD(this->field_AC.arr[2]) = *(_WORD *)v86;
            v87 = v4->field_0 + 2;
            v4->field_0 = v87;
            LOWORD(this->field_B8) = *(_WORD *)v87;
            v88 = (int)(v4->field_0 + 2);
        }
        else
        {
            this->field_AC.arr[0] = *(float *)v80;
            v89 = (float *)(v4->field_0 + 4);
            v4->field_0 = (unsigned __int8 *)v89;
            this->field_AC.arr[1] = *v89;
            v90 = (float *)(v4->field_0 + 4);
            v4->field_0 = (unsigned __int8 *)v90;
            this->field_AC.arr[2] = *v90;
            v91 = (int *)(v4->field_0 + 4);
            v4->field_0 = (unsigned __int8 *)v91;
            this->field_B8 = *v91;
            v88 = (int)(v4->field_0 + 4);
        }

        v4->field_0 = (unsigned __int8 *)v88;
#endif
    } else {
        THISCALL(0x004FBD40, this, a3, a4, a5);
#ifdef OPENUSM_XBPACK_MODE
        actor_xbpack_finish(a5);
#endif
    }
}

void actor::create_player_controller(int a2) {
    assert(this->m_player_controller == nullptr);

    auto *mem = exe_allocator<ai_player_controller> {}.allocate(1);
    this->m_player_controller = new (mem) ai_player_controller{this};

    this->m_player_controller->set_player_num(a2);
}

static _std::list<actor::mesh_buffers *> & stru_95AAB4 = var<_std::list<actor::mesh_buffers *>>(0x0095AAB4);

void actor::swap_all_mesh_buffers()
{
    for (auto &i : stru_95AAB4)
    {
        actor::mesh_buffers *v2 = i;
        auto v3 = v2->field_5;
        if (v3 > 1u) {
            ++v2->field_4;

            v2->field_4 %= (int16_t) v3;
            nglCopyMesh(v2->field_0[v2->field_4], v2->field_0[v3]);
        }
    }
}

vector3d actor::get_colgeom_center() const
{
    void (__fastcall *func)(const actor *, void *, vector3d *) = CAST(func, get_vfunc(m_vtbl, 0x258));

    vector3d result;
    func(this, nullptr, &result);

    return result;
}

void actor::radius_changed(bool )
{
    this->set_ext_flag_recursive_internal(static_cast<entity_ext_flag_t>(0x40u), true);
}

lego_map_root_node *actor::get_lego_map_root() {
    return (lego_map_root_node *) THISCALL(0x00502C70, this);
}

void actor::_render(Float a2)
{
    TRACE("actor::render");

    sp_log("%f", float{a2});

    if constexpr (0)
    {
        auto *mesh = this->get_mesh();
        if (mesh != nullptr)
        {
            assert(mesh != nullptr && is_visible() && is_renderable());

            nglParamSet<nglShaderParamSet_Pool> ShaderParams{static_cast<nglParamSet<nglShaderParamSet_Pool>::nglParamSetType>(1)};

            if (this->is_material_switching())
            {
                auto *root_node = this->get_lego_map_root();
                if (root_node != nullptr) {
                    USMMaterialListParam list_param{root_node->field_4};
                    ShaderParams.SetParam(list_param);

                    USMMaterialIndicesParam indices_param{this->field_90.field_C};
                    ShaderParams.SetParam(indices_param);
                }
            }

            if ((this->field_90.field_6 & 0x3FFF) != 0x3FFF) {
                auto v11 = (this->field_90.field_6 & 0x3FFF);

                nglTextureFrameParam frame_param{v11};
                ShaderParams.SetParam(frame_param);
            }

            auto v12 = this->field_90.field_6 >> 14;
            auto v13 = 3 - v12;
            if (v12 != 3) {
                USDamageFrameParam frame_param{v13};
                ShaderParams.SetParam(frame_param);
            }

            if ( a2 != 1.f
                    || (this->adv_ptrs != nullptr && this->adv_ptrs->field_8 != nullptr)
                    )
            {
                auto v17 = this->get_render_color();
                color *v18 = new color {v17.to_color()};

                v18->a *= this->get_render_alpha_mod() * a2;

                nglTintParam param{(vector4d *) v18};
                ShaderParams.SetParam(param);
            }

            math::MatClass<4, 3> *v21 = bit_cast<decltype(v21)>(&this->get_abs_po());

            static nglMeshParams g_MeshParams{0x80000040};

            FastListAddMesh(mesh, *v21, &g_MeshParams, &ShaderParams);
        }
    }
    else
    {
        THISCALL(0x004E33B0, this, a2);
    }
}

damage_interface *actor::damage_ifc() {
    //return this->my_damage_interface;

    damage_interface * (__fastcall *func)(void *) = CAST(func, get_vfunc(m_vtbl, 0x118));
    return func(this);
}

void actor::create_damage_ifc()
{
    TRACE("actor::create_damage_ifc");

    assert(m_damage_interface == nullptr);

    auto *mem = mem_alloc(sizeof(damage_interface));
    this->m_damage_interface = new (mem) damage_interface {this};
}

float actor::get_colgeom_radius() const
{
    float (__fastcall *func)(const actor *) = CAST(func, get_vfunc(m_vtbl, 0x254));
    return func(this);
}

bool actor::is_frame_delta_valid() const {
    TRACE("actor::is_frame_delta_valid");

    if constexpr (1) {
        auto v1 = (this->adv_ptrs != nullptr && this->adv_ptrs->mi != nullptr );
        return v1 && this->adv_ptrs->mi->field_54;
    } else {
        bool (__fastcall *func)(const void *) = CAST(func, 0x004B8FC0);
        return func(this);
    }
}

movement_info *actor::get_movement_info() {
    if (this->adv_ptrs != nullptr) {
        return this->adv_ptrs->mi;
    }

    return nullptr;
}

po *actor::get_frame_delta() const
{
    if constexpr (1) {
        po *result = nullptr;

        if (this->adv_ptrs == nullptr || this->adv_ptrs->mi == nullptr) {
            result = &this->adv_ptrs->mi->field_0;
        } else {
            static po po_identity_matrix{};

            result = &po_identity_matrix;
        }

        return result;

    } else {
        return (po *) THISCALL(0x004B9000, this);
    }
}

void actor::set_frame_delta(const po &a2, Float a3)
{
    if ( a3 > 0.0f )
    {
        this->set_frame_delta_no_update(a2, a3);
        moved_entities::add_moved({this->my_handle});
    }
}

void actor::set_frame_delta_trans(const vector3d &a2, Float a3)
{
    void (__fastcall *func)(void *, void *, const vector3d *, Float) = CAST(func, get_vfunc(m_vtbl, 0x280));
    func(this, nullptr, &a2, a3);
}

vector4d __fastcall sub_503A90(void *a1, int, vector4d a2) {
    if constexpr (1) {
        struct {
            int field_0;
            int field_4;
            int16_t field_8[3];

        } *self = static_cast<decltype(self)>(a1);

        vector4d a3;

        a3[0] = self->field_8[0] * LARGE_EPSILON;

        a3[1] = self->field_8[1] * LARGE_EPSILON;

        a3[2] = self->field_8[2] * LARGE_EPSILON;

        a3[3] = 1.0;

        return sub_411750(a2, a3);

    } else {
        vector4d result;
        THISCALL(0x00503A90, a1, &result, a2);

        return result;
    }
}

vector3d sub_509170(entity_base *a2, unsigned int a3)
{
    vector3d result;

    if ((a3 & 0x20) != 0) {
        auto &v3 = a2->get_abs_po().get_x_facing();

        result = 4.5f * YVEC + v3 * 3.0f;

    } else if ((a3 & 0x1000000) != 0) {
        auto &v8 = a2->get_abs_po().get_z_facing();

        result = -2.0f * YVEC + v8 * 3.2f;

    } else if ( a2->is_flagged(0x800) ) {
        auto &v12 = a2->get_abs_po().get_z_facing();

        result = v12 * 2.f;

    } else {
        result = ZEROVEC;
    }

    return result;
}

bool actor::has_vertical_obb() {
    return this->field_B8 != 0;
}

vector3d *actor::get_cached_visual_bounding_sphere_center() {
    assert(!has_vertical_obb());

    return &this->field_AC;
}

vector3d actor::_get_visual_center()
{
    TRACE("actor::get_visual_center");

    if constexpr (0)
    {
        vector3d v6;

        if (this->get_mesh() != nullptr)
        {
            if (this->has_vertical_obb()) {
                auto v17 = sub_503A90(this->field_A8, 0, this->get_abs_po().m[3]);

                auto v10 = sub_509170(this, this->field_8);
                v6 = v17 + v10;

            } else {
                if (this->is_ext_flagged(0x40u)) {
                    this->field_8 &= 0xFFFFFFBF;

                    auto *Mesh = this->get_mesh();

                    this->field_AC[0] = Mesh->field_20[0];
                    this->field_AC[1] = Mesh->field_20[1];
                    this->field_AC[2] = Mesh->field_20[2];

                    assert(get_cached_visual_bounding_sphere_center()->is_valid());

                    auto &abs_po = this->get_abs_po();

                    this->field_AC = abs_po.slow_xform(this->field_AC);

                    assert(get_cached_visual_bounding_sphere_center()->is_valid());
                }

                auto v10 = sub_509170(this, this->field_8);
                v6 = this->field_AC + v10;
            }

        } else {
            auto v4 = this->get_abs_position();

            vector3d v5 = sub_509170(this, this->field_8);
            v6 = v5 + v4;
        }

        return v6;

    }
    else
    {
        vector3d result;
        THISCALL(0x004E31F0, this, &result);
        return result;
    }
}

bool actor::add_item(int a4, bool a6)
{
    return (bool) THISCALL(0x004E3B80, this, a4, a6);
}

void actor::add_collision_ignorance(entity_base_vhandle a2)
{
    TRACE("actor::add_collision_ignorance");

    THISCALL(0x004E2C10, this, a2);
}

nglMesh **actor::sub_4B8BCA() {
    return this->field_90.field_0;
}

nglMesh *actor::_get_mesh()
{
    if constexpr (0)
    {
        nglMesh *result;

        if (this->field_90.field_5 <= 1u)
            result = (nglMesh *) this->sub_4B8BCA();
        else
            result = this->field_90.field_0[this->field_90.field_4];
        return result;
    }
    else
    {
        nglMesh * (__fastcall *func)(void *) = CAST(func, 0x004B8BB0);
        return func(this);
    }
}

ai::ai_core *actor::_get_ai_core() {
    ai::ai_core *result = nullptr;

    auto *v1 = this->field_7C;
    if (v1 != nullptr) {
        result = v1->field_14;
    }

    return result;
}

void actor::get_animations(actor *a1, std::list<nalAnimClass<nalAnyPose> *> &a2)
{
    a2.clear();
    auto *v11 = a1->get_resource_context();
    if ( v11 != nullptr )
    {
        auto &res_dir = v11->get_resource_directory();
        auto tlresource_count = res_dir.get_tlresource_count(TLRESOURCE_TYPE_ANIM_FILE);
        for (auto idx = 0; idx < tlresource_count; ++idx)
        {
            auto *tlres_loc = res_dir.get_tlresource_location(idx, TLRESOURCE_TYPE_ANIM_FILE);
            auto *animFile = (nalAnimFile *) tlres_loc->field_8;
            if ( animFile->field_0 == 0x10101 )
            {
                for ( auto *anim = bit_cast<nalAnimClass<nalAnyPose> *>(animFile->field_34);
                        anim != nullptr;
                        anim = anim->field_4 ) {
                    a2.push_back(anim);
                }
            }
        }
    }
}

void actor::mesh_buffers::set_mesh(nglMesh *Mesh)
{
    TRACE("actor::mesh_buffers::set_mesh");

    THISCALL(0x004D6980, this, Mesh);
}

namespace ai {

void setup_hero_capsule(actor *act)
{
    if constexpr (1)
    {
        auto *core = act->get_ai_core();

        core->create_capsule_alter();
        auto *capsule_alter = core->field_70;
        auto *ctrl = act->m_player_controller;
        conglomerate *cngl = CAST(cngl, act);

        if (ctrl != nullptr && ctrl->m_hero_type == 2)
        {
            capsule_alter->set_avoid_floor(false);
            capsule_alter->set_avg_radius(0.64999998);
            capsule_alter->set_mode((capsule_alter_sys::eAlterMode) 3);

            auto *v4 = cngl->get_bone(bip01_l_calf(), true);
            capsule_alter->set_base_avg_node(0, v4, 0.5);
            auto *v5 = cngl->get_bone(bip01_r_calf(), true);
            capsule_alter->set_base_avg_node(1, v5, 0.5);
            auto *v6 = cngl->get_bone(bip01_pelvis(), true);
            capsule_alter->set_base_avg_node(2, v6, 1.0);
            capsule_alter->set_base_avg_node(3, nullptr, 0.0);
            auto *v7 = cngl->get_bone(bip01_head(), true);
            capsule_alter->set_end_avg_node(0, v7, 3.0);
            auto *v8 = cngl->get_bone(bip01_spine(), true);
            capsule_alter->set_end_avg_node(1, v8, 1.0);
            capsule_alter->set_end_avg_node(2, nullptr, 0.0);
        } else {
            set_to_default_capsule_alter(capsule_alter, cngl);
            capsule_alter->set_avoid_floor(false);
        }

    } else {
        CDECL_CALL(0x0068A440, act);
    }
}
} // namespace ai

void actor_patch()
{
    {
        FUNC_ADDRESS(address, &actor::select_and_new_anim_controller);
        REDIRECT(0x004CC64D, address);
        REDIRECT(0x004CC689, address);
    }
    {
        FUNC_ADDRESS(address, &actor::create_damage_ifc);
        SET_JUMP(0x004E2670, address);
    }

    {
        SET_JUMP(0x004B8D00, actor_get_render_color);
    }

    {
        FUNC_ADDRESS(address, &actor::_get_render_alpha_mod);
        SET_JUMP(0x004B8DF0, address);
    }

    {
        FUNC_ADDRESS(address, &actor::_un_mash);
        set_vfunc(0x00884304, address);
    }

    {
        FUNC_ADDRESS(address, &actor::_render);
        set_vfunc(0x0088434C, address);
    }

    {
        FUNC_ADDRESS(address, &actor::is_frame_delta_valid);
        SET_JUMP(0x004B8FC0, address);
    }

    {
        FUNC_ADDRESS(address, &actor::cancel_animated_movement);
        SET_JUMP(0x004E3970, address);
    }
    return;

    {
        FUNC_ADDRESS(address, &actor::_get_visual_center);
        set_vfunc(0x008841CC, address);
    }

    if constexpr (0) {
        REDIRECT(0x006A799C, &ai::setup_hero_capsule);

        {
            FUNC_ADDRESS(address, &actor::has_entity_collision);
            REDIRECT(0x00563817, address);
        }

        {
            FUNC_ADDRESS(address, &actor::kill_interact_anim);
            SET_JUMP(0x004CC740, address);
        }
    }
}
