#include "script_lib_beam.h"

#include "entity.h"
#include "entity_handle_manager.h"
#include "mstring.h"
#include "osassert.h"
#include "utility.h"

slc_beam_t *slc_beam = nullptr;

int slc_beam_t::_find_instance(const mString &a1) const
{
    if (a1 == "NULL") {
        return 0;
    }

    auto *ent = (entity *) entity_handle_manager::find_entity(string_hash {a1.c_str()}, IGNORE_FLAVOR, true);
    if (ent == nullptr)
    {
        auto a2a = "beam " + a1;
        auto v8 = a2a + " not found\n";
        auto *v5 = v8.c_str();
        error(v5);
    }

    if ( ent->get_flavor() != entity_flavor_t::BEAM )
    {
        auto a2b = "entity " + a1;
        auto v9 = a2b + " is not a beam\n";
        auto *v6 = v9.c_str();
        error(v6);
    }

    return ent->get_my_vhandle().get_goodies();
}

void script_lib_beam_patch()
{
    FUNC_ADDRESS(address, &slc_beam_t::_find_instance);
    set_vfunc(0x0089AAFC, address);
}
