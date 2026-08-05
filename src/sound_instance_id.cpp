#include "sound_instance_id.h"
#include "sound_manager.h"


#include "common.h"

#include "func_wrapper.h"
#include "utility.h"

#include <cstring>

VALIDATE_SIZE(sound_instance_slot, 0x54);

Var<sound_instance_slot *> s_sound_instance_slots{0x0095C830};

void sound_instance::stop() {
    THISCALL(0x0053E6F0, this);
}

namespace {
// Engine ABI of 0x0060B960: sound_instance_id comes back through a hidden
// pointer arg (MSVC class return), exactly how CDECL_CALL passes it below.
using sound_play_2d_original_fn = void (__cdecl *)(sound_instance_id *, string_hash, Float, Float);

sound_play_2d_original_fn g_original_sub_60B960 = nullptr;

// SET_JUMP clobbers 6 bytes at the target (E9 rel32 + C3), so the
// trampoline has to carry the whole first instruction pair:
// push ecx; mov eax, [0x009682E0].
sound_play_2d_original_fn make_sub_60B960_trampoline()
{
    constexpr uintptr_t target = 0x0060B960;
    constexpr size_t stolenBytes = 6;

    const auto *prologue = reinterpret_cast<const uint8_t *>(target);
    if (prologue[0] != 0x51 || prologue[1] != 0xA1)   // push ecx; mov eax, moffs32
        return nullptr;

    auto *mem = static_cast<uint8_t *>(VirtualAlloc(nullptr,
                                                    stolenBytes + 5,
                                                    MEM_COMMIT | MEM_RESERVE,
                                                    PAGE_EXECUTE_READWRITE));
    if (!mem)
        return nullptr;

    std::memcpy(mem, prologue, stolenBytes);
    mem[stolenBytes] = 0xE9; // jump back to original+stolenBytes
    *reinterpret_cast<uint32_t *>(mem + stolenBytes + 1) =
        (uint32_t)((target + stolenBytes) - (reinterpret_cast<uintptr_t>(mem) + stolenBytes) - 5);

    FlushInstructionCache(GetCurrentProcess(), mem, stolenBytes + 5);
    return reinterpret_cast<sound_play_2d_original_fn>(mem);
}

// Once sound_instance_id_patch() lands, every play-2D request in the game
// funnels through here — voice-box lines included, not just the UI code
// this DLL reimplements.
void __cdecl sub_60B960_hook(sound_instance_id *result, string_hash a2, Float a3, Float a4)
{
    // WAV mod override: a wav dropped in extra/ keyed to this sound or
    // alias hash — by stem, literal-hash stem, or *hashes*.txt sidecar
    // (mod.h) — plays through DirectSound instead of the engine's bank
    // sample. Returns the invalid instance id (slot lookups yield nullptr,
    // stop() is never reached), so callers stay well-behaved.
    if (auto *snd = getWavMod(a2.source_hash_code)) {
        if (const char *name = modWavHashName(a2.source_hash_code))
            printf("wav mod: override 0x%08X (%s)\n", a2.source_hash_code, name);
        modWavPlay(*snd, a3);

        result->field_0 = 0;
        return;
    }

    if (g_original_sub_60B960)
        g_original_sub_60B960(result, a2, a3, a4);   // 0x0060B960 is patched
    else
        CDECL_CALL(0x0060B960, result, a2, a3, a4);  // patch not installed
}
}

sound_instance_id sub_60B960(string_hash a2, Float a3, Float a4) {
    sound_instance_id result;
    sub_60B960_hook(&result, a2, a3, a4);
    return result;
}

sound_instance *sound_instance_id::get_sound_instance_ptr() {
    return (sound_instance *) THISCALL(0x0050EF00, this);
}

void sound_instance_id_patch() {
    // Trampoline FIRST: SET_JUMP overwrites the very bytes it copies.
    if (!g_original_sub_60B960)
        g_original_sub_60B960 = make_sub_60B960_trampoline();

    if (g_original_sub_60B960)
        SET_JUMP(0x0060B960, sub_60B960_hook);
    else
        sp_log("sound_instance_id_patch: unexpected prologue at 0x0060B960, hook skipped");
}
