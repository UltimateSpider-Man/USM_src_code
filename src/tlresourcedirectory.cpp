#include "tlresourcedirectory.h"

#include "fixedstring.h"
#include "func_wrapper.h"
#include "log.h"

#include "nal_system.h"
#include "ngl.h"
#include "ngl_mesh.h"
#include "return_address.h"
#include "tlresource_directory.h"
#include "trace.h"
#include "utility.h"
#include "variables.h"
#include "vtbl.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <string>

template<>
nglTexture *tlResourceDirectory<nglTexture, tlFixedString>::StandardLoad(const tlFixedString &str) {
    sp_log("StandardLoad %s", str.field_4);

    if constexpr (1) {
        auto *v19 = this;
        auto *tex = static_cast<nglTexture *>(tlMemAlloc(sizeof(nglTexture), 8, 0x1000000u));
        memset(tex, 0, sizeof(nglTexture));

        static Var<nglTexture> stru_975AC0{0x00975AC0};

        tex->field_4 = stru_975AC0().field_4;
        tex->field_0 = &stru_975AC0();
        stru_975AC0().field_4 = tex;
        tex->field_4->field_0 = tex;
        tex->field_8 = 1;
        tex->field_60 = str;
        auto *v4 = str.field_4;
        auto v5 = 0;

        char Str1[256];
        strcpy(Str1, nglTexturePath());

        auto v7 = strlen(v4) + 1;
        auto *v8 = (char *) &v19 + 3;
        while (*++v8) {
            ;
        }

        std::memcpy(v8, v4, v7);
        auto *v10 = &Str1[&Str1[strlen(Str1) + 1] - &Str1[1]];

        char aIfl_1[] = ".ifl";

        *(uint32_t *) v10 = *(uint32_t *) aIfl_1;
        v10[4] = aIfl_1[4];

        bool v12;

        sp_log("StandardLoad: %s", Str1);

        tlFileBuf v18;
        if (nglLoadingIFL() || !tlReadFile(Str1, &v18, 4u, 0)) {
            char aDds[] = ".dds";
            *(uint32_t *) v10 = *(uint32_t *) aDds;
            v10[4] = aDds[4];
            if (!tlReadFile(Str1, &v18, 128u, 0)) {
                char aDdsmp_1[] = ".ddsmp";
                *(uint32_t *) v10 = *(uint32_t *) aDdsmp_1;
                *((uint16_t *) v10 + 2) = *(uint16_t *) &aDdsmp_1[4];
                v10[6] = aDdsmp_1[6];
                if (!tlReadFile(Str1, &v18, 128u, 0)) {
                    char aTga[] = ".tga";
                    *(uint32_t *) v10 = *(uint32_t *) aTga;
                    v10[4] = aTga[4];
                    if (!tlReadFile(Str1, &v18, 4u, 0)) {
                        tex->field_0->field_4 = tex->field_4;
                        tex->field_4->field_0 = tex->field_0;

                        tex->field_0 = tex;
                        tex->field_4 = tex;
                        tlMemFree(tex);
                        return nglDefaultTex();
                    }

                    STDCALL(0x007CA291,
                            g_Direct3DDevice(),
                            (uint8_t *) v18.Buf,
                            v18.Size,
                            (int *) &tex->DXTexture);

                    tlReleaseFile(&v18);
                    goto LABEL_19;
                }
            }

            v12 = nglLoadTextureTM2(tex, (uint8_t *) v18.Buf);
            tlReleaseFile(&v18);
        } else {
            v12 = nglLoadTextureIFL(tex, (uint8_t *) v18.Buf, v18.Size);
            tlReleaseFile(&v18);
        }

        if (!v12) {
            tex->field_0->field_4 = tex->field_4;
            tex->field_4->field_0 = tex->field_0;
            tex->field_0 = tex;
            tex->field_4 = tex;
            tlMemFree(tex);
            return nglDefaultTex();
        }

    LABEL_19:

        auto *vtbl = bit_cast<fastcall_call(*)[1]>(this->m_vtbl);
        void (__fastcall *Add)(void *, void *, nglTexture *) = CAST(Add, (*vtbl)[4]);

        Add(this, nullptr, tex);
        return tex;
    } else {
        return (nglTexture *) THISCALL(0x0077A8A0, this, &str);
    }
}

