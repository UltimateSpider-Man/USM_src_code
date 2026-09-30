#include "mission_table_container.h"

#include "actor.h"
#include "ai_player_controller.h"
#include "common.h"
#include "entity_base_vhandle.h"
#include "mission_manager_script_data.h"
#include "oldmath_po.h"
#include "wds.h"

VALIDATE_SIZE(mission_condition_instance, 0x68u);
VALIDATE_SIZE(mission_condition, 0x34u);
VALIDATE_SIZE(mission_table_game_state_entry, 0xC);

VALIDATE_SIZE(mission_table_container, 0x48u);

bool mission_table_container::append_script_info(_std::vector<mission_table_container::script_info> *info)
{
    assert(info != nullptr);

    bool v9 = false;
    for ( auto &v6 : this->field_38 )
    {
        if ( v6.applies_to_current_hero() )
        {
            auto v5 = v6.instances.size();
            for ( auto i = 0u; i < v5; ++i )
            {
                script_info v3;
                v3.field_0 = v6.field_18;
                v3.field_4 = &v6.instances.at(i);
                v3.field_8 = i;
                info->push_back(v3);
                v9 = true;
            }
        }
    }

    return v9;
}

bool mission_table_container::append_nums(
        const char *a2,
        int a3,
        _std::vector<float> *nums) const
{
    assert(nums != nullptr);

    if constexpr (0)
    {
        bool result = false;
        for ( auto &v6 : this->field_38 )
        {
            auto *v4 = v6.field_18;
            if ( !strcmpi(v4, a2) && v6.append_nums(a3, nums) ) {
                result = true;
            }
        }

        return result;
    }
    else
    {
        bool (__fastcall *func)(const void *, void *edx, const char *, int, _std::vector<float> *) = CAST(func, 0x005DAA90);
        return func(this, nullptr, a2, a3, nums);
    }
}

void mission_table_container::un_mash(generic_mash_header *a2, void *a3, void *a4, generic_mash_data_ptrs *a5)
{
    THISCALL(0x005C6010, this, a2, a3, a4, a5);
}

mission_condition_instance * mission_condition::find_best_instance(
        mission_manager_script_data *data) const
{
    mission_condition_instance * (__fastcall *func)(const void *, void *edx, mission_manager_script_data *) = CAST(func, 0x005DD780);
    return func(this, nullptr, data);
}

bool mission_condition::get_key_po(int instance, po *p) const
{
    assert(p != nullptr);

    assert(instance >= 0);

    if ( instance >= this->instances.size() ) {
        return false;
    }

    auto &v4 = this->instances.at(instance);
    return v4.get_key_po(this, p);
}

bool mission_condition::applies_to_current_hero() const
{
    if (g_world_ptr == nullptr)
    {
        return false;
    }

    auto *hero = bit_cast<actor *>(g_world_ptr->get_hero_ptr(0));
    if (hero == nullptr)
    {
        return false;
    }

    auto *v8 = hero->get_player_controller();
    if (v8 == nullptr)
    {
        return false;
    }

    auto v15 = v8->m_hero_type;
    if ( v15 == 1 )
    {
        if ( this->is_flag_set(0x8000) )
        {
            return true;
        }
    }
    else if ( v15 == 2 )
    {
        if ( this->is_flag_set(0x4000) )
        {
            return true;
        }
    }
    else if ( v15 == 3 && this->is_flag_set(0x10000) )
    {
        return true;
    }

    return false;
}

bool mission_condition::check_condition(mission_manager_script_data *data) const
{
    assert(data != nullptr);

    if ( !this->applies_to_current_hero() ) {
        return false;
    }

    data->uses_script_stack = ( this->is_flag_set(0x1000) || this->is_flag_set(0x2000) );

    bool v11 = false;
    auto *v4 = mission_manager::s_inst;
    if ( v4->field_80 )
    {
        if ( _strcmpi(this->field_18, v4->field_88.to_string()) != 0 ) {
            return false;
        }

        v11 = true;
    }

    auto v12 = this->field_28;
    if ( (v12 & 0x800) != 0 )
    {
        auto v13 = v4->sub_5C5BD0();
        if ( v13 != 0 )
        {
            if ( v13 == 1 )
            {
                if ( !this->is_flag_set(0x200) && !v11 ) {
                    return false;
                }
            }
            else if ( v13 == 2 )
            {
                if ( !this->is_flag_set(0x400) && !v11 ) {
                    return false;
                }
            }
            else
            {
                assert(0 && "unknown time of day!!!");
            }
        }
    }

    for ( auto &entry : this->field_10 )
    {
        if ( !entry.check() ) {
            return false;
        }
    }

    auto *best_instance = this->find_best_instance(data);
    if ( best_instance == nullptr ) {
        return false;
    }

    data->field_0 = this->field_18;
    data->field_B8 = this->field_1C;
    data->field_10 = this->field_20;
    data->field_C8 = this->field_2C;
    data->field_D8 = this->field_30;
    data->field_B0 = ( (this->field_28 & 0x2000) != 0
                        ? best_instance->get_patrol_num()
                        : -1
                    );
    return true;
}

