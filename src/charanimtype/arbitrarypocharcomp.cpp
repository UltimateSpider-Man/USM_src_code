#include "arbitrarypocharcomp.h"

#include "charcomponentmanager.h"
#include "common.h"
#include "func_wrapper.h"
#include "nal_system.h"
#include "string_hash.h"
#include "trace.h"
#include "utility.h"
#include "vector3d.h"
#include "vector4d.h"
#include "vtbl.h"

#include <cmath>

VALIDATE_SIZE(ArbitraryPOCharComp::PerSkelData, 0x1C);

VALIDATE_SIZE(ArbitraryPOCharComp::BoneData, 0x30);

VALIDATE_SIZE(ArbitraryPOCharComp::StdPoseData, 0x20);

nalMatrix4x4 sub_5FE000(const nalMatrix4x4 &arg4, const nalMatrix4x4 &arg8)
{
    nalMatrix4x4 result;

    if constexpr (0)
    {
        vector4d x_axis;
        x_axis[0] = arg8[0][0];
        x_axis[1] = arg8[0][1];
        x_axis[2] = arg8[0][2];
        x_axis[3] = arg8[0][3];

        vector4d y_axis;
        y_axis[0] = arg8[1][0];
        y_axis[1] = arg8[1][1];
        y_axis[2] = arg8[1][2];
        y_axis[3] = arg8[1][3];

        vector4d z_axis;
        z_axis[0] = arg8[2][0];
        z_axis[1] = arg8[2][1];
        z_axis[2] = arg8[2][2];
        z_axis[3] = arg8[2][3];

        vector4d w_axis;
        w_axis[0] = arg8[3][0];
        w_axis[1] = arg8[3][1];
        w_axis[2] = arg8[3][2];
        w_axis[3] = arg8[3][3];

        vector4d a3;
        a3[0] = arg4[0][0];
        a3[1] = arg4[0][1];
        a3[2] = arg4[0][2];
        a3[3] = arg4[0][3];

        vector4d a5;
        a5[0] = arg4[1][0];
        a5[1] = arg4[1][1];
        a5[2] = arg4[1][2];
        a5[3] = arg4[1][3];

        vector4d a7;
        a7[0] = arg4[2][0];
        a7[1] = arg4[2][1];
        a7[2] = arg4[2][2];
        a7[3] = arg4[2][3];

        vector4d arg8a;
        arg8a[0] = arg4[3][0];
        arg8a[1] = arg4[3][1];
        arg8a[2] = arg4[3][2];
        arg8a[3] = arg4[3][3];

        auto v16 = sub_4126E0(
                x_axis,
                a3,
                y_axis,
                a3,
                z_axis,
                a3);

        auto v21 = sub_4126E0(
                x_axis,
                a5,
                y_axis,
                a5,
                z_axis,
                a5);

        auto v25 = sub_4126E0(
                x_axis,
                a7,
                y_axis,
                a7,
                z_axis,
                a7);

        auto v29 = vector4d::sub_413E90(
                x_axis,
                arg8a,
                y_axis,
                arg8a,
                z_axis,
                arg8a,
                w_axis);

        nalMatrix4x4 result;
        result[0][0] = v16[0];
        result[0][1] = v16[1];
        result[0][2] = v16[2];
        result[0][3] = v16[3];
        result[1][0] = v21[0];
        result[1][1] = v21[1];
        result[1][2] = v21[2];
        result[1][3] = v21[3];
        result[2][0] = v25[0];
        result[2][1] = v25[1];
        result[2][2] = v25[2];
        result[2][3] = v25[3];
        result[3][0] = v29[0];
        result[3][1] = v29[1];
        result[3][2] = v29[2];
        result[3][3] = v29[3];
    } else {
        int (__cdecl *func)(nalMatrix4x4 *, const nalMatrix4x4 *, const nalMatrix4x4 *) = CAST(func, 0x005FE000);
        func(&result, &arg4, &arg8);
    }

    return result;
}

ArbitraryPOCharComp::ArbitraryPOCharComp()
{
    this->m_vtbl = 0x008920B8;

    this->m_strTypeString = "ArbitraryPO";
    CharComponentManager::RegisterComponent(this);
}

