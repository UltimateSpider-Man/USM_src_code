#include "mashable_vector.h"

#include "anim_info.h"
#include "anim_map_ptr_entry.h"
#include "common.h"
#include "district_graph_container.h"
#include "entity_base_vhandle.h"
#include "fixedstring.h"
#include "func_wrapper.h"
#include "fx_cache.h"
#include "log.h"
#include "oldmath_po.h"
#include "osassert.h"
#include "parse_generic_mash.h"
#include "resource_allocation_pool.h"
#include "resource_location.h"
#include "resource_pack_group.h"
#include "sin_district_container.h"
#include "sin_strip_container.h"
#include "tlresource_location.h"
#include "utility.h"
#include "variables.h"

VALIDATE_SIZE(mashable_vector<int>, 8u);

namespace
{
template<typename T>
void check_mash(mashable_vector<T> *vector)
{
#ifdef OPENUSM_XBPACK_MODE
    if (g_platform == NL_PLATFORM_XBOX && !vector->from_mash()) {
        assert(vector->m_data == nullptr);
        assert(vector->m_size == 0);
        assert(!vector->m_shared);
        return;
    }
#endif

    //assert(vector->from_mash());
}
}


//0x004C7F50
template<>
void mashable_vector<resource_directory *>::custom_un_mash([[maybe_unused]] generic_mash_header *a2,
                                                           [[maybe_unused]] void *a3,
                                                           generic_mash_data_ptrs *a4,
                                                           [[maybe_unused]] void *a5)
{
    TRACE("mashable_vector<resource_directory *>::custom_un_mash");

    if (this->m_shared) {
        auto &buffer = a4->field_4;

        rebase(buffer, 4u);

        rebase(buffer, 4u);

        this->m_data = a4->get_from_shared<resource_directory *>(this->m_size);

        rebase(buffer, 4u);
    } else {
        auto &buffer = a4->field_0;

        rebase(buffer, 4u);

        rebase(buffer, 4u);

        this->m_data = a4->get<resource_directory *>(this->m_size);

        rebase(buffer, 4u);
    }
}

//0x004C72E0
template<>
void mashable_vector<dsg_region_container>::custom_un_mash(generic_mash_header *header,
                                                           [[maybe_unused]] void *a3,
                                                           generic_mash_data_ptrs *a4,
                                                           [[maybe_unused]] void *a5)
{
    if (this->is_shared()) {
        error("dsg_region_container's cannot be shared!");
    }

    rebase(a4->field_0, 4);

    rebase(a4->field_0, 4);

    this->m_data = a4->get<dsg_region_container>(this->m_size);
    
    for (auto i = 0u; i < this->m_size; ++i)
    {
        assert(((int)header) % 4 == 0);

        this->m_data[i].un_mash(header, &this->m_data[i], a4);
    }

    rebase(a4->field_0, 4);
}

//0x004C7380
template<>
void mashable_vector<dsg_box_container>::custom_un_mash(generic_mash_header *a2,
                                                        [[maybe_unused]] void *a3,
                                                        generic_mash_data_ptrs *a4,
                                                        [[maybe_unused]] void *a5)
{
    if (this->is_shared()) {
        error("dsg_box_container's cannot be shared!");
    }

    rebase(a4->field_0, 4);

    rebase(a4->field_0, 4);

    this->m_data = a4->get<dsg_box_container>(this->m_size);

    for (auto i = 0; i < this->m_size; ++i) {
        this->m_data[i].un_mash(a2, &this->m_data[i], a4);
    }

    rebase(a4->field_0, 4);
}