template<>
nglTexture *tlResourceDirectory<nglTexture, tlFixedString>::Load(const tlFixedString &a1) {
    sp_log("Load %s", a1.field_4);

    if constexpr (0) {
        return this->StandardLoad(a1);
    } else {
        return (nglTexture *) THISCALL(0x005606C0, this, &a1);
    }
}

template<>
nglFont *tlResourceDirectory<nglFont, tlFixedString>::StandardLoad(const tlFixedString &a1)
{
    TRACE("tlResourceDirectory<nglFont, tlFixedString>::StandardLoad", a1.to_string());

    if constexpr (0)
    {
        char Dest[256];
        _snprintf(Dest, 256u, "%s%s%s", nglTexturePath(), a1.to_string(), ".fdf");

        tlFileBuf fileBuf;
        if (!tlReadFile(Dest, &fileBuf, 4u, 0)) {

            sp_log("Unable to open %s.\n", Dest);
            return nullptr;
        }

        auto *font = create_and_parse_fdf(a1, fileBuf.Buf);
        tlReleaseFile(&fileBuf);
        auto *vtbl = bit_cast<fastcall_call(*)[5]>(this->m_vtbl);

        auto *func = (*vtbl)[4];
        assert(bit_cast<std::intptr_t>(func) == 0x00773F60);

        if (bit_cast<tlInstanceBankResourceDirectory<nglFont, tlFixedString> *>(this)->Add(font))
        {
            sp_log("Attempt to load already loaded font %s\n", a1.to_string());
        }

        return font;
    } else {
        return (nglFont *) THISCALL(0x00779220, this, &a1);
    }
}

template<>
nglTexture *tlResourceDirectory<nglTexture, tlFixedString>::Find(unsigned int) {
    return nullptr;
}

template<>
nglFont *tlResourceDirectory<nglFont, tlFixedString>::Load(const tlFixedString &a1) {
    return this->StandardLoad(a1);
}

template<>
nalAnimFile *tlResourceDirectory<nalAnimFile, tlFixedString>::StandardLoad(const tlFixedString &a1) {
    return (nalAnimFile *) THISCALL(0x0078D610, this, &a1);
}

template<>
nglMeshFile *tlResourceDirectory<nglMeshFile, tlFixedString>::StandardLoad(const tlFixedString &a1);

