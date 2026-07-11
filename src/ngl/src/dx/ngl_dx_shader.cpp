#include "ngl_dx_shader.h"

#include <common.h>
#include <func_wrapper.h>
#include <log.h>
#include <matrix4x3.h>
#include <mstring.h>
#include <ngl.h>
#include <ngl_mesh.h>
#include <trace.h>
#include <variables.h>
#include <vtbl.h>

#include <cassert>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include <list.hpp>

#include <d3dx9shader.h>

Var<IDirect3DVertexDeclaration9 *[1]> dword_9738E0 { 0x009738E0 };

Var<_std::list<void *>> g_pixelShaderList{0x00972B10};

//0x007CA2E8
int __stdcall hookD3DXAssembleShader(const char *data,
                                     UINT data_len,
                                     const D3DXMACRO *defines,
                                     ID3DXInclude *include,
                                     DWORD flags,
                                     ID3DXBuffer **shader,
                                     ID3DXBuffer **error_messages) {
    //sp_log("%s %d", a1, a2);

    auto result = STDCALL(0x007CA2E8, data, data_len, defines, include, flags, shader, error_messages);

    return result;
}

int CreatePixelShader(IDirect3DPixelShader9 **a1, const DWORD *a2)
{
    if constexpr (0)
    {}
    else
    {
        return CDECL_CALL(0x00772500, a1, a2);
    }
}

void nglCreateVertexDeclarationAndShader(void *a1, const D3DVERTEXELEMENT9 *a2, const DWORD *a3)
{
    if constexpr (1)
    {
        struct {
            IDirect3DVertexShader9 *field_0;
            IDirect3DVertexDeclaration9 *field_4;
        } *v1 = static_cast<decltype(v1)>(a1);

        g_Direct3DDevice()->lpVtbl->CreateVertexDeclaration(g_Direct3DDevice(), a2, &v1->field_4);
        g_Direct3DDevice()->lpVtbl->CreateVertexShader(g_Direct3DDevice(), a3, &v1->field_0);

        auto *v3 = g_vertexShaderList().m_head;
        assert(v3 != nullptr);

        auto *v4 = (decltype(v3)) THISCALL(0x00772C60,
                                           &g_vertexShaderList(),
                                           g_vertexShaderList().m_head,
                                           g_vertexShaderList().m_head->_Prev,
                                           &v1->field_0);
        THISCALL(0x00772CE0, &g_vertexShaderList(), 1u);
        v3->_Prev = v4;
        v4->_Prev->_Next = v4;

    }
    else
    {
        CDECL_CALL(0x007724A0, a1, a2, a3);
    }
}

const DWORD *g_codes = nullptr;

void nglCreateVShader(const D3DVERTEXELEMENT9 *a2, void *a1, [[maybe_unused]] BOOL a3, const char *SrcCode, ...)
{
    va_list va;
    va_start(va, SrcCode);

    char Work[4096];
    vsprintf(Work, SrcCode, va);

    assert(SrcCode != nullptr);

    static const auto NGL_VS_PRECODE = "vs.1.1\n";
    assert(strlen(Work) + strlen(NGL_VS_PRECODE) + 1 < 4096 &&
           "String overflow in nglCreateVShader !");

    char Dest[4096];
    strcpy(Dest, NGL_VS_PRECODE);

    auto v4 = strlen(Work) + 1;
    char *v5 = Dest;
    v5 = v5 - 1;
    while (*++v5) {
        ;
    }

    std::memcpy(v5, Work, v4);

    ID3DXBuffer *pShader = nullptr;

    if (D3DXAssembleShader(Dest, strlen(Dest), nullptr, nullptr, 2u, &pShader, nullptr) != D3D_OK)
    {
        sp_log("Cannot assemble vertex shader. \n%s\n", Dest);

        assert(0);
    }

    auto *v7 = static_cast<const DWORD *>(pShader->lpVtbl->GetBufferPointer(pShader));
    g_codes = v7;

    nglCreateVertexDeclarationAndShader(a1, a2, v7);

    va_end(va);
}

int size_codes(const DWORD *code)
{
    int size = 0;

    while (code[size] != 0xFFFF) {
        ++size;
    }

    return size;
}

bool compare_codes(const DWORD *code0, const DWORD *code1, int size)
{
    {
        auto size_code0 = size_codes(code0);
        auto size_code1 = size_codes(code1);

        //sp_log("%d", size_code0);
        //sp_log("%d", size_code1);

        assert(size_code0 == size_code1);
    }

    for (int i = 0; i < size; ++i)
    {
        auto c0 = code0[i];
        auto c1 = code1[i];

        if (c0 != c1) {
            sp_log("Codes not equal!: %d 0x%08X 0x%08X", i, c0, c1);
            return false;
        }
    }

    return true;
}

void log_codes(const DWORD *code) {
    mString str;

    int i = 0;

    while (code[i] != 0xFFFF) {
        str += mString{0, "%08x\n", code[i]};

        ++i;
    }

    sp_log("\n%s", str.c_str());
}

