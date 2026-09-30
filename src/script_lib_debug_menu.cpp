#include "script_lib_debug_menu.h"
#include "multiplayer_mode.h"

#include "collide.h"
#include "damage_interface.h"
#include "debug_menu.h"
#include "func_wrapper.h"
#include "game.h"
#include "game_process.h"
#include "entity.h"
#include "entity_base_vhandle.h"
#include "entity_handle_manager.h"
#include "event.h"
#include "event_manager.h"
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
#include <new>
#include <string>
#include <vector>

// These menus are owned by Ultimate_prerelease.cpp / Ultimate_final.cpp /
// Ultimate_build.cpp.  Sharing the pointers prevents script SLFs from creating
// a second duplicate "Script" menu after the native character lineup is added.
extern debug_menu *script_menu;
extern debug_menu *progression_menu;

int vm_debug_menu_entry_garbage_collection_id = -1;

void invalidate_v14_script_debug_menu_entries(script_instance *instance)
{
    // Also used by V10: its script-cleanup callback never removed rows, so
    // every mission reload cloned the Script menu entries and the stale
    // copies dispatched into the destroyed instance (abort on selection).
#if defined(OPENUSM_XBPACK_MODE)
    if (instance == nullptr) {
        return;
    }

    unsigned int removed = 0;
    const auto remove_owned_entries =
        [instance, &removed](debug_menu *menu, bool owns_render_name) {
        if (menu == nullptr || menu->entries == nullptr) {
            return;
        }

        const DWORD old_used_slots = menu->used_slots;
        const DWORD old_selected_index = old_used_slots == 0
            ? 0
            : std::min(menu->window_start + menu->cur_index,
                       old_used_slots - 1);
        DWORD mapped_selected_index = 0;
        bool mapped_selected = false;
        DWORD write_index = 0;
        for (DWORD read_index = 0; read_index < old_used_slots; ++read_index)
        {
            auto &entry = menu->entries[read_index];
            if (entry.field_14 == instance)
            {
                // Script-menu SLFs placement-construct the persistent name
                // after the flat-array copy. Progression entries only retain
                // their fixed text and therefore have no independently
                // constructed destination mString to destroy.
                if (owns_render_name) {
                    entry.m_name.~mString();
                }
                std::memset(&entry, 0, sizeof(entry));
                ++removed;
                continue;
            }

            if (!mapped_selected && read_index >= old_selected_index) {
                mapped_selected_index = write_index;
                mapped_selected = true;
            }

            if (write_index != read_index)
            {
                // Relocate the surviving flat entry as bytes. Copying or
                // destroying its mString here would duplicate ownership of
                // the backing buffer.
                std::memmove(&menu->entries[write_index],
                             &entry,
                             sizeof(entry));
                std::memset(&entry, 0, sizeof(entry));
            }
            ++write_index;
        }

        if (write_index == old_used_slots) {
            return;
        }

        menu->used_slots = write_index;
        menu->highlighted = nullptr;
        if (menu->used_slots == 0)
        {
            menu->window_start = 0;
            menu->cur_index = 0;
            if (current_menu == menu)
            {
                auto *fallback = menu->m_parent != nullptr
                    ? menu->m_parent
                    : debug_menu::root_menu;
                if (fallback != nullptr && fallback != menu) {
                    current_menu = fallback;
                } else {
                    debug_menu::hide();
                }
            }
            return;
        }

        // Preserve the selected survivor. If it was removed, select the next
        // survivor, or the final row when there is no row after it.
        if (!mapped_selected) {
            mapped_selected_index = menu->used_slots - 1;
        }
        constexpr DWORD kMenuPageSize = 18;
        const DWORD max_window = menu->used_slots > kMenuPageSize
            ? menu->used_slots - kMenuPageSize
            : 0;
        DWORD new_window = std::min(menu->window_start, max_window);
        if (mapped_selected_index < new_window) {
            new_window = mapped_selected_index;
        } else if (mapped_selected_index >= new_window + kMenuPageSize) {
            new_window = mapped_selected_index - kMenuPageSize + 1;
        }
        menu->window_start = new_window;
        menu->cur_index = mapped_selected_index - new_window;
    };

    remove_owned_entries(script_menu, true);
    remove_owned_entries(progression_menu, false);

    if (removed != 0) {
        sp_log("Xbox V14 debug menu removed %u expired script entr%s",
               removed,
               removed == 1 ? "y" : "ies");
    }
#else
    (void) instance;
#endif
}

