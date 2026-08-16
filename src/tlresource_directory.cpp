#include "tlresource_directory.h"

#include "common.h"
#include "memory.h"
#include "debugutil.h"
#include "func_wrapper.h"
#include "nal_system.h"
#include "nal_anim.h"  // complete nalAnimClass<nalAnyPose>; needed for clip->field_4
#include "ngl.h"
#include "ngl_mesh.h"
#include "os_developer_options.h"
#include "resource_directory.h"
#include "trace.h"
#include "tlresource_location.h"
#include "utility.h"
#include "vtbl.h"

#include "variables.h"

#include <cassert>
#include <algorithm>
#include <cstring>
#include <map>
#include <set>
#include <string>
#include <vector>

#ifndef TEST_CASE

#define make_var(T0, T1, address) \
template<> \
tlInstanceBankResourceDirectory<T0, T1> *& \
    tlresource_directory<T0, T1>::system_dir = \
    var<tlInstanceBankResourceDirectory<T0, T1> *>(address)

make_var(nglTexture, tlFixedString, 0x00960A10);

make_var(nglMesh, tlHashString, 0x00960A08);

make_var(nglMeshFile, tlFixedString, 0x00960A0C);

make_var(nglMorphSet, tlHashString, 0x00960A00);

make_var(nglMaterialFile, tlFixedString, 0x009609FC);

make_var(nglMaterialBase, tlHashString, 0x009609F8);

#undef make_var

#else

template<typename T0, typename T1>
static auto & make_system_dir()
{
    static tlInstanceBankResourceDirectory<T0, T1> *g_system_dir;
    return g_system_dir;
}

#define make_var(T0, T1) \
template<> \
tlInstanceBankResourceDirectory<T0, T1> *& \
    tlresource_directory<T0, T1>::system_dir { \
        make_system_dir<T0, T1>()}

make_var(nglTexture, tlFixedString);

make_var(nglMesh, tlHashString);

make_var(nglMeshFile, tlFixedString);

make_var(nglMorphSet, tlHashString);

make_var(nglMaterialFile, tlFixedString);

make_var(nglMaterialBase, tlHashString);

#undef make_var

#endif

#define make_var(T0, T1, address) \
template<> \
T0 *& tlresource_directory<T0, T1>::default_tlres = var<T0 *>(address)

make_var(nglMesh, tlHashString, 0x009609DC);

make_var(nalBaseSkeleton, tlFixedString, 0x009609BC);

make_var(nglMeshFile, tlFixedString, 0x009609E0);

make_var(nglTexture, tlFixedString, 0x009609E4);

make_var(nalAnimClass<nalAnyPose>, tlFixedString, 0x009609C4);

#undef make_var

template<>
int tlresource_directory<nglMesh, tlHashString>::tlres_type = 3;


// PCANIM loose-file bridge ---------------------------------------------------
//
// Regular PCANIM headers use the shared embedded name "allanims", so a loose
// file such as extra/ULTIMATE_SPIDERMAN.PCANIM cannot reliably be selected by
// the serialized header name alone.  The animation directory already owns the
// final clip lookup (vfunc 0x008897B0), therefore load loose PCANIM banks once
// here and let this directory prefer their parsed clips over the retail clip.
//
// The parsed images are intentionally kept for the process lifetime.  NAL
// animation clips contain pointers back into the rebased PCANIM image; freeing
// the image after parsing would leave the directory with dangling pointers.

void modScanNalOverrides();

