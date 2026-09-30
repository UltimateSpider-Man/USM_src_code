#include "parse_generic_mash.h"

#include "common.h"
#include "entity_base.h"
#include "entity_mash.h"
#include "log.h"
#include "memory.h"
#include "ngl.h"
#include "trace.h"
#include "parse_generic_mash.h"
#include "resource_directory.h"
#include "resource_pack_header.h"
#include "mission_table_container.h"
#include "variables.h"
#include "xbpack.h"

#include <cassert>
#include <cstring>

VALIDATE_SIZE(generic_mash_header, 0x10);

#ifdef OPENUSM_XBPACK_V10
namespace
{
constexpr uint16_t V10_CONGLOM_TYPE = 5;
constexpr uint16_t V10_GUN_TYPE = 10;
constexpr uint16_t V10_THROWN_ITEM_TYPE = 12;
constexpr uint16_t SKELETON_IFC_FLAG = 0x40;
constexpr size_t V10_ACTOR_SIZE = 0xBC;
constexpr size_t PC_ACTOR_SIZE = 0xC0;
constexpr size_t V10_CONGLOM_SIZE = 0x12C;
constexpr size_t PC_CONGLOM_SIZE = 0x130;
constexpr size_t V10_STREAM_OFFSET = 0x13C;
constexpr size_t SKELETON_IFC_SIZE = 0x14;
constexpr size_t SKELETON_PADDING = 0x0C;
constexpr size_t V10_SKELETON_END = V10_CONGLOM_SIZE + SKELETON_IFC_SIZE;
constexpr size_t PC_SKELETON_END =
    PC_CONGLOM_SIZE + SKELETON_IFC_SIZE + SKELETON_PADDING;
constexpr size_t V10_GUN_PREFIX_END = 0x124;
constexpr size_t V10_GUN_EFFECTS_BEGIN = 0x1A4;
constexpr size_t V10_GUN_EFFECTS_END = 0x2A4;
constexpr size_t V10_GUN_SIZE = 0x328;
constexpr size_t PC_GUN_PREFIX_END = 0x12C;
constexpr size_t PC_GUN_EXTRA_EFFECT = 0x1AC;
constexpr size_t PC_GUN_EFFECTS_END = 0x2EC;
constexpr size_t PC_GUN_SIZE = 0x374;
constexpr size_t V10_THROWN_ITEM_SIZE = 0x340;
constexpr size_t PC_THROWN_ITEM_SIZE = 0x350;

struct v10_mash_clone
{
    uint8_t *object;
    uint8_t *normal;
};

v10_mash_clone clone_v10_conglom(
    const uint8_t *source,
    size_t size,
    uint16_t flags)
{
    const bool has_skeleton = (flags & SKELETON_IFC_FLAG) != 0;
    const size_t result_size = size +
        (has_skeleton
            ? PC_SKELETON_END - V10_SKELETON_END
            : V10_STREAM_OFFSET - V10_CONGLOM_SIZE);
    assert(size >= (has_skeleton ? V10_SKELETON_END : V10_CONGLOM_SIZE));

    auto *result = static_cast<uint8_t *>(
        arch_memalign(sizeof(generic_mash_header), result_size));
    assert(result != nullptr);

    std::memcpy(result, source, V10_ACTOR_SIZE);
    std::memset(result + V10_ACTOR_SIZE, 0, PC_ACTOR_SIZE - V10_ACTOR_SIZE);
    std::memcpy(
        result + PC_ACTOR_SIZE,
        source + V10_ACTOR_SIZE,
        V10_CONGLOM_SIZE - V10_ACTOR_SIZE);

    if (has_skeleton) {
        std::memcpy(
            result + PC_CONGLOM_SIZE,
            source + V10_CONGLOM_SIZE,
            SKELETON_IFC_SIZE);
        std::memset(
            result + PC_CONGLOM_SIZE + SKELETON_IFC_SIZE,
            0,
            SKELETON_PADDING);
        std::memcpy(
            result + PC_SKELETON_END,
            source + V10_SKELETON_END,
            size - V10_SKELETON_END);
    } else {
        std::memset(
            result + PC_CONGLOM_SIZE,
            0,
            V10_STREAM_OFFSET - PC_CONGLOM_SIZE);
        std::memcpy(
            result + V10_STREAM_OFFSET,
            source + V10_CONGLOM_SIZE,
            size - V10_CONGLOM_SIZE);
    }

    return {
        result,
        result + (has_skeleton ? PC_CONGLOM_SIZE : V10_STREAM_OFFSET)};
}

v10_mash_clone clone_v10_gun(const uint8_t *source, size_t size)
{
    assert(size >= V10_GUN_SIZE);

    const size_t result_size = size + PC_GUN_SIZE - V10_GUN_SIZE;
    auto *result = static_cast<uint8_t *>(
        arch_memalign(sizeof(generic_mash_header), result_size));
    assert(result != nullptr);

    std::memcpy(result, source, V10_ACTOR_SIZE);
    std::memset(result + V10_ACTOR_SIZE, 0, PC_ACTOR_SIZE - V10_ACTOR_SIZE);

    // PC adds effect-state booleans at +0x128/+0x129 (ctor 0x004FD8B0).
    // Insert their DWORD after the beta shot counter at +0x120; otherwise
    // beta +0x124 bytes become live PC flags that native unmash does not reset.
    // Preserve those source bytes at PC +0x12C, before the force at +0x130.
    std::memcpy(result + PC_ACTOR_SIZE,
                source + V10_ACTOR_SIZE,
                V10_GUN_PREFIX_END - V10_ACTOR_SIZE);
    std::memset(result + V10_GUN_PREFIX_END +
                    (PC_ACTOR_SIZE - V10_ACTOR_SIZE),
                0,
                PC_GUN_PREFIX_END - V10_GUN_PREFIX_END -
                    (PC_ACTOR_SIZE - V10_ACTOR_SIZE));

    // PC adds a cache BEFORE the four beta effects (+0x1AC).
    // Keep beta firing/impact caches at their native PC consumers:
    // Xbox +0x1A4/+0x1E4/+0x224/+0x264 -> PC +0x1EC/+0x22C/+0x26C/+0x2AC.
    // Inserting at the end sent firing/impact payloads to the wrong caches.
    // The matching native unmash call is skipped by actor_xbpack_patch.
    std::memcpy(result + PC_GUN_PREFIX_END,
                source + V10_GUN_PREFIX_END,
                V10_GUN_EFFECTS_BEGIN - V10_GUN_PREFIX_END);
    std::memset(result + PC_GUN_EXTRA_EFFECT, 0, 0x40);
    std::memcpy(result + PC_GUN_EXTRA_EFFECT + 0x40,
                source + V10_GUN_EFFECTS_BEGIN,
                V10_GUN_EFFECTS_END - V10_GUN_EFFECTS_BEGIN);

    std::memcpy(result + PC_GUN_EFFECTS_END,
                source + V10_GUN_EFFECTS_END,
                V10_GUN_SIZE - V10_GUN_EFFECTS_END);
    std::memset(result + PC_GUN_SIZE - sizeof(uint32_t), 0, sizeof(uint32_t));
    std::memcpy(result + PC_GUN_SIZE,
                source + V10_GUN_SIZE,
                size - V10_GUN_SIZE);

    return {result, result + PC_GUN_SIZE};
}

v10_mash_clone clone_v10_thrown_item(const uint8_t *source, size_t size)
{
    assert(size >= V10_THROWN_ITEM_SIZE);

    const size_t result_size =
        size + PC_THROWN_ITEM_SIZE - V10_THROWN_ITEM_SIZE;
    auto *result = static_cast<uint8_t *>(
        arch_memalign(sizeof(generic_mash_header), result_size));
    assert(result != nullptr);

    std::memcpy(result, source, V10_THROWN_ITEM_SIZE);
    std::memset(result + V10_THROWN_ITEM_SIZE,
                0,
                PC_THROWN_ITEM_SIZE - V10_THROWN_ITEM_SIZE);
    std::memcpy(result + PC_THROWN_ITEM_SIZE,
                source + V10_THROWN_ITEM_SIZE,
                size - V10_THROWN_ITEM_SIZE);

    return {result, result + PC_THROWN_ITEM_SIZE};
}
}
#endif

