#include "state_machine.h"

#include "actor.h"
#include "als_animation_logic_system.h"
#include "als_category.h"
#include "als_state.h"
#include "common.h"
#include "func_wrapper.h"
#include "layer_state_machine_shared.h"
#include "nal_generic.h"
#include "oldmath_po.h"
#include "osassert.h"
#include "trace.h"
#include "utility.h"
#include "vtbl.h"

namespace als {

VALIDATE_SIZE(state_machine, 0x54);

state_machine::state_machine()
{
    if constexpr (0) {
        this->m_vtbl = 0x008813D8;

        [](auto &self) {
            self.clear();
        }(this->field_8);

        this->curr_req_data.did_transition_occur = false;
        this->curr_req_data.field_1 = false;
        this->curr_req_data.field_2 = false;
        this->curr_req_data.field_3 = false;
        this->curr_req_data.field_4 = false;
        this->curr_req_data.field_C = 0;

        this->shared_portion = nullptr;
        this->m_curr_state = nullptr;
        this->m_prev_state = nullptr;
    } else {
        THISCALL(0x004A9180, this);
    }
}

void state_machine::set_anim_handle(animation_controller::anim_ctrl_handle &a2)
{
    this->field_48 = a2;
}

param_node *state_machine::find_external_param(external_parameter_types a2) const
{
    TRACE("als::state_machine::find_external_param");

    if constexpr (0) {
        if ( !this->field_8.field_8.is_empty() && this->is_interruptable() )
        {
            auto *result = this->field_8.field_8.field_0;
            while ( 1 )
            {
                if (result->field_0.field_0 == a2) {
                    return result;
                }

                result = result->field_8;
                if ( result == this->field_8.field_8.field_0 ) {
                    break;
                }
            }
        }

        if ( this->field_34.field_0.is_empty() ) {
            return nullptr;
        }

        auto *result = this->field_34.field_0.field_0;
        while ( result->field_0.field_0 != a2 )
        {
            result = result->field_8;
            if ( result == this->field_34.field_0.field_0 ) {
                return nullptr;
            }
        }

        return result;

    } else {
        return (param_node *) THISCALL(0x00493660, this, a2);
    }
}

float state_machine::get_internal_param(
        animation_logic_system *a3,
        internal_parameter_types a4) const
{
    TRACE("state_machine::get_internal_param");

    float (__fastcall *func)(const void *, void *, animation_logic_system *, internal_parameter_types) = CAST(func, 0x0049CFD0);
    return func(this, nullptr, a3, a4);
}

bool state_machine::is_curr_state_interruptable(
        animation_logic_system *a2) const
{
    auto func = [](const auto *self) {
        return !self->is_flag_set(static_cast<state_flags>(1));
    };

    return a2->sub_49F2A0() && func(this->m_curr_state);
}

bool state_machine::has_ext_param_been_set(uint32_t a2) const
{
    return this->find_external_param(static_cast<external_parameter_types>(a2)) != nullptr;
}

ai::param_block *state_machine::find_param_block_with_param(string_hash a2) const
{
    if ( this->is_active() )
    {
        ai::param_block *v4 = this->m_curr_state->field_10;
        if (v4 != nullptr && v4->does_parameter_exist(a2))
        {
            return v4;
        }
        else
        {
            auto *curr_category = this->get_curr_category();
            auto *v4 = curr_category->field_C;

            if (v4 != nullptr && v4->does_parameter_exist(a2)) {
                return v4;
            }
        }
    }

    return nullptr;
}

ai::param_block *state_machine::find_param_block_with_param(
        string_hash a2,
        ai::param_types a3) const
{
    if ( this->is_active() ) {
        ai::param_block *v5 = this->m_curr_state->field_10;
        if (v5 != nullptr
            && v5->does_parameter_exist(a2)
            && v5->get_parameter_data_type(a2) == a3)
        {
            return v5;
        }
        else
        {
            ai::param_block *v5 = this->get_curr_category()->field_C;

            if (v5 != nullptr
                && v5->does_parameter_exist(a2)
                && v5->get_parameter_data_type(a2) == a3)
            {
                return v5;
            }
        }
    }

    return nullptr;
}

float state_machine::get_param(
        animation_logic_system *a2,
        unsigned int a3) const
{
    TRACE("als::state_machine::get_param");

    if constexpr (0) {
    auto func = [](const param_cache &self, int a2) -> int
    {
        int i;
        auto num_params_in_cache = self.get_num_params_in_cache();
        for ( i = 0; i < num_params_in_cache; ++i )
        {
            if ( self.field_0[i].field_0 == a2 ) {
                return i;
            }
        }

        return -1;
    };

    auto v5 = func(this->field_40, a3);
    if ( v5 != -1 ) {
        return this->field_40.get_from_cache(v5);
    }

        if ( a3 < 91 )
        {
            auto *external_param = this->find_external_param((external_parameter_types)a3);
            if ( external_param != nullptr ) {
                return bit_cast<param_cache *>(&this->field_40)->cache_param(a3, external_param->field_0.field_4);
            } else {
                return 0.0;
            }
        }
        else
        {
            auto internal_param = this->get_internal_param(a2, static_cast<internal_parameter_types>(a3));
            return bit_cast<param_cache *>(&this->field_40)->cache_param(a3, internal_param);
        }

    } else {
        float (__fastcall *func)(const void *, void *, animation_logic_system *, uint32_t) = CAST(func, 0x0049FB00);
        return func(this, nullptr, a2, a3);
    }
}

vector3d state_machine::get_vector_param(
        animation_logic_system *a2,
        uint32_t a3) const
{
    if ( a3 >= 91 )
    {
        vector3d result;
        switch ( a3 )
        {
        case 108u:
        case 109u:
        case 110u: {
            auto *v9 = a2->get_actor();
            result = v9->get_abs_po().get_y_facing();
            break;
        }
        case 111u:
        case 112u:
        case 113u: {
            auto *v11 = a2->get_actor();
            result = v11->get_abs_po().get_x_facing();
            break;
        }
        case 115u:
        case 116u:
        case 117u: {
            auto *v10 = a2->get_actor();
            result = v10->get_abs_po().get_z_facing();
            break;
        }
        default:
            assert("Internal Vector parameter has not been added to get_vector_param");
            result = vector3d {0.0, 0.0, 0.0};
            break;
        }

        return result;
    }

    auto a3a = this->get_param(a2, a3 + 2);
    auto a2a = this->get_param(a2, a3 + 1);
    auto a1a = this->get_param(a2, a3);
    vector3d result {a1a, a2a, a3a};
    return result;
}

bool state_machine::did_do_transition() const
{
    return this->field_14.m_trans_succeed;
}

void state_machine::request_category_transition(string_hash a2)
{
    TRACE("als::state_machine::request_category_transition");

    // Converted from 0x004934B0.  Forced transitions leave field_8.field_2
    // set, so a normal category request must not overwrite them.
    if ( !this->field_8.field_2 )
    {
        this->field_8.field_1 = true;
        this->field_8.m_cat_id = a2;
    }
}

bool state_machine::is_interruptable() const {
    // Converted from 0x004934D0.
    return this->field_14.m_curr_state_interruptable || !this->field_14.m_active;
}

bool state_machine::did_transition_succeed() const
{
    TRACE("als::state_machine::did_transition_succeed");

    return this->field_14.m_trans_succeed;
}

bool state_machine::is_request_satisfied() const
{
    TRACE("als::state_machine::is_request_satisfied");

    if constexpr (1) {
        return !this->field_14.m_request_not_satisfied;
    } else {
        bool (__fastcall *func)(const void *) = CAST(func, get_vfunc(m_vtbl, 0x1C));
        return func(this);
    }
}

bool state_machine::is_active() const
{
    // Converted from 0x00493510.
    return this->field_14.m_active;
}

void state_machine::force_als_state(string_hash a2, int )
{
    TRACE("als::state_machine::force_als_state");

    this->field_8.field_1 = true;
    this->field_8.field_2 = true;
    this->field_8.m_cat_id = a2;
}

bool state_machine::does_category_exist(string_hash a2) const
{
    TRACE("als::state_machine::does_category_exist");

    assert(this->shared_portion != nullptr);

    auto &cat_list = this->shared_portion->category_list;
    auto begin = cat_list.m_data;
    auto end = begin + cat_list.size();
    auto it = std::find_if(begin, end, [a2](auto &cat)
    {
        return cat->field_4 == a2;
    });

    return it != end;
}

float state_machine::get_pb_float(string_hash a1) const
{
    auto *the_pblock = this->find_param_block_with_param(a1, static_cast<ai::param_types>(0));
    assert(the_pblock != nullptr && "Asking for a parameter that doesn't exist.");

    return the_pblock->get_pb_float(a1);
}

vector3d * state_machine::get_pb_vector3d(string_hash a2) const
{
    auto *the_pblock = this->find_param_block_with_param(a2, static_cast<ai::param_types>(4));
    assert(the_pblock != nullptr && "Asking for a parameter that doesn't exist.");

    return the_pblock->get_pb_vector3d(a2);
}

variance_variable<float> * state_machine::get_pb_float_variance(string_hash a2) const
{
    auto *the_pblock = this->find_param_block_with_param(a2, static_cast<ai::param_types>(5));
    assert(the_pblock != nullptr && "Asking for a parameter that doesn't exist.");

    return the_pblock->get_pb_float_variance(a2);
}

bool state_machine::does_parameter_exist(string_hash a1) const
{
    // Converted from 0x004A6A00.
    return this->find_param_block_with_param(a1) != nullptr;
}

int state_machine::get_parameter_data_type(string_hash a2) const
{
    auto *the_param_block = this->find_param_block_with_param(a2);
    if ( the_param_block != nullptr ) {
        return the_param_block->get_parameter_data_type(a2);
    }

    return 9;
}

//TODO
float state_machine::get_time_to_end_of_anim() const
{
    TRACE("als::state_machine::get_time_to_end_of_anim");

    if constexpr (0)
    {
        auto &the_handle = this->get_anim_handle();
        assert(the_handle.is_anim_active());

        auto *anim_ptr = static_cast<nalGeneric::nalGenericAnim *>(the_handle.get_anim_ptr());

        if ( (anim_ptr->field_34 & 1) != 0 ) {
            return -1.0f;
        }

        auto v11 = anim_ptr->field_38 - the_handle.get_anim_time_in_sec();
        return v11 * (1.0f / the_handle.get_anim_speed());
    } else {
        float (__fastcall *func)(const void *) = CAST(func, 0x004993C0);
                //get_vfunc(m_vtbl, 0x50));
        return func(this);
    }
}

bool state_machine::is_cat_our_prev_cat(string_hash a2) const
{
    TRACE("als::state_machine::is_cat_our_prev_cat");

    if constexpr (1) {
        if ( !this->field_14.m_trans_succeed ) {
            return false;
        }

        return a2 == this->field_14.m_cat_id
                && a2 != this->get_category_id();
    } else {
        bool (__fastcall *func)(const void *, void *, string_hash) = CAST(func, get_vfunc(m_vtbl, 0x58));
        return func(this, nullptr, a2);
    }
}

bool state_machine::is_requesting_category(string_hash a2) const
{
    TRACE("als::state_machine::is_requesting_category");

    if constexpr (1) {
        return ( this->field_8.field_1
                    && this->field_8.m_cat_id == a2 );
    } else {
        bool (__fastcall *func)(const void *, void *, string_hash) = CAST(func, get_vfunc(m_vtbl, 0x5C));
        return func(this, nullptr, a2);
    }
}

string_hash state_machine::get_category_id() const
{
    TRACE("als::state_machine::get_category_id");

    if constexpr (1) {
        if ( this->is_active() ) {
            return this->m_curr_state->get_category_id();
        }

        return string_hash {0};
    } else {
        void (__fastcall *func)(const void *, void *, string_hash *) = CAST(func, get_vfunc(m_vtbl, 0x0));

        string_hash id;
        func(this, nullptr, &id);
        return id;
    }
}

double get_character_time_to_signal(const animation_controller::anim_ctrl_handle &a1, string_hash a2, bool a3)
{
    TRACE("get_character_time_to_signal");

    double (__cdecl *func)(const animation_controller::anim_ctrl_handle *, string_hash, bool) = CAST(func, 0x004939D0);
    return func(&a1, a2, a3);
}

double get_generic_time_to_signal(const animation_controller::anim_ctrl_handle &a1, string_hash a2, bool a3)
{
    TRACE("get_generic_time_to_signal");

    double (__cdecl *func)(const animation_controller::anim_ctrl_handle *, string_hash, bool) = CAST(func, 0x0049DF60);
    return func(&a1, a2, a3);
}


float state_machine::get_time_to_signal(string_hash a2)
{
    TRACE("als::state_machine::get_time_to_signal");

    if constexpr (1) {
        auto &the_handle = this->get_anim_handle();
        assert(the_handle.is_anim_active());

        float time_to_signal = ( the_handle.is_same_animtype(tlFixedString {"Character"})
                                    ? get_character_time_to_signal(
                                        the_handle,
                                        a2,
                                        false)
                                    : get_generic_time_to_signal(
                                        the_handle, 
                                        a2,
                                        false)
                                );

        auto result = ( time_to_signal < 0.0f
                        ? time_to_signal
                        : time_to_signal * (1.0f / the_handle.get_anim_speed())
                        );

        sp_log("%f", result);
        return result;
    } else {
        float (__fastcall *func)(void *, void *, string_hash) = CAST(func, 0x0049F4E0);
        auto result = func(this, nullptr, a2);

        return result;
    }
}

void state_machine::set_desired_params(param_list &a2) {
    param_list &v2 = this->field_8.field_8;
    v2.concat_list(a2);
    v2.cull_duplicates_keep_last();
}

void state_machine::set_desired_param(const param &a2) {
    if constexpr (1) {
        param_list v8{};
        auto &v7 = a2;

        v8.add_param(v7);

        auto *v4 = &this->field_8.field_8;
        v4->concat_list(v8);

        v4->cull_duplicates_keep_last();

        v8.clear();
    } else {
        THISCALL(0x004A6A80, this, &a2);
    }
}

string_hash state_machine::get_state_id() const
{
    TRACE("als::state_machine::get_state_id");

    if constexpr (0)
    {
        if ( this->is_active() )
        {
            auto *curr_state = this->get_curr_state();
            if ( curr_state->is_flag_set(static_cast<state_flags>(0x1000)) )
            {
                return curr_state->get_state_id();
            }

            return string_hash {0};
        }
        else
        {
            return string_hash {0};
        }

    } else {
        void (__fastcall *func)(const void *, void *, string_hash *) = CAST(func, get_vfunc(this->m_vtbl, 0x4));

        string_hash id;
        func(this, nullptr, &id);
        return id;
    }
}

float state_machine::get_optional_pb_float(
        const string_hash &a2,
        Float a3,
        bool *a4) const
{
    auto *the_param_block = this->find_param_block_with_param(a2, static_cast<ai::param_types>(0));
    if ( a4 != nullptr ) {
        *a4 = (the_param_block != nullptr);
    }

    if ( the_param_block != nullptr ) {
        return the_param_block->get_pb_float(a2);
    }

    return a3;
}

int state_machine::get_optional_pb_int(
        const string_hash &a2,
        int a3,
        bool *a4) {
    // Converted from 0x004A6860.
    auto *the_param_block = this->find_param_block_with_param(a2, ai::PT_INTEGER);
    if ( a4 != nullptr ) {
        *a4 = (the_param_block != nullptr);
    }

    if ( the_param_block != nullptr ) {
        return the_param_block->get_pb_int(a2);
    }

    return a3;
}

layer_types state_machine::get_layer_id() {
    layer_types (__fastcall *func)(void *) = CAST(func, get_vfunc(m_vtbl, 0x7C));
    return func(this);
}

bool state_machine::determine_if_request_satisfied(animation_logic_system *a2) const
{
    bool (__fastcall *func)(const void *, void *, animation_logic_system *) = CAST(func, get_vfunc(m_vtbl, 0x80));
    return func(this, nullptr, a2);
}

state *state_machine::get_curr_state() const {
    return m_curr_state;
}

void state_machine::process_requests(animation_logic_system *a2)
{
    TRACE("als::state_machine::process_requests");

    {
        auto &v1 = this->field_8;
        sp_log("%d %d %d", v1.field_0, v1.field_1, v1.field_2);
        sp_log("%s", v1.m_cat_id.to_string());
    }

    if constexpr (1)
    {
        this->curr_req_data.clear();
        this->field_40.clear_cache();
        if ( this->field_8.field_0 )
        {
            layer_types v4 = this->get_layer_id();
            a2->sub_4A6630(v4);
            this->field_34.clear();
        }
        else
        {
            if ( this->field_8.field_1 ) {
                if ( this->field_8.field_2 ) {
                    this->do_force_state_trans(a2);
                } else {
                    auto *cat = this->find_category(this->field_8.m_cat_id);
                    if ( cat != nullptr ) {
                        if ( cat->is_flag_set(1u) ) {
                            this->do_cat_force_trans(a2);
                        } else {
                            this->do_explicit_trans(a2);
                        }
                    } else {
                        this->do_implicit_trans(a2);
                    }
                }
            } else if ( this->is_active() ) {
                this->do_implicit_trans(a2);
            }
        }
    } else {
        THISCALL(0x004A6BA0, this, a2);
    }
}

void state_machine::do_force_state_trans(animation_logic_system *a2)
{
    TRACE("als::state_machine::do_force_state_trans");

    THISCALL(0x0049F910, this, a2);
}

void state_machine::do_cat_force_trans(animation_logic_system *a2)
{
    TRACE("als::state_machine::do_cat_force_trans");

    THISCALL(0x0049F850, this, a2);
}

bool category_binary_search(string_hash a1, category **a2, int size, int *found_idx)
{
    return (bool) CDECL_CALL(0x004935B0, a1, a2, size, found_idx);
}

category *state_machine::find_category(string_hash a2) const
{
    assert(this->shared_portion != nullptr);

    auto size = this->shared_portion->category_list.size();
    auto **data = this->shared_portion->category_list.m_data;
    int v8;
    if ( category_binary_search(a2, data, size, &v8) ) {
        return this->shared_portion->category_list.at(v8);
    }

    auto *v4 = a2.to_string();
    sp_log("Could not find category (%s).", v4);
    return nullptr;
}

category *state_machine::get_curr_category() const
{
    auto *curr_state = this->m_curr_state;
    auto v4 = curr_state->get_category_id();
    return this->find_category(v4);
}

scripted_trans_group *state_machine::get_trans_group(int idx) const
{
    return bit_cast<scripted_trans_group *>(this->shared_portion->trans_group_list.at(idx));
}

void state_machine::set_active(
        animation_logic_system *a2,
        string_hash a3)
{
    TRACE("als::state_machine::set_active");
    
    this->field_14.m_active = true;
    auto *the_state = this->find_state(a3);
    this->change_state(a2, the_state);
    this->field_14.m_trans_succeed = true;
    this->field_34.clear();
}

void state_machine::set_pending_params(param_list &a2)
{
    TRACE("state_machine::set_pending_params");

    if constexpr (0)
    {
    }
    else
    {
        THISCALL(0x004995B0, this, &a2);
    }
}

void state_machine::update_pending_params(animation_logic_system *a2)
{
    TRACE("state_machine::update_pending_params");

    this->field_14.m_request_not_satisfied = !this->determine_if_request_satisfied(a2);
    auto v5 = this->field_14.m_curr_state_interruptable;
    if ( this->field_14.m_active )
    {
        this->field_14.m_curr_state_interruptable = this->is_curr_state_interruptable(a2);
    }

    if ( v5 || this->field_14.m_curr_state_interruptable ) {
        this->set_pending_params(this->field_8.field_8);
    }

    this->field_8.clear();

    this->field_40.clear_cache();
}

void state_machine::do_implicit_trans(animation_logic_system *a2)
{
    TRACE("als::state_machine::do_implicit_trans");

    sp_log("0x%08X", this->m_curr_state->m_vtbl);

    if constexpr (1) {
        auto func = [](const request_data &self) -> bool
        {
            return self.did_transition_occur || self.field_1;
        };

        this->curr_req_data = this->m_curr_state->do_implicit_trans(a2, this);

        sp_log("did_transition_occur = %d, %d %d %d", this->curr_req_data.did_transition_occur,
                                this->curr_req_data.field_1,
                                this->curr_req_data.field_2,
                                this->curr_req_data.field_3);
        if ( func(this->curr_req_data) || !this->curr_req_data.field_3 ) {
            if ( func(this->curr_req_data) ) {
                if (this->curr_req_data.field_2) {
                    auto *cat = this->find_category(this->curr_req_data.field_8);
                    this->curr_req_data = cat->do_incoming_trans(a2, this);
                    if ( func(this->curr_req_data) ) {
                        auto *the_state = this->find_state(this->curr_req_data.field_8);
                        this->change_state(a2, the_state);
                    } else {
                        assert(0 && "Implicit transition to category specified failed.");
                    }
                } else {
                    auto *the_state = this->find_state(this->curr_req_data.field_8);
                    this->change_state(a2, the_state);
                }
            } else {
                this->field_14.m_trans_succeed = false;
            }

        } else {
            auto v16 = this->m_curr_state->get_category_id();
            auto *v6 = this->find_category(v16);
            this->curr_req_data = v6->do_implicit_trans(a2, this);
            if ( func(this->curr_req_data) ) {
                if ( this->curr_req_data.field_2 ) {
                    auto *v10 = this->find_category(this->curr_req_data.field_8);
                    this->curr_req_data = v10->do_incoming_trans(a2, this);
                    if ( func(this->curr_req_data) ) {
                        auto *the_state = this->find_state(this->curr_req_data.field_8);
                        this->change_state(a2, the_state);
                    } else {
                        assert(0 && "Implicit transition to category specified failed.");
                    }
                }
            } else {
                this->field_14.m_trans_succeed = false;
            }
        }

    } else {
        THISCALL(0x0049CD30, this, a2);
    }
}

void state_machine::do_explicit_trans(animation_logic_system *a2)
{
    TRACE("als::state_machine::do_explicit_trans");

    THISCALL(0x0049F5A0, this, a2);
}

state *state_machine::find_state(string_hash a2) const
{
    TRACE("als::state_machine::find_state", a2.to_string());

    assert(this->shared_portion != nullptr);

    auto &state_list = this->shared_portion->state_list;
    auto begin = state_list.m_data;
    auto end = begin + state_list.size();
    auto it = std::find_if(begin, end, [a2](auto *the_state)
    {
        return (the_state->get_state_id() == a2);
    });

    if (it != end) {
        return (*it);
    }

    auto *v5 = a2.to_string();
    error("Could not find state (%s).", v5);
    return nullptr;
}

void state_machine::change_state(
        animation_logic_system *a2,
        state *a3)
{
    TRACE("als::state_machine::change_state");

    if (a2 != nullptr) {
    }

    if ( !this->field_14.m_trans_succeed ) {
        this->m_prev_state = this->m_curr_state;

        sp_log("prev_state = %s, curr_state = %s", this->m_prev_state->get_state_id().to_string(),
                a3->get_state_id().to_string());
    }

    this->m_curr_state = a3;
    this->field_14.m_trans_succeed = true;
    this->field_14.m_active = (this->m_curr_state != nullptr);
    auto *prev_state = this->m_prev_state;
    this->field_14.m_cat_id = ( prev_state != nullptr
                                ? prev_state->get_category_id()
                                : string_hash {0}
                                );
}

} // namespace als


