#pragma once

#include <cstdint>

#include <float.hpp>

struct nalPositionOrientation;
struct nalMatrix4x4;

namespace nalComp {
    struct nalCompAnim;
}

struct BaseComponent {
    std::intptr_t m_vtbl;

    //virtual ~BaseComponent() = default;

    //virtual
    int GetType();

    //virtual
    void * ApplyPublicPerSkelDataOffset(uint32_t a1, void *a2) const;

    //virtual
    void * ApplyPublicPerAnimDataOffset(uint32_t a1, const void *) const;
    
    //virtual
    nalPositionOrientation * GetTrajectoryData(
        nalPositionOrientation *out,
        uint32_t a2,
        const void *a3,
        const void *a4);

    //virtual
    void BuildBoneMatrices(
        nalMatrix4x4 *a1,
        uint32_t a2,
        const void *a3,
        const void *a4);

    //virtual
    void * GetSizeOfPerInstData(
        uint32_t a2,
        const void *a3,
        const void *a4,
        const void *a5,
        const void *a6,
        const void *a7,
        bool a8);

    //virtual
    void DestroyPerInstData(
        void *a2,
        uint32_t a3,
        const void *a4,
        const void *a5);

    //virtual
    void CalcPoseDataDirect(
        void *a1,
        uint32_t a2,
        Float a3,
        Float a4,
        const nalComp::nalCompAnim *a5,
        const void *a6,
        const void *a7,
        const void *a8,
        void *a9);

    //virtual
    void CalcPoseDataRemapped(
        void *a1,
        uint32_t a2,
        Float a3,
        Float a4,
        const nalComp::nalCompAnim *a5,
        const void *a6,
        const void *a7,
        const void *a8,
        void *a9);

    //virtual
    void BlendPoseData(void *a1, uint32_t a2, Float a3,const void *a4, const void *a5);

    //virtual
    void SkelPoseProcess(uint32_t a1, void *a2, void *a3) const;

    //virtual
    void SkelPoseRelease(uint32_t a1, void *a2, void *a3) const;

    //virtual
    void PoseDataFree(uint32_t , void *) const;
};

