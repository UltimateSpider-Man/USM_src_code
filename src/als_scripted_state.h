#pragma once

#include "als_state.h"
#include "als_transition_post_handle.h"
#include "mvector.h"

struct mash_info_struct;

namespace als
{
    struct dest_weight_data
    {
        string_hash field_0;
        int field_4;
    };

    struct implicit_transition_rule;

    struct explicit_transition_rule;

    struct layer_transition_rule;

    struct request_data;
    struct animation_logic_system;
    struct state_machine;

    struct scripted_state : state
    {
        string_hash field_14;
        mVectorBasic<int> field_18;
        mVector<als::implicit_transition_rule> field_28;
        mVector<als::explicit_transition_rule> field_3C;
        mVector<als::layer_transition_rule> *field_50;

        scripted_state();

        void _unmash(mash_info_struct *a1, void *a3);

        //virtual
        int get_filter(
            int out,
            animation_logic_system *a2,
            state_machine *a3,
            int a4);

        //virtual
        request_data do_implicit_trans(
            animation_logic_system *a4,
            state_machine *a5);

        request_data do_explicit_trans(
            animation_logic_system *a4,
            state_machine *a5,
            string_hash a6);

        //virtual
        void do_post_trans(
            animation_logic_system *a1,
            state_machine *a2,
            transition_post_handle a4);

        string_hash get_nal_anim_name() const;
    };

    struct base_layer_scripted_state : scripted_state
    {
        int field_54;

        base_layer_scripted_state();

        void _unmash(mash_info_struct *a1, void *a3);
    };
}

extern void als_scripted_state_patch();
