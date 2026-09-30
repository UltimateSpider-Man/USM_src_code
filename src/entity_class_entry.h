#pragma once

#include "mash.h"
#include "mstring.h"
#include "mvector.h"
#include "string_hash.h"
#include "vector3d.h"

struct entity_viseme_entry;
struct mash_info_struct;

struct entity_class_entry {
    string_hash field_0;
    string_hash field_4;
    mString field_8;
    string_hash field_18;
    string_hash field_1C;
    string_hash field_20;
    bool field_24;
    bool field_25;
    vector3d field_28;
    vector3d field_34;
    int field_40;
    mString field_44;
    mString field_54;
    mString field_64;
    mVector<entity_viseme_entry> field_74;

    entity_class_entry();

    void initialize(mash::allocation_scope a2);

    void unmash(mash_info_struct *a1, void *a3);

    void destruct_mashed_class();
};
