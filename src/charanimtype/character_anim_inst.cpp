#include "character_anim_inst.h"

#include "character_pose_skel.h"
#include "common.h"
#include "trace.h"
#include "utility.h"

#include <cassert>

VALIDATE_SIZE(nalChar::nalCharInstance, 0x20u);


void nalChar::nalCharInstance::VirtualGetPose(
        Float a1,
        Float a2,
        nalBasePose *a3,
        const nalBasePose *a4)
{
    TRACE("nalChar::nalCharInstance::VirtualGetPose");

    const nalCharPose *v5 = nullptr;
    if ( a4 != nullptr ) {
        v5 = (const nalCharPose *)&a4[-1];
    }

    nalCharPose *v6 = nullptr;
    if (a3 != nullptr) {
        v6 = (nalCharPose *) &a3[-1];
    }

    this->GetPose(a1, a2, v6, v5);
}

void * nalChar::nalCharAnim::GetPerAnimDataByName(CharComponentBase::Names a2)
{
    TRACE("nalCharAnim::GetPerAnimDataByName");

    auto *v3 = (nalCharSkeleton *) this->GetSkeleton();
    int CompIxByName = v3->GetCompIxByName(a2);
    if ( CompIxByName == -1 ) {
        return nullptr;
    }

    auto *CompPerAnimDataInt = (const void *) this->GetCompPerAnimDataInt(CompIxByName);
    auto *v9 = this->field_30->field_70[CompIxByName].m_component;
    return v9->ApplyPublicPerAnimDataOffset(a2, CompPerAnimDataInt);
}

void nalCharInstance_patch()
{
    {
        FUNC_ADDRESS(address, &nalChar::nalCharInstance::VirtualGetPose);
        set_vfunc(0x00891FF8, address);
    }

    {
        FUNC_ADDRESS(address, &nalChar::nalCharAnim::GetPerAnimDataByName);
        SET_JUMP(0x005F0840, address);
    }
}
