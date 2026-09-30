#include "sound_bank_slot.h"

#include "common.h"
#include "func_wrapper.h"
#include "trace.h"
#include "utility.h"
#include "nfl_system.h"
#include "streams_music_bank.h"

#include <cstring>
#include <filesystem>
#include <string>

VALIDATE_SIZE(sound_bank_slot, 0x38);

Var<bool> s_running_from_resource_pack {0x0095C828};

namespace {
// STREAMS_MUSIC is the common bank in slot zero, shared by the entire game.
// Keep its loose-file ownership independent of s_running_from_resource_pack:
// a pack-backed game can still stream this one bank from an external file.
sound_bank_slot *external_music_slot = nullptr;
nflFileID external_music_file = NFL_FILE_ID_INVALID;

bool is_common_music_slot(const sound_bank_slot *slot, const char *bank)
{
    if (slot != &s_sound_bank_slots()[0] || bank == nullptr)
        return false;

    constexpr char expected[] = "STREAMS_MUSIC";
    for (size_t i = 0; i < sizeof(expected); ++i)
    {
        unsigned char c = static_cast<unsigned char>(bank[i]);
        if (c >= 'a' && c <= 'z') c -= 'a' - 'A';
        if (c != static_cast<unsigned char>(expected[i])) return false;
    }
    return true;
}

bool wait_for_external_music(sound_bank_slot *slot)
{
    // Match the native blocking-load order (0x54D4F2), using the native NSL
    // state accessor. Bound only this extra-file attempt so an I/O failure
    // can return to the game's normal bank loader.
    const DWORD started = GetTickCount();
    while (slot->m_state == SB_STATE_LOADING)
    {
        CDECL_CALL(0x0079A770);
        nflUpdate();
        const int state = nslGetBankState(slot->nsl_non_voice_bank_id);
        if (state < 0 || static_cast<DWORD>(GetTickCount() - started) >= 30000u)
            return false;
        slot->frame_advance(Float{0.0f});
        if (slot->m_state == SB_STATE_LOADING) Sleep(1);
    }
    return nslGetBankState(slot->nsl_non_voice_bank_id) == 0;
}

bool load_external_music(sound_bank_slot *slot, bool blocking)
{
    if (external_music_slot == slot && slot->m_state != SB_STATE_EMPTY)
    {
        if (nslGetBankState(slot->nsl_non_voice_bank_id) >= 0 &&
            (!blocking || wait_for_external_music(slot))) return true;
        sp_log("STREAMS_MUSIC: external bank failed; restoring native bank loading.");
        slot->unload();
        return false;
    }

    std::filesystem::path path;
    std::filesystem::path base;
    char executable[260]{};
    const DWORD executable_length = GetModuleFileNameA(nullptr, executable, sizeof(executable));
    if (executable_length > 0 && executable_length < sizeof(executable))
        base = std::filesystem::path(executable).parent_path();
    uint32_t wave_count = 0;
    std::string diagnostic;
    if (!openusm::streams_music::resolve_external_bank(path, wave_count, diagnostic, base))
    {
        if (!diagnostic.empty()) sp_log("STREAMS_MUSIC: %s", diagnostic.c_str());
        return false;
    }

    const std::string filename = path.string();
    const nflFileID file = nflOpenFile(nflMediaID{1}, filename.c_str());
    if (file == NFL_FILE_ID_INVALID)
    {
        sp_log("STREAMS_MUSIC: cannot open %s; using native bank.", filename.c_str());
        return false;
    }

    // Verified against native sound_bank_slot::load at 0x54D416:
    // nslLoadBank(memory_type, file_id, bank_offset). field_24 is assigned
    // by sound_manager initialization, and is 1 for common streamed banks.
    const auto bank = static_cast<nslBankID>(CDECL_CALL(
        0x00798450, slot->field_24 != 1 ? 1 : 0, file.field_0, 0u));
    if (bank == NSL_BANK_ID_INVALID)
    {
        nflCloseFile(file);
        sp_log("STREAMS_MUSIC: NSL bank registration failed; using native bank.");
        return false;
    }

    // Register successfully before releasing the old bank. The original
    // slot loader can otherwise prefer an embedded WBK and never open this
    // validated loose bank at all.
    slot->unload();
    slot->field_0 = fixedstring<8>{"STREAMS_MUSIC"};
    slot->nsl_voice_bank_id = NSL_BANK_ID_INVALID;
    slot->nsl_non_voice_bank_id = bank;
    slot->field_30 = NFL_FILE_ID_INVALID.field_0;
    slot->field_34 = NFL_FILE_ID_INVALID.field_0;
    slot->m_state = SB_STATE_LOADING;
    external_music_slot = slot;
    external_music_file = file;
    sp_log("STREAMS_MUSIC: registered all %u entries from %s.",
           static_cast<unsigned>(wave_count), filename.c_str());

    if (blocking && !wait_for_external_music(slot))
    {
        sp_log("STREAMS_MUSIC: external bank failed or timed out; using native bank.");
        slot->unload();
        return false;
    }
    return true;
}
}

