#include "script_lib_debug_menu.h"

#include "collide.h"
#include "damage_interface.h"
#include "debug_menu.h"
#include "entity.h"
#include "entity_base_vhandle.h"
#include "entity_handle_manager.h"
#include "filespec.h"
#include "local_collision.h"
#include "mission_stack_manager.h"
#include "mstring.h"
#include "oldmath_po.h"
#include "resource_key.h"
#include "resource_manager.h"
#include "resource_pack_location.h"
#include "resource_pack_slot.h"
#include "resource_partition.h"
#include "script_executable.h"
#include "script_manager.h"
#include "script_object.h"
#include "string_hash.h"
#include "trace.h"
#include "utility.h"
#include "vector3d.h"
#include "vm_executable.h"
#include "vm_thread.h"
#include "wds.h"

#include <algorithm>
#include <cstring>
#include <deque>
#include <string>
#include <vector>

// These menus are owned by Ultimate_prerelease.cpp / Ultimate_final.cpp /
// Ultimate_build.cpp.  Sharing the pointers prevents script SLFs from creating
// a second duplicate "Script" menu after the native character lineup is added.
extern debug_menu *script_menu;
extern debug_menu *progression_menu;

int vm_debug_menu_entry_garbage_collection_id = -1;

// ----------------------------------------------------------------------
//   Native handlers + XBSX-driven population of the Script debug menu.
//
//   The vanilla Script menu in the reference video is fully populated
//   by `pk_character_lineup.{xbsx,pcsx}`. That script's construct does
//   roughly:
//
//       create_debug_menu_entry("Pop Character",      "pop_character(debug_menu_entry)");
//       create_debug_menu_entry("Pop All Characters", "pop_all_characters(debug_menu_entry)");
//       for ent_class in entities_with_prefix("ch_"):
//           create_debug_menu_entry(strip_prefix(ent_class), "spawn_character(debug_menu_entry)");
//
//   We let the script run normally — that's how the per-character
//   list stays accurate to whatever ch_* entity classes the build
//   actually contains — but we intercept the three handler names it
//   binds and reroute them to native C++ in
//   slf__create_debug_menu_entry below. That keeps the spawn /
//   teardown path in C++ where we already track entities, without
//   depending on the script bytecode for those three handlers being
//   functional on this platform.
// ----------------------------------------------------------------------

