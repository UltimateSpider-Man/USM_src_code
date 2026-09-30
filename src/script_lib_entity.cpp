#include "script_lib_entity.h"

#include "actor.h"
#include "base_ai_core.h"
#include "entity_base.h"
#include "entity_base_vhandle.h"
#include "entity_handle_manager.h"
#include "memory.h"
#include "osassert.h"
#include "slc_manager.h"
#include "trace.h"
#include "utility.h"
#include "vm_stack.h"
#include "xbpack.h"

ai::ai_core *get_ai_core_from_vhandle(entity_base_vhandle a1)
{
    auto *ent = a1.get_volatile_ptr();
    assert(ent != nullptr && "This entity is invalid!");

    if ( ent == nullptr ) {
        return nullptr;
    }

    assert(ent->is_an_actor() && "This entity is not an actor!");

    if ( !ent->is_an_actor() ) {
        return nullptr;
    }

    assert(bit_cast<actor *>(ent)->get_ai_core() && "This entity does not have an AI!");
    return ent->get_ai_core();
}

string_hash get_ai_param_hash_and_core(
        entity_base_vhandle a2,
        const char *a3,
        ai::ai_core **a4,
        bool a5)
{
    *a4 = get_ai_core_from_vhandle(a2);
    string_hash a1 {a3};
    if ( *a4 )
    {
        if ( a5 )
        {
            auto *pb = (*a4)->get_param_block();
            if ( !pb->does_parameter_exist(a1) )
            {
                auto *v8 = (*a4)->get_actor(0);
                auto id = v8->get_id();
                auto v10 = id.to_string();
                mString v16 {0, "Unknown AI parameter %s requested by entity %s, from script", a3, v10};
                auto *v11 = v16.c_str();
                error(v11);
            }
        }
    }

    return a1;
}

struct slf__entity__abs_snap_to__entity__t : script_library_class::function {
    slf__entity__abs_snap_to__entity__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl)) 0x0089AE34;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__add_collision_ignorance__entity__t : script_library_class::function {
    slf__entity__add_collision_ignorance__entity__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089AF94;
    }

    struct parms_t {
        entity_base_vhandle me;
        entity_base_vhandle it;
    };

    bool operator()(vm_stack &stack, script_library_class::function::entry_t) const
    {
        TRACE("slf__entity__add_collision_ignorance__entity__t::operator()");

        SLF_PARMS;

        auto *eb_parms_me = bit_cast<actor *>(parms->me.get_volatile_ptr());
        if ( eb_parms_me != nullptr ) {
            auto *eb_parms_it = parms->it.get_volatile_ptr();
            if ( eb_parms_it != nullptr ) {
                if ( eb_parms_me->is_an_actor() && eb_parms_it->is_an_actor() ) {
                    eb_parms_me->add_collision_ignorance(eb_parms_it->my_handle);
                }
            }
        }

        SLF_DONE;
    }
};

struct slf__entity__add_exclusive_interactor__string_hash__interactable_interface__t : script_library_class::function {
    slf__entity__add_exclusive_interactor__string_hash__interactable_interface__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B5C4;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__add_item__entity__t : script_library_class::function {
    slf__entity__add_item__entity__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089AF44;
    }

    struct parms_t {
        entity_base_vhandle me;
        entity_base_vhandle it;
    };

    bool operator()(vm_stack &stack, script_library_class::function::entry_t) const
    {
        TRACE("slf__entity__add_item__entity__t::operator");

        SLF_PARMS;

        auto *eb_parms_me = parms->me.get_volatile_ptr();
        auto *eb_parms_it = parms->it.get_volatile_ptr();
        if ( !eb_parms_me || !eb_parms_it ) {
            SLF_DONE;
        }

        assert(eb_parms_it->get_flavor() == ENTITY_ITEM);

        if ( eb_parms_me->is_an_actor() ) {
            if ( eb_parms_me->is_hero() ) {
                sp_log("This doesn't actually give the item to the hero. Talk to a coder to find out the new way to do this");
            } else {
                bit_cast<actor *>(eb_parms_me)->add_item(eb_parms_it->my_handle.field_0, 1);
            }
        }

        SLF_DONE;
    }
};

