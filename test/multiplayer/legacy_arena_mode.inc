#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "multiplayer_mode.h"
#if (defined(_WIN32) && !defined(OPENUSM_XBPACK_MODE)) || defined(USM_MULTIPLAYER_MODE_TEST)
#include "multiplayer_assets.h"
#include "multiplayer_lan.h"
#include "multiplayer_render.h"
#include "multiplayer_gamepad_win.h"
#if defined(USM_MULTIPLAYER_MODE_TEST)
// The host test supplies devices, clock, engine callbacks and resources, not a
// Windows ABI. All menu/input/match orchestration below is production code.
#include "multiplayer_mode_test_adapter.h"
#else
#include "panelanimfile.h"
#include "variables.h"
#include "main_menu_options.h"
#include "femenu.h"
#include "ngl.h"
#include <windows.h>
#endif
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <limits>
#include <string>

namespace usm::mp {
namespace {
bool native_transition_guard=false;
struct Controls {
    std::array<bool,256> key{},previous{},raw_key{},blocked_key{};
    std::array<pad::Sample,2> sample{};
    std::array<pad::Tracker,2> tracker{};
    std::array<bool,2> connected{};
    bool focused=false,was_focused=false;
    bool pressed(int k)const{return key[k]&&!previous[k];}
    bool pad_pressed(std::uint32_t bit)const {return ((tracker[0].pressed|tracker[1].pressed)&bit)!=0;}
    bool navigate(std::uint32_t bit)const {return ((tracker[0].navigation|tracker[1].navigation)&bit)!=0;}
    void gate() {
        native_transition_guard=true;
        for(unsigned n=0;n<256;++n){blocked_key[n]=blocked_key[n]||raw_key[n];key[n]=previous[n]=false;}
        for(auto &t:tracker)t.gate();
    }
    void poll(double elapsed) {
        previous=key;was_focused=focused;DWORD pid=0;
        GetWindowThreadProcessId(GetForegroundWindow(),&pid);focused=pid==GetCurrentProcessId();
        for(unsigned n=0;n<256;++n) {
            raw_key[n]=focused&&(GetAsyncKeyState(n)&0x8000)!=0;
            if(focused&&!was_focused)blocked_key[n]=raw_key[n];
            blocked_key[n]=blocked_key[n]&&raw_key[n];key[n]=raw_key[n]&&!blocked_key[n];
        }
        const auto old=sample;sample=gamepads_poll(focused,elapsed);
        for(unsigned n=0;n<2;++n) {
            connected[n]=sample[n].connected;tracker[n].deadzone=gamepads_deadzone();tracker[n].poll(sample[n],focused,elapsed);
            if(sample[n].id!=old[n].id||sample[n].connected!=old[n].connected)
                log("Controller "+std::to_string(n+1)+": "+(connected[n]?sample[n].name:"disconnected")+" "+sample[n].id);
        }
        if(native_transition_guard) {
            bool held=std::any_of(raw_key.begin(),raw_key.end(),[](bool down){return down;});
            for(const auto &s:sample)if(s.connected&&(s.buttons||std::abs(s.x)>gamepads_deadzone()||std::abs(s.y)>gamepads_deadzone()))held=true;
            if(!held)native_transition_guard=false;
        }
    }
    bool up()const{return pressed(VK_UP)||navigate(pad::up);}
    bool down()const{return pressed(VK_DOWN)||navigate(pad::down);}
    bool left()const{return pressed(VK_LEFT)||navigate(pad::left);}
    bool right()const{return pressed(VK_RIGHT)||navigate(pad::right);}
    bool confirm()const{return pressed(VK_RETURN)||pad_pressed(pad::south);}
    bool back()const{return pressed(VK_ESCAPE)||pad_pressed(pad::east);}
    bool pause()const{return pressed(VK_ESCAPE)||pad_pressed(pad::start);}
    Input keyboard(int player)const {
        static const int keys[2][10]{{'A','D','W','S',VK_SPACE,'F','G','H','R','T'},
                                     {VK_LEFT,VK_RIGHT,VK_UP,VK_DOWN,VK_NUMPAD0,VK_NUMPAD1,VK_NUMPAD2,VK_NUMPAD3,VK_NUMPAD4,VK_NUMPAD5}};
        Input r{};for(int n=0;n<10;++n)if(key[keys[player][n]])r.held|=static_cast<std::uint16_t>(1u<<n);return r;
    }
    Input controller(int player)const {
        return focused&&connected[player]?pad::match_input(tracker[player].held):Input{};
    }
};
Controls controls;
bool installed=false,initialized=false,seventh=false,paused=false,network_match=false,network_wait=false;
main_menu_options *active_menu=nullptr;
Screen screen=Screen::closed;
Settings selection{};
Match match;
Layout layout=Layout::ds_panels;
int row=0,input_preset=4,connection_mode=0,overlay_row=0;
bool controller_menu_owner=false,raw_main_action=false,results_open=false;
pad::Assignment assignment{};
void main_up(main_menu_options *self,int player);
void main_down(main_menu_options *self,int player);
void main_confirm(main_menu_options *self,int player);
bool root_ready() {
    return active_menu&&!active_menu->field_108&&!active_menu->field_109&&!byte_922994();
}
std::uint16_t port=7777;
std::string ini_path,log_path,host_ip="127.0.0.1",status;
double accumulator=0;
float animation_elapsed=0;
LARGE_INTEGER frequency{},last_counter{};
enum class Request { none,close,lobby,start,rematch };
Request request=Request::none;
using Fn0=void(__thiscall*)(main_menu_options*);
using FnButton=void(__thiscall*)(main_menu_options*,int);
using FnUpdate=void(__thiscall*)(main_menu_options*,Float);
using FnDeactivate=void(__thiscall*)(main_menu_options*,FEMenu*);
Fn0 original_draw=nullptr,original_activate=nullptr;
FnUpdate original_update=nullptr;
FnButton original_up=nullptr,original_down=nullptr,original_cross=nullptr;
FnDeactivate original_deactivate=nullptr;

void reset_clock(){QueryPerformanceCounter(&last_counter);accumulator=0;}
double elapsed_time() {
    LARGE_INTEGER now{};QueryPerformanceCounter(&now);
    double dt=frequency.QuadPart>0&&last_counter.QuadPart?double(now.QuadPart-last_counter.QuadPart)/double(frequency.QuadPart):0;
    last_counter=now;return std::isfinite(dt)&&dt>0?dt:0;
}
void initialize() {
    if(initialized)return;initialized=true;
    char exe[MAX_PATH]{};DWORD n=GetModuleFileNameA(nullptr,exe,MAX_PATH);
    std::string dir=n>0&&n<MAX_PATH?std::string(exe,n):std::string{};
    auto slash=dir.find_last_of("\\/");dir=slash==std::string::npos?".\\":dir.substr(0,slash+1);
    ini_path=dir+"multiplayer.ini";log_path=dir+"multiplayer.log";
    layout=static_cast<Layout>(std::clamp(int(GetPrivateProfileIntA("Multiplayer","View",3,ini_path.c_str())),0,3));
    input_preset=std::clamp(int(GetPrivateProfileIntA("Multiplayer","InputPreset",4,ini_path.c_str())),0,4);
    selection.wins_required=std::clamp(int(GetPrivateProfileIntA("Multiplayer","WinsRequired",2,ini_path.c_str())),1,5);
    char value[128]{};GetPrivateProfileStringA("Multiplayer","HostIP","127.0.0.1",value,sizeof(value),ini_path.c_str());host_ip=value;
    port=static_cast<std::uint16_t>(std::clamp(int(GetPrivateProfileIntA("Multiplayer","Port",7777,ini_path.c_str())),1,65535));
    gamepads_configure(ini_path);log(gamepads_diagnostic());
    QueryPerformanceFrequency(&frequency);reset_clock();log("Retail PC multiplayer initialized; isolated two-fighter state, 60 Hz rules.");
}
void close_mode(Screen next) {
    lan_close();assets_release();screen=next;paused=false;network_match=false;network_wait=false;
    status.clear();request=Request::none;animation_elapsed=0;overlay_row=0;results_open=false;gamepads_lock(false);controls.gate();reset_clock();
}
void error_screen(const std::string &why) {
    std::string message=why;lan_close();assets_release();status=message;screen=Screen::error;paused=false;network_wait=false;request=Request::none;log(message);gamepads_lock(false);controls.gate();reset_clock();
}
void open_lobby() {
    if(!root_ready())return;
    gamepads_lock(false);controls.gate();
    screen=Screen::lobby;row=0;status.clear();request=Request::none;paused=false;reset_clock();log("Opened Multiplayer Mode from main menu.");
}
void change_row(int direction) {
    auto cycle=[direction](int v,int count){return (v+direction+count)%count;};
    switch(row) {
    case 0:case 1:selection.character[row]=static_cast<Character>(cycle(static_cast<int>(selection.character[row]),4));break;
    case 2:selection.arena=static_cast<Arena>(cycle(static_cast<int>(selection.arena),4));break;
    case 3:layout=static_cast<Layout>(cycle(static_cast<int>(layout),4));break;
    case 4:input_preset=cycle(input_preset,5);break;
    case 5:selection.wins_required=cycle(selection.wins_required-1,5)+1;break;
    case 6:{constexpr int times[]{0,60,90,120};int i=0;for(int n=0;n<4;++n)if(selection.time_limit_seconds==times[n])i=n;selection.time_limit_seconds=times[cycle(i,4)];break;}
    case 7:selection.hazards=!selection.hazards;break;
    case 8:connection_mode=cycle(connection_mode,3);break;
    default:break;
    }
}
void begin_match() {
    assignment=pad::assign(input_preset,int(controls.connected[0])+int(controls.connected[1]),connection_mode!=0);
    if(pad::missing(assignment,controls.connected)) {
        status="Connect the required controller(s), or select another INPUT preset.";controls.gate();return;
    }
    gamepads_lock(true);controls.gate();overlay_row=0;results_open=false;
    selection.sanitize();match.reset(selection);screen=Screen::loading;status="Loading independently owned native PC fighter resources...";paused=false;network_wait=false;animation_elapsed=0;
    if(connection_mode!=0) {
        network_match=true;
        if(!lan_begin(connection_mode==1,host_ip,port,selection)){error_screen(lan_error());return;}
        status=connection_mode==1?"Waiting for player 2 on private LAN port "+std::to_string(port)+". Circle / B / Escape cancels.":"Connecting to "+host_ip+":"+std::to_string(port)+"...";
        reset_clock();return;
    }
    network_match=false;std::string why;
    if(!assets_load(selection,ini_path,why)){error_screen(why);return;}
    match.reset(selection);screen=Screen::battle;assets_pose(match,0);reset_clock();log("Started local versus match.");
}
void requests() {
    auto pending=request;request=Request::none;
    if(pending==Request::close)close_mode(Screen::closed);
    else if(pending==Request::lobby)close_mode(Screen::lobby);
    else if(pending==Request::start)begin_match();
    else if(pending==Request::rematch) {
        if(network_match){close_mode(Screen::lobby);return;}
        if(pad::missing(assignment,controls.connected)){status="Reconnect the assigned controller(s), or return to the lobby.";return;}
        match.reset(selection);paused=false;overlay_row=0;results_open=false;status.clear();controls.gate();reset_clock();assets_pose(match,0);log("Local rematch.");
    }
}
bool missing_controller() {return pad::missing(assignment,controls.connected);}
std::array<Input,2> local_inputs() {
    std::array<Input,2> out;
    for(unsigned n=0;n<2;++n)out[n]=assignment.controller[n]>=0?controls.controller(assignment.controller[n]):controls.keyboard(n);
    return out;
}
void pause_local(const std::string &reason) {
    if(!paused){paused=true;overlay_row=0;controls.gate();reset_clock();}
    status=reason;
}
void resume_local() {
    if(!controls.focused||missing_controller())return;
    paused=false;status.clear();controls.gate();reset_clock();
}
void overlay_input(bool finished) {
    const int count=pad::overlay_count(finished,network_match);
    if(controls.up())overlay_row=(overlay_row+count-1)%count;
    else if(controls.down())overlay_row=(overlay_row+1)%count;
    if(controls.pressed('Q')){request=Request::close;return;}
    if(controls.pressed('L')){request=Request::lobby;return;}
    if(controls.pressed('R')){request=Request::rematch;return;}
    if(controls.back()||controls.pad_pressed(pad::start)) {
        if(finished)request=Request::lobby;else resume_local();return;
    }
    if(!controls.confirm())return;
    switch(pad::overlay_action(overlay_row,finished,network_match)) {
    case pad::OverlayAction::resume:resume_local();break;
    case pad::OverlayAction::rematch:request=Request::rematch;break;
    case pad::OverlayAction::lobby:request=Request::lobby;break;
    case pad::OverlayAction::close:request=Request::close;break;
    }
}
void update_ui() {
    raw_main_action=false;
    if(screen==Screen::closed) {
        if(!root_ready())return;
        if(controls.connected[0]||controls.connected[1])controller_menu_owner=true;
        bool click=false;
        if(controls.pressed(VK_LBUTTON)) {
            HWND window=GetForegroundWindow();POINT p{};RECT client{};
            if(GetCursorPos(&p)&&ScreenToClient(window,&p)&&GetClientRect(window,&client)&&client.right>0&&client.bottom>0) {
                int w=nglGetScreenWidth(),h=nglGetScreenHeight();p.x=p.x*w/client.right;p.y=p.y*h/client.bottom;const auto r=menu_entry_rect(w,h);
                click=p.x>=r.x&&p.x<r.x+r.w&&p.y>=r.y&&p.y<r.y+r.h;
            }
        }
        if(controls.pressed('M')||click||controls.pad_pressed(pad::start)){open_lobby();return;}
        // Own the root navigation while a pad is in use; the stock callbacks are
        // suppressed below so native input cannot step a second time this frame.
        if(controller_menu_owner) {
            if(controls.up()){raw_main_action=true;main_up(active_menu,0);}
            else if(controls.down()){raw_main_action=true;main_down(active_menu,0);}
            else if(controls.confirm()){raw_main_action=true;main_confirm(active_menu,0);}
        }
        return;
    }
    if(screen==Screen::lobby) {
        if(controls.back()){request=Request::close;return;}
        if(controls.up())row=(row+10)%11;else if(controls.down())row=(row+1)%11;
        if(controls.left())change_row(-1);else if(controls.right())change_row(1);
        if(controls.pad_pressed(pad::start)){request=Request::start;return;}
        if(controls.confirm()){if(row==9)request=Request::start;else if(row==10)request=Request::close;else change_row(1);}
        return;
    }
    if(screen==Screen::loading||screen==Screen::error){if(controls.back()||(screen==Screen::error&&controls.confirm()))request=Request::lobby;return;}
    if(screen!=Screen::battle)return;
    if(match.phase==Phase::match_end) {
        if(!results_open){results_open=true;overlay_row=network_match?0:1;controls.gate();return;}
        overlay_input(true);return;
    }
    if(network_match) {
        // No independent LAN pause: focus loss/disconnection supplies neutral
        // input. Start/Escape intentionally leaves the current session.
        if(controls.pause())request=Request::lobby;return;
    }
    if(!controls.focused)pause_local("Window lost focus. Release buttons, then resume.");
    else if(missing_controller())pause_local("Controller disconnected. Reconnect it, or return to the lobby.");
    if(paused)overlay_input(false);
    else if(controls.pause())pause_local("");
}

void __fastcall draw_hook(main_menu_options *self,void*) {
    active_menu=self;
    if(screen==Screen::closed){original_draw(self);render_mainmenu_entry(seventh);return;}
    Display display;display.screen=screen;display.match=match;display.selection=selection;display.layout=layout;
    display.row=row;display.overlay_row=overlay_row;display.input_preset=input_preset;display.connection_mode=connection_mode;display.paused=paused;
    display.network_wait=network_wait;display.network_match=network_match;display.status=status;display.warning=assets_warning();
    for(unsigned n=0;n<2;++n)display.controllers[n]=controls.connected[n]?controls.sample[n].name:"not connected";
    display.input_status=gamepads_diagnostic();display.host_ip=host_ip;display.port=port;display.actors=assets_actors();render_mode(display);
}
void __fastcall update_hook(main_menu_options *self,void*,Float dt) {
    active_menu=self;
    if(screen==Screen::closed)original_update(self,dt);
    // Raw UI/input and every resource operation are driven by the safe tick seam.
}
void __fastcall activate_hook(main_menu_options *self,void*) {active_menu=self;seventh=false;controller_menu_owner=false;controls.gate();original_activate(self);}
void __fastcall deactivate_hook(main_menu_options *self,void*,FEMenu *next) {
    active_menu=nullptr;seventh=false;controller_menu_owner=false;if(screen!=Screen::closed)request=Request::close;original_deactivate(self,next);
}
void main_up(main_menu_options *self,int pad) {
    if(screen!=Screen::closed)return;
    if(self->field_E4&&self->field_E4->field_2D)return;
    if(!self->field_108&&!self->field_109) {
        if(seventh){seventh=false;self->field_106=self->field_104;self->field_104=5;self->update_highlight();return;}
        if(usm::mp::pad::menu_step({self->field_104,false},-1).extra){seventh=true;return;}
    }
    original_up(self,pad);
}
void main_down(main_menu_options *self,int pad) {
    if(screen!=Screen::closed)return;
    if(self->field_E4&&self->field_E4->field_2D)return;
    if(!self->field_108&&!self->field_109) {
        if(seventh){seventh=false;self->field_104=5;original_down(self,pad);return;}
        if(usm::mp::pad::menu_step({self->field_104,false},1).extra){seventh=true;return;}
    }
    original_down(self,pad);
}
void main_confirm(main_menu_options *self,int pad) {
    active_menu=self;if(screen!=Screen::closed)return;
    if(seventh&&!self->field_108&&!self->field_109){open_lobby();return;}
    controls.gate();original_cross(self,pad);
}
void __fastcall up_hook(main_menu_options *self,void*,int player) {
    if(native_transition_guard||raw_main_action||(controller_menu_owner&&root_ready()))return;main_up(self,player);
}
void __fastcall down_hook(main_menu_options *self,void*,int player) {
    if(native_transition_guard||raw_main_action||(controller_menu_owner&&root_ready()))return;main_down(self,player);
}
void __fastcall cross_hook(main_menu_options *self,void*,int player) {
    if(native_transition_guard||raw_main_action||(controller_menu_owner&&root_ready()&&!controls.pressed(VK_LBUTTON)))return;
    main_confirm(self,player);
}
} // namespace

void log(const std::string &message) {
    std::string line="[Multiplayer] "+message+"\n";OutputDebugStringA(line.c_str());
    if(log_path.empty())return;
    std::ofstream f(log_path,std::ios::app);if(f)f<<line;
}
} // namespace usm::mp

