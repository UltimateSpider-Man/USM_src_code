#pragma once

#include "string_hash.h"
#include "variable.h"

#include "func_wrapper.h"

struct script_instance;
struct script_object;
struct vm_thread;

namespace script {

//0x0064E4F0
extern int find_function(string_hash a1, const script_object *a2, bool a3);

// ---------------------------------------------------------------------------
// External .PCSX script loading (script.cpp)
//
// resource_manager::get_resource substitutes registered extra/**/*.pcsx
// images for script fetches the game already makes (modPCSXGetOverride,
// script_object.h). These two drive the other half: actually loading .pcsx
// drops the retail data never asks for, so brand-new scripts run.
// ---------------------------------------------------------------------------

// True while script_manager holds a loaded exec for this script name hash.
extern bool is_script_loaded(uint32_t name_hash);

// Resolve a function name across the script objects of the loaded external
// .pcsx scripts, by mangled name first then by short name. Returns the index
// within *owner_out, or -1. The index is ONLY valid against *owner_out --
// find_function cannot hand it back to its own callers, which pair the index
// with an instance of the object they passed in.
extern int find_pcsx_function(string_hash name, script_object **owner_out);

// Resolve and start a function defined in an external .pcsx script, on a
// global instance of its own script object. Returns the new thread, or
// nullptr (logged) if the name is unknown or its object has no global
// instance. This is the safe way to invoke a .pcsx function by name.
extern vm_thread *run_pcsx_function(string_hash name);

// Load every registered .pcsx drop that is not loaded yet, then link them.
// owner_slot is the resource_pack_slot the execs are attributed to (the
// common slot at the game::load_world call site); it may be nullptr.
// Returns how many were loaded. A no-op (returns 0, logs why) unless
// script_manager is initialized and a resource context is pushed, so a
// mistimed call cannot crash.
extern int load_external_pcsx_scripts(void *owner_slot);

inline Var<script_object *> gso {0x0096BB4C};

inline Var<script_instance*> gsoi {0x0096BB50};

inline script_instance * get_gsoi()
{
    return script::gsoi();
}

inline script_object * get_gso()
{
    return script::gso();
}

inline bool exec_thread(bool a1) {
    return (bool)CDECL_CALL(0x0064E740, a1);
}

// 0x0064E770
inline bool exec_thread_no_wait() {
    return (bool)CDECL_CALL(0x0064E770);
}



inline vm_thread * new_thread(int a1, script_instance *a2){
    return (vm_thread *)CDECL_CALL(0x0064E520, a1, a2);
}


inline vm_thread* sub_5028B0(string_hash function_hash, script_instance* instance)
{
     return (vm_thread*)CDECL_CALL(0x005028B0, function_hash, instance);
}

} // namespace script

extern void script_patch();