const DWORD *AssemblePShader(const char *text) {
    ID3DXBuffer *pShader = nullptr;

    if (D3DXAssembleShader(text,
                           strlen(text),
                           nullptr,
                           nullptr,
                           D3DXSHADER_USE_LEGACY_D3DX9_31_DLL,
                           &pShader,
                           nullptr) != D3D_OK) {
        sp_log("Cannot assemble pixel shader. \n%s\n", text);

        assert(0);
    }

    auto *result = static_cast<const DWORD *>(pShader->lpVtbl->GetBufferPointer(pShader));
    return result;
}

std::vector<DWORD> CompilePShader(const char *file_name)
{
    ID3DXBuffer *pShader = nullptr;
    ID3DXBuffer *error_messages = nullptr;

    const char *profile = "ps_1_1";

    if (D3DXCompileShaderFromFile(file_name,
                                  nullptr,
                                  nullptr,
                                  "main",
                                  profile,
                                  D3DXSHADER_USE_LEGACY_D3DX9_31_DLL,
                                  &pShader,
                                  &error_messages,
                                  nullptr) != D3D_OK) {
        sp_log("%s",
               static_cast<const char *>(error_messages->lpVtbl->GetBufferPointer(error_messages)));

        error_messages->lpVtbl->Release(error_messages);

        assert(0);
    }

    assert(error_messages == nullptr);

    auto buffer_size = pShader->lpVtbl->GetBufferSize(pShader);
    auto *buffer = pShader->lpVtbl->GetBufferPointer(pShader);

    std::vector<DWORD> result(buffer_size / 4);

    std::memcpy(result.data(), buffer, buffer_size);

    pShader->lpVtbl->Release(pShader);

    return result;
}

static constexpr auto MAX_BONES = 64u;

void nglSetupVShaderBonesDX(int a5, nglMeshNode *MeshNode, nglMeshSection *Section)
{
    TRACE("nglSetupVShaderBonesDX");

    if constexpr (0)
    {
        static constexpr auto BONES_SCALE = 3.0f;
        static const float BONES_OFFSET = a5;
        float a2[4] {BONES_SCALE, BONES_OFFSET, 1.0, 1.0};

        g_Direct3DDevice()->lpVtbl->SetVertexShaderConstantF(g_Direct3DDevice(), 90u, a2, 1u);

        static Var<matrix4x3[MAX_BONES]> g_boneMatrices {0x00972B20};

        auto *meshParams = MeshNode->field_90;
        if ( (meshParams->Flags & 4) != 0 )
        {
            for ( int i = 0; i < Section->NBones; ++i )
            {
                assert(i < MAX_BONES && "nglSetupVShaderBonesDX: too many bones ! Increase the MAX_BONES value.");

                auto boneIdx = Section->BonesIdx[i];
                auto v9 = MeshNode->sub_4199D0();
                matrix4x4 arg4;

                /*
                struct {
                    matrix4x3 *field_0;
                    matrix4x3 *field_4;
                } v29 {&meshParams->field_8[boneIdx], v9};
                auto *v27 = (const math::VecClass__3_1 *)(v8 + LODWORD(a2[1]));
                sub_770F30(&arg4, (matrix4x4 **)&v25);
                sub_414360(&out, v27 + 3, a3);
                sub_414360((math::VecClass__3_1 *)&arg4.m.arr[3], &out, v9);
                */

                matrix4x3 v10 = sub_413770(arg4);
                std::memcpy(&g_boneMatrices()[i], &v10, sizeof(v10));
            }
        }
        else if ( (meshParams->Flags & 8) != 0 )
        {
            for ( int i = 0; i < Section->NBones; ++i )
            {
                assert(i < MAX_BONES && "nglSetupVShaderBonesDX: too many bones ! Increase the MAX_BONES value.");

                auto boneIdx = Section->BonesIdx[i];
                matrix4x4 arg4;
                /*
                v14 = (const math::VecClass__3_1 *)(v13 + LODWORD(a2[1]));
                v15 = (const math::MatClass__4_3 *)(v13 + LODWORD(a2[2]));
                v25 = (const math::VecClass__3_1 **)(v13 + LODWORD(a2[1]));
                v26 = (const math::MatClass__4_3 *)(v13 + LODWORD(a2[2]));
                sub_770EB0(&arg4, (float **)&v25);
                sub_414360((math::VecClass__3_1 *)&arg4.m.arr[3], v14 + 3, v15);
                */
                matrix4x3 v16 = sub_413770(arg4);
                std::memcpy(&g_boneMatrices()[i], &v16, sizeof(v16));
            }
        }
        else if ( (meshParams->Flags & 0x10) == 0 )
        {
            for ( int i = 0; i < Section->NBones; ++i )
            {
                assert(i < MAX_BONES && "nglSetupVShaderBonesDX: too many bones ! Increase the MAX_BONES value.");

                auto boneIdx = Section->BonesIdx[i];
                matrix4x4 arg4 {};
                memcpy(&arg4, &meshParams->field_8[boneIdx], sizeof(arg4));

                
                matrix4x3 v20 = sub_413770(arg4);
                std::memcpy(&g_boneMatrices()[i], &v20, sizeof(v20));
            }
        }
        else
        {
            for ( int i = 0; i < Section->NBones; ++i )
            {
                assert(i < MAX_BONES && "nglSetupVShaderBonesDX: too many bones ! Increase the MAX_BONES value.");

                bit_cast<matrix4x4 *>(&g_boneMatrices()[i])->sub_415740(nullptr);
            }
        }

        g_Direct3DDevice()->lpVtbl->SetVertexShaderConstantF(g_Direct3DDevice(), a5, &g_boneMatrices()[0][0].x, 3 * Section->NBones);
    }
    else
    {
        CDECL_CALL(0x00772810, a5, MeshNode, Section);
    }

    if constexpr (1)
    {
        matrix4x3 tmp [2];

        g_Direct3DDevice()->lpVtbl->GetVertexShaderConstantF(g_Direct3DDevice(),
                                                             11,
                                                             &tmp[0][0].x,
                                                             3 * 2);

        //sp_log("%s", tmp[0].to_string());
        //sp_log("%s", tmp[1].to_string());

        float f[4] {};
        g_Direct3DDevice()->lpVtbl->GetVertexShaderConstantF(g_Direct3DDevice(), 90u, f, 1u);

        //sp_log("%f %f %f %f", f[0], f[1], f[2], f[3]);
    }
}

