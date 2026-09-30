#include "spiderman_camera.h"

#include "camera_frame.h"
#include "camera_mode.h"
#include "camera_target_info.h"
#include "common.h"
#include "custom_math.h"
#include "dvar.h"
#include "func_wrapper.h"
#include "game.h"
#include "geometry_manager.h"
#include "oldmath_po.h"
#include "os_developer_options.h"
#include "trace.h"
#include "variables.h"
#include "vtbl.h"
#include "wds.h"

#include <cmath>

VALIDATE_SIZE(spiderman_camera, 0x204u);
VALIDATE_OFFSET(spiderman_camera, field_1A0, 0x1A0);

float g_yaw_mult = 2.0f;

float g_pitch_mult = 2.0f;

constexpr float inverse_pow(float a1, float a2)
{
    return std::pow(a2, 1.0f / a1);
}

void set_dvars()
{
    debug_variable_t v0 {"camera_min_dist", g_camera_min_dist};
    g_camera_min_dist = v0;

    debug_variable_t v1 {"camera_max_dist", g_camera_max_dist};
    g_camera_max_dist = v1;

    debug_variable_t v2 {"camera_supermax_dist", g_camera_supermax_dist};
    g_camera_supermax_dist = v2;
}

void set_filter_time(float a1)
{
    {
        set_dvars();
    }

    sluggish_mix() = pow(inverse_pow(1.0, 0.039999999), a1);
    slow_mix() = pow(inverse_pow(0.5, 0.039999999), a1);
    med_mix() = pow(inverse_pow(0.25, 0.039999999), a1);
    fast_mix() = pow(inverse_pow(0.125, 0.039999999), a1);
    pronto_mix() = pow(inverse_pow(0.0625, 0.039999999), a1);

    assert(sluggish_mix() > slow_mix()
            && slow_mix() > med_mix()
            && med_mix() > fast_mix()
            && fast_mix() > pronto_mix());
}

static Var<vector3d> stru_959EBC {0x00959EBC};

void constrain_normal(vector3d &normal, const vector3d &basisA, float a4, float a5)
{
    assert(normal.is_normal());
    
    assert(basisA.is_normal());

    auto a3a = dot(normal, basisA);
    auto v8 = basisA * a3a;
    auto v18 = normal - v8;
    auto v4 = std::min(a3a, a5);
    a3a = std::max(v4, a4);
    auto v9 = 1.0 - sqr(a3a);
    auto v17 = std::sqrt(v9);
    auto v16 = v18.length2();
    if ( v16 > 0.000099999997 )
    {
        auto v5 = (1.0f / sqr(v16));
        auto v10 = v5 * v17;
        auto v11 = v18 * v10;
        auto v6 = basisA * a3a;
        normal = v6 + v11;
    }

    assert(normal.is_normal());
}

Var<spiderman_camera *> g_spiderman_camera_ptr{0x00959A70};

spiderman_camera::spiderman_camera(const string_hash &a2, entity *a3) : game_camera(a2, a3)
{
    if constexpr (0)
    {
        //g_camera_mouse_mode() = DEBUG_CAMERA_MOUSE_MODE;
        //g_pitch_mult() =    os_developer_options::instance->get_int(mString {"DEBUG_CAMERA_PITCH_MULTIPLIER"}) * 0.1;
        g_yaw_mult =      os_developer_options::instance->get_int(mString {"DEBUG_CAMERA_YAW_MULTIPLIER"}) * 0.1;
        g_move_mult() =     os_developer_options::instance->get_int(mString {"DEBUG_CAMERA_MOVE_MULTIPLIER"}) * 0.1;
        g_strafe_mult() =   os_developer_options::instance->get_int(mString {"DEBUG_CAMERA_STRAFE_MULTIPLIER"}) * 0.1;

        this->field_1D0.set_id(input_mgr::instance->field_58);
        this->field_1D0.set_control(static_cast<game_control_t>(102));
    }
    else
    {
        THISCALL(0x004B78E0, this, &a2, a3);
    }
}

void * spiderman_camera::operator new(size_t size)
{
    using aligned_malloc_t = void *(__cdecl *)(size_t, size_t);
    auto aligned_malloc = *bit_cast<aligned_malloc_t *>(0x0086F354);
    return aligned_malloc(size, 4);
}

void spiderman_camera::operator delete(void *ptr)
{
    using aligned_free_t = void (__cdecl *)(void *);
    auto aligned_free = *bit_cast<aligned_free_t *>(0x0086F328);
    aligned_free(ptr);
}

void spiderman_camera::sub_4B3260(bool a2)
{
    this->field_1BC = a2;
}

void spiderman_camera::set_fixedstatic(const vector3d &a2, const vector3d &a3)
{
    this->field_1A0->set_fixedstatic(a2, a3);
}

void spiderman_camera::sub_4B3220(const vector3d &a2)
{
    stru_959EBC() = a2;
    this->field_1CC = true;
}

void spiderman_camera::_sync(camera &a2)
{
    game_camera::_sync(a2);
    set_filter_time(0.0);
    auto *target = this->get_target_entity();
    auto y_facing = target->get_abs_po().get_y_facing();

    camera_target_info v11 { 
            target,
            0.033333335,
            target->get_abs_position(),
            y_facing
    };

    this->target_pos = v11.pos;
    this->target_up = v11.up;

    this->field_1C4 = 0;
    this->field_1C0 = 0;

    this->field_1A0->deactivate();
    this->field_1A0->activate(v11);
}