const void * ArbitraryPOCharComp::ApplyPublicPerSkelDataOffset(uint32_t, const void *a2)
{
    TRACE("ArbitraryPOCharComp::ApplyPublicPerSkelDataOffset");

    return a2;
}

int ArbitraryPOCharComp::ApplyPublicPerAnimDataOffset(uint32_t, const void *)
{
    TRACE("ArbitraryPOCharComp::ApplyPublicPerAnimDataOffset");

    return 0;
}

nalPositionOrientation * ArbitraryPOCharComp::GetTrajectoryData(
        nalPositionOrientation *out,
        uint32_t,
        const void *,
        const void *)
{
    TRACE("ArbitraryPOCharComp::GetTrajectoryData");

    *out = nalPositionOrientation::Identity;
    return out;
}

void ArbitraryPOCharComp::BuildBoneMatrices(
    nalMatrix4x4 *a1,
    uint32_t a2,
    const void *a3,
    const void *a4)
{
    TRACE("ArbitraryPOCharComp::BuildBoneMatrices");

    if constexpr (1)
    {
        auto *v5 = (const uint32_t *)a3;
        auto v23 = *((const DWORD *)a3 + 4);
        auto v24 = (char *)*((const DWORD *)a3 + 5);
        auto v6 = (const char *)a4 + 16;
        auto v7 = (const char *)a4 + 16 * *(const DWORD *)a4 + 16;

        for ( uint32_t i = 0; i < *v5; ++i)
        {
            int16_t *v9 = (int16_t *)(v5[6] + 48 * *(DWORD *)(v5[7] + 4 * i));
            if ( v9[18] != -1 )
            {
                const float *v10 = nullptr;
                if ( v9[20] ) {
                    v10 = (const float *)&v6[16 * (uint16_t)v9[16]];
                } else {
                    v10 = (const float *)(v23 + 16 * (uint16_t)v9[16]);
                }

                if ( !v9[21] ) {
                    v7 = v24;
                }

                auto *v15 = (const nalVector3 *)&v7[sizeof(nalVector3) * (uint16_t)v9[17]];

                nalPositionOrientation v26 {*v15, v10};

                nalMatrix4x4 v30 {v26};

                auto v18 = (int16_t)v9[18];
                a1[v18] = v30;

                int v19 = v9[19];
                if ( v19 != -1 )
                {
                    auto &v20 = a1[(int16_t)v9[18]];
                    v20 = sub_5FE000(v20, a1[v19]);
                }

                v5 = (const uint32_t *)a3;
            }
        }

    } else {
        void (__fastcall *func)(
                void *self,
                void *edx,
                nalMatrix4x4 *a1,
                uint32_t a2,
                const void *a3,
                const void *a4) = CAST(func, 0x005F5E60);
        func(this, nullptr, a1, a2, a3, a4);
    }
}

bool sub_C75AA0(const int *a1, uint32_t a2)
{
    return (a1[a2 >> 5] & (1 << (a2 % 32))) != 0;
}

int ArbitraryPOCharComp::GetSizeOfPerInstData(
        uint32_t,
        const void *,
        const void *a3,
        const void *,
        const void *a5,
        const void *,
        bool)
{
    TRACE("ArbitraryPOCharComp::GetSizeOfPerInstData");

    auto func = [](const int *a5, const int *a3) -> int {
        int result = 0;
        const int v9 = a3[1] + a3[2];
        for ( int i = 0; i < v9; ++i )
        {
            if ( sub_C75AA0(a5, i) ) {
                result += 3;
            }
        }

        return result;
    };

    return 16 * func(static_cast<const int *>(a5), static_cast<const int *>(a3)) + 60;
}

int ArbitraryPOCharComp::GetAlignOfPerInstData(
        uint32_t,
        const void *,
        const void *,
        const void *,
        const void *,
        const void *,
        bool)
{
    return 4;
}

int sub_C7F000(int a1, int a2)
{
    return ~(a2 - 1) & (a1 + a2 - 1);
}

