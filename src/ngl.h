#pragma once

#include "fixedstring.h"
#include "hashstring.h"
#include "matrix4x4.h"
#include "tl_system.h"
#include "variable.h"

#include "nglrendernode.h"
#include "ngl_math.h"
#include "ngl_params.h"
#include "ngl_vertexdef.h"

#include <cstdint>

#include <d3d9.h>

#include <windows.h>

struct generic_mash_header;
struct generic_mash_data_ptrs;
struct nglScene;
struct nglMeshParams;
struct nglMesh;
struct nglVertexDef;
struct mash_info_struct;

extern uint8_t *nglGetDebugFlagPtr(const char *Flag);

extern uint8_t nglGetDebugFlag(const char *Flag);

extern void nglSetDebugFlag(const char *Flag, uint8_t Set);

enum nglFrameLockType {};

enum class FrameLockMode {
    Disabled = 0,
    Mode1 = 1,
    Mode2 = 2
};
struct nglFrameLockType2 {
    int field_0; // Frame lock mode (0, 1, or 2)
    bool enableVSync; // Enable vertical sync
    int frameRate; // Target frame rate
    int frameLockType; // Type of frame lock (e.g., adaptive, fixed)
    bool immediateLock; // Enable immediate frame lock
    int lockThreshold; // Threshold for frame lock

    // Add other fields as necessary
};
enum nglBlendModeType {
    NGLBM_OPAQUE = 0,               // No translucency is performed, alpha is ignored.
	NGLBM_PUNCHTHROUGH = 1,         // Similar to opaque, except if alpha is below a threshold the pixel is skipped.

	NGLBM_BLEND = 2,                // Blends the texel with the background, modulated by Alpha.
	NGLBM_ADDITIVE = 3,             // Adds the texel to the background, modulated by Alpha.
	NGLBM_SUBTRACTIVE = 4,          // Subtracts the texel from the background, modulated by Alpha.

	NGLBM_CONST_BLEND = 5,          // Blends the texel with the background, modulated by BlendModeConstant.
	NGLBM_CONST_ADDITIVE = 6,       // Adds the texel to the background, modulated by BlendModeConstant.
	NGLBM_CONST_SUBTRACTIVE = 7,    // Subtracts the texel from the background, modulated by BlendModeConstant.
    
    NGLBM_DESTALPHA_ADDITIVE = 8,   // Adds the texel to the background, modulated by Destination Alpha

    NGLBM_MAX_BLEND_MODES
};

enum nglTextureFileFormat {
};

struct nglPalette {
    uint32_t m_palette_idx;
    PALETTEENTRY *m_palette_entries;
    int m_size;

    int sub_782A70(int a2, int a3);

    void sub_782A40();
};

extern uint8_t NGLTEX_GET_FORMAT(uint32_t format);

struct nglTexture {
    nglTexture *field_0;
    nglTexture *field_4;
    int field_8;
    uint32_t m_format;
    D3DFORMAT m_d3d_format;
    nglTextureFileFormat field_14;
    uint32_t m_width;
    uint32_t m_height;
    uint32_t m_numLevel;
    D3DLOCKED_RECT field_24;
    int field_2C;
    uint8_t *field_30;
    uint32_t field_34;
    int field_38;

    IDirect3DTexture9 *DXTexture;

    IDirect3DSurface9 **DXSurfaces;
    nglTexture *field_44;
    nglPalette *field_48;
    int field_4C;
    int field_50;
    int field_54;
    uint32_t m_num_palettes;
    nglTexture **Frames;
    tlFixedString field_60;

    //0x00775000
    void CreateTextureOrSurface();

    void sub_774F20();
};

inline constexpr auto NGLP_SCALE = 0x2;

inline constexpr auto NGLP_NO_CULLING = 0x40;
inline constexpr auto NGLP_FORCE_LOD = 0x80;

extern Var<bool> g_valid_texture_format;

