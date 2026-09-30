#pragma once

#include "generic_interface.h"

struct entity_base_interface : generic_interface {

    //virtual
    const char * get_ifc_type_str() const;
};
