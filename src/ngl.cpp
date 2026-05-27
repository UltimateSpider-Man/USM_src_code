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

#include "game.h"

#include <psapi.h>

#include <d3dx9shader.h>

#if MOD_MESH_SUPPORT
#   include <assimp/Importer.hpp>
#   include <assimp/scene.h>
#   include <assimp/postprocess.h>
Mod* dbgReplaceMesh = nullptr;
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
// imports a mesh (by optional index) and creates buffers
// returns number of meshes found within the mesh itself

static inline float u32_as_float(uint32_t u)
{
    float f;
    std::memcpy(&f, &u, sizeof(u));
    return f;
}

static inline uint32_t pack_argb8(uint8_t a, uint8_t r, uint8_t g, uint8_t b)
{
    return (uint32_t(a) << 24) | (uint32_t(r) << 16) | (uint32_t(g) << 8) | uint32_t(b);
}

static inline uint8_t to_u8_sat(float v01)
{
    v01 = std::clamp(v01, 0.0f, 1.0f);
    return (uint8_t)std::lround(v01 * 255.0f);
}

static inline uint32_t get_vertex_color_or_white(const aiMesh* mesh, unsigned i)
{
    if (mesh->HasVertexColors(0) && mesh->mColors[0])
    {
        const aiColor4D& c = mesh->mColors[0][i]; // usually 0..1 floats
        return pack_argb8(to_u8_sat(c.a), to_u8_sat(c.r), to_u8_sat(c.g), to_u8_sat(c.b));
    }
}

