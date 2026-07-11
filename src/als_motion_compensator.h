#pragma once

#include "mash_virtual_base.h"
#include "float.hpp"
#include "vector3d.h"

struct actor;

namespace als {

struct animation_logic_system;
struct state_machine; 

struct motion_compensator : mash_virtual_base {
    animation_logic_system *field_4;
    state_machine *field_8;
    actor *the_actor;
    float field_10;

    void activate(animation_logic_system *a2);

    //virtual
    void pre_anim_action(Float a3);

    //virtual
    void post_anim_action(Float a2);

    //virtual
    void set_facing_to_dir_internal(
        actor *the_actor,
        vector3d a1,
        vector3d a2,
        vector3d a3,
        Float a7,
        Float a8,
        Float a9);

    //virtual
    void set_anim_playback_speed(Float new_anim_speed);

    //virtual
    double get_anim_movement_scale_param();

    double get_anim_playback_speed_param();

};

}

extern void als_motion_compensator_patch();
