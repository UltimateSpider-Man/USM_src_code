#include "ngl.h"

#include "color32.h"
#include "common.h"
#include "custom_math.h"
#include "damage_morphs.h"
#include "femanager.h"
#include "filespec.h"
#include "fileusm.h"
#include "fixedstring.h"
#include "func_wrapper.h"
#include "igofrontend.h"
#include "igozoomoutmap.h"
#include "log.h"
#include "mash_info_struct.h"
#include "mash_config.h"
#include "matrix4x3.h"
#include "memory.h"
#include "ngl_dx_core.h"
#include "ngl_dx_scene.h"
#include "ngl_dx_palette.h"
#include "ngl_dx_texture.h"
#include "ngl_font.h"
#include "ngl_lighting.h"
#include "ngl_mesh.h"
#include "ngl_params.h"
#include "ngl_scene.h"
#include "ngl_support.h"
#include "ngl_vertexdef.h"
#include "ngldebugshader.h"
#include "nglemptyshader.h"
#include "nglshader.h"
#include "nglsortinfo.h"
#include "osassert.h"
#include "os_file.h"
#include "parse_generic_mash.h"
#include "resource_manager.h"
#include "return_address.h"
#include "shadow.h"
#include "timer.h"
#include "tl_instance_bank.h"
#include "tl_system.h"
#include "tlresource_directory.h"
#include "usbuildingsimpleshader.h"
#include "utility.h"
#include "variable.h"
#include "variables.h"
#include "vector2d.h"
#include "vector3d.h"
#include "vtbl.h"

#include <us_frontend.h>
#include <us_outline.h>
#include <us_pcuv_shader.h>
#include <us_person.h>
#include <us_street.h>

#include <ngl_dx_shader.h>
#include <ngl_dx_state.h>

#include <cassert>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <unordered_set>

#include "game.h"

#include <psapi.h>

#include <d3dx9shader.h>

#if MOD_MESH_SUPPORT
#   include "mod_mesh_import.h"
#   include "string_hash.h"
Mod* dbgReplaceMesh = nullptr;
#endif

// The mod texture caches are defined inside the mod block further down, which
// is itself inside #ifndef TARGET_XBOX. One switch keeps the declaration and
// the call sites (which are NOT inside that block) from drifting away from the
// definitions and turning into link errors on an Xbox build.
#if MOD_MESH_SUPPORT && !defined(TARGET_XBOX)
#   define MOD_TEX_CACHE 1
#else
#   define MOD_TEX_CACHE 0
#endif

#if MOD_TEX_CACHE
// Texture-cache invalidation, defined with the importer glue further down.
// Declared here because the texture lifetime functions (nglDestroyTexture,
// nglReleaseTexture, nglReleaseAllTextures) sit above that block and are the
// only honest signal that a pointer we cached has stopped being valid.
void modTexCacheForgetTexture(nglTexture *tex);
void modTexCacheNewEpoch(const char *why);
#endif

VALIDATE_SIZE(nglMeshNode, 0x98);

VALIDATE_SIZE(nglFont, 0x54);

VALIDATE_SIZE(nglMaterialBase, 0x50);

VALIDATE_SIZE(nglQuad, 0x64);

VALIDATE_SIZE(nglQuadNode, 0x70);

VALIDATE_SIZE(*nglFontDirectory(), 0x14);

VALIDATE_SIZE(nglStringNode, 0x2C);

VALIDATE_SIZE(nglTexture, 0x80);
VALIDATE_SIZE(nglPalette, 0xC);
VALIDATE_OFFSET(nglTexture, field_60, 0x60);

VALIDATE_SIZE(nglMeshFile, 0x148);
VALIDATE_OFFSET(nglMeshFile, FileBuf, 0x124);

VALIDATE_SIZE(nglMorphFile, 0x148);

VALIDATE_SIZE(nglMeshSection, 0x60);
VALIDATE_OFFSET(nglMeshSection, field_3C, 0x3C);

VALIDATE_OFFSET(nglMesh, NextMesh, 0x38);

VALIDATE_SIZE(nglDirectoryEntry, 12);

VALIDATE_SIZE(nglPerfomanceInfo, 0x88);

VALIDATE_SIZE(nglScratchBuffer_t, 0x58);

VALIDATE_SIZE(nglLightContext, 0x70);

VALIDATE_SIZE(nglRenderTextureState, 0x60);

Var<char[256]> nglMeshPath{0x00972710};

Var<nglTexture *> nglWhiteTex{0x00973840};

Var<bool> nglLoadingIFL{0x00973844};

Var<int> nglScratchMeshPos{0x00975310};

Var<nglScratchBuffer_t> nglScratchBuffer {0x00972A18};

Var<bool> g_valid_texture_format{0x00971F9D};

Var<unsigned int> nglTextureAnimFrame{0x0097383C};

Var<nglTexture *> nglDefaultTex{0x00973838};

Var<tlInstanceBank> nglVertexDefBank{0x009728A0};

VALIDATE_SIZE(nglDebugStruct, 0x28);
VALIDATE_OFFSET(nglDebugStruct, ShowPerfInfo, 0x18);

Var<nglDebugStruct> nglDebug{0x00975830};
Var<nglDebugStruct> nglSyncDebug{0x009758E0};

Var<nglPerfomanceInfo> nglPerfInfo{0x00975858};
Var<nglPerfomanceInfo> nglSyncPerfInfo{0x00975908};

Var<uint8_t *> nglListWorkPos = (0x00971F0C);

Var<int> nglFrame{0x00972904};

Var<nglMesh *> nglScratch{0x00973B14};

Var<char[256]> nglTexturePath{0x00973738};

Var<char[1024]> nglFontBuffer{0x00974E08};

Var<tlInstanceBankResourceDirectory<nglMeshFile, tlFixedString> *> nglMeshFileDirectory{0x00972814};

Var<tlInstanceBankResourceDirectory<nglFont, tlFixedString> *> nglFontDirectory{0x00974E00};

Var<tlInstanceBankResourceDirectory<nglTexture, tlFixedString> *> nglTextureDirectory = (0x00973730);

Var<tlInstanceBankResourceDirectory<nglMesh, tlHashString> *> nglMeshDirectory = (0x00972810);

Var<tlInstanceBankResourceDirectory<nglMorphSet, tlHashString> *> nglMorphDirectory{0x00972818};

Var<tlInstanceBankResourceDirectory<nglMaterialFile, tlFixedString> *> nglMaterialFileDirectory{
    0x0095C304};

Var<tlInstanceBankResourceDirectory<nglMaterialBase, tlHashString> *> nglMaterialDirectory{
    0x0095C1A0};

static Var<nglTexture *> nglFrontBufferTex{0x009754D0};
static Var<nglTexture *> nglBackBufferTex{0x009754D4};

Var<nglTexture> stru_975AC0{0x00975AC0};

Var<nglMesh *> nglDebugMesh_Sphere{0x00975998};

struct Renderer {
    int m_width;
    int m_height;
    char field_8;
    char field_9;
    char field_A;
    char field_B;
    tlFixedString field_C;
    int field_2C;
    char field_30;
    float field_34;
    float field_38;
};

static Var<Renderer> struct_972688{0x00972688};

int __stdcall hookD3DXAssembleShader(const char *data,
                                     UINT data_len,
                                     const D3DXMACRO *defines,
                                     ID3DXInclude *include,
                                     DWORD flags,
                                     ID3DXBuffer **shader,
                                     ID3DXBuffer **error_messages);

uint8_t *nglGetDebugFlagPtr(const char *Flag)
{
    if ( strcmpi(Flag, "ShowPerfInfo") == 0 ) {
        return &nglDebug().ShowPerfInfo;
    }

    if ( strcmpi(Flag, "ShowPerfBar") == 0 ) {
        return &nglDebug().ShowPerfBar;
    }

    if ( strcmpi(Flag, "ScreenShot") == 0 ) {
        return &nglDebug().ScreenShot;
    }

    if ( strcmpi(Flag, "DisableQuads") == 0 ) {
        return &nglDebug().DisableQuads;
    }

    if ( strcmpi(Flag, "DisableVSync") == 0 ) {
        return &nglDebug().DisableVSync;
    }

    if ( strcmpi(Flag, "DisableScratch") == 0 ) {
        return &nglDebug().DisableScratch;
    }

    if ( strcmpi(Flag, "DebugPrints") == 0 ) {
        return &nglDebug().DebugPrints;
    }

    if ( strcmpi(Flag, "DumpFrameLog") == 0 ) {
        return &nglDebug().DumpFrameLog;
    }

    if ( strcmpi(Flag, "DumpSceneFile") == 0 ) {
        return &nglDebug().DumpSceneFile;
    }

    if ( strcmpi(Flag, "DumpTextures") == 0 ) {
        return &nglDebug().DumpTextures;
    }

    if ( strcmpi(Flag, "DrawLightSpheres") == 0 ) {
        return &nglDebug().DrawLightSpheres;
    }

    if ( strcmpi(Flag, "DrawMeshSpheres") == 0 ) {
        return &nglDebug().DrawMeshSpheres;
    }

    if ( strcmpi(Flag, "DisableDuplicateMaterialWarning") == 0 ) {
        return &nglDebug().DisableDuplicateMaterialWarning;
    }

    if ( strcmpi(Flag, "DisableMissingTextureWarning") == 0 ) {
        return &nglDebug().DisableMissingTextureWarning;
    }

    if ( strcmpi(Flag, "RenderSingleNode") == 0 ) {
        return &nglDebug().RenderSingleNode;
    }

    return nullptr;
}

uint8_t nglGetDebugFlag(const char *Flag)
{
    auto *Ptr = nglGetDebugFlagPtr(Flag);

    uint8_t result = 0;
    if ( Ptr != nullptr ) {
        result = *Ptr;
    }

    return result;

}

void nglSetDebugFlag(const char *Flag, uint8_t Set)
{
    auto *Ptr = nglGetDebugFlagPtr(Flag);
    if ( Ptr != nullptr ) {
        *Ptr = Set;
    }

    nglSyncDebug() = nglDebug();
}

void nglDestroyTexture(nglTexture *a1) {
#if MOD_TEX_CACHE
    // Anything the mod texture cache holds for this pointer dies with it.
    modTexCacheForgetTexture(a1);
#endif
    CDECL_CALL(0x0077BB20, a1);
}

nglMesh *nglGetFirstMeshInFile(const tlFixedString &a1) {
    if constexpr (0) {
        auto *v1 = nglMeshFileDirectory()->Find(a1);
        if (v1 != nullptr) {
            return v1->FirstMesh;
        }

        return nullptr;

    } else {
        return (nglMesh *) CDECL_CALL(0x0076F050, &a1);
    }
}

math::VecClass<3, 1> sub_413E90(
        const vector4d &x_axis,
        const vector4d &arg8,
        const vector4d &y_axis,
        const vector4d &a3,
        const vector4d &z_axis,
        const vector4d &a7,
        const vector4d &a8)
{
    vector4d v14 = a8;
    v14.sub_413530(x_axis, arg8);
    v14.sub_411A50(y_axis, a3);
    
    math::VecClass<3, 1> result = v14 + z_axis * a7.z;
    return result;
}

math::VecClass<3, 1> sub_414360(const math::VecClass<3, 1> &a2, const math::MatClass<4, 3> &a3)
{
    vector4d a5;
    vector4d a4;
    vector4d a3a;
    vector4d a2a;

    a3.decompose(a2a, a3a, a4, a5);

    vector4d a1a = sub_413E90(a2a, a2, a3a, a2, a4, a2, a5);
    return math::VecClass<3, 1>{a1a};
}


void * nglMeshNode::operator new(size_t size)
{
    auto *mem = nglListAlloc(size, 64);
    return mem;
}


matrix4x4 nglMeshNode::sub_41D840()
{
    matrix4x4 result;

    if constexpr (0)
    {
        matrix4x4 v2 {};
        if ( (this->field_90->Flags & 1) != 0 )
        {
            v2 = nglCurScene()->WorldToView;
        }
        else
        {
            struct {
                matrix4x4 *field_0;
                matrix4x4 *field_4;
            } v4 {&this->field_0, &nglCurScene()->WorldToView};
            matrix4x4 v5;
            v5.sub_41D8A0(&v4);
            v2 = v5;
        }

        return v2;
    }
    else
    {
        THISCALL(0x0041D840, this, &result);
    }

    return result;
}

vector4d sub_7A5990(const vector4d &a2)
{
    vector4d v3 {};
    v3[0] = 1.0 / a2[0];
    v3[1] = 1.0 / a2[1];
    v3[2] = 1.0 / a2[2];
    v3[3] = 1.0 / a2[3];
    return v3;
}

void __fastcall sub_770FB0(void *self, vector4d &a2, vector4d &a3, vector4d &a4)
{
    THISCALL(0x00770FB0, self, &a2, &a3, &a4);
}

matrix4x3 sub_771210(void *a2)
{
    matrix4x3 result;
    vector4d a2a, a3, a4;
    sub_770FB0(a2, a2a, a3, a4);
    result[0] = a2a;
    result[1] = a3;
    result[2] = a4;
    return result;
}

matrix4x4 nglMeshNode::sub_419930()
{
    matrix4x4 result;

    if constexpr (0)
    {
        auto *v3 = this->field_90;
        if ( (v3->Flags & 2) != 0 )
        {
            auto v12 = sub_7A5990(v3->Scale);
            auto v2 = this->field_0;

            struct {
                void *field_0;
                void *field_4;
            } a2 {&v12, &v2};

            matrix4x4 v13 {};
            matrix4x3 v14 = sub_771210(&a2);
            std::memcpy(&v13, &v14, sizeof(v14));

            v13[3] = v2[3];

            result = v13;
        }
        else
        {
            result = this->field_0;
        }

        return result;
    }
    else
    {
        THISCALL(0x00419930, this, &result);
    }

    return result;
}

matrix4x4 nglMeshNode::sub_4199D0()
{
    matrix4x4 result;

    if constexpr (0)
    {
        if ( this->field_80 == nullptr )
        {
            auto *mem = nglListAlloc(64, 64);
            this->field_80 = new (mem) matrix4x4 {};
            auto v4 = this->sub_419930();
            *this->field_80 = sub_4150E0(v4);
        }

        result = *this->field_80;
    }
    else
    {
        THISCALL(0x004199D0, this, &result);
    }

    return result;
}

void sub_781F80(nglVertexBuffer *a1, int a2, uint32_t a3)
{
    CDECL_CALL(0x00781F80, a1, a2, a3);
}

bool nglVertexBuffer::createIndexBufferAndWriteData(const void *a2, int size)
{
    TRACE("nglVertexBuffer::createIndexBufferAndWriteData");

    bool result = false;

    if constexpr (0)
    {
        if (createIndexOrVertexBuffer(this, ResourceType::IndexBuffer, size, 0, 0, D3DPOOL_DEFAULT)) {
            return false;
        }

        void *data;
        this->m_indexBuffer->lpVtbl->Lock(this->m_indexBuffer, 0, size, &data, 0);
        memcpy(data, a2, size);
        this->m_indexBuffer->lpVtbl->Unlock(this->m_indexBuffer);

        return true;
    }
    else
    {
        bool (__fastcall *func)(void *, void *, const void *, int) = CAST(func, 0x007707D0);
        result = func(this, nullptr, a2, size);
    }

    return result;
}

bool nglVertexBuffer::createVertexBufferAndWriteData(const void *a2, uint32_t size, int)
{
    TRACE("nglVertexBuffer::createVertexBufferAndWriteData");

    if constexpr (0)
    {
        auto *buf = static_cast<const float *>(a2);

        sp_log("%f %f", buf[0], buf[1]);
    }

    if (createIndexOrVertexBuffer(this,
                                  ResourceType::VertexBuffer,
                                  size,
                                  0,
                                  0,
                                  D3DPOOL_MANAGED)) {
        return false;
    }

    void *data = nullptr;
    this->m_vertexBuffer->lpVtbl->Lock(this->m_vertexBuffer, 0, size, &data, 0);
    std::memcpy(data, a2, size);
    this->m_vertexBuffer->lpVtbl->Unlock(this->m_vertexBuffer);

    return true;
}

void nglDebugMesh_BuildBox(nglVertexDef_MultipassMesh<nglVertexDef_Debug_Base>::Iterator &a1,
                           math::VecClass<3, 0> a2,
                           math::VecClass<3, 0> a3) {
    CDECL_CALL(0x0077F0C0, &a1, a2, a3);
}

void nglMeshSetSphere(math::VecClass<3, 1> a1, Float a2) {
    CDECL_CALL(0x00775650, a1, a2);
}

bool nglVertexBuffer::createVertexBuffer(int size, uint32_t flags)
{
    TRACE("nglVertexBuffer::createVertexBuffer");
    return createIndexOrVertexBuffer(this,
                                     ResourceType::VertexBuffer,
                                     size,
                                     flags,
                                     0,
                                     (D3DPOOL) (~(uint8_t) (flags >> 9) & 1)) == 0;
}

void nglSetScissor(Float a1, Float a2, Float a3, Float a4)
{
    if constexpr (1) {
        nglCurScene()->sx1 = std::clamp<float>(a1, -1.0f, 1.0f);
        nglCurScene()->sy1 = std::clamp<float>(a2, -1.0f, 1.0f);
        nglCurScene()->sx2 = std::clamp<float>(a3, -1.0f, 1.0f);
        nglCurScene()->sy2 = std::clamp<float>(a4, -1.0f, 1.0f);

        float ScreenWidth;
        float ScreenHeight;
        if ( (nglCurScene()->field_334->field_34 & 4) != 0 )
        {
            ScreenWidth = nglGetScreenWidth();
            ScreenHeight = nglGetScreenHeight();
        }
        else
        {
            ScreenHeight = 480.0;
            ScreenWidth = 640.0;
        }

        nglCurScene()->field_354[0] = ((a1 + 1.0f) * 0.5f * ScreenWidth + 0.5f);
        nglCurScene()->field_354[2] = ((a3 + 1.0f) * 0.5f * ScreenWidth + 0.5f);
        nglCurScene()->field_354[1] = ((a2 + 1.0f) * 0.5f * ScreenHeight + 0.5f);
        nglCurScene()->field_354[3] = ((a4 + 1.0f) * 0.5f * ScreenHeight + 0.5f);
        nglCalculateMatrices(true);
    } else {
        CDECL_CALL(0x0076B4D0, a1, a2, a3, a4);
    }
}

void nglSetView(Float x1, Float y1, Float x2, Float y2)
{
    nglCurScene()->vx1 = x1;
    nglCurScene()->vy1 = y1;
    nglCurScene()->vx2 = x2;
    nglCurScene()->vy2 = y2;
    nglCalculateMatrices(true);
}

void nglSetViewport(Float a1, Float a2, Float a3, Float a4)
{
    TRACE("nglSetViewport");

    if constexpr (1) {
        float ScreenWidth;
        float ScreenHeight;
        if ( (nglCurScene()->field_334->field_34 & 4) != 0 )
        {
            ScreenWidth = nglGetScreenWidth();
            ScreenHeight = nglGetScreenHeight();
        }
        else
        {
            auto *tex = nglCurScene()->field_334;
            int width = tex->m_width;
            ScreenWidth = width;
            if ( width < 0 ) {
                ScreenWidth += 4.2949673e9;
            }

            int height = tex->m_height;
            ScreenHeight = height;
            if ( height < 0 ) {
                ScreenHeight += 4.2949673e9;
            }
        }

        auto v9 = 1.0f / ScreenWidth;
        auto a1a = a1 * v9 + a1 * v9 - 1.0f ;
        auto v10 = a3 + 1.0f;
        auto a3a = v10 * v9 + v10 * v9 - 1.0f;
        auto v11 = 1.0f / ScreenHeight;
        auto a2a = a2 * v11 + a2 * v11 - 1.0f;
        auto a4a = (a4 + 1.0f) * v11 + (a4 + 1.0f) * v11 - 1.0f;

        nglSetView(a1a, a2a, a3a, a4a);
        nglSetScissor(a1a, a2a, a3a, a4a);
    } else {
        CDECL_CALL(0x0076B6D0, a1, a2, a3, a4);
    }
}

void nglDumpCamera(const math::MatClass<4, 3> &a1)
{
    CDECL_CALL(0x00782210, &a1);
}

void nglSetWorldToViewMatrix(const math::MatClass<4, 3> &a1)
{
    TRACE("nglSetWorldToViewMatrix");

    nglCurScene()->WorldToView = a1;
    nglCalculateMatrices(true);

    if (nglSyncDebug().DumpSceneFile) {
        nglDumpCamera(a1);
    }
}

void nglSetZTestEnable(bool a1)
{
    nglCurScene()->ZTestEnable = a1;
}

void nglSetZWriteEnable(bool a1)
{
    nglCurScene()->ZWriteEnable = a1;
}

math::VecClass<3, 1> nglProjectPoint(math::VecClass<3, 1> a2)
{
    math::VecClass<3, 1> result;
    CDECL_CALL(0x0076BAA0, &result, a2);

    return result;
}

void nglProjectPoint(math::VecClass<3, 1> &a1, math::VecClass<3, 1> a2)
{
    a1 = nglProjectPoint(a2);
}

nglParamSet<nglSceneParamSet_Pool> * nglGetSceneParams()
{
    return &nglCurScene()->field_404;
}

void nglListAddNode(nglRenderNode *node)
{
    if constexpr (0)
    {
        nglSortInfo v2 {};
        node->GetSortInfo(v2);
        node->m_tex = v2.Tex;
        if ( v2.Type == NGLSORT_TRANSLUCENT )
        {
            node->m_next_node = nglCurScene()->TransNodes;
            nglCurScene()->TransNodes = node;
            ++nglCurScene()->TransListCount;
        }
        else
        {
            node->m_next_node = nglCurScene()->field_340;
            nglCurScene()->field_340 = node;
            ++nglCurScene()->OpaqueListCount;
        }
    }
    else
    {
        CDECL_CALL(0x0040FF00, node);
    }
}

HRESULT nglVertexBuffer::createIndexOrVertexBuffer(nglVertexBuffer *a1,
                                                            ResourceType resource_type,
                                                            int32_t size,
                                                            uint32_t usage,
                                                            uint32_t fvf,
                                                            D3DPOOL pool)
{
    HRESULT result;
    TRACE("nglVertexBuffer::createIndexOrVertexBuffer");

    if constexpr (0) {

        if (resource_type == ResourceType::VertexBuffer && pool == D3DPOOL_DEFAULT) {
            sub_781F80(a1, size, usage);
        }

        int num;
        if (usage & D3DUSAGE_DYNAMIC) {
            num = 20;
        }
        else if (size >= 1000) {
            if (size >= 10000) {
                num = 19;
            }
            else {
                num = size / 1000 + 9;
            }
        }
        else {
            num = size / 100;
        }

        const auto start_idx = 21 * resource_type;

        const auto end_idx = start_idx + num;


        struct Struct_77B1C0 {
            int field_0;
            void* m_buffer;
            int field_8;
            Struct_77B1C0* field_C;
            Struct_77B1C0* field_10;
            int m_size;
        };

        static Var<Struct_77B1C0* [42]> dword_9753C0{ 0x009753C0 };

        static Var<Struct_77B1C0* [42]> dword_975318{ 0x00975318 };

        auto** v11 = dword_975318() + end_idx;

        auto* v10 = *v11;
        if (v10 != nullptr) {
            Struct_77B1C0* v13;

            while (1) {
                v13 = *v11;
                if (v13 != nullptr) {
                    break;
                }

            LABEL_16:
                if (num < 19) {
                    ++v11;
                    ++num;

                    if (*v11 != nullptr) {
                        continue;
                    }
                }

                goto LABEL_18;
            }

            while (v13->m_size < size) {
                v13 = v13->field_10;
                if (v13 == nullptr) {
                    goto LABEL_16;
                }
            }

            auto* v16 = v13->field_C;
            if (v16 != nullptr) {
                auto* v17 = v13->field_10;
                if (v17 != nullptr) {
                    v16->field_10 = v17;
                    v13->field_10->field_C = v13->field_C;
                }
                else {
                    v13->field_C->field_10 = nullptr;

                    dword_9753C0()[start_idx + num] = v13->field_C;
                }
            }
            else if (v13->field_10 != nullptr) {
                v13->field_10->field_C = nullptr;

                dword_975318()[start_idx + num] = v13->field_10;
            }
            else {
                auto v18 = start_idx + num;
                dword_975318()[v18] = nullptr;
                dword_9753C0()[v18] = nullptr;
            }

            auto* v19 = v13->m_buffer;
            if (resource_type == ResourceType::IndexBuffer) {
                a1->m_indexBuffer = CAST(a1->m_indexBuffer, v19);
            }
            else {
                a1->m_vertexBuffer = CAST(a1->m_vertexBuffer, v19);
            }

            operator delete(v13);

            static Var<int[2]> dword_975474{ 0x00975474 };
            --dword_975474()[resource_type];
            result = 0;
        }
        else {
        LABEL_18:
            if (resource_type) {
                result = g_Direct3DDevice()->lpVtbl->CreateIndexBuffer(g_Direct3DDevice(),
                    size,
                    0,
                    D3DFMT_INDEX16,
                    D3DPOOL_MANAGED,
                    &a1->m_indexBuffer,
                    nullptr);
            }
            else {
                result = g_Direct3DDevice()->lpVtbl->CreateVertexBuffer(g_Direct3DDevice(),
                    size,
                    usage,
                    fvf,
                    pool,
                    &a1->m_vertexBuffer,
                    nullptr);
            }
        }
    }
    else
    {
        result = (HRESULT)CDECL_CALL(0x77b440, a1, resource_type, size, usage, fvf, pool);
    }
    return result;
}

void nglVertexBuffer::sub_77B5D0(nglVertexBuffer *a1, ResourceType a2) {
    CDECL_CALL(0x0077B5D0, a1, a2);
}

using SetFVF_t = decltype(g_Direct3DDevice()->lpVtbl->SetFVF);
SetFVF_t origSetFVF;

HRESULT STDMETHODCALLTYPE HookSetFVF(IDirect3DDevice9 *This,
                                                 DWORD FVF
                                                 ) {
    TRACE("HookSetFVF");

    if (FVF != 0) {
        sp_log("FVF = 0x%08X", FVF);
    }

    auto result = origSetFVF(This, FVF);

    return result;
}

using CreateVertexBuffer_t = decltype(g_Direct3DDevice()->lpVtbl->CreateVertexBuffer);
CreateVertexBuffer_t origCreateVertexBuffer;

HRESULT STDMETHODCALLTYPE HookCreateVertexBuffer(IDirect3DDevice9 *This,
                                                 UINT Length,
                                                 DWORD Usage,
                                                 DWORD FVF,
                                                 D3DPOOL Pool,
                                                 IDirect3DVertexBuffer9 **ppVertexBuffer,
                                                 HANDLE *pSharedHandle) {
    TRACE("HookCreateVertexBuffer");

    if (FVF != 0) {
        sp_log("FVF = 0x%08X", FVF);
    }

    auto result = origCreateVertexBuffer(This, Length, Usage, FVF, Pool, ppVertexBuffer, pSharedHandle);
    //printf("0x%08X\n", (*ppVertexBuffer)->lpVtbl);

    return result;
}


using CreateIndexBuffer_t = decltype(g_Direct3DDevice()->lpVtbl->CreateIndexBuffer);

CreateIndexBuffer_t origCreateIndexBuffer;

HRESULT STDMETHODCALLTYPE HookCreateIndexBuffer(IDirect3DDevice9 *This,
                                                 UINT Length,
                                                 DWORD Usage,
                                                 D3DFORMAT Format,
                                                 D3DPOOL Pool,
                                                 IDirect3DIndexBuffer9 **ppIndexBuffer,
                                                 HANDLE *pSharedHandle) {
    TRACE("HookCreateIndexBuffer");

    auto result = origCreateIndexBuffer(This, Length, Usage, Format, Pool, ppIndexBuffer, pSharedHandle);
    //printf("0x%08X\n", (*ppIndexBuffer)->lpVtbl);

    return result;
}

using DrawPrimitive_t = decltype(g_Direct3DDevice()->lpVtbl->DrawPrimitive);

DrawPrimitive_t origDrawPrimitive;

HRESULT STDMETHODCALLTYPE HookDrawPrimitive(IDirect3DDevice9 *This,
                                            D3DPRIMITIVETYPE PrimitiveType,
                                            UINT StartVertex,
                                            UINT PrimitiveCount) {
    //sp_log("HookDrawPrimitive: return to 0x%08X", getReturnAddress());

    return origDrawPrimitive(This, PrimitiveType, StartVertex, PrimitiveCount);
}

using DrawPrimitiveUP_t = decltype(g_Direct3DDevice()->lpVtbl->DrawPrimitiveUP);

DrawPrimitiveUP_t origDrawPrimitiveUP;

HRESULT STDMETHODCALLTYPE HookDrawPrimitiveUP(IDirect3DDevice9 *This,
                                              D3DPRIMITIVETYPE primitive_type,
                                              UINT primitive_count,
                                              const void *data,
                                              UINT stride) {
    //sp_log("HookDrawPrimitiveUP: return to 0x%08X", getReturnAddress());

    return origDrawPrimitiveUP(This, primitive_type, primitive_count, data, stride);
}

using SetViewport_t = decltype(g_Direct3DDevice()->lpVtbl->SetViewport);
SetViewport_t origSetViewport;

HRESULT STDMETHODCALLTYPE HookSetViewport(IDirect3DDevice9 *This, const D3DVIEWPORT9 *pViewport)
{
    TRACE("HookSetViewport");

    return origSetViewport(This, pViewport);
}

using DrawIndexedPrimitiveUP_t = decltype(g_Direct3DDevice()->lpVtbl->DrawIndexedPrimitiveUP);
DrawIndexedPrimitiveUP_t origDrawIndexedPrimitiveUP;

HRESULT STDMETHODCALLTYPE HookDrawIndexedPrimitiveUP(IDirect3DDevice9 *This,
                                                     D3DPRIMITIVETYPE primitive_type,
                                                     UINT min_vertex_idx,
                                                     UINT vertex_count,
                                                     UINT primitive_count,
                                                     const void *index_data,
                                                     D3DFORMAT index_format,
                                                     const void *data,
                                                     UINT stride) {
    sp_log("HookDrawIndexedPrimitiveUP: return to 0x%08X", getReturnAddress());

    return origDrawIndexedPrimitiveUP(This,
                                      primitive_type,
                                      min_vertex_idx,
                                      vertex_count,
                                      primitive_count,
                                      index_data,
                                      index_format,
                                      data,
                                      stride);
}

using DrawIndexedPrimitive_t = decltype(g_Direct3DDevice()->lpVtbl->DrawIndexedPrimitive);
DrawIndexedPrimitive_t origDrawIndexedPrimitive;

HRESULT STDMETHODCALLTYPE HookDrawIndexedPrimitive(IDirect3DDevice9 *This,
                                                     D3DPRIMITIVETYPE primitive_type,
                                                     int BaseVertexIndex,
                                                     uint32_t MinVertexIndex,
                                                     uint32_t NumVertices,
                                                     uint32_t startIndex,
                                                     uint32_t primCount)
{
    TRACE("HookDrawIndexedPrimitive");

    return origDrawIndexedPrimitive(This,
                                     primitive_type,
                                     BaseVertexIndex,
                                     MinVertexIndex,
                                     NumVertices,
                                     startIndex,
                                     primCount);
}


using SetVertexDeclaration_t = decltype(g_Direct3DDevice()->lpVtbl->SetVertexDeclaration);

SetVertexDeclaration_t origSetVertexDeclaration;

HRESULT STDMETHODCALLTYPE HookSetVertexDeclaration(IDirect3DDevice9 *This,
                                                   IDirect3DVertexDeclaration9 *pDecl) {
    //sp_log("HookSetVertexDeclaration: return to 0x%08X", getReturnAddress());

    return origSetVertexDeclaration(This, pDecl);
}

using SetMaterial_t = decltype(g_Direct3DDevice()->lpVtbl->SetMaterial);

SetMaterial_t origSetMaterial;

HRESULT STDMETHODCALLTYPE HookSetMaterial(IDirect3DDevice9 *This, const D3DMATERIAL9 *material)
{
    TRACE("HookSetMaterial");

    return origSetMaterial(This, material);
}

using CreateVertexShader_t = decltype(g_Direct3DDevice()->lpVtbl->CreateVertexShader);

CreateVertexShader_t origCreateVertexShader;

HRESULT STDMETHODCALLTYPE HookCreateVertexShader(IDirect3DDevice9 *This,
                                                 const DWORD *byte_code,
                                                 IDirect3DVertexShader9 **shader) {
    //sp_log("HookCreateVertexShader: return to 0x%08X", getReturnAddress());

    return origCreateVertexShader(This, byte_code, shader);
}

using CreateTexture_t = decltype(g_Direct3DDevice()->lpVtbl->CreateTexture);

CreateTexture_t origCreateTexture;

HRESULT STDMETHODCALLTYPE HookCreateTexture(IDirect3DDevice9 *This,
                                            UINT Width,
                                            UINT Height,
                                            UINT Levels,
                                            DWORD Usage,
                                            D3DFORMAT Format,
                                            D3DPOOL Pool,
                                            IDirect3DTexture9 **ppTexture,
                                            HANDLE *pSharedHandle) {
    //sp_log("HookCreateTexture: return to 0x%08X", getReturnAddress());

    return origCreateTexture(This,
                             Width,
                             Height,
                             Levels,
                             Usage,
                             Format,
                             Pool,
                             ppTexture,
                             pSharedHandle);
}

using CreateVertexDeclaration_t = decltype(g_Direct3DDevice()->lpVtbl->CreateVertexDeclaration);

CreateVertexDeclaration_t origCreateVertexDeclaration;

HRESULT STDMETHODCALLTYPE HookCreateVertexDeclaration(IDirect3DDevice9 *This,
                                                      const D3DVERTEXELEMENT9 *elements,
                                                      IDirect3DVertexDeclaration9 **declaration) {
    //sp_log("HookCreateVertexDeclaration: return to 0x%08X", getReturnAddress());

    return origCreateVertexDeclaration(This, elements, declaration);
}

static Var<int> g_MinVertexIndex{0x009729B0};

void hook_directx()
{
    auto vtbl = g_Direct3DDevice()->lpVtbl;

    auto old_perms = 0ul;
    VirtualProtect((void *) vtbl, 150u, PAGE_READWRITE, &old_perms);

    origSetViewport = vtbl->SetViewport;
    vtbl->SetViewport = &HookSetViewport;

#if 0
    origSetFVF = vtbl->SetFVF;
    vtbl->SetFVF = &HookSetFVF;

    origCreateVertexBuffer = vtbl->CreateVertexBuffer;
    vtbl->CreateVertexBuffer = &HookCreateVertexBuffer;

    origCreateIndexBuffer = vtbl->CreateIndexBuffer;
    vtbl->CreateIndexBuffer = &HookCreateIndexBuffer;

    origDrawPrimitive = vtbl->DrawPrimitive;
    vtbl->DrawPrimitive = &HookDrawPrimitive;

    origDrawPrimitiveUP = vtbl->DrawPrimitiveUP;
    vtbl->DrawPrimitiveUP = &HookDrawPrimitiveUP;

    origDrawPrimitive = vtbl->DrawPrimitive;
    vtbl->DrawPrimitive = &HookDrawPrimitive;

    origDrawIndexedPrimitiveUP = vtbl->DrawIndexedPrimitiveUP;
    vtbl->DrawIndexedPrimitiveUP = &HookDrawIndexedPrimitiveUP;

    origSetVertexDeclaration = vtbl->SetVertexDeclaration;
    vtbl->SetVertexDeclaration = &HookSetVertexDeclaration;

    origSetMaterial = vtbl->SetMaterial;
    vtbl->SetMaterial = &HookSetMaterial;

    origCreateVertexShader = vtbl->CreateVertexShader;
    vtbl->CreateVertexShader = &HookCreateVertexShader;

    origCreateTexture = vtbl->CreateTexture;
    vtbl->CreateTexture = &HookCreateTexture;

    origCreateVertexDeclaration = vtbl->CreateVertexDeclaration;
    vtbl->CreateVertexDeclaration = &HookCreateVertexDeclaration;
#endif

    VirtualProtect((void *) vtbl, 150u, old_perms, &old_perms);
}

