#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>

namespace xbpack::v10_ped
{
inline constexpr std::uint32_t code_begin = 0x006ED000u;
inline constexpr std::size_t code_size = 0x00002000u;

struct instruction_patch
{
    std::uint32_t address;
    std::uint8_t original[7];
    std::uint8_t replacement[7];
    std::uint8_t size;
    const char *purpose;
};

// Port the control flow of the supplied Xbox beta, using PC object offsets
// and native PC callees. Xbox addresses below are evidence, never call targets.
//
// Threat response: PC 0x006ED5A0 / Xbox 0x002D1170.
// Xbox 0x002D12A7 goes directly to the 54/58 response selection. PC inserts
// a flag/probability gate at 0x006ED710 which can select message 52 instead.
// Replacing 52 with TRANS_TOTAL_MSGS (75) leaves the pedestrian waiting.
// Skip the PC-only gate entirely, including its extra random-number draw.
//
// Flee update: PC 0x006EE4A0 / Xbox 0x002BB680.
// After the recovery check, Xbox 0x002BB719 evaluates the threat and repaths.
// PC inserts a getter/message-52 branch at 0x006EE510. Returning 75 from
// that branch skips the threat, failure, and repath results indefinitely.
// Continue at the PC equivalent of Xbox's threat check instead. Preserve the
// existing inactive-state guard, recovery result, and locomotion restoration.
inline constexpr instruction_patch patches[] = {
    {0x006ED710u,
     {0x8Bu, 0x43u, 0x1Cu, 0x8Bu, 0xC8u},
     {0xE9u, 0x33u, 0x00u, 0x00u, 0x00u},
     5u, "beta_threat_response_selection"}, // jmp 0x006ED748
    {0x006EE510u,
     {0x8Bu, 0xCFu, 0xE8u, 0x99u, 0x0Du, 0xFAu, 0xFFu},
     {0xE9u, 0x0Du, 0x00u, 0x00u, 0x00u, 0x90u, 0x90u},
     7u, "beta_flee_threat_and_repath"}, // jmp 0x006EE522
};

inline constexpr std::size_t patch_count = sizeof(patches) / sizeof(patches[0]);

// Launched layers: PC launch_layer_state::frame_advance 0x006AF230 / Xbox
// 0x0027A070.  Xbox machines have no interrupt flag: process_transition
// (Xbox 0x00279AC0) always runs the graph's default transitions unless the
// current state returns action 4.  PC added ai_state_machine +0x34 and moves
// it when a layer launches: a blocking layer takes the parent's flag and the
// parent's is cleared (0x006AF362), a non-blocking layer's is cleared
// (0x006AF344).  Retail ped layer graphs (ped_qp_real, ped_react_real, ...)
// carry their own ped_hit_react and [ped_default_trans]; the v10 layers
// (ped_quad_path_layer, ped_reaction_layer, ped_lane_layer) do not, only the
// base ped graph does, so pedestrians need the beta rule (every machine keeps
// the flag its constructor set).
//
// This used to be a fixed NOP patch for every machine, which also changed
// universal-soldier (Sable mercenary) graphs.  The rule is now chosen per
// launch by the bridge in xbpack_v10_combat.cpp: beta for pedestrian graphs,
// stock PC for all others (openusm.ini [XbpackV10] LayerInterrupts).

// The PC lane-goal state distinguishes a full pedestrian (message 5) from
// a lite pedestrian (SUCCESS, message 1). V10's ped_lane_layer only has
// to_state_on_success/to_state_on_failure, and Xbox 0x002CF734 always returns
// SUCCESS after setting the locomotion goal. Returning 5 leaves the full
// pedestrian in the goal-selection state instead of starting its walk.
inline constexpr std::uint32_t lane_code_begin = 0x0070D000u;
inline constexpr std::size_t lane_code_size = 0x00001000u;
inline constexpr instruction_patch lane_patches[] = {
    {0x0070D7A3u,
     {0x83u, 0xC8u, 0x01u}, // or eax,1 (eax is 0 or 4)
     {0x31u, 0xC0u, 0x40u}, // xor eax,eax; inc eax: beta SUCCESS
     3u, "beta_lane_goal_success"},
};

// Validate every site before writing any. Accept an already installed
// bridge, but do not partially patch an unrecognized executable.
template<std::size_t Count>
inline bool apply(const instruction_patch (&table)[Count],
                  std::uint32_t begin, std::uint8_t *code, std::size_t length,
                  const instruction_patch **failed = nullptr)
{
    if (failed != nullptr)
        *failed = nullptr;

    for (const auto &patch : table)
    {
        const auto offset = std::size_t(patch.address - begin);
        if (code == nullptr || offset > length || patch.size > length - offset ||
            (std::memcmp(code + offset, patch.original, patch.size) != 0 &&
             std::memcmp(code + offset, patch.replacement, patch.size) != 0))
        {
            if (failed != nullptr)
                *failed = &patch;
            return false;
        }
    }

    for (const auto &patch : table)
        std::memcpy(code + (patch.address - begin),
                    patch.replacement, patch.size);
    return true;
}

inline bool apply(std::uint8_t *code, std::size_t length,
                  const instruction_patch **failed = nullptr)
{
    return apply(patches, code_begin, code, length, failed);
}

inline bool apply_lanes(std::uint8_t *code, std::size_t length,
                        const instruction_patch **failed = nullptr)
{
    return apply(lane_patches, lane_code_begin, code, length, failed);
}

}