void nglSetVertexDeclarationAndShader(VShader *a1) {
    TRACE("SetVertexDeclarationAndShader");

    g_Direct3DDevice()->lpVtbl->SetVertexDeclaration(g_Direct3DDevice(), a1->field_4);
    g_Direct3DDevice()->lpVtbl->SetVertexShader(g_Direct3DDevice(), a1->field_0);
}

void SetPixelShader(IDirect3DPixelShader9 **a1) {
    g_Direct3DDevice()->lpVtbl->SetPixelShader(g_Direct3DDevice(), *a1);
}

std::vector<DWORD> CompileVShader(const char *file_name, const D3DXMACRO *defines)
{
    ID3DXBuffer *pShader = nullptr;
    ID3DXBuffer *error_messages = nullptr;

    const char *profile = "vs_1_1";

    if (D3DXCompileShaderFromFile(file_name,
                                  defines,
                                  nullptr,
                                  "main",
                                  profile,
                                  D3DXSHADER_USE_LEGACY_D3DX9_31_DLL,
                                  &pShader,
                                  &error_messages,
                                  nullptr) != D3D_OK)
    {
        sp_log("%s",
               static_cast<const char *>(error_messages->lpVtbl->GetBufferPointer(error_messages)));

        error_messages->lpVtbl->Release(error_messages);
        assert(0);
    }

    auto buffer_size = pShader->lpVtbl->GetBufferSize(pShader);
    auto *buffer = pShader->lpVtbl->GetBufferPointer(pShader);

    //sp_log("buffer_size = %d", buffer_size);

    //std::unique_ptr<DWORD> out = new DWORD[buffer_size / 4];

    std::vector<DWORD> result(buffer_size / 4);
    std::memcpy(result.data(), buffer, buffer_size);

    pShader->lpVtbl->Release(pShader);
    if (error_messages != nullptr) {
        error_messages->lpVtbl->Release(error_messages);
    }

    return result;
}

const char *disassemble_shader(const DWORD *pShader) {
    ID3DXBuffer *pDisassembly;

    if (D3DXDisassembleShader(pShader, FALSE, nullptr, &pDisassembly) != D3D_OK) {
        assert(0);
    }

    auto *disBuf = (const char *) pDisassembly->lpVtbl->GetBufferPointer(pDisassembly);

    return disBuf;
}

