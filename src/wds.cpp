#include "wds.h"

#include "aeps.h"
#include "ai_path.h"
#include "ai_player_controller.h"
#include "base_ai_core.h"
#include "beam.h"
#include "box_trigger.h"
#include "camera.h"
#include "debug_render.h"
#include "collide_aux.h"
#include "collide_trajectories.h"
#include "collision_trajectory_filter.h"
#include "common.h"
#include "conglom.h"
#include "convex_box.h"
#include "cut_scene_player.h"
#include "damage_interface.h"
#include "daynight.h"
#include "debugutil.h"
#include "decal_data_interface.h"
#include "decal_morphs.h"
#include "dirty_sphere.h"
#include "dynamic_rtree.h"
#include "entity_mash.h"
#include "event.h"
#include "facial_expression_interface.h"
#include "femanager.h"
#include "fixed_pool.h"
#include "force_generator.h"
#include "func_wrapper.h"
#include "fx_cache.h"
#include "game.h"
#include "grenade.h"
#include "hierarchical_entity_proximity_map.h"
#include "interactable_interface.h"
#include "intersected_trajectory.h"
#include "intraframe_trajectory.h"
#include "item.h"
#include "light_source.h"
#include "limbo_area.h"
#include "limbo_entities.h"
#include "line_info.h"
#include "loaded_regions_cache.h"
#include "manip_obj.h"
#include "marky_camera.h"
#include "mash_virtual_base.h"
#include "memory.h"
#include "motion_effect_struct.h"
#include "moved_entities.h"
#include "nal_system.h"
#include "nearby_hero_regions.h"
#include "osassert.h"
#include "ped_spawner.h"
#include "physical_interface.h"
#include "poi.h"
#include "polytube.h"
#include "region.h"
#include "region_intersect_visitor.h"
#include "resource_key.h"
#include "resource_manager.h"
#include "scene_brew.h"
#include "scene_entity_brew.h"
#include "scene_spline_path_brew.h"
#include "scratchpad_stack.h"
#include "script.h"
#include "script_lib_list.h"
#include "script_manager.h"
#include "sound_interface.h"
#include "spawnable.h"
#include "spiderman_camera.h"
#include "subdivision_node_obb_base.h"
#include "stack_allocator.h"
#include "thrown_item.h"
#include "terrain.h"
#include "time_interface.h"
#include "trace.h"
#include "traffic_signal_mgr.h"
#include "trajectory_cluster.h"
#include "trigger_manager.h"
#include "utility.h"
#include "vtbl.h"
#include "wds.h"
#include "web_interface.h"
#include "worldly_pack_slot.h"

#include <list>

VALIDATE_SIZE(world_dynamics_system, 0x400u);
VALIDATE_SIZE((*world_dynamics_system::field_0), 0x3C);
VALIDATE_OFFSET(world_dynamics_system, ent_mgr, 0x74);
VALIDATE_OFFSET(world_dynamics_system, the_terrain, 0x1AC);

static constexpr auto ENTITIES_TAG = 0;
static constexpr auto BOX_TRIGGERS_TAG = 6;
static constexpr auto SPLINE_PATHS_TAG = 7;
static constexpr auto AUDIO_BOXES_TAG = 8;
static constexpr auto REGION_MESH_VOBBS_TAG = 13;

world_dynamics_system *& g_world_ptr = var<world_dynamics_system *>(0x0095C770);

world_dynamics_system::world_dynamics_system()
    : field_4(), field_3E0()
{
    if constexpr (1)
    {
        this->field_0 = new slot_pool<nal_anim_control *, uint32_t>{500};

        this->field_4.reserve(20u);

        this->m_loading_from_scn_file = false;
        this->field_230[0] = nullptr;
        this->field_234[0] = nullptr;
        this->field_298 = -1;
        this->field_290 = 4;
        this->the_terrain = nullptr;
        this->num_players = 0;
        mash_virtual_base::generate_vtable();
        physical_interface::clear_static_lists();
        this->field_3F0 = true;
    }
    else
    {
        THISCALL(0x005554D0, this);
    }
}

world_dynamics_system::~world_dynamics_system()
{
    if constexpr (1)
    {
        this->field_23C.clear();
        debug_render_done();

        for ( auto &gen : this->field_260 )
        {
            if ( gen != nullptr )
            {
                void (__fastcall *finalize)(void *, void *, int) = CAST(finalize, get_vfunc(gen->m_vtbl, 0x0));
                finalize(gen, nullptr, 1);
            }
        }

        this->field_260.clear();

        this->field_230[0] = nullptr;

        if ( this->the_terrain != nullptr )
        {
            this->the_terrain->~terrain();
            operator delete(this->the_terrain);
        }

        this->field_230[0] = nullptr;
        this->field_234[0] = nullptr;
        g_spiderman_camera_ptr() = nullptr;
        script::gso() = nullptr;
        script::gsoi() = nullptr;
        destroy_script_lists();

        thrown_item::all_grenades().clear();

        ped_spawner::cleanup();
        poi_manager::cleanup();

        this->num_players = 0;

    } else {
        THISCALL(0x00555750, this);
    }
}

void world_dynamics_system::malor_point(const vector3d &xyz, int a3, bool a4)
{
    if constexpr (0) {
        auto *hero_ptr = this->get_hero_ptr(a3);
        entity_teleport_abs_position(hero_ptr, xyz, a4);
        if ( this->the_terrain != nullptr )
        {
            auto *reg = this->the_terrain->find_region(xyz, nullptr);
            if ( reg != nullptr ) {
                hero_ptr->update_regions(&reg, 1);
            }
        }
    } else {
        THISCALL(0x00530460, this, &xyz, a3, a4);
    }
}

bool region_array::contains(region *a2) const
{
    for ( int i = 0; i < this->count; ++i )
    {
        if ( this->m_data[i] == a2 ) {
            return true;
        }
    }

    return false;
}

void region_array::push_back(region *a2)
{
    assert(count < MAX_REGIONS_IN_ARRAY);

    if ( this->count >= 30 ) {
        error("Region list overflow");
    } else {
        this->m_data[this->count++] = a2;
    }
}

void build_region_list_radius(region_array *arr, region *reg, const vector3d &a3, Float a4, bool a5)
{
    if ( reg != nullptr && arr != nullptr && !arr->contains(reg) )
    {
        if ( !a5 || reg->is_loaded() ) {
            arr->push_back(reg);
        }

        for ( auto i = 0; i < reg->get_num_neighbors(); ++i )
        {
            auto *neighbor = reg->get_neighbor(i);
            if ( !a5 || neighbor->is_loaded() )
            {
                assert(neighbor != nullptr);

                auto *aabb_ptr = neighbor->obb;
                assert(aabb_ptr != nullptr);
                if ( aabb_ptr->sphere_intersection(a3, a4) ) {
                    build_region_list_radius(arr, neighbor, a3, a4, a5);
                }
            }
        }
    }
}

void cleanup_actor_scene_anim_state_hash() {
    CDECL_CALL(0x004D00E0);
}

int world_dynamics_system::add_generator(force_generator *generator) {
    return THISCALL(0x005421B0, this, generator);
}

void world_dynamics_system::advance_entity_animations(Float a3) {
    TRACE("world_dynamics_system::advance_entity_animations");

    if constexpr (1)
    {}
    else
    {
        THISCALL(0x00537170, this, a3);
    }
}

void zero_xz_velocity_for_effectively_standing_physical_interfaces()
{
    TRACE("zero_xz_velocity_for_effectively_standing_physical_interfaces");

    if constexpr (1)
    {
        if ( physical_interface::all_phys_interfaces == nullptr || physical_interface::all_phys_interfaces->empty() )
        {
            return;
        }

        for ( auto &v3 : (*physical_interface::all_phys_interfaces) )
        {
            auto act = v3->get_actor();
            if ( !act->playing_scene_anim() && !act->is_in_limbo() )
            {
                if ( v3->is_effectively_standing() )
                {
                    if ( !v3->is_biped_physics_running() )
                    {
                        if ( !v3->is_prop_physics_running() )
                        {
                            vector3d vel = v3->get_velocity();

                            vel[2] = 0.0;
                            vel[0] = 0.0;

                            v3->set_velocity(vel, false);
                        }
                    }
                }
            }
        }
    }
    else
    {
        CDECL_CALL(0x004D1A30);
    }
}

int sub_A4B0D0()
{
    assert(intraframe_trajectory_t::pool().is_empty());

    assert(local_collision::closest_points_pair_t::pool.is_empty());

    assert(local_collision::primitive_list_t::pool.is_empty());

    assert(intersected_trajectory_t::pool().is_empty());

    assert(dirty_sphere_t::pool().is_empty());

    assert(trajectory_cluster_t::pool().is_empty());
    return 0;
}

