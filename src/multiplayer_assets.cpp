#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "multiplayer_assets.h"
#if defined(_WIN32) && !defined(OPENUSM_XBPACK_MODE)
#include "multiplayer_geometry.h"
#include "multiplayer_pack_lookup.h"
#include "actor.h"
#include "ai_player_controller.h"
#include "als_meta_anim_base.h"
#include "als_meta_anim_table_shared.h"
#include "als_nal_meta_anim.h"
#include "entity_base_vhandle.h"
#include "game.h"
#include "game_process.h"
#include "mission_stack_manager.h"
#include "nal_anim_controller.h"
#include "oldmath_po.h"
#include "resource_key.h"
#include "resource_manager.h"
#include "resource_pack_location.h"
#include "resource_pack_slot.h"
#include "resource_partition.h"
#include "string_hash.h"
#include "wds.h"
#include <windows.h>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <vector>

namespace usm::mp {
namespace {
struct Native {
    entity_base_vhandle handle{0};
    std::array<std::uint32_t,10> animation{};
    Action last_action=Action::knockout;
    int last_age=-1;
    float scale=1.0f,floor_offset=0;
    std::string pack;
};
std::array<Native,2> native;
std::vector<std::string> owned_packs;
std::string warning;
struct Candidate {std::string pack,entity;};
const char *section(Character c) {
    static const char *s[]{"SpiderMan","Venom","BlackSuit","Carnage"};
    return s[static_cast<unsigned>(c)];
}
std::string option(const std::string &ini,const char *group,const char *key,const char *def="") {
    char value[256]{};GetPrivateProfileStringA(group,key,def,value,sizeof(value),ini.c_str());return value;
}
bool name_ok(const std::string &s) {
    if(s.empty()||s.size()>96)return false;
    for(unsigned char c:s)if(!std::isalnum(c)&&c!='_'&&c!='-')return false;
    return true;
}
actor *resolve(const Native &n) {
    if(!n.handle.field_0||g_world_ptr==nullptr)return nullptr;
    auto *base=n.handle.get_volatile_ptr();
    if(base==nullptr||!base->is_an_actor())return nullptr;
    auto *a=static_cast<actor*>(base);
    return g_world_ptr->ent_mgr.is_entity_valid(a) ? a:nullptr;
}
std::vector<Candidate> candidates(Character c,const std::string &ini) {
    const char *s=section(c);std::vector<Candidate> list;
    for(int n=1;n<=6;++n) {
        std::string p=option(ini,s,("Pack"+std::to_string(n)).c_str());
        std::string e=option(ini,s,("Entity"+std::to_string(n)).c_str());
        if(name_ok(p)&&name_ok(e))list.push_back({p,e});
    }
    if(!list.empty())return list; // Explicit user mappings do not silently fall back.
    switch(c) {
    case Character::spiderman:
        return {{"ultimate_spiderman","ultimate_spiderman"},{"ch_ultimate_spiderman","ultimate_spiderman"},
                {"ch_vwr_ultimate_spiderman_vwr","ultimate_spiderman_vwr"}};
    case Character::venom:
        return {{"venom_spider","venom_spider"},{"venom","venom"},{"ch_venom_spider","venom_spider"},
                {"ch_venom","venom"},{"ch_vwr_venom_viewer","venom_viewer"}};
    case Character::blacksuit:
        return {{"usm_blacksuit","usm_blacksuit"},{"ch_usm_blacksuit","usm_blacksuit"}};
    case Character::carnage:
        return {{"carnage","carnage"},{"ch_carnage","carnage"},{"ch_vwr_carnage","carnage"},
                {"ch_vwr_carnage_vwr","carnage_vwr"}};
    default:return {};
    }
}
void retain_owned(const std::string &p) {
    if(std::find(owned_packs.begin(),owned_packs.end(),p)==owned_packs.end())owned_packs.push_back(p);
}
resource_pack_slot *context_for(const Candidate &candidate) {
    if(auto* loaded=find_loaded_actor_pack_context(candidate.pack,candidate.entity))return loaded;
    auto *stack=mission_stack_manager::s_inst;
    auto *part=resource_manager::get_partition_pointer(RESOURCE_PARTITION_MISSION);
    if(!stack||!part||stack->waiting_for_push_or_pop()){
        log(candidate.pack+" / "+candidate.entity+": mission resource stack unavailable or busy.");return nullptr;
    }
    const resource_key pk{string_hash{candidate.pack.c_str()},RESOURCE_KEY_TYPE_PACK};
    mString pname{candidate.pack.c_str()};
    const bool borrowed=stack->is_pack_pushed(pname);
    if(!borrowed) {
        resource_pack_location loc{};
        if(resource_manager::amalgapak_pack_location_table==nullptr||resource_manager::amalgapak_base_offset==-1
           ||!resource_manager::get_pack_file_stats(pk,&loc,nullptr,nullptr)||loc.loc.m_size<=0){
            log(candidate.pack+" / "+candidate.entity+": pack is not present in the PC archive.");return nullptr;
        }
        if(!part->has_room_for_slot(loc.loc.m_size)){
            log(candidate.pack+" / "+candidate.entity+": not enough room in the mission partition.");return nullptr;
        }
        // Never evict debug/mission/hero packs to make room for a versus match.
        stack->push_mission_pack_immediate(pname,pname);
        if(!stack->is_pack_pushed(pname)){
            log(candidate.pack+" / "+candidate.entity+": native pack push failed.");return nullptr;
        }
        retain_owned(candidate.pack);
    }
    if(auto* loaded=find_loaded_actor_pack_context(candidate.pack,candidate.entity))return loaded;
    log(candidate.pack+" / "+candidate.entity+": entity is not owned by a ready instance of this pack.");
    assets_retry_release();return nullptr;
}
struct ResourceContext {
    explicit ResourceContext(resource_pack_slot *p){resource_manager::push_resource_context(p);}
    ~ResourceContext(){resource_manager::pop_resource_context();}
    ResourceContext(const ResourceContext&)=delete;ResourceContext&operator=(const ResourceContext&)=delete;
};
std::string lower_name(const tlFixedString &f) {
    std::size_t len=0;while(len<sizeof(f.field_4)&&f.field_4[len])++len;
    std::string s(f.field_4,len);for(char &c:s)c=static_cast<char>(std::tolower(static_cast<unsigned char>(c)));return s;
}
int animation_score(const std::string &s,Action action) {
    static const char *terms[10][5]{
        {"idle","stand","stance",nullptr,nullptr},{"run","jog","walk",nullptr,nullptr},
        {"jump_in_air","jump","fall",nullptr,nullptr},{"punch1","punch","attack1","kick1",nullptr},
        {"punch3","uppercut","kick3","heavy","kick"},{"web","tentacle","range","attack3",nullptr},
        {"block","guard",nullptr,nullptr,nullptr},{"dodge","roll",nullptr,nullptr,nullptr},
        {"hitreact","hit_react","hurt","damage",nullptr},{"death","knockout","knockdown","dead",nullptr}};
    int best=0;
    for(int n=0;n<5&&terms[static_cast<unsigned>(action)][n];++n) {
        const char *t=terms[static_cast<unsigned>(action)][n];
        if(s==t)best=std::max(best,100-n*5);
        else if(s.find(t)!=std::string::npos)best=std::max(best,55-n*5);
    }
    for(const char *bad:{"wall","ceiling","crawl","car_","swing","cinema","face","dialog"})
        if(s.find(bad)!=std::string::npos)best-=60;
    return best;
}
void map_animations(Native &record,actor *a,Character c,const std::string &ini) {
    auto *ctl=a->anim_ctrl;auto *table=ctl ? ctl->field_C:nullptr;
    if(!ctl||!table||!table->field_14||!table->field_0.m_data||table->field_0.size()<=0||table->field_0.size()>8192) {
        warning="Native animation table unavailable; see multiplayer.log.";log(warning);return;
    }
    ResourceContext scope(a->get_resource_context());
    for(unsigned action=0;action<10;++action) {
        auto override_name=option(ini,section(c),(std::string("Anim")+action_name(static_cast<Action>(action))).c_str());
        std::uint32_t override_hash=override_name.empty()?0:string_hash{override_name.c_str()}.source_hash_code;
        int best=0;std::uint32_t hash=0;std::string chosen;
        for(int n=0;n<table->field_0.size();++n) {
            const auto &entry=table->field_14[n];const std::string name=lower_name(entry.field_8);
            int score=override_hash ? (entry.field_8.m_hash==override_hash ? 1000:0):animation_score(name,static_cast<Action>(action));
            if(score>best){best=score;hash=entry.field_8.m_hash;chosen=name;}
        }
        if(hash) {
            string_hash key{};key.source_hash_code=hash;
            auto *anim=static_cast<als::als_nal_meta_anim*>(get_anim_by_hash(key,table,a));
            if(!anim||!anim->Skeleton||!ctl->field_8||!ctl->is_same_animtype(anim->Skeleton->GetAnimTypeName()))hash=0;
        }
        record.animation[action]=hash;
        log(std::string(section(c))+" "+action_name(static_cast<Action>(action))+" -> "+(hash?chosen:"MISSING"));
    }
    int missing=0;for(unsigned n=0;n<10;++n)if(!record.animation[n])++missing;
    if(missing)warning="Some native action clips are missing: idle fallback. See multiplayer.log.";
}
bool load_one(int index,Character c,const std::string &ini,std::string &error) {
    for(const auto &candidate:candidates(c,ini)) {
        auto *context=context_for(candidate);if(!context)continue;
        po placement{po_identity_matrix};placement.set_position({index?3.2f:-3.2f,scene_height,0});
        entity *ent=nullptr;
        {ResourceContext scope(context);
            ent=g_world_ptr->ent_mgr.create_and_add_entity_or_subclass(
                string_hash{candidate.entity.c_str()},make_unique_entity_id(),placement,mString{},1u,nullptr);
        }
        if(!ent) {log(candidate.pack+" / "+candidate.entity+": native entity creation failed.");assets_retry_release();continue;}
        if(!ent->is_an_actor()) {log(candidate.pack+" / "+candidate.entity+": entity is not an actor.");g_world_ptr->ent_mgr.destroy_entity(ent);assets_retry_release();continue;}
        auto *a=static_cast<actor*>(ent);
        // Reject incomplete actor resources rather than launching an invisible fighter.
        if (!a->m_skeleton || !a->anim_ctrl) {
            log(candidate.pack+" / "+candidate.entity+": actor has no skeleton or animation controller.");
            g_world_ptr->ent_mgr.destroy_entity(a);assets_retry_release();continue;
        }
        auto &record=native[index];
        record.handle=a->get_my_vhandle();record.pack=candidate.pack;
        a->set_active(false);a->set_visible(true,false);a->set_collisions_active(false,true);
        a->set_character_collisions_active(false);a->set_terrain_collisions_active(false);
        if(a->get_player_controller()){a->get_player_controller()->lock_controls(true);a->get_player_controller()->clear_controls();}
        const auto scale_text=option(ini,section(c),"Scale","1.0");char *end=nullptr;
        float scale=std::strtof(scale_text.c_str(),&end);
        record.scale=end!=scale_text.c_str()&&std::isfinite(scale)?std::clamp(scale,0.1f,4.0f):1.0f;
        a->set_render_scale({record.scale,record.scale,record.scale});
        // The model root offset is configurable; no assumed floor-offset sign.
        const auto offset_text=option(ini,section(c),"FloorOffset","0");float offset=std::strtof(offset_text.c_str(),&end);
        record.floor_offset=std::isfinite(offset)?std::clamp(offset,-5.0f,5.0f):0;
        map_animations(record,a,c,ini);
        if (!record.animation[0]) {
            log(candidate.pack+" / "+candidate.entity+": no compatible idle clip; rejecting unsafe animation playback.");
            g_world_ptr->ent_mgr.destroy_entity(a);record=Native{};assets_retry_release();continue;
        }
        log("P"+std::to_string(index+1)+" loaded "+candidate.pack+" / "+candidate.entity);
        return true;
    }
    error=std::string("P")+std::to_string(index+1)+": no usable "+character_name(c)+" pack/entity. Configure ["+section(c)+"] in multiplayer.ini.";
    log(error);return false;
}
} // namespace

void assets_retry_release() {
    if(owned_packs.empty()||!g_world_ptr||resource_manager::partitions==nullptr)return;
    auto *stack=mission_stack_manager::s_inst;auto *part=resource_manager::get_partition_pointer(RESOURCE_PARTITION_MISSION);
    if(!stack||!part||stack->waiting_for_push_or_pop())return;
    auto &slots=part->get_pack_slots();
    while(!owned_packs.empty()&&!slots.empty()) {
        auto *top=slots.back();if(!top||!top->is_pack_ready())return;
        auto it=std::find_if(owned_packs.begin(),owned_packs.end(),[&](const std::string &p){
            return top->get_name_key()==resource_key{string_hash{p.c_str()},RESOURCE_KEY_TYPE_PACK};});
        if(it==owned_packs.end())return;
        for(const auto &n:native)if(n.handle.field_0&&n.pack==*it)return;
        mString p{it->c_str()};if(stack->is_pack_pushed(p))stack->pop_mission_pack_immediate(p,p);
        owned_packs.erase(it);
    }
}
void assets_release(bool world_teardown) {
    for(int i=1;i>=0;--i){auto *a=resolve(native[i]);if(a)g_world_ptr->ent_mgr.destroy_entity(a);native[i]=Native{};}
    assets_retry_release();if(world_teardown)owned_packs.clear();warning.clear();
}
bool assets_load(const Settings &s,const std::string &ini,std::string &error) {
    // World/managers already exist at the main menu. Actor creation needs a
    // completely loaded level, and must not run during native transitions.
    if(!g_game_ptr||!g_world_ptr||g_game_ptr->the_world!=g_world_ptr
       ||!g_game_ptr->flag.level_is_loaded||!g_world_ptr->the_terrain
       ||g_game_ptr->process_stack.size()==0){
        error="Load a saved game, return to free-roam, then open Multiplayer Mode.";return false;
    }
    const auto state=g_game_ptr->get_cur_state();
    if(state!=game_state::RUNNING&&state!=game_state::PAUSED){
        error="Wait for the game to finish loading, then open Multiplayer Mode.";return false;
    }
    if(resource_manager::partitions==nullptr||!mission_stack_manager::s_inst){error="The PC world/resource managers are not ready.";return false;}
    if(mission_stack_manager::s_inst->waiting_for_push_or_pop()){error="The resource stack is busy. Return to the lobby and retry.";return false;}
    assets_release();
    if(!load_one(0,s.character[0],ini,error)||!load_one(1,s.character[1],ini,error)){assets_release();return false;}
    return true;
}
std::array<actor*,2> assets_actors(){return {resolve(native[0]),resolve(native[1])};}
const std::string &assets_warning(){return warning;}
void assets_pose(const Match &m,float elapsed) {
    for(int i=0;i<2;++i) {
        auto &record=native[i];auto *a=resolve(record);if(!a)continue;const auto &f=m.fighters[i];
        auto *context=a->get_resource_context();if(!context||!context->is_pack_ready())continue;
        ResourceContext scope(context);
        a->set_active(false); // Never enters the native one-player update path.
        po pose{po_identity_matrix};pose.set_rotate_y(f.facing>0?1.57079632679f:-1.57079632679f);
        pose.set_position({f.x*0.001f,scene_height+f.y*0.001f+record.floor_offset,f.z*0.001f});
        entity_set_abs_po(a,pose);
        if(a->anim_ctrl) {
            if(record.last_action!=f.action||f.age<record.last_age) {
                std::uint32_t h=record.animation[static_cast<unsigned>(f.action)];if(!h)h=record.animation[0];
                if(h){string_hash key{};key.source_hash_code=h;a->anim_ctrl->play_base_layer_anim(key,0.0f,0x40u,true);}
                record.last_action=f.action;
            }
            a->anim_ctrl->frame_advance(std::clamp(elapsed,0.0f,0.1f),false,false);
        }
        record.last_age=f.age;
        // Keep any native root-motion output out of the authoritative versus pose.
        entity_set_abs_po(a,pose);
    }
}
} // namespace usm::mp
#endif
