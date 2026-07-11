#include "base_state_machine.h"

#include "als_state.h"
#include "func_wrapper.h"
#include "layer_state_machine_shared.h"
#include "state_machine_shared.h"
#include "trace.h"

#include <algorithm>

namespace als {

base_state_machine::base_state_machine()
{

}

void base_state_machine::init(state_machine_shared *shared_machine)
{
    TRACE("als::base_state_machine::init");
    
    if constexpr (1) {
        assert(shared_portion == nullptr);
        assert(shared_machine != nullptr);

        this->shared_portion = (layer_state_machine_shared *)shared_machine;
        assert(!shared_portion->state_list.empty());

        auto &state_list = this->shared_portion->state_list;
        sp_log("%d", state_list.size());
        std::for_each(state_list.begin(), state_list.end(), [this](state *the_state)
        {
            printf("%s %s\n", the_state->get_state_id().to_string(),
                    the_state->get_category_id().to_string());
            if ( the_state->is_flag_set(static_cast<state_flags>(0x10)) ) {
                this->m_curr_state = the_state;
            }
        });

        assert(m_curr_state != nullptr);

        this->field_14.m_active = true;
        this->field_14.m_curr_state_interruptable = (!this->m_curr_state->is_flag_set(static_cast<state_flags>(1u)));
    } else {
        THISCALL(0x004995D0, this, shared_machine);
    }
}

bool base_state_machine::is_curr_state_biped_physics() const
{
    return this->m_curr_state != nullptr
            && this->m_curr_state->is_flag_set(static_cast<state_flags>(0x800));
}

} // namespace als
