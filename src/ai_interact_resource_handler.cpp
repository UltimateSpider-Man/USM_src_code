#include "ai_interact_resource_handler.h"

#include "common.h"
#include "trace.h"
#include "utility.h"
#include "ai_interaction_data.h"
#include "mash_info_struct.h"
#include "resource_directory.h"
#include "resource_location.h"
#include "worldly_pack_slot.h"

#include <cstdint>
#include <cstring>
#include <memory>
#include <vector>

VALIDATE_SIZE(ai_interact_resource_handler, 0x14);

#if defined(OPENUSM_XBPACK_V10) && !defined(TARGET_XBOX)
namespace
{
constexpr size_t XBOX_V10_INTERACT_SIZE = 0x9C;
constexpr size_t PC_INTERACT_SIZE = 0xA8;
constexpr size_t ADDED_HASHES_OFFSET = 0x48;
constexpr size_t ADDED_HASHES_SIZE = PC_INTERACT_SIZE - XBOX_V10_INTERACT_SIZE;

static_assert(sizeof(ai_interaction_data) == PC_INTERACT_SIZE);

struct v10_interact_entry
{
    resource_location *location;
    uint32_t original_offset;
    std::unique_ptr<uint8_t[]> storage;
    ai_interaction_data *object;
};

std::vector<v10_interact_entry> v10_interacts;

auto find_v10_interact(resource_location *location)
{
    for (auto it = v10_interacts.begin(); it != v10_interacts.end(); ++it) {
        if (it->location == location) {
            return it;
        }
    }

    return v10_interacts.end();
}

ai_interaction_data *make_v10_interact(uint8_t *source,
                                      resource_location *location)
{
    assert(location->m_size >= static_cast<int>(XBOX_V10_INTERACT_SIZE));

    const auto normal_size = static_cast<size_t>(location->m_size);
    const auto pc_normal_size = normal_size + ADDED_HASHES_SIZE;
    auto storage = std::make_unique<uint8_t[]>(pc_normal_size + 0x10);

    const auto source_phase = reinterpret_cast<uintptr_t>(source) & 0xF;
    const auto pc_phase = (source_phase + 0x10 - ADDED_HASHES_SIZE) & 0xF;
    const auto storage_phase = reinterpret_cast<uintptr_t>(storage.get()) & 0xF;
    const auto adjustment = (pc_phase - storage_phase) & 0xF;
    auto *pc_normal = storage.get() + adjustment;

    std::memcpy(pc_normal, source, ADDED_HASHES_OFFSET);
    std::memset(pc_normal + ADDED_HASHES_OFFSET, 0, ADDED_HASHES_SIZE);
    std::memcpy(pc_normal + ADDED_HASHES_OFFSET + ADDED_HASHES_SIZE,
                source + ADDED_HASHES_OFFSET,
                normal_size - ADDED_HASHES_OFFSET);

    mash_info_struct info_struct {
        mash::UNMASH_MODE, pc_normal, static_cast<int>(pc_normal_size), true};
    info_struct.mash_image_ptr[mash::SHARED_BUFFER] = source + normal_size;

    auto *object = reinterpret_cast<ai_interaction_data *>(pc_normal);
    info_struct.unmash_class(object, nullptr, mash::NORMAL_BUFFER);
    mash_info_struct::construct_class(object);

    const auto original_offset = location->m_offset;
    const auto pack_base = reinterpret_cast<uintptr_t>(source) - original_offset;
    location->m_offset = static_cast<uint32_t>(
        reinterpret_cast<uintptr_t>(object) - pack_base);

    v10_interacts.push_back({
        location, original_offset, std::move(storage), object});
    return object;
}
}
#endif

ai_interact_resource_handler::ai_interact_resource_handler(worldly_pack_slot *a2) 
{
    this->m_vtbl = 0x00888AE4;
    this->my_slot = a2;
    this->field_10 = RESOURCE_KEY_TYPE_AI_INTERACTION;
}

bool ai_interact_resource_handler::_handle(worldly_resource_handler::eBehavior a2, limited_timer *a3)
{
    TRACE("ai_interact_resource_handler::handle");

    return base_engine_resource_handler::_handle(a2, a3);
}

bool ai_interact_resource_handler::_handle_resource(worldly_resource_handler::eBehavior a2,
        resource_location *a3)
{
    TRACE("ai_interact_resource_handler::handle_resource");

    auto &v3 = this->my_slot->get_resource_directory();
    auto *resource = v3.get_resource(a3, nullptr);
    assert(resource != nullptr);
    
    if ( a2 == UNLOAD)
    {
#if defined(OPENUSM_XBPACK_V10) && !defined(TARGET_XBOX)
        auto entry = find_v10_interact(a3);
        if (entry != v10_interacts.end())
        {
            entry->object->destruct_mashed_class();
            a3->m_offset = entry->original_offset;
            v10_interacts.erase(entry);
        }
        else
#endif
        bit_cast<ai_interaction_data *>(resource)->destruct_mashed_class();
    }
    else
    {
#if defined(OPENUSM_XBPACK_V10) && !defined(TARGET_XBOX)
        auto entry = find_v10_interact(a3);
        if (entry == v10_interacts.end()) {
            make_v10_interact(resource, a3);
        }
#else
        ai_interaction_data *new_interact = CAST(new_interact, resource);
        assert(new_interact != nullptr);

#if OPENUSM_XBOX_MASH_FORMAT
        mash_info_struct info_struct {mash::UNMASH_MODE, resource, a3->m_size, true};
#else
        mash_info_struct info_struct {resource, a3->m_size};
#endif

        info_struct.unmash_class(new_interact, nullptr
#if OPENUSM_XBOX_MASH_FORMAT
            , mash::NORMAL_BUFFER
#endif
                );

        mash_info_struct::construct_class(new_interact);

#if OPENUSM_XBOX_MASH_FORMAT
        a3->m_offset += info_struct.get_header_size();
#endif
#endif
    }
    
    ++this->field_C;
    return false;
}

void ai_interact_resource_handler_xbpack_patch()
{
#ifdef OPENUSM_XBPACK_MODE
    FUNC_ADDRESS(address, &ai_interact_resource_handler::_handle_resource);
    set_vfunc(0x00888AF0, address);
#endif
}

void ai_interact_resource_handler_patch()
{
    {
        FUNC_ADDRESS(address, &ai_interact_resource_handler::_handle);
        set_vfunc(0x00888AE8, address);
    }

    {
        FUNC_ADDRESS(address, &ai_interact_resource_handler::_handle_resource);
        set_vfunc(0x00888AF0, address);
    }
}
