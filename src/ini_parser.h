#pragma once

#include "variable.h"

struct os_developer_options;

namespace ini_parser {

    //0x005CA120
    extern void parse(const char *ini_filename, os_developer_options *a2);

    //0x005C3000
    extern int get_token(char **a1, int *a2, int *a3);

    //0x005B8B00
    extern signed int build_token(char *a1, char *a2);

    extern void new_line(char *lpBuffer);

    inline auto & filename = var<char[255]>(0x009682F8);

    extern int & scan_pos;
    extern char *& line;
    extern char & token;
    extern char & stored_token;
    extern char & stored_type;
    extern char & stored_num;

}


extern void ini_parser_patch();