// ----------------------------------------------------------------------
//   Native handlers + XBSX-driven population of the Script debug menu.
//
//   The vanilla Script menu in the reference video is fully populated
//   by `pk_character_lineup.{xbsx,pcsx}`. That script's construct does
//   roughly:
//
//       create_debug_menu_entry("Pop Character",      "pop_character(debug_menu_entry)");
//       create_debug_menu_entry("Pop All Characters", "pop_all_characters(debug_menu_entry)");
//       for pack_name in get_character_packname_list():
//           create_debug_menu_entry(pack_name, "spawn_character(debug_menu_entry)");
//
//   We let the script run normally — that's how the per-character
//   list stays accurate to whatever CH_* packs the build actually
//   contains — but we intercept handlers that need native
//   behavior and reroute them to C++ in
//   slf__create_debug_menu_entry below. That keeps the spawn /
//   teardown path in C++ where we already track entities, without
//   depending on the corresponding script bytecode being
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

// Some retail PC viewer packs use a class name different from the debug-menu
// label. City civilians also live in CITY_* packs rather than CH_* packs.
// OPENUSM_XBPACK_MODE filters the CH_VWR_* rows below: the Xbox archive has
// gameplay CH_* packs and those contain the complete actor/AI data expected by
// the Xbox character lineup.
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
    // Prefer the gameplay actor so Parker has his combat state graph and can
    // enter hit-react after taking damage.  Keep the viewer actor as a visual
    // fallback for builds that do not contain the gameplay pack.
    {"peter_parker",            "peter_parker",                      "peter_parker"},
    {"peter_parker",            "ch_vwr_peter_parker_viewer",        "peter_parker_viewer"},
    {"peter_shirtless",         "ch_vwr_peter_parker_viewer",        "peter_parker_viewer"},
    {"rhino_igc",               "s04_rhino_igc3",                   "rhino_igc"},
    {"rhino_igc",               "ch_vwr_rhino",                      "rhino"},
    {"shield_agent_jetpack",    "hot_pursuit_sable_pack",           "shield_agent_jetpack"},
    {"shield_agent_jetpack",    "ch_vwr_shield_agent",               "shield_agent"},
    {"shield_agent_test",       "ch_vwr_shield_agent",               "shield_agent"},
    // These Xbox CH_* rows are model attachments, not complete actors. Route
    // them to verified gameplay CH_* packs with compatible skeleton,
    // animation, and AI data so every visible row creates a complete actor.
    {"usm_peterhead",           "ch_peter_parker",                    "peter_parker"},
    {"usm_venomhand",           "ch_venom_spider",                    "venom_spider"},
    // Retail PC stores the same complete actors in non-CH packs. Keep these
    // after the Xbox candidates so each platform retains its native layout.
    {"usm_peterhead",           "peter_parker",                       "peter_parker"},
    {"usm_venomhand",           "venom_spider",                       "venom_spider"},
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

bool &debug_kill_hero_requested()
{
    static bool requested = false;
    return requested;
}

