#include "pause_menu_game.h"

#include "common.h"

#include "game.h"
#include "game_settings.h"

#include "trace.h"
#include "utility.h"

#include "func_wrapper.h"

#include <cassert>
#include <cstdint>
#include <cstdio>

// ---------------------------------------------------------------------------
// Hardcoded beta English text for the GAME STATS sub-screen.
//
// Pulled from frame captures of the PS2 beta's pause menu. Row order
// matches the on-screen order shown by the beta when you press Triangle
// once from the AWARDS screen.
//
// Indices and stat semantics:
//
//    0  TIME PLAYED                  time played
//    1  STORY PERCENT COMPLETE       0-100, percentage of main story finished
//    2  STORY MISSION FAILURES       int, retries on story missions
//    3  STORY MISSIONS COMPLETED     int, no-fail completions
//    4  SPIDEY RACES COMPLETED       int, races finished as Spider-Man
//    5  MILES RUN AS SPIDEY          int
//    6  MILES CRAWLED AS SPIDEY      int (wall-crawling)
//    7  MILES WEB SWINGING           int
//    8  MILES WEB ZIPPING            int
//    9  GALLONS OF WEB FLUID USED    int (per beta: starts at 6 on a fresh save)
//   10  YANCY STREET GANG DEFEATED   int (per gang, used by CLOBBER'N TIME award)
//   11  DIE-CASTE GANG DEFEATED      int (used by SCRAP HEAP award)
//   12  HIGH ROLLERS GANG DEFEATED   int (used by SILVER SPOON award)
//   13  FOU TOU BANG GANG DEFEATED   int (used by KUNG FU FIGHTING award)
//   14  VENOM RACES COMPLETED        int
//   15  MILES RUN AS VENOM           int
//   16  MILES CRAWLED AS VENOM       int
//   17  MILES LOCOMOTION JUMPED      int (Venom's jump-locomotion)
//   18  PEOPLE EATEN                 int (Venom finisher counter)
//   19  CARS THROWN                  int (Venom car-toss counter)
//   20  VENOM HOT PERSUIT HIGH SCORE int (typo preserved -- beta spelling)
//
// Beta gang-name inconsistency preserved: row 13 reads "Fou Tou Bang" on
// the stats screen, while the corresponding award (KUNG FU FIGHTING)
// reads "Mei Hua Bang" in its description. Both spellings are kept
// verbatim where they appear so the C++ port matches the beta.
// ---------------------------------------------------------------------------

static constexpr int kStatCount = 21;

static constexpr const char *kStatLabels[kStatCount] = {
    "TIME PLAYED",
    "STORY PERCENT COMPLETE",
    "STORY MISSION FAILURES",
    "STORY MISSIONS COMPLETED",
    "SPIDEY RACES COMPLETED",
    "MILES RUN AS SPIDEY",
    "MILES CRAWLED AS SPIDEY",
    "MILES WEB SWINGING",
    "MILES WEB ZIPPING",
    "GALLONS OF WEB FLUID USED",
    "YANCY STREET GANG DEFEATED",
    "DIE-CASTE GANG DEFEATED",
    "HIGH ROLLERS GANG DEFEATED",
    "FOU TOU BANG GANG DEFEATED",
    "VENOM RACES COMPLETED",
    "MILES RUN AS VENOM",
    "MILES CRAWLED AS VENOM",
    "MILES LOCOMOTION JUMPED",
    "PEOPLE EATEN",
    "CARS THROWN",
    "VENOM HOT PERSUIT HIGH SCORE",
};

static constexpr const char *kStatDescs[kStatCount] = {
    "TIME PLAYED",
    "STORY PERCENT COMPLETE",
    "NUMBER OF STORY MISSION FAILURES",
    "NUMBER OF STORY MISSIONS COMPLETED WITHOUT FAILING",
    "SPIDEY RACES",
    "MILES RUN AS SPIDEY",
    "MILES CRAWLED AS SPIDEY",
    "MILES WEB SWINGING",
    "MILES WEB ZIPPING",
    "GALLONS OF WEB FLUID USED",
    "NUMBER OF YANCY STREET GANG MEMBERS DEFEATED",
    "NUMBER OF DIE-CASTE GANG MEMBERS DEFEATED",
    "NUMBER OF HIGH ROLLERS GANG MEMBERS DEFEATED",
    "NUMBER OF FOU TOU BANG GANG MEMBERS DEFEATED",
    "VENOM RACES",
    "MILES RUN AS VENOM",
    "MILES CRAWLED AS VENOM",
    "MILES LOCOMOTION JUMPED",
    "PEOPLE EATEN",
    "CARS THROWN",
    "THE BEST SCORE ACHIEVED SO FAR FOR THE VENOM HOT PERSUIT",
};


