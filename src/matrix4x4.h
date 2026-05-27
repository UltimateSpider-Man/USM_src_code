#pragma once

#include "float.hpp"
#include "vector4d.h"

#include "matrix4x3.h"

struct vector3d;

struct matrix4x4 {
    vector4d arr[3];
    vector4d w;

    matrix4x4() {
        arr[0][0] = 0.0;
        arr[0][1] = 0.0;
        arr[0][2] = 0.0;
        arr[0][3] = 0.0;
        arr[1][0] = 0.0;
        arr[1][1] = 0.0;
        arr[1][2] = 0.0;
        arr[1][3] = 0.0;
        arr[2][0] = 0.0;
        arr[2][1] = 0.0;
        arr[2][2] = 0.0;
        arr[2][3] = 0.0;
        arr[3][0] = 0.0;
        arr[3][1] = 0.0;
        arr[3][2] = 0.0;
        arr[3][3] = 0.0;
    }

    matrix4x4(float a2,
              float a3,
              float a4,
              float a5,
              float a6,
              float a7,
              float a8,
              float a9,
              float a10,
              float a11,
              float a12,
              float a13,
              float a14,
              float a15,
              float a16,
              float a17);

    //0x0048AA30
    matrix4x4(const vector3d &a2, const vector3d &a3, const vector3d &a4,
            const vector3d &a5);

    matrix4x4(const matrix4x4 &a1);

    matrix4x4 &operator=(const matrix4x4 &a1) = default;

    auto &operator[](int idx) {
        return arr[idx];
    }

    const auto &operator[](int idx) const {
        return arr[idx];
    }

    void sub_415740(void *a2);

    //0x00588160
    void make_scale(const vector3d &v);

    //0x00597D50
    void make_rotate(const vector3d &axis, Float angle);

    //0x00588110
    void make_translate(const vector3d &a2);

    //0x00587DA0
    void make_projection(Float a2, Float a3, Float near_plane, Float far_plane, Float a6);

    //0x00597E70
    void rotate(const vector3d &v, Float angle);

    //0x00588CE0
    float det();

    //0x00588BF0
    float cofactor(int a2, int a3);

    //@bug
    //0x00597EC0
    matrix4x4 inverse();

    //0x00588D20
    matrix4x4 Cof();

    //0x00588BA0
    [[nodiscard]] matrix4x4 transpose() const;

    void sub_41D8A0(void *a2);

    matrix4x4 sub_76CA50(const matrix4x4 &arg0);

    void sub_76CF20(void *a2);

    void sub_76CE70(void *a2);

    void sub_415A30(const void *a2);

    //0x004134B0
    void decompose(vector4d &a2, vector4d &a3, vector4d &a4, vector4d &a5) const;

    //0x005817B0
    matrix4x4 *operator*=(Float a2);

    friend matrix4x4 operator*(const matrix4x4 &a2, const matrix4x4 &a3);

    //0x0048A8A0
    friend vector3d operator*(const matrix4x4 &a2, const vector3d &a3);

    void sub_415650(const matrix4x3 &a2);

    //0x00588390
    void scale(Float a2);

    matrix4x4* compose_from_basis(vector4d* a2,
        vector4d* a3,
        vector4d* a4,
        vector4d* a5)
    {
        matrix4x4* this_1 = this;
        this->arr[0] = *a2;
        this->arr[1] = *a3;
        this->arr[2] = *a4;
        this->arr[3] = *a5;
        return this_1;
    }

    const char *to_string() const;
};

extern const matrix4x4 identity_matrix;

extern vector3d sub_55DCB0(const matrix4x4 &a2, const vector3d &a3);

extern vector3d sub_5B1370(const matrix4x4 &a2, vector3d a3);

extern vector3d sub_501B20(const matrix4x4 &a2, const vector3d &a3);

extern matrix4x3 sub_413770(const matrix4x4 &arg4);

extern void matrix4x4_patch();


using local_to_world_t = int(__cdecl*)(matrix4x4*, matrix4x4*, matrix4x4*);
using ProjectPointOntoLineXform_t = vector3d * (__cdecl*)(vector3d*, vector3d*, matrix4x4*);

[[maybe_unused]]
inline local_to_world_t local_to_world = reinterpret_cast<local_to_world_t>(0x005FE000);


[[maybe_unused]]
inline ProjectPointOntoLineXform_t ProjectPointOntoLineXform = reinterpret_cast<ProjectPointOntoLineXform_t>(0x005FF390);