extern Var<unsigned int> nglTextureAnimFrame;

extern Var<nglTexture *> nglDefaultTex;

extern Var<tlInstanceBank> nglVertexDefBank;

enum nglBufferType {
};

struct nglShader;
struct nglMeshFile;

struct nglMaterialBase
{

#ifdef TARGET_XBOX
    tlHashString Name;
#else
    tlFixedString *Name;
#endif

    nglShader *m_shader;

    nglMeshFile *File;
    nglMaterialBase *NextMaterial;
    uint32_t Version;
    int field_14;

#ifdef TARGET_XBOX
    tlHashString field_18;
#else
    tlFixedString *field_18;
#endif

    nglTexture *field_1C;
    nglTexture *field_20;
    nglTexture *field_24;
    vector4d field_28;
    float field_38;
    int field_3C;
    int field_40;
    int field_44;
    int m_outlineFeature;
    int m_blend_mode;

    bool IsSwitchable();
};

extern nglMaterialBase *nglGetMaterialInFile(const tlFixedString &a1, nglMeshFile *MeshFile);

enum nglMatrixType {
    NGLMTX_VIEW_TO_WORLD = 0,
    NGLMTX_VIEW_TO_SCREEN = 1,
    NGLMTX_WORLD_TO_VIEW = 2,
    NGLMTX_WORLD_TO_SCREEN = 3,
};

struct nglMatrix {
    float arr[4][4];
};

struct nglFont;

struct nglStringNode
{
    std::intptr_t m_vtbl;
    int field_4;
    float field_8;
    unsigned char *field_C;
    nglFont *field_10;
    float field_14;
    float field_18;
    float field_1C;
    float field_20;
    float field_24;
    uint32_t m_color;

    nglStringNode();

    void * operator new(size_t size);

    void Render();
};

struct tlHashString;

enum ResourceType {
    VertexBuffer = 0,
    IndexBuffer = 1,
};

struct nglVertexBuffer 
{
    union {
        IDirect3DIndexBuffer9 *m_indexBuffer;
        struct
        {
            char *m_vertexData;
            uint32_t Size;
            IDirect3DVertexBuffer9 *m_vertexBuffer;
        };
    }; 

    //0x007707D0
    bool createIndexBufferAndWriteData(const void *a2, int a3);

    //0x00770760
    bool createVertexBufferAndWriteData(const void *a2, uint32_t size, int);

    //0x00754700
    bool createVertexBuffer(int size, uint32_t flags);

    //0x0077B440
    static HRESULT createIndexOrVertexBuffer(nglVertexBuffer *a1,
                                             ResourceType resource_type,
                                             int32_t size,
                                             uint32_t flags,
                                             uint32_t fvf,
                                             D3DPOOL a6);

    static void sub_77B5D0(nglVertexBuffer *a1, ResourceType a2);
};

struct nglMeshSection {
    tlFixedString *MaterialName;
    nglMaterialBase *Material;
    int NBones;
    uint16_t *BonesIdx;

    math::VecClass<3, 1> SphereCenter;
    float SphereRadius;
    uint32_t Flags;
    D3DPRIMITIVETYPE m_primitiveType;
    int NIndices;
    uint16_t *m_indices;
    IDirect3DIndexBuffer9 *m_indexBuffer;
    int NVertices;
    nglVertexBuffer field_3C;
    int m_stride;
    int field_4C;
    int field_50;
    nglVertexDef *VertexDef;
    int StartIndex;
    uint32_t field_5C;
};

extern void nglSetStreamSourceAndDrawPrimitive(
        D3DPRIMITIVETYPE a1,
        IDirect3DVertexBuffer9 *a2,
        uint32_t numVertices,
        uint32_t baseVertexIndex,
        uint32_t stride,
        IDirect3DIndexBuffer9 *a6,
        uint32_t numIndices,
        uint32_t startIndex);

