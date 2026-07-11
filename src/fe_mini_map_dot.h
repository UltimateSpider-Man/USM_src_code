#pragma once

#include "color32.h"
#include "mini_map_dot_type.h"
#include "vector3d.h"

struct PanelQuad;
struct nglQuad;

// Trivially-copyable mirror of vector3d for the constructor-hook ABI boundary.
//
// vector3d has a user-provided copy constructor, so i686-w64-mingw32-g++
// passes it BY HIDDEN POINTER and pops only 8 bytes (`ret 8`), while the two
// stock MSVC call sites (0x005C9D84 / 0x00641215) construct the 12 bytes
// INLINE in the argument slot and the stock ctor does `ret 0x10`. Redirecting
// them onto a `vector3d`-by-value hook therefore misreads the argument AND
// unbalances ESP by 8. A trivial aggregate restores the MSVC layout exactly:
// this in ECX, [esp+4]=type, [esp+8..0x10]=x,y,z inline, callee pops 0x10.
struct mini_map_dot_pos {
    float x;
    float y;
    float z;

    operator vector3d() const {
        return vector3d{x, y, z};
    }
};

struct fe_mini_map_dot {
    PanelQuad *field_0;
    PanelQuad *field_4;
    nglQuad *field_8;
    nglQuad *field_C;
    nglQuad *field_10;
    vector3d field_14;
    mini_map_dot_type field_20;
    bool field_24;
    bool field_25;
    bool field_26;
    bool field_27;
    int highlight_circle_count_down;


fe_mini_map_dot(mini_map_dot_type a2, vector3d a3);


void fe_mini_map_dot_hook(mini_map_dot_type a2, vector3d a3);

void Draw();





void SetZvalueAbs(Float a2);


void InitLines();

};

extern void fe_mini_map_dot_patch();


