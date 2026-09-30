#pragma once

#include "base_engine_resource_handler.h"

struct als_resource_handler : base_engine_resource_handler {
    als_resource_handler(worldly_pack_slot *a2);

    //virtual
    bool _handle(worldly_resource_handler::eBehavior a2,
                                          limited_timer *a3);

    //0x00568930
    /* virtual */ bool _handle_resource(worldly_resource_handler::eBehavior a2,
                                       resource_location *a3) /* override */;
};

extern void als_resource_handler_patch();
extern void als_resource_handler_xbpack_patch();