void collide_all_moved_entities(Float a1) {
    TRACE("collide_all_moved_entities");

    if constexpr (0) {
        stack_allocator allocator;
        scratchpad_stack::save_state(&allocator);
        //sub_A4B0D0();
        collision_trajectory_filter_t filter{};
        auto *trajectories = moved_entities::get_all_trajectories(a1, filter);

        fixed_vector<intraframe_trajectory_t *, 300> v10{};

        for (auto *i = trajectories; i != nullptr; i = i->field_15C) {
            v10.push_back(i);
        }

        auto v4 = 0u;
        for (auto *v5 = trajectories; v5 != nullptr; v5 = v5->field_15C) {
            ++v4;
        }

        assert( v10.size() == v4 );

        if (trajectories != nullptr) {
            resolve_rotations(trajectories, v4);
            resolve_collisions(&trajectories, a1);
            resolve_moving_pendulums(trajectories, a1);
        }

        for (auto j = 0u; j < v10.size(); ++j) {
            auto *v7 = v10.at(j);
            if (!sub_512730(v7)) {
                intraframe_trajectory_t::pool().remove(v7);
            }
        }

        //sub_A4B0D0();
        scratchpad_stack::restore_state(allocator);

    } else {
        CDECL_CALL(0x00605470, a1);
    }
}

void manage_standing_for_all_physical_interfaces(Float a1) {
    TRACE("manage_standing_for_all_physical_interfaces");

    CDECL_CALL(0x004F28B0, a1);
}

entity *world_dynamics_system::get_hero_ptr(int index)
{
    assert(index >= 0 && index <= MAX_GAME_PLAYERS);

    auto *result = this->field_230[index];
    return result;
}

void world_dynamics_system::frame_advance(Float a2)
{
    TRACE("world_dynamics_system::frame_advance");

    // The native path is retained for continued reverse engineering, but it
    // currently corrupts entity-tracker heap state in the prerelease runtime.
    // Delegate to the complete stock implementation for production loaders.
    if constexpr (0)
    {
        this->field_158.frame_advance(a2);
        this->field_28.frame_advance(a2);
        this->field_A0.frame_advance(a2);
        this->field_188.frame_advance(a2);
        daynight::frame_advance(a2);

        traffic_signal_mgr::frame_advance(a2);

        this->ent_mgr.frame_advance(a2);
        cleanup_actor_scene_anim_state_hash();
        this->update_ai_and_visibility_proximity_maps_for_moved_entities(a2);
        moved_entities::reset_all_moved();
        script_manager::run(a2, false);
        this->field_28.advance_controllers(a2);
        this->advance_entity_animations(a2);
        time_interface::frame_advance_all_time_interfaces(a2);
        spawnable::advance_traffic_and_peds(a2);
        if (this->field_3F0) {
            ai::ai_core::frame_advance_all_core_ais(a2);
        }

        ai_path::frame_advance_all_ai_paths(a2);
        interactable_interface::frame_advance_all(a2);
        facial_expression_interface::frame_advance_all_facial_expression_ifc(a2);
        this->field_3A8.frame_advance(a2);
        this->field_1B0.frame_advance(a2);
        this->field_1F0.frame_advance(a2);

        for (auto &generator : this->field_260)
        {
            if (generator->is_active()) {
                generator->frame_advance(a2);
            }
        }

        physical_interface::frame_advance_all_phys_interfaces(a2);
        manage_standing_for_all_physical_interfaces(a2);
        zero_xz_velocity_for_effectively_standing_physical_interfaces();
        collide_all_moved_entities(a2);

        line_info::frame_advance(2);
        beam::frame_advance_all_beams(a2);
        item::frame_advance_all_items(a2);
        grenade::frame_advance_all_grenades(a2);
        manip_obj::frame_advance_all_manip_objs(a2);
        polytube::frame_advance_all_polytubes(a2);
        motion_effect_struct::record_all_motion_fx(a2);

        aeps::FrameAdvance(a2);
        sound_interface::frame_advance_all_sound_ifc(a2);
        damage_interface::frame_advance_all_damage_ifc(a2);
        decal_data_interface::frame_advance_all_decal_interfaces(a2);
        web_interface::frame_advance_all_web_interfaces(a2);
        this->update_collision_proximity_maps_for_moved_entities(a2);
        this->the_terrain->frame_advance(a2);
        trigger_manager::instance->update();
        this->sub_54A3B0();
        decal_morphs::frame_advance(a2);

    } else {
        THISCALL(0x00558370, this, a2);
    }
}

bool world_dynamics_system::is_entity_in_water(vhandle_type<entity> a1)
{
    TRACE("world_dynamics_system::is_entity_in_water");

    return THISCALL(0x0052FE00, this, a1);
}

void world_dynamics_system::entity_sinks(vhandle_type<entity> a2)
{
    TRACE("world_dynamics_system::entity_sinks");

    THISCALL(0x0054A2E0, this, a2);
}

void world_dynamics_system::sub_54A3B0()
{
    if constexpr (0) {
        for ( auto &v1 : this->field_254 )
        {
            vhandle_type<entity> v4 {v1};
            if ( this->is_entity_in_water(v4) ) {
                this->entity_sinks(v4);
            }
        }

        this->field_254.clear();
    } else {
        THISCALL(0x0054A3B0, this);
    }
}

void world_dynamics_system::sub_530460(const vector3d &a2, int visited_regions, bool a4) {
    THISCALL(0x00530460, this, &a2, visited_regions, a4);
}

