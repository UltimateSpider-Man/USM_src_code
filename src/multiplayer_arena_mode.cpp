#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "multiplayer_arena_mode.h"
#if (defined(_WIN32) && !defined(OPENUSM_XBPACK_MODE)) || defined(USM_MULTIPLAYER_ARENA_TEST)
#include "multiplayer_assets.h"
#include "multiplayer_lan.h"
#include "multiplayer_render.h"
#include "multiplayer_gamepad_win.h"
#if defined(USM_MULTIPLAYER_ARENA_TEST)
#include "multiplayer_mode_test_adapter.h"
#else
#include <windows.h>
#endif
#include <algorithm>
#include <array>
#include <cmath>

namespace usm::mp::arena {
namespace {
struct Controls {
    std::array<bool,256> key{},previous{},raw{},blocked{};
    std::array<pad::Sample,2> samples{};
    std::array<pad::Tracker,2> trackers{};
    std::array<bool,2> connected{};
    bool focused=false;
    bool pressed(unsigned k)const{return key[k]&&!previous[k];}
    bool pad_pressed(std::uint32_t b)const{return ((trackers[0].pressed|trackers[1].pressed)&b)!=0;}
    bool navigation(std::uint32_t b)const{return ((trackers[0].navigation|trackers[1].navigation)&b)!=0;}
    bool up()const{return pressed(VK_UP)||navigation(pad::up);}
    bool down()const{return pressed(VK_DOWN)||navigation(pad::down);}
    bool left()const{return pressed(VK_LEFT)||navigation(pad::left);}
    bool right()const{return pressed(VK_RIGHT)||navigation(pad::right);}
    bool confirm()const{return pressed(VK_RETURN)||pad_pressed(pad::south);}
    bool back()const{return pressed(VK_ESCAPE)||pad_pressed(pad::east);}
    bool guide()const{return pressed(0x75)||pad_pressed(pad::guide);} // VK_F6
    bool pause()const{return pressed(VK_ESCAPE)||pad_pressed(pad::start)||guide();}
    void gate(){
        for(unsigned n=0;n<256;++n){blocked[n]=blocked[n]||raw[n];key[n]=previous[n]=false;}
        for(auto& t:trackers)t.gate();
    }
    void poll(double elapsed){
        DWORD pid=0;GetWindowThreadProcessId(GetForegroundWindow(),&pid);
        const bool old_focus=focused;focused=pid==GetCurrentProcessId();previous=key;
        for(unsigned n=0;n<256;++n){
            raw[n]=focused&&(GetAsyncKeyState(n)&0x8000)!=0;
            if(focused&&!old_focus)blocked[n]=raw[n];
            blocked[n]=blocked[n]&&raw[n];key[n]=raw[n]&&!blocked[n];
        }
        samples=gamepads_poll(focused,elapsed);
        for(unsigned n=0;n<2;++n){connected[n]=samples[n].connected;
            trackers[n].deadzone=gamepads_deadzone();trackers[n].poll(samples[n],focused,elapsed);}
    }
    Input keyboard(unsigned player)const{
        static const int keys[2][10]{{'A','D','W','S',VK_SPACE,'F','G','H','R','T'},
            {VK_LEFT,VK_RIGHT,VK_UP,VK_DOWN,VK_NUMPAD0,VK_NUMPAD1,VK_NUMPAD2,VK_NUMPAD3,VK_NUMPAD4,VK_NUMPAD5}};
        Input out{};for(unsigned n=0;n<10;++n)if(key[keys[player][n]])out.held|=static_cast<std::uint16_t>(1u<<n);
        return out;
    }
    Input controller(unsigned player)const{
        return focused&&connected[player]?pad::match_input(trackers[player].held):Input{};
    }
} controls;
Screen screen=Screen::closed;
Settings selection{};
Match match;
Layout layout=Layout::ds_panels;
int row=0,input_preset=4,connection_mode=0,overlay_row=0;
bool paused=false,network_match=false,network_wait=false,results_open=false,switch_requested=false;
pad::Assignment assignment{};
std::string ini_path,host_ip="127.0.0.1",status;
std::uint16_t port=7777;
double accumulator=0;
enum class Request { none,close,lobby,start,rematch,switch_type };
Request request=Request::none;

void reset_clock(){accumulator=0;}
void release_to(Screen next,bool teardown=false){
    // Resource-stack work can synchronously render. Stop presenting a battle
    // before releasing either fighter or popping its pack.
    screen=next;paused=network_match=network_wait=results_open=false;
    lan_close();assets_release(teardown);
    request=Request::none;overlay_row=0;status.clear();gamepads_lock(false);controls.gate();reset_clock();
}
void error_screen(std::string why){
    release_to(Screen::error);status=why;log("NDS arena: "+why);
}
bool missing_controller(){return pad::missing(assignment,controls.connected);}
void begin_match(){
    assignment=pad::assign(input_preset,int(controls.connected[0])+int(controls.connected[1]),connection_mode!=0);
    if(missing_controller()){
        status="Connect the required controller(s), or select another INPUT preset.";controls.gate();return;
    }
    gamepads_lock(true);controls.gate();overlay_row=0;results_open=false;
    selection.sanitize();match.reset(selection);screen=Screen::loading;paused=false;network_wait=false;
    status="Loading native PC fighter models and animations...";reset_clock();
    network_match=connection_mode!=0;
    if(network_match){
        if(!lan_begin(connection_mode==1,host_ip,port,selection)){error_screen(lan_error());return;}
        status=connection_mode==1?"Waiting for player 2 on LAN port "+std::to_string(port)+". Circle/B/Escape cancels."
            :"Connecting to "+host_ip+":"+std::to_string(port)+"...";
        return;
    }
    std::string why;
    if(!assets_load(selection,ini_path,why)){error_screen(why);return;}
    screen=Screen::battle;log("Started NDS-style local match with PC actors.");
}
void change_row(int direction){
    auto cycle=[direction](int v,int count){return (v+direction+count)%count;};
    switch(row){
    case 0:case 1:selection.character[row]=static_cast<Character>(cycle(static_cast<int>(selection.character[row]),4));break;
    case 2:selection.arena=static_cast<Arena>(cycle(static_cast<int>(selection.arena),4));break;
    case 3:layout=static_cast<Layout>(cycle(static_cast<int>(layout),4));break;
    case 4:input_preset=cycle(input_preset,5);break;
    case 5:selection.wins_required=cycle(selection.wins_required-1,5)+1;break;
    case 6:{constexpr int times[]{0,60,90,120};int i=0;for(int n=0;n<4;++n)if(selection.time_limit_seconds==times[n])i=n;
        selection.time_limit_seconds=times[cycle(i,4)];break;}
    case 7:selection.hazards=!selection.hazards;break;
    case 8:connection_mode=cycle(connection_mode,3);break;
    default:break;
    }
}
void pause_local(const std::string& reason){
    if(!paused){paused=true;overlay_row=0;controls.gate();reset_clock();}status=reason;
}
void resume_local(){
    if(!controls.focused||missing_controller())return;
    paused=false;status.clear();controls.gate();reset_clock();
}
void overlay_input(bool finished){
    const int count=pad::overlay_count(finished,network_match);
    if(controls.up())overlay_row=(overlay_row+count-1)%count;
    else if(controls.down())overlay_row=(overlay_row+1)%count;
    if(controls.pressed('Q')){request=Request::close;return;}
    if(controls.pressed('L')){request=Request::lobby;return;}
    if(controls.pressed('R')){request=Request::rematch;return;}
    if(controls.back()||controls.pad_pressed(pad::start)||controls.guide()){
        if(finished)request=Request::lobby;else resume_local();return;
    }
    if(!controls.confirm())return;
    switch(pad::overlay_action(overlay_row,finished,network_match)){
    case pad::OverlayAction::resume:resume_local();break;
    case pad::OverlayAction::rematch:request=Request::rematch;break;
    case pad::OverlayAction::lobby:request=Request::lobby;break;
    case pad::OverlayAction::close:request=Request::close;break;
    }
}
void update_ui(){
    if(screen==Screen::lobby){
        if(controls.back()||controls.guide()){request=Request::close;return;}
        if(controls.up())row=(row+11)%12;else if(controls.down())row=(row+1)%12;
        if(controls.left())change_row(-1);else if(controls.right())change_row(1);
        if(controls.pad_pressed(pad::start)){request=Request::start;return;}
        if(controls.confirm()){
            if(row==9)request=Request::start;else if(row==10)request=Request::switch_type;
            else if(row==11)request=Request::close;else change_row(1);
        }
        return;
    }
    if(screen==Screen::loading||screen==Screen::error){
        if(controls.back()||controls.guide()||(screen==Screen::error&&controls.confirm()))request=Request::lobby;
        return;
    }
    if(screen!=Screen::battle)return;
    if(match.phase==Phase::match_end){
        if(!results_open){results_open=true;overlay_row=network_match?0:1;controls.gate();return;}
        overlay_input(true);return;
    }
    if(network_match){if(controls.pause())request=Request::lobby;return;}
    if(!controls.focused)pause_local("Window lost focus. Release buttons, then resume.");
    else if(missing_controller())pause_local("Controller disconnected. Reconnect it, or return to the lobby.");
    if(paused)overlay_input(false);else if(controls.pause())pause_local("");
}
void requests(){
    const auto pending=request;request=Request::none;
    if(pending==Request::close)release_to(Screen::closed);
    else if(pending==Request::switch_type){release_to(Screen::closed);switch_requested=true;}
    else if(pending==Request::lobby)release_to(Screen::lobby);
    else if(pending==Request::start)begin_match();
    else if(pending==Request::rematch){
        if(network_match){release_to(Screen::lobby);return;}
        if(missing_controller()){status="Reconnect the assigned controller(s), or return to the lobby.";return;}
        match.reset(selection);paused=false;overlay_row=0;results_open=false;status.clear();controls.gate();reset_clock();
        log("NDS arena local rematch.");
    }
}
std::array<Input,2> local_inputs(){
    std::array<Input,2> out;
    for(unsigned n=0;n<2;++n)out[n]=assignment.controller[n]>=0?controls.controller(assignment.controller[n]):controls.keyboard(n);
    return out;
}
}
void open(const std::string& path){
    close();ini_path=path;switch_requested=false;
    selection={};connection_mode=0;
    layout=static_cast<Layout>(std::clamp(int(GetPrivateProfileIntA("Multiplayer","View",3,ini_path.c_str())),0,3));
    input_preset=std::clamp(int(GetPrivateProfileIntA("Multiplayer","InputPreset",4,ini_path.c_str())),0,4);
    selection.wins_required=std::clamp(int(GetPrivateProfileIntA("Multiplayer","WinsRequired",2,ini_path.c_str())),1,5);
    char value[128]{};GetPrivateProfileStringA("Multiplayer","HostIP","127.0.0.1",value,sizeof(value),ini_path.c_str());host_ip=value;
    port=static_cast<std::uint16_t>(std::clamp(int(GetPrivateProfileIntA("Multiplayer","Port",7777,ini_path.c_str())),1,65535));
    gamepads_configure(ini_path);controls=Controls{};controls.poll(0);controls.gate();
    screen=Screen::lobby;row=0;log("Opened NDS-style arena with PC graphics.");
}
void tick(double dt){
    if(!active())return;
    if(!std::isfinite(dt)||dt<0)dt=0;
    controls.poll(std::min(dt,.1));update_ui();requests();assets_retry_release();
    if(screen==Screen::closed||screen==Screen::lobby||screen==Screen::error)return;
    if(network_match&&(screen==Screen::loading||match.phase!=Phase::match_end)){
        lan_pump();if(lan_state()==LanState::error){error_screen(lan_error());return;}
        if(screen==Screen::loading&&lan_state()==LanState::assets){
            selection=lan_settings();std::string why;status="Loading the host's chosen PC fighter packs...";
            if(!assets_load(selection,ini_path,why)){error_screen(why);return;}
            match.reset(selection);lan_ready();reset_clock();
        }
        if(screen==Screen::loading&&lan_state()==LanState::waiting)status="Fighters loaded. Waiting for the other PC...";
        if(screen==Screen::loading&&lan_state()==LanState::playing){screen=Screen::battle;controls.gate();reset_clock();dt=0;}
    }
    if(screen!=Screen::battle)return;
    const auto actors=assets_actors();
    if(!actors[0]||!actors[1]){error_screen("A PC fighter was unloaded. Return to the lobby and reload.");return;}
    if(paused||match.phase==Phase::match_end){reset_clock();assets_pose(match,0);return;}
    if(!network_match&&dt>.5){pause_local("Long frame interruption. Select RESUME MATCH.");assets_pose(match,0);return;}
    accumulator=std::min(accumulator+std::min(dt,.1),.1);network_wait=false;
    float animation_elapsed=0;
    for(int steps=0;accumulator+1e-9>=1.0/tick_rate&&steps<6;++steps){
        auto input=local_inputs();
        if(network_match){
            const auto local=assignment.lan_pad?controls.controller(0):controls.keyboard(0);
            if(!lan_exchange(match.tick,local,state_hash(match),input)){
                if(lan_state()==LanState::error){error_screen(lan_error());return;}
                network_wait=true;break;
            }
        }
        const bool frozen=match.hitstop>0;match.step(input);if(!frozen)animation_elapsed+=1.0f/tick_rate;
        accumulator-=1.0/tick_rate;
        if(match.phase==Phase::match_end){reset_clock();break;}
    }
    assets_pose(match,animation_elapsed);
}
void draw(){
    if(!active())return;
    Display d;d.screen=screen;d.match=match;d.selection=selection;d.layout=layout;
    d.row=row;d.overlay_row=overlay_row;d.input_preset=input_preset;d.connection_mode=connection_mode;d.paused=paused;
    d.network_wait=network_wait;d.network_match=network_match;d.status=status;d.warning=assets_warning();
    for(unsigned n=0;n<2;++n)d.controllers[n]=controls.connected[n]?controls.samples[n].name:"not connected";
    d.input_status=gamepads_diagnostic();d.host_ip=host_ip;d.port=port;
    if(screen==Screen::battle)d.actors=assets_actors();
    render_mode(d);
}
void close(bool world_shutdown){release_to(Screen::closed,world_shutdown);switch_requested=false;}
bool active(){return screen!=Screen::closed;}
bool take_switch_request(){const bool out=switch_requested;switch_requested=false;return out;}
}
#else
namespace usm::mp::arena {
void open(const std::string&){}
void tick(double){}
void draw(){}
void close(bool){}
bool active(){return false;}
bool take_switch_request(){return false;}
}
#endif
