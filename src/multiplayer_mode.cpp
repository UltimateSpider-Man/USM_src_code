#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "multiplayer_mode.h"
#include "multiplayer_arena_mode.h"
#if (defined(_WIN32) && !defined(OPENUSM_XBPACK_MODE)) || defined(USM_ONLINE_MODE_TEST)
#include "multiplayer_online_engine.h"
#include "multiplayer_online_interfaces.h"
#include "multiplayer_online_ui.h"
#include "multiplayer_gamepad_win.h"
#if defined(USM_ONLINE_MODE_TEST)
#include "online_mode_test_adapter.h"
#else
#include "float.hpp"
#include "game.h"
#include "main_menu_options.h"
#include "panelanimfile.h"
#include "pc_joypad_device.h"
#include "variables.h"
#include "ngl.h"
#include <windows.h>
#endif
#include <array>
#include <cmath>
#include <cstring>
#include <fstream>
#include <string>

struct camera;
extern int debug_enabled;
extern int debug_disabled;
namespace usm::mp { void log(const std::string&); }
namespace mp=usm::mp;
namespace usm::online {
namespace {
namespace pad=usm::mp::pad;
bool installed=false,initialized=false,panel=false,seventh=false,input_guard=false,raw_main_action=false,controller_owner=false;
bool auto_continue=false,was_active=false,native_bridge=true;
bool menu_tick=false;
bool arena_type=false;
main_menu_options* active_menu=nullptr;
Session session;
Options options;
std::string ini_path,log_path,status;
std::vector<LocalAddress> addresses;
std::size_t address_index=0;
LARGE_INTEGER frequency{},last_counter{};
double now=0;
int row=0,editing=-1;
Editor editor;
enum class Task {none,host,join,leave,resume,switch_arena};
Task task=Task::none;
using Fn0=void(__thiscall*)(main_menu_options*);
using FnButton=void(__thiscall*)(main_menu_options*,int);
using FnUpdate=void(__thiscall*)(main_menu_options*,Float);
using FnDeactivate=void(__thiscall*)(main_menu_options*,FEMenu*);
Fn0 original_draw=nullptr,original_activate=nullptr;
FnButton original_up=nullptr,original_down=nullptr,original_cross=nullptr;
FnUpdate original_update=nullptr;
FnDeactivate original_deactivate=nullptr;
struct Controls {
    std::array<bool,256> key{},previous{},raw{},blocked{};
    std::array<pad::Sample,2> pads{};
    std::array<pad::Tracker,2> trackers{};
    bool focused=false;
    bool pressed(unsigned k)const{return key[k]&&!previous[k];}
    bool pad_pressed(std::uint32_t b)const{return ((trackers[0].pressed|trackers[1].pressed)&b)!=0;}
    bool nav(std::uint32_t b)const{return ((trackers[0].navigation|trackers[1].navigation)&b)!=0;}
    bool up()const{return pressed(VK_UP)||nav(pad::up);}
    bool down()const{return pressed(VK_DOWN)||nav(pad::down);}
    bool left()const{return pressed(VK_LEFT)||nav(pad::left);}
    bool right()const{return pressed(VK_RIGHT)||nav(pad::right);}
    bool confirm()const{return pressed(VK_RETURN)||pad_pressed(pad::south);}
    bool back()const{return pressed(VK_ESCAPE)||pad_pressed(pad::east);}
    void gate(){
        input_guard=true;
        for(unsigned i=0;i<256;++i){blocked[i]=blocked[i]||raw[i];key[i]=previous[i]=false;}
        for(auto& t:trackers)t.gate();
    }
    void poll(double dt){
        DWORD pid=0;GetWindowThreadProcessId(GetForegroundWindow(),&pid);
        const bool was_focused=focused;focused=pid==GetCurrentProcessId();previous=key;
        for(unsigned i=0;i<256;++i){raw[i]=focused&&(GetAsyncKeyState(i)&0x8000)!=0;
            if(focused&&!was_focused)blocked[i]=raw[i];blocked[i]=blocked[i]&&raw[i];key[i]=raw[i]&&!blocked[i];}
        auto old=pads;pads=mp::gamepads_poll(focused,dt);
        bool acquisition=focused&&!was_focused;
        for(unsigned i=0;i<2;++i){trackers[i].deadzone=mp::gamepads_deadzone();trackers[i].poll(pads[i],focused,dt);
            acquisition=acquisition||(pads[i].connected&&(!old[i].connected||pads[i].id!=old[i].id));}
        if(acquisition)gate();
        if(input_guard){bool held=false;
            for(bool k:raw)held=held||k;
            for(const auto& p:pads)if(p.connected)held=held||p.buttons||std::abs(p.x)>mp::gamepads_deadzone()||std::abs(p.y)>mp::gamepads_deadzone()||std::abs(p.rx)>mp::gamepads_deadzone()||std::abs(p.ry)>mp::gamepads_deadzone();
            if(!held)input_guard=false;}
    }
} controls;
std::string option(const char* key,const char* fallback) {
    char text[160]{};GetPrivateProfileStringA("Online",key,fallback,text,sizeof(text),ini_path.c_str());return text;
}
void write_option(const char* key,const std::string& value){
    if(!WritePrivateProfileStringA("Online",key,value.c_str(),ini_path.c_str()))status="Impossibile salvare multiplayer.ini: controlla i permessi della cartella.";
}
void initialize(){
    if(initialized)return;initialized=true;
    char path[MAX_PATH]{};DWORD size=GetModuleFileNameA(nullptr,path,MAX_PATH);
    std::string dir=size>0&&size<MAX_PATH?std::string(path,size):std::string{};
    auto slash=dir.find_last_of("\\/");dir=slash==std::string::npos?".\\":dir.substr(0,slash+1);
    ini_path=dir+"multiplayer.ini";log_path=dir+"multiplayer_online.log";
    options.nickname=option("Nickname","Player");if(!valid_nickname(options.nickname))options.nickname="Player";
    options.host=option("HostIP","127.0.0.1");options.bind_address=option("BindIP","0.0.0.0");options.content_tag=option("ContentTag","retail-pc-online-v1");
    options.port=std::uint16_t(std::clamp(int(GetPrivateProfileIntA("Online","Port",7777,ini_path.c_str())),1,65535));
    options.capacity=unsigned(std::clamp(int(GetPrivateProfileIntA("Online","MaxPlayers",4,ini_path.c_str())),2,8));
    options.send_hz=unsigned(std::clamp(int(GetPrivateProfileIntA("Online","SendHz",20,ini_path.c_str())),10,30));
    options.interpolation=std::clamp(int(GetPrivateProfileIntA("Online","InterpolationMs",100,ini_path.c_str())),50,300)*.001;
    native_bridge=GetPrivateProfileIntA("Online","NativePadBridge",1,ini_path.c_str())!=0;
    arena_type=GetPrivateProfileIntA("Multiplayer","Type",0,ini_path.c_str())==1;
    mp::gamepads_configure(ini_path);engine_configure(ini_path);
    QueryPerformanceFrequency(&frequency);QueryPerformanceCounter(&last_counter);
    mp::log("Online M1 initialized; native runtime acceptance pending. Port="+std::to_string(options.port));
}
void open_online_panel(){addresses=local_ipv4_addresses();address_index=0;panel=true;editing=-1;seventh=false;row=0;controls.gate();}
void open_panel(){if(arena_type)task=Task::switch_arena;else open_online_panel();}
void save_type(){
    if(!WritePrivateProfileStringA("Multiplayer","Type",arena_type?"1":"0",ini_path.c_str()))
        mp::log("Could not save multiplayer type; current selection still applies.");
}
void close_panel(){panel=false;editing=-1;controls.gate();}
bool root_ready(){return active_menu&&!active_menu->field_108&&!active_menu->field_109&&!byte_922994()&&(!active_menu->field_E4||!active_menu->field_E4->field_2D);}
bool mouse_entry(){
    if(!controls.pressed(VK_LBUTTON))return false;POINT p{};RECT rect{};HWND window=GetForegroundWindow();
    if(!GetCursorPos(&p)||!ScreenToClient(window,&p)||!GetClientRect(window,&rect)||rect.right<=0||rect.bottom<=0)return false;
    const float x=float(p.x)*640/rect.right,y=float(p.y)*480/rect.bottom;
    return x>=165&&x<=475&&y>=420&&y<=452;
}
void main_up(main_menu_options* self,int player){
    if(panel||mp::arena::active()||!root_ready())return;
    if(seventh){seventh=false;self->field_106=self->field_104;self->field_104=5;self->update_highlight();return;}
    if(self->field_104==0){seventh=true;return;}original_up(self,player);
}
void main_down(main_menu_options* self,int player){
    if(panel||mp::arena::active()||!root_ready())return;
    if(seventh){seventh=false;self->field_104=5;original_down(self,player);return;}
    if(self->field_104==5){seventh=true;return;}original_down(self,player);
}
void main_confirm(main_menu_options* self,int player){
    if(panel||mp::arena::active()||!root_ready())return;
    if(seventh){open_panel();return;}controls.gate();original_cross(self,player);
}
bool resume_game(){
    Pose p{};if(engine_capture(p)){close_panel();return true;}
    if(!root_ready()||active_menu->field_10A){status="Carica un salvataggio e torna alla citta' libera. La sessione resta aperta.";return false;}
    // Reuse the native Continue/Load flow. Never manufacture a second native
    // hero, mutate a save slot, or force a story-script transition.
    auto* menu=active_menu;close_panel();seventh=false;menu->field_106=menu->field_104;menu->field_104=0;menu->update_highlight();
    original_cross(menu,0);return true;
}
void accept_edit(){
    if(editing==0){if(!valid_nickname(editor.value)){status="Nickname: 1-24 lettere/cifre/spazi interni o _ - .";return;}options.nickname=editor.value;write_option("Nickname",options.nickname);}
    else if(editing==1){if(!valid_ipv4_text(editor.value)){status="IP non valido: usa quattro numeri, ad esempio 26.1.2.3.";return;}options.host=editor.value;write_option("HostIP",options.host);}
    else if(editing==2){std::uint16_t port=0;if(!parse_port(editor.value,port)){status="Porta non valida: usa un numero da 1 a 65535.";return;}options.port=port;write_option("Port",std::to_string(port));}
    editing=-1;controls.gate();
}
void text_input(){
    if(controls.back()){editing=-1;controls.gate();return;}
    if(controls.confirm()){accept_edit();return;}
    if(controls.left())editor.move(-1);if(controls.right())editor.move(1);
    if(controls.nav(pad::up))editor.cycle(1);if(controls.nav(pad::down))editor.cycle(-1);
    if(controls.pressed(VK_BACK)||controls.pad_pressed(pad::west))editor.backspace();
    if(controls.pressed(VK_DELETE))editor.erase();
    if(controls.pad_pressed(pad::north))editor.insert(editing==0?'A':'0');
    const bool shift=controls.key[VK_SHIFT];
    for(int k='A';k<='Z';++k)if(controls.pressed(k))editor.insert(char(shift?k:k+32));
    for(int k='0';k<='9';++k)if(controls.pressed(k))editor.insert(char(k));
    for(int k=VK_NUMPAD0;k<=VK_NUMPAD9;++k)if(controls.pressed(k))editor.insert(char('0'+k-VK_NUMPAD0));
    if(controls.pressed(VK_SPACE))editor.insert(' ');
    if(controls.pressed(VK_OEM_PERIOD)||controls.pressed(VK_DECIMAL))editor.insert('.');
    if(controls.pressed(VK_OEM_MINUS)||controls.pressed(VK_SUBTRACT))editor.insert(shift?'_':'-');
}
void panel_input(){
    if(editing<0&&controls.pad_pressed(pad::r1)&&!addresses.empty())address_index=(address_index+1)%addresses.size();
    if(editing>=0){text_input();return;}
    if(controls.back()){close_panel();return;}
    if(controls.up())row=(row+9)%10;if(controls.down())row=(row+1)%10;
    if(row==9&&(controls.left()||controls.right())){task=Task::switch_arena;return;}
    const bool connected=session.active()||session.state()==State::connecting;
    if(row==3&&!connected&&(controls.left()||controls.right())){
        options.capacity=unsigned(std::clamp(int(options.capacity)+(controls.right()?1:-1),2,8));write_option("MaxPlayers",std::to_string(options.capacity));}
    if(!controls.confirm())return;
    status.clear();
    if(row<=2){if(connected){status="Disconnetti la sessione prima di cambiare nome o indirizzo.";return;}
        editing=row;editor.begin(row==0?options.nickname:row==1?options.host:std::to_string(options.port),row==0?24:row==1?15:5,row!=0);controls.gate();}
    else if(row==3&&!connected){options.capacity=options.capacity==8?2:options.capacity+1;write_option("MaxPlayers",std::to_string(options.capacity));}
    else if(row==4&&!connected)task=Task::host;
    else if(row==5&&!connected)task=Task::join;
    else if(row==6)task=Task::resume;
    else if(row==7)task=Task::leave;
    else if(row==8)close_panel();
    else if(row==9)task=Task::switch_arena;
}
void update_ui(){
    if(controls.pressed(VK_F6)||controls.pad_pressed(pad::guide)){if(panel)close_panel();else open_panel();return;}
    if(panel){panel_input();return;}
    if(root_ready()){
        controller_owner=controls.pads[0].connected||controls.pads[1].connected;
        if(controls.pressed('M')||mouse_entry()){open_panel();return;}
        if(controller_owner){raw_main_action=true;
            if(controls.up())main_up(active_menu,0);else if(controls.down())main_down(active_menu,0);
            if(controls.confirm())main_confirm(active_menu,0);raw_main_action=false;}
    }
}
void requests(){
    const auto pending=task;task=Task::none;
    if(pending==Task::host||pending==Task::join){
        engine_release();session.close();
        write_option("Nickname",options.nickname);write_option("HostIP",options.host);write_option("Port",std::to_string(options.port));
        bool ok=pending==Task::host?session.host(options,now):session.join(options,now);
        if(!ok){status=session.error();mp::log("Session start failed: "+status);auto_continue=false;return;}
        mp::log(pending==Task::host?"Host session opened.":"Joining "+options.host+":"+std::to_string(options.port));
        auto_continue=true;status=pending==Task::host?"Sessione aperta. Condividi il tuo IPv4 Radmin e la porta.":"Connessione all'host...";
        mp::gamepads_lock(controls.pads[0].connected);controls.gate();
    }else if(pending==Task::leave){
        session.close();engine_release();auto_continue=false;mp::gamepads_lock(false);status="Sessione chiusa. Ritorno al controllo locale.";mp::log("Session closed by local player.");controls.gate();
    }else if(pending==Task::resume)resume_game();
    else if(pending==Task::switch_arena){
        // A type switch owns the teardown: online proxies/sockets never share
        // their resources or controller seats with the two-fighter arena.
        session.close();engine_release();auto_continue=was_active=false;
        mp::gamepads_lock(false);close_panel();seventh=false;
        arena_type=true;save_type();mp::arena::open(ini_path);
    }
    if(auto_continue&&session.active()){auto_continue=false;resume_game();}
}
float sx(){return float(nglGetScreenWidth())/640.f;}
float sy(){return float(nglGetScreenHeight())/480.f;}
void text(float x,float y,const std::string& s,float scale=1,std::uint32_t c=0xffeeeeee){draw_text(x*sx(),y*sy(),s,scale*std::min(sx(),sy()),c);}
void rect(float x,float y,float w,float h,std::uint32_t c){draw_rect(x*sx(),y*sy(),w*sx(),h*sy(),c);}
void draw_panel(){
    rect(12,12,616,456,0xf2141822);text(30,26,"MULTIPLAYER ONLINE",1.1f,0xff58a8ff);
    text(30,49,"Un PC per giocatore - camera nativa - nessuno split-screen",.6f);
    std::array<std::string,10> rows{{"Nickname: "+options.nickname,"IP host (join): "+options.host,"Porta TCP + UDP: "+std::to_string(options.port),
        "Posti sessione: "+std::to_string(options.capacity),"CREA SESSIONE","UNISCITI ALLA SESSIONE","CONTINUA / TORNA AL GIOCO","DISCONNETTI","INDIETRO",
        "SWITCH MULTIPLAYER TYPE: NDS VERSUS"}};
    for(unsigned i=0;i<rows.size();++i){float y=78+i*22.f;if(int(i)==row)rect(24,y-3,342,21,0xff293e60);text(32,y,rows[i],.70f,int(i)==row?0xffffffff:0xffbfcadd);}
    text(385,76,"GIOCATORI",.65f,0xff58a8ff);
    unsigned count=0;for(const auto& p:session.players())if(p.identity.id){
        auto c=player_color(p.identity.seat);auto color=0xff000000u|(unsigned(c.r)<<16)|(unsigned(c.g)<<8)|c.b;
        text(380,101+count*23.f,p.identity.nickname+(p.identity.id==session.local_id()?" (tu)":""),.65f,color);
        ++count;
    }
    text(380,269,"IP LOCALI DA CONDIVIDERE",.50f,0xff58a8ff);
    if(!addresses.empty()){const auto& a=addresses[address_index%addresses.size()];text(380,284,a.adapter.substr(0,22),.50f);text(380,297,a.ipv4+":"+std::to_string(options.port),.60f);}
    else text(380,284,"Controlla l'IP in Radmin VPN",.5f);
    if(addresses.size()>1)text(380,310,"R1/RB: altro indirizzo",.48f);
    text(30,316,"F6 o tasto PS: pannello sessione",.6f);
    text(30,336,"Free-roam sperimentale: mondo/NPC e danni NON condivisi",.59f,0xffffbf69);
    if(editing>=0){rect(24,365,590,90,0xff172b40);auto display=editor.value;display.insert(editor.cursor,"|");
        text(32,373,display,.85f,0xffffffff);text(32,401,"Tastiera: digita. Pad: SX/DX cursore, SU/GIU carattere",.6f);
        text(32,421,"Cross/A salva - Circle/B annulla - Square/X elimina - Triangle/Y aggiunge",.53f);}
    else{
        const auto& error=session.error();const auto& msg=error.empty()?status:error;
        for(unsigned i=0;i<2&&i*88<msg.size();++i)text(30,364+i*17.f,msg.substr(i*88,88),.57f,0xffffbf69);
        const auto& warn=engine_warning();if(!warn.empty())text(30,407,warn.substr(0,90),.55f,0xffffbf69);
        text(30,441,"Pad/tastiera: direzioni, Cross/A/Invio, Circle/B/Esc",.6f);
    }
}
void __fastcall draw_hook(main_menu_options* self,void*) {
    active_menu=self;
    if(mp::arena::active()){mp::arena::draw();return;}
    if(panel){draw_panel();return;}
    original_draw(self);rect(165,420,310,32,seventh?0xee284f80:0xc0141822);
    text(181,428,"MULTIPLAYER MODE",.85f,seventh?0xffffffff:0xff58a8ff);
}
void __fastcall update_hook(main_menu_options* self,void*,Float dt){
    active_menu=self;
    // Front-end updates run without the game-world frame hook. Poll here so
    // connected and newly attached pads can operate the main menu and panel.
    menu_tick=true;multiplayer_mode_tick_before(static_cast<float>(dt));menu_tick=false;
    if(!panel&&!mp::arena::active())original_update(self,dt);
}
void __fastcall activate_hook(main_menu_options* self,void*){active_menu=self;seventh=false;controller_owner=false;controls.gate();original_activate(self);}
void __fastcall deactivate_hook(main_menu_options* self,void*,FEMenu* next){
    if(mp::arena::active())mp::arena::close();
    active_menu=nullptr;seventh=false;controller_owner=false;panel=false;editing=-1;original_deactivate(self,next);
    // A Continue/load transition is NOT a disconnect.
}
void __fastcall up_hook(main_menu_options* self,void*,int p){if(!input_guard&&!raw_main_action&&!(controller_owner&&root_ready()))main_up(self,p);}
void __fastcall down_hook(main_menu_options* self,void*,int p){if(!input_guard&&!raw_main_action&&!(controller_owner&&root_ready()))main_down(self,p);}
void __fastcall cross_hook(main_menu_options* self,void*,int p){if(!input_guard&&!raw_main_action&&!(controller_owner&&root_ready()&&!controls.pressed(VK_LBUTTON)))main_confirm(self,p);}
#if !defined(USM_ONLINE_MODE_TEST)
void __fastcall world_hook(void* self,void*,::camera* camera,int player){
    // Signature/callsite verified against the supplied retail executable bytes.
    using Render=void(__fastcall*)(void*,void*,::camera*,int);
    reinterpret_cast<Render>(0x0054B250)(self,nullptr,camera,player);
    if(player==0)engine_draw_world(session,now);
}
void __fastcall hero_unload_hook(game* self,void*){
    // Call the compiled implementation, not the patched retail entry point.
    // It releases borrowed HERO actors before the native partition is emptied.
    self->unload_hero_packfile();
}
#endif
}
} // namespace usm::online
namespace usm::mp {
void log(const std::string& s){std::string line="[USM Online] "+s+"\n";OutputDebugStringA(line.c_str());
    if(!online::log_path.empty()){std::ofstream file(online::log_path,std::ios::app);if(file)file<<line;}}
}
bool multiplayer_mode_install(){
    using namespace usm::online;if(installed)return true;
#if defined(USM_ONLINE_MODE_TEST)
    return false;
#else
    static_assert(sizeof(void*)==4,"Online integration requires the retail PC x86 ABI");
    static_assert(sizeof(main_menu_options)==0x110,"Unexpected native main menu layout");
    struct Patch {std::uintptr_t at,expected,replacement;};
    const Patch p[]{
        {0x894710,0x6139C0,reinterpret_cast<std::uintptr_t>(&draw_hook)},
        {0x894718,0x637B90,reinterpret_cast<std::uintptr_t>(&update_hook)},
        {0x894724,0x62CE60,reinterpret_cast<std::uintptr_t>(&activate_hook)},
        {0x894728,0x6139B0,reinterpret_cast<std::uintptr_t>(&deactivate_hook)},
        {0x894734,0x623500,reinterpret_cast<std::uintptr_t>(&up_hook)},
        {0x894738,0x6235E0,reinterpret_cast<std::uintptr_t>(&down_hook)},
        {0x894744,0x6236C0,reinterpret_cast<std::uintptr_t>(&cross_hook)}};
    constexpr std::uintptr_t call=0x54E52D;
    // mov ecx,[edi+0x50]; push player 0; push camera ebp; add ecx,0xa0; call render.
    constexpr std::uintptr_t render_abi=0x54E521;
    const unsigned char original_render_abi[]{0x8b,0x4f,0x50,0x6a,0x00,0x55,0x81,0xc1,0xa0,0x00,0x00,0x00,0xe8,0x1e,0xcd,0xff,0xff};
    constexpr std::uintptr_t hero_unload=0x558320;
    // Retail void game::unload_hero_packfile(): mov eax,[partitions];
    // mov eax,[eax+4]; push esi. There are no stack arguments.
    const unsigned char original_hero_unload[]{0xa1,0xf0,0xc7,0x95,0x00,0x8b,0x40,0x04,0x56};
    auto readable=[](std::uintptr_t at,std::size_t n){MEMORY_BASIC_INFORMATION m{};
        return VirtualQuery(reinterpret_cast<void*>(at),&m,sizeof(m))&&m.State==MEM_COMMIT&&!(m.Protect&(PAGE_NOACCESS|PAGE_GUARD))&&reinterpret_cast<std::uintptr_t>(m.BaseAddress)+m.RegionSize>=at+n;};
    if(!readable(0x894710,0x38)||!readable(render_abi,sizeof(original_render_abi))
       ||!readable(hero_unload,sizeof(original_hero_unload))){mp::log("Native hook regions not mapped; online disabled.");return false;}
    for(const auto& x:p)if(*reinterpret_cast<const std::uintptr_t*>(x.at)!=x.expected){mp::log("Native menu signature mismatch; online disabled.");return false;}
    if(std::memcmp(reinterpret_cast<void*>(render_abi),original_render_abi,sizeof(original_render_abi))){mp::log("Native world-render call differs; online disabled.");return false;}
    if(std::memcmp(reinterpret_cast<void*>(hero_unload),original_hero_unload,sizeof(original_hero_unload))){mp::log("Native hero-unload entry differs; online disabled.");return false;}
    DWORD table_old=0,code_old=0,hero_old=0,unused=0;
    if(!VirtualProtect(reinterpret_cast<void*>(0x894710),0x38,PAGE_READWRITE,&table_old))return false;
    if(!VirtualProtect(reinterpret_cast<void*>(call),5,PAGE_EXECUTE_READWRITE,&code_old)){
        VirtualProtect(reinterpret_cast<void*>(0x894710),0x38,table_old,&unused);return false;}
    if(!VirtualProtect(reinterpret_cast<void*>(hero_unload),5,PAGE_EXECUTE_READWRITE,&hero_old)){
        VirtualProtect(reinterpret_cast<void*>(call),5,code_old,&unused);
        VirtualProtect(reinterpret_cast<void*>(0x894710),0x38,table_old,&unused);return false;}
    original_draw=reinterpret_cast<Fn0>(p[0].expected);original_update=reinterpret_cast<FnUpdate>(p[1].expected);
    original_activate=reinterpret_cast<Fn0>(p[2].expected);original_deactivate=reinterpret_cast<FnDeactivate>(p[3].expected);
    original_up=reinterpret_cast<FnButton>(p[4].expected);original_down=reinterpret_cast<FnButton>(p[5].expected);original_cross=reinterpret_cast<FnButton>(p[6].expected);
    // All preconditions/protections succeeded before the first modification.
    for(const auto& x:p)*reinterpret_cast<std::uintptr_t*>(x.at)=x.replacement;
    const auto relative=std::uint32_t(reinterpret_cast<std::uintptr_t>(&world_hook)-(call+5));
    std::memcpy(reinterpret_cast<void*>(call+1),&relative,4);
    const auto hero_relative=std::uint32_t(reinterpret_cast<std::uintptr_t>(&hero_unload_hook)-(hero_unload+5));
    *reinterpret_cast<unsigned char*>(hero_unload)=0xe9;
    std::memcpy(reinterpret_cast<void*>(hero_unload+1),&hero_relative,4);
    VirtualProtect(reinterpret_cast<void*>(hero_unload),5,hero_old,&unused);
    VirtualProtect(reinterpret_cast<void*>(call),5,code_old,&unused);VirtualProtect(reinterpret_cast<void*>(0x894710),0x38,table_old,&unused);
    FlushInstructionCache(GetCurrentProcess(),reinterpret_cast<void*>(hero_unload),5);
    FlushInstructionCache(GetCurrentProcess(),reinterpret_cast<void*>(call),5);installed=true;
    mp::log("Host/join hooks installed. Native hero/camera remain local; remote presence is experimental.");return true;
#endif
}
void multiplayer_mode_tick_before(float game_dt){
    using namespace usm::online;(void)game_dt;if(!installed||(active_menu&&!menu_tick))return;initialize();
    LARGE_INTEGER time{};QueryPerformanceCounter(&time);
    double elapsed=frequency.QuadPart>0?double(time.QuadPart-last_counter.QuadPart)/double(frequency.QuadPart):0;
    last_counter=time;if(!std::isfinite(elapsed)||elapsed<0)elapsed=0;now+=elapsed;
    if(mp::arena::active()){
        // Exactly one controller poll and arena advance per frontend/world
        // frame; the online input tracker sleeps until this mode closes.
        mp::arena::tick(elapsed);
        if(mp::arena::take_switch_request()){
            arena_type=false;save_type();controls.poll(0);open_online_panel();
        }else if(!mp::arena::active()){controls.poll(0);controls.gate();}
        return;
    }
    controls.poll(std::min(elapsed,.1));update_ui();requests();
    if(mp::arena::active())return;
    session.poll(now);
    if(auto_continue&&session.active()){auto_continue=false;resume_game();}
    if(was_active&&!session.active()){mp::log("Session ended: "+session.error());engine_release();mp::gamepads_lock(false);open_panel();status=session.error();}
    was_active=session.active();
    if(session.active()&&controls.pads[0].connected)mp::gamepads_lock(true);
    if(session.active())engine_update(session,now,float(std::min(elapsed,.1)));
}
void multiplayer_mode_tick_after(float game_dt){
    using namespace usm::online;(void)game_dt;if(!installed||!session.active())return;
    Pose p{};engine_capture(p);
    if(!controls.focused){p.flags|=PoseFlags::frozen;p.velocity={};}
    session.set_local_pose(p);
}
void multiplayer_mode_draw_overlay(){using namespace usm::online;if(!installed||!initialized)return;
    if(mp::arena::active()){if(!active_menu)mp::arena::draw();return;}
    if(panel&&!active_menu)draw_panel();
    else if(session.active()&&!active_menu){unsigned count=0;for(const auto& p:session.players())if(p.identity.id)++count;
        text(10,8,"ONLINE  "+std::to_string(count)+" giocatori  |  F6 / PS",.55f,0xff75b7ff);}
}
bool multiplayer_mode_active(){using namespace usm::online;return installed&&(mp::arena::active()||panel||session.active()||session.state()==State::connecting);}
bool multiplayer_mode_blocks_world(){using namespace usm::online;return installed&&mp::arena::active()&&!active_menu;}
bool multiplayer_mode_captures_native_input(){using namespace usm::online;
    return installed&&(mp::arena::active()||panel||raw_main_action||((active_menu||session.active())&&input_guard)||(controller_owner&&root_ready()));}
