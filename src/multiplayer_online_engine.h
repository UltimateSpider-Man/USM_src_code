#pragma once
#include "multiplayer_online_session.h"
#include <string>
namespace usm::online {
void engine_configure(const std::string& ini);
// False while loading, in a story mission, or without a valid native hero.
bool engine_capture(Pose& out);
void engine_update(const Session&,double now,float dt);
void engine_draw_world(const Session&,double now);
void engine_release(bool world_teardown=false);
const std::string& engine_warning();
void draw_text(float x,float y,const std::string&,float scale=1,std::uint32_t color=0xffeeeeee);
void draw_rect(float x,float y,float width,float height,std::uint32_t color);
} // namespace usm::online