bool world_dynamics_system::un_mash_scene_entities(const resource_key &a2, region *reg, worldly_pack_slot *slot_ptr, bool a5, scene_entity_brew &brew)
{
    TRACE("world_dynamics_system::un_mash_scene_entities");

    if constexpr (0)
    {
        if (brew.field_0.is_done()) {
            return false;
        }

        if (!brew.field_0.is_started()) {
            brew.field_0.start();
            brew.field_8 = slot_ptr;
            brew.field_C = 0;
            brew.field_10 = nullptr;
            brew.buffer_index = 0;
            brew.field_1C = 0;
            if (!resource_manager::get_resource_if_exists(a2, reg, &brew.field_10, brew.field_8, &brew.field_C)) {
                brew.field_0.done();
                return false;
            }

            brew.parse_code = *(int *)&brew.field_10[brew.buffer_index];
            assert(brew.parse_code == REGION_MESH_VOBBS_TAG);
            brew.buffer_index += sizeof(int);
            int v91 = *(int *)&brew.field_10[brew.buffer_index];
            brew.buffer_index += sizeof(int);

            brew.buffer_index = (brew.buffer_index + 15) & 0xFFFFFFF0;
            if ( v91 ) {
                reg->field_38 = v91;
                reg->vobbs_for_region_meshes = (int *)&brew.field_10[brew.buffer_index];
                brew.buffer_index += 0x30 * v91;
            }

            brew.parse_code = *(int *)&brew.field_10[brew.buffer_index];
            assert(brew.parse_code == ENTITIES_TAG);

            brew.buffer_index += sizeof(int);
            brew.field_3C = *(int *)&brew.field_10[brew.buffer_index];
            brew.buffer_index += sizeof(int);
            brew.field_24 = *(int *)&brew.field_10[brew.buffer_index];
            brew.buffer_index += sizeof(int);
            assert((brew.buffer_index % 4) == 0);
        }

        auto v90 = brew.field_C;
        auto *buffer_ptr = (char *)brew.field_10;
        auto buffer_index = brew.buffer_index;
        auto v87 = brew.field_1C;
        auto parse_code = brew.parse_code;
        auto v85 = brew.field_24;
        g_femanager.RenderLoadMeter(false);
        if ( reg != nullptr ) {
            reg->field_60 = brew.field_3C;
        }

        auto sub_692686 = [](int &a1, int a2) -> void {
            auto v2 = a1 % a2;
            if ( a2 - v2 < a2 ) {
                a1 += a2 - v2;
            }
        };

        auto sub_6A3981 = [](auto &a1) -> bool {
            auto sub_691AF1 = [](limited_timer *self) -> bool {
                return self->elapsed() >= self->field_4;
            };

            return a1.field_4 != nullptr && sub_691AF1(a1.field_4);
        };

        auto *ent_vec_ptr = slot_ptr->get_entity_instances();
        auto *item_vec_ptr = slot_ptr->get_item_instances();
        auto *box_trigger_instances = slot_ptr->get_box_trigger_instances();
        limited_timer_base v81, v80, v79, v78;
        static auto dword_1568498 = 0.0;
        static auto dword_156849C = 0.0;
        static auto dword_15684A0 = 0.0;
        static auto dword_15684A4 {0.0};
        [[maybe_unused]] auto dword_15684A8 = 0;

        auto sub_68D9F1 = [](limited_timer_base &a1) -> double {
            auto result = a1.elapsed();
            a1.reset();
            return result;
        };

        auto sub_6A4BE7 = [](limited_timer_base &a1) -> double {
            return a1.elapsed();
        };

        v81.reset();
        std::list<region *> list_regions {};
        for ( ; v87 < v85; ++v87 )
        {
            assert((buffer_index % 4) == 0);

            if ( !brew.field_28.is_started() )
            {
                brew.field_28.start();
                brew.field_2C = *(int *)&buffer_ptr[buffer_index];
                buffer_index += sizeof(int);
                brew.field_30 = 0;
                brew.field_34 = 0;
            }

            auto v75 = brew.field_2C;
            auto v74 = brew.field_30;
            auto *a6 = (void *)brew.field_34;
            while ( v74 < v75 )
            {
                list_regions.clear();
                assert((buffer_index % 4) == 0);

                int v72 = *(int *)&buffer_ptr[buffer_index];
                buffer_index += sizeof(int);

                sub_692686(buffer_index, 16);
                v80.reset();
                auto *tmp_e = parse_entity_mash(ent_vec_ptr, item_vec_ptr, &buffer_ptr[buffer_index], nullptr, a6, true);
                assert(tmp_e->is_an_entity());
                auto *ent_ptr = bit_cast<entity *>(tmp_e);
                if (reg != nullptr) {
                    ent_ptr->field_8 |= 0x200u;
                }

                if (ent_ptr->is_renderable()) {
                    ent_ptr->set_timer(0u);
                    ent_ptr->on_fade_distance_changed(ent_ptr->field_8 & 0xF);
                    if ( a5 && ((ent_ptr->field_8 & 0xF) == 15) )
                    {
                        ent_ptr->field_8 |= 0x200000u;
                        auto id = ent_ptr->get_id();
                        auto *v15 = id.to_string();
                        sp_log("found far away entity %s", v15);
                        this->field_23C.push_back(ent_ptr);
                    }
                    else
                    {
                        ent_ptr->field_8 &= 0x200000u;
                    }
                }

                if (a6 == nullptr) {
                    a6 = &buffer_ptr[buffer_index];
                }

                ent_ptr->field_8 |= 0x10u;
                dword_156849C += sub_68D9F1(v80);
                buffer_index += v72;
                sub_692686(buffer_index, 4);

                int v68 = *(int *)&buffer_ptr[buffer_index];
                buffer_index += sizeof(int);

                sub_692686(buffer_index, 8);

                auto *strings = bit_cast<fixedstring<8> *>(buffer_ptr + buffer_index);

                std::for_each(strings, strings + v68, [this,
                                                            ent_ptr,
                                                            &list_regions](auto &v66) {

                    auto *v18 = v66.to_string();
                    auto *found_region = this->the_terrain->find_region(string_hash {v18});
                    if ( found_region != nullptr)
                    {
                        list_regions.push_back(found_region);
                    }
                    else
                    {
                        const char *str = nullptr;
                        if ( ent_ptr != nullptr )
                        {
                            auto id = ent_ptr->get_id();
                            str = id.to_string();
                        }
                        else
                        {
                            str = "<NULL>";
                        }

                        mString v33 {"force_region: unknown region "};
                        auto v32 = v33 + v66.to_string();
                        auto v30 = v32 + " for entity ";
                        auto v28 = v30 + str;
                        auto v26 = v28 + "'";
                        sp_log(v26.c_str());
                        found_region = this->the_terrain->get_region(0);
                    }
                });

                buffer_index += sizeof(fixedstring<8>) * v68;

                v79.reset();
                {
                    assert(ent_ptr != nullptr);
                    v78.reset();
                    g_world_ptr->ent_mgr.add_ent_to_lists(ent_vec_ptr, item_vec_ptr, ent_ptr);
                    dword_15684A4 += sub_68D9F1(v78);
                    //sp_log("adding entities to lists = %f sec", dword_15684A4);

                    check_po(ent_ptr);
                    if ( reg != nullptr )
                    {
                        if ( ent_ptr->is_a_light_source() )
                        {
                            bit_cast<light_source *>(ent_ptr)->force_region_hack(reg);
                            auto *v22 = bit_cast<light_source *>(ent_ptr);
                            reg->add(v22);
                        }
                        else if (reg->collision_proximity_map != nullptr)
                        {
                            bit_cast<entity *>(ent_ptr)->force_region_hack(reg);
                            auto *v22 = bit_cast<entity *>(ent_ptr);
                            reg->add(v22);
                        }
                    }
                }

                dword_15684A0 += sub_68D9F1(v79);
                if ( sub_6A3981(brew) )
                {
                    brew.field_8 = slot_ptr;
                    brew.field_C = v90;
                    brew.field_10 = CAST(brew.field_10, buffer_ptr);
                    brew.buffer_index = buffer_index;
                    brew.field_1C = v87;
                    brew.parse_code = parse_code;
                    brew.field_24 = v85;
                    brew.field_2C = v75;
                    brew.field_34 = (int) a6;
                    brew.field_30 = v74 + 1;
                    return true;
                }

                ++v74;
            }

            brew.field_28.clear();
        }

        dword_1568498 = sub_68D9F1(v81);
        //sp_log("dword_1568498 = %f seconds", dword_1568498);

        if (!brew.field_0.is_done() && !brew.field_38.is_started())
        {
            brew.field_38.start();
            parse_code = *(int *)&buffer_ptr[buffer_index];
            buffer_index += sizeof(parse_code);
        }

        while ( !brew.field_38.is_done() && parse_code != 18 )
        {
            sp_log("parse_code = %d", parse_code);
            switch ( parse_code )
            {
            case 3u: {
                limited_timer_base v58{};
                auto dword_15684AC = 0.0;
                v58.reset();

                assert(the_terrain != nullptr);
                if ( reg == nullptr )
                {
                    auto &a3 = a2.m_hash;
                    auto *v24 = a3.to_string();
                    sp_log("Unknown region %s.  Make sure your sin file is correct.", v24);
                    assert(0);
                }

                int v57;
                this->the_terrain->un_mash_obb(&buffer_ptr[buffer_index], &v57, reg);
                buffer_index += v57;
                dword_15684AC = sub_6A4BE7(v58);
                break;
            }
            case 6u: {
                assert((buffer_index % 4) == 0);

                int a4;
                this->un_mash_box_triggers(
                    parse_code,
                    &buffer_ptr[buffer_index],
                    box_trigger_instances,
                    &a4);
                buffer_index += a4;
                break;
            }
            case 9u: {
                limited_timer_base v60{};
                auto dword_15684BC = 0.0;
                v60.reset();
                assert(the_terrain != nullptr);

                if ( reg == nullptr )
                {
                    auto &a3 = a2.m_hash;
                    auto *v23 = a3.to_string();
                    sp_log("Unknown region %s.  Make sure your sin file is correct.", v23);
                    assert(0);
                }

                int v59;
                this->the_terrain->un_mash_texture_to_frame(&buffer_ptr[buffer_index], &v59, reg);
                buffer_index += v59;
                dword_15684BC = sub_6A4BE7(v60);
                break;
            }
            case 11u: {
                assert(the_terrain != nullptr);
                sub_692686(buffer_index, 8);

                int v61;
                reg->un_mash_lego_map(&buffer_ptr[buffer_index], &v61);
                buffer_index += v61;
                break;
            }
            case 14u: {
                auto v54 = *(int *)&buffer_ptr[buffer_index];
                buffer_index += sizeof(int);

                auto *v55 = &buffer_ptr[buffer_index];
                if ( reg != nullptr ) {
                    reg->field_3C = (int)v55;
                }

                buffer_index += v54;
                buffer_index = (buffer_index + 3) & 0xFFFFFFFC;
                break;
            }
            case 15u: {
                int fade_groups_count = *(int *)&buffer_ptr[buffer_index];
                assert(fade_groups_count >= 0 && fade_groups_count < 255);

                if ( reg != nullptr ) {
                    reg->m_fade_groups_count = fade_groups_count;
                }
                
                buffer_index += sizeof(int);
                if ( fade_groups_count > 0 )
                {
                    if ( reg != nullptr ) {
                        reg->field_44 = (int)&buffer_ptr[buffer_index];
                    }

                    buffer_index += 4 * fade_groups_count;
                    buffer_index = (buffer_index + 15) & 0xFFFFFFF0;
                    if ( reg != nullptr ) {
                        reg->field_48 = (int)&buffer_ptr[buffer_index];
                    }

                    buffer_index += 16 * fade_groups_count;
                }
                break;
            }
            case 17u: {
                auto v51 = *(int *)&buffer_ptr[buffer_index];
                buffer_index += sizeof(int);

                auto *v52 = &buffer_ptr[buffer_index];
                if ( reg != nullptr ) {
                    reg->field_3C = (int)v52;
                }

                buffer_index += v51;
                buffer_index = (buffer_index + 3) & 0xFFFFFFFC;
                break;
            }
            default:
                debug_print_va("Unknown parse code 0x%x\n", parse_code);
                assert(0);

                return false;
            }

            parse_code = *(int *)&buffer_ptr[buffer_index];
            buffer_index += sizeof(parse_code);
            if ( sub_6A3981(brew) )
            {
                brew.field_8 = slot_ptr;
                brew.field_C = v90;
                brew.field_10 = CAST(brew.field_10, buffer_ptr);
                brew.buffer_index = buffer_index;
                brew.field_1C = v87;
                brew.parse_code = parse_code;
                brew.field_24 = v85;
                return true;
            }
        }

        brew.field_38.done();
        brew.field_0.done();
        return false;
    }
    else
    {
#ifdef OPENUSM_XBPACK_V10
        if (brew.field_0.is_done()) {
            return false;
        }

        if (!brew.field_0.is_started()) {
            brew.field_0.start();
            brew.field_8 = slot_ptr;
            brew.field_C = 0;
            brew.field_10 = nullptr;
            brew.buffer_index = 0;
            brew.field_1C = 0;
            if (!resource_manager::get_resource_if_exists(
                    a2, reg, &brew.field_10, brew.field_8, &brew.field_C)) {
                brew.field_0.done();
                return false;
            }

            brew.parse_code = *reinterpret_cast<int *>(brew.field_10);
            assert(brew.parse_code == ENTITIES_TAG);
            brew.field_3C = *reinterpret_cast<int *>(brew.field_10 + 4);
            brew.field_24 = *reinterpret_cast<int *>(brew.field_10 + 8);
            brew.buffer_index = 12;
        }
#endif

        bool (__fastcall *func)(void *, void *, const resource_key *, region *, worldly_pack_slot *, bool , scene_entity_brew *) = CAST(func, 0x0055A680);
        return func(this, nullptr, &a2, reg, slot_ptr, a5, &brew);
    }
}

