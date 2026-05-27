#include "mouselook_controller.h"

#include "camera.h"
#include "common.h"
#include "custom_math.h"
#include "dolly_and_strafe_mcs.h"
#include "entity_handle_manager.h"
#include "func_wrapper.h"
#include "game.h"
#include "gamepad_menu.h"
#include "input_mgr.h"
#include "oldmath_po.h"
#include "spiderman_camera.h"
#include "theta_and_psi_mcs.h"
#include "trace.h"
#include "variables.h"
#include "wds.h"

VALIDATE_SIZE(mouselook_controller, 0x18u);

Var<mouselook_controller *> g_mouselook_controller{0x0095C17C};

mouselook_controller::mouselook_controller(
        dolly_and_strafe_mcs *a2,
        theta_and_psi_mcs *a3,
        camera *a4)
{
    this->field_8 = a2;
    this->m_vtbl = 0x00889114;
    this->field_C = a3;
    this->field_10 = a4;
    this->field_14 = input_mgr::instance->field_58;
}

void mouselook_controller::reset()
{
    auto *v1 = this->field_C;
    if ( v1 != nullptr ) {
        v1->reset_angles();
    }
}

static string_hash entity_id_USER_CAM {int(to_hash("USER_CAM"))};

static vector3d cur_cam_target {};

static Var<bool> lock_target_btn_pressed {0x00960B44};

static Var<bool> byte_960B45 {0x00960B45};

static Var<bool> byte_960B46 {0x00960B46};

static Var<bool> byte_921B60 {0x00921B60};