namespace {

struct tlLoosePCAnimBank
{
    nalAnimFile *image = nullptr;
    std::filesystem::path path;
    uint32_t targetHash = 0;
};

static std::vector<tlLoosePCAnimBank> &tlLoosePCAnimBanks()
{
    static std::vector<tlLoosePCAnimBank> banks;
    return banks;
}

static bool &tlLoosePCAnimLoadInProgress()
{
    static bool value = false;
    return value;
}

static uint32_t tlReadU32(const uint8_t *data, size_t offset)
{
    uint32_t value = 0;
    std::memcpy(&value, data + offset, sizeof(value));
    return value;
}

static bool tlLoosePCAnimImageLooksValid(const std::vector<uint8_t> &data,
                                         size_t &clipCount)
{
    clipCount = 0;
    if (data.size() < 0x70 || tlReadU32(data.data(), 0x0) != 0x10101u)
        return false;

    const uint32_t numSkeletons = tlReadU32(data.data(), 0xC);
    if (numSkeletons == 0 || numSkeletons > 128u
        || tlReadU32(data.data(), 0x8) != numSkeletons * 0x20u)
        return false;

    size_t offset = tlReadU32(data.data(), 0x34);
    const size_t expectedFirst =
        (0x48u + size_t(numSkeletons) * 0x20u + 15u) & ~size_t(15u);
    if (offset != expectedFirst || offset > data.size()
        || data.size() - offset < 0x40u)
        return false;

    // Validate the forward-linked clip list before handing it to native NAL.
    const size_t hardLimit = data.size() / 0x40u + 1u;
    for (;;)
    {
        if (offset > data.size() || data.size() - offset < 0x40u)
            return false;

        // tlFixedString at +0x08 must at least carry a non-zero hash.
        if (tlReadU32(data.data(), offset + 0x08u) == 0)
            return false;

        ++clipCount;
        if (clipCount > hardLimit)
            return false;

        const uint32_t nextRelative = tlReadU32(data.data(), offset + 0x04u);
        if (nextRelative == 0)
            break;
        if (nextRelative < 0x40u || (nextRelative & 3u) != 0
            || nextRelative > data.size() - offset
            || data.size() - (offset + nextRelative) < 0x40u)
            return false;

        offset += nextRelative;
    }

    return clipCount != 0;
}

static void tlStampLoosePCAnimTargetName(uint8_t *image,
                                         size_t size,
                                         const std::string &lowerStem)
{
    // PCANIM field_10 is a 0x20-byte tlFixedString: hash + 28-byte text.
    if (image == nullptr || size < 0x30u)
        return;

    const uint32_t hash = to_hash(lowerStem.c_str());
    std::memcpy(image + 0x10u, &hash, sizeof(hash));
    std::memset(image + 0x14u, 0, 28u);
    const size_t copyLen = std::min<size_t>(27u, lowerStem.size());
    std::memcpy(image + 0x14u, lowerStem.data(), copyLen);
}

static void tlLoadLoosePCAnimBanksForDirectory()
{
    static bool attempted = false;
    if (attempted || tlLoosePCAnimLoadInProgress())
        return;

    attempted = true;
    tlLoosePCAnimLoadInProgress() = true;

    // This recursively discovers extra/**/*.pcanim and registers validated
    // Mod entries.  enumerate_mods() may already have done this; the scan is
    // internally one-shot and path aliases are deduplicated below.
    modScanNalOverrides();

    std::map<std::filesystem::path, const Mod *> uniqueBanks;
    for (const auto &entry : Mods)
    {
        const Mod &mod = entry.second;
        if (mod.Type != TLRESOURCE_TYPE_ANIM_FILE || mod.Data.empty())
            continue;
        if (transformToLower(mod.Path.extension().string()) != ".pcanim")
            continue;
        uniqueBanks.emplace(mod.Path.lexically_normal(), &mod);
    }

    for (const auto &entry : uniqueBanks)
    {
        const Mod &mod = *entry.second;
        size_t clipCount = 0;
        if (!tlLoosePCAnimImageLooksValid(mod.Data, clipCount))
        {
            sp_log("[mod][tlresource] rejected loose PCANIM \"%s\": invalid image",
                   mod.Path.string().c_str());
            continue;
        }

        if (mod.Data.size() > 0xFFFFFFFFu)
        {
            sp_log("[mod][tlresource] rejected loose PCANIM \"%s\": file too large",
                   mod.Path.string().c_str());
            continue;
        }

        void *raw = tlMemAlloc(static_cast<uint32_t>(mod.Data.size()),
                               16u, 0x2000000u);
        if (raw == nullptr)
        {
            sp_log("[mod][tlresource] PCANIM allocation failed for \"%s\"",
                   mod.Path.string().c_str());
            continue;
        }

        std::memcpy(raw, mod.Data.data(), mod.Data.size());

        const std::string lowerStem =
            transformToLower(mod.Path.stem().string());
        const uint32_t targetHash = to_hash(lowerStem.c_str());

        // Do not preserve the generic embedded "allanims" identity.  At the
        // directory layer the external filename is the requested bank name.
        tlStampLoosePCAnimTargetName(static_cast<uint8_t *>(raw),
                                     mod.Data.size(), lowerStem);

        auto *animFile = static_cast<nalAnimFile *>(raw);
        animFile->field_44 = 1;
        animFile->field_4 |= 4u;

        sp_log("[mod][tlresource] loading loose PCANIM \"%s\" as 0x%08X "
               "(%u bytes, %u clips)",
               mod.Path.string().c_str(), targetHash,
               static_cast<unsigned>(mod.Data.size()),
               static_cast<unsigned>(clipCount));

        // Use the retail parser directly here. Calling nalLoadAnimFileInternal
        // would run the filename-override resolver again and can recursively
        // select the same loose file.  0x0078D540 rebases the PCANIM and builds
        // its nalAnimClass list in field_34.
        if (!static_cast<bool>(CDECL_CALL(0x0078D540, animFile)))
        {
            sp_log("[mod][tlresource] native NAL rejected loose PCANIM \"%s\"",
                   mod.Path.string().c_str());
            tlMemFree(animFile);
            continue;
        }

        tlLoosePCAnimBanks().push_back({animFile, mod.Path, targetHash});
        sp_log("[mod][tlresource] loaded loose PCANIM \"%s\" -> 0x%08X",
               mod.Path.string().c_str(), targetHash);
    }

    tlLoosePCAnimLoadInProgress() = false;
}

static nalAnimClass<nalAnyPose> *tlFindLoosePCAnimClip(uint32_t clipHash)
{
    // Later paths take precedence when two loose banks export the same clip.
    auto &banks = tlLoosePCAnimBanks();
    for (auto bankIt = banks.rbegin(); bankIt != banks.rend(); ++bankIt)
    {
        nalAnimClass<nalAnyPose> *clip =
            reinterpret_cast<nalAnimClass<nalAnyPose> *>(bankIt->image->field_34);
        size_t guard = 0;
        while (clip != nullptr && guard++ < 65536u)
        {
            // Serialized/parsed nalAnimClass layout begins with vtbl, next,
            // then tlFixedString at +0x08. The hash remains at +0x08 after
            // native pointer rebasing.
            const uint32_t currentHash = *reinterpret_cast<const uint32_t *>(
                reinterpret_cast<const uint8_t *>(clip) + 0x08u);
            if (currentHash == clipHash)
            {
                static std::set<uint32_t> logged;
                if (logged.insert(clipHash).second)
                {
                    sp_log("[mod][tlresource] animation 0x%08X overridden by \"%s\" "
                           "(bank 0x%08X)",
                           clipHash, bankIt->path.string().c_str(),
                           bankIt->targetHash);
                }
                return clip;
            }
            clip = clip->field_4;
        }
    }
    return nullptr;
}


// PCMESH loose-file bridge ---------------------------------------------------
//
// IMPORTANT: this directory hook must NOT call the directory Load vfunc by
// itself.  nglLoadMeshFile() already has the correct ownership sequence:
//
//     Find(name) -> if missing -> Load(name)
//
// and packed/worldly mesh resources also have their own resource-handler
// lifetime.  Calling Load from inside Find creates a second nglMeshFile shell
// for the same resource and can leave duplicate mesh/material registrations.
// That is the crash seen when a loose .PCMESH is exercised.
//
// The only job performed here is to make the filename/request hash visible to
// modBindRawPCMesh().  The normal caller then performs the one legitimate Load,
// and nglLoadMeshFileInternalPC() owns parsing/rebasing the writable copy.

static std::set<std::filesystem::path> &tlLoosePCMeshRejectedPaths()
{
    static std::set<std::filesystem::path> paths;
    return paths;
}

static Mod *tlFindLoosePCMeshMod(const tlFixedString &requested)
{
    // enumerate_mods() normally registers .pcmesh by stem hash.
    if (Mod *mod = getMod(requested.m_hash, TLRESOURCE_TYPE_MESH_FILE))
    {
        if (!mod->Data.empty()
            && transformToLower(mod->Path.extension().string()) == ".pcmesh")
            return mod;
    }

    // Fallback by actual filename for requests whose printable spelling/path
    // differs from the key used while enumerating Mods.
    std::string request = transformToLower(requested.to_string());
    for (char &c : request)
        if (c == '\\') c = '/';

    const size_t slash = request.find_last_of('/');
    if (slash != std::string::npos)
        request.erase(0, slash + 1);
    if (request.size() > 7u && request.substr(request.size() - 7u) == ".pcmesh")
        request.resize(request.size() - 7u);

    std::set<std::filesystem::path> seen;
    for (auto &entry : Mods)
    {
        Mod &mod = entry.second;
        if (mod.Type != TLRESOURCE_TYPE_MESH_FILE || mod.Data.empty())
            continue;
        if (transformToLower(mod.Path.extension().string()) != ".pcmesh")
            continue;

        const std::filesystem::path normalized = mod.Path.lexically_normal();
        if (!seen.insert(normalized).second)
            continue;

        if (transformToLower(mod.Path.stem().string()) == request)
            return &mod;

        // Literal-hash aliases remain supported.
        if (entry.first == requested.m_hash)
            return &mod;
    }
    return nullptr;
}

static bool tlLoosePCMeshLooksValid(const Mod &mod)
{
    return modPCMESHDetectTLType(mod.Data.data(), mod.Data.size(),
                                 TLRESOURCE_TYPE_MESH_FILE)
        == TLRESOURCE_TYPE_MESH_FILE;
}

// External-only .DDS and .PCSKEL fallbacks ---------------------------------
//
// Packed resources are still handled by their normal resource handlers (which
// already apply same-name Mods). These helpers are only consulted when the
// native directory returns its default/missing object, allowing a dependency
// referenced by another loose asset to exist entirely outside PCPACK.

static nglTexture *tlFindExternalOnlyTexture(const tlFixedString &requested)
{
    Mod *mod = getMod(requested.m_hash, TLRESOURCE_TYPE_TEXTURE);
    if (mod == nullptr || mod->Data.empty()
        || transformToLower(mod->Path.extension().string()) != ".dds")
        return nullptr;

    static std::map<uint32_t, nglTexture *> cache;
    auto found = cache.find(requested.m_hash);
    if (found != cache.end() && found->second != nullptr)
        return found->second;

    if (mod->Data.size() > 0xFFFFFFFFu)
        return nullptr;

    // nglLoadTextureTM2 sees the typed Mod entry and chooses D3DX for ordinary
    // DDS or the engine parser for its serialized texture container.
    nglTexture *tex = nglConstructTexture(
        requested, static_cast<nglTextureFileFormat>(0),
        mod->Data.data(), static_cast<unsigned int>(mod->Data.size()));
    if (tex == nullptr)
        return nullptr;

    cache[requested.m_hash] = tex;
    sp_log("[mod][tlresource] loaded external-only DDS \"%s\" for \"%s\" "
           "(0x%08X, %u bytes; no PCPACK entry)",
           mod->Path.filename().string().c_str(), requested.to_string(),
           requested.m_hash, static_cast<unsigned>(mod->Data.size()));
    return tex;
}

static nalBaseSkeleton *tlFindExternalOnlySkeleton(const tlFixedString &requested)
{
    modScanNalOverrides();

    Mod *mod = getMod(requested.m_hash, TLRESOURCE_TYPE_SKELETON);
    if (mod == nullptr || mod->Data.empty()
        || transformToLower(mod->Path.extension().string()) != ".pcskel")
        return nullptr;

    static std::map<uint32_t, nalBaseSkeleton *> cache;
    auto found = cache.find(requested.m_hash);
    if (found != cache.end() && found->second != nullptr)
        return found->second;

    if (mod->Data.size() > 0xFFFFFFFFu)
        return nullptr;

    void *copy = tlMemAlloc(static_cast<uint32_t>(mod->Data.size()),
                            16u, 0x2000000u);
    if (copy == nullptr)
        return nullptr;
    std::memcpy(copy, mod->Data.data(), mod->Data.size());

    // Call the native constructor directly. Calling the mod wrapper here would
    // perform another filename-based replacement and can recurse back into the
    // same external source. This private copy is already the selected PCSKEL.
    nalBaseSkeleton *skel = reinterpret_cast<nalBaseSkeleton *>(
        static_cast<uintptr_t>(CDECL_CALL(0x0078DC80, copy)));
    if (skel == nullptr)
    {
        tlMemFree(copy);
        return nullptr;
    }

    cache[requested.m_hash] = skel;
    sp_log("[mod][tlresource] loaded external-only PCSKEL \"%s\" for \"%s\" "
           "(0x%08X, %u bytes; no PCPACK entry)",
           mod->Path.filename().string().c_str(), requested.to_string(),
           requested.m_hash, static_cast<unsigned>(mod->Data.size()));
    return skel;
}

static void tlPrepareLoosePCMeshForRequest(const tlFixedString &requested)
{
    Mod *mod = tlFindLoosePCMeshMod(requested);
    if (mod == nullptr)
        return;

    if (!tlLoosePCMeshLooksValid(*mod))
    {
        const std::filesystem::path key = mod->Path.lexically_normal();
        if (tlLoosePCMeshRejectedPaths().insert(key).second)
            sp_log("[mod][tlresource] rejected loose PCMESH \"%s\": invalid raw PCM 0x601 image",
                   mod->Path.string().c_str());
        return;
    }

    // modBindRawPCMesh() prefers the canonical request hash. Publish an
    // alias when the file was found by filename/path fallback. std::multimap
    // insertion does not invalidate the Mod object we just inspected.
    bool aliasPresent = false;
    const auto range = Mods.equal_range(requested.m_hash);
    for (auto it = range.first; it != range.second; ++it)
    {
        if (it->second.Type == TLRESOURCE_TYPE_MESH_FILE
            && it->second.Path == mod->Path)
        {
            aliasPresent = true;
            break;
        }
    }

    if (!aliasPresent)
        Mods.emplace(requested.m_hash, *mod);

    static std::set<std::pair<uint32_t, std::filesystem::path>> logged;
    const auto key = std::make_pair(requested.m_hash, mod->Path.lexically_normal());
    if (logged.insert(key).second)
    {
        sp_log("[mod][tlresource] PCMESH \"%s\" prepared for \"%s\" "
               "(hash 0x%08X, %u bytes); normal mesh loader owns parsing",
               mod->Path.string().c_str(), requested.to_string(), requested.m_hash,
               static_cast<unsigned>(mod->Data.size()));
    }
}

} // namespace