// Character packs are not part of CITY_ARENA.  The retail entity manager only
// resolves ENTITY resources reachable from the active pack context, so a bare
// create call succeeds for city pedestrians and silently fails for nearly all
// named characters.  The original CHARACTER_LINEUP script pushes a character
// pack before create_entity and keeps it pushed until that entity is destroyed.
// The native menu follows the same lifetime here.
namespace {

constexpr size_t kMaxQueuedCharacterSpawns = 128u;
constexpr size_t kMaxLiveDebugCharacters = 8u;

struct debug_character_spawn_request {
    std::string name;
    float offset_x;
    float offset_z;
};

struct debug_character_record {
    entity_base_vhandle handle;
    // Empty when the pack belongs to the level/game rather than this menu.
    std::string owned_pack;
};

struct character_asset_alias {
    const char *menu_name;
    const char *pack_name;
    const char *entity_name;
};

struct character_asset_candidate {
    std::string pack_name;
    std::string entity_name;
};

// Viewer packs occasionally use a class name different from the debug-menu
// label.  City civilians also live in CITY_* packs rather than CH_VWR_*.
constexpr character_asset_alias kCharacterAssetAliases[] {
    {"alex_ohirn_prison",       "ch_vwr_alex_ohirn",                 "alex_ohirn"},
    {"beetle",                  "ch_vwr_beetle_viewer",              "beetle_viewer"},
    {"beetle",                  "pk_v10_beetle",                     "beetle"},
    {"electro_naked",           "ch_vwr_electro_nosuit",             "electro_nosuit"},
    {"gmu_businessman",         "city_gmu_businessman",              "gmu_businessman"},
    {"gmu_child_male",          "city_gmu_child_male",               "gmu_child_male"},
    {"gmu_cop_fat",             "city_fat_cop",                      "gmu_cop_fat"},
    {"gmu_cop_thin",            "city_thin_cop",                     "gmu_cop_thin"},
    {"gmu_cop_thin_igc",        "s02_igc4_pack",                     "gmu_cop_thin_igc_lite"},
    {"gmu_jogger_fem",          "city_gmu_jogger_fem",               "gmu_jogger_fem"},
    {"gmu_lab_fem",             "pk_v12_venvscar",                   "gmu_lab_fem_v09"},
    {"gmu_man",                 "city_gmu_man",                      "gmu_man_ct"},
    {"gmu_medic_fem",           "city_gmu_medic_fem",                "gmu_medic_fem"},
    {"gmu_storeowner",          "city_gmu_storeowner",               "gmu_storeowner"},
    {"gmu_street_vendor",       "city_gmu_street_vender",            "gmu_street_vendor"},
    {"gmu_student_female",      "city_gmu_student_female",           "gmu_student_female"},
    {"gmu_student_male",        "city_gmu_student_male",             "gmu_student_male"},
    {"gmu_utilityworker_male",  "city_gmu_utility_worker_male",      "gmu_utilityworker_male"},
    {"gmu_woman",               "city_gmu_woman",                    "gmu_woman_ct"},
    {"origin_spider",           "ch_vwr_ultimate_spiderman_vwr",     "ultimate_spiderman_vwr"},
    {"ped_fem",                 "city_arena",                        "ped_fem"},
    {"ped_male",                "city_arena",                        "ped_male"},
    {"peter_hooded",            "ch_vwr_peter_hooded_viewer",        "peter_hooded_viewer"},
    {"peter_hooded",            "peter_hooded",                      "peter_hooded"},
    {"peter_parker",            "ch_vwr_peter_parker_viewer",        "peter_parker_viewer"},
    {"peter_parker",            "peter_parker",                      "peter_parker"},
    {"peter_shirtless",         "ch_vwr_peter_parker_viewer",        "peter_parker_viewer"},
    {"rhino_igc",               "s04_rhino_igc3",                   "rhino_igc"},
    {"rhino_igc",               "ch_vwr_rhino",                      "rhino"},
    {"shield_agent_jetpack",    "hot_pursuit_sable_pack",           "shield_agent_jetpack"},
    {"shield_agent_jetpack",    "ch_vwr_shield_agent",               "shield_agent"},
    {"shield_agent_test",       "ch_vwr_shield_agent",               "shield_agent"},
    {"usm_peterhead",           "ch_vwr_ultimate_spiderman_vwr",     "ultimate_spiderman_vwr"},
    {"usm_venomhand",           "ch_vwr_venom_viewer",               "venom_viewer"},
    {"venom_eddie",             "pk_s13_ending_igc",                 "venom_eddie_lite"},
    {"venom_eddie",             "ch_vwr_venom_viewer",               "venom_viewer"},
    {"venom_spider",            "venom_spider",                      "venom_spider"},
    {"venom_spider",            "ch_vwr_venom_viewer",               "venom_viewer"},
};

std::deque<debug_character_spawn_request> &debug_character_spawn_queue()
{
    static std::deque<debug_character_spawn_request> queue;
    return queue;
}

std::vector<debug_character_record> &debug_character_entities()
{
    static std::vector<debug_character_record> entities;
    return entities;
}

std::vector<std::string> &debug_character_owned_packs()
{
    static std::vector<std::string> packs;
    return packs;
}

bool &debug_character_cleanup_requested()
{
    static bool requested = false;
    return requested;
}

bool &debug_character_pop_requested()
{
    static bool requested = false;
    return requested;
}

void append_unique(std::vector<std::string> &values, const std::string &value)
{
    if ( !value.empty()
         && std::find(values.begin(), values.end(), value) == values.end() )
    {
        values.push_back(value);
    }
}

std::string normalize_character_name(const char *name)
{
    if ( name == nullptr ) {
        return {};
    }

    mString normalized {name};
    normalized.to_lower();
    std::string result {normalized.c_str()};

    const auto slash = result.find_last_of("\\/");
    if ( slash != std::string::npos ) {
        result.erase(0u, slash + 1u);
    }

    const auto dot = result.rfind('.');
    if ( dot != std::string::npos ) {
        result.erase(dot);
    }

    return result;
}

std::vector<std::string> get_entity_candidates(const std::string &name)
{
    std::vector<std::string> result;
    append_unique(result, name);

    if ( name.rfind("ch_", 0u) == 0u ) {
        append_unique(result, name.substr(3u));
    } else {
        append_unique(result, std::string {"ch_"} + name);
    }

    // Alias entities are deliberately not pooled here.  If (for example)
    // VENOM_VIEWER is already loaded, it must not steal a venom_spider request
    // before the verified VENOM_SPIDER/venom_spider pair is tried below.
    return result;
}

void append_asset_unique(std::vector<character_asset_candidate> &values,
                         const std::string &pack_name,
                         const std::string &entity_name)
{
    if ( pack_name.empty() || entity_name.empty() ) {
        return;
    }

    const auto duplicate = std::find_if(
        values.begin(), values.end(),
        [&pack_name, &entity_name](const character_asset_candidate &value) {
            return value.pack_name == pack_name
                && value.entity_name == entity_name;
        });
    if ( duplicate == values.end() ) {
        values.push_back(character_asset_candidate {pack_name, entity_name});
    }
}

std::vector<character_asset_candidate> get_asset_candidates(
    const std::string &name)
{
    std::vector<character_asset_candidate> result;

    if ( name.rfind("ch_", 0u) == 0u ) {
        append_asset_unique(result, name, name.substr(3u));
    } else {
        // This is the exact convention used by CHARACTER_LINEUP.XBSX.
        append_asset_unique(result, std::string {"ch_"} + name, name);
    }

    // Verified gameplay/city mappings and explicit viewer fallbacks are kept
    // as pack/entity pairs.  Do not accidentally resolve an alias entity from
    // a different candidate pack.
    for ( const auto &alias : kCharacterAssetAliases ) {
        if ( name == alias.menu_name ) {
            append_asset_unique(
                result, alias.pack_name, alias.entity_name);
        }
    }

    const std::string bare_name =
        name.rfind("ch_", 0u) == 0u ? name.substr(3u) : name;
    append_asset_unique(
        result, std::string {"ch_vwr_"} + bare_name, bare_name);

    if ( bare_name.rfind("gmu_", 0u) == 0u ) {
        append_asset_unique(
            result, std::string {"city_"} + bare_name, bare_name);
    }

    return result;
}

bool find_entity_in_context(resource_pack_slot *context,
                            const std::vector<std::string> &candidates,
                            std::string *resolved_name)
{
    if ( context == nullptr || !context->is_pack_ready() ) {
        return false;
    }

    for ( const auto &candidate : candidates )
    {
        const resource_key entity_key {
            string_hash {candidate.c_str()}, RESOURCE_KEY_TYPE_ENTITY};
        int mash_size = 0;
        resource_pack_slot *owner = nullptr;
        if ( context->get_resource(entity_key, &mash_size, &owner) != nullptr )
        {
            if ( resolved_name != nullptr ) {
                *resolved_name = candidate;
            }
            return true;
        }
    }

    return false;
}

resource_pack_slot *find_loaded_character_context(
    const std::vector<std::string> &entity_candidates,
    std::string *resolved_name)
{
    auto *active_context = resource_manager::get_resource_context();
    if ( find_entity_in_context(active_context, entity_candidates, resolved_name) ) {
        return active_context;
    }

    if ( resource_manager::partitions == nullptr ) {
        return nullptr;
    }

    for ( auto *partition : *resource_manager::partitions )
    {
        if ( partition == nullptr ) {
            continue;
        }

        for ( auto *slot : partition->get_pack_slots() )
        {
            if ( slot != active_context
                 && find_entity_in_context(slot, entity_candidates, resolved_name) )
            {
                return slot;
            }
        }
    }

    return nullptr;
}

void destroy_debug_character(const debug_character_record &record)
{
    if ( g_world_ptr == nullptr ) {
        return;
    }

    auto *base = record.handle.get_volatile_ptr();
    if ( base != nullptr && base->is_an_entity() )
    {
        auto *ent = static_cast<entity *>(base);
        if ( g_world_ptr->ent_mgr.is_entity_valid(ent) ) {
            g_world_ptr->ent_mgr.destroy_entity(ent);
        }
    }
}

void release_debug_character_assets(bool clear_queue)
{
    auto &entities = debug_character_entities();
    if ( g_world_ptr != nullptr )
    {
        for ( auto it = entities.rbegin(); it != entities.rend(); ++it )
        {
            destroy_debug_character(*it);
        }
    }
    entities.clear();

    auto *stack = mission_stack_manager::s_inst;
    auto &packs = debug_character_owned_packs();
    if ( stack != nullptr && !stack->waiting_for_push_or_pop() )
    {
        for ( auto it = packs.rbegin(); it != packs.rend(); ++it )
        {
            mString pack_name {it->c_str()};
            if ( stack->is_pack_pushed(pack_name) ) {
                stack->pop_mission_pack_immediate(pack_name, pack_name);
            }
        }
    }
    packs.clear();

    if ( clear_queue ) {
        debug_character_spawn_queue().clear();
    }
}

void release_one_debug_character()
{
    auto &entities = debug_character_entities();
    if ( entities.empty() ) {
        return;
    }

    const debug_character_record record = entities.back();
    destroy_debug_character(record);
    entities.pop_back();

    if ( record.owned_pack.empty() ) {
        return;
    }

    const bool still_used = std::any_of(
        entities.begin(), entities.end(),
        [&record](const debug_character_record &other) {
            return other.owned_pack == record.owned_pack;
        });
    if ( still_used ) {
        return;
    }

    auto *stack = mission_stack_manager::s_inst;
    mString pack_name {record.owned_pack.c_str()};
    if ( stack != nullptr && !stack->waiting_for_push_or_pop()
         && stack->is_pack_pushed(pack_name) )
    {
        stack->pop_mission_pack_immediate(pack_name, pack_name);
    }

    auto &packs = debug_character_owned_packs();
    packs.erase(std::remove(packs.begin(), packs.end(), record.owned_pack),
                packs.end());
}

bool pack_is_available(const std::string &pack_name,
                       resource_pack_location *location)
{
    if ( resource_manager::amalgapak_pack_location_table == nullptr
         || resource_manager::amalgapak_base_offset == -1 )
    {
        return false;
    }

    const resource_key pack_key {
        string_hash {pack_name.c_str()}, RESOURCE_KEY_TYPE_PACK};
    return resource_manager::get_pack_file_stats(
        pack_key, location, nullptr, nullptr);
}

resource_pack_slot *load_character_context(
    const std::vector<character_asset_candidate> &asset_candidates,
    std::string *resolved_entity,
    std::string *newly_owned_pack)
{
    auto *stack = mission_stack_manager::s_inst;
    auto *mission_partition =
        resource_manager::get_partition_pointer(RESOURCE_PARTITION_MISSION);
    if ( stack == nullptr || mission_partition == nullptr ) {
        return nullptr;
    }

    for ( const auto &asset : asset_candidates )
    {
        const auto &pack = asset.pack_name;
        resource_pack_location location {};
        if ( !pack_is_available(pack, &location) ) {
            continue;
        }

        mString pack_name {pack.c_str()};
        const bool already_pushed = stack->is_pack_pushed(pack_name);
        if ( !already_pushed )
        {
            if ( !mission_partition->has_room_for_slot(location.loc.m_size) )
            {
                // Debug-owned character packs are disposable.  Releasing the
                // existing lineup is safer than overflowing the mission stack.
                if ( !debug_character_owned_packs().empty() ) {
                    release_debug_character_assets(false);
                }
                if ( !mission_partition->has_room_for_slot(location.loc.m_size) ) {
                    printf("[CharList] pack '%s' needs %d bytes; mission stack is full\n",
                           pack.c_str(), location.loc.m_size);
                    continue;
                }
            }

            stack->push_mission_pack_immediate(pack_name, pack_name);
        }

        if ( mission_partition->get_pack_slots().empty() )
        {
            if ( !already_pushed && stack->is_pack_pushed(pack_name) ) {
                stack->pop_mission_pack_immediate(pack_name, pack_name);
            }
            continue;
        }

        auto *context =
            resource_manager::get_best_context(RESOURCE_PARTITION_MISSION);
        const std::vector<std::string> exact_entity {asset.entity_name};
        if ( find_entity_in_context(context, exact_entity, resolved_entity) )
        {
            if ( !already_pushed && newly_owned_pack != nullptr ) {
                *newly_owned_pack = pack;
            }
            return context;
        }

        if ( !already_pushed && stack->is_pack_pushed(pack_name) ) {
            stack->pop_mission_pack_immediate(pack_name, pack_name);
        }
    }

    return nullptr;
}

std::string find_owned_pack_for_assets(
    const std::vector<character_asset_candidate> &asset_candidates)
{
    const auto &owned_packs = debug_character_owned_packs();
    for ( const auto &asset : asset_candidates )
    {
        if ( std::find(owned_packs.begin(), owned_packs.end(), asset.pack_name)
             != owned_packs.end() )
        {
            return asset.pack_name;
        }
    }
    return {};
}

bool spawn_debug_character(const debug_character_spawn_request &request)
{
    if ( g_world_ptr == nullptr ) {
        return false;
    }

    auto *hero = g_world_ptr->get_hero_ptr(0);
    if ( hero == nullptr ) {
        return false;
    }

    const std::string display_name = normalize_character_name(request.name.c_str());
    if ( display_name.empty() ) {
        return false;
    }

    if ( debug_character_entities().size() >= kMaxLiveDebugCharacters )
    {
        printf("[CharList] lineup limit reached; replacing the newest character\n");
        release_one_debug_character();
    }

    const auto entity_candidates = get_entity_candidates(display_name);
    const auto asset_candidates = get_asset_candidates(display_name);
    std::string resolved_entity;
    std::string newly_owned_pack;
    resource_pack_slot *context =
        find_loaded_character_context(entity_candidates, &resolved_entity);

    if ( context == nullptr )
    {
        context = load_character_context(
            asset_candidates,
            &resolved_entity,
            &newly_owned_pack);
    }

    if ( context == nullptr )
    {
        printf("[CharList] no loaded or installable character pack for '%s'\n",
               display_name.c_str());
        return false;
    }

    // Match SpawnXCommand::process_cmd, including its collision fallback.
    po placement = hero->get_abs_po();
    const vector3d local_offset {request.offset_x, 0.0f, request.offset_z};
    const auto world_offset =
        hero->get_abs_po().non_affine_slow_xform(local_offset);
    placement.set_position(hero->get_abs_position() + world_offset);

    vector3d impact_pos {};
    vector3d impact_normal {};
    const vector3d proposed_pos = placement.get_position();
    if ( local_collision::entfilter_accept_all != nullptr
         && local_collision::obbfilter_sphere_test != nullptr
         && find_sphere_intersection(
                proposed_pos,
                0.5f,
                *local_collision::entfilter_accept_all,
                *local_collision::obbfilter_sphere_test,
                &impact_pos,
                &impact_normal,
                nullptr,
                nullptr) )
    {
        placement.set_position(
            hero->get_abs_position() + vector3d {0.0f, 0.1f, 0.0f});
    }

    const string_hash class_id {resolved_entity.c_str()};
    const auto unique_id = make_unique_entity_id();
    mString empty_tag {};

    resource_manager::push_resource_context(context);
    entity *new_ent = g_world_ptr->ent_mgr.create_and_add_entity_or_subclass(
        class_id,
        unique_id,
        placement,
        empty_tag,
        1u,
        nullptr);
    resource_manager::pop_resource_context();

    if ( new_ent == nullptr )
    {
        printf("[CharList] entity manager failed to spawn '%s' as '%s'\n",
               display_name.c_str(), resolved_entity.c_str());
        if ( !newly_owned_pack.empty() )
        {
            mString pack_name {newly_owned_pack.c_str()};
            auto *stack = mission_stack_manager::s_inst;
            if ( stack != nullptr && stack->is_pack_pushed(pack_name) ) {
                stack->pop_mission_pack_immediate(pack_name, pack_name);
            }
        }
        return false;
    }

    if ( new_ent->get_flavor() == ENTITY_ITEM )
    {
        po item_po {identity_matrix};
        item_po.set_position(new_ent->get_abs_position());
        new_ent->set_abs_po(item_po);
    }

    new_ent->set_visible(true, false);
    if ( new_ent->has_damage_ifc() )
    {
        auto *damage = new_ent->damage_ifc();
        if ( damage != nullptr ) {
            const auto base_health = damage->field_1FC.field_0[2];
            damage->field_1FC.sub_48BFB0(base_health);
        }
    }

    std::string associated_pack = newly_owned_pack;
    if ( associated_pack.empty() ) {
        associated_pack = find_owned_pack_for_assets(asset_candidates);
    }
    debug_character_entities().push_back(debug_character_record {
        new_ent->get_my_vhandle(), associated_pack});
    if ( !newly_owned_pack.empty() ) {
        append_unique(debug_character_owned_packs(), newly_owned_pack);
    }

    printf("[CharList] spawned '%s' as '%s'%s%s\n",
           display_name.c_str(),
           resolved_entity.c_str(),
           newly_owned_pack.empty() ? "" : " from ",
           newly_owned_pack.c_str());
    return true;
}

} // namespace