void sub_76DF00()
{
    g_Direct3DDevice()->lpVtbl->Present(g_Direct3DDevice(), nullptr, nullptr, nullptr, nullptr);

    hook_directx();
}

void sub_772D50(const D3DVERTEXELEMENT9 *a1)
{
    CDECL_CALL(0x00772D50, a1);
}

void sub_772E30()
{
    CDECL_CALL(0x00772E30);
}

void sub_772E80()
{
    CDECL_CALL(0x00772E80);
}

void sub_772ED0()
{
    CDECL_CALL(0x00772ED0);
}

void sub_772F70()
{
    CDECL_CALL(0x00772F70);
}

void sub_772630()
{
    if constexpr (0)
    {
        static Var<D3DVERTEXELEMENT9> stru_93B0E0 {0x0093B0E0};
        static Var<D3DVERTEXELEMENT9> stru_93B0C8 {0x0093B0C8};
        static Var<D3DVERTEXELEMENT9> stru_93B098 {0x0093B098};

        static Var<DWORD [1]> dword_8BAF18 {0x008BAF18};
        static Var<DWORD [1]> dword_8BAFD0 {0x008BAFD0};
        static Var<DWORD [1]> dword_8BAF80 {0x008BAF80};
        static Var<DWORD [1]> dword_8BB030 {0x008BB030};

        nglCreateVertexDeclarationAndShader(&stru_975780(), &stru_93B0E0(), dword_8BAF18());
        nglCreateVertexDeclarationAndShader(&stru_9757A4(), &stru_93B0C8(), dword_8BAFD0());
        nglCreateVertexDeclarationAndShader(&stru_975788(), &stru_93B0C8(), dword_8BAF80());
        nglCreateVertexDeclarationAndShader(&stru_975798(), &stru_93B098(), dword_8BB030());

        static Var<D3DVERTEXELEMENT9> stru_93B080 {0x0093B080};
        sub_772D50(&stru_93B080());
        sub_772E30();
        sub_772E80();
        sub_772ED0();
        sub_772F70();

        static Var<const DWORD [1]> dword_8BB560 {0x008BB560};
        g_Direct3DDevice()->lpVtbl->CreatePixelShader(g_Direct3DDevice(), dword_8BB560(), &dword_975790());

        {
            auto *head = g_pixelShaderList().m_head;
            decltype(head) (__fastcall *sub_772C60)(void *, void *, decltype(head) a1, decltype(head) a2, IDirect3DPixelShader9 **a3) = CAST(sub_772C60, 0x00772C60);

            auto *v1 = sub_772C60(
                            &g_pixelShaderList(),
                            nullptr,
                            g_pixelShaderList().m_head,
                            g_pixelShaderList().m_head->_Prev,
                            &dword_975790());

            void (__fastcall *sub_772CE0)(void *, void *, uint32_t) = CAST(sub_772CE0, 0x00772CE0);
            sub_772CE0(&g_pixelShaderList(), nullptr, 1u);
            head->_Prev = v1;
            v1->_Prev->_Next = v1;
        }
    }
    else
    {
        CDECL_CALL(0x00772630);
    }
}

namespace nglHiresScreenShot {
static Var<int> ShotCount{0x00971F48};

static Var<int> NColumns{0x00971F3C};

static Var<int> NRows{0x00971F40};

static Var<int> CurTilesCount{0x00971F38};

static Var<int> TotalTilesCount{0x00971F34};

static Var<float *> xx1{0x00971EFC}, yy1{0x00971EF0}, xx2{0x00971EF8}, yy2{0x00971EF4};

static Var<bool> ScreenshotInProgress{0x00971F44};
} // namespace nglHiresScreenShot

int nglGetScreenWidth() {
    return 640;
}

int nglGetScreenHeight() {
    return 480;
}

void nglBeginHiresScreenShot(int width, int height) {
    nglHiresScreenShot::ScreenshotInProgress() = true;
    nglHiresScreenShot::CurTilesCount() = 0;
    auto screenWidth = nglGetScreenWidth();
    auto screenHeight = nglGetScreenHeight();
    nglHiresScreenShot::NColumns() = screenWidth * (width / screenWidth) / screenWidth;
    nglHiresScreenShot::NRows() = screenHeight * (height / screenHeight) / screenHeight;
    nglHiresScreenShot::TotalTilesCount() = nglHiresScreenShot::NColumns() *
        nglHiresScreenShot::NRows();
    nglHiresScreenShot::xx1() = static_cast<float *>(
        tlMemAlloc(4 * nglHiresScreenShot::NColumns() * nglHiresScreenShot::NRows(),
                   8u,
                   0x1000000u));
    nglHiresScreenShot::yy1() = static_cast<float *>(
        tlMemAlloc(4 * nglHiresScreenShot::TotalTilesCount(), 8u, 0x1000000u));
    nglHiresScreenShot::xx2() = static_cast<float *>(
        tlMemAlloc(4 * nglHiresScreenShot::TotalTilesCount(), 8u, 0x1000000u));
    nglHiresScreenShot::yy2() = static_cast<float *>(
        tlMemAlloc(4 * nglHiresScreenShot::TotalTilesCount(), 8u, 0x1000000u));

    int i = 0;
    if (nglHiresScreenShot::NRows()) {
        uint32_t k = 1;
        do {
            int v7 = 0;
            if (nglHiresScreenShot::NColumns()) {
                uint32_t v8 = 1;
                do {
                    nglHiresScreenShot::xx1()[v7 + i * nglHiresScreenShot::NColumns()] = -(
                        double) v8;
                    nglHiresScreenShot::yy1()[v7 + i * nglHiresScreenShot::NColumns()] = -(double) k;
                    nglHiresScreenShot::xx2()[v7 + i * nglHiresScreenShot::NColumns()] =
                        (double) (2 * (nglHiresScreenShot::NColumns() - v7) - 1);
                    ++v7;
                    v8 += 2;
                    nglHiresScreenShot::yy2()[v7 + i * nglHiresScreenShot::NColumns()] =
                        (double) (2 * (nglHiresScreenShot::NRows() - i) - 1);

                } while (v7 < nglHiresScreenShot::NColumns());
            }

            ++i;
            k += 2;
        } while (i < nglHiresScreenShot::NRows());
    }
}

void nglSetAspectRatio(Float a1) {
    nglCurScene()->AspectRatio = a1;
    nglCalculateMatrices(true);
}

bool nglSaveHiresScreenshot() {
    char Dest[64];

    sprintf(Dest,
            "BigScreenShot%4.4dw%2.2dh%2.2dr%2.2dc%2.2d",
            nglHiresScreenShot::ShotCount(),
            nglHiresScreenShot::NColumns(),
            nglHiresScreenShot::NRows(),
            nglHiresScreenShot::CurTilesCount() / (unsigned int) nglHiresScreenShot::NColumns(),
            nglHiresScreenShot::CurTilesCount() % (unsigned int) nglHiresScreenShot::NColumns());
    nglScreenShot(Dest);
    if (++nglHiresScreenShot::CurTilesCount() != nglHiresScreenShot::TotalTilesCount()) {
        return true;
    }

    ++nglHiresScreenShot::ShotCount();
    tlMemFree(nglHiresScreenShot::xx1());
    tlMemFree(nglHiresScreenShot::yy1());
    tlMemFree(nglHiresScreenShot::xx2());
    tlMemFree(nglHiresScreenShot::yy2());
    return false;
}

void nglScreenShot(const char *a1) {
    static int ScreenCount = 0;

    nglTexture *tex = nglGetFrontBufferTex();
    if (a1 != nullptr) {
        nglSaveTexture(tex, a1);
    } else {
        static char Buf[64];

        auto v2 = ScreenCount++;
        sprintf(Buf, "screenshot%4.4d", v2);
        nglSaveTexture(tex, Buf);
    }
}

void *ngl_memalloc_callback(unsigned int size, unsigned int align, unsigned int a3) {
    void *result;

    if (damage_morphs::intercepting_allocations()) {
        if (a3 & 0x400) {
            result = damage_morphs::memalloc(align, size, 1);
        } else {
            result = damage_morphs::memalloc(align, size, 0);
        }
    } else if ((~(align - 1) & (size + align - 1)) > 176) {
        result = arch_memalign(align, size);
    } else {
        result = slab_allocator::allocate(~(align - 1) & (size + align - 1), nullptr);
    }

    return result;
}

void ngl_memfree_callback(void *Memory) {
    if (Memory != nullptr) {
        if (damage_morphs::intercepting_allocations()) {
            damage_morphs::memfree(Memory);
        } else {
            auto *v1 = slab_allocator::find_slab_for_object(Memory);
            if (v1 != nullptr) {
                slab_allocator::deallocate(Memory, v1);
            } else {
                mem_freealign(Memory);
            }
        }
    }
}

int nglPalette::sub_782A70(int a2, int a3) {
    return THISCALL(0x00782A70, this, a2, a3);
}

void nglPalette::sub_782A40() {
    if (!g_valid_texture_format()) {
        g_Direct3DDevice()->lpVtbl->SetPaletteEntries(g_Direct3DDevice(),
                                                      this->m_palette_idx,
                                                      this->m_palette_entries);
    }
}

void nglTexture::CreateTextureOrSurface()
{
    if constexpr (1)
    {
        auto v2 = this->m_format;
        if ((v2 & 0x2000) != 0)
        {   
            g_Direct3DDevice()
                ->lpVtbl->CreateDepthStencilSurface(g_Direct3DDevice(),
                                                    this->m_width,
                                                    this->m_height,
                                                    this->m_d3d_format,
                                                    D3DMULTISAMPLE_NONE,
                                                    0,
                                                    TRUE,
                                                    (IDirect3DSurface9 **) &this->DXSurfaces,
                                                    nullptr);
            ++nglDebug().field_10;
        } else {
            int usage = 0;
            D3DPOOL pool = D3DPOOL_MANAGED;
            if ((v2 & 0x1000) != 0) {
                usage = 1;
                pool = D3DPOOL_DEFAULT;
            }

            if (NGLTEX_GET_FORMAT(v2) == 7 && g_valid_texture_format())
            {
                auto v9 = this->m_height * this->m_width;
                this->m_d3d_format = D3DFMT_A8R8G8B8;
                this->m_numLevel = 1;
                this->field_30 = static_cast<uint8_t *>(tlMemAlloc(v9, 8, 0x1000000u));

                std::memset(this->field_30, 0, v9);
            } else {
                this->field_30 = nullptr;

            }

            auto format = this->m_d3d_format;

            auto levels = this->m_numLevel;

            if ((this->m_format & 0x10000000) != 0) // Cubemap
            {
                g_Direct3DDevice()
                    ->lpVtbl->CreateCubeTexture(g_Direct3DDevice(),
                                                this->m_width,
                                                levels,
                                                usage,
                                                format,
                                                pool,
                                                (IDirect3DCubeTexture9 **) &this->DXTexture,
                                                nullptr);
            }
            else
            {
                if constexpr (FORCE_MIPS) {
                    if (pool == D3DPOOL_DEFAULT) {
                        this->m_numLevel = 0;
                        levels = 0;
                        usage |= D3DUSAGE_AUTOGENMIPMAP;
                    }
                }

                g_Direct3DDevice()->lpVtbl->CreateTexture(g_Direct3DDevice(),
                                                          this->m_width,
                                                          this->m_height,
                                                          levels,
                                                          usage,
                                                          format,
                                                          pool,
                                                          &this->DXTexture,
                                                          nullptr);

                if constexpr (FORCE_MIPS) {
                    if (pool == D3DPOOL_DEFAULT && this->DXTexture) {
                        IDirect3DBaseTexture9* baseTex = reinterpret_cast<IDirect3DBaseTexture9*>(this->DXTexture);
                        baseTex->lpVtbl->GenerateMipSubLevels(baseTex);
                    }
                }
            }

            ++nglDebug().field_C;
        }

    }
    else
    {
        THISCALL(0x00775000, this);
    }
}

void nglTexture::sub_774F20()
{
    if constexpr (1)
    {
        if ((this->m_format & 0x2000) == 0)
        {
            this->m_numLevel = this->DXTexture->lpVtbl->GetLevelCount(this->DXTexture);
            if ((this->m_format & 0x10000000) == 0)
            {
                this->DXSurfaces = static_cast<decltype(this->DXSurfaces)>(
                    tlMemAlloc(4 * this->m_numLevel, 8, 0x1000000u));
                for (auto i = 0u; i < this->m_numLevel; ++i) {
                    this->DXTexture->lpVtbl
                        ->GetSurfaceLevel(this->DXTexture,
                                          i,
                                          (IDirect3DSurface9 **) &this->DXSurfaces[i]);
                    ++nglDebug().field_8;
                }

            }
            else
            {
                this->DXSurfaces = static_cast<decltype(this->DXSurfaces)>(
                    tlMemAlloc(0x18, 8, 0x1000000u));
                for (auto j = 0; j < 6; ++j)
                {
                    this->DXSurfaces[j] = static_cast<IDirect3DSurface9 *>(
                        tlMemAlloc(4 * this->m_numLevel, 8, 0x1000000u));
                    for (auto k = 0u; k < this->m_numLevel; ++k) {
                        this->DXTexture->lpVtbl->GetSurfaceLevel(this->DXTexture,
                                                                 j,
                                                                 (IDirect3DSurface9 **) k);
                        ++nglDebug().field_8;
                    }
                }
            }
        }

    } else {
        THISCALL(0x00774F20, this);
    }
}


void sub_77B740() {
    CDECL_CALL(0x0077B740);
}

void sub_7740F0() {
#if 0
    D3DVERTEXELEMENT9 v1[2];
    v1[0].Stream = 0;
    v1[0].Offset = 0;
    v1[0].Type = D3DDECLTYPE_FLOAT3;
    v1[0].Method = D3DDECLMETHOD_DEFAULT;
    v1[0].Usage = 0;
    v1[0].UsageIndex = 0;
    v1[1].Stream = 255;
    v1[1].Offset = 0;
    v1[1].Type = D3DDECLTYPE_UNUSED;
    v1[1].Method = D3DDECLMETHOD_DEFAULT;
    v1[1].Usage = 0;
    v1[1].UsageIndex = 0;
    g_Direct3DDevice()->lpVtbl->CreateVertexDeclaration(g_Direct3DDevice(), v1, &dword_97393C);

    D3DVERTEXELEMENT9 v24[3];
    v24[0].Stream = 0;
    v24[0].Offset = 0;
    v24[0].Type = D3DDECLTYPE_FLOAT3;
    v24[0].Method = D3DDECLMETHOD_DEFAULT;
    v24[0].Usage = 0;
    v24[0].UsageIndex = 0;
    v24[1].Stream = 0;
    v24[1].Offset = 12;
    v24[1].Type = D3DDECLTYPE_D3DCOLOR;
    v24[1].Method = D3DDECLMETHOD_DEFAULT;
    v24[1].Usage = 10;
    v24[1].UsageIndex = 0;
    v24[2].Stream = 255;
    v24[2].Offset = 0;
    v24[2].Type = D3DDECLTYPE_UNUSED;
    v24[2].Method = D3DDECLMETHOD_DEFAULT;
    v24[2].Usage = 0;
    v24[2].UsageIndex = 0;
    g_Direct3DDevice()->lpVtbl->CreateVertexDeclaration(g_Direct3DDevice(), v24, &dword_973940);
    v2.Stream = 0;
    v2.Offset = 0;
    v2.Type = 2;
    v2.Method = D3DDECLMETHOD_DEFAULT;
    v2.Usage = 0;
    v2.UsageIndex = 0;
    v3 = 0;
    v4 = 12;
    v5 = 1;
    v6 = 0;
    v7 = 5;
    v8 = 0;
    v9 = 255;
    v10 = 0;
    v11 = 17;
    v12 = 0;
    v13 = 0;
    v14 = 0;
    g_Direct3DDevice()->lpVtbl->CreateVertexDeclaration(g_Direct3DDevice(), &v2, &dword_973944);
    v15.Stream = 0;
    v15.Offset = 0;
    v15.Type = 3;
    v15.Method = 0;
    v15.Usage = 9;
    v15.UsageIndex = 0;
    v16 = 0;
    v17 = 16;
    v18 = 1;
    v19 = 5;
    v20 = 255;
    v21 = 0;
    v22 = 17;
    v23 = 0;
    g_Direct3DDevice()->lpVtbl->CreateVertexDeclaration(g_Direct3DDevice(), &v15, &dword_973948);
    v25.Stream = 0;
    v25.Offset = 0;
    v25.Type = 2;
    v25.Method = 0;
    v25.Usage = 0;
    v25.UsageIndex = 0;
    v26 = 0;
    v27 = 12;
    v28 = 4;
    v29 = 0;
    v30 = 10;
    v31 = 0;
    v32 = 0;
    v33 = 16;
    v34 = 1;
    v35 = 0;
    v36 = 5;
    v37 = 0;
    v38 = 255;
    v39 = 0;
    v40 = 17;
    v41 = 0;
    v42 = 0;
    v43 = 0;
    g_Direct3DDevice()->lpVtbl->CreateVertexDeclaration(g_Direct3DDevice(), &v25, &dword_973950);
    v44.Stream = 0;
    v44.Offset = 0;
    v44.Type = 2;
    v44.Method = 0;
    v44.Usage = 0;
    v44.UsageIndex = 0;
    v45 = 0;
    v46 = 12;
    v47 = 1;
    v48 = 0;
    v49 = 5;
    v50 = 0;
    v51 = 0;
    v52 = 20;
    v53 = 1;
    v54 = 0;
    v55 = 5;
    v56 = 1;
    v57 = 255;
    v58 = 0;
    v59 = 17;
    v60 = 0;
    v61 = 0;
    v62 = 0;
    g_Direct3DDevice()->lpVtbl->CreateVertexDeclaration(g_Direct3DDevice(), &v44, &dword_97394C);
#else

#endif
}

void nglInitWhiteTexture()
{
    TRACE("nglInitWhiteTexture");

    if constexpr (0) {
        nglWhiteTex() = nglCreateTexture(513u, 1, 1, 0, 1);
        nglDxLockTexture(nglWhiteTex(), 0);
        nglDxSetTexel8(nglWhiteTex(), 0, 0, -1);
        nglDxUnlockTexture(nglWhiteTex());
        nglWhiteTex()->field_60 = tlFixedString{"nglwhite"};
        nglWhiteTex()->field_34 |= 2u;

        void (__fastcall *Add)(void *) = CAST(Add, get_vfunc(nglTextureDirectory()->m_vtbl, 0x10));

        Add(nglWhiteTex());
    } else {
        CDECL_CALL(0x007730E0);
    }
}

void nglReleaseSection(nglMeshSection *a1) {
    CDECL_CALL(0x0077C490, a1);
}

Var<int[1024]> dword_975BE8{0x00975BE8};
Var<int> dword_975BE0{0x00975BE0};

uint8_t NGLTEX_GET_FORMAT(uint32_t format) {
    return (format & 0x000000FF);
}

int sub_782FE0(const D3DSURFACE_DESC &desc, const D3DLOCKED_RECT &rect) {
    auto format = desc.Format;

    int result = 0;
    if (format > D3DFMT_DXT1) {
        if (format > D3DFMT_DXT4) {
            if (format != D3DFMT_DXT5) {
                return result;
            }

        } else if (format != D3DFMT_DXT4 && format != D3DFMT_DXT2 && format != D3DFMT_DXT3) {
            return result;
        }

    } else if (format != D3DFMT_DXT1) {
        switch (format) {
        case D3DFMT_R8G8B8:
        case D3DFMT_A8R8G8B8:
        case D3DFMT_R5G6B5:
        case D3DFMT_A1R5G5B5:
        case D3DFMT_A4R4G4B4:
        case D3DFMT_A8:
        case D3DFMT_L8: {
            result = rect.Pitch * desc.Height;
            break;
        }
        default:
            return result;
        }

        return result;
    }

    auto height = desc.Height;
    if (height < 4) {
        height = 4;
    }

    return height * rect.Pitch / 4;
}

void nglTextureInit()
{
    TRACE("nglTextureInit");

    if constexpr (0)
    {

    }
    else
    {
        CDECL_CALL(0x00773830);
    }
}

static Var<D3DCAPS9> g_deviceCaps {0x00972108};


void sub_7726B0(bool a1)
{
    Var<bool> byte_971F90{0x00971F90};

    static_assert(offsetof(D3DCAPS9, VertexShaderVersion) == 0xC4, "");
    static_assert(offsetof(D3DCAPS9, PixelShaderVersion) == 0xCC, "");

    if ((0x100 < (g_deviceCaps().VertexShaderVersion & 0xFFFF)) &&
        (0x100 < (g_deviceCaps().PixelShaderVersion & 0xFFFF)) && !byte_971F90())
    {
        HANDLE v2 = CreateFileA("data\\ForceNoShader", GENERIC_READ, 0, nullptr, 3u, 0, nullptr);

        if (v2 == INVALID_HANDLE_VALUE)
        {
            EnableShader() = true;

            float v3[4] {0.0, 0.5, 1.0, 2.0};
            g_Direct3DDevice()->lpVtbl->SetVertexShaderConstantF(g_Direct3DDevice(), 91u, v3, 1u);


            v3[0] = 3.1415927;
            v3[1] = 0.5;
            v3[2] = 6.2831855;
            v3[3] = 0.15915494;
            g_Direct3DDevice()->lpVtbl->SetVertexShaderConstantF(g_Direct3DDevice(), 92u, v3, 1u);

            v3[0] = 1.0;
            v3[1] = -0.5;
            v3[2] = 0.041666668;
            v3[3] = -0.0013888889;
            g_Direct3DDevice()->lpVtbl->SetVertexShaderConstantF(g_Direct3DDevice(), 93u, v3, 1u);

            v3[0] = 1.0;
            v3[1] = -0.16666667;
            v3[2] = 0.0083333338;
            v3[3] = -0.0001984127;
            g_Direct3DDevice()->lpVtbl->SetVertexShaderConstantF(g_Direct3DDevice(), 94u, v3, 1u);

            if (a1) {
                sub_772630();
            }

            return;
        }

        CloseHandle(v2);
    }

    EnableShader() = false;
    return;
}

void nglGetProjectionParams(float *a1, float *nearz, float *farz)
{
    if (a1 != nullptr) {
        *a1 = nglCurScene()->HFov;
    }

    if (nearz != nullptr) {
        *nearz = nglCurScene()->m_nearz;
    }

    if (farz != nullptr) {
        *farz = nglCurScene()->m_farz;
    }
}

void nglSetTexturePath(const char *a1) {
    std::strncpy(nglTexturePath(), a1, 256u);
    nglTexturePath()[255] = '\0';
}

nglFont *nglLoadFont(const tlFixedString &a1) {
    if constexpr (1) {
        //sp_log("find = 0x%08X, sub_779FC0 = 0x%08X", find, load);

        nglFont *font = nglFontDirectory()->Find(a1);
        if (font == nullptr) {
            return nglFontDirectory()->Load(a1);
        }

        ++font->field_20;
        return font;
    } else {
        return (nglFont *) CDECL_CALL(0x007792B0, &a1);
    }
}

nglMeshFile *nglLoadMeshFile(const tlFixedString &a1)
{
    TRACE("nglLoadMeshFile", a1.to_string());

    if constexpr (1)
    {
        nglMeshFile * (__fastcall *Find)(void *, void *, const tlFixedString *) =
            CAST(Find, get_vfunc(nglMeshFileDirectory()->m_vtbl, 0xC));

        nglMeshFile *MeshFile = Find(nglMeshFileDirectory(), nullptr, &a1);

        sp_log("%s", MeshFile != nullptr ? "mesh file is found" : "mesh file is not found");

        if (MeshFile == nullptr) {
            nglMeshFile *(__fastcall *Load)(void *, void *, const tlFixedString *) =
                CAST(Load, get_vfunc(nglMeshFileDirectory()->m_vtbl, 0x24));

            sp_log("0x%08X", Load);

            return Load(nglMeshFileDirectory(), nullptr, &a1);
        }

        ++MeshFile->field_120;
        return MeshFile;

    } else {
        return (nglMeshFile *) CDECL_CALL(0x0076F140, &a1);
    }
}

void nglMeshFile::un_mash_start(generic_mash_header *header,
                                void *,
                                generic_mash_data_ptrs *a3,
                                void *) {
    if (uint32_t v5 = 8 - ((uint32_t) a3->field_0 % 8); v5 < 8) {
        a3->field_0 += v5;
    }

    assert(((int) header) % 4 == 0);
}

tlFixedString *nglMeshFile::get_string(nglMeshFile *a1) {
    return &a1->FileName;
}

#include "resource_directory.h"

void nglSetTextureDirectory(tlResourceDirectory<nglTexture, tlFixedString> *a1)
{
    TRACE("nglSetTextureDirectory");

    sp_log("0x%08X", a1->m_vtbl);
    sp_log("0x%08x", tlresource_directory<nglTexture,tlFixedString>::system_dir->m_vtbl);

    if constexpr (1) {
        nglTextureDirectory() = CAST(nglTextureDirectory(), a1);
    } else {
        CDECL_CALL(0x007730B0, a1);
    }
}

void nglSetMeshFileDirectory(tlResourceDirectory<nglMeshFile, tlFixedString> *a1) {
    nglMeshFileDirectory() = CAST(nglMeshFileDirectory(), a1);
}

void nglSetMeshDirectory(tlResourceDirectory<nglMesh, tlHashString> *a1) {
    nglMeshDirectory() = CAST(nglMeshDirectory(), a1);
}

void nglSetMorphDirectory(tlResourceDirectory<nglMorphSet, tlHashString> *a1) {
    nglMorphDirectory() = CAST(nglMorphDirectory(), a1);
}

void nglSetMaterialFileDirectory(tlResourceDirectory<nglMaterialFile, tlFixedString> *a1) {
    nglMaterialFileDirectory() = CAST(nglMaterialFileDirectory(), a1);
}

void nglSetMaterialDirectory(tlResourceDirectory<nglMaterialBase, tlHashString> *a1) {
    nglMaterialDirectory() = CAST(nglMaterialDirectory(), a1);
}

bool nglMaterialBase::IsSwitchable() {
    return this->m_shader->IsSwitchable();
}

#ifndef TARGET_XBOX
nglMaterialBase *nglGetMaterialInFile(const tlFixedString &a1, nglMeshFile *MeshFile)
{
    TRACE("nglGetMaterialInFile", tlHashString {a1.GetHash()}.c_str(), a1.to_string());

    nglMaterialBase *result = nullptr;
    if constexpr (1)
    {
        for (result = MeshFile->FirstMaterial; result != nullptr; result = result->NextMaterial)
        {
            if (*result->Name == a1) {
                return result;
            }
        }

        // material not found in file's linked list — fall back to the original game function
        sp_log("nglGetMaterialInFile: material '%s' (0x%08X) not found in file, falling back to original",
               a1.to_string(), a1.GetHash());

        nglMaterialBase * (*func)(const tlFixedString *, nglMeshFile *) = CAST(func, 0x0076F0F0);
        result = func(&a1, MeshFile);
    }
    else
    {
        nglMaterialBase * (*func)(const tlFixedString *, nglMeshFile *) = CAST(func, 0x0076F0F0);
        result = func(&a1, MeshFile);
    }

    return result;
}

#endif

namespace xbox
{
    struct nglMeshSection {
        int field_0;
        nglMaterialBase *Material;
        int field_8;
        uint16_t *BonesIdx;
        float SphereCenter[4];
        float SphereRadius;
        uint32_t Flags;
        int m_primitiveType;
        uint32_t NIndices;
        struct {
            void *field_0;
        } field_30;
        int field_34;
        int field_38;
        int field_3C;
        int NVertices;
        struct {
            void *field_0;
            int Size;
            int field_8;
        } VertexBuffer;
        int m_stride;
        int field_54;
        int field_58;
        nglVertexDef *VertexDef;
    };

    VALIDATE_OFFSET(nglMeshSection, field_30, 0x30);
    VALIDATE_SIZE(nglMeshSection, 0x60);
}

void nglRebaseSection(uint32_t NewBase, uint32_t OldBase, nglMeshSection *a3)
{
    auto idx = NewBase - OldBase;

#ifdef TARGET_XBOX
    auto *Section = bit_cast<xbox::nglMeshSection *>(a3);

    PTR_OFFSET(idx, Section->BonesIdx);

    PTR_OFFSET(idx, Section->VertexBuffer.field_0);
  
    PTR_OFFSET(idx, Section->field_30.field_0 );

    PTR_OFFSET(idx, Section->field_38);

    PTR_OFFSET(idx, Section->Material);
    
    PTR_OFFSET(idx, Section->VertexDef);

    if constexpr (0)
    {
        int (*arr)[sizeof(nglMeshSection) / 4] = CAST(arr, Section);
        int i = 0;
        for (auto &v : *arr)
        {
            sp_log("0x%08X %d", (i++) * 4, v);
        }

        assert(0);
    }

    a3->m_indices = static_cast<uint16_t *>(Section->field_30.field_0);
    a3->m_vertices = Section->VertexBuffer.field_0;
    a3->NVertices = Section->NVertices;
    a3->field_40 = Section->VertexBuffer.Size;
    a3->VertexDef = Section->VertexDef;
    a3->m_stride = Section->m_stride;

#else

    PTR_OFFSET(idx, a3->BonesIdx);

    PTR_OFFSET(idx, a3->field_3C.m_vertexData);

    PTR_OFFSET(idx, a3->m_indices);

    PTR_OFFSET(idx, a3->Material);

    PTR_OFFSET(idx, a3->VertexDef);

    auto *v9 = a3->VertexDef;
    if (v9 != nullptr)
    {
        PTR_OFFSET(idx, v9->m_vtbl);
    }
#endif
}

void nglRebaseMesh(uint32_t NewBase, uint32_t OldBase, nglMesh *pMesh)
{
    TRACE("nglRebaseMesh");

    if constexpr (1)
    {
        int idx = NewBase - OldBase;

        PTR_OFFSET(idx, pMesh->Bones);

        PTR_OFFSET(idx, pMesh->LODs);

        PTR_OFFSET(idx, pMesh->Sections);

#ifndef TARGET_XBOX
        for (int i = 0; i < pMesh->NLODs; ++i) {
            auto &pLOD = pMesh->LODs[i];

            PTR_OFFSET(idx, pLOD.field_0);
        }
#endif

        for (auto j = 0u; j < pMesh->NSections; ++j)
        {
            if (pMesh->Sections[j].Section != nullptr)
            {
                PTR_OFFSET(idx, pMesh->Sections[j].Section);
            }

            nglRebaseSection(NewBase, OldBase, pMesh->Sections[j].Section);
        }
    }
    else
    {
        CDECL_CALL(0x0076F340, NewBase, OldBase, pMesh);
    }
}

void nglProcessMorph(nglMeshFile *MeshFile, nglDirectoryEntry *a2, int base) {
    if constexpr (0) {
        struct {
            int m_extension;
            int field_4;
            int field_8;
            int field_C;
            int field_10;
        } *tmp = CAST(tmp, a2->field_4);

        if (tmp->m_extension) {
            tmp->m_extension += base;
        }

        if (tmp->field_8) {
            tmp->field_8 += base;
        }

        if (tmp->field_10) {
            tmp->field_10 = base;
        }

        nglMorphSet * (__fastcall *Add)(void *, void *, nglMorphSet *) = CAST(Add, get_vfunc(nglMorphDirectory()->m_vtbl, 0x10));

        nglMorphSet *Morph = CAST(Morph, tmp);

        auto duplicate_morph = Add(nglMorphDirectory(), nullptr, Morph);
        if (duplicate_morph != nullptr) {
            auto *v7 = duplicate_morph->field_C->FileName.to_string();
            auto *v6 = duplicate_morph->field_C->FilePath;
            auto *v5 = MeshFile->FileName.to_string();
            auto *v3 = Morph->field_0.c_str();
            sp_log(
                "Duplicate morph %s found in %s%s.pcmorph.  Originally contained in "
                "%s%s.pcmorph.\n",
                v3,
                nglMeshPath(),
                v5,
                v6,
                v7);
        }

        Morph->field_C = MeshFile;
        if (MeshFile->FirstMorph == nullptr) {
            MeshFile->FirstMorph = Morph;
        }

        if (Morph->field_4 != 0) {
            auto *v6 = Morph->field_8 + 2;
            for (auto idx = 0u; idx < Morph->field_4; ++idx) {
                if (*v6) {
                    *v6 += base;
                }

                auto v7 = 0u;
                if (*(v6 - 1)) {
                    auto *v8 = (int *) (*v6 + 12);
                    do {
                        auto *v9 = v8;
                        auto v10 = 8;
                        do {
                            auto v11 = *(v9 - 1);
                            if (v11) {
                                *(v9 - 1) = base + v11;
                            }

                            if (*v9) {
                                *v9 += base;
                            }

                            int v12 = v9[1];
                            if (v12) {
                                v9[1] = base + v12;
                            }

                            int v13 = v9[2];
                            if (v13) {
                                v9[2] = base + v13;
                            }

                            v9 += 4;
                            --v10;
                        } while (v10);
                        v8 += 34;
                        ++v7;
                    } while (v7 < *(v6 - 1));
                }

                v6 += 3;
            }

            *Morph->field_8 = 2;
        } else {
            *Morph->field_8 = 2;
        }
    } else {
        CDECL_CALL(0x00778840, MeshFile, a2, base);
    }
}

matrix4x3 transposed(const matrix4x3 &a2)
{
    TRACE("matrix4x3::transpose");
    matrix4x3 result{};

    if constexpr (0)
    {
        result = a2.transposed();
    }
    else
    {
        CDECL_CALL(0x004135B0, &result, &a2);
    }

    //sp_log("%s", a2.to_string());
    //sp_log("%s", result.to_string());
    //sp_log("%s", a2.transposed().to_string());

    //assert(approx_equals(result[0][3], 0.0, LARGE_EPSILON));
    //assert(result == a2.transposed());

    return result;
}

vector4d xform_inv(const vector4d &a2, const matrix4x3 &a3)
{
    vector4d result;

    if constexpr (0)
    {
        vector4d x = a3[0];
        vector4d y = a3[1];
        vector4d z = a3[2];

        result = sub_4126E0(x, a2, y, a2, z, a2);
        return result;
    }
    else
    {
        CDECL_CALL(0x004139A0, &result, &a2, &a3);
    }

    assert(result == a2 * a3);

    return result;
}