// imports a mesh (by optional index) and creates buffers
// returns number of meshes found within the mesh itself
//
// If `sect` is provided, the section's bone palette (BonesIdx[]) is rebuilt
// as an identity mapping in Assimp bone declaration order. This makes the
// vertex bone-index stream we emit (which uses Assimp's mBones[] order)
// directly index the palette the GPU shader uses:
//
//      vertex bone idx i  ->  Section->BonesIdx[i]  ->  meshParams->field_8[skel_idx]
//
// For this to render correctly the FBX must have its bones in the same order
// as the original PCMESH skeleton (which is what the Blender PCMESH addon
// produces when it exports straight from a PCMESH/PCSKEL pair).
int modImportMesh(IDirect3DDevice9* dev, modGenericMesh& data, char* buf, size_t size, std::string shaderName, int meshIndex = 0, nglMeshSection* sect = nullptr)
{
    if (!buf || size == 0) return 0;

    Assimp::Importer importer;
    const aiScene* scene = nullptr;

    // Cap weights per vertex at 4 — that matches our packed vertex layout.
    importer.SetPropertyInteger(AI_CONFIG_PP_LBW_MAX_WEIGHTS, 4);

    // NOTE: aiProcess_JoinIdenticalVertices is intentionally OFF for skinned
    // meshes. It can renumber vertex IDs while we walk weights, which causes
    // the per-vertex influences[] table to desync from the actual buffer
    // ordering — that produces the floating "shrapnel" you see around skinned
    // characters when an FBX replacement is loaded.
    unsigned int loadFlags =
        aiProcess_Triangulate |
        aiProcess_GenSmoothNormals |
        aiProcess_CalcTangentSpace |
        aiProcess_LimitBoneWeights |
        aiProcess_ImproveCacheLocality |
        aiProcess_ConvertToLeftHanded |
        aiProcess_PopulateArmatureData;

    data.vertices.clear();
    data.indices.clear();
    data.vertexBuffer = nullptr;
    data.indexBuffer = nullptr;
    data.stride = 0;
    data.numIndices = 0;
    data.numVertices = 0;

    std::string ext = transformToLower(data.mod->Path.extension().string());
    if (ext != ".fbx")
        scene = importer.ReadFileFromMemory(buf, size, loadFlags);
    else
        scene = importer.ReadFile(data.mod->Path.string().c_str(), loadFlags);

    if (!scene || !scene->HasMeshes()) return 0;

    UINT stride = 16;
    if (shaderName.find("uslod") != std::string::npos)
    {
        stride = 16;
    }
    else if (shaderName.find("ussimpleprop") != std::string::npos)
    {
        stride = 24;
    }
    else if (shaderName.find("us_character") != std::string::npos ||
        shaderName.find("usperson") != std::string::npos ||
        shaderName.find("uspersonsolid") != std::string::npos)
    {
        stride = 64;
    }

    const bool hasFullVB = (stride == 64);
    const aiMesh* mesh = scene->mMeshes[0];
    if (hasFullVB && !meshIndex)
    {
        for (unsigned int idx = 0; idx < scene->mNumMeshes; ++idx)
        {
            const aiMesh* tmp = scene->mMeshes[idx];
            if (tmp && tmp->mNumVertices && tmp->mNumBones)
            {
                mesh = tmp;
                break;
            }
        }
    }
    else if (meshIndex != 0)
    {
        unsigned int pick = (meshIndex < (int)scene->mNumMeshes) ? (unsigned int)meshIndex : (scene->mNumMeshes - 1);
        mesh = scene->mMeshes[pick];
    }

    if (!mesh || mesh->mNumVertices == 0) return 0;

    // GPU constant table is sized for 64 bones (matches MAX_BONES in
    // ngl_dx_shader.cpp and conglom.cpp). Anything beyond that has nowhere
    // to go in the shader, so we ignore those bones rather than letting
    // garbage indices reach the GPU.
    constexpr unsigned int kMaxBones = 64u;
    const unsigned int boneCount = (mesh->mNumBones < kMaxBones) ? mesh->mNumBones : kMaxBones;

    struct Influence { uint16_t bone = 0; float w = 0.0f; };
    std::vector<std::array<Influence, 4>> influences;
    if (hasFullVB)
    {
        influences.resize(mesh->mNumVertices);
        for (auto& arr : influences)
            for (auto& inf : arr) inf = {};

        auto try_insert = [&](unsigned int vtx, uint16_t boneIndex, float weight)
            {
                if (vtx >= influences.size() || weight <= 0.0f) return;
                if (boneIndex >= kMaxBones) return; // never let an out-of-palette index reach the vertex buffer
                auto& a = influences[vtx];
                int weakest = 0;
                for (int i = 1; i < 4; ++i)
                    if (a[i].w < a[weakest].w) weakest = i;

                if (weight > a[weakest].w)
                    a[weakest] = { boneIndex, weight };
            };

        // Read all weights. The `boneIndex` we emit here is Assimp's mBones[]
        // ordering — which (for FBX exported from a PCMESH/PCSKEL via the
        // Blender addon) is the same ordering as the original skeleton.
        // The per-section palette rebuild below makes BonesIdx[i] == i so that
        // this index lands on the correct skeleton-bone matrix at draw time.
        for (unsigned int boneIndex = 0; boneIndex < boneCount; ++boneIndex)
        {
            const aiBone* bone = mesh->mBones[boneIndex];
            if (!bone) continue;

            for (unsigned int w = 0; w < bone->mNumWeights; ++w)
            {
                const aiVertexWeight& vw = bone->mWeights[w];
                try_insert(vw.mVertexId, (uint16_t)boneIndex, vw.mWeight);
            }
        }

        for (unsigned int v = 0; v < influences.size(); ++v)
        {
            auto& a = influences[v];
            std::sort(a.begin(), a.end(), [](const Influence& x, const Influence& y) { return x.w > y.w; });

            float sum = a[0].w + a[1].w + a[2].w + a[3].w;
            if (sum > 0.0f)
            {
                float inv = 1.0f / sum;
                for (auto& inf : a) inf.w *= inv;
            }
            else
            {
                a[0] = { 0, 1.0f };
                a[1] = { 0, 0.0f };
                a[2] = { 0, 0.0f };
                a[3] = { 0, 0.0f };
            }
        }

        // -------- Section bone-palette rebuild (the actual fix) --------
        // The shader reads vertex.bone_idx -> Section->BonesIdx[bone_idx]
        // -> meshParams->field_8[skel_idx].
        // We just wrote vertex bone indices in Assimp mBones[] order, so we
        // make the palette an identity mapping over those same N bones. This
        // means slot i resolves to skeleton bone i, which is what our FBX
        // (exported from the original PCMESH) already lines up with.
        if (sect && boneCount > 0)
        {
            uint16_t* newPalette = static_cast<uint16_t*>(tlMemAlloc(2u * boneCount, 16u, 0u));
            if (newPalette)
            {
                for (unsigned int i = 0; i < boneCount; ++i)
                    newPalette[i] = static_cast<uint16_t>(i);

                if (sect->BonesIdx)
                    tlMemFree(sect->BonesIdx);

                sect->BonesIdx = newPalette;
                sect->NBones   = static_cast<int>(boneCount);
            }
        }
        // ---------------------------------------------------------------
    }

    std::vector<float> vertices;
    std::vector<uint16_t> indices;

    vertices.reserve(mesh->mNumVertices * (stride / sizeof(float)));

    // fill vertex buffer
    for (unsigned int i = 0; i < mesh->mNumVertices; ++i)
    {
        const aiVector3D& pos = mesh->mVertices[i];
        vertices.push_back(pos.x);
        vertices.push_back(pos.y);
        vertices.push_back(pos.z);

        // ussimpleprop
        if (stride == 24)
        {
            // uv0
            if (mesh->HasTextureCoords(0))
            {
                const aiVector3D& uv = mesh->mTextureCoords[0][i];
                vertices.push_back(uv.x);
                vertices.push_back(uv.y);
            }
            else
            {
                vertices.push_back(0.0f);
                vertices.push_back(0.0f);
            }

            const uint32_t col = get_vertex_color_or_white(mesh, i);
            vertices.push_back(u32_as_float(col));
            continue;
        }

        if (!hasFullVB)
        {
            vertices.push_back(0.0f);
            continue;
        }

        // normal
        if (mesh->HasNormals())
        {
            const aiVector3D& n = mesh->mNormals[i];
            vertices.push_back(n.x);
            vertices.push_back(n.y);
            vertices.push_back(n.z);
        }
        else
        {
            vertices.push_back(0.0f);
            vertices.push_back(0.0f);
            vertices.push_back(0.0f);
        }

        // uv0
        if (mesh->HasTextureCoords(0))
        {
            const aiVector3D& uv = mesh->mTextureCoords[0][i];
            vertices.push_back(uv.x);
            vertices.push_back(uv.y);
        }
        else
        {
            vertices.push_back(0.0f);
            vertices.push_back(0.0f);
        }

        if (mesh->mNumBones && i < influences.size())
        {
            const auto& a = influences[i];

            // indices
            vertices.push_back((float)a[0].bone);
            vertices.push_back((float)a[1].bone);
            vertices.push_back((float)a[2].bone);
            vertices.push_back((float)a[3].bone);

            // weights
            vertices.push_back(a[0].w);
            vertices.push_back(a[1].w);
            vertices.push_back(a[2].w);
            vertices.push_back(a[3].w);
        }
        else
        {
            vertices.push_back(0.0f);
            vertices.push_back(0.0f);
            vertices.push_back(0.0f);
            vertices.push_back(0.0f);

            vertices.push_back(1.0f);
            vertices.push_back(0.0f);
            vertices.push_back(0.0f);
            vertices.push_back(0.0f);
        }
    }

    indices.reserve(mesh->mNumFaces * 3);
    for (unsigned int f = 0; f < mesh->mNumFaces; ++f)
    {
        const aiFace& face = mesh->mFaces[f];
        if (face.mNumIndices == 3)
        {
            indices.push_back((uint16_t)face.mIndices[0]);
            indices.push_back((uint16_t)face.mIndices[1]);
            indices.push_back((uint16_t)face.mIndices[2]);
        }
    }

    // create vertex buffer
    UINT vertexSize = (UINT)(vertices.size() * sizeof(float));
    auto device = g_Direct3DDevice()->lpVtbl;

    if (FAILED(device->CreateVertexBuffer(g_Direct3DDevice(), vertexSize, 0, 0, D3DPOOL_DEFAULT, &data.vertexBuffer, nullptr)))
        return 0;

    void* vbData = nullptr;
    if (FAILED(data.vertexBuffer->lpVtbl->Lock(data.vertexBuffer, 0, vertexSize, &vbData, 0)))
        return 0;

    memcpy(vbData, vertices.data(), vertexSize);
    data.vertexBuffer->lpVtbl->Unlock(data.vertexBuffer);

    // create index buffer
    UINT indexSize = (UINT)(indices.size() * sizeof(uint16_t));
    if (FAILED(device->CreateIndexBuffer(g_Direct3DDevice(), indexSize, 0, D3DFMT_INDEX16, D3DPOOL_DEFAULT, &data.indexBuffer, nullptr)))
        return 0;

    void* ibData = nullptr;
    if (FAILED(data.indexBuffer->lpVtbl->Lock(data.indexBuffer, 0, indexSize, &ibData, 0)))
        return 0;

    memcpy(ibData, indices.data(), indexSize);
    data.indexBuffer->lpVtbl->Unlock(data.indexBuffer);

    // update game meta
    data.vertices = std::move(vertices);
    data.indices = std::move(indices);
    data.stride = stride;
    data.numVertices = (UINT)(data.vertices.size() / (stride / sizeof(float)));
    data.numIndices = (UINT)data.indices.size();

    return (int)scene->mNumMeshes;
}