void queue_debug_character_spawn(const char *name, float offset_x, float offset_z)
{
    if ( name == nullptr || name[0] == '\0' ) {
        return;
    }

    auto &queue = debug_character_spawn_queue();
    if ( queue.size() >= kMaxQueuedCharacterSpawns ) {
        printf("[CharList] spawn queue full; dropping '%s'\n", name);
        return;
    }

    queue.push_back(debug_character_spawn_request {
        std::string {name}, offset_x, offset_z});
}

void queue_debug_character_cleanup()
{
    debug_character_spawn_queue().clear();
    debug_character_pop_requested() = false;
    debug_character_cleanup_requested() = true;
}

void queue_debug_character_pop()
{
    debug_character_pop_requested() = true;
}

void process_debug_character_spawn_queue()
{
    auto *stack = mission_stack_manager::s_inst;
    if ( debug_character_cleanup_requested() )
    {
        if ( stack != nullptr && stack->waiting_for_push_or_pop() ) {
            return;
        }
        release_debug_character_assets(true);
        debug_character_cleanup_requested() = false;
        return;
    }

    if ( debug_character_pop_requested() )
    {
        if ( stack != nullptr && stack->waiting_for_push_or_pop() ) {
            return;
        }
        release_one_debug_character();
        debug_character_pop_requested() = false;
        return;
    }

    auto &queue = debug_character_spawn_queue();
    if ( queue.empty() || g_world_ptr == nullptr
         || g_world_ptr->get_hero_ptr(0) == nullptr
         || resource_manager::partitions == nullptr
         || stack == nullptr
         || stack->waiting_for_push_or_pop() )
    {
        return;
    }

    const debug_character_spawn_request request = queue.front();
    queue.pop_front();
    spawn_debug_character(request);
}

