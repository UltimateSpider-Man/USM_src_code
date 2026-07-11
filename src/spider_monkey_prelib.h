#pragma once

#include "game.h"
#include "message_board.h"
#include "os_developer_options.h"

#include "float.hpp"
#include "limited_timer.h"
#include "variable.h"
#include "debug_menu.h"

inline Var<bool> god_mode_cheat{ 0x0095A6A8 };
inline Var<bool> ultra_god_mode_cheat{ 0x0095A6A9 };
inline Var<bool> mega_god_mode_cheat{ 0x0095A6AA };

struct spider_monkey_prelib {
    spider_monkey_prelib2();

    //0x004B38E0
    static float state_callback2(int a1);

    //0x004B38F0
    static float delta_callback2(int a1);

    //0x004B3910
    static void on_level_load2();

    //0x004B3B20
    static void on_level_unload2();

    //0x004B6690
    static void start2();

   static void stop2();


    //0x004B6890
    static void render2();
	


    //0x004B6770
    static void frame_advance2(Float a1);

    //0x004B3B60
    static bool is_running2();

    static inline Var<bool> m_running2{0x00959E60};

    static inline Var<float> m_ook_timer2{0x00959E64};

    static inline Var<limited_timer> m_clock2{0x00959FE4};

    static inline Var<float> m_runtime2{0x00959E6C};

    static inline Var<int> m_runtime_text2{0x00959E70};
    static inline Var<int> m_runtime_monkey_text2{0x00959E74};

    static inline Var<float[120]> m_game_control_state2{0x00959C58};

    static inline Var<float[120]> m_game_control_state_last_frame2{0x00959A78};
};

extern void spider_monkey_prelib_patch();
