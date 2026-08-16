#include "nal_system.h"

#include "common.h"
#include "func_wrapper.h"
#include "log.h"
#include "nal_anim.h"
#include "nal_component.h"
#include "nfl_system.h"
#include "osassert.h"
#include "resource_directory.h"
#include "resource_manager.h"
#include "resource_pack_slot.h"
#include "scene_anim.h"
#include "tl_system.h"
#include "tlresource_directory.h"
#include "tlresource_location.h"
#include "trace.h"
#include "utility.h"
#include "vtbl.h"

#include <nal_list.h>
#include <nal_skeleton.h>

#include <cassert>
#include <cstring>
#include <limits>
#include <set>
#include <string>

VALIDATE_OFFSET(nalGeneric::nalGenericSkeleton, field_50, 0x50);

tlInstanceBank & nalTypeInstanceBank = var<tlInstanceBank>(0x009770E8);

tlInstanceBank & nalComponentInstanceBank = var<tlInstanceBank>(0x00977100);

#define make_var(T0, T1, address) \
template<> \
tlInstanceBankResourceDirectory<T0, T1> *& \
    tlresource_directory<T0, T1>::system_dir = var<tlInstanceBankResourceDirectory<T0, T1> *>(address)

make_var(nalAnimFile, tlFixedString, 0x009609F4);

make_var(nalBaseSkeleton, tlFixedString, 0x009609E8);

make_var(nalAnimClass<nalAnyPose>, tlFixedString, 0x009609F0);

make_var(nalSceneAnim, tlFixedString, 0x009609EC);

#undef make_var

tlInstanceBankResourceDirectory<nalBaseSkeleton, tlFixedString> *& nalSkeletonDirectory =
    var<tlInstanceBankResourceDirectory<nalBaseSkeleton, tlFixedString> *>(0x00977178);

tlInstanceBankResourceDirectory<nalAnimFile, tlFixedString> *& nalAnimFileDirectory = var<tlInstanceBankResourceDirectory<nalAnimFile, tlFixedString> *>(0x0097716C);

tlInstanceBankResourceDirectory<nalAnimClass<nalAnyPose>, tlFixedString> *& nalAnimDirectory = var<tlInstanceBankResourceDirectory<nalAnimClass<nalAnyPose>, tlFixedString> *>(0x00977170);

tlInstanceBankResourceDirectory<nalSceneAnim, tlFixedString> *& nalSceneAnimDirectory = var<tlInstanceBankResourceDirectory<nalSceneAnim, tlFixedString> *>(0x00977168);

int *& PanelComponentMgr::comp_list = var<int *>(0x0096F7DC);

void * BaseComponent::ApplyPublicPerSkelDataOffset(uint32_t a1, void *a2) const
{
    void * (__fastcall *func)(const void *, void *, uint32_t, void *) = CAST(func, get_vfunc(this->m_vtbl, 0x8));
    return func(this, nullptr, a1, a2);
}

void BaseComponent::SkelPoseProcess(uint32_t a1, void *a2, void *a3) const
{
    void (__fastcall *func)(const void *, void *, uint32_t, void *, void *) = CAST(func, get_vfunc(this->m_vtbl, 0x3C));
    func(this, nullptr, a1, a2, a3);
}

void BaseComponent::PoseDataFree(uint32_t a2, void *a3) const
{
    void (__fastcall *func)(const void *, void *, uint32_t, void *) = CAST(func, get_vfunc(this->m_vtbl, 0x50));
    func(this, nullptr, a2, a3);
}

int *nalComponentU8Base::GetType() {
    return &TypeID;
}

char *nalComponentStringBase::GetType() {
    sp_log("%d", TypeID);

    return &TypeID;
}

nalBaseSkeleton *nalGetSkeleton(const tlFixedString &a1) {
    nalBaseSkeleton * (__fastcall *Find)(void *, void *, const tlFixedString *) = CAST(Find, get_vfunc(nalSkeletonDirectory->m_vtbl, 0xC));

    return Find(nalSkeletonDirectory, nullptr, &a1);
}

struct nalHeap {
    std::intptr_t m_vtbl;
    uint32_t field_4;
    int field_8;
};

struct nalAnimCache {
    nalHeap *field_0;
    int field_4;
    int field_8;
};

static auto & dword_970D64 = var<void *>(0x00970D64);

static auto & nalAnimPath = var<char[1]>(0x00976FC8);

static auto & nalSkeletonPath = var<char[1]>(0x00976EC8);

static nalHeap & nalDefaultHeap = var<nalHeap>(0x00946A84);

static nalAnimCache & nalAnimationCache = var<nalAnimCache>(0x00977114);

static nalHeap *& nalAnimationHeap = var<nalHeap *>(0x00976EC0);


