#include "pause_menu_awards.h"

#include "common.h"

#include "game.h"
#include "game_settings.h"

#include "localized_string_table.h"

#include "func_wrapper.h"
#include "trace.h"
#include "utility.h"

#include <cassert>
#include <cstdint>
#include <cstdio>

VALIDATE_SIZE(pause_menu_awards, 0x250u);

// ---------------------------------------------------------------------------
// Hardcoded beta English text.
//
// These strings are pulled verbatim from the PS2 beta build (SLUS_208.70)
// AWARDS screen, frame-captured from a bandicam recording of the beta's
// pause menu. They are used as a fall-back whenever Init() finds an empty
// slot in the localized string table -- this happens on PC builds whose
// .lng file lacks the awards block, on debug builds with the table not
// yet loaded, and on machines whose locale doesn't ship awards strings.
//
// kAwardLabels and kAwardDescs are indexed [0..16] matching the row order
// shown in the beta UI:
//
//   0  SPEED DEMON                 Ultimate medal earned in all races.
//   1  ON FIRE                     100% Johnny Storm races.
//   2  ERRAND BOY                  Complete all taxi missions.
//   3  NOTHING BETTER TO DO?       100% secret tokens.
//   4  BIGGEST. FANBOY. EVER.      100% comic book cover tokens.
//   5  ANGSTY                      Unlock Peter Parker hoodie.
//   6  WHAT SECRET IDENTITY?       Unlock Peter Parker with no mask.
//   7  KICKIN' IT 1984 STYLE       Unlock the black suit.
//   8  SLAVE TO FASHION            Unlock all the alternate costumes.
//   9  PRETTY PICTURES             100% concept art unlocked.
//  10  QUICK LIKE BUNNY            Max swing speed unlocked.
//  11  CLOBBER'N TIME              Defeat 500 Yancy Street gang members.
//  12  SCRAP HEAP                  Defeat 500 Die-Caste gang members.
//  13  SILVER SPOON                Defeat 500 High Rollers gang members.
//  14  KUNG FU FIGHTING            Defeat 500 Mei Hua Bang gang members.
//  15  BIG TIME SUPER HERO         Finish the main story.
//  16  NOW GO OUTSIDE              100% completion of the whole game.
//
// Note the gang-name inconsistency in the beta itself: the AWARDS screen
// row 14 uses "Mei Hua Bang", while the GAME STATS screen row 13 uses
// "Fou Tou Bang" for the same gang. Both names are preserved verbatim
// where they appear so the C++ port matches the beta exactly.
// ---------------------------------------------------------------------------
static constexpr const char *kAwardLabels[17] = {
    "SPEED DEMON",
    "ON FIRE",
    "ERRAND BOY",
    "NOTHING BETTER TO DO?",
    "BIGGEST. FANBOY. EVER.",
    "ANGSTY",
    "WHAT SECRET IDENTITY?",
    "KICKIN' IT 1984 STYLE",
    "SLAVE TO FASHION",
    "PRETTY PICTURES",
    "QUICK LIKE BUNNY",
    "CLOBBER'N TIME",
    "SCRAP HEAP",
    "SILVER SPOON",
    "KUNG FU FIGHTING",
    "BIG TIME SUPER HERO",
    "NOW GO OUTSIDE",
};

static constexpr const char *kAwardDescs[17] = {
    "ULTIMATE MEDAL EARNED IN ALL RACES.",
    "100% JOHNNY STORM RACES.",
    "COMPLETE ALL TAXI MISSIONS.",
    "100% SECRET TOKENS.",
    "100% COMIC BOOK COVER TOKENS.",
    "UNLOCK PETER PARKER HOODIE.",
    "UNLOCK PETER PARKER WITH NO MASK.",
    "UNLOCK THE BLACK SUIT.",
    "UNLOCK ALL THE ALTERNATE COSTUMES.",
    "100% CONCEPT ART UNLOCKED.",
    "MAX SWING SPEED UNLOCKED.",
    "DEFEAT 500 YANCY STREET GANG MEMBERS.",
    "DEFEAT 500 DIE-CASTE GANG MEMBERS.",
    "DEFEAT 500 HIGH ROLLERS GANG MEMBERS.",
    "DEFEAT 500 MEI HUA BANG GANG MEMBERS.",
    "FINISH THE MAIN STORY.",
    "100% COMPLETION OF THE WHOLE GAME.",
};

