#include "flexiblecharcomp.h"

#include "charcomponentmanager.h"
#include "common.h"
#include "fakerootentcompdecomp.h"
#include "fakerootposedesc.h"
#include "func_wrapper.h"
#include "string_hash.h"
#include "trace.h"
#include "utility.h"


template<>
FlexibleCharComp<FakerootPoseDesc,FakerootEntCompDecomp<FakerootPoseDesc>>::FlexibleCharComp(
        int a2,
        const char *a1)
{
    this->m_vtbl = 0x008921C0;
    this->m_strTypeString = a1;
    this->m_TheType = to_hash(a1);
    this->field_10 = a2;
    CharComponentManager::RegisterComponent(this);
}

template<>
void FlexibleCharComp<FakerootPoseDesc,FakerootEntCompDecomp<FakerootPoseDesc>>::CalcPoseDataDirect(
        void *a2,
        uint32_t a3,
        Float a4,
        Float a5,
        const nalComp::nalCompAnim *a6,
        const void *a7,
        const void *a8,
        const void *a9,
        void *a10)
{
    TRACE("FlexibleCharComp<FakerootPoseDesc,FakerootEntCompDecomp<FakerootPoseDesc>>::CalcPoseDataDirect");

    if constexpr (1) {
        this->field_15.GetPose(
            static_cast<FakerootPoseDesc::StdPoseData *>(a2),
            a3,
            a4,
            a5,
            bit_cast<const nalChar::nalCharAnim *>(a6),
            bit_cast<const FakerootPoseDesc::PerSkelData *>(a7),
            bit_cast<const FakerootPoseDesc::PerAnimData *>(a8),
            a9,
            static_cast<FakerootEntCompDecomp<FakerootPoseDesc>::PerInstData *>(a10),
            &this->field_14
        );
    } else {
        THISCALL(0x005FE940, this, a2, a3, a4, a5, a6, a7, a8, a9, a10);
    }
}

template<>
void FlexibleCharComp<FakerootPoseDesc,FakerootEntCompDecomp<FakerootPoseDesc>>::CalcPoseDataRemapped(
        void *,
        uint32_t ,
        Float ,
        Float ,
        const nalComp::nalCompAnim *,
        const void *,
        const void *,
        const void *,
        void *)
{
    TRACE("FlexibleCharComp<FakerootPoseDesc,FakerootEntCompDecomp<FakerootPoseDesc>>::CalcPoseDataRemapped");

}

FlexibleCharComp<FakerootPoseDesc,FakerootEntCompDecomp<FakerootPoseDesc>> g_FakerootStdEntComp {0x40000000, "Fakeroot Entropy Compressed"};


void FlexibleCharComp_patch()
{
    {
        auto func = &FlexibleCharComp<FakerootPoseDesc,FakerootEntCompDecomp<FakerootPoseDesc>>::CalcPoseDataDirect;

        FUNC_ADDRESS(address, func);
        set_vfunc(0x008921F0, address);
    }

    {
        auto func = &FlexibleCharComp<FakerootPoseDesc,FakerootEntCompDecomp<FakerootPoseDesc>>::CalcPoseDataRemapped;

        FUNC_ADDRESS(address, func);
        set_vfunc(0x008921F4, address);
    }
}