matrix4x4 sub_4150E0(const matrix4x4 &a2)
{
    TRACE("sub_4150E0");

    // sp_log("%s", a2.to_string());

    if constexpr (0)
    {
        struct transform3d {
            matrix4x3 basis;
            vector4d origin;
        } v1 = *bit_cast<transform3d *>(&a2);

        matrix4x3 a3 = v1.basis;

        v1.basis = transposed(a3);
        v1.origin = xform_inv(-v1.origin, v1.basis);

        matrix4x4 result = *bit_cast<matrix4x4 *>(&v1);
        return result;
    }
    else
    {
        matrix4x4 result;

        CDECL_CALL(0x004150E0, &result, &a2);

        // sp_log("%s", result.to_string());

        return result;
    }
}

vector4d sub_401270(const vector4d &a2, const vector4d &a3)
{
    if constexpr (1) {
        float v3;
        if (a2[3] >= a3[3]) {
            v3 = a3[3];
        } else {
            v3 = a2[3];
        }

        float v4;
        if (a2[2] >= a3[2]) {
            v4 = a3[2];
        } else {
            v4 = a2[2];
        }

        float v5;
        if (a2[1] >= a3[1]) {
            v5 = a3[1];
        } else {
            v5 = a2[1];
        }

        float x;
        if (a2[0] >= a3[0]) {
            x = a3[0];
        } else {
            x = a2[0];
        }

        vector4d result;
        result[0] = x;
        result[1] = v5;
        result[2] = v4;
        result[3] = v3;
        return result;
    } else {
        vector4d result;
        CDECL_CALL(0x00401270, &result, &a2, &a3);

        return result;
    }
}

void sub_4013C0(
        vector4d &a1,
        vector4d &a2,
        vector4d &a3,
        vector4d &a4,
        const vector4d &x,
        const vector4d &y,
        const vector4d &z,
        const vector4d &w)
{
    a1 = x;

    a2[0] = x[1];
    a2[1] = y[1];
    a2[2] = z[1];
    a2[3] = w[1];

    a3[0] = x[2];
    a3[1] = y[2];
    a3[2] = z[2];
    a3[3] = w[2];

    a4[0] = x[3];
    a4[1] = y[3];
    a4[2] = z[3];
    a4[3] = w[3];
}

vector4d sub_4012F0(const vector4d &a2, const vector4d &a3) {
    if constexpr (1) {
        float w;
        if (a2[3] <= a3[3]) {
            w = a3[3];
        } else {
            w = a2[3];
        }

        float z;
        if (a2[2] <= a3[2]) {
            z = a3[2];
        } else {
            z = a2[2];
        }

        float y;
        if (a2[1] <= a3[1]) {
            y = a3[1];
        } else {
            y = a2[1];
        }

        float x;
        if (a2[0] <= a3[0]) {
            x = a3[0];
        } else {
            x = a2[0];
        }

        vector4d result;
        result[0] = x;
        result[1] = y;
        result[2] = z;
        result[3] = w;
        return result;

    } else {
        vector4d result;
        CDECL_CALL(0x004012F0, &result, &a2, &a3);

        return result;
    }
}

vector4d sub_411750(const vector4d &a2, const vector4d &a3)
{
    if constexpr (1) {
        vector4d v4 = a3 + a2;
        return v4;

    } else {
        vector4d result;
        CDECL_CALL(0x00411750, &result, &a2, &a3);

        return result;
    }
}

struct nglMeshFileHeader
{
	char Tag[4];                 // 'PCM '
	uint32_t Version;
	uint32_t NDirectoryEntries;
	nglDirectoryEntry *DirectoryEntries;  // Shared vertex buffer for skinned meshes.
    int field_10;
};


void nglRebaseHeader(uint32_t Base, nglMeshFileHeader *&pHeader)
{
	PTR_OFFSET(Base, pHeader->DirectoryEntries);
}

const char *to_string(TypeDirectoryEntry type)
{
    static std::string g_str {};
    switch(type)
    {
        case TypeDirectoryEntry::MATERIAL:
            g_str = std::string {"TypeDirectoryEntry::MATERIAL"};
            break;
        case TypeDirectoryEntry::MESH:
            g_str = std::string {"TypeDirectoryEntry::MESH"};
            break;
        case TypeDirectoryEntry::MORPH:
            g_str = std::string {"TypeDirectoryEntry::MORPH"};
            break;
        default:
            g_str = "";
            break;
    }

    return g_str.c_str();
}

constexpr bool nglLoadMeshFileInternal_hook = 1;

#ifndef TARGET_XBOX
#if MOD_MESH_SUPPORT
// ---------------------------------------------------------------------------
// FBX/OBJ mesh replacement, native importer (mod_mesh_import.h).
//
// modBuildSectionsForMesh runs once per nglMesh right after nglRebaseMesh:
// it snapshots the original (still-float) section vertex streams as donor
// views, parses the mod file (cached across meshes and reloads) and lets the
// three-tier mapper produce per-section replacements.
//
// modApplyBuiltSection swaps one section onto its replacement. Buffers are
// created through the engine's own pool helpers (createVertexBufferAndWrite-
// Data / createIndexBufferAndWriteData), so the retail unload path
// (nglReleaseSection -> 0x77B5D0) recycles them exactly like vanilla section
// buffers. Only >65535-vertex sections fall back to a raw D3DFMT_INDEX32
// buffer (no engine path exists); the case is logged. CPU-side fields the
// engine still reads (m_vertexData, m_indices, BonesIdx) point into the
// per-section registry (modSectionRegistry), which outlives the mesh file
// and also feeds the work-mesh mirroring in nglCopySection. Replaced
// sections keep the full vanilla load tail (VertexDef bank init +
// BindSection): the actor pipeline clones character meshes from the section
// VertexDef, so it must be a real initialized def, not the raw in-file blob
// and not null. On any failure the section is left untouched and the vanilla
// path continues.
// ---------------------------------------------------------------------------

namespace {

// Everything a replaced section needs to be (re)constructed, kept alive for
// the whole session. The same snapshot is mirrored onto the actor's buffered
// work-mesh clones (see nglCopySection): character meshes are copied into
// those clones every frame by actor::swap_all_mesh_buffers, and the vanilla
// fixed-size memcpy shreds any section whose counts differ from what the
// clone was created with.
struct ModSectionStorage {
    // The payload as it goes to the GPU. For a SKINNED section this is the
    // 64-byte float row, so `vertices` reads naturally as floats. For a STATIC
    // section it is the section's OWN vertex format, packed - a byte blob that
    // merely lives in a float vector (every D3D vertex stride is a multiple of
    // 4, so the alignment holds). Everything below therefore measures in
    // strideBytes, never in a hard-coded 64.
    std::vector<float>    vertices;    // padded to >= the vanilla vert count
    std::vector<uint16_t> idx16;
    std::vector<uint32_t> idx32;
    uint32_t strideBytes = 64;         // bytes per vertex, from the section
    bool     rigid = false;            // static section: no palette, no morphs
    // resolved layout of a static section, so a second parse of the same mesh
    // file does not sniff OUR OWN packed output (it would still read correctly,
    // but the original offsets are ground truth and free to keep)
    int      layPos = 0, layNrm = -1, layUv = -1, layCol = -1;
    // The palette is tlMemAlloc-owned, NOT vector storage: the engine's
    // dynamic-section teardown (nglDestroySection / retail 0x775700)
    // tlMemFree's BonesIdx on the actor work-mesh clones we mirror onto, so
    // the pointer must be legal to hand to tlMemFree. File sections are
    // never freed per-section, so on those it just leaks <=128 bytes per
    // reload. Never freed from our side once installed in a section.
    uint16_t *palette = nullptr;
    uint16_t  nbones  = 0;
    uint32_t nverts = 0;               // real (drawn) counts
    uint32_t nidx = 0;
    uint32_t paddedVerts = 0;
    uint32_t baseVertex = 0;           // first REAL vertex row (see morph note)
    uint32_t origVerts = 0;            // VANILLA vertex count of this section.
                                       // Sticky: S->NVertices is ours after the
                                       // first apply, so re-deriving the dead
                                       // zone from it would grow the buffer on
                                       // every re-apply and move the morph
                                       // window off the vanilla index range.
    uint32_t weightClass = 2;
    bool     wide = false;
    uint32_t revision = 0;             // bumped per (re)build of the source
    uint32_t mirroredRevision = 0;     // on clones: which revision they hold

    void setPalette(const uint16_t *src, uint32_t n)
    {
        if (n == 0) { static const uint16_t zero = 0; src = &zero; n = 1; }
        if (n > 64) n = 64;
        palette = static_cast<uint16_t *>(tlMemAlloc(int(n * sizeof(uint16_t)), 8, 0x1000000u));
        std::memcpy(palette, src, n * sizeof(uint16_t));
        nbones = uint16_t(n);
    }
};

std::unordered_map<nglMeshSection *, ModSectionStorage> modSectionRegistry;
uint32_t modSectionRevision = 0;

// A registry entry is only meaningful while the section STILL POINTS AT the
// storage we installed. Two things break that:
//   * nglDestroySection tlMemFree's the section (and the palette we handed it),
//   * a mesh-file unload frees the whole buffer the file's sections live in,
//     without ever calling nglDestroySection.
// Both addresses come straight back from the pool on the next load, so a
// brand-new, unrelated section could be found in the registry and get another
// character's geometry mirrored onto it (nglCopySection) with another
// character's bone palette - sections apparently "reordered", bones scrambled,
// on exactly the second character loaded. Identity is re-established by
// comparing the section's CPU vertex pointer against our storage: only a
// section we really installed can point into that vector.
ModSectionStorage *modLiveStorage(nglMeshSection *S)
{
    if (S == nullptr) return nullptr;
    auto it = modSectionRegistry.find(S);
    if (it == modSectionRegistry.end())
        return nullptr;
    if (it->second.vertices.empty()
        || S->field_3C.m_vertexData != (char *) it->second.vertices.data())
    {
        modSectionRegistry.erase(it);          // recycled address, not ours
        return nullptr;
    }
    return &it->second;
}

// (re)creates the D3D buffers and points the section at `st`. Buffers go
// through the engine pool helpers so the retail teardown recycles them like
// vanilla ones. Returns false without touching the section on failure.
bool modApplyStorageToSection(nglMeshSection *S, ModSectionStorage &st)
{
    const uint32_t stride = st.strideBytes ? st.strideBytes : 64u;
    const uint32_t vbytes = st.paddedVerts * stride;
    const uint32_t ibytes = st.nidx * (st.wide ? 4u : 2u);

    nglVertexBuffer newVB {};
    if (!newVB.createVertexBufferAndWriteData(st.vertices.data(), vbytes, 1028))
        return false;

    IDirect3DIndexBuffer9 *ib = nullptr;
    if (!st.wide) {
        nglVertexBuffer newIB {};
        if (!newIB.createIndexBufferAndWriteData(st.idx16.data(), int(ibytes))) {
            if (newVB.m_vertexBuffer)
                newVB.m_vertexBuffer->lpVtbl->Release(newVB.m_vertexBuffer);
            return false;
        }
        ib = newIB.m_indexBuffer;
    } else {
        // no engine path creates 32-bit index buffers; raw MANAGED fallback
        auto *dev = g_Direct3DDevice();
        if (FAILED(dev->lpVtbl->CreateIndexBuffer(dev, ibytes, D3DUSAGE_WRITEONLY,
                                                  D3DFMT_INDEX32,
                                                  D3DPOOL_MANAGED, &ib, nullptr))) {
            if (newVB.m_vertexBuffer)
                newVB.m_vertexBuffer->lpVtbl->Release(newVB.m_vertexBuffer);
            return false;
        }
        void *p = nullptr;
        if (FAILED(ib->lpVtbl->Lock(ib, 0, ibytes, &p, 0))) {
            ib->lpVtbl->Release(ib);
            if (newVB.m_vertexBuffer)
                newVB.m_vertexBuffer->lpVtbl->Release(newVB.m_vertexBuffer);
            return false;
        }
        std::memcpy(p, st.idx32.data(), ibytes);
        ib->lpVtbl->Unlock(ib);
    }

    // recycle whatever the section held (a clone's own buffers, or a
    // previous replacement) through the engine pool
    if (S->m_indexBuffer != nullptr) {
        nglVertexBuffer::sub_77B5D0((nglVertexBuffer *) &S->m_indexBuffer,
                                    ResourceType::IndexBuffer);
        S->m_indexBuffer = nullptr;
    }
    if (S->field_3C.m_vertexBuffer != nullptr) {
        nglVertexBuffer::sub_77B5D0(&S->field_3C, ResourceType::VertexBuffer);
        S->field_3C.m_vertexBuffer = nullptr;
    }

    S->field_3C.m_vertexData   = (char *)st.vertices.data();
    S->field_3C.Size           = vbytes;
    S->field_3C.m_vertexBuffer = newVB.m_vertexBuffer;
    S->m_indexBuffer   = ib;
    S->m_indices       = st.wide ? nullptr : st.idx16.data();
    S->NVertices       = int(st.nverts);
    S->NIndices        = int(st.nidx);
    S->m_stride        = int(stride);
    S->m_primitiveType = D3DPT_TRIANGLELIST;
    S->StartIndex      = 0;
    S->field_4C        = st.baseVertex * stride; // MinVertexIndex = field_4C /
                                             // stride: the drawn window starts
                                             // past the morph dead zone (see
                                             // the layout note in
                                             // modApplyBuiltSection). Static
                                             // sections have no dead zone, so
                                             // baseVertex is 0 there.
    if (!st.rigid) {
        S->field_5C = st.weightClass;    // 2/3/4-bone shader selector
        S->BonesIdx = st.palette;
        S->NBones   = int(st.nbones);
    }
    // A static section has no skinning fields to set: field_5C, BonesIdx and
    // NBones come from the file and mean something else (or nothing) to the
    // shader bound to it. Writing a palette there would hand the engine a bone
    // array a static mesh does not have.
    return true;
}

} // namespace

bool modIsReplacedSection(nglMeshSection *S)
{
    return modLiveStorage(S) != nullptr;
}

// Called from nglDestroySection before the section (and the tlMemAlloc'd
// palette we installed as BonesIdx) go back to the pool.
void modForgetSection(nglMeshSection *S)
{
    if (S != nullptr)
        modSectionRegistry.erase(S);
}

// Retarget the section material's diffuse texture to the first candidate
// stem that resolves through the engine's texture pipeline: FBX-referenced
// file names first (custom imports shipping their own PNG/DDS overrides by
// name), then the source family ("VENOM" when venom pieces were mapped onto
// another character's mesh). Actor work-mesh clones share material pointers,
// so mutating the material recolors them too. Applied once per
// material+name; unresolvable names leave the target texture untouched.
//
// Source priority per name: mod-shipped image sources (FBX-embedded bytes,
// mods-folder file, loose file next to the FBX) OUTRANK the already-resident
// engine texture. A reskin that keeps the vanilla stem (USM_BLACKSUIT.fbx +
// USM_BLACKSUIT.png) must recolor even while the vanilla texture is resident;
// the resident copy only wins when the mod ships no image of its own (FBX
// referencing an engine stem such as "VENOM" with no override on disk).
// Mod-built textures are cached per stem so sections sharing one image
// decode it once instead of once per material.
static bool modLooksD3DXImage(const uint8_t *d, size_t n)
{
    if (d == nullptr || n < 8) return false;
    if (d[0] == 0x89 && d[1] == 'P' && d[2] == 'N' && d[3] == 'G') return true; // png
    if (d[0] == 0xFF && d[1] == 0xD8)                              return true; // jpg
    if (d[0] == 'B'  && d[1] == 'M')                               return true; // bmp
    if (d[0] == 'D' && d[1] == 'D' && d[2] == 'S' && d[3] == ' ')  return true; // dds
    return false;
}

// A DDS whose pixel format is palette-indexed or pure luminance (the shape a
// pack-texture extractor produces from this engine's P8 textures). D3DX
// decodes those as grayscale index/luma data - the "inverted black & white
// costume" - so in auto texture mode they must never displace a resident
// texture that already carries the real colors.
static bool modDDSIsIndexedOrLuma(const uint8_t *d, size_t n)
{
    if (d == nullptr || n < 128) return false;
    if (!(d[0] == 'D' && d[1] == 'D' && d[2] == 'S' && d[3] == ' ')) return false;
    auto u32 = [&](size_t o) {
        return uint32_t(d[o]) | uint32_t(d[o+1]) << 8
             | uint32_t(d[o+2]) << 16 | uint32_t(d[o+3]) << 24;
    };
    // DDS layout: 4-byte magic, then DDS_HEADER whose DDS_PIXELFORMAT begins
    // 72 bytes in (dwSize..dwReserved1[11] = 4*7 + 44). So from file start:
    // ddspf.dwSize @76, .dwFlags @80, .dwFourCC @84, .dwRGBBitCount @88.
    constexpr size_t pf = 4 + 72;
    const uint32_t pfFlags = u32(pf + 4);       // DDS_PIXELFORMAT::dwFlags
    const uint32_t fourCC  = u32(pf + 8);
    const uint32_t bpp     = u32(pf + 12);      // dwRGBBitCount
    if (pfFlags & 0x00000020u) return true;     // DDPF_PALETTEINDEXED8
    if (pfFlags & 0x00020000u) return true;     // DDPF_LUMINANCE
    if (fourCC == 0 && bpp == 8) return true;   // uncompressed 8bpp
    return false;
}

// A pack extraction RECOMPRESSED after a paletteless decode: a DXT/RGB file
// whose CONTENT is a gray ramp. The header sniff above cannot see it, so
// sample the actual color payload - the RGB565 endpoints of the DXT color
// blocks, or the pixels of an uncompressed RGB surface. Near-total R==G==B
// means the "colors" are palette indices read as luminance: the exact
// texture that turns the black suit white and lets the time-of-day light rig
// tint it by the hour. Only consulted in auto mode when a resident texture
// exists (see usableModBytes), so a mod that genuinely ships a grayscale
// recolor can still force it with sidecar texture=mod.
static bool modDDSLooksGrayscale(const uint8_t *d, size_t n)
{
    if (d == nullptr || n < 128 + 8) return false;
    if (!(d[0] == 'D' && d[1] == 'D' && d[2] == 'S' && d[3] == ' ')) return false;
    auto u32 = [&](size_t o) {
        return uint32_t(d[o]) | uint32_t(d[o+1]) << 8
             | uint32_t(d[o+2]) << 16 | uint32_t(d[o+3]) << 24;
    };
    constexpr size_t pf = 4 + 72;               // DDS_PIXELFORMAT offset
    const uint32_t pfFlags = u32(pf + 4);
    const uint32_t fourCC  = u32(pf + 8);
    const uint32_t bpp     = u32(pf + 12);
    const size_t   data0   = 128;               // legacy header, no DX10 ext
    size_t gray = 0, colored = 0;
    auto tally = [&](int R, int G, int B, int tol) {
        int mx = R > G ? R : G; if (B > mx) mx = B;
        int mn = R < G ? R : G; if (B < mn) mn = B;
        if (mx - mn <= tol) ++gray; else ++colored;
    };
    auto sample565 = [&](uint32_t c) {
        tally(int((c >> 11) & 31) * 255 / 31,
              int((c >>  5) & 63) * 255 / 63,
              int( c        & 31) * 255 / 31, 12);
    };
    if (fourCC == 0x31545844u                   // DXT1
     || fourCC == 0x33545844u                   // DXT3
     || fourCC == 0x35545844u) {                // DXT5
        const size_t blockSz  = (fourCC == 0x31545844u) ? 8u : 16u;
        const size_t colorOff = blockSz - 8u;   // alpha block first on DXT3/5
        for (size_t o = data0; o + blockSz <= n && gray + colored < 4096;
             o += blockSz) {
            sample565(uint32_t(d[o + colorOff])
                    | uint32_t(d[o + colorOff + 1]) << 8);
            sample565(uint32_t(d[o + colorOff + 2])
                    | uint32_t(d[o + colorOff + 3]) << 8);
        }
    } else if (fourCC == 0 && (pfFlags & 0x00000040u)       // DDPF_RGB
               && (bpp == 24 || bpp == 32)) {
        const size_t px = bpp / 8;              // D3D order: B G R (A)
        for (size_t o = data0; o + px <= n && gray + colored < 4096;
             o += px * 7)                       // sparse, stride-agnostic
            tally(d[o + 2], d[o + 1], d[o], 8);
    } else {
        return false;                           // unknown layout: no opinion
    }
    const size_t tot = gray + colored;
    return tot >= 64 && colored * 50 < tot;     // >= 98% of samples are gray
}

// Build a texture named `nm` from raw image file bytes. D3DX (the shipped
// d3dx9_24.dll) decodes the standard formats; anything else goes through the
// engine's own parser. `forceD3DX` covers extensions D3DX loads but that have
// no reliable magic (tga).
static nglTexture *modConstructTexFromBytes(const std::string &nm,
                                            const uint8_t *data, size_t size,
                                            bool forceD3DX = false)
{
    if (data == nullptr || size == 0)
        return nullptr;
    const bool d3dx = forceD3DX || modLooksD3DXImage(data, size);
    nglTexture *tex = nglConstructTexture(tlFixedString{ nm.c_str() },
                                          nglTextureFileFormat(d3dx ? 2 : 0),
                                          const_cast<uint8_t *>(data),
                                          uint32_t(size));
    if (tex != nullptr && d3dx && tex->DXTexture == nullptr)
        return nullptr;                          // d3dx rejected the bytes
    return tex;
}

static bool modReadWholeFile(const std::filesystem::path &p,
                             std::vector<uint8_t> &out)
{
    std::error_code ec;
    if (!std::filesystem::is_regular_file(p, ec))
        return false;
    std::ifstream f(p, std::ios::binary);
    if (!f)
        return false;
    out.assign((std::istreambuf_iterator<char>(f)),
               std::istreambuf_iterator<char>());
    return !out.empty();
}

// ---------------------------------------------------------------------------
// Mod texture caches, and why they need an epoch.
//
// Three caches used to live as function-local statics inside the retarget:
// the per-material "already applied" marker, the per-stem table of textures
// built from mod bytes, and the pin material clones. All three are keyed by a
// RAW POINTER or by a stem whose texture is owned by the engine, and all three
// outlive the objects they describe. That is the same trap modLiveStorage()
// documents for sections ("recycled address, not ours"), and it produced the
// same class of symptom one layer up: switch hero to usm_blacksuit mid-session
// and the character loads WHITE.
//
//   * the reloaded nglMaterialBase lands on the address the previous one was
//     freed from, the "already applied" entry is still there with a matching
//     name hash, and the retarget returns before doing anything at all;
//   * nglReleaseAllTextures() frees everything in the directory, so a stem
//     cached from the previous load resolves to a dangling nglTexture *;
//   * a pin clone is a memcpy of the PREVIOUS load's material - stale shader
//     pointer, stale render state.
//
// Identity is re-established the same way the section registry does it:
// nothing is trusted just because it is in the map. `applied` remembers the
// texture it installed and is only honoured while the material still draws
// with it; clones are refreshed from their source on every lookup; built
// textures are stamped with an epoch and, across an epoch boundary, only
// reused when the engine's own directory still hands back the same pointer.
// ---------------------------------------------------------------------------
namespace {

uint32_t modTexEpoch = 1;

// what the retarget last installed on a material, and what it installed it for
struct ModAppliedTex {
    uint32_t    nameHash = 0;
    nglTexture *tex      = nullptr;
};
std::unordered_map<nglMaterialBase *, ModAppliedTex> modAppliedTex;

// textures built from mod bytes (FBX-embedded, mods/ folder, loose file next
// to the mod), keyed by candidate stem: sections sharing one image decode it
// once instead of once per material.
struct ModBuiltTex {
    nglTexture *tex   = nullptr;
    uint32_t    epoch = 0;
};
std::unordered_map<std::string, ModBuiltTex> modBuiltTex;

// pin/white material clones, keyed by (source material, stem)
std::map<std::pair<nglMaterialBase *, std::string>, nglMaterialBase *> modMatClones;

} // namespace

// Called from nglDestroyTexture: a pointer that is about to be freed must
// leave every cache, whatever epoch it was built in.
void modTexCacheForgetTexture(nglTexture *tex)
{
    if (tex == nullptr)
        return;
    for (auto it = modBuiltTex.begin(); it != modBuiltTex.end(); ) {
        if (it->second.tex == tex) {
            sp_log("[modmesh] cached texture \"%s\" destroyed - dropped\n",
                   it->first.c_str());
            it = modBuiltTex.erase(it);
        } else {
            ++it;
        }
    }
    for (auto &a : modAppliedTex)
        if (a.second.tex == tex)
            a.second.tex = nullptr;      // forces a re-resolve, never a re-bind
}

// Called whenever something invalidates texture pointers wholesale: a texture
// directory purge, or a mesh file load (the reload event itself). Costs at
// most one extra rebuild per stem - cross-epoch reuse is not forbidden, only
// made conditional on the engine confirming the pointer.
void modTexCacheNewEpoch(const char *why)
{
    ++modTexEpoch;
    if (why != nullptr && !modBuiltTex.empty())
        sp_log("[modmesh] texture cache epoch %u (%s): %u cached stem(s) now "
               "need the engine to confirm them before reuse\n",
               modTexEpoch, why, unsigned(modBuiltTex.size()));
}

// A sidecar pin (tex<N>=STEM) and a white= section must repaint ONE section.
// Character materials are shared between sections - and the actor pipeline
// clones work meshes off them - so writing field_1C in place would drag every
// section bound to the same nglMaterialBase along with it: pinning the mouth
// sheet onto the cocoon would repaint the forearms too. Take a shallow
// 0x50-byte copy (the shader pointer and the render state travel with it; only
// the diffuse texture is about to change) and point the section at the copy.
//
// The allocation is cached per (material, stem) so re-applies and rebuilt
// work-mesh clones reuse one - but the CONTENTS are refreshed from the source
// every time. After a character reload the source material has been reloaded
// (or a different material now occupies that address) and the cached clone
// still held the previous load's shader pointer and render state.
static nglMaterialBase *modCloneMaterialForPin(nglMaterialBase *src,
                                               const std::string &stem)
{
    if (src == nullptr)
        return nullptr;
    auto key = std::make_pair(src, stem);
    if (auto it = modMatClones.find(key); it != modMatClones.end()) {
        if (it->second != nullptr) {
            // re-sync with the source: cheap, and the only thing that keeps a
            // clone valid across a mesh-file reload
            nglTexture *keepTex = it->second->field_1C;
            std::memcpy(it->second, src, sizeof(nglMaterialBase));
            it->second->field_1C = keepTex;
            return it->second;
        }
        modMatClones.erase(it);
    }
    auto *copy = static_cast<nglMaterialBase *>(
        tlMemAlloc(int(sizeof(nglMaterialBase)), 8, 0x1000000u));
    if (copy == nullptr)
        return nullptr;
    std::memcpy(copy, src, sizeof(nglMaterialBase));
    modMatClones[key] = copy;
    sp_log("[modmesh] material cloned for pin \"%s\" (%p -> %p)\n",
           stem.c_str(), (void *) src, (void *) copy);
    return copy;
}

// The engine's own 1x1 white texture, the target of a white= section. Falls
// back to a directory lookup when the global has not been filled in yet.
static nglTexture *modWhiteTexture()
{
    if (nglWhiteTex() != nullptr)
        return nglWhiteTex();
    return nglGetTexture(tlFixedString{ "nglwhite" });
}

// Everything this section is called in the file. The engine is the only side
// that knows these: the importer sees FBX material names, not the names the
// retail mesh carries. Both are checked - MaterialName is what the section
// asks for, Material->Name is what it got.
static void modSectionMaterialNames(nglMeshSection *S,
                                    const char *out[2])
{
    out[0] = out[1] = nullptr;
    if (S == nullptr)
        return;
    if (S->MaterialName != nullptr)
        out[0] = S->MaterialName->to_string();
    if (S->Material != nullptr && S->Material->Name != nullptr)
        out[1] = S->Material->Name->to_string();
}

// Does either name read like a piece that is meant to be white? Substring
// match, case-insensitive, against the sidecar's list (white_names=) or the
// importer's default set.
static bool modNameSaysWhite(nglMeshSection *S,
                             const std::vector<std::string> &keys,
                             const char **whichName, const char **whichKey)
{
    const char *names[2];
    modSectionMaterialNames(S, names);
    for (const char *n : names) {
        if (n == nullptr || *n == '\0')
            continue;
        std::string u(n);
        for (auto &c : u) c = char(std::toupper(uint8_t(c)));
        for (const std::string &k : keys)
            if (!k.empty() && u.find(k) != std::string::npos) {
                if (whichName != nullptr) *whichName = n;
                if (whichKey  != nullptr) *whichKey  = k.c_str();
                return true;
            }
    }
    return false;
}

// Should this BLANK section be left white instead of handed to the
// blank-repair fallbacks? Never called for a material that draws a texture.
static bool modBlankShouldStayWhite(nglMeshSection *S,
                                    const modmesh::BuiltSection &B,
                                    int sectionIndex)
{
    if (B.whiteBlank) {
        sp_log("[modmesh] sec%d: untextured material - sidecar white=blank, "
               "drawing WHITE (no salvage, no donor borrow)\n", sectionIndex);
        return true;
    }
    if (!B.whiteByName || !B.whiteNames)
        return false;
    const char *nm = nullptr, *key = nullptr;
    if (!modNameSaysWhite(S, *B.whiteNames, &nm, &key))
        return false;
    sp_log("[modmesh] sec%d: untextured material \"%s\" matches \"%s\" - "
           "drawing WHITE instead of borrowing a sheet (sidecar white=off "
           "disables, white_names= changes the list)\n",
           sectionIndex, nm != nullptr ? nm : "?", key != nullptr ? key : "?");
    return true;
}

// Bind pure white to this section and stop. No candidate search, no salvage,
// no donor borrow: sidecar white= exists precisely because those heuristics
// are wrong for geometry that is white on purpose (the black suit's eye
// lenses and chest spider).
static bool modApplyForcedWhite(nglMeshSection *S, int sectionIndex)
{
    auto *mat = S != nullptr ? S->Material : nullptr;
    if (mat == nullptr)
        return false;
    nglTexture *white = modWhiteTexture();
    if (white == nullptr) {
        sp_log("[modmesh] sec%d: white= requested but the engine white texture "
               "does not exist yet - leaving the material alone\n",
               sectionIndex);
        return false;
    }
    if (auto *c = modCloneMaterialForPin(mat, "\x01white"); c != nullptr) {
        S->Material = c;
        mat = c;
    }
    if (mat->field_1C != white) {
        mat->field_1C = white;
        sp_log("[modmesh] sec%d: forced WHITE (sidecar white=)\n", sectionIndex);
    }
    modAppliedTex[mat] = ModAppliedTex{ 0u, white };
    return true;
}

