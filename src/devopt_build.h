#pragma once

#include <windows.h>

struct mString;

struct game_option_build_t
{
    const char *m_name;
    union {
        BOOL *p_bval;
        int *p_ival;
        float *p_fval;
        mString *p_strval;
    } m_value;
    enum {
        UNDEFINED,
        FLAG_OPTION = 1, 
        INT_OPTION = 2,
        FLOAT_OPTION = 3,
        STRING_OPTION = 4,
    } m_type;
};

extern game_option_build_t *get_option_build(int idx);