void clear_debug_character_spawns()
{
    debug_character_cleanup_requested() = false;
    debug_character_pop_requested() = false;
    release_debug_character_assets(true);
}

// Remove the most recently spawned character and then release its pack if no
// remaining debug character uses it.
static void native_pop_character_handler(debug_menu_entry *)
{
    queue_debug_character_pop();
    debug_menu::hide();
}

// Script-created lineups use this handler for teardown.  Destruction and pack
// pops are deferred for the same resource-manager re-entrancy reason as spawn.
static void native_pop_all_characters_handler(debug_menu_entry *)
{
    queue_debug_character_cleanup();
    debug_menu::hide();
}

static void native_character_select_handler(debug_menu_entry *entry)
{
    if ( entry == nullptr || entry->text[0] == '\0' ) {
        return;
    }

    queue_debug_character_spawn(entry->text);
    debug_menu::hide();
}

// ----------------------------------------------------------------------
// Native dispatch table. When the XBSX's construct method asks the
// engine to bind a script handler to a menu entry, slf__create_debug_menu_entry
// looks the handler name up here first; on a hit we wire a C++
// function in directly and skip set_script_handler. That means the
// spawn / pop paths never enter the script VM, so they don't depend
// on the bytecode for those handlers being functional on this
// platform.
// ----------------------------------------------------------------------
namespace {

struct native_script_handler_t {
    const char *handler_name;
    void (*fn)(debug_menu_entry *);
};

constexpr native_script_handler_t kNativeScriptHandlers[] {
    { "pop_character(debug_menu_entry)",      native_pop_character_handler      },
    { "pop_all_characters(debug_menu_entry)", native_pop_all_characters_handler },
    { "spawn_character(debug_menu_entry)",    native_character_select_handler   },
};

bool try_install_native_handler(debug_menu_entry *entry, const char *name)
{
    if ( entry == nullptr || name == nullptr || name[0] == '\0' ) {
        return false;
    }
    for ( const auto &h : kNativeScriptHandlers ) {
        if ( std::strcmp(name, h.handler_name) == 0 ) {
            entry->m_game_flags_handler = h.fn;
            return true;
        }
    }
    return false;
}

} // namespace

