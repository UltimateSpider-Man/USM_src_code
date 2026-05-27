#pragma once

#include "color32.h"
#include "ngl_vertexdef.h"
#include "sphere.h"
#include "variable.h"
#include "mstring.h"

#include <cstdint>

struct vector3d;
struct nglMesh;

enum debug_render_items_e 
{
    CAPSULE_HISTORY = 0,
    LIGHTS = 1,
    BOX_TRIGGERS = 2,
    WATER_EXCLUSION_TRIGGERS = 3,
    POINT_TRIGGERS = 4,
    ENTITY_TRIGGERS = 5,
    INTERACTABLE_TRIGGERS = 6,
    OCCLUSION = 7,
    LEGOS = 8,
    REGION_MESHES = 9,
    ENTITIES = 10,
    LOW_LODS = 11,
    ACTIVITY_INFO = 12,
    RENDER_INFO = 13,
    COLLIDE_INFO = 14,
    MARKERS = 15,
    PARKING_MARKERS = 16,
    WATER_EXIT_MARKERS = 17,
    MISSION_MARKERS = 18,
    PATHS = 19,
    GLASS_HOUSE = 20,
    OBBS = 21,
    TRAFFIC_PATHS = 22,
    MINI_GAME = 23,
    BRAINS = 24,
    VOICE = 25,
    PATROLS = 26,
    PAUSE_TIMERS = 27,
    ANIM_INFO = 28,
    SCENE_ANIM_INFO,
    TARGETING = 30,
    VIS_SPHERES = 31,
    LADDERS = 32,
    COLLISIONS = 33,
    BRAINS_ENABLED = 34,
    ANCHORS = 35,
    LINE_INFO = 36,
    SUBDIVISION = 37,
    SKELETONS = 38,
    SOUND_STREAM_USAGE = 39,
    SPHERES = 40,
    LINES = 41,
    CYLINDERS = 42,
    DGRAPH = 43,
    PEDS = 44,
    TRAFFIC = 45,
    ALS = 46,
    AI_COVER_MARKERS = 47,
    LIMBO_GLOW = 48,
    BIPED_COLL_VOLUMES = 49,
    DECALS = 50,
    DEBUG_RENDER_ITEMS_COUNT = 51
};

inline Var<mString [51]> debug_render_items_names{0x00961168};

struct debug_line {
    vector3d field_0;
    vector3d field_C;
    color32 field_18;
    float field_1C;

    debug_line() = default;

    debug_line(const vector3d &a2, const vector3d &a3, color32 a4, Float a5);

    void render(nglVertexDef_MultipassMesh<nglVertexDef_PCUV_Base>::Iterator *a2);
};

struct debug_lines_t {
    debug_line field_0[320];
    uint32_t m_size;
    bool field_2804;

    debug_lines_t() = default;

    void add_line(const debug_line &a2);

    void clear();

    void render();
};

struct debug_sphere {
    sphere field_0;
    color32 field_10;

    debug_sphere() = default;

    debug_sphere(const sphere &a2, color32 a3) : field_0(a2), field_10(a3) {}

    void render();
};

struct debug_spheres_t {
    debug_sphere field_0[512];
    uint32_t m_size;
    bool field_2804;

    debug_spheres_t() = default;

    void add_sphere(const debug_sphere &a2);

    void render();

    void clear();
};

extern debug_spheres_t debug_spheres;

extern void add_debug_line(vector3d a1, vector3d a2, color32 a7, Float a8);

extern void add_debug_sphere(vector3d a1, Float a4, color32 a5);

extern void render_debug_spheres();

extern void render_debug_lines();

//0x00578480
extern int debug_render_get_ival(debug_render_items_e item);

//0x0057AF90
extern void debug_render_set_ival(debug_render_items_e a1, int a2);

//0x00578490
extern bool debug_render_get_bval(debug_render_items_e item);

extern int debug_render_get_min(debug_render_items_e item);

extern int debug_render_get_max(debug_render_items_e item);

//0x005784F0
extern void debug_render_init();

extern void debug_render_done();

extern void render_debug_box(const vector3d &a2, const vector3d &a3, color32 a4);

extern void render_beam(const vector3d &a1, const vector3d &a2, color32 a3, Float a4, bool );

extern void render_quad(const vector3d &pt1, const vector3d &pt2, const vector3d &pt3, const vector3d &pt4, color32 col, bool double_sided);

extern void render_billboard(const vector3d &arg0, color32 a2, float a5, const char *a4);

struct line_info;

extern void debug_render_line_info(line_info *a1, [[maybe_unused]] char a2, int a3);

extern void debug_render_line_info();

extern void render_debug_capsule(const vector3d &a2, const vector3d &a3, Float a4, color32 a5);

extern void print_3d_text(const vector3d &arg0, color32 arg4, float a3, const char *a2, ...);

extern void print_3d_text(const vector3d &arg0, color32 arg4, const char *a2, ...);

extern void sub_CB3F80(const vector3d &a1,
                const vector3d &a2,
                const vector3d &a3,
                const vector3d &a4,
                int a5,
                void *a6);

//extern std::vector<vector3d> s_debug_hemisphere_pos;

extern nglMesh *s_debug_hemisphere;
extern nglMesh *s_debug_cylinder;
extern nglMesh *s_debug_box;
extern nglMesh *s_debug_disc;

struct PCUV_ShaderMaterial;

extern PCUV_ShaderMaterial *debug_material;

extern Var<int[34]> debug_render_items;

extern void debug_render_patch();

