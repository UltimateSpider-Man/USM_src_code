#pragma once
#include <string>

// The multiplayer menu owns the native hooks. This controller owns only the
// selected NDS-style PC arena and its independently loaded fighter resources.
namespace usm::mp::arena {
void open(const std::string& ini_path);
void tick(double elapsed);
void draw();
void close(bool world_shutdown=false);
bool active();
bool take_switch_request();
}
