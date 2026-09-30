#include "ai_std_hero.h"

#include "variable.h"


#include "actor.h"
#include "ai_player_controller.h"
#include "ai_state_jump.h"
#include "ai_state_swing.h"
#include "ai_state_web_zip.h"
#include "als_animation_logic_system.h"
#include "als_inode.h"
#include "base_ai_core.h"
#include "colgeom_alter_sys.h"
#include "collide.h"
#include "collision_geometry.h"
#include "combat_state.h"
#include "common.h"
#include "conglom.h"
#include "custom_math.h"
#include "entity.h"
#include "entity_base_vhandle.h"
#include "from_mash_in_place_constructor.h"
#include "func_wrapper.h"
#include "glass_house_manager.h"
#include "hit_react_state.h"
#include "line_info.h"
#include "oldmath_po.h"
#include "param_list.h"
#include "physical_interface.h"
#include "physics_inode.h"
#include "pick_up_state.h"
#include "plr_loco_crawl_state.h"
#include "plr_loco_crawl_transition_state.h"
#include "put_down_state.h"
#include "trace.h"
#include "subdivision_node_obb_base.h"
#include "terrain.h"
#include "throw_state.h"
#include "utility.h"
#include "vector2d.h"
#include "vtbl.h"
#include "wds.h"

#include <cmath>