bool world_dynamics_system::un_mash_scene_box_triggers(const resource_key &a1, region *reg, worldly_pack_slot *slot_ptr, timed_progress *a4) {
    TRACE("world_dynamics_system::un_mash_scene_box_triggers");

    if constexpr (1) {
        if ( a4->is_done() ) {
            return false;
        }

        int parse_code;
        int mash_data_size = 0;
        uint8_t *buffer_ptr = nullptr;
        int buffer_index = 0;
        if ( !resource_manager::get_resource_if_exists(a1, reg, &buffer_ptr, slot_ptr, &mash_data_size)
            || (g_femanager.RenderLoadMeter(false),
                parse_code = *(uint32_t *)&buffer_ptr[buffer_index],
                parse_code == 18) )
        {
            a4->done();
            return false;
        }
        else
        {
            assert(parse_code == BOX_TRIGGERS_TAG);
            buffer_index += 4;
            limited_timer v8{};
            auto dword_15684C0 = 0.0;
            v8.reset();
            assert(( buffer_index % 4 ) == 0);

            auto *box_trigger_instances = slot_ptr->get_box_trigger_instances();
            int size;
            this->un_mash_box_triggers(parse_code, (char *)&buffer_ptr[buffer_index], box_trigger_instances, &size);

            assert(size + sizeof( parse_code ) == mash_data_size);

            dword_15684C0 = v8.elapsed();
            a4->done();
            return false;
        }
    } else {
        return (bool) THISCALL(0x005507F0, this, &a1, reg, slot_ptr, a4);
    }
}

void world_dynamics_system::register_water_exclusion_trigger(box_trigger *trig)
{
    TRACE("world_dynamics_system::register_water_exclusion_trigger");

    assert(trig != nullptr);

    if constexpr (1) {
        trig->field_4 |= 0x10u;
        trig->set_multiple_entrance(true);
        trig->field_4 |= 0x20000u;

        uint32_t v3 = trig->my_handle.field_0;
        this->field_248.push_back(v3);
    } else {
        THISCALL(0x0053CC90, this, trig);
    }
}

bool world_dynamics_system::un_mash_box_triggers(
        int parse_code,
        char *a3,
        _std::vector<box_trigger *> *box_trigger_vec_ptr,
        int *a5) {
    TRACE("world_dynamics_system::un_mash_box_triggers");

    if constexpr (0) {
        assert(parse_code == BOX_TRIGGERS_TAG);

        int buffer_index = 0;
        int v19 = *(int *)a3;
        buffer_index = 4;
        for ( auto i = 0u; i < v19; ++i ) {
            assert((buffer_index % 4) == 0);

            auto *v18 = (fixedstring<8> *)&a3[buffer_index];
            buffer_index += sizeof(*v18);
            assert((buffer_index % 4) == 0);

            auto *v17 = (int *) &a3[buffer_index];
            buffer_index += sizeof(*v17);
            assert((buffer_index % 4 ) == 0);

            auto *v16 = (convex_box *)&a3[buffer_index];
            buffer_index += sizeof(*v16);
            assert((buffer_index % 4) == 0);

            auto *a2 = (vector3d *)&a3[buffer_index];
            buffer_index += sizeof(*a2);

            auto &v10 = *v16;
            auto &v9 = *a2;
            auto *v5 = v18->to_string();
            string_hash v8 {v5};
            auto *v14 = g_world_ptr->ent_mgr.create_and_add_box_trigger(v8, v9, v10);
            if ( v14 != nullptr ) {
                assert(box_trigger_vec_ptr != nullptr);

                box_trigger_vec_ptr->push_back(v14);
                if ( (*v17 & 1) != 0 ) {
                    this->register_water_exclusion_trigger(v14);
                }
            }
        }

        *a5 = buffer_index;
        return true;
    } else {
        return (bool) THISCALL(0x0054A1C0, this, parse_code, a3, box_trigger_vec_ptr, a5);
    }
}

bool world_dynamics_system::un_mash_scene_audio_boxes(const resource_key &key_id,
                                                      region *reg,
                                                      worldly_pack_slot *slot_ptr,
                                                      timed_progress &a4) {
    TRACE("world_dynamics_system::un_mash_scene_audio_boxes");

    if constexpr (1) {
        if (a4.is_done()) {
            return false;
        }

        int mash_data_size = 0;
        int buffer_index = 0;
        uint8_t *buffer_ptr = nullptr;

        int parse_code;

        if (!resource_manager::get_resource_if_exists(key_id,
                                                      reg,
                                                      &buffer_ptr,
                                                      slot_ptr,
                                                      &mash_data_size) ||
            (g_femanager.RenderLoadMeter(false),
             parse_code = *(uint32_t *) &buffer_ptr[buffer_index],
             parse_code == 18)) {
        } else {
            assert(parse_code == AUDIO_BOXES_TAG);

            buffer_index += 4;

            assert(the_terrain != nullptr);

            if (reg == nullptr) {
                auto *str = key_id.m_hash.to_string();

                sp_log("Unknown region '%s'.  Make sure your sin file is correct.", str);
                assert(0);
            }

            int v9;
            this->the_terrain->un_mash_audio_boxes((char *) &buffer_ptr[buffer_index], &v9, reg);
            buffer_index += v9;
        }

        a4.done();

        return false;

    } else {
        return (bool) THISCALL(0x0053CB50, this, &key_id, reg, slot_ptr, &a4);
    }
}

