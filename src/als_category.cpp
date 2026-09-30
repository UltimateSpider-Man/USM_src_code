#include "als_category.h"

#include "common.h"

#include "func_wrapper.h"
#include "mash_info_struct.h"
#include "param_block.h"
#include "trace.h"
#include "mash_config.h"
#include "utility.h"
#include "vtbl.h"

namespace als
{
    VALIDATE_SIZE(category, 0x10);

    request_data category::do_implicit_trans(
            animation_logic_system *a3,
            state_machine *a4)
    {
        als::request_data data;
        void (__fastcall *func)(void *, void *, request_data *, animation_logic_system *, state_machine *) = CAST(func, get_vfunc(m_vtbl, 0x18));
        func(this, nullptr, &data, a3, a4);
        return data;
    }

    request_data category::do_incoming_trans(
        animation_logic_system *a3,
        state_machine *a4)
    {
        als::request_data data;
        void (__fastcall *func)(void *, void *, request_data *, animation_logic_system *, state_machine *) = CAST(func, get_vfunc(m_vtbl, 0x24));
        func(this, nullptr, &data, a3, a4);
        return data;
    }

    int category::get_mash_sizeof() const
    {
        int (__fastcall *func)(const category *) = CAST(func, get_vfunc(m_vtbl, 0x34));
        return func(this);
    }

    void category::_unmash(mash_info_struct *a1, void *a3)
    {
        TRACE("als::category::unmash");

        if constexpr (1)
        {
            a1->unmash_class_in_place(this->field_4, this);

#if OPENUSM_XBOX_MASH_FORMAT && !defined(OPENUSM_XBPACK_V10)
            {
                uint8_t class_mashed = -1;
                class_mashed = *a1->read_from_buffer(mash::SHARED_BUFFER, 1, 1);
                assert(class_mashed == 0xAF || class_mashed == 0);
            }
#endif

            if ( this->field_C != nullptr )
            {
                a1->unmash_class(this->field_C, this
#if OPENUSM_XBOX_MASH_FORMAT
                    , mash::NORMAL_BUFFER
#endif
                        );
            }
        }
        else
        {
            THISCALL(0x0049F110, this, a1, a3);
        }
    }
}

void als_category_patch()
{
    {
        FUNC_ADDRESS(address, &als::category::_unmash);
        set_vfunc(0x00875708, address);
    }
}
