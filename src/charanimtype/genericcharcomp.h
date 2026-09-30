#pragma once

#include "charcomponentbase.h"

#include <float.hpp>


struct GenericCharComp : CharComponentBase {
    GenericCharComp();

    //virtual
    void BuildBoneMatrices(
        nalMatrix4x4 *a1,
        uint32_t a2,
        const void *a3,
        const void *a4);

    //virtual
    void CalcPoseDataDirect(
        void *a3,
        uint32_t _24,
        Float arg8,
        Float a5,
        const nalComp::nalCompAnim *a6,
        const void *a7,
        const void *a8,
        const void *a9,
        void *a10);
};

extern void GenericCharComp_patch();