namespace {

struct nalPCAnimInfo {
    int numSkeletons = 0;
    uint32_t firstListOffset = 0;
    uint32_t outerCount = 0;
    size_t skeletonTableOffset = 0;
    size_t clipCount = 0;
    std::vector<uint32_t> skeletonHashes;
};

struct nalExternalPCAnimIdentity {
    uint32_t hash = 0;
    int size = 0;
    bool sceneFlavor = false;
};

static std::map<const uint8_t *, nalExternalPCAnimIdentity> &nalExternalPCAnimImages()
{
    static std::map<const uint8_t *, nalExternalPCAnimIdentity> images;
    return images;
}

static uint32_t nalReadU32(const uint8_t *raw, size_t offset)
{
    uint32_t value = 0;
    std::memcpy(&value, raw + offset, sizeof(value));
    return value;
}

static size_t nalAlign16(size_t value)
{
    return (value + 15u) & ~size_t(15u);
}

static bool nalFixedNameSane(const uint8_t *raw, size_t size, size_t offset)
{
    if (raw == nullptr || offset > size || size - offset < 0x20
        || nalReadU32(raw, offset) == 0)
        return false;

    const char *text = reinterpret_cast<const char *>(raw + offset + 4);
    size_t length = 0;
    while (length < 28 && text[length] != '\0')
        ++length;
    if (length == 0 || length == 28)
        return false;

    for (size_t i = 0; i < length; ++i)
    {
        const unsigned char c = static_cast<unsigned char>(text[i]);
        if (c < 0x20 || c > 0x7E)
            return false;
    }
    return true;
}

static bool nalValidateSkeletonTable(const uint8_t *raw,
                                     size_t size,
                                     size_t tableOffset,
                                     int numSkeletons,
                                     nalPCAnimInfo &info,
                                     const char *&reason)
{
    const size_t tableSize = size_t(numSkeletons) * 0x20u;
    if (tableOffset > size || tableSize > size - tableOffset)
    {
        reason = "skeleton table is outside the file";
        return false;
    }

    info.skeletonHashes.clear();
    info.skeletonHashes.reserve(size_t(numSkeletons));
    for (int i = 0; i < numSkeletons; ++i)
    {
        const size_t nameOffset = tableOffset + size_t(i) * 0x20u;
        if (!nalFixedNameSane(raw, size, nameOffset))
        {
            reason = "invalid skeleton name";
            return false;
        }
        info.skeletonHashes.push_back(nalReadU32(raw, nameOffset));
    }
    return true;
}

static bool nalValidateClipChain(const uint8_t *raw,
                                 size_t size,
                                 size_t firstOffset,
                                 int numSkeletons,
                                 size_t exactCount,
                                 size_t &countOut,
                                 const char *&reason)
{
    countOut = 0;
    if ((firstOffset & 3u) != 0 || firstOffset > size || size - firstOffset < 0x40)
    {
        reason = "first animation is outside the file";
        return false;
    }

    size_t offset = firstOffset;
    const size_t hardLimit = size / 0x40u + 1u;
    for (;;)
    {
        if (offset > size || size - offset < 0x40)
        {
            reason = "animation header is truncated";
            return false;
        }
        if (!nalFixedNameSane(raw, size, offset + 0x8))
        {
            reason = "invalid animation name";
            return false;
        }

        const uint32_t skeletonIndex = nalReadU32(raw, offset + 0x28);
        if (skeletonIndex >= static_cast<uint32_t>(numSkeletons))
        {
            reason = "animation skeleton index is out of range";
            return false;
        }

        ++countOut;
        if (countOut > hardLimit || (exactCount != 0 && countOut > exactCount))
        {
            reason = "animation list does not terminate";
            return false;
        }

        const uint32_t nextRelative = nalReadU32(raw, offset + 0x4);
        if (nextRelative == 0)
            break;
        if (nextRelative < 0x40u || (nextRelative & 3u) != 0
            || nextRelative > size - offset
            || size - (offset + nextRelative) < 0x40)
        {
            reason = "invalid relative animation link";
            return false;
        }
        offset += nextRelative; // strictly forward: cycles cannot survive this check
    }

    if (exactCount != 0 && countOut != exactCount)
    {
        reason = "animation count does not match its scene node";
        return false;
    }
    return true;
}

static bool nalValidateAnimImage(const uint8_t *raw,
                                 size_t size,
                                 nalPCAnimInfo &info,
                                 const char *&reason)
{
    reason = "malformed animation image";
    info = {};
    if (raw == nullptr || size < 0x70)
    {
        reason = "file is too small";
        return false;
    }
    if (nalReadU32(raw, 0x0) != 0x10101u)
    {
        reason = "unsupported PCANIM version";
        return false;
    }

    const uint32_t numSkeletons = nalReadU32(raw, 0xC);
    if (numSkeletons == 0 || numSkeletons > 128u
        || nalReadU32(raw, 0x8) != numSkeletons * 0x20u)
    {
        reason = "invalid skeleton count/table size";
        return false;
    }
    if (!nalFixedNameSane(raw, size, 0x10))
    {
        reason = "invalid embedded animation-file name";
        return false;
    }

    info.numSkeletons = static_cast<int>(numSkeletons);
    info.skeletonTableOffset = 0x48;
    if (!nalValidateSkeletonTable(raw, size, info.skeletonTableOffset,
                                  info.numSkeletons, info, reason))
        return false;

    const size_t expectedFirst = nalAlign16(
        info.skeletonTableOffset + size_t(info.numSkeletons) * 0x20u);
    info.firstListOffset = nalReadU32(raw, 0x34);
    if (info.firstListOffset != expectedFirst)
    {
        reason = "unexpected first-animation offset";
        return false;
    }

    return nalValidateClipChain(raw, size, info.firstListOffset,
                                info.numSkeletons, 0, info.clipCount, reason);
}

static bool nalValidateSceneAnimImage(const uint8_t *raw,
                                      size_t size,
                                      nalPCAnimInfo &info,
                                      const char *&reason)
{
    reason = "malformed scene-animation image";
    info = {};
    if (raw == nullptr || size < 0x70)
    {
        reason = "file is too small";
        return false;
    }
    if (nalReadU32(raw, 0x0) != 0x10101u)
    {
        reason = "unsupported scene-PCANIM version";
        return false;
    }

    const uint32_t numSkeletons = nalReadU32(raw, 0xC);
    if (numSkeletons == 0 || numSkeletons > 128u
        || nalReadU32(raw, 0x8) != numSkeletons * 0x20u)
    {
        reason = "invalid skeleton count/table size";
        return false;
    }
    if (!nalFixedNameSane(raw, size, 0x10))
    {
        reason = "invalid scene-PCANIM header";
        return false;
    }

    info.numSkeletons = static_cast<int>(numSkeletons);
    info.skeletonTableOffset = 0x50;
    if (!nalValidateSkeletonTable(raw, size, info.skeletonTableOffset,
                                  info.numSkeletons, info, reason))
        return false;

    info.outerCount = nalReadU32(raw, 0x30);
    info.firstListOffset = nalReadU32(raw, 0x34);
    const size_t expectedFirst = nalAlign16(
        info.skeletonTableOffset + size_t(info.numSkeletons) * 0x20u);
    if (info.outerCount == 0 || info.outerCount > size / 0x10u
        || info.firstListOffset != expectedFirst)
    {
        reason = "invalid scene-node list";
        return false;
    }

    size_t outerOffset = info.firstListOffset;
    for (uint32_t outerIndex = 0; outerIndex < info.outerCount; ++outerIndex)
    {
        if ((outerOffset & 0xFu) != 0 || outerOffset > size
            || size - outerOffset < 0x10)
        {
            reason = "scene node is outside the file";
            return false;
        }

        const uint32_t innerCount = nalReadU32(raw, outerOffset + 0x4);
        const uint32_t innerRelative = nalReadU32(raw, outerOffset + 0x8);
        if (innerCount == 0 || innerRelative != 0x10u
            || innerRelative > size - outerOffset)
        {
            reason = "invalid scene animation list";
            return false;
        }

        size_t parsedInner = 0;
        if (!nalValidateClipChain(raw, size, outerOffset + innerRelative,
                                  info.numSkeletons, innerCount,
                                  parsedInner, reason))
            return false;
        info.clipCount += parsedInner;

        const uint32_t nextOuter = nalReadU32(raw, outerOffset);
        const bool last = outerIndex + 1u == info.outerCount;
        if (last)
        {
            if (nextOuter != 0)
            {
                reason = "scene-node count is smaller than its list";
                return false;
            }
        }
        else
        {
            if (nextOuter < 0x10u || (nextOuter & 0xFu) != 0
                || nextOuter > size - outerOffset
                || size - (outerOffset + nextOuter) < 0x10)
            {
                reason = "invalid relative scene-node link";
                return false;
            }
            outerOffset += nextOuter;
        }
    }
    return true;
}

static bool nalValidatePCAnim(const uint8_t *raw,
                              size_t size,
                              bool sceneFlavor,
                              nalPCAnimInfo &info,
                              const char *&reason)
{
    return sceneFlavor ? nalValidateSceneAnimImage(raw, size, info, reason)
                       : nalValidateAnimImage(raw, size, info, reason);
}

// PS2 character animation banks share the regular nalAnimFile outer layout
// with PCANIM.  Their nalChar clips deliberately carry format version 0x10002
// (PC retail is 0x10003).  Keep that marker intact and validate it here so a
// random PCANIM cannot accidentally enter the PS2 compatibility path.
static bool nalValidatePS2AnimImage(const uint8_t *raw,
                                    size_t size,
                                    nalPCAnimInfo &info,
                                    const char *&reason)
{
    if (!nalValidateAnimImage(raw, size, info, reason))
        return false;

    size_t offset = info.firstListOffset;
    size_t count = 0;
    for (;;)
    {
        if (offset > size || size - offset < 0x40u)
        {
            reason = "PS2 animation header is truncated";
            return false;
        }

        const uint32_t charVersion = nalReadU32(raw, offset + 0x2Cu);
        if (charVersion != 0x10002u)
        {
            reason = "animation is not a PS2 nalChar v0x10002 clip";
            return false;
        }

        ++count;
        const uint32_t nextRelative = nalReadU32(raw, offset + 0x04u);
        if (nextRelative == 0)
            break;

        // nalValidateAnimImage already proved the chain bounds/forward links.
        offset += nextRelative;
    }

    if (count != info.clipCount)
    {
        reason = "PS2 animation clip count changed during validation";
        return false;
    }

    return true;
}

static bool nalSameSkeletonContract(const nalPCAnimInfo &candidate,
                                    const nalPCAnimInfo &original)
{
    return candidate.numSkeletons == original.numSkeletons
        && candidate.skeletonHashes == original.skeletonHashes;
}

static bool nalKeyAlreadyHasPath(uint32_t key,
                                 const std::filesystem::path &path,
                                 int type)
{
    const auto range = Mods.equal_range(key);
    for (auto it = range.first; it != range.second; ++it)
        if (it->second.Type == type && it->second.Path == path)
            return true;
    return false;
}

static bool nalParseLiteralHash(const std::string &stem, uint32_t &valueOut)
{
    if (stem.empty())
        return false;
    size_t first = 0;
    bool markedHex = false;
    if (stem.size() > 2 && stem[0] == '0' && (stem[1] == 'x' || stem[1] == 'X'))
    {
        first = 2;
        markedHex = true;
    }
    const size_t digits = stem.size() - first;
    if (digits == 0 || digits > (markedHex ? 8u : 10u))
        return false;

    bool decimal = true;
    bool hexadecimal = true;
    for (size_t i = first; i < stem.size(); ++i)
    {
        const char c = stem[i];
        decimal = decimal && c >= '0' && c <= '9';
        hexadecimal = hexadecimal
            && ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f')
                || (c >= 'A' && c <= 'F'));
    }

    int base = 10;
    if (markedHex || (!decimal && hexadecimal && digits == 8u))
    {
        if (!hexadecimal || digits > 8u)
            return false;
        base = 16;
    }
    else if (!decimal)
    {
        return false;
    }

    const unsigned long long value = std::strtoull(stem.c_str() + first, nullptr, base);
    if (value > std::numeric_limits<uint32_t>::max())
        return false;
    valueOut = static_cast<uint32_t>(value);
    return true;
}

