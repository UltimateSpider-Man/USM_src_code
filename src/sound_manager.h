#pragma once

#include "float.hpp"

#include <cstdint>

struct sound_alias_database;

namespace sound_manager {
    //0x0050FFE0
    extern void set_sound_alias_database(sound_alias_database *a1);

    //0x0050FFF0
    extern sound_alias_database *get_sound_alias_database();

    //0x0054DB10
    extern void load_common_sound_bank(bool a1);

    //0x0050F850
    extern bool is_mission_sound_bank_ready();

    //0x00543500
    extern void create_inst();

    extern void delete_inst();

    extern void load_hero_sound_bank(const char *a1, bool a2);

    extern void unload_hero_sound_bank();

    extern void set_source_type_volume(unsigned int source_type, Float a2, Float a3);

    extern float get_source_type_volume(unsigned int source_type);

    //0x0050FA50
    extern int fade_sounds_by_type(uint32_t a1, Float a2, Float a3, bool a4);

    //0x00520520
    extern void unpause_all_sounds();

    //0x00551C20
    extern void frame_advance(Float a1);
}

extern void sub_54DC10(const char *a1, bool a2);

extern char *sub_50F010();

extern int sub_79A160();

extern void sound_manager_patch();
