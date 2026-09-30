#include "actor_xbpack.h"

#ifdef OPENUSM_XBPACK_MODE

#include "actor.h"
#include "base_ai_data.h"
#include "cached_special_effect.h"
#include "collision_capsule.h"
#include "colmesh.h"
#include "entity.h"
#include "entity_base.h"
#include "func_wrapper.h"
#include "log.h"
#include "mash.h"
#include "memory.h"
#include "parse_generic_mash.h"
#include "resource_key.h"
#include "sound_and_pfx_interface.h"
#include "time_interface.h"
#include "thrown_item.h"
#include "utility.h"
#include "variables.h"

#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <limits>
#include <windows.h>

namespace
{
constexpr uint32_t MASH_SYNC_TEST_VAL5 = 0x5BADF00D;
constexpr uint32_t MASH_SYNC_TEST_VAL6 = 0x6BADF00D;
constexpr uint32_t XB_MASH_MAGIC = 0x6D617368;

constexpr size_t MAX_ACTOR_MASH_SIZE = 1024u * 1024u;
constexpr size_t XB_MASH_HEADER_SIZE = 0x10u;
constexpr uint32_t XB_RESOURCE_KEY_TYPE_COUNT = 71u;
constexpr size_t XB_SOUND_INTERFACE_SIZE = 0x28u;
#ifdef OPENUSM_XBPACK_V10
constexpr size_t XB_V10_SOUND_INTERFACE_SIZE = 0x3Cu;
constexpr size_t XB_V10_PFX_SIZE = 0x8Cu;
constexpr size_t XB_V10_PFX_VECTOR_OFFSET = 0x78u;
constexpr size_t PC_PFX_SIZE = 0x98u;
constexpr size_t PC_PFX_VECTOR_OFFSET = 0x80u;
constexpr size_t PFX_VECTOR_MASH_SIZE = 0x1Cu;
constexpr size_t XB_V10_ACTOR_SIZE = 0xBCu;
constexpr size_t XB_V10_THROWN_ITEM_SIZE = 0x340u;
constexpr uint16_t XB_V10_THROWN_ITEM_TYPE = 12u;
constexpr auto XB_V10_SCRIPT_TYPE = static_cast<resource_key_type>(14);
constexpr auto XB_V10_BASE_AI_TYPE = static_cast<resource_key_type>(54);
#endif
constexpr size_t XB_TIME_INTERFACE_SIZE = 0x34u;
constexpr size_t XB_PHYSICAL_SIZE = 0x1A8u;
constexpr size_t PC_PHYSICAL_SIZE = 0x1B0u;
constexpr size_t XB_DAMAGE_SIZE = 0x1E4u;
constexpr size_t PC_DAMAGE_SIZE = 0x23Cu;
constexpr size_t XB_FACIAL_SIZE = 0x80u;
constexpr size_t PC_FACIAL_SIZE = 0x7Cu;
constexpr size_t ACTOR_BOUNDS_SIZE = 0x12u;
constexpr size_t ACTOR_MASH_STACK_SIZE = 8u;

uint32_t read_u32(const void *source)
{
    uint32_t value = 0;
    std::memcpy(&value, source, sizeof(value));
    return value;
}

uint8_t *align_ptr(uint8_t *value, size_t alignment)
{
    const auto address = reinterpret_cast<uintptr_t>(value);
    return reinterpret_cast<uint8_t *>(
        (address + alignment - 1u) & ~(alignment - 1u));
}

void write_u32(void *destination, uint32_t value)
{
    std::memcpy(destination, &value, sizeof(value));
}

class read_cursor
{
public:
    read_cursor(uint8_t *data, size_t size) : m_data(data), m_size(size) {}

    bool valid() const
    {
        return m_valid;
    }

    size_t remaining() const
    {
        return m_offset <= m_size ? m_size - m_offset : 0u;
    }

    uint8_t *current() const
    {
        return m_data + m_offset;
    }

    const uint8_t *take(size_t size)
    {
        if (!m_valid || size > remaining()) {
            m_valid = false;
            return nullptr;
        }

        auto *result = current();
        m_offset += size;
        return result;
    }

    uint8_t read_byte()
    {
        auto *source = take(1u);
        return source != nullptr ? *source : 0u;
    }

    uint32_t read_dword()
    {
        auto *source = take(sizeof(uint32_t));
        return source != nullptr ? read_u32(source) : 0u;
    }

    void align(size_t alignment)
    {
        assert(alignment != 0u && (alignment & (alignment - 1u)) == 0u);
        const auto address = reinterpret_cast<uintptr_t>(current());
        const auto padding = (alignment - (address & (alignment - 1u))) & (alignment - 1u);
        take(padding);
    }

private:
    uint8_t *m_data = nullptr;
    size_t m_size = 0u;
    size_t m_offset = 0u;
    bool m_valid = true;
};

class write_cursor
{
public:
    write_cursor(uint8_t *data, size_t size) : m_data(data), m_size(size) {}

    bool valid() const
    {
        return m_valid;
    }

    uint8_t *current() const
    {
        return m_data + m_offset;
    }

