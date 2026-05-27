#include "pc_joypad_device.h"

#include "app.h"
#include "common.h"
#include "femanager.h"
#include "frontendmenusystem.h"
#include "func_wrapper.h"
#include "game.h"
#include "input.h"
#include "inputsettings.h"
#include "pausemenusystem.h"
#include "trace.h"
#include "utility.h"
#include "variables.h"

// Defined in main.cpp -- non-zero whenever the debug menu is on screen
// (independent of focus). Used below to suppress the joypad pause flag
// so pressing Start with the debug menu up no longer triggers
// game::pause() and its sound fade.
extern int debug_enabled;
extern int debug_disabled;

VALIDATE_SIZE(InputState, 0x20);

VALIDATE_SIZE(pc_joypad_device, 0x9Cu);
VALIDATE_OFFSET(pc_joypad_device, field_70, 0x70);

VALIDATE_SIZE(InputCapabilities, 0x28);

int sub_81D1C0(int a1)
{
    if constexpr (1)
    {
        auto result = 0u;
        if (!a1) {
            uint32_t v2 = Input::instance()->field_129D0;
            if (v2) {
                result = 1;
            }

            if (v2 > 1) {
                result |= 2u;
            }

            if (v2 > 2) {
                result |= 4u;
            }

            if (v2 > 3) {
                result |= 8u;
            }
        }

        return result;
    } else {
        return CDECL_CALL(0x0081D1C0, a1);
    }
}

int InputOpen(int a1, unsigned int a2) {
    if constexpr (1) {
        uint32_t result;

        if (a1 || a2 >= Input::instance()->field_129D0) {
            result = 0;
        } else {
            result = a2 + 1;
        }

        return result;
    } else {
        return CDECL_CALL(0x0081D0F0, a1, a2);
    }
}

#include <xinput.h>

void InputGetCapabilities(int a1, InputCapabilities *pCapabilities) {
    if constexpr (1) {
        if (Input::instance()->sub_820570(a1 - 1)) {
            pCapabilities->field_0 = XINPUT_DEVTYPE_GAMEPAD;
        }

        if (Input::instance()->sub_820590(a1 - 1)) {
            pCapabilities->field_20 = 0xFFFF;
            pCapabilities->field_24 = 0xFFFF;
        }
    }

    else {
        CDECL_CALL(0x0081D180, a1, pCapabilities);
    }
}

pc_joypad_device::pc_joypad_device(int in_port) : input_device()
{
    if constexpr (0)
    {
        auto v3 = (!g_master_clock_is_up());

        this->m_vtbl = CAST(m_vtbl, 0x0088EA80);

        if (v3) {
            timeBeginPeriod(1u);
        }

        this->field_94 = timeGetTime();
        this->field_98 = 0;
        auto v4 = sub_81D1C0(0);

        this->field_4 = in_port + 1000000;
        this->field_90 = in_port;
        auto v5 = v4 & (1 << in_port);
        if (v5) {
            this->field_8C = false;
            this->field_88 = false;
        } else {
            this->field_8C = true;
            this->field_88 = true;
        }

        if (v5) {
            std::memset(&this->m_axis_state, 0, sizeof(this->m_axis_state));
            auto v6 = InputOpen(0, in_port);
            this->field_70 = v6;
            if (v6) {
                InputGetCapabilities(v6, &this->field_48);
            }

            std::memset(&this->field_74, 0, sizeof(this->field_74));
        }

    } else {
        THISCALL(0x005991E0, this, in_port);
    }
}

float pc_joypad_device::_get_axis_state(Axis a2, [[maybe_unused]] int a3)
{
    float result;

    // sp_log("%d %d", int(a2), a3);

    if (a2 == 22) {
        result = (this->field_88 == 1);

    } else {
        InputState input_state = this->m_axis_state;
        result = this->_get_axis_state(a2, input_state);
    }
    return result;
}

float pc_joypad_device::_get_axis_old_state(Axis a2, [[maybe_unused]] int a3)
{
    if (a2 == 22) {
        return this->field_8C;
    }

    auto v5 = this->m_axis_old_state;
    return this->_get_axis_state(a2, v5);
}

float pc_joypad_device::_get_axis_delta(Axis a2, [[maybe_unused]] int a3)
{
    if (a2 == 22) {
        return (this->field_88 - this->field_8C);
    }

    InputState axis_state;
    std::memcpy(&axis_state, &this->m_axis_state, sizeof(axis_state));
    auto v7 = this->_get_axis_state(a2, axis_state);

    auto a2a = v7;
    std::memcpy(&axis_state, &this->m_axis_old_state, sizeof(axis_state));
    return a2a - this->_get_axis_state(a2, axis_state);
}

bool pc_joypad_device::_is_connected() {
    return this->field_88 == 0;
}

void pc_joypad_device::_clear_state()
{
    std::memset(&this->m_axis_state, 0, sizeof(this->m_axis_state));
    std::memset(&this->m_axis_old_state, 0, sizeof(this->m_axis_old_state));
}