//0x00771AF0
extern HRESULT nglSetStreamSourceAndDrawPrimitive(nglMeshSection *MeshSection);

enum class TypeDirectoryEntry : uint8_t
{
    MATERIAL = 1,
    MESH = 2,
    MORPH = 3,
};

const char *to_string(TypeDirectoryEntry type);

struct nglMorphSet;

struct nglDirectoryEntry {
    char field_0;
    char field_1;
    char field_2;
    TypeDirectoryEntry field_3;
    union {
        nglMaterialBase *Material;
        nglMesh *Mesh;
        nglMorphSet *Morph;
    } field_4;
    void *field_8;
};

struct nglPerfomanceInfo {
    int field_0;
    int field_4;
    int field_8;
    int field_C;
    int field_10;
    int field_14;
    float m_quads_time;
    float m_fonts_time;
    LARGE_INTEGER field_20;
    LARGE_INTEGER field_28;
    LARGE_INTEGER field_30;
    LARGE_INTEGER field_38;
    LARGE_INTEGER field_40;
    LARGE_INTEGER m_counterQuads;
    LARGE_INTEGER field_50;
    float m_fps;
    float field_5C;
    float field_60;
    float m_render_time;
    float m_cpu_time;
    float field_6C;
    float field_70;
    float field_74;
    float m_num_polys;
    float m_num_verts;
    float field_80;
    float field_84;
};

struct nglMorphFile {
    char field_0[4];
    int field_4[72];

    char *field_124;
    int field_128[7];

    int field_144;

    //0x00402090
    void un_mash_start(generic_mash_header *header, void *, generic_mash_data_ptrs *a3, void *);
};

extern Var<char[256]> nglMeshPath;

struct nglMorphSet;

struct nglMeshFile {
    tlFixedString FileName;
    char FilePath[256];
    int field_120;
    tlFileBuf FileBuf;
    bool field_130;
    int field_134;
    nglMesh *FirstMesh;
    nglMaterialBase *FirstMaterial;
    nglMorphSet *FirstMorph;
    int field_144;

    //0x00402050
    void un_mash_start(generic_mash_header *header, void *, generic_mash_data_ptrs *a3, void *);

    static tlFixedString *get_string(nglMeshFile *a1);
};

//0x0077F0C0
extern void nglDebugMesh_BuildBox(nglVertexDef_MultipassMesh<nglVertexDef_Debug_Base>::Iterator &a1,
                                  math::VecClass<3, 0> a2,
                                  math::VecClass<3, 0> a3);

//0x007730E0
extern void nglInitWhiteTexture();

extern matrix4x4 nglGetMatrix(nglMatrixType a2);

//0x00769F00
extern void nglGetProjectionParams(float *a1, float *nearz, float *farz);

//0x00773190
extern void nglSetTexturePath(const char *a1);

//0x007792B0
extern nglFont *nglLoadFont(const tlFixedString &a1);

//0x0076F140
extern nglMeshFile *nglLoadMeshFile(const tlFixedString &a1);

//0x0077C490
extern void nglReleaseSection(nglMeshSection *a1);

//0x00773250
extern nglTexture *nglGetTexture(uint32_t a1);

extern nglTexture *nglGetTexture(const tlFixedString &a1);

//0x007730B0
extern void nglSetTextureDirectory(tlResourceDirectory<nglTexture, tlFixedString> *a1);

//0x0076EF60
extern void nglSetMeshFileDirectory(tlResourceDirectory<nglMeshFile, tlFixedString> *a1);

//0x0076EF80
extern void nglSetMeshDirectory(tlResourceDirectory<nglMesh, tlHashString> *a1);

struct nglMaterialFile;

extern void nglSetMaterialFileDirectory(tlResourceDirectory<nglMaterialFile, tlFixedString> *a1);

extern void nglSetMaterialDirectory(tlResourceDirectory<nglMaterialBase, tlHashString> *a1);