bool world_dynamics_system::un_mash_scene_spline_paths(const resource_key &a2,
                                                       region *reg,
                                                       worldly_pack_slot *slot_ptr,
                                                       scene_spline_path_brew &brew) {
    TRACE("world_dynamics_system::un_mash_scene_spline_paths");

    if constexpr (1) {
        if (brew.field_0.is_done()) {
            return false;
        }

        if (!brew.field_0.is_started()) {
            brew.field_0.start();
            brew.field_8 = slot_ptr;
            brew.field_C = 0;
            brew.field_10 = nullptr;
            brew.field_14 = 0;
            if (!resource_manager::get_resource_if_exists(a2,
                                                          reg,
                                                          &brew.field_10,
                                                          brew.field_8,
                                                          &brew.field_C) ||
                (brew.parse_code = *(uint32_t *) &brew.field_10[brew.field_14],
                 brew.parse_code == 18)) {
                brew.field_0.done();
                return false;
            }

            assert(brew.parse_code == SPLINE_PATHS_TAG);

            brew.field_14 += 4;
        }

        for (uint32_t i = 0; i < nearby_hero_regions::regs().size(); ++i) {
            if (nearby_hero_regions::regs().at(i) == reg) {
                break;
            }
        }

        auto buffer_index = brew.field_14;
        auto v16 = brew.parse_code;
        auto *buffer_ptr = brew.field_10;
        auto a2a = brew.field_C;

        g_femanager.RenderLoadMeter(false);

        assert(the_terrain != nullptr);

        int v17;
        if (this->the_terrain->un_mash_traffic_paths((char *) &buffer_ptr[buffer_index],
                                                     &v17,
                                                     reg,
                                                     brew.field_1C)) {
            brew.field_10 = buffer_ptr;
            brew.field_8 = slot_ptr;
            brew.parse_code = v16;
            brew.field_C = a2a;
            brew.field_14 = buffer_index;
            return true;
        } else {
            buffer_index += v17;

            brew.field_0.done();
        }

        return false;

    } else {
        bool (__fastcall *func)(void *, int, const resource_key *a2,
                                region *reg,
                                worldly_pack_slot *slot_ptr,
                                scene_spline_path_brew *brew) = CAST(func, 0x0052FC90);
        return func(this, 0, &a2, reg, slot_ptr, &brew);
    }
}

bool world_dynamics_system::un_mash_scene_quad_paths(const resource_key &key_id,
                                                     region *reg,
                                                     worldly_pack_slot *slot_ptr,
                                                     timed_progress &a4) {
    TRACE("world_dynamics_system::un_mash_scene_quad_paths");
    if constexpr (1) {
        if (a4.is_done()) {
            return false;
        }

        int mash_data_size = 0;
        int buffer_index = 0;
        uint8_t *buffer_ptr = nullptr;

        int parse_code;

        if (!resource_manager::get_resource_if_exists(key_id,
                                                      reg,
                                                      &buffer_ptr,
                                                      slot_ptr,
                                                      &mash_data_size) ||
            (reg->flags |= 0x800,
             g_femanager.RenderLoadMeter(false),
             parse_code = *(uint32_t *) &buffer_ptr[buffer_index],
             parse_code == 18)) {
            a4.done();
            return false;
        } else {
            static constexpr auto QUAD_PATHS_TAG = 10;

            assert(parse_code == QUAD_PATHS_TAG);

            buffer_index += 4;

            assert(the_terrain != nullptr);

            if (reg == nullptr) {
                auto *str = key_id.m_hash.to_string();

                sp_log("Unknown region '%s'.  Make sure your sin file is correct.", str);
                assert(0);
            }

            int v9;
            this->the_terrain->un_mash_region_paths((char *) &buffer_ptr[buffer_index], &v9, reg);
            buffer_index += v9;
        }

        a4.done();

        return false;

    } else {
        return (bool) THISCALL(0x0053CAC0, this, &key_id, reg, slot_ptr, &a4);
    }
}

bool world_dynamics_system::load_scene(resource_key &a2,
                                       bool a3,
                                       const char *a4,
                                       region *reg,
                                       worldly_pack_slot *slot_ptr,
                                       limited_timer *a7) {
    TRACE("world_dynamics_system::load_scene");

    if (a4 != nullptr) {
        sp_log("Load scene %s", a4);
    }

    if constexpr (1) {
        scene_brew *found_brew = nullptr;
        int brew_idx = 0;
        for (auto i = 0u; i < this->scene_loads.size(); ++i) {
            auto &v = this->scene_loads[i]; 
            if (v.field_8 == a2) {
                found_brew = &v;
                brew_idx = i;
                break;
            }
        }

        if (found_brew == nullptr) {
            scene_brew new_brew {a2, a7};
            this->scene_loads.push_back(new_brew);

            found_brew = &this->scene_loads.back();
            brew_idx = this->scene_loads.size() - 1;
        }

        this->m_loading_from_scn_file = true;
        a2.set_type(RESOURCE_KEY_TYPE_SCN_ENTITY);
        if (this->un_mash_scene_entities(a2, reg, slot_ptr, a3, found_brew->field_10)) {
            return true;
        }

        a2.set_type(RESOURCE_KEY_TYPE_SCN_BOX_TRIGGER);
        if (this->un_mash_scene_box_triggers(a2, reg, slot_ptr, &found_brew->field_CC)) {
            return true;
        }

        if (a3) {
            a2.set_type(RESOURCE_KEY_TYPE_SCN_AI_SPLINE_PATH);
            auto res = this->un_mash_scene_spline_paths(a2, reg, slot_ptr, found_brew->field_50);
            if (res) {
                a2.set_type(RESOURCE_KEY_TYPE_SCN_ENTITY);
                return true;
            }
        }

        if (!a3) {
            a2.set_type(RESOURCE_KEY_TYPE_SCN_QUAD_PATH);
            if (!this->un_mash_scene_quad_paths(a2, reg, slot_ptr, found_brew->field_B4)) {
                a2.set_type(RESOURCE_KEY_TYPE_SCN_AUDIO_BOX);
                if (this->un_mash_scene_audio_boxes(a2, reg, slot_ptr, found_brew->field_C4)) {
                    a2.set_type(RESOURCE_KEY_TYPE_SCN_ENTITY);
                    return true;
                }
            }

            a2.set_type(RESOURCE_KEY_TYPE_SCN_ENTITY);
        }

        if (a3) {
            this->field_A0.init_level(a4);
            auto local_dc = a2;
            local_dc.set_type(RESOURCE_KEY_TYPE_TOKEN_LIST);
            this->field_188.initialize(local_dc);
        }

        assert(brew_idx >= 0 && brew_idx < scene_loads.size());

        auto size = this->scene_loads.size();
        auto &last_scene = this->scene_loads[size - 1];
        auto &v11 = this->scene_loads[brew_idx];
        THISCALL(0x0055F550, &v11, &last_scene);
        this->scene_loads.pop_back();
        return false;
    } else {
        auto result = static_cast<bool>(THISCALL(0x0055B160, this, &a2, a3, a4, reg, slot_ptr, a7));
        return result;
    }
}

void world_dynamics_system::set_chase_cam_ptr(int index, game_camera *a3)
{
    assert(index >= 0 && index <= MAX_GAME_PLAYERS);

    this->field_234[index] = a3;
}

camera *world_dynamics_system::get_chase_cam_ptr(int a2) {
    auto v1 = this->field_234[a2];

    return bit_cast<camera *>(v1);
}

void wds_enter_water_trigger_callback(event *, entity_base_vhandle a2, void *)
{
    assert(g_world_ptr != nullptr);

    auto *ptr = a2.get_volatile_ptr();
    assert(ptr != nullptr);

    assert(ptr->is_a_trigger());

    auto *trig = bit_cast<trigger *>(ptr);
    auto *ent = trig->get_triggered_ent();

    auto v5 = ent->my_handle.field_0;
    auto *list = &g_world_ptr->field_254;
    auto *m_head = list->m_head;
    auto *Prev = m_head->_Prev;

    decltype(m_head) (_fastcall *_Buynode)(void *, void *edx, decltype(m_head), decltype(m_head), uint32_t *) = CAST(_Buynode, 0x006B78D0);
    auto *v8 = _Buynode(list, nullptr, m_head, Prev, &v5);

    void (__fastcall *sub_566E00)(void *, void *edx, uint32_t) = CAST(sub_566E00, 0x00566E00);
    sub_566E00(list, nullptr, 1u);

    m_head->_Prev = v8;
    v8->_Prev->_Next = v8;
}

void world_dynamics_system::create_water_kill_trigger()
{
    TRACE("world_dynamics_system::create_water_kill_trigger");

    if constexpr (1)
    {
        convex_box a3{};
        
        static constexpr auto stru_921BB4 = -3.0f;

        vector3d a2[8] {};
        a2[0][0] = -9999.0;
        a2[0][1] = -80.0;
        a2[0][2] = 9999.0;

        a2[1][0] = 9999.0;
        a2[1][1] = -80.0;
        a2[1][2] = 9999.0;

        a2[2][0] = 9999.0;
        a2[2][1] = -80.0;
        a2[2][2] = -9999.0;

        a2[3][0] = -9999.0;
        a2[3][1] = -80.0;
        a2[3][2] = -9999.0;

        a2[4][0] = -9999.0;
        a2[4][1] = stru_921BB4;
        a2[4][2] = 9999.0;

        a2[5][0] = 9999.0;
        a2[5][1] = stru_921BB4;
        a2[5][2] = 9999.0;

        a2[6][0] = 9999.0;
        a2[6][1] = stru_921BB4;
        a2[6][2] = -9999.0;

        a2[7][0] = -9999.0;
        a2[7][1] = stru_921BB4;
        a2[7][2] = -9999.0;
        a3.set_box_coords(a2);

        string_hash water_id {int(to_hash("WATER_TRIGGER"))};
        auto *trig = this->ent_mgr.create_and_add_box_trigger(water_id, ZEROVEC, a3);
        assert(trig != nullptr);

        trig->set_use_any_char(true);
        trig->set_multiple_entrance(true);
        trig->set_sees_dead_people(true);

        trig->add_callback(event::ENTER, wds_enter_water_trigger_callback, nullptr, false);
    }
    else
    {
        THISCALL(0x0054A430, this);
    }
}