void nglCreatePShader(IDirect3DPixelShader9 **a3, const char *SrcCode, ...) {
    assert(SrcCode != nullptr);

    static const char *NGL_PS_PRECODE = "ps.1.1\n";

    assert((strlen(SrcCode) + strlen(NGL_PS_PRECODE) + 1 < 4096) &&
           "String overflow in nglCreatePShader.");

    if constexpr (0)
    {
        char Dest[4096];

        va_list Args;
        va_start(Args, SrcCode);
        vsprintf(Dest, SrcCode, Args);

        char a1a[4096];
        strcpy(a1a, NGL_PS_PRECODE);
        auto v3 = strlen(Dest) + 1;

        char *v4 = a1a;
        v4 = v4 - 1;
        while (*++v4) {
            ;
        }

        std::memcpy(v4, Dest, v3);

        ID3DXBuffer *pShader = nullptr;
        if (auto hr = D3DXAssembleShader(a1a, &a1a[strlen(a1a) + 1] - &a1a[1], nullptr, nullptr, 2u, &pShader, nullptr);
                hr != D3D_OK)
        {
            sp_log("Cannot assemble pixel shader.\n");
            assert(0);
        }

        auto &v6 = g_Direct3DDevice()->lpVtbl;

        assert(pShader != nullptr);

        auto *v7 = static_cast<const DWORD *>(pShader->lpVtbl->GetBufferPointer(pShader));
        g_codes = v7;

        IDirect3DPixelShader9 *v2;
        v6->CreatePixelShader(g_Direct3DDevice(), v7, &v2);

        auto *v8 = g_pixelShaderList().m_head;
        auto *v9 = (decltype(v8)) THISCALL(0x00772C60,
                                           &g_pixelShaderList(),
                                           g_pixelShaderList().m_head,
                                           g_pixelShaderList().m_head->_Prev,
                                           a3);
        THISCALL(0x00772CE0, &g_pixelShaderList(), 1u);
        v8->_Prev = v9;
        v9->_Prev->_Next = v9;

        va_end(Args);
    }
    else
    {
        CDECL_CALL(0x007723A0, a3, SrcCode);
    }
}

struct D3DXTOKEN {
    int field_0;
    int field_4;
    union {
        const char *str;
        uint32_t u;
        float f;
    } field_8;
    BOOL is_scalar_register;
    union {
        const char *str;
        uint32_t u;
    } field_10;
    int field_14;
    int *field_18;
    int field_1C;
};

VALIDATE_SIZE(D3DXTOKEN, 0x20);

enum NodeType {
    List = 1,
    AsmInstruction = 18,
    AsmRegister = 19,
};

struct CNode {
    int m_vtbl;
    uint32_t m_node_type;
    CNode *field_8;
    CNode *m_nextNode;
};

struct CNodeToken : CNode {
    D3DXTOKEN field_10;
};

VALIDATE_SIZE(CNodeToken, 0x30);

struct CNodeAsmRegister : CNode {
    int field_10;
    uint32_t field_14;
    uint32_t m_index;
    int field_1C;
    uint32_t m_swizzling;
    int field_24;
    CNode *field_28;
};

VALIDATE_SIZE(CNodeAsmRegister, 0x2C);

const char * to_string(CNodeAsmRegister *node)
{
    static char str [1000] {};

    snprintf(str, sizeof(str) - 1,
            "field_10 = 0x%08X\n"
            "field_14 = 0x%08X\n"
            "m_index = 0x%08X\n"
            "field_1C = 0x%08X\n"
            "m_swizzling = 0x%08X\n"
            "field_24 = 0x%08X",
            node->field_10,
            node->field_14,
            node->m_index,
            node->field_1C,
            node->m_swizzling,
            node->field_24);

    return str;
}


struct CNodeAsmInstruction : CNode {
    D3DXTOKEN field_10;
    uint32_t field_30;
    uint32_t field_34;
    int field_38;
    CNodeAsmRegister *m_dest_reg;
    CNodeToken *field_40;
    CNodeAsmRegister *m_src_reg;
    CNodeToken *field_48;
    int field_4C;
    int field_50;
    int field_54;
    int field_58;
    int field_5C;
};

VALIDATE_SIZE(CNodeAsmInstruction, 0x60);

const char * to_string(CNodeAsmInstruction *node)
{
    static char str [1000] {};

    snprintf(str, sizeof(str) - 1,
            "field_30 = 0x%08X\n"
            "field_34 = 0x%08X\n"
            "field_38 = 0x%08X\n",
            node->field_30,
            node->field_34,
            node->field_38);

    return str;
}

struct CNodeList : CNode {
    const char *field_10;
};

VALIDATE_SIZE(CNodeList, 0x14);

struct CTErrors;

struct CAssembler {
    CTErrors *field_0;
    char field_4[0x30];
    CNodeList *field_34;
    int m_version;
    int field_3C;
    int field_40;
    uint32_t m_dest_write_modifier;
    int field_48;
    int field_4C;
    int field_50;
    int field_54;
    DWORD *m_data;
    uint32_t m_size;
    int field_60;
    int field_64;
    int field_68;
    const char *field_6C;
    int field_70;
    int field_74;
    void *field_78;
    void *field_7C;

    int Validate(D3DXTOKEN *a2)
    {
        return THISCALL(0x007E51D3, this, a2);
    }

    int Assure(unsigned int a2)
    {
        return THISCALL(0x007E5228, this, a2);
    }
};

VALIDATE_SIZE(CAssembler, 0x80);

