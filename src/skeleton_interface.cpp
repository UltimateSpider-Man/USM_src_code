#include "skeleton_interface.h"

#include "common.h"
#include "conglom.h"
#include "entity_base.h"
#include "oldmath_po.h"
#include "trace.h"

#if defined(TARGET_XBOX) || defined(OPENUSM_XBPACK_MODE)
#include "mash_info_struct.h"
#endif

VALIDATE_SIZE(skeleton_interface, 0x14);

skeleton_interface::skeleton_interface()
{

}

#if defined(TARGET_XBOX) || defined(OPENUSM_XBPACK_MODE)
void skeleton_interface::unmash(mash_info_struct *a2, void *a3)
{
    assert("abs_po == (po *)mash::CUSTOM_MASH_SENTRY");

    if ( this->po_count <= 0 )
        this->abs_po = nullptr;
    else
        this->abs_po = (po *)a2->read_from_buffer(mash::NORMAL_BUFFER,
                this->po_count << 6, 16);
}
#endif

void skeleton_interface::connect_bone_abs_po(int bone_idx, entity_base *new_bone)
{
    TRACE("skeleton_interface::connect_bone_abs_po");

    assert(bone_idx >= 0 && bone_idx < po_count && bone_idx < 255);

    assert(abs_po != nullptr);

    assert(new_bone->my_abs_po == new_bone->my_rel_po);

    new_bone->my_abs_po = &this->abs_po[bone_idx];
    assert(this->field_4 != nullptr);
    this->field_4->field_8 |= 0x10000000u;
}
