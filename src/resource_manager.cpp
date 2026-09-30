#include "resource_manager.h"

#include "binary_search_array_cmp.h"
#include "common.h"
#include "core_ai_resource.h"
#include "cut_scene.h"
#include "cut_scene_segment.h"
#include "base_ai_res_state_graph.h"
#include "entity_base.h"
#include "entity.h"
#include "filespec.h"
#include "func_wrapper.h"
#include "game.h"
#include "limited_timer.h"
#include "log.h"
#include "trace.h"
#include "memory.h"
#include "mash_info_struct.h"
#include "nal_system.h"
#include "mod_nal_overrides.h"
#include "nfl_system.h"
#include "ngl.h"
#include "nlPlatformEnum.h"
#include "os_file.h"
#include "os_developer_options.h"
#include "debug_menu.h"
#ifdef OPENUSM_XBPACK_V10
#include "exe_allocator.h"
#endif
#include "resource_amalgapak_header.h"
#include "resource_directory.h"
#include "return_address.h"
#include "script_object.h"
#include "utility.h"
#include "variables.h"
#include "worldly_pack_slot.h"
#include "xbpack.h"
#include "osassert.h"

#include <algorithm>
#include <new>
#include <vector>
#include <cassert>
#include <cstring>
#include <numeric>
#include <unordered_map>

// ---------------------------------------------------------------------------
// Loose .MSN / .PANEL serialized resource overrides
//
// These two formats are intentionally NOT pre-unmashed here:
//   * MISSION_TABLE (.MSN) is a generic-mash image. mission_manager parses it
//     with parse_generic_object_mash<mission_table_container>(), which sets the
//     generic header in-use bit and rebases its mashable vectors in place.
//   * PANEL is a raw PanelFile mash stream. PanelFile::UnmashPanelFile() builds
//     a mash_info_struct over the bytes, un-mashes PanelFile and then runs its
//     from-mash constructor.
//
// Returning Mod::Data directly would corrupt the pristine loose-file source on
// the first load and make later loads re-unmash already-rebased pointers.  Each
// lookup therefore gets a new 16-byte-aligned writable serialized image.
// ---------------------------------------------------------------------------

namespace {

inline constexpr int MOD_TYPE_MSN_FILE = 0x107;
inline constexpr int MOD_TYPE_PANEL_FILE = 0x108;
inline constexpr int MOD_TYPE_COLL_FILE = 0x109;
inline constexpr int MOD_TYPE_CUT_FILE = 0x10A;
inline constexpr int MOD_TYPE_PCMESHDEF_FILE = 0x10B;
inline constexpr int MOD_TYPE_SLF_FILE = 0x10C;

#pragma pack(push, 1)
struct mod_generic_mash_header_disk {
    uint32_t safety_key;
    uint32_t flags;
    int32_t mash_data_offset;
    uint16_t class_id;
    uint16_t field_E;
};

struct mod_mashable_vector_disk {
    uint32_t data_cookie;
    uint16_t size;
    uint8_t shared;
    uint8_t from_mash;
};

struct mod_msn_root_disk {
    mod_mashable_vector_disk marker_bases;
    mod_mashable_vector_disk camera_markers;
    mod_mashable_vector_disk transform_markers;
    mod_mashable_vector_disk camera_transform_markers;
    mod_mashable_vector_disk nums;
    mod_mashable_vector_disk strings;
    mod_mashable_vector_disk positions;
    mod_mashable_vector_disk conditions;
    int32_t field_40;
    uint32_t region_cookie;
};

struct mod_panel_vector_disk {
    int32_t field_0;
    int32_t size;
    uint32_t data_cookie;
    int32_t capacity;
    uint8_t from_mash;
    uint8_t padding[3];
};

struct mod_panel_root_disk {
    mod_panel_vector_disk pquads;
    mod_panel_vector_disk ptext;
    mod_panel_vector_disk animations;
};
#pragma pack(pop)

static_assert(sizeof(mod_generic_mash_header_disk) == 0x10u,
              "generic mash header disk layout changed");
static_assert(sizeof(mod_mashable_vector_disk) == 0x08u,
              "mashable_vector disk layout changed");
static_assert(sizeof(mod_msn_root_disk) == 0x48u,
              "mission_table_container disk layout changed");
static_assert(sizeof(mod_panel_vector_disk) == 0x14u,
              "PanelFile mVector disk layout changed");
static_assert(sizeof(mod_panel_root_disk) == 0x3Cu,
              "PanelFile disk layout changed");

bool modMsnVectorUsable(const mod_mashable_vector_disk &vec)
{
    if (vec.shared > 1u || vec.from_mash != 1u)
        return false;
    if (vec.size > 0x7FFFu)
        return false;
    return true;
}

bool modPanelVectorUsable(const mod_panel_vector_disk &vec)
{
    if (vec.size < 0 || vec.size > 0x10000)
        return false;
    if (vec.capacity < vec.size || vec.capacity > 0x10000)
        return false;
    if (vec.from_mash != 1u)
        return false;
    if (vec.size > 0 && vec.data_cookie == 0u)
        return false;
    return true;
}

void modEraseTypedBinding(uint32_t hash, int modType)
{
    auto range = Mods.equal_range(hash);
    for (auto it = range.first; it != range.second; )
    {
        if (it->second.Type == modType)
            it = Mods.erase(it);
        else
            ++it;
    }
}

bool modRegisterSerializedResource(const std::filesystem::path &path,
                                   std::vector<uint8_t> &&fileData,
                                   int modType,
                                   const char *label)
{
    const std::string stem = transformToLower(path.stem().string());
    const uint32_t hash = to_hash(stem.c_str());

    uint32_t literal = 0;
    const bool hasLiteral = modParseLiteralHash(stem, &literal) && literal != hash;

    modEraseTypedBinding(hash, modType);
    if (hasLiteral)
        modEraseTypedBinding(literal, modType);

    const unsigned fileSize = static_cast<unsigned>(fileData.size());
    Mods.emplace(hash, Mod{path, modType, std::move(fileData)});

    if (hasLiteral)
    {
        const Mod *registered = getMod(hash, modType);
        if (registered != nullptr)
            Mods.emplace(literal, Mod{registered->Path, modType, registered->Data});
    }

    sp_log("[mod][%s] registered \"%s\" -> \"%s\" (0x%08X, %u bytes)%s",
           label, path.filename().string().c_str(), stem.c_str(), hash, fileSize,
           hasLiteral ? " [literal-hash alias added]" : "");
    return true;
}

uint8_t *modGetFreshSerializedOverride(uint32_t hash,
                                       int modType,
                                       int *sizeOut,
                                       const char *label)
{
    Mod *mod = getMod(hash, modType);
    if (mod == nullptr || mod->Data.empty() || mod->Data.size() > 0x7FFFFFFFu)
        return nullptr;

    void *raw = tlMemAlloc(static_cast<uint32_t>(mod->Data.size()),
                           16u, 0x2000000u);
    if (raw == nullptr)
        return nullptr;

    std::memcpy(raw, mod->Data.data(), mod->Data.size());
    if (sizeOut != nullptr)
        *sizeOut = static_cast<int>(mod->Data.size());

    sp_log("[mod][%s] prepared fresh writable serialized image 0x%08X (%u bytes)",
           label, hash, static_cast<unsigned>(mod->Data.size()));
    return static_cast<uint8_t *>(raw);
}

} // namespace

bool modMsnImageUsable(const uint8_t *bytes, size_t size)
{
    constexpr size_t rootOffset = sizeof(mod_generic_mash_header_disk);
    constexpr size_t minimumSize = rootOffset + sizeof(mod_msn_root_disk);
    if (bytes == nullptr || size < minimumSize || size > 0x7FFFFFFFu)
        return false;

    mod_generic_mash_header_disk header{};
    mod_msn_root_disk root{};
    std::memcpy(&header, bytes, sizeof(header));
    std::memcpy(&root, bytes + rootOffset, sizeof(root));

    // Mission tables use the non-polymorphic generic-mash path.  0xFFFF is
    // the canonical no-class-id marker for the retail mission-table image.
    if (header.class_id != 0xFFFFu)
        return false;
    if ((header.flags & 0xC0000000u) != 0u) // pristine + no vtable class path
        return false;
    if (header.mash_data_offset < static_cast<int32_t>(minimumSize)
        || static_cast<size_t>(header.mash_data_offset) > size)
        return false;

    const mod_mashable_vector_disk *vectors[] = {
        &root.marker_bases, &root.camera_markers, &root.transform_markers,
        &root.camera_transform_markers, &root.nums, &root.strings,
        &root.positions, &root.conditions
    };
    for (const auto *vec : vectors)
        if (!modMsnVectorUsable(*vec))
            return false;

    return true;
}

bool modMsnRegister(const std::filesystem::path &path,
                    std::vector<uint8_t> &&fileData)
{
    if (!modMsnImageUsable(fileData.data(), fileData.size()))
    {
        sp_log("[mod][msn] \"%s\": invalid mission_table_container generic mash, ignored",
               path.filename().string().c_str());
        return false;
    }
    return modRegisterSerializedResource(path, std::move(fileData),
                                         MOD_TYPE_MSN_FILE, "msn");
}

uint8_t *modMsnGetOverride(uint32_t hash, int *sizeOut)
{
    Mod *mod = getMod(hash, MOD_TYPE_MSN_FILE);
    if (mod == nullptr || !modMsnImageUsable(mod->Data.data(), mod->Data.size()))
        return nullptr;
    return modGetFreshSerializedOverride(hash, MOD_TYPE_MSN_FILE, sizeOut, "msn");
}

bool modPanelImageUsable(const uint8_t *bytes, size_t size)
{
    if (bytes == nullptr || size < sizeof(mod_panel_root_disk)
        || size > 0x7FFFFFFFu)
        return false;

    mod_panel_root_disk root{};
    std::memcpy(&root, bytes, sizeof(root));
    return modPanelVectorUsable(root.pquads)
        && modPanelVectorUsable(root.ptext)
        && modPanelVectorUsable(root.animations);
}

bool modPanelRegister(const std::filesystem::path &path,
                      std::vector<uint8_t> &&fileData)
{
    if (!modPanelImageUsable(fileData.data(), fileData.size()))
    {
        sp_log("[mod][panel] \"%s\": invalid PanelFile mash stream, ignored",
               path.filename().string().c_str());
        return false;
    }
    return modRegisterSerializedResource(path, std::move(fileData),
                                         MOD_TYPE_PANEL_FILE, "panel");
}

uint8_t *modPanelGetOverride(uint32_t hash, int *sizeOut)
{
    Mod *mod = getMod(hash, MOD_TYPE_PANEL_FILE);
    if (mod == nullptr || !modPanelImageUsable(mod->Data.data(), mod->Data.size()))
        return nullptr;
    return modGetFreshSerializedOverride(hash, MOD_TYPE_PANEL_FILE, sizeOut, "panel");
}


// ---------------------------------------------------------------------------
// Loose .COLL / .CUT / .PCMESHDEF / .SLF resource overrides
//
// These are engine resource-key payloads, not generic filesystem blobs:
//   COLL      -> RESOURCE_KEY_TYPE_COLLISION_MESH
//   CUT       -> RESOURCE_KEY_TYPE_CUT_SCENE
//   PCMESHDEF -> RESOURCE_KEY_TYPE_MESH_FILE_STRUCT
//   SLF       -> RESOURCE_KEY_TYPE_SLF_LIST
//
// COLL is modified in-place by cg_mesh::_un_mash (the last signature byte is
// changed to 'Z'). CUT is converted into a constructed retail-PC cut_scene by
// modCutGetOverride, and PCMESHDEF is a generic-mash image. Keep Mod::Data
// pristine and hand the engine aligned writable copies. SLF itself is
// read-only, but using the same copy path keeps external-only resource
// ownership uniform.
// ---------------------------------------------------------------------------

namespace {

#pragma pack(push, 1)
struct mod_coll_header_disk {
    char magic[4];
    uint32_t version;
    int32_t field_8;
    int32_t field_C;
};

struct mod_cut_vector_disk {
    int32_t field_0;
    int32_t size;
    uint32_t data_cookie;
    int32_t capacity;
    uint8_t from_mash;
    uint8_t padding[3];
};

// The host executable's cut_scene unmash routine consumes the retail-PC
// 0x10-byte mash cursor even when the main OpenUSM build is reading Xbox v10
// packs through the larger dual-buffer cursor. A loose .CUT is explicitly a
// retail-PC resource island, so keep that ABI local and never pass it through
// the Xbox cursor wrapper.
struct mod_pc_mash_info {
    uint8_t *image;
    int32_t used;
    int32_t size;
    int32_t field_C;
};
#pragma pack(pop)

static_assert(sizeof(mod_coll_header_disk) == 0x10u,
              "COLL header disk layout changed");
static_assert(sizeof(mod_cut_vector_disk) == 0x14u,
              "CUT mVector disk layout changed");
static_assert(sizeof(mod_pc_mash_info) == 0x10u,
              "retail PC mash cursor layout changed");

struct mod_cut_runtime_image {
    const Mod *source = nullptr;
    uint8_t *copy = nullptr;
    int size = 0;
};

std::unordered_map<uint32_t, mod_cut_runtime_image> &modCutRuntimeImages()
{
    static std::unordered_map<uint32_t, mod_cut_runtime_image> images;
    return images;
}

bool modCutVectorUsable(const mod_cut_vector_disk &vec)
{
    if (vec.size < 0 || vec.size > 0x4000)
        return false;
    if (vec.capacity < vec.size || vec.capacity > 0x10000)
        return false;
    if (vec.from_mash != 1u)
        return false;
    if (vec.size > 0 && vec.data_cookie == 0u)
        return false;
    return true;
}

bool modSlfImageUsableInternal(const uint8_t *bytes, size_t size)
{
    if (bytes == nullptr || size < sizeof(uint32_t) || size > 0x7FFFFFFFu)
        return false;

    uint32_t totalClasses = 0;
    std::memcpy(&totalClasses, bytes, sizeof(totalClasses));
    if (totalClasses == 0u || totalClasses > 0x1000u)
        return false;

    size_t cursor = sizeof(uint32_t);
    for (uint32_t i = 0; i < totalClasses; ++i)
    {
        if (cursor > size || size - cursor < sizeof(uint32_t))
            return false;

        uint32_t totalFuncs = 0;
        std::memcpy(&totalFuncs, bytes + cursor, sizeof(totalFuncs));
        cursor += sizeof(uint32_t);

        if (totalFuncs > 0x10000u)
            return false;

        const size_t funcBytes = static_cast<size_t>(totalFuncs) * sizeof(uint32_t);
        if (cursor > size || funcBytes > size - cursor)
            return false;
        cursor += funcBytes;
    }

    // slc_manager::un_mash_all_funcs walks this exact stream without a size
    // field or footer. Trailing bytes therefore indicate a mismatched image.
    return cursor == size;
}

} // namespace