int __fastcall CAssembler_EmitInstruction(CAssembler *self, void *, CNodeAsmInstruction *token)
{
    TRACE("CAssembler::EmitInstruction");

    int result;
    sp_log("%d", self->m_size);
    sp_log("%u", token->field_30);

    if constexpr (1)
    {
        token->field_58 = 4 * self->m_size;
        auto v4 = token->field_30;
        int a2a = 1;
        bool v43 = false;
        if ( v4 == 31 ) {
            a2a = 2;
        }

        if ( self->field_78 )
        {
            if ( v4 == 26 || v4 == 25 || v4 == 28 || v4 == 30 )
            {
                //D3DXShader::CTErrors::Error(self->field_0, &token->field_10, 0x7E9u, aCallCallnzLabe);
                return 0x80004005;
            }

            if ( v4 == 20 || v4 == 21 || v4 == 22 || v4 == 23 || v4 == 24 )
            {
                auto *v5 = token->field_48;
                if ( v5 != nullptr )
                {
                    if ( !v5->field_10.field_0 )
                    {
                        //D3DXShader::CTErrors::Error(self->field_0, &token->field_10, 0x7EAu, aMatricesCannot);
                        return 0x80004005;
                    }
                }
            }
        }

        auto *v7 = token->m_dest_reg;
        if ( v7 != nullptr && v7->m_node_type == AsmRegister )
        {
            ++a2a;
            if ( v7->field_10 == -1 ) {
                v43 = true;
            }

            if ( v7->field_14 != 0 )
            {
                //D3DXShader::CTErrors::Error(self->field_0, &token->field_10, 0x7DEu, aSourceModifier);
                self->field_4C = 1;
            }

            auto *v8 = bit_cast<CNodeToken *>(v7->field_28);
            if ( v8 != nullptr )
            {
                auto m_version = self->m_version;
                if ( m_version < 4 || m_version > 5 )
                {
                    //D3DXShader::CTErrors::Error(self->field_0, &token->field_10, 0x7D8u, aRelativeAddres);
                    self->field_4C = 1;
                }
                else
                {
                    if ( v8->field_10.field_0 == -1 ) {
                        v43 = true;
                    }

                    if ( v8->field_10.field_18 != nullptr )
                    {
                        //D3DXShader::CTErrors::Error(self->field_0, &token->field_10, 0x7D9u, aOnlyOneAddress);
                        self->field_4C = 1;
                    }
                    ++a2a;
                }
            }
        }

        auto *v10 = token->field_40;
        if ( v10 != nullptr && v10->m_node_type == AsmRegister )
        {
            auto v11 = self->m_version;
            if ( v11 < 2 || (v11 >= 6 && v11 <= 10) )
            {
                //D3DXShader::CTErrors::Error(self->field_0, &token->field_10, 0x7E5u, aPredicatesAreN);
                self->field_4C = 1;
            }

            auto *v12 = token->field_40;
            ++a2a;
            if ( v12->field_10.field_0 == -1 ) {
                v43 = true;
            }

            auto v13 = v12->field_10.field_4;
            if ( v13 != 0 && v13 != 0xD000000 )
            {
                //D3DXShader::CTErrors::Error(self->field_0, &token->field_10, 0x7E3u, aSourceModifier_0);
                self->field_4C = 1;
            }

            if ( v12->field_10.field_18 )
            {
                //D3DXShader::CTErrors::Error(self->field_0, &token->field_10, 0x7E4u, aRelativeAddres_0);
                self->field_4C = 1;
            }
        }

        
        auto **v14 = &token->m_src_reg;
        for ( int tokena = 0; tokena < 4; ++tokena )
        {
            auto *v15 = *v14;
            if ( v15 == nullptr ) {
                break;
            }

            auto v16 = token->field_30;
            ++a2a;
            if ( v16 != 81 && v16 != 48 && v16 != 47 )
            {
                if ( v15->field_10 == -1 ) {
                    v43 = true;
                }

                auto *v17 = bit_cast<CNodeToken *>(v15->field_28);
                if ( v17 != nullptr )
                {
                    if ( v17->field_10.field_0 == -1 ) {
                        v43 = true;
                    }

                    if ( v17->field_10.field_18 != nullptr )
                    {
                        //D3DXShader::CTErrors::Error(self->field_0, &token->field_10, 0x7D9u, aOnlyOneAddress);
                        self->field_4C = 1;
                    }

                    if ( self->m_version != 0 ) {
                        ++a2a;
                    }
                }
            }

            ++v14;
        }

        auto func = [](auto *self, auto *token, int a2a, uint32_t v42, bool v43, int v18)
        {
            if ( token->field_54 ) {
                v18 |= 0x40000000u;
            }

            if ( token->field_40 ) {
                v18 |= 0x10000000u;
            }

            auto v24 = self->m_version;
            if ( (v24 >= 1 && v24 <= 5) || (v24 >= 10 && v24 <= 14) ) {
                v18 |= (a2a - 1) << 24;
            }

            auto v25 = token->field_30;
            if ( v25 == 41 || v25 == 45 || v25 == 94 ) {
                v18 |= (token->field_38 & 7) << 16;
            }

            sp_log("opcode of instruction = 0x%08X", v18);
            self->m_data[self->m_size++] = v18;
            auto m_size = self->m_size;
            if ( token->field_30 == 31 )
            {
                self->m_data[m_size] = token->field_38 | 0x80000000;
                m_size = ++self->m_size;

                sp_log("add code");
            }

            auto *v27 = token->m_dest_reg;
            if ( v27 != nullptr && v27->m_node_type == AsmRegister )
            {
                sp_log("destination register");
                if ( self->m_version == 0 && v27->m_swizzling == 0xF0000 )
                {
                    auto v28 = token->field_30;
                    if ( v28 == 21 || v28 == 23 )
                    {
                        v27->m_swizzling = 0x70000;
                    }
                    else if ( v28 == 24 )
                    {
                        v27->m_swizzling = 0x30000;
                    }
                }

                sp_log("%s", to_string(v27));

                sp_log("%s", to_string(token));
                
                {
                    sp_log("0x%08X", 0x01000000 & 0xF0000);
                    sp_log("0x%08X", 0x01000000 & 0x7FF);
                    sp_log("0x%08X", 0x01000000 & 0xFF00000);
                    sp_log("0x%08X", 0x01000000 & 0x18);
                    sp_log("0x%08X", (0x00000003 | 0xFFFFFFF8) << 20);
                }


                uint32_t v29 = (v27->m_swizzling & 0xF0000)
                            | (v27->m_index & 0x7FF)
                            | (token->field_34 & 0xFF00000)
                            | (((v27->field_10 & 0x18) | ((v27->field_10 | 0xFFFFFFF8) << 20)) << 8);
                if ( v27->field_28 != nullptr ) {
                    v29 |= 0x2000u;
                }

                self->m_data[self->m_size++] = v29;
                sp_log("add code 0x%08X", v29);

                auto *v30 = bit_cast<CNodeAsmRegister *>(v27->field_28);
                m_size = self->m_size;
                if ( v30 != nullptr )
                {
                    sp_log("%s", to_string(v27));

                    auto code = (v30->field_24 & 0xFF0000)
                                    | (v30->m_index & 0x7FF)
                                    | (v30->field_14 & 0xF000000)
                                    | (((v30->field_10 & 0x18) | ((v30->field_10 | 0xFFFFFFF8) << 20)) << 8);
                    sp_log("reg = 0x%08X", code);
                    self->m_data[m_size] = code;
                    m_size = ++self->m_size;
                    sp_log("add code");
                }
            }

            auto *v31 = token->field_40;
            if ( v31 != nullptr && v31->m_node_type == AsmRegister )
            {
                self->m_data[m_size] = (v31->field_10.field_14 & 0xFF0000)
                                        | (v31->field_10.field_8.u & 0x7FF)
                                        | (v31->field_10.field_4 & 0xF000000)
                                        | (((v31->field_10.field_0 & 0x18) | ((v31->field_10.field_0 | 0xFFFFFFF8) << 20)) << 8);
                m_size = ++self->m_size;

                sp_log("add code");
            }

            auto v32 = token->field_30;
            switch ( v32 )
            {
            case 81: {
                auto **v33 = bit_cast<CNodeToken **>(&token->m_src_reg);
                for ( int tokenb = 0; tokenb < 4; ++tokenb )
                {
                    auto *v34 = *v33;
                    if ( v34 == nullptr ) {
                        break;
                    }

                    auto v35 = v34->field_10.field_0;
                    if ( v35 >= 5 && v35 <= 8 )
                    {
                        float a2c = v34->field_10.field_8.f;
                        self->m_data[m_size] = bit_cast<uint32_t>(a2c);
                        sp_log("add code");
                    }

                    ++self->m_size;
                    m_size = self->m_size;
                    ++v33;

                }
                break;
            }
            case 48: {
                auto **v36 = bit_cast<CNodeToken **>(&token->m_src_reg);
                for ( int v49 = 0; v49 < 4; ++v49 )
                {
                    auto *v37 = *v36;
                    if ( v37 == nullptr ) {
                        break;
                    }

                    if ( v37->field_10.field_0 == 2 || v37->field_10.field_0 == 4 ) {
                        self->m_data[m_size] = v37->field_10.field_8.u;

                        sp_log("add code");
                    }

                    ++self->m_size;
                    m_size = self->m_size;
                    ++v36;
                }
                break;
            }
            case 47: {
                self->m_data[m_size] = bit_cast<CNodeToken *>(token->m_src_reg)->field_10.field_8.str != nullptr;
                ++self->m_size;

                sp_log("add code");
                break;
            }
            default: {
                sp_log("source registers");
                auto **a2b = &token->m_src_reg;
                for (int tokend = 0; tokend < 4; ++tokend )
                {
                    auto *v38 = a2b[tokend];
                    if ( v38 == nullptr )
                        break;

                    if ( self->m_version == 0 && v38->field_24 == 0xE40000 )
                    {
                        auto v39 = token->field_30;
                        if ( v39 == 6 || v39 == 7 || v39 == 14 || v39 == 78 || v39 == 15 || v39 == 79 ) {
                            v38->field_24 = 0xFF0000;
                        }
                    }

                    sp_log("%s", to_string(v38));

                    auto v40 = (v38->field_24 & 0xFF0000)
                                | (v38->m_index & 0x7FF)
                                | (v38->field_14 & 0xF000000)
                                | (((v38->field_10 & 0x18) | ((v38->field_10 | 0xFFFFFFF8) << 20)) << 8);
                    if ( v38->field_28 != nullptr ) {
                        v40 |= 0x2000u;
                    }

                    sp_log("add_code: 0x%08X", v40);

                    self->m_data[self->m_size++] = v40;
                    auto *v41 = bit_cast<CNodeAsmRegister *>(v38->field_28);
                    if ( v41 != nullptr )
                    {
                        if ( self->m_version != 0 )
                        {
                            auto code = (v41->field_24 & 0xFF0000)
                                            | (v41->m_index & 0x7FF)
                                            | (v41->field_14 & 0xF000000)
                                            | (((v41->field_10 & 0x18) | ((v41->field_10 | 0xFFFFFFF8) << 20)) << 8);
                            self->m_data[self->m_size++] = code;
                        }
                        else if ( v41->field_10 != 3
                                    || v41->field_14 != 0
                                    || v41->m_index != 0
                                    || v41->field_24 != 0 )
                        {
                            //D3DXShader::CTErrors::Error(self->field_0, &token->field_10, 0x7D7u, aOnlyA0XIsAllow);
                            self->field_4C = 1;
                        }
                    }
                }
                break;
            }
            }

            if ( self->m_size != v42 )
            {
                //D3DXShader::CTErrors::Error(self->field_0, &token->field_10, 0, aInternalErrorI);
                self->field_4C = 1;
            }

            if ( v43 )
            {
                self->field_64 = self->m_size;
            }
            else if ( self->Validate(&token->field_10) < 0 )
            {
                self->field_50 = 1;
            }

            return 0;
        };

        result = self->Assure(a2a);
        if ( result >= 0 )
        {
            auto v18 = token->field_30;
            auto v42 = a2a + self->m_size;
            if ( v18 != 3 ) {
                return func(self, token, a2a, v42, v43, v18);
            }

            int v19 = self->m_version;
            if ( (unsigned int)v19 >= 6 && (v19 < 10 || v19 > 14) ) {
                return func(self, token, a2a, v42, v43, v18);
            }

            auto *v20 = token->field_48;

            sp_log("0x%X", v20->field_10.field_4);

            int v21 = v20->field_10.field_4;
            int v22 = 0x7000000;
            v18 = 2;
            if ( v21 > 0x7000000 )
            {
                if ( v21 != 0x8000000 )
                {
                    switch ( v21 )
                    {
                    case 0x9000000:
                    case 0xA000000: {
                        //D3DXShader::CTErrors::Error(self->field_0, &token->field_10, 0x7DDu, aSourceModifier_1);
                        self->field_4C = 1;
                        return func(self, token, a2a, v42, v43, v18);
                        break;
                    }
                    default:
                        break;
                    }

                    v22 = 0xB000000;
                    if ( v21 == 0xB000000 )
                    {
                        v20->field_10.field_4 = 0xC000000;
                        return func(self, token, a2a, v42, v43, v18);
                    }

                    if ( v21 != 0xC000000 )
                    {
                        bool v23 = v21 == 0xD000000;
                        if ( !v23 )
                        {
                            return func(self, token, a2a, v42, v43, v18);
                        }

                        //D3DXShader::CTErrors::Error(self->field_0, &token->field_10, 0x7DDu, aSourceModifier_1);
                        self->field_4C = 1;
                        return func(self, token, a2a, v42, v43, v18);
                    }
                }
            }
            else
            {
                switch ( v21 )
                {
                case 0x7000000u:
                    v20->field_10.field_4 = 0x8000000;
                    return func(self, token, a2a, v42, v43, v18);
                case 0u:
                    v20->field_10.field_4 = 0x1000000;
                    return func(self, token, a2a, v42, v43, v18);
                case 0x1000000u:
                    v20->field_10.field_4 = 0;
                    return func(self, token, a2a, v42, v43, v18);
                }

                v22 = 0x2000000;
                if ( v21 == 0x2000000 )
                {
                    v20->field_10.field_4 = 0x3000000;
                    return func(self, token, a2a, v42, v43, v18);
                }

                if ( v21 != 50331648 )
                {
                    v22 = 0x4000000;
                    if ( v21 == 0x4000000 )
                    {
                        v20->field_10.field_4 = 0x5000000;
                        return func(self, token, a2a, v42, v43, v18);
                    }

                    if ( v21 != 0x5000000 )
                    {
                        bool v23 = v21 == 0x6000000;
                        if ( !v23 )
                        {
                            return func(self, token, a2a, v42, v43, v18);
                        }
                    }
                }

            }

            v20->field_10.field_4 = v22;
            return func(self, token, a2a, v42, v43, v18);
        }
    }
    else
    {
        int (__fastcall *func)(CAssembler *, void *, void *) = CAST(func, 0x007E52E2);
        result = func(self, nullptr, token);
    }

    sp_log("%d", self->m_size);
    sp_log("0x%X", self->m_data[self->m_size - 1]);

    return result;
}

