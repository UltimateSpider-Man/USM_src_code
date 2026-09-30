#include "xbpack.h"

#ifdef OPENUSM_XBPACK_V10

#include "log.h"

#include <cstdint>
#include <cstring>

bool xbpack_v10_s03_patch()
{
    // Only the default animation in carried-actor release (PC 0x00463E50).
    // PC loads Idle_No_Blend from 0x0096C458; V10 rescue NPCs lack that
    // state, so find_state returns null and force_update crashes at 0x498DE8.
    // Xbox release 0x00310C30 instead loads Idle from 0x0057569C at 0x310DB8
    // (registered by 0x003DFD20). Match that behavior without changing the
    // configured release-animation branch or other forced ALS requests.
    constexpr uintptr_t site = 0x00463FB7;
    constexpr uint8_t pc[] = {0x8B, 0x15, 0x58, 0xC4, 0x96, 0x00};
    constexpr uint8_t v10[] = {0xBA, 0x7E, 0x4B, 0x3B, 0x00, 0x90};
    auto *code = reinterpret_cast<void *>(site);
    if (std::memcmp(code, v10, sizeof(v10)) == 0) {
        return true;
    }
    if (std::memcmp(code, pc, sizeof(pc)) != 0) {
        sp_log("[xbpack] V10 carried-release instruction signature mismatch");
        return false;
    }
    // mov edx, to_hash("Idle"); nop -- same length as the original load.
    std::memcpy(code, v10, sizeof(v10));
    return true;
}

#endif