static void modRetargetSectionTextureInner(nglMeshSection *S,
                                           const modmesh::BuiltSection &B,
                                           const std::filesystem::path &modPath,
                                           int sectionIndex)
{
    auto *mat = S != nullptr ? S->Material : nullptr;
    if (mat == nullptr)
        return;

    // sidecar white=N: the most specific instruction there is - the section
    // draws pure white and nothing below runs. Placed above texture=keep and
    // above the pin handling on purpose: white wins over every policy.
    if (B.forceWhite) {
        modApplyForcedWhite(S, sectionIndex);
        return;
    }

    // sidecar tex<N>=STEM: an explicit instruction outranks every policy below
    const bool pinned = B.texExclusive && !B.textureCandidates.empty();
    if (pinned) {
        if (auto *c = modCloneMaterialForPin(mat, B.textureCandidates.front());
            c != nullptr) {
            S->Material = c;
            mat = c;
        }
    }

    if (B.texMode == 1 && !pinned)               // sidecar texture=keep
        return;

    // BLANK = no diffuse texture bound: the section draws pure white (the
    // venom_eddie cocoon/teeth/eddie-head symptom). Every guard below exists
    // to protect a texture the section already draws with - with nothing
    // bound there is nothing to protect, so blank unlocks the full search.
    const bool blank = (mat->field_1C == nullptr);

    // A blank material draws pure white, and on a character that is often
    // CORRECT - the eye lenses and the chest emblem are untextured white
    // geometry in retail. Everything below this point assumes the opposite
    // (blank == broken, find it a sheet), so the white policies get to answer
    // first. texture=keep is left alone: it already means "do not touch".
    if (blank && B.texMode != 1
        && modBlankShouldStayWhite(S, B, sectionIndex)) {
        modApplyForcedWhite(S, sectionIndex);
        return;
    }

    // automatic blank-fix carrier (importer autotex): it exists ONLY to
    // repair a missing diffuse texture and must never displace a bound one
    if (B.blankOnly && !blank)
        return;

    const std::vector<std::string> &names = B.textureCandidates;
    const std::filesystem::path modDir  = modPath.parent_path();
    const std::string           fbxStem = modPath.stem().string();

    // A section whose geometry was deliberately KEPT (exact round trip) is
    // vanilla in every respect, textures included. Its material assignment in
    // the FBX is only as good as the exporter's guess, and that guess is
    // coarse: the venom_eddie export tags the whole forearm/hand/claw run
    // (sections 2-8) with VENOM_MOUTH and leaves the eddie-reveal pieces with
    // no texture at all. Binding a stem that merely TRAVELLED WITH the FBX -
    // the DDS files it embeds, its .fbm folder, or another resident texture
    // that happens to carry that name - then paints the teeth sheet onto the
    // hands, and a palette-indexed pack extraction read as luminance shows up
    // as the grey/white ramp described above. So for a round-trip section only
    // a file the USER dropped in mods/ may override the resident texture; that
    // is the deliberate act, and it is what keeps the recolor workflow
    // (round-trip FBX + USM_BLACKSUIT.png next to it) working. Sidecar
    // texture=mod restores the full search.
    // ... except when the stem was pinned by hand: the pin IS the deliberate
    // act this restriction exists to protect, so the full search runs.
    // ... and except when the material is BLANK: the restriction protects a
    // vanilla texture, and a blank material has none - white is the one
    // outcome this whole pipeline exists to avoid.
    const bool userFilesOnly = B.keepGeometry && B.texMode != 2 && !pinned
                            && !blank;

    // The stem the FBX names may already BE the texture this material draws
    // with - the normal case for an FBX exported from the game's own asset and
    // re-imported (every material in USM_BLACKSUIT.fbx names "USM_BLACKSUIT",
    // which is exactly what the vanilla section is bound to). There is nothing
    // to retarget: the resident texture IS the intended one.
    //
    // Bailing out here, before the file search, is what makes that safe no
    // matter what sits next to the mod. Otherwise a DDS extracted from the
    // PCPACK - palette-indexed, shipped without its palette - is found in step
    // 4 and shadows the correct texture; read as luminance it renders the suit
    // as a grey ramp, which the time-of-day light rig then tints, so the
    // costume turns white at noon and shifts colour with the hour instead of
    // staying the authored dark purple. Header sniffing (modDDSIsIndexedOrLuma)
    // catches the common variants but cannot catch one that was recompressed on
    // extraction; identity of the target needs no sniffing at all.
    //
    // Sidecar texture=mod (texMode 2) still forces the mod's own bytes.
    if (B.texMode != 2 && !pinned) {
        for (const std::string &nm : names) {
            if (nm.empty() || nm.size() >= 60)
                continue;
            nglTexture *res = nglGetTexture(tlFixedString{ nm.c_str() });
            if (res != nullptr && res == mat->field_1C) {
                sp_log("[modmesh] texture \"%s\" is already the section's own "
                       "texture - retarget skipped, vanilla colors kept "
                       "(sidecar texture=mod forces the mod file)\n",
                       nm.c_str());
                return;
            }
        }
    }

    // In auto mode an indexed/luminance DDS (a pack extraction, not an
    // authored recolor) may only be used when the engine has NO texture of its
    // own for the stem; texture=mod restores the old unconditional behaviour.
    //
    // "Has none" must not be read as "has none RESIDENT RIGHT NOW". That was
    // the reload bug: on a hero switch the character mesh is loaded before its
    // texture pack, nglGetTexture() comes back null purely because of load
    // order, and the guard waves through the very file it exists to reject -
    // the black suit then renders as the grey/white ramp with the light rig
    // tinting it by the hour. Asking the engine to LOAD the stem turns a race
    // into a rule: if the game can produce that texture at all, the mod's
    // palette-indexed extraction of it does not get to win.
    auto engineHasTexture = [](const std::string &nm) -> bool {
        if (nglGetTexture(tlFixedString{ nm.c_str() }) != nullptr)
            return true;
        return nglLoadTexture(tlFixedString{ nm.c_str() }) != nullptr;
    };
    auto usableModBytes = [&](const std::string &nm,
                              const uint8_t *d, size_t n) -> bool {
        if (B.texMode == 2) return true;
        if (!modDDSIsIndexedOrLuma(d, n) && !modDDSLooksGrayscale(d, n))
            return true;
        if (!engineHasTexture(nm)) return true;
        sp_log("[modmesh] texture \"%s\": mod file is a palette/luminance/"
               "grayscale DDS (pack extraction) - keeping the engine's own "
               "texture. Sidecar texture=mod overrides.\n", nm.c_str());
        return false;
    };

    for (const std::string &nm : names) {
        if (nm.empty() || nm.size() >= 60)
            continue;
        uint32_t h = 5381;
        for (char c : nm) h = h * 33u + uint8_t(c);
        // "this name is already applied" is only true while the material STILL
        // DRAWS with the texture we installed for it. A reloaded material lands
        // on a recycled address carrying its own (usually null) diffuse
        // texture; honouring the stale entry there is what made a reloaded
        // character come back white. A blank material is never skipped.
        if (auto it = modAppliedTex.find(mat); it != modAppliedTex.end()) {
            if (it->second.nameHash == h && it->second.tex != nullptr
                && mat->field_1C == it->second.tex)
                return;                          // this name already applied
            if (mat->field_1C == nullptr || it->second.tex != mat->field_1C)
                modAppliedTex.erase(it);         // recycled/reloaded, not ours
        }

        // sidecar tex<N>=WHITE reaching this loop as a stem: never searched
        // for on disk, it means the engine's own white texture
        if (nm == "WHITE" || nm == "NGLWHITE") {
            if (modApplyForcedWhite(S, sectionIndex))
                return;
            continue;
        }

        // 1) a texture already built from mod bytes for this stem. Across an
        //    epoch boundary (a directory purge, a mesh file reload) the
        //    pointer is only trusted when the engine's own directory still
        //    hands back the same object; otherwise it is rebuilt from bytes.
        nglTexture *tex = nullptr;
        if (!userFilesOnly) {
            if (auto mb = modBuiltTex.find(nm); mb != modBuiltTex.end()) {
                if (mb->second.epoch == modTexEpoch) {
                    tex = mb->second.tex;
                } else if (mb->second.tex != nullptr
                           && nglGetTexture(tlFixedString{ nm.c_str() })
                              == mb->second.tex) {
                    mb->second.epoch = modTexEpoch;   // engine confirms it
                    tex = mb->second.tex;
                } else {
                    sp_log("[modmesh] texture \"%s\": cached copy did not "
                           "survive the reload - rebuilding from the mod\n",
                           nm.c_str());
                    modBuiltTex.erase(mb);
                }
            }
        }

        // 2) USER override first: mods/<stem>.png/.jpg/.bmp/.dds bind mods/<stem>.png/.jpg/.bmp/.dds bind
        //    as TLRESOURCE_TYPE_TEXTURE keyed by the lower-case stem hash
        //    (subfolders like mods/VENOM.fbm/ are enumerated too). A file
        //    the user drops in mods/ is the most intentional source there
        //    is, so it OUTRANKS the image embedded in a downloaded FBX -
        //    that is what lets a recolored VENOM_EDDIE_03.png darken a
        //    piece whose embedded texture ships pale.
        if (tex == nullptr) {
            std::string low;
            low.reserve(nm.size());
            for (char c : nm) low.push_back(char(std::tolower(uint8_t(c))));
            std::vector<uint8_t> bytes;
            if (Mod *tm = getMod(to_hash(low.c_str()), TLRESOURCE_TYPE_TEXTURE);
                tm != nullptr && readModFile(tm, bytes) && !bytes.empty()
                && usableModBytes(nm, bytes.data(), bytes.size()))
            {
                tex = modConstructTexFromBytes(nm, bytes.data(), bytes.size());
                if (tex != nullptr)
                    sp_log("[modmesh] texture \"%s\" built from mod file\n",
                           nm.c_str());
            }
        }

        // 3) image bytes embedded in the FBX itself (Video/Content): the
        //    mod is fully self-contained, its colors ship inside it and
        //    take precedence over a resident texture with the same stem
        if (tex == nullptr && !userFilesOnly) {
            if (auto e = B.embeddedTex.find(nm);
                e != B.embeddedTex.end() && e->second && !e->second->empty()
                && usableModBytes(nm, e->second->data(), e->second->size()))
            {
                tex = modConstructTexFromBytes(nm, e->second->data(),
                                               e->second->size());
                if (tex != nullptr)
                    sp_log("[modmesh] texture \"%s\" built from FBX-embedded "
                           "bytes (%u)\n", nm.c_str(),
                           uint32_t(e->second->size()));
            }
        }

        // a round trip carries the FBX's own copy of a VANILLA texture: never
        // let it (or a same-named resident texture) move onto a section whose
        // geometry we kept, only a file the user placed in mods/ may
        if (tex == nullptr && userFilesOnly
            && (B.embeddedTex.count(nm) || B.texRelPath.count(nm)))
            sp_log("[modmesh] texture \"%s\": section is an exact round trip - "
                   "vanilla texture kept (drop mods/%s.png to recolor it, or "
                   "sidecar texture=mod)\n", nm.c_str(), nm.c_str());

        // 4) loose image next to the FBX, resolved from the path written in
        //    the file: <fbxdir>/<rel as written>, the Blender media folder
        //    <fbxdir>/<fbx>.fbm/<basename>, and plain <fbxdir>/<stem>.<ext>
        if (tex == nullptr && !userFilesOnly && !modDir.empty()) {
            std::vector<std::filesystem::path> tries;
            if (auto r = B.texRelPath.find(nm); r != B.texRelPath.end()) {
                std::string rel = r->second;
                std::replace(rel.begin(), rel.end(), '\\', '/');
                std::filesystem::path relP(rel);
                if (relP.is_relative())
                    tries.push_back(modDir / relP);
                tries.push_back(modDir / relP.filename());
                tries.push_back(modDir / (fbxStem + ".fbm") / relP.filename());
            }
            static const char *exts[] = { ".dds", ".png", ".tga",
                                          ".jpg", ".jpeg", ".bmp" };
            for (const char *e : exts) {
                tries.push_back(modDir / (nm + e));
                tries.push_back(modDir / (fbxStem + ".fbm") / (nm + e));
            }
            for (const auto &p : tries) {
                std::vector<uint8_t> bytes;
                if (!modReadWholeFile(p, bytes))
                    continue;
                if (!usableModBytes(nm, bytes.data(), bytes.size()))
                    continue;
                std::string ext = p.extension().string();
                for (auto &c : ext) c = char(std::tolower(uint8_t(c)));
                tex = modConstructTexFromBytes(nm, bytes.data(), bytes.size(),
                                               ext == ".tga");
                if (tex != nullptr) {
                    sp_log("[modmesh] texture \"%s\" read next to the mod: "
                           "\"%s\"\n", nm.c_str(), p.string().c_str());
                    break;
                }
            }
        }

        // anything built from mod bytes above is remembered for the next
        // section/material asking for the same stem, stamped with the epoch it
        // was built in so a later reload knows to re-confirm it
        if (tex != nullptr && !modBuiltTex.count(nm))
            modBuiltTex[nm] = ModBuiltTex{ tex, modTexEpoch };

        // 5) already resident (same character reload, shared pack, ...):
        //    reached only when the mod ships no image of its own, so an
        //    FBX referencing a vanilla engine stem keeps vanilla colors
        if (tex == nullptr && !userFilesOnly)
            tex = nglGetTexture(tlFixedString{ nm.c_str() });

        // 6) the engine's own directory / pack load
        if (tex == nullptr && !userFilesOnly)
            tex = nglLoadTexture(tlFixedString{ nm.c_str() });
        if (tex == nullptr)
            continue;

        if (tex != mat->field_1C) {
            mat->field_1C = tex;
            sp_log("[modmesh] sec%d: material texture -> \"%s\"%s\n",
                   sectionIndex, nm.c_str(), B.texExclusive ? " (pinned)" : "");
        }
        // remember WHAT was installed, not just that something was: the entry
        // is only honoured again while the material still draws with this
        // exact texture (see the lookup above)
        modAppliedTex[mat] = ModAppliedTex{ h, tex };
        return;
    }
}

// Wrapper: run the retarget, then make sure the section did not end up
// without a diffuse texture. A material whose field_1C is null draws pure
// white - the "blank" pieces of the venom_eddie import (the reveal cocoon, the
// teeth, eddie's head), which the exporter ships with no material reference.
// Recovery runs in two stages after the retarget proper:
//   1. numbered-variant salvage - family stems tried with _01/_02/... suffixes
//      against the resident set and the engine loader;
//   2. sibling-donor fallback - borrow the diffuse texture of the best
//      textured section of the SAME mesh (most vertices wins: that is the
//      body sheet; environment/ink sheets are skipped). A blank piece painted
//      with the body's own sheet blends in when it is ever on screen, which
//      beats the alternative in every case: flat white on top of the
//      character.
// Only a genuinely texture-less mesh still falls through to the WHITE
// message, which names the section index and the stems that were tried -
// exactly what a sidecar pin needs:  tex<N>=<STEM>
static void modRetargetSectionTexture(nglMeshSection *S,
                                      const modmesh::BuiltSection &B,
                                      const std::filesystem::path &modPath,
                                      int sectionIndex = -1,
                                      nglMesh *Mesh = nullptr)
{
    // One line per section naming the MATERIAL, before anything is changed.
    // This is what turns "the eyes are the wrong colour" into an edit: the
    // section map from the importer gives the index, this gives the name the
    // white_names= list is matched against and whether the material was blank
    // to begin with.
    if (S != nullptr) {
        const char *names[2];
        modSectionMaterialNames(S, names);
        sp_log("[modmesh] sec%d: material \"%s\" (as \"%s\") diffuse=%s\n",
               sectionIndex,
               names[1] != nullptr ? names[1] : "?",
               names[0] != nullptr ? names[0] : "?",
               (S->Material != nullptr && S->Material->field_1C != nullptr)
                   ? "bound" : "NONE (draws white)");
    }

    modRetargetSectionTextureInner(S, B, modPath, sectionIndex);

    // sidecar white=: the section is finished. Neither the numbered-variant
    // salvage nor the sibling-donor borrow may run on it - "the material has
    // no diffuse texture" is the intended state for a white piece, not a
    // defect, and the donor borrow would paint the body sheet over the eye
    // lenses and the chest spider.
    if (B.forceWhite)
        return;

    auto *mat = S != nullptr ? S->Material : nullptr;
    if (mat == nullptr || mat->field_1C != nullptr)
        return;

    // Still blank after the retarget - the candidate search found nothing.
    // Ask the white policies once more before the fallbacks start guessing:
    // the inner pass can leave a section blank on a path that never reached
    // the check above (an empty candidate list, a userFilesOnly bail-out).
    if (B.texMode != 1 && modBlankShouldStayWhite(S, B, sectionIndex)) {
        modApplyForcedWhite(S, sectionIndex);
        return;
    }

    // Still blank: numbered-variant salvage. The stems the file names are
    // often FAMILIES ("VENOM_EDDIE") whose real sheets carry numbered
    // suffixes ("VENOM_EDDIE_01") - try those against the resident set and
    // the engine loader before giving up. A miss costs nothing: the
    // alternative is a section that draws pure white.
    if (B.texMode != 1) {
        static const char *sfx[] = { "_01", "_02", "_03", "_04",
                                     "_00", "_1", "_2" };
        for (const std::string &nm : B.textureCandidates) {
            if (nm.empty())
                continue;
            std::string base = nm;               // "VENOM_EDDIE000" -> family
            while (!base.empty() && std::isdigit(uint8_t(base.back())))
                base.pop_back();
            while (!base.empty() && base.back() == '_')
                base.pop_back();
            std::vector<std::string> roots{ nm };
            if (!base.empty() && base != nm)
                roots.push_back(base);
            for (const std::string &root : roots) {
                for (const char *s : sfx) {
                    const std::string v = root + s;
                    if (v.size() >= 60)
                        continue;
                    nglTexture *tex = nglGetTexture(tlFixedString{ v.c_str() });
                    if (tex == nullptr)
                        tex = nglLoadTexture(tlFixedString{ v.c_str() });
                    if (tex == nullptr)
                        continue;
                    mat->field_1C = tex;
                    sp_log("[modmesh] sec%d: blank material salvaged - "
                           "\"%s\" resolved for stem \"%s\"\n",
                           sectionIndex, v.c_str(), nm.c_str());
                    return;
                }
            }
        }
    }

    // Still blank: sibling-donor fallback. Every by-name path is exhausted
    // (candidates, mod files, embedded bytes, loose files, resident set,
    // engine loader, numbered variants), so stop resolving NAMES and take a
    // TEXTURE that provably exists: the diffuse of another section of this
    // same mesh. The donor with the most vertices is the body sheet, which
    // is exactly what the venom_eddie reveal pieces should wear when they
    // are ever on screen; environment/ink sheets (SPHRMAP & friends, mostly
    // white hatching) are never borrowed. texture=keep still wins: the user
    // asked for the material to stay untouched, white included.
    if (B.texMode != 1 && Mesh != nullptr && Mesh->Sections != nullptr) {
        auto envSheet = [](const char *nm) -> bool {
            if (nm == nullptr) return false;
            std::string u = nm;
            for (auto &c : u) c = char(std::toupper(uint8_t(c)));
            static const char *bad[] = { "SPHRMAP", "SPHMAP", "SPHEREMAP",
                "ENVMAP", "CUBEMAP", "REFLECT", "LIGHTMAP", "SHADOW" };
            for (const char *b : bad)
                if (u.find(b) != std::string::npos) return true;
            return false;
        };
        nglMeshSection *donor = nullptr;
        for (auto i = 0u; i < Mesh->NSections; ++i) {
            nglMeshSection *o = Mesh->Sections[i].Section;
            if (o == nullptr || o == S)
                continue;
            nglMaterialBase *om = o->Material;
            if (om == nullptr || om == mat || om->field_1C == nullptr)
                continue;
            if (envSheet(om->Name != nullptr ? om->Name->to_string() : nullptr))
                continue;
            if (donor == nullptr || o->NVertices > donor->NVertices)
                donor = o;
        }
        if (donor != nullptr && donor->Material != nullptr
            && donor->Material->field_1C != nullptr) {
            mat->field_1C = donor->Material->field_1C;
            sp_log("[modmesh] sec%d: blank material - borrowed the diffuse "
                   "texture of sibling section \"%s\" (%d verts) so it never "
                   "draws white. Pin the intended sheet with tex%d=<STEM>.\n",
                   sectionIndex,
                   donor->Material->Name != nullptr
                       ? donor->Material->Name->to_string() : "?",
                   donor->NVertices, sectionIndex);
            return;
        }
    }

    std::string tried;
    for (const std::string &nm : B.textureCandidates) {
        if (!tried.empty()) tried += ", ";
        tried += nm;
    }
    sp_log("[modmesh] sec%d: material has NO diffuse texture - this section "
           "renders WHITE. Stems tried: [%s]. Bind one from the sidecar with "
           "tex%d=<STEM> (see the \"texture stems present in the file\" line "
           "of the section map).\n",
           sectionIndex, tried.empty() ? "-" : tried.c_str(), sectionIndex);
}

// ---------------------------------------------------------------------------
//  bounding volumes after a replacement
// ---------------------------------------------------------------------------
// The spheres shipped in the PCMESH describe the VANILLA geometry. Replacing a
// section without refreshing them leaves the engine reasoning about a body that
// is no longer there: sections get frustum-culled while on screen (or drawn
// while off), LOD picks the wrong level, and - because the chase camera frames
// the actor from its bounds - the camera settles at the wrong stand-off, which
// is how a replaced character ends up with the view glued to its back.
// Smallest sphere containing both inputs (a, ra) and (b, rb) -> (out, rout).
// Used to GROW a bounding volume, never to shrink it.
static void modSphereUnion(const float a[3], float ra,
                           const float b[3], float rb,
                           float out[3], float &rout)
{
    const float dx = b[0] - a[0], dy = b[1] - a[1], dz = b[2] - a[2];
    const float d  = std::sqrt(dx * dx + dy * dy + dz * dz);
    if (d + rb <= ra) {                       // b inside a
        out[0] = a[0]; out[1] = a[1]; out[2] = a[2]; rout = ra; return;
    }
    if (d + ra <= rb) {                       // a inside b
        out[0] = b[0]; out[1] = b[1]; out[2] = b[2]; rout = rb; return;
    }
    rout = (d + ra + rb) * 0.5f;
    if (d > 1e-8f) {
        const float t = (rout - ra) / d;      // slide from a towards b
        out[0] = a[0] + dx * t;
        out[1] = a[1] + dy * t;
        out[2] = a[2] + dz * t;
    } else {
        out[0] = a[0]; out[1] = a[1]; out[2] = a[2];
    }
}

// The tight bind-pose fit is NOT a valid replacement for the shipped sphere:
// measured against VENOM.PCPACK the retail radii run up to 0.24 units wider
// than the bind-pose extent, because the volume has to hold the body in every
// animated pose. So the new sphere is the UNION of the vanilla one and the
// replacement's extent - it grows to cover geometry the mod added, and never
// shrinks below the margin the artists shipped.
static void modRecomputeSectionSphere(nglMeshSection *S,
                                      const std::vector<float> &verts)
{
    if (S == nullptr || verts.size() < 16)
        return;
    float lo[3] = { 3.4e38f, 3.4e38f, 3.4e38f };
    float hi[3] = { -3.4e38f, -3.4e38f, -3.4e38f };
    for (size_t v = 0; v + 16 <= verts.size(); v += 16)
        for (int c = 0; c < 3; ++c) {
            const float x = verts[v + c];
            if (!std::isfinite(x)) return;          // leave the vanilla sphere
            if (x < lo[c]) lo[c] = x;
            if (x > hi[c]) hi[c] = x;
        }
    float nc[3] = { (lo[0] + hi[0]) * 0.5f, (lo[1] + hi[1]) * 0.5f,
                    (lo[2] + hi[2]) * 0.5f };
    float nr2 = 0.f;
    for (size_t v = 0; v + 16 <= verts.size(); v += 16) {
        const float dx = verts[v] - nc[0], dy = verts[v + 1] - nc[1],
                    dz = verts[v + 2] - nc[2];
        const float d2 = dx * dx + dy * dy + dz * dz;
        if (d2 > nr2) nr2 = d2;
    }
    float nr = std::sqrt(nr2);

    const bool oldValid = std::isfinite(S->SphereRadius) && S->SphereRadius > 0.f
                       && std::isfinite(S->SphereCenter[0])
                       && std::isfinite(S->SphereCenter[1])
                       && std::isfinite(S->SphereCenter[2]);
    float oc[3] = { S->SphereCenter[0], S->SphereCenter[1], S->SphereCenter[2] };
    if (oldValid) {
        // A fit whose radius or centre offset dwarfs the vanilla sphere is an
        // importer outlier, not real geometry. Unioning it in poisons every
        // consumer of the bounds: nglGetLOD measures the distance to a centre
        // that is no longer on the body (the character trips the last LOD
        // threshold and is culled while standing on screen), and the chase
        // camera solves its stand-off against the same volume (it dives
        // inside the mesh). Keep the vanilla sphere - a slightly-tight
        // sphere at worst pops at the screen edge - and let the log point
        // at the offending import.
        const float lim = S->SphereRadius * 2.0f + 0.5f;
        const float dx = nc[0] - oc[0], dy = nc[1] - oc[1], dz = nc[2] - oc[2];
        const float off = std::sqrt(dx * dx + dy * dy + dz * dz);
        if (nr > lim || off > lim) {
            sp_log("[modmesh] section sphere fit r=%.2f off=%.2f vs vanilla "
                   "r=%.2f - importer outlier, vanilla sphere kept\n",
                   nr, off, S->SphereRadius);
            return;
        }
    }
    float rc[3], rr;
    if (oldValid) modSphereUnion(oc, S->SphereRadius, nc, nr, rc, rr);
    else          { rc[0] = nc[0]; rc[1] = nc[1]; rc[2] = nc[2]; rr = nr; }

    S->SphereCenter[0] = rc[0];
    S->SphereCenter[1] = rc[1];
    S->SphereCenter[2] = rc[2];
    S->SphereCenter[3] = 1.f;
    S->SphereRadius    = rr;
}

// Union of the mesh's section spheres, by the same construction the engine uses
// for the file-wide sphere further down: box over every center +/- radius, then
// the largest reach from that box's center. Runs before the engine's own
// aggregation so it feeds on corrected per-mesh values.
static void modRecomputeMeshBounds(nglMesh *Mesh)
{
    if (Mesh == nullptr || Mesh->NSections <= 0 || Mesh->Sections == nullptr)
        return;
    float lo[3] = { 3.4e38f, 3.4e38f, 3.4e38f };
    float hi[3] = { -3.4e38f, -3.4e38f, -3.4e38f };
    int seen = 0;
    for (int i = 0; i < Mesh->NSections; ++i) {
        nglMeshSection *S = Mesh->Sections[i].Section;
        if (S == nullptr || !std::isfinite(S->SphereRadius))
            continue;
        const float r = S->SphereRadius;
        if (!std::isfinite(S->SphereCenter[0])
            || !std::isfinite(S->SphereCenter[1])
            || !std::isfinite(S->SphereCenter[2]))
            continue;
        for (int c = 0; c < 3; ++c) {
            if (S->SphereCenter[c] - r < lo[c]) lo[c] = S->SphereCenter[c] - r;
            if (S->SphereCenter[c] + r > hi[c]) hi[c] = S->SphereCenter[c] + r;
        }
        ++seen;
    }
    if (seen == 0)
        return;
    const float cx = (lo[0] + hi[0]) * 0.5f;
    const float cy = (lo[1] + hi[1]) * 0.5f;
    const float cz = (lo[2] + hi[2]) * 0.5f;
    float rad = 0.f;
    for (int i = 0; i < Mesh->NSections; ++i) {
        nglMeshSection *S = Mesh->Sections[i].Section;
        if (S == nullptr || !std::isfinite(S->SphereRadius))
            continue;
        const float dx = S->SphereCenter[0] - cx;
        const float dy = S->SphereCenter[1] - cy;
        const float dz = S->SphereCenter[2] - cz;
        const float reach = std::sqrt(dx * dx + dy * dy + dz * dz)
                          + S->SphereRadius;
        if (reach > rad) rad = reach;
    }
    float nc[3] = { cx, cy, cz }, rc[3], rr;
    const bool oldValid = std::isfinite(Mesh->SphereRadius)
                       && Mesh->SphereRadius > 0.f
                       && std::isfinite(Mesh->field_20[0])
                       && std::isfinite(Mesh->field_20[1])
                       && std::isfinite(Mesh->field_20[2]);
    float oc[3] = { Mesh->field_20[0], Mesh->field_20[1], Mesh->field_20[2] };
    if (oldValid) modSphereUnion(oc, Mesh->SphereRadius, nc, rad, rc, rr);
    else          { rc[0] = cx; rc[1] = cy; rc[2] = cz; rr = rad; }

    Mesh->field_20[0] = rc[0];
    Mesh->field_20[1] = rc[1];
    Mesh->field_20[2] = rc[2];
    Mesh->field_20[3] = 1.f;
    Mesh->SphereRadius = rr;
    sp_log("[modmesh] mesh bounds: center (%.3f %.3f %.3f) radius %.3f "
           "(was %.3f)\n", rc[0], rc[1], rc[2], rr,
           oldValid ? Mesh->SphereRadius : 0.f);
}

// Packs the importer's canonical 16-float rows down into a STATIC section's
// own vertex format. Channels the target does not have are simply not written;
// bytes the sniffer did not account for keep the value the ORIGINAL vertex had
// at that offset (tangents, a second UV set, padding), which is the only sane
// default: zeroing them would blank whatever the shader reads there. The
// template row is the original section's first vertex.
static std::vector<float> modPackRigidVertices(const modmesh::BuiltSection &B,
                                               const void *templateRow)
{
    const uint32_t stride = B.targetStride ? B.targetStride : 32u;
    const size_t   nv     = B.vertices.size() / 16;
    std::vector<float> outF((nv * stride + 3) / 4, 0.f);
    uint8_t *out = reinterpret_cast<uint8_t *>(outF.data());

    for (size_t i = 0; i < nv; ++i) {
        uint8_t *dst = out + i * stride;
        if (templateRow != nullptr)
            std::memcpy(dst, templateRow, stride);
        const float *r = B.vertices.data() + i * 16;
        if (B.tPosOff >= 0 && uint32_t(B.tPosOff) + 12 <= stride)
            std::memcpy(dst + B.tPosOff, r + 0, 12);
        if (B.tNrmOff >= 0 && uint32_t(B.tNrmOff) + 12 <= stride)
            std::memcpy(dst + B.tNrmOff, r + 3, 12);
        if (B.tUvOff >= 0 && uint32_t(B.tUvOff) + 8 <= stride)
            std::memcpy(dst + B.tUvOff, r + 6, 8);
        if (B.tColOff >= 0 && uint32_t(B.tColOff) + 4 <= stride
            && i < B.colors.size())
            std::memcpy(dst + B.tColOff, &B.colors[i], 4);
    }
    return outF;
}

static bool modApplyBuiltSection(nglMeshSection *S, const modmesh::BuiltSection &B,
                                 int meshNBones)
{
    if (B.vertices.empty() || B.indices.empty())
        return false;

    const uint32_t nverts = uint32_t(B.vertices.size() / 16);
    const uint32_t nidx   = uint32_t(B.indices.size());

    // -----------------------------------------------------------------
    // STATIC section: the section's own vertex format, no morph dead zone
    // (retail morph playback is a character-mesh feature and never addresses
    // a static section), no bone palette, no weight class. The vertex window
    // therefore starts at 0 and the indices are used as the importer built
    // them.
    // -----------------------------------------------------------------
    if (B.rigid) {
        const uint32_t stride = B.targetStride ? B.targetStride : 32u;
        const bool wide = nverts > 0xFFFF;

        // Template row: keep whatever the original vertex held in the bytes
        // the layout does not describe. On a RE-apply (the mesh file is parsed
        // again, e.g. a second character load or a mod reload) the vanilla
        // stream is long gone - but our own previous packing still carries
        // those bytes, so it serves as the template and they survive every
        // reload instead of decaying to zero on the second one.
        const void *tmpl = nullptr;
        const ModSectionStorage *prevSt = modLiveStorage(S);
        if (prevSt != nullptr) {
            if (prevSt->rigid && prevSt->strideBytes == stride
                && !prevSt->vertices.empty())
                tmpl = prevSt->vertices.data();
        } else if (S->field_3C.m_vertexData != nullptr && S->NVertices > 0
                   && uint32_t(S->m_stride) == stride) {
            tmpl = S->field_3C.m_vertexData;      // still the vanilla stream
        }

        ModSectionStorage next;
        next.nverts      = nverts;
        next.nidx        = nidx;
        next.paddedVerts = nverts;
        next.baseVertex  = 0;
        next.origVerts   = prevSt != nullptr
                         ? prevSt->origVerts
                         : uint32_t(S->NVertices > 0 ? S->NVertices : 0);
        next.wide        = wide;
        next.strideBytes = stride;
        next.rigid       = true;
        next.weightClass = 0;
        next.layPos      = B.tPosOff;
        next.layNrm      = B.tNrmOff;
        next.layUv       = B.tUvOff;
        next.layCol      = B.tColOff;
        next.revision    = ++modSectionRevision;
        next.vertices    = modPackRigidVertices(B, tmpl);
        if (wide) {
            next.idx32.assign(B.indices.begin(), B.indices.end());
            sp_log("[modmesh] static section with %u verts uses a 32-bit index "
                   "buffer\n", nverts);
        } else {
            next.idx16.reserve(nidx);
            for (uint32_t v : B.indices)
                next.idx16.push_back(uint16_t(v));
        }

        ModSectionStorage &st = modSectionRegistry[S];
        st = std::move(next);
        if (!modApplyStorageToSection(S, st)) {
            modSectionRegistry.erase(S);
            return false;
        }
        modRecomputeSectionSphere(S, B.vertices);
        sp_log("[modmesh] static section replaced: %u verts, %u idx, stride %u "
               "(pos %d nrm %d uv %d col %d)%s\n",
               nverts, nidx, stride, B.tPosOff, B.tNrmOff, B.tUvOff, B.tColOff,
               tmpl != nullptr ? "" : ", no template row");
        return true;
    }

    // -----------------------------------------------------------------
    // Morph dead zone. Retail morph playback (facial animation - eddie,
    // the spidey head, MJ) writes per-vertex deltas addressed by VANILLA
    // vertex indices. The old layout kept the replacement vertices at the
    // front of the buffer and only padded the tail, so those writes landed
    // ON TOP of re-welded vertices - random spikes that accumulate frame
    // over frame into the "exploded blob". New layout:
    //
    //     [ 0 .. origVerts )              dead zone (all-zero rows)
    //     [ origVerts .. origVerts+n )    the replacement vertices
    //
    // Index values are shifted by origVerts and field_4C points the draw
    // window (MinVertexIndex = field_4C / stride) past the dead zone, so
    // every vanilla-indexed morph write lands on vertices that are never
    // drawn. Facial morphs become a no-op on replaced sections instead of
    // shredding them. CPU consumers stay consistent: m_vertexData holds the
    // same layout as the VB and m_indices carries the shifted values.
    // -----------------------------------------------------------------
    // S->NVertices is OUR count once the section has been replaced before, so
    // re-deriving the dead zone from it would stack a new dead zone on top of
    // the old one every time the mesh file is parsed again.
    const ModSectionStorage *prev = modLiveStorage(S);
    const uint32_t origVerts  = prev != nullptr
                              ? prev->origVerts
                              : uint32_t(S->NVertices > 0 ? S->NVertices : 0);
    const uint32_t baseVertex = origVerts;
    const uint32_t padded     = baseVertex + nverts;
    const bool     wide       = padded > 0xFFFF;

    ModSectionStorage next;
    next.nverts      = nverts;
    next.nidx        = nidx;
    next.paddedVerts = padded;
    next.baseVertex  = baseVertex;
    next.origVerts   = origVerts;
    next.wide        = wide;
    next.weightClass = B.weightClass;
    next.vertices.assign(size_t(padded) * 16, 0.f);
    std::copy(B.vertices.begin(), B.vertices.end(),
              next.vertices.begin() + size_t(baseVertex) * 16);
    if (wide) {
        next.idx32.reserve(nidx);
        for (uint32_t v : B.indices)
            next.idx32.push_back(v + baseVertex);
        sp_log("[modmesh] section with %u verts uses a 32-bit index buffer\n",
               nverts);
    } else {
        next.idx16.reserve(nidx);
        for (uint32_t v : B.indices)
            next.idx16.push_back(uint16_t(v + baseVertex));
    }

    // final palette snapshot: either the rebuilt one or the section's own,
    // so work-mesh mirrors always get a self-contained copy.
    //
    // Last line of defence before the GPU: no entry may index past
    // Mesh->Bones, whatever the importer produced. An out-of-range entry
    // multiplies the section by a garbage matrix - the screen-crossing
    // spikes. Entry 0 at worst pins those vertices to the first bone: the
    // body stays whole and visible.
    if (!B.keepOriginalPalette && !B.palette.empty()) {
        std::vector<uint16_t> pal(B.palette.begin(), B.palette.end());
        if (meshNBones > 0) {
            uint32_t clamped = 0;
            for (auto &e : pal)
                if (int(e) >= meshNBones) { e = 0; ++clamped; }
            if (clamped)
                sp_log("[modmesh] %u palette entries >= NBones (%d) clamped "
                       "to bone 0\n", clamped, meshNBones);
        }
        next.setPalette(pal.data(), uint32_t(pal.size()));
    }
    else if (S->BonesIdx != nullptr && S->NBones > 0)
        next.setPalette(S->BonesIdx, uint32_t(S->NBones));
    else
        next.setPalette(nullptr, 0);
    next.revision = ++modSectionRevision;

    ModSectionStorage &st = modSectionRegistry[S];
    st = std::move(next);
    if (!modApplyStorageToSection(S, st)) {
        modSectionRegistry.erase(S);
        return false;
    }
    // fit the culling/camera sphere to what is actually drawn now
    modRecomputeSectionSphere(S, B.vertices);
    return true;
}

