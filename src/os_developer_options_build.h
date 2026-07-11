#pragma once

#include "singleton.h"

#include "mstring.h"

#include <optional>

struct os_developer_options_build : singleton {

    enum strings_build_t {
        SOUND_LIST = 0,
        SCENE_NAME = 1,
        HERO_NAME = 2,
        GAME_TITLE = 3,
        GAME_LONG_TITLE = 4,
        SAVE_GAME_DESC = 5,
        VIDEO_MODE = 6,
        GFX_DEVICE = 7,
        FORCE_DEBUG_MISSION = 8,
        FORCE_LANGUAGE = 9,
        SKU = 10,
        CONSOLE_EXEC = 11,
        HERO_START_DISTRICT = 12,
        DEBUG_ENTITY_NAME = 13,

    };

	enum flags_build_t {};

	enum ints_build_t {};

    bool m_flags[150];
    mString m_strings[14];
    int m_ints[76];
    mString field_2AC;


    static os_developer_options_build *& instance;
};

using int_names_build_t = const char *[76];
extern int_names_build_t & int_names_build;

using flag_names_build_t = const char *[150];
extern flag_names_build_t & flag_names_build;

using string_names_build_t = const char *[14];
extern string_names_build_t & string_names_build;

using flag_defaults_build_t = BOOL[150];
inline flag_defaults_build_t & flag_build_defaults = var<flag_defaults_build_t>(0x00936678);

