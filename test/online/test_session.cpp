#include "multiplayer_online_session.h"
#include <chrono>
#include <thread>
#include <cstdlib>
#include <iostream>
#include <memory>
using namespace usm::online;
static unsigned checks=0;
#define CHECK(x) do{++checks;if(!(x)){std::cerr<<__LINE__<<": " #x "\n";std::exit(1);}}while(0)
static unsigned population(const Session& s){unsigned n=0;for(auto& p:s.players())if(p.identity.id)++n;return n;}
int main(){
    Session host;Options options;options.port=0;options.nickname="Host";options.capacity=8;double now=100;
    CHECK(host.host(options,now));CHECK(host.bound_port()!=0);CHECK(host.local_id()==1);CHECK(host.is_host());
    Options join=options;join.port=host.bound_port();join.nickname="Guest";
    std::array<std::unique_ptr<Session>,7> clients;
    for(unsigned i=0;i<7;++i){clients[i]=std::make_unique<Session>();join.nickname="Guest "+std::to_string(i+1);CHECK(clients[i]->join(join,now));}
    auto pump=[&](unsigned steps){for(unsigned k=0;k<steps;++k){now+=.01;host.poll(now);for(auto& c:clients)if(c)c->poll(now);}};
    pump(60);CHECK(population(host)==8);
    for(auto& c:clients){CHECK(c->state()==State::joined);CHECK(population(*c)==8);CHECK(c->players()[0].identity.nickname=="Host");CHECK(c->local_id()!=host.local_id());}
    for(unsigned step=0;step<200;++step){Pose p;p.flags=ready;p.world=9;p.position={float(step)*.1f,4,7};p.velocity={10,0,0};host.set_local_pose(p);
        for(unsigned i=0;i<clients.size();++i){p.position.z=float(i+1)*10;clients[i]->set_local_pose(p);}pump(1);}
    pump(10);CHECK(host.counters().udp_received>100);CHECK(host.counters().udp_sent>100);
    for(auto& c:clients){CHECK(c->counters().udp_received>10);Pose p;CHECK(c->sample(0,now,p));CHECK(p.world==9&&p.position.z==7);
        for(unsigned i=0;i<8;++i){CHECK(c->players()[i].identity.id==host.players()[i].identity.id);CHECK(c->players()[i].has_pose);}}
    auto old_id=clients[2]->local_id();clients[2]->close();pump(20);CHECK(population(host)==7);for(unsigned i=0;i<7;++i)if(i!=2)CHECK(population(*clients[i])==7);
    join.nickname="Rejoined";CHECK(clients[2]->join(join,now));pump(60);CHECK(clients[2]->local_id()!=old_id);CHECK(population(host)==8);
    // Rejected content must not join or corrupt membership.
    clients[6]->close();pump(20);Options wrong=join;wrong.content_tag="different-data-tag";
    CHECK(clients[6]->join(wrong,now));pump(30);CHECK(clients[6]->state()==State::error);CHECK(population(host)==7);
    CHECK(clients[6]->error().find("ContentTag")!=std::string::npos);
    // Duplicate nicknames are deliberately valid: id, not nickname, owns an actor.
    join.nickname="Host";CHECK(clients[6]->join(join,now));pump(60);CHECK(clients[6]->state()==State::joined);CHECK(population(host)==8);
    Session extra;CHECK(extra.join(join,now));for(unsigned i=0;i<50;++i){pump(1);extra.poll(now);}CHECK(extra.state()==State::error);CHECK(population(host)==8);
    host.close();pump(10);for(auto& c:clients)CHECK(c->state()==State::error);
    CHECK(!host.active());CHECK(population(host)==0);
    options.nickname="bad%nick";CHECK(!host.host(options,now));CHECK(host.state()==State::error);
    options.nickname="Host";CHECK(host.host(options,now));join.port=host.bound_port();CHECK(clients[0]->join(join,now));pump(40);
    CHECK(clients[0]->state()==State::joined);
    // No UDP datagrams ever creates a second local hero or steps a simulation:
    // all snapshots come explicitly from set_local_pose; empty streams stay unready.
    Pose out;CHECK(!clients[0]->sample(0,now,out));
    // Stop polling one live process; watchdog expires its reliable connection.
    now+=16;host.poll(now);CHECK(population(host)==1);
    std::cout<<checks<<" actual TCP/UDP session checks passed\n";
}