int sound_bank_slot::get_state() {
    return this->m_state;
}

void sound_bank_slot::unload() {
    if constexpr (1)
    {
        if ( this->nsl_voice_bank_id != NSL_BANK_ID_INVALID )
        {
            nslFreeBank(this->nsl_voice_bank_id);
        }

        auto v2 = this->nsl_non_voice_bank_id;
        this->nsl_voice_bank_id = NSL_BANK_ID_INVALID;
        if ( v2 != NSL_BANK_ID_INVALID )
        {
            nslFreeBank(v2);
        }

        this->nsl_non_voice_bank_id = NSL_BANK_ID_INVALID;
        this->field_0 = {};
        this->m_state = 0;

        if (external_music_slot == this)
        {
            // nslFreeBank above cancels pending reads before the owned file
            // is closed, just as the engine does for ordinary loose banks.
            const nflFileID owned_file = external_music_file;
            external_music_slot = nullptr;
            external_music_file = NFL_FILE_ID_INVALID;
            if (owned_file != NFL_FILE_ID_INVALID) nflCloseFile(owned_file);
        }

        auto v3 = !s_running_from_resource_pack();
        if ( v3 )
        {
            if ( this->field_30 != NSL_BANK_ID_INVALID )
            {
                nflCloseFile(this->field_30);
            }

            auto v4 = this->field_34;
            this->field_30 = NSL_BANK_ID_INVALID;
            if ( v4 != NSL_BANK_ID_INVALID )
            {
                nflCloseFile(v4);
            }

            this->field_34 = NSL_BANK_ID_INVALID;
        }
    }
    else
    {
        THISCALL(0x00520160, this);
    }
}

void sound_bank_slot::load(const char *a1, const char *a2, bool a4, int a5)
{
    TRACE("sound_bank_slot::load");
    //TRACE(("sound_bank_slot " + std::string {a1} + " " + std::string {a2}).c_str());
    if (!is_common_music_slot(this, a2) || !load_external_music(this, a4))
        THISCALL(0x0054CC30, this, a1, a2, a4, a5);

    // WAV mod loader (mod.h): the engine has just kicked off the WBK
    // streaming for (a1 = scene, a2 = bank), which means audio init is long
    // done - upload every *.wav registered by enumerate_mods() into a
    // DirectSound secondary buffer on the game's own device. A wav named
    // exactly like the bank (mods/<a2>.wav) also plays here once, as an
    // audible confirmation that the override pipeline is live.
    modWav_onSoundBankLoad(a1, a2);
}

void sound_bank_slot::frame_advance(Float a2)
{
    TRACE("sound_bank_slot::frame_advance");

    if (external_music_slot == this &&
        nslGetBankState(this->nsl_non_voice_bank_id) < 0)
    {
        // A nonblocking NFL read can fail after registration. Restore the
        // canonical common-bank request rather than labelling the failed
        // external bank ready. Slot zero uses the global pack directory
        // (a5 == 0); no transient mission-directory pointer is retained.
        sp_log("STREAMS_MUSIC: asynchronous load failed; restoring native bank.");
        this->unload();
        THISCALL(0x0054CC30, this, "STREAMS", "STREAMS_MUSIC", false, 0);
        return;
    }

    if constexpr (1)
    {
        if ( this->m_state == 1 )
        {
            auto v3 = this->nsl_voice_bank_id;
            bool v6 = false;
            if ( v3 == NSL_BANK_ID_INVALID)
            {
                v6 = true;
            }
            else if (nslGetBankState(v3) != 0)
            {
                if (nslGetBankState(v3) < 0)
                {
                    sp_log("Could not load voice bank for mission %s.", this->field_0.to_string());
                    v6 = true;
                }
            }
            else
            {
                v6 = true;
            }

            auto v4 = this->nsl_non_voice_bank_id;
            bool v5 = false;
            if (v4 == NSL_BANK_ID_INVALID)
            {
                v5 = true;
            }
            else if (nslGetBankState(v4) != 0)
            {
                if (nslGetBankState(v4) < 0)
                {
                    sp_log("Could not load non-voice bank for mission %s.", this->field_0.to_string());
                    v5 = true;
                }
            }
            else
            {
                v5 = true;
            }

            if ( v6 && v5 )
            {
                this->m_state = 2;
            }
        }
    }
    else
    {
        THISCALL(0x00520200, this, a2);
    }
}

Var<sound_bank_slot[12]> s_sound_bank_slots{0x009601A8};

void sound_bank_slot_patch()
{
    {
        FUNC_ADDRESS(address, &sound_bank_slot::unload);
        SET_JUMP(0x00520160, address);
    }

    {
        FUNC_ADDRESS(address, &sound_bank_slot::load);
        REDIRECT(0x0055A4FA, address);
        REDIRECT(0x0054DB27, address);
    }

    {
        FUNC_ADDRESS(address, &sound_bank_slot::frame_advance);
        REDIRECT(0x0054D4FF, address);
        REDIRECT(0x00551C53, address);
    }
}