//0x004C7DC0
template<>
void mashable_vector<resource_location>::custom_un_mash(generic_mash_header *header,
                                                        [[maybe_unused]] void *a3,
                                                        generic_mash_data_ptrs *a4,
                                                        [[maybe_unused]] void *a5)
{
    check_mash(this);

    if (this->m_shared)
    {
        rebase(a4->field_4, 8u);

        auto v24 = *a4->get_from_shared<uint32_t>();

        auto v11 = *a4->get_from_shared<uint32_t>();
        a4->field_4 += 4;

        auto *v9 = a4->get_from_shared<uint32_t>();

        rebase(a4->field_4, 8u);

        rebase(a4->field_4, 4u);

        this->m_data = a4->get_from_shared<value_t>(this->m_size);

        if (v9[0] != 0)
        {
            a4->field_0 += v24;
            a4->field_4 += v11 - sizeof(value_t) * this->m_size;
        }
        else
        {
            for (auto i = 0u; i < this->m_size; ++i) {
                assert(((int)header) % 4 == 0);
                this->m_data[i].un_mash(header, &this->m_data[i], a4);
            }
        }

        ++(v9[0]);

        rebase(a4->field_4, 4u);

    } else {

        rebase(a4->field_0, 8u);

        rebase(a4->field_0, 4u);

        this->m_data = a4->get<value_t>(this->m_size);

        for (auto i = 0u; i < this->m_size; ++i) {
            assert(((int)header) % 4 == 0);
            this->m_data[i].un_mash(header, &this->m_data[i], a4);
        }

        rebase(a4->field_0, 4u);
    }
}

//0x004C8020
template<>
void mashable_vector<tlresource_location>::custom_un_mash(generic_mash_header *header,
                                                          [[maybe_unused]] void *a3,
                                                          generic_mash_data_ptrs *a4,
                                                          [[maybe_unused]] void *a5)
{
    check_mash(this);

    if (this->is_shared())
    {
        rebase(a4->field_4, 8u);

        auto offset = *a4->get_from_shared<uint32_t>();
        auto offset1 = *a4->get_from_shared<uint32_t>();

        a4->field_4 += 4;

        auto *v9 = a4->get_from_shared<int>();

        rebase(a4->field_4, 8u);

        rebase(a4->field_4, 4u);

        this->m_data = a4->get_from_shared<value_t>(this->m_size);

        if (v9[0] != 0)
        {
            a4->field_0 += offset;
            a4->field_4 += offset1 - sizeof(value_t) * this->m_size;
        }
        else
        {
            for (int i = 0; i < this->m_size; ++i) {
                assert(((int) header) % 4 == 0);
                this->m_data[i].un_mash(header, &this->m_data[i], a4);
            }
        }

        ++v9[0];

        rebase(a4->field_4, 4u);

    } else {

        rebase(a4->field_0, 8u);

        rebase(a4->field_0, 4u);

        this->m_data = a4->get<value_t>(this->m_size);

        for (int i = 0; i < this->m_size; ++i)
        {
            assert(((int) header) % 4 == 0);
            this->m_data[i].un_mash(header, &this->m_data[i], a4);
        }

        rebase(a4->field_0, 4u);
    }
}

//0x004C81B0
template<>
void mashable_vector<resource_pack_group>::custom_un_mash(generic_mash_header *header,
                                                          [[maybe_unused]] void *a3,
                                                          generic_mash_data_ptrs *a4,
                                                          [[maybe_unused]] void *a5) {
    check_mash(this);

    if (this->is_shared()) {

        rebase(a4->field_4, 8u);

#if 0
        auto offset = *bit_cast<uint32_t *>(a4->field_4);
        a4->field_4 += 4;

        auto offset1 = *bit_cast<uint32_t *>(a4->field_4);
        a4->field_4 += 4;
        a4->field_4 += 4;

        auto *v9 = bit_cast<int *>(a4->field_4);
        a4->field_4 += 4;

#else
        struct {
            uint32_t offset;
            uint32_t offset1;
            uint32_t empty;
            uint32_t field_C;
        } *ptr = CAST(ptr, a4->field_4);

        auto offset = ptr->offset;
        auto offset1 = ptr->offset1;
        auto *v9 = &ptr->field_C;

        a4->field_4 += sizeof(*ptr);
#endif

        rebase(a4->field_4, 8u);

        rebase(a4->field_4, 4u);

        this->m_data = a4->get_from_shared<value_t>(this->m_size);

        if (v9[0]) {
            a4->field_0 += offset;
            a4->field_4 += offset1 - sizeof(value_t) * this->m_size;
        } else if (this->m_size) {
            for (int i = 0; i < this->m_size; ++i) {
                assert(((int) header) % 4 == 0);
                this->m_data[i].un_mash(header, &this->m_data[i], a4);
            }
        }

        ++v9[0];

        rebase(a4->field_4, 4u);

    } else {

        rebase(a4->field_0, 8u);

        rebase(a4->field_0, 4u);

        this->m_data = a4->get<value_t>(this->m_size);

        if (this->m_size != 0) {
            for (int i = 0; i < this->m_size; ++i) {
                assert(((int) header) % 4 == 0);
                this->m_data[i].un_mash(header, &this->m_data[i], a4);
            }
        }


        rebase(a4->field_0, 4u);
    }
}

