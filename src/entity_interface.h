#pragma once

#include "generic_interface.h"

struct entity;
struct generic_mash_header;
struct generic_mash_data_ptrs;

struct entity_interface : generic_interface {
    entity *field_4;
    bool field_8;

    //virtual
    void un_mash(
        generic_mash_header *a2,
        void *a3,
        void *a4,
        generic_mash_data_ptrs *a5);

    //virtual
    const char * get_ifc_type_str() const;
};
