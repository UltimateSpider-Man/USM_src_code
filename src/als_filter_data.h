#pragma once

#include <cstdint>

struct mash_info_struct;

namespace als
{
    struct filter_data {
        uint32_t field_0;
        float field_4;
        float field_8;

        void unmash(mash_info_struct *, void *);
    };
}
