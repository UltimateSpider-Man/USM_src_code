#include "mesh_file_resource_handler.h"

#include "common.h"
#include "func_wrapper.h"
#include "game.h" // Keep the private mesh-buffer cleanup enabled in this TU.
#include "limited_timer.h"
#include "log.h"
#include "mesh_resource_rebind.h"
#include "ngl.h"
#include "parse_generic_mash.h"
#include "resource_directory.h"
#include "resource_location.h"
#include "resource_pack_slot.h"
#include "return_address.h"
#include "tl_system.h"
#include "tlresource_location.h"
#include "trace.h"
#include "utility.h"
#include "vtbl.h"
#include "worldly_pack_slot.h"
#include "xbpack_v10_captive_material.h"
#include "xbpack_v10_mesh_identity.h"

#include <ngl_mesh.h>

#include <fstream>
#include <iostream>
#include <map>
#include <memory>

VALIDATE_SIZE(mesh_file_resource_handler, 0x14);

#if MOD_MESH_SUPPORT && !defined(TARGET_XBOX)
namespace {
struct PackedSlotBindings {
    // Claims must outlive the per-file snapshots during destruction.
    modmesh::resourcebinding::Claims claims;
    std::map<nglMeshFile *, modmesh::resourcebinding::Snapshot> files;
};

std::map<resource_directory *, PackedSlotBindings> packedMeshBindings;

void rebindPackedMeshResources(nglMeshFile *file, resource_directory &directory)
{
    if (!modMeshFileNeedsResourceRebind(file)) return;

    auto &slot = packedMeshBindings[&directory];
    auto &bindings = slot.files[file];

    const auto bind = [&](tlresource_type type, uint32_t hash, char *object) {
        auto *locations = directory.tlresource_type_to_vector(type);
        if (locations == nullptr) return;
        bindings.rebindMatching(
            locations->data(), locations->size(), hash, object, slot.claims);
    };
    for (auto *mesh = file->FirstMesh; mesh != nullptr; mesh = mesh->NextMesh) {
        if (mesh->Name != nullptr)
            bind(TLRESOURCE_TYPE_MESH, mesh->Name->m_hash,
                 reinterpret_cast<char *>(mesh));
    }
    for (auto *material = file->FirstMaterial; material != nullptr;
         material = material->NextMaterial) {
        if (material->Name != nullptr)
            bind(TLRESOURCE_TYPE_MATERIAL, material->Name->m_hash,
                 reinterpret_cast<char *>(material));
    }
    for (auto *morph = file->FirstMorph; morph != nullptr; morph = morph->field_10) {
        // nglProcessMorph rebases this first dword into a tlFixedString*,
        // despite the legacy nglMorphSet declaration naming it tlHashString.
        const auto *name = reinterpret_cast<const tlFixedString *>(
            static_cast<uintptr_t>(morph->field_0.field_0));
        if (name != nullptr)
            bind(TLRESOURCE_TYPE_MORPH, name->m_hash,
                 reinterpret_cast<char *>(morph));
    }
    if (bindings.empty()) slot.files.erase(file);
    if (slot.files.empty()) packedMeshBindings.erase(&directory);
}

void restorePackedMeshResources(nglMeshFile *file, resource_directory &directory)
{
    const auto slot = packedMeshBindings.find(&directory);
    if (slot == packedMeshBindings.end()) return;
    const auto found = slot->second.files.find(file);
    if (found == slot->second.files.end()) return;
    found->second.restore();
    slot->second.files.erase(found);
    if (slot->second.files.empty()) packedMeshBindings.erase(slot);
}
} // namespace
#endif

mesh_file_resource_handler::mesh_file_resource_handler(worldly_pack_slot *a2)
{
    this->m_vtbl = 0x00888A38;
    this->my_slot = a2;
    this->field_10 = TLRESOURCE_TYPE_MESH_FILE;
}

