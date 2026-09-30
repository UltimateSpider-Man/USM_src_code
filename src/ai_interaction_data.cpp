#include "ai_interaction_data.h"

#include "anim_record.h"
#include "attach_interact_data.h"
#include "binary_search_array_cmp.h"
#include "config.h"
#include "func_wrapper.h"
#include "common.h"
#include "nal_system.h"
#include "resource_manager.h"
#include "trace.h"

#include <cstdint>

VALIDATE_SIZE(ai_interaction_data, 0xA8);

ai_interaction_data::ai_interaction_data() {}

anim_record *ai_interaction_data::does_anim_exist(enum_anim_key::key_enum a2, bool a3) {
    TRACE("ai_interaction_data::does_anim_exist");

    if constexpr (1) {
        enum_anim_key v8 {a2};

        return this->does_anim_exist(&v8, a3);
    } else {
        return (anim_record *) THISCALL(0x0069D600, this, a2, a3);
    }
}

void ai_interaction_data::unregister_interactor(vhandle_type<actor> a2) {
    THISCALL(0x0069AA50, this, a2);
}

anim_record *ai_interaction_data::does_anim_exist(const anim_key *a2, [[maybe_unused]] bool a3) {
    TRACE("ai_interaction_data::does_anim_exist");

    auto *a1 = const_cast<anim_key *>(a2);

    int v2 = -1;

    if (binary_search_array_cmp<anim_key *, anim_record *>(&a1,
                                                    this->field_1C.m_data,
                                                    0,
                                                    this->field_1C.size(),
                                                    &v2,
                                                    anim_key::compare)) {
        auto *result = this->field_1C.at(static_cast<uint16_t>(v2));
        return result;
    }

    return nullptr;
}

void ai_interaction_data::destruct_mashed_class()
{
    THISCALL(0x006B5400, this);
}

void ai_interaction_data::register_interactor(vhandle_type<actor> a3) {
    THISCALL(0x0069F2F0, this, a3);
}

void ai_interaction_data::unmash(mash_info_struct *a1, void *)
{
    a1->unmash_class_in_place(this->field_1C, this);

    a1->unmash_class_in_place(this->field_44, this);
    a1->unmash_class_in_place(this->field_48, this);
    a1->unmash_class_in_place(this->field_4C, this);
    a1->unmash_class_in_place(this->field_50, this);

    a1->unmash_class_in_place(this->field_54, this);

    a1->unmash_class_in_place(this->field_6C, this);

    a1->unmash_class_in_place(this->field_80, this);

    a1->unmash_class_in_place(this->field_94, this);

#if OPENUSM_XBOX_MASH_FORMAT && !defined(OPENUSM_XBPACK_V10)
    {
        uint8_t class_mashed = -1;
        class_mashed = *a1->read_from_buffer(mash::SHARED_BUFFER, 1, 1);
        assert(class_mashed == 0xAF || class_mashed == 0);
    }
#endif

    if ( this->field_68 != nullptr )
    {
        a1->unmash_class(this->field_68, this
#if OPENUSM_XBOX_MASH_FORMAT
                , mash::NORMAL_BUFFER
#endif
                );
    }
}

string_hash ai_interaction_data::get_anim_hash_name(
        const anim_record *a2,
        bool a3)
{
    TRACE("ai_interaction_data::get_anim_hash_name");

    string_hash result;
    THISCALL(0x0068DDF0, this, &result, a2, a3);

    return result;
}

void *ai_interaction_data::get_anim_ptr(
        const anim_key *the_anim_key,
        bool a3) {
    TRACE("ai_interaction_data::get_anim_ptr");

    assert(the_anim_key != nullptr);

    if constexpr (1) {
        auto *v8 = this->does_anim_exist(the_anim_key, a3);
        if (v8 == nullptr) {
            return nullptr;
        }

        resource_manager::push_resource_context(this->field_7C);
        auto anim_hash_name = this->get_anim_hash_name(v8, a3);
        auto v4 = anim_hash_name.source_hash_code;

        struct {
            char field_0[0x8];
            void *(__fastcall *field_8)(void *, int, unsigned int);
        } * vtbl = CAST(vtbl, nalGetAnimDirectory()->m_vtbl);
        auto *v7 = vtbl->field_8(
                         nalGetAnimDirectory(),
                         0,
                         v4);
        resource_manager::pop_resource_context();
        return v7;
        
    } else {
        return (void *) THISCALL(0x0069D6A0, this, the_anim_key, a3);
    }
}

void ai_interaction_data_patch() {

    {
        FUNC_ADDRESS(address, &ai_interaction_data::get_anim_ptr);
        SET_JUMP(0x0069D6A0, address);
    }
}
