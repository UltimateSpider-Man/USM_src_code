#include "als_animation_logic_system_shared.h"

#include "als_meta_anim_table_shared.h"
#include "als_nal_meta_anim.h"
#include "exe_allocator.h"
#include "func_wrapper.h"
#include "layer_state_machine_shared.h"
#include "mash_info_struct.h"
#include "common.h"
#include "mash_virtual_base.h"
#include "mash_config.h"
#include "state_machine_shared.h"
#include "trace.h"

namespace als {

    VALIDATE_SIZE(animation_logic_system_shared, 0x1C);

    animation_logic_system_shared::animation_logic_system_shared()
    {
        THISCALL(0x004AC000, this);
    }

    animation_logic_system_shared::animation_logic_system_shared(
        from_mash_in_place_constructor *a2) : field_0(a2)
    {
        auto *v3 = this->field_14;
        if ( v3 != nullptr ) {
            this->field_14 = (als::state_machine_shared *)mash_virtual_base::construct_class_helper(v3);
        }

        if ( this->field_18 != nullptr ) {
            this->field_18 = new (this->field_18) als_meta_anim_table_shared {nullptr};
        }
    }

    void animation_logic_system_shared::destruct_mashed_class()
    {
        TRACE("animation_logic_system_shared::destruct_mashed_class");
        if constexpr (1)
        {
            this->field_0.destruct_mashed_class();
            auto *v2 = this->field_14;
            if ( v2 != nullptr )
            {
                v2->destruct_mashed_class();
                this->field_14 = nullptr;
            }

            auto *v3 = this->field_18;
            if ( v3 != nullptr )
            {
                exe_allocator<als_nal_meta_anim> {}.deallocate(
                    v3->field_14,
                    v3->field_0.size());
                v3->field_14 = nullptr;

                v3->field_0.destruct_mashed_class();
                this->field_18 = nullptr;
            }
        }
        else
        {
            THISCALL(0x004A6390, this);
        }
    }

    void animation_logic_system_shared::unmash(mash_info_struct *a1, void *a3)
    {
        TRACE("animation_logic_system_shared::unmash");
        if constexpr(1)
        {
            a1->unmash_class_in_place(this->field_0, this);

#if OPENUSM_XBOX_MASH_FORMAT && !defined(OPENUSM_XBPACK_V10)
            {
                uint8_t class_mashed = -1;
                class_mashed = *a1->read_from_buffer(mash::SHARED_BUFFER, 1, 1);
                assert(class_mashed == 0xAF || class_mashed == 0);
            }
#endif

            if ( this->field_14 != nullptr )
            {
                auto *v4 = a1->read_from_buffer(
#if OPENUSM_XBOX_MASH_FORMAT
                    mash::NORMAL_BUFFER,
#endif
                        sizeof(state_machine_shared), 0);
                this->field_14 = (state_machine_shared *) v4;
                mash_virtual_base::fixup_vtable(this->field_14);
                assert(this->field_14->m_vtbl == 0x0087B8F8);

                auto v5 = this->field_14->get_mash_sizeof();
                a1->advance_buffer(
#if OPENUSM_XBOX_MASH_FORMAT
                    mash::NORMAL_BUFFER,
#endif 
                    v5 - sizeof(als::state_machine_shared));
                
                this->field_14->unmash(a1, this);
            }

#if OPENUSM_XBOX_MASH_FORMAT && !defined(OPENUSM_XBPACK_V10)
            {
                uint8_t class_mashed = -1;
                class_mashed = *a1->read_from_buffer(mash::SHARED_BUFFER, 1, 1);
                assert(class_mashed == 0xAF || class_mashed == 0);
            }
#endif

            if ( this->field_18 != nullptr )
            {
                a1->unmash_class(this->field_18, this
#if OPENUSM_XBOX_MASH_FORMAT
                    , mash::NORMAL_BUFFER
#endif
                        );
            }
        }
        else
        {
            THISCALL(0x004ABAF0, this, a1, a3);
        }
    }
}

void animation_logic_system_shared_patch()
{
    FUNC_ADDRESS(address, &als::animation_logic_system_shared::destruct_mashed_class);
    REDIRECT(0x005689A9, address);
}
