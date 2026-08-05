#pragma once

#include "mstring.h"
#include "string_hash.h"
#include "so_data_block.h"
#include "float.hpp"
#include "msimpletemplates_guts.h"

#include <list.hpp>
#include <set.hpp>

#include <set>

#include <cstdio>
#include <cstdint>
#include <filesystem>
#include <vector>

struct chunk_file;
struct vm_executable;
struct vm_thread;
struct script_instance;
struct script_executable;
struct generic_mash_header;
struct generic_mash_data_ptrs;

inline constexpr auto SCRIPT_OBJECT_FLAG_GLOBAL = (1u << 0);
inline constexpr auto SCRIPT_OBJECT_FLAG_EXTERNAL = (1u << 2);

struct vm_symbol {
    mString field_0;
    mString field_C;
    mString field_18;
    mString field_24;
    int field_30;
    int field_34;
    bool field_38;

    void read(chunk_file *file);
};

class script_object {
public:
    string_hash name;
    script_executable *parent;
    script_instance *global_instance;

    struct debug_info_t {
        string_hash field_0;
        _std::list<vm_symbol> field_4;
        _std::list<vm_symbol> field_10;

        ~debug_info_t() {
            void (__fastcall *func)(void *) = CAST(func, 0x005B7BE0);
            func(this);
        }
    };

    debug_info_t *debug_info;
    so_data_block static_data;
    int data_blocksize;
    vm_executable **funcs;
    int total_funcs;
    int field_28;
    simple_list<script_instance> *instances;
    uint32_t flags;

public:
    script_object();

    //0x005AF6C0
    ~script_object();

    void * operator new(size_t );

    void operator delete(void *, size_t );

    bool is_external_object() const {
        return (this->flags & SCRIPT_OBJECT_FLAG_EXTERNAL) != 0;
    }

    bool is_global_object() const {
        return (this->flags & SCRIPT_OBJECT_FLAG_GLOBAL) != 0;
    }

    auto *get_parent() const {
        return parent;
    }

    void set_parent(script_executable *p) {
        this->parent = p;
    }

    auto * get_global_instance() {
        return this->global_instance;
    }

    auto & get_static_data() {
        return this->static_data;
    }

    char * get_static_data_buffer() const {
        return this->static_data.get_buffer();
    }

    int get_static_data_size() const {
        return this->static_data.size();
    }

    auto &get_name() const {
        return name;
    }


    int get_constructor_parmsize();

    //0x005A0750
    void constructor_common();

    //0x005AD9A0
    void destructor_common();

    void release_mem();

    //0x005AF3E0
    void destroy();

    //0x005AF320
    void create_destructor_instances();

    //0x005AF750
    void run(bool a2);

    bool has_threads() const;

    void quick_un_mash();

    void dump_threads_to_file(FILE *a2);

    //0x005AB120
    script_instance *add_instance(string_hash a2, char *a3, vm_thread **a4);

    //0x005AB200
    script_instance *add_instance(string_hash a2, vm_executable *a3);

    //0x005ADC60
    void remove_instance(script_instance *a2);

    script_instance *add_game_init_instance(string_hash a2, int a3);

    //0x0059ECC0
    void add(script_instance *a2);

	vm_thread *add_thread(script_instance *a2, int fidx);

    //0x00599530
    void link(const script_executable *a2);

    //0x005AB350
    void un_mash(generic_mash_header *a2, void *a3, void *a4, generic_mash_data_ptrs *a5);

    //0x005AAEF0
    void create_auto_instance(Float arg0);

    vm_executable *get_func(int);

    int get_size_instances() const;

    //0x0058EF80
    int find_func(string_hash a2) const;

    int find_func_short(string_hash a2) const;

    int find_function_by_address(const uint16_t *a2) const;

    static void read(chunk_file *file, script_object *so);

    struct function {
        const script_object *field_0;
        string_hash field_4;
        int field_8;
        int field_C;
    };

    static inline Var<function[20]> function_cache {0x00966D10};

    static inline Var<int> usage_counter {0x00965ED4};
};

enum script_instance_callback_reason_t {
};


class script_instance {
public:
    simple_list<script_instance>::vars_t simple_list_vars;

private:
    string_hash name;
    so_data_block data;
    simple_list<vm_thread> threads;

public:
    vm_executable *field_28;

