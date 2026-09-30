#pragma once

#include "string_hash.h"

struct from_mash_in_place_constructor;
struct mash_info_struct;

struct entity_viseme_entry {
    string_hash field_0;
    int field_4;

    entity_viseme_entry() = default;

    entity_viseme_entry(from_mash_in_place_constructor *);

    ~entity_viseme_entry() = default;

    void unmash(mash_info_struct *, void *);

    void destruct_mashed_class();
};
