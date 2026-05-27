#pragma once

#include "config.h"
#include "resource_versions.h"
#include "variable.h"

struct mString;

struct resource_amalgapak_header {
    resource_versions field_0;
    int field_14;
    int field_18;
    int field_1C;
    int location_table_size;
    int field_24;
    int memory_map_table_size;
    int field_2C;
    int prerequisite_table_size;
    int field_34;

    resource_amalgapak_header();

    //0x0050E650
    void clear();

    //0x0050E6B0
    bool verify(const mString &a2);

};

extern void resource_amalgapak_header_patch();

struct resource_amalgapak_header_xbox {
    resource_versions field_0;
    int field_14;
    int field_18;
    int field_1C;
    int location_table_size;
    int field_24;
    int memory_map_table_size;
    int field_2C;
    int prerequisite_table_size;
    int field_34;

    //0x0050E6B0
    bool verify(const mString &a2);

};


