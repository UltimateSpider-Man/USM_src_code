#pragma once

#include "als_data.h"
#include "als_transition_group_base.h"
#include "string_hash.h"

namespace als {

    struct implicit_transition_rule;
    struct explicit_transition_rule;
    struct layer_transition_rule;
    struct request_data;

    struct scripted_trans_group : transition_group_base
    {
        enum transition_type {
            IMPLICIT = 0,
            EXPLICIT = 1,
            LAYER = 2,
            INCOMING = 3,
        };

        mVectorBasic<int> field_4;
        mVector<implicit_transition_rule> field_14;
        mVector<explicit_transition_rule> field_28;
        mVector<layer_transition_rule> *field_3C;

        scripted_trans_group();

        //virtual
        void _unmash(mash_info_struct *a1, void *);

        bool check_transition(
            request_data &a2,
            scripted_trans_group::transition_type a3,
            als_data a4,
            string_hash a5) const;
    };

    extern bool test_all_trans_groups(
        request_data &a1,
        const mVectorBasic<int> &a2,
        scripted_trans_group::transition_type a3,
        als_data a4,
        string_hash a5);


}