bool nglLoadMeshFileInternal(const tlFixedString &FileName, nglMeshFile *MeshFile, const char *ext)
{
    TRACE("nglLoadMeshFileInternal", FileName.to_string());

    if constexpr (1)
    {
#       if MOD_MESH_SUPPORT
            Mod* replacementMesh = getMod(MeshFile->FileName.m_hash, TLRESOURCE_TYPE_MESH_FILE);

            // @todo platform
            if (replacementMesh)
            {
                MeshFile->FileBuf.Buf = (char*)replacementMesh->Data.data();
                MeshFile->FileBuf.Size = replacementMesh->Data.size();
            }
            else
                replacementMesh = getMod(MeshFile->FileName.m_hash, TLRESOURCE_TYPE_MESH);

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

                // @todo: custom submeshes

                modGenericMesh modMesh;
                auto numCustomSubmeshes = 0;
                if (replacementMesh) {
                    modMesh.mod = replacementMesh;
                    numCustomSubmeshes = modImportMesh(g_Direct3DDevice(), modMesh, (char*)replacementMesh->Data.data(), replacementMesh->Data.size(), "", 0);

                    if (Mesh->NSections != numCustomSubmeshes)
                        printf("there are %d sections in the original mesh, but we have %d.\n", Mesh->NSections, numCustomSubmeshes);
                }


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

                    auto *v27 = MeshSection->m_indices;
                    if (v27 != nullptr) {
                        bit_cast<nglVertexBuffer *>(&MeshSection->m_indexBuffer)
                            ->createIndexBufferAndWriteData(v27, 2 * MeshSection->NIndices);
                    }

                    auto *v28 = MeshSection->Material;
                    MeshSection->StartIndex = 0;


                    tlFixedString v112 = v28->m_shader->GetName();
                    auto* v29 = v112.to_string();

#                   if MOD_MESH_DBG_REPLACE_ALL
                        if (!replacementMesh && dbgReplaceMesh)
                            replacementMesh = dbgReplaceMesh;
#                   endif
#                   if MOD_MESH_SUPPORT
                        if (replacementMesh && numCustomSubmeshes) 
                        {
                            if (modImportMesh(g_Direct3DDevice(), modMesh, (char*)replacementMesh->Data.data(), replacementMesh->Data.size(), v29, idx_Section, MeshSection)) {
                                nglVertexBuffer* vb = &MeshSection->field_3C;
                                vb->createVertexBufferAndWriteData(modMesh.vertices.data(), modMesh.vertices.size() * sizeof(float), 1028);
                                bit_cast<nglVertexBuffer*>(&MeshSection->m_indexBuffer)
                                    ->createIndexBufferAndWriteData(modMesh.indices.data(), modMesh.indices.size() * sizeof(uint16_t));

                                MeshSection->NVertices = modMesh.numVertices;
                                MeshSection->NIndices = modMesh.numIndices;
                                MeshSection->m_stride = modMesh.stride;
                                MeshSection->m_primitiveType = D3DPT_TRIANGLELIST;
                               // Mesh->NSections = idx_Section; // @todo: custom submeshes
                                continue; // skip
                            }
                        }
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
                }

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

            for (auto *Mesh = v67; Mesh != nullptr; Mesh = Mesh->NextMesh)
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
        std::copy(a1->Bones, a1->Bones + (newMesh->NBones << 6), newMesh->Bones);
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

#ifdef TARGET_XBOX
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
    auto *vtbl = bit_cast<int(*)[1]>(nglTextureDirectory()->m_vtbl);

    assert((*vtbl)[0] == 0x00560770);

    THISCALL(0x00560770, nglTextureDirectory(), 1, 0, 2);
}

void nglReleaseTexture(nglTexture *Tex) {
    TRACE("nglReleaseTexture");

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
        
        
        if (auto data = getModDataByHash(tex->field_60.m_hash)) {
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