#include "als_state.h"

#include "common.h"
#include "mash_info_struct.h"
#include "mash_config.h"
#include "param_block.h"
#include "vtbl.h"
#include "trace.h"
#include "utility.h"

namespace als {

    VALIDATE_SIZE(state, 0x14);

    state::state()
    {
    }

    void state::_unmash(mash_info_struct *a2, void *a3)
    {
        TRACE("als::state::unmash");

        if constexpr (1)
        {
            a2->unmash_class_in_place(this->m_state_id, a3);
            a2->unmash_class_in_place(this->m_cat_id, a3);

#if OPENUSM_XBOX_MASH_FORMAT && !defined(OPENUSM_XBPACK_V10)
            {
                uint8_t class_mashed = -1;
                class_mashed = *a2->read_from_buffer(mash::SHARED_BUFFER, 1, 1);
                assert(class_mashed == 0xAF || class_mashed == 0);
            }
#endif

            if ( this->field_10 != nullptr )
            {
                a2->unmash_class(this->field_10, a3
#if OPENUSM_XBOX_MASH_FORMAT
                        , mash::NORMAL_BUFFER
#endif
                        );
            }

        }
        else
        {
            THISCALL(0x0049F050, this, a2, a3);
        }
    }

    als::request_data state::do_implicit_trans(
        animation_logic_system *a4,
        state_machine *a5)
    {

        void (__fastcall *func)(void *, void *,
                request_data *,
                animation_logic_system *,
                state_machine *) = CAST(func, get_vfunc(m_vtbl, 0x24));
        request_data data;
        func(this, nullptr, &data, a4, a5);
        return data;
    }

    string_hash state::get_nal_anim_name() const {
        void (__fastcall *func)(const void *, void *, string_hash *) = CAST(func, get_vfunc(m_vtbl, 0x34));
        string_hash result;
        func(this, nullptr, &result);
        return result;
    }

    int state::get_mash_sizeof() const
    {
        if constexpr (1)
        {
            int (__fastcall *func)(const state *) = CAST(func, get_vfunc(m_vtbl, 0x38));
            return func(this);
        }

        return 0x14;
    }

} // namespace als

void als_state_patch()
{
    {
        FUNC_ADDRESS(address, &als::state::_unmash);
        //set_vfunc(0x008756CC, address);
    }

    {
        FUNC_ADDRESS(address, &als::state::get_mash_sizeof);
        //set_vfunc(0x, address);
    }
}
