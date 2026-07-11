#include "entity_tracker_manager.h"

#include "entity_tracker.h"

#include "func_wrapper.h"
#include "game.h"   // SHOW_ENEMY_HEALTH_WIDGETS

#if SHOW_ENEMY_HEALTH_WIDGETS
#include "actor.h"
#include "entity.h"
#include "geometry_manager.h"      // get_xform, XFORM_WORLD_TO_SCREEN
#include "matrix4x4.h"             // sub_501B20 (world->screen project)
#include "mstring.h"               // mString
#include "ngl.h"                   // nglQuad immediate-mode draw
#include "os_developer_options.h"  // runtime SHOW_ENEMY_HEALTH_WIDGETS flag
#include "vector3d.h"
#include "wds.h"                   // g_world_ptr, world_dynamics_system::field_23C

#include <cmath>
#endif

entity_tracker *entity_tracker_manager::id_to_ptr(uint32_t a2) {
    slot_pool<entity_tracker*, unsigned int>::slot_t *v2;
    entity_tracker *result = nullptr;

    if (a2 != 0 && (v2 = &this->tracker_slot_pool.slots[a2 & this->tracker_slot_pool.field_0], a2 == v2->id)) {
        result = v2->field_4;
    }

    return result;
}

bool entity_tracker_manager::get_the_arrow_target_pos(vector3d *a2)
{
    return (bool) THISCALL(0x0062EE10, this, a2);
}

#if SHOW_ENEMY_HEALTH_WIDGETS
// Marker appearance. Colours are 0xAARRGGBB (what nglSetQuadColor takes).
static constexpr float    kEnemyRadius = 9.0f;
static constexpr float    kBossRadius  = 14.0f;
static constexpr uint32_t kEnemyColor  = 0xFFFF8000u;   // opaque orange
static constexpr uint32_t kBossColor   = 0xFFFF2020u;   // opaque red

// TODO(openusm): no clean boss predicate is exposed yet. Bosses (Rhino & co.)
// are driven by the HG_BOSS_* / threat_assessment HUD; flag them here once the
// boss actor type-hash (or a boss controller flag) is wired up. Until then
// every enemy uses the regular orange marker.
static bool is_boss_actor(actor * /*act*/)
{
    return false;
}

// Filled disk built from 16 single-triangle wedges (vertices 0 & 3 are the
// centre so the quad's second triangle is degenerate), drawn directly into the
// ngl quad list at a screen-space centre. Same idiom the swing-debug overlay
// uses, so it's known-safe inside the in-scene pass.
static void draw_poi_disk(float cx, float cy, float radius, uint32_t color)
{
    static constexpr int   wedges = 16;
    static constexpr float two_pi = 6.28318530717958647692f;
    static constexpr float quad_z = 0.5f;

    float prev_x = cx + radius;
    float prev_y = cy;
    for (int i = 1; i <= wedges; ++i)
    {
        const float t      = (static_cast<float>(i) / static_cast<float>(wedges)) * two_pi;
        const float next_x = cx + radius * std::cos(t);
        const float next_y = cy + radius * std::sin(t);

        nglQuad q;
        nglInitQuad(&q);
        nglSetQuadColor(&q, color);
        nglSetQuadBlend(&q, static_cast<nglBlendModeType>(2), 0);
        nglSetQuadZ(&q, quad_z);
        nglSetQuadVPos(&q, 0, cx,     cy);       // centre
        nglSetQuadVPos(&q, 1, prev_x, prev_y);   // last perimeter point
        nglSetQuadVPos(&q, 2, next_x, next_y);   // next perimeter point
        nglSetQuadVPos(&q, 3, cx,     cy);       // centre (degenerate)
        nglListAddQuad(&q);

        prev_x = next_x;
        prev_y = next_y;
    }
}
#endif

void entity_tracker_manager::place_poi_reticles()
{
    // Stock pass: reticles for the script-tracked objective entities.
    THISCALL(0x0062EEB0, this);

#if SHOW_ENEMY_HEALTH_WIDGETS
    this->place_enemy_poi_reticles();
#endif
}

#if SHOW_ENEMY_HEALTH_WIDGETS
void entity_tracker_manager::place_enemy_poi_reticles()
{
    // Runtime gate, toggleable from the developer-options menu without a rebuild
    // (same shape as the spider_monkey debug overlays). SHOW_ENEMY_HEALTH_WIDGETS
    // is a stock dev-options flag, so the name resolves to a real flag_names slot.
    auto *opts = os_developer_options::instance;
    if (opts == nullptr || !opts->get_flag(mString{"SHOW_ENEMY_HEALTH_WIDGETS"})) {
        return;
    }

    if (g_world_ptr == nullptr) {
        return;
    }

    // Concatenated world->view + view->screen transform; one matrix-vector
    // multiply per marker.
    const auto &xform_world_to_screen =
        geometry_manager::get_xform(geometry_manager::XFORM_WORLD_TO_SCREEN);

    for (entity *e : g_world_ptr->field_23C)
    {
        if (e == nullptr) {
            continue;
        }

        // Living, on-screen, AI-controlled, non-hero actors == enemies / bosses.
        if (!e->is_an_actor() || e->is_hero() || !e->is_alive() || !e->is_visible()) {
            continue;
        }

        auto *act = static_cast<actor *>(e);
        if (act->get_player_controller() == nullptr) {
            continue;   // drop non-combatants (pedestrians, props, ...)
        }

        const vector3d wp = e->get_visual_center();
        if (!std::isfinite(wp[0]) || !std::isfinite(wp[1]) || !std::isfinite(wp[2])) {
            continue;
        }

        // World -> screen. screen[2] is post-projection depth; the "> 0" form
        // rejects both behind-the-near-plane points and NaN (NaN fails every
        // ordering compare), which is what kept the swing overlay off bad coords.
        const vector3d screen = sub_501B20(xform_world_to_screen, wp);
        if (!(screen[2] > 0.0f)) {
            continue;
        }
        if (!std::isfinite(screen[0]) || !std::isfinite(screen[1])) {
            continue;
        }

        const float cx = screen[0];
        const float cy = screen[1];

        // 640x480 reference viewport + generous margin (ngl's quad scratch
        // buffer asserts on out-of-range vertex positions in debug builds).
        if (cx < -128.0f || cx > 768.0f || cy < -128.0f || cy > 608.0f) {
            continue;
        }

        const bool boss = is_boss_actor(act);
        draw_poi_disk(cx, cy,
                      boss ? kBossRadius : kEnemyRadius,
                      boss ? kBossColor  : kEnemyColor);
    }
}
#endif