#define USM_MULTIPLAYER_MODE_TEST 1
#include "legacy_arena_mode.inc"
#include <cstdlib>
#include <iostream>
using namespace usm::mp;
namespace gp=usm::mp::pad;
static int checks=0;
#define CHECK(x) do{++checks;if(!(x)){std::cerr<<__FILE__<<":"<<__LINE__<<" " #x "\n";std::abort();}}while(0)
namespace mode_test {
std::vector<gp::Sample> devices;
gp::Seats seats;
actor actors[2];bool loaded=false;
int loads=0,releases=0,poses=0,native_up=0,native_down=0,native_confirm=0,native_draw=0;
Display display;bool transition_on_confirm=false;
LanState net=LanState::off;Settings settings;Input sent;
}
namespace usm::mp {
void gamepads_configure(const std::string&){}
std::array<pad::Sample,2> gamepads_poll(bool,double){return mode_test::seats.update(mode_test::devices);}
void gamepads_lock(bool b){mode_test::seats.lock(b);}
void gamepads_shutdown(){mode_test::seats.clear();}
int gamepads_deadzone(){return 9000;}
std::string gamepads_diagnostic(){return "TEST DEVICES (not Windows)";}
bool assets_load(const Settings&,const std::string&,std::string&){mode_test::loaded=true;++mode_test::loads;return true;}
void assets_release(bool){mode_test::loaded=false;++mode_test::releases;}
void assets_retry_release(){}
void assets_pose(const Match&,float){++mode_test::poses;}
std::array<actor*,2> assets_actors(){return mode_test::loaded?std::array<actor*,2>{{&mode_test::actors[0],&mode_test::actors[1]}}:std::array<actor*,2>{};}
const std::string &assets_warning(){static const std::string blank;return blank;}
void render_mode(const Display &d){mode_test::display=d;}
void render_mainmenu_entry(bool){}
bool lan_begin(bool,const std::string&,std::uint16_t,const Settings &s){mode_test::settings=s;mode_test::net=LanState::assets;return true;}
void lan_pump(){}
void lan_close(){mode_test::net=LanState::off;}
LanState lan_state(){return mode_test::net;}
const std::string &lan_error(){static const std::string blank;return blank;}
const Settings &lan_settings(){return mode_test::settings;}
bool lan_is_host(){return true;}
void lan_ready(){mode_test::net=LanState::playing;}
bool lan_exchange(std::uint32_t,Input local,std::uint32_t,std::array<Input,2> &out){mode_test::sent=local;out={local,{}};return true;}
}
static main_menu_options menu;
static PanelAnimFile animation;
static void old_up(main_menu_options *m,int){++mode_test::native_up;m->field_104=short((m->field_104+5)%6);}
static void old_down(main_menu_options *m,int){++mode_test::native_down;m->field_104=short((m->field_104+1)%6);}
static void old_cross(main_menu_options *m,int){++mode_test::native_confirm;if(mode_test::transition_on_confirm)m->field_108=true;}
static void old_draw(main_menu_options*){++mode_test::native_draw;}
static void old_activate(main_menu_options*){}
static void old_update(main_menu_options*,Float){}
static void old_deactivate(main_menu_options*,FEMenu*){}
static gp::Sample sample(const char *id){gp::Sample p;p.connected=true;p.id=id;p.name=id;return p;}
static void reset(int pads=1) {
    mode_test::keys={};mode_test::focused=true;mode_test::quit_dialog=false;mode_test::clock=1000000;
    mode_test::devices.clear();mode_test::seats.clear();
    if(pads>0)mode_test::devices.push_back(sample("PS5"));if(pads>1)mode_test::devices.push_back(sample("generic"));
    controls=Controls{};native_transition_guard=raw_main_action=false;mode_test::transition_on_confirm=false;menu={};animation={};menu.field_E4=&animation;active_menu=&menu;
    original_up=old_up;original_down=old_down;original_cross=old_cross;original_draw=old_draw;
    original_activate=old_activate;original_update=old_update;original_deactivate=old_deactivate;
    installed=true;initialized=true;screen=Screen::closed;seventh=false;controller_menu_owner=false;
    network_match=network_wait=paused=results_open=false;connection_mode=0;input_preset=4;row=overlay_row=0;
    assignment={};selection={};match.reset(selection);request=Request::none;status.clear();log_path.clear();
    frequency.QuadPart=1000000;last_counter.QuadPart=mode_test::clock;accumulator=0;
    mode_test::loaded=false;mode_test::native_confirm=mode_test::native_up=mode_test::native_down=0;
    controls.poll(0.016);
}
static void frame() {mode_test::clock+=16667;multiplayer_mode_tick_before(1.0f/60);multiplayer_mode_tick_after(1.0f/60);}
static void press(std::uint32_t button,unsigned seat=0) {mode_test::devices[seat].buttons=button;frame();}
static void release(unsigned seat=0) {mode_test::devices[seat].buttons=0;mode_test::devices[seat].x=mode_test::devices[seat].y=0;frame();}
static void tap(std::uint32_t button,unsigned seat=0){press(button,seat);release(seat);}
static void menus() {
    reset();frame();CHECK(multiplayer_mode_captures_native_input());
    tap(gp::up);CHECK(seventh&&menu.field_104==0);
    down_hook(&menu,nullptr,0);up_hook(&menu,nullptr,0);cross_hook(&menu,nullptr,0);CHECK(seventh&&screen==Screen::closed); // stock duplicate callback ignored
    tap(gp::down);CHECK(!seventh&&menu.field_104==0);
    for(int n=0;n<6;++n){tap(gp::down);CHECK(menu.field_104>=0&&menu.field_104<=5);}
    CHECK(seventh);press(gp::south);CHECK(screen==Screen::lobby&&mode_test::native_confirm==0);
    frame();CHECK(screen==Screen::lobby&&row==0&&request==Request::none); // held confirm cannot cycle
    release();tap(gp::down);CHECK(row==1);tap(gp::right);CHECK(selection.character[1]==Character::blacksuit);
    tap(gp::east);CHECK(screen==Screen::closed);CHECK(multiplayer_mode_captures_native_input());
    tap(gp::start);CHECK(screen==Screen::lobby);tap(gp::start);CHECK(screen==Screen::battle);
    CHECK(assignment.controller[0]==0&&assignment.controller[1]==-1);CHECK(!paused);
    draw_hook(&menu,nullptr);CHECK(mode_test::display.controllers[0]=="PS5");
    CHECK(mode_test::display.input_preset==4);
    // Native quit modal and transitions must not be stolen by the raw root controller.
    close_mode(Screen::closed);mode_test::quit_dialog=true;frame();CHECK(!multiplayer_mode_captures_native_input());
    press(gp::start);CHECK(screen==Screen::closed);mode_test::quit_dialog=false;release();
    // Selecting a normal native menu entry must not redispatch its held confirm
    // after it switches field_108 to the leaving state.
    seventh=false;menu.field_104=1;mode_test::transition_on_confirm=true;press(gp::south);
    CHECK(mode_test::native_confirm==1&&menu.field_108&&multiplayer_mode_captures_native_input());
    cross_hook(&menu,nullptr,0);frame();CHECK(mode_test::native_confirm==1&&multiplayer_mode_captures_native_input());
    release();CHECK(!multiplayer_mode_captures_native_input());menu.field_108=false;
    activate_hook(&menu,nullptr);CHECK(!seventh&&!controller_menu_owner);update_hook(&menu,nullptr,0.016f);
    draw_hook(&menu,nullptr);CHECK(mode_test::native_draw>0);deactivate_hook(&menu,nullptr,nullptr);CHECK(active_menu==nullptr);
}
static void gameplay_and_overlay() {
    reset(2);tap(gp::start);tap(gp::start);CHECK(screen==Screen::battle&&assignment.required==3);
    for(int n=0;n<125;++n)frame();CHECK(match.phase==Phase::fight);
    const int x1=match.fighters[0].x,x2=match.fighters[1].x;
    mode_test::devices[0].x=20000;mode_test::devices[1].x=-20000;frame();
    CHECK(match.fighters[0].x>x1&&match.fighters[1].x<x2);release(0);release(1);
    press(gp::start);CHECK(paused&&overlay_row==0);release();
    press(gp::south);CHECK(!paused);CHECK(controls.controller(0).held==0); // resume is not jump
    frame();CHECK(match.fighters[0].grounded);release();
    press(gp::south);CHECK(!match.fighters[0].grounded);release();
    tap(gp::start);CHECK(paused);tap(gp::down);CHECK(overlay_row==1);tap(gp::south);
    CHECK(!paused&&match.phase==Phase::countdown);CHECK(match.fighters[0].grounded);
    // A removed P1 may not transfer P2's controller to P1 midmatch.
    mode_test::devices[0].connected=false;frame();CHECK(paused&&missing_controller());
    CHECK(!controls.connected[0]&&controls.connected[1]);
    tap(gp::south,1);CHECK(paused); // P2 cannot bypass the disconnect guard
    mode_test::devices[0].connected=true;frame();tap(gp::south,1);CHECK(!paused);
    mode_test::focused=false;frame();CHECK(paused);mode_test::devices[0].buttons=gp::south;
    mode_test::focused=true;frame();CHECK(paused);release();tap(gp::south);CHECK(!paused);
    match.phase=Phase::match_end;frame();CHECK(results_open&&overlay_row==1);
    tap(gp::up);CHECK(overlay_row==0);tap(gp::south);CHECK(match.phase==Phase::countdown&&!results_open);
    tap(gp::start);tap(gp::down);tap(gp::down);tap(gp::down);CHECK(overlay_row==3);
    tap(gp::south);CHECK(screen==Screen::closed&&!mode_test::loaded);
}
static void presets_network_keyboard() {
    reset(0);screen=Screen::lobby;input_preset=2;request=Request::start;requests();
    CHECK(screen==Screen::lobby&&!status.empty()&&!mode_test::loaded);
    input_preset=4;request=Request::start;requests();CHECK(screen==Screen::battle&&assignment.required==0);
    for(int n=0;n<121;++n)frame();mode_test::keys['D']=true;const int x=match.fighters[0].x;frame();CHECK(match.fighters[0].x>x);
    mode_test::keys={};frame();mode_test::keys[VK_ESCAPE]=true;frame();CHECK(paused);
    mode_test::keys={};frame();mode_test::keys[VK_RETURN]=true;frame();CHECK(!paused);mode_test::keys={};frame();
    reset(1);tap(gp::start);input_preset=2;connection_mode=1;tap(gp::start);
    CHECK(network_match&&screen==Screen::battle&&assignment.required==1); // LAN requires only local pad
    for(int n=0;n<125;++n)frame();mode_test::devices[0].x=20000;frame();CHECK(mode_test::sent.held&Button::right);
    mode_test::focused=false;frame();CHECK(mode_test::sent.held==0&&!paused);
    mode_test::focused=true;release();tap(gp::start);CHECK(screen==Screen::lobby&&!network_match);
    multiplayer_mode_world_shutdown();CHECK(!multiplayer_mode_active()&&active_menu==nullptr);
}
int main(){menus();gameplay_and_overlay();presets_network_keyboard();std::cout<<checks<<" historical arena fixture orchestration checks passed (simulated devices/engine)\n";}