    uint8_t *reserve(size_t size)
    {
        if (!m_valid || m_offset > m_size || size > m_size - m_offset) {
            m_valid = false;
            return nullptr;
        }

        auto *result = current();
        m_offset += size;
        return result;
    }

    void align(size_t alignment)
    {
        assert(alignment != 0u && (alignment & (alignment - 1u)) == 0u);
        const auto address = reinterpret_cast<uintptr_t>(current());
        const auto padding = (alignment - (address & (alignment - 1u))) & (alignment - 1u);
        reserve(padding);
    }

    void write_byte(uint8_t value)
    {
        auto *destination = reserve(1u);
        if (destination != nullptr) {
            *destination = value;
        }
    }

    void write_dword(uint32_t value)
    {
        auto *destination = reserve(sizeof(value));
        if (destination != nullptr) {
            write_u32(destination, value);
        }
    }

    void write_bytes(const void *source, size_t size)
    {
        auto *destination = reserve(size);
        if (destination != nullptr && size != 0u) {
            std::memcpy(destination, source, size);
        }
    }

private:
    uint8_t *m_data = nullptr;
    size_t m_size = 0u;
    size_t m_offset = 0u;
    bool m_valid = true;
};

struct xb_mash
{
    bool present = false;
    const uint8_t *data = nullptr;
    size_t total_size = 0u;
    const uint8_t *normal = nullptr;
    size_t normal_size = 0u;
    const uint8_t *shared = nullptr;
    size_t shared_size = 0u;
};

struct xb_string
{
    const uint8_t *data = nullptr;
    uint32_t byte_count = 0u;
};

struct actor_mash_frame
{
    generic_mash_data_ptrs *data = nullptr;
    uint8_t *xb_bounds = nullptr;
    uint8_t *xb_shared = nullptr;
    uint8_t *pc_bounds_begin = nullptr;
    uint8_t *pc_bounds_end = nullptr;
};

actor_mash_frame g_actor_mash_stack[ACTOR_MASH_STACK_SIZE] {};
size_t g_actor_mash_depth = 0u;

bool xbpack_err(const char *reason)
{
    sp_log("XBPACK conversion failed %s", reason);
    char message[256];
    std::snprintf(message,
                  sizeof(message),
                  "XBPACK conversion failed %s\n",
                  reason);
    OutputDebugStringA(message);
    DebugBreak();
    return false;
}

bool checked_add(size_t &value, size_t amount)
{
    if (amount > MAX_ACTOR_MASH_SIZE || value > MAX_ACTOR_MASH_SIZE - amount) {
        return false;
    }

    value += amount;
    return true;
}

bool read_mash(read_cursor &cursor, xb_mash &value, bool has_header)
{
    value.present = cursor.read_byte() != 0u;
    cursor.align(4u);
    if (!cursor.valid() || !value.present) {
        return cursor.valid();
    }

    value.total_size = cursor.read_dword();
    cursor.align(16u);
    cursor.align(4u);
    value.data = cursor.take(value.total_size);
    if (!cursor.valid() || value.data == nullptr || value.total_size > MAX_ACTOR_MASH_SIZE) {
        return false;
    }

    if (!has_header) {
        value.normal = value.data;
        value.normal_size = value.total_size;
        return true;
    }

    if (value.total_size < XB_MASH_HEADER_SIZE || read_u32(value.data) != XB_MASH_MAGIC) {
        return false;
    }

    const auto split_offset = static_cast<size_t>(read_u32(value.data + 8u));
    if (split_offset < XB_MASH_HEADER_SIZE || split_offset > value.total_size) {
        return false;
    }

    value.normal = value.data + XB_MASH_HEADER_SIZE;
    value.normal_size = split_offset - XB_MASH_HEADER_SIZE;
    value.shared = value.data + split_offset;
    value.shared_size = value.total_size - split_offset;
    return true;
}

bool read_facial_interface(read_cursor &cursor, xb_mash &value)
{
    value.present = cursor.read_byte() != 0u;

    // Unlike the other V14 Xbox actor interfaces, facial data is not stored
    // in a bool/size/mash wrapper.  The presence byte is followed by an
    // ordinary 4-byte wrapper boundary. When present, the fixed-size Xbox
    // facial_expression_interface object itself starts on an 8-byte boundary.
    // Treating its leading CUSTOM_MASH_SENTRY (0x7ACE5BAD) as a byte count
    // rejects valid gameplay CH_* actors such as Alex O'Hirn and Peter.
    cursor.align(4u);
    if (!cursor.valid() || !value.present) {
        return cursor.valid();
    }

    cursor.align(8u);
    value.data = cursor.take(XB_FACIAL_SIZE);
    if (!cursor.valid() || value.data == nullptr) {
        return false;
    }

    value.total_size = XB_FACIAL_SIZE;
    value.normal = value.data;
    value.normal_size = XB_FACIAL_SIZE;
    return true;
}

bool read_str(const uint8_t *raw_string, read_cursor &strings, xb_string &result)
{
    const auto length = read_u32(raw_string);
    if (read_u32(raw_string + 4u) != mash::CUSTOM_MASH_SENTRY || length >= MAX_ACTOR_MASH_SIZE) {
        return false;
    }

    if (length == 0u) {
        return true;
    }

    const auto byte_count = static_cast<size_t>(length) + 1u;
    auto *data = strings.take(byte_count);
    if (data == nullptr || data[length] != 0) {
        return false;
    }

    strings.align(4u);
    if (!strings.valid()) {
        return false;
    }

    result.data = data;
    result.byte_count = static_cast<uint32_t>(byte_count);
    return true;
}

void write_str(write_cursor &writer, const xb_string &string)
{
    writer.align(4u);
    writer.write_dword(string.byte_count);
    if (string.byte_count != 0u) {
        writer.write_bytes(string.data, string.byte_count);
    }
}

void write_str(write_cursor &writer, const char *string)
{
    xb_string view {};
    if (string != nullptr && string[0] != 0) {
        view.data = reinterpret_cast<const uint8_t *>(string);
        view.byte_count = static_cast<uint32_t>(std::strlen(string) + 1u);
    }
    write_str(writer, view);
}

void write_mash(write_cursor &writer, const xb_mash &value)
{
    writer.write_dword(static_cast<uint32_t>(value.normal_size));
    writer.align(16u);
    writer.align(4u);
    writer.write_bytes(value.normal, value.normal_size);
}

bool write_base_ai(write_cursor &writer, const xb_mash &value)
{
    if (value.normal_size < sizeof(base_ai_data)) {
        return false;
    }

    writer.write_dword(static_cast<uint32_t>(value.normal_size));
    writer.align(16u);
    writer.align(4u);

    auto *destination = writer.reserve(value.normal_size);
    if (destination == nullptr) {
        return false;
    }
    std::memcpy(destination, value.normal, value.normal_size);

    const auto raw_type = read_u32(destination + offsetof(resource_key, m_type));
    if (raw_type >= XB_RESOURCE_KEY_TYPE_COUNT) {
        return false;
    }
    const auto pc_type = raw_type <= 54u ? raw_type : raw_type - 1u;
    if (pc_type != RESOURCE_KEY_TYPE_BASE_AI) {
        return false;
    }
    write_u32(destination + offsetof(resource_key, m_type), pc_type);
    return writer.valid();
}

bool write_physical_interface(
    write_cursor &normal_writer,
    write_cursor &shared_writer,
    const xb_mash &physical)
{
    if (physical.normal_size < XB_PHYSICAL_SIZE || physical.shared_size != 0u) {
        return false;
    }

    read_cursor strings(
        const_cast<uint8_t *>(physical.normal + XB_PHYSICAL_SIZE),
        physical.normal_size - XB_PHYSICAL_SIZE);
    xb_string first_string {};
    xb_string second_string {};
    if (!read_str(physical.normal + 0x188u, strings, first_string) ||
        !read_str(physical.normal + 0x194u, strings, second_string) ||
        strings.remaining() != 0u) {
        return false;
    }

    normal_writer.align(4u);
    auto *destination = normal_writer.reserve(PC_PHYSICAL_SIZE);
    if (destination == nullptr) {
        return false;
    }

    std::memcpy(destination, physical.normal, 0x188u);
    std::memcpy(destination + 0x1A8u, physical.normal + 0x1A0u, 8u);

    shared_writer.write_dword(read_u32(physical.normal + 0x0Cu));
    write_str(shared_writer, first_string);
    write_str(shared_writer, second_string);
    return normal_writer.valid() && shared_writer.valid();
}

bool convert_float_to_int(const uint8_t *source, uint32_t &result)
{
    float value = 0.0f;
    std::memcpy(&value, source, sizeof(value));
    const auto wide_value = static_cast<double>(value);
    if (!std::isfinite(value) ||
        wide_value < static_cast<double>(std::numeric_limits<int32_t>::min()) ||
        wide_value > static_cast<double>(std::numeric_limits<int32_t>::max())) {
        return false;
    }

    result = static_cast<uint32_t>(static_cast<int32_t>(std::lround(value)));
    return true;
}

bool write_damage_interface(
    write_cursor &normal_writer,
    write_cursor &shared_writer,
    const xb_mash &damage)
{
    if (damage.normal_size < XB_DAMAGE_SIZE || damage.shared_size != 0u) {
        return false;
    }

    read_cursor strings(
        const_cast<uint8_t *>(damage.normal + XB_DAMAGE_SIZE),
        damage.normal_size - XB_DAMAGE_SIZE);
    xb_string legacy_string {};
    xb_string first_source {};
    xb_string second_source {};
    if (!read_str(damage.normal + 0x6Cu, strings, legacy_string) ||
        !read_str(damage.normal + 0x168u, strings, first_source) ||
        !read_str(damage.normal + 0x174u, strings, second_source) ||
        strings.remaining() != 0u) {
        return false;
    }

    normal_writer.align(4u);
    auto *destination = normal_writer.reserve(PC_DAMAGE_SIZE);
    if (destination == nullptr) {
        return false;
    }

    std::memcpy(destination + 0x4Cu, damage.normal + 0x0Cu, 0x60u);
    std::memcpy(destination + 0xCCu, damage.normal + 0x78u, 0xB8u);

    const auto *xbox_effect = damage.normal + 0x130u;
    auto *pc_effect = destination + 0x184u;
    std::memcpy(pc_effect, xbox_effect, 0x1Cu);
    std::memcpy(pc_effect + 0x24u, xbox_effect + 0x20u, 0x0Cu);
    std::memcpy(pc_effect + 0x34u, xbox_effect + 0x2Cu, 4u);
    std::memcpy(pc_effect + 0x38u, xbox_effect + 0x30u, 4u);

    std::memcpy(destination + 0x1C8u, damage.normal + 0x164u, 4u);
    std::memcpy(destination + 0x1D4u, damage.normal + 0x180u, 0x20u);
    std::memcpy(destination + 0x1F8u, damage.normal + 0x1A0u, 0x44u);

    const auto flags = read_u32(damage.normal + 0x1A0u);
    if ((flags & 4u) != 0u) {
        write_str(shared_writer, first_source);
    }
    if ((flags & 0x10u) != 0u) {
        write_str(shared_writer, second_source);
    }

    write_str(shared_writer, nullptr);
    write_str(shared_writer, nullptr);
    shared_writer.align(4u);
    shared_writer.reserve(0x14u);

    constexpr size_t xbox_float_offsets[] {0x1ACu, 0x1A4u, 0x1BCu, 0x1B4u};
    for (auto offset : xbox_float_offsets) {
        uint32_t value = 0u;
        if (!convert_float_to_int(damage.normal + offset, value)) {
            return false;
        }
        shared_writer.write_dword(value);
    }

    for (auto i = 0; i < 4; ++i) {
        write_str(shared_writer, "Source");
    }

    return normal_writer.valid() && shared_writer.valid();
}

#ifndef OPENUSM_XBPACK_V10
bool convert_entity_prefix(
    entity_base *self,
    generic_mash_header *header,
    generic_mash_data_ptrs *data)
{
    read_cursor input(data->field_0, MAX_ACTOR_MASH_SIZE);

    if ((header->field_E & 0x880u) != 0u) {
        input.align(8u);

        xb_mash sound {};
        if (!read_mash(input, sound, true) || !sound.present) {
            return xbpack_err("invalid xb sound interface wrapper");
        }
        if (sound.normal_size < XB_SOUND_INTERFACE_SIZE) {
            return xbpack_err("truncated xb sound interface");
        }

        auto *pc_sound = reinterpret_cast<sound_and_pfx_interface *>(
            THISCALL(0x004E0970, self));
        if (pc_sound == nullptr) {
            return xbpack_err("unable to allocate PC sound interface");
        }

        // stores sound base only
        std::memcpy(&pc_sound->field_C,
                    sound.normal + 0x0Cu,
                    sizeof(pc_sound->field_C));
        std::memcpy(&pc_sound->field_14,
                    sound.normal + 0x14u,
                    sizeof(pc_sound->field_14));
        std::memcpy(&pc_sound->field_24,
                    sound.normal + 0x24u,
                    sizeof(pc_sound->field_24));
    }

    xb_mash time {};
    if (!read_mash(input, time, true)) {
        return xbpack_err("invalid xb time interface");
    }
    if (time.present) {
        if (time.normal_size != XB_TIME_INTERFACE_SIZE || time.shared_size != 0u) {
            return xbpack_err("unexpected xb time interface");
        }

        auto *pc_entity = reinterpret_cast<entity *>(self);
        pc_entity->create_time_ifc();
        auto *pc_time = pc_entity->field_58;
        if (pc_time == nullptr) {
            return xbpack_err("unable to allocate PC time interface");
        }

        std::memcpy(reinterpret_cast<uint8_t *>(pc_time) + 0x0Cu,
                    time.normal + 0x0Cu,
                    XB_TIME_INTERFACE_SIZE - 0x0Cu);
    }

    data->field_0 = input.current();
    return true;
}
#endif

#ifdef OPENUSM_XBPACK_V10
bool unmash_v10_sound(
    entity_base *self,
    generic_mash_header *header,
    generic_mash_data_ptrs *data)
{
    if ((header->field_E & 0x880u) == 0u) {
        return true;
    }

    read_cursor input(data->field_0, MAX_ACTOR_MASH_SIZE);
    input.align(8u);
    input.align(4u);
    if (input.take(XB_V10_SOUND_INTERFACE_SIZE) == nullptr) {
        return xbpack_err("truncated v10 sound interface");
    }

    auto *pc_sound = reinterpret_cast<sound_and_pfx_interface *>(
        THISCALL(0x004E0970, self));
    if (pc_sound == nullptr) {
        return xbpack_err("unable to allocate PC sound interface");
    }

    data->field_0 = input.current();
    THISCALL(0x004D5560, pc_sound);
    THISCALL(0x004DE130, pc_sound, header, self, pc_sound, data);
    pc_sound->field_8 = true;
    return true;
}

uint8_t *pfx_data(uint8_t *cursor)
{
    auto *result = align_ptr(cursor, 16u);
    if ((result - cursor) & 8u) {
        result += 0x50u;
    }
    return result;
}
#endif

bool convert_actor_mash(generic_mash_header *header, generic_mash_data_ptrs *data)
{
    read_cursor input(data->field_0, MAX_ACTOR_MASH_SIZE);
    if (input.read_dword() != MASH_SYNC_TEST_VAL5) {
        return xbpack_err("MASH5 marker was not found at the xb actor");
    }

    xb_mash base_ai {};
    xb_mash interactable {};
    xb_mash facial {};
    xb_mash web {};
    if (!read_mash(input, base_ai, true) ||
        !read_mash(input, interactable, true) ||
        !read_facial_interface(input, facial) ||
        !read_mash(input, web, true)) {
        return xbpack_err("invalid xb actor interface wrapper");
    }

    const bool has_traffic = input.read_byte() != 0u;
    input.align(4u);
    const auto traffic_value = has_traffic ? input.read_dword() : 0u;
    input.align(16u);
    if (!input.valid() || input.read_dword() != MASH_SYNC_TEST_VAL6) {
        return xbpack_err("invalid MASH6 marker");
    }

    xb_mash physical {};
    xb_mash damage {};
    if ((header->field_E & 0x1Bu) != 0u &&
        (!read_mash(input, physical, true) || !read_mash(input, damage, true))) {
        return xbpack_err("invalid xb physical or damage");
    }

    const bool expects_physical = (header->field_E & 0x10u) != 0u;
    const bool expects_damage = (header->field_E & 0x08u) != 0u;
    if (physical.present != expects_physical || damage.present != expects_damage) {
        return xbpack_err("actor header flags not compatible with ifc presence records");
    }
    if (facial.present && facial.normal_size != XB_FACIAL_SIZE) {
        return xbpack_err("unexpected xb facial ifc size");
    }
    if (!input.valid() || input.remaining() < ACTOR_BOUNDS_SIZE) {
        return xbpack_err("truncated actor bounding data");
    }

    auto *xb_bounds = input.current();
    size_t normal_capacity = 0x400u;
    size_t shared_capacity = 0x400u;
    if (!checked_add(normal_capacity, base_ai.normal_size) ||
        !checked_add(normal_capacity, interactable.normal_size) ||
        !checked_add(normal_capacity, web.normal_size) ||
        !checked_add(normal_capacity, expects_physical ? PC_PHYSICAL_SIZE : 0u) ||
        !checked_add(normal_capacity, expects_damage ? PC_DAMAGE_SIZE : 0u) ||
        !checked_add(shared_capacity, physical.normal_size) ||
        !checked_add(shared_capacity, damage.normal_size)) {
        return xbpack_err("actor exceeds conversion size limit");
    }
    if (g_actor_mash_depth >= ACTOR_MASH_STACK_SIZE) {
        return xbpack_err("nested actor conversion depth exceeded");
    }

    auto *pc_normal = static_cast<uint8_t *>(arch_memalign(16u, normal_capacity));
    auto *pc_shared = static_cast<uint8_t *>(arch_memalign(16u, shared_capacity));
    if (pc_normal == nullptr || pc_shared == nullptr) {
        return xbpack_err("unable to allocate converted actor buffers");
    }

    std::memset(pc_normal, 0, normal_capacity);
    std::memset(pc_shared, 0, shared_capacity);
    write_cursor normal_writer(pc_normal, normal_capacity);
    write_cursor shared_writer(pc_shared, shared_capacity);
    normal_writer.write_dword(MASH_SYNC_TEST_VAL5);
    normal_writer.write_byte(base_ai.present ? 1u : 0u);
    normal_writer.write_byte(interactable.present ? 1u : 0u);
    normal_writer.write_byte(web.present ? 1u : 0u);
    normal_writer.write_byte(facial.present ? 1u : 0u);
    normal_writer.align(4u);

    if (base_ai.present) {
        if (!write_base_ai(normal_writer, base_ai)) {
            return xbpack_err("unable to convert base ai resource key");
        }
    }
    if (interactable.present) {
        write_mash(normal_writer, interactable);
    }
    if (facial.present) {
        normal_writer.align(8u);
        if ((header->field_E & 0x8000u) != 0u) {
            normal_writer.align(4u);
            normal_writer.write_bytes(facial.normal, PC_FACIAL_SIZE);
        }
    }

    normal_writer.write_byte(has_traffic ? 1u : 0u);
    normal_writer.align(4u);
    if (has_traffic) {
        normal_writer.write_dword(traffic_value);
    }
    normal_writer.align(16u);

    if (web.present) {
        write_mash(normal_writer, web);
    }
    normal_writer.write_dword(MASH_SYNC_TEST_VAL6);

    if ((header->field_E & 0x1Bu) != 0u) {
        if (expects_physical && !write_physical_interface(normal_writer, shared_writer, physical)) {
            return xbpack_err("unable to convert xb physical interface");
        }

        normal_writer.align(8u);
        if (expects_damage && !write_damage_interface(normal_writer, shared_writer, damage)) {
            return xbpack_err("unable to convert xb damage interface");
        }
    }

    auto *pc_bounds_begin = normal_writer.current();
    normal_writer.write_bytes(xb_bounds, ACTOR_BOUNDS_SIZE);
    auto *pc_bounds_end = normal_writer.current();
    if (!normal_writer.valid() || !shared_writer.valid()) {
        return xbpack_err("converted actor buffer overflowed");
    }

    auto &frame = g_actor_mash_stack[g_actor_mash_depth++];
    frame.data = data;
    frame.xb_bounds = xb_bounds;
    frame.xb_shared = data->field_4;
    frame.pc_bounds_begin = pc_bounds_begin;
    frame.pc_bounds_end = pc_bounds_end;

    data->field_0 = pc_normal;
    data->field_4 = pc_shared;
    return true;
}
} // namespace