void ArbitraryPOCharComp::BuildPerInstData(
        void *a1,
        uint32_t a2,
        const void *a3,
        const void *a4,
        const void *a5,
        const void *a6,
        const void *a7,
        bool a8)
{
    TRACE("ArbitraryPOCharComp::BuildPerInstData");

    if constexpr (1)
    {
        *(DWORD *)a1 = 0;
        uint32_t v12 = 0;
        for ( ; v12 < *((const DWORD *)a4 + 1); ++v12 )
        {
            if (sub_C75AA0(static_cast<const int *>(a6), v12)) {
                ++*(DWORD *)a1;
            }
        }

        for ( *((DWORD *)a1 + 1) = *(DWORD *)a1; v12 < *((const DWORD *)a4 + 1) + *((const DWORD *)a4 + 2); ++v12 )
        {
            if (sub_C75AA0(static_cast<const int *>(a6), v12)) {
                ++*((DWORD *)a1 + 1);
            }
        }

        auto v14 = sub_C7F000(*((const DWORD *)a4 + 2) + *((const DWORD *)a4 + 1), 32) >> 5;
        struct local_t {
            int field_0;
            int field_4;
            int field_8;
            int field_C;
            int field_10;
            int field_14;
            int field_18;
            CharEntropyDecoder::CharChannelDecoder field_1C;
            int field_24;
            int field_28;
            int field_2C;
            int field_30;
            int field_34;
            int field_38;

            local_t(const void *a2, int a3) : field_1C(a2, false), field_24(-1), field_28(a3) {}

        };
        local_t* v8 = CAST(v8, a1);

        VALIDATE_SIZE(local_t, 0x3C);

        if (v8 != nullptr) {
            new (v8) local_t {a7, (int)a6 + 4 * v14};
        }

        *((DWORD *)a1 + 7) = int(a7);
        *((BYTE *)a1 + 32) = 0;
        *((BYTE *)a1 + 33) = -1;
        *((WORD *)a1 + 17) = 0;
        auto v16 = (4 * (*((DWORD *)a1 + 1) + *(DWORD *)a1 + 2 * *((DWORD *)a1 + 1)) + 15) & 0xFFFFFFF0;

        v16 = 16 * *(DWORD *)a1;
        v16 += 12 * (*((DWORD *)a1 + 1) - *(DWORD *)a1);
        v16 = sub_C7F000(v16, 16u);

        *((DWORD *)a1 + 9) = -1;
        *((DWORD *)a1 + 3) = v16;
        auto *v17 = tlMemAlloc(2 * v16, 16u, 0);
        *((DWORD *)a1 + 6) = int(v17);
        tlMemFree(v17);
        auto *v18 = (char *)tlMemAlloc(2 * *((DWORD *)a1 + 3), 16u, 0);
        auto *v20 = &v18[*((DWORD *)a1 + 3)];
        *((DWORD *)a1 + 6) = int(v18);
        *((DWORD *)a1 + 4) = int(v18);
        auto v21 = 3 * *((DWORD *)a1 + 1);
        *((DWORD *)a1 + 5) = int(v20);
        *((DWORD *)a1 + 2) = v21;

        if ( a4 == a3 )
        {
            *((DWORD *)a1 + 11) = 0;
            *((DWORD *)a1 + 12) = 0;
            *((DWORD *)a1 + 13) = 0;
            *((DWORD *)a1 + 14) = 0;
        } else {
            *((DWORD *)a1 + 11) = 1;
            *((DWORD *)a1 + 13) = int(a4);

            auto dwSize = sub_C7F000(16 * *((const DWORD *)a4 + 1) + 16 + 12 * *((const DWORD *)a4 + 2), 16);
            DWORD *v22 = (DWORD *)tlMemAlloc(dwSize, 16u, 0);
            *((DWORD *)a1 + 12) = int(v22);
            *v22 = *((const DWORD *)a4 + 1);
            *(DWORD *)(*((DWORD *)a1 + 12) + 4) = *((const DWORD *)a4 + 1) + *((const DWORD *)a4 + 2);
            *((DWORD *)a1 + 14) = (int)tlMemAlloc(8 * (*((const DWORD *)a3 + 1) + *((const DWORD *)a3 + 2)), 4u, 0);

            int v14 = 0;
            int v13 = *((const DWORD *)a3 + 1);
            for ( uint32_t j = 0; j < *(const DWORD *)a3; ++j )
            {
                auto *v25 = bit_cast<int16_t *>(*((const DWORD *)a3 + 6) + 0x30 * j);
                if ( v25[20] || v25[21] )
                {
                    uint32_t k;
                    for ( k = 0;
                          k < *(const DWORD *)a4 && (*bit_cast<const tlFixedString *>(*((const DWORD *)a4 + 6) + 0x30 * k) != *bit_cast<const tlFixedString *>(v25));
                          ++k )
                    {
                        ;
                    }

                    if ( k == *(const DWORD *)a4 )
                    {
                        if ( v25[20] )
                        {
                            *(DWORD *)(*((DWORD *)a1 + 14) + 8 * v14++) = -1;
                        }

                        if ( v25[21] ) {
                            *(DWORD *)(*((DWORD *)a1 + 14) + 8 * v13++) = -1;
                        }
                    }
                    else
                    {
                        auto *v30 = (uint16_t *)(*((const DWORD *)a4 + 6) + 0x30 * k);
                        if ( v25[20] )
                        {
                            if ( v30[20] )
                            {
                                if (sub_C75AA0(static_cast<const int *>(a6), v30[16]))
                                {
                                    *(DWORD *)(*((DWORD *)a1 + 14) + 8 * v14) = v30[16];
                                    *(DWORD *)(*((DWORD *)a1 + 14) + 8 * v14 + 4) = 1;
                                }
                                else
                                {
                                    *(DWORD *)(*((DWORD *)a1 + 14) + 8 * v14) = -1;
                                }

                                ++v14;
                            }
                            else
                            {
                                *(DWORD *)(*((DWORD *)a1 + 14) + 8 * v14) = v30[16];
                                *(DWORD *)(*((DWORD *)a1 + 14) + 8 * v14 + 4) = 0;
                                ++v14;
                            }
                        }

                        if ( v25[21] )
                        {
                            if ( v30[21] )
                            {
                                if (sub_C75AA0(static_cast<const int *>(a6), *((const DWORD *)a4 + 1) + v30[17]))
                                {
                                    *(DWORD *)(*((DWORD *)a1 + 14) + 8 * v13) = v30[17];
                                    *(DWORD *)(*((DWORD *)a1 + 14) + 8 * v13 + 4) = 1;
                                }
                                else
                                {
                                    *(DWORD *)(*((DWORD *)a1 + 14) + 8 * v13) = -1;
                                }

                                ++v13;
                            }
                            else
                            {
                                *(DWORD *)(*((DWORD *)a1 + 14) + 8 * v13) = v30[17];
                                *(DWORD *)(*((DWORD *)a1 + 14) + 8 * v13 + 4) = 0;
                                ++v13;
                            }
                        }
                    }
                }
            }
        }
    } else {
        void (__fastcall *func)(
                void *,
                void *edx,
                void *,
                uint32_t,
                const void *,
                const void *,
                const void *,
                const void *,
                const void *,
                bool) = CAST(func, 0x005F2270);
        func(this, nullptr, a1, a2, a3, a4, a5, a6, a7, a8);
    }
}