namespace ai {

VALIDATE_SIZE(hero_inode::internal, 0x124);
VALIDATE_SIZE(hero_inode, 0x24C);
VALIDATE_OFFSET(hero_inode, field_70, 0x70);

hero_inode::hero_inode(from_mash_in_place_constructor *a2) {
    THISCALL(0x006A1B10, this, a2);
}

bool sub_68A0B0(int a1)
{
    bool result;
    switch ( a1 )
    {
    case 4:
    case 6:
    case 7:
    case 8:
        result = true;
        break;
    default:
        result = false;
        break;
    }

    return result;
}

bool hero_inode::jump_can_go_to(string_hash a2) {
    TRACE("ai::hero_inode::jump_can_go_to", a2.to_string());

    if constexpr (1) {
        auto *v4 = this->field_8;
        auto *info_node = bit_cast<als_inode *>(v4->get_info_node(als_inode::default_id, true));
        if ( this->field_7C ) {
            return false;
        }

        if ( a2 == web_zip_state::default_id ) {
            auto v7 = this->field_50;
            if ( v7 != 13 && v7 != 12 ) {
                return true;
            }

            auto *v8 = this->field_8;
            auto *v9 = bit_cast<physics_inode *>(v8->get_info_node(physics_inode::default_id, true));
            auto vel = v9->field_1C->get_velocity();
            vector2d v10 = {vel.x, vel.z};
            auto len = v10.length();
            return len * 0.2f < -vel.y;
        }

        if ( (a2 == plr_loco_crawl_transition_state::default_id
            || a2 == plr_loco_crawl_state::default_id)
            && sub_68A0B0(this->field_50) )
        {
            auto v2 = this->field_70;
            if ( v2 < 0.30000001f ) {
                return false;
            }
        }

        static string_hash loco_allow_aerial_hit_react_id {int(to_hash("loco_allow_aerial_hit_react"))};

        auto v19 = 0;
        auto *v11 = this->field_8;
        auto optional_pb_int = v11->field_50.get_optional_pb_int(
                  loco_allow_aerial_hit_react_id,
                  v19,
                  nullptr);

        if ( a2 == hit_react_state::default_id && !optional_pb_int ) {
            return false;
        }

        if ( a2 == put_down_state::default_id ) {
            return false;
        }

        if ( a2 == swing_state::default_id ) {
            static string_hash cat_id_jump_to_swing {int(to_hash("Jump_To_Swing"))};

            auto v15 = 0.0f;
            auto cat_id = info_node->get_category_id(static_cast<als::layer_types>(0));
            auto *v13 = bit_cast<als::state_machine *>(info_node->field_1C->get_als_layer(static_cast<als::layer_types>(0)));
            auto &anim_handle = v13->get_anim_handle();
            if ( anim_handle.get_anim_ptr() ) {
                auto &v14 = v13->get_anim_handle();
                v15 = v14.get_anim_norm_time();
            }

            return cat_id != cat_id_jump_to_swing || v15 >= 1.0f;
        } else {
            auto *v16 = info_node->field_1C->get_als_layer(static_cast<als::layer_types>(0));
            return v16->is_interruptable();
        }
    } else {
        return THISCALL(0x006A6E70, this, a2);
    }
}

void hero_inode::_frame_advance(Float a2)
{
    TRACE("hero_inode::frame_advance");

    if constexpr (1)
    {
        auto *v5 = bit_cast<physics_inode *>(this->field_8->get_info_node(physics_inode::default_id, true));
        if (!this->field_1C)
        {
            setup_hero_capsule(this->field_C);
            this->field_1C = true;
        }

        auto *v6 = v5->field_C;

        auto v18 = v6->get_abs_po().get_position();

        this->field_C->get_primary_region();

        static constexpr float max_ground_dist {10.0};

        vector3d v19 = v18 - UP * max_ground_dist;

        vector3d v20, v21;
        if (find_intersection(v18,
                              v19,
                              *local_collision::entfilter_entity_no_capsules,
                              *local_collision::obbfilter_lineseg_test,
                              &v20,
                              &v21,
                              nullptr,
                              nullptr,
                              nullptr,
                              false)) {
            this->field_244 = v18.y - v20.y;
        } else {
            this->field_244 = max_ground_dist;
        }

        float v17 = 0.0;
        auto *player_controller = this->get_actor()->get_player_controller();
        auto &gb_swing = player_controller->get_gb_swing_raw();

        auto cond = gb_swing.is_pressed();
        if (cond)
        {
            auto v11 = [](game_button &self) -> float {
                return (self.is_flagged(0x20) ? 0.0f : self.field_1C);
            }(gb_swing);

            v17 = v11 + 0.050000001f;
        }

        this->field_238 += a2;

        if (player_controller->get_motion_force() > EPSILON) {
            this->field_238 = 0.0;
        }

        als::param_list v16;
        v16.add_param(als::param {17, v17});
        v16.add_param(als::param {19, this->field_238});
        v16.add_param(als::param {70, this->field_244});

        auto *v12 = bit_cast<als_inode *>(
                this->field_8->get_info_node(als_inode::default_id, true));
        auto *v13 = v12->field_1C->get_als_layer(static_cast<als::layer_types>(0));

        v13->set_desired_params(v16);

        cleanup_collision_lists();

    }
    else
    {
        THISCALL(0x006A7950, this, a2);
    }
}

bool hero_inode::is_a_crawl_state(string_hash a1, bool a2)
{
    auto result = (a1 == plr_loco_crawl_state::default_id);
    if (a2)
    {
        if (a1 == plr_loco_crawl_transition_state::default_id
                || result)
        {
            result = true;
        }
    }

    return result;
}

bool hero_inode::ought_to_jump_off_wall(line_info &a2) {
    return (bool) THISCALL(0x006A6280, this, &a2);
}

bool hero_inode::ought_to_stick_to_wall(line_info &a2, bool a3)
{
    if constexpr (0)
    {
        auto *v4 = this->get_actor();
        entity *v20 = 0;
        subdivision_node_obb_base *v21 = 0;

        auto &v5 = v4->get_abs_po();

        auto v6 = this->field_C->get_render_scale()[1] * 0.5f;
        auto v27 = UP * v6;

        a2.field_0 = v5.get_position() + v27;

        vector3d a1;
        vector3d a5;
        if (!find_sphere_intersection(a2.field_0,
                                      2.0,
                                      *local_collision::entfilter_entity_no_capsules,
                                      *local_collision::obbfilter_sphere_test,
                                      &a1,
                                      &a5,
                                      &v20,
                                      &v21) ||
            !glass_house_manager::is_point_in_glass_house(a1) || a5[1] > 0.73242188f)
        {
            return false;
        }

        a2.field_C = a1;

        auto a2a = (v20 ? v20->get_my_handle() : 0);

        a2.hit_entity = {a2a};

        a2.hit_norm = a5;

        a2.hit_pos = a1;

        a2.m_obb = v21;
        a2.collision = true;

        auto *v15 = this->field_C->physical_ifc();
        auto *v16 = v15->field_84.get_volatile_ptr();
        entity *v17 = v20;
        entity *v18 = v16;

        bool result = true;
        if (is_noncrawlable_surface(a2) || have_relative_movement(v18, v17)) {
            result = false;
        }

        return result;
    }
    else
    {
        return (bool) THISCALL(0x0069FAF0, this, &a2, a3);
    }
}

bool hero_inode::accept_crawl_spot(vector3d a1, vector3d a4) {
    return (bool) THISCALL(0x00693B00, this, a1, a4);
}

bool hero_inode::get_closest_corner(corner_info *a2, crawl_request_type a3)
{
    if constexpr (0)
    {}
    else
    {
        bool (__fastcall *func)(void *, void *edx, corner_info *a2, crawl_request_type a3) = CAST(func, 0x006A5CF0);
        return func(this, nullptr, a2, a3);
    }
}

bool hero_inode::run_is_eligible(string_hash a2)
{
    TRACE("hero_inode::run_is_eligible");

    if constexpr (1) {
        auto *v3 = this->field_8;
        auto *the_phys_inode = bit_cast<physics_inode *>(v3->get_info_node(physics_inode::default_id, true));
        this->field_4C = 0;
        if ( auto *phys_ifc = the_phys_inode->field_1C;
                phys_ifc->is_effectively_standing()
                    && phys_ifc->allow_manage_standing() )
        {
            if ( a2 != jump_state::default_id ) {
                this->field_58 = the_phys_inode->get_abs_position();
            }

            return true;
        }

        return false;

    } else {
        return THISCALL(0x006A7770, this, a2);
    }
}

static string_hash loco_allow_crawl_id {int(to_hash("loco_allow_crawl"))};

bool hero_inode::crawl_is_eligible(string_hash a2, bool a3)
{
    TRACE("hero_inode::crawl_is_eligible");

    if constexpr (1) {
        auto &v4 = this->field_8->field_50;
        if ( v4.get_pb_int(loco_allow_crawl_id) == 0 ) {
            return false;
        }

        auto is_eligible = this->crawl_is_eligible_internals(a2, a3);
        if ( is_eligible ) {
            this->field_1B0 = this->field_88.field_7C.field_0;
        }

        return is_eligible;
    } else {
        return (bool) THISCALL(0x006B0EB0, this, a2, a3);
    }
}

bool hero_inode::oldcrawl_is_eligible(string_hash a2, bool a3)
{
    TRACE("hero_inode::oldcrawl_is_eligible");

    return (bool) THISCALL(0x006B0E60, this, a2, a3);
}

void hero_inode::set_surface_info(const line_info &a2)
{
    this->field_1B0.copy(a2);
}

void hero_inode::update_wall_run_als_params()
{
    if constexpr (1)
    {
        auto *v2 = this->field_8;

        auto *v3 = (als_inode *) v2->get_info_node(als_inode::default_id, true);
        auto *v4 = this->get_actor();
        auto *v5 = v3;

        auto *v6 = this->get_actor();

        float v8 = -calculate_xz_angle_relative_to_local_po(v6->get_abs_po(),
                                                            YVEC,
                                                            v4->get_abs_po().get_z_facing());

        constexpr float flt_882080 = PI / 4.0;
        constexpr float flt_8A48CC = -flt_882080;

        v8 = std::clamp(v8, flt_8A48CC, flt_882080);

        float v12 = (v8 + flt_882080) / half_PI;

        als::param_list list;
        list.add_param({18, v12});

        auto *v9 = v5->field_1C->get_als_layer(static_cast<als::layer_types>(0));
        v9->set_desired_params(list);

    } else {
        THISCALL(0x006A67E0, this);
    }
}

void hero_inode::update_crawl_als_params()
{
    TRACE("hero_inode::update_crawl_als_params");

    THISCALL(0x006A63D0, this);
}

bool hero_inode::run_can_go_to(string_hash arg0) {
    if constexpr (1) {
        auto *v3 = this->field_8;

        v3->get_info_node(physics_inode::default_id, true);

        auto *v4 = this->field_8;

        auto *v5 = (als_inode *) v4->get_info_node(als_inode::default_id, true);
        if (arg0 == hit_react_state::default_id) {
            return true;
        }

        if (arg0 == throw_state::default_id &&
            (arg0 = v5->get_category_id(static_cast<als::layer_types>(0)), arg0 != cat_id_idle_walk_run())) {
            return false;
        }

        auto *v7 = v5->field_1C->get_als_layer(static_cast<als::layer_types>(0));
        return v7->is_interruptable();

    } else {
        return THISCALL(0x006A76D0, this, arg0);
    }
}

void hero_inode::engage_water_exit()
{
    TRACE("hero_inode::engage_water_exit");

    THISCALL(0x006944D0, this);
}

bool hero_inode::jump_is_eligible(string_hash a2)
{
    TRACE("hero_inode::jump_is_eligible");

    return THISCALL(0x006A7110, this, a2);
}

bool hero_inode::crawl_can_go_to(string_hash a2, string_hash a3)
{
    TRACE("hero_inode::crawl_can_go_to");

    if constexpr (1) {
        auto *v3 = this->field_8;
        auto *the_als_inode = bit_cast<als_inode *>(v3->get_info_node(als_inode::default_id, true));
        auto v5 = (a2 == jump_state::default_id
                    || a2 == combat_state::default_id
                    || a2 == web_zip_state::default_id
                    || a2 == hit_react_state::default_id
                    || a2 == pick_up_state::default_id
                    || a2 == plr_loco_crawl_state::default_id);

        bool result = false;
        if ( a3 == plr_loco_crawl_state::default_id && v5 )
        {
            result = (v5 && the_als_inode->is_layer_interruptable(static_cast<als::layer_types>(0)));
        }

        return result;
    } else {
        return THISCALL(0x006A6340, this, a2, a3);
    }
}

void hero_inode::set_jump_type(eJumpType a2, bool a3) {
    this->field_54 = static_cast<eJumpType>(a3 ? 21 : this->field_50);
    this->field_50 = a2;
}

void hero_inode::cleanup_crawl_collision_list()
{
    if ( this->nearby_crawl_collidables != nullptr ) {
        local_collision::destroy_primitive_list(&this->nearby_crawl_collidables);
    }

    assert(nearby_crawl_collidables == nullptr);
}

void hero_inode::cleanup_swing_collision_list()
{
    if ( this->nearby_swing_collidables != nullptr ) {
        local_collision::destroy_primitive_list(&this->nearby_swing_collidables);
    }
    
    assert(nearby_swing_collidables == nullptr);
}

void hero_inode::cleanup_collision_lists() {
    cleanup_crawl_collision_list();
    cleanup_swing_collision_list();
}

hero_type_enum hero_inode::get_hero_type() {
    if constexpr (0) {
        //return (hero_type_enum) get_hero_type_helper();
    } else {
        return (hero_type_enum) CDECL_CALL(0x0068A050);
    }
}

bool hero_inode::crawl_is_eligible_internals(string_hash a2, bool a3)
{
    TRACE("hero_inode::crawl_is_eligible_internals");

    if constexpr (0)
    {}
    else
    {
        bool (__fastcall *func)(void *, void *edx, string_hash a2, bool a3) = CAST(func, 0x006A6900);
        return func(this, nullptr, a2, a3);
    }
}

void hero_inode::clear_curr_ground()
{
    this->field_88.field_10[0] = 0.0;
    this->field_88.field_10[1] = 0.0;
    this->field_88.field_10[2] = 0.0;
    this->field_88.field_10[3] = 0.0;

    this->field_88.field_20.clear();
}

bool hero_inode::compute_curr_ground_plane(force_recompute_enum a2, Float a3)
{
    TRACE("hero_inode::compute_curr_ground_plane");

    if constexpr (0)
    {
        auto func = [](const line_info &self) -> vector4d
        {
            auto v3 = -dot(self.hit_norm, self.hit_pos);
            vector4d a2 {self.hit_norm, v3};
            return a2;
        };

        bool v3 = false;
        if ( a2 == 1 || bit_cast<vector3d *>(&this->field_88.field_10)->length2() < 0.000099999997 )
        {
            auto *v8 = this->get_actor();

            auto &abs_po = v8->get_abs_po();
            auto v42 = -abs_po.get_y_facing() * a3;

            line_info a1 {};
            a1.clear();

            auto *v10 = this->get_actor();
            auto &v11 = v10->get_abs_po();

            auto v45 = v11.get_y_facing() * 0.1f;

            auto *v12 = this->get_actor();
            auto v13 = v12->get_abs_position();

            a1.field_0 = v13 - v45;

            a1.field_C = a1.field_0 + v42;
            a1.check_collision(
                *local_collision::entfilter_entity_no_capsules,
                *local_collision::obbfilter_lineseg_test,
                nullptr);
            if ( !a1.collision || is_noncrawlable_surface(a1) )
            {
                a1.clear();
                auto *v19 = this->get_actor();
                auto &v20 = v19->get_abs_po();

                auto *v21 = this->get_actor();
                auto &v22 = v21->get_abs_po();
                vector3d v23 = v22.get_y_facing() + v20.get_z_facing();

                auto *v26 = this->get_actor();
                auto v40 = v23 * EPSILON;
                auto v28 = v26->get_abs_position();

                auto *v29 = this->get_actor();
                a1.field_0 = v28 - v40;

                vector3d v51 = a1.field_0 + v42;
                a1.field_C = v51;

                auto v31 = v29->get_abs_po().get_z_facing();
                vector3d v52 = -1.0f * (v31 * 2.5f);
                a1.field_C += v52;
                if ( !a1.check_collision(
                        *local_collision::entfilter_entity_no_capsules,
                        *local_collision::obbfilter_lineseg_test,
                        nullptr)
                        || is_noncrawlable_surface(a1) )
                {
                    this->clear_curr_ground();
                }
                else
                {
                    auto v34 = func(a1);
                    this->field_88.field_10 = v34;
                    this->field_88.field_20 = a1;
                    v3 = true;
                }
            }
            else
            {
                auto v17 = func(a1);
                this->field_88.field_10 = v17;
                this->field_88.field_20 = a1;
                v3 = true;
            }
        }

        return v3;
    }
    else
    {
        bool (__fastcall *func)(void *, void *edx, force_recompute_enum a2, Float a3) = CAST(func, 0x00698970);
        return func(this, nullptr, a2, a3);
    }
}

static const string_hash bip01_pelvis {int(to_hash("BIP01 PELVIS"))};

static const string_hash bip01_l_foot {int(to_hash("BIP01 L FOOT"))};

static const string_hash bip01_r_foot {int(to_hash("BIP01 R FOOT"))};

void shrink_capsule_for_slanted_surfaces(actor *act)
{
    if constexpr (1)
    {
        assert(act->is_a_conglomerate());

        auto *v1 = act->get_ai_core();
        v1->create_capsule_alter();
        auto *capsule_alter = act->get_ai_core()->field_70;
        assert(capsule_alter != nullptr);

        capsule_alter->set_mode((capsule_alter_sys::eAlterMode) 3);

        conglomerate *conglm_ptr = CAST(conglm_ptr, act);
        auto *v3 = conglm_ptr->get_bone(bip01_pelvis, true);
        capsule_alter->set_base_avg_node(1, v3, 1.0);
        capsule_alter->set_base_avg_node(2, nullptr, 0.0);

        auto *ctrl = act->m_player_controller;
        assert(ctrl != nullptr);

        if (ctrl->m_hero_type == 2) {
            capsule_alter->set_avg_radius(0.5);
        } else {
            capsule_alter->set_avg_radius(0.30000001);
        }

    } else {
        CDECL_CALL(0x0068A5F0, act);
    }
}

void extend_capsule_for_jump(actor *act)
{
    assert(act->is_a_conglomerate());

    ai_core *v1 = act->get_ai_core();
    v1->create_capsule_alter();
    auto *capsule_alter = act->get_ai_core()->field_70;
    capsule_alter->set_mode((capsule_alter_sys::eAlterMode) 3);

    conglomerate *conglom_ptr = CAST(conglom_ptr, act);
    auto *v3 = conglom_ptr->get_bone(bip01_l_foot, true);
    capsule_alter->set_base_avg_node(0, v3, 1.75);

    auto *v4 = conglom_ptr->get_bone(bip01_r_foot, true);
    capsule_alter->set_base_avg_node(1, v4, 1.75);

    auto *v5 = conglom_ptr->get_bone(bip01_pelvis, true);
    capsule_alter->set_base_avg_node(2, v5, 1.0);
    capsule_alter->set_base_avg_node(3, nullptr, 0.0);
}

} // namespace ai