extern "C" __attribute__((noinline, used)) void __fastcall actor_xbpack_entity_prefix_impl(
    entity_base *self,
    int,
    generic_mash_header *header,
    void *context,
    generic_mash_data_ptrs *data)
{
    if (g_platform != NL_PLATFORM_XBOX) {
        THISCALL(0x004CB2F0, self, header, context, data);
        return;
    }

    const auto xb_ifc_flags = header->field_E;
    header->field_E = static_cast<uint16_t>(xb_ifc_flags & ~0x880u);
    THISCALL(0x004CB2F0, self, header, context, data);
    header->field_E = xb_ifc_flags;

#ifdef OPENUSM_XBPACK_V10
    unmash_v10_sound(self, header, data);
#else
    convert_entity_prefix(self, header, data);
#endif
}

#ifdef OPENUSM_XBPACK_V10
extern "C" __attribute__((noinline, used)) void *__cdecl load_v10_pfx(
    uint8_t **normal,
    uint8_t **shared,
    uint32_t size,
    void *owner)
{
    if (g_platform != NL_PLATFORM_XBOX ||
        normal == nullptr || *normal == nullptr ||
        shared == nullptr || *shared == nullptr) {
        return reinterpret_cast<void *>(
            CDECL_CALL(0x004F03B0, normal, shared, size, owner));
    }

    auto *xb_cursor = *normal;
    auto *xb_pfx = pfx_data(xb_cursor);
    const auto count = read_u32(xb_pfx + XB_V10_PFX_VECTOR_OFFSET + 4u);
    const auto has_items = read_u32(xb_pfx + XB_V10_PFX_VECTOR_OFFSET + 8u) != 0u;

    size_t nested_size = 0u;
    if (has_items) {
        if (count > MAX_ACTOR_MASH_SIZE / PFX_VECTOR_MASH_SIZE) {
            xbpack_err("v10 pfx vector exceeds conversion size limit");
            return nullptr;
        }
        nested_size = static_cast<size_t>(count) * PFX_VECTOR_MASH_SIZE;
    }

    size_t allocation_size = 0x80u + PC_PFX_SIZE;
    if (!checked_add(allocation_size, nested_size)) {
        xbpack_err("v10 pfx exceeds conversion size limit");
        return nullptr;
    }

    auto *storage = static_cast<uint8_t *>(arch_memalign(16u, allocation_size));
    if (storage == nullptr) {
        xbpack_err("unable to allocate converted v10 pfx");
        return nullptr;
    }
    std::memset(storage, 0, allocation_size);

    const bool shifted = ((align_ptr(xb_cursor, 16u) - xb_cursor) & 8u) != 0u;
    auto *pc_cursor = storage + (shifted ? 8u : 0u);
    auto *pc_pfx = pfx_data(pc_cursor);
    std::memcpy(pc_pfx, xb_pfx, XB_V10_PFX_VECTOR_OFFSET);
    std::memcpy(pc_pfx + PC_PFX_VECTOR_OFFSET,
                xb_pfx + XB_V10_PFX_VECTOR_OFFSET,
                XB_V10_PFX_SIZE - XB_V10_PFX_VECTOR_OFFSET);
    write_u32(pc_pfx + 0x94u, 1u);
    if (nested_size != 0u) {
        std::memcpy(pc_pfx + PC_PFX_SIZE,
                    xb_pfx + XB_V10_PFX_SIZE,
                    nested_size);
    }

    auto *result = reinterpret_cast<void *>(
        CDECL_CALL(0x004F03B0, &pc_cursor, shared, size, owner));
    *normal = align_ptr(xb_pfx + XB_V10_PFX_SIZE + nested_size, 16u);
    return result;
}