static void nalAddPCAnimAlias(std::set<uint32_t> &aliases, std::string key)
{
    if (key.empty())
        return;

    key = transformToLower(key);
    while (key.rfind("./", 0) == 0 || key.rfind(".\\", 0) == 0)
        key.erase(0, 2);

    aliases.insert(to_hash(key.c_str()));

    std::string alternate = key;
    bool changed = false;
    for (char &c : alternate)
    {
        if (c == '/')
        {
            c = '\\';
            changed = true;
        }
        else if (c == '\\')
        {
            c = '/';
            changed = true;
        }
    }
    if (changed)
        aliases.insert(to_hash(alternate.c_str()));
}

static std::set<uint32_t> nalPCAnimAliasesForPath(const std::filesystem::path &path)
{
    std::set<uint32_t> aliases;

    const std::string stem = transformToLower(path.stem().string());
    nalAddPCAnimAlias(aliases, stem);
    nalAddPCAnimAlias(aliases, path.filename().string());

    uint32_t literal = 0;
    if (nalParseLiteralHash(stem, literal))
        aliases.insert(literal);

    // extra/ is the loose-mod root used by enumerate_mods().  The resource
    // key stored in a pack is not guaranteed to be only the basename: some
    // builds hash a relative path and some keep the extension.  Bind all
    // equivalent spellings so a discovered extra/**/*.pcanim can actually
    // be selected by the runtime resource hash.
    std::error_code ec;
    const std::filesystem::path extraRoot = std::filesystem::current_path() / "extra";
    std::filesystem::path rel = std::filesystem::relative(path, extraRoot, ec);
    if (!ec && !rel.empty())
    {
        const std::string relString = rel.generic_string();
        if (relString != ".." && relString.rfind("../", 0) != 0)
        {
            nalAddPCAnimAlias(aliases, relString);
            std::filesystem::path relNoExt = rel;
            relNoExt.replace_extension();
            nalAddPCAnimAlias(aliases, relNoExt.generic_string());
        }
    }

    return aliases;
}

static bool nalPCAnimPathMatchesHash(const Mod &mod, uint32_t requestedHash)
{
    const auto aliases = nalPCAnimAliasesForPath(mod.Path);
    return aliases.find(requestedHash) != aliases.end();
}

static int nalCandidateScore(const Mod &mod, uint32_t requestedHash)
{
    const std::string stem = transformToLower(mod.Path.stem().string());
    if (to_hash(stem.c_str()) == requestedHash)
        return 4;
    uint32_t literal = 0;
    if (nalParseLiteralHash(stem, literal) && literal == requestedHash)
        return 4;
    if (nalPCAnimPathMatchesHash(mod, requestedHash))
        return 3;
    return 1;
}

// Set by modPCANIMGetOverride and consumed immediately by the NAL load wrapper.
// "forced" is only used when a loose PCANIM explicitly targets a retail bank
// by filename (or the retail bank's own skeleton identity).
static thread_local bool nalLastPCAnimOverrideForced = false;
static thread_local uint32_t nalLastPCAnimResolvedTargetHash = 0;

} // namespace

void modPCANIMTrackExternalImage(const uint8_t *image,
                                 uint32_t nameHash,
                                 int imageSize,
                                 bool sceneFlavor)
{
    if (image == nullptr || nameHash == 0 || imageSize <= 0)
        return;

    nalExternalPCAnimImages()[image] = {nameHash, imageSize, sceneFlavor};
}

bool modPS2ANIMValidate(const uint8_t *raw,
                        size_t size,
                        size_t *clipCountOut)
{
    nalPCAnimInfo info;
    const char *reason = nullptr;
    const bool valid = nalValidatePS2AnimImage(raw, size, info, reason);
    if (clipCountOut != nullptr)
        *clipCountOut = valid ? info.clipCount : 0;
    return valid;
}

// Detect the serialized NAL flavor from the bytes, not from the extension.
// Some extracted/modded scene-animation resources are named .PCANIM even
// though tlresource stores them as TLRESOURCE_TYPE_SCENE_ANIM (type 10).
// Keeping this helper outside the anonymous namespace lets the prerelease
// enumerator and resource_manager fail closed before selecting an override.
int modPCANIMDetectTLType(const uint8_t *raw, size_t size, int preferredType)
{
    nalPCAnimInfo animInfo;
    nalPCAnimInfo sceneInfo;
    const char *animReason = nullptr;
    const char *sceneReason = nullptr;

    const bool anim = nalValidateAnimImage(raw, size, animInfo, animReason);
    const bool scene = nalValidateSceneAnimImage(raw, size, sceneInfo, sceneReason);

    if (scene && !anim)
        return TLRESOURCE_TYPE_SCENE_ANIM;
    if (anim && !scene)
        return TLRESOURCE_TYPE_ANIM_FILE;
    if (anim && scene)
        return preferredType == TLRESOURCE_TYPE_SCENE_ANIM
             ? TLRESOURCE_TYPE_SCENE_ANIM
             : TLRESOURCE_TYPE_ANIM_FILE;
    return TLRESOURCE_TYPE_NONE;
}

// Register loose NAL images missed by a build's startup enumerator.  PCANIM
// identity is always the external filename (or a literal numeric stem), never
// its embedded name: every regular retail bank embeds the shared name
// "allanims", so using that value would make unrelated files collide.
void modScanNalOverrides()
{
    static bool scanned = false;
    if (scanned)
        return;
    scanned = true;

    for (const auto &rootDir : modRootDirs())
    {
        std::error_code ec;
        std::filesystem::recursive_directory_iterator it(
            rootDir, std::filesystem::directory_options::skip_permission_denied, ec);
        if (ec)
            continue;

        int registered = 0;
        for (const auto &entry : it)
        {
            std::error_code fileError;
            if (!entry.is_regular_file(fileError))
                continue;

            const std::filesystem::path path = entry.path();
            const std::string extension = transformToLower(path.extension().string());
            if (extension != ".pcanim" && extension != ".pcsanim"
                && extension != ".ps2anim" && extension != ".pcskel")
                continue;

            Mod mod;
            mod.Path = path;
            std::ifstream file(path, std::ios::binary);
            mod.Data.assign(std::istreambuf_iterator<char>(file),
                            std::istreambuf_iterator<char>());

            int type = TLRESOURCE_TYPE_NONE;
            if (extension == ".pcskel")
            {
                type = TLRESOURCE_TYPE_SKELETON;
            }
            else if (extension == ".ps2anim")
            {
                nalPCAnimInfo ps2Info;
                const char *ps2Reason = nullptr;
                if (nalValidatePS2AnimImage(mod.Data.data(), mod.Data.size(),
                                            ps2Info, ps2Reason))
                    type = TLRESOURCE_TYPE_ANIM_FILE;
                else
                    sp_log("[mod] PS2ANIM \"%s\": invalid (%s), ignored",
                           path.filename().string().c_str(),
                           ps2Reason != nullptr ? ps2Reason : "unknown PS2 animation format");
            }
            else
            {
                type = modPCANIMDetectTLType(
                    mod.Data.data(), mod.Data.size(),
                    extension == ".pcsanim" ? TLRESOURCE_TYPE_SCENE_ANIM
                                             : TLRESOURCE_TYPE_ANIM_FILE);
            }
            mod.Type = type;

            if (type == TLRESOURCE_TYPE_NONE)
            {
                if (extension != ".ps2anim")
                    sp_log("[mod] PCANIM \"%s\": invalid/unknown NAL flavor, ignored",
                           path.filename().string().c_str());
                continue;
            }

            if (type == TLRESOURCE_TYPE_SKELETON)
            {
                if (!nalFixedNameSane(mod.Data.data(), mod.Data.size(), 0x8))
                {
                    sp_log("[mod] skeleton \"%s\": invalid embedded name, ignored",
                           path.filename().string().c_str());
                    continue;
                }
                const uint32_t key = nalReadU32(mod.Data.data(), 0x8);
                if (!nalKeyAlreadyHasPath(key, path, type))
                {
                    Mods.emplace(key, std::move(mod));
                    ++registered;
                }
                continue;
            }

            nalPCAnimInfo animInfo;
            nalPCAnimInfo sceneInfo;
            const char *animReason = nullptr;
            const char *sceneReason = nullptr;
            bool anim = false;
            bool scene = false;
            if (extension == ".ps2anim")
            {
                anim = nalValidatePS2AnimImage(mod.Data.data(), mod.Data.size(),
                                               animInfo, animReason);
            }
            else
            {
                anim = nalValidateAnimImage(mod.Data.data(), mod.Data.size(),
                                            animInfo, animReason);
                scene = nalValidateSceneAnimImage(mod.Data.data(), mod.Data.size(),
                                                  sceneInfo, sceneReason);
            }
            if (!anim && !scene)
            {
                sp_log("[mod] %s \"%s\": invalid (%s), ignored",
                       extension == ".ps2anim" ? "PS2ANIM" : "PCANIM",
                       path.filename().string().c_str(),
                       animReason != nullptr ? animReason : "unknown format");
                continue;
            }

            const std::string stem = transformToLower(path.stem().string());
            const uint32_t stemHash = to_hash(stem.c_str());
            bool added = false;
            const auto aliases = nalPCAnimAliasesForPath(path);
            for (uint32_t alias : aliases)
            {
                if (!nalKeyAlreadyHasPath(alias, path, type))
                {
                    Mods.emplace(alias, mod);
                    added = true;
                }
            }

            const char *registeredKind =
                extension == ".ps2anim" ? "PS2ANIM"
                : (type == TLRESOURCE_TYPE_SCENE_ANIM ? "scene-PCANIM" : "PCANIM");
            sp_log("[mod] registered %s \"%s\" (%u bytes, key 0x%08X, %u aliases)",
                   registeredKind,
                   path.filename().string().c_str(),
                   static_cast<unsigned>(mod.Data.size()), stemHash,
                   static_cast<unsigned>(aliases.size()));
            if (added)
                ++registered;
        }

        sp_log("[mod] NAL override scan: \"%s\" -> %d new file(s)",
               rootDir.string().c_str(), registered);
    }
}

