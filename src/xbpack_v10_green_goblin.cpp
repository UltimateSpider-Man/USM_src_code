#include "xbpack.h"

#if defined(OPENUSM_XBPACK_MODE) && defined(OPENUSM_XBPACK_V10) && !defined(TARGET_XBOX)

#include <cstddef>

#include "actor.h"
#include "als_animation_logic_system.h"
#include "als_inode.h"
#include "base_ai_core.h"
#include "base_state.h"
#include "info_node.h"
#include "log.h"
#include "mashed_state.h"
#include "mission_manager.h"
#include "mission_manager_script_data.h"
#include "oldmath_po.h"
#include "physical_interface.h"
#include "xbpack_v10_green_goblin.h"

#include <array>
#include <cmath>
#include <cstring>

namespace
{
static_assert(sizeof(void *) == 4, "The S07 GG hooks require the PC x86 ABI");
namespace gg = xbpack::v10_gg;
static_assert(offsetof(physical_interface, m_gravity_multiplier) == 0xD0, "PC gravity layout mismatch");
static_assert(offsetof(physical_interface, field_74) == 0x74, "PC gravity vector layout mismatch");

struct tracked_flight
{
    actor *owner = nullptr;
    ai::ai_core *core = nullptr;
    gg::flight_guard guard;
    gg::frame_stamp last_advance;
    float restore_gravity = 1.0f;
};

// Bounded, no allocations per frame; stale owner addresses are never
// dereferenced. Native GG inode activation clears the corresponding entries
// before an actor/core address can be reused by a restart or a new spawn.
std::array<tracked_flight, 32> flights {};

struct launch_capture
{
    ai::base_state *state;
    gg::trajectory plan = gg::make_trajectory(gg::pc_height, gg::pc_duration, 30.0f, false);
    bool seen = false;
    float pursuit_height = 9.0f;
    bool height_repaired = false;
};
thread_local launch_capture *capture = nullptr;

bool in_s07()
{
    const auto *manager = mission_manager::s_inst;
    return manager != nullptr && manager->m_script != nullptr &&
        gg::mission_matches(manager->m_script->field_0.guts);
}

tracked_flight *find_flight(actor *owner, ai::ai_core *core)
{
    for (auto &entry : flights)
        if (entry.owner == owner && entry.core == core)
            return &entry;
    return nullptr;
}

tracked_flight *get_flight(actor *owner, ai::ai_core *core)
{
    if (auto *entry = find_flight(owner, core))
        return entry;
    for (auto &entry : flights) {
        if (entry.owner == nullptr || !entry.guard.armed) {
            entry = {};
            entry.owner = owner;
            entry.core = core;
            return &entry;
        }
    }
    // Do not evict an airborne actor and reset its altitude/lifetime budget.
    static bool warned = false;
    if (!warned) {
        warned = true;
        sp_log("[xbpack] V10 GG flight registry full; extra actor not guarded");
    }
    return nullptr;
}

void forget_flight(actor *owner, ai::ai_core *core)
{
    for (auto &entry : flights)
        if ((owner != nullptr && entry.owner == owner) ||
            (core != nullptr && entry.core == core))
            entry = {};
}

bool native_owns_jump(ai::ai_core *core)
{
    if (core == nullptr)
        return false;
    auto *node = static_cast<ai::als_inode *>(core->get_info_node(ai::als_inode::default_id, false));
    if (node == nullptr || node->get_system() == nullptr)
        return false;
    // PC animation_logic_system +0x74 is its active motion compensator.
    // Type 0x1FF (Xbox 0x1E9) integrates gravity and displacement itself in
    // 0x4A7CC0. Its physical velocity is an output, not the launch source.
    const std::uintptr_t *motion = nullptr;
    std::memcpy(&motion, reinterpret_cast<const std::uint8_t *>(node->get_system()) + 0x74,
                sizeof(motion));
    return motion != nullptr && *motion == gg::ballistic_mocomp_vtable;
}

bool release_native_flight(actor *owner, ai::ai_core *core)
{
    if (!native_owns_jump(core))
        return false;
    // Do not carry a competing flight age, cached impulse or gravity restore
    // into the controller's next jump/landing transition. Native deactivation
    // restores its own physics flags and the AI state restores its gravity.
    forget_flight(owner, core);
    return true;
}

float parameter(const ai::base_state *state, const char *name)
{
    if (state == nullptr || state->my_mashed_state == nullptr)
        return gg::missing_number();
    const auto *array = state->my_mashed_state->field_0.param_array;
    if (array == nullptr)
        return gg::missing_number();
    // Native common_find_data is read-only, but its declaration is not const.
    const auto *value = const_cast<ai::param_block::param_data_array *>(array)->common_find_data(
        string_hash {static_cast<int>(to_hash(name))});
    if (value == nullptr)
        return gg::missing_number();
    std::uint32_t bits = 0;
    std::memcpy(&bits, &value->m_union, sizeof(bits));
    return gg::numeric_parameter(static_cast<int>(value->my_type), bits);
}

gg::trajectory plan_for(const ai::base_state *state, float distance)
{
    const auto *type = gg::find_class(state->m_vtbl);
    if (type == nullptr || !type->fire)
        return gg::make_trajectory(gg::pc_height, gg::pc_duration, distance, false, g_gravity);
    const float d0 = parameter(state, "fire_jump_dist_0");
    const float d1 = parameter(state, "fire_jump_dist_1");
    const gg::curve height {d0, parameter(state, "fire_jump_height_0"),
                           d1, parameter(state, "fire_jump_height_1")};
    const gg::curve duration {d0, parameter(state, "fire_jump_time_0"),
                             d1, parameter(state, "fire_jump_time_1")};
    const float super_value = parameter(state, "fire_jump_is_super");
    return gg::make_trajectory(height, duration, distance,
                               gg::finite(super_value) && super_value != 0.0f, g_gravity);
}

void record_plan(ai::base_state *state, gg::trajectory plan)
{
    if (capture != nullptr && capture->state == state) {
        capture->plan = plan;
        capture->seen = true;
    }
}

// The PC ABI returns these primitive floats in x87 ST(0), with this in ECX
// and one four-byte stack argument. No C++ struct-return ABI is involved.
float __fastcall jump_height(ai::base_state *state, void *, float distance)
{
    const auto *type = gg::find_class(state->m_vtbl);
    if (!in_s07() && type != nullptr) {
        using native_fn = float (__fastcall *)(ai::base_state *, void *, float);
        return reinterpret_cast<native_fn>(type->height)(state, nullptr, distance);
    }
    const auto plan = plan_for(state, distance);
    record_plan(state, plan);
    return plan.height;
}

float __fastcall jump_duration(ai::base_state *state, void *, float distance)
{
    const auto *type = gg::find_class(state->m_vtbl);
    if (!in_s07() && type != nullptr) {
        using native_fn = float (__fastcall *)(ai::base_state *, void *, float);
        return reinterpret_cast<native_fn>(type->duration)(state, nullptr, distance);
    }
    const auto plan = plan_for(state, distance);
    record_plan(state, plan);
    return plan.duration;
}

float multiplier(float acceleration)
{
    const float world_g = gg::normal_gravity(g_gravity);
    return acceleration / world_g;
}

physical_interface *physics_for(actor *owner)
{
    if (owner == nullptr || !owner->has_physical_ifc())
        return nullptr;
    auto *phys = owner->physical_ifc();
    // Never fight a ragdoll/death or prop-physics controller.
    if (phys == nullptr || phys->is_biped_physics_running() || phys->is_prop_physics_running())
        return nullptr;
    return phys;
}

void preserve_flight_gravity(tracked_flight &entry, physical_interface *phys)
{
    phys->m_gravity_multiplier = multiplier(entry.guard.gravity);
    // ALS motion compensators can own displacement and publish their velocity
    // through this interface while native physics is disabled. Preserve that
    // ownership: enabling physics here integrates the same jump a second time.
}

void advance_tracked_flight(actor *owner, ai::ai_core *core, Float dt);

void set_vertical_speed(tracked_flight &entry, physical_interface *phys, float velocity_y)
{
    auto velocity = phys->get_velocity();
    if (!gg::finite(velocity.x)) velocity.x = 0.0f;
    if (!gg::finite(velocity.z)) velocity.z = 0.0f;
    velocity.y = velocity_y;
    // Preserve the native horizontal route unless the velocity is corrupt
    // or exceeds the physical_interface::set_velocity assertion limit.
    const double horizontal2 = double(velocity.x) * velocity.x + double(velocity.z) * velocity.z;
    const double allowed2 = 480.0 * 480.0 - double(velocity.y) * velocity.y;
    if (horizontal2 > allowed2) {
        const double scale = std::sqrt(allowed2 / horizontal2);
        velocity.x = static_cast<float>(velocity.x * scale);
        velocity.z = static_cast<float>(velocity.z * scale);
    }
    // Absolute correction, NOT a second gravity integration or an impulse.
    phys->set_velocity(velocity, false);
    preserve_flight_gravity(entry, phys);
}

ai::info_node *jump_node(ai::ai_core *core)
{
    return core == nullptr ? nullptr : core->get_info_node(
        string_hash {static_cast<int>(to_hash("ai_std_jump_inode"))}, false);
}

void update_cached_speed(ai::base_state *state, const vector3d &cached)
{
    if (state == nullptr || gg::find_class(state->m_vtbl) == nullptr)
        return;
    const double speed2 = double(cached.x) * cached.x + double(cached.y) * cached.y +
                          double(cached.z) * cached.z;
    const float speed = static_cast<float>(std::sqrt(speed2));
    if (gg::finite(speed))
        std::memcpy(reinterpret_cast<std::uint8_t *>(state) + 0x5C, &speed, sizeof(speed));
    // NEVER write +0x5C in pursuit: that is a range endpoint, not a speed.
}

gg::trajectory capture_native_takeoff(ai::base_state *state, ai::ai_core *core,
                                      gg::trajectory plan)
{
    if (auto *node = jump_node(core)) {
        auto *bytes = reinterpret_cast<std::uint8_t *>(node);
        vector3d cached;
        std::memcpy(&cached, bytes + 0x20, sizeof(cached));
        if (gg::finite(cached.y) && cached.y > 0.0f && cached.y <= gg::upward_speed_limit)
            return gg::with_native_takeoff(plan, cached.y);
        // Do not let an invalid cached takeoff poison the physical position
        // before the first airborne observation can run.
        cached.y = plan.vertical_speed;
        if (!gg::finite(cached.x)) cached.x = 0.0f;
        if (!gg::finite(cached.z)) cached.z = 0.0f;
        std::memcpy(bytes + 0x20, &cached, sizeof(cached));
        update_cached_speed(state, cached);
        plan.repaired = true;
    }
    return plan;
}

bool touching_down(const tracked_flight &entry, physical_interface *phys)
{
    return gg::landing_contact(entry.guard, phys->get_velocity().y,
        entry.owner->get_abs_position().y - entry.guard.last_y,
        entry.guard.last_step, phys->is_effectively_standing());
}

void enforce_cached_launch(ai::base_state *state, tracked_flight &entry)
{
    if (!entry.guard.armed || !entry.guard.airborne || entry.guard.ground_time > 0.0f)
        return;
    // ALS has already copied the launch vector. A later changed copy is an
    // additional impulse in 0x4A4010, so aging it cannot repair this flight.
    if (release_native_flight(entry.owner, entry.core))
        return;
    auto *phys = physics_for(entry.owner);
    if (phys == nullptr || touching_down(entry, phys))
        return;
    const float vy = phys->get_velocity().y;
    const float corrected = gg::bounded_vertical_speed(entry.guard, vy);
    if (!gg::finite(vy) || corrected != vy)
        set_vertical_speed(entry, phys, corrected);

    // ALS can apply the cached launch AFTER either AI callback. Age that
    // source as well, from the first flight frame (not only after timeout).
    // Native gravity already present is not subtracted a second time.
    if (auto *node = jump_node(entry.core)) {
        auto *bytes = reinterpret_cast<std::uint8_t *>(node);
        vector3d cached;
        std::memcpy(&cached, bytes + 0x20, sizeof(cached));
        const float cap = gg::ballistic_ceiling(entry.guard) + gg::velocity_tolerance(entry.guard);
        if (!gg::finite(cached.y) || cached.y > cap) {
            cached.y = gg::ballistic_ceiling(entry.guard);
            std::memcpy(bytes + 0x24, &cached.y, sizeof(cached.y));
            update_cached_speed(state, cached);
        }
    }
}

void prepare_tracked_flight(actor *owner, ai::ai_core *core, ai::base_state *state)
{
    if (!in_s07())
        return;
    if (release_native_flight(owner, core))
        return;
    auto *entry = find_flight(owner, core);
    auto *phys = physics_for(owner);
    if (entry == nullptr || phys == nullptr || !entry->guard.airborne ||
        entry->guard.ground_time > 0.0f || touching_down(*entry, phys))
        return;
    // Do not reinstate falling velocity before the post-callback observer
    // can recognize a just-completed native collision/landing.
    preserve_flight_gravity(*entry, phys);
    enforce_cached_launch(state, *entry);
}

void __fastcall setup_jump(ai::base_state *state, void *, void *machine)
{
    using native_fn = void (__fastcall *)(ai::base_state *, void *, void *);
    const auto native = reinterpret_cast<native_fn>(gg::jump_setup);
    auto *owner = state->get_actor();
    auto *core = state->get_core();
    auto *phys = in_s07() ? physics_for(owner) : nullptr;
    if (phys == nullptr || core == nullptr) {
        native(state, nullptr, machine);
        return;
    }
    release_native_flight(owner, core);
    const float before = phys->m_gravity_multiplier;
    launch_capture current {state};
    struct capture_scope {
        launch_capture *previous;
        explicit capture_scope(launch_capture *value) : previous(capture) { capture = value; }
        ~capture_scope() { capture = previous; }
    } scope {&current};
    native(state, nullptr, machine);
    // Native combat uses a baked 1/9.8 factor. Match the selected trajectory
    // even when the world acceleration differs from the original default.
    if (current.seen)
        phys->m_gravity_multiplier = multiplier(current.plan.gravity);
    auto *entry = get_flight(owner, core);
    if (entry == nullptr || !current.seen)
        return;
    if (!entry->guard.armed)
        entry->restore_gravity = gg::finite(before) && before > 0.0f ? before : 1.0f;
    if (!entry->guard.airborne)
        current.plan = capture_native_takeoff(state, core, current.plan);
    gg::begin_flight(entry->guard, owner->get_abs_position().y, current.plan);
    phys->m_gravity_multiplier = multiplier(entry->guard.gravity);
    if (current.plan.repaired) {
        static unsigned reports = 0;
        if (reports++ < 16)
            sp_log("[xbpack] V10 GG v4 authored combat: actor=%p height=%g time=%g vy=%g",
                   static_cast<void *>(owner), current.plan.height,
                   current.plan.duration, current.plan.vertical_speed);
    }
    enforce_cached_launch(state, *entry);
}

// Pursuit height: two four-byte stack arguments; x87 float result, ret 8.
float __fastcall pursuit_height(ai::base_state *state, void *, float distance,
                                float target_distance)
{
    using native_fn = float (__fastcall *)(ai::base_state *, void *, float, float);
    const float raw = reinterpret_cast<native_fn>(gg::chase_height)(
        state, nullptr, distance, target_distance);
    if (!in_s07())
        return raw;
    const float fixed = gg::safe_chase_height(raw, distance, target_distance);
    if (capture != nullptr && capture->state == state) {
        capture->pursuit_height = fixed;
        capture->height_repaired = fixed != raw;
    }
    return fixed;
}

// Pursuit +0x58 is a gravity MULTIPLIER, not a flight duration. Do not use
// combat's height/duration callbacks (different slots and argument counts).
float __fastcall pursuit_gravity(ai::base_state *state, void *, float distance)
{
    using native_fn = float (__fastcall *)(ai::base_state *, void *, float);
    const float native_multiplier = reinterpret_cast<native_fn>(gg::chase_gravity)(
        state, nullptr, distance);
    if (!in_s07())
        return native_multiplier;
    const bool captured = capture != nullptr && capture->state == state;
    auto plan = gg::make_chase_trajectory(captured ? capture->pursuit_height : 9.0f,
                                          native_multiplier, g_gravity);
    plan.repaired = plan.repaired || (captured && capture->height_repaired);
    record_plan(state, plan);
    // 0x6EAA00 writes this to physical_interface +0xD0 BEFORE computing the
    // cached velocity in ai_std_jump_inode +0x20. Thus takeoff and gravity
    // use the same repaired parabola; no post-launch impulse is fabricated.
    return multiplier(plan.gravity);
}

void __fastcall setup_pursuit(ai::base_state *state, void *, void *machine)
{
    using native_fn = void (__fastcall *)(ai::base_state *, void *, void *);
    const auto native = reinterpret_cast<native_fn>(gg::chase_setup);
    auto *owner = state->get_actor();
    auto *core = state->get_core();
    auto *phys = in_s07() ? physics_for(owner) : nullptr;
    if (phys == nullptr || core == nullptr) {
        native(state, nullptr, machine);
        return;
    }
    release_native_flight(owner, core);
    const float before = phys->m_gravity_multiplier;
    launch_capture current {state};
    struct capture_scope {
        launch_capture *previous;
        explicit capture_scope(launch_capture *value) : previous(capture) { capture = value; }
        ~capture_scope() { capture = previous; }
    } scope {&current};
    native(state, nullptr, machine);
    // Native setup has already used pursuit_gravity for both takeoff and
    // physical gravity. Do not overwrite its authored multiplier with 1x.
    if (!current.seen)
        return;
    auto *entry = get_flight(owner, core);
    if (entry == nullptr)
        return;
    if (!entry->guard.armed)
        entry->restore_gravity = gg::finite(before) && before > 0.001f
            ? before : gg::normal_gravity_multiplier;
    if (!entry->guard.airborne)
        current.plan = capture_native_takeoff(state, core, current.plan);
    gg::begin_flight(entry->guard, owner->get_abs_position().y, current.plan);
    phys->m_gravity_multiplier = multiplier(entry->guard.gravity);
    static unsigned reports = 0;
    if (reports++ < 16)
        sp_log("[xbpack] V10 GG v4 authored pursuit: actor=%p height=%g time=%g gravity=%g repaired=%d",
               static_cast<void *>(owner), current.plan.height, current.plan.duration,
               multiplier(current.plan.gravity), int(current.plan.repaired));
    enforce_cached_launch(state, *entry);
}

ai::state_trans_messages __fastcall jump_frame_message(ai::base_state *state, void *, Float dt)
{
    using native_fn = ai::state_trans_messages (__fastcall *)(ai::base_state *, void *, Float);
    const auto *type = gg::find_class(state->m_vtbl);
    const auto address = state->m_vtbl == gg::chase_vtable ? gg::chase_frame
        : (type != nullptr ? type->frame : 0);
    if (address == 0)
        return ai::TRANS_TOTAL_MSGS;
    if (gg::finite(static_cast<float>(dt)) && static_cast<float>(dt) > 0.0f)
        prepare_tracked_flight(state->get_actor(), state->get_core(), state);
    const auto result = reinterpret_cast<native_fn>(address)(state, nullptr, dt);
    // The active state is a second observation point, so the guard also
    // runs when GG's inode update is skipped. The simulation-frame stamp
    // below prevents both callbacks from charging the same dt twice.
    advance_tracked_flight(state->get_actor(), state->get_core(), dt);
    if (in_s07() && gg::finite(static_cast<float>(dt)) && static_cast<float>(dt) > 0.0f)
        if (auto *entry = find_flight(state->get_actor(), state->get_core()))
            enforce_cached_launch(state, *entry);
    return result;
}

void __fastcall deactivate_jump(ai::base_state *state, void *, const ai::mashed_state *next)
{
    if (in_s07())
        release_native_flight(state->get_actor(), state->get_core());
    auto *entry = in_s07() ? find_flight(state->get_actor(), state->get_core()) : nullptr;
    using native_fn = void (__fastcall *)(ai::base_state *, void *, const ai::mashed_state *);
    const auto address = state->m_vtbl == gg::chase_vtable
        ? gg::chase_deactivate : gg::jump_deactivate;
    reinterpret_cast<native_fn>(address)(state, nullptr, next);
    if (entry == nullptr)
        return;
    if (auto *phys = physics_for(entry->owner)) {
        if (entry->guard.armed && entry->guard.airborne)
            phys->m_gravity_multiplier = multiplier(entry->guard.gravity);
        else
            phys->m_gravity_multiplier = entry->restore_gravity;
    }
    if (!entry->guard.armed)
        *entry = {};
    // Deliberately retain airborne age across animation/state transitions.
}

void __fastcall gg_activate(ai::info_node *node, void *, ai::ai_core *core)
{
    using native_fn = void (__fastcall *)(ai::info_node *, void *, ai::ai_core *);
    reinterpret_cast<native_fn>(gg::inode_activate)(node, nullptr, core);
    forget_flight(node->get_actor(), core);
}

void advance_tracked_flight(actor *owner, ai::ai_core *core, Float dt)
{
    auto *entry = find_flight(owner, core);
    if (entry == nullptr)
        return;
    auto *phys = physics_for(owner);
    if (!in_s07() || phys == nullptr) {
        if (phys != nullptr && entry->guard.armed)
            phys->m_gravity_multiplier = entry->restore_gravity;
        forget_flight(owner, core);
        return;
    }
    if (release_native_flight(owner, core))
        return;
    if (!gg::finite(static_cast<float>(dt)) || static_cast<float>(dt) <= 0.0f)
        return;
    gg::correction action;
    const auto frame = *reinterpret_cast<const std::uint32_t *>(gg::simulation_frame);
    if (gg::claim_frame(entry->last_advance, frame, static_cast<float>(dt)))
        action = gg::advance_flight(entry->guard,
            {owner->get_abs_position().y, phys->get_velocity().y,
             static_cast<float>(dt), phys->is_effectively_standing()});
    if (action.landed) {
        phys->m_gravity_multiplier = entry->restore_gravity;
        return;
    }
    if (entry->guard.airborne && entry->guard.ground_time == 0.0f)
        preserve_flight_gravity(*entry, phys);
    if (action.started_recovery)
        sp_log("[xbpack] V10 GG v4 ballistic correction: actor=%p rise=%g age=%g gravity=%g",
               static_cast<void *>(owner), owner->get_abs_position().y - entry->guard.launch_y,
               entry->guard.elapsed, phys->m_gravity_multiplier);
    enforce_cached_launch(nullptr, *entry);
}

void __fastcall gg_frame(ai::info_node *node, void *, Float dt)
{
    using native_fn = void (__fastcall *)(ai::info_node *, void *, Float);
    if (gg::finite(static_cast<float>(dt)) && static_cast<float>(dt) > 0.0f)
        prepare_tracked_flight(node->get_actor(), node->get_core(), nullptr);
    reinterpret_cast<native_fn>(gg::inode_frame)(node, nullptr, dt);
    advance_tracked_flight(node->get_actor(), node->get_core(), dt);
}

void __fastcall gg_change_mocomp(als::animation_logic_system *system, void *, Float dt)
{
    system->frame_advance_change_mocomp(dt);
}

}

