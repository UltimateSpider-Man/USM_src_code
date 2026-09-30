#pragma once

#include "base_tl_resource_handler.h"

#include "ngl.h"
#include "worldly_resource_handler.h"

struct texture_resource_handler : base_tl_resource_handler {
    int field_14;

    texture_resource_handler(worldly_pack_slot *a2);

    void handle_resource_internal(tlresource_location *loc, nglTextureFileFormat a3);

    //virtual
    bool _handle(worldly_resource_handler::eBehavior a2, limited_timer *a3);

    //0x00562FB0
    //virtual
    void _pre_handle_resources(worldly_resource_handler::eBehavior a2);

    //0x0056BB00
    //virtual
    bool _handle_resource(worldly_resource_handler::eBehavior a2, tlresource_location *tlres_loc);
};

extern void texture_resource_handler_patch();
extern void texture_resource_handler_xbpack_patch();
