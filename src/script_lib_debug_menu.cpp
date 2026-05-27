#include "script_lib_debug_menu.h"

#include "damage_interface.h"
#include "debug_menu.h"
#include "entity.h"
#include "entity_handle_manager.h"
#include "filespec.h"
#include "mstring.h"
#include "oldmath_po.h"
#include "resource_key.h"
#include "resource_manager.h"
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

#include <cstring>
#include <vector>

static debug_menu *script_menu = nullptr;

static debug_menu *progression_menu = nullptr;

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

// We keep raw entity pointers — the entity manager owns them, so all
// we need is to pass them back to destroy_entity at teardown.
static std::vector<entity *> &g_popped_entities()
{
    static std::vector<entity *> v;
    return v;
}

// Default to an empty name; native_character_select_handler fills
// this in the moment the user clicks any ch_* entry. Pop Character
// before any selection is then a no-op (mirrors Xbox behaviour).
static const char *g_last_selected_character = "";

// Spawn a single character by name, placing it just in front of the
// hero. Returns true on success. Mirrors SpawnXCommand::process_cmd
// from consolecmds.cpp, which is the native code path the in-game
// `spawn_x` console command takes. Tracks the new entity so Pop All
// can clean it up.
static bool pop_named_character(const char *name)
{
    if ( name == nullptr || name[0] == '\0' ) {
        return false;
    }

    if ( g_world_ptr == nullptr ) {
        printf("pop_named_character: world not ready\n");
        return false;
    }

    auto *hero = g_world_ptr->get_hero_ptr(0);
    if ( hero == nullptr ) {
        printf("pop_named_character: no hero (level not loaded?)\n");
        return false;
    }

    // Place the new actor a couple of metres in front of the hero so
    // it doesn't spawn inside us.
    po placement = hero->get_abs_po();
    const vector3d local_offset {0.0f, 0.0f, 2.0f}; // forward
    const auto offset_world =
        hero->get_abs_po().non_affine_slow_xform(local_offset);
    placement.set_position(hero->get_abs_position() + offset_world);

    // Fill in the standard character path: "characters\<name>\<name>.ent"
    filespec fs {mString{name}};
    if ( fs.m_ext.length() <= 0 ) {
        fs.m_ext = ".ent";
    }
    if ( fs.m_dir.length() <= 0 ) {
        fs.m_dir = "characters\\" + fs.m_name + "\\";
    }

    // Hash of the bare name is the entity-class key the manager uses
    // to look up the right factory.
    const string_hash class_id {fs.m_name.c_str()};
    const auto unique_id = make_unique_entity_id();
    mString empty_tag {};

    auto *new_ent = g_world_ptr->ent_mgr.create_and_add_entity_or_subclass(
        class_id,
        unique_id,
        placement,
        empty_tag,
        1u,
        nullptr);

    if ( new_ent == nullptr ) {
        printf("pop_named_character: failed to spawn '%s'\n", name);
        return false;
    }

    new_ent->set_visible(true, false);

    // Damage interface init mirrors what spawn_x does so the actor
    // starts at full health rather than freshly-zeroed state. Skip
    // gracefully if this kind of entity has no damage_ifc.
    if ( new_ent->has_damage_ifc() )
    {
        auto *dmg = new_ent->damage_ifc();
        const auto base = dmg->field_1FC.field_0[2];
        dmg->field_1FC.sub_48BFB0(base);
    }

    g_popped_entities().push_back(new_ent);
    return true;
}

// Handler for the "Pop Character" entry — pops the last character the
// user clicked on. Bound to script-handler-name
// "pop_character(debug_menu_entry)" referenced by the XBSX.
static void native_pop_character_handler(debug_menu_entry *)
{
    pop_named_character(g_last_selected_character);
    debug_menu::hide();
}

// Handler for the "Pop All Characters" entry. Destroys every entity we
// previously spawned via this UI. Bound to script-handler-name
// "pop_all_characters(debug_menu_entry)" referenced by the XBSX.
static void native_pop_all_characters_handler(debug_menu_entry *)
{
    if ( g_world_ptr == nullptr ) {
        debug_menu::hide();
        return;
    }

    auto &list = g_popped_entities();
    for ( auto *ent : list )
    {
        if ( ent != nullptr ) {
            g_world_ptr->ent_mgr.destroy_entity(ent);
        }
    }

    list.clear();
    debug_menu::hide();
}

// Handler for an individual character-name entry. Records the name as
// the "current" character (so Pop Character will use it) and
// immediately pops it, matching the click-to-spawn behaviour of the
// scripted version of this menu in the reference video. Bound to
// script-handler-name "spawn_character(debug_menu_entry)", which the
// XBSX wires to every ch_* entry it auto-generates.
static void native_character_select_handler(debug_menu_entry *e)
{
    if ( e == nullptr ) {
        return;
    }

    const char *name = e->text;
    g_last_selected_character = name;
    pop_named_character(name);
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