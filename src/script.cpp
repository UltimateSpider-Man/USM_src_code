#include "script.h"

#include "func_wrapper.h"
#include "log.h"
#include "script_object.h"
#include "utility.h"




#include "chunk_file.h"

#include "func_wrapper.h"
#include "memory.h"
#include "parse_generic_mash.h"
#include "resource_key.h"
#include "resource_manager.h"
#include "script_executable.h"
#include "script_executable_entry.h"
#include "script_manager.h"
#include "vm_executable.h"
#include "vm_thread.h"
#include "common.h"
#include "trace.h"


#include <cassert>

namespace script {

// ---------------------------------------------------------------------------
// Loading external .PCSX scripts (extra/**/*.pcsx)
//
// resource_manager::get_resource already substitutes a registered .pcsx image
// for any RESOURCE_KEY_TYPE_SCRIPT fetch (see modPCSXGetOverride,
// script_object.cpp), which covers *replacing* a script the game already asks
// for by name. It does nothing for a script the retail data never mentions:
// no pack entry, no mission table row, so nothing ever requests it.
//
// load_external_pcsx_scripts() closes that gap by driving script_manager for
// those drops directly -- the same three calls game::load_world makes for
// init_gv/init_sv (load -> link -> first run happens on the next
// script_manager::run). Only drops that are NOT already loaded are touched,
// so calling it after the level's own scripts are up is safe and idempotent.
//
// It deliberately does not run itself: script_manager::load needs an
// initialized script_manager, a pushed resource context and the level's
// script_library_classes registered (slc_manager::init), which is only true
// well inside load_world. The guards below make a mistimed call a logged
// no-op instead of a crash, but the intended call site is right after the
// level's own script loads.
// ---------------------------------------------------------------------------

// Registered .pcsx drops, keyed by the engine hash of the file stem. The
// literal-hash aliases modPCSXRegister adds for hash-named drops share the
// same Mod::Path, so entries are de-duplicated by path to load each file once.
static std::vector<std::pair<uint32_t, const Mod *>> collect_pcsx_mods()
{
    std::vector<std::pair<uint32_t, const Mod *>> out;

    for (const auto &[hash, mod] : Mods)
    {
        // Both on-disk forms qualify: a mash image (served through
        // get_resource) and a chunk script (read off disk by
        // script_executable::load). Chunk entries deliberately carry no Data.
        const bool is_mash  = (mod.Type == MOD_TYPE_PCSX_FILE && !mod.Data.empty());
        const bool is_chunk = (mod.Type == MOD_TYPE_PCSX_CHUNK);
        if (!is_mash && !is_chunk)
            continue;

        const uint32_t stem_hash =
                to_hash(transformToLower(mod.Path.stem().string()).c_str());
        if (hash != stem_hash)
            continue;               // literal-hash alias of a drop already listed

        out.emplace_back(hash, &mod);
    }

    return out;
}

bool is_script_loaded(uint32_t name_hash)
{
    auto *execs = script_manager::get_exec_list();
    if (execs == nullptr)
        return false;

    for (auto &entry : (*execs))
    {
        if (entry.first.field_0.m_hash.source_hash_code == name_hash)
            return true;
    }

    return false;
}

int load_external_pcsx_scripts(void *owner_slot)
{
    TRACE("script::load_external_pcsx_scripts");

    const auto mods = collect_pcsx_mods();
    if (mods.empty())
        return 0;

    // Mistimed call: report it instead of tripping script_manager's or
    // resource_manager's asserts.
    if (script_manager::get_exec_list() == nullptr)
    {
        sp_log("[mod] pcsx load skipped: script_manager is not initialized yet");
        return 0;
    }

    if (resource_manager::get_resource_context() == nullptr)
    {
        sp_log("[mod] pcsx load skipped: no resource context is pushed");
        return 0;
    }

    int loaded = 0;
    for (const auto &[hash, mod] : mods)
    {
        const std::string stem = transformToLower(mod->Path.stem().string());

        if (is_script_loaded(hash))
        {
            sp_log("[mod] pcsx \"%s\" is already loaded, left alone", stem.c_str());
            continue;
        }

        const resource_key key {string_hash {stem.c_str()}, RESOURCE_KEY_TYPE_SCRIPT};

        // Sanity: this must resolve through modPCSXGetOverride. If it does
        // not, the drop's stem does not hash to the key the engine would use
        // and loading it would fetch nothing.
        if (!script_manager::is_loadable(key))
        {
            sp_log("[mod] pcsx \"%s\" (0x%08X) is not loadable - stem/hash mismatch?",
                   stem.c_str(), hash);
            continue;
        }

        const resource_key no_owner {};
        if (script_manager::load(key, 0u, owner_slot, no_owner) == nullptr)
        {
            sp_log("[mod] pcsx \"%s\": script_manager::load failed", stem.c_str());
            continue;
        }

        sp_log("[mod] loaded external pcsx script \"%s\" (0x%08X, %s)",
               stem.c_str(), hash,
               mod->Type == MOD_TYPE_PCSX_CHUNK ? "chunk format" : "mash image");
        ++loaded;
    }

    // Newly loaded execs sit on script_manager's pending-link list until
    // this runs; first_run/run then happen on the next script_manager::run.
    if (loaded > 0)
        script_manager::link();

    return loaded;
}

int find_pcsx_function(string_hash name, script_object **owner_out)
{
    if (owner_out != nullptr)
        *owner_out = nullptr;

    auto *execs = script_manager::get_exec_list();
    if (execs == nullptr)
        return -1;

    for (auto &entry : (*execs))
    {
        // Only the externally loaded .pcsx scripts: a retail exec's functions
        // are already reachable through the object the caller passed in.
        if (getMod(entry.first.field_0.m_hash.source_hash_code,
                   MOD_TYPE_PCSX_FILE) == nullptr)
            continue;

        auto *exec = entry.second.exec;
        if (exec == nullptr)
            continue;

        for (int i = 0; i < exec->total_script_objects; ++i)
        {
            auto *so = exec->script_objects[i];
            if (so == nullptr)
                continue;

            int idx = so->find_func(name);
            if (idx < 0)
                idx = so->find_func_short(name);

            if (idx >= 0)
            {
                if (owner_out != nullptr)
                    *owner_out = so;
                return idx;
            }
        }
    }

    return -1;
}

vm_thread *run_pcsx_function(string_hash name)
{
    script_object *owner = nullptr;
    const int idx = find_pcsx_function(name, &owner);
    if (idx < 0 || owner == nullptr)
    {
        sp_log("[mod] pcsx function \"%s\" not found in any loaded .pcsx script",
               name.to_string());
        return nullptr;
    }

    // The thread has to run on an instance of the function's OWN object -
    // add_thread pushes that instance as the implicit first parameter, so
    // pairing the index with a foreign instance would corrupt the stack.
    auto *inst = owner->get_global_instance();
    if (inst == nullptr)
    {
        sp_log("[mod] pcsx function \"%s\" lives in \"%s\", which has no global "
               "instance to run it on", name.to_string(),
               owner->get_name().to_string());
        return nullptr;
    }

    sp_log("[mod] running pcsx function \"%s\" (\"%s\" func %d)",
           name.to_string(), owner->get_name().to_string(), idx);

    return owner->add_thread(inst, idx);
}

int find_function(string_hash a1, const script_object *a2, [[maybe_unused]] bool a3) {
    {
        auto *str = a1.to_string();

        sp_log("%s", str);
    }

    if (a2 == nullptr) {
        assert(0 && "Script has not been initted yet!");
        return -1;              // release builds must not fall into a null deref
    }

    int result = a2->find_func(a1);

    // find_func matches the MANGLED name ("toggle_hero(num)"), so a lookup by
    // bare name misses even when the function is right there. That is the
    // normal case for a name typed by hand or coming from an external .pcsx
    // script, where the caller has no signature to offer. find_func_short
    // matches on the short name, and the index it returns belongs to the same
    // object, so it stays valid for this function's callers (which pair it
    // with an instance of a2 - see script::new_thread).
    if (result < 0) {
        result = a2->find_func_short(a1);
        if (result >= 0) {
            sp_log("resolved \"%s\" by short name -> func %d",
                   a1.to_string(), result);
        }
    }

    if (result < 0) {
        // Last resort: the name may belong to an external .pcsx script rather
        // than to a2. Report where it lives - the index cannot be returned,
        // because every caller of this function applies it to an instance of
        // a2 and a foreign index would run the wrong function or read out of
        // bounds. Use script::run_pcsx_function to actually invoke it.
        script_object *owner = nullptr;
        if (const int pcsx_idx = find_pcsx_function(a1, &owner);
            pcsx_idx >= 0 && owner != nullptr)
        {
            sp_log("[mod] \"%s\" is not in \"%s\" but is func %d of \"%s\" "
                   "(external .pcsx) - call it with script::run_pcsx_function",
                   a1.to_string(), a2->get_name().to_string(), pcsx_idx,
                   owner->get_name().to_string());
        }

        result = -1;
    }

    return result;
}
} // namespace script

void script_patch() {
    SET_JUMP(0x0064E4F0, script::find_function);
}