namespace {

using tlMeshFileBank = tlInstanceBankResourceDirectory<nglMeshFile, tlFixedString>;
using tlMeshBank = tlInstanceBankResourceDirectory<nglMesh, tlHashString>;

tlMeshBank::Node *tlMeshBankNext(tlMeshBank::Node *node, unsigned level)
{
    // Native skip-list nodes have a variable number of links after field_0;
    // the C++ ABI declaration only names the first link. Read the allocated
    // tail without indexing beyond that one-element declared array.
    tlMeshBank::Node *next = nullptr;
    std::memcpy(&next, reinterpret_cast<const unsigned char *>(node)
        + offsetof(tlMeshBank::Node, field_4) + level * sizeof(next), sizeof(next));
    return next;
}

nglMesh *__fastcall tlFindSystemMeshByHash(tlMeshBank *bank, void *, uint32_t hash)
{
    if (bank == nullptr || bank->field_4.field_8 == nullptr
        || bank->field_4.m_size < 0 || bank->field_4.m_size > 15) return nullptr;

    // Native Add sorts first by the unsigned hash, then by the full name.
    // XBXM only stores that hash, so its generated display name must not
    // participate in lookup. Both a four-byte hash and a normal PC name use
    // this same key; no lookup reads 32 bytes from a tlHashString.
    auto *node = bank->field_4.field_8;
    for (int level = bank->field_4.m_size; level >= 0; --level) {
        while (auto *next = tlMeshBankNext(node, static_cast<unsigned>(level))) {
            auto *mesh = next->field_0;
            if (mesh == nullptr || mesh->Name == nullptr) return nullptr;
            if (mesh->Name->m_hash >= hash) break;
            node = next;
        }
    }
    auto *next = tlMeshBankNext(node, 0);
    return next != nullptr && next->field_0 != nullptr
        && next->field_0->Name != nullptr && next->field_0->Name->m_hash == hash
        ? next->field_0 : nullptr;
}

nglMesh *__fastcall tlFindSystemMeshByName(
    tlMeshBank *bank, void *, const tlHashString *name)
{
    return name != nullptr ? tlFindSystemMeshByHash(bank, nullptr, name->GetHash()) : nullptr;
}

bool tlSystemMeshDirectoriesReady()
{
    return tlresource_directory<nglMeshFile, tlFixedString>::system_dir != nullptr
        && tlresource_directory<nglMesh, tlHashString>::system_dir != nullptr
        && tlresource_directory<nglMorphSet, tlHashString>::system_dir != nullptr
        && tlresource_directory<nglMaterialBase, tlHashString>::system_dir != nullptr;
}

// Packed directories only look up resources; their Add/Load/Release slots are
// intentionally empty. Loose files instead belong to the existing system
// banks. Use the same bank context while parsing and while native Release
// deletes their children, then restore the caller's active pack context.
class tlScopedSystemMeshDirectories {
    tlMeshFileBank *files = nglMeshFileDirectory();
    tlInstanceBankResourceDirectory<nglMesh, tlHashString> *meshes = nglMeshDirectory();
    tlInstanceBankResourceDirectory<nglMorphSet, tlHashString> *morphs = nglMorphDirectory();
    tlInstanceBankResourceDirectory<nglMaterialBase, tlHashString> *materials = nglMaterialDirectory();
    bool enabled;

public:
    explicit tlScopedSystemMeshDirectories(bool enable) : enabled(enable)
    {
        if (!enabled) return;
        nglMeshFileDirectory() = tlresource_directory<nglMeshFile, tlFixedString>::system_dir;
        nglMeshDirectory() = tlresource_directory<nglMesh, tlHashString>::system_dir;
        nglMorphDirectory() = tlresource_directory<nglMorphSet, tlHashString>::system_dir;
        nglMaterialDirectory() = tlresource_directory<nglMaterialBase, tlHashString>::system_dir;
    }

    ~tlScopedSystemMeshDirectories()
    {
        if (!enabled) return;
        nglMeshFileDirectory() = files;
        nglMeshDirectory() = meshes;
        nglMorphDirectory() = morphs;
        nglMaterialDirectory() = materials;
    }

    tlScopedSystemMeshDirectories(const tlScopedSystemMeshDirectories &) = delete;
    tlScopedSystemMeshDirectories &operator=(const tlScopedSystemMeshDirectories &) = delete;
};

nglMeshFile *tlFindSystemMeshFile(tlMeshFileBank *bank, const tlFixedString &name)
{
    if (bank == nullptr) return nullptr;
    auto find = bit_cast<nglMeshFile *(__fastcall *)(void *, void *, const tlFixedString *)>(
        get_vfunc(bank->m_vtbl, 0xC));
    return find(bank, nullptr, &name);
}

int tlReleaseSystemMeshFile(tlMeshFileBank *bank, nglMeshFile *file, int mode, bool force)
{
    const tlScopedSystemMeshDirectories scope(tlSystemMeshDirectoriesReady());
    // Native Release only removes the file-bank entry for a nonempty textual
    // name. A raw override can also be requested by hash alone; remove exactly
    // that entry before its final release instead of leaving a dangling node.
    if (file->FileName.field_4[0] == '\0' && (mode != 0 || file->field_120 == 1)) {
        auto del = bit_cast<bool (__fastcall *)(void *, void *, nglMeshFile *)>(
            get_vfunc(bank->m_vtbl, 0x14));
        del(bank, nullptr, file);
    }
    return THISCALL(0x0076F1C0, bank, file, mode, force);
}

nglMeshFile *__fastcall tlLoadLooseMeshFromPackedDirectory(
    void *, void *, const tlFixedString *name)
{
    auto *bank = tlresource_directory<nglMeshFile, tlFixedString>::system_dir;
    if (name == nullptr || !tlSystemMeshDirectoriesReady()) return nullptr;
    if (auto *existing = tlFindSystemMeshFile(bank, *name)) {
        ++existing->field_120;
        return existing;
    }
    // StandardLoad scopes the child directories and adds this file to bank.
    return bank->StandardLoad(*name);
}

int __fastcall tlReleaseLooseMeshFromPackedDirectory(
    void *, void *, nglMeshFile *file, int mode, bool force)
{
    auto *bank = tlresource_directory<nglMeshFile, tlFixedString>::system_dir;
    if (file != nullptr && tlFindSystemMeshFile(bank, file->FileName) == file)
        return tlReleaseSystemMeshFile(bank, file, mode, force);
    // The pack owns its original resources. Preserve the original empty
    // Release slot for any file which is not this system bank's exact object.
    return 0;
}

} // namespace

