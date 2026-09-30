#include "cut_scene_segment.h"

#include "common.h"
#include "func_wrapper.h"
#include "xbpack.h"

VALIDATE_SIZE(cut_scene_segment, 0xB0u);

cut_scene_segment::cut_scene_segment(from_mash_in_place_constructor *a2) {
    THISCALL(0x007425B0, this, a2);
}

#if defined(OPENUSM_XBPACK_MODE) && defined(OPENUSM_XBPACK_V10) && !defined(TARGET_XBOX)

#include "log.h"
#include "resource_key.h"
#include "utility.h"
#include "xbpack_v10_cut_scene.h"

#include <cstdint>
#include <cstring>

VALIDATE_OFFSET(cut_scene_segment, field_20, 0x20);
static_assert(RESOURCE_KEY_TYPE_SCENE_ANIM == xbpack::v10_cut_scene::pc_scene_animation_type);

namespace
{
void __fastcall unmash_v10_segment(cut_scene_segment *segment, void *,
                                   mash_info_struct *info, void *owner)
{
    using native_fn = void (__fastcall *)(cut_scene_segment *, void *, mash_info_struct *, void *);
    reinterpret_cast<native_fn>(xbpack::v10_cut_scene::native_segment_unmash)(
        segment, nullptr, info, owner);

    // V08 IGC2 has two streamed segments, v08_elfn_igc2_01 and _02.
    // Leaving their serialized type at Xbox 0x19 searches the PC PACK
    // slice instead of SCENE_ANIM (0x1A). The native stream caller ignores
    // a failed lookup and would consume an empty resource_location.
    for (auto *key : segment->field_20) {
        if (key == nullptr) {
            continue;
        }
        const auto old_type = static_cast<std::uint32_t>(key->m_type);
        const auto new_type = xbpack::v10_cut_scene::normalize_scene_animation_type(old_type);
        if (new_type != old_type) {
            key->m_type = static_cast<resource_key_type>(new_type);
            sp_log("[xbpack] V10 cutscene animation 0x%08X: resource type 0x19 -> 0x1A",
                   static_cast<unsigned>(key->m_hash.source_hash_code));
        }
    }
}
}

bool cut_scene_segment_xbpack_v10_patch()
{
    const auto address = xbpack::v10_cut_scene::segment_unmash_call;
    const auto *call = reinterpret_cast<const std::uint8_t *>(address);
    std::int32_t displacement = 0;
    std::memcpy(&displacement, call + 1, sizeof(displacement));
    const auto target = address + 5u + displacement;
    const auto replacement = reinterpret_cast<std::uintptr_t>(&unmash_v10_segment);
    if (call[0] == 0xE8 && target == replacement) {
        return true;
    }
    if (call[0] != 0xE8 || target != xbpack::v10_cut_scene::native_segment_unmash) {
        sp_log("[xbpack] V10 cutscene key patch rejected: segment unmash call changed");
        return false;
    }

    REDIRECT(address, unmash_v10_segment);
    sp_log("[xbpack] V10 cutscene scene-animation keys enabled");
    return true;
}

#endif