extern "C" __attribute__((noinline, used)) void __fastcall unmash_v10_advanced(
    void *self,
    int,
    generic_mash_header *header,
    actor *owner,
    void *context,
    generic_mash_data_ptrs *data)
{
    if (g_platform != NL_PLATFORM_XBOX || data == nullptr || data->field_4 == nullptr) {
        THISCALL(0x004CFCE0, self, header, owner, context, data);
        return;
    }

    auto *script_key = reinterpret_cast<resource_key *>(data->field_4);
    const auto raw_type = script_key->m_type;
    if (raw_type != RESOURCE_KEY_TYPE_NONE && raw_type != XB_V10_SCRIPT_TYPE) {
        xbpack_err("unexpected v10 advanced-entity script type");
        return;
    }

    if (raw_type == XB_V10_SCRIPT_TYPE) {
        script_key->m_type = RESOURCE_KEY_TYPE_SCRIPT;
    }
    THISCALL(0x004CFCE0, self, header, owner, context, data);
    script_key->m_type = raw_type;
}

extern "C" __attribute__((noinline, used)) base_ai_data *__cdecl construct_v10_base_ai(
    base_ai_data **value)
{
    if (g_platform != NL_PLATFORM_XBOX ||
        value == nullptr || *value == nullptr) {
        return reinterpret_cast<base_ai_data *>(
            CDECL_CALL(0x005037D0, value));
    }

    auto &type = (*value)->field_0.m_type;
    if (type != XB_V10_BASE_AI_TYPE) {
        char reason[160];
        std::snprintf(reason,
                      sizeof(reason),
                      "unexpected v10 base-ai resource type=%u hash=0x%08X object=%p caller=%p",
                      static_cast<unsigned>(type),
                      (*value)->field_0.m_hash.source_hash_code,
                      static_cast<void *>(*value),
                      __builtin_return_address(0));
        xbpack_err(reason);
        return nullptr;
    }

    type = RESOURCE_KEY_TYPE_BASE_AI;
    return reinterpret_cast<base_ai_data *>(
        CDECL_CALL(0x005037D0, value));
}