void kill_current_hero()
{
    if ( g_world_ptr == nullptr ) {
        return;
    }

    auto *hero = g_world_ptr->get_hero_ptr(0);
    if ( hero == nullptr || !hero->has_damage_ifc() ) {
        return;
    }

    auto *damage = hero->damage_ifc();
    if ( damage == nullptr ) {
        return;
    }

    // Match HealthVariable::setValue("0"): zero the current hit points and
    // raise DESTROYED so the normal hero-death/failure flow runs as well.
    damage->field_1FC.sub_48BFB0(0.0f);
    event_manager::raise_event(event::DESTROYED, hero->get_my_handle());
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

bool is_xbox_attachment_row(const std::string &name)
{
#if defined(OPENUSM_XBPACK_MODE)
    return name == "usm_peterhead" || name == "usm_venomhand";
#else
    (void) name;
    return false;
#endif
}

std::vector<std::string> get_entity_candidates(const std::string &name)
{
    std::vector<std::string> result;

    if ( is_xbox_attachment_row(name) )
    {
        for ( const auto &alias : kCharacterAssetAliases )
        {
            if ( name == alias.menu_name
                 && std::strncmp(alias.pack_name, "ch_vwr_", 7u) != 0 )
            {
                append_unique(result, alias.entity_name);
            }
        }
        return result;
    }

    append_unique(result, name);

    if ( name.rfind("ch_", 0u) == 0u ) {
        append_unique(result, name.substr(3u));
    } else {
        append_unique(result, std::string {"ch_"} + name);
    }

#if !defined(OPENUSM_XBPACK_MODE)
    // PC CHARACTER_LINEUP preserves VWR_ after removing CH_ so it can rebuild
    // CH_VWR_* pack names. The actor inside those packs omits VWR_.
    const std::string pack_suffix =
        name.rfind("ch_", 0u) == 0u ? name.substr(3u) : name;
    if ( pack_suffix.rfind("vwr_", 0u) == 0u ) {
        append_unique(result, pack_suffix.substr(4u));
    }
#endif

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

    if ( is_xbox_attachment_row(name) )
    {
        for ( const auto &alias : kCharacterAssetAliases )
        {
            if ( name == alias.menu_name
                 && std::strncmp(alias.pack_name, "ch_vwr_", 7u) != 0 )
            {
                append_asset_unique(
                    result, alias.pack_name, alias.entity_name);
            }
        }
        return result;
    }

    const std::string pack_suffix =
        name.rfind("ch_", 0u) == 0u ? name.substr(3u) : name;
    std::string entity_name = pack_suffix;
#if !defined(OPENUSM_XBPACK_MODE)
    if ( entity_name.rfind("vwr_", 0u) == 0u ) {
        entity_name.erase(0u, 4u);
    }
#endif

    if ( name.rfind("ch_", 0u) == 0u ) {
        append_asset_unique(result, name, entity_name);
    } else {
        // This is the exact convention used by CHARACTER_LINEUP.XBSX.
        append_asset_unique(result, std::string {"ch_"} + name, entity_name);
    }

    // Verified gameplay/city mappings and explicit viewer fallbacks are kept
    // as pack/entity pairs.  Do not accidentally resolve an alias entity from
    // a different candidate pack.
    for ( const auto &alias : kCharacterAssetAliases ) {
        if ( name == alias.menu_name ) {
#if defined(OPENUSM_XBPACK_MODE)
            if ( std::strncmp(alias.pack_name, "ch_vwr_", 7u) == 0 ) {
                continue;
            }
#endif
            append_asset_unique(
                result, alias.pack_name, alias.entity_name);
        }
    }

    const std::string bare_name = entity_name;
#if !defined(OPENUSM_XBPACK_MODE)
    append_asset_unique(
        result, std::string {"ch_vwr_"} + bare_name, bare_name);
#endif

    if ( bare_name.rfind("gmu_", 0u) == 0u ) {
        append_asset_unique(
            result, std::string {"city_"} + bare_name, bare_name);
    }

    return result;
}

bool find_entity_in_context(resource_pack_slot *context,
                            const std::vector<std::string> &candidates,
                            std::string *resolved_name,
                            const resource_key *required_owner = nullptr)
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
        if ( context->get_resource(entity_key, &mash_size, &owner) != nullptr
             && (required_owner == nullptr
                 || (owner != nullptr
                     && owner->get_name_key() == *required_owner)) )
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

bool debug_character_pack_is_used(const std::string &pack)
{
    const auto &entities = debug_character_entities();
    return std::any_of(
        entities.begin(), entities.end(),
        [&pack](const debug_character_record &record) {
            return record.owned_pack == pack;
        });
}

void release_unused_debug_character_packs()
{
    auto &packs = debug_character_owned_packs();
    if ( packs.empty() || resource_manager::partitions == nullptr ) {
        return;
    }

    auto *stack = mission_stack_manager::s_inst;
    auto *mission_partition =
        resource_manager::get_partition_pointer(RESOURCE_PARTITION_MISSION);
    if ( stack == nullptr || mission_partition == nullptr
         || stack->waiting_for_push_or_pop() )
    {
        return;
    }

    auto &slots = mission_partition->get_pack_slots();
    while ( !packs.empty() && !slots.empty() )
    {
        auto *top = slots.back();
        if ( top == nullptr || !top->is_pack_ready() ) {
            return;
        }

        const auto top_key = top->get_name_key();
        const auto owned = std::find_if(
            packs.begin(), packs.end(),
            [&top_key](const std::string &pack) {
                const resource_key pack_key {
                    string_hash {pack.c_str()}, RESOURCE_KEY_TYPE_PACK};
                return pack_key == top_key;
            });
        if ( owned == packs.end() || debug_character_pack_is_used(*owned) ) {
            return;
        }

        mString pack_name {owned->c_str()};
        if ( !stack->is_pack_pushed(pack_name) ) {
            packs.erase(owned);
            continue;
        }

        // Mission parents are a strict LIFO stack. Only pop an unused debug
        // pack when it is the actual top slot; a mission pack pushed later
        // must never be removed on its behalf.
        stack->pop_mission_pack_immediate(pack_name, pack_name);
        packs.erase(owned);
    }
}

void release_debug_character_assets(bool clear_queue,
                                    bool forget_blocked_packs = false)
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

    auto &packs = debug_character_owned_packs();
    release_unused_debug_character_packs();
    if ( forget_blocked_packs ) {
        // World teardown owns every remaining mission slot. Do not retain
        // names whose slots are about to be destroyed by the stock unload.
        packs.clear();
    }

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

    release_unused_debug_character_packs();
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

    // Preserve the verified gameplay/alias order. A pushed gameplay pack must
    // not lose to a later viewer fallback merely because the viewer would be
    // owned independently by this menu.
    for ( const auto &asset : asset_candidates )
    {
        const auto &pack = asset.pack_name;
        resource_pack_location location {};
        if ( !pack_is_available(pack, &location) ) {
            continue;
        }

        mString pack_name {pack.c_str()};
        const resource_key pack_key {
            string_hash {pack.c_str()}, RESOURCE_KEY_TYPE_PACK};
        const bool already_pushed = stack->is_pack_pushed(pack_name);
        if ( !already_pushed )
        {
            if ( !mission_partition->has_room_for_slot(location.loc.m_size) )
            {
                // Debug-owned character packs are disposable. Releasing the
                // existing lineup is safer than overflowing the stack.
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

        // Resolve through the exact slot so a same-named entity from a
        // different viewer or mission pack cannot win the lookup.
        resource_pack_slot *context = nullptr;
        for ( auto *slot : mission_partition->get_pack_slots() )
        {
            if ( slot != nullptr && slot->is_pack_ready()
                 && slot->get_name_key() == pack_key )
            {
                context = slot;
                break;
            }
        }
        const std::vector<std::string> exact_entity {asset.entity_name};
        if ( find_entity_in_context(
                context, exact_entity, resolved_entity, &pack_key) )
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

#if !defined(OPENUSM_XBPACK_MODE)
    const auto entity_candidates = get_entity_candidates(display_name);
#endif
    const auto asset_candidates = get_asset_candidates(display_name);

    // Pack parents are a strict LIFO stack.  Make room before loading a new
    // character so the previous newest debug pack is still the top parent
    // when it is removed.  Evicting after a replacement push would attempt a
    // non-LIFO parent removal and corrupt the mission directory.
    if ( debug_character_entities().size() >= kMaxLiveDebugCharacters )
    {
        printf("[CharList] lineup limit reached; replacing the newest character\n");
        release_one_debug_character();
    }

    std::string resolved_entity;
    std::string newly_owned_pack;
    // Prefer an exact installable pack so the actor has a pack lifetime owned
    // by this menu. PC falls back to an existing context only for resources
    // (such as base city actors) that have no independent candidate pack.
    resource_pack_slot *context = load_character_context(
        asset_candidates,
        &resolved_entity,
        &newly_owned_pack);
#if !defined(OPENUSM_XBPACK_MODE)
    if ( context == nullptr ) {
        context = find_loaded_character_context(
            entity_candidates, &resolved_entity);
    }
#endif

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
    if ( debug_kill_hero_requested() )
    {
        debug_kill_hero_requested() = false;
        kill_current_hero();
    }

    auto *stack = mission_stack_manager::s_inst;
    release_unused_debug_character_packs();
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
    multiplayer_mode_world_shutdown();
    debug_kill_hero_requested() = false;
    debug_character_cleanup_requested() = false;
    debug_character_pop_requested() = false;
    release_debug_character_assets(true, true);
}

namespace {

// Drain deferred resource work at the stock game-tick boundary. The arena owns
// its simulation while a loaded story world stays intact underneath it.
void __fastcall debug_character_spawner_frame_advance(
    void *self,
    void *,
    Float time_inc)
{
    // Defer debug resource mutations until the isolated versus session ends.
    if (!multiplayer_mode_active()) process_debug_character_spawn_queue();
    multiplayer_mode_tick_before(static_cast<float>(time_inc));

    using stock_frame_advance_t = void (__fastcall *)(
        void *, void *, Float);
    auto stock_frame_advance =
        bit_cast<stock_frame_advance_t>(0x0055D780);
    bool hold_story_world = false;
    if (multiplayer_mode_blocks_world() && g_game_ptr != nullptr
        && self == g_game_ptr && g_world_ptr != nullptr
        && g_game_ptr->the_world == g_world_ptr
        && g_game_ptr->flag.level_is_loaded
        && g_world_ptr->the_terrain != nullptr
        && g_game_ptr->process_stack.size() != 0)
    {
        const auto state = g_game_ptr->get_cur_state();
        hold_story_world = state == game_state::RUNNING || state == game_state::PAUSED;
    }
    // The retail function derives simulation time internally, so dt=0 would
    // still advance missions/AI. Input and resource streaming run before this
    // call; rendering runs in the app's separate render branch. Keep native
    // loading/frontend transitions live and preserve existing pause ownership.
    if (!hold_story_world) stock_frame_advance(self, nullptr, time_inc);
    multiplayer_mode_tick_after(static_cast<float>(time_inc));
}

} // namespace

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

// The Xbox v10 debug scripts create this entry, but its script handler does
// not execute correctly through the PC VM bridge.  Defer the death action out
// of the menu/resource callback, just like character pack work is deferred.
static void native_kill_hero_handler(debug_menu_entry *)
{
    debug_kill_hero_requested() = true;
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
    { "kill_hero(debug_menu_entry)",          native_kill_hero_handler           },
};

bool try_install_native_handler(debug_menu_entry *entry, const char *name)
{
    if ( entry == nullptr || name == nullptr || name[0] == '\0' ) {
        return false;
    }
    for ( const auto &h : kNativeScriptHandlers ) {
        if ( std::strcmp(name, h.handler_name) == 0 ) {
            // Replace only dispatch. V14's one-argument create SLF already
            // records the producing instance in field_14; retaining that
            // lifecycle owner lets runlevel teardown remove native lineup
            // rows before the script is loaded again, just like Xbox.
            entry->field_18 = -1;
            entry->m_game_flags_handler = h.fn;
            return true;
        }
    }
    return false;
}

} // namespace

bool install_native_debug_menu_handler(debug_menu_entry *entry,
                                       const char *handler_name)
{
    return try_install_native_handler(entry, handler_name);
}

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
// per entry. Our hook in that SLF reroutes known handlers to native
// code via try_install_native_handler.
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
        script_menu = create_menu(
            "Script", debug_menu::sort_mode_t::undefined);
        progression_menu = create_menu(
            "Progression", debug_menu::sort_mode_t::undefined);

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
#ifdef OPENUSM_XBPACK_V10
        vm_debug_menu_entry_garbage_collection_id = CDECL_CALL(
            0x005AFE40,
            vm_debug_menu_entry_garbage_collection_callback);
#elif !defined(OPENUSM_XBPACK_MODE)
        vm_debug_menu_entry_garbage_collection_id = script_manager::register_allocated_stuff_callback(
            vm_debug_menu_entry_garbage_collection_callback);
#endif
    }
}