bool multiplayer_mode_owns_native_pad(unsigned handle){using namespace usm::online;
    // InputOpen(0,0) returns 1. It is a native device HANDLE, not network ID 1
    // and not an XInput zero-based index. Every PC keeps that same local handle.
    return installed&&!mp::arena::active()&&session.active()&&native_bridge&&handle==1;
}
void multiplayer_mode_merge_native_input(unsigned handle,InputState& state){using namespace usm::online;
    if(!multiplayer_mode_owns_native_pad(handle)||panel||input_guard||!controls.focused)return;
    auto p=native_pad(controls.pads[0],usm::mp::gamepads_deadzone());
    auto axis=[](int& out,int v){if(std::abs(v)>std::abs(out))out=v;};
    axis(state.field_10,p.move_x);axis(state.field_14,p.move_y);axis(state.field_18,p.camera_x);axis(state.field_1C,p.camera_y);
    state.m_jump=std::max(state.m_jump,p.jump);state.m_stick_to_walls=std::max(state.m_stick_to_walls,p.wall);
    state.m_punch=std::max(state.m_punch,p.punch);state.m_kick=std::max(state.m_kick,p.kick);
    state.m_black_button=std::max(state.m_black_button,p.black);state.m_throw_web=std::max(state.m_throw_web,p.web);
    state.field_C=std::max(state.field_C,p.left_trigger);state.field_D=std::max(state.field_D,p.right_trigger);
    if(debug_enabled||debug_disabled)p.flags&=~0x10u;
    state.m_flags|=p.flags;
}
void multiplayer_mode_world_shutdown(){using namespace usm::online;if(!installed)return;
    mp::arena::close(true);
    engine_release(true);session.set_local_pose(Pose{});panel=false;editing=-1;seventh=false;active_menu=nullptr;controller_owner=false;controls.gate();
    // Keep host/join and nickname alive across native Continue/loading. Ready is
    // re-advertised only after a valid hero/world is available again.
}
void multiplayer_mode_hero_pack_unloading(){using namespace usm::online;if(!installed)return;
    // A hero change does not destroy the world. Keep any deferred mission-pack
    // ownership records, and preserve the online host/join connection.
    mp::arena::close(false);engine_release(false);session.set_local_pose(Pose{});controls.gate();
}
#else
bool multiplayer_mode_install(){return false;}
void multiplayer_mode_tick_before(float){}
void multiplayer_mode_tick_after(float){}
void multiplayer_mode_world_shutdown(){}
void multiplayer_mode_hero_pack_unloading(){}
void multiplayer_mode_draw_overlay(){}
bool multiplayer_mode_active(){return false;}
bool multiplayer_mode_blocks_world(){return false;}
bool multiplayer_mode_captures_native_input(){return false;}
bool multiplayer_mode_owns_native_pad(unsigned){return false;}
void multiplayer_mode_merge_native_input(unsigned,InputState&){}
#endif
