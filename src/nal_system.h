#pragma once

#include "fixedstring.h"
#include "float.hpp"
#include "hashstring.h"
#include "nfl_system.h"
#include "tl_system.h"
#include "variable.h"

#include <nal_generic.h>
#include <nalcomp/nal_pose_comp.h>

#include <memory>

struct BaseComponent {
    std::intptr_t m_vtbl;
    //virtual ~BaseComponent() = default;

    //virtual
    int GetType() { return 0; }

    //virtual
    void * ApplyPublicPerSkelDataOffset(uint32_t a1, void *a2) const;

    //virtual
    void SkelPoseProcess(uint32_t a1, void *a2, void *a3) const;

    //virtual
    void PoseDataFree(uint32_t , void *) const;
};

extern tlInstanceBank & nalTypeInstanceBank;

extern tlInstanceBank & nalComponentInstanceBank;

namespace PanelComponentMgr {
extern int *& comp_list;
}

struct nal_anim_control {
    uint32_t m_vtbl;
};

struct nalMatrix4x4 {};

struct nalPositionOrientation {
    float arr[4];
};

struct nalBasePose {
    nalComp::nalCompSkeleton *field_0;
};

struct nalBaseSkeleton;

namespace nalChar {
struct nalCharAnim {
    struct vtbl {
        void *field_0;
        void *finalize;
        void *Process;
        void *Release;

        using CheckVersion_t = bool (nalCharAnim::*)();
        CheckVersion_t CheckVersion;

        vtbl(void *, void *, void *, void *, CheckVersion_t a4) : CheckVersion(a4) {}
    };

    std::intptr_t m_vtbl;
    int field_4;
    int field_8;
    int field_C;
    int field_10;
    int field_14;
    int field_18;
    int field_1C;
    int field_20;
    int field_24;
    int field_28;
    uint32_t field_2C;

    bool CheckVersion() {
        return this->field_2C == 0x10003;
    }

    static int vtbl_ptr;
};


} // namespace nalChar

namespace nalPanel {

struct nalPanelAnim {
    struct vtbl {
        void *field_0;
        void *finalize;
        void __fastcall (*Process)(void *);
        void *Release;
        bool (nalPanelAnim::*CheckVersion)();
    };

    std::intptr_t m_vtbl;
    uint32_t field_4;

    bool CheckVersion() {
        return this->field_4 == 0x300;
    }

    static int vtbl_ptr;
};

} // namespace nalPanel

namespace nalPed {

struct nalPedAnim {
    struct vtbl {
        void *field_0;
        void *finalize;
        void *Process;
        void *Release;

        using CheckVersion_t = bool (nalPedAnim::*)();
        CheckVersion_t CheckVersion;
    };

    static int vtbl_ptr;
};

struct nalPedSkeleton {
    struct vtbl {
        void *field_0;
        void *finalize;
        void *Process;
        void *Release;

        using CheckVersion_t = bool (nalPedAnim::*)();
        CheckVersion_t CheckVersion;
    };

    static int vtbl_ptr;
};

} // namespace nalPed

namespace nalCam {

struct nalCamAnim {
    struct vtbl {
        void *field_0;
        void *finalize;
        void *Process;
        void *Release;

        using CheckVersion_t = bool (nalCamAnim::*)();
        CheckVersion_t CheckVersion;
    };

    static int vtbl_ptr;
};

struct nalCamSkeleton {
    struct vtbl {
        void *field_0;
        void *finalize;
        void *Process;
        void *Release;

        using CheckVersion_t = bool (nalCamSkeleton::*)();
        CheckVersion_t CheckVersion;
    };

    static int vtbl_ptr;
};

} // namespace nalCam

struct nalAnimFile {
    uint32_t field_0;
    uint32_t field_4;
    int field_8;
    int num_skeletons;
    tlFixedString field_10;
    int field_30;
    char *field_34;
    int field_38[3];
    int field_44;
    tlFixedString field_48;
};

struct nalBaseSkeleton;

//0x0078DC60
extern nalBaseSkeleton *nalGetSkeleton(const tlFixedString &a1);

struct nalComponentU8Base {
    int *GetType();

    static inline int TypeID{0};
};

struct nalComponentStringBase {
    char *GetType();

    static inline char & TypeID = var<char>(0x00959560);
};

struct spideySignalData {};

struct spideySignal {};

struct nalComponentInitList;

template<typename T>
T nalSkeletonPtrCast(T a1) {
    if (a1 != nullptr || *a1 != nalGeneric::nalGenericSkeleton::vtbl_ptr) {
        return nullptr;
    }

    return a1;
}


struct nalHeap;
struct nalSceneAnim;

//0x00783CF0
extern void nalInit(nalHeap *a1);

//0x00783C60
extern void nalExit();

//0x0078D9B0
extern void nalReleaseSceneAnimInternal(nalSceneAnim *a1);

//0x0078D8D0
extern bool nalLoadSceneAnimInternal(nalSceneAnim *a1);

//0x0078D540
extern bool nalLoadAnimFileInternal(nalAnimFile *anim_file);

extern void nalSetSkeletonDirectory(tlResourceDirectory<nalBaseSkeleton, tlFixedString> *a1);

extern void nalSetAnimFileDirectory(tlResourceDirectory<nalAnimFile, tlFixedString> *a1);

struct nalAnyPose;

template<typename >
struct nalAnimClass;

extern void nalSetAnimDirectory(tlResourceDirectory<nalAnimClass<nalAnyPose>, tlFixedString> *a1);

extern tlResourceDirectory<nalAnimClass<nalAnyPose>, tlFixedString> *nalGetAnimDirectory();

extern void nalSetSceneAnimDirectory(tlResourceDirectory<nalSceneAnim, tlFixedString> *a1);

extern void nalStreamInstance_patch();

// one-time scan of mods/ for *.PCSKEL / *.PCANIM overrides, registered into
// the Mods multimap (see utility/mod.h); safe to call from any load path
extern void modScanNalOverrides();
