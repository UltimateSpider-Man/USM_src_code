#pragma once

#include <cstdint>

// Indices into controllerKeys[] (defined in main.cpp).
//
// Each slot is a frame counter:
//   - 0      = input is not active
//   - >= 1   = input has been active for that many polled frames
//
// Read with the helpers in main.cpp:
//   is_menu_key_clicked(k, 0)  -> non-zero on every frame the input is held
//   is_menu_key_pressed(k, 0)  -> true only on the frame the input becomes active
//
// The DS4 / DirectInput hook in main.cpp populates these from
// IDirectInputDevice8::GetDeviceState (DIJOYSTATE2).

typedef enum {
    MENU_SELECT,
    MENU_ACCEPT,        // DS4 Cross  (btn 1)
    MENU_BACK,          // DS4 Circle (btn 2)
    MENU_START,

    MENU_UP,
    MENU_DOWN,
    MENU_LEFT,
    MENU_RIGHT,

    MENU_L3,
    MENU_R3,

    MENU_L3_LEFT, MENU_L3_RIGHT, MENU_L3_UP, MENU_L3_DOWN,
    MENU_R3_LEFT, MENU_R3_RIGHT, MENU_R3_UP, MENU_R3_DOWN,

    /* --- USM game-action slots (pc_joypad) --- */
    MENU_FORWARD,       // alias for "forward" intent (dpad up OR L3 up past deadzone)
    MENU_SQUARE,        // DS4 Square   (btn 0) -> USM Punch / ThrowWeb
    MENU_CROSS,         // DS4 Cross    (btn 1) -> USM Jump  (distinct from MENU_ACCEPT)
    MENU_PUNCH,         // logical: mapped from Square
    MENU_KICK,          // DS4 Triangle (btn 3) -> USM Kick
    MENU_BLACK_BUTTON,  // DS4 R1       (btn 5) -> USM BlackButton (Xbox-era "black")

    MENU_KEY_MAX
} MenuKey;

extern uint32_t controllerKeys[MENU_KEY_MAX];