extern "C" __attribute__((noinline, used)) void __fastcall unmash_v10_base_ai(
    base_ai_data *self,
    int,
    void *info,
    void *context)
{
    uint32_t before[6] {};
    std::memcpy(before, self, sizeof(before));

    THISCALL(0x006D7370, self, info, context);

    if (self->field_0.m_type != XB_V10_BASE_AI_TYPE) {
        uint32_t after[6] {};
        std::memcpy(after, self, sizeof(after));
        const auto *pc_info = static_cast<const uint32_t *>(info);
        char message[320];
        std::snprintf(
            message,
            sizeof(message),
            "XBPACK v10 base-ai unmash changed object=%p "
            "before=%08X,%08X,%08X,%08X,%08X,%08X "
            "after=%08X,%08X,%08X,%08X,%08X,%08X "
            "info=%08X,%08X,%08X,%08X\n",
            static_cast<void *>(self),
            before[0], before[1], before[2], before[3], before[4], before[5],
            after[0], after[1], after[2], after[3], after[4], after[5],
            pc_info[0], pc_info[1], pc_info[2], pc_info[3]);
        xbpack_err(message);
    }
}

extern "C" __attribute__((noinline, used)) void __fastcall init_v10_gun_effect(
    cached_special_effect *self,
    int,
    generic_mash_header *header,
    void *context,
    generic_mash_data_ptrs *data)
{
    if (g_platform != NL_PLATFORM_XBOX) {
        THISCALL(0x004D3650, self, header, context, data);
        return;
    }

    THISCALL(0x005020D0, self);
}

