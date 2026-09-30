#pragma once
#include "multiplayer_gamepad.h"
namespace usm::mp {
void gamepads_configure(const std::string &ini_path);
std::array<pad::Sample,2> gamepads_poll(bool focused,double elapsed);
void gamepads_lock(bool locked);
void gamepads_shutdown();
int gamepads_deadzone();
std::string gamepads_diagnostic();
}
