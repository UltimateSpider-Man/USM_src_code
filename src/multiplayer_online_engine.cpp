#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "multiplayer_online_engine.h"
#if defined(_WIN32) && !defined(OPENUSM_XBPACK_MODE)
#include "multiplayer_pack_lookup.h"
#include "float.hpp"
#include "actor.h"
#include "ai_player_controller.h"
#include "als_meta_anim_base.h"
#include "als_meta_anim_table_shared.h"
#include "als_nal_meta_anim.h"
#include "entity_base_vhandle.h"
#include "fe_mini_map_dot.h"
#include "fe_mini_map_widget.h"
#include "femanager.h"
#include "game.h"
#include "game_level.h"
#include "geometry_manager.h"
#include "igofrontend.h"
#include "igozoomoutmap.h"
#include "mission_manager.h"
#include "mission_stack_manager.h"
#include "nal_anim_controller.h"
#include "nalcomp/nal_instance.h"
#include "ngl.h"
#include "ngl_font.h"
#include "oldmath_po.h"
#include "panelquad.h"
#include "resource_key.h"
#include "resource_manager.h"
#include "resource_pack_location.h"
#include "resource_pack_slot.h"
#include "resource_partition.h"
#include "string_hash.h"
#include "wds.h"
#include <windows.h>
#include <algorithm>
#include <cmath>
#include <vector>

namespace usm::online {
namespace {
struct Remote {
    std::uint32_t id=0,animation=0;
    Model model=Model::spiderman;
    entity_base_vhandle handle{0};
    std::string pack,error;
    double retry_at=0;
    Pose pose{};
    bool drawable=false;
};
struct Marker { fe_mini_map_widget* owner=nullptr;fe_mini_map_dot* dot=nullptr;std::uint32_t id=0; };
struct Candidate {std::string pack,entity;};
std::array<Remote,max_players> remotes{};
std::array<Marker,max_players> markers{};
std::vector<std::string> owned_packs;
std::array<std::vector<Candidate>,unsigned(Model::count)> configured_candidates;
unsigned mutation=0;
struct Mutation { Mutation(){++mutation;} ~Mutation(){--mutation;} Mutation(const Mutation&)=delete;Mutation& operator=(const Mutation&)=delete; };
std::string ini_path,warning;
Pose local_pose{};
bool local_ready=false;
int model_override=-1;
float spawn_distance=250.0f,name_distance=80.0f;
Vec3 as_vec(const vector3d& v){return {v.x,v.y,v.z};}
vector3d native_vec(Vec3 v){return {v.x,v.y,v.z};}
std::string option(const char* group,const char* key,const char* fallback="") {
    char value[192]{};GetPrivateProfileStringA(group,key,fallback,value,sizeof(value),ini_path.c_str());return value;
}
const char* section(Model m) {
    static const char* names[]{"SpiderMan","Venom","Parker","Carnage","BlackSuit"};
    return names[unsigned(m)<unsigned(Model::count)?unsigned(m):0];
}
bool asset_name(const std::string& name) {
    if(name.empty()||name.size()>96)return false;
    for(unsigned char c:name)if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='_'||c=='-'))return false;
    return true;
}
std::vector<Candidate> read_candidates(Model m) {
    std::vector<Candidate> result;
    for(int i=1;i<=6;++i){auto n=std::to_string(i);auto p=option(section(m),("Pack"+n).c_str());auto e=option(section(m),("Entity"+n).c_str());
        if(asset_name(p)&&asset_name(e))result.push_back({p,e});}
    if(!result.empty())return result;
    switch(m){
    case Model::spiderman:return {{"ultimate_spiderman","ultimate_spiderman"},{"ch_ultimate_spiderman","ultimate_spiderman"}};
    case Model::venom:return {{"venom_spider","venom_spider"},{"venom","venom"},{"ch_venom_spider","venom_spider"},{"ch_venom","venom"}};
    case Model::parker:return {{"ch_peter_parker","peter_parker"},{"peter_parker","peter_parker"}};
    case Model::carnage:return {{"carnage","carnage"},{"ch_carnage","carnage"}};
    case Model::blacksuit:return {{"usm_blacksuit","usm_blacksuit"},{"ch_usm_blacksuit","usm_blacksuit"}};
    default:return {};
    }
}
const std::vector<Candidate>& candidates(Model m){return configured_candidates[unsigned(m)];}

