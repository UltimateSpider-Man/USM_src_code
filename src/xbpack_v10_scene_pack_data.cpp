#include "xbpack_v10_scene_pack.h"

#if defined(OPENUSM_XBPACK_MODE) && defined(OPENUSM_XBPACK_V10) && !defined(TARGET_XBOX)
namespace xbpack::v10_scene_pack
{
namespace
{
// Byte-exact SCNANIMS_XB.PAK supplied with this repair request.
// SHA-256: ad2ade5b903ca45273cb012c5663c42aee0ff3795a9c0a071dab2bb7691938e9
const std::uint8_t original_v10_pack[] {
#include "xbpack_v10_scene_pack_data.inc"
};
static_assert(sizeof(original_v10_pack) == 2271232u);
}

bytes embedded_pack()
{
    return {original_v10_pack, sizeof(original_v10_pack)};
}
}
#endif