// ---------------------------------------------------------------------------
// Static section vertex layout.
//
// A character section is the retail 64-byte usperson/us_character row and the
// importer knows it by heart. Everything else - weapons, pickups, hero-gauge
// meshes, props, world geometry - carries an arbitrary interleaved D3D vertex
// whose declaration lives in the section's VertexDef, behind a vtable we do
// not own. And we run BEFORE the per-section loop assigns Material/shader, so
// the shader name is not available to ask either.
//
// What IS available is the stream itself, and D3D vertex data is highly
// self-describing:
//   * POSITION is first in every declaration the game uses -> offset 0;
//   * a NORMAL is a float3 of length ~1;
//   * a D3DCOLOR is a dword whose float reinterpretation is NaN/Inf (any
//     opaque colour has 0xFF in the top byte, i.e. exponent 255) or a
//     denormal-scale value - never a plausible coordinate;
//   * a UV is a float2 of small finite values.
// The sniff is logged, and sidecar "layout=stride:32,pos:0,nrm:12,uv:24" is
// the escape hatch when a build's layout defeats it.
// ---------------------------------------------------------------------------
namespace {

bool modFloatPlausible(float f, float lim)
{
    return std::isfinite(f) && std::fabs(f) <= lim
        && (f == 0.f || std::fabs(f) > 1e-30f);
}

// dword that cannot be a coordinate: packed colour
bool modLooksColorDword(const uint8_t *p)
{
    float f;
    std::memcpy(&f, p, 4);
    if (!std::isfinite(f)) return true;                 // exponent 255
    if (f != 0.f && std::fabs(f) < 1e-30f) return true; // denormal
    if (std::fabs(f) > 1e18f) return true;
    return false;
}

// Does a 64-byte stream really carry the skinned row? Blend indices are -1 or
// small non-negative integers, weights are in [0,1] and sum to ~1. A static
// mesh that happens to be 64 bytes wide (position + normal + two UV sets +
// colour + tangent) fails this and must not be treated as skinnable.
bool modLooksSkinned64(const uint8_t *base, uint32_t nverts)
{
    const uint32_t n = std::min(nverts, 64u);
    if (n == 0) return false;
    uint32_t ok = 0;
    for (uint32_t k = 0; k < n; ++k) {
        const float *r = (const float *)(base + size_t(k) * 64);
        bool good = true;
        float wsum = 0.f;
        for (int i = 0; i < 4; ++i) {
            const float s = r[8 + i], w = r[12 + i];
            if (!std::isfinite(s) || !std::isfinite(w)) { good = false; break; }
            if (w < -1e-4f || w > 1.0001f)             { good = false; break; }
            if (s < -1.5f  || s > 255.f)               { good = false; break; }
            if (std::fabs(s - std::floor(s + 0.5f)) > 1e-3f) { good = false; break; }
            wsum += w > 0.f ? w : 0.f;
        }
        if (good && wsum > 0.9f && wsum < 1.1f) ++ok;
    }
    return ok * 4 >= n * 3;                             // 75% of the sample
}

void modSniffSectionLayout(modmesh::OrigSectionView &ov, unsigned secIdx)
{
    const uint8_t *base = (const uint8_t *)ov.verts;
    const uint32_t stride = ov.strideBytes;
    ov.skinned = false;
    ov.posOff = 0; ov.nrmOff = -1; ov.uvOff = -1; ov.colOff = -1;
    if (base == nullptr || ov.nverts == 0 || stride < 12 || (stride % 4) != 0)
        return;

    const uint32_t n = std::min(ov.nverts, 64u);
    const uint32_t step = std::max(1u, ov.nverts / n);

    auto sampleFloat3Unit = [&](uint32_t off) {
        if (off + 12 > stride) return false;
        uint32_t ok = 0, seen = 0;
        for (uint32_t k = 0; k < ov.nverts && seen < n; k += step, ++seen) {
            float v[3];
            std::memcpy(v, base + size_t(k) * stride + off, 12);
            if (!std::isfinite(v[0]) || !std::isfinite(v[1]) || !std::isfinite(v[2]))
                continue;
            const float l = std::sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
            if (l > 0.9f && l < 1.1f) ++ok;
        }
        return seen > 0 && ok * 4 >= seen * 3;
    };
    auto sampleFloat2UV = [&](uint32_t off) {
        if (off + 8 > stride) return false;
        uint32_t ok = 0, seen = 0;
        for (uint32_t k = 0; k < ov.nverts && seen < n; k += step, ++seen) {
            float v[2];
            std::memcpy(v, base + size_t(k) * stride + off, 8);
            if (modFloatPlausible(v[0], 64.f) && modFloatPlausible(v[1], 64.f))
                ++ok;
        }
        return seen > 0 && ok * 4 >= seen * 3;
    };
    auto sampleColor = [&](uint32_t off) {
        if (off + 4 > stride) return false;
        uint32_t ok = 0, seen = 0;
        for (uint32_t k = 0; k < ov.nverts && seen < n; k += step, ++seen)
            if (modLooksColorDword(base + size_t(k) * stride + off)) ++ok;
        return seen > 0 && ok * 4 >= seen * 3;
    };

    uint32_t cur = 12;                              // position occupies 0..11
    if (sampleFloat3Unit(cur)) { ov.nrmOff = int(cur); cur += 12; }
    // a colour can sit before or after the UV set; take the first one found
    for (uint32_t off = cur; off + 4 <= stride; off += 4)
        if (sampleColor(off)) { ov.colOff = int(off); break; }
    if (ov.colOff == int(cur)) cur += 4;
    if (sampleFloat2UV(cur)) ov.uvOff = int(cur);
    else                                            // colour between nrm and uv
        for (uint32_t off = cur; off + 8 <= stride; off += 4) {
            if (int(off) == ov.colOff) continue;
            if (sampleFloat2UV(off)) { ov.uvOff = int(off); break; }
        }

    sp_log("[modmesh] sec%u: static vertex layout sniffed - stride %u, "
           "pos 0, nrm %d, uv %d, col %d\n",
           secIdx, stride, ov.nrmOff, ov.uvOff, ov.colOff);
    if (ov.uvOff < 0)
        sp_log("[modmesh] sec%u: no UV channel recognised in a %u-byte vertex "
               "- the import will keep the original UVs where it can; add "
               "\"layout=stride:%u,pos:0,nrm:%d,uv:<offset>\" to the sidecar "
               "if the section is textured\n",
               secIdx, stride, stride, ov.nrmOff);
}

} // namespace

static std::vector<std::optional<modmesh::BuiltSection>>
modBuildSectionsForMesh(Mod *mod, nglMesh *Mesh)
{
    static bool logWired = false;
    if (!logWired) {
        logWired = true;
        modmesh::setLog(+[](const char *m) { sp_log("%s", m); });
    }

    auto scene = modmesh::loadScene(mod->Path.string(),
                                    mod->Data.data(), mod->Data.size());
    if (!scene)
        return {};

    if (!scene->anims.empty())
        sp_log("[modmesh] \"%s\": %u animation clip(s) in the file\n",
               mod->Path.filename().string().c_str(),
               unsigned(scene->anims.size()));

    std::vector<modmesh::OrigSectionView> views;
    views.reserve(Mesh->NSections);
    for (auto i = 0u; i < Mesh->NSections; ++i) {
        nglMeshSection *S = Mesh->Sections[i].Section;
        modmesh::OrigSectionView ov;
        ov.strideBytes = uint32_t(S->m_stride);
        ov.nverts      = uint32_t(S->NVertices);
        ov.verts       = (const float *)S->field_3C.m_vertexData;
        ov.palette     = S->BonesIdx;
        ov.nbones      = S->NBones;
        // A section that already carries a replacement stores its real rows
        // BEHIND the morph dead zone. Reading from row 0 handed the weight
        // transfer a block of all-zero donors: every corner then fell into the
        // degenerate branch, every vertex ended up rigid on palette slot 0, and
        // the whole body collapsed onto the root bone.
        const ModSectionStorage *st = modLiveStorage(S);
        if (st != nullptr) {
            const uint32_t stride = st->strideBytes ? st->strideBytes : 64u;
            ov.verts = (const float *)((const uint8_t *)ov.verts
                                       + size_t(st->baseVertex) * stride);
            ov.nverts = st->nverts;
        }
        // Which family is this? The stride is the discriminator - the retail
        // skinned row is exactly 64 bytes - but a static vertex can be 64
        // bytes wide too, so the blend lanes are checked before trusting it.
        // Everything that is not the skinned row gets its layout sniffed.
        const bool skinned64 =
            ov.strideBytes == 64 && ov.verts != nullptr && ov.nverts > 0
            && ((st != nullptr && !st->rigid)
                || modLooksSkinned64((const uint8_t *)ov.verts, ov.nverts));
        if (skinned64) {
            ov.skinned = true;                    // defaults already describe it
        } else if (ov.verts != nullptr && ov.nverts > 0) {
            if (st != nullptr && st->rigid) {
                // re-parse of a section we already replaced: reuse the layout
                // the previous pass resolved instead of sniffing our own output
                ov.skinned = false;
                ov.posOff = st->layPos; ov.nrmOff = st->layNrm;
                ov.uvOff  = st->layUv;  ov.colOff = st->layCol;
            } else {
                modSniffSectionLayout(ov, i);
            }
        }
        views.push_back(ov);
    }

    // reference frame for the importer's fit when the vertex views are not
    // the retail 64-byte layout (prerelease/beta builds). We run BEFORE the
    // bone-inversion loop of nglLoadMeshFileInternal, so Mesh->Bones still
    // holds BIND POSE matrices: row 3 (floats 12..14 of the 4x4 storage) is
    // the bone position in model space.
    modmesh::OrigMeshRef ref;
    // Hard upper bound for anything that ends up in BonesIdx: the engine
    // indexes Mesh->Bones with it directly, so a palette entry >= NBones reads
    // whatever sits behind the bone array.
    ref.nbones = Mesh->NBones;
    if (Mesh->NBones > 0 && Mesh->Bones != nullptr) {
        const float *m = reinterpret_cast<const float *>(Mesh->Bones);
        bool any = false;
        ref.bonePos.reserve(size_t(Mesh->NBones) * 3);
        for (int i = 0; i < Mesh->NBones; ++i, m += 16) {
            const float *t = m + 12;
            if (!std::isfinite(t[0]) || !std::isfinite(t[1]) || !std::isfinite(t[2])) {
                // sentinel far outside any character: never position-matched
                ref.bonePos.insert(ref.bonePos.end(), { 1e9f, 1e9f, 1e9f });
                continue;
            }
            ref.bonePos.insert(ref.bonePos.end(), { t[0], t[1], t[2] });
            if (!any) {
                for (int k = 0; k < 3; ++k)
                    ref.bonesMin[k] = ref.bonesMax[k] = t[k];
                any = true;
                continue;
            }
            for (int k = 0; k < 3; ++k) {
                ref.bonesMin[k] = std::min(ref.bonesMin[k], t[k]);
                ref.bonesMax[k] = std::max(ref.bonesMax[k], t[k]);
            }
        }
        ref.haveBones = any && (ref.bonesMax[1] - ref.bonesMin[1]) > 1e-4f;
    }
    if (std::isfinite(Mesh->SphereRadius) && Mesh->SphereRadius > 1e-4f
        && std::isfinite(Mesh->field_20[0]) && std::isfinite(Mesh->field_20[1])
        && std::isfinite(Mesh->field_20[2]))
    {
        ref.sphereCenter[0] = Mesh->field_20[0];
        ref.sphereCenter[1] = Mesh->field_20[1];
        ref.sphereCenter[2] = Mesh->field_20[2];
        ref.sphereRadius    = Mesh->SphereRadius;
        ref.haveSphere      = true;
    }

    return modmesh::buildSectionsForMesh(
        *scene, Mesh->Name != nullptr ? Mesh->Name->to_string() : "",
        views, ref);
}

// ---------------------------------------------------------------------------
// FBX animation clips -> the mod.h carriers (modAnimClip).
//
// modmesh::loadScene() caches by path, so this re-uses the Scene already
// parsed for the mesh replacement. Call it AFTER the mesh has been through
// modBuildSectionsForMesh at least once: that is where every
// AnimChannel::skelIndex gets resolved against the TARGET mesh's bone array
// (bind-pose cluster match first, Bone_N name digits as fallback). Channels
// that stay at skelIndex -1 animate a node this mesh has no bone for and
// must not be played.
//
// Conventions of the converted clips:
//   - key times and durations in SECONDS (ticksPerSecond = 1.0),
//   - positions in game units (the importer's sceneScale bake),
//   - rotations as x y z w quaternions with the FBX RotationOrder and
//     Pre/PostRotation already folded in - each channel IS the bone's local
//     transform in parent space, composed T*R*S
//     (modmesh::channelLocalMatrix() does that composition if preferred).
// ---------------------------------------------------------------------------
[[maybe_unused]] static void modCollectFbxAnimations(Mod *mod,
                                                     std::vector<modAnimClip> &out)
{
    auto scene = modmesh::loadScene(mod->Path.string(),
                                    mod->Data.data(), mod->Data.size());
    if (!scene || scene->anims.empty())
        return;

    out.reserve(out.size() + scene->anims.size());
    for (const auto &clip : scene->anims) {
        modAnimClip mc;
        mc.name           = clip.name;
        mc.duration       = clip.duration;
        mc.ticksPerSecond = 1.0;                 // times are seconds
        mc.channels.reserve(clip.channels.size());
        for (const auto &ch : clip.channels) {
            modBoneChannel bc;
            bc.boneName  = ch.boneName;
            bc.skelIndex = ch.skelIndex;
            bc.positions.reserve(ch.pos.size());
            for (const auto &k : ch.pos)
                bc.positions.push_back({ k.t, k.v[0], k.v[1], k.v[2] });
            bc.rotations.reserve(ch.rot.size());
            for (const auto &k : ch.rot)
                bc.rotations.push_back({ k.t, k.q[0], k.q[1], k.q[2], k.q[3] });
            bc.scales.reserve(ch.scl.size());
            for (const auto &k : ch.scl)
                bc.scales.push_back({ k.t, k.v[0], k.v[1], k.v[2] });
            mc.channels.push_back(std::move(bc));
        }
        out.push_back(std::move(mc));
    }
}
#endif // MOD_MESH_SUPPORT


// --------------------------------------------------------------------------
// Raw .PCMESH overrides ("mods/VENOM.PCMESH").
//
// A drop-in mesh file is the same "PCM " 0x601 binary nglLoadMeshFileInternal
// parses below, so the substitution point is the file buffer itself: rebind
// MeshFile->FileBuf before the header is read and let the parser consume the
// replacement exactly like the per-pack blob the retail loader feeds it.
//
// The parse rebases the buffer IN PLACE (PTR_OFFSET turns stored offsets into
// live pointers, the usperson/us_character path even rewrites vertex floats
// to packed ints), and the nglMesh/nglMaterialBase/nglMeshSection nodes keep
// living inside the buffer for as long as the file is loaded. Binding the
// Mods[] master bytes directly would therefore 1) corrupt the master on the
// first parse and 2) alias every nglMeshFile that shares the name onto one
// buffer. Instead each nglMeshFile gets a private copy, refreshed whenever
// the previous copy has already been parsed (Header->field_10 != 0) - which
// mirrors the retail blob lifetime: freed and re-read from disk per load.
// Stale slots of destroyed nglMeshFiles are reclaimed on address reuse.
// --------------------------------------------------------------------------
#if MOD_MESH_SUPPORT

bool modBindRawPCMesh(nglMeshFile *MeshFile, const char *ext)
{
    Mod *mod = getMod(MeshFile->FileName.m_hash, TLRESOURCE_TYPE_MESH_FILE);
    if (mod == nullptr) {
        return false;
    }

    constexpr uint32_t version = 0x601;

    // Validate the master copy before touching FileBuf so a malformed drop-in
    // falls back to the vanilla buffer instead of failing the whole load.
    // field_10 must be 0: a from-disk file holds offsets, not live pointers.
    auto *Header = bit_cast<const nglMeshFileHeader *>(mod->Data.data());
    if (mod->Data.size() < sizeof(nglMeshFileHeader) ||
        strncmp(Header->Tag, "PCM ", 4u) != 0 ||
        Header->Version != version ||
        Header->NDirectoryEntries == 0 ||
        Header->field_10 != 0)
    {
        sp_log("[mod] \"%s%s\": rejecting replacement \"%s\" (not a from-disk PCM %x mesh file), keeping the original.",
               MeshFile->FileName.to_string(),
               ext,
               mod->Path.filename().string().c_str(),
               version);
        return false;
    }

    struct raw_copy {
        const Mod *source;
        std::vector<char> bytes;
    };

    static std::unordered_map<nglMeshFile *, raw_copy> s_copies;

    auto &copy = s_copies[MeshFile];

    const bool parsed = !copy.bytes.empty() &&
        bit_cast<const nglMeshFileHeader *>(copy.bytes.data())->field_10 != 0;

    if (copy.bytes.empty() || parsed || copy.source != mod) {
        copy.source = mod;
        copy.bytes.assign(mod->Data.begin(), mod->Data.end());
    }

    MeshFile->FileBuf.Buf = copy.bytes.data();
    MeshFile->FileBuf.Size = uint32_t(copy.bytes.size());

    sp_log("[mod] meshfile \"%s%s\" <- \"%s\" (%u bytes, %u directory entries)",
           MeshFile->FileName.to_string(),
           ext,
           mod->Path.filename().string().c_str(),
           uint32_t(copy.bytes.size()),
           Header->NDirectoryEntries);

    return true;
}

#endif


static bool nglLoadMeshFileInternalPC(const tlFixedString &FileName,
                                      nglMeshFile *MeshFile,
                                      const char *ext)
{
    TRACE("nglLoadMeshFileInternal", FileName.to_string());

    if constexpr (1)
    {
#       if MOD_MESH_SUPPORT
            // Two separate replacement routes:
            //   raw    - "VENOM.PCMESH" drop-in in native format: rebind FileBuf
            //            here and let the parser below consume it untouched;
            //   import - "VENOM.FBX"/.OBJ: parsed by the native importer
            //            (mod_mesh_import.h) and applied per section further
            //            down, the original buffer stays bound.
            // Raw wins when both exist for one name; replacementMesh must stay
            // null for raw mods so the import path is never fed PCM bytes.
            Mod* replacementMesh = nullptr;
            if (!modBindRawPCMesh(MeshFile, ext))
                replacementMesh = getMod(MeshFile->FileName.m_hash, TLRESOURCE_TYPE_MESH);
            // A mesh file load IS the reload event: the materials this pass is
            // about to resolve are new objects on addresses the previous
            // load's materials were freed from, and any texture cached for the
            // previous load may or may not have survived. Opening a new epoch
            // makes every cross-load reuse conditional on the engine
            // confirming the pointer, instead of assumed.
            if (replacementMesh != nullptr)
                modTexCacheNewEpoch(MeshFile->FileName.to_string());
            // discovery aid for hash-only pack entries (prerelease/beta
            // packs): log every mesh file identity once, so the exact name
            // OR literal hash to give a mods/*.fbx is in the log
            if (replacementMesh == nullptr) {
                static std::unordered_set<uint32_t> seenMeshFiles;
                if (seenMeshFiles.insert(MeshFile->FileName.m_hash).second)
                    sp_log("[modmesh] mesh file \"%s\" hash 0x%08X - replace "
                           "with mods/%s.obj|.fbx or mods/0x%08X.obj|.fbx\n",
                           MeshFile->FileName.to_string(),
                           MeshFile->FileName.m_hash,
                           MeshFile->FileName.to_string(),
                           MeshFile->FileName.m_hash);
            }
#       endif

        nglMeshFileHeader *Header = CAST(Header, MeshFile->FileBuf.Buf);

        MeshFile->field_134 = (int) Header;
        MeshFile->field_144 = -1;
        if (strncmp(Header->Tag, "PCM ", 4u) != 0)
        {
            sp_log("Corrupted mesh file: %s%s%s.\n", nglMeshPath(), FileName.to_string(), ext);

            return false;
        }

        constexpr auto version = 0x601;

        if (Header->Version != version)
        {
            auto *v6 = FileName.to_string();
            sp_log("Unsupported mesh file version: %s%s%s (version %x, current version is %x).\n",
                   nglMeshPath(),
                   v6,
                   ext,
                   Header->Version,
                   version);

            return false;
        }

        if (Header->NDirectoryEntries == 0)
        {
            auto *v7 = FileName.to_string();
            sp_log("Mesh file hasn't any directory entries: %s%s%s.\n", nglMeshPath(), v7, ext);

            return false;
        }

        {
            auto *dir_entries = Header->DirectoryEntries;
            sp_log("0x%08X", dir_entries);
        }

        const auto Base = bit_cast<uint32_t>(&MeshFile->FileBuf.Buf[-Header->field_10]);

        nglRebaseHeader(Base, Header);

        assert(Base == int(Header));
        sp_log("Base = 0x%08X", Base);

        MeshFile->FirstMesh = nullptr;
        MeshFile->FirstMaterial = nullptr;
        MeshFile->FirstMorph = nullptr;

        uint32_t num_dir_entries = Header->NDirectoryEntries;
        //sp_log("num_dir_entries = %d", num_dir_entries);

        nglMesh *LastMesh = nullptr;
        nglMaterialBase *LastMaterial = nullptr;
        nglMorphSet *prevMorph = nullptr;

        auto *dir_entries = Header->DirectoryEntries;
        //sp_log("0x%08X", dir_entries);

        std::for_each(dir_entries, dir_entries + num_dir_entries,
                [&](auto &dir_entry)
        {
            PTR_OFFSET(Base, dir_entry.field_4.Material);
            PTR_OFFSET(Base, dir_entry.field_8);

            auto dir_entry_type = dir_entry.field_3;
            //sp_log("dir_entry_type = %s", to_string(dir_entry_type));

            switch (dir_entry_type) {
            case TypeDirectoryEntry::MATERIAL: {

                nglMaterialBase *Material = dir_entry.field_4.Material;

                PTR_OFFSET(Base, Material->Name);
                //sp_log("material_name = %s", Material->Name->to_string());

                PTR_OFFSET(Base, Material->m_shader);

                Material->File = MeshFile;
                if (MeshFile->FirstMaterial == nullptr) {
                    MeshFile->FirstMaterial = Material;
                }

                if (LastMaterial != nullptr) {
                    LastMaterial->NextMaterial = Material;
                }

                LastMaterial = Material;
                if (Header->field_10 == 0)
                {
                    auto *v17 = bit_cast<tlFixedString *>(Material->m_shader);
                    tlHashString a2 = v17->m_hash;
                    //sp_log("0x%08X", v17->m_hash);

                    auto *v18 = nglShaderBank.Search(a2);
                    if (v18 != nullptr)
                    {
                        auto *shader = static_cast<nglShader *>(v18->field_20);

                        if (shader->CheckMaterialVersion(Material)) {
                            Material->m_shader = shader;
                        } else {
                            auto *v27 = a2.c_str();
                            auto v26 = Material->Version;
                            auto *v8 = Material->Name->to_string();
                            sp_log(
                                "Material %s binary version (%d) is not compatible with shader "
                                "%s.\n",
                                v8,
                                v26,
                                v27);
                            Material->m_shader = &gEmptyShader();
                        }

                    } else {
                        auto *v28 = Material->Name->to_string();
                        auto *v9 = a2.c_str();
                        sp_log("NGL: Unable to find shader %s, used by material %s.\n", v9, v28);

                        Material->m_shader = &gEmptyShader();
                    }
                }

                Material->m_shader->RebaseMaterial(Material, Base);

                if (0 ) //v17->m_hash == 0xFC097C8A)
                {
                    struct {
                        char field_0[0x60];
                        tlFixedString *field_60;
                    } *mat = CAST(mat, Material);
                    sp_log("%s", mat->field_60->to_string());
                }


                Material->m_shader->BindMaterial(Material);

            } break;
            case TypeDirectoryEntry::MESH: {

                nglMesh *Mesh = dir_entry.field_4.Mesh;
                PTR_OFFSET(Base, Mesh->Name);

                void (__fastcall *Add)(void *, void *edx, nglMesh *) = CAST(Add, get_vfunc(nglMeshDirectory()->m_vtbl, 0x10));
                Add(nglMeshDirectory(), nullptr, Mesh);

                Mesh->File = MeshFile;
                if (MeshFile->FirstMesh == nullptr) {
                    MeshFile->FirstMesh = Mesh;
                }

                if (LastMesh != nullptr) {
                    LastMesh->NextMesh = Mesh;
                }

                LastMesh = Mesh;
                if ((Mesh->Flags & NGLMESH_PROCESSED) == 0) {
                    nglRebaseMesh(Base, 0, Mesh);
                }

#               if MOD_MESH_SUPPORT
#                   if MOD_MESH_DBG_REPLACE_ALL
                        if (!replacementMesh && dbgReplaceMesh)
                            replacementMesh = dbgReplaceMesh;
#                   endif
                    // One importer pass per nglMesh; sections apply below.
                    // Never on a mesh that already went through the load tail:
                    // its section vertex streams have been packed float->int by
                    // the us_character path, and re-importing from them feeds
                    // the weight transfer garbage donor weights.
                    std::vector<std::optional<modmesh::BuiltSection>> builtSections;
                    bool anySectionReplaced = false;
                    if (replacementMesh && (Mesh->Flags & NGLMESH_PROCESSED) == 0)
                        builtSections = modBuildSectionsForMesh(replacementMesh, Mesh);
#               endif


                for (auto idx_Section = 0u; idx_Section < Mesh->NSections; ++idx_Section)
                {
                    Mesh->Sections[idx_Section].field_0 = 1;

                    nglMeshSection *MeshSection = Mesh->Sections[idx_Section].Section;
                    PTR_OFFSET(Base, MeshSection->MaterialName);

                    MeshSection->Material = nglGetMaterialInFile(*MeshSection->MaterialName, MeshFile);
                    if (!MeshSection->Material->m_shader->CheckVertexDefVersion(MeshSection))
                    {
                        tlFixedString v111 = MeshSection->Material->m_shader->GetName();

                        auto *v12 = v111.to_string();
                        sp_log(
                            "Section VertexDef Binary version (%d) is incompatible with "
                            "shader %s\n.",
                            MeshSection->field_50,
                            v12);
                        MeshSection->Material->m_shader = &gEmptyShader();
                    }

#                   if MOD_MESH_SUPPORT
                        // Apply the imported replacement (if any) BEFORE the
                        // vanilla index-buffer creation. Replaced sections
                        // then flow through the FULL vanilla tail: the
                        // VertexDef bank init (the actor pipeline clones
                        // character work meshes from the section VertexDef,
                        // and the retail teardown destroys it - both need a
                        // real initialized def) and BindSection. Only the
                        // vanilla index-buffer creation and the float->int
                        // packing lambda are bypassed, because the applier's
                        // buffers and float-blend-index layout are final.
                        const bool sectionReplaced =
                            idx_Section < builtSections.size()
                            && builtSections[idx_Section]
                            // an exact-round-trip piece keeps the VANILLA
                            // buffers on purpose: retail morph playback (the
                            // eddie reveal, visemes, facial animation) stays
                            // live; only the texture retarget below runs
                            && !builtSections[idx_Section]->keepGeometry
                            && modApplyBuiltSection(MeshSection,
                                                    *builtSections[idx_Section],
                                                    int(Mesh->NBones));
                        anySectionReplaced |= sectionReplaced;
#                   endif

                    auto *v27 = MeshSection->m_indices;
                    if (v27 != nullptr
#                       if MOD_MESH_SUPPORT
                            // a replaced section already owns its (possibly
                            // 32-bit) index buffer
                            && !sectionReplaced
#                       endif
                        ) {
                        bit_cast<nglVertexBuffer *>(&MeshSection->m_indexBuffer)
                            ->createIndexBufferAndWriteData(v27, 2 * MeshSection->NIndices);
                    }

                    auto *v28 = MeshSection->Material;
                    MeshSection->StartIndex = 0;


                    tlFixedString v112 = v28->m_shader->GetName();
                    auto* v29 = v112.to_string();

#                   if MOD_MESH_SUPPORT
                    if (!sectionReplaced)   // the packing lambda would
                                            // truncate the float blend
                                            // indices and rebuild the
                                            // final buffer over them
#                   endif
                    [&v29](auto *MeshSection) -> void {
                        auto func = [](auto *MeshSection)
                        {
                            auto v31 = (uint32_t) (MeshSection->field_3C.Size >> 6);

                            auto *v32 = (float *) (MeshSection->field_3C.m_vertexData +
                                                   32);
                            MeshSection->field_5C = 2;
                            for (; v31 != 0; --v31)
                            {
                                if (equal(v32[7], 0.0f)) {
                                    if (not_equal(v32[6], 0.0f) && MeshSection->field_5C < 3u) {
                                        MeshSection->field_5C = 3;
                                    }
                                } else {
                                    MeshSection->field_5C = 4;
                                }

                                *(uint32_t *) v32 = v32[0];

                                *((uint32_t *) v32 + 1) = v32[1];

                                *((uint32_t *) v32 + 2) = v32[2];
                                *((uint32_t *) v32 + 3) = v32[3];
                                v32 += 16;
                            }

                            MeshSection->field_3C.createVertexBufferAndWriteData(MeshSection->field_3C.m_vertexData,
                                                                 MeshSection->field_3C.Size,
                                                                 1028);

                            static Var<int> dword_973BC8{0x00973BC8};

                            if (dword_973BC8() < (int) (24 * (MeshSection->field_3C.Size >> 6))) {
                                dword_973BC8() = 24 * (MeshSection->field_3C.Size >> 6);
                            }

                            MeshSection->m_stride = 24;
                        };

                        if (!EnableShader())
                        {
                            sp_log("debug0");
                            if (strncmp(v29, "uslod", 5u) == 0)
                            {
                                sp_log("debug1");

                                nglVertexBuffer::createIndexOrVertexBuffer(
                                    &MeshSection->field_3C,
                                    ResourceType::VertexBuffer,
                                    16 * (MeshSection->field_3C.Size / 12),
                                    520,
                                    0,
                                    D3DPOOL_DEFAULT);
                                MeshSection->m_stride = 16;
                                MeshSection->field_5C = 0;
                                return;
                            }

                            if (!EnableShader())
                            {
                                if (ChromeEffect())
                                {
                                    if (strncmp(v29, "smshiny", 7u) == 0)
                                    {
                                        int v30 = 48 * (MeshSection->field_3C.Size / 60u);
                                        MeshSection->field_3C
                                            .createVertexBuffer(v30, 520u);
                                        MeshSection->m_stride = 48;

                                        static Var<int> dword_972960{0x00972960};

                                        if (dword_972960() < v30) {
                                            dword_972960() = v30;
                                        }

                                        return;
                                    }
                                }
                                else
                                {
                                    sp_log("debug2");
                                    if (!EnableShader())
                                    {
                                        if (strncmp(v29, "usperson", 8u) == 0)
                                        {
                                            func(MeshSection);
                                            return;
                                        }
                                    }
                                }
                            }
                        }

                        if (strncmp(v29, "us_character", 12u) == 0)
                        {
                            func(MeshSection);
                            return;
                        }
                        

                        MeshSection->field_3C.createVertexBufferAndWriteData(MeshSection->field_3C.m_vertexData,
                                                             MeshSection->field_3C.Size,
                                                             1028);
                    }(MeshSection);

                    if (auto *v39 = MeshSection->VertexDef; v39 != nullptr) {
                        tlHashString a1 = *(tlHashString *) v39->m_vtbl;
                        auto *v40 = nglVertexDefBank().Search(a1);
                        if (v40 != nullptr) {
                            MeshSection->VertexDef->field_4 = MeshSection;

                            void (*func)(void *) = CAST(func, v40->field_20);
                            func(MeshSection->VertexDef);
                        } else {
                            MeshSection->VertexDef = nullptr;
                        }
                    }

                    if (auto *v41 = MeshSection->Material; v41 != nullptr)
                    {
                        if (auto *v42 = v41->m_shader; v42 != nullptr) {
                            v42->BindSection(MeshSection);
                        }
                    }

#                   if MOD_MESH_SUPPORT
                        if (idx_Section < builtSections.size()
                            && builtSections[idx_Section]
                            && (sectionReplaced
                                // round-trip pieces keep vanilla geometry but
                                // still retarget: a recolor the user dropped
                                // in mods/ (USM_BLACKSUIT.png) must land even
                                // when nothing was swapped. Bytes that only
                                // travelled with the FBX do not - see
                                // userFilesOnly in the retarget
                                || builtSections[idx_Section]->keepGeometry
                                // a sidecar tex<N>= pin carries no geometry at
                                // all: the record exists only to repaint
                                || builtSections[idx_Section]->texExclusive)
                            && replacementMesh != nullptr)
                        {
                            // An EMPTY candidate list is still worth a call:
                            // the wrapper repairs a section whose material has
                            // no diffuse texture (numbered-variant salvage,
                            // then the sibling-donor fallback) and reports the
                            // white-piece diagnostic if even that fails.
                            modRetargetSectionTexture(MeshSection,
                                *builtSections[idx_Section],
                                replacementMesh->Path,
                                int(idx_Section),
                                Mesh);
                        }
#                   endif
                }

#               if MOD_MESH_SUPPORT
                // Every section of this mesh is in place now: refit the mesh
                // sphere before the engine aggregates the file-wide one below,
                // so culling, LOD and the chase camera all see the geometry
                // that is actually being drawn.
                if (anySectionReplaced)
                    modRecomputeMeshBounds(Mesh);
#               endif

            } break;
            case TypeDirectoryEntry::MORPH: {
                nglMorphSet *new_morph = CAST(new_morph, dir_entry.field_4);
                nglProcessMorph(MeshFile, &dir_entry, Base);
                if (prevMorph != nullptr) {
                    prevMorph->field_10 = new_morph;
                }

                prevMorph = new_morph;
            } break;
            default: {
                auto *v14 = FileName.to_string();

                sp_log(
                    "nglLoadMeshFile: file \"%s%s%s\" has an unknown directory entry ( %u ), "
                    "skipping.\n",
                    nglMeshPath(),
                    v14,
                    ext,
                    uint32_t(dir_entry_type));

                break;
            }
            }
        });

        if (LastMesh != nullptr) {
            LastMesh->NextMesh = nullptr;
        }

        if (LastMaterial != nullptr) {
            LastMaterial->NextMaterial = nullptr;
        }

        vector4d a3a;
        a3a[0] = 1.0e32;
        a3a[1] = 1.0e32;
        a3a[2] = 1.0e32;

        vector4d v103;
        v103[0] = -1.0e32;
        v103[1] = -1.0e32;
        v103[2] = -1.0e32;
        v103[3] = -a3a[3];

        bool v46 = false;

        for (auto *Mesh = MeshFile->FirstMesh; Mesh != nullptr; Mesh = Mesh->NextMesh)
        {
            if ((Mesh->Flags & NGLMESH_PROCESSED) == 0)
            {
                if (Mesh->NBones != 0)
                {
                    for (int i = 0; i < Mesh->NBones; ++i) {
                        Mesh->Bones[i] = sub_4150E0(Mesh->Bones[i]);
                    }

                    auto v89 = Mesh->field_20[0];
                    auto v90 = Mesh->field_20[1];
                    auto v91 = Mesh->field_20[2];
                    auto v93 = Mesh->field_20[3];
                    auto v73 = Mesh->SphereRadius;

                    vector4d v96;
                    v96[0] = v89 - v73;
                    v96[1] = v90 - v73;
                    v96[2] = v91 - v73;
                    v96[3] = v93 - v73;

                    a3a = sub_401270(v96, a3a);

                    vector4d v110;
                    v110[0] = v89 + v73;
                    v110[1] = v90 + v73;
                    v110[2] = v91 + v73;
                    v110[3] = v93 + v73;

                    v103 = sub_4012F0(v110, v103);

                    v46 = true;
                }
                else
                {
                    Mesh->Flags |= NGLMESH_PROCESSED;
                }

                auto *Lods = Mesh->LODs;
                for (int i = 0; i < Mesh->NLODs; ++i)
                {
                    Mesh->LODs[i].field_0 = nglGetMeshInFile(*bit_cast<const tlFixedString *>(
                                                                 Lods[i].field_0),
                                                             MeshFile);
                    Lods = Mesh->LODs;
                    if (Lods[i].field_0 == nullptr) {
                        --i;
                        --Mesh->NLODs;
                    }
                }
            }
        }

        if (v46)
        {
            auto v60 = sub_411750(a3a, v103);

            vector4d v96;
            v96[0] = v60[0] * 0.5f;
            v96[1] = v60[1] * 0.5f;
            v96[2] = v60[2] * 0.5f;
            v96[3] = v60[3] * 0.5f;

            auto v69 = 0.0f;
    
            auto *v67 = MeshFile->FirstMesh;
            for (; v67 != nullptr; v67 = v67->NextMesh)
            {
                if ((v67->Flags & NGLMESH_PROCESSED) == 0)
                {
                    a3a[0] = v96[0] - v67->field_20[0];
                    a3a[1] = v96[1] - v67->field_20[1];
                    a3a[2] = v96[2] - v67->field_20[2];
                    a3a[3] = v96[3] - v67->field_20[3];
                    auto v76 = vector3d {a3a[0], a3a[1], a3a[2]}.length() + v67->SphereRadius;
                    if (v69 <= v76) {
                        v69 = v76;
                    }
                }
            }

            // NOTE: this used to start from `v67`, which the loop above has
            // just walked to nullptr - so the body never ran and boned meshes
            // NEVER got NGLMESH_PROCESSED. A second parse of the same buffer
            // then re-ran nglRebaseMesh over already-live pointers AND inverted
            // Mesh->Bones a second time, which is the "melted / exploding
            // character on load" case. Restart from the list head like retail.
            for (auto *Mesh = MeshFile->FirstMesh; Mesh != nullptr; Mesh = Mesh->NextMesh)
            {
                if ((Mesh->Flags & NGLMESH_PROCESSED) == 0)
                {
                    Mesh->SphereRadius = v69;
                    Mesh->field_20[0] = v96[0];
                    Mesh->field_20[1] = v96[1];
                    Mesh->field_20[2] = v96[2];
                    Mesh->field_20[3] = v96[3];
                    Mesh->Flags |= NGLMESH_PROCESSED;
                }
            }
        }

        if constexpr (0)
        {
            if (std::string {"ultimate_spiderman"} == FileName.to_string()) {
                assert(0);
            }
        }

        Header->field_10 = (int) MeshFile->FileBuf.Buf;
        return true;
    }
    else
    {
        bool (*func)(const tlFixedString &, nglMeshFile *, const char *) = CAST(func, 0x0076F500);
        auto result = func(FileName, MeshFile, ext);
    }
    return true;

}

