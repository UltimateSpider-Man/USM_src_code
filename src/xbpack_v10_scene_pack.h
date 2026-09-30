#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace xbpack::v10_scene_pack
{
inline constexpr std::uintptr_t load_call = 0x00732D29u;
inline constexpr std::uintptr_t native_load = 0x0053E380u;
inline constexpr std::uintptr_t unload_jump = 0x00732D76u;
inline constexpr std::uintptr_t native_unload = 0x0052ABB0u;
inline constexpr std::uintptr_t destroy_jump = 0x0086DDC5u;
inline constexpr std::uintptr_t native_destroy = 0x00531F90u;
inline constexpr std::size_t native_path_capacity = 256;
inline constexpr std::size_t maximum_directory_prefix = 4u * 1024u * 1024u;

struct bytes {
    const std::uint8_t *data;
    std::size_t size;
};

struct scene_location {
    std::uint32_t hash;
    std::uint32_t offset;
    std::uint32_t size;
};

struct directory_info {
    std::uint32_t directory_offset = 0;
    std::uint32_t directory_size = 0;
    std::vector<scene_location> scenes;
};

inline std::uint32_t u32(const std::uint8_t *p)
{
    return p[0] | (std::uint32_t(p[1]) << 8) |
           (std::uint32_t(p[2]) << 16) | (std::uint32_t(p[3]) << 24);
}

inline bool contains(std::uint64_t total, std::uint64_t offset, std::uint64_t size)
{
    return offset <= total && size <= total - offset;
}

// Validate before the native, unchecked in-place directory parser runs.
// The prefix includes the header and directory read; scene data stays on disk.
// This is the V10 standalone SCNANIMS layout, not an ordinary amalgapak.
inline bool inspect_directory(bytes prefix, std::uint64_t file_size,
                              directory_info &result)
{
    result = {};
    if (prefix.data == nullptr || prefix.size < 0x2C) {
        return false;
    }
    constexpr std::array<std::uint32_t, 5> versions {{10, 486, 265, 487, 246}};
    for (std::size_t i = 0; i < versions.size(); ++i) {
        if (u32(prefix.data + i * 4) != versions[i]) {
            return false;
        }
    }
    const auto dir_offset = u32(prefix.data + 0x18);
    const auto dir_size = u32(prefix.data + 0x1C);
    if (u32(prefix.data + 0x14) != 0 || dir_offset < 0x2C ||
        (dir_offset & 15u) != 0 || dir_size < 0x2C0 ||
        !contains(file_size, dir_offset, dir_size) ||
        !contains(prefix.size, dir_offset, dir_size)) {
        return false;
    }

    const auto *mash = prefix.data + dir_offset;
    const auto mash_extent = u32(mash + 8);
    if (u32(mash + 4) != 0 || u32(mash + 12) != 0xFFFFu ||
        mash_extent < 0x2C0 || mash_extent > dir_size ||
        std::uint64_t(dir_offset) + mash_extent > dir_size ||
        u32(mash) != (((mash_extent + 0x7BADBA5Du + 0xFFFFu) & 0x0FFFFFFFu) | 0x70000000u)) {
        return false;
    }
    const auto *root = mash + 0x10;
    std::size_t cursor = 0x2C0; // mash header + Xbox V10 directory (0x2B0)
    std::size_t locations_offset = 0;
    std::size_t tl_locations_offset = 0;
    std::uint32_t location_count = 0;
    std::uint32_t tl_count = 0;
    constexpr std::array<std::size_t, 15> element_sizes {{
        4, 16, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 8, 12
    }};
    for (std::size_t i = 0; i < element_sizes.size(); ++i) {
        const auto *vector = root + i * 8;
        const auto packed_size = u32(vector + 4);
        const auto count = packed_size & 0xFFFFu;
        if (u32(vector) != 0 || (packed_size & 0xFFFF0000u) != 0x01000000u ||
            (i != 0 && i != 1 && i != 11 && count != 0)) {
            return false;
        }
        if (count == 0) {
            continue;
        }
        cursor = (cursor + 7u) & ~std::size_t(7u);
        const auto length = std::size_t(count) * element_sizes[i];
        if (!contains(mash_extent, cursor, length)) {
            return false;
        }
        if (i == 0) {
            for (std::uint32_t n = 0; n < count; ++n) {
                if (u32(mash + cursor + n * 4) != 0) {
                    return false;
                }
            }
        } else if (i == 1) {
            locations_offset = cursor;
            location_count = count;
        } else if (i == 11) {
            tl_locations_offset = cursor;
            tl_count = count;
        }
        cursor += length;
    }
    if (location_count == 0 || location_count != tl_count) {
        return false;
    }
    for (std::size_t type = 0; type < 68; ++type) {
        const auto count = u32(root + 0x19C + type * 4);
        if (count != (type == 25 ? location_count : 0u) ||
            (type == 25 && u32(root + 0x8C + type * 4) != 0)) {
            return false;
        }
    }

    directory_info checked;
    checked.directory_offset = dir_offset;
    checked.directory_size = dir_size;
    bool first_igc2 = false;
    bool second_igc2 = false;
    for (std::uint32_t i = 0; i < location_count; ++i) {
        const auto *entry = mash + locations_offset + i * 16;
        const auto *tl = mash + tl_locations_offset + i * 12;
        const auto hash = u32(entry);
        const auto raw_offset = u32(entry + 8);
        const auto size = u32(entry + 12);
        const std::uint64_t absolute = std::uint64_t(dir_size) + raw_offset;
        if (u32(entry + 4) != 25 || size < 0x50 ||
            absolute > 0xFFFFFFFFu || !contains(file_size, absolute, size) ||
            (i != 0 && hash <= checked.scenes.back().hash) ||
            u32(tl) != hash || (u32(tl + 4) & 0xFFu) != 10u ||
            (u32(tl + 4) >> 8) != size || u32(tl + 8) != raw_offset) {
            return false;
        }
        checked.scenes.push_back({hash, static_cast<std::uint32_t>(absolute), size});
        first_igc2 |= hash == 0x1B749F06u;
        second_igc2 |= hash == 0x1B749F07u;
    }
    if (!first_igc2 || !second_igc2) {
        return false;
    }
    result = checked;
    return true;
}

inline bool inspect_scene_header(bytes header, const scene_location &location)
{
    if (header.data == nullptr || header.size < 0x50 ||
        u32(header.data) != 0x00010101u || u32(header.data + 0x10) != location.hash) {
        return false;
    }
    const auto skeletons = u32(header.data + 0x0C);
    const auto nodes = u32(header.data + 0x30);
    const auto first_node = u32(header.data + 0x34);
    const auto maximum_block = u32(header.data + 0x38);
    return skeletons != 0 && nodes != 0 &&
           std::uint64_t(skeletons) * 32u == u32(header.data + 8) &&
           contains(location.size, 0x50, std::uint64_t(skeletons) * 32u) &&
           first_node >= 0x50u + std::uint64_t(skeletons) * 32u &&
           contains(location.size, first_node, 16) &&
           maximum_block >= 16 && maximum_block <= location.size;
}

// Native streaming allocates from maximum_block, then follows node links.
// A merely bounded scene header is insufficient: each real node must fit
// that allocation, including the final node whose link is zero.
template<typename ReadNode>
bool inspect_scene_nodes(bytes header, const scene_location &location, ReadNode read_node)
{
    if (!inspect_scene_header(header, location)) return false;
    const auto count = u32(header.data + 0x30);
    const auto maximum_block = u32(header.data + 0x38);
    if (count > location.size / 16u) return false;
    auto position = u32(header.data + 0x34);
    for (std::uint32_t i = 0; i < count; ++i) {
        std::uint8_t node[16] {};
        if ((position & 15u) != 0 || !contains(location.size, position, sizeof(node)) ||
            !read_node(position, node, sizeof(node))) return false;
        const auto next = u32(node);
        const auto span = next != 0 ? next : location.size - position;
        const auto clips = u32(node + 4);
        if ((next != 0) != (i + 1 < count) || span < 16u + 0x40u ||
            span > maximum_block || !contains(location.size, position, span) ||
            clips == 0 || clips > (span - 16u) / 0x40u || u32(node + 8) != 16u) return false;
        position += next;
    }
    return true;
}

// Original user-supplied V10 pack, byte for byte; compiled only into V10.
bytes embedded_pack();
}

bool xbpack_v10_scene_pack_patch();