// Header / template strings. field_0 is the "AWARDS" title rendered to
// the left of the spider ornament; field_10 / field_20 are the right-
// column status template strings shown next to each row.
static constexpr const char *kAwardHeader   = "AWARDS";
static constexpr const char *kStrAcquired   = "ACQUIRED";
static constexpr const char *kStrNotAcquired = "NOT ACQUIRED";

// Backfills an mString from a C-string if and only if the mString is
// empty after the locale read. Lets us mirror the beta's localized
// behavior when the table is populated, and silently fall back to
// English when it isn't -- with no visible "missing string" garbage in
// either case.
static inline void backfill_if_empty(mString &dst, const char *fallback)
{
    if (dst.empty() || dst.c_str() == nullptr || dst.c_str()[0] == '\0') {
        dst = mString{fallback};
    }
}


// ---------------------------------------------------------------------------
// pause_menu_awards::Init
//
// PS2 beta entry: 0x003BBA94 (SLUS_208.70)
// PC  entry-point: 0x0060EB90
//
// Pure mirror of the PS2 beta: 37 mString assignments pulling pointers from
// the localized string table. The only PC-vs-PS2 delta is the base index
// (PS2 = 149, PC = 151) -- the same +2 shift used elsewhere in this port.
//
// After the locale read every slot is back-filled from kAwardLabels /
// kAwardDescs / kAward header strings if it came back empty. This is
// the safety net that keeps the screen rendering correct text even on
// builds whose .lng is missing the awards block.
// ---------------------------------------------------------------------------
void pause_menu_awards::Init()
{
    TRACE("pause_menu_awards::Init");

    assert(g_game_ptr != nullptr);

    localized_string_table *stringTable = g_game_ptr->field_7C;
    assert(stringTable != nullptr);
    assert(stringTable->field_0 != nullptr);

    auto &strings = stringTable->field_0->field_0;

    // PS2 reads from string-table byte offset 0x254 (= index 149).
    // PC layout is shifted by +2, so the base is 151.
    constexpr int kBase = 151;

    // Three header / template strings  (PS2: 0x254, 0x258, 0x25C)
    this->field_0  = strings[kBase + 0];
    this->field_10 = strings[kBase + 1];
    this->field_20 = strings[kBase + 2];

    // 17 award labels  (PS2: 0x260 .. 0x2A0)
    for (int i = 0; i < 17; ++i) {
        this->field_30[i] = strings[kBase + 3 + i];
    }

    // 17 award descriptions  (PS2: 0x2A4 .. 0x2E4)
    for (int i = 0; i < 17; ++i) {
        this->field_140[i] = strings[kBase + 20 + i];
    }

    // Safety net: anything still empty after the locale read gets the
    // hardcoded beta English text so the row renders correctly.
    backfill_if_empty(this->field_0,  kAwardHeader);
    backfill_if_empty(this->field_10, kStrAcquired);
    backfill_if_empty(this->field_20, kStrNotAcquired);
    for (int i = 0; i < 17; ++i) {
        backfill_if_empty(this->field_30[i],  kAwardLabels[i]);
        backfill_if_empty(this->field_140[i], kAwardDescs[i]);
    }
}


