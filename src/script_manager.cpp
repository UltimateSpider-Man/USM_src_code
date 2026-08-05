#include "script_manager.h"

#include "chuck_callbacks.h"
#include "func_wrapper.h"
#include "log.h"
#include "game.h"
#include "memory.h"
#include "mstring.h"
#include "parse_generic_mash.h"
#include "resource_manager.h"
#include "script_executable.h"
#include "script_executable_entry.h"
#include "script_object.h"
#include "script_executable_allocated_stuff_record.h"
#include "script_var_container.h"
#include "os_developer_options.h"
#include "os_file.h"
#include "trace.h"
#include "utility.h"
#include "variables.h"

#include <cassert>

#include <set.hpp>

#if !STANDALONE_SYSTEM
auto & script_manager_execs_pending_link_list = var<_std::list<script_executable_entry> *>(0x00965EF4);

auto & script_manager_execs_pending_first_run = var<_std::list<script_executable_entry> *>(0x00965EF8);

auto & script_manager_exec_map = var<_std::map<script_executable_entry_key, script_executable_entry> *>(0x00965EE4);

auto & script_manager_initialized = var<bool>(0x00965EE1);

auto & script_manager_next_stuff_id = var<int>(0x00965EFC);

auto & script_manager_script_allocated_stuff_map = var<_std::map<int, script_executable_allocated_stuff_record> *>(0x00965F00);

auto & script_manager_master_script = var<script_executable *>(0x00965EE8);

auto & script_manager_game_var_container = var<script_var_container *>(0x00965EEC);

auto & script_manager_shared_var_container = var<script_var_container *>(0x00965EF0);

auto & script_manager_time_inc = var<float>(0x00961930);

auto & script_manager_callbacks = var<_std::set<void (*)(script_manager_callback_reason, script_executable *, const char *)> *>(0x00965F04);

#else

#include <list>
#include <set>

