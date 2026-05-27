#include "mission_manager.h"

#include "common.h"
#include "event.h"
#include "event_manager.h"
#include "func_wrapper.h"
#include "game.h"
#include "log.h"
#include "mission_manager_script_data.h"
#include "mission_table_container.h"
#include "mstring.h"
#include "oldmath_po.h"
#include "parse_generic_mash.h"
#include "region.h"
#include "resource_manager.h"
#include "script_manager.h"
#include "sound_manager.h"
#include "script_executable.h"
#include "script_executable_entry.h"
#include "trace.h"
#include "trigger_manager.h"
#include "variables.h"
#include "wds.h"

#include <cassert>

VALIDATE_SIZE(mission_manager, 0x100u);

mission_manager *& mission_manager::s_inst = var<mission_manager *>(0x00968518);

mString & mission_manager::current_mission_debug_title = var<mString>(0x00969E90);

mission_manager::mission_manager()
{
    if constexpr (0)
    {}
    else
    {
        THISCALL(0x005DA010, this);
    }
}

bool  mission_manager::sub_5C5920()
{
    if constexpr (1)
    {}
    else
    {
      return (bool)  THISCALL(0x005C5920, this);
    }
}

bool mission_manager::sub_5C58D0(){

        THISCALL(0x005C58D0, this);
    
}

    // Helper to compare current script name
    bool mission_manager::isCurrentScript(const char* scriptName) const
    {
        if (!this->m_script)
            return false;
        return strncmp(this->m_script->field_0.guts, scriptName, 65535u) == 0;
    }
    // sub_5C58D0 - Check if mission can be restarted
    bool mission_manager::is_restartable_mission() const
    {
        return !isCurrentScript("") 
            && !isCurrentScript("s02_workout");
    }

    // sub_5C5940 - Check if current story mission can be quit
    bool mission_manager::is_quittable_story_mission() const
    {
        return this->m_script
            && mission_manager::s_inst->is_story_mission_active()
            && !isCurrentScript("lm_storm_races");
    }

int mission_manager::is_story_mission_active()
{
    TRACE("mission_manager::is_story_mission_active");

    if constexpr (1)
    {
        if (!this->m_script) {
            return -1;
        }

        mString var_name;
		mString{"g_mission_type"};


        auto *address = script_manager::get_game_var_address(var_name, 0, 0);
        int result = static_cast<int>(*reinterpret_cast<float *>(address));

        var_name.finalize(0);

        return result;
    }
    else
    {
        return THISCALL(0x005BAFF0, this);
    }
}



void mission_manager::prepare_unload_script()
{
    if ( this->m_script != nullptr )
    {
        if ( !this->m_unload_script )
        {
            auto *v1 = this->m_script->field_0.c_str();
            sp_log("Preparing to unload script '%s'", v1);
            this->m_unload_script = true;
            this->field_80 = false;
        }
    }
}

void mission_manager::force_mission(int a2, const char *a3, int a4, const char *a5)
{
    if constexpr (1)
    {
        this->prepare_unload_script();
        this->field_80 = true;
        this->field_84 = a2;

        fixedstring<8> v5 {a3};
        this->field_88 = v5;
        this->field_C8 = a4;
        if ( a5 != nullptr )
        {
            fixedstring<8> v6{a5};
            this->field_A8 = v6;
        }
        else
        {
            fixedstring<8> v7 {mString::null()};
            this->field_A8 = v7;
        } 
    }
    else
    {
        THISCALL(0x005C5A00, this, a2, a3, a4, a5);
    }
}

void mission_manager::set_real_time() {

    {
        mString a1 = mString{"real_world_timer"};

        this->field_60 = CAST(this->field_60,
                              script_manager::get_game_var_address(a1, nullptr, nullptr));
    }

    float *v2 = CAST(v2, this->field_60);
    this->field_64 = 0.0;
    this->field_5C = static_cast<int>(*v2);
}

void mission_manager::show_mission_loading_panel(const mString &a1)
{
    TRACE("mission_manager::show_mission_loading_panel", a1.c_str());

    THISCALL(0x005DA4B0, this, &a1);
}

int mission_manager::run_script(const mission_manager_script_data &arg0) {
    TRACE("mission_manager::run_script");

    if constexpr (0)
    {}
    else
    {
        return THISCALL(0x005DEFA0, this, &arg0);
    }
}

void mission_manager::unlock() {
    this->field_CC = false;
}

void mission_manager::lock() {
    this->field_CC = true;
}

