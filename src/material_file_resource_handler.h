#pragma once

#include "base_tl_resource_handler.h"

struct material_file_resource_handler : base_tl_resource_handler {
    material_file_resource_handler(worldly_pack_slot *a2);

    //virtual
    bool _handle(worldly_resource_handler::eBehavior a2, limited_timer *a3);

    //0x0056C190
    //virtual
    bool _handle_resource(worldly_resource_handler::eBehavior behavior, tlresource_location *a3);
};

extern void material_file_resource_handler_patch();
extern void material_file_resource_handler_xbpack_patch();