bool modCollImageUsable(const uint8_t *bytes, size_t size)
{
    if (bytes == nullptr || size < sizeof(mod_coll_header_disk)
        || size > 0x7FFFFFFFu)
        return false;

    mod_coll_header_disk header{};
    std::memcpy(&header, bytes, sizeof(header));

    if (std::memcmp(header.magic, "COLL", 4) != 0
        && std::memcmp(header.magic, "COLB", 4) != 0)
        return false;

    // cg_mesh::_un_mash accepts this exact retail PC collision version.
    return header.version == 0x0010003Fu;
}

bool modCollRegister(const std::filesystem::path &path,
                     std::vector<uint8_t> &&fileData)
{
    if (!modCollImageUsable(fileData.data(), fileData.size()))
    {
        sp_log("[mod][coll] \"%s\": invalid/unsupported collision mesh, ignored",
               path.filename().string().c_str());
        return false;
    }
    return modRegisterSerializedResource(path, std::move(fileData),
                                         MOD_TYPE_COLL_FILE, "coll");
}

uint8_t *modCollGetOverride(uint32_t hash, int *sizeOut)
{
    Mod *mod = getMod(hash, MOD_TYPE_COLL_FILE);
    if (mod == nullptr || !modCollImageUsable(mod->Data.data(), mod->Data.size()))
        return nullptr;
    return modGetFreshSerializedOverride(hash, MOD_TYPE_COLL_FILE, sizeOut, "coll");
}

bool modCutImageUsable(const uint8_t *bytes, size_t size)
{
    // cut_scene is 0x54 bytes on PC; its mVector<cut_scene_segment> begins at
    // offset 0x10 and is still in serialized/from-mash form on disk.
    constexpr size_t cutSceneSize = 0x54u;
    constexpr size_t segmentsOffset = 0x10u;
    if (bytes == nullptr || size < cutSceneSize || size > 0x7FFFFFFFu)
        return false;

    mod_cut_vector_disk segments{};
    std::memcpy(&segments, bytes + segmentsOffset, sizeof(segments));
    return modCutVectorUsable(segments);
}

bool modCutRegister(const std::filesystem::path &path,
                    std::vector<uint8_t> &&fileData)
{
    if (!modCutImageUsable(fileData.data(), fileData.size()))
    {
        sp_log("[mod][cut] \"%s\": invalid cut_scene mash stream, ignored",
               path.filename().string().c_str());
        return false;
    }
    const uint32_t hash = to_hash(
        transformToLower(path.stem().string()).c_str());
    // Existing game objects may still point into an older copy. Do not free it
    // during re-enumeration; dropping the cache entry makes the next request
    // construct from the newly registered pristine bytes.
    modCutRuntimeImages().erase(hash);
    return modRegisterSerializedResource(path, std::move(fileData),
                                         MOD_TYPE_CUT_FILE, "cut");
}

uint8_t *modCutGetOverride(uint32_t hash, int *sizeOut)
{
    Mod *mod = getMod(hash, MOD_TYPE_CUT_FILE);
    if (mod == nullptr || !modCutImageUsable(mod->Data.data(), mod->Data.size()))
        return nullptr;

    auto &slot = modCutRuntimeImages()[hash];
    if (slot.copy == nullptr || slot.source != mod)
    {
        if (mod->Data.size() > 0x7FFFFFFFu)
            return nullptr;

        void *raw = tlMemAlloc(static_cast<uint32_t>(mod->Data.size()),
                               16u, 0x2000000u);
        if (raw == nullptr)
            return nullptr;
        std::memcpy(raw, mod->Data.data(), mod->Data.size());

        auto *scene = static_cast<cut_scene *>(raw);
        mod_pc_mash_info pcMash {
            static_cast<uint8_t *>(raw),
            static_cast<int32_t>(sizeof(cut_scene)),
            static_cast<int32_t>(mod->Data.size()),
            0,
        };

        // Equivalent to retail mash_info_struct::unmash_class<cut_scene>:
        // the root object already occupies [0, sizeof(cut_scene)), then the
        // stock routine rebases its nested vectors/strings from that cursor.
        THISCALL(0x00742930, scene, &pcMash, nullptr);

        struct scene_binding {
            int segmentIndex;
            int sceneIndex;
            uint32_t hash;
            nalSceneAnim *sceneAnim;
        };
        std::vector<scene_binding> bindings;

        bool bindingsUsable = pcMash.used >= static_cast<int32_t>(sizeof(cut_scene))
                           && pcMash.used <= pcMash.size
                           && scene->segments.m_size > 0
                           && scene->segments.m_size <= 0x1000
                           && scene->segments.m_data != nullptr;
        for (int segmentIndex = 0;
             bindingsUsable && segmentIndex < scene->segments.m_size;
             ++segmentIndex)
        {
            cut_scene_segment *segment = scene->segments.m_data[segmentIndex];
            if (segment == nullptr || segment->field_10.m_size <= 0
                || segment->field_10.m_size > 0x1000
                || segment->field_10.m_data == nullptr)
            {
                bindingsUsable = false;
                break;
            }

            for (int sceneIndex = 0; sceneIndex < segment->field_10.m_size;
                 ++sceneIndex)
            {
                const uintptr_t serializedHash = reinterpret_cast<uintptr_t>(
                    segment->field_10.m_data[sceneIndex]);
                if (serializedHash == 0 || serializedHash > UINT32_MAX)
                {
                    bindingsUsable = false;
                    break;
                }

                const uint32_t dependencyHash =
                    static_cast<uint32_t>(serializedHash);
                nalSceneAnim *externalScene =
                    modEnsureExternalSceneAnim(dependencyHash);
                if (externalScene == nullptr)
                {
                    sp_log("[mod][cut] 0x%08X rejected: external scene 0x%08X is unavailable",
                           hash, dependencyHash);
                    bindingsUsable = false;
                    break;
                }

                bindings.push_back({segmentIndex, sceneIndex,
                                    dependencyHash, externalScene});
            }
        }

        if (!bindingsUsable || bindings.empty())
        {
            sp_log("[mod][cut] 0x%08X: invalid or unresolved scene dependency table",
                   hash);
            tlMemFree(raw);
            return nullptr;
        }

        // The stock constructor resolves scene hashes through a directory hash
        // method which is a null stub for nalSceneAnim. Let it finish all other
        // segment construction, then replace only the snapshotted scene slots
        // with the verified external shells. No constructor code consumes the
        // resolver result after the segment constructor returns.
        THISCALL(0x00742890, scene, nullptr);

        for (const scene_binding &binding : bindings)
        {
            if (binding.segmentIndex >= scene->segments.m_size
                || scene->segments.m_data == nullptr)
            {
                bindingsUsable = false;
                break;
            }
            cut_scene_segment *segment =
                scene->segments.m_data[binding.segmentIndex];
            if (segment == nullptr || binding.sceneIndex >= segment->field_10.m_size
                || segment->field_10.m_data == nullptr)
            {
                bindingsUsable = false;
                break;
            }
            segment->field_10.m_data[binding.sceneIndex] = binding.sceneAnim;
        }

        if (!bindingsUsable)
        {
            sp_log("[mod][cut] 0x%08X: scene binding layout changed during construction",
                   hash);
            tlMemFree(raw);
            return nullptr;
        }

        if (scene->segments.m_size <= 0 || scene->segments.m_data == nullptr)
        {
            sp_log("[mod][cut] 0x%08X: constructed cut_scene has no segments, rejected",
                   hash);
            tlMemFree(raw);
            return nullptr;
        }

        slot.source = mod;
        slot.copy = static_cast<uint8_t *>(raw);
        slot.size = static_cast<int>(mod->Data.size());
        sp_log("[mod][cut] prepared retail-PC cut_scene 0x%08X "
               "(%d bytes, %d segments, %u external scene binding(s))",
               hash, slot.size, scene->segments.m_size,
               static_cast<unsigned>(bindings.size()));
    }

    if (sizeOut != nullptr)
        *sizeOut = slot.size;
    return slot.copy;
}

bool modPcmeshdefImageUsable(const uint8_t *bytes, size_t size)
{
    if (bytes == nullptr || size < sizeof(mod_generic_mash_header_disk) + 4u
        || size > 0x7FFFFFFFu)
        return false;

    mod_generic_mash_header_disk header{};
    std::memcpy(&header, bytes, sizeof(header));

    if (header.class_id != 0xFFFFu)
        return false;
    if ((header.flags & 0xC0000000u) != 0u)
        return false;
    if (header.mash_data_offset < static_cast<int32_t>(sizeof(header))
        || static_cast<size_t>(header.mash_data_offset) > size)
        return false;

    return true;
}

bool modPcmeshdefRegister(const std::filesystem::path &path,
                          std::vector<uint8_t> &&fileData)
{
    if (!modPcmeshdefImageUsable(fileData.data(), fileData.size()))
    {
        sp_log("[mod][pcmeshdef] \"%s\": invalid mesh-file definition generic mash, ignored",
               path.filename().string().c_str());
        return false;
    }
    return modRegisterSerializedResource(path, std::move(fileData),
                                         MOD_TYPE_PCMESHDEF_FILE, "pcmeshdef");
}

uint8_t *modPcmeshdefGetOverride(uint32_t hash, int *sizeOut)
{
    Mod *mod = getMod(hash, MOD_TYPE_PCMESHDEF_FILE);
    if (mod == nullptr
        || !modPcmeshdefImageUsable(mod->Data.data(), mod->Data.size()))
        return nullptr;
    return modGetFreshSerializedOverride(hash, MOD_TYPE_PCMESHDEF_FILE,
                                         sizeOut, "pcmeshdef");
}

bool modSlfRegister(const std::filesystem::path &path,
                    std::vector<uint8_t> &&fileData)
{
    if (!modSlfImageUsableInternal(fileData.data(), fileData.size()))
    {
        sp_log("[mod][slf] \"%s\": invalid SLC function-list stream, ignored",
               path.filename().string().c_str());
        return false;
    }
    return modRegisterSerializedResource(path, std::move(fileData),
                                         MOD_TYPE_SLF_FILE, "slf");
}

uint8_t *modSlfGetOverride(uint32_t hash, int *sizeOut)
{
    Mod *mod = getMod(hash, MOD_TYPE_SLF_FILE);
    if (mod == nullptr
        || !modSlfImageUsableInternal(mod->Data.data(), mod->Data.size()))
        return nullptr;
    return modGetFreshSerializedOverride(hash, MOD_TYPE_SLF_FILE, sizeOut, "slf");
}


// ---------------------------------------------------------------------------
// Loose .BAI resource overrides
//
// BASE_AI resources are raw ai::core_ai_resource mash streams.  The packed
// base_ai_resource_handler gets writable pack bytes, un-mashes the root in
// place, then runs its from-mash constructor.  extra/**/*.bai must follow the
// same lifetime: Mod::Data stays pristine and the game receives a private,
// aligned, writable and already-constructed core_ai_resource object.
// ---------------------------------------------------------------------------

namespace {

inline constexpr int MOD_TYPE_BAI_FILE = 0x105;

#pragma pack(push, 1)
struct mod_bai_vector_disk {
    int32_t field_0;
    int32_t size;
    uint32_t data_cookie;
    int32_t capacity;
    uint8_t flag;
    uint8_t padding[3];
};

struct mod_bai_root_disk {
    uint32_t param_field_0;
    uint32_t param_array_cookie;
    uint8_t param_flag;
    uint8_t param_padding[3];
    uint32_t field_C;
    uint32_t combo_system_cookie;
    mod_bai_vector_disk base_graphs;
    mod_bai_vector_disk locomotion_graphs;
    uint32_t pack_slot_cookie;
    uint32_t field_40;
    uint8_t field_44;
    uint8_t tail_padding[3];
};
#pragma pack(pop)

static_assert(sizeof(mod_bai_vector_disk) == 0x14u,
              "BAI mVector disk layout changed");
static_assert(sizeof(mod_bai_root_disk) == 0x48u,
              "BAI core_ai_resource disk layout changed");

struct mod_bai_runtime_image {
    const Mod *source = nullptr;
    uint8_t *copy = nullptr;
    int size = 0;
};

std::unordered_map<uint32_t, mod_bai_runtime_image> &modBaiRuntimeImages()
{
    static std::unordered_map<uint32_t, mod_bai_runtime_image> images;
    return images;
}

bool modBaiVectorUsable(const mod_bai_vector_disk &vec)
{
    if (vec.size < 0 || vec.size > 0x4000)
        return false;
    if (vec.capacity < vec.size || vec.capacity > 0x10000)
        return false;
    if (vec.flag > 1u)
        return false;
    if (vec.size > 0 && vec.data_cookie == 0u)
        return false;
    return true;
}

void modBaiEraseTypedBinding(uint32_t hash)
{
    auto range = Mods.equal_range(hash);
    for (auto it = range.first; it != range.second; )
    {
        if (it->second.Type == MOD_TYPE_BAI_FILE)
            it = Mods.erase(it);
        else
            ++it;
    }
}

} // namespace