void * __fastcall CNodeAsmRegister_constructor(
        CNodeAsmRegister *self,
        void *,
        unsigned int a2,
        unsigned int a3,
        unsigned int idx,
        int a5,
        CNode *a6)
{
    TRACE("CNodeAsmRegister::CNodeAsmRegister");

    THISCALL(0x007F04A3, self, AsmRegister);
    self->field_10 = a2;
    self->field_14 = a3;
    self->m_index = idx;
    self->field_1C = a5;
    self->field_28 = a6;
    self->m_vtbl = 0x008E1818;
    self->m_swizzling = 0xF0000;
    self->field_24 = 0xE40000;

    return self;
}

void __fastcall CAssembler_Production(CAssembler *self, void *,
                                    unsigned int a2,
                                    unsigned int a3)
{
    TRACE("CAssembler::Production", std::to_string(a2).c_str(), std::to_string(a3).c_str());

    switch (a2)
    {
    case 3:
        sp_log("ADD");
        break;
    case 9:
        sp_log("EMIT_INSTRUCTION");
        break;
    case 60:
        sp_log("CREATE_ASM_INSTRUCTION");
        break;
    default:
        break;
    }

    void (__fastcall *func)(void *self, void *,
                                    unsigned int,
                                    unsigned int) = CAST(func, 0x007E6C9B);

    func(self, nullptr, a2, a3);
}

