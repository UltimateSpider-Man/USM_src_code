// Standalone regression for S07 authored jump timing and physical trajectories.
// g++ -std=c++17 -Wall -Wextra -Werror -Isrc test/xbpack_v10_green_goblin_probe.cpp -o gg-probe
#include "xbpack_v10_green_goblin.h"

#include <cstdlib>
#include <iostream>

namespace gg = xbpack::v10_gg;

static void require(bool condition, const char *message)
{
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}

static bool close(float a, float b)
{
    return std::abs(a - b) <= 0.0001f * std::max(1.0f, std::abs(b));
}

int main()
{
    gg::hook_callbacks callbacks {};
    callbacks.als_change_mocomp = 0x12345678;
    const auto slots = gg::make_slot_patches(callbacks);
    require(slots.back().slot == 0x881494 && slots.back().original == 0x498DB0 &&
            slots.back().replacement == callbacks.als_change_mocomp,
            "V10 startup does not install the ALS controller comparison hook");
    // A jump-start -> jump-loop animation transition keeps the same native
    // ballistic controller, so its accumulated falling velocity survives.
    require(gg::keep_ballistic_mocomp(0x878A00, 0x1E9, 0),
            "Xbox/PC type mismatch restarts the same jump controller");
    require(!gg::keep_ballistic_mocomp(0x878A00, 0x1E9, 0x200),
            "explicit native controller restart was suppressed");
    require(!gg::keep_ballistic_mocomp(0x878A00, 0x1E8, 0) &&
            !gg::keep_ballistic_mocomp(0x878D80, 0x1E9, 0),
            "a different motion controller transition was suppressed");
    // PK_S07_GG_VS_SPIDEY contains these four distinct authored fire arcs.
    // Expected values follow y(t) = v*t - g*t*t/2, apex h at T/2.
    for (const auto sample : {std::array<float, 2>{6.5f, 0.5f},
                              {7.5f, 0.75f}, {3.5f, 0.5f}, {5.0f, 1.0f},
                              {20.0f, 1.5f}, {20.0f, 2.5f}}) {
        const float h = sample[0], t = sample[1];
        const auto plan = gg::make_trajectory({5,h,15,h}, {5,t,15,t}, 10, h == 20);
        require(close(plan.duration, t), "authored combat flight duration changed");
        require(close(plan.height, h), "authored combat apex changed");
        require(close(plan.vertical_speed, 4*h/t), "combat launch does not reach authored apex");
        require(close(plan.gravity, 8*h/(t*t)), "combat gravity disagrees with authored time");
        require(close(plan.vertical_speed*t - plan.gravity*t*t/2, 0), "combat arc misses landing time");

        // Native gravity must remain sufficient throughout the arc; the
        // airborne guard must not apply gravity for a second time.
        for (const int fps : {30, 60, 120}) {
            gg::flight_guard guard;
            gg::begin_flight(guard, 100, plan);
            for (int frame = 1; frame < static_cast<int>(t*fps); ++frame) {
                const float age = float(frame)/fps;
                const float y = 100 + plan.vertical_speed*age - plan.gravity*age*age/2;
                const auto action = gg::advance_flight(guard,
                    {y, plan.vertical_speed-plan.gravity*age, 1.0f/fps, false});
                require(!action.adjust_velocity, "valid ballistic flight was corrected twice");
            }
        }
    }
    const auto interpolated = gg::make_trajectory({5,6.5f,15,7.5f},
        {5,0.5f,15,0.75f}, 10, false);
    require(close(interpolated.height,7) && close(interpolated.duration,0.625f),
            "authored height/time interpolation changed");
    // S07 pursuit authored grav_0=7 at distance10, grav_1=2 at distance50.
    for (float multiplier : {2.0f, 4.5f, 7.0f}) {
        const auto plan = gg::make_chase_trajectory(15, multiplier, 9.8f);
        require(close(plan.gravity, 9.8f*multiplier), "authored pursuit gravity replaced by 1x");
        require(close(plan.vertical_speed*plan.vertical_speed/(2*plan.gravity),15),
                "pursuit launch and falling gravity disagree");
    }
    const auto corrupt = gg::make_trajectory({0,gg::missing_number(),0,-1},
        {0,gg::missing_number(),0,0}, gg::missing_number(), false);
    require(gg::finite(corrupt.gravity) && corrupt.gravity > 0 &&
            corrupt.vertical_speed <= gg::upward_speed_limit,
            "invalid curve is not safely bounded");
    for (float world : {0.1f, 9.8f, 200.0f}) {
        for (float duration : {0.000001f, 1.5f, 1000000.0f}) {
            const auto plan = gg::make_trajectory({5,40,15,40},
                {5,duration,15,duration}, 10, true, world);
            require(gg::finite(plan.gravity) && gg::finite(plan.vertical_speed) &&
                    plan.gravity >= world * 0.9999f &&
                    plan.vertical_speed <= gg::upward_speed_limit * 1.0001f,
                    "competing speed/gravity limits produce an invalid combat arc");
            require(close(plan.vertical_speed*plan.vertical_speed/(2*plan.gravity), plan.height),
                    "bounded combat arc lost its apex");
        }
    }
    for (float raw : {0.00001f, 0.0f, gg::missing_number(),
                       std::numeric_limits<float>::max()}) {
        const auto plan = gg::make_chase_trajectory(40, raw, 200);
        require(gg::finite(plan.gravity) && plan.gravity >= 200 &&
                gg::finite(plan.vertical_speed) && plan.vertical_speed <= gg::upward_speed_limit,
                "corrupt pursuit multiplier produces float or excessive launch speed");
    }
    gg::flight_guard guard;
    const auto plan = gg::make_chase_trajectory(15, 7, 9.8f);
    // The pursuit wind-up moves the root down by about 0.55 while the actor
    // remains standing. It must not spend airborne time or age the launch.
    gg::begin_flight(guard, 1.925f, plan);
    for (int frame = 0; frame < 20; ++frame) {
        const auto action = gg::advance_flight(guard, {1.376f, 0, 1.0f/30, true});
        require(!guard.airborne && guard.elapsed == 0 && !action.adjust_velocity,
                "grounded animation wind-up consumed flight time");
    }
    gg::advance_flight(guard, {2.5f, plan.vertical_speed, 1.0f/30, false});
    require(guard.airborne && guard.elapsed > 0, "actual takeoff was not observed");
    guard = {};
    gg::begin_flight(guard, 0, plan);
    gg::advance_flight(guard, {1, plan.vertical_speed, 1.0f/60, false});
    const float age = guard.elapsed;
    gg::begin_flight(guard, 1000, plan);
    require(guard.elapsed == age && guard.launch_y == 0, "midair reentry reset the launch budget");
    require(gg::mission_matches("s07_gg_vs_spidey") &&
            gg::mission_matches("S07_GG_VS_SPIDEY.XBSX") &&
            !gg::mission_matches("s03_rhino_rampage_enc") &&
            !gg::mission_matches("s07_gg_vs_spidey_other"), "mission scope changed");
    std::cout << "PASS: S07 controller reuse/install, explicit restarts, wind-up, authored arcs at 30/60/120Hz, invalid inputs and mission scope\n";
}
