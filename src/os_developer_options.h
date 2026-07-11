#pragma once

#include "singleton.h"

#include "mstring.h"

#include <optional>

struct os_developer_options : singleton {

    enum strings_t {
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

	enum flags_t {};

	// Retail ints option table @0x00936940 (76 entries), names extracted
	// from USM.exe and index-verified (get_int(ints_t) = 0x005B8830;
	// e.g. retail advance_state_load_level does get_int(0x42) = TIME_OF_DAY).
	enum ints_t {
        DIFFICULTY = 0x00,
        CAMERA_STYLE = 0x01,
        CAMERA_STATE = 0x02,
        CAMERA_FOV = 0x03,
        FOG_RED = 0x04,
        FOG_GREEN = 0x05,
        FOG_BLUE = 0x06,
        FOG_DISTANCE = 0x07,
        BIT_DEPTH = 0x08,
        MONKEY_MODE = 0x09,
        RANDOM_SEED = 0x0A,
        FORCE_WIN = 0x0B,
        CONTROLLER_TYPE = 0x0C,
        FRAME_LOCK = 0x0D,
        FRAME_LIMIT = 0x0E,
        SWING_DEBUG_TRAILS = 0x0F,
        SOAK_SMOKE = 0x10,
        FAR_CLIP_PLANE = 0x11,
        POI_DISPLAY_TYPE = 0x12,
        STORY_MISSION = 0x13,
        EXEC_DELAY = 0x14,
        RUN_LENGTH = 0x15,
        PC_WINDOW_TOP = 0x16,
        PC_WINDOW_LEFT = 0x17,
        PC_WINDOW_WIDTH = 0x18,
        PC_WINDOW_HEIGHT = 0x19,
        ALLOW_SCREENSHOT = 0x1A,
        AMALGA_REFRESH_INTERVAL = 0x1B,
        ENABLE_LONG_MALOR_ASSERTS = 0x1C,
        GOD_MODE = 0x1D,
        PCLISTBUFFER = 0x1E,
        PCSCRATCHBUFFER = 0x1F,
        PCSCRATCHINDEXBUFFER = 0x20,
        PCSCRATCHVERTEXBUFFER = 0x21,
        NAL_HEAP_SIZE = 0x22,
        ASSERT_BOX_MARGIN = 0x23,
        ASSERT_TEXT_MARGIN = 0x24,
        ASSERT_FONT_PCT_X = 0x25,
        ASSERT_FONT_PCT_Y = 0x26,
        STREAMER_INFO_FONT_PCT = 0x27,
        DEBUG_INFO_FONT_PCT = 0x28,
        PITCH_FACTOR = 0x29,
        BANK_FACTOR = 0x2A,
        SWING_INTERPOLATION_TIME = 0x2B,
        BOTH_HANDS_INTERPOLATION_TIME = 0x2C,
        MEM_DUMP_FRAME = 0x2D,
        HERO_START_X = 0x2E,
        HERO_START_Y = 0x2F,
        HERO_START_Z = 0x30,
        SHOW_SOUND_INFO = 0x31,
        SHOW_VOICE_BOX_INFO = 0x32,
        DEBUG_CAMERA_PITCH_MULTIPLIER = 0x33,
        DEBUG_CAMERA_YAW_MULTIPLIER = 0x34,
        DEBUG_CAMERA_MOVE_MULTIPLIER = 0x35,
        DEBUG_CAMERA_STRAFE_MULTIPLIER = 0x36,
        TAM_SCALE_MIN_DISTANCE = 0x37,
        TAM_SCALE_MAX_DISTANCE = 0x38,
        TAM_SCALE_MIN_PERCENT = 0x39,
        THUG_HEALTH_UI_SCALE_MIN_DISTANCE = 0x3A,
        THUG_HEALTH_UI_SCALE_MAX_DISTANCE = 0x3B,
        THUG_HEALTH_UI_SCALE_MIN_PERCENT = 0x3C,
        TARGETING_RETICLE_SCALE_MIN_DISTANCE = 0x3D,
        TARGETING_RETICLE_SCALE_MAX_DISTANCE = 0x3E,
        TARGETING_RETICLE_SCALE_MIN_PERCENT = 0x3F,
        HIRES_SCREENSHOT_X = 0x40,
        HIRES_SCREENSHOT_Y = 0x41,
        TIME_OF_DAY = 0x42,
        MINI_MAP_ZOOM = 0x43,
        RTDT_REPLAY_BUFFER_SIZE = 0x44,
        TIMER_WIDGET_TIME_DELTA_PERCENT = 0x45,
        DEBUG_PARTICLE_LEVEL = 0x46,
        DEBUG_PARTICLE_MEMORY = 0x47,
        MAX_AEPS_ENTITIES = 0x48,
        MAX_AEPS_SPAWNERS = 0x49,
        MAX_AEPS_EMITTERS = 0x4A,
        MAX_AEPS_PARTICLES = 0x4B,
	};

    bool m_flags[150];
    mString m_strings[14];
    int m_ints[76];
    mString field_2AC;

    //0x005B8700
    os_developer_options();

    //0x005E2CB0
    //virtual
    ~os_developer_options();
	
	void set_hero_name(const mString& hero_name);

	void toggle_flag(flags_t a2);
	
	
	bool is_hero_selected(const char* name) const;

	
	bool get_hero_flag() const;



    //0x005B87E0
    char get_flag(flags_t a2) const;

    //0x005C2F20
    char get_flag(const mString &a2) const;

    //0x005B88A0
    int get_flag_from_name(const mString &a1) const;

    //0x005B8830
    int get_int(ints_t a2) const;

    //0x005C2F60
    int get_int(const mString &a2) const;

    //0x005B8950
    int get_int_from_name(const mString &a1) const;

    //0x005B8810
    void set_int(int idx, int a3);

    //0x005C2F40
    void set_int(const mString &a2, int a3);

    //0x005B87D0
    void set_flag(int a2, bool a3);

    //0x005C3150
    mString *get_hero_name() const;

    //0x005C2F00
    void set_flag(const mString &a2, bool a3);

    //0x005B8860
    std::optional<mString> get_string(strings_t a2) const;

    //0x005C2FB0
    std::optional<mString> get_string(const mString &a1) const;

    //0x005B8A00
    strings_t get_string_from_name(const mString &a1) const;

    //0x005C2F80
    void set_string(const mString &a2, const mString &a3);

    //0x005B8840
    void set_string(strings_t a2, const mString &a3);

    //0x005B23E0
    static void os_developer_init();

    static os_developer_options *& instance;
};

using int_names_t = const char *[76];
extern int_names_t & int_names;

using flag_names_t = const char *[150];
extern flag_names_t & flag_names;

using string_names_t = const char *[14];
extern string_names_t & string_names;

using flag_defaults_t = BOOL[150];
inline flag_defaults_t & flag_defaults = var<flag_defaults_t>(0x00936678);

extern void os_developer_options_patch();
