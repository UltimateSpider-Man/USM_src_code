#include "tl_system.h"

#include "common.h"
#include "func_wrapper.h"
#include "game.h" // MOD_MESH_SUPPORT must be defined for mesh unload cleanup.
#include "log.h"
#include "nal_system.h"
#include "ngl.h"
#include "ngl_font.h"
#include "ngl_mesh.h"
#include "nglshader.h"
#include "osassert.h"
#include "resource_directory.h"
#include "tl_instance_bank.h"
#include "tlresourcedirectory.h"
#include "trace.h"
#include "utility.h"
#include "variable.h"
#include "vtbl.h"

#include <cassert>
#include <malloc.h>
#include <windows.h>

using tldir_t = tlInstanceBankResourceDirectory<nglTexture, tlFixedString>;
VALIDATE_SIZE(tldir_t, 0x14);
VALIDATE_SIZE(tldir_t::SkipListIterator, 0xC);
VALIDATE_SIZE(tldir_t::Impl, 0x10);
VALIDATE_SIZE(tldir_t::Node, 0x8);

int & tlScratchPadRefCount = var<int>(0x00970D5C);

auto & tlHostPrefix = var<char[256]>(0x00970D88);

tlInstanceBank & nglShaderBank = var<tlInstanceBank>(0x00972840);

#if !STANDALONE_SYSTEM

int & tlMemAllocCounter = var<int>(0x00970D58);

tlSystemCallbacks & tlCurSystemCallbacks = var<tlSystemCallbacks>(0x00970D6C);

#else

