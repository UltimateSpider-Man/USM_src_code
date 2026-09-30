#include "xbpack_v10_frontend_throb.h"

#if defined(OPENUSM_XBPACK_MODE) && defined(OPENUSM_XBPACK_V10) && !defined(TARGET_XBOX)

#include "common.h"
#include "frontend_throb_bank.h"
#include "frontend_throb_lifecycle.h"
#include "func_wrapper.h"
#include "log.h"
#include "nfl_system.h"
#include "nslbank.h"
#include "sound_instance_id.h"
#include "variable.h"

namespace xbpack::v10_frontend_throb {
namespace {
namespace throb = openusm::frontend_throb;
static_assert(sizeof(void *) == 4, "FE_MM_THROB uses the PC x86 sound ABI");

bool sound_initialized()
{
    return var<bool>(0x0095C829);
}

int find_source()
{
    // Native NSL lookup searches the loaded wave banks before the game's
    // alias database. Verified in the supplied executable at 0x005204AE.
    return CDECL_CALL(0x00798190, throb::hash);
}

struct native_audio {
    sound_instance_id voice{0};
    nslBankID bank = NSL_BANK_ID_INVALID;
    nflFileID file = NFL_FILE_ID_INVALID;
    bool tried_extra = false;
    bool logged_play = false;
    std::uint32_t load_started = 0;

    void begin(std::uint32_t)
    {
        tried_extra = false;
        logged_play = false;
    }

    void close_bank()
    {
        // Order matches sound_bank_slot::unload: cancel/free NSL before
        // closing its owned NFL file. Never free a CITY_ARENA-owned bank.
        if (bank != NSL_BANK_ID_INVALID) {
            if (sound_initialized())
                nslFreeBank(bank);
            bank = NSL_BANK_ID_INVALID;
        }
        if (file != NFL_FILE_ID_INVALID) {
            nflCloseFile(file);
            file = NFL_FILE_ID_INVALID;
        }
    }

    void update(std::uint32_t now)
    {
        if (!sound_initialized())
            return;
        if (bank != NSL_BANK_ID_INVALID) {
            const int state = nslGetBankState(bank);
            if (state < 0 || (state > 0 && std::uint32_t(now - load_started) >= 30000u)) {
                sp_log("[xbpack] V10 FE_MM_THROB: extra bank failed/timed out; menu remains usable");
                stop();
                close_bank();
            }
            return;
        }
        // Prefer the already-loaded CITY_ARENA source. The extra bank is a
        // fallback for installations where the menu precedes that bank or
        // the Xbox pack does not expose this PC wave.
        if (find_source() != -1 || tried_extra)
            return;
        tried_extra = true;

        char executable[260]{};
        const DWORD count = GetModuleFileNameA(nullptr, executable, sizeof(executable));
        if (count == 0 || count >= sizeof(executable)) {
            sp_log("[xbpack] V10 FE_MM_THROB: cannot resolve game directory");
            return;
        }
        std::filesystem::path path;
        std::string reason;
        if (!throb::resolve_bank(std::filesystem::path(executable).parent_path(), path, reason)) {
            sp_log("[xbpack] V10 FE_MM_THROB: %s", reason.c_str());
            return;
        }
        const std::string filename = path.string();
        file = nflOpenFile(nflMediaID{1}, filename.c_str());
        if (file == NFL_FILE_ID_INVALID) {
            sp_log("[xbpack] V10 FE_MM_THROB: cannot open %s", filename.c_str());
            return;
        }
        // nslLoadBank(memory_type, file_id, bank_offset). The sample is
        // resident PC IMA; memory_type 1 matches native resident banks.
        bank = static_cast<nslBankID>(CDECL_CALL(0x00798450, 1, file.field_0, 0u));
        if (bank == NSL_BANK_ID_INVALID) {
            nflCloseFile(file);
            file = NFL_FILE_ID_INVALID;
            sp_log("[xbpack] V10 FE_MM_THROB: no free NSL bank; menu remains usable");
            return;
        }
        load_started = now;
        sp_log("[xbpack] V10 FE_MM_THROB: loading the original CITY_ARENA sample from %s", filename.c_str());
        // No blocking wait or Sleep. The normal game audio/NFL update pumps
        // this request, and lifecycle retries after the bank becomes ready.
    }

    bool ready() const
    {
        return sound_initialized() && find_source() != -1;
    }

    bool playing()
    {
        return sound_initialized() && voice.field_0 != 0 &&
            voice.get_sound_instance_ptr() != nullptr;
    }

    void play()
    {
        // Use an owned native sound_instance_id, not the fire-and-forget
        // WAV-mod route (which returns zero and cannot stop a looping WAV).
        // Native source type 7 preserves menu volume/mute and WBK loop flags.
        voice = sub_60B960_native(string_hash{"FE_MM_THROB"}, 1.0f, 1.0f);
        if (voice.field_0 != 0 && !logged_play) {
            logged_play = true;
            sp_log("[xbpack] V10 FE_MM_THROB: playing on Press Start (0x39E51482)");
        }
    }

    void stop()
    {
        if (sound_initialized() && voice.field_0 != 0) {
            if (auto *instance = voice.get_sound_instance_ptr())
                instance->stop();
        }
        voice.field_0 = 0;
    }

    void close()
    {
        close_bank();
        tried_extra = false;
    }
};

throb::lifecycle<native_audio> session;
} // namespace

void update(bool active, bool ready)
{
    session.update(active, ready, GetTickCount());
}

void stop()
{
    session.stop();
}

} // namespace xbpack::v10_frontend_throb
#endif
