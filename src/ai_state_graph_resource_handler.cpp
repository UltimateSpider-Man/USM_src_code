#include "ai_state_graph_resource_handler.h"

#include "base_ai_res_state_graph.h"
#include "common.h"
#include "func_wrapper.h"
#include "mash_info_struct.h"
#include "utility.h"
#include "resource_directory.h"
#include "trace.h"
#include "variables.h"
#include "worldly_pack_slot.h"

VALIDATE_SIZE(ai_state_graph_resource_handler, 0x14);

ai_state_graph_resource_handler::ai_state_graph_resource_handler(worldly_pack_slot *a2)
{
    this->m_vtbl = 0x00888A18;
    this->my_slot = a2;
    this->field_10 = RESOURCE_KEY_TYPE_AI_STATE_GRAPH;
}

bool ai_state_graph_resource_handler::_handle(worldly_resource_handler::eBehavior a2, limited_timer *a3)
{
    TRACE("ai_state_graph_resource_handler::handle");

#if OPENUSM_XBOX_MASH_FORMAT
    this->field_10 = RESOURCE_KEY_TYPE_AI_STATE_GRAPH;
#endif

    return base_engine_resource_handler::_handle(a2, a3);
}

bool ai_state_graph_resource_handler::_handle_resource(worldly_resource_handler::eBehavior a2,
                                                      resource_location *a3)
{
    TRACE("ai_state_graph_resource_handler::handle_resource", a3->field_0.get_platform_string(g_platform).c_str());

    if constexpr (1)
    {
        auto &directory = this->my_slot->get_resource_directory();
        auto *resource = directory.get_resource(a3, nullptr);
        assert(resource != nullptr);

        if ( a2 == UNLOAD)
        {
            bit_cast<ai::state_graph *>(resource)->destruct_mashed_class();
        }
        else
        {
            ai::state_graph *new_state_graph = CAST(new_state_graph, resource);
            assert(new_state_graph != nullptr);

#if OPENUSM_XBOX_MASH_FORMAT
            mash_info_struct a1 {mash::UNMASH_MODE, resource, a3->m_size, true};
#else
            mash_info_struct a1 {resource, a3->m_size};
#endif

            a1.unmash_class(new_state_graph, nullptr
#if OPENUSM_XBOX_MASH_FORMAT
                , mash::NORMAL_BUFFER
#endif
                    );
            mash_info_struct::construct_class(new_state_graph);

#if OPENUSM_XBOX_MASH_FORMAT
            a3->m_offset += a1.get_header_size();
#endif
        }

        ++this->field_C;
        return false;
    }
    else
    {
        return (bool) THISCALL(0x00568AF0, this, a2, a3);
    }
}

void ai_state_graph_resource_handler_xbpack_patch()
{
#ifdef OPENUSM_XBPACK_MODE
    FUNC_ADDRESS(address, &ai_state_graph_resource_handler::_handle_resource);
    set_vfunc(0x00888A24, address);
#endif
}

void ai_state_graph_resource_handler_patch()
{
    {
        FUNC_ADDRESS(address, &ai_state_graph_resource_handler::_handle);
        set_vfunc(0x00888A1C, address);
    }
    
    {
        FUNC_ADDRESS(address, &ai_state_graph_resource_handler::_handle_resource);
        set_vfunc(0x00888A24, address);
    }
}