template<>
nglMeshFile *tlresource_directory<nglMeshFile, tlFixedString>::Find(const tlFixedString &a2)
{
    TRACE("tlresource_directory<nglMeshFile,tlFixedString>::Find", a2.to_string());

    if constexpr (0)
    {
        nglMeshFile *v5 = nullptr;
        if (this->field_4 != nullptr)
        {
            v5 = CAST(v5, this->field_4->get_tlresource(a2, TLRESOURCE_TYPE_MESH_FILE));
        }

        if ( v5 == nullptr && system_dir != nullptr )
        {
            v5 = system_dir->Find(a2);
            bool SHOW_RESOURCE_SPAM = os_developer_options::instance->get_flag(mString {"SHOW_RESOURCE_SPAM"});
            if ( v5 != nullptr )
            {
                if ( SHOW_RESOURCE_SPAM )
                {
                    auto *v2 = a2.to_string();
                    debug_print_va("found tlresource %s in system directory", v2);
                }
            }
            else if ( SHOW_RESOURCE_SPAM )
            {
                auto *v3 = a2.to_string();
                debug_print_va("didn't find tlresource %s in system directory", v3);
            }
        }

        if ( v5 == nullptr )
        {
            v5 = (nglMeshFile *) default_tlres;
        }

        return v5;
    }
    else
    {
        // Directory layer only prepares the loose-file alias. Never call Load
        // here: nglLoadMeshFile() / the worldly resource handler owns the
        // single valid Find -> Load lifetime and registration sequence.
        tlPrepareLoosePCMeshForRequest(a2);
        return (nglMeshFile *) THISCALL(0x005692B0, this, &a2);
    }
}