void ArbitraryPOCharComp::DestroyPerInstData(void *a1,
        uint32_t,
        const void *,
        const void *)
{
    TRACE("ArbitraryPOCharComp::DestroyPerInstData");

    tlMemFree(*((void **)a1 + 6));
    auto v4 = *((DWORD *)a1 + 11);
    *((DWORD *)a1 + 4) = 0;
    *((DWORD *)a1 + 5) = 0;
    *((DWORD *)a1 + 6) = 0;
    if ( v4 != 0 )
    {
        tlMemFree(*((void **)a1 + 14));
        tlMemFree(*((void **)a1 + 12));
        *((DWORD *)a1 + 14) = 0;
        *((DWORD *)a1 + 12) = 0;
        *((DWORD *)a1 + 13) = 0;
        *((DWORD *)a1 + 11) = 0;
    }
}

bool ArbitraryPOCharComp::WillMapToComponentData(uint32_t, uint32_t, uint32_t a4)
{
    TRACE("ArbitraryPOCharComp::WillMapToComponentData");

    return a4 == this->GetType();
}

void ArbitraryPOCharComp::CalcPoseDataDirect(
        void *a1,
        uint32_t a2,
        Float a3,
        Float a4,
        const nalComp::nalCompAnim *a5,
        const void *a6,
        const void *a7,
        const void *a8,
        void *a9)
{
    TRACE("ArbitraryPOCharComp::CalcPoseDataDirect");

    sp_log("a2 = %u, a3 = %f, a4 = %f, a5 = 0x%08X, a6 = 0x%08X, a7 = 0x%08X, a8 = 0x%08X, a9 = 0x%08X",
            a2, a3, a4, int(a5), int(a6), int(a7), int(a8), int(a9));

    if constexpr (0) {
#if 0
        nalChar::nalCharAnim::ComputeFrameValues(
                (const nalChar::nalCharAnim *)a5,
                (float *)&v23,
                &v21,
                (unsigned int *)&a3,
                &v22,
                a3);
        v11 = (ArbitraryPOCharComp::PerInstData *)a9;
        v12 = *((_DWORD *)a9 + 9);
        v13 = LODWORD(a3);
        if ( LODWORD(a3) != v12 )
        {
            if ( LODWORD(a3) + 1 == v12 )
            {
                v14 = (unsigned __int8 *)*((_DWORD *)a9 + 4);
                v15 = (unsigned __int8 *)*((_DWORD *)a9 + 5);
                *((float *)a9 + 9) = a3;
                v11->field_10 = v15;
                v11->field_14 = v14;
            }
            else
            {
                if ( v12 == -1 || SLODWORD(a3) <= v12 )
                {
                    v16 = 0;
                    v24 = 65280;
                    *((_DWORD *)a9 + 7) = a8;
                    v11->field_1C.field_4 = 65280;
                }
                else
                {
                    v16 = v12 + 2;
                }
                for ( ; v16 <= v13; ++v16 )
                    ArbitraryPOCharComp::AdvanceAnimDataOneFrame(this, v11, (const nalChar::nalCharAnim *)a5, v16);
                v20 = v11->field_10;
                v11->field_24 = v13;
                ArbitraryPOCharComp::RetrievePoseFromInst(this, v20, v11);
            }
            v17 = v21;
            if ( !v21 )
            {
                v24 = 65280;
                v11->field_1C.field_0 = (void *)a8;
                v11->field_1C.field_4 = 65280;
            }
            ArbitraryPOCharComp::AdvanceAnimDataOneFrame(this, v11, (const nalChar::nalCharAnim *)a5, v17);
            ArbitraryPOCharComp::RetrievePoseFromInst(this, v11->field_14, v11);
        }
        v18 = 0;
        if ( v11->field_2C )
        {
            v18 = (ArbitraryPOCharComp::StdPoseData *)a1;
            v19 = v11->field_30;
        }
        else
        {
            v19 = (ArbitraryPOCharComp::StdPoseData *)a1;
        }
        ArbitraryPOCharComp::BlendAnimPoseToSkelData(this, v19, a2, v22, v11->field_10, v11->field_14, (unsigned int *)a7);
        if ( v11->field_2C )
            ArbitraryPOCharComp::CopyRemapDataFromTempPose(this, v18, v19, v11, (const ArbitraryPOCharComp::PerSkelData *)a6);
#endif
    } else {
        THISCALL(0x005F98E0, this, a1, a2, a3, a4, a5, a6, a7, a8, a9);
    }
}

