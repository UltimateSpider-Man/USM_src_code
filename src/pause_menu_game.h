#pragma once

#include "mstring.h"

// ---------------------------------------------------------------------------
// pause_menu_game
//
// Holds the 21 game-stats labels + descriptions that populate the GAME
// STATS sub-screen of pause_menu_status (field_EC == 1). Mirrors the
// shape of pause_menu_awards but with a different element count and a
// right-column value that's the live integer counter instead of an
// ACQUIRED / NOT ACQUIRED flag.
//
// Layout:
//   0x000  field_0    - title / template ("GAME STATS")
//   0x010  field_10   - secondary template
//   0x020  field_20   - secondary template
//   0x030  field_30[21]   - 21 stat labels         (left column)
//   0x180  field_180[21]  - 21 stat descriptions   (bottom box)
//
// Note: this struct does NOT match the engine's original pause_menu_game
// layout byte-for-byte. The engine read field_160[a3] for descriptions
// (only 19 entries fit before that offset); the beta we're reproducing
// has 21 stats. Because pause_menu_status::get_element_desc and
// update_selected are both hooked in the C++ port and dispatch through
// the accessor methods below, the actual byte offsets here are free
// for us to choose.
// ---------------------------------------------------------------------------

struct pause_menu_game
{
    mString field_0;          // "GAME STATS"
    mString field_10;         // template
    mString field_20;         // template
    mString field_30[21];     // 21 stat name labels      (left column)
    mString field_160[21];    // 21 stat descriptions     (bottom box)

    // Populate every slot. The engine has no equivalent entry point
    // (PS2 beta read game-stats text from a different table block we
    // haven't located in the PC binary), so this is a pure C++ fill
    // using the hardcoded beta strings.
    void Init();

    // Left column for row `i`. Bounds-checked: out-of-range falls
    // back to row 0 to mirror the awards-side defensive behavior.
    mString get_element_label(int i) const;

    // Bottom description box for row `i`.
    mString get_element_desc(int i) const;

    // Right column for row `i`. Returns the formatted integer value
    // read from the gamefile counter that backs this stat. Stats with
    // a "time played" layout return "h:mm:ss"; percent-style stats
    // return "NN%"; everything else returns the raw integer.
    mString *get_element_value(mString *out, int a2);
};

extern void pause_menu_game_patch();
