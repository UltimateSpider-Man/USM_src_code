#pragma once

#include "base_engine_resource_handler.h"

struct resource_location;

struct base_ai_resource_handler : base_engine_resource_handler {
    base_ai_resource_handler(worldly_pack_slot *a2);

    //virtual
    bool _handle(worldly_resource_handler::eBehavior a2, limited_timer *a3);

    //0x00568A10
    /* virtual */ bool _handle_resource(worldly_resource_handler::eBehavior behavior,
                                       resource_location *a3);
};

extern void base_ai_resource_handler_patch();
extern void base_ai_resource_handler_xbpack_patch();
