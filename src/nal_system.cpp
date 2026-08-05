#include "nal_system.h"

#include "common.h"
#include "func_wrapper.h"
#include "log.h"
#include "nal_anim.h"
#include "nal_component.h"
#include "nfl_system.h"
#include "osassert.h"
#include "tl_system.h"
#include "tlresource_directory.h"
#include "tlresource_location.h"
#include "trace.h"
#include "utility.h"
#include "vtbl.h"

#include <nal_list.h>
#include <nal_skeleton.h>

#include <cassert>

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


static bool nalEmbeddedNameSane(const uint8_t *raw, size_t size, size_t offs)
{
    if (raw == nullptr || size < offs + 0x20)
        return false;
    const char *text = bit_cast<const char *>(raw + offs + 4);
    size_t len = 0;
    while (len < 28 && text[len] != '\0')
        ++len;
    if (len == 0 || len == 28)          // empty, or not NUL-terminated
        return false;
    for (size_t i = 0; i < len; ++i)
    {
        const unsigned char c = (unsigned char)text[i];
        if (c < 0x20 || c > 0x7E)
            return false;
    }
    return true;
}

// Anim-file image gate, ONE definition on purpose: the scan below uses it
// on disk files and the deferred loop in nalLoadAnimFileInternal uses it on
// every image right before the retail loader touches it (FBX-synthesized
// images never pass through the scan). These checks cover everything the
// loader dereferences before the skeletons resolve: version 0x10101 (the
// retail loader answers anything else with error() + assert), a plausible
// skeleton count, the first clip offset inside the image, the skeleton name
// table inside the image, and a sane embedded name at +0x10.
static bool nalAnimImageUsable(const uint8_t *raw, size_t size,
                               uint32_t &version, int &numSkel)
{
    version = 0;
    numSkel = 0;
    if (raw == nullptr || size < 0x68)
        return false;
    version = *bit_cast<const uint32_t *>(raw);
    numSkel = *bit_cast<const int *>(raw + 0xC);
    const uint32_t firstAnim = *bit_cast<const uint32_t *>(raw + 0x34);
    return version == 0x10101u &&
           numSkel > 0 && numSkel < 64 &&
           firstAnim != 0 && firstAnim < size &&
           size >= 0x48 + (size_t)numSkel * 0x20 &&
           nalEmbeddedNameSane(raw, size, 0x10);
}

// Scene-anim image gate, ONE definition on purpose (same contract as
// nalAnimImageUsable above): the scan uses it to classify a .PCANIM the
// anim-file gate refused, and nalLoadSceneAnimInternal runs it on the
// registered bytes right before they replace a retail image. The two
// .PCANIM flavors share the extension; a scene image names itself at +0x8
// (anim files at +0x10). The retail scene loader validates its own version
// word, so this stays structural: not an anim-file image, non-zero name
// hash, printable embedded name.
static bool nalSceneAnimImageUsable(const uint8_t *raw, size_t size)
{
    if (raw == nullptr || size < 0x68)
        return false;

    uint32_t version = 0;
    int numSkel = 0;
    if (nalAnimImageUsable(raw, size, version, numSkel))
        return false;               // anim-file flavor, not a scene image

    if (*bit_cast<const uint32_t *>(raw + 0x8) == 0)
        return false;               // tlFixedString hash must be populated

    return nalEmbeddedNameSane(raw, size, 0x8);
}

