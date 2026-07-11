#include "als_motion_compensator.h"

#include "actor.h"
#include "als_animation_logic_system.h"
#include "common.h"
#include "custom_math.h"
#include "oldmath_po.h"
#include "trace.h"
#include "utility.h"
#include "vtbl.h"

namespace als {

VALIDATE_SIZE(motion_compensator, 0x14);

void motion_compensator::activate(animation_logic_system *a2)
{
    TRACE("als::motion_compensator::activate");

    // Converted from 0x004A0F30.
    this->field_4 = a2;
    this->field_8 = (als::state_machine *)this->field_4->get_als_layer_internal(static_cast<als::layer_types>(0));
    this->the_actor = this->field_4->get_actor();
    this->field_10 = 10.0f;
}

void motion_compensator::pre_anim_action(Float a3)
{
    void (__fastcall *func)(void *, void *, Float) = CAST(func, get_vfunc(m_vtbl, 0x20));
    func(this, nullptr, a3);
}

void motion_compensator::post_anim_action(Float a2)
{
    void (__fastcall *func)(void *, void *, Float) = CAST(func, get_vfunc(m_vtbl, 0x24));
    func(this, nullptr, a2);
}

void motion_compensator::set_facing_to_dir_internal(
        actor *the_actor,
        vector3d a1,
        vector3d a2,
        vector3d a3,
        Float a7,
        Float a8,
        Float a9)
{
    TRACE("als::motion_compensator::set_facing_to_dir_internal");

    sp_log("0x%08X", this->m_vtbl);

    if constexpr (0) {
        sp_log("DEBUG!!!");

        sp_log("%s", a2.to_string());

        if ( a2.length2() >= EPSILON ) {
            sp_log("DEBUG!!!");
            a2.normalize();
            if ( a1.length2() >= EPSILON ) {
                a1.normalize();
            } else {
                a1 = a2;
            }

            bool v10 = false;
            auto *v9 = this->field_4->get_actor();

            auto &abs_po = v9->get_abs_po();
            vector3d y_facing = abs_po.get_y_facing();
            if ( dot(a3, y_facing) < 1.0f )  {
                v10 = true;
            }

            auto v11 = dot(a1, a2);
            v11 = std::clamp(v11, -1.0f, 1.0f);
            if ( a8 >= v11 || v10 )
            {
                auto v15 = v11;
                auto v16 = bounded_acos(v15);

                abs_po = v9->get_abs_po();
                vector3d x_facing = abs_po.get_x_facing();

                if ( dot(a2, x_facing) > 0.0 ) {
                    v16 *= -1.0f;
                }

                auto v12 = std::min(a7 * a9, 1.0f);
                v16 *= v12;

                po v20 {};
                v20.set_rot(a3, v16);
                auto v21 = v20.non_affine_slow_xform(a1);

                auto v19 = v9->get_abs_position();
                po v22 {};
                v22.set_po(v21, a3, v19);
                entity_set_abs_po(v9, v22);
            }
        }
    } else {
        THISCALL(0x00499E80, this, the_actor, a1, a2, a3, a7, a8, a9);
    }
}

void motion_compensator::set_anim_playback_speed(Float new_anim_speed)
{
    TRACE("motion_compensator::set_anim_playback_speed");

    assert(new_anim_speed < 10.0f && "Unreasonable anim speed shift!");

    auto *the_machine = this->field_4->get_als_layer_internal(static_cast<layer_types>(0));
    auto the_handle = the_machine->get_anim_handle();
    the_handle.set_anim_speed(new_anim_speed);
}

double motion_compensator::get_anim_movement_scale_param()
{
    TRACE("als::motion_compensator::get_anim_movement_scale_param");

    if ( this->field_8->has_ext_param_been_set(0xFu) ) {
        return this->field_8->get_param(this->field_4, 0xFu);
    } else {
        return 1.0;
    }
}


double motion_compensator::get_anim_playback_speed_param()
{
    TRACE("als::motion_compensator::get_anim_playback_speed_param");

    // Converted from 0x004A0F90.
    if ( this->field_8->has_ext_param_been_set(0x10u) ) {
        return this->field_8->get_param(this->field_4, 0x10u);
    }

    return 1.0;
}

}

void als_motion_compensator_patch()
{
    FUNC_ADDRESS(address, &als::motion_compensator::set_facing_to_dir_internal);
    //set_vfunc(0x0087860C, address);

    {
        FUNC_ADDRESS(address, &als::motion_compensator::activate);
        SET_JUMP(0x004A0F30, address);
    }

    {
        FUNC_ADDRESS(address, &als::motion_compensator::set_anim_playback_speed);
        SET_JUMP(0x004A0FC0, address);
    }

    {
        FUNC_ADDRESS(address, &als::motion_compensator::get_anim_movement_scale_param);
        SET_JUMP(0x004A0F60, address);
    }

    {
        FUNC_ADDRESS(address, &als::motion_compensator::get_anim_playback_speed_param);
        SET_JUMP(0x004A0F90, address);
    }
}