bool modBaiImageUsable(const uint8_t *bytes, size_t size)
{
    if (bytes == nullptr || size < sizeof(mod_bai_root_disk))
        return false;

    mod_bai_root_disk root{};
    std::memcpy(&root, bytes, sizeof(root));

    // param_block::field_8 is a bool followed by mash padding.
    if (root.param_flag > 1u)
        return false;

    if (!modBaiVectorUsable(root.base_graphs)
        || !modBaiVectorUsable(root.locomotion_graphs))
        return false;

    // Each populated mVector needs at least one serialized pointer slot per
    // element somewhere in the stream.  This is deliberately only a lower
    // bound: param_block/combo data may appear before those arrays.
    const size_t minimumSize = sizeof(mod_bai_root_disk)
        + sizeof(uint32_t) * static_cast<size_t>(root.base_graphs.size)
        + sizeof(uint32_t) * static_cast<size_t>(root.locomotion_graphs.size);
    if (minimumSize > size)
        return false;

    return true;
}

bool modBaiRegister(const std::filesystem::path &path,
                    std::vector<uint8_t> &&fileData)
{
    if (!modBaiImageUsable(fileData.data(), fileData.size()))
    {
        sp_log("[mod][bai] \"%s\": invalid core_ai_resource mash stream, ignored",
               path.filename().string().c_str());
        return false;
    }

    const std::string stem = transformToLower(path.stem().string());
    const uint32_t hash = to_hash(stem.c_str());

    uint32_t literal = 0;
    const bool hasLiteral = modParseLiteralHash(stem, &literal) && literal != hash;

    modBaiEraseTypedBinding(hash);
    if (hasLiteral)
        modBaiEraseTypedBinding(literal);

    // Do not free a previous live image here: game objects may still point at
    // it.  Dropping the cache entry simply makes a later request build from
    // the newly registered pristine source.
    modBaiRuntimeImages().erase(hash);
    if (hasLiteral)
        modBaiRuntimeImages().erase(literal);

    const unsigned fileSize = static_cast<unsigned>(fileData.size());
    Mods.emplace(hash, Mod{path, MOD_TYPE_BAI_FILE, std::move(fileData)});

    if (hasLiteral)
    {
        const Mod *registered = getMod(hash, MOD_TYPE_BAI_FILE);
        if (registered != nullptr)
            Mods.emplace(literal,
                         Mod{registered->Path, MOD_TYPE_BAI_FILE, registered->Data});
    }

    sp_log("[mod][bai] registered \"%s\" -> \"%s\" "
           "(0x%08X, %u bytes, core_ai_resource mash)%s",
           path.filename().string().c_str(), stem.c_str(), hash, fileSize,
           hasLiteral ? " [literal-hash alias added]" : "");
    return true;
}

uint8_t *modBaiGetOverride(uint32_t baiHash, int *sizeOut)
{
    Mod *mod = getMod(baiHash, MOD_TYPE_BAI_FILE);
    if (mod == nullptr || mod->Data.empty()
        || !modBaiImageUsable(mod->Data.data(), mod->Data.size()))
        return nullptr;

    auto &slot = modBaiRuntimeImages()[baiHash];
    if (slot.copy == nullptr || slot.source != mod)
    {
        if (mod->Data.size() > 0x7FFFFFFFu)
            return nullptr;

        void *raw = tlMemAlloc(static_cast<uint32_t>(mod->Data.size()),
                               16u, 0x2000000u);
        if (raw == nullptr)
            return nullptr;

        std::memcpy(raw, mod->Data.data(), mod->Data.size());

        auto *aiResource = bit_cast<ai::core_ai_resource *>(raw);
        mash_info_struct mash{static_cast<uint8_t *>(raw),
                              static_cast<int>(mod->Data.size())};

        // Same LOAD path as base_ai_resource_handler::_handle_resource.
        mash.unmash_class(aiResource, nullptr);
        mash_info_struct::construct_class(aiResource);

        if (aiResource != raw)
        {
            sp_log("[mod][bai] 0x%08X: root rebased away from image base, rejected",
                   baiHash);
            tlMemFree(raw);
            return nullptr;
        }

        slot.source = mod;
        slot.copy = static_cast<uint8_t *>(raw);
        slot.size = static_cast<int>(mod->Data.size());

        sp_log("[mod][bai] prepared writable BAI 0x%08X (%d bytes)",
               baiHash, slot.size);
    }

    if (sizeOut != nullptr)
        *sizeOut = slot.size;
    return slot.copy;
}


// ---------------------------------------------------------------------------
// Loose .ASG resource overrides
//
// AI_STATE_GRAPH resources are raw ai::state_graph mash streams.  The packed
// ai_state_graph_resource_handler un-mashes the writable resource image in
// place and then runs the state_graph from-mash constructor. extra/**/*.asg
// follows the same path while preserving Mod::Data as a pristine source.
// ---------------------------------------------------------------------------

namespace {

inline constexpr int MOD_TYPE_ASG_FILE = 0x106;

#pragma pack(push, 1)
struct mod_asg_vector_disk {
    int32_t field_0;
    int32_t size;
    uint32_t data_cookie;
    int32_t capacity;
    uint8_t flag;
    uint8_t padding[3];
};

struct mod_asg_root_disk {
    uint32_t name_hash;
    uint32_t resource_type;
    mod_asg_vector_disk states;
    uint32_t initial_state_cookie;
    mod_asg_vector_disk base_states;
};
#pragma pack(pop)

static_assert(sizeof(mod_asg_vector_disk) == 0x14u,
              "ASG mVector disk layout changed");
static_assert(sizeof(mod_asg_root_disk) == 0x34u,
              "ASG ai::state_graph disk layout changed");

struct mod_asg_runtime_image {
    const Mod *source = nullptr;
    uint8_t *copy = nullptr;
    int size = 0;
};

std::unordered_map<uint32_t, mod_asg_runtime_image> &modAsgRuntimeImages()
{
    static std::unordered_map<uint32_t, mod_asg_runtime_image> images;
    return images;
}

bool modAsgVectorUsable(const mod_asg_vector_disk &vec)
{
    if (vec.size < 0 || vec.size > 0x4000)
        return false;
    if (vec.capacity < vec.size || vec.capacity > 0x10000)
        return false;
    if (vec.flag > 1u)
        return false;
    if (vec.size > 0 && vec.data_cookie == 0u)
        return false;
    return true;
}

void modAsgEraseTypedBinding(uint32_t hash)
{
    auto range = Mods.equal_range(hash);
    for (auto it = range.first; it != range.second; )
    {
        if (it->second.Type == MOD_TYPE_ASG_FILE)
            it = Mods.erase(it);
        else
            ++it;
    }
}

} // namespace

bool modAsgImageUsable(const uint8_t *bytes, size_t size)
{
    if (bytes == nullptr || size < sizeof(mod_asg_root_disk))
        return false;

    mod_asg_root_disk root{};
    std::memcpy(&root, bytes, sizeof(root));

    if (root.resource_type
        != static_cast<uint32_t>(RESOURCE_KEY_TYPE_AI_STATE_GRAPH))
        return false;

    if (!modAsgVectorUsable(root.states)
        || !modAsgVectorUsable(root.base_states))
        return false;

    // A populated graph must have a valid initial-state mash cookie.
    if (root.states.size > 0 && root.initial_state_cookie == 0u)
        return false;

    // Minimum pointer-table footprint. Actual state/base-state records add
    // more data after the root; this is only a conservative bounds check.
    const size_t minimumSize = sizeof(mod_asg_root_disk)
        + sizeof(uint32_t) * static_cast<size_t>(root.states.size)
        + sizeof(uint32_t) * static_cast<size_t>(root.base_states.size);
    if (minimumSize > size)
        return false;

    return true;
}

bool modAsgRegister(const std::filesystem::path &path,
                    std::vector<uint8_t> &&fileData)
{
    if (!modAsgImageUsable(fileData.data(), fileData.size()))
    {
        sp_log("[mod][asg] \"%s\": invalid ai::state_graph mash stream, ignored",
               path.filename().string().c_str());
        return false;
    }

    const std::string stem = transformToLower(path.stem().string());
    const uint32_t hash = to_hash(stem.c_str());

    uint32_t literal = 0;
    const bool hasLiteral = modParseLiteralHash(stem, &literal) && literal != hash;

    modAsgEraseTypedBinding(hash);
    if (hasLiteral)
        modAsgEraseTypedBinding(literal);

    // Existing live state graphs can still be referenced by AI state machines,
    // so do not free them here. Dropping the cache binding makes future
    // requests construct from the newly registered pristine source.
    modAsgRuntimeImages().erase(hash);
    if (hasLiteral)
        modAsgRuntimeImages().erase(literal);

    const unsigned fileSize = static_cast<unsigned>(fileData.size());
    Mods.emplace(hash, Mod{path, MOD_TYPE_ASG_FILE, std::move(fileData)});

    if (hasLiteral)
    {
        const Mod *registered = getMod(hash, MOD_TYPE_ASG_FILE);
        if (registered != nullptr)
            Mods.emplace(literal,
                         Mod{registered->Path, MOD_TYPE_ASG_FILE, registered->Data});
    }

    sp_log("[mod][asg] registered \"%s\" -> \"%s\" "
           "(0x%08X, %u bytes, ai::state_graph mash)%s",
           path.filename().string().c_str(), stem.c_str(), hash, fileSize,
           hasLiteral ? " [literal-hash alias added]" : "");
    return true;
}

uint8_t *modAsgGetOverride(uint32_t asgHash, int *sizeOut)
{
    Mod *mod = getMod(asgHash, MOD_TYPE_ASG_FILE);
    if (mod == nullptr || mod->Data.empty()
        || !modAsgImageUsable(mod->Data.data(), mod->Data.size()))
        return nullptr;

    auto &slot = modAsgRuntimeImages()[asgHash];
    if (slot.copy == nullptr || slot.source != mod)
    {
        if (mod->Data.size() > 0x7FFFFFFFu)
            return nullptr;

        void *raw = tlMemAlloc(static_cast<uint32_t>(mod->Data.size()),
                               16u, 0x2000000u);
        if (raw == nullptr)
            return nullptr;

        std::memcpy(raw, mod->Data.data(), mod->Data.size());

        auto *stateGraph = bit_cast<ai::state_graph *>(raw);
        mash_info_struct mash{static_cast<uint8_t *>(raw),
                              static_cast<int>(mod->Data.size())};

        // Same LOAD path as ai_state_graph_resource_handler::_handle_resource.
        mash.unmash_class(stateGraph, nullptr);
        mash_info_struct::construct_class(stateGraph);

        if (stateGraph != raw)
        {
            sp_log("[mod][asg] 0x%08X: root rebased away from image base, rejected",
                   asgHash);
            tlMemFree(raw);
            return nullptr;
        }

        slot.source = mod;
        slot.copy = static_cast<uint8_t *>(raw);
        slot.size = static_cast<int>(mod->Data.size());

        sp_log("[mod][asg] prepared writable ASG 0x%08X (%d bytes)",
               asgHash, slot.size);
    }

    if (sizeOut != nullptr)
        *sizeOut = slot.size;
    return slot.copy;
}

// nal_system.cpp: content-based PCANIM flavor detection used to keep
// RESOURCE_KEY_TYPE_SCENE_ANIM bound to TLRESOURCE_TYPE_SCENE_ANIM.
int modPCANIMDetectTLType(const uint8_t *raw, size_t size, int preferredType);
int modMeshDetectTLType(const uint8_t *raw, size_t size, int preferredType);

// Resource-pack bytes are normally writable pack memory.  A loose external
// file must not expose its pristine Mod::Data directly because NAL, NGL and
// several mash readers patch/rebase the serialized image in place.
static uint8_t *modCloneExternalSerializedImage(const uint8_t *src, int size)
{
    if (src == nullptr || size <= 0)
        return nullptr;

    void *copy = tlMemAlloc(static_cast<uint32_t>(size), 16u, 0x2000000u);
    if (copy == nullptr)
        return nullptr;

    std::memcpy(copy, src, static_cast<size_t>(size));
    return static_cast<uint8_t *>(copy);
}

static uint8_t *modGetLooseTLBytes(uint32_t hash, int tlType, int *sizeOut)
{
    if (sizeOut != nullptr)
        *sizeOut = 0;

    if (tlType == TLRESOURCE_TYPE_ANIM_FILE
        || tlType == TLRESOURCE_TYPE_SCENE_ANIM
        || tlType == TLRESOURCE_TYPE_SKELETON)
        modScanNalOverrides();

    Mod *mod = getMod(hash, tlType);
    if (mod == nullptr || mod->Data.empty() || mod->Data.size() > 0x7FFFFFFFu)
        return nullptr;
    if (modNalConfiguredSource(mod->Path)) return nullptr;

    const int size = static_cast<int>(mod->Data.size());
    uint8_t *copy = modCloneExternalSerializedImage(mod->Data.data(), size);
    if (copy != nullptr && sizeOut != nullptr)
        *sizeOut = size;
    return copy;
}

// Same as modGetLooseTLBytes, but selects an exact loose-file extension from
// the hash bucket.  Texture hashes can have several typed entries (.DDS, .TGA,
// generated images), so the external-only DDS path must not accidentally hand
// another file format to a caller that requested raw DDS bytes.
static uint8_t *modGetLooseTLBytesByExtension(uint32_t hash, int tlType,
                                              const char *extension,
                                              int *sizeOut)
{
    if (sizeOut != nullptr)
        *sizeOut = 0;

    if (extension == nullptr)
        return nullptr;

    if (tlType == TLRESOURCE_TYPE_SKELETON)
        modScanNalOverrides();

    const std::string wanted = transformToLower(extension);
    auto range = Mods.equal_range(hash);
    for (auto it = range.first; it != range.second; ++it)
    {
        Mod &mod = it->second;
        if (modNalConfiguredSource(mod.Path)) continue;
        if (mod.Type != tlType
            || transformToLower(mod.Path.extension().string()) != wanted
            || mod.Data.empty()
            || mod.Data.size() > 0x7FFFFFFFu)
            continue;

        // Reject obviously malformed DDS images before NGL/D3DX sees them.
        if (wanted == ".dds")
        {
            if (mod.Data.size() < 128u)
                continue;
            uint32_t magic = 0;
            std::memcpy(&magic, mod.Data.data(), sizeof(magic));
            if (magic != 0x20534444u && magic != 0x4D534444u)
                continue;
        }

        const int size = static_cast<int>(mod.Data.size());
        uint8_t *copy = modCloneExternalSerializedImage(mod.Data.data(), size);
        if (copy != nullptr && sizeOut != nullptr)
            *sizeOut = size;
        return copy;
    }

    return nullptr;
}

