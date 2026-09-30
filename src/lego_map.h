#pragma once

#include <cstdint>

struct nglMesh;
struct nglMaterialBase;
struct scene_entity;
struct proximity_map;
struct region;

struct lego_map_root_node {
    nglMesh **field_0;
    nglMaterialBase **field_4;
    scene_entity *field_8;
    proximity_map *field_C;
    int field_10;
    int field_14;
    int field_18;
    uint16_t field_1C;

    lego_map_root_node();

    //0x0054E5A0
    void un_mash(char *image, int *a3, region *reg);
};

#ifdef OPENUSM_XBPACK_V10
bool lego_map_xbpack_v10_patch();
#endif