struct nglMorphSet {
    tlHashString field_0;
    int field_4;
    int *field_8;
    nglMeshFile *field_C;
    nglMorphSet *field_10;
};

//0x0076EFA0
extern void nglSetMorphDirectory(tlResourceDirectory<nglMorphSet, tlHashString> *a1);

extern vector4d sub_411750(const vector4d &a2, const vector4d &a3);

//0x0076F500
bool nglLoadMeshFileInternal(const tlFixedString &name, nglMeshFile *MeshFile, const char *ext);

#ifdef OPENUSM_XBPACK_MODE
bool nglLoadMeshFileInternalXbox(const tlFixedString &name,
                                 nglMeshFile *MeshFile,
                                 const char *ext);
void ngl_xbpack_patch();
#endif

//0x0076F340
void nglRebaseMesh(uint32_t a1, uint32_t a2, nglMesh *Mesh);

//0x00778840
void nglProcessMorph(nglMeshFile *MeshFile, nglDirectoryEntry *a2, int base);

//0x0077C430
void nglRebaseSection(void *a1, void *a2, nglMeshSection *a3);

void sub_7726B0(bool a1);

//0x0076F170
extern bool nglCanReleaseMeshFile(nglMeshFile *a1);

//0x007786C0
extern bool nglCanReleaseMorphFile(nglMorphFile *a1);

//0x0076F0A0
nglMesh *nglGetMeshInFile(const tlFixedString &a1, nglMeshFile *a2);

struct nglQuad {
    struct Quad {
        struct {
            float x;
            float y;
        } pos;
        struct {
            float field_0;
            float field_4;
        } uv;
        uint32_t m_color;

    };
    Quad field_0[4];
    union {
        float f;
        nglTexture *tex;
    } field_50;
    int field_54;
    nglBlendModeType field_58;
    uint32_t field_5C;
    nglTexture *m_tex;
};

struct mNglQuad {
    struct {
        struct {
            float field_0;
            float field_4;
        } pos;
        struct {
            float field_0;
            float field_4;
        } uv;
        uint32_t m_color;

    } field_0[4];
    float field_50;
    int field_54;
    nglBlendModeType field_58;
    uint32_t field_5C;
    nglTexture *m_tex;

    void unmash(mash_info_struct *a2, void *a3);

    void custom_unmash(mash_info_struct *, void *);
};

struct nglQuadNode : nglRenderNode {
    nglQuad field_C;

    void * operator new(size_t size);

    //virtual
    //0x00783670
    void Render();
};

struct nglMeshNode {
    matrix4x4 field_0;
    matrix4x4 field_40;
    matrix4x4 *field_80;
    int field_84;
    nglMesh *field_88;
    nglParamSet<nglShaderParamSet_Pool> field_8C;
    nglMeshParams *field_90;
    float field_94;

    void * operator new(size_t size);

    matrix4x4 sub_41D840();

    matrix4x4 sub_4199D0();

    matrix4x4 sub_419930();
};

struct nglDebugStruct {
    uint8_t RenderSingleNode;
    int field_4;
    int field_8;
    int field_C;
    int field_10;
    int field_14;
    uint8_t ShowPerfInfo;
    uint8_t ShowPerfBar;
    uint8_t ScreenShot;
    uint8_t DisableQuads;
    uint8_t DisableFonts;
    uint8_t field_1D;
    uint8_t DisableVSync;
    uint8_t DisableScratch;
    uint8_t DebugPrints;
    uint8_t DumpFrameLog;
    uint8_t DumpSceneFile;
    uint8_t DumpTextures;
    uint8_t DrawLightSpheres;
    uint8_t DrawMeshSpheres;
    uint8_t DisableDuplicateMaterialWarning;
    uint8_t DisableMissingTextureWarning;
};



extern Var<nglDebugStruct> nglDebug;
extern Var<nglDebugStruct> nglSyncDebug;

extern Var<nglPerfomanceInfo> nglPerfInfo;
extern Var<nglPerfomanceInfo> nglSyncPerfInfo;