string_hash * als_state_machine_get_state_id(als::state_machine *self, void *, string_hash *out) {
    *out = self->get_state_id();
    return out;
}

void als_state_machine_patch()
{
    auto REPLACE = [](auto addr, auto func) -> void {
        FUNC_ADDRESS(address, func);
        SET_JUMP(addr, address);
    };

    REPLACE(0x00493850, &als::state_machine::change_state);

    REPLACE(0x0049F9B0, &als::state_machine::set_active);

    REPLACE(0x004934B0, &als::state_machine::request_category_transition);

    REPLACE(0x004934D0, &als::state_machine::is_interruptable);

    REPLACE(0x004934F0, &als::state_machine::did_transition_succeed);

    if constexpr (0)
    {
        auto address = int(&als_state_machine_get_state_id);
        SET_JUMP(0x00499320, address);
    }

    REPLACE(0x00493500, &als::state_machine::is_request_satisfied);

    REPLACE(0x00493510, &als::state_machine::is_active);

    REPLACE(0x004A6A00, &als::state_machine::does_parameter_exist);

    REPLACE(0x004A6860, &als::state_machine::get_optional_pb_int);

    {
        FUNC_ADDRESS(address, &als::state_machine::find_external_param);
        REDIRECT(0x0049FB7E, address);
    }

    {
        FUNC_ADDRESS(address, &als::state_machine::get_internal_param);
        REDIRECT(0x0049FB39, address);
        REDIRECT(0x0049D3CD, address);
        REDIRECT(0x0049DCBD, address);
    }

    {
        FUNC_ADDRESS(address, &als::state_machine::get_time_to_end_of_anim);
        set_vfunc(0x00881428, address);
        set_vfunc(0x00881500, address);
        set_vfunc(0x00881588, address);
    }

    {
        FUNC_ADDRESS(address, &als::state_machine::process_requests);
        REDIRECT(0x004A910D, address);
    }

    return;

    {
        FUNC_ADDRESS(address, &als::state_machine::get_time_to_signal);
        set_vfunc(0x0088142C, address);
        set_vfunc(0x00881504, address);
        set_vfunc(0x0088158C, address);
    }

    {
        FUNC_ADDRESS(address, &als::state_machine::force_als_state);
        SET_JUMP(0x00493520, address);
    }

    {
        FUNC_ADDRESS(address, &als::state_machine::does_category_exist);
        SET_JUMP(0x00499370, address);
    }

    {
        FUNC_ADDRESS(address, &als::state_machine::find_state);
        SET_JUMP(0x004994C0, address);
    }

    {
        FUNC_ADDRESS(address, &als::state_machine::is_cat_our_prev_cat);
        SET_JUMP(0x00493550, address);
    }

    {
        FUNC_ADDRESS(address, &als::state_machine::is_requesting_category);
        SET_JUMP(0x00493470, address);
    }

    {
        FUNC_ADDRESS(address, &als::state_machine::do_force_state_trans);
        REDIRECT(0x004A6C04, address);

    }

    {
        FUNC_ADDRESS(address, &als::state_machine::do_implicit_trans);
        REDIRECT(0x0049F82D, address);
        REDIRECT(0x004A6C55, address);
        REDIRECT(0x004A6C72, address);
    }

    {
        FUNC_ADDRESS(address, &als::state_machine::do_explicit_trans);
        REDIRECT(0x004A6C43, address);
    }
}
