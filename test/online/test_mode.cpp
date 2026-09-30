#define USM_ONLINE_MODE_TEST 1
#include "../../src/multiplayer_mode.cpp"
#include <cstdlib>
#include <iostream>
using namespace usm::online;
namespace gp=usm::mp::pad;
int debug_enabled=0,debug_disabled=0;
unsigned checks=0;
#define CHECK(x) do{++checks;if(!(x)){std::cerr<<__LINE__<<": " #x<<'\n';std::abort();}}while(0)
namespace online_test {
std::vector<gp::Sample> devices;gp::Seats seats;
bool hero_ready=false;int release_count=0,teardowns=0,updates=0,draws=0,continues=0,original_draws=0;
bool arena_open=false,arena_switch=false,arena_exit=false;
int arena_ticks=0,arena_draws=0,arena_teardowns=0,pad_polls=0;
}
namespace usm::mp {
void gamepads_configure(const std::string&){}
std::array<pad::Sample,2> gamepads_poll(bool,double){++online_test::pad_polls;return online_test::seats.update(online_test::devices);}
void gamepads_lock(bool locked){online_test::seats.lock(locked);}
void gamepads_shutdown(){online_test::seats.clear();}
int gamepads_deadzone(){return 9000;}
std::string gamepads_diagnostic(){return "test";}
}
// The arena boundary is faked here to exercise the real shared dispatcher.
// test/multiplayer/test_arena_mode.cpp executes the production arena itself.
namespace usm::mp::arena {
void open(const std::string&){online_test::arena_open=true;}
void close(bool teardown){online_test::arena_open=false;if(teardown)++online_test::arena_teardowns;}
void tick(double){++online_test::arena_ticks;if(online_test::arena_exit||online_test::arena_switch)online_test::arena_open=false;}
void draw(){++online_test::arena_draws;}
bool active(){return online_test::arena_open;}
bool take_switch_request(){bool value=online_test::arena_switch;online_test::arena_switch=false;return value;}
}
namespace usm::online {
std::vector<LocalAddress> local_ipv4_addresses(){return {{"Radmin test","26.1.2.3"},{"LAN test","192.168.1.1"}};}
void engine_configure(const std::string&){}
bool engine_capture(Pose& p){p={};if(!online_test::hero_ready)return false;p.world=123;p.flags=ready;p.position={1,2,3};return true;}
void engine_update(const Session&,double,float){++online_test::updates;}
void engine_draw_world(const Session&,double){}
void engine_release(bool teardown){++online_test::release_count;if(teardown)++online_test::teardowns;}
const std::string& engine_warning(){static const std::string empty;return empty;}
void draw_text(float,float,const std::string&,float,std::uint32_t){++online_test::draws;}
void draw_rect(float,float,float,float,std::uint32_t){++online_test::draws;}
}
main_menu_options menu;PanelAnimFile animation;
void old_up(main_menu_options* m,int){m->field_104=short((m->field_104+5)%6);}
void old_down(main_menu_options* m,int){m->field_104=short((m->field_104+1)%6);}
void old_cross(main_menu_options* m,int){++online_test::continues;m->field_108=true;}
void old_draw(main_menu_options*){++online_test::original_draws;}
void old_activate(main_menu_options*){}
void old_update(main_menu_options*,Float){}
void old_deactivate(main_menu_options*,FEMenu*){}
void frame(){online_test::clock+=10000;multiplayer_mode_tick_before(.01f);if(active_menu)update_hook(active_menu,nullptr,.01f);multiplayer_mode_tick_after(.01f);}
void release(){for(auto& s:online_test::devices){s.buttons=0;s.x=s.y=s.rx=s.ry=0;}online_test::keys={};frame();}
void tap(std::uint32_t bit){online_test::devices[0].buttons=bit;frame();release();}
void key(unsigned k){online_test::keys[k]=true;frame();online_test::keys[k]=false;frame();}
void reset(){
    session.close();installed=initialized=true;panel=seventh=input_guard=controller_owner=raw_main_action=false;
    arena_type=false;menu_tick=false;online_test::arena_open=online_test::arena_switch=online_test::arena_exit=false;
    auto_continue=was_active=false;task=Task::none;row=0;editing=-1;options={};options.port=0;status.clear();log_path.clear();ini_path="test-only.ini";
    menu={};animation={};menu.field_E4=&animation;active_menu=&menu;
    controls={};online_test::keys={};online_test::hero_ready=false;online_test::focused=true;online_test::quit_dialog=false;online_test::continues=0;
    online_test::clock=1000000;frequency.QuadPart=1000000;last_counter.QuadPart=1000000;now=0;
    gp::Sample p;p.connected=true;p.name="DualSense";p.id="local-test-pad";online_test::devices={p};online_test::seats.clear();
    original_up=old_up;original_down=old_down;original_cross=old_cross;original_draw=old_draw;
    original_activate=old_activate;original_update=old_update;original_deactivate=old_deactivate;frame();
}
int main(){
    reset();online_test::devices[0].y=-20000;online_test::clock+=10000;
    update_hook(&menu,nullptr,.01f);CHECK(menu.field_104==1);
    multiplayer_mode_tick_before(.01f);CHECK(menu.field_104==1);
    release();
    tap(gp::start);tap(gp::select|gp::start);CHECK(!panel);
    tap(gp::guide);CHECK(panel);tap(gp::guide);CHECK(!panel);
    online_test::devices.clear();frame();CHECK(!multiplayer_mode_captures_native_input());
    gp::Sample reconnect;reconnect.connected=true;reconnect.id="reconnected";reconnect.buttons=gp::guide;
    online_test::devices={reconnect};frame();CHECK(!panel);release();tap(gp::guide);CHECK(panel);tap(gp::guide);
    online_test::focused=false;frame();online_test::devices[0].buttons=gp::guide;
    online_test::focused=true;frame();CHECK(!panel);release();tap(gp::guide);CHECK(panel);tap(gp::guide);
    reset();CHECK(!panel&&multiplayer_mode_captures_native_input());tap(gp::up);CHECK(seventh&&menu.field_104==0);
    down_hook(&menu,nullptr,0);up_hook(&menu,nullptr,0);cross_hook(&menu,nullptr,0);CHECK(seventh);tap(gp::down);CHECK(!seventh&&menu.field_104==0);
    for(int i=0;i<6;++i){tap(gp::down);CHECK(menu.field_104>=0&&menu.field_104<=5);}CHECK(seventh);
    tap(gp::south);CHECK(panel&&editing==-1&&online_test::continues==0);
    draw_hook(&menu,nullptr);CHECK(online_test::draws>0);
    CHECK(addresses.size()==2&&address_index==0);tap(gp::r1);CHECK(address_index==1);
    tap(gp::south);CHECK(editing==0);editor.begin("OnlineHero",24,false);tap(gp::south);CHECK(editing==-1&&options.nickname=="OnlineHero");
    CHECK(online_test::ini["Online/Nickname"]=="OnlineHero");
    row=2;tap(gp::south);editor.begin("0",5,true);tap(gp::south);CHECK(editing==2);editor.begin("7777",5,true);tap(gp::south);CHECK(editing==-1&&options.port==7777);
    options.port=0;row=4;tap(gp::south);CHECK(session.is_host()&&session.local_id()==1);CHECK(online_test::continues==1&&!panel);CHECK(menu.field_104==0);
    deactivate_hook(&menu,nullptr,nullptr);online_test::hero_ready=true;release();
    CHECK(session.active()&&!multiplayer_mode_captures_native_input());
    CHECK(multiplayer_mode_owns_native_pad(1));CHECK(!multiplayer_mode_owns_native_pad(0)&&!multiplayer_mode_owns_native_pad(2));
    online_test::devices[0].buttons=gp::south|gp::r1;online_test::devices[0].rx=32767;frame();
    InputState input{};multiplayer_mode_merge_native_input(1,input);CHECK(input.m_jump==255&&input.m_throw_web==255&&input.field_18==32767);
    InputState wrong{};multiplayer_mode_merge_native_input(2,wrong);CHECK(!wrong.m_jump&&!wrong.field_18);
    release();tap(gp::start);CHECK(session.active()&&!panel); // native pause, never leave or create arena
    tap(gp::select|gp::start);CHECK(!panel);
    online_test::devices[0].buttons=gp::guide;frame();CHECK(panel&&multiplayer_mode_captures_native_input());frame();CHECK(panel);release();
    tap(gp::east);CHECK(!panel);key(VK_F6);CHECK(panel);key(VK_F6);CHECK(!panel);
    Session peer;Options o;o.nickname="Friend";o.port=session.bound_port();CHECK(peer.join(o,now));
    for(int i=0;i<500;++i){frame();peer.poll(now);Pose p;p.flags=ready;p.world=123;peer.set_local_pose(p);}
    CHECK(peer.active()&&peer.local_id()!=session.local_id());CHECK(session.players()[1].identity.nickname=="Friend");
    Pose sample;CHECK(peer.sample(0,now,sample));
    const auto hero_change_id=session.local_id();const auto hero_change_port=session.bound_port();
    const int hero_change_releases=online_test::release_count,hero_change_teardowns=online_test::teardowns;
    const int hero_change_arena_teardowns=online_test::arena_teardowns;
    online_test::hero_ready=false;online_test::arena_open=true;multiplayer_mode_hero_pack_unloading();
    CHECK(!usm::mp::arena::active()&&session.active());
    CHECK(session.local_id()==hero_change_id&&session.bound_port()==hero_change_port);
    CHECK(online_test::release_count==hero_change_releases+1&&online_test::teardowns==hero_change_teardowns);
    CHECK(online_test::arena_teardowns==hero_change_arena_teardowns);
    for(int i=0;i<100;++i){frame();peer.poll(now);}CHECK(!peer.sample(0,now,sample));
    online_test::hero_ready=true;for(int i=0;i<100;++i){frame();peer.poll(now);}CHECK(peer.sample(0,now,sample));
    online_test::hero_ready=false;multiplayer_mode_world_shutdown();CHECK(session.active()&&online_test::teardowns>0);
    for(int i=0;i<100;++i){frame();peer.poll(now);}CHECK(!peer.sample(0,now,sample));
    online_test::hero_ready=true;for(int i=0;i<100;++i){frame();peer.poll(now);}CHECK(peer.sample(0,now,sample));
    online_test::focused=false;frame();InputState unfocused{};multiplayer_mode_merge_native_input(1,unfocused);CHECK(!unfocused.m_jump);online_test::focused=true;release();
    key(VK_F6);CHECK(panel);row=7;tap(gp::south);CHECK(!session.active());for(int i=0;i<10;++i)peer.poll(now);CHECK(!peer.active());
    reset();menu.field_10A=true;open_panel();release();row=4;tap(gp::south);CHECK(session.active()&&panel&&online_test::continues==0&&!status.empty());
    session.close();reset();activate_hook(&menu,nullptr);release();update_hook(&menu,nullptr,.01f);draw_hook(&menu,nullptr);CHECK(online_test::original_draws>0);
    // Preserve the online rows while adding a type switch at the end.
    reset();open_panel();release();tap(gp::up);CHECK(row==9);
    tap(gp::south);CHECK(arena_type&&usm::mp::arena::active()&&!panel);
    CHECK(online_test::ini["Multiplayer/Type"]=="1");
    CHECK(multiplayer_mode_active()&&multiplayer_mode_captures_native_input());
    CHECK(!multiplayer_mode_blocks_world()); // Frontend must keep advancing.
    const int polls=online_test::pad_polls,ticks=online_test::arena_ticks;
    frame();CHECK(online_test::arena_ticks==ticks+1&&online_test::pad_polls==polls);
    const int arena_draws=online_test::arena_draws;
    draw_hook(&menu,nullptr);multiplayer_mode_draw_overlay();CHECK(online_test::arena_draws==arena_draws+1);
    CHECK(!multiplayer_mode_owns_native_pad(1));
    online_test::arena_switch=true;frame();CHECK(!arena_type&&!usm::mp::arena::active()&&panel);
    CHECK(online_test::ini["Multiplayer/Type"]=="0");
    // Switching away from a live online session closes its sockets/proxies.
    row=4;tap(gp::south);CHECK(session.active());
    deactivate_hook(&menu,nullptr,nullptr);online_test::hero_ready=true;release();key(VK_F6);
    const int released=online_test::release_count;row=9;tap(gp::south);
    CHECK(usm::mp::arena::active()&&!session.active()&&online_test::release_count>released);
    CHECK(multiplayer_mode_blocks_world());
    const int in_world_draws=online_test::arena_draws;multiplayer_mode_draw_overlay();CHECK(online_test::arena_draws==in_world_draws+1);
    online_test::arena_exit=true;frame();CHECK(!usm::mp::arena::active()&&arena_type&&!panel);
    CHECK(!multiplayer_mode_blocks_world());
    online_test::arena_exit=false;release();key(VK_F6);CHECK(usm::mp::arena::active());
    multiplayer_mode_world_shutdown();CHECK(!usm::mp::arena::active()&&online_test::arena_teardowns>0);
    std::cout<<checks<<" production online-menu/input/lifecycle checks passed (fake engine/OS; real transport)\n";
}
