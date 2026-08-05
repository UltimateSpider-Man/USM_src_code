#include "sound_bank_slot.h"

#include "common.h"
#include "func_wrapper.h"
#include "trace.h"
#include "utility.h"
#include "nfl_system.h"

VALIDATE_SIZE(sound_bank_slot, 0x38);

Var<bool> s_running_from_resource_pack {0x0095C828};

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
    }
}