    script_object *parent;
    uint32_t flags;
    void (* m_callback)(script_instance_callback_reason_t, script_instance *, vm_thread *, void *);
    _std::set<void *> field_38;

public:
    //0x005AAA40
    script_instance(string_hash a2,
        int size,
        unsigned int a4);

    ~script_instance();

    void * operator new(size_t size);

    void operator delete(void *, size_t);

    auto & get_name() const {
        return name;
    }

    vm_thread * add_thread_for_debug_menu(const vm_executable *a2)
    {
        vm_thread * (__fastcall *func)(void *, void *, const vm_executable *) = CAST(func, 0x005AAC20);
        return func(this, nullptr, a2);
    }

    void add_thread_for_debug_menu(const vm_executable *a2, const char *a3)
    {
        void (__fastcall *func)(void *, void *, const vm_executable *, const char *) = CAST(func, 0x005AAD00);
        func(this, nullptr, a2, a3);
    }

    script_object * get_parent() {
        return this->parent;
    }

    void set_parent(script_object *so) {
        this->parent = so;
    }

    char * get_buffer() const {
        return this->data.get_buffer();
    }

    auto get_size() const {
        return this->data.size();
    }

    bool run_single_thread(vm_thread *a2, bool a3);

    //0x005AAE60
    simple_list<vm_thread>::iterator delete_thread(
        simple_list<vm_thread>::iterator a3);

    void dump_threads_to_file(FILE *a2);

    void run(bool a2);

    //0x005AAD50
    void add_thread(void *a2, const vm_executable *a3, const char *a4);

    //0x0059EC70
    void run_callbacks(
        script_instance_callback_reason_t a2,
        vm_thread *a3);

    //0x005AF500
    void build_parameters();

    bool has_threads() const;

    //0x005AAC20
    vm_thread *add_thread(const vm_executable *a2);

    vm_thread *add_thread(const vm_executable *a1, const char *a2);

    void recursive_massacre_threads(vm_thread *root);

    //0x005ADB80
    void massacre_threads(const vm_executable *a2, const vm_thread *a3);

    //0x005AD8D0
    void kill_thread(const vm_executable *a2, const vm_thread *a3);

    //0x005A33F0
    void register_callback(
        void (*cb)(script_instance_callback_reason_t, script_instance *, vm_thread *, void *),
        void *user_data);

};

// ---------------------------------------------------------------------------
// .PCSX mods (script_object.cpp)
//
// A .pcsx dropped under the mod root is a packer-produced script_executable
// mash image (generic_mash_header + object image + mashed data with the exec
// code embedded — the same self-contained bytes a retail pack serves for
// RESOURCE_KEY_TYPE_SCRIPT). enumerate_mods() validates+registers them via
// modPCSXRegister under to_hash(stem) — the stem is the script name, e.g.
// extra/M01_INTRO.pcsx overrides the "m01_intro" script, and a stem that
// exists in no loaded pack is injected as a brand-new loadable script — and
// resource_manager::get_resource swaps the retail bytes for the mod image
// via modPCSXGetOverride when script_manager::load fetches the blob.
// ---------------------------------------------------------------------------

// Structural gate: header safety key, plain-object mash shape (class_id
// 0xFFFF, no vtable flag), mash-data offset.
extern bool modPCSXImageUsable(const uint8_t *bytes, size_t size);

// Validate + register one .pcsx under to_hash(stem) (and its literal-hash
// stem, if the name parses as one). Called by enumerate_mods().
extern bool modPCSXRegister(const std::filesystem::path &path,
                            std::vector<uint8_t> &&fileData);

// Writable image for the script name hash, or nullptr. Re-stamped from the
// pristine master bytes whenever no loaded exec is using it, so every load
// un_mashes and links virgin bytes like a re-streamed pack image.
extern uint8_t *modPCSXGetOverride(uint32_t nameHash, int *sizeOut);

// True when the bytes are a CHUCK-compiler chunk script, i.e. a text file
// whose first token is "scrobjs" -- the form a .pcsx on disk actually has,
// as opposed to a mash image lifted out of a pack.
extern bool modPCSXIsChunkImage(const uint8_t *bytes, size_t size);

// Directory (trailing separator included) and file stem of a registered
// chunk-format .pcsx for this script, or false if none. script_manager::load
// hands both to script_executable::load in place of "scripts\" and the
// name string_hash::to_string() would have produced.
extern bool modPCSXGetChunkDir(uint32_t nameHash, std::string *dirOut = nullptr,
                               std::string *stemOut = nullptr);

extern void script_instance_patch();
