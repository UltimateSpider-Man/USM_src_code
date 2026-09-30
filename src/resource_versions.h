#pragma once

#include "mstring.h"
#include "variables.h"

#include <cstdint>

struct resource_versions {
    uint32_t field_0 = RESOURCE_VERSION_INVALID;
    uint32_t field_4 = RESOURCE_VERSION_INVALID;
    uint32_t field_8 = RESOURCE_VERSION_INVALID;
    uint32_t field_C = RESOURCE_VERSION_INVALID;
    uint32_t field_10 = RESOURCE_VERSION_INVALID;

    resource_versions() = default;

    constexpr resource_versions(uint32_t a0, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4)
        : field_0(a0), field_4(a1), field_8(a2), field_C(a3), field_10(a4) {}

    //0x0050E530
    [[nodiscard]] mString to_string() const;
};

inline constexpr resource_versions XBOX_V14_RESOURCE_VERSIONS {
    14, 590, 302, 588, 278
};

inline constexpr resource_versions XBOX_V10_RESOURCE_VERSIONS {
    10, 486, 265, 487, 246
};

inline constexpr resource_versions PC_RETAIL_RESOURCE_VERSIONS {
    17, 592, 304, 590, 280
};

resource_versions expected_resource_versions(_nlPlatformEnum platform);
bool supports_xbox_version(const resource_versions &versions);
