#pragma once

#include "color32.h"
#include "fixed_vector.h"
#include "float.hpp"
#include "mstring.h"
#include "vector3d.h"

struct vector3d;
struct mString;

struct debug_string_t {
    bool field_0;
    mString field_4;
    color32 field_10;
    vector3d field_14;
    float field_20;
    float field_24;

    debug_string_t() = default;

    debug_string_t(bool a2, const vector3d &a3, color32 a4, float a5, const mString &a1, float a7);
};

extern fixed_vector<debug_string_t, 25> *debug_strings;

extern void add_3d_debug_str(const vector3d &arg0, color32 a2, Float a3, const mString &a1);

extern void render_3d_debug_strings();

extern void frame_advance_3d_debug_strings(Float time_inc);

extern void clear_3d_debug_strings();
