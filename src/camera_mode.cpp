#include "camera_mode.h"

#include "ai_player_controller.h"
#include "camera_frame.h"
#include "camera_target_info.h"
#include "collide_aux.h"
#include "common.h"
#include "custom_math.h"
#include "game.h"
#include "game_settings.h" 
#include "input_mgr.h"
#include "oldmath_usefulmath.h"
#include "settings.h"
#include "spiderman_camera.h"
#include "trace.h"
#include "utility.h"
#include "variables.h"
#include "vtbl.h"

#include <cmath>

VALIDATE_SIZE(camera_mode, 0xC);

VALIDATE_OFFSET(camera_mode_shake, frame_fwd, 0x24);
VALIDATE_OFFSET(camera_mode_shake, frame_eye, 0x30);

VALIDATE_SIZE(camera_mode_lookaround, 0x78);

VALIDATE_SIZE(camera_mode_fixedstatic, 0x28u);

static Var<int> dword_959E5C {0x00959E5C};

bool sub_4B2180(vector3d &a1, float a2)
{
    auto v2 = a1.length2();
    if ( v2 <= sqr(a2 + LARGE_EPSILON) ) {
        return false;
    }

    auto v4 = a2 / sqrt(v2);
    a1 *= v4;
    return true;
}

bool pull_sphere(vector3d &a1, float t, vector3d a2)
{
    auto a1a = a1 - a2;
    auto result = sub_4B2180(a1a, t);
    a1 = a1a + a2;
    return result;
}

void camera_mode_chase::pull_by_target(camera_frame &frame, const camera_target_info &target, Float a3)
{
    pull_sphere(frame.eye, a3, target.pos);
    auto v15 = target.pos - frame.eye;
    auto v12 = v15.normalized();
    if ( v12.length() < EPSILON ) {
        v12 = target.facing;
    }

    if ( dword_959E5C()-- != 0 )
    {
        frame.fwd = v12;
    }
    else
    {
        frame.fwd = lerp(v12, frame.fwd, slow_mix());
        frame.fwd.normalize();
    }
}

camera_mode::camera_mode(spiderman_camera *a2, camera_mode *a3)
{
    this->m_vtbl = CAST(m_vtbl, 0x00881E24);
    this->slave = a2;
    this->field_8 = a3;

    assert(slave->get_target_entity());

    assert(slave->get_target_entity()->is_a_conglomerate());
}

void camera_mode::activate(camera_target_info &a2)
{
    this->m_vtbl->activate(this, nullptr, &a2);
}

void camera_mode::deactivate()
{
    this->m_vtbl->deactivate(this);
}

void camera_mode::request_recenter(Float a2, const camera_target_info &a3)
{
    if constexpr (1)
    {
        this->m_vtbl->request_recenter(this, nullptr, a2, &a3);
    }
    else
    {
        auto *v3 = this->field_8;
        if (v3 != nullptr) {
            v3->request_recenter(a2, a3);
        }
    }
}

void camera_mode::frame_advance(Float a2, camera_frame &frame, const camera_target_info &a4)
{
    if constexpr (0)
    {
        assert(frame.is_valid());

        auto *v4 = this->field_8;
        if ( v4 != nullptr ) {
            v4->frame_advance(a2, frame, a4);
        }

        assert(frame.is_valid());
    }
    else
    {
        this->m_vtbl->frame_advance(this, nullptr, a2, &frame, &a4);
    }
}

void camera_mode::set_fixedstatic(
        const vector3d &a2,
        const vector3d &a3)
{

    this->m_vtbl->set_fixedstatic(this, nullptr, &a2, &a3);
}

void camera_mode::clear_fixedstatic()
{
    this->m_vtbl->clear_fixedstatic(this);
}

void camera_mode_shake::_frame_advance(
        Float a2,
        camera_frame &a3,
        const camera_target_info &a4)
{
    TRACE("camera_mode_shake::frame_advance");

    THISCALL(0x004B6CE0, this, a2, &a3, &a4);

#ifdef OPENUSM_XBPACK_V10
    if (!a3.is_valid()) {
        if (frame_eye.is_valid() && frame_fwd.is_valid() && frame_fwd.is_normal()) {
            a3.eye = frame_eye;
            a3.fwd = frame_fwd;
        }
    }
#endif
}

camera_mode_lookaround::camera_mode_lookaround(
                    spiderman_camera *a2,
                    camera_mode *a3) : camera_mode(a2, a3)
{
    this->m_vtbl = CAST(m_vtbl, 0x008820AC);

    this->field_70[0] = 0.0;
    this->field_70[1] = 0.0;

    this->field_C = true;

    this->field_10.set_id(input_mgr::instance->field_58);
    this->field_10.set_control(109);
    this->field_10.field_8 = 0.1;

    this->field_40.set_id(input_mgr::instance->field_58);
    this->field_40.field_4 = 108;
    this->field_40.field_8 = 0.34999999;
}