bool have_relative_movement(entity *a1, entity *a2) {
    return (bool) CDECL_CALL(0x0069F9A0, a1, a2);
}

bool get_axis_correction_delta(const vector3d &a1, const vector3d &a2, float a3, vector3d *corrected_hit_pos)
{
    assert(corrected_hit_pos != nullptr);

    auto v25 = a3 * a2 + a1;
    auto v7 = -a3;
    auto arg4a = v7;

    vector3d a5, a6;
    auto check1 = find_intersection(
                    a1,
                    v25,
                    *local_collision::entfilter_entity_no_capsules,
                    *local_collision::obbfilter_lineseg_test,
                    &a5,
                    &a6,
                    nullptr,
                    nullptr,
                    nullptr,
                    false);
    auto v9 = v7 * a2[0];

    vector3d v27;
    v27[1] = v7 * a2[1];
    v27[2] = v7 * a2[2];

    v25[0] = v9 + a1[0];
    v25[1] = v27[1] + a1[1];
    v25[2] = v27[2] + a1[2];

    vector3d v29, v33;

    auto check2 = find_intersection(
                    a1,
                    v25,
                    *local_collision::entfilter_entity_no_capsules,
                    *local_collision::obbfilter_lineseg_test,
                    &v29,
                    &v33,
                    nullptr,
                    nullptr,
                    nullptr,
                    false);

    if ( !check1 && !check2 )
    {
        auto v11 = a2 * a3;
        auto v26 = v11 + a1;
        auto v13 = a2 * 0.15000001;

        vector3d v24 = a1 - v13;
        check1 = g_world_ptr->the_terrain->find_region(v24, nullptr)
            && find_intersection(
                        v24,
                        v26,
                        *local_collision::entfilter_entity_no_capsules,
                        *local_collision::obbfilter_lineseg_test,
                        &a5,
                        &a6,
                        nullptr,
                        nullptr,
                        nullptr,
                        false);

        auto v15 = arg4a * a2;

        v26 = v15 + a1;

        v24 = a2 * 0.15000001 + a1;
        check2 = g_world_ptr->the_terrain->find_region(v24, nullptr)
            && find_intersection(
                v24,
                v26,
                *local_collision::entfilter_entity_no_capsules,
                *local_collision::obbfilter_lineseg_test,
                &v29,
                &v33,
                nullptr,
                nullptr,
                nullptr,
                false);
    }

    if (check1 && check2) {
        return false;
    }

    if ( check1 != check2 )
    {
        if ( check1 )
        {
            auto v22 = a2 * a3;
            v27 = a5 - v22;

            *corrected_hit_pos = v27;
        }
        else
        {
            assert(check2);

            auto v19 = a2 * a3;
            auto v27 = v19 + v29;

            *corrected_hit_pos = v27;

            *corrected_hit_pos = *corrected_hit_pos - a1;
        }
    }

    return true;
}

