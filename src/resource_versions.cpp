#include "resource_versions.h"

#include "xbpack.h"

mString resource_versions::to_string() const {
    mString result = mString{mString::fmtd{0},
                             "%d.%d.%d.%d.%d",
                             this->field_0,
                             this->field_4,
                             this->field_8,
                             this->field_C,
                             this->field_10};
    return result;
}

resource_versions expected_resource_versions(_nlPlatformEnum platform)
{
    if (platform == NL_PLATFORM_XBOX) {
        if constexpr (xbpack::v10)
            return XBOX_V10_RESOURCE_VERSIONS;
        return XBOX_V14_RESOURCE_VERSIONS;
    }

    return PC_RETAIL_RESOURCE_VERSIONS;
}

bool supports_xbox_version(const resource_versions &versions)
{
    const auto expected = expected_resource_versions(NL_PLATFORM_XBOX);
    return versions.field_0 == expected.field_0 &&
           versions.field_4 == expected.field_4 &&
           versions.field_8 == expected.field_8 &&
           versions.field_C == expected.field_C &&
           versions.field_10 == expected.field_10;
}