slf__create_debug_menu_entry__str__t::slf__create_debug_menu_entry__str__t(const char *a3) : function(a3)
{
    m_vtbl = CAST(m_vtbl, 0x0089C704);
    FUNC_ADDRESS(address, &slf__create_debug_menu_entry__str__t::operator());
    m_vtbl->__cl = CAST(m_vtbl->__cl, address);
}

bool slf__create_debug_menu_entry__str__t::operator()(vm_stack &stack, [[maybe_unused]]script_library_class::function::entry_t entry) const
{
    TRACE("slf__create_debug_menu_entry__str__t::operator()");

#ifdef OPENUSM_XBPACK_MODE
    SLF_PARMS;

    init_script_debug_menu();
    assert(script_menu != nullptr);

    debug_menu_entry menu_entry {};
    menu_entry.entry_type = debug_menu_entry_type::dUNDEFINED;
    std::strncpy(menu_entry.text, parms->str0, MAX_CHARS_SAFE);
    menu_entry.text[MAX_CHARS_SAFE] = '\0';

    auto *thread = stack.get_thread();
    // The one-argument form is used by transient tools such as
    // CHARACTER_VIEWER. Tag its flat entry with the creating instance so the
    // destructor hook can remove it instead of leaving inert *_face_morph
    // rows mixed into the Xbox character lineup.
    menu_entry.field_14 = thread->get_instance();
    menu_entry.field_18 = -1;
    auto *result = static_cast<debug_menu_entry *>(
        add_debug_menu_entry(script_menu, &menu_entry));

    auto *script = thread->get_executable()->get_owner()->get_parent();
    mString source {};
    if (result == nullptr) {
        sp_log("Failed to add Xbox script debug-menu entry: %s",
               parms->str0 != nullptr ? parms->str0 : "<null>");
    } else {
        // The PC menu stores entries in a flat array. Set the render name on
        // that persistent copy, not on the temporary whose mString is
        // destroyed when this SLF returns.
        new (&result->m_name) mString {parms->str0};

#ifdef OPENUSM_XBPACK_V10
        if (vm_debug_menu_entry_garbage_collection_id < 0) {
            sp_log("Xbox debug-menu allocation type was not registered; "
                   "keeping entry without script cleanup: %s",
                   parms->str0 != nullptr ? parms->str0 : "<null>");
        } else {
            THISCALL(
                0x005A34B0,
                script,
                vm_debug_menu_entry_garbage_collection_id,
                int(result),
                &source);
        }
#else
        // V14 entries live in the PC menu's flat storage. Registering those
        // addresses in the stock script cleanup list crosses the injected
        // MinGW allocator with the game's MSVC 7.1 deallocator at runlevel
        // teardown, so keep the persistent menu copies out of that list.
        static bool logged_persistent_entries = false;
        if (!logged_persistent_entries) {
            sp_log("Xbox V14 debug-menu entries use persistent PC menu storage");
            logged_persistent_entries = true;
        }
        (void) script;
        (void) source;
#endif
    }

    SLF_RETURN;
    SLF_DONE;
#else
    bool (__fastcall *func)(const void *, void *, vm_stack *, entry_t) = CAST(func, 0x0067C1E0);
    return func(this, nullptr, &stack, entry);
#endif
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

        debug_menu_entry menu_entry {};
        menu_entry.entry_type = debug_menu_entry_type::dUNDEFINED;
        std::strncpy(menu_entry.text, parms->str0, MAX_CHARS_SAFE);
        menu_entry.text[MAX_CHARS_SAFE] = '\0';

        auto *nt = stack.get_thread();

        // Try to bind a native handler first; only fall back to the
        // script VM if parms->str1 isn't a name we recognise. The
        // three handlers used by pk_character_lineup.xbsx
        // (pop_character / pop_all_characters / spawn_character) all
        // hit the native path here, so the spawn / pop logic stays
        // in C++ regardless of whether the script bytecode for those
        // handlers is functional on this platform.
        if ( try_install_native_handler(&menu_entry, parms->str1) )
        {
            // Native dispatch (field_18 == -1) ignores field_14; keep the
            // producing instance as the row's owner so teardown removes it.
            menu_entry.field_14 = nt->get_instance();
        }
        else
        {
            mString v15 {parms->str1};
            auto *v4 = nt->get_instance();
            menu_entry.set_script_handler(v4, v15);
        }

        auto *result = static_cast<debug_menu_entry *>(
            add_debug_menu_entry(script_menu, &menu_entry));

        // Garbage-collection bookkeeping is unchanged: register the
        // entry against the owning script_object so reload / teardown
        // of the script removes the menu entries it produced.
        mString v16 {};
        uint32_t v11 = int(result);
        auto v10 = vm_debug_menu_entry_garbage_collection_id;
        auto *v6 = nt->get_executable();
        auto *so = v6->get_owner();
        auto *v8 = so->get_parent();

        if (result == nullptr) {
            sp_log("Failed to add Xbox script debug-menu entry: %s",
                   parms->str0 != nullptr ? parms->str0 : "<null>");
        } else {
            new (&result->m_name) mString {parms->str0};

#if defined(OPENUSM_XBPACK_MODE) && !defined(OPENUSM_XBPACK_V10)
            static bool logged_persistent_entries = false;
            if (!logged_persistent_entries) {
                sp_log("Xbox V14 debug-menu entries use persistent PC menu storage");
                logged_persistent_entries = true;
            }
#else
            if (v10 < 0) {
                sp_log("Xbox debug-menu allocation type was not registered; "
                       "keeping entry without script cleanup: %s",
                       parms->str0 != nullptr ? parms->str0 : "<null>");
            } else {
                v8->add_allocated_stuff(v10, v11, v16);
            }
#endif
        }

        SLF_RETURN;
        SLF_DONE;
    }
    else
    {
        bool (__fastcall *func)(const void *, void *edx, vm_stack *, entry_t) = CAST(func, 0x00678210);
        return func(this, nullptr, &stack, entry);
    }
}

