#pragma once

#include "info_node.h"

#include "launch_layer_state.h"

#include "entity.h"
#include "entity_base_vhandle.h"

namespace ai {

struct drive_car_state : launch_layer_state {

    //0x0046C6D0
    //virtual
    state_trans_action _check_transition(Float a3);

    //0x0044DBD0
    //virtual
    state_trans_action _get_default_return_code() const;
};

struct ai_car_inode : info_node {
    bool field_1C;
    char field_1D;
    char field_1E;
    bool field_1F;
    vhandle_type<entity> field_20;
    vhandle_type<entity> field_24;
    vhandle_type<entity> field_28;
    int field_2C;
    bool field_30;
    drive_car_state *field_34;
    float field_38;

    bool inside_car() const {
        return this->field_1C;
    }

    entity * get_selected_car();

    bool search_finished() const;

    //0x0045EED0
    bool car_is_dead() const;

    //0x0046C380
    void clear_car();

    //0x0045EBE0
    void set_inside_car(bool inside);

    static inline const string_hash default_id {int(to_hash("AI_CAR_INODE"))};

    static int &cars_occupied;
};

}
