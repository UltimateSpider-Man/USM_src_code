#include "multiplayer_online_protocol.h"
#include <cstdlib>
#include <iostream>
#include <limits>
#include <random>
using namespace usm::online;
static unsigned checks=0;
#define CHECK(x) do{++checks;if(!(x)){std::cerr<<__LINE__<<": " #x "\n";std::exit(1);}}while(0)
static bool near(float a,float b){return std::abs(a-b)<.0002f;}
int main(){
    CHECK(valid_nickname("Piero"));CHECK(valid_nickname("Player 2"));CHECK(!valid_nickname(" Player"));CHECK(!valid_nickname(""));
    CHECK(!valid_nickname("a\nb"));CHECK(!valid_nickname("%s"));CHECK(!valid_nickname(std::string(25,'a')));
    CHECK(!valid_nickname("../test/"));CHECK(newer(0,0xffffffffu));CHECK(!newer(4,4));CHECK(!newer(0xffffffffu,0));
    auto color=player_color(0);CHECK(color.b>color.r&&color.b>color.g);
    for(unsigned i=0;i<max_players;++i)for(unsigned j=0;j<i;++j){auto a=player_color(i),b=player_color(j);CHECK(a.r!=b.r||a.g!=b.g||a.b!=b.b);}
    Writer w;w.u32(0x12345678);w.f32(1.0f);
    const Bytes expected{0x12,0x34,0x56,0x78,0x3f,0x80,0,0};CHECK(w.bytes==expected);
    Reader r(w.bytes);CHECK(r.u32()==0x12345678);CHECK(r.f32()==1);CHECK(r.done());r.u8();CHECK(!r.good());
    Reader empty(nullptr,0);CHECK(empty.text(24).empty());CHECK(!empty.good());
    Pose p;p.sequence=41;p.world=0x123;p.flags=ready;p.position={1,2,3};p.velocity={10,0,0};p.animation_time=.25f;
    Writer pack;pack.pose(p);for(std::size_t n=0;n<pack.bytes.size();++n){Reader short_read(pack.bytes.data(),n);short_read.pose();CHECK(!short_read.done());}
    Reader unpack(pack.bytes);Pose other=unpack.pose();CHECK(unpack.done());CHECK(other.sequence==41&&other.world==0x123&&other.position.z==3);
    p.position.x=std::numeric_limits<float>::quiet_NaN();CHECK(!valid_pose(p));p.position.x=1;
    p.rotation.w=0;CHECK(!valid_pose(p));p.rotation.w=1;p.model=Model(99);CHECK(!valid_pose(p));p.model=Model::spiderman;
    p.flags=255;CHECK(!valid_pose(p));p.flags=ready;
    // Analytically known movement: at t=.5, x=5; no frame-rate-dependent easing.
    History h;p.sequence=1;p.position={0,0,0};p.velocity={10,0,0};p.animation_time=0;CHECK(h.push(p,0));
    p.sequence=2;p.position.x=10;p.animation_time=1;CHECK(h.push(p,1));CHECK(h.sample(.5,other));CHECK(near(other.position.x,5));CHECK(near(other.animation_time,.5));
    CHECK(h.sample(10,other));CHECK(near(other.position.x,11)); // hard 100 ms extrapolation cap
    CHECK(!h.push(p,2));p.sequence=0;CHECK(!h.push(p,2));p.sequence=3;p.position.x=1000;CHECK(h.push(p,2));CHECK(h.size()==1);CHECK(h.sample(1.5,other));CHECK(other.position.x==1000);
    p.sequence=4;p.world=7;CHECK(h.push(p,3));CHECK(h.size()==1);
    p.sequence=5;p.flags=0;CHECK(h.push(p,4));CHECK(h.size()==1);CHECK(h.sample(3.5,other));CHECK(!(other.flags&ready));
    p.sequence=6;p.flags=ready;CHECK(h.push(p,5));CHECK(h.size()==1);
    // Rotation has to preserve wall/ceiling pitch and roll, not just yaw.
    constexpr float root_half=.70710678118f;Quat wall{root_half,0,0,root_half};
    auto axes=basis(wall);CHECK(near(axes[1].z,1));CHECK(near(axes[2].y,-1));
    CHECK(std::abs(dot(wall,from_basis(axes)))>.9999f);
    Quat half=slerp({},Quat{0,1,0,0},.5f);CHECK(near(std::abs(half.y),root_half));CHECK(near(std::abs(half.w),root_half));
    auto same=slerp(wall,{-wall.x,0,0,-wall.w},.5f);CHECK(std::abs(dot(wall,same))>.9999f);
    std::mt19937 gen(722);std::uniform_real_distribution<float> real(-1,1);
    for(unsigned i=0;i<10000;++i){Quat q=normalized({real(gen),real(gen),real(gen),real(gen)});CHECK(std::abs(dot(q,from_basis(basis(q))))>.9999f);}
    // Lost, duplicate and re-ordered packets; chronological presentation stays bounded.
    h.clear();double last_x=-1;p.world=1;p.rotation={};p.flags=ready;p.velocity={10,0,0};
    for(unsigned i=1;i<=300;++i){p.sequence=i;p.position={float(i)/2,0,0};if(i%5)CHECK(h.push(p,i*.05));
        CHECK(h.sample(i*.05-.10,other));CHECK(other.position.x>=last_x-.0001);CHECK(other.position.x<=p.position.x+1);last_x=other.position.x;
        CHECK(!h.push(p,i*.05-.20));}
    // Fuzz the bounded reader with arbitrary binary inputs; no unbounded length allocations.
    for(unsigned i=0;i<10000;++i){Bytes fuzz(gen()%200);for(auto& b:fuzz)b=std::uint8_t(gen());Reader f(fuzz);f.u64();f.text(24);f.pose();}
    std::cout<<checks<<" protocol/math checks passed\n";
}
