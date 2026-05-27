#include "sound_manager.h"

#include "common.h"
#include "func_wrapper.h"
#include "sound_alias_database.h"
#include "sound_bank_slot.h"
#include "trace.h"
#include "utility.h"
#include "variable.h"
#include "variables.h"

static constexpr int SM_MAX_SOURCE_TYPES = 8;

static Var<bool> s_sound_manager_initialized{0x0095C829};

struct sound_volume {
    float field_0;
    int field_4[7];
};

VALIDATE_SIZE(sound_volume, 0x20);

static Var<sound_volume[8]> s_volumes_by_type{0x0095C9A8};

sound_alias_database *sound_manager::get_sound_alias_database()
{
    return s_sound_alias_database();
}

void sound_manager::set_sound_alias_database(sound_alias_database *a1)
{
    s_sound_alias_database() = a1;
}

bool sound_manager::is_mission_sound_bank_ready() {
    return s_sound_bank_slots()[11].m_state != 1;
}

void sound_manager::load_common_sound_bank(bool a1)
{
    if constexpr (0) {
    } else {
        CDECL_CALL(0x0054DB10, a1);
    }
}

void sound_manager::create_inst()
{
    if constexpr (0) {
    } else {
        CDECL_CALL(0x00543500);
    }
}

void sound_manager::delete_inst()
{
    TRACE("sound_manager::delete_inst");

    if constexpr (0) {
    } else {
        CDECL_CALL(0x00543EF0);
    }
}

void sound_manager::frame_advance(Float a1)
{
    TRACE("sound_manager::frame_advance");

    if constexpr (0) {
    } else {
        CDECL_CALL(0x00551C20, a1);
    }
}

void sound_manager::load_hero_sound_bank(const char *a1, bool a2)
{
    assert(s_sound_bank_slots()[SB_TYPE_LEVEL_COMMON].get_state() == SB_STATE_LOADED);

    char *v11 = sub_50F010();

    s_sound_bank_slots()[SB_TYPE_HERO].load(v11, a1, a2, 0);
}

void sound_manager::unload_hero_sound_bank()
{
    assert(s_sound_bank_slots()[SB_TYPE_LEVEL_COMMON].get_state() == SB_STATE_LOADED);

    s_sound_bank_slots()[SB_TYPE_HERO].unload();
}

float sound_manager::get_source_type_volume(unsigned int source_type) {
    assert(s_sound_manager_initialized());
    assert(source_type < SM_MAX_SOURCE_TYPES);

    return s_volumes_by_type()[source_type].field_0;
}

void sound_manager::set_source_type_volume(unsigned int source_type, Float a2, Float a3)
{
    if constexpr (0) {
    } else {
        CDECL_CALL(0x0050FC50, source_type, a2, a3);
    }
}

void sound_manager::unpause_all_sounds()
{
    if constexpr (0) {
    } else {
        CDECL_CALL(0x00520520);
    }
}

int sound_manager::fade_sounds_by_type(uint32_t a1, Float a2, Float a3, bool a4)
{
    if constexpr (0) {
    } else {
        int (__cdecl *func)(uint32_t a1, Float a2, Float a3, bool a4) = CAST(func, 0x0050FA50);
        return func(a1, a2, a3, a4);
    }
}

char * sub_50F010()
{
    if constexpr (1)
    {
        int curr_char = strlen(g_scene_name) - 1;
        if (curr_char > 0) {
            while (g_scene_name[curr_char] != '\\') {
                if (--curr_char <= 0) {
                    goto LABEL_4;
                }
            }
            return &g_scene_name[curr_char + 1];
        }
    LABEL_4:
        if (g_scene_name[curr_char] == '\\') {
            return &g_scene_name[curr_char + 1];
        }

        return &g_scene_name[curr_char];
    } else {
        char * (__cdecl *func)() = CAST(func, 0x0050F010);
        return func();
    }
}

void sub_54DC10(const char *a1, bool a2)
{
    assert(s_sound_bank_slots()[SB_TYPE_LEVEL_COMMON].get_state() == SB_STATE_LOADED);

    assert(s_sound_bank_slots()[SB_TYPE_MOVIE].get_state() == SB_STATE_EMPTY);

    auto *v2 = sub_50F010();
    s_sound_bank_slots()[SB_TYPE_MISSION].load(v2, a1, a2, 0);
}

int sub_79A160()
{
    if constexpr (0) {
    } else {
        return CDECL_CALL(0x0079A160);
    }
}

void sound_manager_patch()
{
    {
        REDIRECT(0x0055D6F4, sound_manager::frame_advance);
        REDIRECT(0x00559F87, sound_manager::frame_advance);
    }
}
