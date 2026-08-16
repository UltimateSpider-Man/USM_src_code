#pragma once

#include "script_library_class.h"

#include "vm_stack.h"

struct slf__create_debug_menu_entry__str__str__t : script_library_class::function {
    slf__create_debug_menu_entry__str__str__t(const char *a3);

    struct parms_t {
        vm_str_t str0;
        vm_str_t str1;
    };
	

    bool operator()(vm_stack &stack, [[maybe_unused]]script_library_class::function::entry_t entry) const;
};

extern int vm_debug_menu_entry_garbage_collection_id;

void construct_debug_menu_lib();

void init_script_debug_menu();

// Debug-menu callbacks run from resource_manager::frame_advance().  Queue
// character work there and execute it from game::frame_advance(), after the
// resource callback has returned.
void queue_debug_character_spawn(const char *name,
                                 float offset_x = 0.0f,
                                 float offset_z = 2.0f);
void queue_debug_character_pop();
void queue_debug_character_cleanup();
void process_debug_character_spawn_queue();
void clear_debug_character_spawns();

extern void script_lib_debug_menu_patch();
