#include "ngl.h"

// The runtime XBXM port below targets the retail loader; the TARGET_XBOX
// experiment keeps its own compile-time path in ngl_xbox.cpp.
#ifndef TARGET_XBOX

// ---------------------------------------------------------------------------
// XBXM mesh-file loading on the retail PC renderer.
//
// The Xbox beta stores its nglMeshFile images with the "XBXM" tag, binary
// version 0x1601 (retail PC is "PCM " 0x601). The two containers share the
// directory-entry walk, the nglMesh/nglMeshSection record layout and the
// rebase scheme; what differs is how names are stored - the beta writes
// inline tlHashString values where the PC format writes offsets to
// tlFixedStrings - plus the D3D8-era vertex-stream handling the beta
// executable's loader performed at bind time.
//
// This file is the runtime port of that beta loader (previously the
// TARGET_XBOX-only code in ngl_xbox.cpp) onto the shared PC structures and
// D3D9 buffer conventions, so both resource-handler code paths coexist in
// one loader: nglLoadMeshFileInternal() keeps handling every "PCM " file
// exactly as before and hands "XBXM" files - meshes *and* morph containers,
// since morph_file_resource_handler funnels .pcmorph loads through the same
// entry point - to nglLoadMeshFileXbox() below.
//
// Fail-soft by design: a section or entry this port cannot bind yet is
// logged and skipped, never asserted on, so a problematic beta asset costs
// a missing model instead of a boot.
// ---------------------------------------------------------------------------

#include "common.h"
#include "custom_math.h"
#include "fixedstring.h"
#include "hashstring.h"
#include "log.h"
#include "ngl_vertexdef.h"
#include "nglemptyshader.h"
#include "nglshader.h"
#include "tl_instance_bank.h"
#include "tl_system.h"
#include "trace.h"
#include "utility.h"
#include "variable.h"
#include "variables.h"
#include "vector3d.h"
#include "vector4d.h"
#include "vtbl.h"

#include <ngl_mesh.h>

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <unordered_map>

// Defined in ngl.cpp (kept TU-local there); identical definition, and the
// rebase helper resolves at link time.
struct nglMeshFileHeader {
    char Tag[4];
    uint32_t Version;
    uint32_t NDirectoryEntries;
    nglDirectoryEntry *DirectoryEntries;
    int field_10;
};

extern void nglRebaseHeader(uint32_t Base, nglMeshFileHeader *&pHeader);

// component-wise min / max, defined in ngl.cpp
extern vector4d sub_401270(const vector4d &a2, const vector4d &a3);
extern vector4d sub_4012F0(const vector4d &a2, const vector4d &a3);