//0x004C8740
template<>
void mashable_vector<resource_allocation_pool>::custom_un_mash(generic_mash_header *header,
                                                               [[maybe_unused]] void *a3,
                                                               generic_mash_data_ptrs *a4,
                                                               [[maybe_unused]] void *a5) {
    if (this->is_shared())
    {
        rebase(a4->field_4, 4u);

        auto offset = *a4->get_from_shared<uint32_t>();

        auto offset1 = *a4->get_from_shared<uint32_t>();

        auto *v9 = a4->get_from_shared<int>();

        rebase(a4->field_4, 4u);

        rebase(a4->field_4, 4u);

        this->m_data = a4->get_from_shared<value_t>(this->m_size);

        if (v9[0]) {
            a4->field_0 += offset;
            a4->field_4 += offset1 - sizeof(value_t) * this->m_size;
        }
        else
        {
            for (int i = 0; i < this->m_size; ++i) {
                this->m_data[i].un_mash(header, &this->m_data[i], a4);
            }
        }

        ++v9[0];

        rebase(a4->field_4, 4u);

    } else {

        rebase(a4->field_0, 4u);

        rebase(a4->field_0, 4u);

        this->m_data = a4->get<value_t>(this->m_size);

        for (int i = 0; i < this->m_size; ++i) {
            this->m_data[i].un_mash(header, &this->m_data[i], a4);
        }

        rebase(a4->field_0, 4u);
    }
}

//0x
template<>
void mashable_vector<fixedstring<4>>::custom_un_mash([[maybe_unused]] generic_mash_header *a2,
                                                     [[maybe_unused]] void *a3,
                                                     generic_mash_data_ptrs *a4,
                                                     [[maybe_unused]] void *a5) {
    check_mash(this);

    if (this->is_shared()) {

        rebase(a4->field_4, 8u);

        rebase(a4->field_4, 4u);

        this->m_data = a4->get_from_shared<value_t>(this->m_size);

        rebase(a4->field_4, 4u);

    } else {

        rebase(a4->field_0, 8u);

        rebase(a4->field_0, 4u);

        this->m_data = a4->get<value_t>(this->m_size);

        rebase(a4->field_0, 4u);
    }
}

//0x004C5F20
template<>
void mashable_vector<int>::custom_un_mash([[maybe_unused]] generic_mash_header *a2,
                                          [[maybe_unused]] void *a3,
                                          generic_mash_data_ptrs *a4,
                                          [[maybe_unused]] void *a5) {
    if (this->m_shared) {

        rebase(a4->field_4, 4u);

        rebase(a4->field_4, 4u);

        this->m_data = a4->get_from_shared<value_t>(this->m_size);

        rebase(a4->field_4, 4u);

    } else {

        rebase(a4->field_0, 4u);

        rebase(a4->field_0, 4u);

        this->m_data = a4->get<value_t>(this->m_size);

        rebase(a4->field_0, 4u);
    }
}

template<>
void mashable_vector<po>::custom_un_mash(generic_mash_header *, void *, generic_mash_data_ptrs *a4, void *)
{
    TRACE("mashable_vector<po>::custom_un_mash");

    if ( this->m_shared )
    {
        rebase(a4->field_4, 16);

        rebase(a4->field_4, 4);

        this->m_data = a4->get_from_shared<po>(this->m_size);

        rebase(a4->field_4, 4);
    }
    else
    {
        rebase(a4->field_0, 16);

        rebase(a4->field_0, 4);
        
        this->m_data = a4->get<po>(this->m_size);

        rebase(a4->field_0, 4);
    }
}

