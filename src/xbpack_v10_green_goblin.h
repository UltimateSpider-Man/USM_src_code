#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>

// Native S07 trajectory compatibility and a bounded airborne guard. Callback
// addresses below are PC virtual addresses from Ultimate_prerelease.c.
namespace xbpack::v10_gg
{
inline constexpr std::uintptr_t inode_vtable = 0x0087CCACu;
inline constexpr std::uintptr_t inode_frame = 0x0070B990u;
inline constexpr std::uintptr_t inode_activate = 0x0070B870u;
inline constexpr std::uintptr_t jump_setup = 0x0070CCE0u;
inline constexpr std::uintptr_t jump_deactivate = 0x006EACD0u;
inline constexpr std::uintptr_t ballistic_mocomp_vtable = 0x00878A00u;
inline constexpr std::uint32_t ballistic_mocomp_xbox_type = 0x1E9u;

inline bool keep_ballistic_mocomp(std::uintptr_t current_vtable,
                                 std::uint32_t requested_xbox_type,
                                 std::uint16_t state_flags)
{
    // Native 0x498DB0 compares the PC controller type (0x1FF) to the raw
    // Xbox state type. Creation translates 0x1E9 -> 0x1FF, but the comparison
    // does not. Respect the native explicit-restart bit even for equal types.
    return current_vtable == ballistic_mocomp_vtable &&
        requested_xbox_type == ballistic_mocomp_xbox_type &&
        (state_flags & 0x200u) == 0;
}

// GG pursuit is NOT a combat subclass. Its +0x50 takes three stack
// arguments (range), +0x54 takes two (height), +0x58 takes one (gravity).
// Never install the combat height/time callbacks into these slots.
inline constexpr std::uintptr_t chase_vtable = 0x0087A3D8u;
inline constexpr std::uintptr_t chase_setup = 0x006EAA00u;
inline constexpr std::uintptr_t chase_frame = 0x006B1FE0u;
inline constexpr std::uintptr_t chase_deactivate = 0x006EA950u;
inline constexpr std::uintptr_t chase_height = 0x0070CCA0u;
inline constexpr std::uintptr_t chase_gravity = 0x0070CA20u;
// game_clock::frame_advance (0x58E2F0) increments this once per simulation
// frame. Both the GG inode and the active jump state may observe a flight.
inline constexpr std::uintptr_t simulation_frame = 0x00965EB8u;
inline constexpr float pc_gg_gravity_multiplier = 4.0f; // Reference only: PC 0x938CE4
inline constexpr float normal_gravity_multiplier = 1.0f;
inline constexpr float default_world_gravity = 9.8f; // PC 0x921E3C

// Validate the world acceleration independently of GG's authored multiplier.
// Never change g_gravity itself or another actor's physics.
inline float normal_gravity(float world_gravity)
{
    return std::isfinite(world_gravity) && world_gravity >= 0.1f && world_gravity <= 200.0f
        ? world_gravity : default_world_gravity;
}

struct jump_class
{
    std::uintptr_t vtable;
    std::uintptr_t frame;
    std::uintptr_t height;
    std::uintptr_t duration;
    bool fire;
};

// These subclasses use the common GG combat ballistic setup. Pursuit is
// installed separately with its own ABI, gravity curve and launch capture.
inline constexpr jump_class jump_classes[] = {
    {0x0087A490u, 0x006EB030u, 0x0070D140u, 0x0070D1E0u, true},
    {0x0087A438u, 0x006EAD80u, 0x0070D0E0u, 0x0070D110u, false},
    {0x0087A4E8u, 0x006EAD80u, 0x0070D0E0u, 0x0070D110u, false},
    {0x0087A540u, 0x006EAD80u, 0x0070D0E0u, 0x0070D110u, false},
};

inline const jump_class *find_class(std::uintptr_t vtable)
{
    for (const auto &entry : jump_classes)
        if (entry.vtable == vtable)
            return &entry;
    return nullptr;
}

inline bool finite(float value) { return std::isfinite(value); }
inline float missing_number() { return std::numeric_limits<float>::quiet_NaN(); }

inline char lower_ascii(char c)
{
    return c >= 'A' && c <= 'Z' ? static_cast<char>(c + ('a' - 'A')) : c;
}

// Exact mission names only. Both pursuit and combat inside S07 are covered;
// unrelated missions, hero moves and normal retail PC builds are not changed.
inline bool mission_matches(const char *name)
{
    if (name == nullptr)
        return false;
    const char *base = name;
    std::size_t length = 0;
    for (; length < 256 && name[length] != '\0'; ++length)
        if (name[length] == '/' || name[length] == '\\')
            base = name + length + 1;
    if (length == 256)
        return false;
    for (const char *candidate : {"s07_gg_vs_spidey", "s07_gg_spidey"}) {
        std::size_t i = 0;
        while (candidate[i] != '\0' && lower_ascii(base[i]) == candidate[i])
            ++i;
        if (candidate[i] != '\0')
            continue;
        if (base[i] == '\0')
            return true;
        for (const char *extension : {".xbsx", ".pcsx"}) {
            std::size_t j = 0;
            while (extension[j] != '\0' && lower_ascii(base[i + j]) == extension[j])
                ++j;
            if (extension[j] == '\0' && base[i + j] == '\0')
                return true;
        }
    }
    return false;
}

// PC AI param_types: float=0, integer=1. Never reinterpret integer bits or
// a string/pointer as a floating point height or duration. Other types fall
// back rather than being dereferenced. Shared parameter blocks are read-only.
inline float numeric_parameter(int type, std::uint32_t bits)
{
    if (type == 0) {
        float value;
        std::memcpy(&value, &bits, sizeof(value));
        return value;
    }
    if (type == 1) {
        std::int32_t value;
        std::memcpy(&value, &bits, sizeof(value));
        return static_cast<float>(value);
    }
    return missing_number();
}

struct curve
{
    float distance0, value0, distance1, value1;
};

// Recovered PC default GG curves at 0x938D44..0x938D60. Guard limits below
// are deliberate compatibility limits; they are NOT measured Xbox tuning.
inline constexpr curve pc_height {5.0f, 5.5f, 15.0f, 6.5f};
inline constexpr curve pc_duration {5.0f, 0.6f, 15.0f, 0.6f};
inline constexpr float normal_height_limit = 24.0f;
inline constexpr float super_height_limit = 40.0f;
inline constexpr float upward_speed_limit = 96.0f;
inline constexpr float downward_speed_limit = 120.0f;

inline float interpolate(curve c, float distance, curve fallback, bool &repaired)
{
    if (!finite(c.distance0) || !finite(c.distance1) ||
        !finite(c.value0) || !finite(c.value1) ||
        c.distance1 <= c.distance0 || c.value0 <= 0.0f || c.value1 <= 0.0f) {
        c = fallback;
        repaired = true;
    }
    if (!finite(distance)) {
        distance = c.distance0;
        repaired = true;
    }
    // Double intermediates avoid overflow in subtraction with finite floats.
    const double t = std::clamp((double(distance) - c.distance0) /
                                (double(c.distance1) - c.distance0), 0.0, 1.0);
    return static_cast<float>((1.0 - t) * c.value0 + t * c.value1);
}

struct trajectory
{
    float height, duration, vertical_speed, gravity;
    bool repaired;
};

inline trajectory trajectory_for_gravity(float raw_height, float height_limit,
                                         float acceleration)
{
    const float g = acceleration;
    const float safe_limit = std::min(height_limit, upward_speed_limit * upward_speed_limit / (2.0f * g));
    const float h = finite(raw_height) && raw_height > 0.0f
        ? std::clamp(raw_height, 0.5f, safe_limit) : std::min(6.0f, safe_limit);
    const float vy = static_cast<float>(std::sqrt(2.0 * double(g) * h));
    return {h, 2.0f * vy / g, vy, g, h != raw_height};
}

inline trajectory make_trajectory(curve height, curve duration,
                                  float distance, bool super_jump,
                                  float world_gravity = default_world_gravity)
{
    bool repaired = false;
    const float raw_h = interpolate(height, distance, pc_height, repaired);
    const float raw_t = interpolate(duration, distance, pc_duration, repaired);
    // Xbox 0x2B98A0 and PC 0x70CCE0 both derive v=4h/T and g=8h/T^2.
    // Replacing their authored T with a 1x-gravity time makes S07 float.
    // Keep valid asset timing; bound corrupt inputs by the existing speed
    // limit and do not permit gravity weaker than the world acceleration.
    const float g_world = normal_gravity(world_gravity);
    const float height_limit = std::min(
        super_jump ? super_height_limit : normal_height_limit,
        upward_speed_limit * upward_speed_limit / (2.0f * g_world));
    const float h = std::clamp(raw_h, 0.5f, height_limit);
    const float minimum_time = 4.0f * h / upward_speed_limit;
    const float maximum_time = static_cast<float>(2.0 * std::sqrt(2.0 * h / g_world));
    const float t = std::clamp(raw_t, minimum_time, maximum_time);
    const float vy = 4.0f * h / t;
    return {h, t, vy, 2.0f * vy / t,
            repaired || h != raw_h || t != raw_t || g_world != world_gravity};
}

// Pursuit keeps the native range/direction and the native height curve
// (normally 6..15, plus 5 when the target is far away). Only invalid/extreme
// values are repaired; these safety bounds are not claimed as Xbox tuning.
inline float safe_chase_height(float raw, float distance, float target_distance)
{
    if (!finite(raw) || raw <= 0.0f) {
        const double d = finite(distance) ? distance : 60.0;
        const double t = std::clamp((d - 30.0) / 90.0, 0.0, 1.0);
        raw = static_cast<float>(6.0 + 9.0 * t);
        if (finite(target_distance) && target_distance > 50.0f)
            raw += 5.0f;
    }
    return std::clamp(raw, 0.5f, super_height_limit);
}

inline trajectory make_chase_trajectory(float height, float gravity_multiplier,
                                        float world_gravity)
{
    // Xbox 0x2B9540 interpolates grav_0/grav_1. The S07 pack authors 7x
    // at target distance 10 and 2x at 50; neither is normal world gravity.
    const float raw_multiplier = finite(gravity_multiplier) && gravity_multiplier > 0.0f
        ? gravity_multiplier : pc_gg_gravity_multiplier;
    const float g_world = normal_gravity(world_gravity);
    const float g = static_cast<float>(std::clamp(
        double(raw_multiplier) * g_world, double(g_world),
        double(upward_speed_limit) * upward_speed_limit));
    auto plan = trajectory_for_gravity(safe_chase_height(height, 60.0f, 0.0f),
                                       super_height_limit, g);
    plan.repaired = plan.repaired || plan.height != height ||
        raw_multiplier != gravity_multiplier || g != raw_multiplier * g_world ||
        g_world != world_gravity;
    return plan;
}

// Pursuit may aim at a higher roof. Its native apex is target.y + height,
// not launch.y + height. Use the validated native launch vector to track
// that real arc instead of prematurely clipping a legitimate roof transfer.
inline trajectory with_native_takeoff(trajectory plan, float native_vy)
{
    if (!finite(native_vy) || native_vy <= 0.0f || native_vy > upward_speed_limit)
        return plan;
    plan.vertical_speed = native_vy;
    plan.height = static_cast<float>(double(native_vy) * native_vy / (2.0 * plan.gravity));
    plan.duration = 2.0f * native_vy / plan.gravity;
    return plan;
}

struct frame_stamp
{
    std::uint32_t frame = 0;
    bool valid = false;
};

inline bool claim_frame(frame_stamp &stamp, std::uint32_t frame, float dt)
{
    if (!finite(dt) || dt <= 0.0f || (stamp.valid && stamp.frame == frame))
        return false;
    stamp = {frame, true};
    return true;
}

struct flight_guard
{
    bool armed = false;
    bool airborne = false;
    bool recovering = false;
    bool descending = false;
    float launch_y = 0.0f;
    float elapsed = 0.0f;
    float gravity = default_world_gravity;
    float height_limit = 24.0f;
    float time_limit = 3.0f;
    float initial_speed = 0.0f;
    float last_y = 0.0f;
    float ground_time = 0.0f;
    float last_step = 0.0f;
};

struct observation
{
    float y;
    float vertical_speed;
    float dt;
    bool standing;
};

inline bool landing_contact(const flight_guard &guard, float velocity_y,
                            float displacement_y, float step, bool standing)
{
    return guard.descending && standing && finite(velocity_y) &&
        std::abs(velocity_y) <= 1.0f && finite(displacement_y) &&
        displacement_y <= step * 0.5f + 0.001f;
}

// Absolute ballistic envelope: it is NOT an extra per-frame gravity impulse.
// The native integrator remains responsible for ordinary acceleration.
inline float ballistic_ceiling(const flight_guard &guard)
{
    return static_cast<float>(std::clamp(
        double(guard.initial_speed) - double(guard.gravity) * guard.elapsed,
        -double(downward_speed_limit), double(upward_speed_limit)));
}

inline float velocity_tolerance(const flight_guard &guard)
{
    // The inode and state callbacks can straddle the native physics step.
    // Allow one step of scheduling offset, not a reset of the launch budget.
    return guard.gravity * guard.last_step + 0.125f;
}

inline float bounded_vertical_speed(const flight_guard &guard, float speed)
{
    const float ceiling = ballistic_ceiling(guard);
    if (!finite(speed) || speed > ceiling + velocity_tolerance(guard))
        return ceiling;
    return std::max(speed, -downward_speed_limit);
}

struct correction
{
    bool landed = false;
    bool started_recovery = false;
    bool adjust_velocity = false;
    float vertical_speed = 0.0f;
};

inline void begin_flight(flight_guard &guard, float y, trajectory plan)
{
    // A transition/animation re-entry in midair cannot buy a second takeoff.
    if (guard.armed && guard.airborne)
        return;
    guard = {};
    if (!finite(y))
        return;
    guard.armed = true;
    guard.launch_y = y;
    guard.last_y = y;
    guard.gravity = plan.gravity;
    guard.initial_speed = plan.vertical_speed;
    guard.height_limit = std::max(16.0f, plan.height * 1.5f + 4.0f);
    guard.time_limit = std::max(2.0f, plan.duration * 1.5f + 0.5f);
}

inline correction advance_flight(flight_guard &guard, observation current)
{
    correction out;
    if (!guard.armed || !finite(current.dt) || current.dt <= 0.0f ||
        !finite(current.y))
        return out;
    const bool speed_ok = finite(current.vertical_speed);
    const float rise = current.y - guard.launch_y;
    const float dy = current.y - guard.last_y;
    const float step = std::min(current.dt, 0.25f);
    guard.last_y = current.y;
    guard.last_step = step;
    if (!guard.airborne) {
        // Animation wind-up still does not consume airborne time.
        if ((current.standing || std::abs(rise) <= 0.5f) && speed_ok &&
                std::abs(current.vertical_speed) <= 1.0f)
            return out;
        guard.airborne = true;
    }
    if (speed_ok && current.vertical_speed < -0.25f && dy < 0.0f)
        guard.descending = true;
    // A stale standing flag around the apex is NOT a new takeoff surface.
    // Require observed descent before accepting a stable landing.
    const bool touching_down = landing_contact(guard, current.vertical_speed, dy, step, current.standing);
    const bool stable_contact = touching_down && std::abs(dy) <= step * 0.5f + 0.001f;
    // The first contact frame can include the whole last downward step.
    // Suppress velocity repair immediately, then confirm stable contact.
    guard.ground_time = touching_down
        ? (stable_contact ? guard.ground_time + step : 0.000001f) : 0.0f;
    if (guard.ground_time >= 0.075f && rise <= guard.height_limit) {
        guard = {};
        out.landed = true;
        return out;
    }
    // Do not push a grounded actor down while confirming landing contact.
    if (touching_down)
        return out;
    guard.elapsed += step;
    out.vertical_speed = bounded_vertical_speed(guard, current.vertical_speed);
    out.adjust_velocity = !speed_ok || out.vertical_speed != current.vertical_speed;
    const bool stalled = guard.elapsed > guard.time_limit &&
        (!speed_ok || current.vertical_speed >= -0.25f);
    const bool escaped = out.adjust_velocity || rise > guard.height_limit || stalled;
    if (!guard.recovering && escaped) {
        guard.recovering = true;
        out.started_recovery = true;
    }
    // No forced -8 snap, no v += impulse, and no repeated launch timer reset.
    // If native gravity is missing or ALS replays takeoff, the same continuous
    // v0-g*t envelope removes that extra upward velocity from the first arc.
    return out;
}

// Validate all selected slots before changing any of them. Reader/writer
// callbacks also let the production installer be exercised without a game.
struct slot_patch { std::uintptr_t slot, original, replacement; };

struct hook_callbacks
{
    std::uintptr_t inode_frame, inode_activate;
    std::uintptr_t combat_deactivate, combat_frame, combat_setup;
    std::uintptr_t combat_height, combat_duration;
    std::uintptr_t pursuit_deactivate, pursuit_frame, pursuit_setup;
    std::uintptr_t pursuit_height, pursuit_gravity;
    std::uintptr_t als_change_mocomp;
};

inline std::array<slot_patch, 28> make_slot_patches(const hook_callbacks &hooks)
{
    std::array<slot_patch, 28> patches {};
    std::size_t i = 0;
    patches[i++] = {inode_vtable + 0x1C, inode_frame, hooks.inode_frame};
    patches[i++] = {inode_vtable + 0x20, inode_activate, hooks.inode_activate};
    for (const auto &type : jump_classes) {
        patches[i++] = {type.vtable + 0x1C, jump_deactivate, hooks.combat_deactivate};
        patches[i++] = {type.vtable + 0x20, type.frame, hooks.combat_frame};
        patches[i++] = {type.vtable + 0x40, jump_setup, hooks.combat_setup};
        patches[i++] = {type.vtable + 0x4C, type.height, hooks.combat_height};
        patches[i++] = {type.vtable + 0x50, type.duration, hooks.combat_duration};
    }
    patches[i++] = {chase_vtable + 0x1C, chase_deactivate, hooks.pursuit_deactivate};
    patches[i++] = {chase_vtable + 0x20, chase_frame, hooks.pursuit_frame};
    patches[i++] = {chase_vtable + 0x40, chase_setup, hooks.pursuit_setup};
    patches[i++] = {chase_vtable + 0x54, chase_height, hooks.pursuit_height};
    patches[i++] = {chase_vtable + 0x58, chase_gravity, hooks.pursuit_gravity};
    // XBPACK startup returns before the general ALS installer. Install the
    // comparison hook here, alongside the guarded S07 callbacks it supports.
    patches[i++] = {0x00881494u, 0x00498DB0u, hooks.als_change_mocomp};
    return patches;
}

template<class Read, class Write>
inline bool apply_slots(const slot_patch *patches, std::size_t count,
                        Read read, Write write, std::uintptr_t &failed)
{
    failed = 0;
    for (std::size_t i = 0; i < count; ++i) {
        const auto value = read(patches[i].slot);
        if (value != patches[i].original && value != patches[i].replacement) {
            failed = patches[i].slot;
            return false;
        }
    }
    for (std::size_t i = 0; i < count; ++i)
        write(patches[i].slot, patches[i].replacement);
    return true;
}
}

bool xbpack_v10_green_goblin_patch();

struct actor;
bool xbpack_v10_green_goblin_keep_mocomp(actor *owner, std::uintptr_t current_vtable,
                                       std::uint32_t requested_xbox_type,
                                       std::uint16_t state_flags);
