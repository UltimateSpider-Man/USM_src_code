#include "ngl.h"

#ifdef TARGET_XBOX

nglMaterialBase *nglGetMaterialInFile(const tlHashString &a1, nglMeshFile *a2)
{
    TRACE("nglGetMaterialInFile", a1.c_str());

    for ( auto *i = a2->field_13C; i != nullptr; i = i->field_C )
    {
        if ( i->Name == a1 )
        {
            return i;
        }
    }

    return nullptr;
}

nglMesh *nglGetMeshInFile(const tlHashString &a1, nglMeshFile *a2)
{
    TRACE("nglGetMeshInFile", a1.c_str());

    for ( auto *i = a2->FirstMesh; i != nullptr; i = i->NextMesh )
    {
        if ( i->Name == a1 )
        {
            return i;
        }
    }

    return nglGetMesh(a1, true);
}


bool nglLoadMeshFileInternal(const tlFixedString &FileName, nglMeshFile *MeshFile, const char *ext)
{
    TRACE("nglLoadMeshFileInternal", (std::string {FileName.to_string()} + ext).c_str());

    if constexpr (1)
    {
        nglMeshFileHeader *Header = CAST(Header, MeshFile->field_124.Buf);

        MeshFile->field_134 = (int) Header;
        MeshFile->field_144 = -1;
        if (strncmp(Header->Tag, "XBXM", 4u) != 0)
        {
            sp_log("Corrupted mesh file: %s%s%s.\n", nglMeshPath(), FileName.to_string(), ext);

            return false;
        }

        constexpr auto version = 0x1601;

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

        auto Base = bit_cast<uint32_t>(&MeshFile->field_124.Buf[-Header->field_10]);

        nglRebaseHeader(Base, Header);

        MeshFile->FirstMesh = nullptr;
        MeshFile->field_13C = nullptr;
        MeshFile->field_140 = nullptr;
        uint32_t num_dir_entries = Header->NDirectoryEntries;
        sp_log("num_dir_entries = %d", num_dir_entries);

        nglMesh *LastMesh = nullptr;
        nglMaterialBase *LastMaterial = nullptr;
        nglMorphSet *prevMorph = nullptr;

        auto *dir_entries = Header->DirectoryEntries;

        std::for_each(dir_entries, dir_entries + num_dir_entries,
                [&](auto &dir_entry)
        {
            PTR_OFFSET(Base, dir_entry.field_4);

            auto type_dir_entry = dir_entry.field_3;
            sp_log("%s", to_string(type_dir_entry));
            switch (type_dir_entry) {
            case TypeDirectoryEntry::MATERIAL: {

                nglMaterialBase *Material = CAST(Material, dir_entry.field_4);

                Material->File = MeshFile;
                if (MeshFile->field_13C == nullptr) {
                    MeshFile->field_13C = Material;
                }

                if (LastMaterial != nullptr) {
                    LastMaterial->field_C = Material;
                }

                LastMaterial = Material;
                if (Header->field_10 == 0)
                {
                    uint32_t v17 = CAST(v17, Material->field_4);
                    const tlHashString a2 {v17};

                    auto *v18 = nglShaderBank().Search(a2);

                    if (v18 != nullptr)
                    {
                        auto *shader = static_cast<nglShader *>(v18->field_20);

                        sp_log("%s", a2.c_str());
                        sp_log("%s 0x%08X", Material->Name.c_str(), Material->field_10);

                        if (shader->CheckMaterialVersion(Material)) {
                            Material->field_4 = shader;
                        }
                        else
                        {
                            auto *v27 = a2.c_str();
                            auto v26 = Material->field_10;

                            auto *v8 = Material->Name.c_str();
                            sp_log(
                                "Material %s binary version (%d) is not compatible with shader "
                                "%s.\n",
                                v8,
                                v26,
                                v27);
                            Material->field_4 = &gEmptyShader();
                        }

                    } else {
                        auto *v28 = Material->Name.c_str();
                        auto *v9 = a2.c_str();
                        sp_log("NGL: Unable to find shader %s, used by material %s.\n", v9, v28);

                        Material->field_4 = &gEmptyShader();
                    }
                }

                Material->field_4->RebaseMaterial(Material, Base);

                Material->field_4->BindMaterial(Material);

            } break;
            case TypeDirectoryEntry::MESH: {
                nglMesh *Mesh = CAST(Mesh, dir_entry.field_4);

                sp_log("%s", Mesh->Name.c_str());

                {
                    void (__fastcall *Add)(void *, void *edx, nglMesh *) = CAST(Add, get_vfunc(nglMeshDirectory()->m_vtbl, 0x10));
                    Add(nglMeshDirectory(), nullptr, Mesh);
                }

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

                sp_log("Mesh->NSections = %d", Mesh->NSections);
                for (auto idx_Section = 0u; idx_Section < Mesh->NSections; ++idx_Section)
                {
                    Mesh->Sections[idx_Section].field_0 = 1;

                    nglMeshSection *MeshSection = CAST(MeshSection, Mesh->Sections[idx_Section].Section);
                    tlHashString a1 {(uint32_t) MeshSection->Name};

                    MeshSection->Material = nglGetMaterialInFile(a1, MeshFile);

                    if (!MeshSection->Material->field_4->CheckVertexDefVersion(MeshSection))
                    {
                        tlFixedString v111 = MeshSection->Material->field_4->GetName();

                        auto *v12 = v111.to_string();
                        sp_log(
                            "Section VertexDef Binary version (%d) is incompatible with "
                            "shader %s\n.",
                            MeshSection->field_50,
                            v12);
                        MeshSection->Material->field_4 = &gEmptyShader();
                    }

                    if (MeshSection->NIndices != 0)
                    {
                        sp_log("NIndices = %d", MeshSection->NIndices);

                        {
                            auto *arr = bit_cast<uint16_t *>(MeshSection->m_indices);
                            sp_log("indices = %u %u %u", arr[0], arr[1], arr[2]);
                        }

                        sp_log("NVertices = %d", MeshSection->field_40);

                        {
                            auto *arr = bit_cast<float *>(MeshSection->m_vertices);
                            sp_log("vertices = %f %f %f %f", arr[0], arr[1], arr[2], arr[3]);
                        }

                        sp_log("stride = %d", MeshSection->m_stride);
                    }

                    if (auto *v27 = MeshSection->m_indices; v27 != nullptr)
                    {
                        bit_cast<nglVertexBuffer *>(&MeshSection->m_indices)
                            ->createIndexBufferAndWriteData(v27, 2 * MeshSection->NIndices);
                    }

                    auto *v28 = MeshSection->Material;
                    MeshSection->field_58 = 0;

                    tlFixedString v112 = v28->field_4->GetName();

                    auto *v29 = v112.to_string();
                    sp_log(v29);

                    [&v29](nglMeshSection *MeshSection) -> void
                    {
                        auto func = [](nglMeshSection *MeshSection)
                        {
                            auto v31 = static_cast<uint32_t>(MeshSection->field_40 >> 6);
                            auto *v32 = (float *) (static_cast<char *>(MeshSection->m_vertices) +
                                                   32);
                            MeshSection->field_5C = 2;
                            if (v31 > 0)
                            {
                                for (; v31 != 0; --v31)
                                {
                                    if (equal(v32[7], 0.0f))
                                    {
                                        if (not_equal(v32[6], 0.0f) && MeshSection->field_5C < 3u)
                                        {
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
                            }

                            ((nglVertexBuffer *) &MeshSection->m_vertices)
                                ->createVertexBufferAndWriteData((const void *) MeshSection->m_vertices,
                                                                 MeshSection->field_40,
                                                                 1028);

                            static Var<int> dword_973BC8{0x00973BC8};

                            if (dword_973BC8() < (int) (24 * (MeshSection->field_40 >> 6))) {
                                dword_973BC8() = 24 * (MeshSection->field_40 >> 6);
                            }

                            MeshSection->m_stride = 24;
                        };

                        if (!EnableShader())
                        {
                            if (strncmp(v29, "uslod", 5u) == 0)
                            {
                                nglVertexBuffer::createIndexOrVertexBuffer(
                                    (nglVertexBuffer *) &MeshSection->m_vertices,
                                    ResourceType::VertexBuffer,
                                    16 * (MeshSection->field_40 / 12),
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
                                        int v30 = 48 * (MeshSection->field_40 / 60u);
                                        ((nglVertexBuffer *) &MeshSection->m_vertices)
                                            ->createVertexBuffer(v30, 520u);
                                        MeshSection->m_stride = 48;

                                        static Var<int> dword_972960{0x00972960};

                                        if (dword_972960() < v30) {
                                            dword_972960() = v30;
                                        }

                                        return;
                                    }
                                } else {
                                    if (!EnableShader()) {
                                        if (strncmp(v29, "usperson", 8u) == 0)
                                        {
                                            func(bit_cast<nglMeshSection *>(MeshSection));
                                            return;
                                        }
                                    }
                                }
                            }
                        }

                        if (strncmp(v29, "us_character", 12u) == 0)
                        {
                            func(bit_cast<nglMeshSection *>(MeshSection));
                            return;
                        }

                        ((nglVertexBuffer *) &MeshSection->m_vertices)
                            ->createVertexBufferAndWriteData((const void *) MeshSection->m_vertices,
                                                             MeshSection->field_40,
                                                             1028);
                    }(MeshSection);

                    if (auto *v39 = MeshSection->VertexDef; v39 != nullptr)
                    {
                        tlHashString a1 {v39->m_vtbl};

                        auto *v40 = nglVertexDefBank().Search(a1);
                        if (v40 != nullptr)
                        {
                            MeshSection->VertexDef->field_4 = MeshSection;

                            void (*func)(void *) = CAST(func, v40->field_20);
                            func(MeshSection->VertexDef);
                        } else {
                            MeshSection->VertexDef = nullptr;
                        }
                    }

                    auto *v41 = MeshSection->Material;
                    if (v41 != nullptr)
                    {
                        auto *v42 = v41->field_4;
                        if (v42 != nullptr)
                        {
                            v42->BindSection(MeshSection);
                        }
                    }
                }

            } break;
            case TypeDirectoryEntry::MORPH: {
                nglMorphSet *new_morph = CAST(new_morph, dir_entry.field_4);
                nglProcessMorph(MeshFile, &dir_entry, (int) Header);
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
                    type_dir_entry);

                break;
            }
            }
        });

        if (LastMesh != nullptr)
        {
            LastMesh->NextMesh = nullptr;
        }

        if (LastMaterial != nullptr)
        {
            LastMaterial->field_C = nullptr;
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

                    auto v89 = Mesh->field_20.field_0[0];
                    auto v90 = Mesh->field_20.field_0[1];
                    auto v91 = Mesh->field_20.field_0[2];
                    auto v93 = Mesh->field_20.field_0[3];
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
                } else {
                    Mesh->Flags |= NGLMESH_PROCESSED;
                }

                auto *Lods = Mesh->LODs;
                for (int i = 0; i < Mesh->NLODs; ++i)
                {
                    tlHashString v1 {bit_cast<uint32_t >(Lods[i].field_0)};

                    Mesh->LODs[i].field_0 = nglGetMeshInFile(v1, MeshFile);
                    Lods = Mesh->LODs;
                    if (Lods[i].field_0 == nullptr)
                    {
                        --i;
                        --Mesh->NLODs;
                    }
                }
            }
        }

        if (v46)
        {
            auto v60 = sub_411750(a3a, v103);
            auto v78 = v60[0] * 0.5f;
            auto v61 = v78;
            auto v62 = v60[1];

            vector4d v96;
            v96[0] = v78;
            auto v81 = v62 * 0.5f;

            v96[1] = v81;
            auto v84 = v60[2] * 0.5f;

            auto *v67 = MeshFile->FirstMesh;

            auto v87 = v60[3] * 0.5f;
            auto v68 = v87;
            auto v69 = 0.0f;
            v96[2] = v84;
            v96[3] = v87;
            for (; v67 != nullptr; v67 = v67->NextMesh)
            {
                if ((v67->Flags & NGLMESH_PROCESSED) == 0)
                {
                    a3a[0] = v96[0] - v67->field_20.field_0[0];
                    a3a[1] = v96[1] - v67->field_20.field_0[1];
                    a3a[2] = v96[2] - v67->field_20.field_0[2];
                    a3a[3] = v96[3] - v67->field_20.field_0[3];
                    auto v76 = vector3d {a3a[0], a3a[1], a3a[3]}.length() + v67->SphereRadius;
                    if (v69 <= v76) {
                        v69 = v76;
                    }
                }
            }

            for (auto *Mesh = v67; Mesh != nullptr; Mesh = Mesh->NextMesh) {
                if ((Mesh->Flags & NGLMESH_PROCESSED) == 0) {
                    Mesh->SphereRadius = v69;
                    Mesh->field_20.field_0[0] = v61;
                    Mesh->field_20.field_0[1] = v81;
                    Mesh->field_20.field_0[2] = v84;
                    Mesh->field_20.field_0[3] = v68;
                    Mesh->Flags |= NGLMESH_PROCESSED;
                }
            }
        }

        Header->field_10 = (int) MeshFile->field_124.Buf;
        return true;
    }
    else
    {
        auto result = static_cast<bool>(CDECL_CALL(0x0076F500, &FileName, MeshFile, ext));

        return result;
    }
}
#endif

