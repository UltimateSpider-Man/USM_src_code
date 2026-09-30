#include "lego_map.h"

#include "func_wrapper.h"
#include "trace.h"

#ifdef OPENUSM_XBPACK_V10
#include "log.h"
#include <cstring>
#endif

lego_map_root_node::lego_map_root_node() {}

void lego_map_root_node::un_mash(char *image, int *a3, region *reg)
{
    TRACE("lego_map_root_node::un_mash");

    THISCALL(0x0054E5A0, this, image, a3, reg);

}

#ifdef OPENUSM_XBPACK_V10
namespace
{
void __fastcall packed_vertical_bounds_v10(const uint8_t *lego, void *, float *lower, float *upper)
{
    float y;
    std::memcpy(&y, lego + 8, sizeof(y));
    const float half_height = static_cast<float>(lego[0x1F]) * 2.5f;
    const float upper_extension = static_cast<float>((lego[0x11] >> 3) & 1);
    *lower = y - half_height;
    *upper = (upper_extension + half_height) + y;
}
}

bool lego_map_xbpack_v10_patch()
{
    // Xbox 0x001210D0 selects the per-object radius from flags & 15.
    // PC added fade groups and interprets byte +0x11 as their index instead.
    // JM's packed scenery has meaningful flags there, but no fade-group
    // arrays: PC crashes at 0x0053A2D7 as TAM4 enters the subway phase.
    // Keep the serialized flags intact and select the non-group PC path.
    constexpr uintptr_t fade_site = 0x0053A2A5;
    constexpr uintptr_t bounds_site = 0x0053A30A;
    constexpr uint8_t pc_fade[] = {0x8A, 0x5D, 0x11};
    constexpr uint8_t v10_fade[] = {0x30, 0xDB, 0x90}; // xor bl,bl; nop
    constexpr uint8_t pc_bounds[] = {0xE8, 0x31, 0x5D, 0x02, 0x00};
    uint8_t v10_bounds[] = {0xE8, 0, 0, 0, 0};
    const auto displacement = static_cast<uint32_t>(
        reinterpret_cast<uintptr_t>(&packed_vertical_bounds_v10) - bounds_site - 5);
    std::memcpy(v10_bounds + 1, &displacement, sizeof(displacement));

    auto *fade = reinterpret_cast<void *>(fade_site);
    auto *bounds = reinterpret_cast<void *>(bounds_site);
    if ((std::memcmp(fade, pc_fade, sizeof(pc_fade)) != 0
         && std::memcmp(fade, v10_fade, sizeof(v10_fade)) != 0)
        || (std::memcmp(bounds, pc_bounds, sizeof(pc_bounds)) != 0
            && std::memcmp(bounds, v10_bounds, sizeof(v10_bounds)) != 0)) {
        sp_log("[xbpack] V10 scenery visibility instruction signature mismatch");
        return false;
    }

    // The packed upper-bound flag is Xbox bit 11; PC's bounds helper reads
    // bit 30. Adapt just this visitor's call, preserving other consumers.
    std::memcpy(fade, v10_fade, sizeof(v10_fade));
    std::memcpy(bounds, v10_bounds, sizeof(v10_bounds));
    return true;
}
#endif
