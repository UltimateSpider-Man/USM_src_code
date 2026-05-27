#include "mesh_file_resource_handler.h"

#include "common.h"
#include "func_wrapper.h"
#include "limited_timer.h"
#include "log.h"
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

#include <ngl_mesh.h>

#include <fstream>
#include <iostream>
#include <memory>

VALIDATE_SIZE(mesh_file_resource_handler, 0x14);

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
            }

        }
        else
        { //LOAD


            bool result = (bool)THISCALL(0x0056BD00, this, behavior, loc);

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
