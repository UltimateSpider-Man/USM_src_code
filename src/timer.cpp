#include "timer.h"

#include "common.h"
#include "func_wrapper.h"
#include "game.h"
#include "trace.h"
#include "variables.h"
#include "utility.h"

#include <profileapi.h>

VALIDATE_SIZE(Timer, 0x58);

Var<Timer *> g_timer{0x00965BF0};

Timer::Timer(Float a2, Float a3)
{
    TRACE("Timer::Timer", std::to_string(float(a2)).c_str(), std::to_string(float(a3)).c_str());

    this->field_24 = QueryPerformanceFrequency(&this->field_0);
    this->field_8.LowPart = 0;
    this->field_8.HighPart = 0;
    this->field_10 = 0;
    this->field_14 = 0;
    this->field_1C = 0;
    this->field_20 = 0;
    this->field_3C = 0;
    this->field_54 = 0;
    this->field_40 = 0;
    this->field_44 = 0;
    this->field_48 = 0;
    this->field_4C = 0;
    this->field_50 = 0;
    this->field_18 = 1000;
    this->field_28 = a2;
    this->field_2C = a3;
    this->field_38 = 1;
    this->field_30 = 1.0 / a2;
    this->field_34 = 1.0 / a3;
}

float Timer::sub_5821D0()
{
    float (__fastcall *func)(void *) = CAST(func, 0x005821D0);
    auto time_inc = func(this);

#if FPS_UNLOCK_60
    // The Timer was bumped to TargetFPS in main.cpp (Timer{60.0, 60.0}),
    // so the original returns ~1/TargetFPS per call instead of ~1/30.
    //
    // Scale by 30/Target to preserve wall-clock speed for frame-counter
    // based game logic (animation frame counts, "field_140 = 3.0" style
    // cooldowns, hard-coded ticks like frame_advance(0.06666667)).
    //
    // Doing the scale here -- one chokepoint -- is simpler and safer
    // than scaling per-call-site downstream.
    if (g_config.EnableHighFPS &&
        g_config.CompensateSimDt &&
        g_config.TargetFPS > 60)
    {
        time_inc *= 60.0f / static_cast<float>(g_config.TargetFPS);
    }
#endif

    return time_inc;
}

float Timer::super_slow_motion()
{
    float (__fastcall *func)(void *) = CAST(func, 0x005821D0);
    auto time_inc = func(this);
    
    // Apply slow motion multiplier (0.25f = half speed)
    time_inc *= 0.25f;
    
    FUNC_ADDRESS(address, &Timer::super_slow_motion);
    REDIRECT(0x005D7000, address);
    REDIRECT(0x005D702C, address);
    
    return time_inc;
}


float Timer::slow_motion()
{
    float (__fastcall *func)(void *) = CAST(func, 0x005821D0);
    auto time_inc = func(this);
    
    // Apply slow motion multiplier (0.5f = half speed)
    time_inc *= 0.5f;
    
    FUNC_ADDRESS(address, &Timer::slow_motion);
    REDIRECT(0x005D7000, address);
    REDIRECT(0x005D702C, address);
    
    return time_inc;
}

float Timer::normal_motion()
{
    float (__fastcall *func)(void *) = CAST(func, 0x005821D0);
    auto time_inc = func(this);
    
    // Apply slow motion multiplier (1.0f = normal speed)
    time_inc *= 1.0f;
    
    FUNC_ADDRESS(address, &Timer::normal_motion);
    REDIRECT(0x005D7000, address);
    REDIRECT(0x005D702C, address);
    
    return time_inc;
}

float Timer::speed_motion()
{
    float (__fastcall *func)(void *) = CAST(func, 0x005821D0);
    auto time_inc = func(this);
    
    // Apply slow motion multiplier (1.5f = super speed)
    time_inc *= 1.5f;
    
    FUNC_ADDRESS(address, &Timer::speed_motion);
    REDIRECT(0x005D7000, address);
    REDIRECT(0x005D702C, address);
    
    return time_inc;
}

float Timer::super_speed_motion()
{
    float (__fastcall *func)(void *) = CAST(func, 0x005821D0);
    auto time_inc = func(this);
    
    // Apply slow motion multiplier (2.0f = super speed)
    time_inc *= 2.0f;
    
    FUNC_ADDRESS(address, &Timer::super_speed_motion);
    REDIRECT(0x005D7000, address);
    REDIRECT(0x005D702C, address);
    
    return time_inc;
}

void Timer::sub_582180()
{
    if (this->field_24) {
        QueryPerformanceCounter(&this->field_8);
        this->field_3C = 0;
        this->field_54 = 0;
        this->field_40 = 0;
        this->field_44 = 0;
        this->field_4C = 0;
    } else {
        DWORD v2 = GetTickCount();
        this->field_3C = 0;
        this->field_54 = 0;
        this->field_40 = 0;
        this->field_44 = 0;
        this->field_4C = 0;
        this->field_1C = v2;
    }
}

Timer * __fastcall Timer_constructor(Timer *self, void *, Float a2, Float a3)
{
    return new (self) Timer {a2, a3};
}

void Timer_patch()
{
    {
        REDIRECT(0x005AC399, Timer_constructor);
        REDIRECT(0x0076E7DF, Timer_constructor);
    }

    {
        FUNC_ADDRESS(address, &Timer::sub_5821D0);
        REDIRECT(0x005D7000, address);
        REDIRECT(0x005D702C, address);
 
			

    }
}