void spiderman_camera::adjust_geometry_pipe(bool a1)
{
    if constexpr (1)
    {
        auto *abs_po = &this->get_abs_po();
        auto &pos = abs_po->get_position();

        auto &v2 = abs_po->get_z_facing();
        auto eye = v2 + pos;

        auto &up = abs_po->get_y_facing();
        if (a1)
        {
            assert(!is_externally_controlled());

            auto *v7 = entity_handle_manager::find_entity(string_hash{"SCENE_ANALYZER_CAM"},
                                                          entity_flavor_t::CAMERA,
                                                          false);

            this->set_abs_po(v7->get_abs_po());

            auto &pos = abs_po->get_position();

            geometry_manager::set_view(pos, eye, up);
        } else if (g_game_ptr != nullptr && !g_game_ptr->m_user_camera_enabled) {
            auto &pos = abs_po->get_position();
            geometry_manager::set_view(pos, eye, up);
        }

    } else {
        THISCALL(0x004B6480, this, a1);
    }
}

void spiderman_camera::autocorrect(Float a2)
{
    if constexpr (0)
    {
        auto *target_entity = this->get_target_entity();
        camera_target_info v13 {target_entity, 0.033333335, this->target_pos, this->target_up};

        this->field_1A0->request_recenter(a2, v13);
        if ( a2 == 0.0f )
        {
            this->target_pos = v13.pos;
            this->target_up = v13.up;
        }
    }
    else
    {
        void (__fastcall *func)(void *, void *, Float) = CAST(func, get_vfunc(m_vtbl, 0x2D0));
        func(this, nullptr, a2);
    }
}

void spiderman_camera::_autocorrect(Float a2)
{
    TRACE("spiderman_camera::autocorrect");

    if constexpr (1)
    {
        auto *target = this->get_target_entity();
        camera_target_info v13 {target, 0.033333335f, this->target_pos, this->target_up};

        //sp_log("0x%08X", this->field_1A0->m_vtbl);
        this->field_1A0->request_recenter(a2, v13);
        if (equal(a2.value, 0.0f))
        {
            this->target_pos = v13.pos;
            this->target_up = v13.up;
        }

    } else {
        THISCALL(0x004B63F0, this, a2);
    }
}

void spiderman_camera::_set_target_entity(entity *e)
{
    TRACE("spiderman_camera::set_target_entity");

    assert(e->has_physical_ifc());

    assert(e->is_a_conglomerate());

    game_camera::set_target_entity(e);
}

void spiderman_camera::_frame_advance(Float a2)
{
    TRACE("spiderman_camera::frame_advance");

    if (this->field_1A0 != nullptr) {
        sp_log("0x%08X", this->field_1A0->m_vtbl);
    }

    if constexpr (0)
    {
        if ( g_game_ptr->level_is_loaded()
            && !g_game_ptr->is_paused()
            && !os_developer_options::instance->get_flag(mString {"SHOW_PROFILE_INFO"}) )
        {
            static int & old_devopt_fov = var<int>(0x00959E54);
            this->field_1D0.update(a2);
            auto CAMERA_FOV = os_developer_options::instance->get_int(mString {"CAMERA_FOV"});
            if ( CAMERA_FOV != old_devopt_fov )
            {
                old_devopt_fov = CAMERA_FOV;
                auto fov = CAMERA_FOV * 0.017453292f;
                this->set_fov(fov);
            }

            if ( !this->field_12C )
            {
                this->autocorrect(0.0);
                this->field_12C = true;
            }

            set_filter_time(a2);
            if ( this->get_target_entity() == nullptr ) {
                this->set_target_entity(g_world_ptr->get_hero_ptr(0));
            }

            auto *target_entity = this->get_target_entity();
            camera_target_info v18 {target_entity, a2, this->target_pos, this->target_up};

            vector3d v17 = this->get_abs_position() - v18.pos;
            v18.field_48 = v17;
            auto len2 = (v18.pos - this->target_pos).length2();
            if ( len2 > sqr(16.0) ) {
                this->autocorrect(0.0);
            }

            auto *the_controller = v18.field_54->m_player_controller;
            if ( the_controller != nullptr ) {
                the_controller->force_always_camera_relative(false);
            }

            camera_frame v19 {this->get_abs_po()};
            v19.fwd.normalize();

            this->field_1A0->frame_advance(a2, v19, v18);
            auto a2a = v19.get_po();

            this->set_abs_po(a2a);

            this->set_frame_delta(a2a, a2);

            this->target_pos = v18.pos;
            this->target_up = v18.up;
        }
    }
    else
    {
        THISCALL(0x004B60B0, this, a2);
    }
}

void constrain_relative_to_plane(
        vector3d &a1,
        const vector3d &a2,
        const vector3d &norm,
        float a4,
        float a5)
{
    assert(norm.is_normal());

    auto v8 = a1 - a2;
    auto len = v8.length();
    if ( len > EPSILON )
    {
        auto v9 = 1.0f / len;
        vector3d a1a = v8 * v9;
        constrain_normal(a1a, norm, a4, a5);

        a1 = a1a * len + a2;
    }
}

void spiderman_camera_patch()
{
    {
        REDIRECT(0x004B6159, set_filter_time);
    }

    {
        FUNC_ADDRESS(address, &spiderman_camera::_autocorrect);
        set_vfunc(0x008823B0, address);
    }

    {
        FUNC_ADDRESS(address, &spiderman_camera::_set_target_entity);
        set_vfunc(0x00882394, address);
    }

    {
        FUNC_ADDRESS(address, &spiderman_camera::_frame_advance);
        set_vfunc(0x00882284, address);
    }
}