template<>
nglMesh *tlresource_directory<nglMesh,tlHashString>::Find(const tlHashString &a2)
{
    TRACE("tlresource_directory<nglMesh,tlHashString>::Find");

    if constexpr (0)
    {
        nglMesh *v5 = nullptr;
        if ( this->field_4 != nullptr )
        {
            v5 = (nglMesh *) this->field_4->get_tlresource(a2, TLRESOURCE_TYPE_MESH);
        }

        if ( v5 == nullptr && system_dir != nullptr )
        {
            v5 = system_dir->Find(a2);

            bool SHOW_RESOURCE_SPAM = os_developer_options::instance->get_flag(mString {"SHOW_RESOURCE_SPAM"});
            if ( v5 != nullptr )
            {
                if ( SHOW_RESOURCE_SPAM )
                {
                    auto *v2 = a2.c_str();
                    debug_print_va("found tlresource %s in system directory", v2);
                }
            }
            else if ( SHOW_RESOURCE_SPAM )
            {
                auto *v3 = a2.c_str();
                debug_print_va("didn't find tlresource %s in system directory", v3);
            }
        }

        if ( v5 == nullptr ) {
            v5 = default_tlres;
        }

        return v5;
    }
    else
    {
        return (nglMesh *) THISCALL(0x005693B0, this, &a2);
    }
}