#define make_var(type, name) \
    static type g_##name {}; \
    type & name {g_##name}

make_var(int, tlMemAllocCounter);

make_var(tlSystemCallbacks, tlCurSystemCallbacks);

#undef make_var
#endif

template<>
void *tlInstanceBankResourceDirectory<nglFont, tlFixedString>::Add(nglFont *p_font) {
    return (int *) THISCALL(0x00773F60, this, p_font);
}

template<>
int tlInstanceBankResourceDirectory<nglTexture, tlFixedString>::Impl::sub_770B80() {
    auto v2 = 0;

    int v3;
    do {
        v3 = this->field_0 & 3;
        if (v3 == 0) {
            ++v2;
        }

        this->field_0 >>= 2;
        if (this->field_4-- == 1) {
            this->field_0 = rand();
            this->field_4 = 7;
        }

    } while (v3 == 0);

    auto result = 15;
    if (v2 <= 15) {
        result = v2;
    }

    return result;
}

template<>
tlInstanceBankResourceDirectory<nalAnimFile, tlFixedString>::tlInstanceBankResourceDirectory() {
    m_vtbl = 0x008BDC98;

    field_4.field_0 = rand();
    field_4.field_4 = 7;
    field_4.field_8 = nullptr;
}

template<>
tlInstanceBankResourceDirectory<nalAnimClass<nalAnyPose>,
                                tlFixedString>::tlInstanceBankResourceDirectory() {
    m_vtbl = 0x008BDCEC;

    field_4.field_0 = rand();
    field_4.field_4 = 7;
    field_4.field_8 = nullptr;
}

template<>
tlInstanceBankResourceDirectory<nalSceneAnim, tlFixedString>::tlInstanceBankResourceDirectory() {
    m_vtbl = 0x008BDD40;

    field_4.field_0 = rand();
    field_4.field_4 = 7;
    field_4.field_8 = nullptr;
}

template<>
tlInstanceBankResourceDirectory<nalBaseSkeleton, tlFixedString>::tlInstanceBankResourceDirectory() {
    m_vtbl = 0x008BDD94;

    field_4.field_0 = rand();
    field_4.field_4 = 7;
    field_4.field_8 = nullptr;
}

template<>
void tlInstanceBankResourceDirectory<nglTexture, tlFixedString>::Impl::Init() {
    this->m_size = 0;
    this->field_8 = this->NewNodeOfLevel(15);

    for (auto i = 0; i < 16; ++i) {
        this->field_8->field_4[i] = nullptr;
    }
}

//0x00773CE0
template<>
void *tlInstanceBankResourceDirectory<nglTexture, tlFixedString>::Add(nglTexture *tex)
{
    TRACE("tlInstanceBankResourceDirectory<nglTexture, tlFixedString>::Add");

    if constexpr (1) {
        auto *v2 = &this->field_4;
        auto v3 = v2;
        auto v20 = v2;
        if (v2->field_8 == nullptr) {
            v2->Init();
        }

        auto v4 = v3->field_8;
        auto v21 = v3->m_size;
        auto *str0 = &tex->field_60;
        auto v22 = v4;

        Node *v23[16]{};

        Node *v7 = nullptr;
        do {
            v7 = v4->field_4[v21];
            if (v7 != nullptr) {
                do {
                    auto i = 0;
                    auto *v9 = &tex->field_60;
                    while ((*(&str0->m_hash + i)) == (*(&v7->field_0->field_60.m_hash + i))) {
                        ++i;
                        ++v9;
                        if (i >= 8) {
                            goto LABEL_11;
                        }
                    }

                    if ((*(&str0->m_hash + i) < *(&v7->field_0->field_60.m_hash + i) ? 1 : -1) >=
                        0) {
                        break;
                    }

                    v22 = v7;
                    v7 = v7->field_4[v21];
                } while (v7);
            LABEL_11:

                v3 = v20;
            }

            v4 = v22;
            v23[v21--] = v22;

        } while (v21 >= 0);

        if (v7 != nullptr) {
            auto func = [](const tlFixedString &a1, const tlFixedString &a2) -> bool {
                for (int i = 0; i < 8; ++i) {
                    if (*(&a1.m_hash + i) != *(&a2.m_hash + i)) {
                        return false;
                    }
                }

                return true;
            };

            auto *v10 = v7->field_0;
            if (func(v10->field_60, *str0)) {
                return v10;
            }
        }

        auto v14 = v3->sub_770B80();
        auto v15 = v3->m_size;
        if (v14 > v15) {
            auto *v16 = v3->field_8;
            v14 = v15 + 1;
            v3->m_size = v15 + 1;
            v23[v15 + 1] = v16;
        }

        printf("v14 = %d\n", v14);
        auto *v17 = static_cast<Node *>(tlMemAlloc(4 * v14 + 8, 8, 0x1000000u));
        v17->field_0 = tex;
        auto *v18 = &v17->field_4[v14];
        do {
            auto *v19 = v23[v14];
            *v18 = v19->field_4[v14];
            v19->field_4[v14--] = v17;
            --v18;
        } while (v14 >= 0);

        return nullptr;
    } else {
        return (void *) THISCALL(0x00773CE0, this, tex);
    }
}

template<>
nglFont *tlInstanceBankResourceDirectory<nglFont, tlFixedString>::Impl::Find(
    const tlFixedString &a1) {
    if (this->field_8 == nullptr) {
        return nullptr;
    }

    auto *v7 = this->field_8;
    auto v8 = this->m_size;

    Node *v6 = nullptr;
    do {
        while (1) {
            v6 = v7->field_4[v8];
            if (v6 == nullptr) {
                break;
            }

            auto *v3 = &v6->field_0->field_0;
            auto v5 = v3->compare(a1);
            if (v5 == 0) {
                return v6->field_0;
            }

            if (v5 > 0) {
                break;
            }

            v7 = v6;
        }

        --v8;
    } while (v8 >= 0);

    if (v6 != nullptr) {
        if (auto &v4 = v6->field_0->field_0; v4 == a1) {
            return v6->field_0;
        }
    }

    return nullptr;
}

template<>
nglFont *tlInstanceBankResourceDirectory<nglFont, tlFixedString>::Find(const tlFixedString &a1) {
    if constexpr (0) {
        return this->field_4.Find(a1);

    } else {
        return (nglFont *) THISCALL(0x00773E20, this, &a1);
    }
}

void tlReleaseFile(tlFileBuf *File)
{
#if MOD_MESH_SUPPORT && !defined(TARGET_XBOX)
    if (modReleaseMeshFileBuffer(File)) return;
#endif
    if (tlCurSystemCallbacks.ReleaseFile != nullptr) {
        tlCurSystemCallbacks.ReleaseFile(File);
    } else {
        char *fileBuf = File->Buf;

        --tlMemAllocCounter;
        if (tlCurSystemCallbacks.MemFree != nullptr) {
            tlCurSystemCallbacks.MemFree(fileBuf);
        } else {
            _aligned_free(fileBuf);
        }

        File->Buf = nullptr;
        File->Size = 0;
        File->UserData = 0;
    }
}

static int & tlStackBegin = var<int>(0x00970E88);
static int & tlStackEnd = var<int>(0x00970E8C);

void tlStackRangeInit() {
    tlStackBegin = 0;
    tlStackEnd = 0;
}

bool tlIsPow2(int a1) {
    return a1 && (a1 & (a1 - 1)) == 0;
}

void tlMemFree(void *Ptr) {
    --tlMemAllocCounter;

    if (tlCurSystemCallbacks.MemFree != nullptr) {
        tlCurSystemCallbacks.MemFree(Ptr);
    } else {
        _aligned_free(Ptr);
    }
}

void *tlMemAlloc(uint32_t Size, uint32_t Alignment, uint32_t Flags) {
    TRACE("tlMemAlloc", std::to_string(Size).c_str());

    void *memPtr;

    auto alignChk = Alignment;

    assert((Size & 15) == (Size % 16));

    if constexpr (0) {
        if ((Alignment == 0) && !(Size & 15)) {
            alignChk = 0;
        }
    } else {
        if (!(Size & 15)) {
            alignChk = (Alignment + 15) & 0xFFFFFFF0;
        }
    }

    ++tlMemAllocCounter;

    if (tlCurSystemCallbacks.MemAlloc != nullptr) {
        memPtr = tlCurSystemCallbacks.MemAlloc(Size, alignChk, Flags);
    } else {
        memPtr = _aligned_malloc(Size, alignChk);
    }

    if (!(Flags & 2) && (memPtr == nullptr) && Size) {
        error("Memory allocation failed. %d bytes, %d align", Size, alignChk);
    }

    return memPtr;
}

bool tlReadFile(const char *FileName, tlFileBuf *File, unsigned int Align, unsigned int Flags)
{
    TRACE("tlReadFile", FileName);

    if constexpr (1)
    {
        char *allocLoc;
        unsigned int bytesRead = 0;
        char Work[512] = {0};

        unsigned int alignment = Align;
        if (tlCurSystemCallbacks.ReadFile != nullptr) {
            return tlCurSystemCallbacks.ReadFile(FileName, File, Align, Flags);
        }

        if (strncmp(FileName, tlHostPrefix, strlen(tlHostPrefix)) == 0) {
            Work[0] = 0;
        } else {
            int fileLoc = 0;
            char fileRead = '\0';
            do {
                fileRead = tlHostPrefix[fileLoc];
                Work[fileLoc++] = fileRead;
            } while (fileRead);
        }

        size_t fileLen = strlen(FileName) + 1;
        auto *filePtr = (char *) &alignment + 3;
        while (*++filePtr) {
            ;
        }

        std::memcpy(filePtr, FileName, fileLen);
        for (auto *i = Work; *i; ++i) {
            if (*i == '/') {
                *i = '\\';
            }
        }

        HANDLE openedFile = CreateFileA(Work, 0x80000000, 1u, nullptr, 3u, 0x8000080u, nullptr);
        if (openedFile == INVALID_HANDLE_VALUE) {
            return false;
        }

        uint32_t fileSize = GetFileSize(openedFile, nullptr);
        File->Size = fileSize;
        if (!alignment && !(fileSize & 0xF)) {
            alignment = 0;
        }

        ++tlMemAllocCounter;
        if (tlCurSystemCallbacks.MemAlloc != nullptr) {
            allocLoc = static_cast<char *>(tlCurSystemCallbacks.MemAlloc(fileSize, alignment, Flags));
        } else {
            allocLoc = static_cast<char *>(_aligned_malloc(fileSize, alignment));
        }

        if (!(Flags & 2) && allocLoc == nullptr) {
            if (fileSize) {
                error("Memory allocation failed. %d bytes, %d align", fileSize, alignment);
            }
        }

        fileSize = File->Size;
        File->Buf = allocLoc;
        ReadFile(openedFile, allocLoc, fileSize, (LPDWORD) &bytesRead, nullptr);
        CloseHandle(openedFile);
        return true;
    }
    else
    {
        return (bool) CDECL_CALL(0x0074A710, FileName, File, Align, Flags);
    }
}

void tlSetSystemCallbacks(const tlSystemCallbacks &a1) {
    tlCurSystemCallbacks = a1;
}

void tlGetSystemCallbacks(tlSystemCallbacks *a1) {
    a1->ReadFile = tlCurSystemCallbacks.ReadFile;
    a1->ReleaseFile = tlCurSystemCallbacks.ReleaseFile;
    a1->field_8 = tlCurSystemCallbacks.field_8;
    a1->field_C = tlCurSystemCallbacks.field_C;
    a1->MemAlloc = tlCurSystemCallbacks.MemAlloc;
    a1->MemFree = tlCurSystemCallbacks.MemFree;
}

void set_tl_system_directories() {
    CDECL_CALL(0x0050EAD0);
}

//0x0078A1B0
template<>
nalBaseSkeleton *tlInstanceBankResourceDirectory<nalBaseSkeleton, tlFixedString>::Find(
    const tlFixedString &a1) {
    return (nalBaseSkeleton *) THISCALL(0x0078A1B0, this, &a1);
}

//0x00773030
template<>
int tlResourceDirectory<nglTexture, tlFixedString>::StandardRelease(nglTexture *tex,
                                                                    int a3,
                                                                    bool a4) {
    if constexpr (1) {
        if (tex == nullptr) {
            return 0;
        }

        auto v5 = (tex->field_34 >> 1) & 1;
        if (!a4 && v5) {
            return 1;
        }

        if (a3)
        {
            if (a3 == 1 && tex->m_format != NGLTEX_ANIMATED) {
                return tex->field_8;
            }

        } else {
            auto result = tex->field_8 - 1;
            auto v6 = tex->field_8 == 1;
            tex->field_8 = result;
            if (result >= 0 && !v6) {
                return result;
            }
        }

        if (tex->field_60.field_4[0]) {
            void (__fastcall *Del)(void *, void *, nglTexture *) = CAST(Del, get_vfunc(this->m_vtbl, 0x14));
            Del(this, nullptr, tex);
        }

        nglDestroyTexture(tex);
        return -(v5 != 0);
    } else {
        return THISCALL(0x00773030, this, tex, a3, a4);
    }
}

//0x005606D0
template<>
int tlResourceDirectory<nglTexture, tlFixedString>::Release(nglTexture *a1, int a2, bool a3) {
    return this->StandardRelease(a1, a2, a3);
}

//0x00773B90
template<>
nglTexture *tlInstanceBankResourceDirectory<nglTexture, tlFixedString>::Find(
    const tlFixedString &a1) {
    if constexpr (0) {
        return this->Find(a1);

    } else {
        return (nglTexture *) THISCALL(0x00773B90, this, &a1);
    }
}

//0x00773C60
template<>
bool tlInstanceBankResourceDirectory<nglTexture, tlFixedString>::Del(nglTexture *tex) {
    auto result = (bool) THISCALL(0x00773C60, this, tex);

    return result;
}

template<>
bool tlInstanceBankResourceDirectory<nglMesh, tlHashString>::Del(nglMesh *a2) {
    return (bool) THISCALL(0x00770850, this, a2);
}

//0x00773C70
template<>
tlInstanceBankResourceDirectory<nglTexture, tlFixedString>::SkipListIterator *
tlInstanceBankResourceDirectory<nglTexture, tlFixedString>::Enumerate() {
    if constexpr (1) {
        auto *iter = static_cast<SkipListIterator *>(tlMemAlloc(0xC, 8, 0x1000000u));
        if (iter == nullptr) {
            return nullptr;
        }

        iter->m_vtbl = 0x008B8D40;
        iter->field_4 = &this->field_4;
        auto *v3 = this->field_4.field_8;
        if (v3 != nullptr) {
            iter->field_8 = v3->field_4[0];
        } else {
            iter->field_8 = nullptr;
        }

        return iter;
    } else {
        return (tlInstanceBankResourceDirectory<nglTexture, tlFixedString>::SkipListIterator *)
            THISCALL(0x00773C70, this);
    }
}

//0x0078A160
template<>
void tlInstanceBankResourceDirectory<nglTexture, tlFixedString>::SkipListIterator::reset() {
    auto *v1 = this->field_4->field_8;
    if (v1 != nullptr) {
        this->field_8 = v1->field_4[0];
    } else {
        this->field_8 = nullptr;
    }
}

//0x00773CB0
template<>
nglTexture *tlInstanceBankResourceDirectory<nglTexture, tlFixedString>::SkipListIterator::operator*() {
    nglTexture *result = nullptr;
    auto v1 = this->field_8;
    if (v1 != nullptr) {
        result = v1->field_0;
    }

    return result;
}

//0x00778DF0
template<>
bool tlInstanceBankResourceDirectory<nglTexture, tlFixedString>::SkipListIterator::operator()() {
    return this->field_8 != nullptr;
}

//0x0077A160
template<>
void tlInstanceBankResourceDirectory<nglTexture, tlFixedString>::SkipListIterator::operator++() {
    auto *v1 = this->field_8;
    if (v1 != nullptr) {
        this->field_8 = v1->field_4[0];
    }
}

//0x00560770
template<>
void tlResourceDirectory<nglTexture, tlFixedString>::ReleaseAll(bool a2, bool a3, int a4) {
    if constexpr (1) {
        //sp_log("ReleaseAll: %d", a4);

        struct {
            int field_0[1];
            char * (__fastcall *DirectoryName)(void *);
            int field_8[4];

            tlInstanceBankResourceDirectory<nglTexture, tlFixedString>::SkipListIterator * (
                __fastcall *Enumerate)(void *);
            int field_1[3];
            int(__fastcall *Release)(void *, void *, nglTexture *, int, bool);
        } *vtbl = CAST(vtbl, this->m_vtbl);

        auto *iter = vtbl->Enumerate(this);
        for (int i = 1; i <= a4; ++i) {
            iter->reset();
            while ((*iter)()) {
                auto *v7 = *(*iter);
                auto v8 = vtbl->Release(this, nullptr, v7, i, a3);
                if (v8 > 0) {
                    ++(*iter);
                } else {
                    iter->reset();
                }

                if (!v8 && a2) {
                    vtbl->DirectoryName(this);
                }
            }
        }

        if (iter != nullptr) {
            iter->~SkipListIterator();
        }
    } else {
        THISCALL(0x00560770, this, a2, a3, a4);
    }
}

template<>
void tlInstanceBankResourceDirectory<nglMeshFile, tlFixedString>::Impl::Init() {
    this->m_size = 0;
    this->field_8 = this->NewNodeOfLevel(15);
    for (int i = 0; i < 16; ++i) {
        this->field_8->field_4[i] = nullptr;
    }
}

template<>
int tlInstanceBankResourceDirectory<nglMeshFile, tlFixedString>::Impl::sub_770B80() {
    auto v2 = 0;

    int v3;
    do {
        v3 = this->field_0 & 3;
        if (v3 == 0) {
            ++v2;
        }

        this->field_0 >>= 2;
        if (this->field_4-- == 1) {
            this->field_0 = rand();
            this->field_4 = 7;
        }

    } while (!v3);

    auto result = 15;
    if (v2 <= 15) {
        result = v2;
    }

    return result;
}

template<>
void *tlInstanceBankResourceDirectory<nglMeshFile, tlFixedString>::Impl::Add(nglMeshFile *meshFile) {
    if (this->field_8 == nullptr) {
        this->Init();
    }

    Node *v11[16]{};

    auto *a1a = nglMeshFile::get_string(meshFile);
    auto v9 = this->field_8;
    auto v12 = this->m_size;

    Node *v7 = nullptr;
    for (; v12 >= 0; --v12) {
        while (1) {
            v7 = v9->field_4[v12];
            if (v7 == nullptr) {
                break;
            }

            auto *v2 = nglMeshFile::get_string(v7->field_0);
            if (v2->compare(*a1a) >= 0) {
                break;
            }

            v9 = v7;
        }

        v11[v12] = v9;
    }

    if (v7 != nullptr) {
        auto *v3 = nglMeshFile::get_string(v7->field_0);
        if (*v3 == *a1a) {
            return v7->field_0;
        }
    }

    v12 = this->sub_770B80();
    if (v12 > this->m_size) {
        v12 = ++this->m_size;
        v11[v12] = this->field_8;
    }

    Node *v8 = this->NewNodeOfLevel(v12);
    v8->field_0 = meshFile;
    do {
        auto *v10 = v11[v12];
        v8->field_4[v12] = v10->field_4[v12];
        v10->field_4[v12--] = v8;
    } while (v12 >= 0);
    return nullptr;
}

template<>
void *tlInstanceBankResourceDirectory<nglMeshFile, tlFixedString>::Add(nglMeshFile *Mesh) {
    return this->field_4.Add(Mesh);
}

template<>
nglMeshFile *tlInstanceBankResourceDirectory<nglMeshFile, tlFixedString>::Impl::Find(
    const tlFixedString &a2) {
    if (this->field_8 == nullptr) {
        return nullptr;
    }

    auto *iter = this->field_8;
    int size = this->m_size;

    Node *v6;
    do {
        while (1) {
            v6 = iter->field_4[size];
            if (v6 == nullptr) {
                break;
            }

            tlFixedString *v3 = v6->GetString();

            auto v5 = v3->compare(a2);
            if (v5 == 0) {
                return v6->field_0;
            }

            if (v5 > 0) {
                break;
            }

            iter = v6;
        }

        --size;
    } while (size >= 0);

    if (v6 != nullptr) {
        tlFixedString *v4 = nglMeshFile::get_string(v6->field_0);
        if (v4->compare(a2)) {
            return v6->field_0;
        }
    }

    return nullptr;
}

template<>
nglMeshFile *tlInstanceBankResourceDirectory<nglMeshFile, tlFixedString>::Find(
    const tlFixedString &a2) {
    if constexpr (1) {
        return this->field_4.Find(a2);

    } else {
        return (nglMeshFile *) THISCALL(0x00778CE0, this, &a2);
    }
}

template<>
nglMesh *tlInstanceBankResourceDirectory<nglMesh, tlHashString>::Impl::Find(const tlHashString &a2) {
    if (this->field_8 == nullptr) {
        return nullptr;
    }

    Node *v7 = this->field_8;
    Node *v6 = nullptr;
    for (auto i = this->m_size; i >= 0; --i)
    {
        v6 = v7->field_4[i];
        if (v6 == nullptr) {
            break;
        }

        auto *v3 = nglMesh::get_string(v6->field_0);
        auto v5 = v3->compare(*bit_cast<tlFixedString *>(&a2));
        if (v5 == 0) {
            return v6->field_0;
        }

        if (v5 > 0) {
            break;
        }

        v7 = v6;
    }

    if (v6 != nullptr) {
        const auto v4 = *nglMesh::get_string(v6->field_0);
        if (v4 == *bit_cast<tlFixedString *>(a2)) {
            return v6->field_0;
        }
    }

    return nullptr;
}

template<>
nglMesh *tlInstanceBankResourceDirectory<nglMesh, tlHashString>::Find(const tlHashString &a2) {
    if constexpr (1) {
        return this->field_4.Find(a2);
    } else {
        return (nglMesh *) THISCALL(0x00770C30, this, &a2);
    }
}

struct tlInitList {
    static inline auto & head = var<void *>(0x00970D4C);
};

void tlInitListInit()
{
    TRACE("tlInitListInit");

    if constexpr (1)
    {
        for (auto *shader = static_cast<nglShader *>(tlInitList::head);
                shader != nullptr;
                shader = shader->field_4) {
            void (__fastcall *Register)(void *) = CAST(Register, get_vfunc(shader->m_vtbl, 0x0));
            Register(shader);
        }
    } else {
        CDECL_CALL(0x00749FD0);
    }
}

void tl_patch() {

    SET_JUMP(0x00749FD0, tlInitListInit);

    SET_JUMP(0x0074A710, tlReadFile);

    SET_JUMP(0x0074A5C0, tlMemAlloc);

    SET_JUMP(0x0074A600, tlMemFree);

    SET_JUMP(0x0074A6C0, tlReleaseFile);

    SET_JUMP(0x0074A520, tlSetSystemCallbacks);

    {
        auto func = &tlInstanceBankResourceDirectory<nglTexture, tlFixedString>::Add;

        FUNC_ADDRESS(address, func);
        set_vfunc(0x008B8D24, address);
    }

    {
        auto func = &tlInstanceBankResourceDirectory<nglMesh, tlHashString>::Find;

        FUNC_ADDRESS(address, func);
        set_vfunc(0x008B81E0, address);
    }


    return;

    {
        auto func = &tlInstanceBankResourceDirectory<nglTexture, tlFixedString>::Find;

        FUNC_ADDRESS(address, func);
        //set_vfunc(0x008B8D20, address);
    }

    {
        auto func = &tlInstanceBankResourceDirectory<nglTexture, tlFixedString>::Del;

        FUNC_ADDRESS(address, func);
        set_vfunc(0x008B8D28, address);
    }

    {
        auto func = &tlInstanceBankResourceDirectory<nglMeshFile, tlFixedString>::Find;

        FUNC_ADDRESS(address, func);
        set_vfunc(0x008B818C, address);
    }

    {
        auto func = &tlInstanceBankResourceDirectory<nglMeshFile, tlFixedString>::Add;

        FUNC_ADDRESS(address, func);
        set_vfunc(0x008B8190, address);
    }

    {
        auto func = &tlResourceDirectory<nglTexture, tlFixedString>::ReleaseAll;
        FUNC_ADDRESS(address, func);
        set_vfunc(0x008B8D30, address);
    }

    {
        tlInstanceBank::Node *(tlInstanceBank::*func)(
            const tlHashString &a2) = &tlInstanceBank::Search;
        FUNC_ADDRESS(address, func);
        //SET_JUMP(0x0074A310, address);
    }

    {
        FUNC_ADDRESS(address, &tlInstanceBank::Insert);
        //REDIRECT(0x007837C8, address);
        //REDIRECT(0x00402118, address);

        //REDIRECT(0x00402BF8, address);
        //REDIRECT(0x00403B08, address);

        REDIRECT(0x004115B9, address);
    }
}
