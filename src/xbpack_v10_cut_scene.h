#pragma once

#include <cstdint>

namespace xbpack::v10_cut_scene
{
inline constexpr std::uintptr_t segment_unmash_call = 0x00748CDCu;
inline constexpr std::uintptr_t native_segment_unmash = 0x00742680u;
inline constexpr std::uint32_t xbox_scene_animation_type = 0x19u;
inline constexpr std::uint32_t pc_scene_animation_type = 0x1Au;

// CUT segment::field_20 contains scene-animation keys. The directory has
// already been converted to PC types, but native resource_key::unmash only
// processes the hash. Normalize this typed field once after segment unmash.
// PC keys and other values are left intact, including a second invocation.
inline constexpr std::uint32_t normalize_scene_animation_type(std::uint32_t type)
{
    return type == xbox_scene_animation_type ? pc_scene_animation_type : type;
}
}

bool cut_scene_segment_xbpack_v10_patch();