bool is_noncrawlable_surface(line_info &a1)
{
    if constexpr (0)
    {
        auto v_hit_entity = a1.hit_entity;
        if ( v_hit_entity.get_volatile_ptr() != nullptr )
        {
            assert(v_hit_entity.get_volatile_ptr()->is_an_actor());

            auto *act = bit_cast<actor *>(v_hit_entity.get_volatile_ptr());
            if (act->is_a_car()) {
                return true;
            }

            if ( !(!act->is_ext_flagged(0x8000u))) {
                return true;
            }

            if ( auto *colgeom = act->get_colgeom(); colgeom != nullptr
                || colgeom->get_type() == 1 )
            {
                return true;
            }
        }

        return ( a1.m_obb != nullptr && a1.m_obb->is_flagged(0x40) );
    }
    else
    {
        bool (*func)(line_info *a1) = CAST(func, 0x0068A9D0);
        return func(&a1);
    }
}


void hero_inode_patch()
{
    {
        FUNC_ADDRESS(address, &ai::hero_inode::_frame_advance);
        set_vfunc(0x0087DAC0, address);
    }

    {
        FUNC_ADDRESS(address, &ai::hero_inode::crawl_is_eligible);
        //SET_JUMP(0x006B0EB0, address);
    }

    {
        FUNC_ADDRESS(address, &ai::hero_inode::run_is_eligible);
        SET_JUMP(0x006A7770, address);
    }

    {
        FUNC_ADDRESS(address, &ai::hero_inode::crawl_can_go_to);
        SET_JUMP(0x006A6340, address);
    }

    {
        FUNC_ADDRESS(address, &ai::hero_inode::engage_water_exit);
        REDIRECT(0x00478EAA, address);
    }

    {
        FUNC_ADDRESS(address, &ai::hero_inode::jump_can_go_to);
        SET_JUMP(0x006A6E70, address);
    }

   
{
    void *address =
        reinterpret_cast<void *>(&ai::hero_inode::get_hero_type);

    REDIRECT(0x006EC609, address);
}
		
}
