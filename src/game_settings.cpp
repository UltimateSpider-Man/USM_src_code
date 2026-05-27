#include "game_settings.h"

#include "common.h"
#include "damage_interface.h"
#include "func_wrapper.h"
#include "game.h"
#include "memory.h"
#include "mission_manager.h"
#include "mstring.h"
#include "os_developer_options.h"
#include "resource_key.h"
#include "rumble_manager.h"
#include "script_manager.h"
#include "settings.h"
#include "sound_manager.h"
#include "trace.h"
#include "utility.h"
#include "variables.h"
#include "wds.h"

#include <cassert>

VALIDATE_SIZE(game_settings, 0x4CCu);

#if USE_CXX_CONSTRUCTOR

void __stdcall vector_constructor(void *a1,
                                  uint32_t size,
                                  int count,
                                  void (__fastcall *constructor)(void *),
                                  [[maybe_unused]] fastcall_call destructor) {
    FUNC_ADDRESS(address, &game_data_essentials::initialize);

    constructor = bit_cast<decltype(constructor)>(address);

    for (int i{0}; i < count; ++i) {
        constructor(static_cast<int *>(a1));
        a1 = static_cast<char *>(a1) + size;
    }
}
#endif

game_settings::game_settings() : field_4{""}
{
    if constexpr (1)
    {
        this->m_vtbl = 0x0088B234;

        this->field_4BF = false;
        this->field_4C0 = false;
        this->field_4C1 = false;
        this->field_4C2 = false;
        this->field_4BF = false;

        MemoryUnitManager::Initialize(0);

        [[maybe_unused]] auto v4 = os_developer_options::instance->get_string(os_developer_options::strings_t::SKU);

        MemoryUnitManager::RegisterObserver(bit_cast<MemoryUnitManager::Observer *>(this));

        this->set_script_buffer_size();

        strncpy(this->field_4A8, "", 12u);
        this->field_4A8[11] = '\0';

        this->field_4B8 = 0;
        this->m_slot_num = 0;
        this->field_4C8 = 0;
    } else {
        THISCALL(0x0057BF50, this);
    }
}

void sub_5288B0(void *Memory)
{
    CDECL_CALL(0x005288B0, Memory);
}

game_settings::~game_settings()
{
    this->m_vtbl = 0x0088B234;
    
    for (int i = 0; i < 3; ++i)
    {
        mem_freealign(this->field_49C[i]);
    }

    for (int i = 0; i < 2; ++i)
    {
        sub_5288B0(this->field_494[i]);
    }
}

void game_settings::Callback(MemoryUnitManager::eOperation a2) {
    THISCALL(0x0057C0B0, this, a2);
}

void game_settings::init_script_buffer()
{
    TRACE("game_settings::init_script_buffer");

    if constexpr (1)
    {
        this->sub_579990();
        script_manager::save_game_var_buffer(this->field_494[0]);
        script_manager::save_game_var_buffer(this->field_494[1]);

    } else {
        THISCALL(0x005799E0, this);
    }
}

void game_settings::update_miles_crawled_venom(Float a2)
{
    this->field_340.field_C0 += a2 * 0.0006213712;
}

void game_settings::update_miles_crawled_spidey(Float a2)
{
    this->field_340.field_98 += a2 * 0.0006213712;
}

void game_settings::start_new_game() {
    THISCALL(0x0057EAB0, this);
}

void game_settings::frame_advance(Float a2)
{
    TRACE("game_settings::frame_advance");

    if constexpr (1)
    {
        if (this->field_4C2 && ++this->field_4C8 > 2) {
            this->load_game(this->m_slot_num);
        }

    }
    else
    {
        THISCALL(0x005802D0, this, a2);
    }
}

void game_settings::export_game_options()
{
    sound_manager::set_source_type_volume(0, Settings::GameSoundVolume(), 0.0);
    sound_manager::set_source_type_volume(1u, Settings::GameSoundVolume(), 0.0);
    sound_manager::set_source_type_volume(2u, Settings::MusicVolume(), 0.0);
    sound_manager::set_source_type_volume(3u, Settings::GameSoundVolume(), 0.0);
    sound_manager::set_source_type_volume(4u, Settings::GameSoundVolume(), 0.0);
    sound_manager::set_source_type_volume(5u, Settings::GameSoundVolume(), 0.0);
    sound_manager::set_source_type_volume(6u, Settings::GameSoundVolume(), 0.0);
    sound_manager::set_source_type_volume(7u, Settings::GameSoundVolume(), 0.0);
    auto *v2 = input_mgr::instance->rumble_ptr;
    if (this->field_340.field_31) {
        v2->enable_vibration();
    } else {
        v2->disable_vibration();
    }

    if (g_world_ptr->get_hero_ptr(0) != nullptr)
    {
        mString a1{"gv_hero_spawn_point"};

        auto *v3 = (const vector3d *) script_manager::get_game_var_address(a1, nullptr, nullptr);

        g_world_ptr->sub_530460(*v3, 0, 0);
    }
}