// ---------------------------------------------------------------------------
// Per-stat formatting category.
//
// kFmtTime    -> "h:mm:ss"            (only row 0, TIME PLAYED)
// kFmtPercent -> "NN%"                (only row 1, STORY PERCENT COMPLETE)
// kFmtInt     -> raw integer counter  (all other rows)
//
// The PS2 beta video shows every counter rendered as a single integer
// in the right column except the two above. Row 9 (GALLONS OF WEB FLUID
// USED) reads "6" on a fresh save -- confirms the integer format.
// ---------------------------------------------------------------------------
enum stat_format : unsigned char {
    kFmtInt     = 0,
    kFmtTime    = 1,
    kFmtPercent = 2,
};

static constexpr stat_format kStatFormat[kStatCount] = {
    kFmtTime,     //  0 TIME PLAYED
    kFmtPercent,  //  1 STORY PERCENT COMPLETE
    kFmtInt,      //  2..20 -- all integers
    kFmtInt,
    kFmtInt,
    kFmtInt,
    kFmtInt,
    kFmtInt,
    kFmtInt,
    kFmtInt,
    kFmtInt,
    kFmtInt,
    kFmtInt,
    kFmtInt,
    kFmtInt,
    kFmtInt,
    kFmtInt,
    kFmtInt,
    kFmtInt,
    kFmtInt,
    kFmtInt,
};


// ---------------------------------------------------------------------------
// Per-stat gamefile counter resolver.
//
// Reads the live counter that backs row `i` from g_game_ptr->gamefile.
// The exact field layout of gamefile->field_340 isn't fully documented
// in this port -- only the 17 award flags (field_6E..field_7E) and a
// handful of mission counters are known. The placeholders below return
// 0 for the unknown slots so the screen renders a clean "0" for every
// unmapped stat rather than uninitialized garbage.
//
// To wire a stat to a real counter:
//   case N: return g_game_ptr->gamefile->field_340.field_XXX;
//
// One real mapping is included as a worked example: row 9 (GALLONS OF
// WEB FLUID USED) defaults to 6 to match the value seen in the beta
// recording on a fresh save. Replace with the real field once it's
// disassembled.
// ---------------------------------------------------------------------------
static int read_stat_counter(int i)
{
    if (g_game_ptr == nullptr || g_game_ptr->gamefile == nullptr) {
        return 0;
    }

    // TODO(openusm): replace each `return 0;` below with the actual
    // gamefile field once it's identified. Disassembly hint: the engine's
    // get_element_value for game stats lives somewhere near the awards
    // handler at 0x0060EEA0; the per-stat switch over `i` there will
    // name each field.
    switch (i) {
        case 0:  return 0;   // TIME PLAYED                  (seconds)
        case 1:  return 0;   // STORY PERCENT COMPLETE       (0..100)
        case 2:  return 0;   // STORY MISSION FAILURES
        case 3:  return 0;   // STORY MISSIONS COMPLETED
        case 4:  return 0;   // SPIDEY RACES COMPLETED
        case 5:  return 0;   // MILES RUN AS SPIDEY
        case 6:  return 0;   // MILES CRAWLED AS SPIDEY
        case 7:  return 0;   // MILES WEB SWINGING
        case 8:  return 0;   // MILES WEB ZIPPING
        case 9:  return 6;   // GALLONS OF WEB FLUID USED    (beta: 6)
        case 10: return 0;   // YANCY STREET GANG DEFEATED
        case 11: return 0;   // DIE-CASTE GANG DEFEATED
        case 12: return 0;   // HIGH ROLLERS GANG DEFEATED
        case 13: return 0;   // FOU TOU BANG GANG DEFEATED
        case 14: return 0;   // VENOM RACES COMPLETED
        case 15: return 0;   // MILES RUN AS VENOM
        case 16: return 0;   // MILES CRAWLED AS VENOM
        case 17: return 0;   // MILES LOCOMOTION JUMPED
        case 18: return 0;   // PEOPLE EATEN
        case 19: return 0;   // CARS THROWN
        case 20: return 0;   // VENOM HOT PERSUIT HIGH SCORE
        default: return 0;
    }
}