vector3d sub_4B22E0(
        const vector3d &a2,
        const vector3d &a3,
        float a4)
{
    auto v4 = std::cos(a4);
    auto v5 = (a2[0] * a3[0] + a2[1] * a3[1] + a3[2] * a2[2]) * (1.0f  - v4);

    vector3d v8;
    v8[0] = a3[1] * a2[2] - a2[1] * a3[2];
    v8[1] = a3[2] * a2[0] - a3[0] * a2[2];
    v8[2] = a2[1] * a3[0] - a3[1] * a2[0];

    auto v6 = std::sin(a4);
    v8 *= v6;

    auto v9 = v5 *  a3[0];
    auto v10 = v5 * a3[1];
    auto v13 = v4 * a2[2];
    auto v11 = v4 * a2[0] + v9;
    auto v12 = v4 * a2[1] + v10;
    auto v7 = v5 * a3[2] + v13;

    vector3d result;
    result[0] = v11 + v8[0];
    result[1] = v12 + v8[1];
    result[2] = v7 +  v8[2];
    return result;
}

void camera_mode_lookaround::_frame_advance(
        Float a2,
        camera_frame &a3,
        const camera_target_info &a4)
{
    TRACE("camera_mode_lookaround::frame_advance");

    if constexpr (0)
    {
        auto *v4 = &a4;
        auto *v6 = this->slave;
        auto *v7 = &a3;
        if ( v6->field_1BC || !this->field_C )
        {
            v6->field_1CC = false;
        }
        else
        {
            eHeroLocoMode v9;
            auto *the_controller = a4.field_54->m_player_controller;
            if ( the_controller != nullptr )
            {
                auto loco_mode = static_cast<eHeroLocoMode>(the_controller->get_spidey_loco_mode());
                v9 = static_cast<eHeroLocoMode>(1);
                if ( loco_mode >= 0 ) {
                    v9 = loco_mode;
                }
            }
            else
            {
                v9 = static_cast<eHeroLocoMode>(1);
            }

            this->field_10.update(a2);
            this->field_40.update(a2);

            if ( this->field_40.field_2D && this->field_10.field_2D )
            {
                if ( this->slave->field_1CC )
                {
                    this->field_70 = vector2d {0.0, };
                    if ( (v9 == 3 || v9 == 5 || v9 == 6 || !v4->sub_4B2980()) && !v4->sub_4B29C0() ) {
                        this->slave->field_1CC = false;
                    }
                }
            }
            else
            {
                vector2d v82 {this->field_10.field_10, this->field_40.field_10};
                if ( !Settings::MouseLook() )
                {
                    auto v13 = v82.length2();
                    if ( v13 > sqr(1.0f) )
                    {
                        v82 /= std::sqrt(v13);
                    }

                    auto v15 = v82.length2();
                    vector2d v81 = v82 * v15;

                    v82 = lerp(v82, v81, 0.80000001f);

                    auto v16 = v82 - this->field_70;
                    auto v18 = v16.length2();
                    if ( v18 > sqr(0.25f) )
                    {
                        auto v19 = 0.25f / sqrt(v18);
                        v16 *= v19;
                    }

                    v82 = v16 + this->field_70;
                    auto v20 = v82.length2();
                    if ( v20 > sqr(1.0f) )
                    {
                        auto v21 = 1.0f / std::sqrt(v20);
                        v82 *= v21;
                    }
                }

                float v78 = v82[0] * a2;
                float v81 = v82[1] * a2;

                vector3d v86 = a2 * v4->field_24;
                v7->eye += v86;

                float v25 = 0.0f;
                if ( v9 == 1 ) {
                    v25 = v4->radius * 1.25f;
                }

                vector3d v84 = v4->up * v25 + v4->pos;
                auto v28 = (v84 - v7->eye).length();

                auto v29 = v7->fwd * v28;

                vector3d a3a = v29 + v7->eye;
                a3a = lerp(v84, a3a, slow_mix());
                auto *gamefile = g_game_ptr->gamefile;
                auto invert_camera_vert = gamefile->field_340.m_invert_camera_vert;
                v84 = v7->eye - a3a;

                auto invert_camera_horz = gamefile->field_340.m_invert_camera_horz;
                float v33 = ( invert_camera_horz ? -1.0f : 1.0f );

                auto v72 = -(v33 * v78 * 4.0f);
                v84 = sub_4B22E0(v84, v7->up, v72);

                auto v37 = ( invert_camera_vert ? -1.0f : 1.0f );

                auto v38 = v37 * v81 * 3.5;
                auto a2b = v38;
                auto v39 = v7->get_right();
                auto v40 = sub_4B22E0(v84, v39, -v38);
                eHeroLocoMode v46 = v9;
                auto v47 = v40.length();
                if ( v47 < 4.0f )
                {
                    v40 *= 1.0f / v47;
                    if ( v46 == 2 || v46 == 7 )
                    {
                        auto v50 = dot(v40, v4->up);
                        if ( v50 > 0.0f ) {
                            v47 = (1.0f - std::sqrt(1.0f - v50 * v50)) * 4.0f + v47;
                        }
                    }
                    else if ( a2b < 0.0f )
                    {
                        v47 = v47 - a2b * 4.0f ;
                    }

                    v47 = std::min(v47, 4.0f);
                    v40 *= v47;
                }

                v7->eye = a3a + v40;
                if ( v46 == 1
                    || v46 == 2
                    || v46 == 7 )
                {
                    auto abs_pos = v4->field_54->get_abs_position();

                    auto a2c = v4->radius * 1.3f;
                    sub_4B2680(v7->eye, a2c, abs_pos);

                    auto v56 = v4->sub_4B42E0() - 0.25f;

                    auto v58 = std::max(v56, 0.0f);
                    auto v60 = v4->up * v58;

                    auto v84 = abs_pos - v60;

                    float t = 1.0f;
                    sub_5B8F40(a3a, v7->eye, v84, v4->up, &t);
                    if ( t > 0.0f && t < 1.0f )
                    {
                        auto v64 = v7->eye - a3a;
                        auto v81 = v64 * t;
                        v7->eye = v81 + a3a;
                        sub_4B2680(v7->eye, a2c, v84);
                    }
                }

                const float min_val = ( v46 == 1 ? -0.89999998f : -0.97000003f);

                v7->constrain_pos_relative_to_plane(a3a, YVEC, min_val, 0.97000003f);

                this->slave->sub_4B3220(a3a);
                this->field_70 = v82;
            }
        }

        auto *v11 = this->slave;
        if ( v11->field_1CC )
        {
            v11->field_1C4 = v11->field_1C0;
            v11->field_1C0 = 1;
        }
        else
        {
            auto *v71 = this->field_8;
            if ( v71 != nullptr ) {
                v71->frame_advance(a2, *v7, *v4);
            }

            this->field_70 = vector2d {0.0, 0.0};
        }
    }
    else
    {
        THISCALL(0x004B5480, this, a2, &a3, &a4);
    }
}

