#pragma once
#include "multiplayer_match.h"
#include <array>
#include <cstdint>
#include <string>
namespace usm::mp {
enum class LanState { off,listening,connecting,handshake,assets,waiting,playing,error };
bool lan_begin(bool host,const std::string &ipv4,std::uint16_t port,const Settings&);
void lan_pump();
void lan_close();
LanState lan_state();
const std::string &lan_error();
const Settings &lan_settings();
bool lan_is_host();
void lan_ready();
// Nonblocking lockstep: samples each local input once, never predicts a remote input.
bool lan_exchange(std::uint32_t frame,Input local,std::uint32_t checksum,std::array<Input,2>&);
}
