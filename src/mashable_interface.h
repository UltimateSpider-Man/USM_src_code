#pragma once

#if defined(TARGET_XBOX) || defined(OPENUSM_XBPACK_MODE)

#include "mash_virtual_base.h"
#include "mash_info_struct.h"

template<typename T>
struct mashable_interface : mash_virtual_base
{
    T *ifc = nullptr;

    auto *get_interface()
    {
        return ifc;
    }

    bool exists() const
    {
        return ifc != nullptr;
    }

    void custom_un_mash(generic_mash_header *a2, void *a3, generic_mash_data_ptrs *a4)
    {
        int a1 = 0;
        std::memcpy(&a1, a4->field_0, 4);
        a4->field_0 += 4;
        if ( a1 != 0 )
        {
            auto v9 = 4 - ((int) a4->field_0 % 4);
            if ( v9 < 4 )
            {
                a4->field_0 += v9;
            }

            int size = 0;
            std::memcpy(&size, a4->field_0, 4);
            a4->field_0 += 4;

            auto v7 = 16 - ((int) a4->field_0 % 16);
            if ( v7 < 0x10 )
            {
                a4->field_0 += v7;
            }

            uint8_t *a2a = nullptr;
            auto v5 = 4 - ((int) a4->field_0 % 4);
            if ( v5 < 4 )
            {
                a4->field_0 += v5;
            }

            a2a = CAST(a2a, a4->field_0);
            a4->field_0 += size;
            mash_info_struct v4 {mash::UNMASH_MODE, a2a, size, true};
            v4.unmash_class(this->ifc, 0, mash::NORMAL_BUFFER);
            mash_info_struct::construct_class(this->ifc);
        }
    }
};

#endif