template<>
void mashable_vector<entity_base *>::custom_un_mash(generic_mash_header *, void *, generic_mash_data_ptrs *a4, void *)
{
    TRACE("mashable_vector<entity_base *>::custom_un_mash");

    if ( this->m_shared)
    {
        rebase(a4->field_4, 4);

        rebase(a4->field_4, 4);

        this->m_data = a4->get_from_shared<entity_base *>(this->m_size);

        rebase(a4->field_4, 4);
    }
    else
    {
        rebase(a4->field_0, 4);

        rebase(a4->field_0, 4);
        
        this->m_data = a4->get<entity_base *>(this->m_size);

        rebase(a4->field_0, 4);
    }
}

template<>
void mashable_vector<int8_t>::custom_un_mash(generic_mash_header *, void *, generic_mash_data_ptrs *a4, void *)
{
    if ( this->m_shared )
    {
        rebase(a4->field_4, 4);

        rebase(a4->field_4, 4);

        this->m_data = a4->get_from_shared<int8_t>(this->m_size);

        rebase(a4->field_4, 4);
    }
    else
    {
        rebase(a4->field_0, 4);

        rebase(a4->field_0, 4);
        
        this->m_data = a4->get<int8_t>(this->m_size);

        rebase(a4->field_0, 4);
    }
}

template<>
void mashable_vector<anim_info>::custom_un_mash(generic_mash_header *a2, void *, generic_mash_data_ptrs *a4, void *)
{
    if ( this->m_shared )
    {
        rebase(a4->field_4, 8);

        auto v8 = (int)a4->field_4;
        auto v9 = (int *)(v8 + 4);
        auto offset = *(DWORD *)v8;
        auto v10 = (uint8_t *)(v8 + 16);
        a4->field_4 = (uint8_t *)(v8 + 4);
        auto offset1 = *(DWORD *)(v8 + 4);
        auto v12 = 8 - (((BYTE)v8 + 16) & 7);
        a4->field_4 = v10;
        if ( v12 < 8 )
        {
            a4->field_4 = &v10[v12];
        }

        rebase(a4->field_4, 4);

        this->m_data = a4->get_from_shared<anim_info>(this->m_size);

        if ( v9[2] )
        {
            a4->field_0 += offset;
            a4->field_4 += offset1 - sizeof(value_t) * this->m_size;
        }
        else
        {
            for ( auto i {0u}; i < this->m_size; ++i )
            {
                this->m_data[i].un_mash(a2, &this->m_data[i], a4);
            }
        }

        ++v9[2];

        rebase(a4->field_4, 4);
    }
    else
    {
        rebase(a4->field_0, 8);

        rebase(a4->field_0, 4);

        this->m_data = a4->get<anim_info>(this->m_size);

        for ( auto i {0u}; i < this->m_size; ++i ) {
            this->m_data[i].un_mash(a2, &this->m_data[i], a4);
        }

        rebase(a4->field_0, 4);
    }
}

template<>
void mashable_vector<anim_map_ptr_entry>::custom_un_mash(generic_mash_header *header, void *, generic_mash_data_ptrs *a4, void *)
{
    if ( this->is_shared() )
    {
        rebase(a4->field_4, 8);

        auto v8 = (int)a4->field_4;
        auto offset = *(DWORD *)v8;
        auto v9 = (int *)(v8 + 4);
        auto v10 = (uint8_t *)(v8 + 16);
        a4->field_4 = (uint8_t *)(v8 + 4);
        auto offset1 = *(DWORD *)(v8 + 4);
        auto v12 = 8 - (((BYTE)v8 + 16) & 7);
        a4->field_4 = v10;
        if ( v12 < 8 )
        {
            a4->field_4 = &v10[v12];
        }

        rebase(a4->field_4, 4);

        this->m_data = a4->get_from_shared<anim_map_ptr_entry>(this->m_size);

        if ( v9[2] )
        {
            a4->field_0 += offset;
            a4->field_4 += offset1 - sizeof(value_t) * this->m_size;
        }
        else
        {
            for ( auto i = 0; i < this->m_size; ++i )
            {
                assert(((int) header) % 4 == 0);
                this->m_data[i].field_8.custom_un_mash(header, &this->m_data[i].field_8, a4, nullptr);
            }
        }

        ++v9[2];

        rebase(a4->field_4, 4);
    }
    else
    {
        rebase(a4->field_0, 8);

        rebase(a4->field_0, 4);

        this->m_data = a4->get<anim_map_ptr_entry>(this->m_size);

        for ( auto i = 0; i < this->m_size; ++i )
        {
            assert(((int) header) % 4 == 0);
            this->m_data[i].field_8.custom_un_mash(header, &this->m_data[i].field_8, a4, nullptr);
        }

        rebase(a4->field_0, 4);
    }
}

