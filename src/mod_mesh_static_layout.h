#pragma once

// Retail PC declaration/material layouts. Use serialized declarations rather
// than guessing from float values: a transparent D3DCOLOR can be a normal
// finite float, and UV coordinates can look like a unit normal.
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>

namespace modmesh { namespace staticlayout {

inline std::string normalized(std::string name)
{
    for (char &c : name) c = char(std::tolower(static_cast<unsigned char>(c)));
    return name;
}

struct VertexLayout {
    int position = 0, normal = -1, uv = -1, color = -1;
};

inline bool vertexLayout(const std::string &declaration, uint32_t stride, VertexLayout &out)
{
    const auto name = normalized(declaration);
    if (stride != 24 || (name != "us_frontend" && name != "usfloor"
        && name != "ussimpleinterior" && name != "ustranslucentinterior"
        && name != "usmsimplemorphable" && name != "us_decal3d")) return false;
    out = {0, -1, 12, 20};
    return true;
}

struct MaterialLayout {
    size_t minimumSize = 0, textureName = 0, texture = 0;
    int color = -1;
};

inline bool materialLayout(const std::string &shader, MaterialLayout &out)
{
    const auto name = normalized(shader);
    if (name == "us_frontend") out = {0x2c, 0x18, 0x1c, -1};
    else if (name == "usfloor") out = {0x3c, 0x1c, 0x20, -1};
    else if (name == "ussimpleinterior") out = {0x40, 0x1c, 0x20, -1};
    else if (name == "ustranslucentinterior") out = {0x44, 0x1c, 0x20, -1};
    else if (name == "usmsimplemorphable") out = {0x88, 0x60, 0x64, -1};
    else if (name == "us_decal3d") out = {0x6c, 0x60, 0x64, -1};
    else if (name == "uspersonsolid") out = {0x58, 0x18, 0x1c, 0x28};
    else if (name == "usperson" || name == "uspersonmorphable")
        out = {0x50, 0x18, 0x1c, 0x28};
    else return false;
    return true;
}

// Accept pointer words explicitly because the serialized PC ABI is 32-bit,
// even when the standalone verifier is built for a 64-bit host.
inline bool retargetMaterial(void *material, size_t size, const MaterialLayout &layout,
                             uint32_t textureName, uint32_t texture,
                             const float *color = nullptr, bool externalAlbedo = false)
{
    if (!material || !layout.minimumSize || size < layout.minimumSize
        || layout.textureName > size - 4 || layout.texture > size - 4
        || (layout.color >= 0 && (size_t(layout.color) > size - 16))) return false;
    auto *bytes = static_cast<uint8_t *>(material);
    std::memcpy(bytes + layout.textureName, &textureName, 4);
    std::memcpy(bytes + layout.texture, &texture, 4);
    if (layout.color >= 0 && color) std::memcpy(bytes + layout.color, color, 16);
    if (externalAlbedo && layout.color == 0x28 && layout.minimumSize >= 0x50) {
        // USPerson's +0x3c/+0x40 select shaders which multiply/add the native
        // sphere map. Custom base colors (images or a solid tint on white)
        // have no matching native sphere map; inherited highlights otherwise
        // wash out the authored colors. This also applies to explicit white=.
        // Keep native lighting (+0x44), outline (+0x48), and blend (+0x4c).
        const uint32_t disabled = 0;
        std::memcpy(bytes + 0x3c, &disabled, 4);
        std::memcpy(bytes + 0x40, &disabled, 4);
    }
    return true;
}

}} // namespace modmesh::staticlayout
