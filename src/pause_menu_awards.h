#pragma once

#include "mstring.h"

// ---------------------------------------------------------------------------
// pause_menu_awards
//
// Holds the 17 award labels + 17 descriptions that populate the AWARDS
// sub-screen of the pause_menu_status menu (field_EC == 0).
//
// Layout matches the PS2 beta and PC engine:
//   0x00  field_0   - title / header template string ("AWARDS")
//   0x10  field_10  - "ACQUIRED" template
//   0x20  field_20  - "NOT ACQUIRED" template
//   0x30  field_30[17]  - 17 award name strings  (left column)
//   0x140 field_140[17] - 17 award description strings (bottom box)
// Total size = 0x250 bytes (verified by VALIDATE_SIZE in the .cpp).
//
// Element-data accessors mirror the engine's calling convention from
// pause_menu_status::update_selected and pause_menu_status::get_element_desc.
// ---------------------------------------------------------------------------

struct pause_menu_awards
{
    mString field_0;          // "AWARDS"
    mString field_10;         // "ACQUIRED"
    mString field_20;         // "NOT ACQUIRED"
    mString field_30[17];     // 17 award name labels   (left column)
    mString field_140[17];    // 17 award descriptions  (bottom box)

    // PC entry 0x0060EB90.
    // Pulls all 37 strings from the localized string table at base index
    // 151. If any slot comes back empty (locale file missing strings),
    // the slot is back-filled with the hardcoded beta English text so
    // the screen never renders blank rows.
    void Init();

    // PC entry 0x0060EEA0.
    // Returns "ACQUIRED" if the award is unlocked for the current save,
    // "NOT ACQUIRED" otherwise. Replaces the previous "%d / %d" stub
    // that mis-formatted the right-hand column.
    //
    // The unlock state is the byte at gamefile->field_340.field_6E + a3
    // (17 contiguous bytes, one per award). Any non-zero value counts
    // as acquired.
    mString* get_element_value(mString *out, int a2);

    // Convenience accessors for pause_menu_status::update_selected and
    // get_element_desc, both of which would otherwise reach into the
    // arrays directly. Centralising the bounds check here prevents the
    // crash documented in pause_menu_status::update_selected.
    mString get_element_label(int a3) const;
    mString get_element_desc(int a3)  const;
};

void pause_menu_awards_patch();