//0x004CE640
template<>
void mashable_vector<fx_cache_ent>::custom_un_mash(
        generic_mash_header *header,
        void *a3,
        generic_mash_data_ptrs *a4,
        void *a5)
{
    TRACE("mashable_vector<fx_cache_ent>::custom_un_mash");

    if constexpr (1)
    {
        if ( this->is_shared() )
        {
            rebase(a4->field_4, 8u);

            auto offset = *a4->get_from_shared<int>();
            auto offset1 = *a4->get_from_shared<int>();

            a4->field_4 += 4;

            auto *v9 = a4->get_from_shared<int>();

            rebase(a4->field_4, 8u);

            rebase(a4->field_4, 4u);

            this->m_data = a4->get_from_shared<fx_cache_ent>(this->m_size);

            if ( v9[0] != 0 )
            {
                a4->field_0 += offset;
                a4->field_4 += offset1 - sizeof(value_t) * this->m_size;
            }
            else
            {
                for ( int i = 0; i < this->m_size; ++i )
                {
                    assert(((int) header) % 4 == 0);
                    auto *ent = &this->m_data[i];
                    ent->un_mash(header, static_cast<cached_special_effect *>(a5), ent, a4);
                }
            }

            ++v9[0];

            rebase(a4->field_4, 4u);
        }
    }
    else
    {
        THISCALL(0x004CE640, this, header,
                        a3,
                        a4,
                        a5);
    }
}

template<>
void mashable_vector<sin_district_container>::custom_un_mash(
        generic_mash_header *header,
        void *,
        generic_mash_data_ptrs *a4,
        void *)
{
    if ( this->is_shared() ) {
        error("sin_district_container's cannot be shared!");
    }

    {
        rebase(a4->field_0, 4u);

        rebase(a4->field_0, 4u);

        this->m_data = a4->get<sin_district_container>(this->m_size);

        for ( int i = 0; i < this->m_size; ++i )
        {
            assert(((int)header) % 4 == 0);

            this->m_data[i].un_mash(
                header,
                &this->m_data[i],
                a4);
        }

        rebase(a4->field_0, 4u);
    }
}

template<>
void mashable_vector<sin_strip_container>::custom_un_mash(
        generic_mash_header *header,
        void *,
        generic_mash_data_ptrs *a4,
        void *)
{

    if ( this->is_shared() ) {
        error("sin_strip_container's cannot be shared!");
    }

    {
        rebase(a4->field_0, 4u);

        rebase(a4->field_0, 4u);

        this->m_data = a4->get<sin_strip_container>(this->m_size);

        for ( int i = 0; i < this->m_size; ++i )
        {
            assert(((int)header) % 4 == 0);

            this->m_data[i].un_mash(header, &this->m_data[i], a4);
        }

        rebase(a4->field_0, 4u);
    }
}


void mashable_vector_patch()
{
    {
        FUNC_ADDRESS(address, &mashable_vector<resource_directory *>::custom_un_mash);
        REDIRECT(0x0051F70B, address);
    }

    {
        FUNC_ADDRESS(address, &mashable_vector<fx_cache_ent>::custom_un_mash);
        REDIRECT(0x004D36F5, address);
    }

#if 0
    {
        FUNC_ADDRESS(address, &mashable_vector<resource_location>::custom_un_mash);
        REDIRECT(0x0051F718, address);
    }
#endif
}