template<>
void tlresource_directory<nglMesh, tlHashString>::Add([[maybe_unused]] nglMesh *Mesh)
{
    TRACE("tlresource_directory<nglMesh, tlHashString>::Add");
    ;
}


//0x00569BA0
template<>
nalBaseSkeleton *tlresource_directory<nalBaseSkeleton, tlFixedString>::Find(const tlFixedString &a1)
{
    TRACE("tlresource_directory<nalBaseSkeleton, tlFixedString>::Find", a1.to_string());

    if constexpr (0)
    {
        nalBaseSkeleton *v5 = nullptr;
        if ( this->field_4 != nullptr )
        {
            v5 = (nalBaseSkeleton *) this->field_4->get_tlresource(a1, TLRESOURCE_TYPE_SKELETON);
        }

        if ( v5 == nullptr && system_dir != nullptr )
        {
            v5 = system_dir->Find(a1);

            auto SHOW_RESOURCE_SPAM = os_developer_options::instance->get_flag(mString {"SHOW_RESOURCE_SPAM"});
            if ( v5 != nullptr )
            {
                if ( SHOW_RESOURCE_SPAM )
                {
                    auto *v2 = a1.to_string();
                    debug_print_va("found tlresource %s in system directory", v2);
                }
            }
            else if ( SHOW_RESOURCE_SPAM )
            {
                auto *v3 = a1.to_string();
                debug_print_va("didn't find tlresource %s in system directory", v3);
            }
        }

        if ( v5 == nullptr ) {
            v5 = default_tlres;
        }

        return v5;
    }
    else
    {
        nalBaseSkeleton *result =
            (nalBaseSkeleton *) THISCALL(0x00569BA0, this, &a1);
        if (result == nullptr || result == default_tlres)
        {
            if (nalBaseSkeleton *external = tlFindExternalOnlySkeleton(a1))
                return external;
        }
        return result;
    }
}