extern Var<int> nglFrame;

extern Var<nglTexture *> nglWhiteTex;

extern Var<int> nglScratchMeshPos;

inline Var<IDirect3DBaseTexture9 *> celshadingTex {0x0095635C};

inline Var<IDirect3DBaseTexture9 *> celshadingSolidTex {0x00956360};

inline Var<IDirect3DTexture9 *> water_texture {0x009562C0};

struct nglScratchBuffer_t
{
    nglVertexBuffer field_0[1];
    nglVertexBuffer field_C;
    int field_18[2];
    int16_t *field_20;
    int m_numVertices;
    int field_28;
    int field_2C;
    int field_30;
    int field_34;
    int field_38;
    int field_3C;
    int field_40;
    int field_44;
    IDirect3DIndexBuffer9 *field_48;
    nglVertexBuffer field_4C;

    int GetIndexCount() const
    {
        return this->field_30;
    }

    void IncreaseIndexCount(int a2)
    {
        this->field_30 += a2;
    }

    void SetCountVerts(int a2)
    {
        this->m_numVertices = a2;
    }

    int16_t * GetIndexBuffer()
    {
        return this->field_20;
    }

    int GetVertexOffset() const
    {
        return this->field_28;
    }
};

extern Var<nglScratchBuffer_t> nglScratchBuffer;

inline Var<void * (*) (int, int, int)> nglMeshAllocFn {0x00973B1C};

//0x0076F090
extern nglMesh *nglGetNextMeshInFile(nglMesh *a1);

//0x007756D0
extern void *nglMeshScratchAlloc(int Size, int Alignment, int);

//0x007756F0
extern void *nglMeshMemAlloc(int Size, int Alignment, int a3);

extern nglMesh *nglCloseMesh();

//0x00773380
extern void nglReleaseTexture(nglTexture *a1);

//0x00773290
extern nglTexture *nglLoadTexture(const tlFixedString &a1);

extern nglTexture *nglLoadTexture(const tlHashString &a1);

//0x007791A0
extern nglFont *create_and_parse_fdf(const tlFixedString &a1, char *a2);

//0x00775650
extern void nglMeshSetSphere(math::VecClass<3, 1> a1, Float a2);

//0x0076E0E0
extern void nglRenderPerfInfo();

//0x007733A0
extern bool nglLoadTextureIFL(nglTexture *tex, uint8_t *a2, int a3);

//0x0077A870
extern bool nglLoadTextureTM2(nglTexture *tex, uint8_t *a2);

//0x0077AB30
extern nglTexture *nglConstructTexture(const tlFixedString &a1,
                                       nglTextureFileFormat a2,
                                       void *a3,
                                       unsigned int a4);

//0x007732F0
extern nglTexture *nglLoadTextureInPlace(const tlFixedString &a1,
                                         nglTextureFileFormat a2,
                                         void *a3,
                                         int a4);

//0x00773350
extern bool nglCanReleaseTexture(nglTexture *tex);

//0x0077F930
extern void nglDebugAddSphere(const math::MatClass<4, 3> &a1, math::VecClass<3, 1> a2, uint32_t a3);

//0x0077B610
extern void nglSetBufferSize(nglBufferType a1, uint32_t a2, bool a3);

struct nglSortInfo;


//0x00779E40
void nglListAddString(nglFont *arg0, float arg4, float a3, float a4, float a5, float a6, const char *a2, ...);

//0x0076C3A0
extern void nglListAddCustomNode(void (*a1)(unsigned int *&, void *),
                                 void *a2,
                                 const nglSortInfo *a3);

//0x0077AFE0
extern void nglListAddQuad(nglQuad *Quad);

//0x007798E0
extern void nglGetStringDimensions(
    nglFont *Font, char *a2, uint32_t *a3, uint32_t *a4, Float a5, Float a6);

