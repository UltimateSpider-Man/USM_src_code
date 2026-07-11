#include "scripted_trans_group.h"

#include "als_basic_rule_data.h"
#include "als_request_data.h"
#include "als_transition_rule.h"
#include "common.h"
#include "func_wrapper.h"
#include "trace.h"
#include "utility.h"

#include <cassert>

namespace als
{

VALIDATE_SIZE(scripted_trans_group, 0x40u);

const char *to_string(scripted_trans_group::transition_type trans_type)
{
    switch (trans_type) {
    case scripted_trans_group::IMPLICIT: return "IMPLICIT";
    case scripted_trans_group::EXPLICIT: return "EXPLICIT";
    case scripted_trans_group::LAYER:    return "LAYER";
    case scripted_trans_group::INCOMING: return "INCOMING";
    default:                            return "UNKNOWN";
    }
}

scripted_trans_group::scripted_trans_group()
{
    THISCALL(0x004AC950, this);
}

void scripted_trans_group::_unmash(mash_info_struct *a1, void *)
{
    TRACE("scripted_trans_group::unmash");

    a1->unmash_class_in_place(this->field_4, this);

    a1->unmash_class_in_place(this->field_14, this);

    a1->unmash_class_in_place(this->field_28, this);

#ifdef TARGET_XBOX
    {
    uint8_t class_mashed = -1;
    class_mashed = *a1->read_from_buffer(mash::SHARED_BUFFER, 1, 1);
    assert(class_mashed == 0xAF || class_mashed == 0);
    }
#endif

    if ( this->field_3C != nullptr )
    {
    a1->unmash_class(this->field_3C, this
#ifdef TARGET_XBOX
        , mash::NORMAL_BUFFER
#endif
            );
    }
}

bool scripted_trans_group::check_transition(
        request_data &data,
        scripted_trans_group::transition_type trans_type,
        als_data a4,
        string_hash a5) const
{
    TRACE("scripted_trans_group::check_transition", to_string(trans_type));

    // Converted from 0x004A0090.  A child transition group returning true
    // short-circuits the caller; local rule hits process their action but this
    // routine still returns false, matching the original control flow.
    if ( test_all_trans_groups(data, this->field_4, trans_type, a4, a5) ) {
        return true;
    }

    switch (trans_type) {
    case IMPLICIT:
        for (int i = 0; i < this->field_14.size(); ++i) {
            auto **slot = &this->field_14.m_data[i];
            auto *rule = *slot;
            if (rule != nullptr && rule->can_transition(a4)) {
                rule->field_0.field_14.process_action(data);
                if (rule->field_0.has_post_action()) {
                    data.field_10 = trans_type;
                    data.field_C = reinterpret_cast<int>(slot);
                }
                break;
            }
        }
        return false;

    case EXPLICIT:
        for (int i = 0; i < this->field_28.size(); ++i) {
            auto **slot = &this->field_28.m_data[i];
            auto *rule = *slot;
            if (rule != nullptr && rule->can_transition(a4, a5)) {
                rule->field_0.field_14.process_action(data);
                if (rule->field_0.has_post_action()) {
                    data.field_10 = trans_type;
                    data.field_C = reinterpret_cast<int>(slot);
                }
                break;
            }
        }
        return false;

    case LAYER:
        if (this->field_3C != nullptr) {
            for (int i = 0; i < this->field_3C->size(); ++i) {
                auto *rule = this->field_3C->at(static_cast<uint16_t>(i));
                if (rule != nullptr && rule->can_transition(a4)) {
                    rule->field_8.process_action(data);
                }
            }
        }
        return false;

    case INCOMING:
    default:
        assert(0 && "Unknown type specified for trans group.");
        return false;
    }
}

}


void als_scripted_trans_group_patch()
{
    {
        FUNC_ADDRESS(address, &als::scripted_trans_group::_unmash);
        set_vfunc(0x0087E1BC, address);
    }
}
