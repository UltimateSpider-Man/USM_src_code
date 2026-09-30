#include "combo_system.h"
#include "common.h"
#include "mash_info_struct.h"
#include "trace.h"
#include "vtbl.h"

#include <cstdint>

VALIDATE_SIZE(combo_system_chain, 0x44);
VALIDATE_SIZE(combo_system_chain::telegraph_info, 0xC);
VALIDATE_SIZE(combo_system, 0x50);

#if defined(OPENUSM_XBPACK_V10) && !defined(TARGET_XBOX)
namespace
{
struct pc_mash_info
{
    uint8_t *image;
    int used;
    int size;
    int field_C;
};

static_assert(sizeof(pc_mash_info) == 0x10);
}
#endif


void combo_system_chain::telegraph_info::_unmash(mash_info_struct *, void *)
{
    TRACE("combo_system_chain::telegraph_info::unmash");

    ;
}

int combo_system_chain::telegraph_info::get_mash_sizeof()
{
    int (__fastcall *func)(combo_system_chain::telegraph_info *) = CAST(func, get_vfunc(m_vtbl, 0x18));
    return func(this);
}

void combo_system_chain::unmash(mash_info_struct *a1, void *)
{
    TRACE("combo_system_chain::unmash");

    a1->unmash_class_in_place(this->field_0, this);
    a1->unmash_class_in_place(this->field_14, this);
    a1->unmash_class_in_place(this->field_1C, this);
}

combo_system::combo_system() {}

combo_system_weapon *combo_system::get_weapon(int idx) {
    return this->field_28.m_data[(uint16_t) idx];
}

int combo_system::get_num_weapons() {
    return this->field_28.size();
}

void combo_system::unmash(mash_info_struct *a1, void *a3)
{
    TRACE("combo_system::unmash");

    if constexpr (OPENUSM_XBOX_MASH_FORMAT)
    {
#if defined(OPENUSM_XBPACK_V10) && !defined(TARGET_XBOX)
        auto *pc_mash = reinterpret_cast<pc_mash_info *>(a1);
        mash_info_struct mash_ctx {pc_mash->image, pc_mash->size};
        mash_ctx.buffer_size_used[mash::NORMAL_BUFFER] = pc_mash->used;
        a1 = &mash_ctx;
#endif

        a1->unmash_class_in_place(this->field_0, this);

        a1->unmash_class_in_place(this->field_14, this);

        a1->unmash_class_in_place(this->field_28, this);

        a1->unmash_class_in_place(this->field_3C, this);

#if defined(OPENUSM_XBPACK_V10) && !defined(TARGET_XBOX)
        pc_mash->used = a1->buffer_size_used[mash::NORMAL_BUFFER];
#endif
    }
    else
    {
        THISCALL(0x00489720, this, a1, a3);
    }
}

void combo_system_patch()
{
    {
        FUNC_ADDRESS(address, &combo_system_chain::telegraph_info::_unmash);
        set_vfunc(0x008737C4, address);
    }

    {
        FUNC_ADDRESS(address, &combo_system::unmash);
        REDIRECT(0x006D7265, address);
    }
}
