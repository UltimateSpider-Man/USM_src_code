#pragma once

#include "mash.h"
#include "mstring.h"
#include "string_hash.h"

#include <cstddef>

struct mash_info_struct;

struct string_hash_entry {
    string_hash field_0;
    mString field_4;

    string_hash_entry();

    string_hash_entry(const char *a2, const string_hash *a3);

#ifdef OPENUSM_XBPACK_V10
    // Stock mAvlTree::destroy_element frees dictionary keys through the
    // executable's MSVCR71 operator delete.  V10 must allocate them through
    // the matching executable operator new, not the injected MinGW CRT.
    void *operator new(std::size_t size);
    void operator delete(void *ptr, std::size_t size) noexcept;
#endif

    void initialize(mash::allocation_scope, const char *a2, const string_hash *a3);

    mString generate_text(const char *a3) const;

    void unmash(mash_info_struct *a1, void *a2);

    void custom_unmash(mash_info_struct *a2, void *a3);
};
