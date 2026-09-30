#include "base_ai_res_state_graph.h"

#include "binary_search_array_deref.h"
#include "common.h"
#include "func_wrapper.h"
#include "mashed_state.h"
#include "trace.h"
#include "utility.h"
#include "xbpack.h"

namespace ai {

VALIDATE_SIZE(state_graph, 0x34);

state_graph::state_graph()
{

}

void state_graph::destruct_mashed_class()
{
    THISCALL(0x006DA0D0, this);
}

void state_graph::unmash(mash_info_struct *a1, void *)
{
    TRACE("ai::state_graph::unmash");

    a1->unmash_class_in_place(this->field_0, this);

#ifdef OPENUSM_XBPACK_MODE
    if (this->field_0.m_type != RESOURCE_KEY_TYPE_AI_STATE_GRAPH) {
        this->field_0.m_type = static_cast<resource_key_type>(
            xbpack::pc_type(static_cast<int>(this->field_0.m_type)));
    }
    assert(this->field_0.m_type == RESOURCE_KEY_TYPE_AI_STATE_GRAPH);
#endif

    a1->unmash_class_in_place(this->my_states, this);
    a1->unmash_class_in_place(this->field_20, this);

#if OPENUSM_XBOX_MASH_FORMAT && !defined(OPENUSM_XBPACK_V10)
    {
        uint8_t class_mashed = -1;
        class_mashed = *a1->read_from_buffer(mash::SHARED_BUFFER, 1, 1);
        assert(class_mashed == 0xAF || class_mashed == 0);
    }
#endif

    if ( this->field_1C != nullptr )
    {
        a1->unmash_class(this->field_1C, this
#if OPENUSM_XBOX_MASH_FORMAT
            , mash::NORMAL_BUFFER
#endif
                );

#ifdef OPENUSM_XBPACK_V10
        bool converted_with_states = false;
        for (auto *state : this->my_states) {
            if (state == this->field_1C) {
                converted_with_states = true;
                break;
            }
        }

        if (!converted_with_states) {
            auto &type = this->field_1C->field_14;
            type = static_cast<mash::virtual_types_enum>(
                xbpack::pc_state_type(static_cast<uint32_t>(type)));
        }
#endif
    }
}

void sub_86B3C0() {
    CDECL_CALL(0x0086B3C0);
}

unsigned int state_graph::get_size_memory_block() const
{
    int size = this->my_states.size();
    auto result = 0;
    for ( int i = 0; i < size; ++i )
    {
        if (result < 24) {
            result = 24;
        }
    }

    return result;
}

mashed_state *state_graph::find_state(string_hash a2) const
{
    TRACE("ai::state_graph::find_state", a2.to_string());

    if constexpr (1)
    {
        static mashed_state searcher{};

        searcher.field_C = a2;
        auto **data = this->my_states.m_data;
        auto size = this->my_states.m_size;
        int index = -1;

        mashed_state *result = nullptr;
        if (binary_search_array_deref(&searcher, data, size, &index)) {
            assert(index >= 0);

            result = data[index];
        }

        return result;

    } else {
        return (mashed_state *) THISCALL(0x006D8480, this, a2);
    }
}
} // namespace ai

void state_graph_patch() {
    {
        FUNC_ADDRESS(address, &ai::state_graph::unmash);
        SET_JUMP(0x006DA070, address);
    }

    {
        FUNC_ADDRESS(address, &ai::state_graph::find_state);
        SET_JUMP(0x006D8480, address);
    }
}
