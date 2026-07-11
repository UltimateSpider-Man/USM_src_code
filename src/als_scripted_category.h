#pragma once

#include "als_category.h"
#include "string_hash.h"
#include "mvector.h"
#include "force_transitions.h"

struct mash_info_struct;

namespace als
{
    struct alter_conditions;
    struct implicit_transition_rule;
    struct explicit_transition_rule;
    struct layer_transition_rule;
    struct incoming_transition_rule;

    struct scripted_category : category
    {
        string_hash field_10;
        force_transitions field_14;
        mVectorBasic<int> field_2C;
        mVector<als::implicit_transition_rule> field_3C;
        mVector<als::explicit_transition_rule> field_50;
        mVector<als::incoming_transition_rule> field_64;
        mVector<als::layer_transition_rule> *field_78;

        scripted_category();

        //0x004AC850
        void _unmash(mash_info_struct *a1, void *);

        //virtual
        request_data do_implicit_trans(
            animation_logic_system *a4,
            state_machine *a5);

        //virtual
        request_data do_incoming_trans(
            animation_logic_system *a3,
            state_machine *a4);

        request_data do_explicit_trans(
            animation_logic_system *a3,
            state_machine *a4,
            string_hash a5);
    };
}

extern void als_scripted_category_patch();