void *parse_generic_mash_init(generic_mash_header *&header,
                              void *a2,
                              bool *allocated_mem,
                              generic_mash_data_ptrs *a4,
                              uint32_t struct_size,
                              uint32_t *virtual_table_lookup,
                              uint32_t *size_table_lookup,
                              [[maybe_unused]] uint32_t num_table_entries,
                              [[maybe_unused]] uint32_t base_class_size,
                              void *a10) {
    assert(allocated_mem != nullptr);

    *allocated_mem = false;

    uint8_t *cur_ptr;
#ifdef OPENUSM_XBPACK_V10
    uint8_t *normal_override = nullptr;
#endif

    auto *copy_a2 = static_cast<uint8_t *>(a2);

    //condition is false
    if (a10 != nullptr) {
        cur_ptr = copy_a2;
        header = static_cast<generic_mash_header *>(a10);
#ifdef OPENUSM_XBPACK_V10
        if (header->is_flagged(0x40000000) &&
            header->class_id == V10_CONGLOM_TYPE) {
            const size_t size = header->field_8 - sizeof(generic_mash_header);
            const auto clone = clone_v10_conglom(
                copy_a2, size, header->field_E);
            cur_ptr = clone.object;
            normal_override = clone.normal;
            *allocated_mem = true;
        } else if (header->is_flagged(0x40000000) &&
                   header->class_id == V10_GUN_TYPE) {
            const size_t size = header->field_8 - sizeof(generic_mash_header);
            const auto clone = clone_v10_gun(copy_a2, size);
            cur_ptr = clone.object;
            normal_override = clone.normal;
            *allocated_mem = true;
        } else if (header->is_flagged(0x40000000) &&
                   header->class_id == V10_THROWN_ITEM_TYPE) {
            const size_t size = header->field_8 - sizeof(generic_mash_header);
            const auto clone = clone_v10_thrown_item(copy_a2, size);
            cur_ptr = clone.object;
            normal_override = clone.normal;
            *allocated_mem = true;
        }
    } else if (header->is_flagged(0x40000000) &&
               header->class_id == V10_CONGLOM_TYPE) {
        const size_t size = header->field_8 - sizeof(generic_mash_header);
        const auto clone = clone_v10_conglom(
            copy_a2 + sizeof(generic_mash_header), size, header->field_E);
        cur_ptr = clone.object;
        normal_override = clone.normal;
        *allocated_mem = true;
    } else if (header->is_flagged(0x40000000) &&
               header->class_id == V10_GUN_TYPE) {
        const size_t size = header->field_8 - sizeof(generic_mash_header);
        const auto clone = clone_v10_gun(
            copy_a2 + sizeof(generic_mash_header), size);
        cur_ptr = clone.object;
        normal_override = clone.normal;
        *allocated_mem = true;
    } else if (header->is_flagged(0x40000000) &&
               header->class_id == V10_THROWN_ITEM_TYPE) {
        const size_t size = header->field_8 - sizeof(generic_mash_header);
        const auto clone = clone_v10_thrown_item(
            copy_a2 + sizeof(generic_mash_header), size);
        cur_ptr = clone.object;
        normal_override = clone.normal;
        *allocated_mem = true;
#endif
    } else if (header->is_flagged(_MASH_FLAG_IN_USE)) {
        //condition is false

        size_t v12 = header->field_8 - sizeof(generic_mash_header);
        auto *object_mash_data = static_cast<uint8_t *>(
            arch_memalign(sizeof(generic_mash_header), v12));
        assert(object_mash_data != nullptr && "Out of memory?  Prepare to crash.\"");

        memcpy(object_mash_data, copy_a2 + sizeof(generic_mash_header), v12);
        cur_ptr = object_mash_data;
        *allocated_mem = true;

    } else {
        cur_ptr = copy_a2 + sizeof(generic_mash_header);
        header->field_4 |= _MASH_FLAG_IN_USE;
        *allocated_mem = false;
    }

    sp_log("cur_ptr = 0x%08X", cur_ptr);
    assert(cur_ptr != nullptr);

    auto *addr = cur_ptr;
    uint8_t *v16;

    //condition is false
    if (header->is_flagged(0x40000000))
    {
        const auto mash_class_id = header->class_id;
        auto class_id = mash_class_id;
#ifdef OPENUSM_XBPACK_V10
        const bool is_entity_mash =
            virtual_table_lookup ==
            reinterpret_cast<uint32_t *>(&ent_v_table_lookup()[0]);
#endif
#ifdef OPENUSM_XBPACK_V10
        if (is_entity_mash) {
            class_id = pc_entity_mash_type(class_id);
        }
#endif

        assert(base_class_size >= 4);
        assert(class_id < num_table_entries);
        assert(num_table_entries > 0);
        assert(size_table_lookup != nullptr);
        assert(virtual_table_lookup != nullptr);

        v16 = cur_ptr + size_table_lookup[class_id];
#ifdef OPENUSM_XBPACK_V10
        if (is_entity_mash) {
            v16 = cur_ptr + entity_mash_size(mash_class_id);
        }
        if (normal_override != nullptr) {
            v16 = normal_override;
        }
#endif

        //sp_log("%d %d %d %d", addr[0], addr[1], addr[2], addr[3]);
        assert(addr[0] == MASH_V_TABLE_VAL[0] ||
               addr[0] == ((char *) &virtual_table_lookup[class_id])[0]);

        assert(addr[1] == MASH_V_TABLE_VAL[1] ||
               addr[1] == ((char *) &virtual_table_lookup[class_id])[1]);

        assert(addr[2] == MASH_V_TABLE_VAL[2] ||
               addr[2] == ((char *) &virtual_table_lookup[class_id])[2]);

        assert(addr[3] == MASH_V_TABLE_VAL[3] ||
               addr[3] == ((char *) &virtual_table_lookup[class_id])[3]);

        std::memcpy(addr, &virtual_table_lookup[class_id], 4);

    }
    else
    {
        assert(base_class_size == 0);
        assert(num_table_entries == 0);
        assert(size_table_lookup == nullptr);
        assert(virtual_table_lookup == nullptr);

        v16 = cur_ptr + struct_size;
    }

    a4->field_0 = v16;
    a4->field_4 = header->get_mash_data();

    return addr;
}