void mission_manager::unload_script_now()
{
    if (this->m_script == nullptr) {
        return;
    }

    auto *mission_partition = resource_manager::get_partition_pointer(RESOURCE_PARTITION_MISSION);
    if (this->m_script == nullptr) {
        goto LABEL_5;
    }

    if (!this->m_unload_script) {
        this->m_unload_script = true;
        this->field_80 = false;
    LABEL_5:

        if (!this->m_unload_script) {
            return;
        }

        goto LABEL_6;
    }

    do {
    LABEL_6:
        this->unload_script_if_requested();
        this->hero_switch_frame = -1;
        if (mission_partition != nullptr) {
            mission_partition->get_streamer()->flush(game::render_empty_list);
        }

    } while (this->m_unload_script);
}

void mission_manager::unload_script_if_requested() {
    TRACE("mission_manager::unload_script_if_requested");

    THISCALL(0x005DBD00, this);
}

void mission_manager::load_script(const mission_manager_script_data &data)
{
    TRACE("mission_manager::load_script");

    if constexpr (0)
    {
        assert(data.uses_script_stack);

        assert(m_script_to_load == nullptr);

        mString v10{"pk_"};

        mString a3 = v10 + data.field_0;

        sp_log("Loading script %s", data.field_0.c_str());

        string_hash v7{a3.c_str()};

        resource_key v8{v7, RESOURCE_KEY_TYPE_PACK};

        if (resource_manager::get_pack_file_stats(v8, nullptr, nullptr, nullptr)) {
            this->m_script_to_load = new mission_manager_script_data{};

            this->m_script_to_load->copy(data);
            if (data.field_B8 == mString{""}) {
                this->show_mission_loading_panel(data.field_B8);
            }

            this->field_4 = 0;
            mission_stack_manager::s_inst->push_mission_pack(data.field_0, a3, -1, false);
        } else {
            sp_log("Could not find pack file %s", a3.c_str());
            assert(0);
        }

    } else {
        THISCALL(0x005DEE40, this, &data);
    }
}

void mission_manager::render_fade() {
    THISCALL(0x005BAE20, this);
}

void mission_manager::sub_5BACA0(Float a2) {
    THISCALL(0x005BACA0, this, a2);
}

