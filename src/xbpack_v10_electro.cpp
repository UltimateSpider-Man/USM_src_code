#include "xbpack.h"

#if defined(OPENUSM_XBPACK_MODE) && defined(OPENUSM_XBPACK_V10) && !defined(TARGET_XBOX)

#include "log.h"
#include "param_block.h"
#include "utility.h"
#include "xbpack_v10_electro.h"

#include <cstdint>
#include <cstring>

namespace
{
static_assert(to_hash("spiderman_entity") == xbpack::v10_electro::spiderman_parameter);
static_assert(to_hash("SPIDERMAN") == 0x33FEBAA3u);

// Native get_pb_hash uses ECX for this, then stack arguments (result, name),
// returns result in EAX and pops eight bytes. Make the result pointer explicit
// so this bridge does not depend on a compiler's struct-return convention.
string_hash *__fastcall get_electro_spiderman_hash(const ai::param_block *block, void *,
                                                  string_hash *result, string_hash name)
{
    if (name.source_hash_code == xbpack::v10_electro::spiderman_parameter &&
        block->param_array != nullptr) {
        const auto *parameter = block->param_array->common_find_data(name);
        if (xbpack::v10_electro::try_read_fixed_string_target(
                parameter, result->source_hash_code,
                [](const char *text) { return to_hash(text); })) {
            sp_log("[xbpack] V10 Electro spiderman_entity: fixed string -> hash 0x%08X",
                   static_cast<unsigned>(result->source_hash_code));
            return result;
        }
    }

    using native_fn = string_hash *(__fastcall *)(const ai::param_block *, void *,
                                                  string_hash *, string_hash);
    return reinterpret_cast<native_fn>(xbpack::v10_electro::native_parameter_hash)(
        block, nullptr, result, name);
}
}

bool xbpack_v10_electro_patch()
{
    const auto address = xbpack::v10_electro::spiderman_hash_call;
    const auto *call = reinterpret_cast<const std::uint8_t *>(address);
    std::int32_t displacement = 0;
    std::memcpy(&displacement, call + 1, sizeof(displacement));
    const auto target = address + 5u + displacement;
    const auto replacement = reinterpret_cast<std::uintptr_t>(&get_electro_spiderman_hash);
    if (call[0] == 0xE8 && target == replacement) {
        return true;
    }
    if (call[0] != 0xE8 || target != xbpack::v10_electro::native_parameter_hash) {
        sp_log("[xbpack] V10 Electro patch rejected: spiderman_entity getter call changed");
        return false;
    }

    // A pointer copied as a hash leaves electro_inode +0x44 unresolved.
    // Native suited frame_advance then skips both movement and attacks.
    REDIRECT(address, get_electro_spiderman_hash);
    sp_log("[xbpack] V10 Electro fixed-string target lookup enabled");
    return true;
}

#endif