namespace resource_manager {

extern int &amalgapak_pack_location_count;
extern resource_pack_location *&amalgapak_pack_location_table;

namespace
{
constexpr auto XBOX_AMALGAPAK_LOCATION_SIZE = 0x28u;
// Actual format of the selected amalgapak. This cannot be inferred from
// g_platform in PC-hosted XBPACK mode because that runtime remains Xbox while
// a missing Xbox index may legitimately fall back to amalga_PC.PAK.
static _nlPlatformEnum selected_amalgapak_platform = NL_PLATFORM_PC;

struct xbox_amalgapak_location {
    resource_location loc;
    int field_10;
    int field_14;
#ifdef OPENUSM_XBPACK_V10
    int prerequisite_offset;
    int prerequisite_count;
    int field_18;
    int field_1C;
#else
    int field_18;
    int field_1C;
    int prerequisite_offset;
    int prerequisite_count;
#endif
};

VALIDATE_SIZE(xbox_amalgapak_location, XBOX_AMALGAPAK_LOCATION_SIZE);

resource_key_type convert_key_type(resource_key_type type)
{
    const auto raw_type = static_cast<int>(type);
    assert(raw_type >= 0 && raw_type < xbpack::type_count);
    return static_cast<resource_key_type>(xbpack::pc_type(raw_type));
}

void convert_key(resource_key &key)
{
    key.m_type = convert_key_type(key.m_type);
}

bool has_full_location_table(os_file &file, const resource_amalgapak_header &header)
{
    if (header.location_table_size <= 0
        || header.location_table_size % sizeof(resource_pack_location) != 0)
        return false;

    std::vector<uint8_t> table(header.location_table_size);
    file.set_fp(header.field_1C, os_file::FP_BEGIN);
    if (file.read(table.data(), header.location_table_size) != header.location_table_size)
        return false;

    for (size_t offset = 0; offset < table.size(); offset += sizeof(resource_pack_location)) {
        const auto *entry = table.data() + offset;
        uint32_t hash = 0;
        std::memcpy(&hash, entry, sizeof(hash));

        const auto *name = reinterpret_cast<const char *>(
            entry + offsetof(resource_pack_location, m_name));
        const auto *name_end = static_cast<const char *>(
            std::memchr(name, 0, sizeof(resource_pack_location::m_name)));
        if (name_end == nullptr || name_end == name || to_hash(name) != hash)
            return false;
    }

    return true;
}

void load_pack_location_table(os_file &file, const resource_amalgapak_header &pack_file_header)
{
    file.set_fp(pack_file_header.field_1C, os_file::FP_BEGIN);

    const auto full_table = selected_amalgapak_platform == NL_PLATFORM_XBOX
        && has_full_location_table(file, pack_file_header);
    file.set_fp(pack_file_header.field_1C, os_file::FP_BEGIN);

    if (selected_amalgapak_platform == NL_PLATFORM_XBOX && !full_table) {
        assert(pack_file_header.location_table_size % XBOX_AMALGAPAK_LOCATION_SIZE == 0);

        amalgapak_pack_location_count =
            pack_file_header.location_table_size / XBOX_AMALGAPAK_LOCATION_SIZE;

        std::vector<xbox_amalgapak_location> raw_locations(amalgapak_pack_location_count);
        auto how_many_did_we_get =
            file.read(raw_locations.data(), pack_file_header.location_table_size);
        assert(how_many_did_we_get == pack_file_header.location_table_size);

        amalgapak_pack_location_table = static_cast<resource_pack_location *>(arch_memalign(
            16u, amalgapak_pack_location_count * sizeof(resource_pack_location)));
        assert(amalgapak_pack_location_table != nullptr);

        for (int i = 0; i < amalgapak_pack_location_count; ++i) {
            const auto &src = raw_locations[i];
            auto &dst = amalgapak_pack_location_table[i];

            ::new (static_cast<void *>(&dst)) resource_pack_location();
            dst.loc = src.loc;
            convert_key(dst.loc.field_0);
            dst.field_10 = src.field_10;
            dst.field_14 = src.field_14;
            dst.field_18 = src.field_18;
            dst.field_1C = src.field_1C;
            dst.prerequisite_offset = src.prerequisite_offset;
            dst.prerequisite_count = src.prerequisite_count;
        }

        return;
    }

    amalgapak_pack_location_count =
        pack_file_header.location_table_size / sizeof(resource_pack_location);

    amalgapak_pack_location_table =
        static_cast<resource_pack_location *>(arch_memalign(16u, pack_file_header.location_table_size));
    assert(amalgapak_pack_location_table != nullptr);

    auto how_many_did_we_get =
        file.read(amalgapak_pack_location_table, pack_file_header.location_table_size);
    assert(how_many_did_we_get == pack_file_header.location_table_size);

    if (selected_amalgapak_platform == NL_PLATFORM_XBOX) {
        for (int i = 0; i < amalgapak_pack_location_count; ++i)
            convert_key(amalgapak_pack_location_table[i].loc.field_0);
    }
}
}

VALIDATE_SIZE(resource_memory_map, 0x90);

VALIDATE_SIZE((*partitions), 16u);

_std::vector<resource_partition *> *& partitions = var<_std::vector<resource_partition *> *>(0x0095C7F0);

_std::vector<resource_pack_slot *> & resource_context_stack = var<_std::vector<resource_pack_slot *>>(0x0096015C);

mString & amalgapak_name = var<mString>(0x0095CAD4);

#if !STANDALONE_SYSTEM 

int & amalgapak_base_offset = var<int>(0x00921CB4);

nflFileID & amalgapak_id = var<nflFileID>(0x00921CB8);

int & resource_buffer_used = var<int>(0x0095C180);

int & memory_maps_count = var<int>(0x0095C7F4);

int & resource_buffer_size = var<int>(0x0095C1C8);

int & in_use_memory_map = var<int>(0x00921CB0);

uint8_t *& resource_buffer = var<uint8_t *>(0x0095C738);

bool & using_amalga = var<bool>(0x0095C800);

int & amalgapak_signature = var<int>(0x0095C804);

resource_memory_map *& memory_maps = var<resource_memory_map *>(0x0095C2F0);

int & amalgapak_pack_location_count = var<int>(0x0095C7FC);

resource_pack_location *& amalgapak_pack_location_table = var<resource_pack_location *>(0x0095C7F8);

int & amalgapak_prerequisite_count = var<int>(0x0095C174);

resource_key *& amalgapak_prerequisite_table = var<resource_key *>(0x0095C300);

#else

#define make_var(type, name) \
    static type g_##name {}; \
    type& name {g_##name}

make_var(int, amalgapak_base_offset);

make_var(nflFileID, amalgapak_id);

make_var(int, resource_buffer_used);

make_var(int, memory_maps_count);

make_var(int, resource_buffer_size);

make_var(int, in_use_memory_map);

make_var(uint8_t *, resource_buffer);

make_var(bool, using_amalga);

make_var(int, amalgapak_signature);

make_var(resource_memory_map *, memory_maps);

make_var(int, amalgapak_pack_location_count);

make_var(resource_pack_location *, amalgapak_pack_location_table);

make_var(int, amalgapak_prerequisite_count);

make_var(resource_key *, amalgapak_prerequisite_table);

//make_var(mString, amalgapak_name);

#undef make_var
#endif

namespace
{
void release_memory_maps()
{
    if (memory_maps == nullptr)
        return;

#ifdef OPENUSM_XBPACK_V10
    // V10 loads these through the retail executable's MSVCR71 allocator.
    // Freeing them through the injected DLL's delete[] crosses CRT heaps and
    // corrupts mission reload/teardown.
    exe_allocator<resource_memory_map> allocator;
    for (int i = 0; i < memory_maps_count; ++i)
        allocator.destroy(&memory_maps[i]);
    allocator.deallocate(memory_maps, memory_maps_count);
#else
    delete[] memory_maps;
#endif

    memory_maps = nullptr;
}
}

//0x005BA9A0
[[nodiscard]] mString get_amalgapak_filename(_nlPlatformEnum arg4)
{
    const char *a2[] = {".PAK", "_XB.PAK", "_GC.PAK", "_PC.PAK"};

#ifdef TARGET_XBOX
    mString v1{a2[1]};
#else
    mString v1{a2[arg4]};
#endif
    
    mString v2{"packs\\amalga"};

    mString res = v2 + v1;

    return res;
}

int get_pack_location_count()
{
    assert(amalgapak_pack_location_table != nullptr);
    return amalgapak_pack_location_count;
}

resource_key *get_prerequisiste(int prereq_idx)
{
    assert(amalgapak_prerequisite_table != nullptr);
    assert(prereq_idx < amalgapak_prerequisite_count);

    return &amalgapak_prerequisite_table[prereq_idx];
}

void load_amalgapak()
{
    TRACE("resource_manager::load_amalgapak");

    if constexpr (1)
    {
        os_file file;

        {
            amalgapak_name = resolve_amalgapak_filename();
            sp_log("Loading amalgapak...");

            mString a1 {amalgapak_name.c_str()};

            file.open(a1, os_file::FILE_READ);
        }

        if (!file.is_open()) {
            auto *v1 = amalgapak_name.c_str();
            error("Could not open amalgapak file %s!", v1);
        }

        resource_amalgapak_header pack_file_header{};
        file.read(&pack_file_header, sizeof(resource_amalgapak_header));

        {
            mString a1 {amalgapak_name.c_str()};

            if (selected_amalgapak_platform == NL_PLATFORM_XBOX) {
                // The on-disk header has the same 0x38-byte shape on both
                // platforms, but the version contract does not.  Calling the
                // PC member here made a valid v14 Xbox index report itself as
                // older than the v17 retail-PC code even in XBPACK mode.
                reinterpret_cast<resource_amalgapak_header_xbox *>(
                    &pack_file_header)->verify(a1);
            } else {
                sp_log("Using native PC amalgapak versions: %s",
                       pack_file_header.field_0.to_string().c_str());
            }
        }

#ifndef OPENUSM_XBPACK_MODE
        if constexpr (1)
        {
            pack_file_header.field_18 = 0;
        }
#endif

        amalgapak_base_offset = pack_file_header.field_18;
        using_amalga = (pack_file_header.field_18 != 0);
        amalgapak_signature = pack_file_header.field_14;
        load_pack_location_table(file, pack_file_header);

        amalgapak_prerequisite_count = static_cast<uint32_t>(
                                             pack_file_header.prerequisite_table_size) >>
            3;

        amalgapak_prerequisite_table = static_cast<resource_key *>(
            arch_memalign(8u, pack_file_header.prerequisite_table_size));
        assert(amalgapak_prerequisite_table != nullptr);

        file.set_fp(pack_file_header.field_2C, os_file::FP_BEGIN);
        auto how_many_did_we_get = file.read(amalgapak_prerequisite_table,
                                             pack_file_header.prerequisite_table_size);
        assert(how_many_did_we_get == pack_file_header.prerequisite_table_size);

        if (selected_amalgapak_platform == NL_PLATFORM_XBOX) {
            for (int i = 0; i < amalgapak_prerequisite_count; ++i) {
                convert_key(amalgapak_prerequisite_table[i]);
            }
        }

        resource_buffer_size = pack_file_header.field_34;
        assert(pack_file_header.memory_map_table_size % sizeof(resource_memory_map) == 0);

        memory_maps_count = pack_file_header.memory_map_table_size / sizeof(resource_memory_map);

#ifdef OPENUSM_XBPACK_V10
        exe_allocator<resource_memory_map> allocator;
        memory_maps = allocator.allocate(memory_maps_count);
        for (int i = 0; i < memory_maps_count; ++i)
            allocator.construct(&memory_maps[i]);
#else
        memory_maps = new resource_memory_map[memory_maps_count];
#endif
        file.set_fp(pack_file_header.field_24, os_file::FP_BEGIN);
        how_many_did_we_get = file.read(memory_maps, pack_file_header.memory_map_table_size);
        assert(how_many_did_we_get == pack_file_header.memory_map_table_size);

        file.close();

        if (using_amalgapak())
        {
            amalgapak_id = nflOpenFile({1}, amalgapak_name.c_str());

            if (amalgapak_id == NFL_FILE_ID_INVALID)
            {
                amalgapak_id = nflOpenFile({2}, amalgapak_name.c_str());

                if (amalgapak_id == NFL_FILE_ID_INVALID)
                {
                    mString v12 {amalgapak_name.c_str()};
                    mString v13 {"data\\"};

                    mString a1 = v13 + v12;

                    amalgapak_id = nflOpenFile({2}, a1.c_str());
                }
            }

            sp_log("Using amalgapak found on the HOST");
        } else {
            sp_log("Using amalgapak found on the CD");
        }

    } else {
        CDECL_CALL(0x00537650);
    }
}


void add_resource_pack_modified_callback(void (*callback)(_std::vector<resource_key> &))
{
    assert(callback != nullptr);

    //push_back
    auto *v18 = resource_pack_modified_callbacks.m_last;
    auto *a2 = callback;
    if ( resource_pack_modified_callbacks.size() < resource_pack_modified_callbacks.capacity()
         )
    {
        *resource_pack_modified_callbacks.m_last = a2;
        resource_pack_modified_callbacks.m_last = v18 + 1;
    }
    else
    {
        void (__fastcall *_Insert_n)(void *, void *, void *, int, decltype(&callback)) = CAST(_Insert_n, 0x0056A260);
        _Insert_n(&resource_pack_modified_callbacks,
                nullptr,
                resource_pack_modified_callbacks.m_last,
                1,
                &a2);
    }
}

bool using_amalgapak()
{
    return using_amalga;
}

bool is_idle()
{
    if constexpr (1)
    {
        assert(partitions != nullptr);

        for ( auto &partition : (*partitions) )
        {
            assert(partition != nullptr);
            if ( !partition->get_streamer()->is_idle() )
            {
                return false;
            }
        }

        return true;
    }
    else
    {
        return (bool) CDECL_CALL(0x00537AC0);
    }
}

bool can_reload_amalgapak()
{
    if constexpr (1)
    {
        if ( using_amalgapak() )
        {
            return false;
        }

        if ( !is_idle() )
        {
            return false;
        }

        bool result = false;
        os_file v11{};
        auto *v1 = amalgapak_name.c_str();
        mString v4 {v1};
        v11.open(v4, os_file::FILE_READ);
        if ( v11.is_open() )
        {
            resource_amalgapak_header data{};
            v11.read(&data, sizeof(data));
            auto *v2 = amalgapak_name.c_str();
            auto a2 = mString{v2};
            data.verify(a2);
            if ( data.field_18 != 0 )
            {
                result = false;
            }
            else if ( data.field_14 == amalgapak_signature )
            {
                result = false;
            }
            else
            {
                result = true;
            }
        }
        else
        {
            result = false;
        }

        return result;
    }
    else
    {
        return (bool) CDECL_CALL(0x0053DE90);
    }
}

void reload_amalgapak()
{
    TRACE("resource_manager::reload_amalgapak");

    if constexpr (1)
    {
        assert(!using_amalgapak());

        assert(amalgapak_pack_location_table != nullptr);

        assert(amalgapak_prerequisite_table != nullptr);

        assert(memory_maps != nullptr);

        mem_freealign(amalgapak_prerequisite_table);
        mem_freealign(amalgapak_pack_location_table);

        release_memory_maps();
        amalgapak_prerequisite_table = nullptr;
        amalgapak_pack_location_table = nullptr;

        load_amalgapak();

        _std::vector<resource_key> v3;
        for ( auto i = 0; i < amalgapak_pack_location_count; ++i )
        {
            if ( amalgapak_pack_location_table[i].field_2C != 0 )
            {
                v3.push_back(amalgapak_pack_location_table[i].loc.field_0);
            }
        }

        for ( auto &cb : resource_pack_modified_callbacks )
        {
            (*cb)(v3);
        }
    }
    else
    {
        CDECL_CALL(0x0054C2E0);
    }
}


resource_pack_slot *get_best_context(resource_pack_slot *slot)
{
    TRACE("resource_manager::get_best_context");

    if constexpr (1)
    {
        assert(slot != nullptr);
        assert(slot->is_data_ready());
        assert(partitions != nullptr);

        resource_partition *the_partition = nullptr;

        const auto &vec = (*partitions);
        sp_log("%d", vec.size());
        for (const auto &my_partition : vec)
        {
            assert(my_partition != nullptr);

            auto &pack_slots = my_partition->get_pack_slots();
            for (uint32_t i = 0; i < pack_slots.size(); ++i)
            {
                if (pack_slots[i] == slot) {
                    the_partition = my_partition;
                    sp_log("%d", i);
                    break;
                }
            }
        }

        assert(the_partition != nullptr && "what partition uses this slot!?");

        if (the_partition->field_0 != 2) {
            return slot;
        }

        assert(!the_partition->get_pack_slots().empty());

        auto *result = the_partition->get_pack_slots().front();
        //sp_log("0x%08X", result->pack_directory.field_4.m_vtbl);

        return result;
    }
    else
    {
        return (resource_pack_slot *) CDECL_CALL(0x005375A0, slot);
    }
}

resource_pack_slot *get_and_push_resource_context(resource_partition_enum a1)
{
    auto *v1 = get_best_context(a1);
    return push_resource_context(v1);
}

bool get_pack_location(int a1, resource_pack_location *a2)
{
    assert(amalgapak_pack_location_table != nullptr);
    assert(amalgapak_base_offset != -1);

    if ( a1 < 0 || a1 >= amalgapak_pack_location_count )
    {
        return false;
    }

    if ( a2 != nullptr )
    {
        *a2 = amalgapak_pack_location_table[a1];
        a2->loc.m_offset += amalgapak_base_offset;
    }

    return true;
}

resource_pack_slot *get_best_context(resource_partition_enum a1)
{
    if constexpr (1)
    {
        assert(partitions != nullptr);

        resource_partition *the_partition = partitions->at(a1);
        assert(the_partition != nullptr);

        const auto &pack_slots = the_partition->get_pack_slots();
        if (pack_slots.empty()) {
            the_partition = partitions->front();
        }

        resource_pack_slot *best_slot = the_partition->get_pack_slots().front();
        assert(best_slot != nullptr);

        return best_slot;
    } else {
        return (resource_pack_slot *) CDECL_CALL(0x00537610, a1);
    }
}

void frame_advance(Float a2)
{
    auto v8 =
        os_developer_options::instance->get_int(mString {"AMALGA_REFRESH_INTERVAL"});

    static float amalga_refresh_timer {0};
    amalga_refresh_timer += a2;
    if ( v8 > 0 && amalga_refresh_timer > v8 )
    {
        if ( can_reload_amalgapak() )
        {
            reload_amalgapak();
        }

        amalga_refresh_timer = 0.0;
    }

    if constexpr (0)
    {
        static auto & dword_960CB0 = var<int>(0x00960CB0);

        if (dword_960CB0 == 0)
        {
            limited_timer timer{0.02};

            if (g_game_ptr != nullptr && g_game_ptr->field_165)
            {
                limited_timer v4{0.5};

                timer = v4;
            }

            timer.reset();

            assert(partitions != nullptr);

            for (auto *partition : (*partitions)) {

                assert(partition != nullptr);

                partition->frame_advance(a2, &timer);
            }
        }
    }
    else
    {
        CDECL_CALL(0x00558D20, a2);
    }

#if defined(ENABLE_DEBUG_MENU) && DEBUG_MENU_REIMPL == 0
    debug_menu::frame_advance(a2);
#endif
}

bool get_pack_file_stats(const resource_key &a1, resource_pack_location *a2, mString *a3, int *a4)
{
    TRACE("resource_manager::get_pack_file_stats", a1.get_platform_string(g_platform).c_str());

    if constexpr (1)
    {
        assert(amalgapak_pack_location_table != nullptr);

        if (a3 != nullptr) {
            *a3 = amalgapak_name.c_str();
        }

        assert(amalgapak_base_offset != -1);

        {
            auto is_sorted = std::is_sorted(amalgapak_pack_location_table,
                    amalgapak_pack_location_table + amalgapak_pack_location_count,
                    [](auto &a1, auto &a2) {
                        return a1.loc.field_0 <= a2.loc.field_0;
                    });
        //    assert(is_sorted);
        }

        auto i = 0;
        if (!binary_search_array_cmp<const resource_key, const resource_pack_location>(
                &a1,
                amalgapak_pack_location_table,
                0,
                amalgapak_pack_location_count,
                &i,
                compare_resource_key_resource_pack_location))
        {
            for (int j = 0; j < amalgapak_pack_location_count; ++j) {
                if (amalgapak_pack_location_table[j].loc.field_0.m_hash == a1.m_hash) {
                    i = j;
                    break;
                }
            }

            if (i < 0 || i >= amalgapak_pack_location_count ||
                amalgapak_pack_location_table[i].loc.field_0.m_hash != a1.m_hash) {
                sp_log("Pack lookup failed: hash=0x%08X type=%d platform=%d count=%d",
                       a1.m_hash.source_hash_code,
                       a1.m_type,
                       g_platform,
                       amalgapak_pack_location_count);
                return false;
            }
        }


        if (a2 != nullptr) {
            *a2 = amalgapak_pack_location_table[i];
            a2->loc.m_offset += amalgapak_base_offset;
        }

        if (a4 != nullptr) {
            *a4 = i;
        }

        return true;
    } else {
        auto result = (bool) CDECL_CALL(0x0052A820, &a1, a2, a3, a4);
        sp_log("%s", result ? "true" : "false");
        return result;
    }
}

resource_pack_slot *push_resource_context(resource_pack_slot *pack_slot)
{
    TRACE("resource_manager::push_resource_context");

    sp_log("%s", pack_slot->get_name_key().get_platform_string(3).c_str());

    if constexpr (1)
    {
        assert(pack_slot != nullptr);

        resource_pack_slot *v2 = get_resource_context();

        //push_back
        if (resource_context_stack.size() < resource_context_stack.capacity())
        {
            *resource_context_stack.m_last = pack_slot;
            ++resource_context_stack.m_last;

        }
        else
        {
            if constexpr (1)
            {
                void (__fastcall *func)(void *, void *edx, void *, int, resource_pack_slot **) = CAST(func, 0x0056A260);
                func(&resource_context_stack, nullptr,
                     resource_context_stack.m_last,
                     1,
                     &pack_slot);
            }
            else
            {
                resource_context_stack.insert(resource_context_stack.end(), pack_slot);
            }
        }

        set_active_resource_context(pack_slot);

        return v2;
    } else {
        return (resource_pack_slot *) CDECL_CALL(0x00542740, pack_slot);
    }
}

resource_directory *get_resource_directory(const resource_key &a1)
{
    if constexpr (1)
    {
        assert(partitions != nullptr);

        for (size_t i = 0; i < partitions->size(); ++i) {
            auto &partition = partitions->at(i);
            assert(partition != nullptr);

            auto *streamer = partition->get_streamer();
            assert(streamer != nullptr);

            auto *pack_slots = streamer->get_pack_slots();
            assert(pack_slots != nullptr);

            for (auto &pack_slot : (*pack_slots)) {
                assert(pack_slot != nullptr);

                if (pack_slot->is_data_ready())
                {
                    if (pack_slot->get_name_key() == a1) {
                        return &pack_slot->get_resource_directory();
                    }
                }
            }
        }

        return nullptr;
    } else {
        return (resource_directory *) CDECL_CALL(0x00537A10, &a1);
    }
}

void set_active_resource_context(resource_pack_slot *a1)
{
    TRACE("resource_manager::set_active_resource_context");

    if constexpr (0)
    {
        if (a1 != nullptr && a1->is_data_ready())
        {
            auto &pack_dir = a1->get_resource_pack_directory();
            nglSetTextureDirectory(&pack_dir.field_4);
            nglSetMeshFileDirectory(&pack_dir.field_C);
            nglSetMeshDirectory(&pack_dir.field_14);
            nglSetMorphDirectory(&pack_dir.field_1C);
            nglSetMaterialFileDirectory(&pack_dir.field_34);
            nglSetMaterialDirectory(&pack_dir.field_2C);
            nalSetSkeletonDirectory(&pack_dir.field_54);
            nalSetAnimFileDirectory(&pack_dir.field_3C);
            nalSetAnimDirectory(&pack_dir.field_44);
            nalSetSceneAnimDirectory(&pack_dir.field_4C);
        }
        else
        {
            nglSetTextureDirectory(tlresource_directory<nglTexture, tlFixedString>::system_dir);
            nglSetMeshFileDirectory(tlresource_directory<nglMeshFile, tlFixedString>::system_dir);
            nglSetMeshDirectory(tlresource_directory<nglMesh, tlHashString>::system_dir);
            nglSetMorphDirectory(tlresource_directory<nglMorphSet, tlHashString>::system_dir);
            nglSetMaterialFileDirectory(
                tlresource_directory<nglMaterialFile, tlFixedString>::system_dir);
            nglSetMaterialDirectory(
                tlresource_directory<nglMaterialBase, tlHashString>::system_dir);
            nalSetAnimFileDirectory(tlresource_directory<nalAnimFile, tlFixedString>::system_dir);
            nalSetSkeletonDirectory(
                tlresource_directory<nalBaseSkeleton, tlFixedString>::system_dir);
            nalSetAnimDirectory(
                tlresource_directory<nalAnimClass<nalAnyPose>, tlFixedString>::system_dir);
            nalSetSceneAnimDirectory(
                tlresource_directory<nalSceneAnim, tlFixedString>::system_dir);
        }

    } else {
        CDECL_CALL(0x0051EC80, a1);
    }
}

resource_pack_slot *pop_resource_context()
{
    TRACE("resource_manager::pop_resource_context");

    if constexpr (1)
    {
        auto *old_context = get_resource_context();
        assert(old_context != nullptr);

#if 0 
        if (!resource_context_stack.empty())
        {
#ifndef TEST_CASE
            --resource_context_stack.m_last;
#else
            resource_context_stack.resize(resource_context_stack.size() - 1);
#endif
        }
    
#else
        sp_log("%d", resource_context_stack.size());
        resource_context_stack.pop_back();
        sp_log("%d", resource_context_stack.size());
#endif

        auto *v0 = get_resource_context();
        set_active_resource_context(v0);

        return old_context;
    } else {
        return (resource_pack_slot *) CDECL_CALL(0x00537530);
    }
}

void delete_inst() {
    TRACE("resource_manager::delete_inst");
    if constexpr (1)
    {
        if (amalgapak_pack_location_table != nullptr)
        {
            assert(amalgapak_pack_location_count > 0);

            mem_freealign(amalgapak_pack_location_table);
            amalgapak_pack_location_table = nullptr;
            nflCloseFile(amalgapak_id);
        }

        if (resource_buffer != nullptr) {
            mem_freealign(resource_buffer);
        }

        resource_buffer = nullptr;

        if (partitions != nullptr)
        {
            for (auto &part : (*partitions)) {
                if (part != nullptr) {
                    delete part;
                }
            }

            if (partitions != nullptr) {
                operator delete(partitions);
            }
        }

        partitions = nullptr;
        if (memory_maps_count > 0) {
            assert(memory_maps != nullptr);
        }

        release_memory_maps();
    }
    else
    {
        CDECL_CALL(0x00547AD0);
    }
}

void create_inst()
{
    TRACE("resource_manager::create_inst");

    if constexpr (1)
    {
        using vector_t = std::remove_pointer_t<std::decay_t<decltype(partitions)>>;
        partitions = new vector_t {};

        partitions->reserve(8u);

        in_use_memory_map = -1;
        amalgapak_base_offset = -1;
        amalgapak_id = NFL_FILE_ID_INVALID;
        memory_maps_count = 0;
        amalgapak_pack_location_count = 0;
        amalgapak_pack_location_table = nullptr;

        if (!g_is_the_packer())
        {
            load_amalgapak();
        }

        resource_buffer = static_cast<uint8_t *>(arch_memalign(4096u, resource_buffer_size));
        resource_buffer_used = 0;
        configure_packs_by_memory_map(0);

    }
    else
    {
        CDECL_CALL(0x0055BA30);
    }
}

void configure_packs_by_memory_map(int idx)
{
    TRACE("resource_manager::configure_packs_by_memory_map");

    assert(partitions != nullptr);

    {
        sp_log("--- begin ---");
        sp_log("in_use_memory_map = %d", in_use_memory_map);
        const auto partitions_size = partitions->size();
        sp_log("partitions_size = %u", partitions_size);

        sp_log("resource_buffer_used = %d", resource_buffer_used);
    }

    if constexpr (1)
    {
        const auto v14 = in_use_memory_map;
        int pop_start_idx = 0;

        const auto partitions_size = partitions->size();
        for (auto i = 0u; i < partitions_size; ++i) {
            auto func = [](const auto *self, const auto *a2) -> bool {
                return (self->field_0 == a2->field_0 && self->field_4 == a2->field_4
                        && self->field_8 == a2->field_8
                        && self->field_C == a2->field_C);
            };

            if (memory_maps[idx].field_10[i].field_4 == 1 &&
                func(&memory_maps[v14].field_10[i], &memory_maps[idx].field_10[i])) {
                ++pop_start_idx;
            }
        }

        for (int i = partitions_size - 1; i >= pop_start_idx; --i) {
            resource_buffer_used -= partitions->at(i)->partition_buffer_size;
            auto *part = partitions->back();
            assert(part != nullptr && part->get_streamer() != nullptr);

            auto *streamer = part->get_streamer();
            if (streamer->is_active()) {
                streamer->flush(nullptr);
                streamer->unload_all();
                streamer->flush(nullptr);
            }

            if (part != nullptr) {
                THISCALL(0x0053DFD0, part);
                operator delete(part);
                part = nullptr;
            }

            if (!partitions->empty()) {
#ifndef TEST_CASE
                --partitions->m_last;
#else
                partitions->resize(partitions->size() - 1);
#endif
            }
        }

        assert(static_cast<int>(partitions->size()) == pop_start_idx);

        for (uint32_t i = pop_start_idx; i < RESOURCE_PARTITION_END; ++i)
        {
            auto *new_partition = new resource_partition {static_cast<resource_partition_enum>(i)};

            auto &memory_map = memory_maps[idx];
            auto &tmp = memory_map.field_10[i];

            new_partition->field_0 = tmp.field_4;
            new_partition->partition_buffer_size = tmp.field_C *
                tmp.field_8;

            assert((new_partition->partition_buffer_size + resource_buffer_used <=
                    resource_buffer_size) &&
                   "Verify we have room for this partition");
        
            new_partition->partition_buffer_used = 0;
            new_partition->field_A8 = &resource_buffer[resource_buffer_used];
            resource_buffer_used += new_partition->partition_buffer_size;
            if (new_partition->field_0 >= 0 && new_partition->field_0 <= 1)
            {
                for (int j = 0; j < tmp.field_C; ++j) {
                    new_partition->push_pack_slot(tmp.field_8, nullptr);
                }
            }

            if constexpr (1)
            {
                if (partitions->size() < partitions->capacity())
                {
                    auto *v30 = partitions->m_last;
                    *v30 = new_partition;
                    partitions->m_last = v30 + 1;
                }
                else
                {
                    void (__fastcall *_Insert_n)(void *, void *edx, void *, int, resource_partition **) = CAST(_Insert_n, 0x0056A260);
                    _Insert_n(partitions, nullptr, partitions->m_last, 1, &new_partition);
                }
            }
            else
            {
                partitions->push_back(new_partition);
            }
        }

        assert(partitions->size() == RESOURCE_PARTITION_END &&
               "If this fails there's something wrong with the partition preserving code.");

        {
            auto begin = std::begin(memory_maps[idx].field_10);
            auto end = begin + RESOURCE_PARTITION_END;
            auto v7 = std::accumulate(begin, end, 0, [](auto prev_result, auto &v) {
                return v.field_C * v.field_8 + prev_result;
            });

            sp_log("Resource manager now using a memory map of size %d MB (%d KB)",
               v7 / 1024 / 1024,
               v7 / 1024);
        }

        in_use_memory_map = idx;
        set_active_resource_context(nullptr);
    }
    else
    {
        CDECL_CALL(0x00558930, idx);
    }

    {
        printf("\n");
        sp_log("--- end ---");

        sp_log("in_use_memory_map %d", in_use_memory_map);

        const auto partitions_size = partitions->size();
        sp_log("partitions_size = %u", partitions_size);

        sp_log("resource_buffer_used %d", resource_buffer_used);
    }
}

void set_active_district(bool a1)
{
    auto *district_partition = get_partition_pointer(RESOURCE_PARTITION_DISTRICT);
    assert(district_partition != nullptr);

    auto *district_streamer = district_partition->get_streamer();
    assert(district_streamer != nullptr);

    district_streamer->set_active(a1);
}

resource_partition *get_partition_pointer(resource_partition_enum which_type)
{
    assert(partitions != nullptr);
    assert(which_type >= 0 && which_type < static_cast<int>(partitions->size()));

    return partitions->at(which_type);
}

// openusm: ordered list of platform asset folders to try when opening a
// standalone pack.  Pack formats cannot be mixed inside one process: PC and
// Xbox builds use different resource-directory and mash layouts.  Keep each
// release variant locked to the format selected at compile time.
static int get_pack_search_order(_nlPlatformEnum out[2]) {
    int n = 0;
#ifdef OPENUSM_XBPACK_MODE
    out[n++] = NL_PLATFORM_XBOX;
#else
    out[n++] = g_platform;
#endif
    return n;
}

// openusm: build "data\packs\<dir>\<name><ext>" for a given platform slot,
// using the same packfile_dir()/packfile_ext() tables the stock open_pack
// used (so the on-disk layout is exactly what the game already expects:
// packs\xbox\NAME.XBPACK, packs\pc\NAME.PCPACK, ...).
static mString make_pack_path(const char *name, _nlPlatformEnum plat) {
    mString dir = mString{"data\\"} + mString{packfile_dir()[plat]};
    filespec spec{dir, mString{name}, mString{packfile_ext()[plat]}};
    return spec.fullname();
}

nflFileID open_pack_ex(const char *name, int *out_data_size) {
    TRACE("resource_manager::open_pack_ex", name);

    if (out_data_size != nullptr) {
        *out_data_size = 0;
    }

    _nlPlatformEnum order[2];
    int order_count = get_pack_search_order(order);

#ifdef OPENUSM_XBPACK_MODE
    // ultimate_release/xbpack builds may carry standalone Xbox packs in the
    // user-controlled extra folder.  Resolve these before the stock
    // data\packs\xbox location, but retain the normal pack-name lifecycle:
    // CHARACTERB_ARENA still loads only when the game asks for that pack.
    const mString extra_path =
        mString{"extra\\"} + mString{name} + mString{".XBPACK"};
    nflFileID extra_handle = nflOpenFile(1, extra_path.c_str());
    if (extra_handle == NFL_FILE_ID_INVALID)
        extra_handle = nflOpenFile(2, extra_path.c_str());
    if (extra_handle != NFL_FILE_ID_INVALID) {
        if (out_data_size != nullptr) {
            os_file probe;
            probe.open(extra_path, os_file::FILE_READ);
            if (probe.is_open()) {
                const int size = probe.get_size();
                if (size > 0)
                    *out_data_size = size;
                probe.close();
            }
        }
        sp_log("XBPACK override: %s -> %s", name, extra_path.c_str());
        return extra_handle;
    }
#endif

    for (int i = 0; i < order_count; ++i) {
        mString path = make_pack_path(name, order[i]);

        // Mirror the original open_pack media-id behaviour: host (1)
        // first, then CD (2).
        nflFileID handle = nflOpenFile(1, path.c_str());
        if (handle == NFL_FILE_ID_INVALID) {
            handle = nflOpenFile(2, path.c_str());
        }

        if (handle != NFL_FILE_ID_INVALID) {
            // Report the actual on-disk size so the streamer reads the
            // real file length. Every standalone pack location in the
            // amalga index has offset 0, so the index size is not needed
            // to position the read -- and reading the file's own length
            // is what lets an Xbox-indexed run fall back to a
            // differently sized .PCPACK without truncating it.
            if (out_data_size != nullptr) {
                os_file probe;
                probe.open(mString{path.c_str()}, os_file::FILE_READ);
                if (probe.is_open()) {
                    int sz = probe.get_size();
                    if (sz > 0) {
                        *out_data_size = sz;
                    }
                    probe.close();
                }
            }
            return handle;
        }
    }

    sp_log("Could not open packfile %s in any pack folder", name);
    return NFL_FILE_ID_INVALID;
}

nflFileID open_pack(const char *name) {
    return open_pack_ex(name, nullptr);
}

mString resolve_amalgapak_filename() {
#ifdef TARGET_XBOX
    // On real Xbox hardware keep the stock behaviour (always _XB.PAK).
    return get_amalgapak_filename(g_platform);
#else
    // Suffixes indexed by platform, matching get_amalgapak_filename().
    static const char *suffix[] = {".PAK", "_XB.PAK", "_GC.PAK", "_PC.PAK"};

    _nlPlatformEnum order[2];
    int order_count = get_pack_search_order(order);

    for (int i = 0; i < order_count; ++i) {
        mString candidate = mString{"packs\\amalga"} + mString{suffix[order[i]]};

        bool present = false;
        {
            os_file probe;
            probe.open(mString{candidate.c_str()}, os_file::FILE_READ);
            present = probe.is_open();
            if (present) {
                probe.close();
            }
        }
        sp_log("Probed amalga index: %s (platform=%d, present=%s)",
               candidate.c_str(), static_cast<int>(order[i]),
               present ? "yes" : "no");
        if (!present) {
            // Also try a data\-rooted copy, matching load_amalgapak's
            // secondary "data\\" lookup.
            os_file probe;
            mString rooted = mString{"data\\"} + candidate;
            probe.open(rooted, os_file::FILE_READ);
            present = probe.is_open();
            if (present) {
                probe.close();
            }
            sp_log("Probed rooted amalga index: %s (platform=%d, present=%s)",
                   rooted.c_str(), static_cast<int>(order[i]),
                   present ? "yes" : "no");
        }

        if (present) {
            selected_amalgapak_platform = order[i];
            sp_log("Selected amalga index: %s", candidate.c_str());
            return candidate;
        }
    }

    // Nothing present: return the native-platform name so the existing
    // "Could not open amalgapak file ..." error reports something sane.
    selected_amalgapak_platform = g_platform;
    return mString{"packs\\amalga"} + mString{suffix[g_platform]};
#endif
}

resource_pack_slot *get_resource_context()
{
    resource_pack_slot *result = nullptr;

    if (!resource_context_stack.empty()) {
        result = resource_context_stack.back();
    }

    return result;
}

bool get_resource_if_exists(const resource_key &resource_id,
                            [[maybe_unused]] void *a2,
                            uint8_t **a3,
                            worldly_pack_slot *slot_ptr,
                            int *mash_data_size)
{
    TRACE("resource_manager::get_resource_if_exists");

    assert(slot_ptr != nullptr);

    auto v6 = slot_ptr->get_resource(resource_id, mash_data_size, nullptr);

    // These file classes may be referenced by another loose resource even when
    // no active PCPACK has a directory entry for them.  Resolve the external
    // registry before the retail-null early return.
    if (resource_id.get_type() == RESOURCE_KEY_TYPE_ENTITY)
    {
        int overrideSize = 0;
        if (uint8_t *img = modEntGetOverride(
                resource_id.m_hash.source_hash_code, &overrideSize))
        {
            if (mash_data_size != nullptr)
                *mash_data_size = overrideSize;
            *a3 = img;
            return true;
        }
    }

    if (resource_id.get_type() == RESOURCE_KEY_TYPE_ALS_FILE)
    {
        int overrideSize = 0;
        if (uint8_t *img = modAlsGetOverride(
                resource_id.m_hash.source_hash_code, &overrideSize))
        {
            if (mash_data_size != nullptr)
                *mash_data_size = overrideSize;
            *a3 = img;
            return true;
        }
    }

    // DDS/PCSKEL are fallback-only in this path: a real PCPACK entry stays
    // authoritative.  Loose files are consulted only when the active pack has
    // no resource with this key.  This is the same external-only behavior used
    // for resources that exist solely under extra/.
    if (v6 == nullptr && resource_id.get_type() == RESOURCE_KEY_TYPE_TEXTURE)
    {
        int externalSize = 0;
        if (uint8_t *img = modGetLooseTLBytesByExtension(
                resource_id.m_hash.source_hash_code, TLRESOURCE_TYPE_TEXTURE,
                ".dds", &externalSize))
        {
            if (mash_data_size != nullptr)
                *mash_data_size = externalSize;
            *a3 = img;
            sp_log("[mod][resource_manager] external-only DDS found for %s "
                   "(0x%08X, %d bytes; no PCPACK entry)",
                   resource_id.m_hash.to_string(),
                   resource_id.m_hash.source_hash_code, externalSize);
            return true;
        }
    }

    if (v6 == nullptr && resource_id.get_type() == RESOURCE_KEY_TYPE_NAL_SKL)
    {
        int externalSize = 0;
        if (uint8_t *img = modGetLooseTLBytesByExtension(
                resource_id.m_hash.source_hash_code, TLRESOURCE_TYPE_SKELETON,
                ".pcskel", &externalSize))
        {
            if (mash_data_size != nullptr)
                *mash_data_size = externalSize;
            *a3 = img;
            sp_log("[mod][resource_manager] external-only PCSKEL found for %s "
                   "(0x%08X, %d bytes; no PCPACK entry)",
                   resource_id.m_hash.to_string(),
                   resource_id.m_hash.source_hash_code, externalSize);
            return true;
        }
    }

    // Explicit raw PCMesh/XBMesh override. Unlike DDS/PCSKEL fallback resources,
    // extra/<name> is a drop-in mesh-file replacement: when it exists it
    // must win even if the active PCPACK also contains the same mesh key.
    //
    // Return the PRISTINE Mod::Data bytes here. nglLoadMeshFileInternal will
    // immediately clone them through tlMemAlloc before the retail parser rebases
    // offsets in place. This keeps the registry immutable and gives FileBuf the
    // same allocator ownership expected by tlReleaseFile().
    if (resource_id.get_type() == RESOURCE_KEY_TYPE_MESH)
    {
        int externalSize = 0;
        const uint8_t *external = modMeshGetOverride(
            resource_id.m_hash.source_hash_code, &externalSize);
        if (external != nullptr
            && externalSize > 0
            && modMeshDetectTLType(external, static_cast<size_t>(externalSize),
                                     TLRESOURCE_TYPE_MESH_FILE)
                == TLRESOURCE_TYPE_MESH_FILE)
        {
            if (mash_data_size != nullptr)
                *mash_data_size = externalSize;
            *a3 = const_cast<uint8_t *>(external);
            sp_log("[mod][resource_manager] native mesh override selected for %s "
                   "(0x%08X, %d bytes; packed=%s)",
                   resource_id.m_hash.to_string(),
                   resource_id.m_hash.source_hash_code, externalSize,
                   v6 != nullptr ? "yes" : "no");
            return true;
        }
    }

    if (resource_id.get_type() == RESOURCE_KEY_TYPE_ANIMATION
        || resource_id.get_type() == RESOURCE_KEY_TYPE_SCENE_ANIM)
    {
        const bool sceneFlavor =
            resource_id.get_type() == RESOURCE_KEY_TYPE_SCENE_ANIM;
        const int originalSize = (v6 != nullptr && mash_data_size != nullptr)
                               ? *mash_data_size : 0;
        int overrideSize = 0;
        uint8_t *src = modPCANIMGetOverride(
            resource_id.m_hash.source_hash_code, &overrideSize,
            v6, originalSize, sceneFlavor);
        if (src != nullptr)
        {
            uint8_t *img = modCloneExternalSerializedImage(src, overrideSize);
            if (img != nullptr)
            {
                modPCANIMTrackExternalImage(
                    img, resource_id.m_hash.source_hash_code,
                    overrideSize, sceneFlavor);
                if (mash_data_size != nullptr)
                    *mash_data_size = overrideSize;
                *a3 = img;
                return true;
            }
        }
    }

    // Keep BASE_AI loose-file behavior consistent with get_resource(). Some
    // gameplay paths use this "if exists" helper instead of the global
    // context getter, so consult the typed BAI registry even when the retail
    // slot has no same-name resource.
    if (resource_id.get_type() == RESOURCE_KEY_TYPE_BASE_AI)
    {
        int overrideSize = 0;
        if (uint8_t *img = modBaiGetOverride(
                resource_id.m_hash.source_hash_code, &overrideSize))
        {
            if (mash_data_size != nullptr)
                *mash_data_size = overrideSize;
            *a3 = img;
            return true;
        }
    }

    // State graphs can be queried through this helper too. Keep loose ASG
    // behavior identical to get_resource(), including external-only graphs.
    if (resource_id.get_type() == RESOURCE_KEY_TYPE_AI_STATE_GRAPH)
    {
        int overrideSize = 0;
        if (uint8_t *img = modAsgGetOverride(
                resource_id.m_hash.source_hash_code, &overrideSize))
        {
            if (mash_data_size != nullptr)
                *mash_data_size = overrideSize;
            *a3 = img;
            return true;
        }
    }

    // Mission tables and panels are serialized images whose consumers perform
    // in-place un-mashing. Return a fresh writable copy even for external-only
    // resources so repeated loads never see previously rebased pointers.
    if (resource_id.get_type() == RESOURCE_KEY_TYPE_MISSION_TABLE)
    {
        int overrideSize = 0;
        if (uint8_t *img = modMsnGetOverride(
                resource_id.m_hash.source_hash_code, &overrideSize))
        {
            if (mash_data_size != nullptr)
                *mash_data_size = overrideSize;
            *a3 = img;
            return true;
        }
    }

    if (resource_id.get_type() == RESOURCE_KEY_TYPE_PANEL)
    {
        int overrideSize = 0;
        if (uint8_t *img = modPanelGetOverride(
                resource_id.m_hash.source_hash_code, &overrideSize))
        {
            if (mash_data_size != nullptr)
                *mash_data_size = overrideSize;
            *a3 = img;
            return true;
        }
    }

    // External-only serialized engine resources. Check these before the
    // retail-null early return so references from other loose assets resolve
    // even when the active PCPACK has no directory entry for the resource.
    if (resource_id.get_type() == RESOURCE_KEY_TYPE_COLLISION_MESH)
    {
        int overrideSize = 0;
        if (uint8_t *img = modCollGetOverride(
                resource_id.m_hash.source_hash_code, &overrideSize))
        {
            if (mash_data_size != nullptr)
                *mash_data_size = overrideSize;
            *a3 = img;
            return true;
        }
    }

    if (resource_id.get_type() == RESOURCE_KEY_TYPE_CUT_SCENE)
    {
        int overrideSize = 0;
        if (uint8_t *img = modCutGetOverride(
                resource_id.m_hash.source_hash_code, &overrideSize))
        {
            if (mash_data_size != nullptr)
                *mash_data_size = overrideSize;
            *a3 = img;
            return true;
        }
    }

    if (resource_id.get_type() == RESOURCE_KEY_TYPE_MESH_FILE_STRUCT)
    {
        int overrideSize = 0;
        if (uint8_t *img = modPcmeshdefGetOverride(
                resource_id.m_hash.source_hash_code, &overrideSize))
        {
            if (mash_data_size != nullptr)
                *mash_data_size = overrideSize;
            *a3 = img;
            return true;
        }
    }

    if (resource_id.get_type() == RESOURCE_KEY_TYPE_SLF_LIST)
    {
        int overrideSize = 0;
        if (uint8_t *img = modSlfGetOverride(
                resource_id.m_hash.source_hash_code, &overrideSize))
        {
            if (mash_data_size != nullptr)
                *mash_data_size = overrideSize;
            *a3 = img;
            return true;
        }
    }

    if (v6 == nullptr) {
        return false;
    }

    *a3 = v6;
    return true;
}

uint8_t *get_resource(const resource_key &resource_id, int *mash_data_size, resource_pack_slot **a3)
{
    TRACE("resource_manager::get_resource", resource_id.get_platform_string(g_platform).c_str());

    if constexpr (1)
    {
        assert(!g_is_the_packer() && "Don't call this function while packing!");
     //   assert(resource_id.is_set());
        assert(get_resource_context() != nullptr && "Can't get a resource without a context!");
        assert(get_resource_context()->is_data_ready() && "Invalid resource context");

        // Keep the retail byte count even when the caller did not request it:
        // PCANIM compatibility checks need the exact bounds of the packed
        // source image before an external file may replace it.
        int localMashDataSize = 0;
        int *actualMashDataSize = mash_data_size != nullptr
                                ? mash_data_size : &localMashDataSize;
        auto *result = get_resource_context()->get_resource(
            resource_id, actualMashDataSize, a3);

        // Explicit PCMesh/XBMesh override. The loose file intentionally wins over
        // a same-name PCPACK resource. Publish only pristine registry bytes;
        // NGL clones them into engine-owned writable memory before parsing.
        if (resource_id.get_type() == RESOURCE_KEY_TYPE_MESH)
        {
            int externalSize = 0;
            const uint8_t *external = modMeshGetOverride(
                resource_id.m_hash.source_hash_code, &externalSize);

            if (external != nullptr
                && externalSize > 0
                && modMeshDetectTLType(external, static_cast<size_t>(externalSize),
                                         TLRESOURCE_TYPE_MESH_FILE)
                    == TLRESOURCE_TYPE_MESH_FILE)
            {
                const bool replacedPacked = result != nullptr;
                result = const_cast<uint8_t *>(external);
                *actualMashDataSize = externalSize;
                if (a3 != nullptr)
                    *a3 = nullptr;

                sp_log("[mod][resource_manager] serving native mesh override \"%s\" "
                       "(hash 0x%08X, %d bytes; replaced packed=%s); "
                       "NGL will clone+parse it",
                       resource_id.m_hash.to_string(),
                       resource_id.m_hash.source_hash_code,
                       externalSize,
                       replacedPacked ? "yes" : "no");
            }
        }

        // Loose .DDS is fallback-only here.  If the pack already has the
        // texture, leave its bytes and owner untouched.  Only synthesize a
        // resource from extra/<name>.dds when the PCPACK lookup failed.
        if (result == nullptr && resource_id.get_type() == RESOURCE_KEY_TYPE_TEXTURE)
        {
            int externalSize = 0;
            if (uint8_t *img = modGetLooseTLBytesByExtension(
                    resource_id.m_hash.source_hash_code, TLRESOURCE_TYPE_TEXTURE,
                    ".dds", &externalSize))
            {
                *actualMashDataSize = externalSize;
                if (a3 != nullptr)
                    *a3 = nullptr;
                result = img;
                sp_log("[mod][resource_manager] serving external-only DDS for \"%s\" "
                       "(0x%08X, %d bytes; no PCPACK entry)",
                       resource_id.m_hash.to_string(),
                       resource_id.m_hash.source_hash_code, externalSize);
            }
        }

        // Same policy for .PCSKEL.  Packed skeletons remain authoritative; a
        // private writable loose image is returned only for a missing pack key.
        if (result == nullptr && resource_id.get_type() == RESOURCE_KEY_TYPE_NAL_SKL)
        {
            int externalSize = 0;
            if (uint8_t *img = modGetLooseTLBytesByExtension(
                    resource_id.m_hash.source_hash_code, TLRESOURCE_TYPE_SKELETON,
                    ".pcskel", &externalSize))
            {
                *actualMashDataSize = externalSize;
                if (a3 != nullptr)
                    *a3 = nullptr;
                result = img;
                sp_log("[mod][resource_manager] serving external-only PCSKEL for \"%s\" "
                       "(0x%08X, %d bytes; no PCPACK entry)",
                       resource_id.m_hash.to_string(),
                       resource_id.m_hash.source_hash_code, externalSize);
            }
        }

        // External animation banks are selected by the requested resource
        // hash (regular banks often embed the shared name "allanims"). Packed
        // resources retain their original TL shell, while an external-only
        // PCANIM/PCSANIM receives a private writable shell and no pack owner.
        if (resource_id.get_type() == RESOURCE_KEY_TYPE_ANIMATION
            || resource_id.get_type() == RESOURCE_KEY_TYPE_SCENE_ANIM)
        {
            const bool sceneFlavor =
                resource_id.get_type() == RESOURCE_KEY_TYPE_SCENE_ANIM;
            const int expectedTLType = sceneFlavor
                                     ? TLRESOURCE_TYPE_SCENE_ANIM
                                     : TLRESOURCE_TYPE_ANIM_FILE;
            const bool externalOnly = (result == nullptr);
            const int retailSize = externalOnly ? 0 : *actualMashDataSize;

            bool retailCompatible = true;
            if (!externalOnly)
            {
                retailCompatible = retailSize >= 0x70
                    && modPCANIMDetectTLType(
                        result, static_cast<size_t>(retailSize), expectedTLType)
                       == expectedTLType;
                if (!retailCompatible)
                {
                    sp_log("[mod] %s resource 0x%08X has an invalid/wrong NAL flavor; "
                           "override bridge skipped",
                           sceneFlavor ? "scene-PCANIM" : "PCANIM",
                           resource_id.m_hash.source_hash_code);
                }
            }

            if (retailCompatible)
            {
                int overrideSize = 0;
                uint8_t *img = nullptr;
                const char *overrideKind = nullptr;

                if (!sceneFlavor)
                {
                    img = modPS2ANIMGetOverride(
                        resource_id.m_hash.source_hash_code, &overrideSize,
                        externalOnly ? nullptr : result, retailSize);
                    if (img != nullptr)
                        overrideKind = "PS2ANIM";
                }

                if (img == nullptr)
                {
                    img = modPCANIMGetOverride(
                        resource_id.m_hash.source_hash_code, &overrideSize,
                        externalOnly ? nullptr : result, retailSize, sceneFlavor);
                    if (img != nullptr)
                        overrideKind = sceneFlavor ? "scene-PCANIM" : "PCANIM";
                }

                if (img != nullptr)
                {
                    if (externalOnly)
                    {
                        uint8_t *shell =
                            modCloneExternalSerializedImage(img, overrideSize);
                        if (shell != nullptr)
                        {
                            modPCANIMTrackExternalImage(
                                shell, resource_id.m_hash.source_hash_code,
                                overrideSize, sceneFlavor);
                            *actualMashDataSize = overrideSize;
                            if (a3 != nullptr)
                                *a3 = nullptr;
                            result = shell;
                            sp_log("[mod] serving external-only %s for \"%s\" "
                                   "(0x%08X, %d bytes; no PCPACK entry)",
                                   overrideKind != nullptr ? overrideKind : "animation",
                                   resource_id.m_hash.to_string(),
                                   resource_id.m_hash.source_hash_code, overrideSize);
                        }
                    }
                    else
                    {
                        // Keep the pack-owned shell. The NAL wrapper parses a
                        // private writable copy and publishes its lists onto it.
                        sp_log("[mod] validated %s override for \"%s\" (%d bytes)",
                               overrideKind != nullptr ? overrideKind : "animation",
                               resource_id.m_hash.to_string(), overrideSize);
                    }
                }
            }
        }

        // Loose .ALS override (entity.cpp).  ALS resources are raw
        // animation_logic_system_shared mash streams, not generic-mash
        // images. modAlsGetOverride owns a private writable copy and performs
        // the exact un-mash + construct sequence that als_resource_handler
        // performs for packed ALS bytes before publishing the pointer here.
        //
        // Unlike .ENT, an ALS may be injected even when the active pack does
        // not contain a same-name resource: the entity's als_res_data already
        // carries the RESOURCE_KEY_TYPE_ALS_FILE key and only needs a live
        // animation_logic_system_shared pointer back.
        if (resource_id.get_type() == RESOURCE_KEY_TYPE_ALS_FILE)
        {
            const uint32_t alsHash = resource_id.m_hash.source_hash_code;
            const bool externalOnly = (result == nullptr);
            int overrideSize = 0;
            if (uint8_t *img = modAlsGetOverride(alsHash, &overrideSize))
            {
                sp_log("[mod][resource_manager] serving ALS override for \"%s\" "
                       "(0x%08X, %d bytes)%s",
                       resource_id.m_hash.to_string(), alsHash, overrideSize,
                       externalOnly ? " [external-only]" : "");

                *actualMashDataSize = overrideSize;
                if (externalOnly && a3 != nullptr)
                    *a3 = nullptr;
                result = img;
            }
        }

        // Loose .BAI override.  BASE_AI is a raw ai::core_ai_resource mash
        // stream.  modBaiGetOverride keeps Mod::Data pristine, prepares a
        // 16-byte aligned writable image and performs the same un-mash +
        // construct sequence as base_ai_resource_handler before returning it.
        // This can also satisfy a BASE_AI key that is referenced by an entity
        // but absent from the active retail pack.
        if (resource_id.get_type() == RESOURCE_KEY_TYPE_BASE_AI)
        {
            const uint32_t baiHash = resource_id.m_hash.source_hash_code;
            const bool externalOnly = (result == nullptr);
            int overrideSize = 0;
            if (uint8_t *img = modBaiGetOverride(baiHash, &overrideSize))
            {
                sp_log("[mod][resource_manager] serving BAI override for \"%s\" "
                       "(0x%08X, %d bytes)%s",
                       resource_id.m_hash.to_string(), baiHash, overrideSize,
                       externalOnly ? " [external-only]" : "");

                *actualMashDataSize = overrideSize;
                if (externalOnly && a3 != nullptr)
                    *a3 = nullptr;
                result = img;
            }
        }

        // Loose .ASG override. AI_STATE_GRAPH is a raw ai::state_graph mash
        // stream. modAsgGetOverride owns a private 16-byte aligned writable
        // image and performs the same un-mash + construct sequence as
        // ai_state_graph_resource_handler before publishing it. A graph may
        // therefore live only in extra/ and still satisfy a BASE_AI reference.
        if (resource_id.get_type() == RESOURCE_KEY_TYPE_AI_STATE_GRAPH)
        {
            const uint32_t asgHash = resource_id.m_hash.source_hash_code;
            const bool externalOnly = (result == nullptr);
            int overrideSize = 0;
            if (uint8_t *img = modAsgGetOverride(asgHash, &overrideSize))
            {
                sp_log("[mod][resource_manager] serving ASG override for \"%s\" "
                       "(0x%08X, %d bytes)%s",
                       resource_id.m_hash.to_string(), asgHash, overrideSize,
                       externalOnly ? " [external-only]" : "");

                *actualMashDataSize = overrideSize;
                if (externalOnly && a3 != nullptr)
                    *a3 = nullptr;
                result = img;
            }
        }

        // Loose .MSN override. MISSION_TABLE is still serialized generic-mash
        // data at this layer; mission_manager parses and rebases it afterwards.
        // Always return a fresh writable image so header/vector mutations from
        // a previous mission-table parse cannot leak into a later load.
        if (resource_id.get_type() == RESOURCE_KEY_TYPE_MISSION_TABLE)
        {
            const uint32_t msnHash = resource_id.m_hash.source_hash_code;
            const bool externalOnly = (result == nullptr);
            int overrideSize = 0;
            if (uint8_t *img = modMsnGetOverride(msnHash, &overrideSize))
            {
                sp_log("[mod][resource_manager] serving MSN override for \"%s\" "
                       "(0x%08X, %d bytes)%s",
                       resource_id.m_hash.to_string(), msnHash, overrideSize,
                       externalOnly ? " [external-only]" : "");
                *actualMashDataSize = overrideSize;
                if (externalOnly && a3 != nullptr)
                    *a3 = nullptr;
                result = img;
            }
        }

        // Loose .PANEL override. PanelFile::UnmashPanelFile() owns the actual
        // un-mash/construct step, so resource_manager supplies untouched but
        // writable serialized bytes. A fresh copy is required on every call.
        if (resource_id.get_type() == RESOURCE_KEY_TYPE_PANEL)
        {
            const uint32_t panelHash = resource_id.m_hash.source_hash_code;
            const bool externalOnly = (result == nullptr);
            int overrideSize = 0;
            if (uint8_t *img = modPanelGetOverride(panelHash, &overrideSize))
            {
                sp_log("[mod][resource_manager] serving PANEL override for \"%s\" "
                       "(0x%08X, %d bytes)%s",
                       resource_id.m_hash.to_string(), panelHash, overrideSize,
                       externalOnly ? " [external-only]" : "");
                *actualMashDataSize = overrideSize;
                if (externalOnly && a3 != nullptr)
                    *a3 = nullptr;
                result = img;
            }
        }

        // Loose .COLL override. cg_mesh::_un_mash validates and marks the
        // signature in place, so it receives a private writable serialized
        // image. This also supports a COLL referenced by a loose ENT even when
        // that collision resource has no PCPACK directory entry.
        if (resource_id.get_type() == RESOURCE_KEY_TYPE_COLLISION_MESH)
        {
            const uint32_t collHash = resource_id.m_hash.source_hash_code;
            const bool externalOnly = (result == nullptr);
            int overrideSize = 0;
            if (uint8_t *img = modCollGetOverride(collHash, &overrideSize))
            {
                sp_log("[mod][resource_manager] serving COLL override for \"%s\" "
                       "(0x%08X, %d bytes)%s",
                       resource_id.m_hash.to_string(), collHash, overrideSize,
                       externalOnly ? " [external-only]" : "");
                *actualMashDataSize = overrideSize;
                if (externalOnly && a3 != nullptr)
                    *a3 = nullptr;
                result = img;
            }
        }

        // Loose .CUT serialized cut_scene image. The cut-scene loader un-mashes
        // this payload in place, therefore Mod::Data itself is never exposed.
        if (resource_id.get_type() == RESOURCE_KEY_TYPE_CUT_SCENE)
        {
            const uint32_t cutHash = resource_id.m_hash.source_hash_code;
            const bool externalOnly = (result == nullptr);
            int overrideSize = 0;
            if (uint8_t *img = modCutGetOverride(cutHash, &overrideSize))
            {
                sp_log("[mod][resource_manager] serving CUT override for \"%s\" "
                       "(0x%08X, %d bytes)%s",
                       resource_id.m_hash.to_string(), cutHash, overrideSize,
                       externalOnly ? " [external-only]" : "");
                *actualMashDataSize = overrideSize;
                if (externalOnly && a3 != nullptr)
                    *a3 = nullptr;
                result = img;
            }
        }

        // Loose .PCMESHDEF generic-mash image paired with a mesh-file key.
        // Keep a pristine master because generic-mash parsing rebases pointers
        // and flips header state in the writable image.
        if (resource_id.get_type() == RESOURCE_KEY_TYPE_MESH_FILE_STRUCT)
        {
            const uint32_t defHash = resource_id.m_hash.source_hash_code;
            const bool externalOnly = (result == nullptr);
            int overrideSize = 0;
            if (uint8_t *img = modPcmeshdefGetOverride(defHash, &overrideSize))
            {
                sp_log("[mod][resource_manager] serving PCMESHDEF override for \"%s\" "
                       "(0x%08X, %d bytes)%s",
                       resource_id.m_hash.to_string(), defHash, overrideSize,
                       externalOnly ? " [external-only]" : "");
                *actualMashDataSize = overrideSize;
                if (externalOnly && a3 != nullptr)
                    *a3 = nullptr;
                result = img;
            }
        }

        // Loose .SLF table used by slc_manager::un_mash_all_funcs(). The file
        // is structurally validated at enumeration time and may be external-only.
        if (resource_id.get_type() == RESOURCE_KEY_TYPE_SLF_LIST)
        {
            const uint32_t slfHash = resource_id.m_hash.source_hash_code;
            const bool externalOnly = (result == nullptr);
            int overrideSize = 0;
            if (uint8_t *img = modSlfGetOverride(slfHash, &overrideSize))
            {
                sp_log("[mod][resource_manager] serving SLF override for \"%s\" "
                       "(0x%08X, %d bytes)%s",
                       resource_id.m_hash.to_string(), slfHash, overrideSize,
                       externalOnly ? " [external-only]" : "");
                *actualMashDataSize = overrideSize;
                if (externalOnly && a3 != nullptr)
                    *a3 = nullptr;
                result = img;
            }
        }

        // .ENT mod override (entity_base.cpp). The loose registry owns an
        // immortal writable generic-mash image, so a class absent from every
        // active PCPACK can now be injected exactly like an external PCSX.
        // ENTITY only: .ENTEXT shares the name hash but expects another payload.
        if (resource_id.get_type() == RESOURCE_KEY_TYPE_ENTITY)
        {
            const uint32_t entHash = resource_id.m_hash.source_hash_code;
            const bool ps2BetaPreview = modEntIsPS2BetaPreviewHash(entHash);
            const bool externalOnly = (result == nullptr);
            int overrideSize = 0;
            if (uint8_t *img = modEntGetOverride(entHash, &overrideSize))
            {
                sp_log("[mod] serving %s ENT override for \"%s\" "
                       "(0x%08X, %d bytes)%s",
                       ps2BetaPreview ? "PS2 beta-preview" : "PC",
                       resource_id.m_hash.to_string(), entHash, overrideSize,
                       externalOnly ? " [external-only; no PCPACK entry]" : "");
                *actualMashDataSize = overrideSize;
                if (externalOnly && a3 != nullptr)
                    *a3 = nullptr;
                result = img;
            }
        }

        // PCSX / translated PS2SX override (script_object.cpp):
        // script_manager::load and
        // script_manager::is_loadable fetch every script-executable blob
        // through here, so a validated external image replaces the retail
        // bytes at the one spot that knows the canonical script-name key.
        // Unlike the .ENT case above no pack slot is involved in the exec's
        // lifetime — it is governed by script_manager's exec map plus
        // release_generic_mash on the image itself — so a script absent
        // from every loaded pack CAN be injected: the override also makes
        // is_loadable() report it, which is what lets brand-new scripts
        // load. SCRIPT only: SCRIPT_INST/GV/SV requests share the name hash
        // but expect different payloads, so handing them this image would
        // be wrong.
        if (resource_id.get_type() == RESOURCE_KEY_TYPE_SCRIPT)
        {
            const uint32_t nameHash = resource_id.m_hash.source_hash_code;
            const bool translatedPS2SX = modPS2SXOverrideSelected(nameHash);
            int overrideSize = 0;
            if (uint8_t *img = modPCSXGetOverride(nameHash, &overrideSize))
            {
                const bool externalOnly = (result == nullptr);
                sp_log("[mod] serving %s override for \"%s\" (0x%08X, %d bytes)%s",
                       translatedPS2SX ? "translated ps2sx" : "pcsx",
                       resource_id.m_hash.to_string(), nameHash, overrideSize,
                       externalOnly ? " [not in any pack - injected as new]" : "");

                // Keep the local size valid even when the public caller passed
                // mash_data_size == nullptr.  For a script that exists only in
                // extra/, explicitly report no pack owner: the runtime image is
                // owned by the PCSX registry/script manager, not by a PCPACK slot.
                *actualMashDataSize = overrideSize;
                if (externalOnly && a3 != nullptr)
                    *a3 = nullptr;
                result = img;
            }
        }

        return result;
    }
    else
    {
        uint8_t * (* func)(const resource_key *, int *, resource_pack_slot **) = CAST(func, 0x00531B30);
        return func(&resource_id, mash_data_size, a3);
    }
}

} // namespace resource_manager

void resource_manager_patch()
{
    SET_JUMP(0x00542740, resource_manager::push_resource_context);

    SET_JUMP(0x00537530, resource_manager::pop_resource_context);

    // Route every retail get_resource caller (the dynamic-spawn body at
    // 0x005E0A10, fx caches at 0x00594836, ...) through the reimplementation
    // above so the .ENT mod override sees all of them. The reimplementation
    // is complete (context->get_resource -> retail 0x0052AA70) and never
    // calls back into 0x00531B30, so the detour cannot recurse.
    SET_JUMP(0x00531B30, resource_manager::get_resource);

    {
        resource_pack_slot * (* func)(resource_pack_slot *) = &resource_manager::get_best_context;
        REDIRECT(0x00542A04, func);
    }

    //REDIRECT(0x0055A6E1, resource_manager::get_resource_if_exists);

    REDIRECT(0x005D70A6, resource_manager::frame_advance);

    SET_JUMP(0x0052A820, resource_manager::get_pack_file_stats);

    SET_JUMP(0x00537650, resource_manager::load_amalgapak);

    SET_JUMP(0x0055BA30, resource_manager::create_inst);

    SET_JUMP(0x00547AD0, resource_manager::delete_inst);

    SET_JUMP(0x0054C2E0, resource_manager::reload_amalgapak);

    SET_JUMP(0x0053DE90, resource_manager::can_reload_amalgapak);

    SET_JUMP(0x0051ED70, resource_manager::get_pack_location);

    {
        REDIRECT(0x0055A371, resource_manager::configure_packs_by_memory_map);
    }
}


void resource_manager2_patch()
{

    SET_JUMP(0x00531B30, resource_manager::get_resource);


    
}

void resource_manager_xbpack_patch()
{
#ifdef OPENUSM_XBPACK_MODE
    SET_JUMP(0x00537650, resource_manager::load_amalgapak);
    SET_JUMP(0x0052A820, resource_manager::get_pack_file_stats);
    SET_JUMP(0x0055DEA0, compare_resource_key_resource_pack_location);
#endif
}
