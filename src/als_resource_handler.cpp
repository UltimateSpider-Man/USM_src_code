#include "als_resource_handler.h"

#include "common.h"
#include "func_wrapper.h"
#include "trace.h"
#include "utility.h"
#include "resource_directory.h"
#include "worldly_pack_slot.h"
#include "mash_info_struct.h"
#include "als_animation_logic_system_shared.h"
#include "als_category.h"
#include "als_meta_anim_base.h"
#include "als_meta_anim_swing.h"
#include "als_scripted_category.h"
#include "als_scripted_state.h"
#include "anim_record.h"
#include "layer_state_machine_shared.h"
#include "mash_config.h"
#include "meta_anim_interact.h"
#include "state_machine_shared.h"
#include "string_hash_dictionary.h"
#include "scripted_trans_group.h"
#include "variables.h"
#include "vtbl.h"

VALIDATE_SIZE(als_resource_handler, 0x14);

als_resource_handler::als_resource_handler(worldly_pack_slot *a2)
{
    this->m_vtbl = 0x008889F8;
    this->my_slot = a2;
    this->field_10 = RESOURCE_KEY_TYPE_ALS_FILE;
}

bool als_resource_handler::_handle(worldly_resource_handler::eBehavior a2,
                                          limited_timer *a3)
{
    TRACE("als_resource_handler::handle");

    return base_engine_resource_handler::_handle(a2, a3);
}

bool als_resource_handler::_handle_resource(worldly_resource_handler::eBehavior a2,
                                           resource_location *a3)
{
    TRACE("als_resource_handler::handle_resource", a3->field_0.get_platform_string(g_platform).c_str());

    if constexpr (1)
    {
        auto &v3 = this->my_slot->get_resource_directory();
        auto *resource = v3.get_resource(a3, nullptr);
        assert(resource != nullptr);

        auto *new_als = bit_cast<als::animation_logic_system_shared *>(resource);
        if ( a2 == UNLOAD )
        {
            new_als->destruct_mashed_class();
        }
        else
        {
            assert(new_als != nullptr);

#if OPENUSM_XBOX_MASH_FORMAT
            mash_info_struct v5 {mash::UNMASH_MODE, resource, a3->m_size, true};
#else
            mash_info_struct v5 {resource, a3->m_size};
#endif

            v5.unmash_class(new_als, nullptr
#if OPENUSM_XBOX_MASH_FORMAT
                , mash::NORMAL_BUFFER
#endif 
                    );

            mash_info_struct::construct_class(new_als);

#if OPENUSM_XBOX_MASH_FORMAT
            a3->m_offset += v5.get_header_size();
#endif
        }

        ++this->field_C;
        return false;
    }
    else
    {
        return (bool) THISCALL(0x00568930, this, a2, a3);
    }
}

void als_resource_handler_patch()
{
    {
        FUNC_ADDRESS(address, &als_resource_handler::_handle_resource);
        set_vfunc(0x00888A04, address);
    }

    {
        FUNC_ADDRESS(address, &als_resource_handler::_handle);
        set_vfunc(0x008889FC, address);
    }
}

void als_resource_handler_xbpack_patch()
{
    FUNC_ADDRESS(handler, &als_resource_handler::_handle_resource);
    set_vfunc(0x00888A04, handler);

    FUNC_ADDRESS(state_machine_unmash, &als::state_machine_shared::_unmash);
    set_vfunc(0x0087B8FC, state_machine_unmash);

    FUNC_ADDRESS(layer_machine_unmash, &als::layer_state_machine_shared::_unmash);
    set_vfunc(0x0087E3A8, layer_machine_unmash);

    FUNC_ADDRESS(category_unmash, &als::category::_unmash);
    set_vfunc(0x00875708, category_unmash);

    FUNC_ADDRESS(scripted_category_unmash, &als::scripted_category::_unmash);
    set_vfunc(0x0087E254, scripted_category_unmash);

    FUNC_ADDRESS(scripted_trans_group_unmash, &als::scripted_trans_group::_unmash);
    set_vfunc(0x0087E1BC, scripted_trans_group_unmash);

    FUNC_ADDRESS(scripted_state_unmash, &als::scripted_state::_unmash);
    set_vfunc(0x0087E1DC, scripted_state_unmash);

    FUNC_ADDRESS(base_layer_state_unmash, &als::base_layer_scripted_state::_unmash);
    set_vfunc(0x0087E218, base_layer_state_unmash);

    FUNC_ADDRESS(meta_anim_base_unmash, &als::als_meta_anim_base::_unmash);
    set_vfunc(0x0087545C, meta_anim_base_unmash);

    FUNC_ADDRESS(meta_anim_swing_unmash, &als::als_meta_anim_swing::_unmash);
    set_vfunc(0x0087B91C, meta_anim_swing_unmash);

    FUNC_ADDRESS(meta_anim_interact_unmash, &ai::meta_anim_interact::_unmash);
    set_vfunc(0x00875564, meta_anim_interact_unmash);

    FUNC_ADDRESS(meta_anim_strength_unmash, &ai::meta_anim_strength_test::_unmash);
    set_vfunc(0x008755A0, meta_anim_strength_unmash);

    FUNC_ADDRESS(meta_anim_blend_unmash, &als::als_meta_linear_blend::_unmash);
    set_vfunc(0x0087B958, meta_anim_blend_unmash);

    FUNC_ADDRESS(anim_record_unmash, &anim_record::_unmash);
    set_vfunc(0x0087392C, anim_record_unmash);

    FUNC_ADDRESS(attach_anim_record_unmash, &attach_anim_record::_unmash);
    set_vfunc(0x008739B0, attach_anim_record_unmash);

    // The prerelease startup path installs this XBPACK patch set instead of
    // the normal anim_record_patch() sequence.
    anim_record_v10_patch();
}
