#pragma once

// Serialized XBXM inspection only: no engine headers, native pointers, GPU
// calls or mutation. The runtime converter remains in ngl_xbox.cpp.
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace modmesh::xbmeshformat {

inline constexpr std::uint32_t version = 0x1601;

inline bool isSkinnedVertexDef(std::uint32_t hash)
{
    switch (hash) {
    case 0x0A79CDB4: // us_character
    case 0x0C9A7666: // USPersonMorphable_NickFuryEye
    case 0x9B2581FF: // USPerson
    case 0x9EF152BA: // USPersonSolid
    case 0xAC364499: // USPersonMorphable
        return true;
    default:
        return false;
    }
}

namespace detail {

struct Image {
    const std::uint8_t *bytes;
    std::size_t size;

    bool range(std::uint32_t offset, std::uint64_t count) const
    {
        return offset <= size && count <= size - offset;
    }

    bool array(std::uint32_t offset, std::uint32_t count,
               std::uint32_t stride) const
    {
        return count == 0 || (offset != 0 &&
            range(offset, std::uint64_t(count) * stride));
    }

    std::uint32_t u32(std::size_t offset) const
    {
        const auto *p = bytes + offset;
        return std::uint32_t(p[0]) | (std::uint32_t(p[1]) << 8) |
               (std::uint32_t(p[2]) << 16) | (std::uint32_t(p[3]) << 24);
    }

    bool material(std::uint32_t offset, std::uint32_t stored_size) const
    {
        if (!range(offset, 8)) return false;
        // us_character v1 is 0x48 bytes, not the later 0x50-byte USPerson
        // layout. It is legal as the final directory object, with no eight
        // padding bytes after it. The runtime converter allocates its larger
        // replacement separately; the structural gate must not reject it.
        if (u32(std::size_t(offset) + 4) == 0x0A79CDB4u)
            return stored_size >= 0x48 && range(offset, 0x48);
        if (!range(offset, 0x50)) return false;
        // The converter writes runtime texture-name pointers at these known
        // shader offsets; their following slots hold the bound textures.
        // Unknown shader layouts still require the full common material.
        std::uint32_t minimum = 0x50;
        switch (u32(std::size_t(offset) + 4)) {
        case 0x100DE499: case 0x2561BB40: case 0x287B09F3:
        case 0x530520E7: case 0x8F463565: case 0xA3342E9F:
        case 0xDDB856F7: case 0xE50807FC: case 0xFA4ABAD3:
        case 0xFC097C8A:
            minimum = 0x68;
            break;
        case 0xD6E6B9E2: case 0xD98097F0:
            minimum = 0x70;
            break;
        case 0x9B076DEB: case 0xA3C2A47A:
            minimum = 0x74;
            break;
        case 0x42317C08: case 0xE7E31E4F:
            minimum = 0x78;
            break;
        }
        return range(offset, minimum);
    }

    bool section(std::uint32_t offset) const
    {
        if (offset == 0 || !range(offset, 0x60)) return false;
        const std::size_t at = offset;
        const auto bones = u32(at + 0x08);
        const auto boneArray = u32(at + 0x0C);
        const auto primitive = u32(at + 0x28);
        const auto indices = u32(at + 0x2C);
        const auto indexArray = u32(at + 0x30);
        const auto vertices = u32(at + 0x40);
        const auto vertexArray = u32(at + 0x44);
        const auto vertexBytes = u32(at + 0x48);
        const auto stride = u32(at + 0x50);
        const auto vertexDef = u32(at + 0x5C);
        // These fields are signed in the loader. Reject negative values before
        // their conversion to allocation sizes or loop bounds.
        if (bones > 0x7FFFFFFFu || indices > 0x7FFFFFFFu ||
            vertices > 0x7FFFFFFFu || stride > 0x7FFFFFFFu) return false;
        if (!array(boneArray, bones, 2) || !array(indexArray, indices, 2) ||
            !array(vertexArray, vertexBytes, 1)) return false;
        if (vertices != 0 && (stride == 0 ||
            std::uint64_t(vertices) * stride > vertexBytes)) return false;
        // No second-stream conversion exists in the runtime loader.
        if (u32(at + 0x38) != 0 || u32(at + 0x3C) != 0) return false;
        if (vertexDef != 0 && !range(vertexDef, 8)) return false;
        if (primitive != 1 && primitive != 2 && primitive != 4 &&
            primitive != 5 && primitive != 6 && primitive != 7 &&
            primitive != 8) return false;

        const auto elements = indices != 0 ? indices : vertices;
        if (primitive == 8 && (elements == 0 || elements % 4 != 0 ||
            std::uint64_t(elements / 4) * 6 > 0x7FFFFFFFu)) return false;
        // The existing quad converter reads a non-null index view even when
        // its count is zero and uses the vertex count in that case.
        if (primitive == 8 && indexArray != 0 && indices == 0 &&
            !array(indexArray, vertices, 2)) return false;
        if (indices != 0 || (primitive == 8 && indexArray != 0)) {
            const auto n = indices != 0 ? indices : vertices;
            for (std::uint32_t i = 0; i < n; ++i) {
                const auto p = std::size_t(indexArray) + std::size_t(i) * 2;
                const auto index = std::uint32_t(bytes[p]) |
                                   (std::uint32_t(bytes[p + 1]) << 8);
                if (index >= vertices) return false;
            }
        }
        if (stride == 0x20 && vertexDef != 0 &&
            isSkinnedVertexDef(u32(vertexDef))) {
            if (std::uint64_t(vertices) * 0x20 != vertexBytes ||
                std::uint64_t(vertices) * 0x40 > 0xFFFFFFFFu) return false;
            if (bones > 20 && (bones > 127 ||
                (primitive != 5 && primitive != 6 && primitive != 7))) return false;
            for (std::uint32_t i = 0; i < vertices; ++i) {
                const auto p = std::size_t(vertexArray) + std::size_t(i) * 0x20;
                for (unsigned channel = 0; channel < 4; ++channel) {
                    if (bytes[p + 28 + channel] == 0) continue;
                    const auto bone = bytes[p + 24 + channel];
                    if (bone >= 128 || bone >= bones) return false;
                }
            }
        }
        return true;
    }

    bool mesh(std::uint32_t offset) const
    {
        if (!range(offset, 0x40)) return false;
        const std::size_t at = offset;
        const auto sections = u32(at + 8);
        const auto sectionArray = u32(at + 0x0C);
        const auto bones = u32(at + 0x10);
        const auto lods = u32(at + 0x18);
        if (bones > 0x7FFFFFFFu || lods > 0x7FFFFFFFu ||
            !array(sectionArray, sections, 8) ||
            !array(u32(at + 0x14), bones, 0x30) ||
            !array(u32(at + 0x1C), lods, 8)) return false;
        for (std::uint32_t i = 0; i < sections; ++i) {
            if (!section(u32(std::size_t(sectionArray) + std::size_t(i) * 8 + 4)))
                return false;
        }
        return true;
    }

    bool morph(std::uint32_t offset) const
    {
        if (!range(offset, 0x14)) return false;
        const auto sections = u32(std::size_t(offset) + 4);
        const auto sectionArray = u32(std::size_t(offset) + 8);
        // nglProcessMorph writes the marker at sectionArray even for zero
        // sections. Its section/batch containers are 12/136 bytes.
        if (sections > 0x7FFFFFFFu || sectionArray == 0 ||
            !range(sectionArray, 4) || !array(sectionArray, sections, 12)) return false;
        for (std::uint32_t i = 0; i < sections; ++i) {
            const auto entry = std::size_t(sectionArray) + std::size_t(i) * 12;
            const auto batches = u32(entry + 4);
            const auto batchArray = u32(entry + 8);
            if (!array(batchArray, batches, 136)) return false;
            for (std::uint32_t j = 0; j < batches; ++j) {
                const auto batch = std::size_t(batchArray) + std::size_t(j) * 136;
                for (unsigned stream = 0; stream < 8; ++stream) {
                    for (unsigned field = 0; field < 4; ++field) {
                        const auto pointer = u32(batch + 8 + stream * 16 + field * 4);
                        if (pointer != 0 && !range(pointer, 1)) return false;
                    }
                }
            }
        }
        return true;
    }
};

} // namespace detail

// Structural gate for a pristine, self-contained file image. Names in XBXM
// are hashes, not PCMESH string offsets. Do not impose PC string validation,
// directory order or undocumented padding on Xbox assets. This checks the
// containers the converter traverses; it does not claim every shader/vertex
// definition or morph payload is supported by the PC renderer.
inline bool imageUsable(const std::uint8_t *bytes, std::size_t size,
                        const char **reason = nullptr)
{
    if (reason) *reason = nullptr;
    auto reject = [&](const char *why) {
        if (reason) *reason = why;
        return false;
    };
    if (bytes == nullptr || size < 0x14 || size > 0xFFFFFFFFu)
        return reject("truncated or oversized XBXM header");
    const detail::Image image{bytes, size};
    if (std::memcmp(bytes, "XBXM", 4) != 0 || image.u32(4) != version)
        return reject("unsupported XBXM tag or version");
    if (image.u32(16) != 0)
        return reject("XBXM image has already been rebased");
    const auto count = image.u32(8);
    const auto directory = image.u32(12);
    if (count == 0 || directory < 0x14 || !image.array(directory, count, 12))
        return reject("invalid XBXM directory");
    for (std::uint32_t i = 0; i < count; ++i) {
        const auto at = std::size_t(directory) + std::size_t(i) * 12;
        const auto type = bytes[at + 3];
        const auto object = image.u32(at + 4);
        const auto storedSize = std::uint32_t(bytes[at]) |
            (std::uint32_t(bytes[at + 1]) << 8) | (std::uint32_t(bytes[at + 2]) << 16);
        if (object < 0x14 || !image.range(object, storedSize))
            return reject("XBXM directory object is outside the image");
        if (type == 1) {
            if (!image.material(object, storedSize)) return reject("truncated XBXM material");
        } else if (type == 2) {
            if (!image.mesh(object)) return reject("invalid XBXM mesh or section");
        } else if (type == 3) {
            if (!image.morph(object)) return reject("invalid XBXM morph containers");
        } else {
            return reject("unsupported XBXM directory entry");
        }
    }
    return true;
}

} // namespace modmesh::xbmeshformat