extern void nglGetStringDimensions(
    nglFont *arg0, unsigned int *arg4, unsigned int *a3, const char *a2, ...);

//0x00779C40
extern void nglListAddString(nglFont *font,
                             const char *a2,
                             Float a3,
                             Float a4,
                             Float z_value,
                             uint32_t a6,
                             Float a7,
                             Float a8);

extern void nglListAddString(nglFont* a1, Float a2, Float a3, Float a4, unsigned int a5, Float a6, Float a8, const char* Format, ...);

extern void nglListAddString(nglFont *a1, Float a3, Float a4, Float a5, int a6, const char *Format, ...);

extern Var<char[1024]> nglFontBuffer;

extern Var<nglMesh *> nglScratch;

inline Var<int> nglScratchSectionIdx {0x00973B18};

//0x00775770
extern nglMesh *nglCreateMeshClone(nglMesh *a1);

extern bool sub_578420(unsigned int a1);

//0x00775A30
extern void nglMakeSectionUnique(nglMesh *a1, int a2);

//0x00771F90
extern nglMeshSection *nglCreateSectionCopy(nglMeshSection *a1);

//0x0077AD30
extern void nglSetQuadRect(nglQuad *a1, Float a2, Float a3, Float a4, Float a5);

extern void nglReleaseAllTextures();


//0x00771E40
extern void nglCopySection(nglMesh *a1, int a2, nglMesh *a3, int a4);

//0x007759F0
extern void nglCopyMesh(nglMesh *a1, nglMesh *a2);

//0x0076F300
extern void nglReleaseAllMeshFiles();

//0x0076F2D0
extern int nglReleaseMeshFile(const tlFixedString &a1);

//0x0077BB20
extern void nglDestroyTexture(nglTexture *a1);

//0x0076E050
extern void nglListInit();

extern int nglHostPrintf(HANDLE hObject, const char *a2, ...);

void nglDumpQuad(nglQuad *Quad);

//0x0077EC70
extern void nglDebugInit();

//0x0076EA10
extern void nglListSend(bool a3);

//0x00769DB0
extern void nglSetClearFlags(unsigned int a1);

//0x0076A380
extern void nglSetClearColor(Float a1, Float a2, Float a3, Float a4);

//0x0076C970
extern void nglListBeginScene(nglSceneParamType a2);

//0x0076A030
extern void nglListEndScene();

//0x0076E750
extern void SetFrameLock(nglFrameLockType a2);

//0x0077AC90
extern void nglSetQuadTex(nglQuad *a1, nglTexture *a2);

//00775390
extern nglTexture *nglGetFrontBufferTex();

//0x0077ACB0
extern void nglSetQuadBlend(nglQuad *a1, nglBlendModeType a2, unsigned a3);

//0x0077ACD0
extern void nglSetQuadUV(nglQuad *a1, Float a2, int a3, Float a4, Float a5);

//0x0076E020
extern int nglGetScreenWidth();

//0x0076E030
extern int nglGetScreenHeight();

extern math::VecClass<3, 1> nglProjectPoint(math::VecClass<3, 1> a2);

extern void nglProjectPoint(math::VecClass<3, 1> &a1, math::VecClass<3, 1> a2);

extern nglParamSet<nglSceneParamSet_Pool> * nglGetSceneParams();

extern void nglDumpMesh(nglMesh *Mesh, const math::MatClass<4, 3> &a2, nglMeshParams *MeshParams);

//0x0076A070
extern void nglBeginHiresScreenShot(int width, int height);

//0x0076B800
extern void nglSetAspectRatio(Float a1);

//0x0076A240
extern bool nglSaveHiresScreenshot();

//0x007731D0
extern void nglScreenShot(const char *a1);

//0x005A02B0
extern void *ngl_memalloc_callback(unsigned int size, unsigned int align, unsigned int a3);

//0x0059E0B0
extern void ngl_memfree_callback(void *Memory);

