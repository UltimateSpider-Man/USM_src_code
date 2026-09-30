#pragma once
#include "multiplayer_match.h"
#include <cmath>
#include <vector>

namespace usm::mp {
constexpr float scene_height=350.0f; // Same isolation height as the native character viewer.
struct Vec3 {float x=0,y=0,z=0;};
inline Vec3 operator-(Vec3 a,Vec3 b){return {a.x-b.x,a.y-b.y,a.z-b.z};}
inline float dot(Vec3 a,Vec3 b){return a.x*b.x+a.y*b.y+a.z*b.z;}
inline Vec3 cross(Vec3 a,Vec3 b){return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
inline Vec3 unit(Vec3 a){float n=std::sqrt(dot(a,a));return n>0.00001f ? Vec3{a.x/n,a.y/n,a.z/n}:Vec3{0,0,1};}
struct Matrix { float m[4][4]{}; };
inline Matrix look_at(Vec3 eye,Vec3 at) {
    Vec3 z=unit(at-eye),x=unit(cross({0,1,0},z)),y=cross(z,x);
    return {{{x.x,y.x,z.x,0},{x.y,y.y,z.y,0},{x.z,y.z,z.z,0},
             {-dot(x,eye),-dot(y,eye),-dot(z,eye),1}}};
}
struct Rect {int x=0,y=0,w=0,h=0;};
enum class Layout : unsigned { shared, horizontal, vertical, ds_panels, count };
inline const char* layout_name(Layout l) {
    constexpr const char* names[]{"SHARED CAMERA","TOP / BOTTOM","SIDE BY SIDE","DS-STYLE PANELS"};
    return static_cast<unsigned>(l)<4 ? names[static_cast<unsigned>(l)]:names[0];
}
struct Views {std::array<Rect,2> play{};std::array<Rect,2> cards{};int count=1;};
inline Views make_views(int w,int h,Layout mode) {
    Views v; if(w<4||h<4)return v;
    const int gap=2;
    if(mode==Layout::shared) {v.play[0]={0,0,w,h};return v;}
    v.count=2;
    if(mode==Layout::horizontal) {
        int half=(h-gap)/2;v.play[0]={0,0,w,half};v.play[1]={0,half+gap,w,h-half-gap};
    } else if(mode==Layout::vertical) {
        int half=(w-gap)/2;v.play[0]={0,0,half,h};v.play[1]={half+gap,0,w-half-gap,h};
    } else {
        int ww=(w-gap)/2,hh=(h-gap)/2;
        v.play[0]={0,0,ww,hh};v.play[1]={ww+gap,0,w-ww-gap,hh};
        v.cards[0]={0,hh+gap,ww,h-hh-gap};v.cards[1]={ww+gap,hh+gap,w-ww-gap,h-hh-gap};
    }
    return v;
}
inline Matrix camera_for(const Match &m,int player,Rect view,bool shared) {
    const auto &a=m.fighters[0];const auto &b=m.fighters[1];
    float middle=(a.x+b.x)*0.0005f;
    float center=shared ? middle:middle*0.35f+m.fighters[player].x*0.00065f;
    const float halfSpan=std::abs(a.x-b.x)*0.0005f+3.5f+std::abs(center-middle);
    const float aspect=view.h>0 ? static_cast<float>(view.w)/view.h:1.33f;
    const float distance=std::max(halfSpan/0.64f,6.0f*aspect);
    float targetY=scene_height+1.35f+std::max(a.y,b.y)*0.0004f;
    return look_at({center,targetY+distance*0.16f,-distance},{center,targetY,0});
}
// D3DFVF_XYZ | D3DFVF_DIFFUSE, explicitly no engine/pointer dependency.
struct Vertex {float x,y,z;std::uint32_t color;};
static_assert(sizeof(Vertex)==16,"Unexpected render vertex ABI");
inline std::uint32_t shade(std::uint32_t c,unsigned n) {
    auto ch=[&](unsigned shift){return (((c>>shift)&255u)*n/100u)<<shift;};
    return (c&0xff000000u)|ch(16)|ch(8)|ch(0);
}
inline void box(std::vector<Vertex>& v,float x,float y,float z,float w,float h,float d,std::uint32_t c) {
    Vec3 p[8]{{x-w/2,y,z-d/2},{x+w/2,y,z-d/2},{x+w/2,y+h,z-d/2},{x-w/2,y+h,z-d/2},
              {x-w/2,y,z+d/2},{x+w/2,y,z+d/2},{x+w/2,y+h,z+d/2},{x-w/2,y+h,z+d/2}};
    constexpr int f[6][4]{{0,1,2,3},{5,4,7,6},{4,0,3,7},{1,5,6,2},{3,2,6,7},{4,5,1,0}};
    constexpr unsigned light[6]{88,65,70,94,100,55};
    for(int face=0;face<6;++face)for(int n:{0,1,2,0,2,3}) {
        const auto &q=p[f[face][n]];v.push_back({q.x,q.y+scene_height,q.z,shade(c,light[face])});
    }
}
inline std::vector<Vertex> arena_geometry(Arena arena) {
    std::vector<Vertex> v;v.reserve(10000);const auto s=stage_for(arena);
    float w=s.half_width*0.002f,d=s.half_depth*0.002f;
    std::uint32_t floor=arena==Arena::football ? 0xff326743u:arena==Arena::warehouse ? 0xff515b68u:0xff555b65u;
    box(v,0,-0.3f,0,w,0.3f,d,floor);
    if(arena==Arena::warehouse) {
        box(v,0,0,4.5f,w+3,6,0.5f,0xff474354);
        for(int x=-15;x<=15;x+=5){box(v,float(x),0,4,0.28f,6,0.28f,0xffac8a48);}
        for(int x=-12;x<=12;x+=6) {
            box(v,float(x),0,3.5f,4,1.8f,1,0xff6b8092);
            box(v,float(x),1.85f,3.5f,4,1.8f,1,0xff98734d);
        }
    } else if(arena==Arena::subway) {
        box(v,0,0,5.5f,w+8,5,0.5f,0xff78858b);
        for(int x=-15;x<=15;x+=5){box(v,float(x),0,4.2f,0.4f,4.6f,0.4f,0xff39817e);}
        for(float z:{2.05f,3.25f})box(v,0,0.025f,z,w,0.08f,0.12f,0xffb3b4b5);
        box(v,0,0.02f,1.6f,w,0.02f,0.2f,0xffe7c355);
        for(int x=-14;x<=14;x+=7)box(v,float(x),2.5f,5.19f,3,0.65f,0.04f,0xff172a38);
    } else if(arena==Arena::bridge) {
        for(float z:{-3.5f,3.5f}) {
            for(int x=-18;x<=18;x+=3)box(v,float(x),0,z,0.15f,1.1f,0.15f,0xff999ab1);
            box(v,0,0.95f,z,w+1,0.15f,0.2f,0xffb6b3bb);
        }
        for(int x:{-12,12}) {
            box(v,float(x),0,6,0.8f,12,1,0xffa38872);
            box(v,float(x),9.5f,6,7,0.7f,1,0xffb09c83);
        }
        for(int x=-25;x<=25;x+=5)box(v,float(x),-4,17,3.5f,5+float((x+25)%4),3,0xff615d79);
        box(v,0,0.02f,0,w,0.02f,0.12f,0xffc7ba82);
    } else {
        for(int x=-16;x<=16;x+=4) {
            box(v,float(x),0.006f,0,0.07f,0.012f,d,0xffe7e8d1);
            for(int z=-4;z<=4;z+=2)box(v,float(x)+1,0.008f,float(z),0.5f,0.012f,0.06f,0xffe7e8d1);
        }
        for(float z:{-4.85f,4.85f})box(v,0,0.008f,z,w,0.012f,0.08f,0xffefefe0);
        for(int x:{-18,18}){
            box(v,float(x),0,4.8f,0.15f,3.5f,0.15f,0xffffd55d);
            box(v,float(x),3.5f,4.8f,3,0.13f,0.13f,0xffffd55d);
            for(float dx:{-1.5f,1.5f})box(v,float(x)+dx,3.5f,4.8f,0.13f,2,0.13f,0xffffd55d);
        }
        for(int row=0;row<5;++row)box(v,0,float(row)*0.55f,7+float(row),w+6,0.4f,0.9f,0xff797784);
    }
    for(unsigned i=0;i<s.count;++i){const auto&p=s.platforms[i];
        box(v,(p.x0+p.x1)*0.0005f,0,(p.z0+p.z1)*0.0005f,(p.x1-p.x0)*0.001f,p.top*0.001f,(p.z1-p.z0)*0.001f,0xffb28751);
    }
    return v;
}
inline std::vector<Vertex> match_effects(const Match &m) {
    std::vector<Vertex> v;v.reserve(700);
    for(int i=0;i<2;++i) {
        const auto&f=m.fighters[i];float x=f.x*0.001f,y=f.y*0.001f,z=f.z*0.001f;
        box(v,x,0.014f,z,0.9f,0.015f,0.65f,0xff22252b);
        if(f.action==Action::guard)box(v,x+f.facing*0.65f,y+0.65f,z,0.08f,1.2f,0.65f,0xff699ecd);
        if(is_attack(f.action)) {
            const auto a=attack_for(f.action,f.character);
            if(f.age>=a.startup&&f.age<a.startup+a.active) {
                float reach=a.reach*0.001f;
                std::uint32_t color=f.character==Character::carnage?0xffc93347u:f.character==Character::venom?0xff7563aau:0xffc4daebu;
                box(v,x+f.facing*reach/2,y+1.1f,z,reach,0.055f,0.06f,color);
            }
        }
    }
    if(m.settings.hazards&&m.settings.arena==Arena::subway) {
        int phase=m.fight_ticks%900;
        if(phase>=780&&phase<840) {
            float x=-28+(phase-780)*0.96f;
            box(v,x,0.1f,2.8f,12,2.4f,1.2f,0xffb8bbc2);
            for(int n=-4;n<=4;n+=2)box(v,x+n,1.25f,2.17f,1.3f,0.8f,0.02f,0xff253847);
        }
    }
    return v;
}
} // namespace usm::mp