bool nglLoadMeshFileInternal(const tlFixedString &FileName,
                             nglMeshFile *MeshFile,
                             const char *ext)
{
#ifdef OPENUSM_XBPACK_MODE
    if (MeshFile != nullptr && MeshFile->FileBuf.Buf != nullptr &&
        std::memcmp(MeshFile->FileBuf.Buf, "XBXM", 4) == 0) {
        return nglLoadMeshFileInternalXbox(FileName, MeshFile, ext);
    }
#endif

    return nglLoadMeshFileInternalPC(FileName, MeshFile, ext);
}
#endif

bool nglCanReleaseMeshFile(nglMeshFile *a1) {
    return a1->field_144 + 1 < nglFrame();
}

void nglMorphFile::un_mash_start(generic_mash_header *header,
                                 void *,
                                 generic_mash_data_ptrs *a3,
                                 void *)
{
    auto v5 = 8 - ((int) a3->field_0 % 8u);
    if (v5 < 8) {
        a3->field_0 += v5;
    }

    assert(((int) header) % 4 == 0);
}

bool nglCanReleaseMorphFile(nglMorphFile *a1) {
    static Var<int> nglFrame{0x00972904};

    return a1->field_144 + 1 < nglFrame();
}

nglMesh *nglGetMeshInFile(const tlFixedString &a1, nglMeshFile *a2)
{
    TRACE("nglGetMeshInFile", a1.to_string());

    if constexpr (1)
    {
        for (auto *result = a2->FirstMesh; result != nullptr; result = result->NextMesh)
        {
            auto *name = result->Name;
            sp_log("%s", name->to_string());
            if (*result->Name == a1) {
                return result;
            }
        }

        return nglGetMesh(a1, true);
    } else {
        return (nglMesh *) CDECL_CALL(0x0076F0A0, &a1, a2);
    }
}

nglMesh *nglGetMesh(const tlHashString &a1, bool a2);

nglTexture *nglGetTexture(uint32_t a1)
{
    struct Vtbl {
        int empty[2];
        nglTexture *(__fastcall *Find)(void *, void *, uint32_t);
    };

    void *address = get_vtbl(nglTextureDirectory());

    Vtbl *vtbl = CAST(vtbl, address);

    return vtbl->Find(nglTextureDirectory(), nullptr, a1);
}

nglTexture *nglGetTexture(const tlFixedString &a1)
{
    TRACE("nglGetTexture", a1.to_string());

    sp_log("0x%08x", nglTextureDirectory()->m_vtbl);

    nglTexture * (__fastcall *Find)(void *, void *, uint32_t) = CAST(Find, get_vfunc(nglTextureDirectory()->m_vtbl, 0x8));
    return Find(nglTextureDirectory(), nullptr, a1.m_hash);
}

static constexpr auto NGLFONT_TOKEN_COLOR = '\1';
static constexpr auto NGLFONT_TOKEN_SCALE = '\2';
static constexpr auto NGLFONT_TOKEN_SCALEXY = '\3';

uint32_t RGBA2ARGB(uint32_t c) {
    return (((c >> 8) & 0xFFFFFF) | ((c & 0xFF) << 24));
}

//TODO
void nglGetStringDimensions(
    nglFont *Font,
    char *Text,
    uint32_t *Width,
    uint32_t *Height,
    Float a5,
    Float a6)
{
    TRACE("nglGetStringDimensions", Text);

    if constexpr (0)
    {
        float CurMaxScaleY = a6;
        auto *TextPtr = Text;
        char v7 = '\0';
        float CurMaxWidth = 0.0;
        float fWidth = 0.0;
        float fHeight = 0.0;
        for (char c = *TextPtr; c != '\0'; ++TextPtr) {
            switch (c) {
            case NGLFONT_TOKEN_COLOR:
                if constexpr (0) {
                    Text = TextPtr + 1;
                    strtoul(TextPtr + 1, (char **) &Text, 16);
                    TextPtr = ++Text;
                } else {
                    static auto sub_FDBBE0 = [](char *&Text, uint32_t &color)
                    {
                        assert( *Text != '[' && "Invalid character found in Token.  Should be '['.\n" );

                        color = strtoul(Text + 1, &Text, 16u);
                        ++Text;
                        assert( *Text != ']' && "Invalid character found in Token.  Should be ']'.\n" );

                        ++Text;
                    };

                    uint32_t v18;
                    [](char *&a1, uint32_t &c)
                    {
                          sub_FDBBE0(a1, c);
                          c = RGBA2ARGB(c);
                    }(TextPtr, v18);
                }
                break;
            case NGLFONT_TOKEN_SCALE:

                if constexpr (0) {
                    Text = TextPtr + 1;
                    a5 = strtod(TextPtr + 1, (char **) &Text);
                    a6 = a5;
                    TextPtr = ++Text;
                    if (CurMaxScaleY < a5) {
                        CurMaxScaleY = a5;
                    }
                } else {
                    static auto sub_FDBD50 = [](char *&Text, float &a2)
                    {
                        assert(*Text == '[' && "Invalid character found in Token.  Should be '['.\n" );

                        a2 = strtod(Text + 1, &Text);
                        ++Text;

                        assert(*Text == ']' && "Invalid character found in Token.  Should be ']'.\n" );
                        ++Text;
                    };
                    [](char *&Text, float &ScaleX, float &ScaleY, float &CurMaxScaleY) {
                        sub_FDBD50(Text, ScaleX);
                        ScaleY = ScaleX;
                        if ( ScaleY > CurMaxScaleY ) {
                            CurMaxScaleY = ScaleY;
                        }
                    }(TextPtr, a5.value, a6.value, CurMaxScaleY);
                }

                break;
            case NGLFONT_TOKEN_SCALEXY: {
                if constexpr (0) {
                    Text = TextPtr + 1;
                    a5 = strtod(TextPtr + 1, (char **) &Text);
                    ++Text;
                    a6 = strtod(Text, (char **) &Text);
                    TextPtr = ++Text;
                    if (CurMaxScaleY < a6) {
                        CurMaxScaleY = a6;
                    }
                } else {
                    static auto sub_FDBEA0 = [](char *&Text, float &ScaleX, float &ScaleY)
                    {
                        assert( *Text != '[' && "Invalid character found in Token.  Should be '['.\n" );
                        ScaleX = strtod(Text + 1, &Text);
                        ++Text;

                        assert( *Text != ',' && "Invalid character found in Token.  Should be ','.\n" );
                        ScaleY = strtod(Text + 1, &Text);
                        ++Text;

                        assert( *Text != ']' && "Invalid character found in Token.  Should be ']'.\n" );
                        ++Text;
                    };

                    [](char *&Text, float &ScaleX, float &ScaleY, float &CurMaxScaleY)
                    {
                        sub_FDBEA0(Text, ScaleX, ScaleY);
                        if ( ScaleY > CurMaxScaleY ) {
                            CurMaxScaleY = ScaleY;
                        }
                    }(TextPtr, a5.value, a6.value, CurMaxScaleY);
                }
            } break;
            case '\t': {
                int CellWidth = Font->GlyphInfo[' ' - Font->Header.FirstGlyph].CellWidth;
                double v22 = (CellWidth < 0
                                ? CellWidth + 4.2949673e9
                                : CellWidth
                                );

                v7 = ' ';
                fWidth += v22 * a5 * 4.0f;
                break;
            }
            case '\n': {
                if (v7 != '\0') {
                    auto v10 = Font->Header.FirstGlyph;
                    auto v11 = v7;
                    int v12;
                    if (v7 < v10 || (v12 = v7, v7 >= v10 + Font->Header.NumGlyphs)) {
                        v12 = 32;
                    }

                    auto *v13 = Font->GlyphInfo;
                    auto *v14 = &v13[v12 - v10];
                    int v15;
                    if (v11 < v10 || (v15 = v11, v11 >= v10 + Font->Header.NumGlyphs)) {
                        v15 = 32;
                    }

                    auto *v16 = &v13[v15 - v10];
                    if (v11 < v10 || v11 >= v10 + Font->Header.NumGlyphs) {
                        v11 = 32;
                    }

                    auto v17 = v11 - v10;
                    auto v18 = v14->GlyphSize[0];
                    auto v19 = v16->GlyphOrigin[0];
                    TextPtr = Text;
                    fWidth += (v19 + v18 - v13[v17].CellWidth) * a5;
                }

                if ( fWidth > CurMaxWidth ) {
                    CurMaxWidth = fWidth;
                }

                fWidth = 0.0;
                v7 = '\0';
                fHeight += Font->Header.CellHeight * CurMaxScaleY;
                CurMaxScaleY= a6;
                break;
            };
            default: {
                int v27 = Font->GetFontCellWidth(c);
                fWidth += v27 * a5;
                v7 = c;
                break;
            }
            }

        }

        if (v7) {
            auto v9 = Font->GetGlyphInfo(v7);
            auto v10 = Font->GetGlyphInfo(v7)->GlyphOrigin[0] + v9->GlyphSize[0];
            auto v11 = Font->GetFontCellWidth(v7);
            fWidth += (v10 - v11) * a5;
        }

        if (Width != nullptr) {
            *Width = (fWidth <= CurMaxWidth ? CurMaxWidth : fWidth);
        }

        if (Height != nullptr) {
            *Height = Font->Header.CellHeight * CurMaxScaleY + fHeight;
        }

    } else {
        CDECL_CALL(0x007798E0, Font, Text, Width, Height, a5, a6);
    }
}

void nglGetStringDimensions(
    nglFont *Font, unsigned int *arg4, unsigned int *a3, const char *Format, ...)
{
    static Var<char[1024]> nglFontBuffer {0x00974E08};
    va_list va;

    va_start(va, Format);
    vsprintf(nglFontBuffer(), Format, va);
    nglGetStringDimensions(Font, nglFontBuffer(), arg4, a3, 1.0, 1.0);
}

nglMesh *nglCreateMeshClone(nglMesh *a1)
{
    if (a1 == nullptr) {
        return nullptr;
    }

    auto *mem = static_cast<nglMesh *>(tlMemAlloc(0x40, 8, 0x1000000u));
    auto *newMesh = new (mem) nglMesh {};
    newMesh->Flags = a1->Flags;
    newMesh->NSections = a1->NSections;
    newMesh->Sections = static_cast<decltype(newMesh->Sections)>(
        tlMemAlloc(8 * newMesh->NSections, 8, 0x1000000u));

    if (newMesh->NSections != 0) {
        for (auto i = 0u; i < newMesh->NSections; ++i) {
            newMesh->Sections[i].field_0 = 0;
            newMesh->Sections[i].Section = a1->Sections[i].Section;
        }
    }

    newMesh->NBones = a1->NBones;
    if (newMesh->NBones != 0) {
        newMesh->Bones = static_cast<decltype(newMesh->Bones)>(
            tlMemAlloc(newMesh->NBones << 6, 64, 0x1000000u));
        // NBones << 6 is the size in BYTES (one bone matrix is 64 bytes).
        // std::copy walks ELEMENTS, so the old
        //     std::copy(a1->Bones, a1->Bones + (NBones << 6), ...)
        // read and wrote NBones * 64 matrices - a 64x overrun of both the
        // source and the freshly allocated destination. Every character actor
        // goes through this clone, so it smashed the heap right behind the
        // bone array (palettes, section headers) on every hero/NPC spawn.
        std::copy_n(a1->Bones, newMesh->NBones, newMesh->Bones);
    } else {
        newMesh->Bones = nullptr;
    }

    newMesh->NLODs = a1->NLODs;
    if (newMesh->NLODs) {
        newMesh->LODs = static_cast<decltype(newMesh->LODs)>(
            tlMemAlloc(8 * newMesh->NLODs, 8, 0x1000000u));
        memcpy(newMesh->LODs, a1->LODs, 8 * newMesh->NLODs);
    } else {
        newMesh->LODs = nullptr;
    }

    newMesh->field_20 = a1->field_20;
    newMesh->File = nullptr;
    newMesh->NextMesh = nullptr;
    newMesh->SphereRadius= a1->SphereRadius;
    newMesh->field_3C = a1->field_3C;
    return newMesh;
}

void nglMakeSectionUnique(nglMesh *a1, int a2) {
    auto *v2 = a1->Sections;
    char v3 = v2[a2].field_0;
    auto *v4 = &v2[a2];
    if ((v3 & 1) == 0) {
        a1->Sections[a2].Section = nglCreateSectionCopy(v4->Section);
        a1->Sections[a2].field_0 |= 1u;
    }
}

nglMeshSection *nglCreateSectionCopy(nglMeshSection *a1) {
    return (nglMeshSection *) CDECL_CALL(0x00771F90, a1);
}

void mNglQuad::unmash(mash_info_struct *a2, void *a3)
{
    this->custom_unmash(a2, a3);
}

void mNglQuad::custom_unmash(mash_info_struct *a2, void *a3)
{
    TRACE("mNglQuad::custom_unmash");
    mString *v5 = nullptr;

#if OPENUSM_XBOX_MASH_FORMAT && !defined(OPENUSM_XBPACK_V10)
    struct {
        int m_size;
        char *guts;
        int field_8;
    } *temp = CAST(temp, a2->read_from_buffer(mash::NORMAL_BUFFER, 0xC, 4));
    VALIDATE_SIZE(decltype(*temp), 0xC);

    struct {
        int field_0;
        int m_size;
        char *guts;
        int field_8;
    } v1;
    std::memcpy(&v1.m_size, temp, sizeof(*temp));

    v5 = CAST(v5, &v1);

    v5->unmash(a2, a3);

#else
    a2->unmash_class(v5, a3);
#endif

    if ( v5->m_size > 0 )
    {
        tlFixedString a1 {v5->c_str()};
        this->m_tex = nglGetTexture(a1);
    }
}

void nglSetQuadRect(nglQuad *a1, Float a2, Float a3, Float a4, Float a5)
{
    a1->field_0[0].pos.x = a2;
    a1->field_0[1].pos.x = a4;
    a1->field_0[0].pos.y = a3;
    a1->field_0[1].pos.y = a3;
    a1->field_0[2].pos.x = a2;
    a1->field_0[2].pos.y = a5;
    a1->field_0[3].pos.x = a4;
    a1->field_0[3].pos.y = a5;
}

void nglSetQuadColor(nglQuad *a1, unsigned int a2)
{
    a1->field_0[0].m_color = a2;
    a1->field_0[1].m_color = a2;
    a1->field_0[2].m_color = a2;
    a1->field_0[3].m_color = a2;
}

void nglSetQuadTex(nglQuad *a1, nglTexture *a2) {
    a1->m_tex = a2;
}

void nglSetQuadBlend(nglQuad *a1, nglBlendModeType a2, unsigned a3) {
    a1->field_58 = a2;
    a1->field_5C = a3;
}

void nglSetQuadUV(nglQuad *a1, Float a2, int a3, Float a4, Float a5)
{
    a1->field_0[0].uv.field_0 = a2;
    a1->field_0[1].uv.field_0 = a4;
    a1->field_0[0].uv.field_4 = a3;
    a1->field_0[1].uv.field_4 = a3;
    a1->field_0[2].uv.field_0 = a2;
    a1->field_0[2].uv.field_4 = a5;
    a1->field_0[3].uv.field_0 = a4;
    a1->field_0[3].uv.field_4 = a5;
}

int nglGetLOD(nglMesh *a1, const math::MatClass<4, 3> &a2)
{
    TRACE("nglGetLOD");

    auto v5 = sub_414360(a1->field_20, a2);
    auto v6 = sub_414360(v5, nglCurScene()->WorldToView);
    auto v7 = v6[2];

    for ( auto i = a1->NLODs - 1; i >= 0; --i )
    {
        if ( v7 > a1->LODs[i].field_4 ) {
            return i + 1;
        }
    }


    return 0;
}

nglTexture *nglGetFrontBufferTex() {
    return nglFrontBufferTex();
}

void nglCopySection(nglMesh *DstMesh, int a2, nglMesh *SrcMesh, int a4)
{
    TRACE("nglCopySection");

    if constexpr (1)
    {
        auto *SrcSection = SrcMesh->Sections[a4].Section;
        auto *DstSection = DstMesh->Sections[a2].Section;

#if MOD_MESH_SUPPORT
        // Importer-replaced sections can differ in vertex/index count from
        // what the destination (an actor's buffered work-mesh clone) was
        // created with, and the fixed-size memcpy below would shred them.
        // Mirror the stored replacement onto the destination instead. Once
        // the counts are mirrored, the per-frame call still refreshes the
        // VB contents from storage: this per-frame copy IS the vanilla
        // behaviour, and it is what resets the morph deltas retail facial
        // animation wrote last frame (without it they accumulate).
        // Both ends go through modLiveStorage: a stale entry left behind by a
        // destroyed section whose address got recycled would otherwise mirror
        // one character's geometry and bone palette onto another's section.
        if (ModSectionStorage *src = modLiveStorage(SrcSection);
            src != nullptr && SrcSection != DstSection)
        {
            ModSectionStorage *live = modLiveStorage(DstSection);
            if (live == nullptr || live->mirroredRevision != src->revision) {
                ModSectionStorage next;                 // self-contained copy
                next.vertices    = src->vertices;
                next.idx16       = src->idx16;
                next.idx32       = src->idx32;
                next.nverts      = src->nverts;
                next.nidx        = src->nidx;
                next.paddedVerts = src->paddedVerts;
                next.baseVertex  = src->baseVertex;
                next.origVerts   = live != nullptr
                                 ? live->origVerts
                                 : uint32_t(DstSection->NVertices > 0
                                            ? DstSection->NVertices : 0);
                next.weightClass = src->weightClass;
                next.wide        = src->wide;
                next.strideBytes = src->strideBytes;
                next.rigid       = src->rigid;
                next.layPos      = src->layPos;
                next.layNrm      = src->layNrm;
                next.layUv       = src->layUv;
                next.layCol      = src->layCol;
                // fresh engine-ownable palette: the clone teardown
                // tlMemFree's it, so it must never be shared with the source.
                // A static section has none - and must not be given one.
                if (!src->rigid)
                    next.setPalette(src->palette, src->nbones);
                next.mirroredRevision = src->revision;
                ModSectionStorage &dst = modSectionRegistry[DstSection];
                dst = std::move(next);
                if (!modApplyStorageToSection(DstSection, dst)) {
                    modSectionRegistry.erase(DstSection);
                    return;             // leave the clone as-is this frame
                }
                // The corrected culling/camera sphere must travel with the
                // geometry: the buffered clones are what the renderer draws,
                // and until now they kept whatever sphere they were cloned
                // with - stale vanilla bounds on a replaced body.
                for (int k = 0; k < 4; ++k)
                    DstSection->SphereCenter[k] = SrcSection->SphereCenter[k];
                DstSection->SphereRadius = SrcSection->SphereRadius;
                sp_log("[modmesh] mirrored replaced section onto work mesh "
                       "(%u verts, %u idx)\n", dst.nverts, dst.nidx);
                return;
            }
            ModSectionStorage &dst = *live;
            // counts already mirrored: refresh the vertex payload so morph
            // deltas from the previous frame are wiped, like vanilla does
            if (auto *vb = DstSection->field_3C.m_vertexBuffer;
                vb != nullptr && !dst.vertices.empty())
            {
                void *p = nullptr;
                if (SUCCEEDED(vb->lpVtbl->Lock(vb, 0, 0, &p, 0))) {
                    std::memcpy(p, dst.vertices.data(),
                                dst.vertices.size() * sizeof(float));
                    vb->lpVtbl->Unlock(vb);
                }
            }
            return;
        }
        // The SOURCE is vanilla but the DESTINATION clone holds a replaced
        // section (a mesh-morph pose copy, or a mid-session reload that
        // removed the mod). The vanilla memcpy below would read
        // DstSection->field_3C.Size bytes from a smaller source buffer.
        // Restore the destination from its own storage instead.
        if (ModSectionStorage *dst = modLiveStorage(DstSection))
        {
            if (auto *vb = DstSection->field_3C.m_vertexBuffer;
                vb != nullptr && !dst->vertices.empty())
            {
                void *p = nullptr;
                if (SUCCEEDED(vb->lpVtbl->Lock(vb, 0, 0, &p, 0))) {
                    std::memcpy(p, dst->vertices.data(),
                                dst->vertices.size() * sizeof(float));
                    vb->lpVtbl->Unlock(vb);
                }
            }
            return;
        }
#endif

        assert(SrcSection->field_3C.Size == DstSection->field_3C.Size
                && "Section VB sizes do not match !");

        assert(SrcSection->NIndices == DstSection->NIndices
                && "Section IB sizes do not match !");

        void *SrcVertices = nullptr;
        void *DstVertices = nullptr;

        DstSection->field_3C.m_vertexBuffer->lpVtbl->Lock(DstSection->field_3C.m_vertexBuffer, 0, 0, &DstVertices, 0);
        SrcSection->field_3C.m_vertexBuffer->lpVtbl->Lock(SrcSection->field_3C.m_vertexBuffer, 0, 0, &SrcVertices, 0);

        std::memcpy(DstVertices, SrcVertices, DstSection->field_3C.Size);
        DstSection->field_3C.m_vertexBuffer->lpVtbl->Unlock(DstSection->field_3C.m_vertexBuffer);
        SrcSection->field_3C.m_vertexBuffer->lpVtbl->Unlock(SrcSection->field_3C.m_vertexBuffer);
        if ( DstSection->m_indices != nullptr )
        {
            void *SrcIndices = nullptr;
            void *DstIndices = nullptr;

            DstSection->m_indexBuffer->lpVtbl->Lock(DstSection->m_indexBuffer, 0, 0, &DstIndices, 0);
            SrcSection->m_indexBuffer->lpVtbl->Lock(SrcSection->m_indexBuffer, 0, 0, &SrcIndices, 0);

            assert(SrcIndices != nullptr && DstIndices != nullptr
                    && "About to access NULL pointer.");

            std::memcpy(DstIndices, SrcIndices, 2 * DstSection->NIndices);
            DstSection->m_indexBuffer->lpVtbl->Unlock(DstSection->m_indexBuffer);
            SrcSection->m_indexBuffer->lpVtbl->Unlock(SrcSection->m_indexBuffer);
        }
    }
    else
    {
        CDECL_CALL(0x00771E40, DstMesh, a2, SrcMesh, a4);
    }
}

void nglCopyMesh(nglMesh *a1, nglMesh *a2) {
    for (uint32_t i = 0; i < a1->NSections; ++i) {
        if ((a1->Sections[i].field_0 & 1) != 0) {
            nglCopySection(a1, i, a2, i);
        }
    }
}

int nglReleaseMeshFile(const tlFixedString &a1) {
    return (int) CDECL_CALL(0x0076F2D0, &a1);
}

void nglReleaseAllMeshFiles() {
    CDECL_CALL(0x0076F300);
}

nglMesh *nglGetNextMeshInFile(nglMesh *a1) {
    nglMesh *result = nullptr;
    if (a1 != nullptr) {
        result = a1->NextMesh;
    }

    return result;
}

void *nglMeshScratchAlloc(int Size, int Alignment, int) {
    auto *result = (void *) (~(Alignment - 1) & (nglScratchMeshPos() + Alignment - 1));
    nglScratchMeshPos() = (int) result + Size;
    return result;
}

void *nglMeshMemAlloc(int Size, int Alignment, int a3) {
    return tlMemAlloc(Size, Alignment, a3);
}

void nglReleaseAllTextures() {
#if MOD_TEX_CACHE
    // every pointer the mod texture cache holds is about to be freed
    modTexCacheNewEpoch("nglReleaseAllTextures");
#endif
    auto *vtbl = bit_cast<int(*)[1]>(nglTextureDirectory()->m_vtbl);

    assert((*vtbl)[0] == 0x00560770);

    THISCALL(0x00560770, nglTextureDirectory(), 1, 0, 2);
}

void nglReleaseTexture(nglTexture *Tex) {
    TRACE("nglReleaseTexture");

#if MOD_TEX_CACHE
    // the release may take the last reference: never let a cache hand this
    // pointer out again without the engine confirming it first
    modTexCacheForgetTexture(Tex);
#endif
    CDECL_CALL(0x00773380, Tex);
}

nglTexture *nglLoadTexture(const tlFixedString &a1)
{
    TRACE("nglLoadTexture", a1.to_string());

    assert(a1.GetHash() != 0);

    if constexpr (1)
    {
        struct Vtbl {
            char field_0[0xC];
            nglTexture * (__fastcall *Find)(void *, int edx, const tlFixedString *);
            int field_10[5];
            nglTexture * (__fastcall *Load)(void *, int edx, const tlFixedString *);
        };

        auto *vtbl = bit_cast<Vtbl *>(nglTextureDirectory()->m_vtbl);

        auto Find = vtbl->Find;

        //sp_log("0x%08X", bit_cast<std::intptr_t>(Find));

        nglTexture *tex = Find(nglTextureDirectory(), 0, &a1);
        if (tex == nullptr) {
            return vtbl->Load(nglTextureDirectory(), 0, &a1);
        }

        ++tex->field_8;
        return tex;
    } else {
        return (nglTexture *) CDECL_CALL(0x00773290, &a1);
    }
}

nglTexture *nglLoadTexture(const tlHashString &a1)
{
    TRACE("nglLoadTexture", string_hash {int(a1.GetHash())}.to_string());

    auto v1 = a1.GetHash();

    nglTexture * (__fastcall *Find)(void *, void *, uint32_t) = CAST(Find, get_vfunc(nglTextureDirectory()->m_vtbl, 0x8));

    auto *tex = Find(nglTextureDirectory(), nullptr, v1);
    if (tex == nullptr) {
        nglTexture * (__fastcall *Load)(void *, void *, const tlHashString *) = CAST(Load, get_vfunc(nglTextureDirectory()->m_vtbl, 0x20));
        return Load(nglTextureDirectory(), nullptr, &a1);
    }

    ++tex->field_8;
    return tex;
}

nglFont *create_and_parse_fdf(const tlFixedString &a1, char *a2)
{
    auto *mem = tlMemAlloc(sizeof(nglFont), 8u, 0x1000000u);
    auto *font = new (mem) nglFont {};
    font->field_20 = 1;
    font->field_40 = 2;
    font->m_blend_mode = NGLBM_BLEND;
    font->field_0 = a1;
    font->field_24 = nglLoadTexture(a1);
    nglParseFDF(a2, font);
    return font;
}

bool nglCanReleaseTexture(nglTexture *tex)
{
    TRACE("nglCanReleaseTexture");

    auto *v1 = tex;
    if (tex->m_format == 17) {
        v1 = (nglTexture *) tex->m_num_palettes;
    }

    static Var<int> nglFrame{0x00972904};

    return v1->field_38 + 1 < nglFrame();
}

void sub_783080(nglTexture *Tex, uint8_t **a2, uint8_t *a3, int a4) {
    if constexpr (0) {
        IDirect3DSurface9 *v20;

        D3DLOCKED_RECT rect;
        D3DSURFACE_DESC a1;

        if ((Tex->m_format & 0x10000000) != 0) {
            auto *v14 = Tex->DXTexture;

            int v24[6];
            v24[0] = 0;
            v24[1] = 1;
            v24[2] = 2;
            v24[3] = 3;
            v24[4] = 4;
            v24[5] = 5;

            for (auto v15 = 0u; v15 < 6; ++v15) {
                auto **v16 = a2;
                *a2 += (*a2 - a3) & 0x7F;

                auto v17 = 0u;
                if (Tex->m_numLevel) {
                    auto v18 = v24[v15];
                    for (auto i = v18; v17 < Tex->m_numLevel; v18 = i, ++v17) {
                        v14->lpVtbl->GetSurfaceLevel(v14, v18, (IDirect3DSurface9 **) v17);
                        ++nglDebug().field_8;

                        v20->lpVtbl->GetDesc(v20, &a1);
                        v14->lpVtbl->LockRect(v14,
                                              v18,
                                              (D3DLOCKED_RECT *) v17,
                                              (const RECT *) &rect,
                                              0);
                        auto v19 = sub_782FE0(a1, rect);
                        std::memcpy(rect.pBits, *v16, v19);
                        *a2 += v19;
                        v14->lpVtbl->UnlockRect(v14, i);
                        v20->lpVtbl->Release(v20);
                        --nglDebug().field_8;

                        v16 = a2;
                    }
                }
            }
        } else {
            auto *v6 = Tex->DXTexture;
            auto lvl = 0u;
            int i = 0;
            if (Tex->m_numLevel) {
                while (1) {
                    v6->lpVtbl->GetSurfaceLevel(v6, lvl, &v20);
                    ++nglDebug().field_8;

                    D3DSURFACE_DESC a1;
                    v20->lpVtbl->GetDesc(v20, &a1);
                    v6->lpVtbl->LockRect(v6, lvl, &rect, nullptr, 0);
                    if (!a4) {
                        break;
                    }

                    if (a4 == 10) {
                        auto *v8 = (int *) rect.pBits;
                        for (auto j = (a1.Height * rect.Pitch) >> 2; j != 0; ++*a2) {
                            *v8++ = (**a2 << 24) | 0xFFFFFF;
                            --j;
                        }

                        goto LABEL_15;
                    }

                    if (a4 == 11) {
                        auto *v10 = (int *) rect.pBits;
                        if ((a1.Height * rect.Pitch) >> 2) {
                            auto v11 = (a1.Height * rect.Pitch) >> 2;
                            do {
                                *v10++ = **a2 | ((**a2 | ((**a2 | (**a2 << 8)) << 8)) << 8);
                                --v11;
                                ++*a2;
                            } while (v11);

                            goto LABEL_14;
                        }
                    }

                LABEL_15:
                    v6->lpVtbl->UnlockRect(v6, lvl);
                    v20->lpVtbl->Release(v20);
                    --nglDebug().field_8;

                    i = ++lvl;
                    if (lvl >= Tex->m_numLevel) {
                        return;
                    }
                }

                {
                    auto v12 = sub_782FE0(a1, rect);
                    std::memcpy(rect.pBits, *a2, v12);
                    lvl = i;
                    *a2 += v12;
                }
            LABEL_14:

                goto LABEL_15;
            }
        }

    } else {
        CDECL_CALL(0x00783080, Tex, a2, a3, a4);
    }
}

struct nglTextureInfo {
    uint32_t m_extension;

    struct {
        uint32_t Version;
        uint32_t field_4;
        uint32_t Height;
        int Width;
        int field_10;
        int field_14;
        int field_18;
        int field_1C;
        int field_20;
        int field_24;
        int field_28;
        int field_2C;
        int field_30;
        int field_34;
        int field_38;
        int field_3C;
        int field_40;
        int field_44;
        int field_48;
        int field_4C;
        D3DFORMAT field_50;
        int field_54;
        int field_58;
        int field_5C;
        uint32_t field_60;
        uint32_t field_64;
        uint32_t field_68;
        uint32_t field_6C;
        unsigned int field_70;
        int field_74;
        char field_78;
        char field_79;
        char field_7A;
        char field_7B;

    } Header;

    char field_80[4];
    char field_84[4];
    char field_88[4];
};

