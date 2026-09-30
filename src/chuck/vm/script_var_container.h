#pragma once

#include "chuck/vm/so_data_block.h"

#include <map.hpp>

struct script_library_class;
struct generic_mash_header;
struct generic_mash_data_ptrs;
struct chunk_file;
struct mString;

class script_var_container {
    struct script_var_to_addr_t {
        char *name;
        int address;

        void un_mash(
                void *,
                generic_mash_header *,
                script_var_container *a3,
                void *,
                generic_mash_data_ptrs *a5);
    };

private:
    so_data_block script_var_block {};
    script_var_to_addr_t *script_var_to_addr;
    int field_10;
    struct {
        _std::map<mString, int> *var_to_offset;
        _std::map<mString, script_library_class *> *var_to_slc;
    } *debug_info;

public:
    int flags;

    script_var_container();

    void * operator new(size_t size);

    void operator delete(void *ptr, size_t size);

    bool is_game() const {
        return (this->flags & 2) != 0;
    }

    //0x0059B9B0
    void un_mash_start(generic_mash_header *a2, void *a3, generic_mash_data_ptrs *a4, void *a5);

    //0x00599440
    void un_mash(generic_mash_header *a2, void *a3, generic_mash_data_ptrs *a4);

    void destroy();

    //0x005A0520
    void *get_script_var_address(const char *a2, script_library_class **a3);

    int save_script_var_buffer(char *a2);

    int load_script_var_buffer(char *a2);

    char *get_address(int offset);

    static void read(chunk_file *file, script_var_container *c);
};