bool mesh_file_resource_handler::_handle_resource(worldly_resource_handler::eBehavior behavior,
                                                 tlresource_location *loc)
{
    TRACE("mesh_file_resource_handler::handle_resource", loc->name.to_string());
    sp_log("0x%08X", loc->field_8);

    if constexpr (1)
    {
     //   assert(my_slot->get_resource_directory().get_tlresource_count(TLRESOURCE_TYPE_MESH_FILE) ==
           //    my_slot->get_resource_directory().get_resource_count(
             //      RESOURCE_KEY_TYPE_MESH_FILE_STRUCT));

        if (behavior == UNLOAD)
        {
            if (loc->field_8 != nullptr &&
                !nglCanReleaseMeshFile(bit_cast<nglMeshFile *>(loc->field_8))) {
                return true;
            }

            nglMeshFile *MeshFile = CAST(MeshFile, loc->field_8);
            if (MeshFile != nullptr) {
#if MOD_MESH_SUPPORT && !defined(TARGET_XBOX)
                // Detach the slot's references before releasing any resources
                // in the replacement. The unload can yield between meshes;
                // restoration is done once and is safe on the next visit.
                restorePackedMeshResources(MeshFile, my_slot->get_resource_directory());
#endif
                auto *Mesh = MeshFile->FirstMesh;
                if (Mesh != nullptr) {
                LABEL_10:
                    auto *v12 = dword_95C824();
                    while (1) {
                        if (Mesh->NSections != 0 && (Mesh->Sections->field_0 & 4) == 0) {
                            for (auto i = 0u; i < Mesh->NSections; ++i) {
                                nglReleaseSection(Mesh->Sections[i].Section);
                            }

                            Mesh->Sections->field_0 |= 4u;
                            v12 = dword_95C824();
                        }

                        Mesh = Mesh->NextMesh;
                        if (Mesh == nullptr) {
                            break;
                        }

                        if (v12 != nullptr) {
                            if (v12->elapsed() < v12->field_4) {
                                goto LABEL_10;
                            }

                            return true;
                        }
                    }
                }

                for (auto *Mesh = MeshFile->FirstMesh; Mesh != nullptr; Mesh = Mesh->NextMesh) {
                    if (Mesh->NSections != 0)
                    {
                        Mesh->Sections->field_0 &= 0xFFFFFFFB;
                    }
                }
#if MOD_MESH_SUPPORT && !defined(TARGET_XBOX)
                // Packed mesh images are freed with their resource pool, so
                // they never reach tlReleaseFile. All native sections are now
                // released; drop donor materials, textures and CPU snapshots
                // while their owning mesh file is still valid. The helper
                // leaves the original packed buffer under engine ownership.
                modReleaseMeshFileBuffer(&MeshFile->FileBuf, true);
#endif
            }

        }
        else
        { //LOAD


#if defined(OPENUSM_XBPACK_MODE) && defined(OPENUSM_XBPACK_V10) && !defined(TARGET_XBOX)
            // Native LOAD turns the resource ID into a diagnostic string and
            // then hashes that string again at 0x0056BD9A. Supply the real ID
            // for the V08 captive; other resources retain their existing path.
            // At this point field_8 is the raw image, not a live nglMeshFile.
            xbpack::v10_mesh_identity::scoped_source source_identity(
                loc->field_8,
                xbpack_v10_captive_material::enabled_for(loc->name.source_hash_code)
                    ? loc->name.source_hash_code : 0u);
#endif
            bool result = (bool)THISCALL(0x0056BD00, this, behavior, loc);

#if MOD_MESH_SUPPORT && !defined(TARGET_XBOX)
            // Native LOAD updates only the MESH_FILE location. Its child
            // MESH/MATERIAL/MORPH locations still refer to the original pack
            // image after the loader binds a private override. Publish the
            // parsed nodes solely in the slot that owns this resource.
            if (!result && loc->field_8 != nullptr)
                rebindPackedMeshResources(reinterpret_cast<nglMeshFile *>(loc->field_8),
                                          my_slot->get_resource_directory());
#endif
            return result;

#if 0
            REDIRECT(0x0056BD63, parse_generic_mash_init);

#endif

#if 0
            printf("hash = 0x%08X\n", loc->name.source_hash_code);

            auto &res_dir = my_slot->get_resource_directory();
            auto idx = this->field_C +
                res_dir.get_type_start_idxs(RESOURCE_KEY_TYPE_MESH_FILE_STRUCT);

            auto *struct_loc = res_dir.get_resource_location(idx);
            assert(struct_loc != nullptr);

            auto *struct_mash = res_dir.get_resource(struct_loc, nullptr);
            assert(struct_mash != nullptr);

            //sp_log("%d 0x%08X", this->field_C, (int) struct_mash);

            if (Mod* mod = getMod(loc->name.source_hash_code)) {
                //struct_mash = mod->Data.data();
            }

            nglMeshFile *meshFile = nullptr;
            auto alloced_mem = parse_generic_object_mash(meshFile,
                                                         struct_mash,
                                                         nullptr,
                                                         nullptr,
                                                         nullptr,
                                                         0,
                                                         0,
                                                         nullptr);


            assert(!alloced_mem && "This should NOT allocate anything!");

            auto *v5 = loc;
            auto *v7 = loc->name.to_string();
            tlFixedString v20{v7};

            if (!nglLoadMeshFileInternal(v20, meshFile, ".pcmesh"))
            {
                auto *v10 = v5->name.to_string();
                sp_log("Invalid mesh file %s", v10);
                assert(0);
            }

            v5->field_8 = CAST(v5->field_8, meshFile);
#endif

        }

        ++this->field_C;
        return false;

    }
    else
    {
        bool result = (bool) THISCALL(0x0056BD00, this, behavior, loc);

        return result;
    }
}

bool mesh_file_resource_handler::handle(worldly_resource_handler::eBehavior a2, limited_timer *a3) {
    //sp_log("return to 0x%08X", getReturnAddress());

    if constexpr (1) {
        return base_tl_resource_handler::handle(a2, a3);
    } else {
        return (bool) THISCALL(0x00562EC0, this, a2, a3);
    }
}

void mesh_file_resource_handler_patch()
{
    FUNC_ADDRESS(address, &mesh_file_resource_handler::_handle_resource);
    set_vfunc(0x00888A44, address);

    if constexpr (0)
    {
        {
            REDIRECT(0x0056BD63, parse_generic_mash_init);

            REDIRECT(0x0056BDAA, nglLoadMeshFileInternal);

            REDIRECT(0x0056BE3D, nglReleaseSection);
        }

        {
            FUNC_ADDRESS(address, &mesh_file_resource_handler::handle);
            //set_vfunc(0x00888A3C, address);
        }
    }
}
