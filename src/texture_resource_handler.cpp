#include "texture_resource_handler.h"

#include "common.h"
#include "func_wrapper.h"
#include "log.h"
#include "osassert.h"
#include "resource_directory.h"
#include "utility.h"
#include "trace.h"
#include "variables.h"
#include "worldly_pack_slot.h"

#include <cassert>

VALIDATE_SIZE(texture_resource_handler, 0x18);

texture_resource_handler::texture_resource_handler(worldly_pack_slot *a2)
{
    this->m_vtbl = 0x00888A28;
    this->my_slot = a2;
    this->field_10 = TLRESOURCE_TYPE_TEXTURE;
}

bool texture_resource_handler::_handle(worldly_resource_handler::eBehavior a2, limited_timer *a3)
{
    TRACE("texture_resource_handler::handle");

    return base_tl_resource_handler::_handle(a2, a3);
}

void texture_resource_handler::handle_resource_internal(tlresource_location *loc,
                                                        nglTextureFileFormat a3)
{
    TRACE("texture_resource_handler::handle_resource_internal", loc->name.to_string());

    if (a3 == static_cast<nglTextureFileFormat>(1))
    {
        assert(loc != nullptr);

        // Keep texture construction byte-for-byte native.  The resource
        // backlink pass below makes the retail representation explicit:
        // DDSMP PaletteFrames is one contiguous nglTexture array; ordinary
        // IFL Frames remains a pointer table at the same ABI offset.
        // Use the retail helpers here as well: 0x005374B0 resolves the pack
        // hash through the native dictionary, and 0x004018D0 zeroes all 0x20
        // bytes before copying/lowercasing the bounded name.
        const auto *texture_name_text = reinterpret_cast<const char *>(
            THISCALL(0x005374B0, &loc->name));
        tlFixedString texture_name {};
        THISCALL(0x004018D0, &texture_name, texture_name_text);
        auto *Tex = reinterpret_cast<nglTexture *>(
            CDECL_CALL(0x0077AB30,
                       &texture_name,
                       a3,
                       loc->field_8,
                       loc->get_size()));

        if (Tex == nullptr) {
            Tex = nglDefaultTex();
        }

        loc->field_8 = bit_cast<char *>(Tex);
        if (Tex == nglDefaultTex()) {
            sp_log("ERROR: multipalette texture not found: %s", loc->name.to_string());
            return;
        }

        auto &dir = this->my_slot->get_resource_directory();
        for (uint32_t i = 0; i < Tex->m_num_palettes; ++i)
        {
            auto *frame = &Tex->PaletteFrames[i];
            tlresource_location *frame_loc = nullptr;
            const bool found = dir.find_tlresource(frame->field_60.m_hash,
                                                   TLRESOURCE_TYPE_TEXTURE,
                                                   nullptr,
                                                   &frame_loc);

            if (!found || frame_loc == nullptr) {
                sp_log("ERROR: multipalette sub-texture not found: %s",
                       frame->field_60.to_string());
                continue;
            }

            // Publish the actual inline frame object.  Interpreting a DDSMP
            // owner through the IFL Frames view reads the frame's m_format
            // (0x11) as though it were a pointer.
            frame_loc->field_8 = bit_cast<char *>(frame);
        }

        return;
    }

    THISCALL(0x0056BBC0, this, loc, a3);
}

void texture_resource_handler::_pre_handle_resources(worldly_resource_handler::eBehavior a2)
{
    TRACE("texture_resource_handler::pre_handle_resources");

    if constexpr (1)
    {
        if (a2 == LOAD)
        {
            auto &res_dir = this->my_slot->get_resource_directory();

            const auto size = res_dir.get_tlresource_count(TLRESOURCE_TYPE_TEXTURE);
            //sp_log("pre_handle_resources: %u", size);
            for (int i = 0; i < size; ++i)
            {
                auto *loc = res_dir.get_tlresource_location(i, TLRESOURCE_TYPE_TEXTURE);
                assert(loc != nullptr
                       //&& loc->get_size() >= 4
                );

                auto func = [](tlresource_location *loc, int a2) -> void {
                    loc->m_type = a2 + (loc->m_type & 0xFFFFFF00);
                };

                char *v4 = CAST(v4, loc->field_8);
                if (v4[0] == 'D' && v4[1] == 'D' && v4[2] == 'S' && v4[3] == 'M') {
                    func(loc, 14);
                } else if (v4[0] != 'D' || v4[1] != 'D' || v4[2] != 'S') {
                    if (v4[0] == 'D' && v4[1] == 'S' && v4[2] == 'M') {
                        func(loc, 15);
                    } else {
                        func(loc, 13);
                    }
                }

                //sp_log("%c %c %c", v4[0], v4[1], v4[2]);
            }

            this->field_14 = 0;
        }
    } else {
        THISCALL(0x00562FB0, this, a2);
    }
}

bool texture_resource_handler::_handle_resource(worldly_resource_handler::eBehavior behavior,
                                               tlresource_location *tlres_loc)
{
    TRACE("texture_resource_handler::handle_resource");

    if constexpr (1)
    {
        if (behavior == UNLOAD)
        {
            if (tlres_loc->get_type() != 15)
            {
                auto *tex = bit_cast<nglTexture *>(tlres_loc->field_8);
                if (tex != nullptr)
                {
                    if ((tex->field_34 & 2) == 0)
                    {
                        if (!nglCanReleaseTexture(tex)) {
                            return true;
                        }

                        nglDestroyTexture(tex);
                    }
                }
            }

            ++this->field_C;

        }
        else
        {
            auto v4 = this->field_14;
            if (v4 || tlres_loc->get_type() != 14)
            {
                if (v4 == 1 && tlres_loc->get_type() == TLRESOURCE_TYPE_TEXTURE) {
                    this->handle_resource_internal(tlres_loc, static_cast<nglTextureFileFormat>(0));
                } else if (v4 == 2 && tlres_loc->get_type() == 13) {
                    this->handle_resource_internal(tlres_loc, static_cast<nglTextureFileFormat>(3));
                }
            }
            else
            {
                this->handle_resource_internal(tlres_loc, static_cast<nglTextureFileFormat>(1));
            }

            ++this->field_C;
            auto *dir = &this->my_slot->get_resource_directory();
            if (this->field_C >= dir->get_tlresource_count(static_cast<tlresource_type>(1)))
            {
                if (auto v7 = this->field_14; v7 < 2) {
                    this->field_C = 0;
                    ++this->field_14;
                }
            }
        }
    }
    else
    {
        THISCALL(0x0056BB00, this, behavior, tlres_loc);
    }

    return false;
}

void texture_resource_handler_patch() {

    FUNC_ADDRESS(address, &texture_resource_handler::pre_handle_resources);
    //set_vfunc(0x00888A30, address);

    {
        FUNC_ADDRESS(address, &texture_resource_handler::_handle);
        set_vfunc(0x00888A2C, address);
    }

    {
        FUNC_ADDRESS(address, &texture_resource_handler::_handle_resource);
        set_vfunc(0x00888A34, address);
    }
}

void texture_resource_handler_xbpack_patch()
{
    // Patch the sole native caller rather than replacing 0x0056BBC0.  This
    // keeps ordinary textures on the stock implementation. PC arena packs
    // can also omit DDSMP sub-texture entries: the native backlink pass
    // writes through null at 0x0056BCD8 when that lookup fails.
    FUNC_ADDRESS(address, &texture_resource_handler::handle_resource_internal);
    REDIRECT(0x0056BB3E, address);
}