extern "C" __attribute__((noinline, used)) int __fastcall unmash_v10_thrown_item(
    thrown_item *self,
    int,
    generic_mash_header *header,
    void *context,
    generic_mash_data_ptrs *data)
{
    if (g_platform == NL_PLATFORM_XBOX &&
        header != nullptr &&
        header->is_flagged(0x40000000u) &&
        header->class_id == XB_V10_THROWN_ITEM_TYPE) {
        auto *bytes = reinterpret_cast<uint8_t *>(self);

        std::memmove(bytes + 0x300u, bytes + 0x2F0u,
                     XB_V10_THROWN_ITEM_SIZE - 0x2F0u);
        std::memmove(bytes + 0xC0u, bytes + 0xBCu,
                     0x2F0u - 0xBCu);
        std::memset(bytes + 0xBCu, 0, sizeof(uint32_t));
        std::memset(bytes + 0x2F4u, 0, 3u * sizeof(uint32_t));
    }

    return THISCALL(0x00549E80, self, header, context, data);
}

extern "C" __attribute__((noinline, used)) void __cdecl set_v10_actor_context(
    actor *self,
    resource_pack_slot *context)
{
    auto *colgeom = self->colgeom;
    if (g_platform == NL_PLATFORM_XBOX &&
        reinterpret_cast<uint8_t *>(colgeom) ==
            reinterpret_cast<uint8_t *>(self) + XB_V10_ACTOR_SIZE) {
        collision_geometry *instance = nullptr;
        if (colgeom->m_vtbl == collision_capsule_v_table) {
            instance = reinterpret_cast<collision_capsule *>(colgeom)->make_instance(self);
        } else if (colgeom->m_vtbl == collision_mesh_v_table()) {
            instance = reinterpret_cast<cg_mesh *>(colgeom)->make_instance(self);
        }

        if (instance == nullptr) {
            xbpack_err("unable to move v10 actor collision geometry");
            return;
        }
        instance->m_vtbl = colgeom->m_vtbl;
        self->colgeom = instance;
    }

    self->m_resource_context = context;
}

