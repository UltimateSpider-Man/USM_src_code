#pragma once

#include "mash_virtual_base.h"
#include "string_hash.h"

struct anim_key;

struct anim_record : mash_virtual_base {

    anim_key *my_key;
    string_hash field_8;

    anim_record();

    void _unmash(mash_info_struct *a2, void *a3);

    int get_mash_sizeof();
};

struct attach_anim_record : anim_record {
    int field_C;
    string_hash field_10;
    int field_14;
    int field_18;

    void _unmash(mash_info_struct *a2, void *a3);
};

extern void anim_record_patch();
extern void anim_record_v10_patch();