void mouselook_controller::_frame_advance(Float time_inc)
{
    TRACE("mouselook_controller::frame_advance");

    {
        g_debug_cam_target_actor() = bit_cast<actor *>(g_world_ptr->get_hero_ptr(0));

        vector3d cur_cam_target {0x0095CB00};
        //cur_cam_target() = ZEROVEC;
    }

    if constexpr (0)
    {
        if ( !g_game_ptr->is_user_camera_enabled() ) {
            return;
        }

        auto *user_cam = [&]() -> entity_base * {
            auto *user_cam = this->field_10;
            if ( user_cam == nullptr ) {
                user_cam = entity_handle_manager::find_entity(entity_id_USER_CAM, entity_flavor_t::CAMERA, false);
            }

            return user_cam;
        }();

        auto *v4 = input_mgr::instance;

        float speed = 10.0;
        if ( AXIS_MAX == v4->get_control_state(26, (device_id_t)-1) ) {
            speed *= 10.0;
        }

        if ( AXIS_MAX == v4->get_control_state(27, (device_id_t)-1) ) {
            speed *= 0.2f;
        }

        float clamped_speed = std::clamp(speed, 5.0f, 20.0f);

        auto pitch = v4->get_control_state(20, (device_id_t)-1);
        auto yaw = v4->get_control_state(21, (device_id_t)-1);

        auto control_state = v4->get_control_state(14, (device_id_t)-1);
        auto v5 = v4->get_control_state(17, (device_id_t)-1);

        pitch *= std::abs(pitch);

        yaw *= std::abs(yaw);
    
        float lift = 0.0;
        auto v39 = -(std::abs(control_state) * control_state * g_move_mult());
        auto strafe = std::abs(v5) * v5 * g_strafe_mult();

        if ( AXIS_MAX == v4->get_control_state(18, (device_id_t)-1) ) {
            lift = 1.0;
        }

        if ( AXIS_MAX == v4->get_control_state(19, (device_id_t)-1) ) {
            lift -= 1.0f;
        }

        if ( cam_target_locked() )
        {
            if ( AXIS_MAX == v4->get_control_state(22, (device_id_t)-1) ) {
                lift -= 1.0f;
            }

            if ( AXIS_MAX == v4->get_control_state(23, (device_id_t)-1) ) {
                lift += 1.0f;
            }
        }

        if ( AXIS_MID == v4->get_control_state(31, (device_id_t)-1) )
        {
            if ( byte_960B46() ) {
                byte_921B60() = !byte_921B60();
            }

            byte_960B46() = false;
        }
        else
        {
            byte_960B46() = true;
        }

        if ( AXIS_MID == v4->get_control_state(32, (device_id_t)-1) )
        {
            if ( byte_960B45() ) {
                byte_921B60() = !byte_921B60();
            }

            byte_960B45() = false;
        }
        else if ( byte_960B45() )
        {
            byte_960B45() = true;
        }
        else
        {
            byte_921B60() = !byte_921B60();
        }

        auto d_psi = (0.0f - g_pitch_mult) * pitch;
        auto d_theta = (0.0f - g_yaw_mult) * yaw;

        if ( AXIS_MAX == v4->get_control_state(22, (device_id_t)-1) ) {
            d_psi += 1.0f;
        }

        if ( AXIS_MAX == v4->get_control_state(23, (device_id_t)-1) ) {
            d_psi -= 1.0f;
        }

        if ( AXIS_MAX == v4->get_control_state(24, (device_id_t)-1) )
        {
            if ( cam_target_locked() ) {
                strafe = 1.0;
            }

            d_theta -= 1.0f;
        }

        if ( AXIS_MAX == v4->get_control_state(25, INVALID_DEVICE_ID) )
        {
            if ( cam_target_locked() ) {
                strafe = -1.0;
            }

            d_theta += 1.0f;
        }

        if ( AXIS_MAX == v4->get_control_state(14, INVALID_DEVICE_ID) ) {
            v39 = 1.0;
        }

        if ( AXIS_MAX == v4->get_control_state(15, INVALID_DEVICE_ID) ) {
            v39 = -1.0;
        }

        if ( AXIS_MAX == v4->get_control_state(17, INVALID_DEVICE_ID) )
        {
            strafe = -1.0;
            if ( !cam_target_locked() ) {
                strafe = 1.0;
            }
        }

        if ( AXIS_MAX == v4->get_control_state(16, INVALID_DEVICE_ID) )
        {
            strafe = 1.0;
            if ( !cam_target_locked() ) {
                strafe = -1.0;
            }
        }

        auto *v8 = g_debug_cam_target_actor();
        if ( AXIS_MID == v4->get_control_state(29, INVALID_DEVICE_ID) )
        {
            if ( lock_target_btn_pressed() )
            {
                if ( !cam_target_locked() )
                {
                    if ( g_debug_cam_target_actor() != nullptr )
                    {
                        cur_cam_target = g_debug_cam_target_actor()->get_abs_position();
                    }
                    else
                    {
                        cur_cam_target = vector3d {0.0, 0.0, 0.0};
                        g_debug_cam_get_next_target() = true;
                    }
                }

                this->reset();
                cam_target_locked() = !cam_target_locked();
            }

            lock_target_btn_pressed() = false;
        }
        else
        {
            lock_target_btn_pressed() = true;
        }

        if ( cam_target_locked() && v8 != nullptr )
        {
            auto abs_pos = v8->get_abs_position();
            if ( approx_equals(cur_cam_target, ZEROVEC, 0.0099999998f) ) {
                cur_cam_target = g_debug_cam_target_actor()->get_abs_position();
            }

            auto v17 = abs_pos - cur_cam_target;

            cur_cam_target = abs_pos;

            this->reset();

            auto user_cam_pos = user_cam->get_abs_position();
            auto v22 = user_cam_pos + v17;

            auto v24 = v22 - cur_cam_target;

            auto v26 = v24.length();
            if ( v26 > 10.0f ) {
                v26 = 10.0f;
            }

            static float kd_24596 = v26 * 0.02f;

            auto v35 = kd_24596 * v39 * speed * time_inc;
            auto v40 = kd_24596 * strafe * speed * time_inc * 1.2f;
            auto v56 = kd_24596 * lift * speed * time_inc;

            vector3d v50 = v22 - cur_cam_target;
            auto v37 = v50.length();

            vector3d v54 {v50[2], 0.0f, 0.0f - v50[0]};
            v54.normalize();

            v54 = v50 + v54 * v40;
            v54.normalize();

            v54 = cur_cam_target + v54 * v37;

            user_cam->set_abs_position(v54);
            user_cam->look_at(cur_cam_target);

            auto v44 = user_cam->get_abs_position();

            auto y_facing = user_cam->get_abs_po().get_y_facing();
            if ( y_facing[1] > 0.1f
                || (v44[1] > cur_cam_target[1] && v56 < 0.0f)
                || (v44[1] < cur_cam_target[1] && v56 > 0.0f) )
            {
                auto v55 = y_facing * v56;
                v54 = v44 + v55;
                user_cam->set_abs_position(v54);
            }

            auto v45 = user_cam->get_abs_position();

            auto z_facing = user_cam->get_abs_po().get_z_facing();
            v54 = v45 + z_facing * v35;
            if ( (v54 - cur_cam_target).length() > 0.30000001f ) {
                user_cam->set_abs_position(v54);
            }
        }
        else
        {
            if ( v8 == nullptr ) {
                g_debug_cam_get_next_target() = true;
            }

            this->field_C->d_psi_for_next_frame = d_psi * clamped_speed * 0.0033333334f;
            this->field_C->d_theta_for_next_frame = d_theta * clamped_speed * 0.0033333334f;
            this->field_8->m_dolly = v39 * speed * time_inc;
            this->field_8->m_lift = lift * speed * time_inc;
            this->field_8->m_strafe = strafe * speed * time_inc;
        }
    }
    else
    {
        THISCALL(0x00528BB0, this, time_inc);

        // -------------------------------------------------------------
        //   Debug user-camera extras: L3 zoom / R3 move
        // -------------------------------------------------------------
        //   Uses the DualShock 4 / DirectInput hook in main.cpp which
        //   populates controllerKeys[] every poll. Active only when the
        //   debug menu has set "Camera: User".
        //
        //   Hold L3  + L3_UP   -> dolly zoom IN  (camera forward)
        //   Hold L3  + L3_DOWN -> dolly zoom OUT (camera backward)
        //
        //   Hold R3  + R3_LEFT  -> strafe camera left
        //   Hold R3  + R3_RIGHT -> strafe camera right
        //   Hold R3  + R3_UP    -> lift camera up
        //   Hold R3  + R3_DOWN  -> lift camera down
        // -------------------------------------------------------------
        if (g_game_ptr != nullptr && g_game_ptr->is_user_camera_enabled())
        {
            const bool l3_held = (controllerKeys[MENU_L3] != 0);
            const bool r3_held = (controllerKeys[MENU_R3] != 0);

            if (l3_held || r3_held)
            {
                auto *user_cam = entity_handle_manager::find_entity(
                    entity_id_USER_CAM, entity_flavor_t::CAMERA, false);

                if (user_cam != nullptr)
                {
                    // Speed modifiers from the existing user-cam buttons:
                    //   axis 26 = USERCAM_FAST, axis 27 = USERCAM_SLOW
                    auto *im = input_mgr::instance;
                    float speed = 10.0f;
                    if (AXIS_MAX == im->get_control_state(26, (device_id_t)-1)) speed *= 4.0f;
                    if (AXIS_MAX == im->get_control_state(27, (device_id_t)-1)) speed *= 0.25f;

                    const float dt   = float(time_inc);
                    const float step = speed * dt;

                    auto pos = user_cam->get_abs_position();
                    auto &po = user_cam->get_abs_po();

                    // ---- L3: dolly zoom along camera forward (z facing) ----
                    if (l3_held)
                    {
                        float dolly = 10.0f;
                        if (controllerKeys[MENU_L3_UP]   != 0) dolly += step;
                        if (controllerKeys[MENU_L3_DOWN] != 0) dolly -= step;

                        if (dolly != 0.0f)
                        {
                            const auto &fwd = po.get_z_facing();
                            pos = pos + fwd * dolly;
                        }
                    }

                    // ---- R3: pan/translate (strafe X + lift Y) ----
                    if (r3_held)
                    {
                        float strafe = 10.0f;
                        float lift   = 10.0f;
                        if (controllerKeys[MENU_R3_RIGHT] != 0) strafe += step;
                        if (controllerKeys[MENU_R3_LEFT]  != 0) strafe -= step;
                        if (controllerKeys[MENU_R3_UP]    != 0) lift   += step;
                        if (controllerKeys[MENU_R3_DOWN]  != 0) lift   -= step;

                        if (strafe != 0.0f)
                        {
                            const auto &right = po.get_x_facing();
                            pos = pos + right * strafe;
                        }
                        if (lift != 0.0f)
                        {
                            const auto &up = po.get_y_facing();
                            pos = pos + up * lift;
                        }
                    }

                    user_cam->set_abs_position(pos);
                }
            }
        }
    }
}

bool mouselook_controller::_is_mouselook_controller() const {
    return true;
}

void mouselook_controller_patch()
{
    {
        FUNC_ADDRESS(address, &mouselook_controller::_frame_advance);
        set_vfunc(0x00889118, address);
    }
}