int InputGetState(unsigned int dwUserIndex, InputState &pState)
{
    if constexpr (1)
    {
        Input::instance()->poll();
        auto *v2 = (InputSettings *) *(&Input::instance()->m_current_connected + dwUserIndex);
        auto &v20 = pState.m_flags;
        *(uint32_t *) &pState.m_flags = 0;
        *(uint32_t *) &pState.m_punch = 0;
        *(uint32_t *) &pState.field_C = 0;

        pState.field_10 = 0;
        pState.field_14 = 0;
        pState.field_18 = 0;
        pState.field_1C = 0;

        auto &v3 = v2->field_18;

        auto func = [&v3](int &result, InputAction minInput, InputAction maxInput) {
            int min = v3.get_state(minInput) * -32767.0f;
            int max = v3.get_state(maxInput) * 32767.0f;
            auto abs_min = std::abs(min);
            auto abs_max = std::abs(max);
            if (abs_min <= abs_max) {
                if (abs_min != abs_max) {
                    result = max;
                }
            } else {
                result = min;
            }
        };

        func(pState.field_10, InputAction::TurnLeft, InputAction::TurnRight);

        func(pState.field_14, InputAction::Backward, InputAction::Forward);

        func(pState.field_18, InputAction::CameraLeft, InputAction::CameraRight);

        func(pState.field_1C, InputAction::CameraDown, InputAction::CameraUp);

        static constexpr float flt_871978 = 255.f;

        pState.m_jump = v3.get_state(InputAction::Jump) * flt_871978;
        pState.m_stick_to_walls = v3.get_state(InputAction::StickToWalls) * flt_871978;
        pState.m_punch = v3.get_state(InputAction::Punch) * flt_871978;
        pState.m_kick = v3.get_state(InputAction::Kick) * flt_871978;
        pState.m_black_button = v3.get_state(InputAction::BlackButton) * flt_871978;
        pState.m_throw_web = v3.get_state(InputAction::ThrowWeb) * flt_871978;
        pState.field_C = v3.get_state(static_cast<InputAction>(10u)) * flt_871978;
        pState.field_D = v3.get_state(static_cast<InputAction>(11u)) * flt_871978;

        // Pause flag (Start button -> 0x10 -> game::pause() + sound fade).
        // While the debug menu is on screen we don't want Start to trigger
        // the pause-menu sound effect, so drop the flag at the input layer.
        // Keeping this gated on (debug_enabled || debug_disabled) means
        // game::pause() itself stays usable from the console or scripts.
        const bool debug_menu_on_screen = (debug_enabled != 0) || (debug_disabled != 0);
        if (0.0f != v3.get_state(InputAction::Pause) && !debug_menu_on_screen) {
            v20 |= 0x10u;
        }

        if (0.0f != v3.get_state(InputAction::BackButton)) {
            v20 |= 0x20u;
        }

        if (0.0f != v3.get_state(static_cast<InputAction>(14u))) {
            v20 |= 0x40u;
        }

        if (0.0f != v3.get_state(InputAction::CameraCenter)) {
            v20 |= 0x80u;
        }

        if (0.0f != v3.get_state(static_cast<InputAction>(24u))) {
            v20 |= 1u;
        }

        if (0.0f != v3.get_state(static_cast<InputAction>(25u))) {
            v20 |= 2u;
        }

        if (0.0f != v3.get_state(static_cast<InputAction>(26u))) {
            v20 |= 4u;
        }

        if (0.0f != v3.get_state(static_cast<InputAction>(27u))) {
            v20 |= 8u;
        }

        return 0;
    }
    else
    {
        return CDECL_CALL(0x0081D240, dwUserIndex, pState);
    }
}

void pc_joypad_device::_poll()
{
    if constexpr (1)
    {
        static Var<bool> dword_967CE4{0x00967CE4};

        if (this->field_70 == 1)
        {
            if ((g_femanager.m_pause_menu_system != nullptr && g_femanager.m_pause_menu_system->m_index != -1) ||
                (g_femanager.m_fe_menu_system != nullptr && g_femanager.m_fe_menu_system->m_index != -1) ||
                dword_967CE4())
            {
                if (((g_femanager.m_pause_menu_system != nullptr && g_femanager.m_pause_menu_system->m_index != -1) ||
                     (g_femanager.m_fe_menu_system != nullptr && g_femanager.m_fe_menu_system->m_index != -1)) &&
                    dword_967CE4())
                {
                    dword_967CE4() = false;
                    Input::instance()->sub_8203F0(0, g_inputSettingsMenu());
                }
            }
            else
            {
                dword_967CE4() = true;
                Input::instance()->sub_8203F0(0, g_inputSettingsInGame());
            }
        }

        if (this->field_70 && !this->field_88)
        {
            if (this->field_8C) {
                this->_clear_state();
            }

            this->m_axis_old_state = this->m_axis_state;
            auto v2 = InputGetState(this->field_70, this->m_axis_state);
            if (v2 != 0)
            {
                assert(0);

                this->field_98 = 0;
                mString a1{0, "state %d", v2};

                return;
            }

            Var<BOOL> dword_9363E4{0x009363E4};

            if (g_game_ptr != nullptr) {
                if (dword_9363E4()) {
                    if (this->m_axis_state.m_black_button > 50u) {
                        dword_9363E4() = false;
                    }
                }

                if (!dword_9363E4() && this->m_axis_state.m_black_button == 0) {
                    dword_9363E4() = true;
                }
            }
        }

    } else {
        THISCALL(0x0058E5C0, this);
    }
}

