#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <initializer_list>

// The V08 captive is an older, otherwise valid model. Its us_character v1
// shader/vertex-definition name is absent from the retail PC banks. Do not
// reinterpret its 0x48-byte material as the later 0x50-byte USPerson record.
namespace xbpack_v10_captive_material {

inline constexpr std::uint32_t file_hash = 0x43366C2Fu; // ultimate_spidey_v08
inline constexpr std::uint32_t legacy_shader = 0x0A79CDB4u; // us_character
inline constexpr std::uint32_t person_shader = 0x9B2581FFu; // USPerson
inline constexpr std::size_t legacy_size = 0x48;
inline constexpr std::size_t person_size = 0x50;

inline bool enabled_for(std::uint32_t hash)
{
#if defined(OPENUSM_XBPACK_MODE) && defined(OPENUSM_XBPACK_V10)
    return hash == file_hash;
#else
    (void)hash;
    return false;
#endif
}

inline std::uint32_t shader_for(std::uint32_t resource_hash,
                                std::uint32_t original_hash)
{
    return enabled_for(resource_hash) && original_hash == legacy_shader
        ? person_shader : original_hash;
}

struct person_material {
    std::uint32_t words[person_size / sizeof(std::uint32_t)];
};
static_assert(sizeof(person_material) == person_size);

inline bool upgrade(std::uint32_t resource_hash, const void *source,
                    std::size_t source_size, person_material &destination)
{
    if (!enabled_for(resource_hash) || source == nullptr ||
        source_size != legacy_size) return false;

    std::uint32_t original[legacy_size / sizeof(std::uint32_t)]{};
    std::memcpy(original, source, sizeof(original));
    if (original[1] != legacy_shader || original[4] != 1) return false;

    // Only the three observed V08 material profiles are supported. Their v1
    // extension fields are not a v2 colour/feature block; reject other legacy
    // profiles instead of applying this compatibility material to them.
    const auto name = original[0];
    if (name != 0x06C84289u && name != 0x142D51EEu && name != 0x0001BDA3u)
        return false;
    for (const auto index : {2u, 3u, 5u, 7u, 9u, 10u, 11u, 12u, 13u,
                            15u, 16u}) {
        if (original[index] != 0) return false;
    }
    if (original[14] != (name == 0x142D51EEu ? 1u : 0u) || original[17] != 1u)
        return false;

    person_material result{};
    result.words[0] = original[0];
    result.words[1] = person_shader;
    result.words[4] = 2;
    result.words[6] = original[6]; // Original captive diffuse texture hash.
    result.words[8] = original[8]; // Original captive secondary texture hash.

    // The neutral opaque USPerson v2 surface is present in the same Xbox
    // amalgapak (material 6907D24A at absolute offset 074068D0). It uses
    // white colour, ordinary lighting and outline, with no added reflection
    // or mask effect. Those v2 defaults are needed because v1 has no v2
    // colour/outline/blend block. Texture names and geometry remain original.
    for (unsigned i = 10; i != 14; ++i) result.words[i] = 0x3F800000u;
    result.words[17] = 1; // Native lighting feature.
    result.words[18] = 1; // Native outline feature.
    result.words[19] = 0; // NGLBM_OPAQUE.
    destination = result;
    return true;
}

} // namespace xbpack_v10_captive_material
