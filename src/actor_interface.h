#pragma once

#include "generic_interface.h"

struct actor;
struct generic_mash_header;
struct generic_mash_data_ptrs;

struct actor_interface : generic_interface {
    actor *my_actor;
    bool dynamic;

    actor_interface(actor *a2);

    //virtual
    void un_mash(
        generic_mash_header *a2,
        void *a3,
        void *a4,
        generic_mash_data_ptrs *a5);
};