inline constexpr auto NGLTEX_ANIMATED = 16u;

inline constexpr auto NGLTEX_SWIZZLED = 0x100u;
inline constexpr auto NGLTEX_LINEAR = 0x200u;

inline constexpr auto NGLTEX_RENDER_TARGET = 0x1000u;

inline constexpr auto NGLTEX_CUBE = 0x10000000u;

//0x0077A3C0
extern void nglSaveTexture(nglTexture *Tex, const char *a2);

extern Var<nglTexture> stru_975AC0;

extern Var<int[1024]> dword_975BE8;
extern Var<int> dword_975BE0;

inline Var<HANDLE> h_sceneDump {0x00976E20};

//0x0077BC90
extern nglTexture *nglCreateTexture(uint32_t Format, int Width, int Height, int a4, bool a5);

extern Var<tlInstanceBankResourceDirectory<nglMeshFile, tlFixedString> *> nglMeshFileDirectory;

extern Var<tlInstanceBankResourceDirectory<nglFont, tlFixedString> *> nglFontDirectory;

extern Var<tlInstanceBankResourceDirectory<nglMesh, tlHashString> *> nglMeshDirectory;

extern Var<tlInstanceBankResourceDirectory<nglMorphSet, tlHashString> *> nglMorphDirectory;

extern Var<tlInstanceBankResourceDirectory<nglMaterialFile, tlFixedString> *>
    nglMaterialFileDirectory;

extern Var<tlInstanceBankResourceDirectory<nglMaterialBase, tlHashString> *> nglMaterialDirectory;

extern void ngl_releasefile_callback(tlFileBuf *);

//0x00594740
extern bool ngl_readfile_callback(const char *FileName, tlFileBuf *File, unsigned int, unsigned int);

//0x0076E980
extern void nglFlip(bool a1);

extern void sub_77B2F0(bool a1);

extern void sub_76DF00();

//0x0076EFF0
extern nglMesh *nglGetMesh(const tlFixedString &a1, bool a2);

//0x0076F010
extern nglMesh *nglGetMesh(uint32_t a1, bool a2);

//0x0076E750
extern void nglSetFrameLock(nglFrameLockType a2);


extern void nglSetFrameLock2(nglFrameLockType2 a2);

//0x007793E0
extern void nglReleaseFont(nglFont *);

//0x00775700
extern void nglDestroyMesh(nglMesh *Mesh);

extern void nglSetQuadVPos(nglQuad *a1, int a2, float a3, float a4);

extern void nglSetQuadVUV(nglQuad *a1, int a2, float a3, float a4);

extern void nglSetQuadVColor(nglQuad *a1, int a2, unsigned int a3);

//0x0077AD70
extern void nglSetQuadZ(nglQuad *a1, Float a2);

extern void nglSetQuadMapFlags(nglQuad *a1, unsigned int a2);

//0x0077AD10
extern void nglSetQuadColor(nglQuad *a1, unsigned int a2);

//0x0077AC40
extern void nglInitQuad(nglQuad *a1);

//0x0077ADE0
extern void nglRotateQuad(nglQuad *a2, Float a3, Float a4, Float a5);

//0x0076E3E0
extern void nglInit(HWND hWnd);

extern void sub_769DE0(int a1);

//0x0076B6D0
extern void nglSetViewport(Float a1, Float a2, Float a3, Float a4);

//0x0076B8C0
extern void nglSetWorldToViewMatrix(const math::MatClass<4, 3> &a1);

//0x00769E20
extern void nglSetZTestEnable(bool a1);

extern void nglSetZWriteEnable(bool a1);

struct nglShaderNode;

extern void nglListAddNode(nglRenderNode *a1);

//0x0076B870
extern void nglSetOrthoMatrix(Float nearz, Float farz);

//0x0076F050
extern nglMesh *nglGetFirstMeshInFile(const tlFixedString &a1);

//0x0076ABA0
extern void nglCalculateMatrices(bool a1);