uint8_t *modPCANIMGetOverride(uint32_t nameHash,
                              int *sizeOut,
                              const uint8_t *originalImage,
                              int originalSize,
                              bool sceneFlavor)
{
    nalLastPCAnimOverrideForced = false;
    nalLastPCAnimResolvedTargetHash = 0;

    if (sizeOut != nullptr)
        *sizeOut = 0;
    if (nameHash == 0)
        return nullptr;

    modScanNalOverrides();

    const Mod *selected = nullptr;
    nalPCAnimInfo selectedInfo;
    int selectedScore = -1;
    bool selectedExplicitFilename = false;
    uint32_t selectedTargetHash = nameHash;

    const int expectedType = sceneFlavor ? TLRESOURCE_TYPE_SCENE_ANIM
                                         : TLRESOURCE_TYPE_ANIM_FILE;

    // Decode the retail image early. Normal PCANIMs often serialize only the
    // fixed-string "allanims", so the retail skeleton table is also useful as
    // a target-identity fallback when the pack filename hash was lost.
    nalPCAnimInfo originalInfo;
    bool haveOriginalInfo = false;
    const char *originalReason = nullptr;
    if (originalImage != nullptr && originalSize >= 0x70)
    {
        haveOriginalInfo = nalValidatePCAnim(
            originalImage, static_cast<size_t>(originalSize),
            sceneFlavor, originalInfo, originalReason);
    }

    auto filenameTargetsHash = [&](const Mod &mod, uint32_t targetHash)
    {
        const std::string stem = transformToLower(mod.Path.stem().string());
        if (to_hash(stem.c_str()) == targetHash)
            return true;
        uint32_t literal = 0;
        return nalParseLiteralHash(stem, literal) && literal == targetHash;
    };

    auto considerCandidate = [&](const Mod &mod,
                                 uint32_t scoreHash,
                                 bool forceExplicit)
    {
        const std::string extension = transformToLower(mod.Path.extension().string());
        if (mod.Type != expectedType || mod.Data.empty()
            || (extension != ".pcanim" && extension != ".pcsanim"))
            return;

        nalPCAnimInfo candidateInfo;
        const char *reason = nullptr;
        if (!nalValidatePCAnim(mod.Data.data(), mod.Data.size(), sceneFlavor,
                              candidateInfo, reason))
        {
            sp_log("[mod] PCANIM \"%s\" rejected for 0x%08X: %s",
                   mod.Path.filename().string().c_str(), scoreHash,
                   reason != nullptr ? reason : "wrong PCANIM flavor");
            return;
        }

        const int score = nalCandidateScore(mod, scoreHash);
        const bool explicitFilename = forceExplicit
                                   || filenameTargetsHash(mod, scoreHash);
        if (selected == nullptr || score > selectedScore
            || (score == selectedScore && explicitFilename
                && !selectedExplicitFilename))
        {
            selected = &mod;
            selectedInfo = std::move(candidateInfo);
            selectedScore = score;
            selectedExplicitFilename = explicitFilename;
            selectedTargetHash = scoreHash;
        }
    };

    // Normal hash/alias path.
    const auto range = Mods.equal_range(nameHash);
    for (auto it = range.first; it != range.second; ++it)
        considerCandidate(it->second, nameHash, false);

    if (selected == nullptr)
    {
        for (const auto &entry : Mods)
        {
            const Mod &mod = entry.second;
            if (mod.Type == expectedType && nalPCAnimPathMatchesHash(mod, nameHash))
                considerCandidate(mod, nameHash, false);
        }
    }

    // If the runtime only knows "allanims", infer the intended loose file from
    // the retail target bank's skeleton names. This specifically makes
    // extra/ULTIMATE_SPIDERMAN.PCANIM discoverable even when the serialized
    // PCANIM identity itself is only "allanims".
    if (selected == nullptr && haveOriginalInfo)
    {
        for (uint32_t skeletonHash : originalInfo.skeletonHashes)
        {
            for (const auto &entry : Mods)
            {
                const Mod &mod = entry.second;
                if (mod.Type != expectedType)
                    continue;
                if (filenameTargetsHash(mod, skeletonHash))
                    considerCandidate(mod, skeletonHash, true);
            }

            if (selected != nullptr)
            {
                sp_log("[mod] PCANIM request 0x%08X resolved through retail "
                       "skeleton 0x%08X -> \"%s\"",
                       nameHash, selectedTargetHash,
                       selected->Path.filename().string().c_str());
                break;
            }
        }
    }

    // Diagnostic fallback: if the identity is still just "allanims" and there
    // is exactly one loose regular PCANIM, select it instead of silently using
    // retail. Never guess when two different loose PCANIM paths are present.
    if (selected == nullptr && !sceneFlavor && nameHash == to_hash("allanims"))
    {
        const Mod *only = nullptr;
        std::filesystem::path onlyPath;
        bool ambiguous = false;

        for (const auto &entry : Mods)
        {
            const Mod &mod = entry.second;
            if (mod.Type != expectedType || mod.Data.empty()
                || transformToLower(mod.Path.extension().string()) != ".pcanim")
                continue;

            if (only == nullptr)
            {
                only = &mod;
                onlyPath = mod.Path;
            }
            else if (mod.Path != onlyPath)
            {
                ambiguous = true;
                break;
            }
        }

        if (!ambiguous && only != nullptr)
        {
            const std::string onlyStem =
                transformToLower(only->Path.stem().string());
            const uint32_t onlyHash = to_hash(onlyStem.c_str());
            considerCandidate(*only, onlyHash, true);
            if (selected != nullptr)
                sp_log("[mod] PCANIM allanims fallback -> sole loose bank \"%s\"",
                       selected->Path.filename().string().c_str());
        }
    }

    if (selected == nullptr)
    {
        sp_log("[mod] no loose %s override matched request 0x%08X",
               sceneFlavor ? "scene-PCANIM" : "PCANIM", nameHash);
        return nullptr;
    }

    // Exact filename-targeted replacement = explicit user intent. Keep the
    // strict contract check for ordinary aliases, but do not silently discard
    // a deliberate foreign-bank test (e.g. VENOM renamed to
    // ULTIMATE_SPIDERMAN.PCANIM).
    if (originalImage != nullptr)
    {
        if (originalSize <= 0)
        {
            if (!selectedExplicitFilename)
            {
                sp_log("[mod] PCANIM \"%s\" rejected for 0x%08X: original size is unknown",
                       selected->Path.filename().string().c_str(), nameHash);
                return nullptr;
            }
            sp_log("[mod] forcing filename-targeted PCANIM \"%s\": "
                   "retail size is unknown",
                   selected->Path.filename().string().c_str());
        }
        else if (!haveOriginalInfo)
        {
            if (!selectedExplicitFilename)
            {
                sp_log("[mod] PCANIM \"%s\" rejected for 0x%08X: original image is not "
                       "a compatible %s (%s)",
                       selected->Path.filename().string().c_str(), nameHash,
                       sceneFlavor ? "scene-PCANIM" : "PCANIM",
                       originalReason != nullptr ? originalReason : "invalid original image");
                return nullptr;
            }

            sp_log("[mod] forcing filename-targeted PCANIM \"%s\" although "
                   "retail validation failed (%s)",
                   selected->Path.filename().string().c_str(),
                   originalReason != nullptr ? originalReason : "invalid retail image");
        }
        else if (!nalSameSkeletonContract(selectedInfo, originalInfo))
        {
            if (!selectedExplicitFilename)
            {
                sp_log("[mod] PCANIM \"%s\" rejected for 0x%08X: skeleton contract "
                       "differs from retail (%d versus %d skeletons)",
                       selected->Path.filename().string().c_str(), nameHash,
                       selectedInfo.numSkeletons, originalInfo.numSkeletons);
                return nullptr;
            }

            sp_log("[mod] forcing filename-targeted PCANIM \"%s\": skeleton contract "
                   "differs from retail (%d versus %d skeletons)",
                   selected->Path.filename().string().c_str(),
                   selectedInfo.numSkeletons, originalInfo.numSkeletons);
        }
    }

    if (selected->Data.size() > static_cast<size_t>(std::numeric_limits<int>::max()))
        return nullptr;

    if (sizeOut != nullptr)
        *sizeOut = static_cast<int>(selected->Data.size());

    nalLastPCAnimOverrideForced = selectedExplicitFilename;
    nalLastPCAnimResolvedTargetHash = selectedTargetHash;

    sp_log("[mod] selected %s override \"%s\": request=0x%08X target=0x%08X "
           "forced=%d bytes=%u",
           sceneFlavor ? "scene-PCANIM" : "PCANIM",
           selected->Path.filename().string().c_str(),
           nameHash, selectedTargetHash,
           selectedExplicitFilename ? 1 : 0,
           static_cast<unsigned>(selected->Data.size()));

    if (originalImage != nullptr && originalSize > 0)
        nalExternalPCAnimImages()[originalImage] = {
            selectedTargetHash != 0 ? selectedTargetHash : nameHash,
            originalSize, sceneFlavor
        };

    return const_cast<uint8_t *>(selected->Data.data());
}

