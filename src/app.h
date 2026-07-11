#pragma once

#include "singleton.h"

#include "variable.h"
#include "variables.h"

#include "limited_timer.h"
#include "mstring.h"

struct game;

struct app : singleton {
    struct internal {
        mString field_0;
        int field_10;
        int field_14;
        int field_18;
        int field_1C;
        int field_20;
        float field_24;
        int field_28;

        //0x005B85D0
        internal() : field_0() {
            this->field_1C = 640;
            this->field_20 = 480;
            this->field_24 = 1.3333334;
            this->field_10 = 0;
            this->field_14 = -1;

            this->field_0 = "";

            this->field_18 = 0;
            this->field_28 = 0;
        }

        void sub_5B8670();

        void begin_screen_recording(const mString &a2, int a3);

        void end_screen_recording();
    };

    internal field_4;
    game *m_game;
    limited_timer_base field_34;
    int field_38;

    //0x005E1070
    app();
	
		void app1();

    //0x005D6FC0
    void tick();
	

    //0x005B2450
    static void create_inst();

    //0x005D9430
    static void cleanup();

    static app *& instance;

    //0x005E99D0 + 0x005DBC10
    //virtual
    ~app();
	
};

//0x00544E90
extern void colgeom_init_lists();

//0x004BC040
extern void set_god_mode(int a1);

extern void app_patch();

extern void app1_patch();