#ifndef TARGET_XBOX

#include "common.h"
#include "fixedstring.h"
#include "log.h"
#include "ngl_mesh.h"
#include "ngl_vertexdef.h"
#include "nglemptyshader.h"
#include "nglshader.h"
#include "tl_instance_bank.h"
#include "tl_system.h"
#include "trace.h"
#include "vtbl.h"
#include "xbmesh_format.h"
#include "xbpack_v10_captive_material.h"
#include "xbpack_v10_mesh_identity.h"
#include "ngl_xbox_morph_remap.h"
#include "ngl_xbox_vertex_stream.h"

#include <algorithm>
#include <array>
#include <cstdio>
#include <cstring>
#include <iterator>
#include <limits>
#include <map>
#include <memory>
#include <new>
#include <utility>
#include <vector>

namespace
{
// File sections normally live entirely inside FileBuf, so the native release
// path only releases their GPU resources. XBXM conversion additionally owns
// CPU vertex streams, names, split palettes/sections and replacement tables.
// Keep those allocations with the file, including a rollback list while the
// image is being converted. Never publish a partly converted mesh.
struct xbox_morph_runtime {
    uint32_t hash;
    nglMorphSet *set;
    xbox_morph::frame *frames;
    uint32_t pointer_base;
    bool rebased = false;
    bool remapped = false;
};

struct xbox_mesh_runtime {
    nglMeshFile *file;
    const uint32_t resource_hash;
    unsigned captive_materials = 0;
    unsigned captive_vertex_defs = 0;
    bool committed = false;
    bool packed_unload = false;
    std::vector<void *> allocations;
    std::vector<nglMeshSection *> sections;
    std::vector<nglMaterialBase *> materials;
    std::vector<xbox_morph::mesh_map> mesh_maps;
    std::vector<xbox_morph_runtime> morphs;

    xbox_mesh_runtime(nglMeshFile *owner, uint32_t hash)
        : file(owner), resource_hash(hash) {}

    void *allocate(size_t bytes, unsigned alignment)
    {
        if (bytes > std::numeric_limits<uint32_t>::max()) return nullptr;
        auto *memory = tlMemAlloc(static_cast<uint32_t>(bytes), alignment, 0x1000000u);
        if (memory == nullptr) return nullptr;
        try {
            allocations.push_back(memory);
        } catch (...) {
            tlMemFree(memory);
            throw;
        }
        return memory;
    }

