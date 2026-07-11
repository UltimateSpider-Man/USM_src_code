#include "als_scripted_category.h"

#include "als_basic_rule_data.h"
#include "als_data.h"
#include "als_request_data.h"
#include "als_transition_rule.h"
#include "mash_info_struct.h"
#include "scripted_trans_group.h"
#include "utility.h"
#include "trace.h"
#include "func_wrapper.h"
#include "common.h"

#include <cassert>

namespace als
{
    VALIDATE_SIZE(scripted_category, 0x7C);

    scripted_category::scripted_category()
    {
        THISCALL(0x004ACBF0, this);
    }

    void scripted_category::_unmash(mash_info_struct *a1, void *a3)
    {
        TRACE("als::scripted_category::unmash");

        if constexpr (1)
        {
            category::_unmash(a1, this);

            a1->unmash_class_in_place(this->field_10, this);

            a1->unmash_class_in_place(this->field_14, this);

            a1->unmash_class_in_place(this->field_2C, this);

            a1->unmash_class_in_place(this->field_3C, this);
            a1->unmash_class_in_place(this->field_50, this);
            a1->unmash_class_in_place(this->field_64, this);

#ifdef TARGET_XBOX
            {
                uint8_t class_mashed = -1;
                class_mashed = *a1->read_from_buffer(mash::SHARED_BUFFER, 1, 1);
                assert(class_mashed == 0xAF || class_mashed == 0);
            }
#endif

            if ( this->field_78 != nullptr )
            {
                a1->unmash_class(this->field_78, this
#ifdef TARGET_XBOX
                    , mash::NORMAL_BUFFER
#endif
                        );
            }
        }
        else
        {
            THISCALL(0x004AC850, this, a1, a3);
        }
    }

    request_data scripted_category::do_implicit_trans(
        animation_logic_system *a4,
        state_machine *a5)
    {
        TRACE("als::scripted_category::do_implicit_trans");

        // Converted from 0x004A7300.
        request_data data;
        als_data context {a4, a5};
        string_hash no_hash {0};

        if (!test_all_trans_groups(
                data,
                this->field_2C,
                scripted_trans_group::IMPLICIT,
                context,
                no_hash))
        {
            for (int i = 0; i < this->field_3C.size(); ++i) {
                auto **slot = &this->field_3C.m_data[i];
                auto *rule = *slot;
                if (rule != nullptr && rule->can_transition(context)) {
                    rule->field_0.field_14.process_action(data);
                    if (rule->field_0.has_post_action()) {
                        data.field_10 = scripted_trans_group::IMPLICIT;
                        data.field_C = reinterpret_cast<int>(slot);
                    }
                    break;
                }
            }
        }

        return data;
    }

    request_data scripted_category::do_incoming_trans(
        animation_logic_system *a3,
        state_machine *a4)
    {
        TRACE("als::scripted_category::do_incoming_trans");

        // Converted from 0x004A0230.
        request_data data;
        als_data context {a3, a4};

        for (int i = 0; i < this->field_64.size(); ++i) {
            auto **slot = &this->field_64.m_data[i];
            auto *rule = *slot;
            if (rule != nullptr && rule->can_transition(context)) {
                rule->field_0.field_14.process_action(data);
                if (rule->field_0.has_post_action()) {
                    data.field_10 = scripted_trans_group::INCOMING;
                    data.field_C = reinterpret_cast<int>(slot);
                }
                break;
            }
        }

        return data;
    }

    request_data scripted_category::do_explicit_trans(
        animation_logic_system *a3,
        state_machine *a4,
        string_hash a5)
    {
        TRACE("als::scripted_category::do_explicit_trans");

        // Converted from 0x004A7420.
        request_data data;
        als_data context {a3, a4};

        if (!test_all_trans_groups(
                data,
                this->field_2C,
                scripted_trans_group::EXPLICIT,
                context,
                a5))
        {
            for (int i = 0; i < this->field_50.size(); ++i) {
                auto **slot = &this->field_50.m_data[i];
                auto *rule = *slot;
                if (rule != nullptr && rule->can_transition(context, a5)) {
                    rule->field_0.field_14.process_action(data);
                    if (rule->field_0.has_post_action()) {
                        data.field_10 = scripted_trans_group::EXPLICIT;
                        data.field_C = reinterpret_cast<int>(slot);
                    }
                    break;
                }
            }
        }

        return data;
    }
}


als::request_data * __fastcall als_scripted_category_do_implicit_trans(
    als::scripted_category *self, void *,
    als::request_data *out,
    als::animation_logic_system *a4,
    als::state_machine *a5)
{
    *out = self->do_implicit_trans(a4, a5);
    return out;
}

als::request_data * __fastcall als_scripted_category_do_incoming_trans(
    als::scripted_category *self, void *,
    als::request_data *out,
    als::animation_logic_system *a3,
    als::state_machine *a4)
{
    *out = self->do_incoming_trans(a3, a4);
    return out;
}

als::request_data * __fastcall als_scripted_category_do_explicit_trans(
    als::scripted_category *self, void *,
    als::request_data *out,
    als::animation_logic_system *a3,
    als::state_machine *a4,
    string_hash a5)
{
    *out = self->do_explicit_trans(a3, a4, a5);
    return out;
}

void als_scripted_category_patch()
{
    {
        FUNC_ADDRESS(address, &als::scripted_category::_unmash);
        set_vfunc(0x0087E254, address);
    }

    {
        auto address = int(&als_scripted_category_do_implicit_trans);
        set_vfunc(0x0087E268, address);
    }

    {
        auto address = int(&als_scripted_category_do_incoming_trans);
        set_vfunc(0x0087E274, address);
    }

    {
        auto address = int(&als_scripted_category_do_explicit_trans);
        SET_JUMP(0x004A7420, address);
    }
}
