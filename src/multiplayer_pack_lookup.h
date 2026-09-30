#pragma once
#include "resource_key.h"
#include "resource_manager.h"
#include "resource_pack_slot.h"
#include "resource_partition.h"
#include "string_hash.h"
#include <string>

namespace usm::mp {
// The story hero is normally in HERO, not in the mission stack. Borrow its
// exact pack/entity pair before attempting another allocation. The caller
// must not mark a borrowed slot as owned or pop it when multiplayer closes.
inline resource_pack_slot* find_loaded_actor_pack_context(const std::string& pack,
                                                          const std::string& entity)
{
    if(!resource_manager::partitions)return nullptr;
    const resource_key pk{string_hash{pack.c_str()},RESOURCE_KEY_TYPE_PACK};
    const resource_key ek{string_hash{entity.c_str()},RESOURCE_KEY_TYPE_ENTITY};
    for(auto* partition:*resource_manager::partitions){
        if(!partition)continue;
        const auto type=partition->get_type();
        // Streamed district/strip packs can disappear during ordinary play.
        // HERO is protected by the pre-unload cleanup hook; COMMON and the
        // mission stack follow the existing world/mission lifetime.
        if(type!=RESOURCE_PARTITION_HERO&&type!=RESOURCE_PARTITION_COMMON
           &&type!=RESOURCE_PARTITION_MISSION)continue;
        for(auto* slot:partition->get_pack_slots()){
            if(!slot||!slot->is_pack_ready()||!(slot->get_name_key()==pk))continue;
            int bytes=0;resource_pack_slot* owner=nullptr;
            if(slot->get_resource(ek,&bytes,&owner)&&bytes>=16&&owner==slot)
                return slot;
        }
    }
    return nullptr;
}
}
