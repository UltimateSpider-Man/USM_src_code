#include "resource_amalgapak_header.h"

#include "common.h"
#include "log.h"
#include "mstring.h"
#include "variables.h"

#include "utility.h"

#include "func_wrapper.h"

#include <cassert>
#include <cstdlib>

VALIDATE_SIZE(resource_amalgapak_header, 0x38);

resource_amalgapak_header::resource_amalgapak_header() {
    this->clear();
}

void resource_amalgapak_header::clear() {
    this->field_0 = {};

    this->field_14 = rand();

    this->field_18 = 0;
    this->field_1C = 0;
    this->location_table_size = 0;
    this->field_24 = 0;
    this->memory_map_table_size = 0;
    this->field_2C = 0;
    this->prerequisite_table_size = 0;
    this->field_34 = 0;
}

bool resource_amalgapak_header::verify([[maybe_unused]] const mString &a2) {
    static constexpr resource_versions v20 = {RESOURCE_PACK_VERSION,
                                              RESOURCE_ENTITY_MASH_VERSION,
                                              RESOURCE_NONENTITY_MASH_VERSION,
                                              RESOURCE_AUTO_MASH_VERSION,
                                              RESOURCE_RAW_MASH_VERSION};

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

        auto v2 = a2.c_str();
        sp_log("Error. The amalgapak file %s (v%s) is newer than this executable code (v%s).",
               v2,
               v19.c_str(),
               v18.c_str());
        //assert(0);

        return false;
    } else if (v28) {
        auto v18 = v20.to_string();
        auto v19 = this->field_0.to_string();

        auto v2 = a2.c_str();

        sp_log("Error. The amalgapak file %s (v%s) is older than this executable code (v%s).",
               v2,
               v19.c_str(),
               v18.c_str());
        //assert(0);

        return false;
    } else {
        return true;
    }

    return true;
}

bool resource_amalgapak_header_xbox::verify([[maybe_unused]] const mString &a2) {
    static constexpr resource_versions v20 = {RESOURCE_PACK_VERSION_XBOX_BUILD,
                                              RESOURCE_ENTITY_MASH_VERSION_XBOX_BUILD,
                                              RESOURCE_NONENTITY_MASH_VERSION_XBOX_BUILD,
                                              RESOURCE_AUTO_MASH_VERSION_XBOX_BUILD,
                                              RESOURCE_RAW_MASH_VERSION_XBOX_BUILD};

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

        auto v2 = a2.c_str();
        sp_log("Error. The amalgapak file %s (v%s) is newer than this executable code (v%s).",
               v2,
               v19.c_str(),
               v18.c_str());
        //assert(0);

        return false;
    } else if (v28) {
        auto v18 = v20.to_string();
        auto v19 = this->field_0.to_string();

        auto v2 = a2.c_str();

        sp_log("Error. The amalgapak file %s (v%s) is older than this executable code (v%s).",
               v2,
               v19.c_str(),
               v18.c_str());
        //assert(0);

        return false;
    } else {
        return true;
    }

    return true;
}







void resource_amalgapak_header_patch() {

	
	{
       FUNC_ADDRESS(address, &resource_amalgapak_header::verify);
      // REDIRECT(0x00537762, address);
	  // REDIRECT(0x0053DF4A, address);
    }
	
    {
       FUNC_ADDRESS(address, &resource_amalgapak_header_xbox::verify);
       REDIRECT(0x00537762, address);
	   REDIRECT(0x0053DF4A, address);
    }
	
}