uint8_t *modPS2ANIMGetOverride(uint32_t nameHash,
                               int *sizeOut,
                               const uint8_t *originalImage,
                               int originalSize)
{
    if (sizeOut != nullptr)
        *sizeOut = 0;
    if (nameHash == 0)
        return nullptr;

    modScanNalOverrides();

    // Avoid emitting a "no PS2ANIM" line for every normal animation lookup.
    bool anyPS2Anim = false;
    for (const auto &entry : Mods)
    {
        const Mod &mod = entry.second;
        if (mod.Type == TLRESOURCE_TYPE_ANIM_FILE && !mod.Data.empty()
            && transformToLower(mod.Path.extension().string()) == ".ps2anim")
        {
            anyPS2Anim = true;
            break;
        }
    }
    if (!anyPS2Anim)
        return nullptr;

    const Mod *selected = nullptr;
    nalPCAnimInfo selectedInfo;
    int selectedScore = -1;
    uint32_t selectedTargetHash = nameHash;

    nalPCAnimInfo originalInfo;
    bool haveOriginalInfo = false;
    const char *originalReason = nullptr;
    if (originalImage != nullptr && originalSize >= 0x70)
    {
        haveOriginalInfo = nalValidateAnimImage(
            originalImage, static_cast<size_t>(originalSize),
            originalInfo, originalReason);
    }

    auto filenameTargetsHash = [&](const Mod &mod, uint32_t targetHash)
    {
        const std::string stem = transformToLower(mod.Path.stem().string());
        if (to_hash(stem.c_str()) == targetHash)
            return true;
        uint32_t literal = 0;
        return nalParseLiteralHash(stem, literal) && literal == targetHash;
    };

    auto considerCandidate = [&](const Mod &mod, uint32_t scoreHash)
    {
        if (mod.Type != TLRESOURCE_TYPE_ANIM_FILE || mod.Data.empty()
            || transformToLower(mod.Path.extension().string()) != ".ps2anim")
            return;

        nalPCAnimInfo candidateInfo;
        const char *reason = nullptr;
        if (!nalValidatePS2AnimImage(mod.Data.data(), mod.Data.size(),
                                     candidateInfo, reason))
        {
            sp_log("[mod] PS2ANIM \"%s\" rejected for 0x%08X: %s",
                   mod.Path.filename().string().c_str(), scoreHash,
                   reason != nullptr ? reason : "invalid PS2 animation");
            return;
        }

        const int score = nalCandidateScore(mod, scoreHash);
        if (selected == nullptr || score > selectedScore)
        {
            selected = &mod;
            selectedInfo = std::move(candidateInfo);
            selectedScore = score;
            selectedTargetHash = scoreHash;
        }
    };

    // Exact resource hash and all aliases generated by enumerate_mods/scan.
    const auto range = Mods.equal_range(nameHash);
    for (auto it = range.first; it != range.second; ++it)
        considerCandidate(it->second, nameHash);

    if (selected == nullptr)
    {
        for (const auto &entry : Mods)
        {
            const Mod &mod = entry.second;
            if (mod.Type == TLRESOURCE_TYPE_ANIM_FILE
                && nalPCAnimPathMatchesHash(mod, nameHash))
                considerCandidate(mod, nameHash);
        }
    }

    // Regular NAL banks serialize the shared name "allanims".  When that is
    // the only identity available, bind a filename such as venom.ps2anim to
    // the matching skeleton name from the retail target bank.
    if (selected == nullptr && haveOriginalInfo)
    {
        for (uint32_t skeletonHash : originalInfo.skeletonHashes)
        {
            for (const auto &entry : Mods)
            {
                const Mod &mod = entry.second;
                if (mod.Type == TLRESOURCE_TYPE_ANIM_FILE
                    && filenameTargetsHash(mod, skeletonHash))
                    considerCandidate(mod, skeletonHash);
            }
            if (selected != nullptr)
            {
                sp_log("[mod] PS2ANIM request 0x%08X resolved through retail "
                       "skeleton 0x%08X -> \"%s\"",
                       nameHash, selectedTargetHash,
                       selected->Path.filename().string().c_str());
                break;
            }
        }
    }

    // Same unambiguous diagnostic fallback used by PCANIM: a single loose
    // PS2 bank may stand in for an "allanims" identity, but never guess when
    // multiple different .ps2anim files are present.
    if (selected == nullptr && nameHash == to_hash("allanims"))
    {
        const Mod *only = nullptr;
        std::filesystem::path onlyPath;
        bool ambiguous = false;

        for (const auto &entry : Mods)
        {
            const Mod &mod = entry.second;
            if (mod.Type != TLRESOURCE_TYPE_ANIM_FILE || mod.Data.empty()
                || transformToLower(mod.Path.extension().string()) != ".ps2anim")
                continue;

            if (only == nullptr)
            {
                only = &mod;
                onlyPath = mod.Path;
            }
            else if (mod.Path != onlyPath)
            {
                ambiguous = true;
                break;
            }
        }

        if (!ambiguous && only != nullptr)
        {
            const uint32_t onlyHash =
                to_hash(transformToLower(only->Path.stem().string()).c_str());
            considerCandidate(*only, onlyHash);
            if (selected != nullptr)
                sp_log("[mod] PS2ANIM allanims fallback -> sole loose bank \"%s\"",
                       selected->Path.filename().string().c_str());
        }
    }

    if (selected == nullptr)
        return nullptr;

    // Unlike the old forced-PCANIM diagnostic path, PS2 animation never
    // bypasses the skeleton contract.  PS2 rotations are decoded against the
    // target skeleton's bind pose; accepting a foreign skeleton is a direct
    // route to displaced hips/legs even when the clip itself parses.
    if (originalImage != nullptr)
    {
        if (originalSize <= 0 || !haveOriginalInfo)
        {
            sp_log("[mod] PS2ANIM \"%s\" rejected for 0x%08X: retail animation "
                   "image/size is unavailable or invalid (%s)",
                   selected->Path.filename().string().c_str(), nameHash,
                   originalReason != nullptr ? originalReason : "unknown retail image");
            return nullptr;
        }

        if (!nalSameSkeletonContract(selectedInfo, originalInfo))
        {
            sp_log("[mod] PS2ANIM \"%s\" rejected for 0x%08X: skeleton contract "
                   "differs from retail (%d versus %d skeletons)",
                   selected->Path.filename().string().c_str(), nameHash,
                   selectedInfo.numSkeletons, originalInfo.numSkeletons);
            return nullptr;
        }
    }

    if (selected->Data.size() > static_cast<size_t>(std::numeric_limits<int>::max()))
        return nullptr;

    if (sizeOut != nullptr)
        *sizeOut = static_cast<int>(selected->Data.size());

    sp_log("[mod] selected PS2ANIM override \"%s\": request=0x%08X target=0x%08X "
           "bytes=%u; preserving nalChar v0x10002",
           selected->Path.filename().string().c_str(),
           nameHash, selectedTargetHash,
           static_cast<unsigned>(selected->Data.size()));

    // Prime the same one-shot identity bridge used by PCANIM.  The caller
    // keeps the retail shell; nalLoadAnimFileInternal parses a private copy.
    if (originalImage != nullptr && originalSize > 0)
        nalExternalPCAnimImages()[originalImage] = {
            selectedTargetHash != 0 ? selectedTargetHash : nameHash,
            originalSize, false
        };

    return const_cast<uint8_t *>(selected->Data.data());
}