template<>
bool parse_generic_object_mash(mission_table_container *&a1,
                            void *a2,
                            [[maybe_unused]] void *a3,
                            unsigned int *a4,
                            unsigned int *a5,
                            uint32_t a6,
                            uint32_t a7,
                            void *a8)
{
    TRACE("parse_generic_object_mash");

    bool allocated_mem = false;
    generic_mash_data_ptrs a4a;

    auto *header = static_cast<generic_mash_header *>(a2);
    auto *v8 = static_cast<mission_table_container *>(parse_generic_mash_init(
                                    header,
                                    a2,
                                    &allocated_mem,
                                    &a4a,
                                    0x48u,
                                    a4,
                                    a5,
                                    static_cast<uint32_t>(a6),
                                    a7,
                                    a8));
    a1 = v8;
    s_current_un_mashing_mission_table_container() = v8;
    v8->un_mash(header, v8, v8, &a4a);
    s_current_un_mashing_mission_table_container() = nullptr;
    return allocated_mem;
}

template<>
bool parse_generic_object_mash(resource_directory *&arg0,
                               void *a1,
                               void *a5,
                               uint32_t *a6,
                               uint32_t *a7,
                               uint32_t a8,
                               uint32_t a9,
                               void *a10) {
    bool allocated_mem = false;

    generic_mash_data_ptrs a4;

    auto *header = static_cast<generic_mash_header *>(a1);
    const uint32_t directory_object_size =
        (g_platform == NL_PLATFORM_XBOX) ? xbpack::directory_size : sizeof(resource_directory);

    auto *dir = static_cast<resource_directory *>(parse_generic_mash_init(
        header, a1, &allocated_mem, &a4, directory_object_size, a6, a7, static_cast<uint32_t>(a8), a9, a10));

    generic_mash_header *v10 = static_cast<generic_mash_header *>(a1);
    arg0 = dir;
    dir->un_mash_start(v10, dir, &a4, a5);

    return allocated_mem;
}

