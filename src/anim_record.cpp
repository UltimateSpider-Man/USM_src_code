#include "anim_record.h"

#include "common.h"
#include "enum_anim_key.h"
#include "mash_info_struct.h"
#include "func_wrapper.h"
#include "utility.h"
#include "vtbl.h"
#include "mash_config.h"

VALIDATE_SIZE(anim_record, 0xC);
VALIDATE_SIZE(attach_anim_record, 0x1C);
VALIDATE_OFFSET(attach_anim_record, field_10, 0x10);

#if defined(OPENUSM_XBPACK_MODE) && defined(OPENUSM_XBPACK_V10)
namespace
{
// Xbox V10 type 0x89 maps to the prerelease-PC vtable at 0x00873948.
// Its stock unmash function (0x0069D5D0) handles anim_record followed by one
// string_hash, but it expects the PC 0x10-byte mash cursor.  Reproduce that
// exact field sequence here so the object consumes the Xbox dual-buffer
// cursor used by the surrounding ALS resource.
struct v10_hash_anim_record : anim_record
{
    string_hash field_C;

    void _unmash(mash_info_struct *a2, void *a3)
    {
        anim_record::_unmash(a2, a3);
        field_C.unmash(a2, this);
    }
};

VALIDATE_SIZE(v10_hash_anim_record, 0x10);
}
#endif

anim_record::anim_record()
{
    THISCALL(0x00695CE0, this);
}

void anim_record::_unmash(mash_info_struct *a2, void *)
{
    a2->unmash_class_in_place(this->field_8, this);
    if ( this->my_key != nullptr )
    {
        a2->unmash_class(this->my_key, this
#if OPENUSM_XBOX_MASH_FORMAT
            , mash::NORMAL_BUFFER
#endif
                );
    }
}

int anim_record::get_mash_sizeof()
{
    int (__fastcall *func)(anim_record *) = CAST(func, get_vfunc(m_vtbl, 0x1C));
    return func(this);
}

void attach_anim_record::_unmash(mash_info_struct *a2, void *a3)
{
    anim_record::_unmash(a2, a3);
    a2->unmash_class_in_place(this->field_10, this);
}


void anim_record_v10_patch()
{
#if defined(OPENUSM_XBPACK_MODE) && defined(OPENUSM_XBPACK_V10)
    FUNC_ADDRESS(address, &v10_hash_anim_record::_unmash);
    set_vfunc(0x0087394C, address);
#endif
}

void anim_record_patch()
{
    {
        FUNC_ADDRESS(address, &anim_record::_unmash);
        set_vfunc(0x0087392C, address);
    }

    anim_record_v10_patch();

}