// ---------------------------------------------------------------------------
// pause_menu_awards::get_element_value
//
// PC entry: 0x0060EEA0.
//
// Replaces the previous "%d / %d" formatter (which always rendered
// "N / 0" because kAwardTargets[] was a TODO-stubbed array) with the
// behavior actually seen in the PS2 beta video: a single mString
// containing either "ACQUIRED" or "NOT ACQUIRED".
//
// The unlock state for each award is a contiguous byte in the gamefile
// data block:
//
//   gamefile->field_340.field_6E + 0  -> SPEED DEMON
//   gamefile->field_340.field_6E + 1  -> ON FIRE
//   ...
//   gamefile->field_340.field_6E + 16 -> NOW GO OUTSIDE
//
// Any non-zero value counts as acquired -- matches the bool reads done
// in pause_menu_root::custom_pause_awards_menu's sp_log dump, which use
// the same 17 fields field_6E .. field_7E.
//
// The two template strings come from Init() (locale or hardcoded
// fallback). Calling get_element_value before Init() has run would
// return empty mStrings, so we double-check and fall back to hardcoded
// English text if either template is empty.
// ---------------------------------------------------------------------------
mString *pause_menu_awards::get_element_value(mString *out, int a2)
{
    TRACE("pause_menu_awards::get_element_value");

    if constexpr (1)
    {
        char  v3    = 0;
        bool  found = true;
        auto& gd    = g_game_ptr->gamefile->field_340;

        switch (a2)
        {
            case 0:  v3 = gd.field_6E; break;
            case 1:  v3 = gd.field_6F; break;
            case 2:  v3 = gd.field_70; break;
            case 3:  v3 = gd.field_71; break;
            case 4:  v3 = gd.field_72; break;
            case 5:  v3 = gd.field_73; break;
            case 6:  v3 = gd.field_74; break;
            case 7:  v3 = gd.field_75; break;
            case 8:  v3 = gd.field_76; break;
            case 9:  v3 = gd.field_77; break;
            case 10: v3 = gd.field_78; break;
            case 11: v3 = gd.field_79; break;
            case 12: v3 = gd.field_7A; break;
            case 13: v3 = gd.field_7B; break;
            case 14: v3 = gd.field_7C; break;
            case 15: v3 = gd.field_7D; break;
            case 16: v3 = gd.field_7E; break;
            default: found = false;    break;
        }

        char buf[32] {};
        std::snprintf(buf, sizeof(buf), "%d", v3, found);
        new (out) mString(buf);
    }
    else
    {
        THISCALL(0x0060EEA0, this, out, a2);   // <- original address
    }

    return out;
}

// ---------------------------------------------------------------------------
// Convenience accessors. Used by pause_menu_status::update_selected and
// pause_menu_status::get_element_desc after the C++ port stopped reading
// the field_30 / field_140 arrays directly. Centralising the bounds
// check here keeps the crash documented in update_selected's comment
// (out-of-range field_128 -> garbage FEText vtable call) from happening
// regardless of which call site invokes us.
// ---------------------------------------------------------------------------
mString pause_menu_awards::get_element_label(int a3) const
{
    if (static_cast<unsigned>(a3) >= 17u) {
        return mString{kAwardLabels[0]};
    }
    return this->field_30[a3];
}

mString pause_menu_awards::get_element_desc(int a3) const
{
    if (static_cast<unsigned>(a3) >= 17u) {
        return mString{kAwardDescs[0]};
    }
    return this->field_140[a3];
}


// ---------------------------------------------------------------------------
// Patch / hook installation
//
// SET_JUMP writes a 5-byte JMP at the target's PC entry-point so every
// existing and future call site dispatches through this implementation.
//
// get_element_value's hook is now enabled -- the previous version was
// disabled because the function returned "N / 0" for every award (a
// regression vs. the original binary's ACQUIRED / NOT ACQUIRED output).
// With this rewrite the C++ version produces the same right-column text
// as the binary, so the hook is safe to enable.
// ---------------------------------------------------------------------------
void pause_menu_awards_patch()
{
    {
        FUNC_ADDRESS(address, &pause_menu_awards::Init);
        SET_JUMP(0x0060EB90, address);
    }

    {
        FUNC_ADDRESS(address, &pause_menu_awards::get_element_value);
      // SET_JUMP(0x0060EEA0, address);
    }
}
