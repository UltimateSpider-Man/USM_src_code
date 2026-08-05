#pragma once

#include "fixedstring.h"
#include "float.hpp"
#include "mstring.h"
#include "string_hash.h"

#include <list.hpp>
#include <map.hpp>

#include <set>

#include <cstdio>

struct script_executable_allocated_stuff_record;
struct mString;
struct generic_mash_header;
struct generic_mash_data_ptrs;
struct resource_key;
struct script_object;
struct vm_executable;
struct vm_thread;
struct script_library_class;

struct script_executable {
    fixedstring<8> field_0;
    uint16_t *sx_exe_image;
    int sx_exe_image_size;
    script_object **script_objects;
    script_object **script_objects_by_name;
    int total_script_objects;
    script_object *global_script_object;
    char **permanent_string_table;
    int permanent_string_table_size;
    char **system_string_table;
    int system_string_table_size;

    struct var_to_slc_t {
        mString m_str;
        script_library_class *m_slc;
    };
    _std::map<mString, script_library_class *> *script_object_dummy_list;
    _std::map<int, script_executable_allocated_stuff_record> *script_allocated_stuff_map;
    uint32_t flags;
    struct info_t {
        string_hash field_0;
        string_hash so_name;
        vm_executable *field_8;
        int field_C;
        int field_10;
        int field_14;
        int field_18;

        void un_mash(
            generic_mash_header *header,
            script_executable *a3,
            void *,
            generic_mash_data_ptrs *a5);
    };

    info_t *field_54;
    int field_58;

    script_executable();

    void * operator new(size_t size);

    void operator delete(void *ptr, size_t size);

    static bool compare(const script_executable &, const script_executable &);

    //0x005AFC50
    void constructor_common();

    //0x005A34B0
    void add_allocated_stuff(
        int a2,
        uint32_t a3,
        const mString &a1);

    //0x005A0790
    void remove_allocated_stuff(int a2, uint32_t a3);
	
	    void add_allocated_stuff_for_debug_menu(int a2, int a3, int a4) {
        void  (__fastcall *func)(script_executable *, void *edx, int a2, int a3, int a4) = bit_cast<decltype(func)>(0x005A34B0);
        func(this, nullptr, a2, a3, a4);
    }

    //0x005A07F0
    int get_total_allocated_stuff(int a2);

    script_object *get_global_object()
    {
        return this->global_script_object;
    }

    //0x0059BBA0
    bool has_threads() const;

    bool is_linked() const {
        return (this->flags & SCRIPT_EXECUTABLE_FLAG_LINKED) != 0;
    }

    bool is_from_mash() const;

    bool is_un_mashed() const {
        return (this->flags & SCRIPT_EXECUTABLE_FLAG_UN_MASHED) != 0;
    }

    script_library_class * find_library_class(const mString &a2) const;

    const char *get_permanent_string(unsigned int index) const;

    static uint32_t get_system_string_index(const std::set<string_hash> &set, const string_hash &p);

    const char *get_system_string(unsigned int index) const;

    uint16_t * get_exec_code(unsigned int offset);

    //0x005A3500
    void link();

    //0x005AFF90
    void quick_un_mash();

    void dump_threads_to_file(FILE *a2);

    //0x005AF990
    void run(Float a2, bool a3);

    //0x005AB440
    void first_run(Float a2, bool a3);

    script_object *find_object(int index) const;

    script_object *find_object(
            const string_hash &a2,
            int *a3) const;

    void add_object(script_object *so, int &a3);

    void add_object_by_name(script_object *a1, int a3);

    void load(const resource_key &a1);

    // Same loader against an arbitrary directory (trailing separator
    // included) instead of the built-in "scripts\", optionally under an
    // explicit base filename instead of the one string_hash::to_string()
    // derives from the key -- which for a hash the engine's string table does
    // not know is a synthetic rendering, not the script's name. Used to read a
    // chunk-format .pcsx dropped under the mod root; see modPCSXGetChunkDir
    // (script_object.h) and script_manager::load.
    void load(const resource_key &a1, const mString &dir,
              const mString &name_override = mString {});

    //0x005B0470
    void release_mem();

    //0x005AF780
    void un_load(bool a2);

    //0x0058F280
    vm_executable *find_function_by_address(const uint16_t *a2) const;

    //0x0058F310
    vm_executable *find_function_by_name(string_hash a2) const;

    void un_mash_start(generic_mash_header *a2, void *a3, generic_mash_data_ptrs *a4, void *a5);

    void un_mash(generic_mash_header *a2, void *a3, generic_mash_data_ptrs *a4);

    //0x005AF460
    void register_allocated_stuff_callback(int a1, void (*a2)(script_executable *, _std::list<uint32_t> &, _std::list<mString> &));

    vm_thread *sub_5AB510(Float a2);

    static inline constexpr auto SCRIPT_EXECUTABLE_FLAG_LINKED = 1u;
    static inline constexpr auto SCRIPT_EXECUTABLE_FLAG_FROM_MASH = 2u;
    static inline constexpr auto SCRIPT_EXECUTABLE_FLAG_UN_MASHED = 4u;
};

extern void script_executable_patch();
