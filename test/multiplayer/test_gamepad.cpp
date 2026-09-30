#include "multiplayer_gamepad.h"
#include <cstdlib>
#include <iostream>
using namespace usm::mp;
namespace gp=usm::mp::pad;
static int checks=0;
#define CHECK(x) do{++checks;if(!(x)){std::cerr<<__FILE__<<":"<<__LINE__<<" " #x "\n";std::abort();}}while(0)
static gp::Sample device(const char *id) {gp::Sample s;s.id=id;s.name=id;s.connected=true;return s;}
static void mappings() {
    CHECK(gp::sony_id(0x054c,0x05c4));CHECK(gp::sony_id(0x054c,0x09cc));CHECK(gp::sony_id(0x054c,0x0ce6));
    CHECK(gp::sony_id(0x054c,0x0df2));CHECK(!gp::sony_id(0x045e,0x05c4));CHECK(!gp::sony_id(0x054c,0xffff));
    const auto ps=gp::default_mapping(true),generic=gp::default_mapping(false);
    constexpr std::uint16_t expected[]{Button::jump,Button::special,Button::light,Button::heavy,Button::guard,Button::dodge};
    constexpr unsigned psbuttons[]{1,2,0,3,4,5},genericbuttons[]{0,1,2,3,4,5};
    constexpr std::uint16_t xbuttons[]{0x1000,0x2000,0x4000,0x8000,0x0100,0x0200};
    for(unsigned n=0;n<6;++n) {
        gp::DirectSample raw;raw.button[psbuttons[n]]=0x80;
        CHECK(gp::match_input(gp::map_direct(raw,ps).buttons).held==expected[n]);
        raw={};raw.button[genericbuttons[n]]=0x80;
        CHECK(gp::match_input(gp::map_direct(raw,generic).buttons).held==expected[n]);
        CHECK(gp::match_input(gp::map_xinput(xbuttons[n],0,0).buttons).held==expected[n]);
    }
    gp::DirectSample s;s.button[9]=0x80;CHECK(gp::map_direct(s,ps).buttons==gp::start);
    s={};s.button[12]=0x80;CHECK(gp::map_direct(s,ps).buttons==gp::guide);
    CHECK(gp::map_direct(s,generic).buttons==0);
    auto guide_mapping=generic;guide_mapping.guide_button=12;
    CHECK(gp::map_direct(s,guide_mapping).buttons==gp::guide);
    guide_mapping.guide_button=128;CHECK(gp::map_direct(s,guide_mapping).buttons==0);
    s={};s.button[7]=0x80;CHECK(gp::map_direct(s,generic).buttons==gp::start);
    s={};s.axis[0]=-32767;s.axis[1]=-32767;auto p=gp::map_direct(s,ps);CHECK(p.x==-32767&&p.y==32767);
    auto custom=generic;custom.axis_x=7;custom.axis_y=-1;custom.invert_x=true;custom.button[0]=127;
    custom.pov=-1;custom.dpad_button[0]=30;s={};s.axis[7]=12000;s.button[127]=0x80;s.button[30]=0x80;
    p=gp::map_direct(s,custom);CHECK(p.x==-12000&&p.y==0&&p.buttons==(gp::south|gp::up));
    custom.button[0]=128;custom.dpad_button[0]=-1;CHECK(gp::map_direct(s,custom).buttons==0);
    CHECK(gp::match_input(gp::start|gp::select|gp::l3|gp::r3|gp::l2|gp::r2).held==0);
    CHECK(gp::match_input(gp::left|gp::right|gp::up|gp::down).held==0);
    CHECK(gp::map_xinput(0xffff,-32768,32767).x==-32767);
    CHECK(gp::normalize_axis(0,0,65535)==-32767);CHECK(gp::normalize_axis(65535,0,65535)==32767);
    CHECK(gp::normalize_axis(32767,0,65535)==0 || gp::normalize_axis(32767,0,65535)==-1);
    CHECK(gp::normalize_axis(0,-32767,32767)==0);CHECK(gp::normalize_axis(5,5,5)==0);
    CHECK(gp::normalize_axis(-999,0,255)==-32767);CHECK(gp::normalize_axis(999,0,255)==32767);
    CHECK(gp::normalize_axis(2147483647,-2147483648LL,2147483647)==32767);
    constexpr std::uint32_t directions[]{gp::up,gp::up|gp::right,gp::right,gp::right|gp::down,gp::down,gp::down|gp::left,gp::left,gp::left|gp::up};
    for(unsigned n=0;n<8;++n)CHECK(gp::pov_buttons(n*4500)==directions[n]);
    CHECK(gp::pov_buttons(35999)==gp::up);CHECK(gp::pov_buttons(65535)==0);CHECK(gp::pov_buttons(0xffffffff)==0);CHECK(gp::pov_buttons(36000)==0);
}
static void transitions() {
    gp::Tracker t;auto s=device("PS5");s.buttons=gp::south;t.poll(s,true,0.016);
    CHECK(t.held==0&&t.pressed==0); // connected while pressed: require release
    s.buttons=0;t.poll(s,true,0.016);s.buttons=gp::south;t.poll(s,true,0.016);
    CHECK(t.pressed==gp::south&&t.held==gp::south);
    t.poll(s,true,0.016);CHECK(t.pressed==0&&t.held==gp::south);
    t.gate();t.poll(s,true,0.016);CHECK(t.held==0); // confirm cannot become a jump
    s.buttons=0;t.poll(s,true,0.016);s.buttons=gp::south;t.poll(s,true,0.016);CHECK(t.held==gp::south);
    t.poll(s,false,0.016);CHECK(t.held==0);t.poll(s,true,0.016);CHECK(t.held==0);
    s.buttons=0;t.poll(s,true,0.016);s.buttons=gp::west;t.poll(s,true,0.016);CHECK(t.pressed==gp::west);
    s.connected=false;t.poll(s,true,0.016);CHECK(t.held==0);
    s.connected=true;t.poll(s,true,0.016);CHECK(t.held==0); // reconnect same held button
    s.buttons=0;t.poll(s,true,0.016);s.buttons=gp::west;t.poll(s,true,0.016);CHECK(t.held==gp::west);
    s.id="DS4";t.poll(s,true,0.016);CHECK(t.held==0); // replacement must not inherit input
    s.buttons=0;t.poll(s,true,0.016);s.x=9500;t.poll(s,true,0.016);CHECK(t.navigation==gp::right);
    s.x=7000;t.poll(s,true,0.016);CHECK(t.held==gp::right&&t.navigation==0); // hysteresis
    s.x=5900;t.poll(s,true,0.016);CHECK(t.held==0);
    s.x=20000;t.poll(s,true,0.01);CHECK(t.navigation==gp::right);
    for(int n=0;n<3;++n){t.poll(s,true,0.1);CHECK(t.navigation==0);}
    t.poll(s,true,0.1);CHECK(t.navigation==gp::right); // initial repeat delay
    t.poll(s,true,0.11);CHECK(t.navigation==gp::right);
    t.gate();t.poll(s,true,0.1);CHECK(t.held==0&&t.navigation==0);
    s.x=0;t.poll(s,true,0.016);s.x=-20000;t.poll(s,true,0.016);CHECK(t.held==gp::left);
}
static void seats_and_presets() {
    gp::Seats seats;auto a=device("XI:0"),b=device("DI:PS5"),c=device("DI:generic");
    auto s=seats.update({a,b});CHECK(s[0].id==a.id&&s[1].id==b.id);
    seats.lock(true);s=seats.update({b,c});CHECK(!s[0].connected&&s[0].id==a.id&&s[1].id==b.id);
    s=seats.update({c,b,a});CHECK(s[0].id==a.id&&s[1].id==b.id); // order independent
    seats.lock(false);s=seats.update({b});CHECK(s[0].id==b.id&&!s[1].connected); // lobby compacts
    seats.clear();s=seats.update({a,a});CHECK(s[0].id==a.id&&!s[1].connected); // no duplicate seat
    auto k=gp::assign(0,2,false);CHECK(k.required==0);
    auto mixed=gp::assign(1,1,false);CHECK(mixed.controller[0]==-1&&mixed.controller[1]==0);
    auto two=gp::assign(2,2,false);CHECK(two.controller[0]==0&&two.controller[1]==1&&two.required==3);
    auto single=gp::assign(3,1,false);CHECK(single.controller[0]==0&&single.controller[1]==-1);
    CHECK(gp::assign(4,0,false).required==0);CHECK(gp::assign(4,1,false).controller[0]==0);CHECK(gp::assign(4,2,false).required==3);
    CHECK(gp::assign(2,1,true).required==1);CHECK(gp::assign(3,1,true).lan_pad);CHECK(!gp::assign(1,1,true).lan_pad);
    CHECK(gp::assign(4,1,true).lan_pad);CHECK(!gp::assign(4,0,true).lan_pad);
    CHECK(!gp::missing(two,{{true,true}}));CHECK(gp::missing(two,{{true,false}}));CHECK(gp::missing(two,{{false,true}}));
    CHECK(!gp::missing(single,{{true,false}}));CHECK(!gp::missing(k,{{false,false}}));
}
static void menus_and_match() {
    gp::MenuPosition p;
    for(int n=1;n<=7;++n){p=gp::menu_step(p,1);CHECK(p.native>=0&&p.native<=5);CHECK((p.extra?6:p.native)==n%7);}
    p=gp::menu_step(p,-1);CHECK(p.extra);p=gp::menu_step(p,-1);CHECK(p.native==5&&!p.extra);
    CHECK(gp::overlay_count(false,false)==4);CHECK(gp::overlay_count(true,false)==3);CHECK(gp::overlay_count(true,true)==2);
    CHECK(gp::overlay_action(0,false,false)==gp::OverlayAction::resume);
    CHECK(gp::overlay_action(0,true,false)==gp::OverlayAction::rematch);
    CHECK(gp::overlay_action(1,true,false)==gp::OverlayAction::lobby);
    CHECK(gp::overlay_action(0,true,true)==gp::OverlayAction::lobby);
    CHECK(gp::overlay_action(1,true,true)==gp::OverlayAction::close);
    Match m;for(int n=0;n<120;++n)m.step({});CHECK(m.phase==Phase::fight);
    auto p1=device("ps4"),p2=device("generic");gp::Tracker t1,t2;t1.poll(p1,true,0.016);t2.poll(p2,true,0.016);
    p1.x=20000;p2.x=-20000;t1.poll(p1,true,0.016);t2.poll(p2,true,0.016);
    const int x1=m.fighters[0].x,x2=m.fighters[1].x;m.step({gp::match_input(t1.held),gp::match_input(t2.held)});
    CHECK(m.fighters[0].x>x1&&m.fighters[1].x<x2);
    m.fighters[0].x=-500;m.fighters[1].x=500;
    gp::DirectSample ps;ps.button[0]=0x80;auto light=gp::match_input(gp::map_direct(ps,gp::default_mapping(true)).buttons);
    m.step({light,{}});for(int n=0;n<6;++n)m.step({});CHECK(m.fighters[1].health==920);
}
int main(){mappings();transitions();seats_and_presets();menus_and_match();std::cout<<checks<<" gamepad checks passed\n";}