#define make_var(type, name) \
    static type g_##name {}; \
    auto & name {g_##name}

std::list<script_executable_entry> *g_script_manager_execs_pending_first_run {nullptr};
auto & script_manager_execs_pending_first_run {g_script_manager_execs_pending_first_run};

std::list<script_executable_entry> *g_script_manager_execs_pending_link_list {nullptr};
auto & script_manager_execs_pending_link_list = g_script_manager_execs_pending_link_list;

std::map<script_executable_entry_key, script_executable_entry> *g_script_manager_exec_map {nullptr};
auto & script_manager_exec_map = g_script_manager_exec_map;

static bool g_script_manager_initialized {false};
auto & script_manager_initialized = g_script_manager_initialized;

static int g_script_manager_next_stuff_id {};
auto & script_manager_next_stuff_id {g_script_manager_next_stuff_id};

static std::map<int, script_executable_allocated_stuff_record> *
    g_script_manager_script_allocated_stuff_map {nullptr};
auto & script_manager_script_allocated_stuff_map = g_script_manager_script_allocated_stuff_map;

make_var(script_executable *, script_manager_master_script);

make_var(script_var_container *, script_manager_game_var_container);

make_var(script_var_container *, script_manager_shared_var_container);

make_var(float, script_manager_time_inc);

make_var(std::set<void (*)(script_manager_callback_reason, script_executable *, const char *)> *, script_manager_callbacks);

#undef make_var

#endif

namespace script_manager {

void *get_game_var_address(const mString &a1, bool *a2, script_library_class **a3) {
    TRACE("script_manager::get_game_var_address", a1.c_str());

    assert(script_manager_game_var_container != nullptr);
    assert(script_manager_shared_var_container != nullptr);

    auto result = script_manager_game_var_container->get_script_var_address(a1.c_str(), a3);
    if (result != nullptr)
    {
        if (a2 != nullptr) {
            *a2 = true;
        }
    }
    else
    {
        result = script_manager_shared_var_container->get_script_var_address(a1.c_str(), a3);
        if (a2 != nullptr) {
            *a2 = false;
        }
    }

    if (result == nullptr && a3 == nullptr) {
        mString v1{"unknown game/shared var "};
        v1 += a1;

        sp_log("%s", v1.c_str());
        assert(0);
    }

    return result;
}

char *get_game_var_address(int a1)
{
    TRACE("script_manager::get_game_var_address");

    assert(script_manager_game_var_container != nullptr);
    return script_manager_game_var_container->get_address(a1);
}

char *get_shared_var_address(int a1)
{
    TRACE("script_manager::get_shared_var_address");

    assert(script_manager_shared_var_container != nullptr);
    return script_manager_shared_var_container->get_address(a1);
}

bool using_chuck_old_fashioned() {
    return os_developer_options::instance->get_flag(mString {"CHUCK_OLD_FASHIONED"});
}

int get_total_loaded()
{
    int result = (script_manager_exec_map != nullptr
                    ? script_manager_exec_map->size()
                    : 0
                    );
    return result;
}

void init_game_var()
{
    TRACE("script_manager::init_game_var");

    if constexpr (1)
    {
        if ( script_manager_game_var_container == nullptr )
        {
            resource_key a1 {string_hash {"master"}, RESOURCE_KEY_TYPE_SCRIPT_GV};
            auto *v0 = resource_manager::get_resource(a1, nullptr, nullptr);
            if ( v0 != nullptr )
            {
                auto allocated_mem = parse_generic_object_mash(script_manager_game_var_container,
                    v0,
                    nullptr,
                    nullptr,
                    nullptr,
                    0,
                    0,
                    nullptr);

                assert(script_manager_game_var_container != nullptr);

                assert(!allocated_mem);
            }
        }

        if ( script_manager_shared_var_container == nullptr ) {
            resource_key a1 {string_hash {"master"}, RESOURCE_KEY_TYPE_SCRIPT_SV};
            auto *v1 = resource_manager::get_resource(a1, nullptr, nullptr);
            if ( v1 != nullptr ) {
                auto allocated_mem = parse_generic_object_mash(
                    script_manager_shared_var_container,
                    v1,
                    nullptr,
                    nullptr,
                    nullptr,
                    0,
                    0,
                    nullptr);

                assert(script_manager_shared_var_container != nullptr);
                assert(!allocated_mem);
            }
        }
    }
    else
    {
        CDECL_CALL(0x0059EE90);
    }
}

void link()
{
    TRACE("script_manager::link");

    if constexpr (1)
    {
        for ( auto &entry : (*script_manager_execs_pending_link_list) )
        {
            if ( !entry.exec->is_linked() ) {
                script_manager::run_callbacks((script_manager_callback_reason)6, entry.exec, entry.field_8);
                entry.exec->link();
                script_manager::run_callbacks((script_manager_callback_reason)7, entry.exec, entry.field_8);
            }

            script_manager_execs_pending_first_run->push_back(entry);
        }

        script_manager_execs_pending_link_list->clear();
    }
    else
    {
        CDECL_CALL(0x005A3620);
    }
}

int load_game_var_buffer(char *a1)
{
    TRACE("script_manager::load_game_var_buffer");

    if (a1 == nullptr || script_manager_game_var_container == nullptr) {
        return 0;
    }

    assert(script_manager_game_var_container != nullptr);

    return script_manager_game_var_container->load_script_var_buffer(a1);
}

void run_callbacks(script_manager_callback_reason a1, script_executable *a2, const char *a3)
{
    TRACE("script_manager::run_callbacks");

    assert(script_manager_callbacks != nullptr);

    if constexpr (1) {
        for ( auto &cb : (*script_manager_callbacks) )
        {
            cb(a1, a2, a3);
        }
    } else {
        CDECL_CALL(0x005A0AC0, a1, a2, a3);
    }
}

bool is_loadable(const resource_key &a1)
{
    TRACE("script_manager::is_loadable");

    // A chunk-format .pcsx drop is loadable without being in any pack: it is
    // read off disk by script_executable::load, so get_resource below knows
    // nothing about it (script_object.h).
    if (modPCSXGetChunkDir(a1.m_hash.source_hash_code, nullptr))
        return true;

    resource_key a1a {a1.m_hash, RESOURCE_KEY_TYPE_SCRIPT};
    int mash_data_size = 0;
    return (resource_manager::get_resource(a1a, &mash_data_size, nullptr) != nullptr);
}

int register_allocated_stuff_callback(
        void (*a1)(script_executable *, _std::list<uint32_t> &, _std::list<mString> &))
{
    TRACE("script_manager::register_allocated_stuff_callback");

    if ( script_manager_script_allocated_stuff_map == nullptr ) {
        using map_t = std::decay_t<decltype(*script_manager_script_allocated_stuff_map)>;
        script_manager_script_allocated_stuff_map = new map_t {};
        assert(script_manager_script_allocated_stuff_map != nullptr);
    }

    auto v7 = script_manager_next_stuff_id++;
    script_executable_allocated_stuff_record a2{};
    a2.field_0 = a1;
    script_manager_script_allocated_stuff_map->insert({v7, a2});
    return v7;
}

script_executable_entry * load(const resource_key &a1, uint32_t a2, void *a3, const resource_key &a4)
{
    TRACE("script_manager::load", a1.get_platform_string(g_platform).c_str());

    assert(script_manager_exec_map != nullptr);

    if constexpr (1)
    {
        script_executable_entry_key key{};
        key.field_0 = a1;
        key.field_8 = a4;

        assert(script_manager_exec_map != nullptr);
        
        script_executable_entry *result = nullptr;
        if ( auto it = script_manager_exec_map->find(key);
            it != script_manager_exec_map->end() )
        {
            auto *v27 = &it->second;
            ++v27->field_4;
            result = v27;
        }
        else
        {
            // A chunk-format .pcsx dropped under the mod root is the CHUCK
            // compiler's text output, not a mash image, so it can only come in
            // through the old-fashioned loader -- reading its .pcsx/.pcsst/
            // .pcpst/.pcsxl set out of the mod folder instead of "scripts\".
            // This is per-key: every other script still loads from the packs,
            // so dropping one script never disturbs the rest.
            std::string pcsx_chunk_dir, pcsx_chunk_stem;
            const bool from_pcsx_chunk =
                    modPCSXGetChunkDir(a1.m_hash.source_hash_code,
                                       &pcsx_chunk_dir, &pcsx_chunk_stem);
            const bool old_fashioned =
                    g_is_the_packer() || using_chuck_old_fashioned() || from_pcsx_chunk;

            script_executable_entry entry {};
            if ( old_fashioned )
            {
                entry.exec = new script_executable {};
            }
            else
            {
                entry.exec = bit_cast<script_executable *>(resource_manager::get_resource(a1, nullptr, nullptr));
            }

            assert(entry.exec != nullptr);

            entry.field_4 = 1;
            entry.field_8 = static_cast<const char *>(a3);
            if ( old_fashioned )
            {
                if ( from_pcsx_chunk )
                {
                    sp_log("[mod] loading chunk-format pcsx script \"%s\" from \"%s%s.pcsx\"",
                           a1.m_hash.to_string(), pcsx_chunk_dir.c_str(),
                           pcsx_chunk_stem.c_str());
                    entry.exec->load(a1, mString {pcsx_chunk_dir.c_str()},
                                     mString {pcsx_chunk_stem.c_str()});
                }
                else
                {
                    entry.exec->load(a1);
                }
            }
            else
            {
#ifdef TARGET_XBOX
                bool allocated_mem = false;
                {
                    auto *a1 = (generic_mash_header *) entry.exec;
                    generic_mash_data_ptrs a4;
                    entry.exec = (script_executable *) parse_generic_mash_init(a1, entry.exec, &allocated_mem, &a4, 0x60, nullptr, nullptr, 0, 0, nullptr);
                    entry.exec->un_mash_start(a1, entry.exec, &a4, nullptr);
                }
#else

                auto allocated_mem = parse_generic_object_mash(
                    entry.exec,
                    entry.exec,
                    nullptr,
                    nullptr,
                    nullptr,
                    0,
                    0,
                    nullptr);

                assert(entry.exec != nullptr);
                assert(!allocated_mem);
#endif

            }

            if ( (a2 & 1) != 0 )
            {
                if ( script_manager_master_script != nullptr )
                {
                    script_manager::run_callbacks(
                        (script_manager_callback_reason)4,
                        nullptr,
                        "trying to load more than one master script!");
                }

                script_manager_master_script = entry.exec;
            }

            assert(script_manager_execs_pending_link_list != nullptr);

            script_manager_execs_pending_link_list->push_back(entry);

            for ( auto &v23 : (*script_manager_script_allocated_stuff_map) )
            {
                auto &v9 = v23.second;
                auto &v7 = v23.first;
                entry.exec->register_allocated_stuff_callback(v7, v9.field_0);
            }

            script_manager_exec_map->insert({key, entry});
            
            if ( auto it = script_manager_exec_map->find(key);
                it != script_manager_exec_map->end() )
            {
                result = &it->second;
            }
            else
            {
                assert("Entry was not added to list" && 0);
                result = nullptr;
            }
        }

        return result;
    }
    else
    {
        script_executable_entry * (*func)(const resource_key *a1, uint32_t a2, void *a3, const resource_key *a4) = CAST(func, 0x005B0750);
        return func(&a1, a2, a3, &a4);
    }
}

script_object *find_global_object()
{
    TRACE("script_manager::find_global_object");

    if ( script_manager_master_script != nullptr ) {
        return script_manager_master_script->get_global_object();
    }

    return nullptr;
}

float get_time_inc()
{
    return script_manager_time_inc;
}

void init()
{
    TRACE("script_manager::init");

    if constexpr (1)
    {
        if ( !script_manager_initialized )
        {
            script_manager_time_inc = 0.0;

            using script_manager_execs_pending_link_list_t = std::decay_t<decltype(*script_manager_execs_pending_link_list)>;
            script_manager_execs_pending_link_list = new  script_manager_execs_pending_link_list_t {};
            assert(script_manager_execs_pending_link_list != nullptr);

            using script_manager_execs_pending_first_run_t = std::decay_t<decltype(*script_manager_execs_pending_first_run)>;
            script_manager_execs_pending_first_run = new  script_manager_execs_pending_first_run_t {};
            assert(script_manager_execs_pending_first_run != nullptr);

            if ( script_manager_exec_map == nullptr )
            {
                using script_manager_exec_map_t = std::decay_t<decltype(*script_manager_exec_map)>;
                script_manager_exec_map = new script_manager_exec_map_t {};

                assert(script_manager_exec_map != nullptr);
            }

            if ( script_manager_script_allocated_stuff_map == nullptr ) {
                using script_manager_script_allocated_stuff_map_t = std::decay_t<decltype(*script_manager_script_allocated_stuff_map)>;
                script_manager_script_allocated_stuff_map = new script_manager_script_allocated_stuff_map_t {};
                assert(script_manager_script_allocated_stuff_map != nullptr);
            }

            if ( script_manager_callbacks == nullptr )
            {
                using script_manager_callbacks_t = std::decay_t<decltype(*script_manager_callbacks)>;

                auto *mem = mem_alloc(12u);
                script_manager_callbacks = new (mem) script_manager_callbacks_t {};

                assert(script_manager_callbacks != nullptr);
            }

            script_manager_initialized = true;
        }
    }
    else
    {
        CDECL_CALL(0x005AFCE0);
    }
}

void kill()
{
    TRACE("script_manager::kill");

    assert(script_manager_initialized);

    script_manager::clear();
    if ( script_manager_execs_pending_link_list != nullptr )
    {
        delete script_manager_execs_pending_link_list;
        script_manager_execs_pending_link_list = nullptr;
    }

    if ( script_manager_execs_pending_first_run != nullptr )
    {
        delete script_manager_execs_pending_first_run;
        script_manager_execs_pending_first_run = nullptr;
    }

    if ( script_manager_exec_map != nullptr )
    {
        delete script_manager_exec_map;
        script_manager_exec_map = nullptr;
    }

    if ( script_manager_script_allocated_stuff_map != nullptr )
    {
        delete script_manager_script_allocated_stuff_map;
        script_manager_script_allocated_stuff_map = nullptr;
    }

    if ( script_manager_callbacks != nullptr )
    {
        delete script_manager_callbacks;
        script_manager_callbacks = nullptr;
    }

    script_manager_initialized = false;
}

FILE * host_fopen(const char *a1, char arg4)
{
    char v14[3] {};
    auto v7 = arg4 & 7;
    switch ( v7 )
    {
    case 1:
        v14[0] = 'r';
        break;
    case 2:
        v14[0] = 'w';
        break;
    case 4:
        v14[0] = 'a';
        break;
    default:
        v14[0] = 'r';
        break;
    }

    auto v8 = arg4 & 0x18;
    if ( v8 == 8 ) {
        v14[1] = 'b';
    } else if ( v8 == 16 ) {
        v14[1] = 't';
    } else {
        v14[1] = 'b';
    }

    v14[2] = 0;

    mString v13 {a1};
    if ( v13.at(1) != ':' && v13.at(0) != '\\' ) {
#if 0
        if ( !aDData[0] ) {
            debug_print_va("Trying to use os_file::root_dir before it has been set in host_fopen\n");
        }
#endif

        mString a2 {"C:\\usm\\pc_usm\\data\\"};
        v13 = a2 + mString {a1};
    }

    auto *v2 = v13.c_str();
    auto *v12 = fopen(v2, v14);
    return v12;
}

void dump_threads_to_console()
{
}

void dump_threads_to_file()
{
    TRACE("script_manager::dump_threads_to_file");

    auto *file = host_fopen("scriptdump.txt", 2);
    fprintf(file, "instance thread time ops\n");

    assert(script_manager_exec_map != nullptr);

    for ( auto &v0 : (*script_manager_exec_map) ) {
        v0.second.exec->dump_threads_to_file(file);
    }

    fclose(file);
}

void run(Float a1, bool a2)
{
    TRACE("script_manager::run");

    if constexpr (1)
    {
        script_manager_time_inc = a1;
        if ( !script_manager_execs_pending_first_run->empty() )
        {
            for ( auto &entry : (*script_manager_execs_pending_first_run) ) {
                run_callbacks((script_manager_callback_reason)8, entry.exec, entry.field_8);
                entry.exec->first_run(a1, a2);
                run_callbacks((script_manager_callback_reason)9, entry.exec, entry.field_8);
            }

            script_manager_execs_pending_first_run->clear();
        }

        assert(script_manager_exec_map != nullptr);

        auto it = script_manager_exec_map->begin();
        auto end = script_manager_exec_map->end();
        for ( ; it != end; ++it )
        {
            if ( !script_manager_execs_pending_first_run->empty() ) {
                break;
            }

            auto &exec = it->second.exec;
            if ( (exec->flags & 8) == 0 )
            {
                run_callbacks((script_manager_callback_reason)10, exec, it->second.field_8);
                exec->run(a1, a2);
                run_callbacks((script_manager_callback_reason)11, exec, it->second.field_8);
            }
        }

    }
    else
    {
        CDECL_CALL(0x005AF9F0, a1, a2);
    }
}

void destroy_game_var() {
    TRACE("script_manager::destroy_game_var");

    if constexpr(1) {
        if ( script_manager_game_var_container != nullptr )
        {
            if ( (script_manager_game_var_container->flags & 1) == 0 )
            {
                auto &v0 = script_manager_game_var_container;
                v0->destroy();
                operator delete(v0);
            }

            script_manager_game_var_container = nullptr;
        }

        if ( script_manager_shared_var_container != nullptr )
        {
            if ( (script_manager_shared_var_container->flags & 1) == 0 )
            {
                auto &v1 = script_manager_shared_var_container;
                v1->destroy();
                operator delete(v1);
            }

            script_manager_shared_var_container = nullptr;
        }
    } else {
        CDECL_CALL(0x005A52F0);
    }
}

#if !STANDALONE_SYSTEM
_std::map<script_executable_entry_key, script_executable_entry> *get_exec_list()
#else
std::map<script_executable_entry_key, script_executable_entry> *get_exec_list()
#endif
{
    return script_manager_exec_map;
}

script_object *find_object(const string_hash &a1) {
    TRACE("script_manager::find_object");

    assert(script_manager_exec_map != nullptr);

    for ( auto &v1 : (*script_manager_exec_map) )
    {
        auto *so = v1.second.exec->find_object(a1, nullptr);
        if ( so != nullptr) {
            return so;
        }
    }

    return nullptr;
}

script_object *find_object(
        const resource_key &a1,
        const string_hash &a2,
        const resource_key &a3)
{
    TRACE("script_manager::find_object");

    if ( a1.m_hash == string_hash {0} ) {
        return find_object(a2);
    }

    assert(script_manager_exec_map != nullptr);

    script_executable_entry_key v14{};
    v14.field_0 = a1;
    v14.field_8 = a3;
    auto v13 = script_manager_exec_map->find(v14);
    if ( auto end = script_manager_exec_map->end();
            v13 != end ) {
        auto &v5 = (*v13);
        auto *so = v5.second.exec->find_object(a2, nullptr);
        return so;
    }

    return nullptr;
}

void add_global_constructor_thread(Float a1, bool a2)
{
    TRACE("script_manager::add_global_constructor_thread");

    assert(script_manager_exec_map != nullptr);
    if ( a2 )
    {
        assert(script_manager_master_script != nullptr);
        script_manager_master_script->sub_5AB510(a1);
    }
    else
    {
        for ( auto &v2 : (*script_manager_exec_map) ) {
            v2.second.exec->sub_5AB510(a1);
        }
    }
}

void un_load(const resource_key &a1, bool a2, const resource_key &a3) {
    TRACE("script_manager::un_load");

    script_executable_entry_key v32{};
    v32.field_0 = a1;
    v32.field_8 = a3;

    assert(script_manager_exec_map != nullptr);

    auto a1a = script_manager_exec_map->find(v32);
    if ( auto end = script_manager_exec_map->end();
            a1a == end )
    {
        ;
        auto v9 = mString {"Trying to un load a non-loaded script executable '"}
                + mString {a1.m_hash.to_string()}
                + mString {"'"};
        [[maybe_unused]] auto *v5 = v9.c_str();
        //script_manager::run_callbacks((script_manager_callback_reason)5, nullptr, v5);
    }
    else
    {
        auto &v6 = (*a1a);
        auto *found = &v6.second;
        --v6.second.field_4;
        if ( found->field_4 == 0 )
        {
            assert(script_manager_execs_pending_link_list != nullptr);

            auto pending_it = script_manager_execs_pending_link_list->begin();
            auto end = script_manager_execs_pending_link_list->end();
            while ( pending_it != end ) {
                assert(pending_it->exec != found->exec && "why are we trying to un_load a non-linked executable?");
                ++pending_it;
            }

            if ( found->exec == script_manager_master_script ) {
                script_manager_master_script = nullptr;
            }

            found->exec->un_load(a2);
            if ( !script_manager_execs_pending_first_run->empty() )
            {
                auto v27 = script_manager_execs_pending_first_run->begin();
                auto v26 = script_manager_execs_pending_first_run->end();
                while ( v27 != v26 )
                {
                    auto &v8 = (*v27);
                    if ( v8.exec == found->exec )
                    {
                        script_manager_execs_pending_first_run->erase(v27);
                        break;
                    }

                    ++v27;
                }
            }

            if ( found->exec->is_from_mash() )
            {
                found->exec->release_mem();
            }
            else
            {
                auto *exec = found->exec;
                if ( exec != nullptr ) {
                    delete exec;
                }
            }

            script_manager_exec_map->erase(a1a);
        }
    }
}

void clear()
{
    TRACE("script_manager::clear");

    if constexpr (1)
    {
        script_manager_time_inc = 0.0;
        script_manager_master_script = nullptr;

        assert(script_manager_exec_map != nullptr);
        while ( !script_manager_exec_map->empty() )
        {
            auto it = script_manager_exec_map->begin();
            auto *exec = it->second.exec;
            exec->un_load(false);
            if ( exec->is_from_mash() ) {
                exec->release_mem();
            } else if ( exec != nullptr ) {
                delete exec;
            }

            script_manager_exec_map->erase(it);
        }
        
        assert(script_manager_exec_map->size() == 0);

        script_manager_execs_pending_link_list->clear();
        script_manager_execs_pending_first_run->clear();

        assert(script_manager_callbacks != nullptr);
        script_manager_callbacks->clear();
    }
    else
    {
        CDECL_CALL(0x005B0640);
    }
}

vm_executable *find_function_by_address(const uint16_t *a1)
{
    TRACE("script_manager::find_function_by_address");

    if constexpr(1)
    {
        if ( script_manager_exec_map == nullptr ) {
            return nullptr;
        }

        for ( auto &v2 : (*script_manager_exec_map) )
        {
            if ( v2.second.exec != nullptr )
            {
                auto *v5 = v2.second.exec->find_function_by_address(a1);
                if ( v5 != nullptr ) {
                    return v5;
                }
            }
        }

        return nullptr;
    } else {
        return (vm_executable *) CDECL_CALL(0x0059ED70, a1);
    }
}

vm_executable *find_function_by_name(string_hash a1) {
    TRACE("script_manager::find_function_by_name");

    assert(script_manager_exec_map != nullptr);
    for ( auto &v1 : (*script_manager_exec_map) ) {
        auto *exec = v1.second.exec;
        auto *func = exec->find_function_by_name(a1);
        if ( func != nullptr) {
            return func;
        }
    }

    return nullptr;
}

script_executable_entry *find_entry(const script_executable *a1) {
    TRACE("script_manager::find_entry");

    if constexpr(1) {
        assert(script_manager_exec_map != nullptr);

        for ( auto &p : (*script_manager_exec_map) ) {
            if ( p.second.exec == a1 ) {
                return &p.second;
            }
        }

        return nullptr;
    } else {
        return (script_executable_entry *) CDECL_CALL(0x0059EE10, a1);
    }
}

int save_game_var_buffer(char *a1)
{
    TRACE("script_manager::save_game_var_buffer");
    sp_log("0x%08X", int(a1));

    if constexpr (1)
    {
        if ( a1 == nullptr && script_manager_game_var_container == nullptr ) {
            return 0;
        }

        assert(script_manager_game_var_container != nullptr);
        return script_manager_game_var_container->save_script_var_buffer(a1);

    } else {
        return CDECL_CALL(0x0058F4C0, a1);
    }
}

int register_callback(
        void (*a2)(script_manager_callback_reason, script_executable *, const char *))
{
    TRACE("script_manager::register_callback");

    assert(script_manager_callbacks != nullptr && "need to initialize the script_manager first!!!");

    if constexpr (0)
    {
        auto ret = script_manager_callbacks->insert(a2);
        assert(ret.second && "c is already registered!!!!");

        return 0;
    }
    else
    {
        return CDECL_CALL(0x005A3600, a2);
    }
}

}

void *parse_generic_mash_init_hook(generic_mash_header *&header, void *a2, bool *allocated_mem, generic_mash_data_ptrs *a4, unsigned int struct_size, unsigned int *virtual_table_lookup, unsigned int *size_table_lookup, unsigned int num_table_entries, unsigned int base_class_size, void *a10)
{
#ifdef TARGET_XBOX
    struct_size = 0x60;
#endif

    return parse_generic_mash_init(header, a2, allocated_mem, a4, struct_size, virtual_table_lookup,
            size_table_lookup, num_table_entries, base_class_size, a10);
}

void script_manager_patch()
{
    {
        REDIRECT(0x005B0834, parse_generic_mash_init_hook);
    }

    {
        REDIRECT(0x0055C951, register_chuck_callbacks);
        REDIRECT(0x0055BEA7, register_chuck_callbacks);
    }

    {

        void (* run_callbacks)(script_manager_callback_reason a1, script_executable *a2, const char *a3) = &script_manager::run_callbacks;
        SET_JUMP(0x005A0AC0, run_callbacks);
    }

    {
        using find_object_t = script_object * (*)(
                const resource_key &,
                const string_hash &,
                const resource_key &);
        find_object_t find_object = &script_manager::find_object;
        SET_JUMP(0x005A0870, find_object);
    }

    REDIRECT(0x006607E5, script_manager::register_callback);

    SET_JUMP(0x005A52F0, script_manager::destroy_game_var);

    SET_JUMP(0x0058F470, script_manager::get_shared_var_address);

    {
        void * (*func)(const mString &, bool *, script_library_class **) = &script_manager::get_game_var_address;
        SET_JUMP(0x005A09B0, func);
    }

    {
        char * (*func)(int) = &script_manager::get_game_var_address;
        SET_JUMP(0x0058F460, func);
    }

    SET_JUMP(0x0058F480, script_manager::load_game_var_buffer);

    SET_JUMP(0x0058F4C0, script_manager::save_game_var_buffer);

    SET_JUMP(0x0058F390, script_manager::find_global_object);

    SET_JUMP(0x0058F400, script_manager::get_time_inc);

    SET_JUMP(0x005AFE40, script_manager::register_allocated_stuff_callback); 

    SET_JUMP(0x005AB5D0, script_manager::add_global_constructor_thread);

    SET_JUMP(0x0059EE10, script_manager::find_entry);

    SET_JUMP(0x0059EDC0, script_manager::find_function_by_name);

    SET_JUMP(0x0059ED70, script_manager::find_function_by_address);

    SET_JUMP(0x0058F500, script_manager::get_exec_list);

    SET_JUMP(0x005A3620, script_manager::link);

    SET_JUMP(0x005AF9F0, script_manager::run);

    SET_JUMP(0x005AFCE0, script_manager::init);

    SET_JUMP(0x005B0970, script_manager::kill);

    SET_JUMP(0x005B0640, script_manager::clear);

    SET_JUMP(0x005B04F0, script_manager::un_load);

    {
        REDIRECT(0x0055C8A6, script_manager::is_loadable);
    }

    SET_JUMP(0x0059EE90, script_manager::init_game_var);

    //REDIRECT(0x005DF0B7, script_manager::load);

    if constexpr (1)
    {
        SET_JUMP(0x005B0750, script_manager::load);
    }
    else
    {
        REDIRECT(0x005B07E9, resource_manager::get_resource);

        REDIRECT(0x005B0834, parse_generic_mash_init);

        {
            FUNC_ADDRESS(address, &script_executable::un_mash);
            REDIRECT(0x005B0850, address);
        }

        {
            FUNC_ADDRESS(address, &script_executable::register_allocated_stuff_callback);
            REDIRECT(0x005B08B0, address);

        }
    }
}
