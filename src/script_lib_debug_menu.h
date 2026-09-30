#pragma once

#include "script_library_class.h"

#include "vm_stack.h"

struct debug_menu_entry;
struct script_instance;

struct slf__create_debug_menu_entry__str__t : script_library_class::function {
    slf__create_debug_menu_entry__str__t(const char *a3);

    struct parms_t { vm_str_t str0; };

    bool operator()(vm_stack &stack, [[maybe_unused]]script_library_class::function::entry_t entry) const;
};

struct slf__create_debug_menu_entry__str__str__t : script_library_class::function {
    slf__create_debug_menu_entry__str__str__t(const char *a3);

    struct parms_t {
        vm_str_t str0;
        vm_str_t str1;
    };
	

    bool operator()(vm_stack &stack, [[maybe_unused]]script_library_class::function::entry_t entry) const;
};

struct slf__create_progression_menu_entry__str__str__t : script_library_class::function {
    slf__create_progression_menu_entry__str__str__t(const char *a3);

    struct parms_t { vm_str_t str0; vm_str_t str1; };

    bool operator()(vm_stack &stack, [[maybe_unused]]script_library_class::function::entry_t entry) const;
};

extern int vm_debug_menu_entry_garbage_collection_id;

void construct_debug_menu_lib();

void init_script_debug_menu();

// V14 uses the PC menu's persistent flat storage. Remove entries owned by a
// script instance before that instance is destroyed so expired tools do not
// leave inert rows or dangling VM handlers in the Xbox-style menu.
void invalidate_v14_script_debug_menu_entries(script_instance *instance);

// Installs a C++ replacement for Script/Progression menu handlers whose Xbox
// bytecode is not reliable through the PC VM bridge.  Returns false when the
// handler should continue through the normal script path.
bool install_native_debug_menu_handler(debug_menu_entry *entry,
                                       const char *handler_name);

// Debug-menu callbacks run from resource_manager::frame_advance(). Queue
// native character/hero actions there; the game-tick wrapper executes them
// after the resource callback has returned on both PC and XBPACK targets.
void queue_debug_character_spawn(const char *name,
                                 float offset_x = 0.0f,
                                 float offset_z = 2.0f);
void queue_debug_character_pop();
void queue_debug_character_cleanup();
void process_debug_character_spawn_queue();
void clear_debug_character_spawns();

extern void script_lib_debug_menu_patch();