int world_dynamics_system::add_player(const mString &a2)
{
    TRACE("world_dynamics_system::add_player");

    if constexpr (1)
    {
        if ( this->num_players < 1 )
        {
            if ( this->num_players == 0 )
            {
                auto *v3 = a2.c_str();
                g_game_ptr->load_hero_packfile(v3, false);
            }

            auto *marker = this->field_230[0];
            if ( marker == nullptr ) {
                marker = (entity *) find_marker(string_hash {"HERO_START"});
            }

            auto v82 = marker->get_abs_po();
            switch ( this->num_players )
            {
            case 0u:
                break;
            case 1u: {
                auto &x_facing = v82.get_x_facing();
                auto v40 = x_facing * 1.5f;
                auto &position = v82.get_position();
                auto v8 = position + v40;
                v82.set_position(v8);
                break;
            }
            case 2u: {
                auto &v9 = v82.get_x_facing();
                auto v40 = v9 * 1.5f;
                auto v11 = v82.get_position() - v40;
                v82.set_position(v11);
                break;
            }
            case 3u: {
                auto v40 = v82.get_z_facing() * 1.5f;
                auto v14 = v82.get_position() - v40;
                v82.set_position(v14);
                break;
            }
            default: {
                auto &v15 = v82.get_z_facing();
                auto v40 = v15 * 1.5f;
                auto v17 = v82.get_position() + v40;
                v82.set_position(v17);
                break;
            }
            }

            auto &y_facing = v82.get_y_facing();
            if ( y_facing != YVEC )
            {
                auto v81 = v82.get_z_facing();
                if ( v81.y <= 0.99000001 ) {
                    v81.y = 0.0;
                } else {
                    v81 = ZVEC;
                }

                v81.normalize();
                auto v80 = vector3d::cross(YVEC, v81);
                auto &v20 = v82.get_position();
                void (__fastcall *sub_48AA30)(void *, void *, const vector3d *, const vector3d *, const vector3d *, const vector3d *) = CAST(sub_48AA30, 0x0048AA30);

                po v45;
                sub_48AA30(&v45, nullptr, &v80, &YVEC, &v81, &v20);
                v82 = v45;

                void (__fastcall *sub_48D840)(void *) = CAST(sub_48D840, 0x0048D840);
                sub_48D840(&v82);
            }

            mString v79 = ( this->num_players >= 1
                            ? "HERO" + mString {this->num_players}
                            : mString {"HERO"}
                            );

            auto *__old_context = resource_manager::get_and_push_resource_context(RESOURCE_PARTITION_HERO);
            mString v62 {};
            auto *v23 = v79.c_str();

            string_hash v36 {v23};
            string_hash v35 {a2.c_str()};
            this->field_230[this->num_players] = g_world_ptr->ent_mgr.create_and_add_entity_or_subclass(
               v35,
               v36,
               v82,
               v62,
               1,
               nullptr);
            auto *v27 = this->field_230[this->num_players];
            auto v40 = v27->get_rel_position() + YVEC;
            auto *v29 = this->field_230[this->num_players];
            v29->set_abs_position(v40);
            resource_manager::pop_resource_context();
            
            assert(resource_manager::get_resource_context() == __old_context);

            mString v76 = ( this->num_players >= 1
                            ? "CHASE_CAM" + mString {this->num_players}
                            : mString {"CHASE_CAM"}
                            );

            string_hash v43 {v76.c_str()};
            auto *v31 = this->field_230[this->num_players];

            this->field_234[this->num_players] = new spiderman_camera {v43, v31};

            auto *v73 = this->field_234[this->num_players];
            g_world_ptr->ent_mgr.add_camera(nullptr, v73);

            if ( this->num_players == 0 ) {
                g_spiderman_camera_ptr() = CAST(g_spiderman_camera_ptr(), this->field_234[0]);
            }

            {
                auto *player = bit_cast<actor *>(this->field_230[this->num_players]);

                // A freshly created player can arrive with zero current health
                // even though its resource contains a nonzero maximum. Start it
                // at that maximum before the controller begins advancing it.
                if (player->has_damage_ifc()) {
                    auto *damage = player->damage_ifc();
                    damage->field_1FC.sub_48BFB0(damage->field_1FC.field_0[2]);
                }

                player->create_player_controller(this->num_players);
            }

            this->field_3E0 = a2;
            ++this->num_players;
        }

        return this->num_players;
    }
    else
    {
        return THISCALL(0x0055B400, this, &a2);
    }
}

void world_dynamics_system::deactivate_corner_web_splats()
{
    resource_key &v2 = this->field_1F0.field_8;
    if ( v2.is_set() )
    {
        v2 = {};

        this->field_1F0.field_10[0] = 0.0;
        this->field_1F0.field_10[2] = 0.0;
        this->field_1F0.field_10[1] = 0.0;
        this->field_1F0.field_38 = 0;

        fx_cache *v3 = this->field_1F0.field_30;
        auto &v5 = v3->field_8;
        if (!v5.field_7)
        {
            if (v5.m_data != nullptr) {
                CDECL_CALL(0x0082209A, v5.m_data);
            }
        }

        v5.m_data = nullptr;
        v5.m_size = 0;
    }
}

terrain *world_dynamics_system::create_terrain(const mString &a2) {
    TRACE("world_dynamics_system::create_terrain");

    if constexpr (1)
    {
        this->the_terrain = new terrain{a2};
        return this->the_terrain;
    }
    else
    {
        return (terrain *) THISCALL(0x0055B100, this, &a2);
    }
}

void world_dynamics_system::activate_corner_web_splats()
{
    TRACE("world_dynamics_system::activate_corner_web_splats");

    auto *act = bit_cast<actor *>(this->get_hero_ptr(0));

    auto *__old_context = resource_manager::push_resource_context(act->get_resource_context());

    resource_key a2 {string_hash {"websplat_crnr"}, RESOURCE_KEY_TYPE_ENTITY};

    this->field_1F0.field_8 = a2;

    this->field_1F0.field_10[0] = 15.0;
    this->field_1F0.field_10[1] = 0.33000001;
    this->field_1F0.field_10[2] = 0.5;

    this->field_1F0.field_38 = 5;
    this->field_1F0.fill_cache();
    resource_manager::pop_resource_context();

    assert(resource_manager::get_resource_context() == __old_context);
}

entity *world_dynamics_system::get_hero_or_marky_cam_ptr()
{
    if constexpr (1)
    {
        auto *result = this->get_hero_ptr(0);
        if (result == nullptr) {
            result = (entity *) this->field_28.field_44;
        }

        return result;
    } else {
        return (entity *) THISCALL(0x0050D310, this);
    }
}

void world_dynamics_system::activate_web_splats()
{
    TRACE("world_dynamics_system::activate_web_splats");
    
    if constexpr (0)
    {
        auto *act = bit_cast<actor *>(this->get_hero_ptr(0));

        auto *__old_context = resource_manager::push_resource_context(act->get_resource_context());
        string_hash v3{"spdywebsplatter"};

        resource_key a2;
        a2.set(v3, RESOURCE_KEY_TYPE_ENTITY);

        this->field_1B0.field_8 = a2;

        this->field_1B0.field_10[0] = 15.0;
        this->field_1B0.field_10[1] = 0.33000001;
        this->field_1B0.field_10[2] = 0.5;

        this->field_1B0.field_38 = 5;
        this->field_1B0.fill_cache();
        resource_manager::pop_resource_context();

        assert(resource_manager::get_resource_context() == __old_context);
    } else {
        THISCALL(0x0054ABE0, this);
    }
}

void world_dynamics_system::deactivate_web_splats()
{
    resource_key &v2 = this->field_1B0.field_8;
    if ( v2.is_set() )
    {
        v2 = {};

        this->field_1B0.field_10[0] = 0.0;
        this->field_1B0.field_10[1] = 0.0;
        this->field_1B0.field_10[2] = 0.0;

        this->field_1B0.field_38 = 0;
        fx_cache *v3 = this->field_1B0.field_30;
        auto *v5 = &v3->field_8;
        if (!v5->field_7)
        {
            if (v5->m_data != nullptr) {
                CDECL_CALL(0x0082209A, v5->m_data);
            }
        }

        v5->m_data = nullptr;
        v5->m_size = 0;
    }
}