bool nglLoadTextureTM2_internal(nglTexture *Tex, nglTextureInfo *TexInfo)
{
    TRACE("nglLoadTextureTM2_internal");

    if constexpr (1)
    {
        assert(Tex != nullptr && "Cannot load a NULL texture !");

        auto v3 = TexInfo->m_extension == 0x4D534444;

        if (TexInfo->m_extension != 0x20534444 && TexInfo->m_extension != 0x4D534444) {
            auto *v2 = Tex->field_60.to_string();
            sp_log("NGL: %s does not seem to be a DDS or DDSMP file !\n", v2);
            return false;
        }

        if (TexInfo->Header.Version != 124) {
            auto *v4 = Tex->field_60.to_string();
            sp_log("NGL: %s invalid DDS header !\n", v4);

            return false;
        }

        if ((TexInfo->Header.field_6C & 0xFE00) != 0)
        {
            Tex->m_format |= 0x10000000u;
            if (TexInfo->Header.field_6C != 0xFE00) {
                auto *v5 = Tex->field_60.to_string();
                sp_log("NGL: %s is not a valid cubemap (must have exactly 6 faces) !\n", v5);

                return false;
            }
        }

        auto *v5 = TexInfo->field_80;
        char *a3 = TexInfo->field_80;
        uint16_t num_palettes = 0;
        if (v3)
        {
            auto v6 = *(uint16_t *) v5;
            if (v6 == 0) {
                auto *v6 = Tex->field_60.to_string();
                sp_log("NGL: %s doesn't contain any palettes !\n", v6);
            }

            num_palettes = *(uint16_t *) v5;

            a3 = &TexInfo->field_88[32 * v6 + 128 - ((32 * (BYTE) v6 - 120) & 0x7F)];
            Tex->Frames = static_cast<nglTexture **>(tlMemAlloc(num_palettes << 7, 8, 0x1000000u));
            int v28 = 0;
            if (num_palettes)
            {
                auto v29 = 0u;
                auto *v8 = TexInfo + 0x90;
                for (auto *i = TexInfo + 0x90; v28 < num_palettes; v8 = i, ++v28) {
                    nglTexture *v9 = CAST(v9, &Tex->Frames[v29 / 4]);
                    *v9 = {};
                    v9->m_format = 17;
                    v9->field_60 = *bit_cast<tlFixedString *>((uint32_t *) v8 - 2);

                    nglTexture **v10 = CAST(v10, v28);

                    v9->m_num_palettes = (int) Tex;
                    v9->Frames = v10;
                    v9->field_34 |= 8u;
                    v9->field_48 = nglCreatePalette(0, 0x100u, a3);

                    void (__fastcall *Add)(void *, void *, void *) = CAST(Add, get_vfunc(nglTextureDirectory()->m_vtbl, 0x10));
                    Add(nglTextureDirectory(), nullptr, v9);
                    v5 = a3 + 1024;

                    a3 += 1024;

                    v29 += 128;
                    i += 32;
                }

            } else {
                v5 = a3;
            }
        } else {
            Tex->m_num_palettes = 0;
            Tex->Frames = nullptr;
        }

        Tex->m_width = TexInfo->Header.Width;
        Tex->m_height = TexInfo->Header.Height;
        Tex->m_numLevel = TexInfo->Header.field_18;

        Tex->m_d3d_format = D3DFMT_UNKNOWN;

        Tex->m_num_palettes = num_palettes;

        Tex->m_format |= 0x200u;

        auto func = [](const auto &header, uint32_t a2, uint32_t a3) -> uint32_t {
            auto v1 = a3 * 16u;
            return (v1 * ((header.field_1C & a3) != 0)) | (a2 & (~v1));
        };

        Tex->field_34 = func(TexInfo->Header, Tex->field_34, 1u);
        Tex->field_34 = func(TexInfo->Header, Tex->field_34, 2u);

        if (!tlIsPow2(Tex->m_width) || !tlIsPow2(Tex->m_height)) {
            sp_log("Loaded textures (DDS) must have power of 2 dimensions !\n");
        }

        if (Tex->m_numLevel == 0) {
            Tex->m_numLevel = 1;
        }

        int v20 = ((0x800000 & TexInfo->Header.field_4) != 0 ? TexInfo->Header.field_14 : 0);

        int a2a = 0;
        if (v20) {
        LABEL_32:
            auto v22 = TexInfo->Header.field_4C;
            if (v22 == 65 && TexInfo->Header.field_54 == 32 &&
                TexInfo->Header.field_64 == 0xFF000000) {
                Tex->m_d3d_format = D3DFMT_A8R8G8B8;
                Tex->m_format |= 1;
                goto LABEL_56;
            }

            if (v22 == 64 && TexInfo->Header.field_54 == 16 && TexInfo->Header.field_5C == 0x7E0) {
                Tex->m_d3d_format = D3DFMT_R5G6B5;
                Tex->m_format |= 5;
                goto LABEL_56;
            }

            if (v22 != 65) {
                goto LABEL_47;
            }

            if (TexInfo->Header.field_54 == 16 && TexInfo->Header.field_64 == 0x8000) {
                Tex->m_d3d_format = D3DFMT_A1R5G5B5;
                Tex->m_format |= 3;
                goto LABEL_56;
            }

            if (TexInfo->Header.field_54 == 16 && TexInfo->Header.field_64 == 0xF000) {
                Tex->m_d3d_format = D3DFMT_A4R4G4B4;
                Tex->m_format |= 2;
            } else {
            LABEL_47:
                if (TexInfo->Header.field_54 != 8) {
                    return false;
                }

                switch (v22) {
                case 0x20000:
                    Tex->m_d3d_format = D3DFMT_L8;
                    Tex->m_format |= 9;
                    break;
                case 2:
                    Tex->m_d3d_format = D3DFMT_A8R8G8B8;
                    a2a = 10;
                    Tex->m_format |= 0xA;
                    break;
                case 0x20001:
                    Tex->m_d3d_format = D3DFMT_A8R8G8B8;
                    a2a = 11;
                    Tex->m_format |= 0xB;
                    break;
                default:
                    Tex->m_format |= 7;
                    Tex->m_d3d_format = D3DFMT_P8;
                    if (!v3) {
                        Tex->field_48 = nglCreatePalette(0, 256u, v5);
                        a3 += 1024;
                    }
                    break;
                }
            }

        LABEL_56:
            if (!v20) {
                goto LABEL_57;
            }

            return false;
        }

        if (auto v21 = TexInfo->Header.field_50; v21 != D3DFMT_DXT1) {
            switch (v21) {
            case 0x32545844:
                Tex->m_d3d_format = D3DFMT_DXT2;
                Tex->m_format |= 1;
                goto LABEL_57;
            case 0x33545844:
                Tex->m_d3d_format = D3DFMT_DXT3;
                Tex->m_format |= 1;
                goto LABEL_57;
            case 0x34545844:
                Tex->m_d3d_format = D3DFMT_DXT4;
                Tex->m_format |= 1;
                goto LABEL_57;
            case 0x35545844:
                Tex->m_d3d_format = D3DFMT_DXT5;
                Tex->m_format |= 1;
                goto LABEL_57;
            default:
                break;
            }

            goto LABEL_32;
        }

        Tex->m_d3d_format = D3DFMT_DXT1;
        Tex->m_format |= 1;

    LABEL_57:

        if ((Tex->m_format & 0x10000000) != 0) {
            Tex->CreateTextureOrSurface();

            sub_783080(Tex, (uint8_t **) &a3, (uint8_t *) TexInfo, a2a);
            TexInfo->Header.field_7B = 77;
            return true;
        }

        Tex->CreateTextureOrSurface();
        if (LOBYTE(Tex->m_format) != 7 || !g_valid_texture_format()) {
            sub_783080(Tex, (uint8_t **) &a3, (uint8_t *) TexInfo, a2a);
            TexInfo->Header.field_7B = 77;
            return true;
        }

        auto v25 = Tex->m_width * Tex->m_height;
        Tex->field_34 |= 8u;
        std::memcpy(Tex->field_30, a3, v25);
        TexInfo->Header.field_7B = 77;
        return true;
    } else {
        return (bool) CDECL_CALL(0x0077A420, Tex, TexInfo);
    }
}

bool nglLoadTextureTM2(nglTexture *tex, uint8_t *a2)
{
    TRACE("nglLoadTextureTM2");

    if constexpr (1) {
        bool result = false;
        
        
        Mod *texMod = getMod(tex->field_60.m_hash, TLRESOURCE_TYPE_TEXTURE);
        std::vector<uint8_t> texBytes;
        if (texMod != nullptr && !readModFile(texMod, texBytes))
            texBytes.clear();

        if (!texBytes.empty()
            && modLooksD3DXImage(texBytes.data(), texBytes.size())) {
            // plain PNG/JPG/BMP/DDS mod replacing a pack texture: NEVER feed
            // it to the engine container parser (its header reads would run
            // wild on image bytes) - decode straight through D3DX
            if (SUCCEEDED(D3DXCreateTextureFromFileInMemory(
                    g_Direct3DDevice(), texBytes.data(),
                    uint32_t(texBytes.size()), &tex->DXTexture))
                && tex->DXTexture != nullptr) {
                tex->field_38 = -1;
                sp_log("[modmesh] texture \"%s\" decoded via D3DX\n",
                       tex->field_60.to_string());
                return true;
            }
            sp_log("[modmesh] texture \"%s\": D3DX rejected the mod image\n",
                   tex->field_60.to_string());
        }

        if (!texBytes.empty()) {
            a2 = texBytes.data();          // engine-container texture mod
        } else if (auto data = getModDataByHash(tex->field_60.m_hash)) {
            a2 = data;
        }

        if ( nglLoadTextureTM2_internal(tex, bit_cast<nglTextureInfo *>(a2)) ) {
            tex->sub_774F20();
            tex->field_38 = -1;
            result = true;
        } else {
            auto *v2 = tex->field_60.to_string();
            sp_log("NGL: \"%s\" cannot be loaded properly (unsupported format ?) !\n", v2);
        }

        return result;

    } else {
        return (bool) CDECL_CALL(0x0077A870, tex, a2);
    }
}

bool nglLoadTextureIFL(nglTexture *tex, uint8_t *a2, int a3) {
    return (bool) CDECL_CALL(0x007733A0, tex, a2, a3);
}

const char *GETFOURCC(uint32_t format) {
    static char result[5]{};

    auto *p_8 = bit_cast<uint8_t *>(&format);

    result[0] = p_8[0];
    result[1] = p_8[1];
    result[2] = p_8[2];
    result[3] = p_8[3];

    return result;
}

nglTexture *nglConstructTexture(const tlFixedString &a1,
                                nglTextureFileFormat a2,
                                void *a3,
                                unsigned int a4)
{

    TRACE("nglConstructTexture", a1.to_string());

    if constexpr (1) {
        auto *tex = static_cast<nglTexture *>(tlMemAlloc(sizeof(nglTexture), 8, 0x1000000u));
        *tex = {};

        tex->field_4 = stru_975AC0().field_4;
        tex->field_0 = &stru_975AC0();
        stru_975AC0().field_4 = tex;
        tex->field_4->field_0 = tex;
        tex->field_8 = 1;
        tex->field_60 = a1;
        tex->field_14 = a2;

        bool v5 = false;
        switch (a2) {
        case 0:
        case 1:
            v5 = nglLoadTextureTM2(tex, static_cast<uint8_t *>(a3));
            break;
        case 2: {
            D3DXCreateTextureFromFileInMemory(g_Direct3DDevice(), a3, a4, &tex->DXTexture);

            v5 = true;
            break;
        }
        case 3:
            v5 = nglLoadTextureIFL(tex, static_cast<uint8_t *>(a3), a4);
            break;
        default:

            break;
        }

        if (v5) {
            return tex;
        }

        tex->field_0->field_4 = tex->field_4;
        tex->field_4->field_0 = tex->field_0;
        tex->field_0 = tex;
        tex->field_4 = tex;
        tlMemFree(tex);

        return nullptr;
    } else {
        return (nglTexture *) CDECL_CALL(0x0077AB30, &a1, a2, a3, a4);
    }
}

nglTexture *nglLoadTextureInPlace(const tlFixedString &a1,
                                  nglTextureFileFormat a2,
                                  void *a3,
                                  int a4) {
    nglTexture * (__fastcall *Find)(void *, void *, int) = CAST(Find, get_vfunc(nglTextureDirectory()->m_vtbl, 0x8));

    nglTexture *result = Find(nglTextureDirectory(), nullptr, a1.m_hash);
    if (result != nullptr) {
        ++result->field_8;
    } else {
        auto *tex = nglConstructTexture(a1, a2, a3, a4);
        if (tex != nullptr) {
            void (__fastcall *Add)(void *, void *, nglTexture *) = CAST(Add, get_vfunc(nglTextureDirectory()->m_vtbl, 0x10));
            Add(nglTextureDirectory(), nullptr, tex);
            result = tex;
        } else {
            result = nglDefaultTex();
        }
    }
    return result;
}

vector4d sub_411C10(color32 a2)
{
    vector4d result;
    result[0] = a2.field_0[2] * 0.0039215689f;

    result[1] = a2.field_0[1] * 0.0039215689f;

    result[2] = a2.field_0[0] * 0.0039215689f;

    result[3] = a2.field_0[3] * 0.0039215689f;
    return result;
}

void nglDebugAddSphere(const math::MatClass<4, 3> &a1, math::VecClass<3, 1> a2, uint32_t a3)
{
    if constexpr (1)
    {
        nglParamSet<nglShaderParamSet_Pool> a4 {static_cast<nglParamSet<nglShaderParamSet_Pool>::nglParamSetType>(1)};

        auto *mem = nglListAlloc(sizeof(vector4d), 16);
        auto *tmp = new (mem) vector4d {sub_411C10(*bit_cast<color32 *>(&a3))};

        nglTintParam v5 {tmp};

        a4.SetParam(v5);

        nglMeshParams v8{2};
        v8.Scale = a2;

        nglListAddMesh(nglDebugMesh_Sphere(), a1, &v8, &a4);

    } else {
        CDECL_CALL(0x0077F930, &a1, a2, a3);
    }
}

void nglSetBufferSize(nglBufferType a1, uint32_t a2, bool a3)
{
    if constexpr (0)
    {
    }
    else
    {
        CDECL_CALL(0x0077B610, a1, a2, a3);
    }
}

nglMesh *nglCloseMesh()
{
    TRACE("nglCloseMesh");

    if constexpr (0)
    {
    }
    else
    {
        return (nglMesh *) CDECL_CALL(0x00772130);
    }
}

void nglListAddCustomNode(void (*a1)(unsigned int *&, void *), void *a2, const nglSortInfo *a3) {
    CDECL_CALL(0x0076C3A0, a1, a2, a3);
}

void nglRenderQuad(nglQuad *a2)
{
    if (nglSyncDebug().DisableQuads) {
        return;
    }

    auto perf_counter = query_perf_counter();

    g_renderState().setCullingMode(D3DCULL_NONE);

    g_renderState().setDepthBuffer(D3DZB_FALSE);

    g_renderState().setBlending(a2->field_58, a2->field_5C, 128);

    if ( EnableShader() )
    {
        nglSetVertexDeclarationAndShader(&stru_975780());
    }
    else
    {
        g_Direct3DDevice()->lpVtbl->SetVertexDeclaration(g_Direct3DDevice(), dword_9738E0()[28]);
        g_Direct3DDevice()->lpVtbl->SetTransform(
            g_Direct3DDevice(),
            (D3DTRANSFORMSTATETYPE)256,
            bit_cast<const D3DMATRIX *>(&nglCurScene()->field_24C));
    }
    
    if ( struct_972688().field_30 && (nglCurScene()->field_334->field_34 & 4) != 0 )
    {
        a2->field_0[0].pos.x *= struct_972688().field_34;
        a2->field_0[0].pos.y *= struct_972688().field_38;
        a2->field_0[1].pos.x *= struct_972688().field_34;
        a2->field_0[1].pos.y *= struct_972688().field_38;
        a2->field_0[2].pos.x *= struct_972688().field_34;
        a2->field_0[2].pos.y *= struct_972688().field_38;
        a2->field_0[3].pos.x *= struct_972688().field_34;
        a2->field_0[3].pos.y *= struct_972688().field_38;
    }

    auto m_tex = a2->m_tex;
    if ( m_tex != nullptr )
    {
        nglSetSamplerState(0, D3DSAMP_ADDRESSU, ((a2->field_54 & 0x40) | 0x20u) >> 5);
        nglSetSamplerState(0, D3DSAMP_ADDRESSV, ((a2->field_54 & 0x80) | 0x40u) >> 6);

        nglTextureAnimFrame() = nglCurScene()->IFLFrame;
        nglDxSetTexture(0, m_tex, a2->field_54, 3);

        if ( EnableShader() ) {
            SetPixelShader(&dword_9757A0());
        } else {
            nglSetTextureStageState(0, D3DTSS_COLOROP, 4u);
            nglSetTextureStageState(0, D3DTSS_COLORARG1, 2u);
            nglSetTextureStageState(0, D3DTSS_COLORARG2, 0);
            nglSetTextureStageState(0, D3DTSS_ALPHAOP, 4u);
            nglSetTextureStageState(0, D3DTSS_ALPHAARG1, 2u);
            nglSetTextureStageState(0, D3DTSS_ALPHAARG2, 0);
            nglSetTextureStageState(1u, D3DTSS_COLOROP, 1u);
            nglSetTextureStageState(1u, D3DTSS_ALPHAOP, 1u);
            g_renderState().setLighting(0);
        }
    }
    else
    {
        if ( EnableShader() ) {
            SetPixelShader(&dword_975794());
        } else {
            nglSetTextureStageState(0, D3DTSS_COLOROP, 2u);
            nglSetTextureStageState(0, D3DTSS_COLORARG1, 0);
            nglSetTextureStageState(0, D3DTSS_ALPHAOP, 2u);
            nglSetTextureStageState(0, D3DTSS_ALPHAARG1, 0);
            nglSetTextureStageState(1u, D3DTSS_COLOROP, 1u);
            nglSetTextureStageState(1u, D3DTSS_ALPHAOP, 1u);
            g_renderState().setLighting(0);
        }

        g_renderTextureState().field_0[0] = nullptr;
        g_Direct3DDevice()->lpVtbl->SetTexture(g_Direct3DDevice(), 0, nullptr);
    }

    g_renderState().setFogEnable(false);

    auto v8 = sub_77E820(a2->field_50.f);
    struct {
        struct {
            float x, y;
        } pos;
        float field_8;
        uint32_t m_color;
        struct {
            float x, y;
        } uv;
    } v9[4] {};

    auto *quads = &a2->field_0[0];
    for ( auto &v2 : v9 )
    {
        v2.pos.x = sub_77E940(quads->pos.x);
        v2.pos.y = sub_77EA00(quads->pos.y);
        v2.field_8 = v8;
        v2.m_color = quads->m_color;
        v2.uv.x = quads->uv.field_0;
        v2.uv.y = quads->uv.field_4;
        ++quads;
    }

    g_Direct3DDevice()->lpVtbl->DrawPrimitiveUP(g_Direct3DDevice(), D3DPT_TRIANGLESTRIP, 2, v9, 24);
    if ( g_distance_clipping_enabled()
            && !sub_581C30())
    {
        g_renderState().setFogEnable(true);
    }

    g_renderState().setDepthBuffer(D3DZB_TRUE);

    nglPerfInfo().m_counterQuads.QuadPart += query_perf_counter().QuadPart - perf_counter.QuadPart;
}

void * nglQuadNode::operator new(size_t size)
{
    auto *mem = nglListAlloc(size, 16);
    return mem;
}

void nglQuadNode::Render()
{
    TRACE("nglQuadNode::Render");

    if ( !nglSyncDebug().DisableQuads ) {
        nglRenderQuad(&this->field_C);
    }
}

void nglListAddQuad(nglQuad *Quad)
{
    if constexpr (0)
    {
        if (Quad != nullptr)
        {
            auto *v1 = new nglQuadNode{};

            if (nglCurScene()->field_3E4) {
                nglCalculateMatrices(false);
            }

            std::memcpy(&v1->field_C, Quad, sizeof(v1->field_C));
            if (((1 << Quad->field_58) & 3) != 0)
            {
                v1->m_tex = Quad->m_tex;
                v1->m_next_node = nglCurScene()->field_340;
                nglCurScene()->field_340 = v1;
                ++nglCurScene()->OpaqueListCount;
            }
            else
            {
                v1->m_tex = Quad->field_50.tex;
                v1->m_next_node = nglCurScene()->TransNodes;
                nglCurScene()->TransNodes = v1;
                ++nglCurScene()->TransListCount;
            }

            if (0) //(nglSyncDebug().DumpMesh)
            {
                nglDumpQuad(Quad);
            }
        }
        else
        {
            error("NULL mesh passed to nglListAddMesh !\n");
        }
    }
    else
    {
        CDECL_CALL(0x0077AFE0, Quad);
    }
}

nglStringNode::nglStringNode() {
    m_vtbl = 0x0088EBB4;
}

void * nglStringNode::operator new(size_t size)
{
    auto *mem = nglListAlloc(size, 16);
    return mem;
}

void sub_754640(void *a1)
{
    if constexpr (0)
    {
        auto *node = static_cast<nglRenderNode *>(a1);
        node->m_next_node = nglCurScene()->TransNodes;
        nglCurScene()->TransNodes = node;
        ++nglCurScene()->TransListCount;
    }
    else
    {
        CDECL_CALL(0x00754640, a1);
    }
}

double sub_77E940(Float a1)
{
    auto v2 = a1 * nglCurScene()->field_20C[0][0];
    return v2 + nglCurScene()->field_20C[3][0];
}

double sub_77EA00(Float a1)
{
    auto v2 = a1 * nglCurScene()->field_20C[1][1];
    return v2 + nglCurScene()->field_20C[3][1];
}

double sub_77E820(Float a1)
{
    auto m_nearz = a1;
    if ( a1 < nglCurScene()->m_nearz ) {
        m_nearz = nglCurScene()->m_nearz;
    }

    if ( m_nearz > nglCurScene()->m_farz ) {
        m_nearz = nglCurScene()->m_farz;
    }

    auto v3 = m_nearz * nglCurScene()->ViewToScreen[2][2];
    auto v5 = m_nearz * nglCurScene()->ViewToScreen[2][3];
    auto v4 = v3 + nglCurScene()->ViewToScreen[3][2];
    auto v6 = v5 + nglCurScene()->ViewToScreen[3][3];
    auto result = v4 / v6;
    if ( result < 0.0 ) {
        return 0.0;
    }

    if ( result > 1.0f ) {
        return 1.0f;
    }

    return result;
}

bool sub_581C30()
{
    auto *v0 = g_femanager.IGO->field_44;
    return v0->field_5C4 || v0->field_5C3;
}

void nglListAddString(nglFont* a1, Float a2, Float a3, Float a4, unsigned int a5, Float a6, Float a8, const char* Format, ...)
{
    char buffer[1024];
    va_list Args;

    va_start(Args, Format);
    vsprintf(buffer, Format, Args);
    nglListAddString(a1, buffer, a2, a3, a4, a5, a6, a8);
}

void nglListAddString(nglFont *font,
                      const char *a2,
                      Float a3,
                      Float a4,
                      Float z_value,
                      uint32_t color,
                      Float a7,
                      Float a8)
{
    //sp_log("%s %f %f", a2, float{a3}, float{a4});
    //sp_log("%f", float{z_value});

    if constexpr (1)
    {
        if (nglCurScene()->field_3E4) {
            nglCalculateMatrices(false);
        }

        if (a2 != nullptr && a2[0] != '\0' && font != nullptr && font->field_24 != nullptr)
        {
            auto *v8 = new nglStringNode{};

            auto v9 = strlen(a2) + 1;
            v8->field_C = static_cast<unsigned char *>(nglListAlloc(v9, 16));
            memcpy(v8->field_C, a2, v9);
            v8->m_color = color;
            v8->field_14 = a3;
            v8->field_18 = a4;
            v8->field_10 = font;
            v8->field_1C = z_value;
            v8->field_20 = a7;
            v8->field_24 = a8;
            v8->field_8 = z_value;
            sub_754640(v8);
        }
    } else {
        CDECL_CALL(0x00779C40, font, a2, a3, a4, z_value, color, a7, a8);
    }
}

void nglListAddString(nglFont *arg0, float arg4, float a3, float a4, float a5, float a6, const char *a2, ...)
{
    char a1[1024];
    va_list va;

    va_start(va, a2);
    vsprintf(a1, a2, va);
    nglListAddString(arg0, a1, arg4, a3, a4, -1, a5, a6);
}

void nglListAddString(nglFont *a1, Float a3, Float a4, Float a5, int a6, const char *Format, ...)
{
    va_list Args;

    va_start(Args, Format);
    vsprintf(nglFontBuffer(), Format, Args);
    nglListAddString(a1, nglFontBuffer(), a3, a4, a5, a6, 1.0f, 1.0f);
    va_end(Args);
}

nglMesh *nglGetMesh(const tlFixedString &Name, bool Warn)
{
    TRACE("nglGetMesh", Name.to_string());

    if constexpr (0)
    {
        tlHashString v2 {Name.m_hash};

        nglMesh * (__fastcall *Find)(void *, void *, const tlHashString *) = CAST(Find, get_vfunc(nglMeshDirectory()->m_vtbl, 0xC));
        auto *Mesh = Find(nglMeshDirectory(), nullptr, &v2);

        if (Mesh == nullptr && Warn) {
            sp_log("nglGetMesh: Unable to find mesh %s.\n", Name.to_string());
        }

        return Mesh;

    } else {
        return (nglMesh *) CDECL_CALL(0x0076EFF0, &Name, Warn);
    }
}

nglMesh *nglGetMesh(uint32_t a1, bool a2) {
    nglMesh * (__fastcall *Find)(void *, void *, uint32_t) = CAST(Find, get_vfunc(nglMeshDirectory()->m_vtbl, 0x8));

    auto *Mesh = Find(nglMeshDirectory(), nullptr, a1);

    if (Mesh == nullptr && a2) {
        sp_log("nglGetMesh: Unable to find mesh %d.\n", a1);
    }

    return Mesh;
}

nglMesh *nglGetMesh(const tlHashString &a1, bool a2)
{
    nglMesh * (__fastcall *Find)(void *, int, const tlHashString *) =
        CAST(Find, get_vfunc(nglMeshDirectory()->m_vtbl, 0xC));
    auto *v4 = Find(nglMeshDirectory(), 0, &a1);
    if ( v4 == nullptr && a2 )
    {
        auto *v2 = a1.c_str();
        sp_log("nglGetMesh: Unable to find mesh %s.\n", v2);
    }

    return v4;
}

void nglDestroySection(nglMeshSection *a1)
{
#if MOD_MESH_SUPPORT
    // The palette tlMemFree'd below is the one we handed the section, and the
    // section address itself is about to be recycled. Drop the replacement
    // storage now so nothing can be mirrored from (or onto) a dead section.
    modForgetSection(a1);
#endif

    if (a1->m_indexBuffer != nullptr)
    {
        nglVertexBuffer::sub_77B5D0((nglVertexBuffer *) &a1->m_indexBuffer, ResourceType::IndexBuffer);
        a1->m_indexBuffer = nullptr;
    }

    if (a1->field_3C.m_vertexBuffer != nullptr)
    {
        nglVertexBuffer::sub_77B5D0(&a1->field_3C, ResourceType::VertexBuffer);
        a1->field_3C.m_vertexBuffer = nullptr;
    }

    a1->VertexDef->Destroy();
    if (a1->NBones != 0)
    {
        tlMemFree(a1->BonesIdx);
        a1->BonesIdx = nullptr;
    }

    tlMemFree(a1);
}

void nglDestroyMesh(nglMesh *Mesh) {
    if constexpr (1) {
        if (Mesh != nullptr) {
            if (Mesh->NBones) {
                tlMemFree(Mesh->Bones);
                Mesh->Bones = nullptr;
            }

            for (auto i = 0u; i < Mesh->NSections; ++i) {
                auto *v2 = &Mesh->Sections[i];
                if (v2->field_0) {
                    nglDestroySection(v2->Section);
                }
            }

            tlMemFree(Mesh->Sections);
            tlMemFree(Mesh);
        }

    } else {
        CDECL_CALL(0x00775700, Mesh);
    }
}

void nglScaleQuad(nglQuad *a1, Float a2, Float a3, Float a4, Float a5)
{
    a1->field_0[0].pos.x = (a1->field_0[0].pos.x - a2) * a4 + a2;
    a1->field_0[0].pos.y = (a1->field_0[0].pos.y - a3) * a5 + a3;
    a1->field_0[1].pos.x = (a1->field_0[1].pos.x - a2) * a4 + a2;
    a1->field_0[1].pos.y = (a1->field_0[1].pos.y - a3) * a5 + a3;
    a1->field_0[2].pos.x = (a1->field_0[2].pos.x - a2) * a4 + a2;
    a1->field_0[2].pos.y = (a1->field_0[2].pos.y - a3) * a5 + a3;
    a1->field_0[3].pos.x = (a1->field_0[3].pos.x - a2) * a4 + a2;
    a1->field_0[3].pos.y = (a1->field_0[3].pos.y - a3) * a5 + a3;
}

void nglSetQuadVPos(nglQuad *a1, int a2, float a3, float a4)
{
    auto &pos = a1->field_0[a2].pos;
    pos.x = a3;
    pos.y = a4;
}

void nglSetQuadVUV(nglQuad *a1, int a2, float a3, float a4)
{
    auto &quad = a1->field_0[a2];
    quad.uv.field_0 = a3;
    quad.uv.field_4 = a4;
}

void nglSetQuadVColor(nglQuad *a1, int a2, unsigned int a3)
{
    a1->field_0[a2].m_color = a3;
}

void nglSetQuadZ(nglQuad *a1, Float a2)
{
    a1->field_50.f = a2;
}

void nglSetQuadMapFlags(nglQuad *a1, unsigned int a2) {
    a1->field_54 = a2;
}

void nglInitQuad(nglQuad *a1)
{
    std::memset(a1, 0, sizeof(nglQuad));
    a1->field_0[0].m_color = 0xFFFFFFFF;
    a1->field_0[1].m_color = 0xFFFFFFFF;
    a1->field_0[2].m_color = 0xFFFFFFFF;
    a1->field_0[3].m_color = 0xFFFFFFFF;
    a1->field_0[0].uv.field_0 = 0;
    a1->field_0[1].uv.field_0 = 1.0;
    a1->field_0[2].uv.field_0 = 0;
    a1->field_0[3].uv.field_0 = 1.0;
    a1->field_0[0].uv.field_4 = 0;
    a1->field_0[1].uv.field_4 = 0;
    a1->field_0[2].uv.field_4 = 1.0;
    a1->field_0[3].uv.field_4 = 1.0;
    a1->field_54 = 194;
    a1->field_58 = static_cast<nglBlendModeType>(2);
}

void nglRotateQuad(nglQuad *a2, Float a3, Float a4, Float a5)
{
    for ( int i = 0; i < 4; ++i )
    {
        auto &v8 = a2->field_0[i];
        auto v7 = v8.pos.x - a3;
        auto v6 = v8.pos.y - a4;
        auto v4 = std::cos(a5) * v7;
        v8.pos.x = v4 - std::sin(a5) * v6 + a3;
        auto v5 = std::cos(a5) * v6;
        v8.pos.y = std::sin(a5) * v7 + v5 + a4;
    }
}

void sub_781980(int width, int height) {
    CDECL_CALL(0x00781980, width, height);
}

void sub_771B60() {
    if constexpr (0) {
#if 0
        void *v6 = nullptr;

        static Var<nglMeshSection::internal> stru_9729C0{0x009729C0};
        nglMeshSection::internal::createIndexOrVertexBuffer(&stru_9729C0(),
                                                            ResourceType::IndexBuffer,
                                                            1536,
                                                            0,
                                                            0,
                                                            D3DPOOL_DEFAULT);
        stru_9729C0().field_0->lpVtbl->Lock(stru_9729C0().field_0, 0, 0, (void **) &v6, 0);
        v2 = retaddr;
        v3 = 1;
        v6 = a2;
        do {
            uint16_t *v4 = (uint16_t *) (v2 + 2);
            *(v4 - 1) = v3 - 1;
            *v4++ = v3;
            v5 = v3 + 2;
            *v4++ = v3 + 2;
            *v4++ = v3;
            *v4++ = v3 + 1;
            v3 += 4;
            *v4 = v5;
            v2 = (char *) (v4 + 1);
        } while ((unsigned __int16) (v3 - 1) < 512u);

        stru_9729C0().field_0->lpVtbl->Unlock(stru_9729C0().field_0);
#endif

    } else {
        CDECL_CALL(0x00771B60);
    }
}

void create_front_and_back_buffer_tex() {
    struct {
        int m_width;
        int m_height;
    } *v1 = bit_cast<decltype(v1)>(0x00972688);

    nglFrontBufferTex() = nglCreateTexture(4609u, v1->m_width, v1->m_height, 0, 1);
    nglFrontBufferTex()->field_60 = tlFixedString{"nglFrontBuffer"};
    nglTextureDirectory()->Add(nglFrontBufferTex());
    nglBackBufferTex() = nglCreateTexture(20993u, v1->m_width, v1->m_height, 0, 1);
    nglBackBufferTex()->field_60 = tlFixedString{"nglBackBuffer"};
    nglBackBufferTex()->field_34 |= 4u;
    nglTextureDirectory()->Add(nglBackBufferTex());
}

void nglReleaseFont(nglFont *font) {
    CDECL_CALL(0x007793E0, font);
}

void sub_77B2F0(bool a1) {
    CDECL_CALL(0x0077B2F0, a1);
}

void ngl_releasefile_callback(tlFileBuf *) {
    ;
}

bool ngl_readfile_callback(const char *FileName, tlFileBuf *File, unsigned int a3, unsigned int a4)
{
    TRACE("ngl_readfile_callback", FileName);

    if constexpr (1)
    {
        mString v10 {FileName};

        filespec v11 {v10};

        File->Buf = nullptr;
        File->Size = 0;
        v11.m_ext.to_lower();
        mString v9 {v11.m_name};

        if (v11.m_ext == ".tga") {
            v9 += ".DDS";
        } else {
            v9 += v11.m_ext;
        }

        resource_key key = create_resource_key_from_path(v9.c_str(), RESOURCE_KEY_TYPE_NONE);
        int size = 0;
        if (key.get_type() != RESOURCE_KEY_TYPE_NONE) {
            File->Buf = (char *) resource_manager::get_resource(key, &size, nullptr);
        }

        bool v4 = (File->Buf == nullptr);
        File->Size = size;

        if (v4) {
            return false;
        }

        return true;
    }
    else
    {
        return (bool) CDECL_CALL(0x00594740, FileName, File, a3, a4);
    }
}

static Var<WINDOWPLACEMENT> wndpl = {0x00975710};

void sub_77EB40() {
    HKEY phkResult;
    HKEY hKey;
    DWORD cbData;
    DWORD Type;

    RegOpenKeyExA(HKEY_CURRENT_USER, "Software", 0, 0xF003Fu, &phkResult);
    RegCreateKeyExA(phkResult, "NGL", 0, nullptr, 0, 0xF003Fu, nullptr, &hKey, nullptr);
    RegCloseKey(phkResult);
    if (RegQueryValueExA(hKey, "Placement", nullptr, &Type, (LPBYTE) &wndpl(), &cbData)) {
        wndpl().length = 0;
    }

    RegCloseKey(hKey);
}

static Var<BOOL> dword_93AE80 = {0x0093AE80};

void sub_77EBD0()
{
    if constexpr (1)
    {
        HKEY phkResult;
        RegOpenKeyExA(HKEY_CURRENT_USER, "Software", 0, 0xF003Fu, &phkResult);

        HKEY hKey;
        RegCreateKeyExA(phkResult, "NGL", 0, nullptr, 0, 0xF003Fu, nullptr, &hKey, nullptr);
        RegCloseKey(phkResult);

        RegSetValueExA(hKey, "Placement", 0, 3u, (const BYTE *)&wndpl(), 0x2Cu);

        RegSetValueExA(hKey, "Windowed", 0, 4u, (const BYTE *)&g_Windowed(), 4u);
        RegCloseKey(hKey);
    }
    else
    {
        CDECL_CALL(0x0077EBD0);
    }

    assert(0);
}

void ToggleFullScreen(BOOL isFullscreen)
{
    TRACE("ToggleFullScreen");

    if (!byte_971F9C() && s_d3dpresent_params().Windowed != isFullscreen) {
        s_d3dpresent_params().Windowed = isFullscreen;
        Reset3DDevice();
        if (isFullscreen) {
            SetWindowPlacement(g_hWnd(), &wndpl());
        } else {
            auto v2 = GetSystemMetrics(1);
            auto v1 = GetSystemMetrics(0);

            SetWindowPos(g_hWnd(), nullptr, 0, 0, v1, v2, SWP_NOACTIVATE);
        }

        SetWindowPos(g_hWnd(), (HWND) (-isFullscreen - 1), 0, 0, 0, 0, 3u);
        ShowCursor(isFullscreen);
    }
}