//0x005691B0
template<>
nglTexture *tlresource_directory<nglTexture, tlFixedString>::Find(const tlFixedString &a1) 
{
    TRACE("tlresource_directory<nglTexture, tlFixedString>::Find(const tlFixedString &)", a1.to_string());

    if constexpr (0)
    {
        nglTexture *v5 = nullptr;
        if ( this->field_4 != nullptr )
        {
            v5 = (nglTexture *) this->field_4->get_tlresource(a1, TLRESOURCE_TYPE_TEXTURE);
        }

        if ( v5 == nullptr && system_dir != nullptr )
        {
            auto SHOW_RESOURCE_SPAM = os_developer_options::instance->get_flag(mString {"SHOW_RESOURCE_SPAM"});
            v5 = system_dir->Find(a1);
            if ( v5 != nullptr )
            {
                if ( SHOW_RESOURCE_SPAM )
                {
                    auto *v2 = a1.to_string();
                    debug_print_va("found tlresource %s in system directory", v2);
                }
            }
            else if ( SHOW_RESOURCE_SPAM )
            {
                auto *v3 = a1.to_string();
                debug_print_va("didn't find tlresource %s in system directory", v3);
            }
        }

        if ( v5 == nullptr ) {
            v5 = default_tlres;
        }

        return v5;
    }
    else
    {
        nglTexture * (__fastcall *func)(void *, void *edx, const tlFixedString *a1) = CAST(func, 0x005691B0);
        nglTexture *result = func(this, nullptr, &a1);
        if (result == nullptr || result == default_tlres)
        {
            if (nglTexture *external = tlFindExternalOnlyTexture(a1))
                return external;
        }
        return result;
    }
}