// ----------------------------------------------------------------------
// Load pk_character_lineup.{xbsx,pcsx}.
//
// The compiled file ships under the standard scripts directory and is
// named pk_character_lineup.{xbsx|pcsx}; the resource_key extension
// table picks the right suffix for the active platform via
// g_resource_key_type_ext. This function follows the same
// load → link sequence game::load_world() uses for init_gv / init_sv:
//
//   - script_manager::load() registers an exec entry and queues it
//     onto pending_link_list.
//   - script_manager::link() drains pending_link_list and pushes onto
//     pending_first_run.
//   - The next wds::frame_advance() naturally calls
//     script_manager::run(), which runs first_run() for everything in
//     pending_first_run — i.e. the script's construct method fires
//     there.
//
// During construct, the XBSX calls slf__create_debug_menu_entry once
// per entry. Our hook in that SLF reroutes the three known handler
// names (pop_character / pop_all_characters / spawn_character) to
// native code via try_install_native_handler.
// ----------------------------------------------------------------------
static bool load_pk_character_lineup_script()
{
    auto *common_partition =
        resource_manager::get_partition_pointer(RESOURCE_PARTITION_COMMON);
    if ( common_partition == nullptr
         || common_partition->get_pack_slots().empty() )
    {
        printf("[script_lib_debug_menu] common partition not ready, "
               "deferring pk_character_lineup load\n");
        return false;
    }

    auto *common_slot = common_partition->get_pack_slots().at(0u);
    if ( common_slot == nullptr ) {
        return false;
    }

    const resource_key script_key {string_hash {"pk_character_lineup"},
                                   RESOURCE_KEY_TYPE_SCRIPT};

    if ( !script_manager::is_loadable(script_key) ) {
        printf("[script_lib_debug_menu] pk_character_lineup script not "
               "found in common pack — Script menu will only have "
               "entries registered by other scripts\n");
        return false;
    }

    const resource_key empty_key {};
    auto *exec_entry = script_manager::load(script_key, 0u, common_slot, empty_key);
    if ( exec_entry == nullptr ) {
        printf("[script_lib_debug_menu] script_manager::load failed for "
               "pk_character_lineup\n");
        return false;
    }

    // Drain pending_link_list onto pending_first_run so the script's
    // construct fires on the next wds::frame_advance() tick. Safe to
    // call from inside an SLF — link() doesn't recurse into run().
    script_manager::link();
    return true;
}