actor* hero() {
    if(!g_world_ptr||!g_game_ptr||!g_game_ptr->flag.level_is_loaded||!g_world_ptr->the_terrain)return nullptr;
    auto* e=g_world_ptr->get_hero_ptr(0);
    if(!e||!g_world_ptr->ent_mgr.is_entity_valid(e)||!e->is_an_actor())return nullptr;
    return static_cast<actor*>(e);
}
actor* resolve(const Remote& r) {
    if(!r.handle.field_0||!g_world_ptr)return nullptr;
    auto* e=r.handle.get_volatile_ptr();
    if(!e)return nullptr;
    // Check membership before invoking virtual actor methods. Handles, not cached
    // actor pointers, survive an ordinary entity removal/recreation.
    auto* a=static_cast<actor*>(e);
    return g_world_ptr->ent_mgr.is_entity_valid(a)&&e->is_an_actor()?a:nullptr;
}
fe_mini_map_widget* map_widget(){return g_femanager.IGO?g_femanager.IGO->field_4:nullptr;}
void remove_marker(Marker& m) {
    if(!m.dot)return;
    if(m.owner&&map_widget()==m.owner){
        auto& dots=m.owner->field_364;
        auto it=std::find(dots.begin(),dots.end(),m.dot);
        if(it!=dots.end())dots.erase(it);
        // The native widget owns its list, not these privately allocated dots.
        delete m.dot->field_0;delete m.dot->field_4;delete m.dot;
    }
    // A widget replacement can mean the engine has already torn down its
    // resources. Never dereference a previous widget or its PanelQuads then.
    m={};
}
void destroy(Remote& r) {
    if(auto* a=resolve(r))g_world_ptr->ent_mgr.destroy_entity(a);
    auto retry=r.retry_at;r=Remote{};r.retry_at=retry;
}
void release_unused_packs() {
    if(owned_packs.empty()||!g_world_ptr||!resource_manager::partitions)return;
    auto* stack=mission_stack_manager::s_inst;auto* part=resource_manager::get_partition_pointer(RESOURCE_PARTITION_MISSION);
    if(!stack||!part||stack->waiting_for_push_or_pop())return;
    auto& slots=part->get_pack_slots();
    while(!slots.empty()&&!owned_packs.empty()){
        auto* top=slots.back();if(!top||!top->is_pack_ready())break;
        auto i=std::find_if(owned_packs.begin(),owned_packs.end(),[&](const std::string& p){return top->get_name_key()==resource_key{string_hash{p.c_str()},RESOURCE_KEY_TYPE_PACK};});
        if(i==owned_packs.end())break;
        for(const auto& r:remotes)if(r.handle.field_0&&r.pack==*i)return;
        if(auto* local=hero())if(local->get_resource_context()==top)return;
        mString name{i->c_str()};if(stack->is_pack_pushed(name))stack->pop_mission_pack_immediate(name,name);
        owned_packs.erase(i);
    }
}
struct Context {
    explicit Context(resource_pack_slot* p){resource_manager::push_resource_context(p);}
    ~Context(){resource_manager::pop_resource_context();}
    Context(const Context&)=delete;Context& operator=(const Context&)=delete;
};
resource_pack_slot* acquire(const Candidate& c) {
    if(auto* loaded=mp::find_loaded_actor_pack_context(c.pack,c.entity))return loaded;
    auto* stack=mission_stack_manager::s_inst;auto* part=resource_manager::get_partition_pointer(RESOURCE_PARTITION_MISSION);
    if(!stack||!part||stack->waiting_for_push_or_pop())return nullptr;
    mString name{c.pack.c_str()};resource_key pk{string_hash{c.pack.c_str()},RESOURCE_KEY_TYPE_PACK};
    if(!stack->is_pack_pushed(name)){
        resource_pack_location location{};
        if(!resource_manager::amalgapak_pack_location_table||resource_manager::amalgapak_base_offset==-1||
           !resource_manager::get_pack_file_stats(pk,&location,nullptr,nullptr)||location.loc.m_size<=0||!part->has_room_for_slot(location.loc.m_size))return nullptr;
        // Deferred to the update thread, never the render hook. This is still a
        // synchronous pack load: first-spawn streaming stalls need native profiling.
        stack->push_mission_pack_immediate(name,name);
        if(!stack->is_pack_pushed(name))return nullptr;
        if(std::find(owned_packs.begin(),owned_packs.end(),c.pack)==owned_packs.end())owned_packs.push_back(c.pack);
    }
    return mp::find_loaded_actor_pack_context(c.pack,c.entity);
}
po placement(const Pose& p) {
    po out{po_identity_matrix};const auto b=basis(p.rotation);
    out.set_po(native_vec(b[0]),native_vec(b[1]),native_vec(b[2]),native_vec(p.position));return out;
}
bool spawn(Remote& r,const Identity& id,const Pose& p) {
    for(const auto& c:candidates(p.model)){
        auto* context=acquire(c);if(!context)continue;
        entity* e=nullptr;
        {Context scope(context);e=g_world_ptr->ent_mgr.create_and_add_entity_or_subclass(
            string_hash{c.entity.c_str()},make_unique_entity_id(),placement(p),mString{},1u,nullptr);}
        if(!e)continue;
        if(!e->is_an_actor()){g_world_ptr->ent_mgr.destroy_entity(e);continue;}
        auto* a=static_cast<actor*>(e);
        if(!a->m_skeleton||!a->anim_ctrl){g_world_ptr->ent_mgr.destroy_entity(a);continue;}
        r.id=id.id;r.model=p.model;r.handle=a->get_my_vhandle();r.pack=c.pack;
        a->set_active(false);a->set_visible(false,false);a->set_collisions_active(false,true);
        a->set_character_collisions_active(false);a->set_terrain_collisions_active(false);
        if(auto* controls=a->get_player_controller()){controls->lock_controls(true);controls->clear_controls();}
        return true;
    }
    r.error="Risorsa remota mancante: configura ["+std::string(section(p.model))+"] in multiplayer.ini.";warning=r.error;return false;
}
void animate(Remote& r,actor* a,const Pose& p,float dt) {
    auto* ctl=a->anim_ctrl;if(!ctl)return;
    auto* table=ctl->field_C;auto* context=a->get_resource_context();
    if(!context||!context->is_pack_ready()||!table||!table->field_14||!table->field_0.m_data||table->field_0.size()<=0||table->field_0.size()>8192)return;
    Context scope(context);
    if(p.animation&&p.animation!=r.animation){
        // Never let a remote hash call an unchecked native animation lookup.
        const als::als_nal_meta_anim* selected=nullptr;
        for(int i=0;i<table->field_0.size();++i)if(table->field_14[i].field_8.m_hash==p.animation){selected=&table->field_14[i];break;}
        if(selected){string_hash hash{};hash.source_hash_code=p.animation;
            auto* anim=static_cast<als::als_nal_meta_anim*>(get_anim_by_hash(hash,table,a));
            if(anim&&anim->Skeleton&&ctl->field_8&&ctl->is_same_animtype(anim->Skeleton->GetAnimTypeName())){
                ctl->play_base_layer_anim(hash,0.0f,0x40u,(anim->field_34&1)!=0);
                r.animation=p.animation;
                if(ctl->my_player.field_14[0])ctl->_set_base_anim_time_in_sec(p.animation_time);
            }
        }
    }
    if(ctl->my_player.field_14[0]){
        ctl->_set_base_anim_speed((p.flags&PoseFlags::frozen)?0.0f:p.animation_speed);
        // Occasional bounded phase correction, not a reset on every snapshot.
        const double time=ctl->get_base_anim_time_in_sec();
        if(r.animation==p.animation&&std::isfinite(time)&&std::abs(time-p.animation_time)>.20)ctl->_set_base_anim_time_in_sec(p.animation_time);
        ctl->frame_advance(std::clamp(dt,0.0f,.1f),false,false);
    }
}
void update_marker(unsigned seat,const Identity& id,const Pose& p) {
    auto& m=markers[seat];auto* widget=map_widget();
    if(m.owner!=widget||m.id!=id.id)remove_marker(m);
    if(!widget||!widget->map_icon_others)return;
    if(!m.dot){
        // y=0 avoids the unrelated legacy InitLines constructor path. Real
        // coordinates are assigned below, before the stock map update pass.
        m.dot=new fe_mini_map_dot(mini_map_dot_type{5},vector3d{0,0,0});
        m.dot->field_4=nullptr;m.dot->field_27=false;m.owner=widget;m.id=id.id;
    }
    const auto c=player_color(seat);m.dot->field_14=native_vec(p.position);
    m.dot->field_24=m.dot->field_25=true;m.dot->field_26=false;
    m.dot->field_0->SetColor(color32{c.r,c.g,c.b,255});
}
std::uint32_t argb(unsigned seat,unsigned alpha=255){auto c=player_color(seat);return (alpha<<24)|(unsigned(c.r)<<16)|(unsigned(c.g)<<8)|c.b;}
bool project(Vec3 point,float& x,float& y) {
    const auto& m=geometry_manager::get_xform(geometry_manager::XFORM_WORLD_TO_SCREEN);
    const float w=point.x*m[0][3]+point.y*m[1][3]+point.z*m[2][3]+m.w[3];
    if(!std::isfinite(w)||w<=.001f)return false;
    auto out=sub_501B20(m,native_vec(point));x=out.x;y=out.y;
    return std::isfinite(x)&&std::isfinite(y)&&out.z>0&&x>=0&&y>=0&&x<float(nglGetScreenWidth())&&y<float(nglGetScreenHeight());
}
}
void engine_configure(const std::string& ini) {
    ini_path=ini;for(unsigned m=0;m<unsigned(Model::count);++m)configured_candidates[m]=read_candidates(Model(m));model_override=int(GetPrivateProfileIntA("Online","LocalModel",-1,ini.c_str()));
    if(model_override<0||model_override>=int(Model::count))model_override=-1;
    spawn_distance=float(std::clamp(int(GetPrivateProfileIntA("Online","ActorDistance",250,ini.c_str())),50,500));
    name_distance=float(std::clamp(int(GetPrivateProfileIntA("Online","NameDistance",80,ini.c_str())),10,200));
}
bool engine_capture(Pose& out) {
    local_ready=false;out=Pose{};auto* a=hero();if(!a)return false;
    if(mission_manager::s_inst&&mission_manager::s_inst->is_story_mission_active()>0){warning="Missione storia: replica sospesa. Torna alla citta' libera.";return false;}
    const auto state=g_game_ptr->get_cur_state();if(state!=game_state::RUNNING&&state!=game_state::PAUSED)return false;
    auto* descriptor=g_game_ptr->level.descriptor;if(!descriptor)return false;
    const char* name=descriptor->field_0.to_string();std::size_t len=0;while(len<32&&name[len])++len;
    if(!len)return false;
    out.world=tag_hash(std::string(name,len));out.flags=PoseFlags::ready;
    if(state==game_state::PAUSED)out.flags|=PoseFlags::frozen;
    const auto& transform=a->get_abs_po();out.position=as_vec(transform.get_position());
    out.rotation=from_basis({as_vec(transform.get_x_facing()),as_vec(transform.get_y_facing()),as_vec(transform.get_z_facing())});
    auto v=as_vec(a->get_velocity());out.velocity={std::clamp(v.x,-250.f,250.f),std::clamp(v.y,-250.f,250.f),std::clamp(v.z,-250.f,250.f)};
    if(out.flags&PoseFlags::frozen)out.velocity={};
    if(model_override>=0)out.model=Model(model_override);
    else if(auto* pc=a->get_player_controller()){
        const auto type=pc->find_hero_type();out.model=type==hero_type_enum::CARNAGE?Model::carnage:(type==hero_type_enum::VENOM?Model::venom:(type==hero_type_enum::PARKER?Model::parker:Model::spiderman));
        auto* resource=a->get_resource_context();
        if(resource)for(Model special:{Model::blacksuit,Model::carnage})for(const auto& c:candidates(special))
            if(resource->get_name_key()==resource_key{string_hash{c.pack.c_str()},RESOURCE_KEY_TYPE_PACK})out.model=special;
    }
    // Capture the actual native base clip, not a guessed 'walk/attack' action.
    auto* ctl=a->anim_ctrl;
    if(ctl&&ctl->my_player.field_14[0]&&ctl->my_player.field_14[0]->field_0){
        auto* clip=ctl->my_player.field_14[0]->field_0->field_10;
        if(clip){out.animation=clip->field_8.m_hash;double t=ctl->get_base_anim_time_in_sec(),s=ctl->_get_base_anim_speed();
            if(std::isfinite(t)&&t>=0&&t<=3600)out.animation_time=float(t);
            if(std::isfinite(s)&&s>=0&&s<=8)out.animation_speed=float(s);}
    }
    if(!valid_pose(out)){out=Pose{};return false;}
    local_pose=out;local_ready=true;return true;
}
void engine_update(const Session& session,double now,float dt) {
    Mutation updating;warning.clear();Pose current{};engine_capture(current);
    if(!session.active()||!local_ready){
        for(auto& m:markers)remove_marker(m);
        for(auto& r:remotes)if(r.handle.field_0)destroy(r);
        release_unused_packs();return;
    }
    bool spawned=false;
    for(unsigned i=0;i<max_players;++i){
        auto& r=remotes[i];const auto& person=session.players()[i];Pose p{};
        const bool local=person.identity.id&&person.identity.id==session.local_id();
        bool visible=local?true:session.sample(i,now,p);if(local)p=current;
        visible=visible&&person.identity.id&&p.world==current.world&&(p.flags&PoseFlags::ready);
        if(!visible){remove_marker(markers[i]);if(r.handle.field_0)destroy(r);continue;}
        update_marker(i,person.identity,p);
        if(local)continue;
        if(r.id!=person.identity.id||r.model!=p.model){destroy(r);r.retry_at=0;r.id=person.identity.id;r.model=p.model;}
        auto* a=resolve(r);const float distance2=length2(p.position-current.position);
        if(distance2>(spawn_distance+100)*(spawn_distance+100)){if(a)destroy(r);continue;}
        if(!a&&distance2<spawn_distance*spawn_distance&&!spawned&&now>=r.retry_at){
            spawned=true;r.retry_at=now+5;
            if(spawn(r,person.identity,p))a=resolve(r);
        }
        r.pose=p;r.drawable=a!=nullptr;
        if(!a){if(!r.error.empty())warning=r.error;continue;}
        a->set_active(false);a->set_visible(false,false);auto transform=placement(p);
        entity_set_abs_po(a,transform);animate(r,a,p,dt);
        // Animation root motion must never add a second displacement to a
        // network pose. Native local hero/root motion is left untouched.
        entity_set_abs_po(a,transform);
    }
    release_unused_packs();
}
void engine_draw_world(const Session& session,double now) {
    if(mutation||!session.active()||!local_ready||!hero())return;
    for(auto& r:remotes)if(r.drawable)if(auto* a=resolve(r)){
        a->set_visible(true,false);a->render(1.0f);a->set_visible(false,false);
    }
    const bool large_map=g_femanager.IGO&&g_femanager.IGO->field_44&&g_femanager.IGO->field_44->sub_55F320();
    for(unsigned i=0;i<max_players;++i){const auto& person=session.players()[i];if(!person.identity.id)continue;
        Pose p{};bool local=person.identity.id==session.local_id();
        if(local){if(!large_map)continue;p=local_pose;}else if(!session.sample(i,now,p))continue;
        if(p.world!=local_pose.world||!(p.flags&PoseFlags::ready))continue;
        const float d=std::sqrt(length2(p.position-local_pose.position));if(!large_map&&d>name_distance)continue;
        float x=0,y=0;Vec3 above=p.position;if(!large_map)above.y+=2.4f;
        if(!project(above,x,y))continue;
        unsigned alpha=large_map?255u:unsigned(std::clamp((name_distance-d)/15.f,0.f,1.f)*255);
        draw_rect(x-4,y-4,8,8,argb(i,alpha));
        const float scale=large_map?.65f:.8f;
        draw_text(x-float(person.identity.nickname.size())*3.5f*scale,y-20,person.identity.nickname,scale,argb(i,alpha));
    }
}
void engine_release(bool world_teardown) {
    Mutation releasing;
    for(auto& m:markers)remove_marker(m);
    for(auto& r:remotes)destroy(r);
    release_unused_packs();if(world_teardown)owned_packs.clear();local_ready=false;warning.clear();
}
const std::string& engine_warning(){return warning;}
void draw_rect(float x,float y,float w,float h,std::uint32_t color) {
    nglQuad q{};nglInitQuad(&q);nglSetQuadRect(&q,x,y,x+w,y+h);nglSetQuadColor(&q,color);
    nglSetQuadZ(&q,.1f);nglSetQuadBlend(&q,static_cast<nglBlendModeType>(2),0);nglListAddQuad(&q);
}
void draw_text(float x,float y,const std::string& s,float scale,std::uint32_t color) {
    auto* font=nglSysFont();if(font)nglListAddString(font,s.c_str(),x,y,.05f,color,scale,scale);
}
} // namespace usm::online
#endif
