#pragma once

#include "nal_anim_comp.h"
#include "nal_instance.h"

#include "charcomponentbase.h"

struct nalBasePose;

namespace nalChar {

struct nalCharSkeleton;

struct nalCharInstance : nalComp::nalCompInstance {

    //virtual
    void VirtualGetPose(
        Float a1,
        Float a2,
        nalBasePose *a3,
        const nalBasePose *a4);
};

struct nalCharAnim : nalComp::nalCompAnim {
    struct vtbl {
        void *field_0;
        void *finalize;
        void *Process;
        void *Release;

        using CheckVersion_t = bool (nalCharAnim::*)();
        CheckVersion_t CheckVersion;

        vtbl(void *, void *, void *, void *, CheckVersion_t a4) : CheckVersion(a4) {}
    };

    bool CheckVersion() {
        return this->field_2C == 0x10003;
    }

    auto * GetSkeleton() {
        return this->field_30;
    }

    void * GetPerAnimDataByName(CharComponentBase::Names a2);

    static int vtbl_ptr;
};


} // namespace nalChar

extern void nalCharInstance_patch();
