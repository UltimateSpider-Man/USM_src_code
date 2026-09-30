#pragma once
// Online-session wire format. No game pointers, native structs or platform ABI
// cross the network. All integers/floats are explicitly encoded big endian.
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <deque>
#include <string>
#include <vector>

namespace usm::online {
constexpr std::uint32_t magic = 0x55534d4f; // USMO, not the old arena protocol
constexpr std::uint16_t version = 1;
constexpr unsigned max_players = 8;
constexpr unsigned nickname_limit = 24;
constexpr unsigned max_packet = 1200;
constexpr std::uint32_t build_id = 0x20260918;
using Bytes = std::vector<std::uint8_t>;
inline bool newer(std::uint32_t a, std::uint32_t b) { return a != b && a-b < 0x80000000u; }
inline bool nick_character(unsigned char c) {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
           (c >= '0' && c <= '9') || c == ' ' || c == '_' || c == '-' || c == '.';
}
inline bool valid_nickname(const std::string& s) {
    if (s.empty() || s.size() > nickname_limit || s.front()==' ' || s.back()==' ') return false;
    for (unsigned char c : s) if (!nick_character(c)) return false;
    return true;
}
inline std::uint32_t tag_hash(const std::string& s) {
    std::uint32_t h=2166136261u;
    for (unsigned char c:s) { h ^= c; h *= 16777619u; }
    return h;
}
struct RGB { std::uint8_t r,g,b; };
inline RGB player_color(unsigned seat) {
    // Session seat, NOT the local controller index: host stays blue everywhere.
    constexpr RGB colors[]{{55,140,255},{255,146,45},{80,215,120},{210,105,245},
                           {255,90,115},{55,220,220},{240,215,65},{235,235,235}};
    return colors[seat % max_players];
}
struct Vec3 { float x=0,y=0,z=0; };
inline Vec3 operator+(Vec3 a,Vec3 b){return {a.x+b.x,a.y+b.y,a.z+b.z};}
inline Vec3 operator-(Vec3 a,Vec3 b){return {a.x-b.x,a.y-b.y,a.z-b.z};}
inline Vec3 operator*(Vec3 a,float s){return {a.x*s,a.y*s,a.z*s};}
inline float length2(Vec3 v){return v.x*v.x+v.y*v.y+v.z*v.z;}
struct Quat { float x=0,y=0,z=0,w=1; };
inline float dot(Quat a,Quat b){return a.x*b.x+a.y*b.y+a.z*b.z+a.w*b.w;}
inline Quat normalized(Quat q) {
    float n=std::sqrt(dot(q,q));
    return n>1e-8f && std::isfinite(n) ? Quat{q.x/n,q.y/n,q.z/n,q.w/n}:Quat{};
}
inline Quat slerp(Quat a,Quat b,float t) {
    a=normalized(a);b=normalized(b);float d=dot(a,b);
    if(d<0){b={-b.x,-b.y,-b.z,-b.w};d=-d;}
    float u=1-t,v=t;
    if(d<0.9995f){float angle=std::acos(std::clamp(d,-1.0f,1.0f));float s=std::sin(angle);u=std::sin((1-t)*angle)/s;v=std::sin(t*angle)/s;}
    return normalized({a.x*u+b.x*v,a.y*u+b.y*v,a.z*u+b.z*v,a.w*u+b.w*v});
}
// Rotation bases are rows, matching po::get_[xyz]_facing().
inline std::array<Vec3,3> basis(Quat q) {
    q=normalized(q);float x=q.x,y=q.y,z=q.z,w=q.w;
    return {{{1-2*y*y-2*z*z,2*x*y+2*z*w,2*x*z-2*y*w},
             {2*x*y-2*z*w,1-2*x*x-2*z*z,2*y*z+2*x*w},
             {2*x*z+2*y*w,2*y*z-2*x*w,1-2*x*x-2*y*y}}};
}
inline Quat from_basis(const std::array<Vec3,3>& m) {
    const float a=m[0].x,b=m[0].y,c=m[0].z,d=m[1].x,e=m[1].y,f=m[1].z,g=m[2].x,h=m[2].y,i=m[2].z;
    Quat q;float tr=a+e+i;
    if(tr>0){float s=std::sqrt(tr+1)*2;q={ (f-h)/s,(g-c)/s,(b-d)/s,s/4};}
    else if(a>e && a>i){float s=std::sqrt(std::max(0.0f,1+a-e-i))*2;if(s<1e-6f)return {};q={s/4,(b+d)/s,(c+g)/s,(f-h)/s};}
    else if(e>i){float s=std::sqrt(std::max(0.0f,1+e-a-i))*2;if(s<1e-6f)return {};q={(b+d)/s,s/4,(f+h)/s,(g-c)/s};}
    else {float s=std::sqrt(std::max(0.0f,1+i-a-e))*2;if(s<1e-6f)return {};q={(c+g)/s,(f+h)/s,s/4,(b-d)/s};}
    return normalized(q);
}
enum class Model:std::uint8_t { spiderman,venom,parker,carnage,blacksuit,count };
enum PoseFlags:std::uint8_t { ready=1,frozen=2,teleport=4 };
struct Pose {
    std::uint32_t sequence=0,world=0,animation=0;
    Vec3 position{},velocity{};
    Quat rotation{};
    float animation_time=0,animation_speed=1;
    Model model=Model::spiderman;
    std::uint8_t flags=0;
};
inline bool valid_pose(const Pose& p) {
    auto finite=[](float x,float limit){return std::isfinite(x) && std::abs(x)<=limit;};
    float n=dot(p.rotation,p.rotation);
    return finite(p.position.x,100000)&&finite(p.position.y,100000)&&finite(p.position.z,100000)&&
           finite(p.velocity.x,250)&&finite(p.velocity.y,250)&&finite(p.velocity.z,250)&&
           std::isfinite(n)&&n>=0.5f&&n<=1.5f &&
           finite(p.animation_time,3600)&&p.animation_time>=0&&finite(p.animation_speed,8)&&p.animation_speed>=0 &&
           unsigned(p.model)<unsigned(Model::count) && (p.flags & ~7u)==0;
}
struct Identity { std::uint32_t id=0; std::uint8_t seat=0; std::string nickname; };
class Writer {
public:
    Bytes bytes;
    void u8(std::uint8_t x){bytes.push_back(x);}
    void u16(std::uint16_t x){u8(std::uint8_t(x>>8));u8(std::uint8_t(x));}
    void u32(std::uint32_t x){u16(std::uint16_t(x>>16));u16(std::uint16_t(x));}
    void u64(std::uint64_t x){u32(std::uint32_t(x>>32));u32(std::uint32_t(x));}
    void f32(float x){static_assert(sizeof(float)==4);std::uint32_t v;std::memcpy(&v,&x,4);u32(v);}
    void text(const std::string& x){u8(std::uint8_t(x.size()));bytes.insert(bytes.end(),x.begin(),x.end());}
    void vec(Vec3 x){f32(x.x);f32(x.y);f32(x.z);}
    void pose(const Pose& p){u32(p.sequence);u32(p.world);u32(p.animation);vec(p.position);vec(p.velocity);
        f32(p.rotation.x);f32(p.rotation.y);f32(p.rotation.z);f32(p.rotation.w);
        f32(p.animation_time);f32(p.animation_speed);u8(unsigned(p.model));u8(p.flags);}
};
class Reader {
    const std::uint8_t* p_;std::size_t size_,at_=0;bool ok_=true;
public:
    Reader(const void* p,std::size_t n):p_(static_cast<const std::uint8_t*>(p)),size_(n){}
    explicit Reader(const Bytes& b):Reader(b.data(),b.size()){}
    std::uint8_t u8(){if(at_>=size_){ok_=false;return 0;}return p_[at_++];}
    std::uint16_t u16(){auto a=u8();auto b=u8();return std::uint16_t((a<<8)|b);}
    std::uint32_t u32(){auto a=u16();auto b=u16();return (std::uint32_t(a)<<16)|b;}
    std::uint64_t u64(){auto a=u32();auto b=u32();return (std::uint64_t(a)<<32)|b;}
    float f32(){auto u=u32();float f;std::memcpy(&f,&u,4);return f;}
    std::string text(unsigned max){unsigned n=u8();if(!ok_||n>max||n>size_-at_){ok_=false;return {};}
        std::string s(reinterpret_cast<const char*>(p_+at_),n);at_+=n;return s;}
    Vec3 vec(){float x=f32(),y=f32(),z=f32();return {x,y,z};}
    Pose pose(){Pose p;p.sequence=u32();p.world=u32();p.animation=u32();p.position=vec();p.velocity=vec();
        p.rotation.x=f32();p.rotation.y=f32();p.rotation.z=f32();p.rotation.w=f32();
        p.animation_time=f32();p.animation_speed=f32();p.model=Model(u8());p.flags=u8();return p;}
    bool done()const{return ok_&&at_==size_;}
    bool good()const{return ok_;}
};
enum class Control:std::uint8_t { hello=1,welcome,roster,ping,pong,error,bye };
enum class Datagram:std::uint8_t { pose=1,snapshot };
inline Writer control(Control kind){Writer w;w.u32(magic);w.u16(version);w.u8(unsigned(kind));return w;}
inline bool control_header(Reader& r,Control& c){auto m=r.u32();auto v=r.u16();c=Control(r.u8());return r.good()&&m==magic&&v==version&&unsigned(c)>=1&&unsigned(c)<=7;}
inline Bytes frame(const Writer& w){Writer h;h.u16(std::uint16_t(w.bytes.size()));h.bytes.insert(h.bytes.end(),w.bytes.begin(),w.bytes.end());return h.bytes;}

// Bounded jitter buffer, continuous orientation for walls/ceiling/swinging.
// Not an authoritative native-physics predictor or gameplay rollback system.
class History {
    struct Sample {double time;Pose pose;};
    std::deque<Sample> samples_;
public:
    void clear(){samples_.clear();}
    std::size_t size()const{return samples_.size();}
    bool push(const Pose& p,double time) {
        if(!valid_pose(p)||!std::isfinite(time))return false;
        if(!samples_.empty()) {
            const auto& last=samples_.back();
            if(!newer(p.sequence,last.pose.sequence)||time<last.time)return false;
            if(p.world!=last.pose.world||p.model!=last.pose.model||((p.flags^last.pose.flags)&ready)||(p.flags&teleport)||length2(p.position-last.pose.position)>2500)
                samples_.clear();
        }
        samples_.push_back({time,p});while(samples_.size()>32)samples_.pop_front();return true;
    }
    bool sample(double time,Pose& out,double max_extrapolation=.10)const {
        if(samples_.empty()||!std::isfinite(time))return false;
        const auto& first=samples_.front();const auto& last=samples_.back();
        if(time<=first.time){out=first.pose;return true;}
        for(std::size_t i=1;i<samples_.size();++i) {
            const auto& a=samples_[i-1];const auto& b=samples_[i];
            if(time>b.time)continue;
            float t=b.time>a.time?float((time-a.time)/(b.time-a.time)):1;
            out=a.pose;out.position=a.pose.position*(1-t)+b.pose.position*t;
            out.velocity=a.pose.velocity*(1-t)+b.pose.velocity*t;out.rotation=slerp(a.pose.rotation,b.pose.rotation,t);
            if(a.pose.animation==b.pose.animation && b.pose.animation_time>=a.pose.animation_time)
                out.animation_time=a.pose.animation_time*(1-t)+b.pose.animation_time*t;
            else if(t>=1)out=b.pose;
            return true;
        }
        out=last.pose;float dt=float(std::clamp(time-last.time,0.0,std::max(0.0,max_extrapolation)));
        if(!(out.flags&frozen)){out.position=out.position+out.velocity*dt;out.animation_time=std::min(3600.0f,out.animation_time+dt*out.animation_speed);}
        return true;
    }
};
} // namespace usm::online
