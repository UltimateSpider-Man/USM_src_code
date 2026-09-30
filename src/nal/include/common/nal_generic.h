#pragma once

#include "hashstring.h"
#include "variable.h"

#include <cstdint>

namespace nalGeneric {

struct nalGenericSkeleton;

struct nalGenericAnim {
    std::intptr_t m_vtbl;
    int field_4[9];

    tlHashString field_28;
    unsigned int field_2C;
    nalGenericSkeleton *field_30;
    int field_34;
    int field_38;
    int field_3C;
    int field_40;
    int field_44;
    int field_48;
    int field_4C;
    int field_50;

    struct vtbl {};

    static int vtbl_ptr;
};

struct nalComponentInfo {
    tlHashString field_0;
    char field_4[0x1C];
    struct {
        std::intptr_t m_vtbl;
    } * field_20;
    int field_24;
    int field_28;
    int field_2C;
};

template<uint32_t I>
struct MorphSliderPoseTemplate {
    int field_0 {0xFF};
    int field_4;
    int field_8;
    int field_C;
};

template<typename T>
struct nalGenericComponentHandle {
    nalGeneric::nalGenericSkeleton *Skeleton {nullptr};
    int field_4;
    int field_8;
    int field_C;

};

template<typename T>
struct nalGenericConstComponentHandle {
    nalGeneric::nalGenericSkeleton *field_0 {nullptr};
    int field_4;
    int field_8;
    int field_C;
};

struct nalGenericSkeleton {
    std::intptr_t m_vtbl;
    uint32_t field_4;
    int field_8[8];

    tlHashString field_28;
    int field_2C[9];
    int field_50;
    int field_54[4];

    int field_64;
    int field_68;
    int field_6C;
    int field_70;
    int field_74;
    int field_78;
    int field_7C;
    int field_80;
    int field_84;
    int field_88;
    nalComponentInfo *field_8C;
    int field_90;
    int m_pose_alignment;
    int field_98;
    int field_9C;
    int field_A0;
    int field_A4;
    nalComponentInfo *field_A8;
    int field_AC;
    int field_B0;
    int field_B4;
    int field_B8;
    int field_BC;
    int field_C0;
    int field_C4;
    int field_C8;
    int field_CC;
    int field_D0;
    int field_D4;
    int field_D8;
    int field_DC;
    int field_E0;

    template<typename T>
    void GetComponentHandle(
        nalGenericComponentHandle<T> &a2,
        tlFixedString &a3,
        tlFixedString &a4);

    template<typename T>
    void GetComponentHandle(
        nalGenericConstComponentHandle<T> &a2,
        tlFixedString &a3,
        tlFixedString &a4) const;

    struct vtbl {
        void *field_0;
        void *field_4;
        void (nalGenericSkeleton::*Process)();
        void (nalGenericSkeleton::*Release)();
        bool (nalGenericSkeleton::*CheckVersion)();
    };

    nalGenericSkeleton();

    //0x00793610
    void Process();

    void Release();

    uint32_t GetPoseAlignment() const;

    bool UsesXboxV10PoseLayout() const;

    bool CheckVersion() {
        return this->field_4 == 0x10200;
    }

    static inline Var<std::intptr_t> vtbl_ptr {0x009770E0};

};

struct nalGenericPose {
    nalGenericSkeleton *field_0;
    int field_4;
    bool field_8;

    auto * GetSkeleton() const {
        return this->field_0;
    }

    template<typename T>
    T * operator[](nalGeneric::nalGenericComponentHandle<T> &handle)
    {
        static T g_invalidObject {};

        assert(handle.Skeleton != nullptr && "attempting to de-reference an invalid handle");

        if ( handle->Skeleton == nullptr ) {
            return &g_invalidObject;
        }

        assert(handle.Skeleton == GetSkeleton() && "handle and pose skeletons don't match");

        return bit_cast<T *>(handle->field_4->field_2C + 12 * handle->field_8 + this->field_4);
    }
};

} // nalGeneric
