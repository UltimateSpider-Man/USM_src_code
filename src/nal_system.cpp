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
#include "utility/mod.h"
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

// ---------------------------------------------------------------------------
// mods/*.PCSKEL and mods/*.PCANIM overrides.
//
// Keys come from the file's own EMBEDDED resource name, not the filename:
// analysis of the extracted corpus shows the two can differ (BLACK_SUIT.PCSKEL
// is internally named "ultimate_spiderman"), and the lookup side only ever
// sees the embedded name. A skeleton's tlFixedString sits at +0x8, an anim
// file's at +0x10 - so a mod file can be called anything. The scan is lazy
// and idempotent, and skips anything the main mod enumeration already
// registered under the same key and type.
// ---------------------------------------------------------------------------
void modScanNalOverrides()
{
    static bool scanned = false;
    if (scanned)
        return;
    scanned = true;

    std::error_code ec;
    std::filesystem::directory_iterator it("mods", ec);
    if (ec)
        return;

    for (const auto &e : it)
    {
        std::error_code fec;
        if (!e.is_regular_file(fec))
            continue;

        const std::string ext = transformToLower(e.path().extension().string());
        int type = 0;
        size_t nameOffs = 0;
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
            continue;   // too small to be either format

        const auto *name = bit_cast<const tlFixedString *>(mod.Data.data() + nameOffs);
        const uint32_t key = name->m_hash;
        if (getMod(key, type) != nullptr)
            continue;   // the main mod enumeration got here first

        sp_log("[mod] registered %s override \"%s\" -> \"%s\" (%u bytes, key 0x%08X)",
               (type == TLRESOURCE_TYPE_SKELETON) ? "skeleton" : "anim-file",
               e.path().filename().string().c_str(),
               name->to_string(),
               (unsigned)mod.Data.size(),
               key);

        Mods.emplace(key, std::move(mod));
    }
}

bool nalLoadAnimFileInternal(nalAnimFile *anim_file)
{
    TRACE("nalLoadAnimFileInternal", anim_file->field_10.to_string(), 
            anim_file->field_48.to_string());

    // mods/*.PCANIM, deferred. Every retail anim file is internally named
    // "allanims" (verified across the extracted corpus), so name-matching
    // can't select an override. Instead, each mod anim file loads exactly
    // once, on the first call where EVERY skeleton it references resolves
    // through nalGetSkeleton - which is during the load of the pack that
    // brought those skeletons in, BEFORE that pack's own anims register.
    // nalAnimDirectory keeps the first registration and logs later ones as
    // "Duplicate anim", so same-named retail anims lose the race and the
    // mod's clips win globally.
    static bool s_overrideActive = false;
    if (!s_overrideActive)
    {
        modScanNalOverrides();

        static std::set<Mod *> s_loaded;
        for (auto &entry : Mods)
        {
            Mod &mod = entry.second;
            if (mod.Type != TLRESOURCE_TYPE_ANIM_FILE ||
                mod.Data.size() < 0x68 ||
                s_loaded.count(&mod) != 0)
                continue;

            const uint8_t *raw = mod.Data.data();
            const int numSkel = *bit_cast<const int *>(raw + 0xC);
            const uint32_t firstAnim = *bit_cast<const uint32_t *>(raw + 0x34);
            if (numSkel <= 0 || numSkel >= 64 ||
                firstAnim == 0 || firstAnim >= mod.Data.size() ||
                mod.Data.size() < 0x48 + (size_t)numSkel * 0x20)
            {
                s_loaded.insert(&mod);   // malformed, don't retry
                sp_log("[mod] anim file \"%s\": malformed header, ignored",
                       mod.Path.filename().string().c_str());
                continue;
            }

            bool ready = true;
            for (int i = 0; i < numSkel && ready; ++i)
            {
                const auto *skelName =
                    bit_cast<const tlFixedString *>(raw + 0x48 + (size_t)i * 0x20);
                if (nalGetSkeleton(*skelName) == nullptr)
                    ready = false;
            }
            if (!ready)
                continue;   // its pack hasn't loaded yet, stays pending

            auto *copy = static_cast<nalAnimFile *>(
                tlMemAlloc((uint32_t)mod.Data.size(), 16u, 0x2000000u));
            if (copy == nullptr)
                continue;

            std::memcpy(copy, mod.Data.data(), mod.Data.size());
            copy->field_4  |= 4u;
            copy->field_44  = 1;

            sp_log("[mod] loading anim overrides from \"%s\" (%u bytes, %d skeleton(s))",
                   mod.Path.filename().string().c_str(),
                   (unsigned)mod.Data.size(), numSkel);

            s_loaded.insert(&mod);
            s_overrideActive = true;
            nalLoadAnimFileInternal(copy);
            s_overrideActive = false;
        }
    }

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
            // field_4 is the byte offset of the next anim in the file image;
            // convert it to an absolute pointer WITHOUT pointer-arithmetic
            // scaling, and remember it - the loop advances on it below.
            nalAnimClass<nalAnyPose> *next = nullptr;
            if (anim_class->field_4) {
                next = bit_cast<nalAnimClass<nalAnyPose> *>(
                    bit_cast<uint32_t>(anim_class->field_4) +
                    bit_cast<uint32_t>(anim_class));
                anim_class->field_4 = next;
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

            anim_class = next;
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

    // The retail anim_resource_handler (0x0055F930) calls the anim-file
    // loader at 0x0078D540 internally; detouring the function entry routes
    // every caller through nalLoadAnimFileInternal above, which is where the
    // mods/*.PCANIM override lives. The reimplementation is complete (no
    // fallback into 0x0078D540), so the detour cannot recurse.
    SET_JUMP(0x0078D540, nalLoadAnimFileInternal);
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
