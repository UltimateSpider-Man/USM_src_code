#pragma once

#include "mstring.h"

struct FEMenuSystem;

// ---------------------------------------------------------------------------
// pause_menu_goals
//
// Total size: 0x110 (272 bytes) — matches IDA `new(0x110u)` allocation
// in pause_menu_status::pause_menu_status.  This is a PLAIN STRUCT, NOT
// a FEMenu derivative (the original ctor at 0x0060F820 takes only `this`
// and just default-constructs the 17 mString slots).
// ---------------------------------------------------------------------------

struct pause_menu_goals {
    mString field_0;            // 0x000 - "GOALS" header / template string
    mString field_10[4];        // 0x010 - per-goal progress format ("%d / %d ...")
    mString field_50[4];        // 0x050 - per-goal "X more" descriptions
    mString field_90[4];        // 0x090 - per-goal "ONE MORE" final-step strings
    mString field_D0[4];        // 0x0D0 - per-goal completion strings

    //0x0060F980
    void Init();

    //0x0060FB00
    mString* get_element_value(mString* out ,int a2);

    //0x0060FCD0
    mString* get_element_desc(mString* out ,int a2);


};


extern void pause_menu_goals_patch();