template<>
int tlResourceDirectory<nglMeshFile, tlFixedString>::Release(nglMeshFile *a2, int a3, bool a4) {
    auto *bank = tlresource_directory<nglMeshFile, tlFixedString>::system_dir;
    if (this == bank && a2 != nullptr && tlFindSystemMeshFile(bank, a2->FileName) == a2)
        return tlReleaseSystemMeshFile(bank, a2, a3, a4);
    return THISCALL(0x005606F0, this, a2, a3, a4);
}

template<>
nglMeshFile *tlResourceDirectory<nglMeshFile, tlFixedString>::StandardLoad(const tlFixedString &a1)
{
    TRACE("tlResourceDirectory<nglMeshFile, tlFixedString>::StandardLoad");

    if constexpr (1)
    {
        const tlScopedSystemMeshDirectories directoryScope(
            this == tlresource_directory<nglMeshFile, tlFixedString>::system_dir
            && tlSystemMeshDirectoriesReady());
        char Dest[256] {};
        std::string requestName = a1.to_string();
        if (requestName.size() > 7u) {
            std::string extension = requestName.substr(requestName.size() - 7u);
            std::transform(extension.begin(), extension.end(), extension.begin(),
                [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            if (extension == ".pcmesh" || extension == ".xbmesh")
                requestName.resize(requestName.size() - 7u);
        }
        auto *MeshFile = static_cast<nglMeshFile *>(tlMemAlloc(sizeof(nglMeshFile), 8, 0x1000000u));
        if (MeshFile == nullptr)
            return nullptr;

        // tlMemAlloc returns raw/recycled storage.  Clear all pointer-bearing
        // loader state before file reading/modBindRawMesh sees this shell; stale
        // FirstMesh/FileBuf values can otherwise make an address-reused character
        // load look like the previous live PCMESH instance.
        MeshFile->FileBuf.Buf = nullptr;
        MeshFile->FileBuf.Size = 0;
        MeshFile->FileBuf.UserData = 0;
        MeshFile->FirstMesh = nullptr;
        MeshFile->FirstMaterial = nullptr;
        MeshFile->FirstMorph = nullptr;
        MeshFile->field_134 = 0;
        MeshFile->field_144 = -1;

        strcpy(MeshFile->FilePath, nglMeshPath());
        MeshFile->FileName = a1;
        MeshFile->field_120 = 1;
        MeshFile->field_130 = false;

        nglMeshFile *result = nullptr;
        // Bind registered PCMESH/XBMESH bytes before reading: an external
        // mesh need not have an entry in the current PC pack. The typed
        // registry already decides root and same-name format precedence.
        const char *extension = ".pcmesh";
        bool haveFile = modBindRawMesh(MeshFile, extension);
        if (haveFile && MeshFile->FileBuf.Size >= 4u
            && std::memcmp(MeshFile->FileBuf.Buf, "XBXM", 4u) == 0)
            extension = ".xbmesh";

        // Keep the game's file callbacks/pack reader. Reusing its original
        // entry point avoids enabling the unrelated tl_patch() replacements.
        auto readNativeFile = [&](const char *suffix) {
            const int length = std::snprintf(Dest, sizeof(Dest), "%s%s%s",
                                             nglMeshPath(), requestName.c_str(), suffix);
            if (length < 0 || static_cast<size_t>(length) >= sizeof(Dest))
                return false;
            return static_cast<bool>(CDECL_CALL(
                0x0074A710, Dest, &MeshFile->FileBuf, 4u, 0u));
        };
        if (!haveFile) {
            haveFile = readNativeFile(extension);
            if (!haveFile) {
                if (MeshFile->FileBuf.Buf != nullptr)
                    tlReleaseFile(&MeshFile->FileBuf);
                extension = ".xbmesh";
                haveFile = readNativeFile(extension);
            }
        }
        if (haveFile)
        {
            if (nglLoadMeshFileInternal(a1, MeshFile, extension))
            {
                bool (__fastcall *Add)(void *, void *, nglMeshFile *) = CAST(Add, get_vfunc(this->m_vtbl, 0x10));

                if (Add(this, nullptr, MeshFile)) {
                    auto *v5 = a1.to_string();
                    sp_log("Attempt to load already loaded MeshFile %s\n", v5);
                }

                result = MeshFile;
            } else {
                // Failed conversion may already have allocated private
                // storage. Release both raw overrides and ordinary file data.
                tlReleaseFile(&MeshFile->FileBuf);
                tlMemFree(MeshFile);

                result = nullptr;
            }

        } else {
            auto *v3 = a1.to_string();
            sp_log("Unable to open mesh %s%s (.pcmesh or .xbmesh).\n", nglMeshPath(), v3);

            if (MeshFile->FileBuf.Buf != nullptr)
                tlReleaseFile(&MeshFile->FileBuf);
            tlMemFree(MeshFile);
            result = nullptr;
        }

        return result;
    }
    else
    {
        return (nglMeshFile *) THISCALL(0x00770000, this, &a1);
    }
}

template<>
nglMeshFile *tlResourceDirectory<nglMeshFile, tlFixedString>::Load(const tlFixedString &a1)
{
    return this->StandardLoad(a1);
}

void tlResourceDirectory_patch() {

    {
        auto func = &tlResourceDirectory<nglMeshFile, tlFixedString>::Load;
        FUNC_ADDRESS(address, func);
        set_vfunc(0x00888F90, address);
    }
    {
        auto func = &tlResourceDirectory<nglMeshFile, tlFixedString>::StandardLoad;
        FUNC_ADDRESS(address, func);
        SET_JUMP(0x00770000, address);
    }
    // Native pack Find already falls back to the system bank. Its Load and
    // Release slots do not, so bridge only these mesh-file operations.
    set_vfunc(0x00889698, bit_cast<std::intptr_t>(&tlLoadLooseMeshFromPackedDirectory));
    set_vfunc(0x0088969C, bit_cast<std::intptr_t>(&tlReleaseLooseMeshFromPackedDirectory));
    // Packed Find already delegates missing meshes to these two system-bank
    // slots. Retail leaves hash Find empty and compares all 32 name bytes in
    // name Find, which cannot match an XBXM hash against a friendly PC name.
    set_vfunc(0x008B81DC, bit_cast<std::intptr_t>(&tlFindSystemMeshByHash));
    set_vfunc(0x008B81E0, bit_cast<std::intptr_t>(&tlFindSystemMeshByName));
    {
        auto func = &tlResourceDirectory<nglMeshFile, tlFixedString>::Release;
        FUNC_ADDRESS(address, func);
        set_vfunc(0x008B81A8, address);
    }
    return;
    {
        auto func = &tlResourceDirectory<nglFont, tlFixedString>::StandardLoad;
        FUNC_ADDRESS(address, func);
        SET_JUMP(0x00779220, address);
    }

    {
        FUNC_ADDRESS(address, (&tlResourceDirectory<nglMeshFile, tlFixedString>::Release));

        //set_vfunc(0x00888F94, address);
        //set_vfunc(0x00889070, address);
        //set_vfunc(0x008B81A8, address);
    }

    {
        FUNC_ADDRESS(address, (&tlResourceDirectory<nglTexture, tlFixedString>::StandardLoad));
        //REDIRECT(0x005606C0, address);
    }

    {
        FUNC_ADDRESS(address, (&tlResourceDirectory<nglFont, tlFixedString>::Load));
        //set_vfunc(0x008B9BD4, address);
        //set_vfunc(0x008B9C00, address);
    }
}
