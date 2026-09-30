#pragma once

#include "entity.h"
#include "entity_base_vhandle.h"
#include "vector3d.h"

struct physical_interface;

struct guidance_system {
    int m_vtbl;
    physical_interface *owner;
    float field_8;
    vector3d field_C;
    int field_18;
    bool field_1C;
};

struct rocket_guidance_sys : guidance_system {
    vhandle_type<entity> field_20;
    vector3d field_24;
    vector3d field_30;
    float field_3C;
    float field_40;
    float field_44;
    float field_48;
    float field_4C;
    float field_50;
    float field_54;
    float field_58;
    float field_5C;
    float field_60;
    char field_64[0x28u];
    vector3d field_8C;
    vector3d field_98;
    int field_A4;
};