void init_script_debug_menu()
{
    if ( script_menu == nullptr )
    {
        script_menu = new debug_menu {"Script", (DWORD)debug_menu::sort_mode_t::undefined};

        progression_menu = new debug_menu {"Progression", (DWORD)debug_menu::sort_mode_t::undefined};

        debug_menu::root_menu->add_entry(script_menu);
        debug_menu::root_menu->add_entry(progression_menu);

        // Pull entries from pk_character_lineup.{xbsx,pcsx}. Done
        // here (not inside the slf hook) because root_menu must
        // already exist by the time the script's construct method
        // starts adding children to script_menu — and the hook only
        // fires once thanks to the (script_menu == nullptr) guard
        // above, so we don't double-load on later slf calls from the
        // very same script.
        load_pk_character_lineup_script();
    }
}

void vm_debug_menu_entry_garbage_collection_callback(script_executable *,
                                                    _std::list<uint32_t> &a2,
                                                    _std::list<mString> &)
{
    for ( auto &v2 : a2 )
    {
        assert(script_menu != nullptr);

        auto *entry = bit_cast<debug_menu_entry *>(v2);
        
        // script_menu->remove_entry(entry);
      //  remove_debug_menu_entry(entry);
    }
}

void construct_debug_menu_lib()
{
    if ( vm_debug_menu_entry_garbage_collection_id == -1 ) {
        vm_debug_menu_entry_garbage_collection_id = script_manager::register_allocated_stuff_callback(vm_debug_menu_entry_garbage_collection_callback);
    }
}

