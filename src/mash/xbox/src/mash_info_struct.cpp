#include "mash_info_struct.h"

#include "common.h"
#include "variables.h"
#include "trace.h"

#include <utility.h>

#include <cassert>

VALIDATE_SIZE(mash_header, 0x10);
VALIDATE_SIZE(mash_info_struct, 0x28);

mash_info_struct::mash_info_struct(mash::mode_t a2, uint8_t *a3, int size, bool a5)
{
    this->state = (decltype(this->state)) 0;
    this->mode = a2;
    this->field_C = 0;
    assert(mode == mash::UNMASH_MODE);

    this->mash_image_ptr[mash::NORMAL_BUFFER] = a3;
    this->buffer_size_used[mash::NORMAL_BUFFER] = 0;

#ifdef OPENUSM_XBPACK_V10
    this->header = nullptr;
    this->m_index = size;
    this->mash_image_ptr[mash::SHARED_BUFFER] = &a3[size];
    this->buffer_size_used[mash::SHARED_BUFFER] = 0;
    this->m_size = 0;
#else
    this->header = CAST(this->header, a5 ? a3 : nullptr);
    this->m_index = (a5 ? this->header->field_8 : size);
    this->mash_image_ptr[mash::SHARED_BUFFER] = &a3[this->m_index];
    this->buffer_size_used[mash::SHARED_BUFFER] = 0;
    this->m_size = size - this->m_index;
    if ( a5 )
    {
        this->advance_buffer(mash::NORMAL_BUFFER, 16);
    }
#endif
}

void mash_info_struct::advance_buffer(mash::buffer_type buffer, int how_many_bytes)
{
    assert(mode != mash::MASH_MODE || state == INITIALIZED);

    assert(mash_image_ptr[buffer] != nullptr);

    assert(how_many_bytes >= 0);
    this->buffer_size_used[buffer] += how_many_bytes;
}

int mash_info_struct::get_header_size()
{
    int result = 0;
    if ( this->header != nullptr )
        result = 16;

    return result;
}

uint8_t *mash_info_struct::read_from_buffer(mash::buffer_type buffer, int a2, int align)
{
    TRACE("mash_info_struct::read_from_buffer");

    if (align != 0)
    {
        this->align_buffer(buffer, align);
    }
    else
    {
        this->deductive_align_buffer(buffer);
    }

    int v4 = this->buffer_size_used[buffer];
    auto result = &this->mash_image_ptr[buffer][v4];
    this->buffer_size_used[buffer] += a2;

    sp_log("0x%08X", result);
    return result;
}

void mash_info_struct::align_buffer(mash::buffer_type buffer, int a2)
{
    assert(mode != mash::MASH_MODE || state == INITIALIZED);

    auto v1 = (this->mode == mash::MASH_MODE || this->mode == mash::ALIGN_MODE);

    auto *buffer_ptr = this->mash_image_ptr[buffer];
    assert(buffer_ptr != nullptr);

    auto *v2 = &buffer_ptr[this->buffer_size_used[buffer]];
    auto *v3 = bit_cast<uint8_t *>(~(a2 - 1) & ((uint32_t) &v2[a2 - 1]));
    if (v3 != v2)
    {
        if (v1) {
            std::memset(v2, 0xA1u, v3 - v2);
        }

        this->buffer_size_used[buffer] += v3 - v2;
    }
}

void mash_info_struct::deductive_align_buffer(mash::buffer_type buffer) {
    assert(mode != mash::MASH_MODE || state == INITIALIZED);
    assert(mash_image_ptr[buffer] != nullptr);

    auto *i = &this->mash_image_ptr[buffer][this->buffer_size_used[buffer]];

    for (; *i == 0xA1; ++i) {
        ;
    }

    auto new_used = i - this->mash_image_ptr[0];
    if ((new_used % 4) != 0)
    {
        new_used = 4 * (new_used / 4);
        assert(new_used >= buffer_size_used[buffer]);
    }

    this->buffer_size_used[buffer] = new_used;
}

#if 0
template<>
void unmash_class(mash_info_struct *a1, als::animation_logic_system_shared *&a2, void *a3)
{
    TRACE("unmash_class<als::animation_logic_system_shared>");
    using T = als::animation_logic_system_shared;
    auto *res = bit_cast<T *>(a1->read_from_buffer(sizeof(T), 4));
    a2 = res;
    res->unmash(a1, a3);
}

template<>
void unmash_class(mash_info_struct *a1, token_def_list *&a2, void *a3)
{
    using T = token_def_list;
    auto *res = bit_cast<T *>(a1->read_from_buffer(sizeof(T), 4));
    a2 = res;
    res->unmash(a1, a3);
}

template<>
void unmash_class(mash_info_struct *a1, mAvlTree<string_hash_entry> *&a2, void *a3)
{
    using T = mAvlTree<string_hash_entry>;
    auto *res = bit_cast<T *>(a1->read_from_buffer(sizeof(T), 4));
    a2 = res;
    res->unmash(a1, a3);
}

template<>
void unmash_class(mash_info_struct *a1, ghetto_mash_file_header *&a2, void *a3)
{
    using T = ghetto_mash_file_header;
    auto *res = bit_cast<T *>(a1->read_from_buffer(sizeof(T), 4));
    a2 = res;
    res->unmash(a1, a3);
}

template<>
void unmash_class(mash_info_struct *a1, path_graph *&a2, void *a3)
{
    auto *res = bit_cast<path_graph *>(a1->read_from_buffer(sizeof(path_graph), 4));
    a2 = res;
    res->unmash(a1, a3);
}

template<>
void unmash_class(mash_info_struct *a1, gab_database *&a2, void *a3)
{
    auto *res = bit_cast<gab_database*>(a1->read_from_buffer(sizeof(gab_database), 4));
    a2 = res;
    res->unmash(a1, a3);
}

template<>
void unmash_class(mash_info_struct *a1, cut_scene *&a2, void *a3)
{
    auto *res = bit_cast<cut_scene *> (a1->read_from_buffer(sizeof(cut_scene), 4));
    a2 = res;
    res->unmash(a1, a3);
}

template<>
void unmash_class(mash_info_struct *a1, ai::core_ai_resource *&a2, void *a3)
{
    auto *res = bit_cast<ai::core_ai_resource *>(a1->read_from_buffer(sizeof(ai::core_ai_resource), 4));
    a2 = res;
    res->unmash(a1, a3);
}
#endif

void mash_info_struct_patch() {}