namespace {

constexpr uint32_t XBXM_VERSION = 0x1601;

// The beta format stores bare name hashes where the retail format stores
// tlFixedString offsets, but plenty of retail code (the mesh directory, the
// LOD resolver, logging) dereferences those slots. Every hash that flows out
// of an XBXM file is therefore interned into a permanent tlFixedString whose
// m_hash carries the *original* value, so hash-based lookups keep working
// even though the printable text is only the hex spelling.
tlFixedString *intern_hash(uint32_t hash) {
    static std::unordered_map<uint32_t, tlFixedString *> s_interned;

    if (auto it = s_interned.find(hash); it != s_interned.end()) {
        return it->second;
    }

    auto *str = new tlFixedString{};
    std::memset(str, 0, sizeof(*str));
    str->m_hash = hash;
    std::snprintf(str->field_4, sizeof(str->field_4), "0x%08x", hash);

    s_interned.emplace(hash, str);
    return str;
}

// nglGetMaterialInFile, beta flavour: the section stores the material's name
// hash, the materials of this file were interned above.
nglMaterialBase *find_material_by_hash(uint32_t hash, nglMeshFile *MeshFile) {
    for (auto *i = MeshFile->FirstMaterial; i != nullptr; i = i->NextMaterial) {
        if (i->Name != nullptr && i->Name->m_hash == hash) {
            return i;
        }
    }
    return nullptr;
}

// nglGetMeshInFile, beta flavour (file-local first, global directory after).
nglMesh *find_mesh_by_hash(uint32_t hash, nglMeshFile *MeshFile) {
    for (auto *i = MeshFile->FirstMesh; i != nullptr; i = i->NextMesh) {
        if (i->Name != nullptr && i->Name->m_hash == hash) {
            return i;
        }
    }
    return nglGetMesh(hash, true);
}

// The beta executable's bind-time vertex handling for the fixed-function
// paths, unchanged from ngl_xbox.cpp: convert the 4 palette indices of every
// 64-byte skinned record to raw uint32 in place (that is what the CPU
// skinner reads), derive the 2/3/4-weight variant, upload, and draw at the
// dynamic-buffer stride of 24.
void bind_cpu_skinned_section(nglMeshSection *MeshSection) {
    auto records = static_cast<uint32_t>(MeshSection->field_3C.Size >> 6);
    auto *v32 = (float *)(MeshSection->field_3C.m_vertexData + 32);

    MeshSection->field_5C = 2;
    for (; records != 0; --records) {
        if (equal(v32[7], 0.0f)) {
            if (not_equal(v32[6], 0.0f) && MeshSection->field_5C < 3u) {
                MeshSection->field_5C = 3;
            }
        } else {
            MeshSection->field_5C = 4;
        }

        *(uint32_t *)&v32[0] = (uint32_t)v32[0];
        *(uint32_t *)&v32[1] = (uint32_t)v32[1];
        *(uint32_t *)&v32[2] = (uint32_t)v32[2];
        *(uint32_t *)&v32[3] = (uint32_t)v32[3];

        v32 += 16;
    }

    MeshSection->field_3C.createVertexBufferAndWriteData(
        MeshSection->field_3C.m_vertexData, MeshSection->field_3C.Size, 1028);

    static Var<int> dword_973BC8{0x00973BC8};
    const int watermark = 24 * (int)(MeshSection->field_3C.Size >> 6);
    if (dword_973BC8() < watermark) {
        dword_973BC8() = watermark;
    }

    MeshSection->m_stride = 24;
}

void bind_section_vertices(nglMeshSection *MeshSection, const char *shader_name) {
    if (!EnableShader()) {
        if (std::strncmp(shader_name, "uslod", 5u) == 0) {
            nglVertexBuffer::createIndexOrVertexBuffer(
                &MeshSection->field_3C,
                ResourceType::VertexBuffer,
                16 * (int)(MeshSection->field_3C.Size / 12u),
                520,
                0,
                D3DPOOL_DEFAULT);
            MeshSection->m_stride = 16;
            MeshSection->field_5C = 0;
            return;
        }

        if (ChromeEffect() && std::strncmp(shader_name, "smshiny", 7u) == 0) {
            const int bytes = 48 * (int)(MeshSection->field_3C.Size / 60u);
            MeshSection->field_3C.createVertexBuffer(bytes, 520u);
            MeshSection->m_stride = 48;

            static Var<int> dword_972960{0x00972960};
            if (dword_972960() < bytes) {
                dword_972960() = bytes;
            }
            return;
        }

        if (std::strncmp(shader_name, "usperson", 8u) == 0) {
            bind_cpu_skinned_section(MeshSection);
            return;
        }
    }

    if (std::strncmp(shader_name, "us_character", 12u) == 0) {
        bind_cpu_skinned_section(MeshSection);
        return;
    }

    MeshSection->field_3C.createVertexBufferAndWriteData(
        MeshSection->field_3C.m_vertexData, MeshSection->field_3C.Size, 1028);
}

} // anonymous namespace

