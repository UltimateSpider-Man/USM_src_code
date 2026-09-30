// Standalone transport diagnostic. Synthetic poses only: does not launch USM.
#include "multiplayer_online_session.h"
#include "multiplayer_online_interfaces.h"
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <thread>
using namespace usm::online;
int main(int argc,char** argv){
    if(argc!=7){std::cerr<<"usage: online_probe host|join IP port nickname seconds expected_players\n";return 2;}
    const bool host=std::string(argv[1])=="host";
    if(!host&&std::string(argv[1])!="join")return 2;
    char* end=nullptr;long port=std::strtol(argv[3],&end,10);if(!end||*end||port<0||port>65535)return 2;
    double duration=std::strtod(argv[5],&end);if(!end||*end||!std::isfinite(duration)||duration<.2||duration>3600)return 2;
    long expected=std::strtol(argv[6],&end,10);if(!end||*end||expected<1||expected>8)return 2;
    Options o;o.host=argv[2];o.port=std::uint16_t(port);o.nickname=argv[4];o.capacity=8;
    Session s;auto start=std::chrono::steady_clock::now();
    if(!(host?s.host(o,0):s.join(o,0))){std::cerr<<s.error()<<'\n';return 1;}
    const auto local_addresses=local_ipv4_addresses();
    for(const auto& a:local_addresses)std::cout<<"ADDRESS "<<a.adapter<<" "<<a.ipv4<<'\n';
    std::cout<<"READY "<<s.bound_port()<<'\n'<<std::flush;
    bool complete=false;unsigned max_seen=0,samples=0;double now=0;
    while(now<duration){now=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();s.poll(now);
        if(s.state()==State::error){std::cerr<<s.error()<<'\n';return 1;}
        Pose p;p.flags=ready;p.world=0x13579;p.position={float(std::sin(now)*10),3.f,float(now)};p.velocity={float(std::cos(now)*10),0,1};s.set_local_pose(p);
        unsigned count=0;for(unsigned i=0;i<max_players;++i){const auto& who=s.players()[i];if(!who.identity.id)continue;++count;
            if(who.identity.id!=s.local_id()){Pose q;if(s.sample(i,now,q)&&q.world==p.world&&valid_pose(q))++samples;}}
        max_seen=std::max(max_seen,count);if(count>=unsigned(expected))complete=true;
        std::this_thread::sleep_for(std::chrono::milliseconds(3));
    }
    const auto c=s.counters();std::cout<<"RESULT peers="<<max_seen<<" samples="<<samples<<" tx="<<c.tx_bytes<<" rx="<<c.rx_bytes<<" udp_rx="<<c.udp_received<<" rejected="<<c.rejected<<'\n';
    // Keeping the host alive slightly longer is the caller's responsibility.
    return complete&&(expected==1||(samples>10&&c.udp_received>5))?0:1;
}
