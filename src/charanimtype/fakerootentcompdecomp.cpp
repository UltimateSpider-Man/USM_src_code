#include "fakerootentcompdecomp.h"

#include "character_anim_inst.h"
#include "common.h"
#include "fakerootposedesc.h"
#include "func_wrapper.h"
#include "trace.h"
#include "utility.h"

#include <cmath>

float *__fastcall sub_5FC520(float *self, void *, const float *a2)
{
    auto v2 = a2[2];
    auto result = self;
    auto v4 = a2[1];
    auto v5 = a2[0];
    auto v6 = a2[1];
    auto v7 = a2[2];
    self[0] = a2[0];
    self[1] = v6;
    self[2] = v7;
    auto v8 = std::sqrt(std::abs(1.0f - (v4 * v4 + v2 * v2 + v5 * v5)));
    self[3] = v8;
    return result;
}

template<>
void FakerootEntCompDecomp<FakerootPoseDesc>::RetrievePoseFromInst(
        FakerootPoseDesc::StdPoseData &a1,
        FakerootEntCompDecomp<FakerootPoseDesc>::PerInstData *a2,
        const FakerootPoseDesc::PerAnimData *a3)
{
    if constexpr (0) {
        int v4 = 0;
        if ( (a3->field_1C & 1) != 0 )
        {
            auto v5 = a2->field_90.field_0[1][0];
            auto v6 = a2->field_90.field_0[2][0];
            auto v12 = a2->field_90.field_0[0][0];
            [[maybe_unused]] auto v13 = v5;
            auto v14 = v6;

            float v15[4];
            auto v7 = sub_5FC520(v15, nullptr, &v12);
            a1.field_0[0] = v7[0];
            a1.field_0[1] = v7[1];
            a1.field_0[2] = v7[2];
            a1.field_0[3] = v7[3];
            auto v8 = a2->field_D0;
            auto v9 = a2->field_E0;
            v12 = a2->field_90.field_0[3][0];
            v13 = v8;
            v14 = v9;
            auto v10 = v8;
            a1.field_10[0] = v12;
            auto v11 = v14;
            a1.field_10[1] = v10;
            a1.field_10[2] = v11;
            v4 = 6;
        }

        if ( (a3->field_1C & 2) != 0 ) {
            a1.field_1C = a2->field_90.field_0[v4][0];
        }
    } else {
        THISCALL(0x005FF0E0, this, &a1, a2, a3);
    }
}

