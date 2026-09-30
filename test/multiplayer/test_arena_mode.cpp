#define USM_MULTIPLAYER_ARENA_TEST 1
#include "../../src/multiplayer_arena_mode.cpp"
#include <cstdlib>
#include <iostream>
#include <limits>

namespace gp=usm::mp::pad;
namespace arena=usm::mp::arena;
using namespace usm::mp;
static unsigned checks=0;
#define CHECK(x) do{++checks;if(!(x)){std::cerr<<__LINE__<<": " #x<<'\n';std::abort();}}while(0)
namespace arena_test {
std::vector<gp::Sample> devices;
gp::Seats seats;
actor fighters[2];
Display display;
bool loaded=false,load_ok=true,exchange_ready=true;
int loads=0,releases=0,poses=0,teardowns=0;
LanState net=LanState::off;
std::string net_error;
Settings settings;
Input sent;
}
namespace usm::mp {
void log(const std::string&){}
void gamepads_configure(const std::string&){}
std::array<pad::Sample,2> gamepads_poll(bool,double){return arena_test::seats.update(arena_test::devices);}
void gamepads_lock(bool value){arena_test::seats.lock(value);}
void gamepads_shutdown(){arena_test::seats.clear();}
int gamepads_deadzone(){return 9000;}
std::string gamepads_diagnostic(){return "Test devices";}
bool assets_load(const Settings&,const std::string&,std::string& error){
    ++arena_test::loads;arena_test::loaded=arena_test::load_ok;
    if(!arena_test::loaded)error="The PC world/resource managers are not ready.";
    return arena_test::loaded;
}
void assets_release(bool teardown){arena_test::loaded=false;++arena_test::releases;if(teardown)++arena_test::teardowns;}
void assets_retry_release(){}
void assets_pose(const Match&,float){++arena_test::poses;}
std::array<actor*,2> assets_actors(){return arena_test::loaded?std::array<actor*,2>{&arena_test::fighters[0],&arena_test::fighters[1]}:std::array<actor*,2>{};}
const std::string& assets_warning(){static const std::string blank;return blank;}
void render_mode(const Display& d){arena_test::display=d;}
void render_mainmenu_entry(bool){}
bool lan_begin(bool,const std::string&,std::uint16_t,const Settings& s){arena_test::settings=s;arena_test::net=LanState::assets;return true;}
void lan_pump(){}
void lan_close(){arena_test::net=LanState::off;arena_test::net_error.clear();}
LanState lan_state(){return arena_test::net;}
const std::string& lan_error(){return arena_test::net_error;}
const Settings& lan_settings(){return arena_test::settings;}
bool lan_is_host(){return true;}
void lan_ready(){arena_test::net=LanState::playing;}
bool lan_exchange(std::uint32_t,Input local,std::uint32_t,std::array<Input,2>& out){
    arena_test::sent=local;out={local,{}};return arena_test::exchange_ready;
}
}
static gp::Sample pad_sample(const char* id){gp::Sample p;p.id=p.name=id;p.connected=true;return p;}
static const Display& view(){arena::draw();return arena_test::display;}
static void frame(double dt=1.0/60){arena::tick(dt);arena::draw();}
static void release(){
    for(auto& p:arena_test::devices){p.buttons=0;p.x=p.y=p.rx=p.ry=0;}
    mode_test::keys={};frame();
}
static void tap(std::uint32_t button,unsigned seat=0){arena_test::devices[seat].buttons=button;frame();release();}
static void key(unsigned k){mode_test::keys[k]=true;frame();release();}
static void select_row(int target){for(int i=0;view().row!=target&&i<12;++i)key(VK_DOWN);CHECK(view().row==target);}
static void reset(unsigned pads=1){
    arena::close();mode_test::keys={};mode_test::focused=true;
    arena_test::devices.clear();arena_test::seats.clear();
    if(pads)arena_test::devices.push_back(pad_sample("PS5"));
    if(pads>1)arena_test::devices.push_back(pad_sample("P2"));
    arena_test::load_ok=arena_test::exchange_ready=true;
    arena::open("test-only.ini");release();
    CHECK(arena::active()&&view().screen==Screen::lobby);
}
static void menus_and_switching(){
    reset();CHECK(view().layout==Layout::ds_panels);
    arena_test::devices[0].buttons=gp::south;arena::open("test-only.ini");frame();
    CHECK(view().selection.character[0]==Character::spiderman); // Entry confirm never changes selection.
    release();tap(gp::right);CHECK(view().selection.character[0]==Character::venom);
    select_row(1);tap(gp::right);CHECK(view().selection.character[1]==Character::blacksuit);
    select_row(2);tap(gp::right);CHECK(view().selection.arena==Arena::subway);
    select_row(3);tap(gp::right);CHECK(view().layout==Layout::shared);
    select_row(5);tap(gp::right);CHECK(view().selection.wins_required==3);
    select_row(6);tap(gp::right);CHECK(view().selection.time_limit_seconds==60);
    select_row(7);tap(gp::right);CHECK(view().selection.hazards);
    select_row(10);tap(gp::south);CHECK(!arena::active()&&!arena_test::loaded);
    CHECK(arena::take_switch_request());CHECK(!arena::take_switch_request());
    reset();select_row(11);key(VK_RETURN);CHECK(!arena::active()&&!arena::take_switch_request());
    reset();tap(gp::guide);CHECK(!arena::active());
    reset();key(0x75);CHECK(!arena::active());
    reset();arena_test::devices[0].buttons=gp::guide;arena::open("test-only.ini");frame();
    CHECK(arena::active());release();tap(gp::guide);CHECK(!arena::active());
}
static void local_controls_and_lifetime(){
    reset(2);tap(gp::start);CHECK(view().screen==Screen::battle&&arena_test::loaded);
    CHECK(view().actors[0]&&view().actors[1]);
    for(int n=0;n<125;++n)frame();CHECK(view().match.phase==Phase::fight);
    const int x1=view().match.fighters[0].x,x2=view().match.fighters[1].x;
    arena_test::devices[0].x=20000;arena_test::devices[1].x=-20000;frame();
    CHECK(view().match.fighters[0].x>x1&&view().match.fighters[1].x<x2);release();
    tap(gp::guide);CHECK(view().paused);const auto stopped=view().match.tick;
    frame();CHECK(view().match.tick==stopped);tap(gp::guide);CHECK(!view().paused);
    key(0x75);CHECK(view().paused);tap(gp::south);CHECK(!view().paused&&view().match.fighters[0].grounded);
    tap(gp::south);CHECK(!view().match.fighters[0].grounded);
    tap(gp::start);CHECK(view().paused);tap(gp::down);tap(gp::south);
    CHECK(!view().paused&&view().match.phase==Phase::countdown&&view().match.fighters[0].grounded);
    arena_test::devices[0].connected=false;frame();CHECK(view().paused);
    tap(gp::south,1);CHECK(view().paused); // P2 cannot steal disconnected P1's seat.
    arena_test::devices[0].connected=true;release();tap(gp::south,1);CHECK(!view().paused);
    mode_test::focused=false;frame();CHECK(view().paused);
    arena_test::devices[0].buttons=gp::south;mode_test::focused=true;frame();CHECK(view().paused);
    release();tap(gp::south);CHECK(!view().paused);
    frame(1.0);CHECK(view().paused);tap(gp::south);CHECK(!view().paused);
    auto tick=view().match.tick;frame(std::numeric_limits<double>::quiet_NaN());CHECK(view().match.tick==tick);
    frame(-1);CHECK(view().match.tick==tick);
    const int shutdowns=arena_test::teardowns;arena::close(true);
    CHECK(!arena::active()&&!arena_test::loaded&&arena_test::teardowns==shutdowns+1);
    reset();tap(gp::start);arena_test::loaded=false;frame();CHECK(view().screen==Screen::error);
}
static void keyboard_errors_and_lan(){
    reset(0);select_row(4);key(VK_LEFT);key(VK_LEFT);CHECK(view().input_preset==2);
    select_row(9);key(VK_RETURN);CHECK(view().screen==Screen::lobby&&!view().status.empty());
    select_row(4);key(VK_RIGHT);key(VK_RIGHT);select_row(9);key(VK_RETURN);
    CHECK(view().screen==Screen::battle);for(int n=0;n<125;++n)frame();
    const auto x=view().match.fighters[0].x;key('D');CHECK(view().match.fighters[0].x>x);
    key(VK_ESCAPE);CHECK(view().paused);key(VK_RETURN);CHECK(!view().paused);
    reset();arena_test::load_ok=false;tap(gp::start);
    CHECK(view().screen==Screen::error&&view().status=="The PC world/resource managers are not ready."&&!arena_test::loaded);
    tap(gp::east);CHECK(view().screen==Screen::lobby);arena_test::load_ok=true;tap(gp::start);CHECK(view().screen==Screen::battle);
    reset();select_row(8);tap(gp::right);CHECK(view().connection_mode==1);tap(gp::start);
    CHECK(view().network_match&&view().screen==Screen::battle);
    for(int n=0;n<125;++n)frame();arena_test::devices[0].x=20000;frame();CHECK(arena_test::sent.held&Button::right);
    mode_test::focused=false;frame();CHECK(!arena_test::sent.held&&!view().paused);
    mode_test::focused=true;release();arena_test::exchange_ready=false;
    const auto tick=view().match.tick;frame();CHECK(view().network_wait&&view().match.tick==tick);
    arena_test::exchange_ready=true;frame();CHECK(view().match.tick>tick);
    tap(gp::guide);CHECK(view().screen==Screen::lobby&&!view().network_match&&!arena_test::loaded);
    select_row(8);tap(gp::right);CHECK(view().connection_mode==2);tap(gp::start);CHECK(view().network_match);
    arena_test::net_error="Peer disconnected.";arena_test::net=LanState::error;frame();
    CHECK(view().screen==Screen::error&&!arena_test::loaded);
    CHECK(view().status=="Peer disconnected."); // Teardown clears the transport-owned string.
    arena::close();CHECK(arena_test::net==LanState::off);
}
int main(){
    menus_and_switching();local_controls_and_lifetime();keyboard_errors_and_lan();
    std::cout<<checks<<" production NDS arena menu/gameplay/lifecycle checks passed (simulated OS/assets/transport)\n";
}
