#pragma once

#include "base_tl_resource_handler.h"

struct morph_file_resource_handler : base_tl_resource_handler {
    morph_file_resource_handler(worldly_pack_slot *a2);

    //0x0056C080
    bool _handle_resource(worldly_resource_handler::eBehavior behavior,
                         tlresource_location *tlres_loc);
};

extern void morph_file_resource_handler_patch();
extern void morph_file_resource_handler_xbpack_patch();
