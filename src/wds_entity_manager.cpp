#include "wds_entity_manager.h"

#include "box_trigger.h"
#include "camera.h"
#include "common.h"
#include "debugutil.h"
#include "entity.h"
#include "entity_mash.h"
#include "func_wrapper.h"
#include "item.h"
#include "mic.h"
#include "mstring.h"
#include "oldmath_po.h"
#include "osassert.h"
#include "resource_key.h"
#include "resource_manager.h"
#include "script_lib_debug_menu.h"
#include "trace.h"
#include "trigger_manager.h"
#include "utility.h"
#include "vtbl.h"
#include "multi_vector.h"
#include "variables.h"
#include "wds.h"
#include "worldly_pack_slot.h"

#include <list.hpp>

#include <cassert>

VALIDATE_SIZE(wds_entity_manager, 0x2C);

wds_entity_manager::wds_entity_manager() {
    THISCALL(0x005DF4C0, this);
}

template<typename T>
typename multi_vector<T>::iterator multi_vector_find(typename multi_vector<T>::iterator &begin, typename multi_vector<T>::iterator &end, T &a4)
{
    auto it = begin;
    while ( it != end )
    {
        auto v4 = *it.field_8;
        if ( v4 == a4 )
        {
            return it;
        }

        ++it;
    }

    return end;
}

bool wds_entity_manager::is_entity_valid(entity *a1)
{
    auto &v12 = this->entities;

    auto end = v12.end();
    auto begin = v12.begin();
    auto v11 = multi_vector_find<entity *>(begin, end, a1);

    auto v4 = v12.end();
    return v11 != v4;
}

bool wds_entity_manager::is_item_valid(item *a2)
{
    auto end = this->items.end();
    auto begin = this->items.begin();

    auto v11 = multi_vector_find<item *>(begin, end, a2);

    return v11 != this->items.end();
}

entity *wds_entity_manager::acquire_entity(
        string_hash a2,
        string_hash a3,
        uint32_t a6)
{
    mString v6 {};
    auto *v5 = this->create_and_add_entity_or_subclass(a2, a3, {identity_matrix}, v6, a6, nullptr);
    return v5;
}

entity *wds_entity_manager::acquire_entity(string_hash a1, uint32_t a2)
{
    TRACE("wds_entity_manager::acquire_entity");

    if constexpr (1) {
        mString a5 {};
        auto v6 = make_unique_entity_id();
        auto *v4 = this->create_and_add_entity_or_subclass(a1, v6, identity_matrix, a5, a2, nullptr);
        return v4;
    } else {
        return (entity *) THISCALL(0x005E0D40, this, a1, a2);
    }
}

void wds_entity_manager::add_dynamic_instanced_entity(entity *a2) {
    TRACE("wds_entity_manager::add_dynamic_instanced_entity");

    THISCALL(0x005E0760, this, a2);
}

void wds_entity_manager::destroy_all_entities_and_items() {
    THISCALL(0x005D9060, this);
}

void __fastcall debug_character_destroy_all_entities(wds_entity_manager *self, void *)
{
    // The PC character-list spawner owns any packs it pushed. Destroy its
    // actors and release those packs before stock world teardown invalidates
    // the tracked handles.
    clear_debug_character_spawns();
    self->destroy_all_entities_and_items();
}

#ifdef OPENUSM_XBPACK_MODE
void __fastcall xbpack_destroy_all_entities(wds_entity_manager *self, void *)
{
    // Destroy native Script-menu characters and release their pushed CH_ packs
    // before the world removes players and tears down the remaining entities.
    clear_debug_character_spawns();

    while (g_world_ptr != nullptr && g_world_ptr->num_players > 0) {
        g_world_ptr->remove_player(g_world_ptr->num_players - 1);
    }

    self->destroy_all_entities_and_items();
}
#endif

void wds_entity_manager::destroy_entity(entity *e) {
    assert(e != nullptr);

    if constexpr (1) {
        bool v4;

        auto v3 = e->get_flavor() - 9;
        if (v3 && v3 == 2) {
            v4 = this->remove_item((item *) e);
        } else {
            v4 = this->remove_entity(e);
        }

        if (v4) {
            if ((e->field_8 & 0x80000000) == 0) {
                void (__fastcall *release_mem)(entity *, void *) =
                    CAST(release_mem, get_vfunc(e->m_vtbl, 0x10));
                release_mem(e, nullptr);
            } else {
                void (__fastcall *finalize)(entity *, void *, bool) =
                    CAST(finalize, get_vfunc(e->m_vtbl, 0x0));
                finalize(e, nullptr, true);
            }
        }
    } else {
        THISCALL(0x005D6F20, this, e);
    }
}

bool wds_entity_manager::remove_item(item *a2) {
    return (bool) THISCALL(0x005D5410, this, a2);
}

bool wds_entity_manager::remove_entity(entity *a2) {
    return (bool) THISCALL(0x005D5350, this, a2);
}

