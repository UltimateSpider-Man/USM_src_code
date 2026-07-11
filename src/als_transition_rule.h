#pragma once

#include "string_hash.h"
#include "als_basic_rule_data.h"

struct mash_info_struct;

namespace als
{
    struct als_data;
    struct filter_data;

    struct implicit_transition_rule
    {
        als::basic_rule_data field_0;

        bool can_transition(als_data &a1) const;

        void unmash(mash_info_struct *a1, void *a3);
    };

    struct explicit_transition_rule
    {
        als::basic_rule_data field_0;
        string_hash field_24;

        bool can_transition(
            als_data &a1,
            string_hash a3) const;

        void unmash(mash_info_struct *a1, void *a3);
    };

    struct layer_transition_rule {
        int field_0;
        int field_4;
        basic_rule_data::rule_action field_8;
        int field_14;

        void unmash(mash_info_struct *, void *);

        bool can_transition(als_data &a2) const;
    };

    struct incoming_transition_rule {
        basic_rule_data field_0;
        int field_24;
        int field_28;

        void unmash(mash_info_struct *a1, void *a3);

        bool can_transition(als_data &a2) const;
    };
}
