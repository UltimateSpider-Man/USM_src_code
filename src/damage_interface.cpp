#include "damage_interface.h"

#include "actor.h"
#include "common.h"
#include "func_wrapper.h"
#include "parse_generic_mash.h"
#include "resource_key.h"
#include "trace.h"
#include "utility.h"
#include "variables.h"
#include "vtbl.h"
#include "xbpack.h"

#include <cstdint>
#include <cstring>

VALIDATE_SIZE(damage_interface, 0x23Cu);
VALIDATE_OFFSET(damage_interface, field_1FC, 0x1FCu);

template<>
void bounded_variable<float>::sub_48BFB0(const float &a2)
{
    this->field_0[0] = a2;
    if (a2 > this->field_0[2]) {
        this->field_0[0] = this->field_0[2];
    }

    if (this->field_0[0] < this->field_0[1]) {
        this->field_0[0] = this->field_0[1];
    }
}

damage_interface::damage_interface(actor *a2)
{
    THISCALL(0x004DE8A0, this, a2);
}

damage_interface::~damage_interface()
{
    THISCALL(0x004D9BF0, this);
}

bool damage_interface::get_ifc_num(const resource_key &att, float *a3, bool is_log) {
    assert(att.get_type() == RESOURCE_KEY_TYPE_IFC_ATTRIBUTE);

    return (bool) THISCALL(0x004C8C60, this, &att, a3, is_log);
}

bool damage_interface::set_ifc_num(const resource_key &att, Float a3, bool is_log) {
    assert(att.get_type() == RESOURCE_KEY_TYPE_IFC_ATTRIBUTE);

    return (bool) THISCALL(0x004CE940, this, &att, a3, is_log);
}

void damage_interface::frame_advance_all_damage_ifc(Float a1)
{
    TRACE("damage_interface::frame_advance_all_damage_ifc");

    if constexpr (1)
    {
        if ( all_damage_interfaces() != nullptr && !all_damage_interfaces()->empty() )
        {
            for ( auto &dam : (*all_damage_interfaces()) )
            {
                if ( dam != nullptr )
                {
                    // Offset 0x28 is damage_interface::frame_advance. Dispatch
                    // through the vtable exactly once so derived interfaces are
                    // respected and hit/death processing is not duplicated.
                    void (__fastcall *func)(void *, void *, Float) = CAST(func, get_vfunc(dam->m_vtbl, 0x28));
                    func(dam, nullptr, a1);
                }
            }
        }
    }
    else
    {
        CDECL_CALL(0x004D1990, a1);
    }
}

void damage_interface::_un_mash(
        generic_mash_header *header,
        void *a3,
        void *a4,
        generic_mash_data_ptrs *a5)
{
    TRACE("damage_interface::un_mash");

#ifdef OPENUSM_XBPACK_V10
    if (g_platform == NL_PLATFORM_XBOX) {
        auto *bytes = reinterpret_cast<std::uint8_t *>(this);
        std::uint32_t insertion_marker = 0u;
        std::uint32_t tail_padding = 0u;
        std::memcpy(&insertion_marker, bytes + 0xACu, sizeof(insertion_marker));
        std::memcpy(&tail_padding, bytes + 0x1F8u, sizeof(tail_padding));

        // Prototype v10 damage_interface is 0x240 bytes: it inserts the
        // A1A1A1A1 dword at +0xAC, shifting every later field four bytes.
        // Collapse the complete suffix into the retail PC 0x23C layout before
        // stock unmarshaling.  The second sentinel makes this narrow enough to
        // leave already-PC-shaped loose overrides untouched.
        if (insertion_marker == 0xA1A1A1A1u && tail_padding == 0xCDCD0000u) {
            std::memmove(bytes + 0xACu, bytes + 0xB0u, 0x190u);

            // cached_special_effect begins at PC +0x184.  Its first embedded
            // resource key still uses the v10 Xbox type numbering after the
            // structural move, so translate that type before stock fills its
            // destruction/effect cache.
            std::uint32_t effect_type = 0u;
            std::memcpy(&effect_type, bytes + 0x188u, sizeof(effect_type));
            effect_type = static_cast<std::uint32_t>(
                xbpack::pc_type(static_cast<int>(effect_type)));
            std::memcpy(bytes + 0x188u, &effect_type, sizeof(effect_type));

            // actor::_un_mash pre-advances the normal cursor by the PC size.
            // Consume the removed v10 dword so the following normal-buffer
            // object starts at its real serialized address.
            a5->field_0 += sizeof(std::uint32_t);
        }
    }
#endif

    if constexpr (0)
    {}
    else
    {
        THISCALL(0x004D9E20, this, header, a3, a4, a5);
    }
}

void damage_interface::frame_advance(Float a3)
{
    TRACE("damage_interface::frame_advance");

    THISCALL(0x004EC4A0, this, a3);
}

void damage_interface_patch()
{
    REDIRECT(0x00558500, damage_interface::frame_advance_all_damage_ifc);

    {
        FUNC_ADDRESS(address, &damage_interface::_un_mash);
        set_vfunc(0x0088398C, address);
    }

    {
        FUNC_ADDRESS(address, &damage_interface::frame_advance);
        set_vfunc(0x00883998, address);
    }
}

void damage_interface_xbpack_patch()
{
    FUNC_ADDRESS(address, &damage_interface::_un_mash);
    set_vfunc(0x0088398C, address);
}