int __fastcall CAssembler_Token(CAssembler *self)
{
    TRACE("CAssembler::Token");
    auto result = THISCALL(0x007E661B, self);

    sp_log("%d", result);
    return result;
}

void * __fastcall CAssembler_DecodeRegister(
        void *self, void *,
        D3DXTOKEN *token,
        void *a3)
{
    TRACE("CAssembler::DecodeRegister");
    sp_log("%s", token->field_8);


    void * (__fastcall *func)(
            void *self, void *,
            D3DXTOKEN *token,
            void *a3) = CAST(func, 0x007E66F9);
    return func(self, nullptr, token, a3);
}

int __fastcall CAssembler_DecodeOpcode(CAssembler *self, void *, D3DXTOKEN *a2)
{
    TRACE("CAssembler::DecodeOpcode", a2->field_8.str);
    auto result = THISCALL(0x007E4771, self, a2);

    return result;
}

BOOL d3dxasm_parse()
{
    TRACE("d3dxasm_parse");

    return CDECL_CALL(0x007E7248);
}

int d3dxtok_parse()
{
    TRACE("d3dxtok_parse");

    return CDECL_CALL(0x007E2567);
}

int __fastcall CPreProcessor_GetToken(
        void *self, void *,
        D3DXTOKEN *token)
{
    TRACE("CPreProcessor::GetToken");

    return THISCALL(0x007E2AFB, self, token);
}

