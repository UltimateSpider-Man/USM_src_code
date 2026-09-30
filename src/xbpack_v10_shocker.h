#pragma once

#include <cstdint>

namespace xbpack::v10_shocker
{
inline constexpr std::uintptr_t frame_slot = 0x0087D6ECu;
inline constexpr std::uintptr_t native_frame = 0x006DD670u;
inline constexpr std::uint32_t die_from_webtie_hash = 0x477C3704u;

// Xbox shocker_inode::frame_advance (default.xbe 0x002C90A0): an enabled
// die_from_webtie and strictly positive subdual set health to -100, then
// clamp to its upper and lower bounds, in that order. On PC these bounded
// variables live at damage_interface +0x21C and +0x1FC (Xbox +0x220/+0x200).
// Leave the bounds and the fourth, unrelated word of the variable intact.
inline bool apply_webtie_defeat(bool enabled, float subdual, float (&health)[4])
{
    if (!enabled || !(subdual > 0.0f)) {
        return false;
    }

    float value = -100.0f;
    if (value > health[2]) {
        value = health[2];
    }
    if (value < health[1]) {
        value = health[1];
    }

    const bool changed = health[0] != value;
    health[0] = value;
    return changed;
}
}

// Installs only the Shocker info-node frame callback in the PC-hosted V10
// loader. Returns false if another/unrecognized callback occupies the slot.
bool xbpack_v10_shocker_patch();