camera_mode_passive::camera_mode_passive(
            spiderman_camera *a2,
            camera_mode *a3) : camera_mode(a2, a3)
{
    this->m_vtbl = CAST(m_vtbl, 0x00882408);
    this->field_10 = g_camera_max_dist;
    this->field_C = g_camera_min_dist;
    this->field_14 = -0.5;
    this->field_18 = 0.69999999;
    this->field_1C = YVEC;
}

void camera_mode_passive::_activate(camera_target_info &a2)
{
    auto *v3 = this->field_8;
    if ( v3 != nullptr ) {
        v3->activate(a2);
    }

    this->field_1C = YVEC;
}

void camera_mode_passive::_frame_advance(
        [[maybe_unused]] Float a2,
        camera_frame &frame,
        camera_target_info &target)
{
    TRACE("camera_mode_passive::frame_advance");

    if constexpr (0)
    {
        [](spiderman_camera *self) -> void {
            self->field_1C4 = self->field_1C0;
            self->field_1C0 = 3;
        }(this->slave);

        int loco_mode = target.get_loco_mode();
        int prev_loco_mode = target.get_prev_loco_mode();

        bool is_crawling = prev_loco_mode == 2;
        bool is_running = loco_mode == 1;
        bool is_falling = loco_mode == 5;
        bool is_jumping = loco_mode == 6;
        auto is_swinging = loco_mode == 3;
        auto v47 = loco_mode == 9;
        auto v13 = (loco_mode == 2
                    || loco_mode == 7
                    || loco_mode == 14
                    || (is_crawling && loco_mode == 9));

        bool v12 = ( is_falling || is_jumping || is_swinging );

        assert(target.min_look_dist > target.radius);

        float v14, v15;
        if ( v13 )
        {
            v14 = 0.30000001f;
            v15 = 1.0f;
        }
        else
        {
            if ( is_running ) {
                v14 = 0.30000001f;
            } else {
                v14 = 0.0f;
            }

            v15 = 0.69999999f;
        }

        target.min_look_dist = lerp(target.min_look_dist, this->field_C, slow_mix());
        this->field_C = target.min_look_dist;

        target.max_look_dist = lerp(target.max_look_dist, this->field_10, slow_mix());
        this->field_10 = target.max_look_dist;

        this->field_14 = lerp(v14, this->field_14, slow_mix());
        this->field_18 = lerp(v15, this->field_18, slow_mix());

        vector3d v19 = YVEC;
        if ( v12 )
        {
            auto v50 = vector3d::cross(frame.up, frame.fwd);
            auto v23 = dot(target.field_24, v50);
            v23 = (2.0f / 30.0f) * v23;

            int a2a = sign(v23);

            auto v24 = sqr(v23) * a2a * 0.125f;

            v24 = std::clamp(v24, -0.5f, 0.5f);

            auto a2b = std::sqrt(1.0f - sqr(v24));
            v19 = a2b * frame.up + v50 * v24;
        }

        this->field_1C.sub_4B9FA0(v19, slow_mix());
        frame.up = this->field_1C;
        if ( v47 )
        {
            auto v32 = target.pos + target.facing * 5.0f;

            frame.rotate_to_include_target(v32, target.pos, target.up, 0.80000001);

            frame.include_target(v32, 0.0f, 0.80000001f);
        }

        frame.constrain_pos_relative_to_plane(target.pos, target.up, this->field_14, this->field_18);
        if ( !v13 || target.up[1] < -0.15000001f ) {
            frame.avoid_target(target, target.min_look_dist);
        }

        camera_mode_chase::pull_by_target(frame, target, target.max_look_dist);
        if ( v12 )
        {
            auto v35 = target.pos - frame.eye;
            auto v51 = vector3d::cross(v35, YVEC);
            auto v38 = v51.normalized();
            if ( v38.length() > EPSILON ) {
                constrain_normal(frame.fwd, v38, -0.1f, 0.1f);
            }
        }
    }
    else
    {

        void (__fastcall *func)(void *, void *,
                        Float,
                        camera_frame *,
                        camera_target_info *) = CAST(func, 0x004B7400);
        func(this, nullptr, a2, &frame, &target);
    }
}

