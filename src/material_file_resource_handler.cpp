#include "material_file_resource_handler.h"

#include "common.h"
#include "ngl.h"
#include "parse_generic_mash.h"
#include "resource_directory.h"
#include "trace.h"
#include "utility.h"
#include "worldly_pack_slot.h"
#include "variables.h"

VALIDATE_SIZE(material_file_resource_handler, 0x14);

#ifdef OPENUSM_XBPACK_V10
namespace {
bool __fastcall handle_xbox_material_file(
    material_file_resource_handler *handler, void *,
    worldly_resource_handler::eBehavior behavior, tlresource_location *location)
{
    auto *file = behavior == worldly_resource_handler::UNLOAD
        ? reinterpret_cast<nglMeshFile *>(location->field_8) : nullptr;
    using native_handler = bool (__fastcall *)(material_file_resource_handler *,
        void *, worldly_resource_handler::eBehavior, tlresource_location *);
    const bool pending = bit_cast<native_handler>(0x0056C190)(
        handler, nullptr, behavior, location);
    if (!pending && file != nullptr) {
        // Native packed-material unload waits for the renderer, then advances
        // the handler without releasing file memory. Xbox conversion also
        // owns hash names, texture references and a registry entry. Remove
        // them while this pack is valid, before the pool reuses its address.
        modReleaseXboxMeshFileResources(&file->FileBuf, true);
    }
    return pending;
}
}
#endif

material_file_resource_handler::material_file_resource_handler(worldly_pack_slot *a2)
{
    this->m_vtbl = 0x00888A94;
    this->my_slot = a2;
    this->field_10 = TLRESOURCE_TYPE_MATERIAL_FILE;
}

bool material_file_resource_handler::_handle(worldly_resource_handler::eBehavior a2, limited_timer *a3)
{
    TRACE("material_file_resource_handler::handle");

    return base_tl_resource_handler::_handle(a2, a3);
}

bool material_file_resource_handler::_handle_resource(worldly_resource_handler::eBehavior behavior,
                                                     tlresource_location *a3)
{
    TRACE("material_file_resource_handler::handle_resource", a3->name.to_string());

    assert(my_slot->get_resource_directory().get_tlresource_count(TLRESOURCE_TYPE_MATERIAL_FILE) ==
           my_slot->get_resource_directory().get_resource_count(
               RESOURCE_KEY_TYPE_MATERIAL_FILE_STRUCT));

    if (behavior == worldly_resource_handler::UNLOAD) {
        auto *MeshFile = bit_cast<nglMeshFile *>(a3->field_8);

        if (MeshFile != nullptr && !nglCanReleaseMeshFile(MeshFile)) {
            return true;
        }
    } else {
        auto &v6 = my_slot->get_resource_directory();
        auto v18 = this->field_C + v6.get_type_start_idxs(RESOURCE_KEY_TYPE_MATERIAL_FILE_STRUCT);
        auto &v7 = my_slot->get_resource_directory();

        auto *struct_loc = v7.get_resource_location(v18);
        assert(struct_loc != nullptr);

        auto &v8 = my_slot->get_resource_directory();
        auto *struct_mash = v8.get_resource(struct_loc, nullptr);
        assert(struct_mash != nullptr);

        nglMeshFile *v4{nullptr};
        auto alloced_mem =
            parse_generic_object_mash(v4, struct_mash, nullptr, nullptr, nullptr, 0, 0, nullptr);
        assert(!alloced_mem && "This should NOT allocate anything!");

        v4->FileBuf.Buf = a3->field_8;
        tlFixedString _14C{a3->name.to_string()};
        if (!nglLoadMeshFileInternal(_14C, v4, resource_key_type_ext[g_platform][23])) {
            sp_log("Invalid material file %s", a3->name.to_string());
            assert(0);
        }

        a3->field_8 = (char *) v4;
    }

    ++this->field_C;

    return false;
}

void material_file_resource_handler_patch()
{
    {
        FUNC_ADDRESS(address, &material_file_resource_handler::_handle);
        set_vfunc(0x00888A98, address);
    }

    FUNC_ADDRESS(address, &material_file_resource_handler::_handle_resource);
    set_vfunc(0x00888AA0, address);
}

void material_file_resource_handler_xbpack_patch()
{
#ifdef OPENUSM_XBPACK_V10
    set_vfunc(0x00888AA0, &handle_xbox_material_file);
#endif
}
