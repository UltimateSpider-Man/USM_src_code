#include "mission_stack_manager.h"

#include "game.h"
#include "resource_pack_streamer.h"
#include "func_wrapper.h"
#include "game.h"
#include "osassert.h"
#include "resource_directory.h"
#include "resource_manager.h"
#include "resource_pack_group.h"
#include "resource_partition.h"
#include "sound_bank_slot.h"
#include "sound_manager.h"
#include "trace.h"
#include "terrain.h"
#include "wds.h"

#include "common.h"

VALIDATE_SIZE(mission_stack_manager, 12u);

mission_stack_manager *& mission_stack_manager::s_inst = var<mission_stack_manager *>(0x0096851C);

mission_stack_manager::mission_stack_manager() {
    s_inst = this;
    this->pack_loads_or_unloads_pending = 0;
    this->field_4 = 0;
    this->loading_started = 0;
    this->unloading_started = 0;
    this->field_A = 0;
}

resource_pack_group *mission_stack_manager::get_pack_group_for_pack(const mString &a1) {
    TRACE("mission_stack_manager::get_pack_group_for_pack");

    return (resource_pack_group *) THISCALL(0x005D2250, this, &a1);
}


void mission_stack_manager::sub_5D8430(const mString &a1, const mString &a2)
{
	TRACE("mission_stack_manager::sub_5D8430");
	
    this->push_mission_pack(a1, a2, -1, false);

    auto *v3 = resource_manager::get_partition_pointer(RESOURCE_PARTITION_MISSION);
    auto *streamer = bit_cast<resource_pack_streamer *>(v3 + 24);
    streamer->flush(game::render_empty_list);
	
	THISCALL(0x005D8430, this, &a1, &a2);
}

resource_pack_group *mission_stack_manager::get_pack_group(const mString &a1)
{
    TRACE("mission_stack_manager::get_pack_group");

    resource_key v12 {string_hash {a1.c_str()}, static_cast<resource_key_type>(69)};

    auto *mission_partition = resource_manager::get_partition_pointer(RESOURCE_PARTITION_MISSION);
    resource_directory &v2 = mission_partition->get_pack_slots().front()->get_resource_directory();
    auto &v4 = v2.field_68;
    auto size = v4.m_size;

    if ( size == 0 ) {
        return nullptr;
    }

    auto &m_data = v4.m_data;
    for ( uint16_t i = 0; i < size; ++i )
    {
        resource_key v8 = m_data[i].field_0;
        if ( v8 == v12 ) {
            return &m_data[i];
        }
    }

    return nullptr;

}

bool mission_stack_manager::waiting_for_push_or_pop() {
    return (bool) THISCALL(0x005BB640, this);
}

void mission_stack_manager::insert_mission_pack(resource_pack_group *a2, const mString &a3, int a4) {
    THISCALL(0x005D82D0, this, a2, &a3, a4);
}

void mission_stack_manager::push_mission_pack(const mString &a2,
                                              const mString &a3,
                                              int district_slot_override_idx,
                                              bool a5)
{
    TRACE("mission_stack_manager::push_mission_pack");

    if constexpr (0) {

        mString v29 = a2;
        v29.to_lower();

        if (this->pack_loads_or_unloads_pending != 0) {
            auto *v5 = v29.c_str();
            error(
                "Script mission (%s) tried to push a mission pack when a pack was currently being "
                "unloaded",
                v5);
        }

        auto *my_partition = resource_manager::get_partition_pointer(RESOURCE_PARTITION_MISSION);
        assert(my_partition != nullptr);

        auto *my_streamer = my_partition->get_streamer();
        assert(my_streamer != nullptr);

        resource_pack_location v33{};

        string_hash v25{a3.c_str()};

        resource_key v27{v25, RESOURCE_KEY_TYPE_PACK};

        if (!resource_manager::get_pack_file_stats(v27, &v33, nullptr, nullptr)) {
            auto *v7 = a3.c_str();
            error("Packfile %s not found in amalgapak.  Perhaps you need to repack it?", v7);
        }

        assert(v33.loc.m_size > 0);

        void *v23 = nullptr;
        if (district_slot_override_idx == -1)
		{
            if (!my_partition->has_room_for_slot(v33.loc.m_size))
			{
                mString v32 {mString::fmtd{0},
                            "%s tried to push pack %s, but it won't fit on the "
                            "stack.\r\nCurrently on the stack:",
                            a2.c_str(),
                            a3.c_str()};
                auto *pack_slots = my_streamer->get_pack_slots();

                if ( pack_slots != nullptr )
				{
                    for (auto &slot : (*pack_slots))
					{
                        if (slot != nullptr
								&& !slot->is_empty())
						{
                            auto v14 = slot->get_name_key().m_hash;
                            auto *v15 = v14.to_string();
                            mString v16{mString::fmtd{0}, " %s", v15};

                            v32 += v16;
                        }
                    }
                }
            }
        } else {
            auto *district_partition = resource_manager::get_partition_pointer(RESOURCE_PARTITION_DISTRICT);
            assert(district_partition != nullptr);

            auto &pack_slots = district_partition->get_pack_slots();
            assert(district_slot_override_idx >= 0 &&
                   district_slot_override_idx < pack_slots.size());

            auto *s = pack_slots[district_slot_override_idx];
            assert(s->is_empty());

            v23 = s->get_header_mem_addr();
            if (s->get_slot_size() < v33.loc.m_size) {
                auto *v10 = a3.c_str();
                error("Cannot load pack %s (size: %d) into district slot %d (size: %d)",
                       v10,
                       v33.loc.m_size,
                       district_slot_override_idx,
                       s->get_slot_size());
            }
        }

        my_partition->push_pack_slot(v33.loc.m_size, v23);
        assert(my_partition->get_streamer() != nullptr);

        resource_pack_token v28{district_slot_override_idx, 0};

        auto v18 = my_partition->get_pack_slots().size();
        auto *v19 = my_partition->get_streamer();
        v19->load(a3.c_str(), v18 - 1, mission_stack_manager::mission_stack_callback, &v28);
        ++this->pack_loads_or_unloads_pending;
        if (a5) {
            v19->flush(nullptr);

            assert(my_partition->get_streamer()->is_idle());
        }

        auto v20 = "pk_" + a2;
        auto v21 = (_strcmpi(a3.c_str(), v20.c_str()) == 0 && !a2.is_equal("loading screen"));

        if (v21) {
            sub_54DC10(a2.c_str(), false);
        }

    } else {
        THISCALL(0x005D7FE0, this, &a2, &a3, district_slot_override_idx, a5);
    }
}