bool world_dynamics_system::is_loading_from_scn_file() {
    return this->m_loading_from_scn_file;
}

int world_dynamics_system::remove_player(int player_num)
{
    // Some menu/mission teardown paths can request a removal after the last
    // player has already gone away.  The stock routine decrements zero to -1
    // and then indexes field_234[-1] (which aliases the hero slot), eventually
    // calling destroy_entity(nullptr).  Treat that duplicate removal as the
    // no-op it was intended to be and keep the player count normalized.
    if (this->num_players <= 0) {
        this->num_players = 0;
        return 0;
    }

    // The game supports removal only from the end of this fixed-size player
    // array.  Ignore a stale callback rather than indexing a different slot.
    if (player_num != this->num_players - 1) {
        return this->num_players;
    }

    cut_scene_player *v3 = g_cut_scene_player();
    v3->stop(nullptr);
    bool v4 = this->num_players-- == 1;
    if (v4) {
        this->field_28.field_44->sync(*(this->field_234[0]));
    }

    g_world_ptr->ent_mgr.destroy_entity(this->field_234[this->num_players]);
    this->field_234[this->num_players] = nullptr;
    bit_cast<actor *>(this->field_230[this->num_players])->destroy_player_controller();
    g_world_ptr->ent_mgr.destroy_entity(this->field_230[this->num_players]);
    this->field_230[this->num_players] = nullptr;
    if (this->num_players == 0)
    {
        g_game_ptr->unload_hero_packfile();
        this->field_234[0] = CAST(this->field_234[0], this->field_28.field_44);
        g_spiderman_camera_ptr() = nullptr;
    }

    auto *v6 = this->get_chase_cam_ptr(0);
    g_game_ptr->set_current_camera(v6, true);
    this->deactivate_web_splats();
    this->deactivate_corner_web_splats();
    return this->num_players;
}

void entity_get_max_visual_and_collision_bounding_sphere(
        entity *ent,
        vector3d *center_result,
        float *radius_result)
{
    auto visual_center = ent->get_visual_center();
    auto visual_radius = ent->get_visual_radius();
    if ( ent->is_an_actor() && ent->colgeom != nullptr )
    {
        auto colgeom_center = ent->get_colgeom_center();
        auto colgeom_radius = ent->get_colgeom_radius();
        merge_spheres(visual_center, visual_radius, colgeom_center, colgeom_radius, *center_result, *radius_result);
    }
    else
    {
        *center_result = visual_center;
        *radius_result = visual_radius;
    }
}

void world_dynamics_system::update_ai_and_visibility_proximity_maps_for_moved_entities(Float a1)
{
    TRACE("world_dynamics_system::update_ai_and_visibility_proximity_maps_for_moved_entities");

    if constexpr (0)
    {
        update_limbo_list_if_needed();

        static constexpr auto n_bytes = 2400u;
        auto entity_pointers = (entity **) scratchpad_stack::alloc(n_bytes);
        int num_entity_pointers = 0;

        for (int v6 = 0; v6 < moved_entities::moved_count; ++v6)
        {
            auto *v7 = &moved_entities::moved_list[v6];
            auto *ent = v7->get_volatile_ptr();
            if ( ent != nullptr )
            {
                assert(!(ent->is_a_conglomerate() && ((conglomerate *)ent)->is_cloned_conglomerate()));

                assert("regions_[ 0 ] can not be NULL when regions_[ 1 ] is not. "
                        && ( ent->regions[ 1 ] ? ent->regions[ 0 ] != nullptr : 1 ) );

                assert("regions_[ 0 ] and regions_[ 1 ] should not be NULL while extended_regions_ is not."
                        && ent->extended_regions != nullptr ? ent->regions[ 0 ] && ent->regions[ 1 ] : 1);

                assert(ent->extended_regions != nullptr ? ent->extended_regions->size() > 0 : 1);

                auto &abs_po = ent->get_abs_po();
                if ( !abs_po.is_valid(true, false) )
                {
                    mString a2a {"Message: "};
                    auto v23 = a2a + "Entity Assert";
                    auto v22 = v23 + "\n";
                    auto v21 = v22 + "File: ";
                    auto v19 = v21 + " : ";
                    auto v17 = v19 + "\n";
                    auto v16 = v17 + "Expression: ";
                    auto v15 = v16 + "ent->get_abs_po().is_valid()";
                    auto v14 = v15 + "\n";
                    entity_report(ent, v14, false);
                }

                assert(ent->get_abs_po().is_valid());

                entity_pointers[num_entity_pointers++] = ent;
            }
        }

        assert(num_entity_pointers + 2 < moved_entities::MAX_MOVED);

        if ( num_entity_pointers > 0 )
        {
            entity_pointers[num_entity_pointers] = entity_pointers[num_entity_pointers - 1];
            entity_pointers[num_entity_pointers + 1] = entity_pointers[num_entity_pointers - 1];
            auto *mem_for_visitor = scratchpad_stack::alloc(0x44);
            struct vec4_t {
                vector3d field_0;
                float field_C;
            };
            VALIDATE_SIZE(vec4_t, 0x10);

            vec4_t *vec4_pointers = CAST(vec4_pointers, scratchpad_stack::alloc(sizeof(vec4_t) * (num_entity_pointers + 1)));

            for (int k = 0; k < num_entity_pointers; ++k) 
            {
                auto *v1 = &vec4_pointers[k].field_0;
                auto *v2 = &vec4_pointers[k].field_C;
                entity_get_max_visual_and_collision_bounding_sphere(entity_pointers[k], v1, v2);
            }

            for (int m = 0; m < num_entity_pointers; ++m) 
            {
                auto *ent = entity_pointers[m];
                region *reg = nullptr;
                if ( ent->has_region_idx() ) {
                    assert(ent->has_region_idx() && ( ent->get_region_idx() < the_terrain->get_num_regions() ));

                    auto reg_idx = ent->get_region_idx();
                    reg = this->the_terrain->get_region(reg_idx);
                    assert(reg != nullptr);
                }

                auto *visitor = new (mem_for_visitor) region_intersect_visitor {reg};

                fixed_vector<region *, 15> a2a {};
                auto *v60 = &vec4_pointers[m];
                loaded_regions_cache::get_regions_intersecting_sphere(v60->field_0, v60->field_C, &a2a);
                for (int n = 0; n < a2a.size(); ++n)
                {
                    auto *v25 = a2a.m_data[n];
                    [](region_intersect_visitor *self, region *r) -> int {
                        assert(r != nullptr);

                        for ( auto i = 0; i < self->field_4; ++i ) {
                            if ( r == self->field_8[i] ) {
                                return 0;
                            }
                        }

                        if ( self->field_4 < 15 ) {
                            self->field_8[self->field_4++] = r;
                        }

                        return 0;
                    }(visitor, v25);
                }

                if ( visitor->field_4 != 0 ) {
                    ent->update_regions(visitor->field_8, visitor->field_4);
                } else if ( ent->regions[0] != nullptr ) {
                    ent->remove_from_regions();
                }
            }

            for (auto v29 = 0; v29 < num_entity_pointers; ++v29)
            {
                auto *v30 = &vec4_pointers[v29];
                auto *v31 = entity_pointers[v29];
                bool v51 = false;
                if ( auto *primary_region = v31->get_primary_region();
                        primary_region != nullptr )
                {
                    if ( !primary_region->is_interior() ) {
                        if (limbo_area::sphere_intersects_unsafe_area(v30->field_0, v30->field_C)) {
                            v51 = true;
                        }
                    }

                } else {
                    v51 = true;
                }

                if (v51) {
                    entity_pointers[v29]->enter_limbo();
                } else {
                    entity_pointers[v29]->exit_limbo();
                }
            }

            scratchpad_stack::pop(vec4_pointers, sizeof(vec4_t) * (num_entity_pointers + 1));
            scratchpad_stack::pop(mem_for_visitor, sizeof(region_intersect_visitor));

            for (int v35 = 0; v35 < num_entity_pointers; ++v35) {
                entity_pointers[v35]->update_proximity_maps();
            }
        }

        scratchpad_stack::pop(entity_pointers, n_bytes);
    }
    else
    {
        THISCALL(0x00530100, this, a1);
    }
}

