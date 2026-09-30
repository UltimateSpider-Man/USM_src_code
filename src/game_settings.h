#pragma once

#include "memoryunitmanager.h"

#include "float.hpp"
#include "game_data_essentials.h"
#include "game_data_meat.h"
#include "variable.h"

struct mString;
struct resource_key;

struct game_settings {

    static inline constexpr auto NUM_SOFT_SAVE_BUFFERS = 2;

    std::intptr_t m_vtbl;
    MemoryUnitManager::Container field_4;
    game_data_essentials field_28C[3] {};
    game_data_meat field_340;
    char *field_494[NUM_SOFT_SAVE_BUFFERS] {};
    char *field_49C[3] {};
    char field_4A8[12];
    int field_4B4;
    int field_4B8;
    bool m_game_data_valid[3] {};
    bool field_4BF;
    bool field_4C0;
    bool field_4C1;
    bool field_4C2;
    int m_slot_num;
    int field_4C8;

    //0x0057BF50
    game_settings();

    ~game_settings();

    //0x0057C0B0
    /* virtual */ void Callback(MemoryUnitManager::eOperation a2);

    //0x005799E0
    void init_script_buffer();

    //0x005737A0
    int get_game_size();

    //0x0057C1B0
    int soft_save(uint32_t soft_save_type);

    void update_miles_crawled_venom(Float a2);

    void update_miles_crawled_spidey(Float a2);

    //0x0057EAB0
    void start_new_game();

    //0x005802D0
    void frame_advance(Float a2);

    //0x00579A60
    void export_game_options();

    //0x00579A10
    void export_game_settings();

    //0x0057C1E0
    void soft_load(uint32_t soft_save_type);

    //0x0057D460
    void save(int slot_num);

    //0x00579B90
    void collect_game_settings();

    void sub_579990();

    //0x00579BF0
    void collect_game_options();

    char *get_buffer(int a2);

    //0x00579790
    void reset_container(bool a2);

    //0x00579970
    int load();

    //0x0057F410
    void load_game(int slot_num);

    void load_most_recent_game();

    //0x00573700
    void set_script_buffer_size();

    //0x005739E0
    void update_miles_web_zipping(Float a2);

    //0x00573A60
    void update_miles_run_venom(Float a2);

    //0x00573A00
    void update_miles_run_spidey(Float a2);

    //0x00573AA0
    void update_web_fluid_used(Float a2);

    //0x00579CE0
    bool set_str(const resource_key &a2, const mString &a3);

    //0x00579F40
    bool get_str(const resource_key &a2, mString &a3) const;

    //0x00573AE0
    bool set_num(const resource_key &att, Float a3);

    //0x00575930
    bool get_num(const resource_key &att, float &a3, bool a4) const;
};

extern void game_settings_patch();
extern void game_settings_continue_hero_patch();
extern void game_settings_request_continue_hero_override();
extern void game_settings_cancel_continue_hero_override();