void ArbitraryPOCharComp::CalcPoseDataRemapped(
        void *a1,
        uint32_t a2,
        Float a3,
        Float a4,
        const nalComp::nalCompAnim *a5,
        const void *a6,
        uint32_t a7,
        uint32_t a8,
        const void *a9,
        const void *a10,
        void *a11)
{
    TRACE("ArbitraryPOCharComp::CalcPoseDataRemapped");

    if constexpr (0) {
        this->CalcPoseDataDirect(
                a1,
                a2,
                a3,
                a4,
                a5,
                a6,
                a9,
                a10,
                a11);
    } else {
        THISCALL(0x005EF710, this, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11);
    }
}

void ArbitraryPOCharComp::BlendPoseData(
        void *a1,
        uint32_t a2,
        Float a3,
        const void *a4,
        const void *a5,
        uint32_t a6,
        uint32_t a7)
{
    TRACE("ArbitraryPOCharComp::BlendPoseData");

    if constexpr (1)
    {
        uint32_t i = 0;
        for (; i < a6; ++i)
        {
            auto v13 = sub_5FD0C0(
                a3,
                *static_cast<const vector4d *>(a4),
                *static_cast<const vector4d *>(a5)
            );

            vector3d *v14 = static_cast<vector3d *>(a1);
            *v14 = v13;

            a1 = static_cast<char *>(a1) + 16;
            a4 = static_cast<const char *>(a4) +16;
            a5 = static_cast<const char *>(a5) +16;
        }

        for ( ; i < a7; ++i )
        {
            vector3d *v16 = static_cast<vector3d *>(a1);
            const vector3d *v4 = static_cast<const vector3d *>(a4);
            const vector3d *v5 = static_cast<const vector3d *>(a5);
            auto v10 = (*v5) - (*v4);
            auto v11 = v10 * a3;
            auto v12 = (*v4) + v11;

            *v16 = v12;

            a1 = static_cast<char *>(a1) + 12;
            a4 = static_cast<const char *>(a4) + 12;
            a5 = static_cast<const char *>(a5) + 12;
        }
    } else {
        THISCALL(0x005F6130, this, a1, a2, a3, a4, a5, a6, a7);
    }
}