extern "C" __attribute__((naked, used)) void actor_v10_context_hook()
{
    __asm__ volatile(
        "push eax\n\t"
        "push ecx\n\t"
        "push edx\n\t"
        "push eax\n\t"
        "push ebx\n\t"
        "call _set_v10_actor_context\n\t"
        "add esp, 8\n\t"
        "pop edx\n\t"
        "pop ecx\n\t"
        "pop eax\n\t"
        "test ecx, ecx\n\t"
        "push 0x004FC4DA\n\t"
        "ret\n\t");
}

extern "C" __attribute__((naked, used)) void actor_v10_finish_hook()
{
    __asm__ volatile(
        "pop edi\n\t"
        "pop esi\n\t"
        "pop ebp\n\t"
        "pop ebx\n\t"
        "add esp, 0x2C\n\t"
        "ret 0x0C\n\t");
}
#endif

extern "C" __attribute__((noinline, used)) void __cdecl actor_xbpack_prepare_impl(
    actor *self,
    generic_mash_data_ptrs *data,
    generic_mash_header *header)
{
    THISCALL(0x00502C70, self);

    if (g_platform == NL_PLATFORM_XBOX) {
#ifdef OPENUSM_XBPACK_V10
        if (read_u32(data->field_0) != MASH_SYNC_TEST_VAL5) {
            data->field_0 -= sizeof(uint32_t);
        }
#else
        convert_actor_mash(header, data);
#endif
    }
}

