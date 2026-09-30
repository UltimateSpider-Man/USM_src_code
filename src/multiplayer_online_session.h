#pragma once
#include "multiplayer_online_protocol.h"
#include <memory>

namespace usm::online {
enum class State { offline,connecting,hosting,joined,error };
struct Options {
    std::string nickname="Player",host="127.0.0.1",bind_address="0.0.0.0",content_tag="retail-pc-online-v1";
    std::uint16_t port=7777;
    unsigned capacity=4,send_hz=20;
    double interpolation=.10;
};
struct Participant {
    Identity identity{};
    Pose latest{};
    History history{};
    bool has_pose=false;
    double received=0;
    unsigned ping_ms=0;
};
struct Counters {std::uint64_t tx_bytes=0,rx_bytes=0,rejected=0,udp_received=0,udp_sent=0;};
class Session {
    struct Impl;
    std::unique_ptr<Impl> impl_;
public:
    Session();~Session();
    Session(const Session&)=delete;Session&operator=(const Session&)=delete;
    bool host(const Options&,double now);
    bool join(const Options&,double now);
    void close();
    void poll(double now); // nonblocking, bounded work; engine calls remain on game thread
    void set_local_pose(Pose);
    State state()const;
    bool active()const;
    bool is_host()const;
    std::uint32_t local_id()const;
    const std::array<Participant,max_players>& players()const;
    bool sample(unsigned seat,double now,Pose&)const;
    const std::string& error()const;
    const Counters& counters()const;
    std::uint16_t bound_port()const;
};
} // namespace usm::online