void game_settings::export_game_settings()
{
    if (g_world_ptr != nullptr)
    {
        auto *v1 = g_world_ptr->get_hero_ptr(0);
        if (v1 != nullptr)
        {
            if (v1->has_damage_ifc())
            {
                auto &v3 = v1->damage_ifc()->field_1FC.field_0[2];
                auto *v2 = v1->damage_ifc();
                v2->field_1FC.sub_48BFB0(v3);
            }
        }
    }
}

static constexpr auto NUM_SOFT_SAVE_BUFFERS = 2;

void game_settings::soft_load(uint32_t soft_save_type) {
    if constexpr (1) {
        assert(soft_save_type < NUM_SOFT_SAVE_BUFFERS);

        if (soft_save_type == 1) {
            std::memcpy(this->field_494[0], this->field_494[1], this->field_4B4);
        }

        g_game_ptr->field_15D = false;
        this->field_4C0 = false;
        this->export_game_settings();
        script_manager::load_game_var_buffer(this->field_494[soft_save_type]);
        mission_manager::s_inst->set_real_time();

    } else {
        THISCALL(0x0057C1E0, this, soft_save_type);
    }
}

void game_settings::update_miles_run_venom(Float a2) {
    this->field_340.field_BC += a2 * 0.0006213712f;
}

void game_settings::update_miles_web_zipping(Float a2) {
    this->field_340.m_miles_web_zipping += a2 * 0.0006213712f;
}

void game_settings::update_miles_run_spidey(Float a2) {
    this->field_340.field_94 += a2 * 0.0006213712f;
}

void game_settings::set_script_buffer_size()
{
    TRACE("game_settings::set_script_buffer_size");

    this->field_4B4 = script_manager::save_game_var_buffer(nullptr);
}

void game_settings::update_web_fluid_used(Float a2) {
    this->field_340.m_web_fluid_used += a2;
}

void game_settings::reset_container(bool a2)
{
    THISCALL(0x00579790, this, a2);
}

int game_settings::load()
{
    this->reset_container(true);
    return MemoryUnitManager::LoadGame(this->field_4);
}

void game_settings::load_game(int slot_num)
{

        THISCALL(0x0057F410, this, slot_num);
    
}



void game_settings::load_most_recent_game() 
{

   THISCALL(0x0057F580, this);
}

int *GetSystemDate(int *out) {
    return (int *) CDECL_CALL(0x00573510, out);
}

void game_settings::collect_game_settings()
{
    if constexpr (1)
    {
        if (g_world_ptr != nullptr)
        {
            auto *hero_ptr = g_world_ptr->get_hero_ptr(0);
            if (hero_ptr != nullptr)
            {
                if (hero_ptr->has_damage_ifc())
                {
                    this->field_340.m_hero_health = hero_ptr->damage_ifc()->field_1FC.field_0[0];
                    this->field_4BF = true;
                    return;
                }

                this->field_340.m_hero_health = 0.0;
            }
        }

        this->field_4BF = true;

    } else {
        THISCALL(0x00579B90, this);
    }
}

void game_settings::sub_579990()
{
    if constexpr (1)
    {
        if (this->field_494[0] == nullptr) {
            if (this->field_4B4 == 0) {
                this->set_script_buffer_size();
            }

            this->field_494[0] = static_cast<char *>(arch_malloc(this->field_4B4));
            this->field_494[1] = static_cast<char *>(arch_malloc(this->field_4B4));
        }

    } else {
        THISCALL(0x00579990, this);
    }
}

void game_settings::collect_game_options() {
    THISCALL(0x00579BF0, this);
}

char *game_settings::get_buffer(int a2) {
    return this->field_49C[a2];
}

int game_settings::soft_save(uint32_t soft_save_type)
{
    assert(soft_save_type < NUM_SOFT_SAVE_BUFFERS);

    this->collect_game_settings();
    this->sub_579990();
    return script_manager::save_game_var_buffer(this->field_494[soft_save_type]);
}

static constexpr auto MAX_GAME_SIZE = 16384;

int game_settings::get_game_size()
{
    if ( this->field_4B4 == 0 ) {
        this->field_4B4 = script_manager::save_game_var_buffer(nullptr);
    }

    auto size = 2 * this->field_4B4 + 400;
    assert(size <= MAX_GAME_SIZE && "Uh-oh!!  We've exceeded the maximum size for our save game!!!");

    if ( size < MAX_GAME_SIZE ) {
        return MAX_GAME_SIZE;
    }

    return size;
}