struct slf__entity__add_selectable_target__entity__t : script_library_class::function {
    slf__entity__add_selectable_target__entity__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B17C;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__add_vehicle_to_traffic_system__num__t : script_library_class::function {
    slf__entity__add_vehicle_to_traffic_system__num__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B474;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__ai_get_viseme_morph_set__str__str__t : script_library_class::function {
    slf__entity__ai_get_viseme_morph_set__str__str__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B3F4;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__ai_get_viseme_stream__str__t : script_library_class::function {
    slf__entity__ai_get_viseme_stream__str__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B3EC;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__ai_is_speaking__t : script_library_class::function {
    slf__entity__ai_is_speaking__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B40C;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__ai_run_lip_sync__str__t : script_library_class::function {
    slf__entity__ai_run_lip_sync__str__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B3E4;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__ai_say_file__str__num__num__t : script_library_class::function {
    slf__entity__ai_say_file__str__num__num__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B3CC;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__ai_say_gab__str__num__num__t : script_library_class::function {
    slf__entity__ai_say_gab__str__num__num__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B424;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__ai_say_sound_group__str__num__num__t : script_library_class::function {
    slf__entity__ai_say_sound_group__str__num__num__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B414;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__ai_shut_up__t : script_library_class::function {
    slf__entity__ai_shut_up__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B404;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__ai_traffic_come_on_camera__vector3d__num__t : script_library_class::function {
    slf__entity__ai_traffic_come_on_camera__vector3d__num__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B2EC;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__ai_traffic_come_on_camera__vector3d__vector3d__num__t : script_library_class::function {
    slf__entity__ai_traffic_come_on_camera__vector3d__vector3d__num__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B2F4;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__ai_traffic_follow_entity__entity__num__t : script_library_class::function {
    slf__entity__ai_traffic_follow_entity__entity__num__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B2FC;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__ai_traffic_follow_vehicle__entity__t : script_library_class::function {
    slf__entity__ai_traffic_follow_vehicle__entity__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B304;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__ai_traffic_get_value__num__t : script_library_class::function {
    slf__entity__ai_traffic_get_value__num__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B2BC;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__ai_traffic_get_value__num__num__t : script_library_class::function {
    slf__entity__ai_traffic_get_value__num__num__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B2B4;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__ai_traffic_goto__vector3d__num__num__num__t : script_library_class::function {
    slf__entity__ai_traffic_goto__vector3d__num__num__num__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B2CC;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__ai_traffic_set_value__num__num__t : script_library_class::function {
    slf__entity__ai_traffic_set_value__num__num__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B2C4;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__ai_traffic_spawn_away_from__vector3d__num__t : script_library_class::function {
    slf__entity__ai_traffic_spawn_away_from__vector3d__num__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B2DC;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__ai_traffic_spawn_behind__entity__t : script_library_class::function {
    slf__entity__ai_traffic_spawn_behind__entity__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B2E4;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__ai_traffic_spawn_near__vector3d__t : script_library_class::function {
    slf__entity__ai_traffic_spawn_near__vector3d__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B2D4;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__ai_voice_box_set_team_respect__string_hash__num__t : script_library_class::function {
    slf__entity__ai_voice_box_set_team_respect__string_hash__num__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B3C4;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__ai_wait_say_file__str__num__num__t : script_library_class::function {
    slf__entity__ai_wait_say_file__str__num__num__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B3D4;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__ai_wait_say_gab__str__num__num__t : script_library_class::function {
    slf__entity__ai_wait_say_gab__str__num__num__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B42C;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__ai_wait_say_preregistered_file__str__num__num__t : script_library_class::function {
    slf__entity__ai_wait_say_preregistered_file__str__num__num__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B3DC;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__ai_wait_say_sound_group__str__num__num__t : script_library_class::function {
    slf__entity__ai_wait_say_sound_group__str__num__num__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B41C;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__anim_finished__t : script_library_class::function {
    slf__entity__anim_finished__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089AEC4;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__anim_finished__num__t : script_library_class::function {
    slf__entity__anim_finished__num__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089AECC;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__apply_continuous_rotation__vector3d__num__num__t : script_library_class::function {
    slf__entity__apply_continuous_rotation__vector3d__num__num__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089ACA4;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__apply_damage__num__t : script_library_class::function {
    slf__entity__apply_damage__num__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089ADBC;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__apply_directed_damage__num__vector3d__t : script_library_class::function {
    slf__entity__apply_directed_damage__num__vector3d__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089ADD4;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__apply_directed_damage_cat__num__vector3d__str__num__t : script_library_class::function {
    slf__entity__apply_directed_damage_cat__num__vector3d__str__num__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089ADDC;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__apply_explosive_damage__num__vector3d__t : script_library_class::function {
    slf__entity__apply_explosive_damage__num__vector3d__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089ADE4;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__apply_explosive_damage__num__vector3d__vector3d__t : script_library_class::function {
    slf__entity__apply_explosive_damage__num__vector3d__vector3d__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089ADEC;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__apply_subdue__num__t : script_library_class::function {
    slf__entity__apply_subdue__num__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089ADC4;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__camera_get_target__t : script_library_class::function {
    slf__entity__camera_get_target__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089AE44;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__camera_orbit__vector3d__num__num__num__t : script_library_class::function {
    slf__entity__camera_orbit__vector3d__num__num__num__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089AE74;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__camera_set_collide_with_world__num__t : script_library_class::function {
    slf__entity__camera_set_collide_with_world__num__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089AE5C;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__camera_set_roll__num__t : script_library_class::function {
    slf__entity__camera_set_roll__num__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089AE4C;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__camera_set_target__vector3d__t : script_library_class::function {
    slf__entity__camera_set_target__vector3d__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089AE3C;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__camera_slide_to__vector3d__vector3d__num__num__t : script_library_class::function {
    slf__entity__camera_slide_to__vector3d__vector3d__num__num__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089AE64;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__camera_slide_to_orbit__vector3d__num__num__num__num__t : script_library_class::function {
    slf__entity__camera_slide_to_orbit__vector3d__num__num__num__num__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089AE6C;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__cancel_tether__t : script_library_class::function {
    slf__entity__cancel_tether__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B1D4;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__car_random_body_and_color__t : script_library_class::function {
    slf__entity__car_random_body_and_color__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B1C4;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__change_ai_base_machine__str__t : script_library_class::function {
    slf__entity__change_ai_base_machine__str__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B144;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__collisions_enabled__t : script_library_class::function {
    slf__entity__collisions_enabled__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B43C;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__compute_sector__t : script_library_class::function {
    slf__entity__compute_sector__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089ABE4;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__create_damage_interface__t : script_library_class::function {
    slf__entity__create_damage_interface__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B34C;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__create_interactable_ifc__t : script_library_class::function {
    slf__entity__create_interactable_ifc__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B5B4;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__create_physical_interface__t : script_library_class::function {
    slf__entity__create_physical_interface__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B344;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__create_script_data_interface__t : script_library_class::function {
    slf__entity__create_script_data_interface__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B334;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__create_web_ifc__str__num__num__t : script_library_class::function {
    slf__entity__create_web_ifc__str__num__num__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B534;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__disable_as_target__t : script_library_class::function {
    slf__entity__disable_as_target__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B14C;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__disable_collisions__t : script_library_class::function {
    slf__entity__disable_collisions__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089AF84;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__disable_fading__t : script_library_class::function {
    slf__entity__disable_fading__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089ADB4;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__disgorge_items__t : script_library_class::function {
    slf__entity__disgorge_items__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089AF74;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__enable_as_target__t : script_library_class::function {
    slf__entity__enable_as_target__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B154;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__enable_collisions__t : script_library_class::function {
    slf__entity__enable_collisions__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089AF8C;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__enable_collisions__num__t : script_library_class::function {
    slf__entity__enable_collisions__num__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B434;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__eye_check_ent__entity__t : script_library_class::function {
    slf__entity__eye_check_ent__entity__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B054;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__eye_check_pos__vector3d__t : script_library_class::function {
    slf__entity__eye_check_pos__vector3d__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B04C;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__force_activate_interaction__string_hash__interactable_interface__t : script_library_class::function {
    slf__entity__force_activate_interaction__string_hash__interactable_interface__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B62C;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__force_current_region__t : script_library_class::function {
    slf__entity__force_current_region__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089AD24;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__force_region__entity__t : script_library_class::function {
    slf__entity__force_region__entity__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089AD1C;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__get_abs_position__t : script_library_class::function {
    slf__entity__get_abs_position__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089ABCC;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__get_ai_base_machine_name__t : script_library_class::function {
    slf__entity__get_ai_base_machine_name__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B16C;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__get_ai_param_float__str__t : script_library_class::function {
    slf__entity__get_ai_param_float__str__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B074;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__get_ai_param_hash__str__t : script_library_class::function {
    slf__entity__get_ai_param_hash__str__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B08C;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__get_ai_param_int__str__t : script_library_class::function {
    slf__entity__get_ai_param_int__str__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B07C;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__get_ai_param_str__str__t : script_library_class::function {
    slf__entity__get_ai_param_str__str__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B084;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__get_ai_param_vector3d__str__t : script_library_class::function {
    slf__entity__get_ai_param_vector3d__str__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B094;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__get_ai_signaller__t : script_library_class::function {
    slf__entity__get_ai_signaller__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089AF6C;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__get_anchor_point__t : script_library_class::function {
    slf__entity__get_anchor_point__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089AC1C;
    }

    struct parms_t {
        entity_base_vhandle me;
    };

    bool operator()(vm_stack &stack, script_library_class::function::entry_t) const
    {
        SLF_PARMS;

        vector3d result {0, 0, 0};
        if (parms->me.get_volatile_ptr() != nullptr) {
            assert(0 && "This function has been disabled (for now?)");
        }

        SLF_RETURN;
        SLF_DONE;
    }
};

struct slf__entity__get_carry_slave__t : script_library_class::function {
    slf__entity__get_carry_slave__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B19C;
    }

    struct parms_t {
        entity_base_vhandle me;
    };

    bool operator()(vm_stack &stack, script_library_class::function::entry_t) const {
        SLF_PARMS;

        SLF_DONE;
    }
};

struct slf__entity__get_current_animation_name__t : script_library_class::function {
    slf__entity__get_current_animation_name__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B044;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__get_damage_force__t : script_library_class::function {
    slf__entity__get_damage_force__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089ABD4;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__get_debug_name__t : script_library_class::function {
    slf__entity__get_debug_name__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B444;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__get_detonate_position__t : script_library_class::function {
    slf__entity__get_detonate_position__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089ABDC;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__get_district__t : script_library_class::function {
    slf__entity__get_district__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B024;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__get_facing__t : script_library_class::function {
    slf__entity__get_facing__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089ABEC;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__get_fade_timer__t : script_library_class::function {
    slf__entity__get_fade_timer__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B1AC;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__get_first_child__t : script_library_class::function {
    slf__entity__get_first_child__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089AC4C;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__get_hash_name__t : script_library_class::function {
    slf__entity__get_hash_name__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B44C;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__get_hidey_pos__vector3d__num__t : script_library_class::function {
    slf__entity__get_hidey_pos__vector3d__num__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089AC2C;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__get_ifc_num__str__t : script_library_class::function {
    slf__entity__get_ifc_num__str__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B284;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__get_ifc_str__str__t : script_library_class::function {
    slf__entity__get_ifc_str__str__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B2A4;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__get_ifc_vec__str__t : script_library_class::function {
    slf__entity__get_ifc_vec__str__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B294;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__get_ifl_frame__t : script_library_class::function {
    slf__entity__get_ifl_frame__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B03C;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__get_inode_param_float__str__str__t : script_library_class::function {
    slf__entity__get_inode_param_float__str__str__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B0DC;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__get_inode_param_hash__str__str__t : script_library_class::function {
    slf__entity__get_inode_param_hash__str__str__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B0F4;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__get_inode_param_int__str__str__t : script_library_class::function {
    slf__entity__get_inode_param_int__str__str__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B0E4;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__get_inode_param_str__str__str__t : script_library_class::function {
    slf__entity__get_inode_param_str__str__str__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B0EC;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__get_inode_param_vector3d__str__str__t : script_library_class::function {
    slf__entity__get_inode_param_vector3d__str__str__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B0FC;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__get_interactable_ifc__t : script_library_class::function {
    slf__entity__get_interactable_ifc__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B5BC;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__get_item__num__t : script_library_class::function {
    slf__entity__get_item__num__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089AF54;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__get_item_by_name__str__t : script_library_class::function {
    slf__entity__get_item_by_name__str__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089AF5C;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__get_item_quantity__num__t : script_library_class::function {
    slf__entity__get_item_quantity__num__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089AF64;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__get_last_anchor__t : script_library_class::function {
    slf__entity__get_last_anchor__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089AC0C;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__get_last_attacker__t : script_library_class::function {
    slf__entity__get_last_attacker__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089AC14;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__get_last_item_used__t : script_library_class::function {
    slf__entity__get_last_item_used__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B20C;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__get_member__str__t : script_library_class::function {
    slf__entity__get_member__str__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089AE8C;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__get_next_sibling__t : script_library_class::function {
    slf__entity__get_next_sibling__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089AC54;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__get_num_items__t : script_library_class::function {
    slf__entity__get_num_items__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089AF4C;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__get_parent__t : script_library_class::function {
    slf__entity__get_parent__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089AC44;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__get_pendulum_length__t : script_library_class::function {
    slf__entity__get_pendulum_length__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B3B4;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__get_rel_position__t : script_library_class::function {
    slf__entity__get_rel_position__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089AC24;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__get_rel_velocity__entity__t : script_library_class::function {
    slf__entity__get_rel_velocity__entity__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089AD04;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__get_render_alpha__t : script_library_class::function {
    slf__entity__get_render_alpha__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B254;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__get_render_color__t : script_library_class::function {
    slf__entity__get_render_color__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B24C;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__get_scripted_target__t : script_library_class::function {
    slf__entity__get_scripted_target__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B194;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__get_sector_name__t : script_library_class::function {
    slf__entity__get_sector_name__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089AD14;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__get_state__num__t : script_library_class::function {
    slf__entity__get_state__num__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B56C;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__get_time_dilation__t : script_library_class::function {
    slf__entity__get_time_dilation__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B314;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__get_time_mode__t : script_library_class::function {
    slf__entity__get_time_mode__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B324;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__get_x_facing__t : script_library_class::function {
    slf__entity__get_x_facing__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089ABF4;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__get_y_facing__t : script_library_class::function {
    slf__entity__get_y_facing__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089ABFC;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__get_z_facing__t : script_library_class::function {
    slf__entity__get_z_facing__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089AC04;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__has_carry_slave__t : script_library_class::function {
    slf__entity__has_carry_slave__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B1A4;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__has_member__str__t : script_library_class::function {
    slf__entity__has_member__str__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089AE94;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__has_script_data_interface__t : script_library_class::function {
    slf__entity__has_script_data_interface__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B33C;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__hates__entity__t : script_library_class::function {
    slf__entity__hates__entity__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B51C;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__ifl_damage_lock__num__t : script_library_class::function {
    slf__entity__ifl_damage_lock__num__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B004;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__ifl_lock__num__t : script_library_class::function {
    slf__entity__ifl_lock__num__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B00C;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__ifl_pause__t : script_library_class::function {
    slf__entity__ifl_pause__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B014;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__ifl_play__t : script_library_class::function {
    slf__entity__ifl_play__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B01C;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__in_sector__vector3d__vector3d__num__t : script_library_class::function {
    slf__entity__in_sector__vector3d__vector3d__num__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089AD0C;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__inhibit_universal_soldier_ability__str__num__t : script_library_class::function {
    slf__entity__inhibit_universal_soldier_ability__str__num__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B174;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__invoke_facial_expression__num__num__num__num__t : script_library_class::function {
    slf__entity__invoke_facial_expression__num__num__num__num__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B3FC;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__is_a_car__t : script_library_class::function {
    slf__entity__is_a_car__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089AD5C;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__is_picked_up__t : script_library_class::function {
    slf__entity__is_picked_up__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089AE1C;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__is_suspended__t : script_library_class::function {
    slf__entity__is_suspended__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B45C;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__is_throwable__t : script_library_class::function {
    slf__entity__is_throwable__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089AD64;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__is_valid__t : script_library_class::function {
    slf__entity__is_valid__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B464;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__is_visible__t : script_library_class::function {
    slf__entity__is_visible__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089AD74;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__kill_anim_in_slot__num__t : script_library_class::function {
    slf__entity__kill_anim_in_slot__num__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089AEBC;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__likes__entity__t : script_library_class::function {
    slf__entity__likes__entity__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B524;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__look_at__vector3d__t : script_library_class::function {
    slf__entity__look_at__vector3d__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089AC7C;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__motion_blur_off__t : script_library_class::function {
    slf__entity__motion_blur_off__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089ADFC;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__motion_blur_on__num__num__num__t : script_library_class::function {
    slf__entity__motion_blur_on__num__num__num__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089ADF4;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__motion_trail_off__t : script_library_class::function {
    slf__entity__motion_trail_off__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089AE14;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__motion_trail_on__entity__entity__vector3d__num__num__num__num__t : script_library_class::function {
    slf__entity__motion_trail_on__entity__entity__vector3d__num__num__num__num__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089AE04;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__motion_trail_on__entity__str__num__num__vector3d__num__num__num__num__t : script_library_class::function {
    slf__entity__motion_trail_on__entity__str__num__num__vector3d__num__num__num__num__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089AE0C;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__neutral__entity__t : script_library_class::function {
    slf__entity__neutral__entity__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B52C;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__operator_not_equals__entity__t : script_library_class::function {
    slf__entity__operator_not_equals__entity__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089AD3C;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__operator_equals_equals__entity__t : script_library_class::function {
    slf__entity__operator_equals_equals__entity__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089AD34;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__physical_ifc_add_particle__str__t : script_library_class::function {
    slf__entity__physical_ifc_add_particle__str__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B4A4;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__physical_ifc_apply_force__vector3d__num__t : script_library_class::function {
    slf__entity__physical_ifc_apply_force__vector3d__num__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B354;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__physical_ifc_cancel_all_velocity__t : script_library_class::function {
    slf__entity__physical_ifc_cancel_all_velocity__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B36C;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__physical_ifc_clear_pendulum__t : script_library_class::function {
    slf__entity__physical_ifc_clear_pendulum__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B3A4;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__physical_ifc_get_attached_particle_name__t : script_library_class::function {
    slf__entity__physical_ifc_get_attached_particle_name__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B4BC;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__physical_ifc_get_bounce_particle_name__t : script_library_class::function {
    slf__entity__physical_ifc_get_bounce_particle_name__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B4AC;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__physical_ifc_is_biped_physics_running__t : script_library_class::function {
    slf__entity__physical_ifc_is_biped_physics_running__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B374;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__physical_ifc_is_effectively_standing__t : script_library_class::function {
    slf__entity__physical_ifc_is_effectively_standing__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B35C;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__physical_ifc_is_prop_physics_at_rest__t : script_library_class::function {
    slf__entity__physical_ifc_is_prop_physics_at_rest__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B4CC;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__physical_ifc_is_prop_physics_running__t : script_library_class::function {
    slf__entity__physical_ifc_is_prop_physics_running__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B4D4;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__physical_ifc_manage_standing__num__t : script_library_class::function {
    slf__entity__physical_ifc_manage_standing__num__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B364;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__physical_ifc_set_allow_biped_physics__num__t : script_library_class::function {
    slf__entity__physical_ifc_set_allow_biped_physics__num__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B38C;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__physical_ifc_set_attached_particle_name__str__t : script_library_class::function {
    slf__entity__physical_ifc_set_attached_particle_name__str__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B4C4;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__physical_ifc_set_bounce_particle_name__str__t : script_library_class::function {
    slf__entity__physical_ifc_set_bounce_particle_name__str__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B4B4;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__physical_ifc_set_pendulum__entity__num__t : script_library_class::function {
    slf__entity__physical_ifc_set_pendulum__entity__num__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B39C;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__physical_ifc_set_pendulum__vector3d__num__t : script_library_class::function {
    slf__entity__physical_ifc_set_pendulum__vector3d__num__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B394;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__physical_ifc_start_biped_physics__t : script_library_class::function {
    slf__entity__physical_ifc_start_biped_physics__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B37C;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__physical_ifc_start_prop_physics__vector3d__num__t : script_library_class::function {
    slf__entity__physical_ifc_start_prop_physics__vector3d__num__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B494;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__physical_ifc_stop_biped_physics__t : script_library_class::function {
    slf__entity__physical_ifc_stop_biped_physics__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B384;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__physical_ifc_stop_prop_physics__t : script_library_class::function {
    slf__entity__physical_ifc_stop_prop_physics__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B49C;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__play_anim__str__t : script_library_class::function {
    slf__entity__play_anim__str__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089AEA4;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__play_anim__str__num__num__t : script_library_class::function {
    slf__entity__play_anim__str__num__num__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089AEAC;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__poison__num__num__t : script_library_class::function {
    slf__entity__poison__num__num__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B1DC;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__pop_ai_base_machine__t : script_library_class::function {
    slf__entity__pop_ai_base_machine__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B164;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__pre_roll__num__t : script_library_class::function {
    slf__entity__pre_roll__num__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B55C;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__push_ai_base_machine__str__t : script_library_class::function {
    slf__entity__push_ai_base_machine__str__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B15C;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__randomize_position__vector3d__num__num__num__t : script_library_class::function {
    slf__entity__randomize_position__vector3d__num__num__num__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B4DC;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__regenerate__num__num__t : script_library_class::function {
    slf__entity__regenerate__num__num__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B1E4;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__rel_angle__vector3d__t : script_library_class::function {
    slf__entity__rel_angle__vector3d__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089AC34;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__remove_collision_ignorance__entity__t : script_library_class::function {
    slf__entity__remove_collision_ignorance__entity__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089AF9C;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__remove_exclusive_interactor__string_hash__interactable_interface__t : script_library_class::function {
    slf__entity__remove_exclusive_interactor__string_hash__interactable_interface__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B5CC;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__remove_selectable_target__entity__t : script_library_class::function {
    slf__entity__remove_selectable_target__entity__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B184;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__remove_vehicle_from_traffic_system__t : script_library_class::function {
    slf__entity__remove_vehicle_from_traffic_system__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B47C;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__render_name__num__t : script_library_class::function {
    slf__entity__render_name__num__t(script_library_class *slc, const char *name) :
            function(slc, name)
    {
        m_vtbl = CAST(m_vtbl, 0x0089B47C);
        auto local_vtbl = CAST(m_vtbl, mem_alloc(sizeof(*m_vtbl)));
        *local_vtbl = *m_vtbl;
        FUNC_ADDRESS(address, &slf__entity__render_name__num__t::operator());
        local_vtbl->__cl = CAST(local_vtbl->__cl, address);
        m_vtbl = local_vtbl;
    }

    struct parms_t {
        entity_base_vhandle me;
        vm_num_t enabled;
    };

    bool operator()(vm_stack &stack, entry_t) const
    {
        SLF_PARMS;

        auto *entity = parms->me.get_volatile_ptr();
        if (entity != nullptr && parms->enabled != 0.0f) {
            (void)entity->get_id().to_string();
        }

        return true;
    }
};

struct slf__entity__reset_ai__t : script_library_class::function {
    slf__entity__reset_ai__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B06C;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__restart__t : script_library_class::function {
    slf__entity__restart__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B564;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__seriously_kill__t : script_library_class::function {
    slf__entity__seriously_kill__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089ADCC;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__set_abs_xz_facing__vector3d__t : script_library_class::function {
    slf__entity__set_abs_xz_facing__vector3d__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089AC94;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__set_active__num__t : script_library_class::function {
    slf__entity__set_active__num__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089AD9C;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__set_ai_param_float__str__num__t : script_library_class::function {
    slf__entity__set_ai_param_float__str__num__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B09C;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const
    {

        SLF_DONE;
    }
};

struct slf__entity__set_ai_param_float_variance__str__num__num__t : script_library_class::function {
    slf__entity__set_ai_param_float_variance__str__num__num__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B0A4;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__set_ai_param_hash__str__num__t : script_library_class::function {
    slf__entity__set_ai_param_hash__str__num__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B0C4;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__set_ai_param_hash__str__str__t : script_library_class::function {
    slf__entity__set_ai_param_hash__str__str__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B0BC;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__set_ai_param_hash__str__string_hash__t : script_library_class::function {
    slf__entity__set_ai_param_hash__str__string_hash__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B0CC;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__set_ai_param_int__str__num__t : script_library_class::function {
    slf__entity__set_ai_param_int__str__num__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B0AC;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__set_ai_param_str__str__str__t : script_library_class::function {
    slf__entity__set_ai_param_str__str__str__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B0B4;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__set_ai_param_vector3d__str__vector3d__t : script_library_class::function {
    slf__entity__set_ai_param_vector3d__str__vector3d__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B0D4;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__set_ambient_factor__vector3d__t : script_library_class::function {
    slf__entity__set_ambient_factor__vector3d__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B32C;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__set_anchor_activated__num__t : script_library_class::function {
    slf__entity__set_anchor_activated__num__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089AFA4;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__set_car_combat_info__num__num__num__num__num__num__num__num__t : script_library_class::function {
    slf__entity__set_car_combat_info__num__num__num__num__num__num__num__num__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B1BC;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__set_crawlable__num__t : script_library_class::function {
    slf__entity__set_crawlable__num__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089AD84;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__set_default_variant__t : script_library_class::function {
    slf__entity__set_default_variant__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B544;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__set_distance_clip__num__t : script_library_class::function {
    slf__entity__set_distance_clip__num__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089ADAC;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__set_entity_blur__num__t : script_library_class::function {
    slf__entity__set_entity_blur__num__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B224;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__set_facing__vector3d__vector3d__t : script_library_class::function {
    slf__entity__set_facing__vector3d__vector3d__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089AC8C;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__set_fade_timer__num__t : script_library_class::function {
    slf__entity__set_fade_timer__num__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B1B4;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__set_hires_shadow__num__t : script_library_class::function {
    slf__entity__set_hires_shadow__num__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089AD4C;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__set_ifc_num__str__num__t : script_library_class::function {
    slf__entity__set_ifc_num__str__num__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B28C;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__set_ifc_str__str__str__t : script_library_class::function {
    slf__entity__set_ifc_str__str__str__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B2AC;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__set_ifc_vec__str__vector3d__t : script_library_class::function {
    slf__entity__set_ifc_vec__str__vector3d__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B29C;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__set_ignore_limbo__num__t : script_library_class::function {
    slf__entity__set_ignore_limbo__num__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B4E4;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__set_immobile__num__t : script_library_class::function {
    slf__entity__set_immobile__num__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B484;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__set_inode_param_entity__str__str__entity__t : script_library_class::function {
    slf__entity__set_inode_param_entity__str__str__entity__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B11C;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__set_inode_param_float__str__str__num__t : script_library_class::function {
    slf__entity__set_inode_param_float__str__str__num__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B104;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__set_inode_param_float_variance__str__str__num__num__t : script_library_class::function {
    slf__entity__set_inode_param_float_variance__str__str__num__num__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B10C;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__set_inode_param_hash__str__str__num__t : script_library_class::function {
    slf__entity__set_inode_param_hash__str__str__num__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B134;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__set_inode_param_hash__str__str__str__t : script_library_class::function {
    slf__entity__set_inode_param_hash__str__str__str__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B12C;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__set_inode_param_int__str__str__num__t : script_library_class::function {
    slf__entity__set_inode_param_int__str__str__num__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B114;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__set_inode_param_str__str__str__str__t : script_library_class::function {
    slf__entity__set_inode_param_str__str__str__str__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B124;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__set_inode_param_vector3d__str__str__vector3d__t : script_library_class::function {
    slf__entity__set_inode_param_vector3d__str__str__vector3d__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B13C;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__set_invulnerable__num__t : script_library_class::function {
    slf__entity__set_invulnerable__num__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089AD44;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__set_kill_ent_on_destroy__num__t : script_library_class::function {
    slf__entity__set_kill_ent_on_destroy__num__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B48C;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__set_member_hidden__num__t : script_library_class::function {
    slf__entity__set_member_hidden__num__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089AE9C;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__set_parent__entity__t : script_library_class::function {
    slf__entity__set_parent__entity__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089AC3C;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__set_parent_rel__entity__t : script_library_class::function {
    slf__entity__set_parent_rel__entity__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089AC5C;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__set_path_graph__str__t : script_library_class::function {
    slf__entity__set_path_graph__str__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B214;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__set_path_graph_start_node__num__t : script_library_class::function {
    slf__entity__set_path_graph_start_node__num__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B21C;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__set_pendulum_attach_limb__num__t : script_library_class::function {
    slf__entity__set_pendulum_attach_limb__num__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B3BC;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__set_pendulum_length__num__t : script_library_class::function {
    slf__entity__set_pendulum_length__num__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B3AC;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__set_physical__num__t : script_library_class::function {
    slf__entity__set_physical__num__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089ADA4;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__set_po_facing__vector3d__t : script_library_class::function {
    slf__entity__set_po_facing__vector3d__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089AC9C;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__set_rel_position__vector3d__t : script_library_class::function {
    slf__entity__set_rel_position__vector3d__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089AC64;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__set_render_alpha__num__t : script_library_class::function {
    slf__entity__set_render_alpha__num__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B244;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__set_render_color__vector3d__t : script_library_class::function {
    slf__entity__set_render_color__vector3d__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B23C;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__set_render_scale__vector3d__t : script_library_class::function {
    slf__entity__set_render_scale__vector3d__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B22C;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__set_scale__num__t : script_library_class::function {
    slf__entity__set_scale__num__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B274;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__set_scripted_target__entity__t : script_library_class::function {
    slf__entity__set_scripted_target__entity__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B18C;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__set_see_thru__num__t : script_library_class::function {
    slf__entity__set_see_thru__num__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089AD94;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__set_special_target__num__t : script_library_class::function {
    slf__entity__set_special_target__num__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B454;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__set_state__num__num__t : script_library_class::function {
    slf__entity__set_state__num__num__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B574;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__set_targetting__num__t : script_library_class::function {
    slf__entity__set_targetting__num__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089AD54;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__set_throwable__num__t : script_library_class::function {
    slf__entity__set_throwable__num__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089AD6C;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__set_time_dilation__num__t : script_library_class::function {
    slf__entity__set_time_dilation__num__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B30C;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__set_time_mode__num__t : script_library_class::function {
    slf__entity__set_time_mode__num__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B31C;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__set_variant__string_hash__t : script_library_class::function {
    slf__entity__set_variant__string_hash__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B53C;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__set_velocity__vector3d__t : script_library_class::function {
    slf__entity__set_velocity__vector3d__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089AC74;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__set_visible__num__t : script_library_class::function {
    slf__entity__set_visible__num__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089AD7C;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__set_visible_and_disable_fading__num__t : script_library_class::function {
    slf__entity__set_visible_and_disable_fading__num__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089AD8C;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__set_xz_facing__vector3d__t : script_library_class::function {
    slf__entity__set_xz_facing__vector3d__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089AC84;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__setup_tether__vector3d__num__t : script_library_class::function {
    slf__entity__setup_tether__vector3d__num__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B1CC;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__snap_to__entity__t : script_library_class::function {
    slf__entity__snap_to__entity__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089AE2C;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__suspend__t : script_library_class::function {
    slf__entity__suspend__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089AF7C;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__teleport_to_point__vector3d__t : script_library_class::function {
    slf__entity__teleport_to_point__vector3d__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089AC6C;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__unforce_regions__t : script_library_class::function {
    slf__entity__unforce_regions__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089AD2C;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__unsuspend__t : script_library_class::function {
    slf__entity__unsuspend__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089AFAC;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__use_item__str__t : script_library_class::function {
    slf__entity__use_item__str__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B1FC;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__use_item_by_id__str__t : script_library_class::function {
    slf__entity__use_item_by_id__str__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B204;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__wait_camera_set_roll__num__num__t : script_library_class::function {
    slf__entity__wait_camera_set_roll__num__num__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089AE54;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__wait_change_color__vector3d__vector3d__num__t : script_library_class::function {
    slf__entity__wait_change_color__vector3d__vector3d__num__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089AE7C;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__wait_change_range__num__num__num__t : script_library_class::function {
    slf__entity__wait_change_range__num__num__num__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089AE84;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__wait_change_render_color__vector3d__num__num__t : script_library_class::function {
    slf__entity__wait_change_render_color__vector3d__num__num__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B25C;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__wait_change_render_scale__vector3d__num__t : script_library_class::function {
    slf__entity__wait_change_render_scale__vector3d__num__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B234;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__wait_for_not_sector__str__t : script_library_class::function {
    slf__entity__wait_for_not_sector__str__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089AFEC;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__wait_for_pickup__t : script_library_class::function {
    slf__entity__wait_for_pickup__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089AFB4;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__wait_for_sector__str__t : script_library_class::function {
    slf__entity__wait_for_sector__str__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089AFE4;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__wait_lookat__vector3d__num__t : script_library_class::function {
    slf__entity__wait_lookat__vector3d__num__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B26C;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__wait_lookat2__entity__entity__vector3d__num__t : script_library_class::function {
    slf__entity__wait_lookat2__entity__entity__vector3d__num__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B264;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__wait_looping_anim__str__num__num__t : script_library_class::function {
    slf__entity__wait_looping_anim__str__num__num__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B1F4;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__wait_play_anim__str__num__num__num__t : script_library_class::function {
    slf__entity__wait_play_anim__str__num__num__num__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089AEB4;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__wait_prox__entity__num__t : script_library_class::function {
    slf__entity__wait_prox__entity__num__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089AFBC;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__wait_prox__vector3d__num__t : script_library_class::function {
    slf__entity__wait_prox__vector3d__num__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089AFC4;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__wait_prox__vector3d__num__vector3d__num__t : script_library_class::function {
    slf__entity__wait_prox__vector3d__num__vector3d__num__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089AFCC;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__wait_prox_maxY__vector3d__num__num__t : script_library_class::function {
    slf__entity__wait_prox_maxY__vector3d__num__num__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089AFDC;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__wait_prox_minY__vector3d__num__num__t : script_library_class::function {
    slf__entity__wait_prox_minY__vector3d__num__num__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089AFD4;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__wait_prox_sector__vector3d__num__str__t : script_library_class::function {
    slf__entity__wait_prox_sector__vector3d__num__str__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089AFF4;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__wait_rotate__vector3d__num__num__t : script_library_class::function {
    slf__entity__wait_rotate__vector3d__num__num__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089ACAC;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__wait_rotate_WCS__vector3d__vector3d__num__num__t : script_library_class::function {
    slf__entity__wait_rotate_WCS__vector3d__vector3d__num__num__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089ACBC;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__wait_rotate_WCS_cosmetic__vector3d__vector3d__num__num__t : script_library_class::function {
    slf__entity__wait_rotate_WCS_cosmetic__vector3d__vector3d__num__num__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089ACCC;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__wait_rotate_WCS_with_compute_sector__vector3d__vector3d__num__num__t : script_library_class::function {
    slf__entity__wait_rotate_WCS_with_compute_sector__vector3d__vector3d__num__num__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089ACC4;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__wait_rotate_cosmetic__vector3d__num__num__t : script_library_class::function {
    slf__entity__wait_rotate_cosmetic__vector3d__num__num__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089ACB4;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__wait_set_scale__num__num__t : script_library_class::function {
    slf__entity__wait_set_scale__num__num__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089B27C;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__wait_translate__vector3d__num__t : script_library_class::function {
    slf__entity__wait_translate__vector3d__num__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089ACD4;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__wait_translate_WCS__vector3d__num__t : script_library_class::function {
    slf__entity__wait_translate_WCS__vector3d__num__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089ACEC;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__wait_translate_WCS_cosmetic__vector3d__num__t : script_library_class::function {
    slf__entity__wait_translate_WCS_cosmetic__vector3d__num__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089ACFC;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__wait_translate_WCS_with_compute_sector__vector3d__num__t : script_library_class::function {
    slf__entity__wait_translate_WCS_with_compute_sector__vector3d__num__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089ACF4;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__wait_translate_cosmetic__vector3d__num__t : script_library_class::function {
    slf__entity__wait_translate_cosmetic__vector3d__num__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089ACE4;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__wait_translate_with_compute_sector__vector3d__num__t : script_library_class::function {
    slf__entity__wait_translate_with_compute_sector__vector3d__num__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089ACDC;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

struct slf__entity__was_occluded_last_frame__t : script_library_class::function {
    slf__entity__was_occluded_last_frame__t (script_library_class *slc, const char *a3) : function(slc, a3)  {
        m_vtbl = (decltype(m_vtbl))0x0089AE24;
    }

    bool operator()(vm_stack &, script_library_class::function::entry_t) const { return true;}
};

slc_entity_t *slc_entity = nullptr;

void register_entity_lib()
{
#define BUILD_SLF_NAME(_KLASS, _TYPE) slf__ ## _KLASS ## __ ## _TYPE ## __t

#define CREATE_SLF(KLASS, TYPE, NAME)                                              \
    new (mem_alloc(sizeof(BUILD_SLF_NAME(KLASS, TYPE)))) BUILD_SLF_NAME(KLASS, TYPE) {slc_entity, NAME}

    CREATE_SLF(entity, abs_snap_to__entity, "abs_snap_to(entity)");
    CREATE_SLF(entity, add_collision_ignorance__entity, "add_collision_ignorance(entity)");
    CREATE_SLF(entity, add_exclusive_interactor__string_hash__interactable_interface, "add_exclusive_interactor(string_hash,interactable_interface)");
    CREATE_SLF(entity, add_item__entity, "add_item(entity)");
    CREATE_SLF(entity, add_selectable_target__entity, "add_selectable_target(entity)");
    CREATE_SLF(entity, add_vehicle_to_traffic_system__num, "add_vehicle_to_traffic_system(num)");
    CREATE_SLF(entity, ai_get_viseme_morph_set__str__str, "ai_get_viseme_morph_set(str,str)");
    CREATE_SLF(entity, ai_get_viseme_stream__str, "ai_get_viseme_stream(str)");
    CREATE_SLF(entity, ai_is_speaking, "ai_is_speaking()");
    CREATE_SLF(entity, ai_run_lip_sync__str, "ai_run_lip_sync(str)");
    CREATE_SLF(entity, ai_say_file__str__num__num, "ai_say_file(str,num,num)");
    CREATE_SLF(entity, ai_say_gab__str__num__num, "ai_say_gab(str,num,num)");
    CREATE_SLF(entity, ai_say_sound_group__str__num__num, "ai_say_sound_group(str,num,num)");
    CREATE_SLF(entity, ai_shut_up, "ai_shut_up()");
    CREATE_SLF(entity, ai_traffic_come_on_camera__vector3d__num, "ai_traffic_come_on_camera(vector3d,num)");
    CREATE_SLF(entity, ai_traffic_come_on_camera__vector3d__vector3d__num, "ai_traffic_come_on_camera(vector3d,vector3d,num)");
    CREATE_SLF(entity, ai_traffic_follow_entity__entity__num, "ai_traffic_follow_entity(entity,num)");
    CREATE_SLF(entity, ai_traffic_follow_vehicle__entity, "ai_traffic_follow_vehicle(entity)");
    CREATE_SLF(entity, ai_traffic_get_value__num, "ai_traffic_get_value(num)");
    CREATE_SLF(entity, ai_traffic_get_value__num__num, "ai_traffic_get_value(num,num)");
    CREATE_SLF(entity, ai_traffic_goto__vector3d__num__num__num, "ai_traffic_goto(vector3d,num,num,num)");
    CREATE_SLF(entity, ai_traffic_set_value__num__num, "ai_traffic_set_value(num,num)");
    CREATE_SLF(entity, ai_traffic_spawn_away_from__vector3d__num, "ai_traffic_spawn_away_from(vector3d,num)");
    CREATE_SLF(entity, ai_traffic_spawn_behind__entity, "ai_traffic_spawn_behind(entity)");
    CREATE_SLF(entity, ai_traffic_spawn_near__vector3d, "ai_traffic_spawn_near(vector3d)");
    CREATE_SLF(entity, ai_voice_box_set_team_respect__string_hash__num, "ai_voice_box_set_team_respect(string_hash,num)");
    CREATE_SLF(entity, ai_wait_say_file__str__num__num, "ai_wait_say_file(str,num,num)");
    CREATE_SLF(entity, ai_wait_say_gab__str__num__num, "ai_wait_say_gab(str,num,num)");
    CREATE_SLF(entity, ai_wait_say_preregistered_file__str__num__num, "ai_wait_say_preregistered_file(str,num,num)");
    CREATE_SLF(entity, ai_wait_say_sound_group__str__num__num, "ai_wait_say_sound_group(str,num,num)");
    CREATE_SLF(entity, anim_finished, "anim_finished()");
    CREATE_SLF(entity, anim_finished__num, "anim_finished(num)");
    CREATE_SLF(entity, apply_continuous_rotation__vector3d__num__num, "apply_continuous_rotation(vector3d,num,num)");
    CREATE_SLF(entity, apply_damage__num, "apply_damage(num)");
    CREATE_SLF(entity, apply_directed_damage__num__vector3d, "apply_directed_damage(num,vector3d)");
    CREATE_SLF(entity, apply_directed_damage_cat__num__vector3d__str__num, "apply_directed_damage_cat(num,vector3d,str,num)");
    CREATE_SLF(entity, apply_explosive_damage__num__vector3d, "apply_explosive_damage(num,vector3d)");
    if constexpr (!xbpack::v10) {
        CREATE_SLF(entity, apply_explosive_damage__num__vector3d__vector3d, "apply_explosive_damage(num,vector3d,vector3d)");
    }
    CREATE_SLF(entity, apply_subdue__num, "apply_subdue(num)");
    CREATE_SLF(entity, camera_get_target, "camera_get_target()");
    CREATE_SLF(entity, camera_orbit__vector3d__num__num__num, "camera_orbit(vector3d,num,num,num)");
    CREATE_SLF(entity, camera_set_collide_with_world__num, "camera_set_collide_with_world(num)");
    CREATE_SLF(entity, camera_set_roll__num, "camera_set_roll(num)");
    CREATE_SLF(entity, camera_set_target__vector3d, "camera_set_target(vector3d)");
    CREATE_SLF(entity, camera_slide_to__vector3d__vector3d__num__num, "camera_slide_to(vector3d,vector3d,num,num)");
    CREATE_SLF(entity, camera_slide_to_orbit__vector3d__num__num__num__num, "camera_slide_to_orbit(vector3d,num,num,num,num)");
    CREATE_SLF(entity, cancel_tether, "cancel_tether()");
    CREATE_SLF(entity, car_random_body_and_color, "car_random_body_and_color()");
    CREATE_SLF(entity, change_ai_base_machine__str, "change_ai_base_machine(str)");
    CREATE_SLF(entity, collisions_enabled, "collisions_enabled()");
    CREATE_SLF(entity, compute_sector, "compute_sector()");
    CREATE_SLF(entity, create_damage_interface, "create_damage_interface()");
    CREATE_SLF(entity, create_interactable_ifc, "create_interactable_ifc()");
    CREATE_SLF(entity, create_physical_interface, "create_physical_interface()");
    CREATE_SLF(entity, create_script_data_interface, "create_script_data_interface()");
    CREATE_SLF(entity, create_web_ifc__str__num__num, "create_web_ifc(str,num,num)");
    CREATE_SLF(entity, disable_as_target, "disable_as_target()");
    CREATE_SLF(entity, disable_collisions, "disable_collisions()");
    CREATE_SLF(entity, disable_fading, "disable_fading()");
    CREATE_SLF(entity, disgorge_items, "disgorge_items()");
    CREATE_SLF(entity, enable_as_target, "enable_as_target()");
    CREATE_SLF(entity, enable_collisions, "enable_collisions()");
    CREATE_SLF(entity, enable_collisions__num, "enable_collisions(num)");
    CREATE_SLF(entity, eye_check_ent__entity, "eye_check_ent(entity)");
    CREATE_SLF(entity, eye_check_pos__vector3d, "eye_check_pos(vector3d)");
    CREATE_SLF(entity, force_activate_interaction__string_hash__interactable_interface, "force_activate_interaction(string_hash,interactable_interface)");
    CREATE_SLF(entity, force_current_region, "force_current_region()");
    CREATE_SLF(entity, force_region__entity, "force_region(entity)");
    CREATE_SLF(entity, get_abs_position, "get_abs_position()");
    CREATE_SLF(entity, get_ai_base_machine_name, "get_ai_base_machine_name()");
    CREATE_SLF(entity, get_ai_param_float__str, "get_ai_param_float(str)");
    CREATE_SLF(entity, get_ai_param_hash__str, "get_ai_param_hash(str)");
    CREATE_SLF(entity, get_ai_param_int__str, "get_ai_param_int(str)");
    CREATE_SLF(entity, get_ai_param_str__str, "get_ai_param_str(str)");
    CREATE_SLF(entity, get_ai_param_vector3d__str, "get_ai_param_vector3d(str)");
    CREATE_SLF(entity, get_ai_signaller, "get_ai_signaller()");
    CREATE_SLF(entity, get_anchor_point, "get_anchor_point()");
    CREATE_SLF(entity, get_carry_slave, "get_carry_slave()");
    CREATE_SLF(entity, get_current_animation_name, "get_current_animation_name()");
    CREATE_SLF(entity, get_damage_force, "get_damage_force()");
    CREATE_SLF(entity, get_debug_name, "get_debug_name()");
    if constexpr (!xbpack::v10) {
        CREATE_SLF(entity, get_detonate_position, "get_detonate_position()");
    }
    CREATE_SLF(entity, get_district, "get_district()");
    CREATE_SLF(entity, get_facing, "get_facing()");
    CREATE_SLF(entity, get_fade_timer, "get_fade_timer()");
    CREATE_SLF(entity, get_first_child, "get_first_child()");
    if constexpr (!xbpack::v10) {
        CREATE_SLF(entity, get_hash_name, "get_hash_name()");
    }
    CREATE_SLF(entity, get_hidey_pos__vector3d__num, "get_hidey_pos(vector3d,num)");
    CREATE_SLF(entity, get_ifc_num__str, "get_ifc_num(str)");
    CREATE_SLF(entity, get_ifc_str__str, "get_ifc_str(str)");
    CREATE_SLF(entity, get_ifc_vec__str, "get_ifc_vec(str)");
    CREATE_SLF(entity, get_ifl_frame, "get_ifl_frame()");
    CREATE_SLF(entity, get_inode_param_float__str__str, "get_inode_param_float(str,str)");
    CREATE_SLF(entity, get_inode_param_hash__str__str, "get_inode_param_hash(str,str)");
    CREATE_SLF(entity, get_inode_param_int__str__str, "get_inode_param_int(str,str)");
    CREATE_SLF(entity, get_inode_param_str__str__str, "get_inode_param_str(str,str)");
    CREATE_SLF(entity, get_inode_param_vector3d__str__str, "get_inode_param_vector3d(str,str)");
    CREATE_SLF(entity, get_interactable_ifc, "get_interactable_ifc()");
    CREATE_SLF(entity, get_item__num, "get_item(num)");
    CREATE_SLF(entity, get_item_by_name__str, "get_item_by_name(str)");
    CREATE_SLF(entity, get_item_quantity__num, "get_item_quantity(num)");
    CREATE_SLF(entity, get_last_anchor, "get_last_anchor()");
    CREATE_SLF(entity, get_last_attacker, "get_last_attacker()");
    CREATE_SLF(entity, get_last_item_used, "get_last_item_used()");
    CREATE_SLF(entity, get_member__str, "get_member(str)");
    if constexpr (xbpack::v10) {
        CREATE_SLF(entity, get_debug_name, "get_name()");
    }
    CREATE_SLF(entity, get_next_sibling, "get_next_sibling()");
    CREATE_SLF(entity, get_num_items, "get_num_items()");
    CREATE_SLF(entity, get_parent, "get_parent()");
    CREATE_SLF(entity, get_pendulum_length, "get_pendulum_length()");
    CREATE_SLF(entity, get_rel_position, "get_rel_position()");
    CREATE_SLF(entity, get_rel_velocity__entity, "get_rel_velocity(entity)");
    CREATE_SLF(entity, get_render_alpha, "get_render_alpha()");
    CREATE_SLF(entity, get_render_color, "get_render_color()");
    CREATE_SLF(entity, get_scripted_target, "get_scripted_target()");
    CREATE_SLF(entity, get_sector_name, "get_sector_name()");
    CREATE_SLF(entity, get_state__num, "get_state(num)");
    CREATE_SLF(entity, get_time_dilation, "get_time_dilation()");
    CREATE_SLF(entity, get_time_mode, "get_time_mode()");
    CREATE_SLF(entity, get_x_facing, "get_x_facing()");
    CREATE_SLF(entity, get_y_facing, "get_y_facing()");
    CREATE_SLF(entity, get_z_facing, "get_z_facing()");
    CREATE_SLF(entity, has_carry_slave, "has_carry_slave()");
    CREATE_SLF(entity, has_member__str, "has_member(str)");
    CREATE_SLF(entity, has_script_data_interface, "has_script_data_interface()");
    CREATE_SLF(entity, hates__entity, "hates(entity)");
    CREATE_SLF(entity, ifl_damage_lock__num, "ifl_damage_lock(num)");
    CREATE_SLF(entity, ifl_lock__num, "ifl_lock(num)");
    CREATE_SLF(entity, ifl_pause, "ifl_pause()");
    CREATE_SLF(entity, ifl_play, "ifl_play()");
    CREATE_SLF(entity, in_sector__vector3d__vector3d__num, "in_sector(vector3d,vector3d,num)");
    CREATE_SLF(entity, inhibit_universal_soldier_ability__str__num, "inhibit_universal_soldier_ability(str,num)");
    CREATE_SLF(entity, invoke_facial_expression__num__num__num__num, "invoke_facial_expression(num,num,num,num)");
    CREATE_SLF(entity, is_a_car, "is_a_car()");
    CREATE_SLF(entity, is_picked_up, "is_picked_up()");
    CREATE_SLF(entity, is_suspended, "is_suspended()");
    CREATE_SLF(entity, is_throwable, "is_throwable()");
    CREATE_SLF(entity, is_valid, "is_valid()");
    CREATE_SLF(entity, is_visible, "is_visible()");
    CREATE_SLF(entity, kill_anim_in_slot__num, "kill_anim_in_slot(num)");
    CREATE_SLF(entity, likes__entity, "likes(entity)");
    CREATE_SLF(entity, look_at__vector3d, "look_at(vector3d)");
    CREATE_SLF(entity, motion_blur_off, "motion_blur_off()");
    CREATE_SLF(entity, motion_blur_on__num__num__num, "motion_blur_on(num,num,num)");
    CREATE_SLF(entity, motion_trail_off, "motion_trail_off()");
    CREATE_SLF(entity, motion_trail_on__entity__entity__vector3d__num__num__num__num, "motion_trail_on(entity,entity,vector3d,num,num,num,num)");
    CREATE_SLF(entity, motion_trail_on__entity__str__num__num__vector3d__num__num__num__num, "motion_trail_on(entity,str,num,num,vector3d,num,num,num,num)");
    CREATE_SLF(entity, neutral__entity, "neutral(entity)");
    CREATE_SLF(entity, operator_not_equals__entity, "operator!=(entity)");
    CREATE_SLF(entity, operator_equals_equals__entity, "operator==(entity)");
    CREATE_SLF(entity, physical_ifc_add_particle__str, "physical_ifc_add_particle(str)");
    CREATE_SLF(entity, physical_ifc_apply_force__vector3d__num, "physical_ifc_apply_force(vector3d,num)");
    CREATE_SLF(entity, physical_ifc_cancel_all_velocity, "physical_ifc_cancel_all_velocity()");
    CREATE_SLF(entity, physical_ifc_clear_pendulum, "physical_ifc_clear_pendulum()");
    CREATE_SLF(entity, physical_ifc_get_attached_particle_name, "physical_ifc_get_attached_particle_name()");
    CREATE_SLF(entity, physical_ifc_get_bounce_particle_name, "physical_ifc_get_bounce_particle_name()");
    CREATE_SLF(entity, physical_ifc_is_biped_physics_running, "physical_ifc_is_biped_physics_running()");
    CREATE_SLF(entity, physical_ifc_is_effectively_standing, "physical_ifc_is_effectively_standing()");
    CREATE_SLF(entity, physical_ifc_is_prop_physics_at_rest, "physical_ifc_is_prop_physics_at_rest()");
    CREATE_SLF(entity, physical_ifc_is_prop_physics_running, "physical_ifc_is_prop_physics_running()");
    CREATE_SLF(entity, physical_ifc_manage_standing__num, "physical_ifc_manage_standing(num)");
    CREATE_SLF(entity, physical_ifc_set_allow_biped_physics__num, "physical_ifc_set_allow_biped_physics(num)");
    CREATE_SLF(entity, physical_ifc_set_attached_particle_name__str, "physical_ifc_set_attached_particle_name(str)");
    CREATE_SLF(entity, physical_ifc_set_bounce_particle_name__str, "physical_ifc_set_bounce_particle_name(str)");
    CREATE_SLF(entity, physical_ifc_set_pendulum__entity__num, "physical_ifc_set_pendulum(entity,num)");
    CREATE_SLF(entity, physical_ifc_set_pendulum__vector3d__num, "physical_ifc_set_pendulum(vector3d,num)");
    CREATE_SLF(entity, physical_ifc_start_biped_physics, "physical_ifc_start_biped_physics()");
    CREATE_SLF(entity, physical_ifc_start_prop_physics__vector3d__num, "physical_ifc_start_prop_physics(vector3d,num)");
    CREATE_SLF(entity, physical_ifc_stop_biped_physics, "physical_ifc_stop_biped_physics()");
    CREATE_SLF(entity, physical_ifc_stop_prop_physics, "physical_ifc_stop_prop_physics()");
    CREATE_SLF(entity, play_anim__str, "play_anim(str)");
    CREATE_SLF(entity, play_anim__str__num__num, "play_anim(str,num,num)");
    CREATE_SLF(entity, poison__num__num, "poison(num,num)");
    CREATE_SLF(entity, pop_ai_base_machine, "pop_ai_base_machine()");
    CREATE_SLF(entity, pre_roll__num, "pre_roll(num)");
    CREATE_SLF(entity, push_ai_base_machine__str, "push_ai_base_machine(str)");
    CREATE_SLF(entity, randomize_position__vector3d__num__num__num, "randomize_position(vector3d,num,num,num)");
    CREATE_SLF(entity, regenerate__num__num, "regenerate(num,num)");
    // Original Xbox V10 slot 175 is rel_angle. These registrations fill
    // positional SLC slots; omitting it shifts remove_selectable_target
    // from 178 to 177 and makes the V08 sign callback consume only one of
    // its two entity handles, eventually hiding the hero.
    CREATE_SLF(entity, rel_angle__vector3d, "rel_angle(vector3d)");
    CREATE_SLF(entity, remove_collision_ignorance__entity, "remove_collision_ignorance(entity)");
    CREATE_SLF(entity, remove_exclusive_interactor__string_hash__interactable_interface, "remove_exclusive_interactor(string_hash,interactable_interface)");
    CREATE_SLF(entity, remove_selectable_target__entity, "remove_selectable_target(entity)");
    CREATE_SLF(entity, remove_vehicle_from_traffic_system, "remove_vehicle_from_traffic_system()");
    if constexpr (!xbpack::v10) {
        // V10 goes directly from remove_vehicle (179) to restart (180).
        CREATE_SLF(entity, render_name__num, "render_name(num)");
        CREATE_SLF(entity, reset_ai, "reset_ai()");
    }
    CREATE_SLF(entity, restart, "restart()");
    CREATE_SLF(entity, seriously_kill, "seriously_kill()");
    CREATE_SLF(entity, set_abs_xz_facing__vector3d, "set_abs_xz_facing(vector3d)");
    CREATE_SLF(entity, set_active__num, "set_active(num)");
    CREATE_SLF(entity, set_ai_param_float__str__num, "set_ai_param_float(str,num)");
    CREATE_SLF(entity, set_ai_param_float_variance__str__num__num, "set_ai_param_float_variance(str,num,num)");
    CREATE_SLF(entity, set_ai_param_hash__str__num, "set_ai_param_hash(str,num)");
    CREATE_SLF(entity, set_ai_param_hash__str__str, "set_ai_param_hash(str,str)");
    if constexpr (!xbpack::v10) {
        CREATE_SLF(entity, set_ai_param_hash__str__string_hash, "set_ai_param_hash(str,string_hash)");
    }
    CREATE_SLF(entity, set_ai_param_int__str__num, "set_ai_param_int(str,num)");
    CREATE_SLF(entity, set_ai_param_str__str__str, "set_ai_param_str(str,str)");
    CREATE_SLF(entity, set_ai_param_vector3d__str__vector3d, "set_ai_param_vector3d(str,vector3d)");
    CREATE_SLF(entity, set_ambient_factor__vector3d, "set_ambient_factor(vector3d)");
    CREATE_SLF(entity, set_anchor_activated__num, "set_anchor_activated(num)");
    CREATE_SLF(entity, set_car_combat_info__num__num__num__num__num__num__num__num, "set_car_combat_info(num,num,num,num,num,num,num,num)");
    CREATE_SLF(entity, set_crawlable__num, "set_crawlable(num)");
    if constexpr (!xbpack::v10) {
        CREATE_SLF(entity, set_default_variant, "set_default_variant()");
    }
    CREATE_SLF(entity, set_distance_clip__num, "set_distance_clip(num)");
    CREATE_SLF(entity, set_entity_blur__num, "set_entity_blur(num)");
    CREATE_SLF(entity, set_facing__vector3d__vector3d, "set_facing(vector3d,vector3d)");
    CREATE_SLF(entity, set_fade_timer__num, "set_fade_timer(num)");
    CREATE_SLF(entity, set_hires_shadow__num, "set_hires_shadow(num)");
    CREATE_SLF(entity, set_ifc_num__str__num, "set_ifc_num(str,num)");
    CREATE_SLF(entity, set_ifc_str__str__str, "set_ifc_str(str,str)");
    CREATE_SLF(entity, set_ifc_vec__str__vector3d, "set_ifc_vec(str,vector3d)");
    CREATE_SLF(entity, set_ignore_limbo__num, "set_ignore_limbo(num)");
    CREATE_SLF(entity, set_immobile__num, "set_immobile(num)");
    CREATE_SLF(entity, set_inode_param_entity__str__str__entity, "set_inode_param_entity(str,str,entity)");
    CREATE_SLF(entity, set_inode_param_float__str__str__num, "set_inode_param_float(str,str,num)");
    CREATE_SLF(entity, set_inode_param_float_variance__str__str__num__num, "set_inode_param_float_variance(str,str,num,num)");
    CREATE_SLF(entity, set_inode_param_hash__str__str__num, "set_inode_param_hash(str,str,num)");
    CREATE_SLF(entity, set_inode_param_hash__str__str__str, "set_inode_param_hash(str,str,str)");
    CREATE_SLF(entity, set_inode_param_int__str__str__num, "set_inode_param_int(str,str,num)");
    CREATE_SLF(entity, set_inode_param_str__str__str__str, "set_inode_param_str(str,str,str)");
    CREATE_SLF(entity, set_inode_param_vector3d__str__str__vector3d, "set_inode_param_vector3d(str,str,vector3d)");
    CREATE_SLF(entity, set_invulnerable__num, "set_invulnerable(num)");
    CREATE_SLF(entity, set_kill_ent_on_destroy__num, "set_kill_ent_on_destroy(num)");
    CREATE_SLF(entity, set_member_hidden__num, "set_member_hidden(num)");
    CREATE_SLF(entity, set_parent__entity, "set_parent(entity)");
    CREATE_SLF(entity, set_parent_rel__entity, "set_parent_rel(entity)");
    CREATE_SLF(entity, set_path_graph__str, "set_path_graph(str)");
    CREATE_SLF(entity, set_path_graph_start_node__num, "set_path_graph_start_node(num)");
    CREATE_SLF(entity, set_pendulum_attach_limb__num, "set_pendulum_attach_limb(num)");
    CREATE_SLF(entity, set_pendulum_length__num, "set_pendulum_length(num)");
    CREATE_SLF(entity, set_physical__num, "set_physical(num)");
    CREATE_SLF(entity, set_po_facing__vector3d, "set_po_facing(vector3d)");
    CREATE_SLF(entity, set_rel_position__vector3d, "set_rel_position(vector3d)");
    CREATE_SLF(entity, set_render_alpha__num, "set_render_alpha(num)");
    CREATE_SLF(entity, set_render_color__vector3d, "set_render_color(vector3d)");
    CREATE_SLF(entity, set_render_scale__vector3d, "set_render_scale(vector3d)");
    CREATE_SLF(entity, set_scale__num, "set_scale(num)");
    CREATE_SLF(entity, set_scripted_target__entity, "set_scripted_target(entity)");
    CREATE_SLF(entity, set_see_thru__num, "set_see_thru(num)");
    CREATE_SLF(entity, set_special_target__num, "set_special_target(num)");
    CREATE_SLF(entity, set_state__num__num, "set_state(num,num)");
    CREATE_SLF(entity, set_targetting__num, "set_targetting(num)");
    if constexpr (!xbpack::v10) {
        CREATE_SLF(entity, set_throwable__num, "set_throwable(num)");
    }
    CREATE_SLF(entity, set_time_dilation__num, "set_time_dilation(num)");
    CREATE_SLF(entity, set_time_mode__num, "set_time_mode(num)");
    CREATE_SLF(entity, set_variant__string_hash, "set_variant(string_hash)");
    CREATE_SLF(entity, set_velocity__vector3d, "set_velocity(vector3d)");
    CREATE_SLF(entity, set_visible__num, "set_visible(num)");
    CREATE_SLF(entity, set_visible_and_disable_fading__num, "set_visible_and_disable_fading(num)");
    CREATE_SLF(entity, set_xz_facing__vector3d, "set_xz_facing(vector3d)");
    CREATE_SLF(entity, setup_tether__vector3d__num, "setup_tether(vector3d,num)");
    CREATE_SLF(entity, snap_to__entity, "snap_to(entity)");
    CREATE_SLF(entity, suspend, "suspend()");
    CREATE_SLF(entity, teleport_to_point__vector3d, "teleport_to_point(vector3d)");
    CREATE_SLF(entity, unforce_regions, "unforce_regions()");
    CREATE_SLF(entity, unsuspend, "unsuspend()");
    CREATE_SLF(entity, use_item__str, "use_item(str)");
    CREATE_SLF(entity, use_item_by_id__str, "use_item_by_id(str)");
    CREATE_SLF(entity, wait_camera_set_roll__num__num, "wait_camera_set_roll(num,num)");
    CREATE_SLF(entity, wait_change_color__vector3d__vector3d__num, "wait_change_color(vector3d,vector3d,num)");
    CREATE_SLF(entity, wait_change_range__num__num__num, "wait_change_range(num,num,num)");
    CREATE_SLF(entity, wait_change_render_color__vector3d__num__num, "wait_change_render_color(vector3d,num,num)");
    CREATE_SLF(entity, wait_change_render_scale__vector3d__num, "wait_change_render_scale(vector3d,num)");
    CREATE_SLF(entity, wait_for_not_sector__str, "wait_for_not_sector(str)");
    CREATE_SLF(entity, wait_for_pickup, "wait_for_pickup()");
    CREATE_SLF(entity, wait_for_sector__str, "wait_for_sector(str)");
    CREATE_SLF(entity, wait_lookat__vector3d__num, "wait_lookat(vector3d,num)");
    CREATE_SLF(entity, wait_lookat2__entity__entity__vector3d__num, "wait_lookat2(entity,entity,vector3d,num)");
    CREATE_SLF(entity, wait_looping_anim__str__num__num, "wait_looping_anim(str,num,num)");
    CREATE_SLF(entity, wait_play_anim__str__num__num__num, "wait_play_anim(str,num,num,num)");
    CREATE_SLF(entity, wait_prox__entity__num, "wait_prox(entity,num)");
    CREATE_SLF(entity, wait_prox__vector3d__num, "wait_prox(vector3d,num)");
    CREATE_SLF(entity, wait_prox__vector3d__num__vector3d__num, "wait_prox(vector3d,num,vector3d,num)");
    CREATE_SLF(entity, wait_prox_maxY__vector3d__num__num, "wait_prox_maxY(vector3d,num,num)");
    CREATE_SLF(entity, wait_prox_minY__vector3d__num__num, "wait_prox_minY(vector3d,num,num)");
    CREATE_SLF(entity, wait_prox_sector__vector3d__num__str, "wait_prox_sector(vector3d,num,str)");
    CREATE_SLF(entity, wait_rotate__vector3d__num__num, "wait_rotate(vector3d,num,num)");
    CREATE_SLF(entity, wait_rotate_WCS__vector3d__vector3d__num__num, "wait_rotate_WCS(vector3d,vector3d,num,num)");
    CREATE_SLF(entity, wait_rotate_WCS_cosmetic__vector3d__vector3d__num__num, "wait_rotate_WCS_cosmetic(vector3d,vector3d,num,num)");
    CREATE_SLF(entity, wait_rotate_WCS_with_compute_sector__vector3d__vector3d__num__num, "wait_rotate_WCS_with_compute_sector(vector3d,vector3d,num,num)");
    CREATE_SLF(entity, wait_rotate_cosmetic__vector3d__num__num, "wait_rotate_cosmetic(vector3d,num,num)");
    CREATE_SLF(entity, wait_set_scale__num__num, "wait_set_scale(num,num)");
    CREATE_SLF(entity, wait_translate__vector3d__num, "wait_translate(vector3d,num)");
    CREATE_SLF(entity, wait_translate_WCS__vector3d__num, "wait_translate_WCS(vector3d,num)");
    CREATE_SLF(entity, wait_translate_WCS_cosmetic__vector3d__num, "wait_translate_WCS_cosmetic(vector3d,num)");
    CREATE_SLF(entity, wait_translate_WCS_with_compute_sector__vector3d__num, "wait_translate_WCS_with_compute_sector(vector3d,num)");
    CREATE_SLF(entity, wait_translate_cosmetic__vector3d__num, "wait_translate_cosmetic(vector3d,num)");
    CREATE_SLF(entity, wait_translate_with_compute_sector__vector3d__num, "wait_translate_with_compute_sector(vector3d,num)");
    CREATE_SLF(entity, was_occluded_last_frame, "was_occluded_last_frame()");

#undef CREATE_SLF
#undef BUILD_SLF_NAME
}

void script_lib_entity_patch() {
    {
        FUNC_ADDRESS(address, &slf__entity__add_item__entity__t::operator());
        set_vfunc(0x0089AF48, address);
    }
}
