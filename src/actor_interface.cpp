#include "actor_interface.h"

#include "common.h"

#include <cassert>

VALIDATE_SIZE(actor_interface, 0xC);

actor_interface::actor_interface(actor *a2) : generic_interface()
{
    this->my_actor = a2;

    assert(this->my_actor != nullptr);
    this->dynamic = true;
}

void actor_interface::un_mash(
        generic_mash_header *,
        void *a3,
        void *,
        generic_mash_data_ptrs *)
{
    this->my_actor = static_cast<actor *>(a3);
    this->dynamic = false;
}
