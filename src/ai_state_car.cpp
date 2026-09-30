#include "ai_state_car.h"

#include "base_ai_core.h"
#include "common.h"
#include "func_wrapper.h"
#include "traffic.h"

VALIDATE_SIZE(ai::drive_car_state, 0x44);

VALIDATE_SIZE(ai::ai_car_inode, 0x3C);

namespace ai {

int &ai_car_inode::cars_occupied = var<int>(0x00958068);

state_trans_action drive_car_state::_check_transition(
        Float a3)
{
    auto result = this->_get_default_return_code();
    auto *ai_core = this->get_core();
    auto *info_node = (ai_car_inode *) ai_core->get_info_node(ai_car_inode::default_id, true);
    auto *selected_car = info_node->get_selected_car();
    if ( selected_car != nullptr 
            && info_node->inside_car()
            && selected_car->is_visible() )
    {
        if ( info_node->car_is_dead() )
        {
            result = this->process_message(a3, static_cast<state_trans_messages>(1));
        }
    }
    else
    {
        info_node->clear_car();
        result = this->process_message(a3, static_cast<state_trans_messages>(2));
    }

    return result;
}

state_trans_action drive_car_state::_get_default_return_code() const
{
    auto result = state_trans_action {
        static_cast<state_trans_actions>(4),
        string_hash {0},
        static_cast<state_trans_messages>(75),
        nullptr
    };

    return result;
}

entity * ai_car_inode::get_selected_car()
{
    assert(this->search_finished());

    if ( this->field_28.get_volatile_ptr() == nullptr ) {
        return nullptr;
    } else {
        return this->field_28.get_volatile_ptr();
    }
}

bool ai_car_inode::search_finished() const
{
    return (this->field_20.get_volatile_ptr() == nullptr);
}

bool ai_car_inode::car_is_dead() const
{
    return (bool) THISCALL(0x0045EED0, this);
}

void ai_car_inode::clear_car()
{
    THISCALL(0x0046C380, this);
}

void ai_car_inode::set_inside_car(bool inside)
{
    assert(!inside || this->get_selected_car() != nullptr);

    bool v9 = this->inside_car();
    auto *selected_car = this->get_selected_car();
    if ( selected_car != nullptr )
    {
        this->field_1C = inside;
        auto *the_traffic = traffic::get_traffic_from_entity((vhandle_type<entity>)selected_car->my_handle.field_0);
        if ( the_traffic != nullptr )
        {
            auto v6 = this->field_1C;
            the_traffic->set_ai_car_occupied(v6);
            if ( !this->field_1E )
            {
                if ( inside ) {
                    the_traffic->set_driver_type(2);
                } else {
                    the_traffic->set_driver_type(0);
                }
            }

            if ( the_traffic->is_ai_car_occupied() )
            {
                if ( !v9 ) {
                    this->field_38 = the_traffic->field_1B8;
                }
            }
            else if ( v9 )
            {
                the_traffic->field_1B8 = this->field_38;
            }
        }
    }
    else
    {
        this->field_1C = false;
    }

    bool v8 = this->field_1C;
    if ( !v8 ) {
        this->field_34 = nullptr;
    }

    if ( v9 )
    {
        if ( !v8 ) {
            --ai_car_inode::cars_occupied;
        }
    }
    else if ( v8 )
    {
        ++ai_car_inode::cars_occupied;
    }
}

}

