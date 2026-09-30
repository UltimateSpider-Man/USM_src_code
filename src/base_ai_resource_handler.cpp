#include "base_ai_resource_handler.h"

#include "common.h"
#include "core_ai_resource.h"
#include "mash_info_struct.h"
#include "resource_directory.h"
#include "trace.h"
#include "utility.h"
#include "variables.h"
#include "worldly_pack_slot.h"

VALIDATE_SIZE(base_ai_resource_handler, 0x14);

base_ai_resource_handler::base_ai_resource_handler(worldly_pack_slot *a2)
{
    this->m_vtbl = 0x00888A08;
    this->my_slot = a2;
    this->field_10 = RESOURCE_KEY_TYPE_BASE_AI;
}

bool base_ai_resource_handler::_handle(worldly_resource_handler::eBehavior a2, limited_timer *a3)
{
    TRACE("base_ai_resource_handler::handle");

#if OPENUSM_XBOX_MASH_FORMAT
    this->field_10 = RESOURCE_KEY_TYPE_BASE_AI;
#endif

    return base_engine_resource_handler::_handle(a2, a3);
}

bool base_ai_resource_handler::_handle_resource(worldly_resource_handler::eBehavior behavior,
                                               resource_location *a3)
{
    TRACE("base_ai_resource_handler::handle_resource", a3->field_0.get_platform_string(g_platform).c_str());

    if constexpr (1)
    {
        auto &res_dir = this->my_slot->get_resource_directory();
        auto *resource = res_dir.get_resource(a3, nullptr);
        assert(resource != nullptr);

        if ( behavior == UNLOAD )
        {
            bit_cast<ai::core_ai_resource *>(resource)->destruct_mashed_class();
        }
        else
        {
            auto *new_ai_resource = bit_cast<ai::core_ai_resource *>(resource);
            assert(new_ai_resource != nullptr);

#if OPENUSM_XBOX_MASH_FORMAT
            mash_info_struct info_struct {mash::UNMASH_MODE, resource, a3->m_size, true};
#else
            mash_info_struct info_struct {resource, a3->m_size};
#endif

            info_struct.unmash_class(new_ai_resource, nullptr
#if OPENUSM_XBOX_MASH_FORMAT
                , mash::NORMAL_BUFFER
#endif 
                    );
            mash_info_struct::construct_class(new_ai_resource);

#if OPENUSM_XBOX_MASH_FORMAT
            a3->m_offset += info_struct.get_header_size();
#endif
        }

        ++this->field_C;
        return false;
    }
    else
    {
        bool (__fastcall *func)(void *, void *, worldly_resource_handler::eBehavior ,
                                               resource_location *) = CAST(func, 0x00568A10);
        return func(this, nullptr, behavior, a3); 
    }
}

void base_ai_resource_handler_xbpack_patch()
{
#ifdef OPENUSM_XBPACK_MODE
    FUNC_ADDRESS(address, &base_ai_resource_handler::_handle_resource);
    set_vfunc(0x00888A14, address);
#endif
}

void base_ai_resource_handler_patch()
{
    {
        FUNC_ADDRESS(address, &base_ai_resource_handler::_handle);
        set_vfunc(0x00888A0C, address);
    }

    {
        FUNC_ADDRESS(address, &base_ai_resource_handler::_handle_resource);
        set_vfunc(0x00888A14, address);
    }
}
