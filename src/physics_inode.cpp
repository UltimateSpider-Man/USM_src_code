#include "physics_inode.h"

#include "actor.h"
#include "base_ai_core.h"
#include "common.h"
#include "config.h"
#include "entity_base.h"

#include "entity.h"
#include "func_wrapper.h"
#include "game.h"
#include "game_settings.h"
#include "oldmath_po.h"
#include "utility.h"
#include "vector3d.h"
#include "vtbl.h"

namespace ai {

VALIDATE_SIZE(physics_inode, 0x20);

physics_inode::physics_inode(from_mash_in_place_constructor *a2) : info_node(a2) {
    
}

po &physics_inode::get_abs_po() {
    auto *v1 = this->field_C;

    return v1->get_abs_po();
}

vector3d &physics_inode::get_y_facing() {
    auto &v1 = this->get_abs_po();
    return v1.get_y_facing();
}

vector3d &physics_inode::get_z_facing() {
    auto &v1 = this->get_abs_po();
    return v1.get_z_facing();
}

void physics_inode::setup_for_crawl_zip()
{
    this->field_1C->field_C &= ~0x400u;
    this->field_C->invalidate_frame_delta();
    this->field_1C->set_control_parent(nullptr);
    this->field_1C->suspend(true);
    this->field_1C->enable(false);
    this->set_collisions_active(true, true);
    this->field_1C->field_C &= ~0x200u;
    this->set_gravity(false);
    auto *v2 = this->field_C->physical_ifc();

    static const auto v3 = -YVEC;
    v2->set_current_gravity_vector(v3);
}

void physics_inode::setup_for_jump(const vector3d &a2)
{
    if constexpr (1)
    {
        this->field_C->set_collisions_active(true, true);
        this->field_1C->field_C &= 0xFFFFFBFF;
        this->field_1C->field_C &= 0xFFFFFDFF;

        this->field_1C->set_gravity(true);
        this->field_C->field_4 |= 0x40u;

        this->field_1C->suspend(false);
        this->field_1C->enable(true);
        auto *v3 = this->field_C->physical_ifc();
        v3->set_current_gravity_vector(a2);

    } else {
        THISCALL(0x00694A80, this, &a2);
    }
}

void physics_inode::set_gravity_multiplier(Float a2) {
    this->field_1C->m_gravity_multiplier = a2;
}

float physics_inode::get_gravity_multiplier() {
    return this->field_1C->m_gravity_multiplier;
}

vector3d physics_inode::get_velocity() {
    return this->field_1C->get_velocity();
}

void physics_inode::set_gravity(bool a2) {
    this->field_1C->set_gravity(a2);
}

void physics_inode::enable(bool a2) {
    this->field_1C->enable(a2);
}

void physics_inode::disable() {
    this->field_1C->enable(false);
}

void physics_inode::unsuspend() {
    this->field_1C->suspend(false);
}

void physics_inode::suspend() {
    this->field_1C->suspend(true);
}

void physics_inode::setup_for_bounce() {
    if constexpr (1) {
        this->field_1C->field_C &= 0xFFFFFBFF;
        this->field_C->invalidate_frame_delta();
        this->field_1C->set_control_parent(nullptr);
        this->set_gravity(true);
        this->suspend();
        this->disable();

        this->set_collisions_active(true, true);
        this->set_adv_standing(false);

        auto *v2 = this->field_C->physical_ifc();
        v2->set_current_gravity_vector(-YVEC);
    } else {
        THISCALL(0x00695030, this);
    }
}

void physics_inode::set_adv_standing(bool a2) {
    entity_base *v1 = this->field_C;
    if (a2) {
        v1->field_4 |= 0x200u;
    } else {
        v1->field_4 &= 0xFFFFFDFF;
    }
}

void physics_inode::set_always_standing(bool a2) {
    entity_base *v1 = this->field_C;
    if (a2) {
        v1->field_4 |= 0x400u;
    } else {
        v1->field_4 &= 0xFFFFFBFF;
    }
}

void physics_inode::setup_for_walk()
{
    if constexpr (1)
    {
        this->set_collisions_active(true, true);
        this->set_always_standing(false);
        this->set_adv_standing(false);
        this->set_gravity(true);
        this->set_stationary(false);
        this->unsuspend();
        this->enable(true);
        auto *v2 = this->field_C->physical_ifc();
        v2->set_current_gravity_vector(-YVEC);
        this->cleanup_from_swing();

    }
    else
    {
        THISCALL(0x00698E50, this);
    }
}

static const string_hash physics_normal_drag_id {int(to_hash("physics_normal_drag"))};

static const string_hash physics_gravity_normal_id {int(to_hash("physics_gravity_normal"))};

static const string_hash physics_normal_drag_down_scale_id {int(to_hash("physics_normal_drag_down_scale"))};

static const string_hash physics_normal_drag_up_scale_id {int(to_hash("physics_normal_drag_up_scale"))};

static const string_hash physics_normal_drag_min_speed_id {int(to_hash("physics_normal_drag_min_speed"))};

static const string_hash physics_normal_drag_horz_scale_id {int(to_hash("physics_normal_drag_horz_scale"))};

static constexpr auto g_normal_gravity = 3.75f;

static constexpr auto g_normal_air_res = 0.001f;

static constexpr auto g_normal_air_res_up_scale = 1.0f;

static constexpr auto g_normal_air_res_down_scale = 1.0f;

static float g_normal_air_res_horz_scale = 1.0f;

static float & g_normal_air_res_min_speed = var<float>(0x0096BE3C);

void physics_inode::cleanup_from_swing()
{
    if constexpr (1)
    {
        const auto &pb = this->field_8->field_50;

        auto v16 = pb.get_optional_pb_float(physics_normal_drag_id, g_normal_air_res, nullptr);

        auto v17 = pb.get_optional_pb_float(physics_normal_drag_min_speed_id,
                                            g_normal_air_res_min_speed,
                                            nullptr);

        auto v18 = pb.get_optional_pb_float(physics_normal_drag_horz_scale_id,
                                            g_normal_air_res_horz_scale,
                                            nullptr);

        auto v19 = pb.get_optional_pb_float(physics_normal_drag_up_scale_id,
                                            g_normal_air_res_up_scale,
                                            nullptr);

        auto v20 = pb.get_optional_pb_float(physics_normal_drag_down_scale_id,
                                            g_normal_air_res_down_scale,
                                            nullptr);

        auto *phys_ifc = this->field_C->physical_ifc();
        phys_ifc->field_15C = v16;

        phys_ifc->field_160 = v17;

        phys_ifc->field_164 = v18;

        phys_ifc->field_168 = v19;

        phys_ifc->field_16C = v20;


        this->field_1C->m_gravity_multiplier = pb.get_optional_pb_float(physics_gravity_normal_id,
                                                                         g_normal_gravity,
                                                                         nullptr);

    }
    else
    {
        THISCALL(0x00694D80, this);
    }
}

void physics_inode::apply_force_increment(const vector3d &a2,
                                          physical_interface::force_type a3,
                                          const vector3d &a4,
                                          int a5) {
    this->field_1C->apply_force_increment(a2, a3, a4, a5);
}

static float & g_swing_gravity = var<float>(0x00937D34);

static const string_hash physics_gravity_swing_id {int(to_hash("physics_gravity_swing"))};

static const string_hash physics_swing_drag_0_id {int(to_hash("physics_swing_drag_0"))};
static const string_hash physics_swing_drag_1_id {int(to_hash("physics_swing_drag_1"))};
static const string_hash physics_swing_drag_2_id {int(to_hash("physics_swing_drag_2"))};
static const string_hash physics_swing_drag_3_id {int(to_hash("physics_swing_drag_3"))};

static const string_hash physics_swing_drag_min_speed_0_id {int(to_hash("physics_swing_drag_min_speed_0"))};
static const string_hash physics_swing_drag_min_speed_1_id {int(to_hash("physics_swing_drag_min_speed_1"))};
static const string_hash physics_swing_drag_min_speed_2_id {int(to_hash("physics_swing_drag_min_speed_2"))};
static const string_hash physics_swing_drag_min_speed_3_id {int(to_hash("physics_swing_drag_min_speed_3"))};

static const string_hash physics_swing_drag_horz_scale_id {int(to_hash("physics_swing_drag_horz_scale"))};
static const string_hash physics_swing_drag_up_scale_id {int(to_hash("physics_swing_drag_up_scale"))};
static const string_hash physics_swing_drag_down_scale_id {int(to_hash("physics_swing_drag_down_scale"))};

static constexpr float g_swing_air_res = 0.022500001f;

void physics_inode::set_collisions_active(bool a1, bool a2) {
    this->field_C->set_collisions_active(a1, a2);
}

void physics_inode::setup_for_swing()
{
    if constexpr (0)
    {
        this->set_collisions_active(true, true);
        this->set_always_standing(false);
        this->set_adv_standing(false);
        this->set_gravity(true);
        this->set_stationary(false);
        this->unsuspend();
        this->enable(true);

        physical_interface *v4 = this->field_C->physical_ifc();
        v4->set_current_gravity_vector(vector3d{0.0, -1.0, 0.0});

        auto v27 = g_swing_air_res;

        auto horz_scale = this->field_8->field_50
                              .get_optional_pb_float(physics_swing_drag_horz_scale_id,
                                                     g_normal_air_res_horz_scale,
                                                     nullptr);

        auto up_scale = this->field_8->field_50
                            .get_optional_pb_float(physics_swing_drag_up_scale_id,
                                                   g_normal_air_res_up_scale,
                                                   nullptr);

        auto down_scale = this->field_8->field_50
                              .get_optional_pb_float(physics_swing_drag_down_scale_id,
                                                     g_normal_air_res_down_scale,
                                                     nullptr);

        int swing_speed = g_game_ptr->gamefile->field_340.m_swing_speed;

        float v28;

        switch (swing_speed) {
        case 0:
            v27 = this->field_8->field_50.get_pb_float(physics_swing_drag_0_id);

            v28 = this->field_8->field_50.get_pb_float(physics_swing_drag_min_speed_0_id);
            break;
        case 1:
            v27 = this->field_8->field_50.get_pb_float(physics_swing_drag_1_id);

            v28 = this->field_8->field_50.get_pb_float(physics_swing_drag_min_speed_1_id);
            break;
        case 2:
            v27 = this->field_8->field_50.get_pb_float(physics_swing_drag_2_id);

            v28 = this->field_8->field_50.get_pb_float(physics_swing_drag_min_speed_2_id);

            break;
        case 3:
            v27 = this->field_8->field_50.get_pb_float(physics_swing_drag_3_id);

            v28 = this->field_8->field_50.get_pb_float(physics_swing_drag_min_speed_3_id);
            break;
        default:
            break;
        }

        auto *phys_ifc = this->field_C->physical_ifc();

        phys_ifc->field_15C = v27;

        phys_ifc->field_160 = v28;

        phys_ifc->field_164 = horz_scale;

        phys_ifc->field_168 = up_scale;

        phys_ifc->field_16C = down_scale;

        this->field_8->field_50.get_optional_pb_float(physics_gravity_swing_id,
                                                      g_swing_gravity,
                                                      nullptr);
        this->field_1C->m_gravity_multiplier = g_swing_gravity;
    }
    else
    {
        THISCALL(0x00694AF0, this);
    }
}

void physics_inode::set_velocity(const vector3d &a2, bool a3) {
    this->field_1C->set_velocity(a2, a3);
}

void physics_inode::set_stationary(bool a2) {
    entity_base *v1 = this->field_C;
    if (!a2) {
        v1->field_4 |= 0x40u;
    } else {
        v1->field_4 &= 0xFFFFFFBF;
    }
}

bool physics_inode::get_collided_last_frame() const
{
    return this->field_1C->is_flag(0x20u);
}


vector3d physics_inode::get_abs_position()
{
    auto *ent = this->field_C;

    if (ent->is_ext_flagged(0x10000000u)) {
        ent->update_abs_po(true);
    }

    return ent->get_abs_po().get_position();
}

vector3d physics_inode::get_last_collision_normal() const
{
    return this->field_1C->field_5C;
}

} // namespace ai

void physics_inode_patch() {
    {
        FUNC_ADDRESS(address, &physical_interface::set_current_gravity_vector);
        REDIRECT(0x00694B75, address);
    }
}