extern "C" __attribute__((naked)) void actor_xbpack_prepare_hook()
{
    __asm__ volatile(
        // esi=meshbuf   actor hdr [esp+44h]
        "mov eax, [esp + 0x44]\n\t"
        "push eax\n\t"
        "push ebp\n\t"
        "push ebx\n\t"
        "call _actor_xbpack_prepare_impl\n\t"
        "add esp, 12\n\t"
        "ret\n\t");
}

void actor_xbpack_finish(generic_mash_data_ptrs *data)
{
    if (g_platform != NL_PLATFORM_XBOX || g_actor_mash_depth == 0u) {
        return;
    }

    auto &frame = g_actor_mash_stack[g_actor_mash_depth - 1u];
    if (frame.data != data) {
        for (size_t i = 0u; i + 1u < g_actor_mash_depth; ++i) {
            if (g_actor_mash_stack[i].data == data) {
                xbpack_err("actor conversions didnt finish in order");
                return;
            }
        }
        return;
    }

    auto *current = data->field_0;
    const auto current_address = reinterpret_cast<uintptr_t>(current);
    const auto bounding_begin_address = reinterpret_cast<uintptr_t>(frame.pc_bounds_begin);
    const auto bounding_end_address = reinterpret_cast<uintptr_t>(frame.pc_bounds_end);
    if (current_address < bounding_begin_address || current_address > bounding_end_address) {
        data->field_0 = frame.xb_bounds;
        data->field_4 = frame.xb_shared;
        --g_actor_mash_depth;
        xbpack_err("pc actor parser exceeded bounds");
        return;
    }

    const auto bounding_bytes_consumed =
        static_cast<size_t>(current - frame.pc_bounds_begin);
    data->field_0 = frame.xb_bounds + bounding_bytes_consumed;
    data->field_4 = frame.xb_shared;
    --g_actor_mash_depth;
}

extern "C" __attribute__((noinline, used)) void __cdecl actor_xbpack_finish_impl(
    generic_mash_data_ptrs *data)
{
    actor_xbpack_finish(data);
}

extern "C" __attribute__((naked, used)) void actor_xbpack_finish_hook()
{
    __asm__ volatile(
        //  out= [esp+38h]
        "mov eax, [esp + 0x38]\n\t"
        "push eax\n\t"
        "call _actor_xbpack_finish_impl\n\t"
        "add esp, 4\n\t"
        "add esp, 0x2C\n\t"
        "ret 0x0C\n\t");
}

void actor_xbpack_patch()
{
    REDIRECT(0x004F0E37, actor_xbpack_entity_prefix_impl);
    REDIRECT(0x004FBD6D, actor_xbpack_entity_prefix_impl);

#ifdef OPENUSM_XBPACK_V10
    REDIRECT(0x004FC022, actor_xbpack_prepare_hook);
    REDIRECT(0x004F05F3, load_v10_pfx);
    REDIRECT(0x004F0E47, load_v10_pfx);
    REDIRECT(0x004F78AB, load_v10_pfx);
    REDIRECT(0x004F78D3, load_v10_pfx);
    REDIRECT(0x004FBDE8, unmash_v10_advanced);
    REDIRECT(0x004FC0EC, unmash_v10_base_ai);
    REDIRECT(0x004FC0F2, construct_v10_base_ai);
    // Only the PC-only first cache lacks a beta shared-buffer payload.
    // The other four caches must consume their original serialized data.
    REDIRECT(0x004FF88C, init_v10_gun_effect);
    set_vfunc(0x008892C4, unmash_v10_thrown_item);
    SET_JUMP(0x004FC4D4, actor_v10_context_hook);
    SET_JUMP(0x004FC548, actor_v10_finish_hook);
#else
    REDIRECT(0x004FC022, actor_xbpack_prepare_hook);
    // actor::un_mash 
    SET_JUMP(0x004FC616, actor_xbpack_finish_hook);
    SET_JUMP(0x004FC66A, actor_xbpack_finish_hook);
#endif
}

#endif