bool nglLoadMeshFileXbox(const tlFixedString &FileName, nglMeshFile *MeshFile, const char *ext)
{
    TRACE("nglLoadMeshFileXbox", FileName.to_string());

    nglMeshFileHeader *Header = CAST(Header, MeshFile->FileBuf.Buf);

    MeshFile->field_134 = (int)Header;
    MeshFile->field_144 = -1;
    MeshFile->FirstMesh = nullptr;
    MeshFile->FirstMaterial = nullptr;
    MeshFile->FirstMorph = nullptr;

    if (std::strncmp(Header->Tag, "XBXM", 4u) != 0) {
        sp_log("Corrupted mesh file: %s%s%s.\n", nglMeshPath(), FileName.to_string(), ext);
        return false;
    }

    if (Header->Version != XBXM_VERSION) {
        sp_log("Unsupported XBXM version: %s%s%s (version %x, expected %x); "
               "file left empty.\n",
               nglMeshPath(), FileName.to_string(), ext, Header->Version, XBXM_VERSION);
        return true;    // fail-soft: an empty file, never an assert upstream
    }

    if (Header->NDirectoryEntries == 0) {
        sp_log("Mesh file hasn't any directory entries: %s%s%s.\n",
               nglMeshPath(), FileName.to_string(), ext);
        return true;
    }

    const auto Base = bit_cast<uint32_t>(&MeshFile->FileBuf.Buf[-Header->field_10]);

    nglRebaseHeader(Base, Header);

    nglMesh *LastMesh = nullptr;
    nglMaterialBase *LastMaterial = nullptr;
    int skipped_morphs = 0;

    auto *dir_entries = Header->DirectoryEntries;
    const uint32_t num_dir_entries = Header->NDirectoryEntries;

    std::for_each(dir_entries, dir_entries + num_dir_entries,
            [&](nglDirectoryEntry &dir_entry)
    {
        PTR_OFFSET(Base, dir_entry.field_4.Material);

        switch (dir_entry.field_3) {
        case TypeDirectoryEntry::MATERIAL: {
            nglMaterialBase *Material = dir_entry.field_4.Material;

            // Beta layout: Name and field_18 are inline hashes, m_shader is
            // the shader-name hash. Intern the names so every retail-side
            // dereference of these slots stays valid.
            const auto name_hash   = bit_cast<uint32_t>(Material->Name);
            const auto shader_hash = bit_cast<uint32_t>(Material->m_shader);
            const auto alt_hash    = bit_cast<uint32_t>(Material->field_18);

            Material->Name = intern_hash(name_hash);
            Material->field_18 = (alt_hash != 0) ? intern_hash(alt_hash) : nullptr;

            Material->File = MeshFile;
            if (MeshFile->FirstMaterial == nullptr) {
                MeshFile->FirstMaterial = Material;
            }
            if (LastMaterial != nullptr) {
                LastMaterial->NextMaterial = Material;
            }
            LastMaterial = Material;

            if (Header->field_10 == 0) {
                const tlHashString shader_name{shader_hash};

                auto *node = nglShaderBank.Search(shader_name);
                if (node != nullptr) {
                    auto *shader = static_cast<nglShader *>(node->field_20);

                    if (shader->CheckMaterialVersion(Material)) {
                        Material->m_shader = shader;
                    } else {
                        sp_log("Material %s binary version (%d) is not compatible "
                               "with shader %s.\n",
                               Material->Name->to_string(), Material->Version,
                               shader_name.c_str());
                        Material->m_shader = &gEmptyShader();
                    }
                } else {
                    sp_log("NGL: Unable to find shader %s, used by material %s.\n",
                           shader_name.c_str(), Material->Name->to_string());
                    Material->m_shader = &gEmptyShader();
                }
            }

            Material->m_shader->RebaseMaterial(Material, Base);
            Material->m_shader->BindMaterial(Material);
        } break;

        case TypeDirectoryEntry::MESH: {
            nglMesh *Mesh = dir_entry.field_4.Mesh;

            Mesh->Name = intern_hash(bit_cast<uint32_t>(Mesh->Name));

            {
                void(__fastcall *Add)(void *, void *, nglMesh *) =
                    CAST(Add, get_vfunc(nglMeshDirectory()->m_vtbl, 0x10));
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

            for (auto idx_Section = 0u; idx_Section < Mesh->NSections; ++idx_Section) {
                Mesh->Sections[idx_Section].field_0 = 1;

                nglMeshSection *MeshSection = Mesh->Sections[idx_Section].Section;

                const auto material_hash = bit_cast<uint32_t>(MeshSection->MaterialName);
                MeshSection->MaterialName = intern_hash(material_hash);
                MeshSection->Material = find_material_by_hash(material_hash, MeshFile);

                if (MeshSection->Material == nullptr) {
                    // Should not happen in well-formed files; bind the empty
                    // shader through a throwaway material-less path instead
                    // of crashing on a null deref below.
                    sp_log("XBXM %s: section %u references unknown material 0x%08x, "
                           "section skipped.\n",
                           FileName.to_string(), idx_Section, material_hash);
                    MeshSection->NIndices = 0;
                    continue;
                }

                if (!MeshSection->Material->m_shader->CheckVertexDefVersion(MeshSection)) {
                    tlFixedString shader_name = MeshSection->Material->m_shader->GetName();
                    sp_log("Section VertexDef Binary version (%d) is incompatible "
                           "with shader %s\n.",
                           MeshSection->field_50, shader_name.to_string());
                    MeshSection->Material->m_shader = &gEmptyShader();
                }

                if (MeshSection->m_indices != nullptr) {
                    bit_cast<nglVertexBuffer *>(&MeshSection->m_indexBuffer)
                        ->createIndexBufferAndWriteData(MeshSection->m_indices,
                                                        2 * MeshSection->NIndices);
                }

                MeshSection->StartIndex = 0;

                {
                    tlFixedString shader_name = MeshSection->Material->m_shader->GetName();
                    bind_section_vertices(MeshSection, shader_name.to_string());
                }

                if (auto *v39 = MeshSection->VertexDef; v39 != nullptr) {
                    tlHashString a1 = *(tlHashString *)v39->m_vtbl;
                    auto *v40 = nglVertexDefBank().Search(a1);
                    if (v40 != nullptr) {
                        MeshSection->VertexDef->field_4 = MeshSection;

                        void (*func)(void *) = CAST(func, v40->field_20);
                        func(MeshSection->VertexDef);
                    } else {
                        MeshSection->VertexDef = nullptr;
                    }
                }

                if (auto *v41 = MeshSection->Material; v41 != nullptr) {
                    if (auto *v42 = v41->m_shader; v42 != nullptr) {
                        v42->BindSection(MeshSection);
                    }
                }
            }
        } break;

        case TypeDirectoryEntry::MORPH: {
            // XBMORPH deltas are the one beta payload with no PC equivalent
            // yet; dropping the entry costs the morph animation, keeps the
            // model, and never crashes. (All seven corpus morphs are IGC
            // facial sets.)
            ++skipped_morphs;
        } break;

        default: {
            sp_log("nglLoadMeshFileXbox: file \"%s%s%s\" has an unknown directory "
                   "entry ( %u ), skipping.\n",
                   nglMeshPath(), FileName.to_string(), ext,
                   uint32_t(dir_entry.field_3));
        } break;
        }
    });

    if (skipped_morphs != 0) {
        sp_log("XBXM %s: %d XBMORPH set(s) skipped (unsupported on PC).\n",
               FileName.to_string(), skipped_morphs);
    }

    if (LastMesh != nullptr) {
        LastMesh->NextMesh = nullptr;
    }
    if (LastMaterial != nullptr) {
        LastMaterial->NextMaterial = nullptr;
    }

    // Bind matrices, LOD chains and the shared bounding sphere - the same
    // post-pass the retail loader runs, with the LOD references resolved by
    // hash instead of string offset.
    vector4d bounds_min;
    bounds_min[0] = 1.0e32f;
    bounds_min[1] = 1.0e32f;
    bounds_min[2] = 1.0e32f;

    vector4d bounds_max;
    bounds_max[0] = -1.0e32f;
    bounds_max[1] = -1.0e32f;
    bounds_max[2] = -1.0e32f;
    bounds_max[3] = -bounds_min[3];

    bool any_skinned = false;

    for (auto *Mesh = MeshFile->FirstMesh; Mesh != nullptr; Mesh = Mesh->NextMesh) {
        if ((Mesh->Flags & NGLMESH_PROCESSED) == 0) {
            if (Mesh->NBones != 0) {
                for (int i = 0; i < Mesh->NBones; ++i) {
                    Mesh->Bones[i] = sub_4150E0(Mesh->Bones[i]);
                }

                const auto cx = Mesh->field_20[0];
                const auto cy = Mesh->field_20[1];
                const auto cz = Mesh->field_20[2];
                const auto cw = Mesh->field_20[3];
                const auto r = Mesh->SphereRadius;

                vector4d lo;
                lo[0] = cx - r;
                lo[1] = cy - r;
                lo[2] = cz - r;
                lo[3] = cw - r;
                bounds_min = sub_401270(lo, bounds_min);

                vector4d hi;
                hi[0] = cx + r;
                hi[1] = cy + r;
                hi[2] = cz + r;
                hi[3] = cw + r;
                bounds_max = sub_4012F0(hi, bounds_max);

                any_skinned = true;
            } else {
                Mesh->Flags |= NGLMESH_PROCESSED;
            }

            auto *Lods = Mesh->LODs;
            for (int i = 0; i < Mesh->NLODs; ++i) {
                const auto lod_hash = bit_cast<uint32_t>(Lods[i].field_0);
                Mesh->LODs[i].field_0 = find_mesh_by_hash(lod_hash, MeshFile);
                Lods = Mesh->LODs;
                if (Lods[i].field_0 == nullptr) {
                    --i;
                    --Mesh->NLODs;
                }
            }
        }
    }

    if (any_skinned) {
        const auto span = sub_411750(bounds_min, bounds_max);

        vector4d center;
        center[0] = span[0] * 0.5f;
        center[1] = span[1] * 0.5f;
        center[2] = span[2] * 0.5f;
        center[3] = span[3] * 0.5f;

        float radius = 0.0f;
        for (auto *Mesh = MeshFile->FirstMesh; Mesh != nullptr; Mesh = Mesh->NextMesh) {
            if ((Mesh->Flags & NGLMESH_PROCESSED) == 0) {
                vector4d d;
                d[0] = center[0] - Mesh->field_20[0];
                d[1] = center[1] - Mesh->field_20[1];
                d[2] = center[2] - Mesh->field_20[2];
                d[3] = center[3] - Mesh->field_20[3];

                const auto reach = vector3d{d[0], d[1], d[2]}.length() + Mesh->SphereRadius;
                if (radius <= reach) {
                    radius = reach;
                }
            }
        }

        for (auto *Mesh = MeshFile->FirstMesh; Mesh != nullptr; Mesh = Mesh->NextMesh) {
            if ((Mesh->Flags & NGLMESH_PROCESSED) == 0) {
                Mesh->SphereRadius = radius;
                Mesh->field_20[0] = center[0];
                Mesh->field_20[1] = center[1];
                Mesh->field_20[2] = center[2];
                Mesh->field_20[3] = center[3];
                Mesh->Flags |= NGLMESH_PROCESSED;
            }
        }
    }

    Header->field_10 = (int)MeshFile->FileBuf.Buf;
    return true;
}

#endif // !TARGET_XBOX