bool xbpack_v10_green_goblin_keep_mocomp(actor *owner, std::uintptr_t current_vtable,
                                       std::uint32_t requested_xbox_type,
                                       std::uint16_t state_flags)
{
    if (!gg::keep_ballistic_mocomp(current_vtable, requested_xbox_type, state_flags) ||
        !in_s07() || owner == nullptr)
        return false;
    auto *core = owner->get_ai_core();
    if (core == nullptr || core->field_60 == nullptr)
        return false;
    for (const auto *node : *core->field_60)
        if (node != nullptr && node->m_vtbl == gg::inode_vtable && node->get_actor() == owner)
            return true;
    return false;
}

bool xbpack_v10_green_goblin_patch()
{
    namespace gg = xbpack::v10_gg;
    const auto pointer = [](auto callback) {
        return reinterpret_cast<std::uintptr_t>(callback);
    };
    const auto patches = gg::make_slot_patches({
        pointer(&gg_frame), pointer(&gg_activate),
        pointer(&deactivate_jump), pointer(&jump_frame_message), pointer(&setup_jump),
        pointer(&jump_height), pointer(&jump_duration),
        pointer(&deactivate_jump), pointer(&jump_frame_message), pointer(&setup_pursuit),
        pointer(&pursuit_height), pointer(&pursuit_gravity), pointer(&gg_change_mocomp)});
    std::uintptr_t failed = 0;
    const bool ok = gg::apply_slots(patches.data(), patches.size(),
        [](std::uintptr_t slot) { return *reinterpret_cast<const std::uintptr_t *>(slot); },
        [](std::uintptr_t slot, std::uintptr_t value) {
            *reinterpret_cast<std::uintptr_t *>(slot) = value;
        }, failed);
    if (!ok) {
        sp_log("[xbpack] V10 GG jump guard rejected: incompatible callback at 0x%08X",
               static_cast<unsigned>(failed));
        return false;
    }
    sp_log("[xbpack] V10 GG jump guard v5 installed (S07 authored arcs; native ALS motion ownership; 28 checked slots)");
    return true;
}

#endif