camera_mode_fixedstatic::camera_mode_fixedstatic(
        spiderman_camera *a2,
        camera_mode *a3) : camera_mode(a2, a3)
{
    this->m_vtbl = CAST(m_vtbl, 0x00881E74);
    this->field_C = ZEROVEC;
    this->field_18 = ZVEC;
    this->enabled = false;
}

void camera_mode_fixedstatic::_frame_advance(
        Float a2,
        camera_frame &a3,
        const camera_target_info &a4)
{
    if ( this->enabled )
    {
        a3.eye = this->field_C;
        a3.fwd = this->field_18;
        a3.up = YVEC;
        a3.include_target(a4.field_C, a4.radius, 0.80000001);
        auto *v5 = this->slave;

        {
            v5->field_1C4 = v5->field_1C0;
            v5->field_1C0 = 0;
        }
    }
    else
    {
        auto *v6 = this->field_8;
        if ( v6 != nullptr ) {
            v6->frame_advance(a2, a3, a4);
        }
    }
}

void camera_mode_fixedstatic::_set_fixedstatic(
        const vector3d &a2,
        const vector3d &a3)
{
    auto *v4 = this->field_8;
    if ( v4 != nullptr ) {
        v4->set_fixedstatic(this->field_C, a3);
    }

    this->enabled = true;
    this->field_C = a2;

    this->field_18 = (a3 - a2).normalized();
}

void camera_mode_fixedstatic::_clear_fixedstatic()
{
    auto *v2 = this->field_8;
    if ( v2 != nullptr ) {
        v2->clear_fixedstatic();
    }

    this->enabled = false;
}

void camera_mode_combat::_frame_advance(
        Float a2,
        camera_frame &a3,
        const camera_target_info &a4)
{
    {
        static float & flt_882444 = var<float>(0x00882444);
        flt_882444 = 0.66000003 * g_camera_min_dist;
    }

    if constexpr (0)
    {}
    else
    {
        THISCALL(0x004B7F90, this, a2, &a3, &a4);
    }
}

void camera_mode_patch()
{
    {
        FUNC_ADDRESS(address, &camera_mode_shake::_frame_advance);
        set_vfunc(0x00881EAC, address);
    }

    {
        FUNC_ADDRESS(address, &camera_mode_lookaround::_frame_advance);
        set_vfunc(0x008820BC, address);
    }

    {
        FUNC_ADDRESS(address, &camera_mode_passive::_frame_advance);
        set_vfunc(0x00882418, address);
    }

    {
        FUNC_ADDRESS(address, &camera_mode_combat::_frame_advance);
        set_vfunc(0x00882068, address);
    }
}