void mission_manager::frame_advance(Float a2)
{
    TRACE("mission_manager::frame_advance");
    if constexpr (0)
    {
        if ( (!g_game_ptr->flag.physics_enabled || g_game_ptr->flag.single_step)
            && g_game_ptr->level.load_completed
            && g_game_ptr->flag.level_is_loaded )
        {
            auto v4 = this->field_FC;
            if ( v4 == 4 || v4 == 3 )
            {
                this->sub_5BB220(a2);
            }

            auto v5 = a2 * this->field_F8 + this->field_F4;
            this->field_F4 = v5;
            if ( v5 < 0.0f)
            {
                auto v6 = this->field_FC == 2;
                this->field_F4 = 0.0;
                this->field_F8 = 0.0;
                if ( v6 )
                {
                    this->field_FC = 0;
                    g_game_ptr->field_166 = 0;
                }
            }

            if ( this->field_F4 > 1.f )
            {
                auto v6 = this->field_FC == 1;
                this->field_F4 = 1.0;
                this->field_F8 = 0.0;
                if ( v6 )
                {
                    this->field_FC = 3;
                }
            }

            if ( this->field_60 == nullptr )
            {
                mString a1 {"real_world_timer"};
                this->field_60 = (float *)script_manager::get_game_var_address(a1, nullptr, nullptr);
                *this->field_60 = 0.0;
            }

            auto v7 = a2 + this->field_64;
            this->field_64 = v7;
            if ( v7 >= 1.f)
            {
                auto v8 = this->field_5C + 1;
                this->field_5C = v8;
                auto v9 = (double)this->field_5C;
                if ( v8 < 0 )
                    v9 = v9 + flt_86F860();

                *this->field_60 = v9;
                this->field_64 = this->field_64 - 1.f;
            }

            if ( this->field_6C == nullptr )
            {
                mString a1{"game_clock_timer"};
                this->field_6C = (float *)script_manager::get_game_var_address(a1, nullptr, nullptr);
            }

            if ( this->field_7C == nullptr )
            {
                mString a1{"game_day_of_the_week"};
                this->field_7C = (float *)script_manager::get_game_var_address(a1, nullptr, nullptr);
            }

            if ( this->field_78 == nullptr )
            {
                mString v22{"game_days"};
                this->field_78 = (float *)script_manager::get_game_var_address(v22, nullptr, nullptr);
            }

            if ( !g_game_ptr->flag.game_paused || s_freeze_game_time() )
            {
                auto v10 = (double)this->field_74;
                if ( this->field_74 < 0 )
                    v10 = v10 + flt_86F860();

                auto v11 = v10 * a2 + this->field_70;
                this->field_70 = v11;
                if ( v11 >= 1.f )
                {
                    do
                    {
                        auto v12 = this->field_68 + 1;
                        this->field_68 = v12;
                        if ( !(v12 % 60) )
                        {
                            event_manager::raise_event(event::TIME_MINUTE_INC, entity_base_vhandle {0});
                            if ( !(this->field_68 / 60 % 60) )
                            {
                                event_manager::raise_event(event::TIME_HOUR_INC, entity_base_vhandle {0});
                            }
                        }

                        auto v13 = this->field_68;
                        if ( v13 > 86400 )
                        {
                            this->field_68 = v13 - 86400;
                            *this->field_78 += 1.f;
                            event_manager::raise_event(event::TIME_DAY_INC, entity_base_vhandle {0});
                            *this->field_7C += 1.f;
                            auto *v14 = this->field_7C;
                            if ( *v14 > (double)flt_87EBD4() )
                              *v14 = 0.0;
                        }

                        auto v15 = (double)this->field_68;
                        if ( this->field_68 < 0 )
                        {
                            v15 += flt_86F860();
                        }

                        *this->field_6C = v15;
                        this->field_70 = this->field_70 - 1.f;
                    }
                    while ( this->field_70 >= 1.f );
                }
            }

            this->kill_braindead_script();
            mission_manager_script_data script_data{};
            this->unload_script_if_requested();

            bool v16, v17;
            if ( this->m_script_to_load
                && (v16 = mission_stack_manager::s_inst->pack_loads_or_unloads_pending == 0,
                v17 = sound_manager::is_mission_sound_bank_ready(),
                v16)
                && v17
                && this->field_FC != 1 )
            {
                this->run_script(*this->m_script_to_load);
                auto *v18 = this->m_script_to_load;
                if ( v18 != nullptr )
                {
                    v18->~mission_manager_script_data();
                    delete v18;
                }

                this->m_script_to_load = nullptr;
            }
            else
            {                                           
                this->sort_district_priorities();

                if ( this->m_script_to_load == nullptr
                    && this->m_script == nullptr
                    && !this->field_CC
                    && !g_game_ptr->flag.game_paused
                    && this->get_script(&script_data) )
                {
                    if ( script_data.uses_script_stack )
                        this->load_script(script_data);
                    else
                        this->run_script(script_data);
                }
            }

            this->update_hero_switch();
        }
    }
    else
    {
        THISCALL(0x005E16B0, this, a2);
    }
}

void mission_manager::kill_braindead_script()
{
    TRACE("mission_manager::kill_braindead_script");

    if constexpr (1) {
        if ( this->m_script != nullptr && !this->m_unload_script ) {
            auto *exec_list = script_manager::get_exec_list();

            script_executable_entry_key a2 {};
            auto *v1 = this->m_script->field_0.c_str();
            string_hash v6 {v1};
            resource_key v9 {v6, RESOURCE_KEY_TYPE_SCRIPT};
            a2.field_0 = v9;
            a2.field_8 = resource_key {};
            auto it = exec_list->find(a2);
            auto end = exec_list->end();
            if ( it != end ) {
                auto &v4 = (*it);
                if ( !v4.second.exec->has_threads() ) {
                    if ( !event_manager::does_script_have_callbacks(v4.second.exec) ) {
                        assert(false && "mission script appears to be dead");
                    }
                }
            }
        }
    } else {
        THISCALL(0x005D7EF0, this);
    }
}

void mission_manager::sort_district_priorities()
{
    THISCALL(0x005DBCD0, this);
}

void mission_manager::sub_5BB220(Float a2)
{
    THISCALL(0x005BB220, this, a2);
}

bool mission_manager::get_script(mission_manager_script_data *return_script_data)
{
    return (bool) THISCALL(0x005E13D0, this, return_script_data);
}

int mission_manager::add_global_table(const resource_key &a2) {
    return THISCALL(0x005D1EA0, this, &a2);
}

void mission_manager::add_district_table(void *a2, region *a3)
{
    if constexpr (1)
    {
        parse_generic_object_mash<mission_table_container> (
            this->m_district_table_containers[this->m_district_table_count],
            a2,
            nullptr,
            nullptr,
            nullptr,
            0u,
            0u,
            nullptr);

        this->m_district_table_containers[this->m_district_table_count++]->field_44 = a3;
    }
    else
    {
        THISCALL(0x005D1EE0, this, a2, a3);
    }
}