void ArbitraryPOCharComp::BlendPoseData(
        void *a1,
        uint32_t a2,
        Float a3,
        const void *a4,
        const void *a5)
{
    this->BlendPoseData(
        static_cast<char *>(a1) + 16,
        a2,
        a3,
        static_cast<const char *>(a4) + 16,
        static_cast<const char *>(a5) + 16,
        static_cast<const uint32_t *>(a4)[0],
        static_cast<const uint32_t *>(a4)[1]
    );
}

void ArbitraryPOCharComp::SkelPoseProcess(uint32_t , void *a2, void *)
{
    TRACE("ArbitraryPOCharComp::SkelPoseProcess");

    *((int *)a2 + 6) += int(a2);
    *((int *)a2 + 7) += int(a2);
    auto v5 = *((int *)a2 + 5);
    char *v6 = ( v5 != 0 ) ? ((char *)a2 + v5) : nullptr;

    *((int *)a2 + 5) = int(v6);
    auto v7 = *((int *)a2 + 4);

    if ( v7 != 0 ) {
        *((int *)a2 + 4) = int((char *)a2 + v7);
    } else {
        *((int *)a2 + 4) = 0;
    }
}

void ArbitraryPOCharComp::SkelPoseRelease(uint32_t, void *out, void *)
{
    TRACE("ArbitraryPOCharComp::SkelPoseRelease");

    *((DWORD *)out + 6) -= int(out);
    *((DWORD *)out + 7) -= int(out);
    auto v3 = *((DWORD *)out + 5);
    int v4;
    if ( v3 ) {
        v4 = v3 - (DWORD)out;
    } else {
        v4 = 0;
    }

    *((DWORD *)out + 5) = v4;
    auto v5 = *((DWORD *)out + 4);
    if ( v5 ) {
        *((DWORD *)out + 4) = v5 - (DWORD)out;
    } else {
        *((DWORD *)out + 4) = 0;
    }
}

void ArbitraryPOCharComp::AnimProcess(
        uint32_t,
        void *,
        void *,
        const void *)
{
    ;
}

void ArbitraryPOCharComp::AnimRelease(
        uint32_t,
        void *,
        void *,
        const void *)
{
    ;
}

void ArbitraryPOCharComp::CopyPoseExtraData(void *a1, uint32_t, const void *a3)
{
    TRACE("ArbitraryPOCharComp::CopyPoseExtraData");

    std::memcpy(
        a1,
        a3,
        16 * (*(const DWORD *)a3 + 1) + 12 * (*((const DWORD *)a3 + 1) - *(const DWORD *)a3)
    );
}

void ArbitraryPOCharComp::PoseDataFree(uint32_t, void *)
{
    ;
}

int ArbitraryPOCharComp::GetDomain() const
{
    return 5;
}

uint32_t ArbitraryPOCharComp::GetPoseTypeID() const
{
    return to_hash("ArbitraryPO");
}