// Lazy .PCANIM/.PCSKEL registration - the AUTHORITATIVE path for NAL
// overrides: it keys every image by the tlFixedString EMBEDDED in the file
// (+0x10 anim-file flavor, +0x8 scene flavor and skeletons), which is the
// key every NAL consumer actually looks up. enumerate_mods() additionally
// registers the same files eagerly under their FILENAME-stem hash (startup
// visibility, prerelease parity); the two coexist by design: when the stem
// equals the embedded name the same-path duplicate check below skips the
// re-registration, when they differ both entries live in Mods and the
// flush's path-keyed retire set makes sure the image still loads only once.
void modScanNalOverrides()
{
    static bool scanned = false;
    if (scanned)
        return;
    scanned = true;

#if MOD_MESH_SUPPORT
    // FBX takes -> synthesized PCANIM images (ngl.cpp); registers further
    // TLRESOURCE_TYPE_ANIM_FILE entries picked up by the deferred loop below
    extern void modBuildFbxAnimOverrides();

#endif

    for (const auto &rootDir : modRootDirs())
    {
    std::error_code ec;
    std::filesystem::recursive_directory_iterator it(
        rootDir, std::filesystem::directory_options::skip_permission_denied, ec);
    if (ec)
        continue;       // root absent: legitimate, stay silent

    int registered = 0;

    for (const auto &e : it)
    {
        std::error_code fec;
        if (!e.is_regular_file(fec))
            continue;

        const std::string ext = transformToLower(e.path().extension().string());
        int type = 0;
        size_t nameOffs = 0;
        bool sceneFlavor = false;
        if (ext == ".pcskel")
        {
            type = TLRESOURCE_TYPE_SKELETON;
            nameOffs = 0x8;
        }
        else if (ext == ".pcanim")
        {
            type = TLRESOURCE_TYPE_ANIM_FILE;
            nameOffs = 0x10;
        }
        else
        {
            continue;
        }

        Mod mod;
        mod.Path = e.path();
        mod.Type = type;

        std::ifstream f(e.path(), std::ios::binary);
        mod.Data.assign(std::istreambuf_iterator<char>(f),
                        std::istreambuf_iterator<char>());
        if (mod.Data.size() < 0x68)
        {
            sp_log("[mod] \"%s\": too small for a %s file (%u bytes), ignored",
                   e.path().filename().string().c_str(),
                   (type == TLRESOURCE_TYPE_SKELETON) ? "skeleton" : "anim",
                   (unsigned)mod.Data.size());
            continue;
        }

        // reject anything the retail loader would refuse loudly
        if (type == TLRESOURCE_TYPE_ANIM_FILE)
        {
            uint32_t version = 0;
            int numSkel = 0;
            if (!nalAnimImageUsable(mod.Data.data(), mod.Data.size(),
                                    version, numSkel))
            {
                // .PCANIM hides two flavors behind one extension: an image
                // the anim-file gate refuses may still be a scene anim,
                // which keeps its resource name at +0x8 instead of +0x10
                if (nalSceneAnimImageUsable(mod.Data.data(), mod.Data.size()))
                {
                    sceneFlavor = true;
                    nameOffs = 0x8;
                }
                else
                {
                    sp_log("[mod] anim file \"%s\": malformed header (version %08X, "
                           "%d skeleton(s)), ignored",
                           e.path().filename().string().c_str(),
                           (unsigned)version, numSkel);
                    continue;
                }
            }
        }
        else if (!nalEmbeddedNameSane(mod.Data.data(), mod.Data.size(), nameOffs))
        {
            // nalConstructSkeleton swaps the blob in verbatim, keyed by this
            // name: payload bytes here mean a foreign file, not a PCSKEL
            sp_log("[mod] skeleton file \"%s\": no embedded resource name, ignored",
                   e.path().filename().string().c_str());
            continue;
        }

        const auto *name = bit_cast<const tlFixedString *>(mod.Data.data() + nameOffs);
        const uint32_t key = name->m_hash;
        // every retail-style anim file is named "allanims", so several mod
        // packs legitimately share one key: only skip an entry that is the
        // SAME file (the main mod enumeration registered it first)
        {
            bool duplicate = false;
            auto range = Mods.equal_range(key);
            for (auto mit = range.first; mit != range.second; ++mit)
                if (mit->second.Type == type && mit->second.Path == e.path())
                {
                    duplicate = true;
                    break;
                }
            if (duplicate)
                continue;
        }

        sp_log("[mod] registered %s override \"%s\" -> \"%s\" (%u bytes, key 0x%08X)",
               (type == TLRESOURCE_TYPE_SKELETON) ? "skeleton"
                                                  : sceneFlavor ? "scene-anim"
                                                                : "anim-file",
               e.path().filename().string().c_str(),
               name->to_string(),
               (unsigned)mod.Data.size(),
               key);

        Mods.emplace(key, std::move(mod));
        ++registered;
    }

    // Logged even at 0: "why is my extra/ ignored" must be answerable from
    // the log alone - a missing line means the root does not exist (or is
    // not where the scan is anchored), a "-> 0" means it was walked and
    // nothing in it survived validation.
    sp_log("[mod] NAL override scan: \"%s\" -> %d file(s)",
           rootDir.string().c_str(), registered);
    }
}

