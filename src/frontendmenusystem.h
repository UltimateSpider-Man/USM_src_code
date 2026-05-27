#pragma once

#include "femenusystem.h"
#include "limited_timer.h"
#include "mAvlTree.h"
#include "mstring.h"
#include "sound_instance_id.h"
#include "variable.h"

struct PanelFile;
struct fe_state;
struct string_hash_entry;

struct FrontEndMenuSystem : FEMenuSystem {
    struct fe_state {
        int field_0;
    };

    limited_timer_base field_2C;
    int field_30;
    int field_34;
    sound_instance_id field_38;
    mString field_3C;
    int field_4C;
    bool field_50;
    bool field_51;
    bool field_52;
    int field_54;
    int field_58;
    mAvlTree<string_hash_entry> field_5C;
    mAvlTree<string_hash_entry> field_6C;
    PanelFile *field_7C;

    //0x00648580
    FrontEndMenuSystem();





    bool sub_60C230();

    void sub_60C240();

    //0x00619210
    bool WaitForMemCheck();
	
	void Update(float a2);

	void sub_612F40(float a2);

    //0x00619230
    void RenderLoadMeter(bool a1);

    //0x00635BC0
    void GoNextState();

    //0x0060C1E0
    /* virtual */ void MakeActive(int a2);

    void BringUpDialogBox(int a2, FrontEndMenuSystem::fe_state a3, FrontEndMenuSystem::fe_state a4);

    void sub_60C290();

    void sub_6342D0();

    void sub_619030(bool a2);
};


extern bool & already_drew_this_frame;


extern void FrontEndMenuSystem_patch();
