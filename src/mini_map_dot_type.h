#pragma once

struct mini_map_dot_type {
    int field_0;

    operator int() const {
        return field_0;
    }
};

// openusm: dedicated dot ids for the enemy-reticle pass
// (entity_tracker_manager / SHOW_ENEMY_HEALTH_WIDGETS). Stock ids stop at 19,
// and the red marker id 3 is shared with token_def::show_dot (0x5C9CE0), so
// enemies get their own ids to keep the minimap blip SFX enemy-only.
// Only the reimplemented fe_mini_map_dot ctor understands these (red tint,
// boss highlight ring); the stock ctor would fall back to grey/no-ring, but
// these ids are never spawned through the stock path.
inline constexpr int kEnemyDotType = 20;
inline constexpr int kBossDotType  = 21;