    ~xbox_mesh_runtime()
    {
        // Both successful unload paths release sections before this owner.
        // A partial conversion must also roll back buffers created so far.
        if (!committed) {
            for (auto *section : sections) nglReleaseSection(section);
        }
        if (!committed || packed_unload) {
            // A loose file reaches this owner after the native release has
            // already dropped its material references. Packed files signal
            // their different teardown order explicitly.
            for (auto it = materials.rbegin(); it != materials.rend(); ++it)
                (*it)->m_shader->ReleaseMaterial(*it);
        }
        file->FirstMesh = nullptr;
        file->FirstMaterial = nullptr;
        file->FirstMorph = nullptr;
        file->field_134 = 0;
        for (auto it = allocations.rbegin(); it != allocations.rend(); ++it)
            tlMemFree(*it);
    }
};

using xbox_mesh_owners = std::map<char *, std::unique_ptr<xbox_mesh_runtime>>;

xbox_mesh_owners *&runtime_owners_pointer()
{
    // Engine allocation callbacks may already be gone during DLL shutdown.
    // Owners are released at file unload, not from a static destructor.
    static xbox_mesh_owners *owners = nullptr;
    return owners;
}

xbox_mesh_owners &runtime_owners()
{
    auto *&owners = runtime_owners_pointer();
    if (owners == nullptr) owners = new xbox_mesh_owners;
    return *owners;
}

struct xbox_ngl_mesh_file_header {
    char tag[4];
    uint32_t version;
    uint32_t directory_count;
    uint32_t directory;
    uint32_t old_base;
};

struct xbox_ngl_directory_entry {
    uint8_t field_0;
    uint8_t field_1;
    uint8_t field_2;
    TypeDirectoryEntry type;
    uint32_t object;
    uint32_t aux_hash;
};

struct xbox_ngl_mesh_section {
    uint32_t material_hash;
    uint32_t material;
    int32_t bone_count;
    uint32_t bone_indices;
    float sphere_center[4];
    float sphere_radius;
    uint32_t flags;
    D3DPRIMITIVETYPE primitive_type;
    int32_t index_count;
    uint32_t indices;
    uint32_t runtime_index_buffer;
    uint32_t secondary_vertices;
    uint32_t secondary_vertices_size;
    int32_t vertex_count;
    uint32_t vertices;
    uint32_t vertices_size;
    int32_t field_4C;
    int32_t stride;
    int32_t field_54;
    int32_t vertex_def_version;
    uint32_t vertex_def;
};

struct xbox_skinned_vertex {
    float position[3];
    uint32_t packed_normal;
    float uv[2];
    int8_t bone_indices[4];
    uint8_t bone_weights[4];
};

struct pc_skinned_vertex {
    float position[3];
    float normal[3];
    float uv[2];
    float bone_indices[4];
    float bone_weights[4];
};

static_assert(sizeof(xbox_ngl_mesh_file_header) == 0x14);
static_assert(sizeof(xbox_ngl_directory_entry) == 0x0C);
static_assert(sizeof(xbox_ngl_mesh_section) == 0x60);
static_assert(sizeof(xbox_skinned_vertex) == 0x20);
static_assert(sizeof(pc_skinned_vertex) == 0x40);

constexpr int PC_SKIN_BONE_LIMIT = 20;
constexpr auto xbox_line_strip = static_cast<D3DPRIMITIVETYPE>(4);
constexpr auto xbox_triangle_list = static_cast<D3DPRIMITIVETYPE>(5);
constexpr auto xbox_triangle_strip = static_cast<D3DPRIMITIVETYPE>(6);
constexpr auto xbox_triangle_fan = static_cast<D3DPRIMITIVETYPE>(7);
constexpr auto xbox_quad_list = static_cast<D3DPRIMITIVETYPE>(8);

template<typename T>
T *rebase_pointer(uint32_t base, uint32_t pointer)
{
    if (pointer == 0) {
        return nullptr;
    }

    return bit_cast<T *>(base + pointer);
}

int32_t sign_extend(uint32_t value, uint32_t bit_count)
{
    const uint32_t sign_bit = 1u << (bit_count - 1u);
    return static_cast<int32_t>((value ^ sign_bit) - sign_bit);
}

float decode_normal_component(uint32_t value, uint32_t bit_count)
{
    const auto signed_value = sign_extend(value, bit_count);
    const auto positive_max = static_cast<float>((1u << (bit_count - 1u)) - 1u);
    const auto decoded = static_cast<float>(signed_value) / positive_max;
    return decoded < -1.0f ? -1.0f : decoded;
}

bool is_skinned_vertex_def(uint32_t hash)
{
    return modmesh::xbmeshformat::isSkinnedVertexDef(hash);
}

bool convert_skinned_vertices(xbox_mesh_runtime &owner, uint32_t base,
                              const xbox_ngl_mesh_section &disk,
                              nglMeshSection *section)
{
    if (disk.vertex_count < 0 ||
        disk.vertices_size != static_cast<uint32_t>(disk.vertex_count) * sizeof(xbox_skinned_vertex)) {
        sp_log("XBXM skinned vertex stream has an invalid size");
        return false;
    }

    auto *source = rebase_pointer<xbox_skinned_vertex>(base, disk.vertices);
    if (disk.vertex_count != 0 && source == nullptr) {
        return false;
    }

    const auto converted_size =
        static_cast<uint32_t>(disk.vertex_count) * sizeof(pc_skinned_vertex);
    auto *converted = static_cast<pc_skinned_vertex *>(
        owner.allocate(converted_size, 16));
    if (converted_size != 0 && converted == nullptr) {
        return false;
    }

    for (int32_t i = 0; i < disk.vertex_count; ++i) {
        const auto &input = source[i];
        auto &output = converted[i];

        std::memcpy(output.position, input.position, sizeof(output.position));
        output.normal[0] = decode_normal_component(input.packed_normal & 0x7FFu, 11);
        output.normal[1] = decode_normal_component((input.packed_normal >> 11u) & 0x7FFu, 11);
        output.normal[2] = decode_normal_component((input.packed_normal >> 22u) & 0x3FFu, 10);
        std::memcpy(output.uv, input.uv, sizeof(output.uv));

        for (int component = 0; component < 4; ++component) {
            output.bone_indices[component] = static_cast<float>(input.bone_indices[component]);
            output.bone_weights[component] =
                static_cast<float>(input.bone_weights[component]) / 255.0f;
        }
    }

    section->field_3C.m_vertexData = bit_cast<char *>(converted);
    section->field_3C.Size = converted_size;
    section->m_stride = sizeof(pc_skinned_vertex);
    return true;
}

// Match the serialized vertex definition as well as its stride. A 24-byte
// PCUV/colour stream is already valid, while US_Outline stores a packed
// normal there. SMShiny and USShinyInterior similarly pack three vectors.
// Convert before GPU upload and keep the expanded allocation with this file.
bool convert_static_vertices(xbox_mesh_runtime &owner,
                             const xbox_ngl_mesh_section &disk,
                             nglMeshSection *section,
                             uint32_t vertex_def_hash)
{
    ngl_xbox_vertex_stream::plan layout;
    if (!ngl_xbox_vertex_stream::make_plan(vertex_def_hash, disk.stride,
            disk.vertex_count, disk.vertices_size, disk.field_54, layout)) {
        sp_log("XBXM static vertex stream 0x%08X has an invalid size or offset",
               vertex_def_hash);
        return false;
    }
    if (layout.type == ngl_xbox_vertex_stream::kind::unchanged) return true;

    auto *converted = layout.pc_bytes != 0
        ? static_cast<char *>(owner.allocate(layout.pc_bytes, 16)) : nullptr;
    if (!ngl_xbox_vertex_stream::convert(layout,
            section->field_3C.m_vertexData, disk.vertices_size,
            converted, layout.pc_bytes)) {
        sp_log("XBXM static vertex stream 0x%08X could not be expanded",
               vertex_def_hash);
        return false;
    }

    section->field_3C.m_vertexData = converted;
    section->field_3C.Size = layout.pc_bytes;
    section->m_stride = static_cast<int>(layout.pc_stride);
    section->field_4C = static_cast<int>(layout.pc_offset);
    sp_log("XBXM 0x%08X: vertex definition 0x%08X expanded %u records "
           "(%u -> %u bytes/vertex)", owner.resource_hash, vertex_def_hash,
           layout.records, layout.source_stride, layout.pc_stride);
    return true;
}

tlFixedString *make_runtime_fixed_string(xbox_mesh_runtime &owner, uint32_t hash)
{
    auto *result = static_cast<tlFixedString *>(owner.allocate(sizeof(tlFixedString), 8));
    if (result == nullptr) {
        return nullptr;
    }

    result->m_hash = hash;
    if (hash == 0) {
        std::memset(result->field_4, 0, sizeof(result->field_4));
    } else {
        std::snprintf(result->field_4, sizeof(result->field_4), "0x%08x", hash);
    }
    return result;
}

bool fixup_texture_name(xbox_mesh_runtime &owner, nglMaterialBase *material, uint32_t offset)
{
    auto *field = bit_cast<char *>(material) + offset;
    uint32_t hash = 0;
    std::memcpy(&hash, field, sizeof(hash));

    auto *texture_name = make_runtime_fixed_string(owner, hash);
    if (texture_name == nullptr) {
        return false;
    }

    std::memcpy(field, &texture_name, sizeof(texture_name));
    return true;
}

bool fixup_texture_names(xbox_mesh_runtime &owner, nglMaterialBase *material,
                         uint32_t shader_hash,
                         bool &uses_hash_names)
{
    uses_hash_names = true;

    switch (shader_hash) {
    // NewLOD, USPCUV, and FrontEnd use one name at +0x18.
    case 0x07FE3B49:
    case 0x41953B85:
    case 0x72EA54E7:
        return fixup_texture_name(owner, material, 0x18);

    // Panel, USObject, and the USPerson family use names at +0x18/+0x20.
    case 0x0821CA90:
    case 0x0C9A7666:
    case 0x9B2581FF:
    case 0x9EF152BA:
    case 0xAC364499:
    case 0xF964AC5E:
        return fixup_texture_name(owner, material, 0x18) &&
               fixup_texture_name(owner, material, 0x20);

    // Interior simple/translucent variants use one name at +0x1C.
    case 0x0372147E:
    case 0x4211E7CA:
    case 0x50923D45:
    case 0x8C7BB8C7:
    case 0x8CAD0814:
        return fixup_texture_name(owner, material, 0x1C);

    // Interior shiny variants use names at +0x1C/+0x28.
    case 0x56F12DDF:
    case 0x91F54E26:
        return fixup_texture_name(owner, material, 0x1C) &&
               fixup_texture_name(owner, material, 0x28);

    // Exterior simple/translucent/decal variants use one name at +0x60.
    case 0x100DE499:
    case 0x2561BB40:
    case 0x287B09F3:
    case 0x530520E7:
    case 0x8F463565:
    case 0xA3342E9F:
    case 0xDDB856F7:
    case 0xE50807FC:
    case 0xFA4ABAD3:
    case 0xFC097C8A:
        return fixup_texture_name(owner, material, 0x60);

    // Building materials use names at +0x60/+0x68.
    case 0xD6E6B9E2:
    case 0xD98097F0:
        return fixup_texture_name(owner, material, 0x60) &&
               fixup_texture_name(owner, material, 0x68);

    // Exterior shiny materials use names at +0x60/+0x6C.
    case 0x9B076DEB:
    case 0xA3C2A47A:
        return fixup_texture_name(owner, material, 0x60) &&
               fixup_texture_name(owner, material, 0x6C);

    // Grunge materials use names at +0x60/+0x70.
    case 0x42317C08:
    case 0xE7E31E4F:
        return fixup_texture_name(owner, material, 0x60) &&
               fixup_texture_name(owner, material, 0x70);

    case 0x98A4BA80:
        return fixup_texture_name(owner, material, 0x18) &&
               fixup_texture_name(owner, material, 0x20) &&
               fixup_texture_name(owner, material, 0x28) &&
               fixup_texture_name(owner, material, 0x30) &&
               fixup_texture_name(owner, material, 0x38) &&
               fixup_texture_name(owner, material, 0x40);

    default:
        uses_hash_names = false;
        return true;
    }
}

nglMaterialBase *find_material(nglMeshFile *mesh_file, uint32_t hash)
{
    for (auto *material = mesh_file->FirstMaterial;
         material != nullptr;
         material = material->NextMaterial) {
        if (material->Name != nullptr && material->Name->m_hash == hash) {
            return material;
        }
    }

    return nullptr;
}

nglMesh *find_mesh(nglMeshFile *mesh_file, uint32_t hash)
{
    for (auto *mesh = mesh_file->FirstMesh; mesh != nullptr; mesh = mesh->NextMesh) {
        if (mesh->Name != nullptr && mesh->Name->m_hash == hash) {
            return mesh;
        }
    }

    return nglGetMesh(hash, true);
}

bool convert_section(xbox_mesh_runtime &owner, uint32_t base,
                     nglMeshFile *mesh_file,
                     nglMeshSection *section)
{
    xbox_ngl_mesh_section disk{};
    std::memcpy(&disk, section, sizeof(disk));

    if (disk.secondary_vertices != 0 || disk.secondary_vertices_size != 0) {
        sp_log("XBXM section uses an unsupported secondary vertex stream");
        return false;
    }

    auto *material_name = make_runtime_fixed_string(owner, disk.material_hash);
    if (material_name == nullptr) {
        return false;
    }

    auto *material = find_material(mesh_file, disk.material_hash);
    if (material == nullptr) {
        sp_log("XBXM material 0x%08X was not found", disk.material_hash);
        return false;
    }

    section->MaterialName = material_name;
    section->Material = material;
    section->NBones = disk.bone_count;
    section->BonesIdx = rebase_pointer<uint16_t>(base, disk.bone_indices);
    std::memcpy(&section->SphereCenter, disk.sphere_center, sizeof(disk.sphere_center));
    section->SphereRadius = disk.sphere_radius;
    section->Flags = disk.flags;
    section->m_primitiveType = disk.primitive_type;
    section->NIndices = disk.index_count;
    section->m_indices = rebase_pointer<uint16_t>(base, disk.indices);
    section->m_indexBuffer = nullptr;
    section->NVertices = disk.vertex_count;
    section->field_3C.m_vertexData = rebase_pointer<char>(base, disk.vertices);
    section->field_3C.Size = disk.vertices_size;
    section->field_3C.m_vertexBuffer = nullptr;
    section->m_stride = disk.stride;
    section->field_4C = disk.field_54;
    section->field_50 = disk.vertex_def_version;
    section->VertexDef = rebase_pointer<nglVertexDef>(base, disk.vertex_def);
    section->StartIndex = 0;
    section->field_5C = 0;
    owner.sections.push_back(section);

    uint32_t vertex_def_hash = 0;
    if (section->VertexDef != nullptr) {
        vertex_def_hash = bit_cast<uint32_t>(section->VertexDef->m_vtbl);
    }

    if (disk.stride == sizeof(xbox_skinned_vertex) &&
        is_skinned_vertex_def(vertex_def_hash) &&
        !convert_skinned_vertices(owner, base, disk, section)) {
        return false;
    }

    if (!convert_static_vertices(owner, disk, section, vertex_def_hash)) {
        return false;
    }

    if (section->m_primitiveType == xbox_line_strip) {
        section->m_primitiveType = D3DPT_LINESTRIP;
    } else if (section->m_primitiveType == xbox_triangle_list) {
        section->m_primitiveType = D3DPT_TRIANGLELIST;
    } else if (section->m_primitiveType == xbox_triangle_strip) {
        section->m_primitiveType = D3DPT_TRIANGLESTRIP;
    } else if (section->m_primitiveType == xbox_triangle_fan) {
        section->m_primitiveType = D3DPT_TRIANGLEFAN;
    } else if (section->m_primitiveType == xbox_quad_list) {
        const int source_index_count = section->NIndices > 0
            ? section->NIndices
            : section->NVertices;
        if (source_index_count <= 0 || (source_index_count % 4) != 0) {
            sp_log("XBXM quad list has an invalid element count: %d", source_index_count);
            return false;
        }

        const int quad_count = source_index_count / 4;
        const int triangle_index_count = quad_count * 6;
        auto *triangle_indices = static_cast<uint16_t *>(
            owner.allocate(sizeof(uint16_t) * triangle_index_count, 8));
        if (triangle_indices == nullptr) {
            return false;
        }

        for (int quad = 0; quad < quad_count; ++quad) {
            uint16_t vertices[4];
            for (int vertex = 0; vertex < 4; ++vertex) {
                const int source_index = quad * 4 + vertex;
                vertices[vertex] = section->m_indices != nullptr
                    ? section->m_indices[source_index]
                    : static_cast<uint16_t>(source_index);
            }

            auto *output = triangle_indices + quad * 6;
            output[0] = vertices[0];
            output[1] = vertices[1];
            output[2] = vertices[2];
            output[3] = vertices[0];
            output[4] = vertices[2];
            output[5] = vertices[3];
        }

        section->m_primitiveType = D3DPT_TRIANGLELIST;
        section->NIndices = triangle_index_count;
        section->m_indices = triangle_indices;
    } else if (section->m_primitiveType != D3DPT_POINTLIST &&
               section->m_primitiveType != D3DPT_LINELIST) {
        sp_log("XBXM section uses unsupported primitive type %u",
               static_cast<unsigned>(section->m_primitiveType));
        return false;
    }

    if (!material->m_shader->CheckVertexDefVersion(section)) {
        sp_log("XBXM section vertex definition is incompatible with its PC shader");
        material->m_shader = &gEmptyShader();
    }

    if (section->m_indices != nullptr && section->NIndices > 0) {
        bit_cast<nglVertexBuffer *>(&section->m_indexBuffer)
            ->createIndexBufferAndWriteData(section->m_indices, 2 * section->NIndices);
    }

    if (section->field_3C.m_vertexData != nullptr && section->field_3C.Size > 0) {
        section->field_3C.createVertexBufferAndWriteData(
            section->field_3C.m_vertexData, section->field_3C.Size, 1028);
    }

    if (section->VertexDef != nullptr) {
        const auto source_vertex_def_hash =
            bit_cast<uint32_t>(section->VertexDef->m_vtbl);
        const auto compatible_vertex_def_hash =
            xbpack_v10_captive_material::shader_for(owner.resource_hash,
                                                   source_vertex_def_hash);
        auto *node = nglVertexDefBank().Search(
            tlHashString {compatible_vertex_def_hash});
        if (node != nullptr) {
            section->VertexDef->field_4 = section;
            auto construct = bit_cast<void (*)(void *)>(node->field_20);
            construct(section->VertexDef);
            if (compatible_vertex_def_hash != source_vertex_def_hash)
                ++owner.captive_vertex_defs;
        } else {
            section->VertexDef = nullptr;
            if (xbpack_v10_captive_material::enabled_for(owner.resource_hash)) {
                sp_log("[xbpack] V10 captive vertex definition unavailable: "
                       "source=0x%08X required=0x%08X",
                       source_vertex_def_hash, compatible_vertex_def_hash);
                return false;
            }
        }
    }

    material->m_shader->BindSection(section);
    return true;
}

using xbox_skin_triangle = std::array<uint16_t, 3>;

struct xbox_skin_batch {
    std::vector<xbox_skin_triangle> triangles;
    std::vector<uint16_t> bones;
};

bool append_unique_bone(std::vector<uint16_t> &bones, uint16_t bone)
{
    if (std::find(bones.begin(), bones.end(), bone) != bones.end()) {
        return false;
    }

    bones.push_back(bone);
    return true;
}

bool decode_triangles(uint32_t base,
                      const xbox_ngl_mesh_section &disk,
                      std::vector<xbox_skin_triangle> &triangles)
{
    if (disk.vertex_count < 0 ||
        disk.vertex_count > static_cast<int32_t>(std::numeric_limits<uint16_t>::max()) + 1 ||
        disk.index_count < 0) {
        return false;
    }

    const auto *source_indices = disk.index_count > 0
        ? rebase_pointer<uint16_t>(base, disk.indices)
        : nullptr;
    if (disk.index_count > 0 && source_indices == nullptr) {
        return false;
    }

    const int source_count = disk.index_count > 0 ? disk.index_count : disk.vertex_count;
    auto index_at = [&](int index) -> uint16_t {
        return source_indices != nullptr
            ? source_indices[index]
            : static_cast<uint16_t>(index);
    };
    auto append_triangle = [&](xbox_skin_triangle triangle) -> bool {
        for (const auto vertex : triangle) {
            if (vertex >= disk.vertex_count) {
                sp_log("XBXM skinned section contains vertex index %u outside %d vertices",
                       vertex,
                       disk.vertex_count);
                return false;
            }
        }

        if (triangle[0] != triangle[1] &&
            triangle[1] != triangle[2] &&
            triangle[0] != triangle[2]) {
            triangles.push_back(triangle);
        }
        return true;
    };

    if (disk.primitive_type == xbox_triangle_list) {
        if ((source_count % 3) != 0) {
            return false;
        }
        for (int index = 0; index < source_count; index += 3) {
            if (!append_triangle({index_at(index), index_at(index + 1), index_at(index + 2)})) {
                return false;
            }
        }
    } else if (disk.primitive_type == xbox_triangle_strip) {
        for (int index = 2; index < source_count; ++index) {
            xbox_skin_triangle triangle {
                index_at(index - 2),
                index_at(index - 1),
                index_at(index),
            };
            if ((index & 1) != 0) {
                std::swap(triangle[0], triangle[1]);
            }
            if (!append_triangle(triangle)) {
                return false;
            }
        }
    } else if (disk.primitive_type == xbox_triangle_fan) {
        for (int index = 2; index < source_count; ++index) {
            if (!append_triangle({index_at(0), index_at(index - 1), index_at(index)})) {
                return false;
            }
        }
    } else {
        sp_log("XBXM oversized skinned section uses unsupported primitive type %u",
               static_cast<unsigned>(disk.primitive_type));
        return false;
    }

    return !triangles.empty();
}

bool build_skin_batches(uint32_t base,
                        const xbox_ngl_mesh_section &disk,
                        std::vector<xbox_skin_batch> &batches)
{
    if (disk.vertex_count < 0 ||
        disk.bone_count <= PC_SKIN_BONE_LIMIT ||
        disk.bone_count > std::numeric_limits<int8_t>::max() ||
        disk.vertices_size != static_cast<uint32_t>(disk.vertex_count) *
                                  sizeof(xbox_skinned_vertex)) {
        return false;
    }

    const auto *source_vertices = rebase_pointer<xbox_skinned_vertex>(base, disk.vertices);
    const auto *source_bones = rebase_pointer<uint16_t>(base, disk.bone_indices);
    if (source_vertices == nullptr || source_bones == nullptr) {
        return false;
    }

    std::vector<xbox_skin_triangle> triangles;
    if (!decode_triangles(base, disk, triangles)) {
        return false;
    }

    xbox_skin_batch current;
    for (const auto &triangle : triangles) {
        std::vector<uint16_t> triangle_bones;
        for (const auto vertex_index : triangle) {
            const auto &vertex = source_vertices[vertex_index];
            for (int component = 0; component < 4; ++component) {
                if (vertex.bone_weights[component] == 0) {
                    continue;
                }

                const int local_bone = vertex.bone_indices[component];
                if (local_bone < 0 || local_bone >= disk.bone_count) {
                    sp_log("XBXM skinned vertex uses invalid local bone %d", local_bone);
                    return false;
                }
                append_unique_bone(triangle_bones, source_bones[local_bone]);
            }
        }

        size_t merged_bone_count = current.bones.size();
        for (const auto bone : triangle_bones) {
            if (std::find(current.bones.begin(), current.bones.end(), bone) == current.bones.end()) {
                ++merged_bone_count;
            }
        }

        if (merged_bone_count > PC_SKIN_BONE_LIMIT && !current.triangles.empty()) {
            batches.push_back(current);
            current = {};
        }

        for (const auto bone : triangle_bones) {
            append_unique_bone(current.bones, bone);
        }
        if (current.bones.size() > PC_SKIN_BONE_LIMIT) {
            return false;
        }
        current.triangles.push_back(triangle);
    }

    if (!current.triangles.empty()) {
        batches.push_back(current);
    }
    return !batches.empty();
}

uint32_t encode_pointer(uint32_t base, const void *pointer)
{
    return bit_cast<uint32_t>(pointer) - base;
}

bool should_split_section(uint32_t base, const nglMeshSection *section)
{
    xbox_ngl_mesh_section disk {};
    std::memcpy(&disk, section, sizeof(disk));
    if (disk.bone_count <= PC_SKIN_BONE_LIMIT ||
        disk.stride != sizeof(xbox_skinned_vertex)) {
        return false;
    }

    const auto *vertex_def = rebase_pointer<nglVertexDef>(base, disk.vertex_def);
    return vertex_def != nullptr &&
           is_skinned_vertex_def(bit_cast<uint32_t>(vertex_def->m_vtbl));
}

// Morph records keep their original frame addresses because animation players
// retain pointers to individual frames. Only the section tables are replaced.
// The tables/streams belong to the morph file, never the mesh file: streaming
// may unload either resource first.
bool remap_matching_morphs()
{
    struct frame_update {
        xbox_morph::frame *target;
        uint32_t section_count;
        uint32_t sections;
    };
    struct morph_update {
        xbox_morph_runtime *target;
        uint32_t old_count;
        uint32_t new_count;
        std::vector<frame_update> frames;
    };
    std::vector<morph_update> updates;
    for (auto &morph_owner_entry : runtime_owners()) {
        auto &owner = *morph_owner_entry.second;
        for (auto &morph : owner.morphs) {
            if (morph.remapped) continue;
            const xbox_morph::mesh_map *mapping = nullptr;
            for (const auto &mesh_owner_entry : runtime_owners()) {
                for (const auto &candidate : mesh_owner_entry.second->mesh_maps) {
                    if (candidate.hash == morph.hash) {
                        mapping = &candidate;
                        break;
                    }
                }
                if (mapping != nullptr) break;
            }
            if (mapping == nullptr) continue;
            morph_update update {&morph, mapping->source_sections,
                static_cast<uint32_t>(mapping->sections.size()), {}};
            if (!mapping->changed) {
                updates.push_back(std::move(update));
                continue;
            }
            if (morph.set->field_4 < 0 || morph.frames == nullptr) return false;
            auto decode = [&](uint32_t value) -> uint32_t {
                return value == 0 ? 0 : value + (morph.rebased ? 0 : morph.pointer_base);
            };
            auto encode = [&](const void *value) -> uint32_t {
                return bit_cast<uint32_t>(value) - (morph.rebased ? 0 : morph.pointer_base);
            };
            const auto image_begin = bit_cast<uint32_t>(owner.file->FileBuf.Buf);
            const auto image_size = owner.file->FileBuf.Size;
            auto in_image = [&](uint32_t pointer, size_t bytes) {
                // Packed files can have Size == 0, as in the native loader.
                return image_size == 0 || (pointer >= image_begin &&
                    pointer - image_begin <= image_size &&
                    bytes <= image_size - (pointer - image_begin));
            };
            for (int frame_index = 0; frame_index < morph.set->field_4; ++frame_index) {
                auto &frame = morph.frames[frame_index];
                if (frame.section_count != mapping->source_sections ||
                    !in_image(decode(frame.sections),
                        size_t(frame.section_count) * sizeof(xbox_morph::section))) {
                    sp_log("XBXM morph %08x has incompatible source sections", morph.hash);
                    return false;
                }
                auto *source = bit_cast<const xbox_morph::section *>(decode(frame.sections));
                auto *sections = static_cast<xbox_morph::section *>(owner.allocate(
                    sizeof(xbox_morph::section) * mapping->sections.size(), 8));
                if (source == nullptr || sections == nullptr) return false;
                for (size_t i = 0; i < mapping->sections.size(); ++i) {
                    const auto &map = mapping->sections[i];
                    auto &section = sections[i];
                    section = source[map.source_section];
                    if (!map.vertices.empty() && section.component_mask != 0) {
                        // All v10 facial morph assets use sparse float3 position
                        // deltas. Reject an unsupported split stream explicitly
                        // instead of applying it to unrelated vertex indices.
                        if (section.component_mask != 1 ||
                            section.vertex_count != map.source_vertices ||
                            section.streams[0][0] == 0) {
                            sp_log("XBXM morph %08x has unsupported split components", morph.hash);
                            return false;
                        }
                        for (size_t component = 0; component < 8; ++component)
                            for (size_t format = 0; format < 4; ++format)
                                if ((component != 0 || format != 0) && section.streams[component][format])
                                    return false;
                        const auto stream = decode(section.streams[0][0]);
                        if (!in_image(stream, 4)) return false;
                        const size_t bytes = image_size != 0
                            ? image_size - (stream - image_begin)
                            : size_t(map.source_vertices) * 16 + 4;
                        std::vector<uint8_t> remapped;
                        if (!xbox_morph::remap_positions(bit_cast<const uint8_t *>(stream),
                                bytes, map.source_vertices, map.vertices, remapped)) {
                            sp_log("XBXM morph %08x has an invalid position run", morph.hash);
                            return false;
                        }
                        auto *runtime_stream = static_cast<uint8_t *>(owner.allocate(remapped.size(), 8));
                        if (runtime_stream == nullptr) return false;
                        std::memcpy(runtime_stream, remapped.data(), remapped.size());
                        section.streams[0][0] = encode(runtime_stream);
                    } else {
                        // Native nglProcessMorph will rebase a newly loaded
                        // morph; existing morph frames already contain pointers.
                        // Both are copied in the representation expected there.
                    }
                    if (!map.vertices.empty() && section.vertex_count != 0)
                        section.vertex_count = static_cast<uint32_t>(map.vertices.size());
                }
                update.frames.push_back({&frame, update.new_count, encode(sections)});
            }
            updates.push_back(std::move(update));
        }
    }
    // No frame visible to a live animation changes on a failed preparation.
    for (auto &update : updates) {
        for (const auto &frame : update.frames) {
            frame.target->section_count = frame.section_count;
            frame.target->sections = frame.sections;
        }
        update.target->remapped = true;
        if (!update.frames.empty())
            sp_log("XBXM morph %08x remapped %u to %u sections",
                update.target->hash, update.old_count, update.new_count);
    }
    return true;
}

bool split_section(xbox_mesh_runtime &owner, uint32_t base,
                   nglMeshFile *mesh_file,
                   nglMeshSection *source_section,
                   std::vector<nglMeshSection *> &output_sections,
                   xbox_morph::mesh_map &morph_map, uint32_t source_section_index)
{
    xbox_ngl_mesh_section disk {};
    std::memcpy(&disk, source_section, sizeof(disk));

    const auto *source_vertices = rebase_pointer<xbox_skinned_vertex>(base, disk.vertices);
    const auto *source_bones = rebase_pointer<uint16_t>(base, disk.bone_indices);
    const auto *source_vertex_def = rebase_pointer<nglVertexDef>(base, disk.vertex_def);
    if (source_vertices == nullptr || source_bones == nullptr || source_vertex_def == nullptr) {
        return false;
    }

    std::vector<xbox_skin_batch> batches;
    if (!build_skin_batches(base, disk, batches)) {
        return false;
    }

    for (size_t batch_index = 0; batch_index < batches.size(); ++batch_index) {
        const auto &batch = batches[batch_index];
        std::vector<int32_t> vertex_remap(static_cast<size_t>(disk.vertex_count), -1);
        std::vector<xbox_skinned_vertex> batch_vertices;
        xbox_morph::section_map batch_map {source_section_index,
            static_cast<uint32_t>(disk.vertex_count), {}};
        std::vector<uint16_t> batch_indices;
        batch_indices.reserve(batch.triangles.size() * 3);

        for (const auto &triangle : batch.triangles) {
            for (const auto source_index : triangle) {
                auto &mapped_index = vertex_remap[source_index];
                if (mapped_index < 0) {
                    if (batch_vertices.size() > std::numeric_limits<uint16_t>::max()) {
                        return false;
                    }

                    auto vertex = source_vertices[source_index];
                    for (int component = 0; component < 4; ++component) {
                        if (vertex.bone_weights[component] == 0 ||
                            vertex.bone_indices[component] < 0) {
                            vertex.bone_indices[component] = -1;
                            continue;
                        }

                        const auto global_bone = source_bones[vertex.bone_indices[component]];
                        const auto found = std::find(batch.bones.begin(), batch.bones.end(), global_bone);
                        if (found == batch.bones.end()) {
                            return false;
                        }
                        vertex.bone_indices[component] = static_cast<int8_t>(
                            std::distance(batch.bones.begin(), found));
                    }

                    mapped_index = static_cast<int32_t>(batch_vertices.size());
                    batch_vertices.push_back(vertex);
                    batch_map.vertices.push_back(source_index);
                }
                batch_indices.push_back(static_cast<uint16_t>(mapped_index));
            }
        }

        auto *runtime_vertices = static_cast<xbox_skinned_vertex *>(
            owner.allocate(sizeof(xbox_skinned_vertex) * batch_vertices.size(), 16));
        auto *runtime_indices = static_cast<uint16_t *>(
            owner.allocate(sizeof(uint16_t) * batch_indices.size(), 8));
        auto *runtime_bones = static_cast<uint16_t *>(
            owner.allocate(sizeof(uint16_t) * batch.bones.size(), 8));
        auto *runtime_vertex_def = static_cast<nglVertexDef *>(
            owner.allocate(sizeof(nglVertexDef), 8));
        auto *runtime_section = batch_index == 0
            ? source_section
            : static_cast<nglMeshSection *>(
                  owner.allocate(sizeof(nglMeshSection), 8));
        if (runtime_vertices == nullptr || runtime_indices == nullptr ||
            runtime_bones == nullptr || runtime_vertex_def == nullptr ||
            runtime_section == nullptr) {
            return false;
        }

        std::memcpy(runtime_vertices,
                    batch_vertices.data(),
                    sizeof(xbox_skinned_vertex) * batch_vertices.size());
        std::memcpy(runtime_indices,
                    batch_indices.data(),
                    sizeof(uint16_t) * batch_indices.size());
        std::memcpy(runtime_bones,
                    batch.bones.data(),
                    sizeof(uint16_t) * batch.bones.size());
        std::memcpy(runtime_vertex_def, source_vertex_def, sizeof(nglVertexDef));

        auto runtime_disk = disk;
        runtime_disk.bone_count = static_cast<int32_t>(batch.bones.size());
        runtime_disk.bone_indices = encode_pointer(base, runtime_bones);
        runtime_disk.primitive_type = xbox_triangle_list;
        runtime_disk.index_count = static_cast<int32_t>(batch_indices.size());
        runtime_disk.indices = encode_pointer(base, runtime_indices);
        runtime_disk.runtime_index_buffer = 0;
        runtime_disk.secondary_vertices = 0;
        runtime_disk.secondary_vertices_size = 0;
        runtime_disk.vertex_count = static_cast<int32_t>(batch_vertices.size());
        runtime_disk.vertices = encode_pointer(base, runtime_vertices);
        runtime_disk.vertices_size = static_cast<uint32_t>(
            sizeof(xbox_skinned_vertex) * batch_vertices.size());
        runtime_disk.stride = sizeof(xbox_skinned_vertex);
        runtime_disk.vertex_def = encode_pointer(base, runtime_vertex_def);

        std::memcpy(runtime_section, &runtime_disk, sizeof(runtime_disk));
        if (!convert_section(owner, base, mesh_file, runtime_section)) {
            return false;
        }
        output_sections.push_back(runtime_section);
        morph_map.sections.push_back(std::move(batch_map));
        morph_map.changed = true;
    }

    return true;
}
}

