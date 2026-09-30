#pragma once
#include "mod.h"

#include <cstddef>
#include <type_traits>

extern std::multimap<uint32_t, Mod> Mods;

#ifdef CHECK_STRUCT_SIZES

#define VALIDATE_SIZE(struc, size) \
    static_assert(sizeof(struc) == size, "Invalid structure size of " #struc)
#else
#define VALIDATE_SIZE(struc, size)
#endif

#define VALIDATE_OFFSET(struc, member, offset)       \
    static_assert(offsetof(struc, member) == offset, \
                  "The offset of " #member " in " #struc " is not " #offset "...")

#define VALIDATE_ALIGNMENT(struc, align) \
    static_assert(std::alignment_of_v<struc> == align, "Invalid structure alignment of " #struc)