void ArbitraryPOCharComp::CopyPoseDataToNothing(void *a1, uint32_t, const void *a3)
{
    TRACE("ArbitraryPOCharComp::CopyPoseDataToNothing");

    std::memcpy(a1, a3, 16 * (*(const DWORD *)a3 + 1) + 12 * (*((const DWORD *)a3 + 1) - *(const DWORD *)a3));
}

void sub_853300()
{
    static ArbitraryPOCharComp g_ArbitraryPOCharComp {};
}

void ArbitraryPOCharComp_patch()
{
    static constexpr auto address_vtbl = 0x008920B8;

    auto set_vfunc_local = [](std::intptr_t offset, auto func) {
        set_vfunc(address_vtbl + offset, func_address(func));
    };

    {
        set_vfunc_local(0x4, &ArbitraryPOCharComp::GetType);
        set_vfunc_local(0x8, &ArbitraryPOCharComp::ApplyPublicPerSkelDataOffset);
        set_vfunc_local(0xC, &ArbitraryPOCharComp::ApplyPublicPerAnimDataOffset);
        set_vfunc_local(0x10, &ArbitraryPOCharComp::GetTrajectoryData);
        set_vfunc_local(0x14, &ArbitraryPOCharComp::BuildBoneMatrices);
        set_vfunc_local(0x18, &ArbitraryPOCharComp::DoesContributeToPose);
        set_vfunc_local(0x1C, &ArbitraryPOCharComp::GetSizeOfPerInstData);
        set_vfunc_local(0x20, &ArbitraryPOCharComp::GetAlignOfPerInstData);
        set_vfunc_local(0x24, &ArbitraryPOCharComp::BuildPerInstData);
        set_vfunc_local(0x28, &ArbitraryPOCharComp::DestroyPerInstData);
        set_vfunc_local(0x2C, &ArbitraryPOCharComp::WillMapToComponentData);
        set_vfunc_local(0x30, &ArbitraryPOCharComp::CalcPoseDataDirect);
    }

    {
        void (ArbitraryPOCharComp::*func)(
            void *a1,
            uint32_t a2,
            Float a3,
            Float a4,
            const nalComp::nalCompAnim *a5,
            const void *a6,
            uint32_t a7,
            uint32_t a8,
            const void *a9,
            const void *a10,
            void *a11) = &ArbitraryPOCharComp::CalcPoseDataRemapped;
        set_vfunc_local(0x34, func);
    }

    {
        void (ArbitraryPOCharComp::*func)(
                void *,
                uint32_t,
                Float,
                const void *,
                const void *) = &ArbitraryPOCharComp::BlendPoseData;
        set_vfunc_local(0x38, func);
    }

    {
        set_vfunc_local(0x3C, &ArbitraryPOCharComp::SkelPoseProcess);
        set_vfunc_local(0x40, &ArbitraryPOCharComp::SkelPoseRelease);
        set_vfunc_local(0x44, &ArbitraryPOCharComp::AnimProcess);
        set_vfunc_local(0x48, &ArbitraryPOCharComp::AnimRelease);
        set_vfunc_local(0x4C, &ArbitraryPOCharComp::CopyPoseExtraData);
        set_vfunc_local(0x50, &ArbitraryPOCharComp::PoseDataFree);
        set_vfunc_local(0x54, &ArbitraryPOCharComp::GetDomain);
        set_vfunc_local(0x58, &ArbitraryPOCharComp::GetPoseTypeID);
        set_vfunc_local(0x5C, &CharComponentBase::GetRemapSizeOfPerInstData);
        set_vfunc_local(0x60, &CharComponentBase::GetRemapAlignOfPerInstData);
        set_vfunc_local(0x64, &CharComponentBase::BuildRemapPerInstData);
        set_vfunc_local(0x68, &CharComponentBase::DestroyRemapPerInstData);
        set_vfunc_local(0x6C, &CharComponentBase::CalcPoseDataRemapped);
        set_vfunc_local(0x70, &CharComponentBase::AnimRelease);
        set_vfunc_local(0x74, &ArbitraryPOCharComp::CopyPoseDataToNothing);
    }

    {
        SET_JUMP(0x00853300, sub_853300);
    }
}