void wds_entity_manager::release_entity(entity *e) {
    this->destroy_entity(e);
}

void wds_entity_manager::remove_entity_from_misc_lists(entity *e) {
    assert(e != nullptr);
}

void wds_entity_manager::make_time_limited(entity *a1, Float a2) {
    THISCALL(0x005DBBC0, this, a1, a2);
}

item *wds_entity_manager::add_item(_std::vector<item *> *a2, item *a3) {
    TRACE("wds_entity_manager::add_item");

    return (item *) THISCALL(0x005DF7D0, this, a2, a3);
}

int wds_entity_manager::add_ent_to_lists(_std::vector<entity *> *a2,
                                         _std::vector<item *> *a3,
                                         entity *ent)
{
    TRACE("wds_entity_manager::add_ent_to_lists");

    if constexpr (1)
    {
        auto result = 0;
        switch ((entity_flavor_t) ent->get_flavor()) {
        case ACTOR:
        case ENTITY:
        case MARKER:
        case PARKING_MARKER:
        case WATER_EXIT_MARKER:
        case LIGHT_SOURCE:
        case PFX:
        case CONGLOMERATE:
        case CONGLOMERATE_CLONE:
        case BEAM:
        case MANIP:
        case SWITCH:
        case LENSFLARE:
        case NEOLIGHT:
        case ANCHOR_MARKER:
        case LINE_ANCHOR:
        case AI_COVER_MARKER: {
            result = this->add_entity_internal(a2, ent);
            break;
        }
        case ENTITY_ITEM: {
            result = (int) this->add_item(a3, (item *) ent);
            break;
        }
        default:
            assert(0);
            break;
        }

        return result;
    } else {
        THISCALL(0x005E0970, this, a2, a3, ent);
    }
}

entity_base *wds_entity_manager::get_entity(string_hash a1) {
    return entity_handle_manager::find_entity(a1, entity_flavor_t::IGNORE_FLAVOR, true);
}

entity * wds_entity_manager::create_and_add_entity_or_subclass(string_hash a2,
                                                          string_hash a3,
                                                          const po &a4,
                                                          const mString &a5,
                                                          uint32_t a6,
                                                          const _std::list<region *> *regions)
{
    TRACE("wds_entity_manager::create_and_add_entity_or_subclass");

    if constexpr (0)
    {
        entity *v71 = nullptr;

        auto v68 = a4;
        auto v66 = (a6 & 1) != 0;
        auto v64 = (a6 & 4) != 0;
        [[maybe_unused]] auto v63 = (a6 & 8) != 0;
        auto v62 = (a6 & 0x10) != 0;
        [[maybe_unused]] auto v61 = (a6 & 0x80) != 0;
        auto v60 = (a6 & 0x20) != 0;
        [[maybe_unused]] auto v59 = (a6 & 0x20000000) == 0;
        uint32_t v65 = 1;
        if ( !g_is_the_packer() )
        {
            auto v33 = a2;
            resource_key v58 {v33, RESOURCE_KEY_TYPE_ENTITY};

            int v57;
            worldly_pack_slot *slot_ptr;
            auto *v51 = resource_manager::get_resource(v58, &v57, bit_cast<resource_pack_slot **>(&slot_ptr));
            if ( v51 != nullptr )
            {
                assert(slot_ptr != nullptr);

                auto *ent_vec_ptr = slot_ptr->get_entity_instances();
                auto *item_vec_ptr = slot_ptr->get_item_instances();

                assert(ent_vec_ptr != nullptr);

                assert(item_vec_ptr != nullptr);

                auto *__old_context = resource_manager::push_resource_context(slot_ptr);
                auto *tmp_e = parse_entity_mash(ent_vec_ptr, item_vec_ptr, v51, &a3, nullptr, false);
                resource_manager::pop_resource_context();
                assert(resource_manager::get_resource_context() == __old_context);

                assert(tmp_e->is_an_entity());
                v71 = bit_cast<entity *>(tmp_e);
                if ( v71 == nullptr )
                {
                    auto *v11 = v58.m_hash.to_string();
                    error("parse_entity_mash error on entity '%s.ent'", v11);
                }

                this->add_ent_to_lists(ent_vec_ptr, item_vec_ptr, v71);
            }
            else
            {
                auto *v12 = a2.to_string();
                warning("Entity '%s.ent' not found in any loaded packfiles!", v12);
                auto *partition_pointer = resource_manager::get_partition_pointer(RESOURCE_PARTITION_MISSION);
                debug_print_va("Mission stack contains:");
                if ( partition_pointer != nullptr ) {
                    //sub_65ED68(partition_pointer);
                }
            }
        }

        bool v13 = (v71 != nullptr);
        uint32_t v70 = 0;
        if ( v13 )
        {
            auto v38 = ( (v66 && !v65)
                        || (v70 & 0x2000) != 0
                        || v71->is_flagged(0x2000)
                        );
            v71->set_flag_recursive(static_cast<entity_flag_t>(0x2000), v38);

            v38 = !v65
                || (v70 & 0x40) != 0
                || v71->is_flagged(0x40);
            v71->set_flag_recursive(static_cast<entity_flag_t>(0x40), v38);

            v38 = v62
                || (v70 & 0x80) != 0
                || v71->is_flagged(0x80);
            v71->set_flag_recursive(static_cast<entity_flag_t>(0x80), v38);

            v38 = !v64 || (v70 & 0x200) != 0;
            v71->set_flag_recursive(static_cast<entity_flag_t>(0x200), v38);

            auto *tmp_e = v71;
            if ( tmp_e->is_an_actor() )
            {
                auto *v26 = bit_cast<actor *>(v71);
                v26->process_extra_scene_flags(a6);
            }

            v71->set_abs_po(v68);
            if ( regions != nullptr )
            {
                for ( auto &reg : (*regions))
                {
                    auto *tmp_e = v71;
                    tmp_e->force_region(reg);
                }
            }
            else if ( !v60 )
            {
                auto *tmp_e = v71;
                auto *the_terrain = g_world_ptr->get_the_terrain();
                tmp_e->compute_sector(the_terrain, g_world_ptr->is_loading_from_scn_file(), nullptr);
            }
        }

        return v71;
    }
    else
    {
        entity * (__fastcall *func)(void *, void *edx,
                  string_hash,
                  string_hash,
                  const po *,
                  const mString *,
                  uint32_t ,
                  const _std::list<region *> *) = CAST(func, 0x005E0A10);
        return func(this, nullptr, a2, a3, &a4, &a5, a6, regions);
    }
}