#if 0
template<>
bool parse_generic_object_mash(
    nglMeshFile **arg0, void *a1, void *a5, uint32_t *a6, uint32_t *a7, int a8, int a9, int *a10) {
    bool allocated_mem = false;

    generic_mash_data_ptrs a4;
    auto header = bit_cast<generic_mash_header *>(a1);
    nglMeshFile *v4 = (nglMeshFile *) parse_generic_mash_init(
        header, a1, &allocated_mem, &a4, 328, a6, a7, static_cast<uint32_t>(a8), a9, a10);

    void *v9 = CAST(v9, a5);
    generic_mash_header *v10 = CAST(v10, a1);
    *arg0 = v4;
    v4->un_mash_start(v10, v4, &a4, nullptr);

    return allocated_mem;
}
#endif

//0x00509D70
#if 0
template<>
bool parse_generic_object_mash<entity_base>(
    entity_base **arg0, void *a1, int a5, uint32_t *a6, uint32_t *a7, int a8, int a9, int *a10) {
    bool allocated_mem = false;

    generic_mash_data_ptrs a4;
    auto header = bit_cast<generic_mash_header *>(a1);
    auto *v4 = static_cast<entity_base *>(parse_generic_mash_init(header,
                                                                  a1,
                                                                  &allocated_mem,
                                                                  &a4,
                                                                  sizeof(entity_base),
                                                                  a6,
                                                                  a7,
                                                                  static_cast<uint32_t>(a8),
                                                                  a9,
                                                                  a10));

    void *v9 = CAST(v9, a5);
    generic_mash_header *v10 = CAST(v10, a1);
    *arg0 = v4;
    v4->un_mash_start(v10, v4, &a4, nullptr);

    return allocated_mem;
}
#endif
