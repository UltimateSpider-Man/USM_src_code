#pragma once

#include "mstring.h"
#include "variable.h"

#include <vector.hpp>

#include <set>

struct script_library_class;

struct slc_manager {
    //0x005AD720
    static void init();

    //0x005A5280
    static void add(script_library_class *slc);

    //0x005A5200
    static void kill();

    //0x0059EB10
    static script_library_class *get_class(int class_index);

    static script_library_class *get(const char *a1);

    //0x0059EC00
    static void un_mash_all_funcs();

    static bool using_xbox_v14();
};

#if !STANDALONE_SYSTEM
extern _std::vector<script_library_class *> *& slc_manager_class_array;
#else
#include <vector>

extern std::vector<script_library_class *> *& slc_manager_class_array;
#endif

inline std::set<script_library_class *> *slc_manager_classes = nullptr;

//0x005AB800
extern void register_standard_script_libs();

extern void slc_manager_patch();

// Restores the character-lineup name list for standalone PC pack indexes.
// This is deliberately narrower than slc_manager_patch().
extern void character_packname_list_patch();