box_trigger *wds_entity_manager::create_and_add_box_trigger(
        string_hash a1,
        const vector3d &a3,
        const convex_box &a4)
{
    TRACE("wds_entity_manager::create_and_add_box_trigger");

    if constexpr (1) {
        auto *new_trigger = (box_trigger *) trigger_manager::instance->new_box_trigger(a1, a3);
        new_trigger->set_box_info(a4);
        return new_trigger;
    } else {
        return (box_trigger *) THISCALL(0x005C2D80, this, a1, &a3, &a4);
    }
}

entity *wds_entity_manager::add_to_entities(_std::vector<entity *> *vec, entity *a3) {
    TRACE("wds_entity_manager::add_to_entities");

    return (entity *) THISCALL(0x005DFAB0, this, vec, a3);
}

int wds_entity_manager::add_entity_internal(_std::vector<entity *> *vec, entity *ent) {
    TRACE("wds_entity_manager::add_entity_internal");

    if constexpr (1) {
        this->add_to_entities(vec, ent);

        if constexpr (0)
        {
            FUNC_ADDRESS(address, &entity_base::get_entity_size);
            FUNC_ADDRESS(address1, &entity_base::is_alive);

            sp_log("0x%08X 0x%08X", (int) address, (int) address1);
            sp_log("0x%08X", ent->m_vtbl);
        }

        auto result = ent->get_flavor() - 4;

        switch (result) {
        case 0:
        case 1:
        case 4:
        case 5:
        case 8:
        case 10:
        case 21:
        case 22: {
            ent->field_4 &= 0xFFFFDFFF;
            break;
        }
        default:
            break;
        }

        return result;
    } else {
        return THISCALL(0x005DFB10, this, vec, ent);
    }
}

void wds_entity_manager::add_camera(_std::vector<entity *> *vec, camera *a2) {
    TRACE("wds_entity_manager::add_camera");

    this->add_entity_internal(vec, a2);
}

void wds_entity_manager::add_mic(_std::vector<entity *> *a1, mic *a2)
{
    this->add_entity_internal(a1, a2);
}

void wds_entity_manager::process_time_limited_entities(Float a2) {
    TRACE("wds_entity_manager::process_time_limited_entities");

    THISCALL(0x005D92D0, this, a2);
}

void wds_entity_manager::check_water(Float) {}

void wds_entity_manager::frame_advance(Float a2) {
    TRACE("wds_entity_manager::frame_advance");

    this->process_time_limited_entities(a2);

    this->check_water(a2);
}

void wds_entity_manager_patch()
{
    {
        FUNC_ADDRESS(address, &wds_entity_manager::add_entity_internal);
        //SET_JUMP(0x005DFB10, address);
    }

    {
        FUNC_ADDRESS(address, &wds_entity_manager::add_ent_to_lists);
        REDIRECT(0x005E0B1C, address);
        REDIRECT(0x0055AC96, address);
    }

    if constexpr (0) {
        {

            entity * (wds_entity_manager::*func)(string_hash, uint32_t) = &wds_entity_manager::acquire_entity;
            FUNC_ADDRESS(address, func);
            REDIRECT(0x00635795, address);
        }

        {
            FUNC_ADDRESS(address, &wds_entity_manager::add_camera);
            REDIRECT(0x0055B94E, address);
        }

    }
}