slf__create_debug_menu_entry__str__str__t::slf__create_debug_menu_entry__str__str__t(const char *a3) : function(a3)
{
    m_vtbl = CAST(m_vtbl, 0x0089C70C);
    FUNC_ADDRESS(address, &slf__create_debug_menu_entry__str__str__t::operator());
    m_vtbl->__cl = CAST(m_vtbl->__cl, address);
}

bool slf__create_debug_menu_entry__str__str__t::operator()(vm_stack &stack, [[maybe_unused]]script_library_class::function::entry_t entry) const
{
    TRACE("slf__create_debug_menu_entry__str__str__t::operator()");

    if constexpr (1)
    {
        SLF_PARMS;

        init_script_debug_menu();
        assert(script_menu != nullptr);

        mString v14 {parms->str0};
        auto *result = new debug_menu_entry {v14};

        auto *nt = stack.get_thread();

        // Try to bind a native handler first; only fall back to the
        // script VM if parms->str1 isn't a name we recognise. The
        // three handlers used by pk_character_lineup.xbsx
        // (pop_character / pop_all_characters / spawn_character) all
        // hit the native path here, so the spawn / pop logic stays
        // in C++ regardless of whether the script bytecode for those
        // handlers is functional on this platform.
        if ( !try_install_native_handler(result, parms->str1) )
        {
            mString v15 {parms->str1};
            auto *v4 = nt->get_instance();
            result->set_script_handler(v4, v15);
        }

        // Garbage-collection bookkeeping is unchanged: register the
        // entry against the owning script_object so reload / teardown
        // of the script removes the menu entries it produced.
        mString v16 {};
        uint32_t v11 = int(result);
        auto v10 = vm_debug_menu_entry_garbage_collection_id;
        auto *v6 = nt->get_executable();
        auto *so = v6->get_owner();
        auto *v8 = so->get_parent();

        v8->add_allocated_stuff(v10, v11, v16);
        script_menu->add_entry(result);

        SLF_RETURN;
        SLF_DONE;
    }
    else
    {
        bool (__fastcall *func)(const void *, void *edx, vm_stack *, entry_t) = CAST(func, 0x00678210);
        return func(this, nullptr, &stack, entry);
    }
}

void script_lib_debug_menu_patch()
{
    REDIRECT(0x0089C710, construct_debug_menu_lib);
}