template<>
nglTexture *tlresource_directory<nglTexture,tlFixedString>::Find(unsigned int a2)
{
    TRACE("tlresource_directory<nglTexture,tlFixedString>::Find(uint32_t )", string_hash {int(a2)}.to_string());

    nglTexture *v3 = nullptr;
    if ( this->field_4 != nullptr ) {
        v3 = (nglTexture *) this->field_4->get_tlresource(a2, TLRESOURCE_TYPE_TEXTURE);
    }

    if ( v3 == nullptr && system_dir != nullptr )
    {
        auto SHOW_RESOURCE_SPAM = os_developer_options::instance->get_flag(mString{"SHOW_RESOURCE_SPAM"});

        nglTexture * (__fastcall *find)(void *, void *, uint32_t) = CAST(find, get_vfunc(system_dir->m_vtbl, 0x8));
        v3 = find(system_dir, nullptr, a2);
        if ( v3 != nullptr )
        {
            if ( SHOW_RESOURCE_SPAM )
            {
                debug_print_va("found tlresource %08x in system directory", a2);
            }
        }
        else if ( SHOW_RESOURCE_SPAM )
        {
            debug_print_va("didn't find tlresource %08x in system directory", a2);
        }
    }

    if (v3 == nullptr || v3 == default_tlres)
    {
        tlFixedString requested{string_hash{static_cast<int>(a2)}.to_string()};
        requested.m_hash = a2;
        if (nglTexture *external = tlFindExternalOnlyTexture(requested))
            return external;
        v3 = default_tlres;
    }

    return v3;
}

template<>
nalAnimClass<nalAnyPose> *tlresource_directory<nalAnimClass<nalAnyPose>, tlFixedString>::Find(
        unsigned int a2)
{
    TRACE("tlresource_directory<nalAnimClass<nalAnyPose>, tlFixedString>::Find");

    if constexpr (0)
    {
        nalAnimClass<nalAnyPose> *tlresource = nullptr;
        if (this->field_4 != nullptr) {
            tlresource = CAST(tlresource, this->field_4->get_tlresource(a2, TLRESOURCE_TYPE_ANIM));
        }

        if ( tlresource == nullptr && system_dir != nullptr )
        {
            nalAnimClass<nalAnyPose> * (__fastcall *find)(void *, void *, uint32_t) = CAST(find, get_vfunc(system_dir->m_vtbl, 0x8));
            tlresource  = find(system_dir, nullptr, a2);

            auto SHOW_RESOURCE_SPAM = os_developer_options::instance->get_flag(mString{"SHOW_RESOURCE_SPAM"});

            if ( tlresource != nullptr )
            {
                if ( SHOW_RESOURCE_SPAM ) {
                    debug_print_va("found tlresource %08x in system directory", a2);
                }
            }
            else if ( SHOW_RESOURCE_SPAM )
            {
                debug_print_va("didn't find tlresource %08x in system directory", a2);
            }
        }

        if ( tlresource == nullptr ) {
            return default_tlres;
        }

        return tlresource;
    }
    else
    {
        // PS2ANIM is parsed/owned exactly once by nal_system through the
        // retail resource shell.  Give its already-loaded clips explicit
        // tlresource precedence here without parsing/registering the same
        // .ps2anim bank a second time.
        if (nalAnimClass<nalAnyPose> *ps2Clip = modPS2ANIMFindLoadedClip(a2))
        {
            static std::set<uint32_t> loggedPS2;
            if (loggedPS2.insert(a2).second)
                sp_log("[mod][tlresource] animation 0x%08X resolved from loaded PS2ANIM",
                       a2);
            return ps2Clip;
        }

        // Existing loose PCANIM path.  It predates the managed PS2 bridge and
        // keeps its process-lifetime direct bank for compatibility.
        tlLoadLoosePCAnimBanksForDirectory();
        if (nalAnimClass<nalAnyPose> *overrideClip = tlFindLoosePCAnimClip(a2))
            return overrideClip;

        nalAnimClass<nalAnyPose> *result = CAST(result, THISCALL(0x00566630, this, a2));
        return result;
    }
}

template<>
nglMesh *tlresource_directory<nglMesh, tlHashString>::Find(uint32_t a2)
{
    if constexpr (1)
    {
        assert(((tlresource_type) tlres_type) != TLRESOURCE_TYPE_NONE &&
               "This type must have a matching tlresource type");

        nglMesh *v3 = nullptr;
        if (this->field_4 != nullptr) {
            v3 = (nglMesh *) this->field_4->get_tlresource(a2, TLRESOURCE_TYPE_MESH);
        }

        if (v3 == nullptr && system_dir != nullptr) {
            nglMesh * (__fastcall *Find)(void *, void *, uint32_t) = CAST(Find, get_vfunc(system_dir->m_vtbl, 0x8));
            auto *v3 = Find(system_dir, nullptr, a2);
            if (v3 != nullptr) {
                //if (byte_15B2C1A)
                sp_log("found tlresource %08x in system directory", a2);
            } else
            //if (byte_15B2C1A)
            {
                sp_log("didn't find tlresource %08x in system directory", a2);
            }
        }

        if (v3 == nullptr) {
            v3 = default_tlres;
        }
        return v3;
    }
}