bool mission_condition::append_nums(int instance,
                        _std::vector<float> *nums) const
{
    assert(nums != nullptr);

    assert(instance >= 0);

    if ( instance >= this->instances.size() ) {
        return false;
    }

    auto &v4 = this->instances.at(instance);
    return v4.append_nums(this, nums);
}

bool mission_condition_instance::get_key_po(
        const mission_condition *,
        po *p) const
{
    assert(this->sentinel == 0x31415926 && "corruption!");

    assert(p != nullptr);

    if ( !this->is_flag_set(1) ) {
        return false;
    }

    *p = *this->field_4;
    return true;
}

bool mission_condition_instance::check_game_state() const
{
    for ( auto &entry : this->field_58 )
    {
        if ( !entry.check() ) {
            return false;
        }
    }

    return true;
}

const char *mission_condition_instance::get_script_data_name() const
{
    if ( !this->is_flag_set(8) ) {
        return nullptr;
    }

    assert(script_data_name != nullptr);

    return this->script_data_name;
}

bool mission_condition_instance::append_nums(
        const mission_condition *,
        _std::vector<float> *num_list) const
{
    assert(this->sentinel == 0x31415926 && "corruption!");

    assert(num_list != nullptr);

    const auto v4 = this->num_nums;
    auto begin = this->nums;
    auto end = this->nums + v4;
    std::for_each( begin, end, [&](const float v)
    {
        num_list->push_back(v);
    });

    return v4 > 0;
}

bool mission_condition_instance::append_strings(
        const mission_condition *,
        _std::vector<mString> *str_list) const
{
    assert(this->sentinel == 0x31415926 && "corruption!");

    assert(str_list != nullptr);

    auto v4 = this->num_strings;
    auto begin = this->strings;
    auto end = begin + v4;
    std::for_each(begin, end, [&](const auto &v)
    {
        str_list->push_back(v);
    });

    //std::copy(begin, end, std::back_inserter(*str_list));

    return v4 > 0;
}

bool mission_table_game_state_entry::check() const
{
    auto sub_6786F0 = [](const mission_table_game_state_entry *self, uint32_t a2) -> bool
    {
        return (a2 & self->field_8) != 0;
    };

    auto v1 = mission_manager::s_inst->field_80;
    auto v2 = this->field_8;
    bool v3 = false;
    float v4;
    if ( (v2 & 0x80u) != 0 ) {
        v4 = *this->field_4.p;
    } else if ( (v2 & 0x100) != 0 ) {
        v4 = *this->field_4.p;
    } else {
        v4 = this->field_4.f;
    }

    if ( sub_6786F0(this, 1) )
    {
        if ( v1 ) {
            *this->field_0 = v4;
        }

        v3 = ( *this->field_0 != v4 );
    }
    else if ( sub_6786F0(this, 2) )
    {
        if ( v1 && *this->field_0 == v4 ) {
            *this->field_0 = 1.0f + v4;
        }

        v3 = ( *this->field_0 != v4 );
    }
    else if ( sub_6786F0(this, 4) )
    {
        if ( v1 && v4 >= *this->field_0 ) {
            *this->field_0 = 1.0f + v4;
        }

        v3 = ( *this->field_0 > v4 );
    }
    else if ( sub_6786F0(this, 8) )
    {
        if ( v1 && v4 <= *this->field_0 ) {
            *this->field_0 = v4 - 1.0;
        }

        v3 = ( *this->field_0 < v4 );
    }
    else if ( sub_6786F0(this, 0x10) )
    {
        if ( v1 && v4 > *this->field_0 ) {
            *this->field_0 = v4;
        }

        v3 = ( *this->field_0 >= v4 );
    }
    else if ( !sub_6786F0(this, 0x20) )
    {
        if ( v1 && v4 < *this->field_0 ) {
            *this->field_0 = v4;
        }

        v3 = ( *this->field_0 <= v4 );
    }
    else
    {
        assert(0 && "unknown game state operator");
    }

    if ( v1 )
    {
        auto v5 = mission_manager::s_inst->field_60;
        if ( this->field_0 == v5 ) {
            mission_manager::s_inst->field_5C = *v5;
        }
    }

    return v3;
}
