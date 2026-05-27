#pragma once

struct vector3d;
struct vector4d;
struct matrix4x4;
struct quaternion;

#include <cstdint>

#include <fixedstring.h>

struct nalBaseSkeleton {
    std::intptr_t m_vtbl;
    int field_4;
    tlFixedString field_8;
    tlFixedString field_28;

    const tlFixedString & GetName() const
    {
        return this->field_8;
    }

    const tlFixedString & GetAnimTypeName() const
    {
        return this->field_28;
    }

};

//0x0078DC80
extern void *nalConstructSkeleton(void *a1);

namespace inverse_kinematics {
    extern void __cdecl nalIKMap2DTo3D(
        float chain_scale,
        float sin0,
        float cos0,
        float sin1,
        float cos1,
        vector3d* origin,
        vector3d* bone_axis_dir,
        vector4d* bend_dir,
        float chain_sin0,
        float chain_cos0,
        matrix4x4* joint0,
        matrix4x4* joint1);

    extern void __cdecl nalIKSolve2D(
        matrix4x4* hinge,
        vector3d* root,
        vector3d* target,
        float b0a_len,
        float b1a_len,
        float b0b_len,
        float b1b_len,
        vector3d* proj_point,
        vector3d* bone_axis_dir,
        float* sin0,
        float* cos0,
        float* sin1,
        float* cos1);

    struct ik_bone_chain_t
    {
        float b0a_len;
        float b1a_len;
        float b0b_len;
        float b1b_len;
        float chain_scale;
    };
    using get_bend_dir_t = vector3d * (__cdecl*)(
        vector3d* outTmp,
        matrix4x4* line_xform,
        matrix4x4* effector,
        float       bendX,
        float       bendY,
        float       bendZ);

    extern void __cdecl solve_two_bone(
        matrix4x4* j0,
        matrix4x4* j1,
        matrix4x4* line_xform,
        vector3d* root,
        matrix4x4* effector,
        ik_bone_chain_t* chain,
        get_bend_dir_t get_bend_dir);

    extern void __cdecl DecomposeIKSpin(
        matrix4x4* joint0,
        matrix4x4* joint1,
        matrix4x4* hinge,
        vector3d* root,
        matrix4x4* effector,
        ik_bone_chain_t* chain,
        get_bend_dir_t get_bend_dir,
        float                   twistAngle);

    extern vector4d* __cdecl compute_bend_plane_normal(
        vector4d* out,
        float*     /*unused*/,
        matrix4x4* m,
        float      axis_x,
        float      axis_y,
        float      axis_z);

    extern vector3d* __cdecl compute_arm_elbow_bend_direction(
        vector3d* out,
        matrix4x4* m,
        matrix4x4* /*ent*/,
        float      dirX,
        float      dirY,
        float      dirZ);

    extern vector3d* __cdecl compute_arm_elbow_bend_direction_mirrored(
        vector3d* out,
        matrix4x4* m,
        matrix4x4* /*ent*/,
        float      dirX,
        float      dirY,
        float      dirZ);

    extern quaternion* __cdecl quat_blend(
        quaternion* quat, quaternion* quatA, float* weightA, quaternion* quatB, vector4d* weights);

}