double sub_58E7F0(int a1) {
    float result = 0.0f;
    float v3 = a1;
    auto a1a = v3;
    if (v3 > 7000.0f || a1a < -7000.0f) {
        if (a1 <= 0) {
            result = -(((double) -a1 - 7000.0f) * 0.000038809329f);
        } else {
            result = (a1a - 7000.0f) * 0.000038809329f;
        }
    }

    if (std::abs(result) < 0.1f) {
        result = 0.0f;
    }

    return result;
}

float pc_joypad_device::_get_axis_state(Axis axis, InputState input_state)
{
    TRACE("pc_joypad_device::get_axis_state");

    auto result = 0.0f;

    float v5;
    double a2a;
    switch (axis) {
    case 0: {
        if ((input_state.m_flags & 4) != 0) {
            return (-1.0f);
        }

        return ((input_state.m_flags & 8) != 0);
    }
    case 1: {
        if ((input_state.m_flags & 1) != 0) {
            return (-1.0f);
        }

        return ((input_state.m_flags & 2) != 0);
    }
    case 2: {
        a2a = sub_58E7F0(input_state.field_10);
        if ((input_state.m_flags & 8) != 0)
        {
            result = 1.0f;
            v5 = (-1.0f);

            if (a2a == v5) {
                return 0.0f;
            }

            return result;
        }

        if ((input_state.m_flags & 4) == 0) {
            return a2a;
        }

        result = (-1.0f);
        v5 = 1.0f;
        if (a2a == v5) {
            return 0.0f;
        }

        return result;
    }
    case 3: {
        a2a = -sub_58E7F0(input_state.field_14);
        if ((input_state.m_flags & 1) != 0) {
            result = (-1.0f);
            v5 = 1.0f;
        } else {
            if ((input_state.m_flags & 2) == 0) {
                return a2a;
            }
        }

        if (a2a == v5) {
            return 0.0f;
        }

        return result;
    }
    case 4:
        return sub_58E7F0(input_state.field_10);
    case 5:
        return -sub_58E7F0(input_state.field_14);
    case 6: {
        auto result = ((input_state.m_flags & 0x40) != 0);
        return result;
    }
    case 7:
        return sub_58E7F0(input_state.field_18);
    case 8: {
        result = -sub_58E7F0(input_state.field_1C);
        return result;
    }
    case 9: { // CameraCenter
        auto result = ((input_state.m_flags & 0x80u) != 0);

        return result;
    }
    case Axis::Jump: {
        return (input_state.m_jump > 30u);
    }
    case 11: {
        return (input_state.m_stick_to_walls > 30u);
    }
    case 12:
    case 15: {
        return 0.0f;
    }
    case 13: {
        return (input_state.m_punch > 30u);
    }
    case 14: {
        return (input_state.m_kick > 30u);
    }
    case 16: {
        return (input_state.field_C * 0.0039215689f);
    }
    case 17: {
        auto result = (input_state.field_D * 0.0039215689f);
        return result;
    }
    case 18: {
        float result = (input_state.m_black_button > 30u);
        return result;
    }
    case 19: {
        float result = (input_state.m_throw_web > 30u );
        return result;
    }
    case 20: {
        return ((input_state.m_flags & 0x10) != 0);
    }
    case 21: {
        return ((input_state.m_flags & 0x20) != 0);
    }
    case 22: {
        return (this->field_88 != 0);
    }
    default:
        assert(0 && "Invalid axis.");

        return result;
    }
}

void pc_joypad_device_patch()
{
    {
        FUNC_ADDRESS(address, &pc_joypad_device::_poll);
        set_vfunc(0x0088EAA0, address);
    }

    {

        float (pc_joypad_device::*func)(pc_joypad_device::Axis, int) = &pc_joypad_device::_get_axis_state;
        FUNC_ADDRESS(address, func);
        set_vfunc(0x0088EA94, address);
    }

    {

        float (pc_joypad_device::*func)(pc_joypad_device::Axis, InputState) = &pc_joypad_device::_get_axis_state;
        FUNC_ADDRESS(address, func);
        set_vfunc(0x0088EAC4, address);
    }

}