slf__create_progression_menu_entry__str__str__t::slf__create_progression_menu_entry__str__str__t(const char *a3) : function(a3)
{
    m_vtbl = CAST(m_vtbl, 0x0089C714);
    FUNC_ADDRESS(address, &slf__create_progression_menu_entry__str__str__t::operator());
    m_vtbl->__cl = CAST(m_vtbl->__cl, address);
}

bool slf__create_progression_menu_entry__str__str__t::operator()(vm_stack &stack, [[maybe_unused]]script_library_class::function::entry_t entry) const
{
    TRACE("slf__create_progression_menu_entry__str__str__t::operator()");

    SLF_PARMS;

    init_script_debug_menu();
    assert(progression_menu != nullptr);

    debug_menu_entry menu_entry {parms->str0};
    menu_entry.set_script_handler(stack.get_thread()->get_instance(), mString {parms->str1});
    progression_menu->add_entry(&menu_entry);

    int result = 0;
    SLF_RETURN;
    SLF_DONE;
}

#if defined(OPENUSM_XBPACK_MODE)
namespace {

// The prerelease executable still destroys some instances through its stock
// destructor at 0x005AD7A0. Intercept its first callback call while `this` is
// intact, invalidate persistent PC menu bindings, then preserve the original
// callback behavior unchanged.
void __fastcall v14_script_instance_destructor_callbacks(
    script_instance *instance,
    void *,
    script_instance_callback_reason_t reason,
    vm_thread *thread)
{
    using stock_run_callbacks_t = void (__fastcall *)(
        script_instance *,
        void *,
        script_instance_callback_reason_t,
        vm_thread *);
    auto stock_run_callbacks = bit_cast<stock_run_callbacks_t>(0x0059EC70);
    stock_run_callbacks(instance, nullptr, reason, thread);
    // Destruction callbacks may still inspect their menu entries. Compact the
    // persistent flat arrays only after those callbacks have completed.
    invalidate_v14_script_debug_menu_entries(instance);
}

} // namespace
#endif

void script_lib_debug_menu_patch()
{
    // 0x0089C710 is a four-byte vtable slot, not a CALL site. A five-byte
    // REDIRECT here corrupts the next function's destructor at 0x0089C714.
    // Retail initializes through the client-library CALL at 0x005AD77D;
    // XBPACK calls construct_debug_menu_lib directly during setup.

    // CALL game::frame_advance from the stock application tick.  Character
    // menu callbacks only enqueue work; this post-callback seam performs pack
    // pushes, entity creation, and Pop actions outside resource-manager input.
    REDIRECT(0x005D70B8, debug_character_spawner_frame_advance);

#if defined(OPENUSM_XBPACK_MODE)
    // CALL script_instance::run_callbacks inside the stock instance dtor.
    // A call-site hook avoids replacing or reimplementing the surrounding
    // stock teardown and its exception-unwind bookkeeping.
    REDIRECT(0x005AD7CE, v14_script_instance_destructor_callbacks);
#endif
}