bool multiplayer_mode_install() {
    using namespace usm::mp;if(installed)return true;
#if defined(USM_MULTIPLAYER_MODE_TEST)
    return false; // Tests explicitly initialize callbacks; never patch host memory.
#else
    static_assert(sizeof(void*)==4,"Multiplayer native hooks require a 32-bit PC build");
    static_assert(sizeof(main_menu_options)==0x110,"Retail main-menu layout differs");
    struct Patch{std::uintptr_t location,expected,replacement;};
    const Patch patches[]{
        {0x00894710,0x006139C0,reinterpret_cast<std::uintptr_t>(&draw_hook)},
        {0x00894718,0x00637B90,reinterpret_cast<std::uintptr_t>(&update_hook)},
        {0x00894724,0x0062CE60,reinterpret_cast<std::uintptr_t>(&activate_hook)},
        {0x00894728,0x006139B0,reinterpret_cast<std::uintptr_t>(&deactivate_hook)},
        {0x00894734,0x00623500,reinterpret_cast<std::uintptr_t>(&up_hook)},
        {0x00894738,0x006235E0,reinterpret_cast<std::uintptr_t>(&down_hook)},
        {0x00894744,0x006236C0,reinterpret_cast<std::uintptr_t>(&cross_hook)}};
    MEMORY_BASIC_INFORMATION info{};constexpr std::uintptr_t first=0x00894710,last=0x00894748;
    if(!VirtualQuery(reinterpret_cast<void*>(first),&info,sizeof(info))||info.State!=MEM_COMMIT
       ||(info.Protect&(PAGE_NOACCESS|PAGE_GUARD))||reinterpret_cast<std::uintptr_t>(info.BaseAddress)+info.RegionSize<last) {
        log("Menu table is not mapped; hooks were not installed.");return false;}
    for(const auto &p:patches)if(*reinterpret_cast<const std::uintptr_t*>(p.location)!=p.expected){
        log("Main-menu vtable differs from the verified retail PC executable; hooks were not installed.");return false;}
    DWORD old=0;if(!VirtualProtect(reinterpret_cast<void*>(first),last-first,PAGE_READWRITE,&old)){log("Cannot make menu vtable writable.");return false;}
    original_draw=reinterpret_cast<Fn0>(patches[0].expected);original_update=reinterpret_cast<FnUpdate>(patches[1].expected);
    original_activate=reinterpret_cast<Fn0>(patches[2].expected);original_deactivate=reinterpret_cast<FnDeactivate>(patches[3].expected);
    original_up=reinterpret_cast<FnButton>(patches[4].expected);original_down=reinterpret_cast<FnButton>(patches[5].expected);original_cross=reinterpret_cast<FnButton>(patches[6].expected);
    for(const auto &p:patches)*reinterpret_cast<std::uintptr_t*>(p.location)=p.replacement;
    DWORD ignored=0;VirtualProtect(reinterpret_cast<void*>(first),last-first,old,&ignored);installed=true;
    log("Installed seventh main-menu option; native selection indices remain in [0,5].");return true;
#endif
}
void multiplayer_mode_tick_before(float game_dt) {
    using namespace usm::mp;(void)game_dt;if(!installed)return;initialize();animation_elapsed=0;
    double dt=elapsed_time();controls.poll(dt);
    if(active_menu&&(controls.connected[0]||controls.connected[1]))controller_menu_owner=true;
    update_ui();requests();assets_retry_release();
    if(screen==Screen::closed||screen==Screen::lobby||screen==Screen::error)return;
    if(network_match&&(screen==Screen::loading||match.phase!=Phase::match_end)) {
        lan_pump();if(lan_state()==LanState::error){error_screen(lan_error());return;}
        if(screen==Screen::loading&&lan_state()==LanState::assets) {
            selection=lan_settings();std::string why;status="Loading the host's chosen fighter packs...";
            if(!assets_load(selection,ini_path,why)){error_screen(why);return;}
            match.reset(selection);assets_pose(match,0);lan_ready();reset_clock();
        }
        if(screen==Screen::loading&&lan_state()==LanState::waiting)status="Fighters loaded. Waiting for the other PC to finish loading...";
        if(screen==Screen::loading&&lan_state()==LanState::playing){screen=Screen::battle;controls.gate();reset_clock();dt=0;log("LAN match ready on both PCs.");}
    }
    if(screen!=Screen::battle)return;
    if(!assets_actors()[0]||!assets_actors()[1]){error_screen("A multiplayer fighter was unloaded by the engine. Return to the lobby and reload.");return;}
    if(paused||match.phase==Phase::match_end){accumulator=0;return;}
    if(!network_match&&dt>0.5){pause_local("Long frame interruption. Select RESUME MATCH.");accumulator=0;return;}
    accumulator=std::min(accumulator+std::min(dt,0.1),0.1);network_wait=false;
    int steps=0;
    while(accumulator+1e-9>=1.0/tick_rate&&steps<6) {
        auto input=local_inputs();
        if(network_match) {
            auto local=assignment.lan_pad?controls.controller(0):controls.keyboard(0);
            if(!lan_exchange(match.tick,local,state_hash(match),input)) {
                if(lan_state()==LanState::error){error_screen(lan_error());return;}
                network_wait=true;break;
            }
        }
        bool frozen=match.hitstop>0;match.step(input);if(!frozen)animation_elapsed+=1.0f/tick_rate;
        accumulator-=1.0/tick_rate;++steps;
        // Keep TCP open through results: the peer may still need our final input.
        if(match.phase==Phase::match_end){accumulator=0;break;}
    }
}
void multiplayer_mode_tick_after(float game_dt) {
    using namespace usm::mp;(void)game_dt;if(!installed||screen!=Screen::battle)return;
    assets_pose(match,paused?0.0f:animation_elapsed);
}
bool multiplayer_mode_active() {
    return usm::mp::installed && usm::mp::screen != usm::mp::Screen::closed;
}
bool multiplayer_mode_captures_native_input() {
    using namespace usm::mp;
    return installed&&(native_transition_guard||raw_main_action||screen!=Screen::closed||(controller_menu_owner&&root_ready()));
}
void multiplayer_mode_world_shutdown() {
    using namespace usm::mp;if(!installed)return;
    lan_close();assets_release(true);screen=Screen::closed;active_menu=nullptr;seventh=false;request=Request::none;
    paused=false;network_match=false;network_wait=false;accumulator=0;animation_elapsed=0;controller_menu_owner=false;gamepads_shutdown();controls.gate();
}
#else
bool multiplayer_mode_install(){return false;}
void multiplayer_mode_tick_before(float){}
void multiplayer_mode_tick_after(float){}
bool multiplayer_mode_active(){return false;}
bool multiplayer_mode_captures_native_input(){return false;}
void multiplayer_mode_world_shutdown(){}
#endif
