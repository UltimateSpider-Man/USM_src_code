#include "morph_file_resource_handler.h"

#include "common.h"
#include "func_wrapper.h"
#include "ngl.h"
#include "parse_generic_mash.h"
#include "resource_directory.h"
#include "utility.h"
#include "worldly_pack_slot.h"

#include <cassert>

VALIDATE_SIZE(morph_file_resource_handler, 0x14);

#ifdef OPENUSM_XBPACK_V10
namespace {
bool __fastcall handle_xbox_morph_file(
    morph_file_resource_handler *handler, void *,
    worldly_resource_handler::eBehavior behavior, tlresource_location *location)
{
    auto *file = behavior == worldly_resource_handler::UNLOAD
        ? reinterpret_cast<nglMeshFile *>(location->field_8) : nullptr;
    using native_handler = bool (__fastcall *)(morph_file_resource_handler *,
        void *, worldly_resource_handler::eBehavior, tlresource_location *);
    const bool pending = bit_cast<native_handler>(0x0056C080)(
        handler, nullptr, behavior, location);
    if (!pending && file != nullptr) {
        // Packed morph containers use the same Xbox conversion owner as
        // meshes. Retain native render deferral, then drop the added owner
        // before the pack's storage can be reused by another container.
        modReleaseXboxMeshFileResources(&file->FileBuf, true);
    }
    return pending;
}
}
#endif

morph_file_resource_handler::morph_file_resource_handler(worldly_pack_slot *a2)
{
    this->m_vtbl = 0x00888A84;
    this->my_slot = a2;
    this->field_10 = TLRESOURCE_TYPE_MORPH_FILE;
}

bool morph_file_resource_handler::_handle_resource(worldly_resource_handler::eBehavior behavior,
                                                  tlresource_location *tlres_loc)
{
    TRACE("morph_file_resource_handler::handle_resource", tlres_loc->name.to_string());

    if constexpr (1)
    {
        assert(my_slot->get_resource_directory().get_tlresource_count(TLRESOURCE_TYPE_MORPH_FILE) ==
               my_slot->get_resource_directory().get_resource_count(
                   RESOURCE_KEY_TYPE_MORPH_FILE_STRUCT));

        if (behavior == UNLOAD) {
            if (tlres_loc->field_8 != nullptr &&
                !nglCanReleaseMorphFile((nglMorphFile *) tlres_loc->field_8)) {
                return true;
            }

        } else if (behavior == LOAD) {
            auto &res_dir = my_slot->get_resource_directory();
            auto idx = this->field_C +
                res_dir.get_type_start_idxs(RESOURCE_KEY_TYPE_MORPH_FILE_STRUCT);

            auto *struct_loc = res_dir.get_resource_location(idx);
            assert(struct_loc != nullptr);

            auto *struct_mash = res_dir.get_resource(struct_loc, nullptr);
            assert(struct_mash != nullptr);

            nglMorphFile *morphFile = nullptr;
            auto alloced_mem = parse_generic_object_mash(morphFile,
                                                         struct_mash,
                                                         nullptr,
                                                         nullptr,
                                                         nullptr,
                                                         0,
                                                         0,
                                                         nullptr);
            assert(!alloced_mem && "This should NOT allocate anything!");

            morphFile->field_124 = tlres_loc->field_8;
            auto *v7 = tlres_loc->name.to_string();
            tlFixedString v20{v7};

            if (nglLoadMeshFileInternal(v20, bit_cast<nglMeshFile *>(morphFile), ".pcmorph")) {
                tlres_loc->field_8 = (char *) morphFile;

            } else {
                auto *v10 = tlres_loc->name.to_string();
                sp_log("Invalid morph file %s", v10);
                tlres_loc->field_8 = nullptr;
            }
        }

        ++this->field_C;

        return false;
    } else {
        return (bool) THISCALL(0x0056C080, this, behavior, tlres_loc);
    }
}

void morph_file_resource_handler_patch() {
    {
        FUNC_ADDRESS(address, &morph_file_resource_handler::_handle_resource);
        set_vfunc(0x00888A90, address);
    }
}

void morph_file_resource_handler_xbpack_patch()
{
#ifdef OPENUSM_XBPACK_V10
    set_vfunc(0x00888A90, &handle_xbox_morph_file);
#endif
}