void nalInit(nalHeap *a1) {
    TRACE("nalInit");

    if constexpr (1) {
        tlStackRangeInit();
        if (tlScratchPadRefCount++ == 0) {
            dword_970D64 = tlMemAlloc(0x4000, 16, 0x2000000u);
        }

        nalAnimPath[0] = 0;
        nalSkeletonPath[0] = 0;
        auto *mem = tlMemAlloc(20, 8, 0x2000000u);

        nalAnimFileDirectory = new (mem)
            tlInstanceBankResourceDirectory<nalAnimFile, tlFixedString>{};

        mem = tlMemAlloc(0x14, 8, 0x2000000u);
        nalAnimDirectory = new (mem)
            tlInstanceBankResourceDirectory<nalAnimClass<nalAnyPose>, tlFixedString>{};

        mem = tlMemAlloc(0x14, 8, 0x2000000u);
        nalSceneAnimDirectory = new (mem)
            tlInstanceBankResourceDirectory<nalSceneAnim, tlFixedString>{};

        mem = tlMemAlloc(0x14, 8, 0x2000000u);
        nalSkeletonDirectory = new (mem)
            tlInstanceBankResourceDirectory<nalBaseSkeleton, tlFixedString>{};

        nalTypeInstanceBank.Init();
        nalComponentInstanceBank.Init();
        nalInitListInit();

        auto *v10 = a1;
        if (v10 == nullptr) {
            nalDefaultHeap.field_4 = 0x100000;
            nalDefaultHeap.field_8 = 0;
            v10 = &nalDefaultHeap;
        }

        nalAnimationCache.field_8 = 0;
        nalAnimationCache.field_4 = 0;
        nalAnimationHeap = v10;
        nalAnimationCache.field_0 = v10;

    } else {
        CDECL_CALL(0x00783CF0, a1);
    }
}

void nalExit() {
    CDECL_CALL(0x00783C60);
}

namespace {

struct nalResourceIdentity {
    uint32_t hash = 0;
    int size = 0;
};

static thread_local const void *nalForcedIdentityImage = nullptr;
static thread_local nalResourceIdentity nalForcedIdentity;

static bool nalFindImageInDirectory(resource_directory *directory,
                                    const void *image,
                                    tlresource_type type,
                                    nalResourceIdentity &identity,
                                    std::set<resource_directory *> &visited)
{
    if (directory == nullptr || !visited.insert(directory).second)
        return false;

    const int count = directory->get_tlresource_count(type);
    for (int i = 0; i < count; ++i)
    {
        tlresource_location *location = directory->get_tlresource_location(i, type);
        if (location != nullptr && location->field_8 == image)
        {
            const uint32_t size = location->get_size();
            identity.hash = location->name.source_hash_code;
            identity.size = size <= static_cast<uint32_t>(std::numeric_limits<int>::max())
                          ? static_cast<int>(size) : 0;
            return identity.hash != 0 && identity.size > 0;
        }
    }

    for (size_t i = 0; i < directory->parents.size(); ++i)
        if (nalFindImageInDirectory(directory->parents.at(i), image, type,
                                    identity, visited))
            return true;
    return false;
}

// Packed anim-file images all call themselves "allanims".  Recover the
// actual resource/filename hash by matching the pack-owned image pointer to
// its tlresource_location.  StandardLoad-created images are not in that
// table, but it stamps their requested name into +0x10 and keeps their byte
// count in the tlFileBuf descriptor in the fixed header.
static nalResourceIdentity nalResolveImageIdentity(const void *image,
                                                   tlresource_type type,
                                                   uint32_t fallbackHash,
                                                   int fallbackSize)
{
    nalResourceIdentity identity {fallbackHash, fallbackSize};

    if (image == nalForcedIdentityImage)
        return nalForcedIdentity;

    std::set<resource_directory *> visited;

    if (resource_pack_slot *active = resource_manager::get_resource_context())
        if (active->is_data_ready()
            && nalFindImageInDirectory(&active->get_resource_directory(), image,
                                       type, identity, visited))
            return identity;

    // The handler being advanced can belong to a strip/parent slot other
    // than the currently pushed context.  Search every live pack slot by
    // pointer; no name guessing is involved.
    if (resource_manager::partitions != nullptr)
    {
        for (resource_partition *partition : *resource_manager::partitions)
        {
            if (partition == nullptr)
                continue;
            auto *slots = partition->get_streamer()->get_pack_slots();
            if (slots == nullptr)
                continue;
            for (resource_pack_slot *slot : *slots)
            {
                if (slot == nullptr || !slot->is_data_ready())
                    continue;
                if (nalFindImageInDirectory(&slot->get_resource_directory(), image,
                                            type, identity, visited))
                    return identity;
            }
        }
    }

    // resource_manager records the requested key for transient StandardLoad
    // shells which are not present in a live tlresource directory.  Consume
    // that bridge once: streamed buffers can later reuse the same address,
    // and an old pointer key must never retarget a different animation bank.
    auto &externalImages = nalExternalPCAnimImages();
    const auto external = externalImages.find(static_cast<const uint8_t *>(image));
    if (external != externalImages.end())
    {
        const nalExternalPCAnimIdentity cached = external->second;
        externalImages.erase(external);
        const bool fallbackNamesRequestedAnim =
                type == TLRESOURCE_TYPE_ANIM_FILE
                && fallbackHash == to_hash("allanims");
        if (cached.sceneFlavor == (type == TLRESOURCE_TYPE_SCENE_ANIM)
            && (fallbackHash == cached.hash || fallbackNamesRequestedAnim)
            && (fallbackSize <= 0 || fallbackSize == cached.size))
        {
            identity.hash = cached.hash;
            identity.size = cached.size;
        }
    }
    return identity;
}

static bool nalPCAnimDependenciesReady(const uint8_t *raw,
                                       size_t size,
                                       bool sceneFlavor,
                                       const char *&missingName)
{
    missingName = nullptr;
    nalPCAnimInfo info;
    const char *reason = nullptr;
    if (!nalValidatePCAnim(raw, size, sceneFlavor, info, reason)
        || nalSkeletonDirectory == nullptr)
        return false;

    for (int i = 0; i < info.numSkeletons; ++i)
    {
        const auto *name = reinterpret_cast<const tlFixedString *>(
            raw + info.skeletonTableOffset + size_t(i) * 0x20u);
        if (nalGetSkeleton(*name) == nullptr)
        {
            missingName = name->to_string();
            return false;
        }
    }
    return true;
}

static void *nalMakePersistentPCAnimCopy(const uint8_t *raw, int size)
{
    if (raw == nullptr || size <= 0)
        return nullptr;
    void *copy = tlMemAlloc(static_cast<uint32_t>(size), 16u, 0x2000000u);
    if (copy != nullptr)
        std::memcpy(copy, raw, static_cast<size_t>(size));
    return copy;
}

static std::map<nalSceneAnim *, void *> &nalOverriddenSceneShells()
{
    static std::map<nalSceneAnim *, void *> shells;
    return shells;
}

static std::map<nalAnimFile *, void *> &nalOverriddenAnimShells()
{
    static std::map<nalAnimFile *, void *> shells;
    return shells;
}

// Parsed PS2 copies are tracked separately so tlresource_directory can return
// their clips with override precedence without parsing/registering the same
// .ps2anim a second time.
static std::set<nalAnimFile *> &nalLoadedPS2AnimCopies()
{
    static std::set<nalAnimFile *> images;
    return images;
}

} // namespace

