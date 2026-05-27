#include "moved_entities.h"

#include "conglom.h"
#include "func_wrapper.h"
#include "oldmath_po.h"
#include "osassert.h"
#include "trace.h"
#include "utility.h"

#include <cassert>

void moved_entities::reset_all_moved() {
    TRACE("moved_entities::reset_all_moved");

    CDECL_CALL(0x005125D0);
}

void moved_entities::add_moved(vhandle_type<entity> e_arg) {
    TRACE("moved_entities::add_moved");

    //assert(e_arg != INVALID_VHANDLE);

    auto *e = e_arg.get_volatile_ptr();
    assert(e != nullptr);

    assert(e->get_abs_po().is_valid());

    if (e->is_conglom_member()) {
        e = (entity *) e->get_conglom_owner();
        assert(e != nullptr && "Failed to obtain conglom owner in add_moved");
    }

    if (e == nullptr || e->is_flagged_in_the_moved_list()) {
        return;
    }

    e->set_ext_flag_recursive_internal(static_cast<entity_ext_flag_t>(0x40), true);
    e->set_ext_flag_recursive_internal(static_cast<entity_ext_flag_t>(0x800000), true);

    if (moved_count >= 600) {
        if (moved_count == 600) {
            for (auto i = 0; i < moved_count; ++i) {
                auto v20 = moved_list[i];
                if (auto *v2 = v20.get_volatile_ptr()) {
                    auto *v4 = v2->get_id().to_string();
                    sp_log("Entity at slot %d: %s 0x%x\n", i, v4, v2);
                } else {
                    sp_log("Destroyed entity at slot %d\n", i);
                }
            }

            error("Too many moved_entities this frame.");
        }
    } else {
        moved_list[moved_count++].field_0 = e->my_handle.field_0;

        assert("Cloned conglomerates should not be added to the moved list!"
            && !(e->is_a_conglomerate() && ((conglomerate *) e)->is_cloned_conglomerate()));
    }

    if (e->empty_adopted_children()) {
        auto *adopted_children = e->get_adopted_children();
        assert(adopted_children != nullptr);

        for (auto &child : (*adopted_children)) {
            if (child->is_an_actor() || child->is_a_pfx_entity()) {
                moved_entities::add_moved(vhandle_type<entity>{child->my_handle});
            }
        }
    }
}


intraframe_trajectory_t *moved_entities::get_all_trajectories(
    Float a1, const moved_entities::trajectory_filter_t &filter)
{
    TRACE("moved_entities::get_all_trajectories");

    if constexpr (0)
    {
    }
    else
    {
        intraframe_trajectory_t * (__cdecl *func)(Float, const trajectory_filter_t *) = CAST(func, 0x0053F2A0);
        
        return func(a1, &filter);
    }
}

void moved_entities_patch()
{
    SET_JUMP(0x00533D00, &moved_entities::add_moved);
    
}
