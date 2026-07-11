#pragma once

#include "entity_base_vhandle.h"
#include "slot_pool.h"

#include <cstdint>
#include <set.hpp>

struct entity_tracker;
struct vector3d;

struct entity_tracker_manager {
    _std::set<entity_base_vhandle> field_0;
    slot_pool<entity_tracker *, unsigned int> tracker_slot_pool;

    //0x00629E30
    entity_tracker *id_to_ptr(uint32_t a2);

    //0x0062EE10
    bool get_the_arrow_target_pos(vector3d *);

    void place_poi_reticles();

    // openusm: per-frame POI markers over enemies/bosses (SHOW_ENEMY_HEALTH_WIDGETS).
    void place_enemy_poi_reticles();
};
