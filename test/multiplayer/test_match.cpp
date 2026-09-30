#include "multiplayer_match.h"
#include "multiplayer_geometry.h"
#include "multiplayer_protocol.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <random>
#include <vector>
using namespace usm::mp;
static int tests=0;
#define CHECK(x) do{++tests;if(!(x)){std::cerr<<__FILE__<<":"<<__LINE__<<" CHECK failed: " #x "\n";std::abort();}}while(0)
static void start(Match &m){for(int n=0;n<120;++n)m.step({});CHECK(m.phase==Phase::fight);}
static void step(Match &m,int n,std::uint16_t a=0,std::uint16_t b=0){for(int i=0;i<n;++i)m.step({Input{a},Input{b}});}
static void close(Match &m){m.fighters[0].x=-500;m.fighters[1].x=500;}
static void test_rules() {
    Settings s;s.wins_required=0;s.time_limit_seconds=999;s.arena=static_cast<Arena>(90);s.character[0]=static_cast<Character>(99);s.sanitize();
    CHECK(s.wins_required==1&&s.time_limit_seconds==600&&s.arena==Arena::warehouse&&s.character[0]==Character::spiderman);
    Match m;step(m,119);CHECK(m.phase==Phase::countdown&&m.phase_ticks==1);step(m,1);CHECK(m.phase==Phase::fight);
    step(m,1,Button::right);CHECK(m.fighters[0].x==-3095&&m.fighters[1].x==3200);
    auto y=m.fighters[0].y;step(m,1,Button::jump);CHECK(m.fighters[0].y>y&&!m.fighters[0].grounded);
    step(m,90);CHECK(m.fighters[0].y==0&&m.fighters[0].grounded);
    close(m);step(m,1,Button::light);CHECK(m.fighters[1].health==1000);step(m,5);CHECK(m.fighters[1].health==1000);
    step(m,1);CHECK(m.fighters[1].health==920);CHECK(m.fighters[0].meter==110&&m.fighters[1].meter==45);
    step(m,40);CHECK(m.fighters[1].health==920); // no repeated hits from one attack
    Match held;start(held);close(held);step(held,120,Button::light);CHECK(held.fighters[1].health==920); // edge-triggered
    Match blocked;start(blocked);close(blocked);step(blocked,1,Button::light,Button::guard);step(blocked,6,0,Button::guard);
    CHECK(blocked.fighters[1].health==992&&blocked.fighters[1].guard_meter==760); // 80/10, 80*3
    Match both;start(both);close(both);both.fighters[1].character=Character::spiderman;
    step(both,1,Button::heavy,Button::heavy);step(both,11);
    CHECK(both.fighters[0].health==855&&both.fighters[1].health==855);
    CHECK(both.fighters[0].stun==27&&both.fighters[1].stun==27); // captured reciprocal strike
    CHECK(both.fighters[0].x==-1150&&both.fighters[1].x==1150);
    Match special;start(special);step(special,1,Button::special);CHECK(special.fighters[0].action==Action::idle);
    step(special,1);special.fighters[0].meter=350;step(special,1,Button::special);CHECK(special.fighters[0].meter==0&&special.fighters[0].action==Action::special);
    Match dodge;start(dodge);dodge.fighters[0].meter=100;step(dodge,1,Button::dodge);CHECK(dodge.fighters[0].action==Action::dodge&&dodge.fighters[0].invulnerability==12);
    Match result;start(result);result.fighters[1].health=0;step(result,1);CHECK(result.wins[0]==1&&result.phase==Phase::round_end);
    step(result,150);CHECK(result.round==2&&result.phase==Phase::countdown&&result.fighters[1].health==1000);
    start(result);result.fighters[1].health=0;step(result,151);CHECK(result.phase==Phase::match_end&&result.winner==0&&result.wins[0]==2);
    Match tie;start(tie);tie.fighters[0].health=tie.fighters[1].health=0;step(tie,1);CHECK(tie.round_winner==-1&&tie.wins[0]==0&&tie.wins[1]==0);
    Settings timed;timed.time_limit_seconds=1;Match timeout(timed);start(timeout);timeout.fighters[1].health=900;step(timeout,60);CHECK(timeout.phase==Phase::round_end&&timeout.round_winner==0);
    Match platform;start(platform);platform.fighters[0].x=-8000;platform.fighters[0].y=1800;platform.fighters[0].grounded=false;step(platform,40);
    CHECK(platform.fighters[0].y==800&&platform.fighters[0].grounded);
    Settings train;train.arena=Arena::subway;train.hazards=true;Match hazard(train);start(hazard);hazard.fighters[0].z=2400;hazard.fight_ticks=779;step(hazard,1);CHECK(hazard.fighters[0].health==850);
    step(hazard,59);CHECK(hazard.fighters[0].health==850); // invulnerability prevents per-frame damage
}
static void test_geometry() {
    for(auto wh:{std::pair<int,int>{640,480},{1920,1080},{3440,1440},{801,601},{4,4}})for(unsigned m=0;m<4;++m) {
        auto v=make_views(wh.first,wh.second,static_cast<Layout>(m));CHECK(v.count==1||v.count==2);
        for(int n=0;n<v.count;++n){auto r=v.play[n];CHECK(r.x>=0&&r.y>=0&&r.w>0&&r.h>0&&r.x+r.w<=wh.first&&r.y+r.h<=wh.second);}
        if(v.count==2){auto a=v.play[0],b=v.play[1];CHECK(a.x+a.w<=b.x||a.y+a.h<=b.y);}
    }
    auto mat=look_at({2,3,-5},{2,3,0});CHECK(mat.m[0][0]==1&&mat.m[1][1]==1&&mat.m[2][2]==1);
    CHECK(mat.m[3][0]==-2&&mat.m[3][1]==-3&&mat.m[3][2]==5&&mat.m[3][3]==1);
    for(unsigned n=0;n<4;++n){auto g=arena_geometry(static_cast<Arena>(n));CHECK(!g.empty()&&g.size()%3==0&&g.size()<10000);
        for(auto v:g)CHECK(std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z));}
}
static void test_protocol() {
    using namespace usm::mp::wire;
    Packet p;p.kind=Kind::input;p.player=1;p.frame=0x01020304;p.checksum=0xaabbccdd;p.buttons=Button::jump|Button::heavy;
    auto b=encode(p);CHECK(b[0]=='U'&&b[6]==4&&b[7]==1&&b[8]==1&&b[9]==2&&b[10]==3&&b[11]==4);
    CHECK(b[12]==0xaa&&b[15]==0xdd&&b[16]==0&&b[17]==80);Packet q;CHECK(decode(b,q));CHECK(q.frame==p.frame&&q.checksum==p.checksum&&q.buttons==p.buttons);
    for(unsigned n=0;n<4;++n){auto bad=b;bad[n]='?';CHECK(!decode(bad,q));}
    auto bad=b;bad[7]=2;CHECK(!decode(bad,q));bad=b;bad[18]=1;CHECK(!decode(bad,q));bad=b;bad[16]=0x80;CHECK(!decode(bad,q));
    Packet settings;settings.kind=Kind::settings;settings.settings.character[1]=Character::carnage;settings.settings.time_limit_seconds=120;
    auto sb=encode(settings);CHECK(decode(sb,q)&&q.settings.character[1]==Character::carnage&&q.settings.time_limit_seconds==120);
    for(unsigned n:{20u,21u,22u}){auto bad=sb;bad[n]=4;CHECK(!decode(bad,q));}bad=sb;bad[23]=0;CHECK(!decode(bad,q));
    for(std::size_t split=0;split<=packet_size;++split){Decoder d;int count=0;auto consume=[&](const Packet& a){++count;return a.frame==p.frame;};
        CHECK(d.append(b.data(),split,consume));CHECK(d.append(b.data()+split,packet_size-split,consume));CHECK(count==1);}
    std::vector<std::uint8_t> joined;for(int n=0;n<20;++n)joined.insert(joined.end(),b.begin(),b.end());Decoder d;int count=0;
    CHECK(d.append(joined.data(),joined.size(),[&](const Packet&){++count;return true;}));CHECK(count==20);
    Decoder rejected;CHECK(!rejected.append(b.data(),b.size(),[](const Packet&){return false;}));
}
static void test_replay() {
    std::mt19937 rng(0x1234); // Fixed test seed, not a gameplay source.
    for(unsigned arena=0;arena<4;++arena){Settings s;s.arena=static_cast<Arena>(arena);s.hazards=true;Match a(s),b(s);
        for(int n=0;n<50000;++n){std::array<Input,2> in{{{static_cast<std::uint16_t>(rng()&1023)},{static_cast<std::uint16_t>(rng()&1023)}}};
            a.step(in);b.step(in);CHECK(state_hash(a)==state_hash(b));
            if(a.phase==Phase::match_end){a.reset(s);b.reset(s);}
            const auto stage=stage_for(s.arena);for(const auto&f:a.fighters){CHECK(f.health>=0&&f.health<=1000);CHECK(f.meter>=0&&f.meter<=1000);CHECK(f.guard_meter>=0&&f.guard_meter<=1000);
                CHECK(f.x>=-stage.half_width+400&&f.x<=stage.half_width-400);CHECK(f.z>=-stage.half_depth+400&&f.z<=stage.half_depth-400);CHECK(f.y>=0&&f.y<10000);CHECK(f.card_count>=0&&f.card_count<=3);}
        }
    }
}
int main(){test_rules();test_geometry();test_protocol();test_replay();std::cout<<"PASS: "<<tests<<" checks; 200,000 deterministic/fuzz simulation ticks.\n";}