void mission_manager::update_hero_switch()
{

    if (int v3 = g_world_ptr->get_num_players(); v3 > 1) {
        do {
            g_world_ptr->remove_player(--v3);

        } while (v3 != 1);
    }

    if (auto v4 = this->hero_switch_frame; v4 != -1) {
        switch (v4) {
        case 0:
            assert(g_world_ptr->get_num_players() <= 1 && "update code for multiple players");

            if (g_world_ptr->get_num_players() <= 0) {
                ++this->hero_switch_frame;
            } else {
                sp_log("Removing player");
                g_world_ptr->remove_player(g_world_ptr->num_players - 1);
            }

            break;
        case 1:
            ++this->hero_switch_frame;
            break;
        case 2:
            ++this->hero_switch_frame;
            break;
        case 3: {
            auto old_num_players = g_world_ptr->get_num_players();

            sp_log("Adding player");

            mString a2 = mString{this->field_D0.to_string()};
            auto new_num_players = g_world_ptr->add_player(a2);

            assert(new_num_players > old_num_players &&
                   "unable to add player (while switching hero costumes)");

            this->hero_switch_frame = -1;
        } break;
        default:
            return;
        }
    }
}


bool mission_manager::sub_5C24C0()
{
  return THISCALL(0x005C24C0, this);
}



entity_base *mission_manager::get_mission_key_entity() const
{
    assert(m_script != nullptr);

    string_hash a1 {this->m_script->field_84.c_str()};
    return entity_handle_manager::find_entity(a1, IGNORE_FLAVOR, false);
}

trigger * mission_manager::get_mission_key_trigger() const
{
    mString v3 {this->m_script->field_84.c_str()};
    auto *instance = trigger_manager::instance->find_instance(v3);
    return instance;
}

_std::vector<float> * mission_manager::get_mission_nums()
{
    assert(m_script != nullptr);

    assert(m_script->nums.size() > 0);

    return &this->m_script->nums;
}

_std::vector<mString> * mission_manager::get_mission_strings()
{
    assert(m_script != nullptr);

    assert(m_script->strings.size() > 0);

    return &this->m_script->strings;
}

void mission_manager::set_mission_key_po(const po &a2)
{
    assert(m_script != nullptr);

    *this->m_script->field_94 = a2;
}

po mission_manager::get_mission_key_po() const
{
    assert(m_script != nullptr);

    return *this->m_script->field_94;
}

bool mission_manager::is_story_active() const
{
    mString v3 {"gv_story_finished"};
    float *game_var_address = (float *)script_manager::get_game_var_address(v3, nullptr, nullptr);
    return *game_var_address == 0.0f;
}

bool mission_manager::is_mission_active() const
{
    return this->m_script != nullptr;
}

void mission_manager::get_missions_nums_by_index(
        int a2,
        const char *a3,
        int a4,
        _std::vector<float> *nums_result)
{
    assert(nums_result != nullptr);

    for ( int i = 0; i < this->m_district_table_count; ++i )
    {
        auto *reg = this->m_district_table_containers[i]->get_region();
        if ( a2 == reg->get_district_id()
            && this->m_district_table_containers[i]->append_nums(a3, a4, nums_result) )
        {
            return;
        }
    }

    assert(0 && "Attempted to find nums for nonexistent mission.");
}

int mission_manager::sub_5C5BD0() const
{
    int v1 = (this->field_68 / 60u) / 60 % 24;
    if ( v1 <= 7 ) {
        return 2;
    }

    if ( v1 >= 19 ) {
        return 2;
    }

    return 1;
}

void mission_manager_patch()
{
    {
        FUNC_ADDRESS(address, &mission_manager::load_script);
        REDIRECT(0x005E1B1F, address);
    }

    {
        FUNC_ADDRESS(address, &mission_manager::run_script);
        REDIRECT(0x005E1A96, address);
        REDIRECT(0x005E1B2B, address);
    }

    {
        FUNC_ADDRESS(address, &mission_manager::show_mission_loading_panel);
        REDIRECT(0x005DEF4C, address);
    }

    {
        FUNC_ADDRESS(address, &mission_manager::unload_script_if_requested);
        REDIRECT(0x005DBE99, address);
        REDIRECT(0x005E1A65, address);
    }

    {
        FUNC_ADDRESS(address, &mission_manager::frame_advance);
        REDIRECT(0x0055D75B, address);
    }

    {
        FUNC_ADDRESS(address, &mission_manager::kill_braindead_script);
        SET_JUMP(0x005D7EF0, address);
    }
}
