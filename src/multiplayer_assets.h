#pragma once
#include "multiplayer_match.h"
#include <array>
#include <cstdint>
#include <string>
struct actor;
namespace usm::mp {
// These functions are only called at the deferred game-tick seam, never by Draw.
bool assets_load(const Settings&,const std::string &ini_path,std::string &error);
void assets_release(bool world_teardown=false);
void assets_retry_release();
void assets_pose(const Match&,float elapsed);
std::array<actor*,2> assets_actors();
const std::string &assets_warning();
void log(const std::string &message);
}
