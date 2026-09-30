#include "genericcharcomp.h"

#include "charcomponentmanager.h"
#include "common.h"
#include "nal_generic.h"
#include "trace.h"
#include "utility.h"

GenericCharComp::GenericCharComp()
{
    this->m_vtbl = 0x00892140;
    this->m_strTypeString = "Generic";
    CharComponentManager::RegisterComponent(this);
}

void GenericCharComp::BuildBoneMatrices(
    nalMatrix4x4 *a1,
    uint32_t ,
    const void *a3,
    const void *a4)
{
    bit_cast<const nalGeneric::nalGenericSkeleton *>(a3)->GetBoneMatrices(bit_cast<const nalGeneric::nalGenericPose *>(a4), a1);
}

void GenericCharComp::CalcPoseDataDirect(
        void *a3,
        uint32_t ,
        Float arg8,
        Float a5,
        const nalComp::nalCompAnim *,
        const void *,
        const void *,
        const void *,
        void *a10)
{
    TRACE("GenericCharComp::CalcPoseDataDirect");

    nalGeneric::nalGenericPose a1 {*static_cast<nalGeneric::nalGenericPose *>(a3), true};
    static_cast<nalGeneric::nalGenericInstance *>(a10)->GetPose(
            arg8,
            a5,
            *static_cast<nalGeneric::nalGenericPose *>(a3),
            a1);
}

void GenericCharComp_patch()
{
    {
        FUNC_ADDRESS(address, &GenericCharComp::CalcPoseDataDirect);
        set_vfunc(0x00892170, address);
    }
}

