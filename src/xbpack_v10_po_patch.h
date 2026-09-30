#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>

namespace xbpack::v10_po
{
// Xbox source/default/live PO records contain 7 floats and 4 padding bytes.
// The PC decoder's PRIVATE cache still contains 7 floats, with 4-byte
// alignment. Keep that cache format: its producers, apply/skip callbacks,
// frame stride and the generic scratch buffer must agree with each other.
// In particular, do not replace every 0x1C in the PO decoder with 0x20.
inline constexpr std::uint32_t source_pose_size = 0x20u;
inline constexpr std::uint32_t native_cache_pose_size = 0x1Cu;
inline constexpr std::uint32_t code_begin = 0x00780000u;
inline constexpr std::size_t code_size = 0x00020000u;

struct instruction_patch
{
    std::uint32_t address;
    std::uint8_t original[4];
    std::uint8_t replacement[4];
    std::uint8_t size;
    const char *purpose;
};

// Verified against the PE embedded in Ultimate_prerelease.c and default.xbe.
// Source/base PO streams require Xbox 16-byte alignment as well as a 32-byte
// stride. The entropy trajectory scalar header is still aligned to 4; only
// the following base pose aligns to 16. Never change the PC private cache.
// Full instruction
// signatures protect the opcode/register as well as the size immediate.
// Raw copies deliberately remain 7 DWORDs: padding is skipped at the source
// boundary, never inserted into the PC decoder cache.
inline constexpr instruction_patch patches[] = {
    {0x0078802Du, {0x83u, 0xc0u, 0x03u}, {0x83u, 0xc0u, 0x0fu}, 3u, "raw_control_source_alignment"},
    {0x00788030u, {0x83u, 0xe0u, 0xfcu}, {0x83u, 0xe0u, 0xf0u}, 3u, "raw_control_source_alignment"},
    {0x007887AAu, {0x83u, 0xc1u, 0x03u}, {0x83u, 0xc1u, 0x0fu}, 3u, "raw_base_control_base_alignment"},
    {0x007887ADu, {0x83u, 0xe1u, 0xfcu}, {0x83u, 0xe1u, 0xf0u}, 3u, "raw_base_control_base_alignment"},
    {0x007887EEu, {0x83u, 0xc0u, 0x03u}, {0x83u, 0xc0u, 0x0fu}, 3u, "raw_base_control_source_alignment"},
    {0x007887F1u, {0x83u, 0xe0u, 0xfcu}, {0x83u, 0xe0u, 0xf0u}, 3u, "raw_base_control_source_alignment"},
    {0x0078885Cu, {0x83u, 0xc2u, 0x03u}, {0x83u, 0xc2u, 0x0fu}, 3u, "raw_base_decode_base_alignment"},
    {0x0078885Fu, {0x83u, 0xe2u, 0xfcu}, {0x83u, 0xe2u, 0xf0u}, 3u, "raw_base_decode_base_alignment"},
    {0x007889ACu, {0x83u, 0xc2u, 0x03u}, {0x83u, 0xc2u, 0x0fu}, 3u, "raw_base_apply_base_alignment"},
    {0x007889AFu, {0x83u, 0xe2u, 0xfcu}, {0x83u, 0xe2u, 0xf0u}, 3u, "raw_base_apply_base_alignment"},
    {0x00788A9Cu, {0x83u, 0xc0u, 0x03u}, {0x83u, 0xc0u, 0x0fu}, 3u, "raw_base_extract_base_alignment"},
    {0x00788A9Fu, {0x83u, 0xe0u, 0xfcu}, {0x83u, 0xe0u, 0xf0u}, 3u, "raw_base_extract_base_alignment"},
    {0x00788B6Fu, {0x83u, 0xc0u, 0x03u}, {0x83u, 0xc0u, 0x0fu}, 3u, "raw_base_blend_base_alignment"},
    {0x00788B72u, {0x83u, 0xe0u, 0xfcu}, {0x83u, 0xe0u, 0xf0u}, 3u, "raw_base_blend_base_alignment"},
    {0x00788CF0u, {0x83u, 0xc2u, 0x03u}, {0x83u, 0xc2u, 0x0fu}, 3u, "entropy_base_control_base_alignment"},
    {0x00788CF3u, {0x83u, 0xe2u, 0xfcu}, {0x83u, 0xe2u, 0xf0u}, 3u, "entropy_base_control_base_alignment"},
    {0x00788DD5u, {0x83u, 0xc1u, 0x03u}, {0x83u, 0xc1u, 0x0fu}, 3u, "entropy_base_decode_base_alignment"},
    {0x00788DD8u, {0x83u, 0xe1u, 0xfcu}, {0x83u, 0xe1u, 0xf0u}, 3u, "entropy_base_decode_base_alignment"},
    {0x00788FBDu, {0x83u, 0xc0u, 0x03u}, {0x83u, 0xc0u, 0x0fu}, 3u, "entropy_base_apply_base_alignment"},
    {0x00788FC0u, {0x83u, 0xe0u, 0xfcu}, {0x83u, 0xe0u, 0xf0u}, 3u, "entropy_base_apply_base_alignment"},
    {0x007890CDu, {0x83u, 0xc0u, 0x03u}, {0x83u, 0xc0u, 0x0fu}, 3u, "entropy_base_extract_base_alignment"},
    {0x007890D0u, {0x83u, 0xe0u, 0xfcu}, {0x83u, 0xe0u, 0xf0u}, 3u, "entropy_base_extract_base_alignment"},
    {0x007891C5u, {0x83u, 0xc1u, 0x03u}, {0x83u, 0xc1u, 0x0fu}, 3u, "entropy_base_blend_base_alignment"},
    {0x007891C8u, {0x83u, 0xe1u, 0xfcu}, {0x83u, 0xe1u, 0xf0u}, 3u, "entropy_base_blend_base_alignment"},
    {0x00788043u, {0x6bu, 0xc0u, 0x1cu}, {0x6bu, 0xc0u, 0x20u}, 3u, "raw_control_source_frames"},
    {0x007880E8u, {0x83u, 0xc7u, 0x1cu}, {0x83u, 0xc7u, 0x20u}, 3u, "raw_decode_source"},
    {0x00788804u, {0x6bu, 0xc0u, 0x1cu}, {0x6bu, 0xc0u, 0x20u}, 3u, "raw_base_control_source_frames"},
    {0x0078880Eu, {0x83u, 0x03u, 0x1cu}, {0x83u, 0x03u, 0x20u}, 3u, "raw_base_control_base_pose"},
    {0x007888C1u, {0x83u, 0xc7u, 0x1cu}, {0x83u, 0xc7u, 0x20u}, 3u, "raw_base_decode_source"},
    {0x007888DAu, {0x83u, 0x07u, 0x1cu}, {0x83u, 0x07u, 0x20u}, 3u, "raw_base_decode_base_pose"},
    {0x0078896Du, {0x83u, 0x00u, 0x1cu}, {0x83u, 0x00u, 0x20u}, 3u, "raw_base_skip_base_pose"},
    {0x00788A64u, {0x83u, 0x00u, 0x1cu}, {0x83u, 0x00u, 0x20u}, 3u, "raw_base_apply_base_pose"},
    {0x00788B37u, {0x83u, 0x06u, 0x1cu}, {0x83u, 0x06u, 0x20u}, 3u, "raw_base_extract_base_pose"},
    {0x00788C29u, {0x83u, 0x03u, 0x1cu}, {0x83u, 0x03u, 0x20u}, 3u, "raw_base_blend_base_pose"},
    {0x00788CA4u, {0x83u, 0x00u, 0x1cu}, {0x83u, 0x00u, 0x20u}, 3u, "advance_one_base_pose"},
    {0x00788D59u, {0x83u, 0xc0u, 0x1cu}, {0x83u, 0xc0u, 0x20u}, 3u, "entropy_base_control_base_pose"},
    {0x00788ECDu, {0x83u, 0x00u, 0x1cu}, {0x83u, 0x00u, 0x20u}, 3u, "entropy_base_decode_base_pose"},
    {0x00788F5Bu, {0x83u, 0x03u, 0x1cu}, {0x83u, 0x03u, 0x20u}, 3u, "entropy_base_skip_base_pose"},
    {0x0078906Fu, {0x83u, 0x45u, 0x00u, 0x1cu}, {0x83u, 0x45u, 0x00u, 0x20u}, 4u, "entropy_base_apply_base_pose"},
    {0x0078916Cu, {0x83u, 0x07u, 0x1cu}, {0x83u, 0x07u, 0x20u}, 3u, "entropy_base_extract_base_pose"},
    {0x0078927Cu, {0x83u, 0x07u, 0x1cu}, {0x83u, 0x07u, 0x20u}, 3u, "entropy_base_blend_base_pose"},
    {0x0078C4E6u, {0x83u, 0xc6u, 0x1cu}, {0x83u, 0xc6u, 0x20u}, 3u, "blend_pose_arrays_constant_weight"},
    {0x0078C4E9u, {0x83u, 0xc7u, 0x1cu}, {0x83u, 0xc7u, 0x20u}, 3u, "blend_pose_arrays_constant_weight"},
    {0x0078C4ECu, {0x83u, 0xc3u, 0x1cu}, {0x83u, 0xc3u, 0x20u}, 3u, "blend_pose_arrays_constant_weight"},
    {0x0078C551u, {0x83u, 0xc6u, 0x1cu}, {0x83u, 0xc6u, 0x20u}, 3u, "blend_pose_arrays_per_bone_weight"},
    {0x0078C554u, {0x83u, 0xc7u, 0x1cu}, {0x83u, 0xc7u, 0x20u}, 3u, "blend_pose_arrays_per_bone_weight"},
    {0x0078C557u, {0x83u, 0xc3u, 0x1cu}, {0x83u, 0xc3u, 0x20u}, 3u, "blend_pose_arrays_per_bone_weight"},
    {0x00794DFAu, {0x83u, 0xc7u, 0x1cu}, {0x83u, 0xc7u, 0x20u}, 3u, "generic_get_bone_matrices"},
    {0x00796961u, {0x83u, 0xc3u, 0x1cu}, {0x83u, 0xc3u, 0x20u}, 3u, "generic_build_pose_from_matrices"},
};

inline constexpr std::size_t patch_count = sizeof(patches) / sizeof(patches[0]);

// One validation pass before any writes. Accept an already installed patch
// (including the two generic strides installed by older loaders), but reject
// unknown instructions without leaving a partially updated set of PO readers.
inline bool apply(std::uint8_t *code, std::size_t length,
                  const instruction_patch **failed = nullptr)
{
    if (failed != nullptr)
        *failed = nullptr;
    for (const auto &patch : patches)
    {
        const auto offset = std::size_t(patch.address - code_begin);
        if (code == nullptr || offset > length || patch.size > length - offset ||
            (std::memcmp(code + offset, patch.original, patch.size) != 0 &&
             std::memcmp(code + offset, patch.replacement, patch.size) != 0))
        {
            if (failed != nullptr)
                *failed = &patch;
            return false;
        }
    }
    for (const auto &patch : patches)
        std::memcpy(code + (patch.address - code_begin),
                    patch.replacement, patch.size);
    return true;
}
}
