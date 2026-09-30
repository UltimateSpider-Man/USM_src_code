#include "gab_database_resource_handler.h"

#include "common.h"
#include "gab_manager.h"
#include "mash_config.h"
#include "mash_info_struct.h"
#include "resource_directory.h"
#include "worldly_pack_slot.h"
#include "trace.h"

VALIDATE_SIZE(gab_database_resource_handler, 0x14);

gab_database_resource_handler::gab_database_resource_handler(worldly_pack_slot *a2)
{
    this->m_vtbl = 0x00888AF4;
    this->my_slot = a2;
    this->field_10 = RESOURCE_KEY_TYPE_GAB_DATABASE;
}

bool gab_database_resource_handler::_handle_resource(worldly_resource_handler::eBehavior a2,
                                                    resource_location *a3)
{
    TRACE("gab_database_resource_handler::handle_resource");

    assert(my_slot->get_resource_directory().get_resource_count(RESOURCE_KEY_TYPE_GAB_DATABASE) ==
           1);

    auto *resource = this->my_slot->get_resource_directory().get_resource(a3, nullptr);
    assert(resource != nullptr);

    if (a2 == worldly_resource_handler::UNLOAD) {
        gab_database *the_gab_database = CAST(the_gab_database, resource);

//        assert(the_gab_database == gab_manager::get_gab_database());
        assert(the_gab_database != nullptr);

        gab_manager::set_gab_database(nullptr);
#if !OPENUSM_XBOX_MASH_FORMAT
        // The retail-PC destructor recursively frees vectors owned by the PC
        // GAB layout.  Xbox GAB vectors point into the pack's mash buffers and
        // use a different layout, so the slot teardown owns their lifetime.
        the_gab_database->destruct_mashed_class();
#endif
    } else {

#if OPENUSM_XBOX_MASH_FORMAT
        mash_info_struct info_struct {mash::UNMASH_MODE, resource, a3->m_size, true};
#else
        mash_info_struct info_struct{resource, a3->m_size};
#endif

        gab_database *the_gab_database = nullptr;
        info_struct.unmash_class(the_gab_database, nullptr
#if OPENUSM_XBOX_MASH_FORMAT
            , mash::NORMAL_BUFFER
#endif 
                );

        mash_info_struct::construct_class(the_gab_database);
        gab_manager::set_gab_database(the_gab_database);
    }

    ++this->field_C;
    return false;
}

void gab_database_resource_handler_patch()
{
    FUNC_ADDRESS(address, &gab_database_resource_handler::_handle_resource);
    set_vfunc(0x00888B00, address);
}