bool mission_stack_manager::nonstatic_mission_stack_callback(resource_pack_slot::callback_enum a2,
                                                             resource_pack_streamer *a3,
                                                             resource_pack_slot *a4,
                                                             limited_timer *a5) {
    return (bool) THISCALL(0x005D56E0, this, a2, a3, a4, a5);
}

bool mission_stack_manager::mission_stack_callback(resource_pack_slot::callback_enum a1,
                                                   resource_pack_streamer *a2,
                                                   resource_pack_slot *a3,
                                                   limited_timer *a5) {
    return s_inst->nonstatic_mission_stack_callback(a1, a2, a3, a5);
}

void mission_stack_manager::unmap_directory_parent(resource_pack_slot *a1)
{
    if constexpr (1)
    {
        auto *part_ptr = resource_manager::get_partition_pointer(RESOURCE_PARTITION_MISSION);
        assert(part_ptr != nullptr);

        auto &pack_slots = part_ptr->get_pack_slots();
        auto size = pack_slots.size();

        if (size > 1) {
            assert(pack_slots[size - 1] != nullptr);

            auto *dir = &a1->get_resource_directory();

            pack_slots.at(0)->get_resource_directory().remove_parent(dir);
        }

    } else {
        CDECL_CALL(0x005D1FC0, a1);
    }
}

void mission_stack_manager::push_mission_pack_immediate(const mString &a1, const mString &a2)
{
    push_mission_pack(a1, a2, -1, false);

    resource_partition *partition = resource_manager::get_partition_pointer(RESOURCE_PARTITION_MISSION);
    assert(partition != nullptr);
    assert(partition->get_streamer() != nullptr);

    auto *streamer = partition->get_streamer();
    streamer->flush(game::render_empty_list);
}

void mission_stack_manager::pop_mission_pack(const mString &a2, const mString &a3) {
    THISCALL(0x005D5800, this, &a2, &a3);
}

void mission_stack_manager::map_directory_parent(resource_pack_slot *a1)
{
    auto *part_ptr = resource_manager::get_partition_pointer(RESOURCE_PARTITION_MISSION);
    assert(part_ptr != nullptr);

    auto &pack_slots = part_ptr->get_pack_slots();
    if (pack_slots.size() > 1)
    {
        auto &front = pack_slots.front();
        a1->get_resource_directory().remove_parent(&front->get_resource_directory());
        front->get_resource_directory().add_parent(&a1->get_resource_directory());
    }
}

bool mission_stack_manager::is_pack_pushed(const mString &a1) {
    return (bool) THISCALL(0x005D2360, this, &a1);
}

void mission_stack_manager::pop_mission_pack_internal()
{
    if constexpr (1)
    {
        auto *my_partition = resource_manager::get_partition_pointer(RESOURCE_PARTITION_MISSION);
        assert(my_partition != nullptr);

        auto &pack_slots = my_partition->get_pack_slots();

        assert(pack_slots.empty());

        assert(my_partition->get_streamer() != nullptr);

        auto *slot = pack_slots.back();
        if (slot->is_empty()) {
            auto slot_idx = slot->get_pack_token().field_0;
            if (slot_idx != -1) {
                g_world_ptr->the_terrain->unlock_district_pack_slot(slot_idx);
            }

            my_partition->pop_pack_slot();

        } else {
            --this->pack_loads_or_unloads_pending;

            my_partition->get_streamer()->unload(pack_slots.size() - 1);
        }
    } else {
        THISCALL(0x005D22F0, this);
    }
}

void mission_stack_manager::pop_mission_pack_immediate(const mString &a1, const mString &a2)
{
    this->pop_mission_pack(a1, a2);

    resource_partition *partition = resource_manager::get_partition_pointer(RESOURCE_PARTITION_MISSION);
    assert(partition != nullptr);
    assert(partition->get_streamer() != nullptr);

    auto *streamer = partition->get_streamer();
    streamer->flush(game::render_empty_list);
}

mission_stack_manager *mission_stack_manager::get_instance()
{
    assert(s_inst != nullptr);

    return s_inst;
}

void mission_stack_manager::start_streaming()
{
    resource_partition *mission_partition = resource_manager::get_partition_pointer(RESOURCE_PARTITION_MISSION);
    assert(mission_partition != nullptr);

    auto *mission_streamer = mission_partition->get_streamer();
    assert(mission_streamer != nullptr);

    mission_streamer->set_active(true);
}

void mission_stack_manager_patch()
{
    {
        FUNC_ADDRESS(address, &mission_stack_manager::push_mission_pack);
        REDIRECT(0x005D843E, address);
        REDIRECT(0x005DA58E, address);
        REDIRECT(0x005DEF68, address);
        REDIRECT(0x0068081F, address);
        REDIRECT(0x00682C88, address);
    }
}
