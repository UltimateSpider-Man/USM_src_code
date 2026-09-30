#include "resource_pack_header.h"

#include "common.h"
#include "func_wrapper.h"
#include "log.h"
#include "utility.h"
#include "variables.h"

#include <cassert>
#include <cstdio>
#include <windows.h>

VALIDATE_SIZE(resource_pack_header, 0x2Cu);

namespace
{
    void debug_version_mismatch(const char *relation,
                                const resource_key &key,
                                const resource_versions &actual,
                                const resource_versions &expected)
    {
        char buffer[512];
        std::snprintf(buffer,
                      sizeof(buffer),
                      "resource_pack_header::verify: pack hash=0x%08X type=%d actual=%u.%u.%u.%u.%u is %s "
                      "than expected=%u.%u.%u.%u.%u platform=%d\n",
                      key.m_hash.source_hash_code,
                      static_cast<int>(key.m_type),
                      actual.field_0,
                      actual.field_4,
                      actual.field_8,
                      actual.field_C,
                      actual.field_10,
                      relation,
                      expected.field_0,
                      expected.field_4,
                      expected.field_8,
                      expected.field_C,
                      expected.field_10,
                      static_cast<int>(g_platform));
        OutputDebugStringA(buffer);
        sp_log("%s", buffer);
    }

    bool should_assert_resource_version_mismatch()
    {
        return g_platform != NL_PLATFORM_XBOX;
    }
}

resource_pack_header::resource_pack_header() : field_0() {
    this->field_14 = 0;
    this->directory_offset = 0;
    this->res_dir_mash_size = 0;
    this->field_20 = 0;
    this->field_24 = 0;
    this->field_28 = 0;
}

void resource_pack_header::clear() {
    this->field_0 = {};

    this->field_14 = 0;
    this->directory_offset = 0;
    this->res_dir_mash_size = 0;
    this->field_20 = 0;
    this->field_24 = 0;
    this->field_28 = 0;
}

bool resource_pack_header::verify(resource_key a2) const
{
    if constexpr (1)
    {
        //sp_log("0x%08X %d", a2.field_0, a2.m_type);

        if (g_platform == NL_PLATFORM_XBOX &&
            supports_xbox_version(this->field_0)) {
            return true;
        }

        const resource_versions v20 = expected_resource_versions(g_platform);

        bool v29 = false, v28 = false;

        if (this->field_0.field_0 < v20.field_0 || this->field_0.field_4 < v20.field_4 ||
            this->field_0.field_8 < v20.field_8 || this->field_0.field_C < v20.field_C ||
            this->field_0.field_10 < v20.field_10) {
            v28 = true;
        } else if (this->field_0.field_0 > v20.field_0 || this->field_0.field_4 > v20.field_4 ||
                   this->field_0.field_8 > v20.field_8 || this->field_0.field_C > v20.field_C ||
                   this->field_0.field_10 > v20.field_10) {
            v29 = true;
        }

        if (v29) {
            auto v18 = v20.to_string();
            auto v19 = this->field_0.to_string();

            auto v8 = a2.m_hash.sub_501E80();
            auto v2 = v8.to_string();
            sp_log("Error. The pack file %s (v%s) is newer than this executable code (v%s).",
                   v2,
                   v19.c_str(),
                   v18.c_str());
            debug_version_mismatch("newer", a2, this->field_0, v20);
            if (should_assert_resource_version_mismatch()) {
                assert(0);
            }

            return false;
        } else if (v28) {
            auto v18 = v20.to_string();
            auto v19 = this->field_0.to_string();

            auto v10 = a2.m_hash.sub_501E80();
            auto v2 = v10.to_string();

            sp_log("Error. The pack file %s (v%s) is older than this executable code (v%s).",
                   v2,
                   v19.c_str(),
                   v18.c_str());

            debug_version_mismatch("older", a2, this->field_0, v20);
            if (should_assert_resource_version_mismatch()) {
                assert(0);
            }

            return false;
        } else {
            if (this->field_14 & 1) {
                auto v14 = a2.m_hash.sub_501E80();
                auto v5 = v14.to_string();

                sp_log(
                    "Error. The pack file %s has sync testing ON, but this executable has sync "
                    "testing OFF.",
                    v5);
            }
        }

#ifdef TARGET_XBOX
        assert(0 && "Load amalgapak from xbox");
#endif

        return true;
    } else {
        return (bool) THISCALL(0x00537DA0, this, a2);
    }
}

void resource_pack_header_patch() {
    {
        FUNC_ADDRESS(address, &resource_pack_header::verify);
        REDIRECT(0x0053E1D2, address);
    }
}