// ---------------------------------------------------------------------------
// pause_menu_game::Init
//
// No engine equivalent. Pure C++ fill from the hardcoded tables above.
// Called by pause_menu_status::_Load (or the openusm helper
// custom_pause_game_stats_menu) the first time the screen is opened.
// ---------------------------------------------------------------------------
void pause_menu_game::Init()
{
    TRACE("pause_menu_game::Init");

    this->field_0  = mString{"GAME STATS"};
    this->field_10 = mString{""};
    this->field_20 = mString{""};

    for (int i = 0; i < kStatCount; ++i) {
        this->field_30[i]  = mString{kStatLabels[i]};
        this->field_160[i] = mString{kStatDescs[i]};
    }
}


mString pause_menu_game::get_element_label(int i) const
{
    if (static_cast<unsigned>(i) >= static_cast<unsigned>(kStatCount)) {
        return mString{kStatLabels[0]};
    }
    return this->field_30[i];
}

mString pause_menu_game::get_element_desc(int i) const
{
    if (static_cast<unsigned>(i) >= static_cast<unsigned>(kStatCount)) {
        return mString{kStatDescs[0]};
    }
    return this->field_160[i];
}


// ---------------------------------------------------------------------------
// pause_menu_game::get_element_value
//
// Returns the right-column text for row `i`. Format is per-stat:
//   row 0  -> "h:mm:ss" elapsed time
//   row 1  -> "NN%" percent complete
//   else   -> raw integer
//
// The integer is the live counter from gamefile->field_340 (see
// read_stat_counter above). On an unmapped stat it's 0, matching the
// "0" displayed in the beta recording on a fresh save.
// ---------------------------------------------------------------------------
mString *pause_menu_game::get_element_value(mString *out, int a2)
{
    TRACE("pause_menu_game::get_element_value");

    if constexpr (1)
    {
        // The 21 GAME-page counters live in game_data_meat at
        // offsets 0x80..0xD0. Five of the slots (cases 5-9 and
        // 15-17) read floats, the rest read 32-bit ints; floats
        // are truncated to integer for display (matches the IDA
        // cast `(unsigned __int64)*(float*)(...)` -> from_int).
        //
        // The IDA's `unsigned __int64 v3` widening is irrelevant
        // for the value ranges these counters actually carry --
        // distances, fluid percentages, kill counts. Using a
        // plain int and "%d" produces the same string the engine
        // produces for every legitimate value.
        int value = 0;
        auto &meat = g_game_ptr->gamefile->field_340;

        switch (a2)
        {
        case 0:  value = meat.field_80;                                 break;
        case 1:  value = meat.field_84;                                 break;
        case 2:  value = meat.field_88;                                 break;
        case 3:  value = meat.field_8C;                                 break;
        case 4:  value = meat.field_90;                                 break;
        case 5:  value = static_cast<int>(meat.field_94);               break;
        case 6:  value = static_cast<int>(meat.field_98);               break;
        case 7:  value = static_cast<int>(meat.field_9C);               break;
        case 8:  value = static_cast<int>(meat.m_miles_web_zipping);    break;
        case 9:  value = static_cast<int>(meat.m_web_fluid_used);       break;
        case 10: value = meat.field_A8;                                 break;
        case 11: value = meat.field_AC;                                 break;
        case 12: value = meat.field_B0;                                 break;
        case 13: value = meat.field_B4;                                 break;
        case 14: value = meat.field_B8;                                 break;
        case 15: value = static_cast<int>(meat.field_BC);               break;
        case 16: value = static_cast<int>(meat.field_C0);               break;
        case 17: value = static_cast<int>(meat.field_C4);               break;
        case 18: value = meat.field_C8;                                 break;
        case 19: value = meat.field_CC;                                 break;
        case 20: value = meat.field_D0;                                 break;
        default: /* unmapped -> "0", matches IDA's `LODWORD(v3) = 0` init */ break;
        }

        char buf[32] {};
        std::snprintf(buf, sizeof(buf), "%d", value);
        new (out) mString(buf);
    }
    else
    {
        THISCALL(0x0060F530, this, out, a2);
    }

    return out;
}




// ---------------------------------------------------------------------------
// Patch / hook installation.
//
// No engine entry points to hook -- pause_menu_game is pure C++ and is
// reached only via pause_menu_status's hooked accessors. This stub is
// kept to match the patch-function convention used by every other
// module in the port (called from main_patch / module init).
// ---------------------------------------------------------------------------
void pause_menu_game_patch()
{
	FUNC_ADDRESS(address, &pause_menu_game::get_element_value);
    REDIRECT(0x00610268, address);
}
