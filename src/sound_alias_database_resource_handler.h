#pragma once

#include "base_engine_resource_handler.h"

struct resource_location;

struct sound_alias_database_resource_handler : base_engine_resource_handler {
    sound_alias_database_resource_handler(worldly_pack_slot *a2);

    //virtual
    bool _handle(worldly_resource_handler::eBehavior a2, limited_timer *a3);

    //0x00568FC0
    /* virtual */ bool _handle_resource(worldly_resource_handler::eBehavior a2,
                                       resource_location *a3) /* override */;
};

extern void sound_alias_database_resource_handler_patch();
extern void sound_alias_database_resource_handler_xbpack_patch();