void world_dynamics_system::update_collision_proximity_maps_for_moved_entities(Float a1)
{
    TRACE("world_dynamics_system::update_collision_proximity_maps_for_moved_entities");

    if constexpr (0)
    {
        for (int i = 0; i < moved_entities::moved_count; ++i)
        {
            auto *ent = moved_entities::moved_list[i].get_volatile_ptr();
            if ( ent != nullptr )
            {
                assert("regions[ 0 ] can not be NULL when regions[ 1 ] is not."
                        && ( ent->regions[ 1 ] ? ent->regions[ 0 ] != nullptr : 1 ));

                assert("regions[ 0 ] and regions[ 1 ] should not be NULL while extended_regions is not."
                        && ent->extended_regions ? ent->regions[ 0 ] && ent->regions[ 1 ] : 1);

                assert(ent->extended_regions != nullptr
                        ? ent->extended_regions->size() > 0
                        : 1);
                if ( ent->possibly_collide() )
                {
                    auto *v2 = ent->regions[0]->collision_proximity_map;
                    if ( v2 != nullptr ) {
                        v2->update_entity(ent);
                    }
                }
            }
        }

        collision_dynamic_rtree().sort();
    }
    else
    {
        THISCALL(0x0054A610, this, a1);
    }
}

void world_dynamics_system::update_light_proximity_maps_for_moved_entities(Float a1)
{
    TRACE("world_dynamics_system::update_light_proximity_maps_for_moved_entities");

    if constexpr (0)
    {
        for (int i = 0; i < moved_entities::moved_count; ++i)
        {
            auto *ent = moved_entities::moved_list[i].get_volatile_ptr();
            if ( ent != nullptr )
            {
                if ( ent->is_a_conglomerate() )
                {
                    auto *the_conglom = bit_cast<conglomerate *>(ent);
                    auto *list = the_conglom->field_100;
                    if ( list != nullptr && list->size() > 0
                            && (the_conglom->field_110 & 0x4000) == 0 )
                    {
                        assert("regions[ 0 ] can not be NULL when regions[ 1 ] is not. "
                                && ( ent->regions[ 1 ] ? ent->regions[ 0 ] != nullptr : 1 ));

                        assert("regions[ 0 ] and regions[ 1 ] should not be NULL while extended_regions is not."
                                && ent->extended_regions ? ent->regions[ 0 ] && ent->regions[ 1 ] : 1);

                        assert(ent->extended_regions ? ent->extended_regions->size() > 0 : 1);

                        int v14 = 0;
                        region *v9 = nullptr;
                        for (auto *v6 = ent->regions[0]; v6 != nullptr; v6 = v9)
                        {
                            for (auto v8 : (*the_conglom->field_100))
                            {
                                v6->light_proximity_map->update_entity(v8);
                            }

                            if (++v14 >= 2)
                            {
                                v9 = nullptr;
                                if (ent->extended_regions != nullptr)
                                {
                                    int v2 = v14 - 2;
                                    v9 = ( v2 < ent->extended_regions->size()
                                             ? ent->extended_regions->m_data[v2]
                                             : nullptr );
                                }
                            }
                            else
                            {
                                v9 = ent->regions[v14];
                            }
                        }
                    }
                }
            }
        }
    } else {
        THISCALL(0x00529CC0, this, a1);
    }
}

void world_dynamics_system::add_anim_ctrl(animation_controller *a2)
{
    TRACE("world_dynamics_system::add_anim_ctrl");

    if constexpr (0) {
        this->field_4.push_back(a2);
    } else {
        THISCALL(0x00542160, this, a2);
    }

    {
        auto *v6 = a2;
        auto e = v6->field_4->get_my_vhandle();

        assert(e.get_volatile_ptr() != nullptr);

    }
}

nal_anim_control *world_dynamics_system::get_anim_ctrl(uint32_t a1)
{
    auto **slot_contents_ptr = this->field_0->get_slot_contents_ptr(a1);
    if ( slot_contents_ptr != nullptr ) {
        return *slot_contents_ptr;
    }

    return nullptr;
}

int get_hero_type_helper()
{
    auto *hero_ptr = (actor *) g_world_ptr->get_hero_ptr(0);
    if (hero_ptr != nullptr) {
        return hero_ptr->m_player_controller->m_hero_type;
    }

    assert(0 && "no hero available right now");
    return 0;
}

void world_dynamics_system_remove_player_patch()
{
    FUNC_ADDRESS(address, &world_dynamics_system::remove_player);
    SET_JUMP(0x00558550, address);
}

void world_dynamics_system_patch()
{
    world_dynamics_system_remove_player_patch();

    REDIRECT(0x005584BD, zero_xz_velocity_for_effectively_standing_physical_interfaces);

    {
        FUNC_ADDRESS(address, &world_dynamics_system::add_anim_ctrl);
        REDIRECT(0x0049BD38, address);
    }

    {
        FUNC_ADDRESS(address, &world_dynamics_system::update_collision_proximity_maps_for_moved_entities);
        REDIRECT(0x00558517, address);
    }

    {
        FUNC_ADDRESS(address, &world_dynamics_system::create_terrain);
        REDIRECT(0x0055BB5C, address);
    }

    {
        FUNC_ADDRESS(address, &world_dynamics_system::update_ai_and_visibility_proximity_maps_for_moved_entities);
        REDIRECT(0x00558403, address);
    }

    REDIRECT(0x005584B8, manage_standing_for_all_physical_interfaces);

    REDIRECT(0x005584C3, collide_all_moved_entities);

    {
        FUNC_ADDRESS(address, &world_dynamics_system::advance_entity_animations);
        REDIRECT(0x00558423, address);
    }

    {
        FUNC_ADDRESS(address, &world_dynamics_system::load_scene);
        SET_JUMP(0x0055B160, address);
    }

    {
        FUNC_ADDRESS(address, &world_dynamics_system::un_mash_scene_entities);
        REDIRECT(0x0055B268, address);
    }

    {
        FUNC_ADDRESS(address, &world_dynamics_system::un_mash_scene_quad_paths);
        REDIRECT(0x0055B2FE, address);
    }

    {
        FUNC_ADDRESS(address, &world_dynamics_system::un_mash_scene_audio_boxes);
        REDIRECT(0x0055B328, address);
    }

    {
        FUNC_ADDRESS(address, &world_dynamics_system::un_mash_scene_spline_paths);
        REDIRECT(0x0055B2CC, address);
    }

    if constexpr (1)
    {
        FUNC_ADDRESS(address, &world_dynamics_system::add_player);
        REDIRECT(0x0055CCA3, address);
        REDIRECT(0x005C5889, address);
        REDIRECT(0x00641C5C, address);
        REDIRECT(0x00641C97, address);
        REDIRECT(0x00676CFB, address);
    }

    {
        FUNC_ADDRESS(address, &world_dynamics_system::frame_advance);
        REDIRECT(0x0055A0F7, address);
    }

    return;

    {
        FUNC_ADDRESS(address, &world_dynamics_system::un_mash_scene_box_triggers);
        REDIRECT(0x0055B296, address);
    }

    {
        FUNC_ADDRESS(address, &world_dynamics_system::create_water_kill_trigger);
        REDIRECT(0x0055BF7B, address);
    }

    {
        FUNC_ADDRESS(address, &world_dynamics_system::activate_corner_web_splats);

        REDIRECT(0x0047DB5F, address);
    }

}

bool wds_xbpack_patch()
{
#ifdef OPENUSM_XBPACK_V10
    // V10 empty scene resources contain END=13, while PC uses END=18.
    // FL_INT_A's audio resource is exactly four bytes. Without this check,
    // native 0x0053CB50 advances past it and 0x00520600 rebases the following
    // light entity as an audio rtree; traversal then crashes at 0x00521408.
    // Validate both END consumers before changing either one. Accepting 13
    // also makes repeated installation harmless.
    const auto *entity_end = bit_cast<const uint8_t *>(0x0055AD64);
    const auto *audio_end = bit_cast<const uint8_t *>(0x0053CBAF);
    if (entity_end[0] != 0x83 || entity_end[1] != 0xF9
        || (entity_end[2] != 18 && entity_end[2] != 13)
        || entity_end[3] != 0x0F || entity_end[4] != 0x84
        || audio_end[0] != 0x83 || audio_end[1] != 0x3F
        || (audio_end[2] != 18 && audio_end[2] != 13)
        || audio_end[3] != 0x74 || audio_end[4] != 0x34) {
        sp_log("[xbpack] V10 scene END patch rejected: native comparison changed");
        return false;
    }

    *bit_cast<uint8_t *>(0x0055AD66) = 13u;
    *bit_cast<uint8_t *>(0x0053CBB1) = 13u;
    *bit_cast<uint8_t *>(0x00558793) = 0x3Cu;

    FUNC_ADDRESS(address, &world_dynamics_system::un_mash_scene_entities);
    REDIRECT(0x0055B268, address);
#endif
    return true;
}