nalAnimClass<nalAnyPose> *modPS2ANIMFindLoadedClip(uint32_t clipHash)
{
    if (clipHash == 0)
        return nullptr;

    auto &images = nalLoadedPS2AnimCopies();
    for (auto imageIt = images.rbegin(); imageIt != images.rend(); ++imageIt)
    {
        nalAnimClass<nalAnyPose> *clip =
            reinterpret_cast<nalAnimClass<nalAnyPose> *>((*imageIt)->field_34);
        size_t guard = 0;
        while (clip != nullptr && guard++ < 65536u)
        {
            const uint32_t currentHash = *reinterpret_cast<const uint32_t *>(
                reinterpret_cast<const uint8_t *>(clip) + 0x08u);
            if (currentHash == clipHash)
                return clip;
            clip = clip->field_4;
        }
    }
    return nullptr;
}

void nalReleaseSceneAnimInternal(nalSceneAnim *a1) {
    if (a1 == nullptr)
        return;

    // Native release must run while the shell still points at the parsed copy.
    CDECL_CALL(0x0078D9B0, a1);
    nalExternalPCAnimImages().erase(reinterpret_cast<const uint8_t *>(a1));

    auto &shells = nalOverriddenSceneShells();
    const auto found = shells.find(a1);
    if (found != shells.end())
    {
        // These four fields were published from the persistent external image.
        // Clear every borrowed pointer/count before freeing that image so a
        // second unload or a recycled tlresource shell cannot walk stale data.
        a1->field_30 = 0;
        a1->field_34 = 0;
        a1->field_38 = 0;
        a1->field_3C = 0;
        tlMemFree(found->second);
        shells.erase(found);
    }
}

bool nalLoadSceneAnimInternal(nalSceneAnim *sceneAnim)
{
    if (sceneAnim == nullptr)
        return false;
    if (nalOverriddenSceneShells().count(sceneAnim) != 0)
        return true;

    // field_44 is serialized scene-animation state, not a trustworthy byte
    // count.  Packed tlresources obtain the exact size from tlresource_location
    // (forced by modSceneAnimResourceHandler below); StandardLoad uses the
    // one-shot resource_manager bridge.  Passing 0 here intentionally accepts
    // either exact source instead of rejecting it against an unrelated field.
    const nalResourceIdentity identity = nalResolveImageIdentity(
        sceneAnim, TLRESOURCE_TYPE_SCENE_ANIM,
        sceneAnim->field_10.m_hash, 0);

    int overrideSize = 0;
    uint8_t *overrideBytes = modPCANIMGetOverride(
        identity.hash, &overrideSize,
        identity.size > 0 ? reinterpret_cast<const uint8_t *>(sceneAnim) : nullptr,
        identity.size, true);
    // The getter may refresh resource_manager's pointer bridge while it
    // validates the retail shell.  This wrapper now owns that lookup, so do
    // not leave an address key behind for a later streamed allocation.
    nalExternalPCAnimImages().erase(reinterpret_cast<const uint8_t *>(sceneAnim));
    if (overrideBytes == nullptr)
        return static_cast<bool>(CDECL_CALL(0x0078D8D0, sceneAnim));

    const char *missingSkeleton = nullptr;
    if (!nalPCAnimDependenciesReady(overrideBytes,
                                    static_cast<size_t>(overrideSize), true,
                                    missingSkeleton))
    {
        // Native NAL dereferences the scene skeletons while rebasing.  Never
        // force an external image through when a dependency is missing: use
        // the known-good retail shell instead of turning a mod mismatch into
        // an access violation.
        sp_log("[mod] scene-PCANIM 0x%08X skipped: skeleton \"%s\" is not loaded",
               identity.hash,
               missingSkeleton != nullptr ? missingSkeleton : "<invalid>");
        return static_cast<bool>(CDECL_CALL(0x0078D8D0, sceneAnim));
    }

    auto *copy = static_cast<nalSceneAnim *>(
        nalMakePersistentPCAnimCopy(overrideBytes, overrideSize));
    if (copy == nullptr)
        return static_cast<bool>(CDECL_CALL(0x0078D8D0, sceneAnim));

    // Keep the target identity and ownership state on the parsed copy.  The
    // native routine only consumes the serialized fields and rebases its
    // lists in place.
    copy->field_10 = sceneAnim->field_10;
    copy->field_4 = sceneAnim->field_4;
    copy->field_4C = sceneAnim->field_4C;

    if (!static_cast<bool>(CDECL_CALL(0x0078D8D0, copy)))
    {
        tlMemFree(copy);
        return false;
    }

    // The pack/StandardLoad code continues to own and register sceneAnim.
    // Publish only the parsed scene-list fields; ownership/header state stays
    // on the caller's tlresource shell and the backing copy stays private.
    sceneAnim->field_30 = copy->field_30;
    sceneAnim->field_34 = copy->field_34;
    sceneAnim->field_38 = copy->field_38;
    sceneAnim->field_3C = copy->field_3C;
    nalOverriddenSceneShells()[sceneAnim] = copy;

    sp_log("[mod] loaded scene-PCANIM override 0x%08X (%d bytes)",
           identity.hash, overrideSize);
    return true;
}

bool nalLoadAnimFileInternal(nalAnimFile *anim_file)
{
    if (anim_file == nullptr)
        return false;
    if ((anim_file->field_4 & 8u) != 0)
        return true; // relative links have already been rebased

    int descriptorSize = anim_file->field_38[1];
    if (descriptorSize < 0x70)
        descriptorSize = 0;
    const nalResourceIdentity identity = nalResolveImageIdentity(
        anim_file, TLRESOURCE_TYPE_ANIM_FILE,
        anim_file->field_10.m_hash, descriptorSize);

    int overrideSize = 0;
    bool ps2Override = false;

    // PS2ANIM gets first refusal for a regular animation bank.  It is a real
    // PS2 nalAnimFile, not a renamed PCANIM: preserve its nalChar v0x10002
    // clips and let the patched version gate use the compatible PC entropy/
    // legs decoders.  If no PS2 file targets this bank, fall through to the
    // existing PCANIM override path unchanged.
    uint8_t *overrideBytes = modPS2ANIMGetOverride(
        identity.hash, &overrideSize,
        identity.size > 0 ? reinterpret_cast<const uint8_t *>(anim_file) : nullptr,
        identity.size);
    if (overrideBytes != nullptr)
    {
        ps2Override = true;
    }
    else
    {
        overrideBytes = modPCANIMGetOverride(
            identity.hash, &overrideSize,
            identity.size > 0 ? reinterpret_cast<const uint8_t *>(anim_file) : nullptr,
            identity.size, false);
    }

    nalExternalPCAnimImages().erase(reinterpret_cast<const uint8_t *>(anim_file));
    if (overrideBytes == nullptr)
        return static_cast<bool>(CDECL_CALL(0x0078D540, anim_file));

    const char *missingSkeleton = nullptr;
    if (!nalPCAnimDependenciesReady(overrideBytes,
                                    static_cast<size_t>(overrideSize), false,
                                    missingSkeleton))
    {
        sp_log("[mod] %s 0x%08X skipped: skeleton \"%s\" is not loaded",
               ps2Override ? "PS2ANIM" : "PCANIM",
               identity.hash,
               missingSkeleton != nullptr ? missingSkeleton : "<invalid>");
        return static_cast<bool>(CDECL_CALL(0x0078D540, anim_file));
    }

    auto *copy = static_cast<nalAnimFile *>(
        nalMakePersistentPCAnimCopy(overrideBytes, overrideSize));
    if (copy == nullptr)
        return static_cast<bool>(CDECL_CALL(0x0078D540, anim_file));

    copy->field_10 = anim_file->field_10;
    copy->field_4 = (copy->field_4 & ~4u) | (anim_file->field_4 & 4u);
    copy->field_44 = anim_file->field_44;
    if (!static_cast<bool>(CDECL_CALL(0x0078D540, copy)))
    {
        tlMemFree(copy);
        return false;
    }

    // Keep the caller/pack-owned header as the registered object.  Its list
    // now points into the persistent parsed copy, so the native unload walk
    // releases exactly the override clips without touching pristine mod data.
    anim_file->field_30 = copy->field_30;
    anim_file->field_34 = copy->field_34;
    anim_file->field_4 |= 8u;
    nalOverriddenAnimShells()[anim_file] = copy;
    if (ps2Override)
        nalLoadedPS2AnimCopies().insert(copy);

    sp_log("[mod] loaded %s override 0x%08X (%d bytes)%s",
           ps2Override ? "PS2ANIM" : "PCANIM",
           identity.hash, overrideSize,
           ps2Override ? " [nalChar v0x10002, native PC legs decoder]" : "");
    return true;
}

