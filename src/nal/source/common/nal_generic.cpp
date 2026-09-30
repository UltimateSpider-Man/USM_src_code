#include "nal_generic.h"

#include "func_wrapper.h"
#include "common.h"
#include "nal_system.h"
#include "tl_instance_bank.h"
#include "vtbl.h"

namespace nalGeneric {

VALIDATE_SIZE(nalGenericSkeleton, 0xE4);
VALIDATE_SIZE(nalComponentInfo, 0x30);
VALIDATE_OFFSET(nalGenericSkeleton, field_64, 0x64);
VALIDATE_OFFSET(nalGenericSkeleton, m_pose_alignment, 0x94);

VALIDATE_SIZE(nalGenericPose, 0xC);

nalGenericSkeleton::nalGenericSkeleton()
{
    vtbl_ptr() = (int) std::addressof(bit_cast<int *>(this)[0]);
}

uint32_t nalGenericSkeleton::GetPoseAlignment() const
{
    return static_cast<uint32_t>(m_pose_alignment);
}

bool nalGenericSkeleton::UsesXboxV10PoseLayout() const
{
    // PC and Xbox Generic skeletons share the same version/type identifiers.
    // The serialized pose alignment is the format-owned discriminator: retail
    // PC layouts use 4, while Xbox v10 layouts with bridged components use 16.
    return field_4 == 0x00010200u && GetPoseAlignment() == 16u;
}

void nalGenericSkeleton::Process()
{
    if constexpr (1) {
        auto v2 = this->field_64;
        auto v3 = this->field_6C;
        this->field_68 = (int) &this->field_E0;
        auto *v4 = (char *) &this->field_E0 + v2;
        auto v5 = 48 * this->field_74;
        this->field_70 = (int) v4;
        auto v6 = (uint32_t) &v4[v3 + 3] & 0xFFFFFFFC;
        this->field_78 = v6;
        auto v7 = v5 + v6 + 3;
        auto v8 = 5 * this->field_7C;
        auto v9 = this->m_pose_alignment;
        v7 &= 0xFFFFFFFC;
        this->field_84 = v7;
        auto v10 = (v7 + 8 * v8 + 3) & 0xFFFFFFFC;
        auto v11 = v10 + 48 * this->field_88;
        this->field_8C = CAST(field_8C, v10);
        auto v12 = v11 + v9 - 1;
        auto v13 = this->field_9C;
        auto v14 = ~(v9 - 1) & v12;
        auto v15 = (this->field_90 + v14 + 3) & 0xFFFFFFFC;
        this->field_A0 = v15;
        auto v16 = v13 + v15 + 3;
        auto v17 = this->field_B0;
        v16 &= 0xFFFFFFFC;
        auto v18 = (char *) (v16 + 0x30 * this->field_A4);
        this->field_A8 = CAST(field_A8, v16);
        auto v19 = (int) &v18[v17 - 1];
        auto v20 = this->field_B8;
        auto v21 = ~(v17 - 1) & v19;
        auto v22 = this->field_AC;
        this->field_B4 = v21;
        auto v23 = v22 + v21 + 3;
        auto v24 = this->field_C4;
        v23 &= 0xFFFFFFFC;
        this->field_BC = v23;
        this->field_98 = v14;
        this->field_C8 = ~(v24 - 1) & (v23 + v20 + v24 - 1);
        this->field_CC = (int) this;
        this->field_D0 = v14;

        //sp_log("%s", this->field_8C[0].field_0.c_str());

        for (int i = 0; i < this->field_88; ++i) {
            this->field_8C[i].field_20 = static_cast<decltype(this->field_8C[i].field_20)>(
                nalComponentInstanceBank.Search(this->field_8C[i].field_0)->field_20);
        }

        for (int i = 0; i < this->field_A4; ++i) {
            this->field_A8[i].field_20 = static_cast<decltype(this->field_A8[i].field_20)>(
                nalComponentInstanceBank.Search(this->field_A8[i].field_0)->field_20);
        }

        auto v29 = this->field_A0;

        auto *v30 = this->field_8C;
        auto v38 = this->field_98;

        auto v37 = v29;
        if (this->field_88 > 0) {
            for (int i = 0; i < this->field_88; ++i) {
                void (__fastcall *Process)(void *, void *, void *, void *, void *) = CAST(Process, get_vfunc(v30->field_20->m_vtbl, 0x10));
                Process(v30->field_20, nullptr, v30, &v38, &v37);

                ++v30;
            }
        }

        auto v34 = this->field_BC;
        auto *v35 = this->field_A8;
        v38 = this->field_B4;

        v37 = v34;
        if (this->field_A4 > 0) {
            for (int i = 0; i < this->field_A4; ++i) {
                void (__fastcall *Process)(void *, void *, void *, void *, void *) = CAST(Process, get_vfunc(v35->field_20->m_vtbl, 0x10));

                Process(v35->field_20, nullptr, v35, &v38, &v37);

                ++v35;
            }
        }

    } else {
        THISCALL(0x00793610, this);
    }
}

void nalGenericSkeleton::Release() {
    ;
}

template<>
void nalGenericSkeleton::GetComponentHandle<nalPositionOrientation>(
        nalGenericConstComponentHandle<nalPositionOrientation> &a2,
        tlFixedString &a3,
        tlFixedString &a4) const
{
    THISCALL(0x004AE7B0, this, &a2, &a3, &a4);
}

template<>
void nalGenericSkeleton::GetComponentHandle<float>(
        nalGenericComponentHandle<float> &a2,
        tlFixedString &a3,
        tlFixedString &a4)
{
    THISCALL(0x004AE9F0, this, &a2, &a3, &a4);
}

template<>
void nalGenericSkeleton::GetComponentHandle<float>(
        nalGenericConstComponentHandle<float> &a2,
        tlFixedString &a3,
        tlFixedString &a4) const
{
    THISCALL(0x004AEC90, this, &a2, &a3, &a4);
}

template<>
void nalGenericSkeleton::GetComponentHandle<nalPositionOrientation>(
        nalGenericComponentHandle<nalPositionOrientation> &a2,
        tlFixedString &a3,
        tlFixedString &a4)
{
    THISCALL(0x004AE660, this, &a2, &a3, &a4);
}

template<>
void nalGenericSkeleton::GetComponentHandle<unsigned char>(
        nalGenericComponentHandle<unsigned char> &a2,
        tlFixedString &a3,
        tlFixedString &a4)
{
    THISCALL(0x004AEB40, this, &a2, &a3, &a4);
}

} // namespace nalGeneric