int __stdcall WndProcEx(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam) {
    //sp_log("%s", g_Windowed() ? "TRUE" : "FALSE");

    if constexpr (0) {
        int result;

        if (Msg > WM_GETMINMAXINFO)
        {
            switch (Msg) {
            case WM_NCHITTEST: {
                if (!g_Windowed()) {
                    return 1;
                }
            }
            case WM_KEYDOWN: {
                if (wParam != VK_ESCAPE) {
                    result = DefWindowProcA(hWnd, Msg, wParam, lParam);
                    break;
                }

                SendMessageA(g_hWnd(), WM_CLOSE, 0, 0);
                return DefWindowProcA(hWnd, Msg, VK_ESCAPE, lParam);
            }
            case WM_SYSKEYDOWN:
                if (wParam == VK_RETURN) { // Alt + Enter - switch to fullscreen or window
                    g_Windowed() = !g_Windowed();

                    ToggleFullScreen(g_Windowed());
                }

                result = DefWindowProcA(hWnd, Msg, wParam, lParam);
                break;

            case WM_SYSCOMMAND:
                if (wParam > SC_MAXIMIZE) {
                    if (wParam != SC_KEYMENU && wParam != SC_MONITORPOWER) {
                        result = DefWindowProcA(hWnd, Msg, wParam, lParam);
                        break;
                    }
                } else if (wParam != SC_MAXIMIZE && wParam != SC_SIZE && wParam != SC_MOVE) {
                    result = DefWindowProcA(hWnd, Msg, wParam, lParam);
                    break;
                }

                if (!g_Windowed()) {
                    return 1;
                }

                result = DefWindowProcA(hWnd, Msg, wParam, lParam);
                break;
            default:
                result = DefWindowProcA(hWnd, Msg, wParam, lParam);
                break;
            }
        }
        else
        {
            if (Msg != WM_GETMINMAXINFO)
            {
                switch (Msg) {
                case WM_MOVE:
                    if (!g_Windowed() || !g_hWnd()) {
                        return DefWindowProcA(hWnd, Msg, wParam, lParam);
                    }

                    GetWindowPlacement(hWnd, &wndpl());
                    sub_77EBD0();
                    return DefWindowProcA(hWnd, Msg, wParam, lParam);
                case WM_PAINT:
                    if (!g_Windowed() || !g_hWnd() || !byte_971F9C()) {
                        return DefWindowProcA(hWnd, Msg, wParam, lParam);
                    }

                    sub_76DF00();
                    return DefWindowProcA(hWnd, Msg, wParam, lParam);
                case WM_CLOSE:
                    g_Windowed() = dword_93AE80();
                    ToggleFullScreen(dword_93AE80());
                    DestroyWindow(g_hWnd());
                    PostQuitMessage(0);
                    g_hWnd() = 0;
                    return DefWindowProcA(hWnd, Msg, wParam, lParam);
                case WM_ACTIVATEAPP: {
                    if (wParam == 0) {
                        return DefWindowProcA(hWnd, Msg, wParam, lParam);
                    }

                    byte_971F9C() = false;
                    if (!g_hWnd() || g_Windowed()) {
                        return DefWindowProcA(hWnd, Msg, wParam, lParam);
                    }

                    s_d3dpresent_params().Windowed = true;

                    ToggleFullScreen(false);

                    return DefWindowProcA(hWnd, Msg, wParam, lParam);
                }
                case WM_CANCELMODE:
                    if (g_Windowed()) {
                        return DefWindowProcA(hWnd, Msg, wParam, lParam);
                    }

                    ShowCursor(TRUE);
                    byte_971F9C() = true;
                    return DefWindowProcA(hWnd, Msg, wParam, lParam);
                default:
                    return DefWindowProcA(hWnd, Msg, wParam, lParam);
                }
            } else { // WM_GETMINMAXINFO
                bit_cast<MINMAXINFO *>(lParam)->ptMinTrackSize.x = 100;
                bit_cast<MINMAXINFO *>(lParam)->ptMinTrackSize.y = 100;
                result = DefWindowProcA(hWnd, WM_GETMINMAXINFO, wParam, lParam);
            }
        }
        return result;
    } else {
        return STDCALL(0x0076D340, hWnd, Msg, wParam, lParam);
    }
}

void create_renderer(HWND hWnd)
{
    TRACE("create renderer");

    WNDCLASSEXA v13;

    static Var<IDirect3D9 *> g_pD3D {0x00971FA4};

    s_d3dpresent_params() = {};
    HWND v1 = hWnd;
    if (hWnd == nullptr)
    {
        sub_77EB40();
        dword_93AE80() = g_Windowed();
        v13 = {};
        v13.cbSize = 48;
        v13.style = 0x2000;
        v13.lpfnWndProc = bit_cast<WNDPROC>(&WndProcEx);
        v13.hInstance = GetModuleHandleA(nullptr);
        v13.hIcon = LoadIconA(nullptr, IDI_APPLICATION);
        v13.hCursor = LoadCursorA(nullptr, IDC_ARROW);
        v13.lpszClassName = "NGL";
        RegisterClassExA(&v13);

        v1 = CreateWindowExA(0,
                             "NGL",
                             "NGL",
                             0x10CF0000u,
                             0,
                             0,
                             nWidth(),
                             nHeight(),
                             nullptr,
                             nullptr,
                             v13.hInstance,
                             nullptr);
    }

    g_hWnd() = v1;

    if (g_Windowed()) {
        if (wndpl().length) {
            SetWindowPlacement(v1, &wndpl());
        } else {
            struct tagRECT Rect;
            Rect.top = (GetSystemMetrics(SM_CYSCREEN) - nHeight()) / 2;
            Rect.bottom = Rect.top + nHeight();
            Rect.left = (GetSystemMetrics(SM_CXSCREEN) - nWidth()) / 2;
            Rect.right = Rect.left + nWidth();

            WINDOWINFO v13;
            v13.cbSize = 60;
            GetWindowInfo(g_hWnd(), &v13);
            AdjustWindowRectEx(&Rect, v13.dwStyle, 0, v13.dwExStyle);
            SetWindowPos(g_hWnd(),
                         nullptr,
                         Rect.left,
                         Rect.top,
                         Rect.right - Rect.left + 1,
                         Rect.bottom - Rect.top + 1,
                         0);
        }
    } else {
        int v3 = GetSystemMetrics(SM_CYSCREEN);
        int v4 = GetSystemMetrics(SM_CXSCREEN);
        SetWindowPos(g_hWnd(), nullptr, 0, 0, v4, v3, 0);
        ShowCursor(false);
    }

    g_pD3D() = Direct3DCreate9(0x80000020);

    D3DDISPLAYMODE d3ddm;
    g_pD3D()->lpVtbl->GetAdapterDisplayMode(g_pD3D(), 0, &d3ddm);

    if (d3ddm.Format != D3DFMT_A8R8G8B8 && d3ddm.Format != D3DFMT_X8R8G8B8) {
        auto *v7 = get_msg(g_fileUSM(), "MSGBOX_32BIT");
        MessageBoxA(g_hWnd(), v7, "USM.exe", 0x10u);
        exit(255);
    }

    Var<int[15]> dword_972688 = {0x00972688};

    dword_972688()[0] = nWidth();
    dword_972688()[1] = nHeight();
    s_d3dpresent_params().BackBufferWidth = nWidth();
    s_d3dpresent_params().BackBufferHeight = nHeight();
    s_d3dpresent_params().Windowed = g_Windowed();
    s_d3dpresent_params().BackBufferCount = 1;
    s_d3dpresent_params().BackBufferFormat = D3DFMT_A8R8G8B8;
    s_d3dpresent_params().MultiSampleType = D3DMULTISAMPLE_NONE;
    s_d3dpresent_params().SwapEffect = D3DSWAPEFFECT_DISCARD;
    s_d3dpresent_params().hDeviceWindow = g_hWnd();
    s_d3dpresent_params().EnableAutoDepthStencil = true;
    s_d3dpresent_params().AutoDepthStencilFormat = D3DFMT_D24S8;
    s_d3dpresent_params().Flags = 0;
    s_d3dpresent_params().PresentationInterval = 1;
    s_d3dpresent_params().FullScreen_RefreshRateInHz = (g_Windowed() ? 0 : 60);


    g_valid_texture_format() = g_pD3D()->lpVtbl->CheckDeviceFormat(g_pD3D(),
                                                        D3DADAPTER_DEFAULT,
                                                        D3DDEVTYPE_HAL,
                                                        D3DFMT_X8R8G8B8,
                                                        0,
                                                        D3DRTYPE_TEXTURE,
                                                        D3DFMT_P8) < 0;

    if (g_pD3D()->lpVtbl->CheckDeviceType(g_pD3D(),
                                          D3DADAPTER_DEFAULT,
                                          D3DDEVTYPE_HAL,
                                          D3DFMT_X8R8G8B8,
                                          D3DFMT_A8R8G8B8,
                                          g_Windowed())) {
        const char *v8 = get_msg(g_fileUSM(), "MSGBOX_WARNING");
        const char *v9 = get_msg(g_fileUSM(), "MSGBOX_NOHARDWARE");
        MessageBoxA(g_hWnd(), v9, v8, 0x30u);
        g_pD3D()->lpVtbl->CreateDevice(g_pD3D(),
                                       D3DADAPTER_DEFAULT,
                                       D3DDEVTYPE_REF,
                                       g_hWnd(),
                                       D3DCREATE_SOFTWARE_VERTEXPROCESSING,
                                       &s_d3dpresent_params(),
                                       &g_Direct3DDevice());
    }
    else
    {
        g_pD3D()->lpVtbl->CreateDevice(g_pD3D(),
                                       D3DADAPTER_DEFAULT,
                                       D3DDEVTYPE_HAL,
                                       g_hWnd(),
                                       D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_PUREDEVICE,
                                       &s_d3dpresent_params(),
                                       &g_Direct3DDevice());

        if (g_Direct3DDevice() == nullptr) {

            g_pD3D()->lpVtbl->CreateDevice(g_pD3D(),
                                           D3DADAPTER_DEFAULT,
                                           D3DDEVTYPE_HAL,
                                           g_hWnd(),
                                           D3DCREATE_SOFTWARE_VERTEXPROCESSING,
                                           &s_d3dpresent_params(),
                                           &g_Direct3DDevice());

            Var<bool> byte_971F90 = {0x00971F90};
            byte_971F90() = true;
        }
    }

    g_Direct3DDevice()->lpVtbl->Clear(g_Direct3DDevice(), 0, nullptr, 7u, 0, 1.0, 0);
    sub_76DF00();
    g_Direct3DDevice()->lpVtbl->Clear(g_Direct3DDevice(), 0, nullptr, 7u, 0, 1.0, 0);
    g_Direct3DDevice()->lpVtbl->SetStreamSource(g_Direct3DDevice(), 0, nullptr, 0, 0);
}

void nglListBeginScene(nglSceneParamType a2) {
    TRACE("nglListBeginScene");

    if constexpr (1) {
        auto *v2 = new nglScene {};

        if (nglCurScene() != nullptr) {
            auto *v3 = nglCurScene()->field_318;
            if (v3 != nullptr) {
                v3->field_310 = v2;
            } else {
                nglCurScene()->field_314 = v2;
            }

            nglCurScene()->field_318 = v2;
        } else {
            nglRootScene() = v2;
        }

        nglSetupScene(v2, a2);
    } else {
        CDECL_CALL(0x0076C970, a2);
    }
}

void nglSetFBWriteMask(unsigned int a1)
{
    nglCurScene()->FBWriteMask = a1;
}

void nglSetClearFlags(unsigned int a1)
{
    nglCurScene()->ClearFlags = a1;
}

void nglDebugInit() {
    TRACE("nglDebugInit");

    nglDebug() = {};
    nglSyncDebug() = nglDebug();
    nglPerfInfo() = {};
    nglSyncPerfInfo() = {};
    nglDebug().field_4 = 65280;
}

//0x00783A90
int nglHostPrintf(HANDLE hObject, const char *a2, ...)
{
    va_list va;
    va_start(va, a2);

    char a1[4096];
    auto result = vsprintf(a1, a2, va);

    if (hObject) {
        auto v3 = strlen(a1);
        DWORD numOfBytesWritten;

        result = WriteFile(hObject, a1, v3, &numOfBytesWritten, nullptr);
        if (!result) {
            CloseHandle(hObject);
            sp_log("nglHostPrintf: write error !\n");
        }
    }

    va_end(va);

    return result;
}

void nglDumpQuad(nglQuad *Quad)
{
    nglHostPrintf(h_sceneDump(), "\n");
    nglHostPrintf(h_sceneDump(), "QUAD\n");
    auto *v1 = Quad->m_tex;

    const char *v2 = "[none]";
    if (v1 != nullptr) {
        v2 = v1->field_60.field_4;
    }

    nglHostPrintf(h_sceneDump(), "  TEXTURE %s\n", v2);
    nglHostPrintf(h_sceneDump(), "  BLEND %d %d\n", Quad->field_58, Quad->field_5C);
    nglHostPrintf(h_sceneDump(), "  MAPFLAGS 0x%x\n", Quad->field_54);
    nglHostPrintf(h_sceneDump(), "  Z %f\n", Quad->field_50.f);

    for (auto i = 0u; i < 4u; ++i) {
        nglHostPrintf(h_sceneDump(),
                      "  VERT %f %f 0x%08X %f %f\n",
                      Quad->field_0[i].pos.x,
                      Quad->field_0[i].pos.y,
                      Quad->field_0[i].m_color,
                      Quad->field_0[i].uv.field_0,
                      Quad->field_0[i].uv.field_4);
    }

    nglHostPrintf(h_sceneDump(), "ENDQUAD\n");
}

void nglDumpMesh(nglMesh *Mesh, const math::MatClass<4, 3> &a2, nglMeshParams *MeshParams)
{
    if constexpr (1)
    {
        if ((Mesh->Flags & NGLMESH_SCRATCH_MESH) == 0)
        {
            nglHostPrintf(h_sceneDump(), "\n");
            nglHostPrintf(h_sceneDump(),
                          "MESHFILE %s  // Path: %s\n",
                          Mesh->File->FileName.to_string(),
                          Mesh->File->FilePath);
            nglHostPrintf(h_sceneDump(), "\n");
            nglHostPrintf(h_sceneDump(), "MODEL %s\n", Mesh->Name->to_string());

            if (MeshParams != nullptr && (MeshParams->Flags & NGLP_SCALE) != 0)
            {
                nglHostPrintf(h_sceneDump(),
                              "  SCALE %f %f %f\n",
                              MeshParams->Scale[0],
                              MeshParams->Scale[1],
                              MeshParams->Scale[2]);
            }

            nglHostPrintf(h_sceneDump(), "  ROW1 %f %f %f %f\n", a2[0][0], a2[0][1], a2[0][2], 0.0f);
            nglHostPrintf(h_sceneDump(), "  ROW2 %f %f %f %f\n", a2[1][0], a2[1][1], a2[1][2], 0.0f);
            nglHostPrintf(h_sceneDump(), "  ROW3 %f %f %f %f\n", a2[2][0], a2[2][1], a2[2][2], 0.0f);
            nglHostPrintf(h_sceneDump(), "  ROW4 %f %f %f %f\n", a2[3][0], a2[3][1], a2[3][2], 1.f);

            if (MeshParams != nullptr)
            {
                if ((MeshParams->Flags & 0x3C) != 0)
                {
                    nglHostPrintf(h_sceneDump(), "  NBONES %d\n", MeshParams->NBones);

                    for (auto i = 0; i < MeshParams->NBones; ++i)
                    {
                        auto *v6 = MeshParams->field_8;
                        nglHostPrintf(
                            h_sceneDump(),
                            "  BONE %d %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f\n",
                            i,
                            v6[i][0][0],
                            v6[i][0][1],
                            v6[i][0][2],
                            0.0f,
                            v6[i][1][0],
                            v6[i][1][1],
                            v6[i][1][2],
                            0.0f,
                            v6[i][2][0],
                            v6[i][2][1],
                            v6[i][2][2],
                            0.0f,
                            v6[i][3][0],
                            v6[i][3][1],
                            v6[i][3][2],
                            1.f);
                    }
                }
            }

            nglHostPrintf(h_sceneDump(), "ENDMODEL\n");
        }

    }
    else
    {
        CDECL_CALL(0x007825A0, Mesh, &a2, MeshParams);
    }
}

void nglSetClearColor(Float a1, Float a2, Float a3, Float a4)
{
    nglCurScene()->ClearColor = color {a1, a2, a3, a4};
}

void nglListEndScene() {
    nglCurScene() = nglCurScene()->field_30C;
}

void nglDestroyDebugMeshes() {
#if 0
    nglDestroyMesh(nglDebugMesh_SolidBox);
    nglDestroyMesh(nglDebugMesh_WireframeBox);
    nglDestroyMesh(nglDebugMesh_Sphere);
    nglDestroyMesh(nglDebugMesh_Pyramid);
#else
    CDECL_CALL(0x0077F040);
#endif
}

void nglSetRenderTarget(nglTexture *a1)
{
    nglCurScene()->field_334 = a1;
    nglCurScene()->field_8 = 6;
}

nglTexture *nglGetBackBufferTex() {
    return nglBackBufferTex();
}

void nglCreateDebugMeshes() {
    CDECL_CALL(0x00780260);
}

static Var<IDirect3DDevice9 *> dword_987520{0x00987520};
static Var<IDirect3DVertexBuffer9 *> dword_987524{0x00987524};

static Var<int> dword_987534{0x00987534};

void sub_81E8E0(int Length)
{
    dword_987520()->lpVtbl->CreateVertexBuffer(dword_987520(),
                                               Length,
                                               D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY,
                                               0,
                                               D3DPOOL_DEFAULT,
                                               &dword_987524(),
                                               nullptr);
    dword_987534() = Length;
}

#include "float.h"

//FIXME
void nglInit(HWND hWnd)
{
    TRACE("nglInit");

    if constexpr (0)
    {
        _controlfp(0x300u, 0x300u);
        _controlfp(0x20000u, 0x30000u);
        if (!struct_972688().field_B) {
            int a1 = 0;
            CDECL_CALL(0x0076E320, &a1);
        }

        create_renderer(hWnd);
        CDECL_CALL(0x00782930);

        g_Direct3DDevice()->lpVtbl->GetDeviceCaps(g_Direct3DDevice(), &g_deviceCaps());

        sub_7740F0();
        sub_77B740();
        auto v1 = (double) struct_972688().m_width;
        if (struct_972688().m_width < 0) {
            v1 = v1 + 4.2949673e9;
        }

        struct_972688().field_34 = v1 * 0.0015625;
        auto v2 = (double) struct_972688().m_height;
        if (struct_972688().m_height < 0) {
            v2 = v2 + 4.2949673e9;
        }

        struct_972688().field_38 = v2 * 0.0020833334;
        nglDebugInit();
        nglMeshInit();

        sp_log("Setting up the global renderstates...\n");
        g_renderState().Init();
        int v3 = 0;

        auto *v4 = &SamplerStates()[0][5];
        for (int i = 0; i < 132; i += 33)
        {
            if (TextureStageStates()[i + 1] != 1) {
                g_Direct3DDevice()->lpVtbl->SetTextureStageState(g_Direct3DDevice(),
                                                                 v3,
                                                                 D3DTSS_COLOROP,
                                                                 1);
                TextureStageStates()[i + 1] = 1;
            }

            if (TextureStageStates()[i + 4] != 1) {
                g_Direct3DDevice()->lpVtbl->SetTextureStageState(g_Direct3DDevice(),
                                                                 v3,
                                                                 D3DTSS_ALPHAOP,
                                                                 1);
                TextureStageStates()[i + 4] = 1;
            }

            if (TextureStageStates()[i + 24]) {
                g_Direct3DDevice()->lpVtbl->SetTextureStageState(g_Direct3DDevice(),
                                                                 v3,
                                                                 D3DTSS_TEXTURETRANSFORMFLAGS,
                                                                 0);
                TextureStageStates()[i + 24] = 0;
            }

            if (v4[1] != 2)
            {
                g_Direct3DDevice()->lpVtbl->SetSamplerState(g_Direct3DDevice(),
                                                            v3,
                                                            D3DSAMP_MINFILTER,
                                                            D3DTEXF_LINEAR);
                v4[1] = 2;
            }

            if (*v4 != 2) {
                g_Direct3DDevice()->lpVtbl->SetSamplerState(g_Direct3DDevice(),
                                                            v3,
                                                            D3DSAMP_MAGFILTER,
                                                            D3DTEXF_LINEAR);
                *v4 = 2;
            }

            if (v4[2] != 2) {
                g_Direct3DDevice()->lpVtbl->SetSamplerState(g_Direct3DDevice(),
                                                            v3,
                                                            D3DSAMP_MIPFILTER,
                                                            D3DTEXF_LINEAR);
                v4[2] = 2;
            }

            ++v3;
            v4 += 14;
        }

        sub_7726B0(1);
        nglTextureInit();
        tlInitListInit();
        if (!g_Direct3DDevice()->lpVtbl->CreateQuery(g_Direct3DDevice(),
                                                     D3DQUERYTYPE_OCCLUSION,
                                                     nullptr))
        {
            static Var<IDirect3DQuery9 *> dword_972660{0x00972660};

            g_Direct3DDevice()->lpVtbl->CreateQuery(g_Direct3DDevice(),
                                                    D3DQUERYTYPE_OCCLUSION,
                                                    &dword_972660());
        }

        create_front_and_back_buffer_tex();

        if (s_d3dpresent_params().BackBufferCount != static_cast<uint32_t>(-1)) {
            for (auto v7 = 0u; v7 < s_d3dpresent_params().BackBufferCount + 1; ++v7) {
                auto *v8 = nglGetBackBufferTex();
                SetRenderTarget(v8, nullptr, 0, 6);
                g_Direct3DDevice()->lpVtbl->Clear(g_Direct3DDevice(), 0, nullptr, 7u, 0, 1.0, 0);
                g_Direct3DDevice()->lpVtbl->Present(g_Direct3DDevice(),
                                                    nullptr,
                                                    nullptr,
                                                    nullptr,
                                                    nullptr);
            }
        }

        sub_771B60();
        sub_781980(256, 256);

        dword_987520() = g_Direct3DDevice();
        if (!EnableShader())
        {
            D3DXCreateTextureFromFileW(g_Direct3DDevice(),
                                       L"data\\packs\\celshading.dat",
                                       bit_cast<IDirect3DTexture9 **>(&celshadingTex()));
            D3DXCreateTextureFromFileW(g_Direct3DDevice(),
                                       L"data\\packs\\celshadingSolid.dat",
                                       bit_cast<IDirect3DTexture9 **>(&celshadingSolidTex()));

            switch (g_TOD()) {
            case 0: {
                const WCHAR *v9 = L"data\\packs\\water_day.dat";
                D3DXCreateTextureFromFileW(g_Direct3DDevice(), v9, &water_texture());
                break;
            }
            case 1:
                D3DXCreateTextureFromFileW(g_Direct3DDevice(),
                                           L"data\\packs\\water_night.dat",
                                           &water_texture());
                break;
            case 2:
                D3DXCreateTextureFromFileW(g_Direct3DDevice(),
                                           L"data\\packs\\water_rainy.dat",
                                           &water_texture());
                break;
            case 3: {
                const WCHAR *v9 = L"data\\packs\\water_sunset.dat";
                D3DXCreateTextureFromFileW(g_Direct3DDevice(), v9, &water_texture());
                break;
            }
            default:
                break;
            }

            static Var<int> dword_9562E0{0x009562E0};
            dword_9562E0() = g_TOD();
            g_player_shadows_enabled() = false;
            sub_81E8E0(2465792);
        }

        static Var<nglVertexBuffer> dword_956558{0x00956558};
        nglVertexBuffer::createIndexOrVertexBuffer(&dword_956558(),
                                                            ResourceType::VertexBuffer,
                                                            819200,
                                                            520,
                                                            0,
                                                            D3DPOOL_DEFAULT);
        memset(SamplerStates(), 255u, sizeof(SamplerStates()));
        memset(TextureStageStates(), 255u, sizeof(TextureStageStates()));

    }
    else
    {
        CDECL_CALL(0x0076E3E0, hWnd);
    }
}

nglVertexDef_MultipassMesh<nglVertexDef_PCUV_Base> *sub_507920(
    nglMaterialBase *a1, int a2, int a3, int a4, const void *a5, int a6, bool a7)
{
    return (nglVertexDef_MultipassMesh<nglVertexDef_PCUV_Base> *) CDECL_CALL(0x00507920, a1, a2, a3, a4, a5, a6, a7);
}


bool sub_578420(unsigned int a1)
{
    nglCreateMesh(0x40000u, a1, 0, nullptr);
    return nglScratch() != nullptr;
}

void sub_57F3C0()
{
    CDECL_CALL(0x0057F3C0);
}

void aeps_Init() {
    CDECL_CALL(0x004DDDC0);

    nglCreateDebugMeshes();
}

void sub_76DF40() {
    CDECL_CALL(0x0076DF40);

    nglDestroyDebugMeshes();
}

void sub_782030()
{
    CDECL_CALL(0x00782030);
}

void sub_81E910()
{
    CDECL_CALL(0x0081E910);
}

void nglRenderTextureState::setSamplerState(
        int stage,
        uint8_t a3,
        uint32_t a4)
{
    if constexpr (0)
    {
        if ( (a3 & 2) != 0 )            // Linear + Point
        {
            if ( this->field_20[0][stage] == 2 )
            {
                nglSetSamplerState(stage, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
                nglSetSamplerState(stage, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
                nglSetSamplerState(stage, D3DSAMP_MIPFILTER, D3DTEXF_POINT);
                this->field_20[0][stage] = 2;
            }
        }
        else if ( (a3 & 4) != 0 )       // Trilinear
        {
            if ( this->field_20[0][stage] == 4 )
            {
                nglSetSamplerState(stage, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
                nglSetSamplerState(stage, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
                nglSetSamplerState(stage, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
                this->field_20[0][stage] = 4;
            }
        }
        else if ((a3 & 8) != 0)         // Anisotropic
        {
            if ( this->field_20[0][stage] != 8 )
            {
                nglSetSamplerState(stage, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
                nglSetSamplerState(stage, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
                nglSetSamplerState(stage, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
                this->field_20[0][stage] = 8;
            }

            auto MaxAnisotropy = a4;
            if ( a4 > g_deviceCaps().MaxAnisotropy ) {
                MaxAnisotropy = g_deviceCaps().MaxAnisotropy;
            }

            nglSetSamplerState(stage, D3DSAMP_MAXANISOTROPY, MaxAnisotropy);
        }
        else if ( this->field_20[0][stage] != 1 )   // Point Filter
        {
            nglSetSamplerState(stage, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
            nglSetSamplerState(stage, D3DSAMP_MINFILTER, D3DTEXF_POINT);
            nglSetSamplerState(stage, D3DSAMP_MIPFILTER, D3DTEXF_POINT);
            this->field_20[0][stage] = 1;
        }
    }
    else
    {
        THISCALL(0x00401E00, this, stage, a3, a4);
    }
}

void ngl_patch()
{
    ngl_dx_shader_patch();

    SET_JUMP(0x0076B8C0, nglSetWorldToViewMatrix);

    REDIRECT(0x0041517C, xform_inv);

    {
        HRESULT (*func)(nglMeshSection *) = &nglSetStreamSourceAndDrawPrimitive;
        SET_JUMP(0x00771AF0, func);

    }

    REDIRECT(0x0076D44F, sub_77EBD0);

    //FIXME
    if constexpr (nglLoadMeshFileInternal_hook)
    {
        REDIRECT(0x0056BDAA, nglLoadMeshFileInternal);
        REDIRECT(0x0056C126, nglLoadMeshFileInternal);
        REDIRECT(0x0056C244, nglLoadMeshFileInternal);
        REDIRECT(0x0076FF90, nglLoadMeshFileInternal);
        REDIRECT(0x007700D9, nglLoadMeshFileInternal);
        REDIRECT(0x00778649, nglLoadMeshFileInternal);
    }

    {
        auto *func = &nglRenderList::nglOpaqueCompare<nglRenderNode>;
        REDIRECT(0x0077D162, func);
    }

    {
        //auto *func = &nglRenderList::nglTransCompare<nglRenderNode>;
        //REDIRECT(0x0077D1D4, func);
    }

    REDIRECT(0x0077D0F2, nglVif1SetupScene);

    {
        REDIRECT(0x0052B3B3, nglCalculateMatrices);
        REDIRECT(0x0052B49F, nglCalculateMatrices);
        REDIRECT(0x0053ACA4, nglCalculateMatrices);
        REDIRECT(0x0053D79F, nglCalculateMatrices);
        REDIRECT(0x0053DB64, nglCalculateMatrices);
        REDIRECT(0x0054E31B, nglCalculateMatrices);
        REDIRECT(0x0054E4EA, nglCalculateMatrices);
        REDIRECT(0x005B27D6, nglCalculateMatrices);
        REDIRECT(0x0060BFE2, nglCalculateMatrices);
        REDIRECT(0x0060C0BD, nglCalculateMatrices);
        REDIRECT(0x0060D749, nglCalculateMatrices);
        REDIRECT(0x006191D2, nglCalculateMatrices);
        REDIRECT(0x00629D99, nglCalculateMatrices);
        REDIRECT(0x00635B86, nglCalculateMatrices);
        REDIRECT(0x00735441, nglCalculateMatrices);
        REDIRECT(0x007368BF, nglCalculateMatrices);
        REDIRECT(0x007369CC, nglCalculateMatrices);
        REDIRECT(0x0073AB00, nglCalculateMatrices);
        REDIRECT(0x0073D37A, nglCalculateMatrices);
        REDIRECT(0x0073D4D3, nglCalculateMatrices);
        REDIRECT(0x0073DD65, nglCalculateMatrices);
        REDIRECT(0x0076B941, nglCalculateMatrices);
        REDIRECT(0x0076BAA7, nglCalculateMatrices);
        REDIRECT(0x0076C3A2, nglCalculateMatrices);
        REDIRECT(0x0076C66D, nglCalculateMatrices);
        REDIRECT(0x007703F1, nglCalculateMatrices);
        REDIRECT(0x00779C51, nglCalculateMatrices);
        REDIRECT(0x0077B026, nglCalculateMatrices);
        REDIRECT(0x0077D0E4, nglCalculateMatrices);
    }

    //REDIRECT(0x0041C704, nglDxSetTexture);

    SET_JUMP(0x0077A870, nglLoadTextureTM2);

    SET_JUMP(0x00507690, FastListAddMesh);

    REDIRECT(0x004F9BB3, nglListAddMesh);

    SET_JUMP(0x0076C970, nglListBeginScene);

    SET_JUMP(0x0076B6D0, nglSetViewport);
    
    ngl_lighting_patch();

    {
        nglTexture * (* func)(const tlFixedString &) = &nglGetTexture;
        SET_JUMP(0x00773230, func);
    }

    SET_JUMP(0x007730B0, nglSetTextureDirectory);

    //SET_JUMP(0x0076C400, nglSetDefaultSceneParams);

    //SET_JUMP(0x0076C700, nglSetupScene);

    {
        FUNC_ADDRESS(address, &nglQuadNode::Render);
        set_vfunc(0x008B9FB4, address);
    }

    {
        auto address = func_address(&nglQuadNode::Render);
        SET_JUMP(0x00783670, address);
    }
    REDIRECT(0x0076EA59, nglRenderPerfInfo);

    {
        FUNC_ADDRESS(address, &nglStringNode::Render);
        set_vfunc(0x0088EBB4, address);
    }

    SET_JUMP(0x0076E750, nglSetFrameLock);

    SET_JUMP(0x00773350, nglCanReleaseTexture);

    SET_JUMP(0x0076E050, nglListInit);

    SET_JUMP(0x0076EA10, nglListSend);

    SET_JUMP(0x0076E980, nglFlip);

    REDIRECT(0x0077392C, nglInitWhiteTexture);

    REDIRECT(0x0076E598, nglTextureInit);

    REDIRECT(0x005AD218, nglInit);

    SET_JUMP(0x0077AB30, nglConstructTexture);

    SET_JUMP(0x007791A0, create_and_parse_fdf);


    {
        void (*func)(nglFont *Font, char *, uint32_t *, uint32_t *a4, Float a5, Float a6) = nglGetStringDimensions;
        //SET_JUMP(0x007798E0, func);
    }

    {
        set_vfunc(0x00922924, ngl_readfile_callback);
    }

    {
        nglMesh * (*func)(const tlFixedString &, nglMeshFile *) = &nglGetMeshInFile;
        REDIRECT(0x00637F8B, func);
        REDIRECT(0x0076FD55, func);
    }

    REDIRECT(0x0076F727, nglRebaseMesh);

    {
        REDIRECT(0x0064302D, nglLoadMeshFile);
    }


    {
        nglTexture *(*func)(const tlFixedString &) = &nglLoadTexture;
        REDIRECT(0x004100D0, func);
    }

    {
        REDIRECT(0x0076F873, nglVertexBuffer::createIndexOrVertexBuffer);
    }

    {
        FUNC_ADDRESS(address, &nglVertexBuffer::createIndexBufferAndWriteData);
        REDIRECT(0x0076F814, address);
    }

    {
        auto func = &nglVertexBuffer::createVertexBufferAndWriteData;
        FUNC_ADDRESS(address, func);
        REDIRECT(0x0076F9B0, address);
        REDIRECT(0x0076F9E9, address);
    }

    us_outline_patch();

    return;

    SET_JUMP(0x0076DF00, sub_76DF00);

    SET_JUMP(0x0076D680, create_renderer);

    SET_JUMP(0x005A02B0, ngl_memalloc_callback);

    SET_JUMP(0x007724A0, nglCreateVertexDeclarationAndShader);

#if 0

    REDIRECT(0x0076E0CC, nglSceneDumpStart);

    


    REDIRECT(0x005B86CD, nglSaveTexture);
    REDIRECT(0x007731E2, nglSaveTexture);
    REDIRECT(0x00773210, nglSaveTexture);

    {
        FUNC_ADDRESS(address, &nglMeshSection::internal::createVertexBufferAndWriteData);
        REDIRECT(0x0076F9B0, address);
        REDIRECT(0x0076F9E9, address);
    }

    {
        REDIRECT(0x005502BE, nglGetNextMeshInFile);
        REDIRECT(0x0055034A, nglGetNextMeshInFile);
    }

    REDIRECT(0x0076EA59, nglRenderPerfInfo);

    

    {
        FUNC_ADDRESS(address, &nglMeshSection::internal::createIndexBufferAndWriteData);
        REDIRECT(0x0076F814, address);
    }

    REDIRECT(0x00771B3C, sub_7719D0);

    REDIRECT(0x0077A809, sub_783080);

    REDIRECT(0x00783177, sub_782FE0);

    {
        REDIRECT(0x0041EA44, sub_772270);
        REDIRECT(0x0041EBAA, sub_772270);
        REDIRECT(0x0041ECA0, sub_772270);
        REDIRECT(0x0041EE26, sub_772270);
    }

    REDIRECT(0x00772432, hookD3DXAssembleShader);

    REDIRECT(0x00403B5C, nglCreatePShader);

    

    {
        FUNC_ADDRESS(address, &USBuildingSimpleShader::Register);
        set_vfunc(0x008709E4, address);
    }

    {
        FUNC_ADDRESS(address, &nglDebugShader::Register);
        set_vfunc(0x008BCDEC, address);
    }

    {
        FUNC_ADDRESS(address, &PCUV_Shader::Register);
        set_vfunc(0x00870AA0, address);
    }

    {
        FUNC_ADDRESS(address, &FrontEnd_Shader::Register);
        set_vfunc(0x008714E8, address);
    }

    {
        FUNC_ADDRESS(address, &USPersonShaderSpace::USPersonShader::Register);
        set_vfunc(0x008717DC, address);
    }

    {
        FUNC_ADDRESS(address, &USPersonShaderSpace::USPersonSolidShader::Register);
        set_vfunc(0x00871808, address);
    }

    {
        REDIRECT(0x0076E590, sub_7726B0);
        REDIRECT(0x0076E955, sub_7726B0);
    }

    {
        //REDIRECT(0x005AD2DF, aeps_Init);

        //REDIRECT(0x005AD5EA, sub_76DF40);
    }

    REDIRECT(0x0054B474, send_shadow_projectors);

    us_street_patch();

    us_person_patch();

    us_pcuv_patch();
#endif
}

#ifdef OPENUSM_XBPACK_MODE
void ngl_xbpack_patch()
{
    REDIRECT(0x0056BDAA, nglLoadMeshFileInternal);
    REDIRECT(0x0056C126, nglLoadMeshFileInternal);
    REDIRECT(0x0056C244, nglLoadMeshFileInternal);
    REDIRECT(0x0076FF90, nglLoadMeshFileInternal);
    REDIRECT(0x007700D9, nglLoadMeshFileInternal);
    REDIRECT(0x00778649, nglLoadMeshFileInternal);
}
#endif