extern const char *GETFOURCC(uint32_t format);

extern math::VecClass<3, 1> sub_414360(const math::VecClass<3, 1> &a2, const math::MatClass<4, 3> &a3);

extern Var<bool> nglLoadingIFL;

extern Var<bool> EnableShader;

extern Var<char[256]> nglTexturePath;

extern Var<uint8_t *> nglListWorkPos;

inline Var<int> nglScratchMeshWorkSize {0x009752F0};

inline Var<int> nglPhysListWorkSize {0x009752F4};

extern LARGE_INTEGER query_perf_counter();

inline Var<IDirect3DPixelShader9 *> dword_9757A0 {0x009757A0};

inline Var<IDirect3DPixelShader9 *> dword_975794 {0x00975794};

struct nglRenderTextureState 
{
    static constexpr auto MAX_SIZE = 4u;
    IDirect3DTexture9 *field_0[MAX_SIZE];
    int field_10[MAX_SIZE];
    int field_20[3][MAX_SIZE];
    int field_50[MAX_SIZE];

    void clear()
    {
        for ( auto i = 0u; i < MAX_SIZE; ++i ) {
            this->field_0[i] = nullptr;
            this->field_10[i] = -1;
            this->field_20[0][i] = -1;
            this->field_20[1][i] = -1;
            this->field_20[2][i] = -1;
            this->field_50[i] = -1;
        }
    }

    void setSamplerState(
        int stage,
        uint8_t a3,
        uint32_t a4);
};

inline Var<uint32_t[4][14]> SamplerStates {0x00971FF0};

inline Var<uint32_t[264]> TextureStageStates {0x00972240};

inline Var<nglRenderTextureState> g_renderTextureState {0x0093BD50};

inline Var<IDirect3DQuery9 *> g_occlusionQueryTest {0x00972660};

inline Var<D3DPRESENT_PARAMETERS> s_d3dpresent_params{0x009720D0};

extern void sub_782030();

extern void sub_81E910();

extern void sub_76DF40();

extern void sub_57F3C0();

extern void sub_81E8E0(int a1);

extern double sub_77E820(Float a1);

extern double sub_77E940(Float a1);

extern double sub_77EA00(Float a1);

extern bool sub_581C30();

extern matrix4x3 sub_771210(void *a2);

extern matrix4x4 sub_4150E0(const matrix4x4 &a2);

extern void sub_4013C0(
        vector4d &a1,
        vector4d &a2,
        vector4d &a3,
        vector4d &a4,
        const vector4d &x,
        const vector4d &y,
        const vector4d &z,
        const vector4d &w);

extern matrix4x3 transposed(const matrix4x3 &a2);

extern vector4d xform_inv(const vector4d &a2, const matrix4x3 &a3);

extern bool nglIsSphereVisible(math::VecClass<3, 1> a1, Float radius);

extern void Reset3DDevice();

extern void SetRenderTarget(nglTexture *Tex, nglTexture *a2, int a3, int a4);

extern nglTexture *nglGetBackBufferTex();

extern void nglSetRenderTarget(nglTexture *a1);


inline float stru_946840[2] {1.0f, 1.0f};

extern nglVertexDef_MultipassMesh<nglVertexDef_PCUV_Base> *sub_507920(
    nglMaterialBase *a1, int a2, int a3, int a4, const void *a5, int a6, bool a7);

template<typename T>
auto PTR_OFFSET(uint32_t Base, T &Ptr) -> void
{
    if constexpr (std::is_pointer_v<T>)
    {
        if ( Ptr != nullptr)
        {
            Ptr = bit_cast<T>( (uint32_t)Ptr + (uint32_t)Base );
        }
    }
    else
    {
        if ( Ptr != 0)
        {
            Ptr = bit_cast<T>( (uint32_t)Ptr + (uint32_t)Base );
        }
    }
}


extern void ngl_patch();