static bool load_xbox_mesh_image(xbox_mesh_runtime &owner,
                                 const tlFixedString &FileName,
                                 nglMeshFile *mesh_file,
                                 const char *ext)
{
    TRACE("nglLoadMeshFileInternalXbox", FileName.to_string());

    if (mesh_file == nullptr || mesh_file->FileBuf.Buf == nullptr) {
        return false;
    }

    auto *header = bit_cast<xbox_ngl_mesh_file_header *>(mesh_file->FileBuf.Buf);
    mesh_file->field_134 = bit_cast<int>(header);
    mesh_file->field_144 = -1;

    if (std::memcmp(header->tag, "XBXM", 4) != 0) {
        return false;
    }

    constexpr uint32_t xbox_mesh_version = 0x1601;
    if (header->version != xbox_mesh_version) {
        sp_log("Unsupported Xbox mesh file version: %s%s (version %x, expected %x)",
               FileName.to_string(),
               ext,
               header->version,
               xbox_mesh_version);
        return false;
    }

    if (header->directory_count == 0 || header->directory == 0) {
        return false;
    }

    const auto buffer = bit_cast<uint32_t>(mesh_file->FileBuf.Buf);
    if (header->old_base == buffer) {
        return true;
    }

    const uint32_t base = buffer - header->old_base;
    auto *entries = rebase_pointer<xbox_ngl_directory_entry>(base, header->directory);
    header->directory = bit_cast<uint32_t>(entries);

    mesh_file->FirstMesh = nullptr;
    mesh_file->FirstMaterial = nullptr;
    mesh_file->FirstMorph = nullptr;

    nglMesh *last_mesh = nullptr;
    nglMaterialBase *last_material = nullptr;
    nglMorphSet *last_morph = nullptr;
    std::vector<std::pair<nglMorphSet *, uint32_t>> pending_morphs;

    for (uint32_t i = 0; i < header->directory_count; ++i) {
        auto &entry = entries[i];
        entry.object = bit_cast<uint32_t>(rebase_pointer<void>(base, entry.object));

        switch (entry.type) {
        case TypeDirectoryEntry::MATERIAL: {
            auto *material = bit_cast<nglMaterialBase *>(entry.object);
            const uint32_t name_hash = bit_cast<uint32_t>(material->Name);
            uint32_t shader_hash = bit_cast<uint32_t>(material->m_shader);
            bool converted_captive_material = false;

            if (xbpack_v10_captive_material::shader_for(owner.resource_hash,
                    shader_hash) != shader_hash) {
                const uint32_t stored_size = uint32_t(entry.field_0) |
                    (uint32_t(entry.field_1) << 8u) |
                    (uint32_t(entry.field_2) << 16u);
                xbpack_v10_captive_material::person_material compatible{};
                if (!xbpack_v10_captive_material::upgrade(owner.resource_hash,
                        material, stored_size, compatible)) {
                    sp_log("XBXM V08 captive has an unsupported legacy material profile");
                    return false;
                }
                // The replacement belongs to this private file instance. The
                // v1 source is shorter than USPerson and must not be expanded
                // in place over its following directory object.
                material = static_cast<nglMaterialBase *>(owner.allocate(
                    sizeof(compatible), 16));
                if (material == nullptr) return false;
                std::memcpy(material, &compatible, sizeof(compatible));
                entry.object = bit_cast<uint32_t>(material);
                shader_hash = xbpack_v10_captive_material::person_shader;
                converted_captive_material = true;
            }

            material->Name = make_runtime_fixed_string(owner, name_hash);
            if (material->Name == nullptr) {
                return false;
            }

            auto *shader_node = nglShaderBank.Search(tlHashString {shader_hash});
            if (converted_captive_material && shader_node == nullptr) {
                sp_log("[xbpack] V10 captive USPerson shader 0x%08X unavailable; "
                       "not publishing an invisible mesh", shader_hash);
                return false;
            }
            material->m_shader = shader_node != nullptr
                ? static_cast<nglShader *>(shader_node->field_20)
                : static_cast<nglShader *>(&gEmptyShader());
            material->File = mesh_file;
            material->NextMaterial = nullptr;

            if (mesh_file->FirstMaterial == nullptr) {
                mesh_file->FirstMaterial = material;
            }
            if (last_material != nullptr) {
                last_material->NextMaterial = material;
            }
            last_material = material;

            if (!material->m_shader->CheckMaterialVersion(material)) {
                if (converted_captive_material) {
                    sp_log("[xbpack] V10 captive converted material 0x%08X rejected",
                           name_hash);
                    return false;
                }
                material->m_shader = &gEmptyShader();
            }

            bool uses_hash_names = false;
            if (!fixup_texture_names(
                    owner, material, shader_hash, uses_hash_names)) {
                return false;
            }
            if (!uses_hash_names) {
                material->m_shader->RebaseMaterial(material, base);
            }
            // Reserve rollback bookkeeping before acquiring texture refs.
            owner.materials.push_back(material);
            material->m_shader->BindMaterial(material);
            if (converted_captive_material) ++owner.captive_materials;
            break;
        }

        case TypeDirectoryEntry::MESH: {
            auto *mesh = bit_cast<nglMesh *>(entry.object);
            const uint32_t name_hash = bit_cast<uint32_t>(mesh->Name);
            mesh->Name = make_runtime_fixed_string(owner, name_hash);
            if (mesh->Name == nullptr) {
                return false;
            }

            mesh->Bones = rebase_pointer<math::MatClass<4, 3>>(
                base, bit_cast<uint32_t>(mesh->Bones));
            mesh->LODs = rebase_pointer<nglMesh::Lod>(
                base, bit_cast<uint32_t>(mesh->LODs));
            const uint32_t sections_pointer = bit_cast<uint32_t>(mesh->Sections);
            mesh->Sections = sections_pointer != 0
                ? bit_cast<decltype(mesh->Sections)>(base + sections_pointer)
                : nullptr;

            mesh->File = mesh_file;
            mesh->NextMesh = nullptr;
            if (mesh_file->FirstMesh == nullptr) {
                mesh_file->FirstMesh = mesh;
            }
            if (last_mesh != nullptr) {
                last_mesh->NextMesh = mesh;
            }
            last_mesh = mesh;

            const uint32_t source_section_count = mesh->NSections;
            xbox_morph::mesh_map morph_map {name_hash, source_section_count};
            auto *source_section_refs = mesh->Sections;
            std::vector<nglMeshSection *> converted_sections;
            converted_sections.reserve(source_section_count);

            for (uint32_t section_index = 0;
                 section_index < source_section_count;
                 ++section_index) {
                auto *section = rebase_pointer<nglMeshSection>(
                    base, bit_cast<uint32_t>(source_section_refs[section_index].Section));
                if (section == nullptr) {
                    return false;
                }

                if (should_split_section(base, section)) {
                    if (!split_section(
                            owner, base, mesh_file, section, converted_sections,
                            morph_map, section_index)) {
                        return false;
                    }
                } else {
                    xbox_ngl_mesh_section disk {};
                    std::memcpy(&disk, section, sizeof(disk));
                    morph_map.sections.push_back({section_index,
                        static_cast<uint32_t>(disk.vertex_count), {}});
                    if (!convert_section(owner, base, mesh_file, section)) {
                        return false;
                    }
                    converted_sections.push_back(section);
                }
            }

            if (converted_sections.size() > std::numeric_limits<uint32_t>::max()) {
                return false;
            }

            if (converted_sections.empty()) {
                mesh->Sections = nullptr;
            } else {
                auto *runtime_section_refs = static_cast<decltype(mesh->Sections)>(
                    owner.allocate(sizeof(*mesh->Sections) * converted_sections.size(), 8));
                if (runtime_section_refs == nullptr) {
                    return false;
                }

                for (size_t section_index = 0;
                     section_index < converted_sections.size();
                     ++section_index) {
                    runtime_section_refs[section_index].field_0 = 1;
                    runtime_section_refs[section_index].Section = converted_sections[section_index];
                }
                mesh->Sections = runtime_section_refs;
            }
            mesh->NSections = static_cast<uint32_t>(converted_sections.size());
            owner.mesh_maps.push_back(std::move(morph_map));
            break;
        }

        case TypeDirectoryEntry::MORPH: {
            auto *morph = bit_cast<nglMorphSet *>(entry.object);
            const uint32_t morph_hash = morph->field_0.field_0;
            auto *name = make_runtime_fixed_string(owner, morph_hash);
            if (name == nullptr) {
                return false;
            }
            morph->field_0.field_0 =
                encode_pointer(base, name);
            owner.morphs.push_back({morph_hash, morph,
                rebase_pointer<xbox_morph::frame>(base,
                    bit_cast<uint32_t>(morph->field_8)), base});
            pending_morphs.emplace_back(morph, entry.aux_hash);
            break;
        }

        default:
            sp_log("XBXM file %s%s contains unknown directory entry type %u",
                   FileName.to_string(),
                   ext,
                   static_cast<unsigned>(entry.type));
            break;
        }
    }

    for (auto *mesh = mesh_file->FirstMesh; mesh != nullptr; mesh = mesh->NextMesh) {
        if (mesh->NBones == 0) {
            mesh->Flags |= NGLMESH_PROCESSED;
        } else {
            for (int bone = 0; bone < mesh->NBones; ++bone) {
                mesh->Bones[bone] = sub_4150E0(mesh->Bones[bone]);
            }
        }

#ifdef OPENUSM_XBPACK_V10
        const int lod_count = mesh->NLODs;
        int resolved_lods = 0;
        for (int lod = 0; lod < lod_count; ++lod) {
            const uint32_t lod_hash = bit_cast<uint32_t>(mesh->LODs[lod].field_0);
            auto *lod_mesh = find_mesh(mesh_file, lod_hash);
            if (lod_mesh == nullptr) {
                continue;
            }

            if (resolved_lods != lod) {
                mesh->LODs[resolved_lods] = mesh->LODs[lod];
            }
            mesh->LODs[resolved_lods].field_0 = lod_mesh;
            ++resolved_lods;
        }
        mesh->NLODs = resolved_lods;
#else
        for (int lod = 0; lod < mesh->NLODs; ++lod) {
            const uint32_t lod_hash = bit_cast<uint32_t>(mesh->LODs[lod].field_0);
            mesh->LODs[lod].field_0 = find_mesh(mesh_file, lod_hash);
        }
#endif
    }

    // Morphs and meshes are separate resources, and either can arrive first.
    // Prepare every matching table before changing any live frame pointer.
    if (!remap_matching_morphs()) return false;

    // All conversions and allocations have succeeded. Publish only now: a
    // failed section used to leave registered meshes pointing at a freed file.
    for (const auto &pending : pending_morphs) {
        auto *morph = pending.first;
        if (mesh_file->FirstMorph == nullptr) mesh_file->FirstMorph = morph;
        nglDirectoryEntry compatible_entry{};
        compatible_entry.field_3 = TypeDirectoryEntry::MORPH;
        compatible_entry.field_4.Morph = morph;
        compatible_entry.field_8 = bit_cast<void *>(pending.second);
        nglProcessMorph(mesh_file, &compatible_entry, static_cast<int>(base));
        if (last_morph != nullptr) last_morph->field_10 = morph;
        last_morph = morph;
    }
    if (last_morph != nullptr) last_morph->field_10 = nullptr;
    for (auto &morph : owner.morphs) morph.rebased = true;
    for (auto *mesh = mesh_file->FirstMesh; mesh != nullptr; mesh = mesh->NextMesh) {
        auto add_mesh = bit_cast<void (__fastcall *)(void *, void *, nglMesh *)>(
            get_vfunc(nglMeshDirectory()->m_vtbl, 0x10));
        add_mesh(nglMeshDirectory(), nullptr, mesh);
    }
    header->old_base = buffer;
    // sp_log("Loaded Xbox mesh file %s%s with %u directory entries",
    //        FileName.to_string(), ext, header->directory_count);
    return true;
}

