#pragma once

#include <cstdint>

#include "variable.h"

#ifdef TARGET_XBOX
#include <map>
#endif

namespace mash {

enum virtual_types_enum { };

} // namespace mash

struct mash_info_struct;

struct mash_virtual_base {
    std::intptr_t m_vtbl;

    mash_virtual_base();

    void destruct_mashed_class();

    void unmash(mash_info_struct *, void *);

    ~mash_virtual_base() = default;

    //0x0041F840
    uint32_t get_virtual_type_enum() const;

    bool is_subclass_of(mash::virtual_types_enum) const;

    //0x0041F850
    bool is_or_is_subclass_of(mash::virtual_types_enum a2) const;

    //0x00432B60
    static void generate_vtable();

    //0x0042AB60
    static void *create_subclass_by_enum(mash::virtual_types_enum a1);

    //0x004227E0
    static void *create_subclass_by_enum_in_place(mash::virtual_types_enum a1,
                                                  mash_virtual_base *a2,
                                                  int a3);

    //0x0042A7C0
    static void *construct_class_helper(void *a1);

    //0x0041F820
    static void fixup_vtable(void *a1);

    static inline Var<void *[1]> vtable {0x00957040};

#ifdef TARGET_XBOX
    static inline std::map<uint32_t, mash_virtual_base *> map_vtable {}; 
#endif
};


extern void mash_virtual_base_patch();

#if defined(OPENUSM_XBPACK_MODE) && !defined(TARGET_XBOX)
extern void mash_virtual_base_xbpack_patch();
#endif
