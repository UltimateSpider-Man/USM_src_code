#pragma once

#include "float.hpp"

#include <list.hpp>
#include <map.hpp>

#include <cstdint>

#if STANDALONE_SYSTEM
#include <map>
#endif

struct mString;
struct script_var_container;
struct script_library_class;
struct resource_key;
struct script_executable_entry_key;
struct script_executable_entry;
struct script_executable;
struct script_executable_allocated_stuff_record;
struct script_object;
struct string_hash;
struct vm_executable;

enum script_manager_callback_reason
{};

namespace script_manager {
    //0x005A09B0
    void *get_game_var_address(const mString &a1, bool *a2, script_library_class **a3);

    char *get_game_var_address(int a1);

    char *get_shared_var_address(int a1);

    //0x0058F4C0
    int save_game_var_buffer(char *a1);

    //0x005AFCE0
    void init();

    void dump_threads_to_console();

    void dump_threads_to_file();

    //0x005A3620
    void link();

    //0x005A0AC0
    void run_callbacks(script_manager_callback_reason a1, script_executable *a2, const char *a3);

    //0x0058F480
    int load_game_var_buffer(char *a1);

    void un_load(const resource_key &a1, bool a2, const resource_key &a3);

    //0x005B0750
    script_executable_entry *load(const resource_key &a1, uint32_t a2, void *a3, const resource_key &a4);

    //0x0059EE90
    void init_game_var();

    //0x0059EE10
    script_executable_entry *find_entry(const script_executable *a1);

    bool using_chuck_old_fashioned();

    //0x0058F3A0
    bool is_loadable(const resource_key &a1);

    //0x005A52F0
    void destroy_game_var();

    //0x005B0640
    void clear();

    //0x005B0970
    void kill();

    //0x005AF9F0
    void run(Float a1, bool a2);

    //0x0059ED70
    vm_executable *find_function_by_address(const uint16_t *a1);

    //0x0059EDC0
    vm_executable *find_function_by_name(string_hash a1);

    int get_total_loaded();


#if !STANDALONE_SYSTEM
    _std::map<script_executable_entry_key, script_executable_entry> *get_exec_list();
#else
    std::map<script_executable_entry_key, script_executable_entry> *get_exec_list();
#endif

    //0x0059ED20
    script_object *find_object(const string_hash &a1);
    
    //0x005A0870
    script_object *find_object(
        const resource_key &,
        const string_hash &,
        const resource_key &);

    //0x005AFE40
    int register_allocated_stuff_callback(
        void (*a1)(script_executable *, _std::list<uint32_t> &, _std::list<mString> &));

    script_object *find_global_object();

    float get_time_inc();

    int register_callback(
        void (*a2)(script_manager_callback_reason, script_executable *, const char *));
}

extern void script_manager_patch();
extern void script_manager_xbpack_patch();
