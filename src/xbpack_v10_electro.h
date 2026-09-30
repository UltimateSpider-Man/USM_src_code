#pragma once

#include "ai_param_types.h"

#include <cstdint>

namespace xbpack::v10_electro
{
inline constexpr std::uintptr_t spiderman_hash_call = 0x00707FB6u;
inline constexpr std::uintptr_t native_parameter_hash = 0x006CDCE0u;
inline constexpr std::uint32_t spiderman_parameter = 0xD15189DFu;

// The beta stores spiderman_entity as a writable PT_FIXED_STRING. PC
// Electro activation expects its hash instead. Read the current string at
// activation without changing its type/storage: the mission writes it again
// through set_ai_param_str. Other types keep the native getter's behavior.
template <typename Parameter, typename HashName>
inline bool try_read_fixed_string_target(const Parameter *parameter,
                                         std::uint32_t &result, HashName hash_name)
{
    if (parameter == nullptr || parameter->my_type != ai::PT_FIXED_STRING) {
        return false;
    }
    const char *name = parameter->m_union.str;
    result = name != nullptr ? hash_name(name) : 0u;
    return true;
}
}

bool xbpack_v10_electro_patch();
