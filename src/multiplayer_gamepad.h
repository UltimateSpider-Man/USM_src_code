#pragma once
// Portable controller mapping/state. No native game addresses or Windows types.
#include "multiplayer_match.h"
#include <algorithm>
#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace usm::mp::pad {
enum Key : std::uint32_t {
    up=1u<<0, down=1u<<1, left=1u<<2, right=1u<<3,
    south=1u<<4, east=1u<<5, west=1u<<6, north=1u<<7,
    l1=1u<<8, r1=1u<<9, select=1u<<10, start=1u<<11,
    l2=1u<<12, r2=1u<<13, l3=1u<<14, r3=1u<<15, guide=1u<<16
};
constexpr std::uint32_t directions=up|down|left|right;
struct Sample {
    bool connected=false;
    std::uint32_t buttons=0;
    int x=0,y=0; // [-32767,32767]; positive Y is UP.
    std::string id,name;
    int rx=0,ry=0; // Native camera axes, same range/sign convention.
    unsigned lt=0,rt=0; // [0,255], independent triggers.
};
struct Mapping {
    // Button order: south, east, west, north, L1, R1, select, start, L2, R2, L3, R3.
    // Internally zero-based; -1 disables a binding. INI uses joy.cpl's 1-based labels.
    std::array<int,12> button{{0,1,2,3,4,5,6,7,-1,-1,8,9}};
    std::array<int,4> dpad_button{{-1,-1,-1,-1}};
    int axis_x=0,axis_y=1,pov=0;
    int axis_rx=3,axis_ry=4;
    int guide_button=-1; // PS button; disabled for unknown generic layouts.
    bool invert_x=false,invert_y=true,invert_rx=false,invert_ry=true;
};
inline bool sony_id(unsigned vid,unsigned pid) {
    return vid==0x054c && (pid==0x05c4||pid==0x09cc||pid==0x0ba0||pid==0x0ce6||pid==0x0df2);
}
inline Mapping default_mapping(bool playstation) {
    Mapping m;
    if(playstation){m.button={{1,2,0,3,4,5,8,9,6,7,10,11}};m.axis_rx=2;m.axis_ry=5;m.guide_button=12;}
    return m;
}
inline int normalize_axis(std::int64_t value,std::int64_t minimum,std::int64_t maximum) {
    if(maximum<=minimum)return 0;
    value=std::clamp(value,minimum,maximum);
    return static_cast<int>(((value-minimum)*65534)/(maximum-minimum)-32767);
}
inline std::uint32_t pov_buttons(std::uint32_t angle) {
    if(angle==0xffffffffu||angle==0xffffu||angle>=36000u)return 0;
    constexpr std::uint32_t compass[]{up,up|right,right,right|down,down,down|left,left,left|up};
    return compass[((angle+2250u)/4500u)%8u];
}
struct DirectSample {
    std::array<int,8> axis{}; // Already normalized; absent axes MUST be zero.
    std::array<std::uint8_t,128> button{};
    std::array<std::uint32_t,4> pov{{0xffffffffu,0xffffffffu,0xffffffffu,0xffffffffu}};
};
inline Sample map_direct(const DirectSample &s,const Mapping &m) {
    Sample out;out.connected=true;
    constexpr std::uint32_t bits[]{south,east,west,north,l1,r1,select,start,l2,r2,l3,r3};
    auto held=[&](int i){return i>=0&&i<int(s.button.size())&&(s.button[std::size_t(i)]&0x80u);};
    for(std::size_t n=0;n<m.button.size();++n)if(held(m.button[n]))out.buttons|=bits[n];
    if(held(m.guide_button))out.buttons|=guide;
    constexpr std::uint32_t dbits[]{up,down,left,right};
    for(std::size_t n=0;n<4;++n)if(held(m.dpad_button[n]))out.buttons|=dbits[n];
    if(m.pov>=0&&m.pov<4)out.buttons|=pov_buttons(s.pov[std::size_t(m.pov)]);
    auto axis=[&](int n){return n>=0&&n<8?std::clamp(s.axis[std::size_t(n)],-32767,32767):0;};
    out.x=axis(m.axis_x)*(m.invert_x?-1:1);out.y=axis(m.axis_y)*(m.invert_y?-1:1);
    out.rx=axis(m.axis_rx)*(m.invert_rx?-1:1);out.ry=axis(m.axis_ry)*(m.invert_ry?-1:1);
    out.lt=(out.buttons&l2)?255:0;out.rt=(out.buttons&r2)?255:0;
    return out;
}
inline Sample map_xinput(std::uint16_t buttons,int x,int y,int rx=0,int ry=0,unsigned lt=0,unsigned rt=0) {
    Sample out;out.connected=true;out.x=std::clamp(x,-32767,32767);out.y=std::clamp(y,-32767,32767);
    out.rx=std::clamp(rx,-32767,32767);out.ry=std::clamp(ry,-32767,32767);
    out.lt=std::min(lt,255u);out.rt=std::min(rt,255u);
    if(out.lt>30)out.buttons|=l2;
    if(out.rt>30)out.buttons|=r2;
    constexpr std::uint16_t xb[]{0x0001,0x0002,0x0004,0x0008,0x1000,0x2000,0x4000,0x8000,0x0100,0x0200,0x0020,0x0010,0x0040,0x0080};
    constexpr std::uint32_t pb[]{up,down,left,right,south,east,west,north,l1,r1,select,start,l3,r3};
    for(std::size_t n=0;n<14;++n)if(buttons&xb[n])out.buttons|=pb[n];
    return out;
}
inline Input match_input(std::uint32_t buttons) {
    Input out;
    constexpr std::uint32_t p[]{left,right,up,down,south,west,north,east,l1,r1};
    for(unsigned n=0;n<10;++n)if(buttons&p[n])out.held|=std::uint16_t(1u<<n);
    // Opposite directions cancel instead of depending on branch order in the simulation.
    if((out.held&(Button::left|Button::right))==(Button::left|Button::right))out.held&=~(Button::left|Button::right);
    if((out.held&(Button::forward|Button::backward))==(Button::forward|Button::backward))out.held&=~(Button::forward|Button::backward);
    return out;
}
class Tracker {
    std::uint32_t previous_=0,blocked_=0,raw_=0,stick_=0;
    bool connected_=false,focused_=false;
    std::string id_;
    std::array<double,4> held_time_{};
    std::array<double,4> next_repeat_{{0.35,0.35,0.35,0.35}};
public:
    std::uint32_t held=0,pressed=0,navigation=0;
    int deadzone=9000;
    void gate() {
        blocked_|=raw_;held=pressed=navigation=previous_=0;
        held_time_.fill(0);next_repeat_.fill(0.35);
    }
    void poll(const Sample &s,bool focused,double dt) {
        const int dz=std::clamp(deadzone,2000,25000),release=dz*2/3;
        auto axis=[&](int value,std::uint32_t positive,std::uint32_t negative){
            if(value>dz) {stick_|=positive;stick_&=~negative;}
            else if(value<-dz) {stick_|=negative;stick_&=~positive;}
            else {if(value<release)stick_&=~positive;if(value>-release)stick_&=~negative;}
        };
        if(!s.connected||s.id!=id_)stick_=0;
        axis(s.x,right,left);axis(s.y,up,down);
        raw_=s.connected?(s.buttons|stick_):0;
        // First acquisition, identity change and focus recovery all require release.
        if(s.connected&&(!connected_||s.id!=id_||(focused&&!focused_)))blocked_|=raw_;
        blocked_&=raw_;
        const std::uint32_t value=s.connected&&focused?(raw_&~blocked_):0;
        held=value;pressed=value&~previous_;navigation=pressed&directions;
        constexpr std::uint32_t keys[]{up,down,left,right};
        const double elapsed=dt>0&&dt<0.25?dt:0.0;
        for(unsigned n=0;n<4;++n) {
            if(!(value&keys[n])) {held_time_[n]=0;next_repeat_[n]=0.35;}
            else if(previous_&keys[n]) {
                held_time_[n]+=elapsed;
                if(held_time_[n]>=next_repeat_[n]) {navigation|=keys[n];next_repeat_[n]=held_time_[n]+0.10;}
            }
        }
        previous_=value;connected_=s.connected;focused_=focused;id_=s.id;
    }
};
// Two logical seats, never reorder a surviving player when another pad disappears.
class Seats {
    std::array<std::string,2> id_{};
    bool locked_=false;
public:
    void lock(bool value){locked_=value;}
    void clear(){id_={};locked_=false;}
    std::array<Sample,2> update(const std::vector<Sample> &devices) {
        auto find=[&](const std::string &id)->const Sample*{
            for(const auto &s:devices)if(s.connected&&!id.empty()&&s.id==id)return &s;
            return nullptr;
        };
        if(!locked_) {
            for(auto &id:id_)if(!find(id))id.clear();
            // Lobby-only compact: one available pad must be usable as controller 1.
            if(id_[0].empty()&&!id_[1].empty()){id_[0]=id_[1];id_[1].clear();}
            for(auto &id:id_)if(id.empty())for(const auto &s:devices)
                if(s.connected&&!s.id.empty()&&s.id!=id_[0]&&s.id!=id_[1]){id=s.id;break;}
        }
        std::array<Sample,2> out;
        for(unsigned n=0;n<2;++n){if(auto *s=find(id_[n]))out[n]=*s;else out[n].id=id_[n];}
        return out;
    }
};
// Existing presets 0..2 retained. 3 = P1 pad + P2 keyboard; 4 = auto.
struct Assignment {
    std::array<int,2> controller{{-1,-1}}; // -1 selects that player's keyboard layout.
    bool lan_pad=false;
    unsigned required=0; // Logical-seat bitmask, frozen at match start.
};
inline Assignment assign(int preset,int connected_count,bool lan) {
    Assignment a;
    if(lan){a.lan_pad=preset==2||preset==3||(preset==4&&connected_count>0);a.required=a.lan_pad?1u:0u;return a;}
    if(preset==4)preset=connected_count>=2?2:connected_count==1?3:0;
    if(preset==1){a.controller[1]=0;a.required=1;}
    if(preset==2){a.controller={{0,1}};a.required=3;}
    if(preset==3){a.controller[0]=0;a.required=1;}
    return a;
}
inline bool missing(const Assignment &a,const std::array<bool,2> &connected) {
    return ((a.required&1u)&&!connected[0])||((a.required&2u)&&!connected[1]);
}
enum class OverlayAction { resume,rematch,lobby,close };
inline int overlay_count(bool finished,bool network){return network?2:finished?3:4;}
inline OverlayAction overlay_action(int row,bool finished,bool network) {
    row=std::clamp(row,0,overlay_count(finished,network)-1);
    if(network)return row==0?OverlayAction::lobby:OverlayAction::close;
    if(finished)++row;
    constexpr OverlayAction a[]{OverlayAction::resume,OverlayAction::rematch,OverlayAction::lobby,OverlayAction::close};
    return a[row];
}
inline const char *overlay_label(OverlayAction a) {
    switch(a){case OverlayAction::resume:return "RESUME MATCH";case OverlayAction::rematch:return "REMATCH";
    case OverlayAction::lobby:return "CHARACTER SELECT / LOBBY";case OverlayAction::close:return "EXIT MULTIPLAYER";}
    return "";
}
// The seventh row is virtual: never put 6 in the native six-entry array index.
struct MenuPosition { int native=0;bool extra=false; };
inline MenuPosition menu_step(MenuPosition p,int direction) {
    int row=p.extra?6:std::clamp(p.native,0,5);
    row=(row+(direction<0?6:1))%7;
    return {row==6?p.native:row,row==6};
}
} // namespace usm::mp::pad