bool nglLoadMeshFileInternalXbox(const tlFixedString &name,
                               nglMeshFile *mesh_file, const char *ext)
{
    if (mesh_file == nullptr || mesh_file->FileBuf.Buf == nullptr ||
        (mesh_file->FileBuf.Size != 0 && mesh_file->FileBuf.Size < 0x14)) return false;
    auto *buffer = mesh_file->FileBuf.Buf;
    auto *header = bit_cast<xbox_ngl_mesh_file_header *>(buffer);
    if (std::memcmp(header->tag, "XBXM", 4) != 0 ||
        header->version != modmesh::xbmeshformat::version) return false;

    try {
        auto &owners = runtime_owners();
        auto found = owners.find(buffer);
        if (found != owners.end()) {
            // Each consumer must own a private writable image. A second file
            // must not adopt names/streams which the first file will release.
            return found->second->file == mesh_file && found->second->committed &&
                header->old_base == bit_cast<uint32_t>(buffer);
        }
        if (header->old_base == bit_cast<uint32_t>(buffer)) {
            sp_log("XBXM %s%s has no owner for its already-rebased image",
                   name.to_string(), ext);
            return false;
        }
        // Packed mesh images may have Size == 0. Keep their existing trusted
        // engine path; bounds-check pristine files whenever a size is known.
        if (mesh_file->FileBuf.Size != 0 && header->old_base == 0) {
            const char *reason = nullptr;
            if (!modmesh::xbmeshformat::imageUsable(
                    reinterpret_cast<const uint8_t *>(buffer),
                    mesh_file->FileBuf.Size, &reason)) {
                sp_log("XBXM %s%s rejected: %s", name.to_string(), ext, reason);
                return false;
            }
        }
        // Allocate the registry node before parsing; committing a finished
        // image must not introduce another allocation/failure after publish.
        const auto resource_hash = xbpack::v10_mesh_identity::resource_hash(
            mesh_file->FileName.m_hash, name.m_hash);
        auto inserted = owners.emplace(buffer,
            std::make_unique<xbox_mesh_runtime>(mesh_file, resource_hash));
        if (!load_xbox_mesh_image(*inserted.first->second, name, mesh_file, ext)) {
            owners.erase(inserted.first);
            return false;
        }
        auto &owner = *inserted.first->second;
        owner.committed = true;
        if (xbpack_v10_captive_material::enabled_for(owner.resource_hash)) {
            sp_log("[xbpack] V10 captive material conversion active: "
                   "resource=0x%08X materials=%u vertex_defs=%u sections=%u",
                   owner.resource_hash, owner.captive_materials,
                   owner.captive_vertex_defs,
                   static_cast<unsigned>(owner.sections.size()));
        }
        return true;
    } catch (const std::bad_alloc &) {
        // Uncommitted owners release any buffers/materials allocated so far.
        // The registry's first allocation can itself be the failed request.
        if (auto *owners = runtime_owners_pointer()) owners->erase(buffer);
        sp_log("XBXM %s%s conversion ran out of memory", name.to_string(), ext);
        return false;
    }
}

void modReleaseXboxMeshFileResources(tlFileBuf *buffer, bool releaseMaterials)
{
    if (buffer == nullptr || buffer->Buf == nullptr) return;
    auto *owners = runtime_owners_pointer();
    if (owners == nullptr) return;
    auto found = owners->find(buffer->Buf);
    if (found == owners->end()) return;
    found->second->packed_unload = releaseMaterials;
    owners->erase(found);
}

#endif
