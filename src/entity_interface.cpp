#include "entity_interface.h"

void entity_interface::un_mash(
        generic_mash_header *,
        void *a3,
        void *,
        generic_mash_data_ptrs *)
{
    this->field_4 = static_cast<entity *>(a3);
    this->field_8 = false;
}

const char * entity_interface::get_ifc_type_str() const
{
    return "entity";
}
