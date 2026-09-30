#pragma once

#include "charcomponentbase.h"

template<typename T0, typename T1>
struct FlexibleCharComp : CharComponentBase {

    T0 field_14;
    T1 field_15;
    int field_1C;
    T0::StdPoseData field_20;

    FlexibleCharComp(
        int a2,
        const char *a1);

    //0x005FE940
    void CalcPoseDataDirect(
        void *a2,
        uint32_t a3,
        Float a4,
        Float a5,
        const nalComp::nalCompAnim *a6,
        const void *a7,
        const void *a8,
        const void *a9,
        void *a10);

    void CalcPoseDataRemapped(
        void *,
        uint32_t ,
        Float ,
        Float ,
        const nalComp::nalCompAnim *,
        const void *,
        const void *,
        const void *,
        void *);
};

extern void FlexibleCharComp_patch();
