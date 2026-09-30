#include "xbpack.h"

#if defined(OPENUSM_XBPACK_MODE) && defined(OPENUSM_XBPACK_V10) && !defined(TARGET_XBOX)

#include "actor.h"
#include "base_ai_core.h"
#include "damage_interface.h"
#include "info_node.h"
#include "log.h"
#include "utility.h"
#include "xbpack_v10_shocker.h"

#include <cstdint>

namespace
{
static_assert(to_hash("die_from_webtie") == xbpack::v10_shocker::die_from_webtie_hash);

// V10 S02_SHOCKER_PACK enables die_from_webtie in Shocker's core params.
// Mapping Xbox shocker_inode (0xE7) to PC (0xF7) retains the combat timers,
// but PC frame_advance has no equivalent of the beta's web-tie defeat.
// Restore that rule after the existing native update, using PC interfaces
// and the live core parameter, as the Xbox function does each frame.
void __fastcall shocker_frame_advance(ai::info_node *node, void *, Float dt)
{
    using native_frame_fn = void (__fastcall *)(ai::info_node *, void *, Float);
    reinterpret_cast<native_frame_fn>(xbpack::v10_shocker::native_frame)(node, nullptr, dt);

    auto *core = node->get_core();
    auto *owner = node->get_actor();
    if (core == nullptr || owner == nullptr) {
        return;
    }

    const string_hash flag {static_cast<int>(xbpack::v10_shocker::die_from_webtie_hash)};
    // Loose/custom Shocker cores that omit the beta property keep their
    // existing behavior. Never apply this rule to every subdued character.
    const int enabled = core->get_param_block()->get_optional_pb_int(flag, 0, nullptr);
    if (enabled == 0 || !owner->has_damage_ifc()) {
        return;
    }

    auto *damage = owner->damage_ifc();
    if (damage == nullptr) {
        return;
    }

    const float before = damage->field_1FC.field_0[0];
    if (xbpack::v10_shocker::apply_webtie_defeat(
            true, damage->field_21C.field_0[0], damage->field_1FC.field_0)) {
        sp_log("[xbpack] V10 Shocker web-tie defeat: actor=%p subdual=%g health=%g -> %g",
               static_cast<void *>(owner), damage->field_21C.field_0[0],
               before, damage->field_1FC.field_0[0]);
    }
}
}

bool xbpack_v10_shocker_patch()
{
    auto *slot = reinterpret_cast<std::uintptr_t *>(xbpack::v10_shocker::frame_slot);
    const auto replacement = reinterpret_cast<std::uintptr_t>(&shocker_frame_advance);
    if (*slot == replacement) {
        return true;
    }
    if (*slot != xbpack::v10_shocker::native_frame) {
        sp_log("[xbpack] V10 Shocker web-tie patch rejected: frame callback=0x%08X",
               static_cast<unsigned>(*slot));
        return false;
    }

    *slot = replacement;
    sp_log("[xbpack] V10 Shocker die_from_webtie behavior installed");
    return true;
}

#endif
