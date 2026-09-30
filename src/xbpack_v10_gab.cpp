#include "xbpack.h"

#ifdef OPENUSM_XBPACK_V10

#include "func_wrapper.h"
#include "log.h"
#include "string_hash.h"
#include "utility.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>

// Pedestrian chatter and screams ("city", "Fear", "Terror", ...) are gab
// expressions looked up by the speaker's archetype (gab lookup 0x005D9800,
// called only from voice_box_inode::say at 0x006D140A).  The PC prerelease
// keeps speaker_id as a fixed-string param ("MW1", ...) and compares
// archetypes with _strnicmp(code, archetype, 3) (0x005BA7E0).  V10 data keys
// them by hash instead: VOICE_BOX nodes carry speaker_id as a string_hash
// param (city peds use 0x0001C5A9 and 0x0001C5A2), gab_database archetypes
// start with that hash, and the Xbox lookup compares both unsigned
// (0x0015B600).  PC read the hash as a char*, so every ped gab lookup failed
// and peds never spoke or screamed.
namespace
{
constexpr uintptr_t FIND_PARAM = 0x006CD450;          // param_data_array::common_find_data
constexpr uintptr_t SAY_FIND_SPEAKER = 0x006D13E6;    // voice_box_inode::say -> FIND_PARAM(speaker_id)
constexpr uintptr_t ARCHETYPE_COMPARE = 0x005BA7E0;   // bsearch comparator of the archetype search
constexpr uintptr_t PICK_RANDOM_SPEAKER = 0x006D7EA0; // PC-only FW1..MW2 speaker assignment
constexpr uint32_t PT_STRING_HASH = 2;

struct param_data
{
    uint32_t value;
    uint32_t type;
    uint32_t name;
};

// voice_box_inode::say only reads the first field of the returned param, as
// the archetype key string.
char speaker_key[12];
param_data speaker_param;

param_data *__fastcall find_speaker(void *params, void *, uint32_t name)
{
    auto *param = reinterpret_cast<param_data *>(THISCALL(FIND_PARAM, params, name));
    if (param == nullptr || param->type != PT_STRING_HASH) {
        return param;
    }

    std::snprintf(speaker_key, sizeof(speaker_key), "#%08X", param->value);
    speaker_param = {static_cast<uint32_t>(reinterpret_cast<uintptr_t>(speaker_key)),
                     param->type,
                     param->name};
    return &speaker_param;
}

uint32_t archetype_hash(const char *key)
{
    if (key == nullptr) {
        return 0;
    }

    if (key[0] == '#') {
        return static_cast<uint32_t>(std::strtoul(key + 1, nullptr, 16));
    }

    return to_hash(key);
}

// Archetypes are sorted by their leading string_hash, compared unsigned.
int __cdecl compare_archetype(const char *key, const uint32_t *const *archetype)
{
    const auto lhs = archetype_hash(key);
    const auto rhs = **archetype;
    return lhs > rhs ? 1 : (lhs < rhs ? -1 : 0);
}
}

void xbpack_v10_gab_patch()
{
    REDIRECT(SAY_FIND_SPEAKER, find_speaker);
    SET_JUMP(ARCHETYPE_COMPARE, compare_archetype);

    // V10 speakers come from AI data and variant speaker sets.  The PC random
    // human speaker would overwrite them with codes the V10 gab_database lacks.
    *reinterpret_cast<uint8_t *>(PICK_RANDOM_SPEAKER) = 0xC3;
}

#endif