struct CTErrors {
    char *field_0;
    uint32_t field_4;
    uint32_t m_numError;
    int GetErrorCount()
    {
        return this->m_numError;
    }
};

struct CPreProcessor {
    char field_0[0x18];
    CTErrors field_18;
};

int __fastcall CAssembler_Assemble(
        CAssembler *self, void *,
        CPreProcessor *a3,
        unsigned int a4,
        void *a5,
        void **shader)
{
    TRACE("CAssembler::Assemble");

    auto result = THISCALL(0x007E782F, self, a3, a4, a5, shader);

    if (self->field_0->GetErrorCount() != 0)
    {
        sp_log("%s", self->field_0->field_0 + sizeof(int));
    }

    return result;
}


void ngl_dx_shader_patch()
{
    REDIRECT(0x007E2BBF, d3dxtok_parse);

    REDIRECT(0x007E7BFE, d3dxasm_parse);

    REDIRECT(0x007CA35C, CAssembler_Assemble);

    REDIRECT(0x007E66E5, CAssembler_DecodeOpcode);

    REDIRECT(0x007E663B, CPreProcessor_GetToken);

    REDIRECT(0x007E6C26, CNodeAsmRegister_constructor);

    REDIRECT(0x007E6D70, CAssembler_EmitInstruction);

    {
        REDIRECT(0x007E78F2, CAssembler_Token);
        REDIRECT(0x007E7298, CAssembler_Token);
        REDIRECT(0x007E75E2, CAssembler_Token);
    }

    REDIRECT(0x007E7586, CAssembler_Production);
    {
        REDIRECT(0x007E6FC6, CAssembler_DecodeRegister);
    }
} 
