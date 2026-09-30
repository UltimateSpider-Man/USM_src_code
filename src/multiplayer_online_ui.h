#pragma once
#include "multiplayer_online_protocol.h"
#include "multiplayer_gamepad.h"
#include <cmath>
namespace usm::online {
inline bool parse_port(const std::string& s,std::uint16_t& result) {
    if(s.empty()||s.size()>5)return false;unsigned value=0;
    for(char c:s){if(c<'0'||c>'9')return false;value=value*10+unsigned(c-'0');}
    if(!value||value>65535)return false;result=std::uint16_t(value);return true;
}
inline bool valid_ipv4_text(const std::string& s) {
    unsigned count=0;std::size_t start=0;
    while(start<s.size()){
        auto end=s.find('.',start);if(end==std::string::npos)end=s.size();
        const auto n=end-start;if(!n||n>3||(n>1&&s[start]=='0'))return false;
        unsigned value=0;for(auto i=start;i<end;++i){if(s[i]<'0'||s[i]>'9')return false;value=value*10+unsigned(s[i]-'0');}
        if(value>255)return false;++count;
        if(end==s.size())break;start=end+1;if(start==s.size())return false;
    }
    return count==4;
}
// Edit model is independent of Win32 and of the game. Gamepad editing does not
// require OS text injection: arrows select/change characters; face buttons commit.
class Editor {
public:
    std::string value;std::size_t cursor=0;unsigned limit=nickname_limit;bool numeric=false;
    void begin(const std::string& s,unsigned cap,bool numbers){value=s;cursor=value.size();limit=cap;numeric=numbers;}
    bool allowed(char c)const{return numeric?((c>='0'&&c<='9')||c=='.'):nick_character(static_cast<unsigned char>(c));}
    void insert(char c){if(allowed(c)&&value.size()<limit){value.insert(value.begin()+std::ptrdiff_t(cursor),c);++cursor;}}
    void paste(const std::string& s){for(char c:s)insert(c);}
    void backspace(){if(cursor){value.erase(--cursor,1);}}
    void erase(){if(cursor<value.size())value.erase(cursor,1);}
    void move(int delta){cursor=std::size_t(std::clamp(int(cursor)+delta,0,int(value.size())));}
    void cycle(int direction){const std::string alphabet=numeric?"0123456789.":"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_-. ";
        if(cursor==value.size()){if(value.size()>=limit)return;value.push_back(alphabet[0]);return;}
        auto pos=alphabet.find(value[cursor]);int n=pos==std::string::npos?0:int(pos);n=(n+direction+int(alphabet.size()))%int(alphabet.size());value[cursor]=alphabet[std::size_t(n)];}
};
struct NativePad {
    int move_x=0,move_y=0,camera_x=0,camera_y=0;
    std::uint8_t jump=0,wall=0,punch=0,kick=0,black=0,web=0,left_trigger=0,right_trigger=0,flags=0;
};
inline int analog(int value,int deadzone){int d=std::clamp(deadzone,0,25000);int v=std::clamp(value,-32767,32767);
    if(std::abs(v)<=d)return 0;return (v<0?-1:1)*(std::abs(v)-d)*32767/(32767-d);}
inline NativePad native_pad(const mp::pad::Sample& s,int deadzone) {
    NativePad out;if(!s.connected)return out;
    out.move_x=analog(s.x,deadzone);out.move_y=analog(s.y,deadzone);
    out.camera_x=analog(s.rx,deadzone);out.camera_y=analog(s.ry,deadzone);
    const auto b=s.buttons;auto on=[b](std::uint32_t mask)->std::uint8_t{return (b&mask)?255:0;};
    using namespace mp::pad;
    out.jump=on(south);out.wall=on(east);out.punch=on(west);out.kick=on(north);
    out.black=on(l1);out.web=on(r1);out.left_trigger=std::uint8_t(s.lt);out.right_trigger=std::uint8_t(s.rt);
    if(b&start)out.flags|=0x10;if(b&select)out.flags|=0x20;if(b&l3)out.flags|=0x40;if(b&r3)out.flags|=0x80;
    if(b&up)out.flags|=1;if(b&down)out.flags|=2;if(b&left)out.flags|=4;if(b&right)out.flags|=8;
    return out;
}
} // namespace usm::online
