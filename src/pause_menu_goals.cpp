#include "pause_menu_goals.h"

#include "common.h"

#include "game.h"
#include "game_settings.h"

#include "localized_string_table.h"
#include "mission_manager.h"

#include "func_wrapper.h"
#include "trace.h"
#include "utility.h"

#include <cassert>
#include <cstdint>
#include <cstdio>

VALIDATE_SIZE(pause_menu_goals, 0x110u);


void pause_menu_goals::Init()
{
    TRACE("pause_menu_goals::Init");

    assert(g_game_ptr != nullptr);

    localized_string_table *stringTable = g_game_ptr->field_7C;
    assert(stringTable != nullptr);
    assert(stringTable->field_0 != nullptr);

    auto &strings = stringTable->field_0->field_0;

    constexpr int kBase = 275;          // PC index; PS2 = 268.

    this->field_0 = strings[kBase + 0];

    for (int i = 0; i < 4; ++i) this->field_10[i] = strings[kBase + 1  + i];
    for (int i = 0; i < 4; ++i) this->field_50[i] = strings[kBase + 5  + i];
    for (int i = 0; i < 4; ++i) this->field_90[i] = strings[kBase + 9  + i];
    for (int i = 0; i < 4; ++i) this->field_D0[i] = strings[kBase + 13 + i];
}

mString *pause_menu_goals::get_element_value(mString *out, int a2)
{
    TRACE("pause_menu_goals::get_element_value");

    if constexpr (1)
    {
        int current = 0;   // v3
        int target  = 0;   // v4

        // v5 in IDA. Short-circuit: true if story isn't active, OR story is
        // active but sub_5C5920 returns true. Only false when story is active
        // AND sub_5C5920 is false — in which case we read the alternate set
        // of counters that lives 0x20 bytes earlier in the struct.
        const bool use_main =
            !mission_manager::s_inst->is_story_active()
            || mission_manager::s_inst->sub_5C5920();

        auto &gd = g_game_ptr->gamefile->field_340;

        switch (a2)
        {
        case 0:
            if (use_main) { current = gd.field_F4;  target = gd.field_104; }
            else          { current = gd.field_D4;  target = gd.field_E4;  }
            break;
        case 1:
            if (use_main) { current = gd.field_F8;  target = gd.field_108; }
            else          { current = gd.field_D8;  target = gd.field_E8;  }
            break;
        case 2:
            if (use_main) { current = gd.field_FC;  target = gd.field_10C; }
            else          { current = gd.field_DC;  target = gd.field_EC;  }
            break;
        case 3:
            if (use_main) { current = gd.field_100; target = gd.field_110; }
            else          { current = gd.field_E0;  target = gd.field_F0;  }
            break;
        default:
            // Out of range: original leaves v3/v4 at 0 → "0 / 0".
            break;
        }

        char buf[32] {};
        snprintf(buf, sizeof(buf), "%d / %d", current, target);
        new (out) mString(buf);
    }
    else
    {
        THISCALL(0x0060FB00, this, out, a2);   // <- original address
    }

    return out;
}



mString *pause_menu_goals::get_element_desc(mString *out, int a2)
{
    TRACE("pause_menu_goals::get_element_desc");

    if constexpr (1)
    {
        auto *gf = g_game_ptr->gamefile;
        int target;   // v4
        int current;  // v5

        switch (a2)
        {
        case 0:
            target  = gf->field_340.field_104;
            current = gf->field_340.field_F4;
            break;
        case 1:
            target  = gf->field_340.field_108;
            current = gf->field_340.field_F8;
            break;
        case 2:
            target  = gf->field_340.field_10C;
            current = gf->field_340.field_FC;
            break;
        default:  // a2 >= 3
            target  = gf->field_340.field_110;
            current = gf->field_340.field_100;
            break;
        }

        if (current < target)
        {
            if (current == target - 1)
            {
                // Exactly one left — show the "almost done" message
                new (out) mString(this->field_90[a2]);
            }
            else
            {
                // N remaining — format the count into the template string.
                // Use snprintf to bound the write; matches behavior of the
                // original sprintf without the buffer-overflow footgun.
                char buf[256] {};
                const char *fmt = this->field_50[a2].c_str();
                if (fmt && *fmt)
                    snprintf(buf, sizeof(buf), fmt, target - current);
                new (out) mString(buf);
            }
        }
        else
        {
            // current >= target → goal completed
            new (out) mString(this->field_D0[a2]);
        }
    }
    else
    {
        THISCALL(0x0060FCD0, this, out, a2);   // <- original address
    }

    return out;
}

void pause_menu_goals_patch()
{


    {
        FUNC_ADDRESS(address, &pause_menu_goals::Init);
        SET_JUMP(0x0060F980, address);
    }

    {
        FUNC_ADDRESS(address, &pause_menu_goals::get_element_value);
        SET_JUMP(0x0060FB00, address);
    }

    {
        FUNC_ADDRESS(address, &pause_menu_goals::get_element_desc);
        SET_JUMP(0x0060FCD0, address);
    }


    // sub_60FE10 is OpenUSM-only -- no SET_JUMP.
}