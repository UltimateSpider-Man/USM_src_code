#include "conglomerate_interface.h"

#include "common.h"

#include <cassert>

VALIDATE_SIZE(conglomerate_interface, 0xC);

conglomerate_interface::conglomerate_interface(conglomerate *a2) : generic_interface()
{
    this->my_conglomerate = a2;

    assert(this->my_conglomerate != nullptr);
    this->dynamic = true;
}

const char * conglomerate_interface::get_ifc_type_str() const
{
    return "conglomerate";
}
