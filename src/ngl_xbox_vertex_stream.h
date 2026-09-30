#pragma once

// Static Xbox streams whose packed vectors do not match the PC declarations.
// Layouts are checked against default.xbe and the PC renderer, not inferred
// from stride alone: several unrelated shaders have valid 24-byte vertices.
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>

namespace ngl_xbox_vertex_stream {

enum class kind { unchanged, outline, shiny };

struct plan {
    kind type = kind::unchanged;
    std::uint32_t source_stride = 0;
    std::uint32_t pc_stride = 0;
    std::uint32_t records = 0;
    std::uint32_t source_bytes = 0;
    std::uint32_t pc_bytes = 0;
    std::uint32_t pc_offset = 0;
};

inline bool make_plan(std::uint32_t vertex_def_hash,
                      std::int32_t source_stride,
                      std::int32_t vertex_count,
                      std::uint32_t source_bytes,
                      std::int32_t source_offset,
                      plan &result)
{
    result = {};
    plan candidate;
    if (vertex_def_hash == 0x530520E7u && source_stride == 24) {
        // US_Outline: position, UV, packed normal -> position, UV, float3.
        candidate.type = kind::outline;
        candidate.source_stride = 24;
        candidate.pc_stride = 32;
    } else if ((vertex_def_hash == 0x9B076DEBu ||
                vertex_def_hash == 0x56F12DDFu) && source_stride == 36) {
        // SMShiny / USShinyInterior: packed N/B/T -> float3 N/B/T.
        candidate.type = kind::shiny;
        candidate.source_stride = 36;
        candidate.pc_stride = 60;
    } else {
        // Includes already expanded PC-sized streams and all other shaders.
        return true;
    }

    if (vertex_count < 0 || source_offset < 0 ||
        source_bytes % candidate.source_stride != 0 ||
        std::uint32_t(source_offset) % candidate.source_stride != 0)
        return false;

    const auto first = std::uint32_t(source_offset) / candidate.source_stride;
    candidate.records = source_bytes / candidate.source_stride;
    if (first > candidate.records ||
        std::uint32_t(vertex_count) > candidate.records - first)
        return false;

    const auto bytes = std::uint64_t(candidate.records) * candidate.pc_stride;
    const auto offset = std::uint64_t(first) * candidate.pc_stride;
    if (bytes > std::numeric_limits<std::uint32_t>::max() ||
        offset > std::uint64_t(std::numeric_limits<std::int32_t>::max()))
        return false;

    candidate.source_bytes = source_bytes;
    candidate.pc_bytes = static_cast<std::uint32_t>(bytes);
    candidate.pc_offset = static_cast<std::uint32_t>(offset);
    result = candidate;
    return true;
}

inline float signed_normal(std::uint32_t value, unsigned bits)
{
    const auto sign = 1u << (bits - 1u);
    const auto signed_value = static_cast<std::int32_t>(value & (sign - 1u)) -
        static_cast<std::int32_t>(value & sign);
    const float decoded = static_cast<float>(signed_value) /
        static_cast<float>(sign - 1u);
    return decoded < -1.0f ? -1.0f : decoded;
}

inline void expand_normal(const unsigned char *source, unsigned char *output)
{
    static_assert(sizeof(float) == 4, "PC vertex components must be 32-bit");
    std::uint32_t packed;
    std::memcpy(&packed, source, sizeof(packed));
    const float normal[3] {
        signed_normal(packed & 0x7FFu, 11),
        signed_normal((packed >> 11u) & 0x7FFu, 11),
        signed_normal((packed >> 22u) & 0x3FFu, 10)
    };
    std::memcpy(output, normal, sizeof(normal));
}

inline bool convert(const plan &layout, const void *source,
                    std::size_t source_size, void *destination,
                    std::size_t destination_size)
{
    if (layout.type == kind::unchanged ||
        source_size < layout.source_bytes || destination_size < layout.pc_bytes ||
        (layout.records != 0 && (source == nullptr || destination == nullptr)))
        return false;

    const auto *input = static_cast<const unsigned char *>(source);
    auto *output = static_cast<unsigned char *>(destination);
    for (std::uint32_t i = 0; i < layout.records; ++i) {
        if (layout.type == kind::outline) {
            std::memcpy(output, input, 20); // Preserve position and UV exactly.
            expand_normal(input + 20, output + 20);
        } else {
            std::memcpy(output, input, 12); // Position.
            expand_normal(input + 12, output + 12);
            expand_normal(input + 16, output + 24);
            expand_normal(input + 20, output + 36);
            std::memcpy(output + 48, input + 24, 12); // UV and colour.
        }
        input += layout.source_stride;
        output += layout.pc_stride;
    }
    return true;
}

} // namespace ngl_xbox_vertex_stream