// Retail anim_resource_handler::_handle_resource, with one extra piece of
// context: tlresource_location carries the real filename hash that regular
// PCANIM headers deliberately omit (they all say "allanims").  Keeping this
// shim here also lets us release the persistent override copy at pack unload.
static bool __fastcall modAnimResourceHandler(void *handler,
                                              void *,
                                              int behavior,
                                              tlresource_location *location)
{
    auto *animFile = location != nullptr
                   ? reinterpret_cast<nalAnimFile *>(location->field_8)
                   : nullptr;
    if (animFile == nullptr)
    {
        sp_log("[mod] animation resource handler received a null PCANIM");
        if (handler != nullptr)
            ++*reinterpret_cast<int *>(static_cast<uint8_t *>(handler) + 0xC);
        return false;
    }

    if (behavior != 0) // worldly_resource_handler::UNLOAD
    {
        auto *clip = reinterpret_cast<nalAnimClass<nalAnyPose> *>(animFile->field_34);
        while (clip != nullptr)
        {
            nalAnimClass<nalAnyPose> *next = clip->field_4;
            using ReleaseFn = void (__fastcall *)(void *, void *);
            ReleaseFn release = reinterpret_cast<ReleaseFn>(get_vfunc(clip->m_vtbl, 0x8));
            release(clip, nullptr);
            clip = next;
        }

        auto &shells = nalOverriddenAnimShells();
        const auto found = shells.find(animFile);
        if (found != shells.end())
        {
            animFile->field_34 = nullptr;
            animFile->field_4 &= ~8u;
            auto *runtimeCopy = static_cast<nalAnimFile *>(found->second);
            nalLoadedPS2AnimCopies().erase(runtimeCopy);
            tlMemFree(runtimeCopy);
            shells.erase(found);
        }
        nalExternalPCAnimImages().erase(
            reinterpret_cast<const uint8_t *>(animFile));
    }
    else
    {
        const uint32_t packedSize = location->get_size();
        const void *previousImage = nalForcedIdentityImage;
        const nalResourceIdentity previousIdentity = nalForcedIdentity;
        nalForcedIdentityImage = animFile;
        nalForcedIdentity = {
            location->name.source_hash_code,
            packedSize <= static_cast<uint32_t>(std::numeric_limits<int>::max())
                ? static_cast<int>(packedSize) : 0
        };

        animFile->field_44 = 1;
        animFile->field_4 |= 4u;
        nalLoadAnimFileInternal(animFile);

        nalForcedIdentityImage = previousImage;
        nalForcedIdentity = previousIdentity;
    }

    if (handler != nullptr)
        ++*reinterpret_cast<int *>(static_cast<uint8_t *>(handler) + 0xC);
    return false;
}

// Scene tlresources need the same exact filename/size context as regular
// animation banks.  Hook the handler entry itself instead of guessing the
// identity later from the embedded fixedstring.
static bool __fastcall modSceneAnimResourceHandler(void *handler,
                                                   void *,
                                                   int behavior,
                                                   tlresource_location *location)
{
    auto *sceneAnim = location != nullptr
                    ? reinterpret_cast<nalSceneAnim *>(location->field_8)
                    : nullptr;
    if (sceneAnim == nullptr)
    {
        sp_log("[mod] scene animation resource handler received a null scene anim");
        if (handler != nullptr)
            ++*reinterpret_cast<int *>(static_cast<uint8_t *>(handler) + 0xC);
        return false;
    }

    if (behavior != 0) // worldly_resource_handler::UNLOAD
    {
        nalReleaseSceneAnimInternal(sceneAnim);
    }
    else
    {
        const uint32_t packedSize = location->get_size();
        const void *previousImage = nalForcedIdentityImage;
        const nalResourceIdentity previousIdentity = nalForcedIdentity;

        nalForcedIdentityImage = sceneAnim;
        nalForcedIdentity = {
            location->name.source_hash_code,
            packedSize <= static_cast<uint32_t>(std::numeric_limits<int>::max())
                ? static_cast<int>(packedSize) : 0
        };

        sceneAnim->field_4C = 1;
        sceneAnim->field_4 |= 4u;
        nalLoadSceneAnimInternal(sceneAnim);

        nalForcedIdentityImage = previousImage;
        nalForcedIdentity = previousIdentity;
    }

    if (handler != nullptr)
        ++*reinterpret_cast<int *>(static_cast<uint8_t *>(handler) + 0xC);
    return false;
}

void nalSetSkeletonDirectory(tlResourceDirectory<nalBaseSkeleton, tlFixedString> *a1) {
    nalSkeletonDirectory = CAST(nalSkeletonDirectory, a1);
}

void nalSetAnimFileDirectory(tlResourceDirectory<nalAnimFile, tlFixedString> *a1) {
    nalAnimFileDirectory = CAST(nalAnimFileDirectory, a1);
}

void nalSetAnimDirectory(tlResourceDirectory<nalAnimClass<nalAnyPose>, tlFixedString> *a1) {
    nalAnimDirectory = CAST(nalAnimDirectory, a1);
}

tlResourceDirectory<nalAnimClass<nalAnyPose>, tlFixedString> *nalGetAnimDirectory()
{
    return nalAnimDirectory;
}

void nalSetSceneAnimDirectory(tlResourceDirectory<nalSceneAnim, tlFixedString> *a1) {
    nalSceneAnimDirectory = CAST(nalSceneAnimDirectory, a1);
}

void nalStreamInstance_patch()
{

    REDIRECT(0x005AD21F, nalInit);

    REDIRECT(0x0055F8F4, nalConstructSkeleton);

    // PC nalChar::nalCharAnim::CheckVersion normally accepts only 0x10003.
    // PS2 .ps2anim character clips are 0x10002.  The PC and PS2 legs entropy
    // decoders in this engine generation use the same pose layout (including
    // the IK foot-position sanity limit), so accept v2 at the class gate
    // instead of rewriting the serialized data to v3.
    {
        FUNC_ADDRESS(address, &nalChar::nalCharAnim::CheckVersion);
        set_vfunc(0x00891FDC, address);
    }

    // Hook both TL handlers at their entries. They own the exact location
    // hash and byte size, so regular and scene animation banks cannot be
    // misidentified from their embedded names.
    SET_JUMP(0x0055F930, modAnimResourceHandler);
    SET_JUMP(0x0055F990, modSceneAnimResourceHandler);

    // Wrap every internal NAL load call while leaving the native parser
    // entries untouched.  The wrappers validate/copy, then call those native
    // entries without recursion.
    REDIRECT(0x0078D6C4, nalLoadAnimFileInternal);
    REDIRECT(0x0078D7A4, nalLoadAnimFileInternal);

    REDIRECT(0x0078DA94, nalLoadSceneAnimInternal);
    REDIRECT(0x0078DB74, nalLoadSceneAnimInternal);

    // Clear/free the persistent scene override copy on both packed and
    // StandardLoad release paths, preventing stale shell-address guards.
    REDIRECT(0x0078DBC3, nalReleaseSceneAnimInternal);
    return;


    {
        FUNC_ADDRESS(address, &nalGeneric::nalGenericSkeleton::Process);
        set_vfunc(0x008BD3D8, address);
    }

    {
        FUNC_ADDRESS(address, &nalComponentU8Base::GetType);
        SET_JUMP(0x004AE4C0, address);
    }

    {
        FUNC_ADDRESS(address, &nalComponentStringBase::GetType);
        SET_JUMP(0x004AE4D0, address);
    }

    {
        FUNC_ADDRESS(address, &nalComponentInitList::Register);
        //set_vfunc(0x00880958, address);
    }

    {
        FUNC_ADDRESS(address, &nalComp::nalCompSkeleton::UnMash);
        set_vfunc(0x00891FC8, address);
        set_vfunc(0x008AA300, address);
    }

#if 0
    {
        FUNC_ADDRESS(address, &nalStreamInstance::IsReady);
        set_vfunc(0x00880A8C, address);
    }

    //nalStreamInstance::Advance
    {
        REDIRECT(0x004985FB, nflReadFileAsync);

        REDIRECT(0x00498622, nflGetRequestInfo);

        REDIRECT(0x0049862B, nflGetRequestState);

        REDIRECT(0x004986FF, tlMemAlloc);
    }

    {
        FUNC_ADDRESS(address, &nalStreamInstance::Advance);
        //set_vfunc(0x00880A90, address);
    }

    {
        FUNC_ADDRESS(address, &nalStreamInstance::AdvanceStream);
        REDIRECT(0x00498943, address);
    }
#endif
}