template<>
void FakerootEntCompDecomp<FakerootPoseDesc>::GetPose(
        FakerootPoseDesc::StdPoseData *a2,
        uint32_t a3,
        Float a4,
        Float a5,
        const nalChar::nalCharAnim *a6,
        const FakerootPoseDesc::PerSkelData *a7,
        const FakerootPoseDesc::PerAnimData *a8,
        const void *a9,
        FakerootEntCompDecomp<FakerootPoseDesc>::PerInstData *a10,
        const FakerootPoseDesc *a11)
{
    TRACE("FakerootEntCompDecomp<FakerootPoseDesc>::GetPose");

    if constexpr (0) {
#if 0
        v92 = (int)a8 + a8->field_24;
        v11 = a8->field_1C;
        self = this;
        if ( (v11 & 4) == 0 || (v11 & 3) != 0 )
        {
            v13 = a1;
            v12 = a6;
        }
        else
        {
            v12 = a6;
            v13 = a1;
            FakerootPoseDesc::GenerateSignalPoseData(a11, a1, a8, a3, a4, a6);
        }
        v83 = a8->field_1C & 1;
        nalChar::nalCharAnim::ComputeFrameValues(v12, &v93, &v91, &v102, &v90, a3);
        if ( a4 != a2->field_8C )
        {
            nalChar::nalCharAnim::ComputeFrameValues(v12, &v85, &a5a, (unsigned int *)&v93, &v89, a4);
            v84 = 0;
            if ( v83 && (v12->field_34 & 1) != 0 )
            {
                v14 = LODWORD(v93);
                if ( LODWORD(v93) > a5a )
                {
                    a5a = a6->field_50 - 1;
                    v84 = 1;
                    v14 = a5a - 1;
                }
            }
            else
            {
                v14 = LODWORD(v93);
            }
            v15 = a2->field_7C;
            if ( v14 == v15 )
                goto LABEL_24;
            if ( v15 == -1 )
                goto LABEL_17;
            if ( v14 == v15 + 1 )
            {
                a2->field_7C = v14;
                qmemcpy(&a2->field_10, &a2->field_40, 0x30u);
                v16 = self;
    LABEL_21:
                v19 = a5a;
                if ( !a5a )
                {
                    LODWORD(v86) = 65280;
                    a2->field_70.field_0 = (void *)a9;
                    a2->field_70.field_4 = 65280;
                }
                sub_5FF1A0(v16, a2, a8, a6, v92, v19);
                FakerootEntCompDecomp<FakerootPoseDesc>::RetrievePoseFromInst(v16, &a2->field_40, a2, a8);
                v13 = a1;
    LABEL_24:
                if ( v84 )
                {
                    v20 = a8->field_10 - a2->field_10.field_10[0];
                    v21 = a8->field_14 - a2->field_10.field_10[1];
                    v87 = a8->field_18 - a2->field_10.field_10[2];
                    v95 = v20 * v89;
                    v85 = v95 + a2->field_10.field_10[0];
                    v86 = v21 * v89 + a2->field_10.field_10[1];
                    v22 = LODWORD(v86);
                    v23 = v87 * v89 + a2->field_10.field_10[2];
                    a2->field_80 = v85;
                    a2->field_84 = v22;
                    v87 = v23;
                    a2->field_88 = v87;
                    v24 = sub_5FD0C0(&v95, v89, a2->field_10.field_0, &a8->field_0);
                    a2->field_0[0] = *v24;
                    a2->field_0[1] = v24[1];
                    a2->field_0[2] = v24[2];
                    a2->field_0[3] = v24[3];
                }
                else
                {
                    FakerootPoseDesc::BlendPoseDataPartial(a11, &v112, arg4, v89, &a2->field_10, &a2->field_40, a8->field_1C);
                    v25 = LODWORD(v112.field_10[1]);
                    a2->field_80 = v112.field_10[0];
                    v26 = v112.field_10[2];
                    a2->field_84 = v25;
                    a2->field_88 = v26;
                    v27 = v112.field_0[1];
                    a2->field_0[0] = v112.field_0[0];
                    v28 = v112.field_0[2];
                    a2->field_0[1] = v27;
                    v29 = v112.field_0[3];
                    a2->field_0[2] = v28;
                    a2->field_0[3] = v29;
                }
                goto LABEL_27;
            }
            if ( v14 <= v15 )
            {
    LABEL_17:
                v17 = 0;
                LODWORD(v86) = 65280;
                a2->field_70.field_0 = (void *)a9;
                a2->field_70.field_4 = 65280;
            }
            else
            {
                v17 = v15 + 2;
            }
            for ( ; v17 <= v14; ++v17 )
                sub_5FF1A0(self, a2, a8, a6, v92, v17);
            v16 = self;
            v18 = self;
            a2->field_7C = v14;
            FakerootEntCompDecomp<FakerootPoseDesc>::RetrievePoseFromInst(v18, &a2->field_10, a2, a8);
            goto LABEL_21;
        }
    LABEL_27:
        v30 = a2->field_7C;
        v31 = v102;
        if ( v102 == v30 )
            goto LABEL_40;
        if ( v30 == -1 )
            goto LABEL_33;
        if ( v102 != v30 + 1 )
        {
            if ( (int)v102 > v30 )
            {
    LABEL_34:
                for ( i = v30 + 2; i <= v31; ++i )
                    sub_5FF1A0(self, a2, a8, a6, v92, i);
                v32 = self;
                v34 = self;
                a2->field_7C = v31;
                FakerootEntCompDecomp<FakerootPoseDesc>::RetrievePoseFromInst(v34, &a2->field_10, a2, a8);
                goto LABEL_37;
            }
    LABEL_33:
            i = 0;
            LODWORD(v86) = 65280;
            a2->field_70.field_0 = (void *)a9;
            a2->field_70.field_4 = 65280;
            goto LABEL_34;
        }
        a2->field_7C = v102;
        qmemcpy(&a2->field_10, &a2->field_40, 0x30u);
        v31 = v102;
        v32 = self;
    LABEL_37:
        v35 = v91;
        if ( !v91 )
        {
            LODWORD(v86) = 65280;
            a2->field_70.field_0 = (void *)a9;
            a2->field_70.field_4 = 65280;
        }
        sub_5FF1A0(v32, a2, a8, a6, v92, v35);
        FakerootEntCompDecomp<FakerootPoseDesc>::RetrievePoseFromInst(v32, &a2->field_40, a2, a8);
        v13 = a1;
    LABEL_40:
        FakerootPoseDesc::BlendPoseDataPartial(a11, v13, arg4, v90, &a2->field_10, &a2->field_40, a8->field_1C);
        if ( !v83 )
            goto LABEL_51;
        if ( (a6->field_34 & 1) != 0 && v31 > v91 )
        {
            v36 = a8->field_10 - a2->field_10.field_10[0];
            v37 = a8->field_14 - a2->field_10.field_10[1];
            v97 = a8->field_18 - a2->field_10.field_10[2];
            v85 = v36 * v90;
            v85 = v85 + a2->field_10.field_10[0];
            v86 = v37 * v90 + a2->field_10.field_10[1];
            v38 = v86;
            v39 = v97 * v90 + a2->field_10.field_10[2];
            v13->field_10[0] = v85;
            v13->field_10[1] = v38;
            v40 = v90;
            v87 = v39;
            v13->field_10[2] = v87;
            v41 = sub_5FD0C0(&v95, v40, a2->field_10.field_0, &a8->field_0);
            v13->field_0[0] = *v41;
            v13->field_0[1] = v41[1];
            v13->field_0[2] = v41[2];
            v13->field_0[3] = v41[3];
        }
        a2->field_8C = a3;
        v42 = LODWORD(v13->field_0[1]);
        v95 = v13->field_0[0];
        v43 = v13->field_0[2];
        v96 = *(float *)&v42;
        v44 = LODWORD(v13->field_0[3]);
        v45 = LODWORD(v13->field_10[0]);
        v97 = v43;
        v46 = LODWORD(v13->field_10[1]);
        v99 = v45;
        v98 = *(float *)&v44;
        v47 = LODWORD(v13->field_10[2]);
        v100 = v46;
        v48 = LODWORD(a2->field_0[0]);
        v101 = v47;
        v49 = LODWORD(a2->field_0[1]);
        v111[0] = v48;
        v50 = LODWORD(a2->field_0[2]);
        v51 = &a2->field_80;
        v111[1] = v49;
        v52 = LODWORD(a2->field_0[3]);
        v53 = a2->field_80;
        v111[2] = v50;
        v54 = a2->field_84;
        *(float *)&v111[4] = v53;
        v111[5] = v54;
        v55 = (a6->field_34 & 1) == 0;
        v111[3] = v52;
        v56 = a2->field_88;
        v91 = (unsigned int)&a2->field_80;
        *(float *)&v111[6] = v56;
        if ( v55 )
            goto LABEL_49;
        if ( (unsigned __int64)a3 == (unsigned __int64)a4 || a4 == float_NULL )
        {
            v51 = (float *)v91;
    LABEL_49:
            v70 = sub_5FF2B0((float *)v111, (vector4d *)&v112);
            sub_5FF490((int)&v103, &v95, v70->base.arr);
            v71 = v108;
            v72 = v109;
            v13->field_10[0] = v107;
            LODWORD(v13->field_10[1]) = v71;
            v73 = v103;
            LODWORD(v13->field_10[2]) = v72;
            v74 = v104;
            v13->field_0[0] = v73;
            v75 = v105;
            LODWORD(v13->field_0[1]) = v74;
            v76 = v106;
            LODWORD(v13->field_0[2]) = v75;
            LODWORD(v13->field_0[3]) = v76;
            goto LABEL_50;
        }
        v57 = a8->field_4;
        v103 = a8->field_0;
        v58 = a8->field_8;
        v104 = v57;
        v59 = a8->field_C;
        v60 = a8->field_10;
        v105 = v58;
        v61 = a8->field_14;
        v107 = v60;
        v106 = v59;
        v62 = a8->field_18;
        v108 = LODWORD(v61);
        v109 = LODWORD(v62);
        v63 = sub_5FF2B0((float *)v111, (vector4d *)&v112);
        sub_5FF490((int)v110, &v103, v63->base.arr);
        qmemcpy(v110, (const void *)sub_5FF490((int)&v112, &v95, v110), sizeof(v110));
        v64 = v110[5];
        v51 = (float *)v91;
        a1->field_10[0] = v110[4];
        v65 = v110[6];
        a1->field_10[1] = v64;
        v66 = v110[0];
        a1->field_10[2] = v65;
        v67 = v110[1];
        a1->field_0[0] = v66;
        v68 = v110[2];
        a1->field_0[1] = v67;
        v69 = v110[3];
        a1->field_0[2] = v68;
        a1->field_0[3] = v69;
        v13 = a1;
    LABEL_50:
        v77 = v96;
        v78 = v97;
        a2->field_0[0] = v95;
        v79 = v98;
        a2->field_0[1] = v77;
        v80 = v99;
        a2->field_0[2] = v78;
        v81 = v100;
        a2->field_0[3] = v79;
        v82 = v101;
        *(_DWORD *)v51 = v80;
        *((_DWORD *)v51 + 1) = v81;
        *((_DWORD *)v51 + 2) = v82;
    LABEL_51:
        if ( (a8->field_1C & 4) != 0 )
            FakerootPoseDesc::GenerateSignalPoseData(a11, v13, a8, a3, a4, a6);
#endif
    } else {
        THISCALL(0x005FE980,
                this,
                a2,
                a3,
                a4,
                a5,
                a6,
                a7,
                a8,
                a9,
                a10,
                a11);
    }
}

void FakerootEntCompDecomp_patch()
{
    {
        auto func = &FakerootEntCompDecomp<FakerootPoseDesc>::GetPose;

        FUNC_ADDRESS(address, func);
        REDIRECT(0x005FE974, address);
    }
}
