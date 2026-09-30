#include "gab_manager.h"

#include "common.h"
#include "func_wrapper.h"
#include "mash_config.h"
#include "mash_info_struct.h"

#include <cstddef>
#include <cstdint>

VALIDATE_SIZE(gab_database, 0x14);

Var<_std::list<gab_history_entry> *> s_gab_history{0x00968514};

Var<gab_database *> s_gab_database{0x00968510};

#if OPENUSM_XBOX_MASH_FORMAT
namespace {

struct gab_value {
    std::uint32_t field_0;
    std::uint32_t field_4;
};

struct gab_value_vector {
    int field_0;
    int m_size;
    gab_value *m_data;
    int field_C;
};

struct gab_expression {
    std::uint32_t unused_string_hash[2];
    gab_value_vector values;
};

struct gab_expression_vector {
    int field_0;
    int m_size;
    gab_expression **m_data;
    int field_C;
    bool field_10;
    std::uint8_t padding[3];
};

struct gab_archetype {
    std::uint32_t field_0;
    gab_expression_vector expressions;
};

struct gab_database_layout {
    int field_0;
    int m_size;
    gab_archetype **m_data;
    int field_C;
    bool field_10;
    std::uint8_t padding[3];
};

static_assert(sizeof(gab_value) == 0x8);
static_assert(sizeof(gab_value_vector) == 0x10);
static_assert(sizeof(gab_expression) == 0x18);
static_assert(sizeof(gab_expression_vector) == 0x14);
static_assert(sizeof(gab_archetype) == 0x18);
static_assert(sizeof(gab_database_layout) == sizeof(gab_database));

int read_vector_size(mash_info_struct *info)
{
    return *reinterpret_cast<int *>(
        info->read_from_buffer(mash::SHARED_BUFFER, sizeof(int), alignof(int)));
}

template<typename T>
T *read_object(mash_info_struct *info)
{
    return reinterpret_cast<T *>(
        info->read_from_buffer(mash::NORMAL_BUFFER, sizeof(T), alignof(T)));
}

template<typename T>
T **read_pointer_table(mash_info_struct *info, int size)
{
    return reinterpret_cast<T **>(info->read_from_buffer(
        mash::NORMAL_BUFFER, sizeof(T *) * size, alignof(T *)));
}

void finish_container(int &field_0, const void *container, mash_info_struct *info)
{
    const auto normal_end = reinterpret_cast<std::uintptr_t>(
        info->mash_image_ptr[mash::NORMAL_BUFFER] +
        info->buffer_size_used[mash::NORMAL_BUFFER]);
    field_0 = static_cast<int>(
        normal_end - reinterpret_cast<std::uintptr_t>(container));
}

void unmash_values(gab_value_vector &values, mash_info_struct *info)
{
    values.m_size = read_vector_size(info);
    values.field_C = values.m_size;

    if (values.m_size > 0) {
        values.m_data = reinterpret_cast<gab_value *>(info->read_from_buffer(
            mash::NORMAL_BUFFER,
            sizeof(gab_value) * values.m_size,
            alignof(gab_value)));
    } else {
        values.m_data = nullptr;
    }

    finish_container(values.field_0, &values, info);
}

void unmash_expression(gab_expression &expression, mash_info_struct *info)
{
    unmash_values(expression.values, info);
}

void unmash_expressions(gab_expression_vector &expressions,
                             mash_info_struct *info)
{
    expressions.m_size = read_vector_size(info);
    expressions.field_C = expressions.m_size;

    if (expressions.m_size > 0) {
        expressions.m_data =
            read_pointer_table<gab_expression>(info, expressions.m_size);

        for (int i = 0; i < expressions.m_size; ++i) {
            expressions.m_data[i] = read_object<gab_expression>(info);
            unmash_expression(*expressions.m_data[i], info);
        }
    } else {
        expressions.m_data = nullptr;
    }

    finish_container(expressions.field_0, &expressions, info);
}

void unmash_database(gab_database *database, mash_info_struct *info)
{
    auto &layout = *reinterpret_cast<gab_database_layout *>(database);
    layout.m_size = read_vector_size(info);
    layout.field_C = layout.m_size;

    if (layout.m_size > 0) {
        layout.m_data = read_pointer_table<gab_archetype>(info, layout.m_size);

        for (int i = 0; i < layout.m_size; ++i) {
            layout.m_data[i] = read_object<gab_archetype>(info);
            unmash_expressions(layout.m_data[i]->expressions, info);
        }
    } else {
        layout.m_data = nullptr;
    }

    finish_container(layout.field_0, &layout, info);
}

} // namespace
#endif

gab_database::gab_database(from_mash_in_place_constructor *a2) {
    THISCALL(0x005E0E80, this, a2);
}

void gab_database::destruct_mashed_class() {
    THISCALL(0x005E0860, this);
}

void gab_database::unmash(mash_info_struct *a1, void *a3) {
#if OPENUSM_XBOX_MASH_FORMAT
    (void)a3;
    unmash_database(this, a1);
#else
    THISCALL(0x005DF4A0, this, a1, a3);
#endif
}

void gab_manager::create_inst() {
    CDECL_CALL(0x005D7E20);
}

void gab_manager::delete_inst() {
    delete s_gab_history();
    s_gab_history() = nullptr;
}

void gab_manager::frame_advance(Float a1) {
    CDECL_CALL(0x005D1DA0, a1);
}

void gab_manager::set_gab_database(gab_database *a1) {
    s_gab_database() = a1;
}

gab_database *gab_manager::get_gab_database() {
    return s_gab_database();
}