// ---------------------------------------------------------------------------
// Deferred override consumption. modScanNalOverrides() only REGISTERS
// images; nothing may load them eagerly because a clip can only bind once
// every skeleton in its table is resolvable. Draining at the top of
// nalLoadAnimFileInternal (and at the scene entry) means override clips
// reach nalAnimDirectory BEFORE the retail image being loaded registers its
// own: the directory keeps the first registration, so the retail duplicate
// loses ("Duplicate anim %s found") and the override plays. Readiness turns
// true at exactly the right moment - the pack's skeletons are in by the
// time the pack's anim file (or scene) binds its first clip.
// ---------------------------------------------------------------------------
static void nalFlushPendingAnimOverrides()
{
    // scan before the reentrancy check so a caller that only wants the
    // registry populated (the scene lookup) always gets it
    modScanNalOverrides();

    // the flush loads through the very entry it is called from
    static bool s_overrideActive = false;
    if (s_overrideActive)
        return;

    // path-keyed retire set: FBX sibling images carry virtual "#<skel>"
    // path suffixes so each retires independently. Retirement is permanent
    // on purpose - the copies below are never registered in
    // nalAnimFileDirectory, so no unload walk ever removes their clips.
    static std::set<std::string> s_loaded;

    for (const auto &kv : Mods)
    {
        const Mod &mod = kv.second;
        if (mod.Type != TLRESOURCE_TYPE_ANIM_FILE || mod.Data.empty())
            continue;

        const std::string modKey = mod.Path.string();
        if (s_loaded.count(modKey))
            continue;

        const uint8_t *raw = mod.Data.data();

        // same gate the scan ran, re-run right before the retail-shaped
        // body dereferences anything: Mods also carries entries that never
        // passed this scan (FBX-synthesized images from ngl.cpp, the
        // generic enumeration), and scene-flavored images share the type -
        // those belong to nalLoadSceneAnimInternal, not to this path
        uint32_t version = 0;
        int numSkel = 0;
        if (!nalAnimImageUsable(raw, mod.Data.size(), version, numSkel))
            continue;

        // readiness: every skeleton in the image's table must already be
        // resolvable - exactly what the loader body error()+asserts on.
        // Not ready is not an error; a later flush picks the image up.
        bool ready = true;
        for (int i = 0; i < numSkel; ++i)
        {
            const auto *skelName = bit_cast<const tlFixedString *>(
                raw + 0x48 + (size_t)i * 0x20);
            if (nalGetSkeleton(*skelName) == nullptr)
            {
                ready = false;
                break;
            }
        }
        if (!ready)
            continue;

        // immortal copy, same contract as the skeleton and scene overrides:
        // the load below parses in place (offsets become pointers) and
        // nalAnimDirectory keeps clip pointers into the image, so the copy
        // is never freed; the bytes in Mods stay pristine as the source
        auto *copy = static_cast<nalAnimFile *>(
            tlMemAlloc((uint32_t)mod.Data.size(), 16u, 0x2000000u));
        if (copy == nullptr)
            continue;

        std::memcpy(copy, mod.Data.data(), mod.Data.size());
        copy->field_4 |= 4u;    // resident flags carried from the previous
        copy->field_44 = 1;     // drop's retail memory-image setup

        sp_log("[mod] loading anim override(s) from \"%s\" "
               "(%u bytes, %d skeleton(s))",
               mod.Path.filename().string().c_str(),
               (unsigned)mod.Data.size(), numSkel);

        s_loaded.insert(modKey);

        // RAII so the guard survives the loader error()ing out, and absorbs
        // any re-entry from inside the retail machinery
        struct OverrideScope
        {
            bool &f;
            explicit OverrideScope(bool &b) : f(b) { f = true; }
            ~OverrideScope() { f = false; }
        } scope(s_overrideActive);

        nalLoadAnimFileInternal(copy);
    }
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

void nalReleaseSceneAnimInternal(nalSceneAnim *a1) {
    CDECL_CALL(0x0078D9B0, a1);
}

bool nalLoadSceneAnimInternal(nalSceneAnim *a1) {
    return (bool) CDECL_CALL(0x0078D8D0, a1);
}

bool nalLoadAnimFileInternal(nalAnimFile *anim_file)
{
    TRACE("nalLoadAnimFileInternal", anim_file->field_10.to_string(), 
            anim_file->field_48.to_string());

    if (anim_file->field_0 != 0x10101)
    {
        error("Unsupported anim file version %x, current version is %x.\n",
              anim_file->field_0,
              0x10101);
    }

    if constexpr (1)
    {
        auto *v1 = &anim_file->field_48;
        auto **skeletons = static_cast<nalBaseSkeleton **>(tlMemAlloc(4 * anim_file->num_skeletons,
                                                           8,
                                                           0x2000000u));

        for (auto i = 0; i < anim_file->num_skeletons; ++i)
        {
            nalBaseSkeleton * (__fastcall *Find)(void *, void *, const tlFixedString *) = CAST(Find, get_vfunc(nalSkeletonDirectory->m_vtbl, 0xC));

            skeletons[i] = Find(nalSkeletonDirectory, nullptr, &v1[i]);
            if (skeletons[i] == nullptr)
            {
                auto v8 = anim_file->field_10.to_string();
                auto v3 = v1[i].to_string();
                error(
                    "The skeleton resource file %s was not found while loading animfile %s. "
                    "Perhaps something is wrong with the packer?\n",
                    v3,
                    v8);

                assert(0);
            }
        }

        nalAnimClass<nalAnyPose> *anim_class = nullptr;
        if (anim_file->field_34 != nullptr) {
            anim_file->field_34 += (unsigned int) anim_file;
            anim_class = CAST(anim_class, anim_file->field_34);
        }

        while (anim_class != nullptr)
        {
            if (anim_class->field_4) {
                anim_class->field_4 += (unsigned int) anim_class;
            }

            auto *v7 = skeletons[anim_class->field_28];
            anim_class->Skeleton = v7;
            auto *instance = nalTypeInstanceBank.Search(v7->field_28);
            if (instance == nullptr) {
                assert(0 && "couldn't find animation type instance");
            }

            auto vtbl = static_cast<nalInitListAnimType *>(instance->field_20)->anim_vtbl_ptr;
            anim_class->m_vtbl = vtbl;

            bool (__fastcall *CheckVersion)(void *) = CAST(CheckVersion, get_vfunc(anim_class->m_vtbl, 0xC));

            if (!CheckVersion(anim_class)) {
                auto *v3 = &anim_class->field_8;
                auto *v9 = v3->to_string();
                auto v4 = anim_class->Version;
                error("Unsupported anim version %x (%s).\n", v4, v9);
            }

            anim_class->InstanceCount = 0;

            void (__fastcall *Process)(void *) = CAST(Process, get_vfunc(anim_class->m_vtbl, 0x4));
            Process(anim_class);

            bool (__fastcall *Add)(void *, void *, nalAnimClass<nalAnyPose> *) = CAST(Add, get_vfunc(nalAnimDirectory->m_vtbl, 0x10));
            if (Add(nalAnimDirectory, nullptr, anim_class)) {
                auto *v6 = anim_class->field_8.to_string();
                sp_log("Duplicate anim %s found.\n", v6);
            }
        }

        tlMemFree(skeletons);
        anim_file->field_4 |= 8u;
        return true;
    } else {
        return (bool) CDECL_CALL(0x0078D540, anim_file);
    }
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