void game_settings::save(int slot_num)
{
    if constexpr (1) {
        static constexpr auto MAX_GAMEFILE_SLOTS = 3;

        assert(slot_num >= 0 && slot_num <= MAX_GAMEFILE_SLOTS);

        auto &v1 = this->field_28C[slot_num];

        strncpy(v1.field_14, "02:29:05", 25u);
        strncpy(v1.field_2E, this->field_4A8, 12u);

        auto a2a = *(float *) script_manager::get_game_var_address(mString{"real_world_timer"},
                                                                   nullptr,
                                                                   nullptr);

        v1.field_C = (int) a2a;

        int v7[3];
        auto *v5 = GetSystemDate(v7);

        *((int *) &v1.field_0) = v5[0];
        *((int *) &v1.field_4) = v5[1];
        *((int *) &v1.field_8) = v5[2];
        v1.field_10 = this->field_4B4;
        this->m_game_data_valid[slot_num] = true;
        this->soft_save(0);
        this->collect_game_options();
        auto size = this->get_game_size();

        memset(this->field_49C[slot_num], 0, MemoryUnitManager::GetGameSaveSize(size));
        std::memcpy(this->field_49C[slot_num], &v1, sizeof(game_data_essentials));
        std::memcpy(this->field_49C[slot_num] + sizeof(game_data_essentials),
                    &this->field_340,
                    sizeof(game_data_meat));
        std::memcpy(this->field_49C[slot_num] + sizeof(game_data_essentials) +
                        sizeof(game_data_essentials),
                    this->field_494[0],
                    this->field_4B4);
        std::memcpy(&this->field_49C[slot_num][this->field_4B4 + 400],
                    this->field_494[1],
                    this->field_4B4);

        if (!MemoryUnitManager::SaveGame(this->field_4)) {
            g_game_ptr->field_15D = false;
        }

        this->field_4B8 = slot_num;

    } else {
        THISCALL(0x0057D460, this, slot_num);
    }
}

bool game_settings::set_str(const resource_key &att, const mString &a3)
{
    assert(att.get_type() == RESOURCE_KEY_TYPE_IFC_ATTRIBUTE);

    static resource_key pstring_set[2] {
        resource_key {string_hash {int(to_hash("HERO_NAME"))}, RESOURCE_KEY_TYPE_IFC_ATTRIBUTE},
        resource_key {string_hash {int(to_hash("DISTRICT_NAME"))}, RESOURCE_KEY_TYPE_IFC_ATTRIBUTE},
    };

    if ( att == pstring_set[0] )
    {
        this->field_340.m_hero_name = a3.c_str();
        return true;
    }
    else if ( att == pstring_set[1] )
    {
        this->field_340.m_district_name = a3.c_str();
        return true;
    }
    else
    {
        assert(0 && "invalid game setting");

        return false;
    }
}

bool game_settings::get_str(const resource_key &att, mString &a3) const
{
    assert(att.get_type() == RESOURCE_KEY_TYPE_IFC_ATTRIBUTE);

    static resource_key pstring_get[2] {
        resource_key {string_hash {int(to_hash("HERO_NAME"))}, RESOURCE_KEY_TYPE_IFC_ATTRIBUTE},
        resource_key {string_hash {int(to_hash("DISTRICT_NAME"))}, RESOURCE_KEY_TYPE_IFC_ATTRIBUTE},
    };

    if ( att == pstring_get[0] )
    {
        a3 = this->field_340.m_hero_name.to_string();
        return true;
    }
    else if ( att == pstring_get[1] )
    {
        a3 = this->field_340.m_district_name.to_string();
        return true;
    }
    else
    {
        assert(0 && "invalid game setting");

        return false;
    }
}

bool game_settings::set_num(const resource_key &att, Float a3)
{
    TRACE("game_settings::set_num", att.m_hash.to_string());

    assert(att.get_type() == RESOURCE_KEY_TYPE_IFC_ATTRIBUTE);

    if constexpr (0)
    {}
    else
    {
        bool (__fastcall *func)(const void *, void *edx, const resource_key *, Float) = CAST(func, 0x00573AE0);
        return func(this, nullptr, &att, a3);
    }
}

bool game_settings::get_num(const resource_key &att, float &a3, bool a4) const
{
    TRACE("game_settings::get_num", att.m_hash.to_string());

    assert(att.get_type() == RESOURCE_KEY_TYPE_IFC_ATTRIBUTE);

    if constexpr (0)
    {}
    else
    {
        bool (__fastcall *func)(const void *, void *edx, const resource_key *att, float *a3, bool a4) = CAST(func, 0x00575930);
        bool result = func(this, nullptr, &att, &a3, a4);
        sp_log("%f", a3);

        if (!result) {
            assert(0 && "invalid game setting");
        }

        return result;
    }
}

void game_settings_patch()
{
    {
        FUNC_ADDRESS(address, &game_settings::frame_advance);
        REDIRECT(0x0055D770, address);
    }

    {
        FUNC_ADDRESS(address, &game_settings::set_num);
        REDIRECT(0x00663BED, address);
    }

    {
        FUNC_ADDRESS(address, &game_settings::get_num);
        REDIRECT(0x00663C6A, address);
    }

    if constexpr (0) {
        {
            FUNC_ADDRESS(address, &game_settings::load_game);
            //REDIRECT(0x005802F8, address);
        }
    }
}
