#pragma once

#include "charcompressor.h"
#include <float.hpp>

namespace nalChar {
    struct nalCharAnim;
}

template<typename T>
struct FakerootEntCompDecomp {

    struct PerInstData {
        char field_0[0x90];
        CharEntropyQuantConverter::EncTrackData field_90;
        float field_D0;
        float field_D4;
        float field_D8;
        float field_DC;
        float field_E0;
    };

    //0x005FF0E0
    void RetrievePoseFromInst(
        T::StdPoseData &a1,
        PerInstData *a2,
        const T::PerAnimData *a3);

    void GetPose(
        T::StdPoseData *a1,
        unsigned int arg4,
        Float a3,
        Float a4,
        const nalChar::nalCharAnim *a6,
        const T::PerSkelData *a7,
        const T::PerAnimData *a8,
        const void *a9,
        PerInstData *a2,
        const T *a11);

};

extern void FakerootEntCompDecomp_patch();