void tlresource_directory_patch()
{
    {
        nglMeshFile * (tlresource_directory<nglMeshFile, tlFixedString>::*func)(const tlFixedString &) = tlresource_directory<nglMeshFile, tlFixedString>::Find;
        FUNC_ADDRESS(address, func);
        set_vfunc(0x00889680, address);
    }

    {
        nglMesh * (tlresource_directory<nglMesh, tlHashString>::*func)(const tlHashString &) = tlresource_directory<nglMesh, tlHashString>::Find;
        FUNC_ADDRESS(address, func);
        set_vfunc(0x008896AC, address);
    }

    {
        void (tlresource_directory<nglMesh, tlHashString>::*func)(nglMesh *) = tlresource_directory<nglMesh, tlHashString>::Add;
        FUNC_ADDRESS(address, func);
        set_vfunc(0x008896B0, address);
    }

    {
        nglTexture * (tlresource_directory<nglTexture, tlFixedString>::*func)(uint32_t ) = tlresource_directory<nglTexture, tlFixedString>::Find;
        FUNC_ADDRESS(address, func);
        set_vfunc(0x00889650, address);
    }

    {
        nalAnimClass<nalAnyPose> * (tlresource_directory<nalAnimClass<nalAnyPose>,tlFixedString>::*func)(uint32_t ) = tlresource_directory<nalAnimClass<nalAnyPose>,tlFixedString>::Find;
        FUNC_ADDRESS(address, func);
        set_vfunc(0x008897B0, address);
    }

    {
        nglTexture * (tlresource_directory<nglTexture, tlFixedString>::*func)(const tlFixedString &) = tlresource_directory<nglTexture, tlFixedString>::Find;
        FUNC_ADDRESS(address, func);
        set_vfunc(0x00889654, address);
    }

    {
        nalBaseSkeleton * (tlresource_directory<nalBaseSkeleton, tlFixedString>::*func)(const tlFixedString &) = tlresource_directory<nalBaseSkeleton, tlFixedString>::Find;
        FUNC_ADDRESS(address, func);
        set_vfunc(0x0088980C, address);
    }
}


void tlresource_directory2_patch()
{
    {
        nglMeshFile * (tlresource_directory<nglMeshFile, tlFixedString>::*func)(const tlFixedString &) = tlresource_directory<nglMeshFile, tlFixedString>::Find;
        FUNC_ADDRESS(address, func);
        set_vfunc(0x00889680, address);
    }

    {
        nglMesh * (tlresource_directory<nglMesh, tlHashString>::*func)(const tlHashString &) = tlresource_directory<nglMesh, tlHashString>::Find;
        FUNC_ADDRESS(address, func);
        set_vfunc(0x008896AC, address);
    }

    {
        void (tlresource_directory<nglMesh, tlHashString>::*func)(nglMesh *) = tlresource_directory<nglMesh, tlHashString>::Add;
        FUNC_ADDRESS(address, func);
        set_vfunc(0x008896B0, address);
    }

    {
      //  nglTexture * (tlresource_directory<nglTexture, tlFixedString>::*func)(uint32_t ) = tlresource_directory<nglTexture, tlFixedString>::Find;
     //   FUNC_ADDRESS(address, func);
     //   set_vfunc(0x00889650, address);
    }

    {
        nalAnimClass<nalAnyPose> * (tlresource_directory<nalAnimClass<nalAnyPose>,tlFixedString>::*func)(uint32_t ) = tlresource_directory<nalAnimClass<nalAnyPose>,tlFixedString>::Find;
        FUNC_ADDRESS(address, func);
        set_vfunc(0x008897B0, address);
    }

    {
      //  nglTexture * (tlresource_directory<nglTexture, tlFixedString>::*func)(const tlFixedString &) = tlresource_directory<nglTexture, tlFixedString>::Find;
      //  FUNC_ADDRESS(address, func);
      //  set_vfunc(0x00889654, address);
    }

    {
        nalBaseSkeleton * (tlresource_directory<nalBaseSkeleton, tlFixedString>::*func)(const tlFixedString &) = tlresource_directory<nalBaseSkeleton, tlFixedString>::Find;
        FUNC_ADDRESS(address, func);
        set_vfunc(0x0088980C, address);
    }
}
