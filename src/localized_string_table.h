#pragma once

#include "global_text_enum.h"

#include "variable.h"

struct localized_string_table {
    struct internal {
        const char *field_0[479];
        const char *field_778[1];
    };

    internal *field_0;
    int field_4;
    int field_8;
    int scripttext_number;

    void sub_60BD30();

    //0x0060BDD0
    const char *lookup_scripttext_string(int num);

    //0x0060BDC0
    const char *lookup_localized_string(global_text_enum num);

    //0x0062EF10
    static void load_localizer();
	
	const char *find(const char *key);


const char *lookup(localized_string_table *inst, const char *key);

    static inline localized_string_table *&instance =
        var<localized_string_table *>(0x0096190C);

};

extern void localized_string_table_patch();


    
	

