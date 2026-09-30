#include "wds_render_manager.h"

#include "GL/gl.h"

#include "aeps.h"
#include "beam.h"
#include "bitvector.h"
#include "box_trigger.h"
#include "camera.h"
#include "camera_teleport_update_visitor.h"
#include "city_lod.h"
#include "comic_panels.h"
#include "common.h"
#include "culling_params.h"
#include "cut_scene_player.h"
#include "debug_render.h"
#include "debug_string.h"
#include "entity_trigger.h"
#include "femanager.h"
#include "filespec.h"
#include "func_wrapper.h"
#include "game.h"
#include "geometry_manager.h"
#include "glass_house_manager.h"
#include "hierarchical_entity_proximity_map.h"
#include "igofrontend.h"
#include "line_info.h"
#include "loaded_regions_cache.h"
#include "motion_effect_struct.h"
#include "ngl.h"
#include "ngl_mesh.h"
#include "ngl_support.h"
#include "render_text.h"
#include "occlusion.h"
#include "occlusion_visitor.h"
#include "oldmath_po.h"
#include "oriented_bounding_box_root_node.h"
#include "os_developer_options.h"
#include "physical_interface.h"
#include "point_trigger.h"
#include "proximity_map.h"
#include "region.h"
#include "renderoptimizations.h"
#include "sector2d.h"
#include "shadow.h"
#include "subdivision_node_obb_base.h"
#include "trace.h"
#include "trigger_manager.h"
#include "terrain.h"
#include "us_colorvol.h"
#include "us_pcuv_shader.h"
#include "utility.h"
#include "variables.h"
#include "vector2di.h"
#include "vector2d.h"
#include "wds.h"

#include <algorithm>
#include <cassert>
#include <cmath>

VALIDATE_SIZE(wds_render_manager, 156u);

struct traversed_entity {
    entity_base_vhandle m_handle;
    int field_4;
};
static Var<fixed_vector<traversed_entity, 750> *> traversed_entities_last_frame {0x0095C7B4};

wds_render_manager::wds_render_manager() {
    this->field_30.sub_56FCB0();

    this->field_0 = new RenderOptimizations();
    this->field_94 = nullptr;
    this->field_98 = 0;

    this->field_10[0] = 0.30000001f;
    this->field_10[1] = -1.0f;
    this->field_10[2] = 0.2f;
    this->field_10[3] = 0;

    this->field_20[0] = 0.80000001f;
    this->field_20[1] = 0.80000001f;
    this->field_20[2] = 0.85000002f;
    this->field_20[3] = 1.0f;

    this->field_60 = {0, 0, 0, 1};

    this->field_90 = 1.0f;
    this->field_5C = nullptr;
    this->field_8C = 0;
    this->field_84 = 175.0f;
    this->field_88 = 250.0f;
}

void show_terrain_info()
{
    if ( g_world_ptr != nullptr )
    {
        auto *v0 = g_world_ptr->get_hero_ptr(0);
        if ( v0 != nullptr )
        {
            auto *v8 = g_world_ptr->get_hero_ptr(0);
            if ( v8->has_physical_ifc() )
            {
                auto *v3 = g_world_ptr->get_hero_ptr(0);
                auto *v4 = v3->physical_ifc();

                string_hash v17;
                v4->get_parent_terrain_type(&v17);

                vector2d v5{512.0, 32.0};
                vector2di v14 {v5};

                auto *v6 = v17.to_string();
                mString v16 {v6};
                color32 v7{255, 255, 255, 255};
                render_text(v16, v14, v7, 1.0, 1.0);
            }
        }
    }
}

#ifdef OPENUSM_ULTIMATE_RELEASE
namespace {

void render_trigger_debug_overlays()
{
    const bool show_boxes = debug_render_get_bval(BOX_TRIGGERS);
    const bool show_water_boxes =
        debug_render_get_bval(WATER_EXCLUSION_TRIGGERS);
    const bool show_points = debug_render_get_bval(POINT_TRIGGERS);
    const bool show_entities = debug_render_get_bval(ENTITY_TRIGGERS);

    auto *manager = trigger_manager::instance;
    if (manager == nullptr ||
        (!show_boxes && !show_water_boxes && !show_points && !show_entities)) {
        return;
    }

    for (auto *base = manager->m_triggers;
         base != nullptr;
         base = base->m_next_trigger) {
        vector3d label_position{};
        color32 label_color{};
        bool rendered = false;

        if (base->is_point_trigger() && show_points) {
            auto *point = static_cast<point_trigger *>(base);
            const auto radius = std::max(point->field_48, 0.05f);
            label_position = point->field_58 + YVEC * (radius + 0.15f);
            label_color = color32{64, 255, 64, 255};
            render_debug_hemisphere(point->field_58,
                                    radius,
                                    color32{64, 255, 64, 96});
            rendered = true;
        } else if (base->is_box_trigger()) {
            auto *box = static_cast<box_trigger *>(base);
            const bool is_water_box = (box->field_4 & 0x20000u) != 0;
            if (show_boxes || (show_water_boxes && is_water_box)) {
                auto *box_entity = box->get_box_ent();
                const auto origin = box_entity != nullptr
                    ? box_entity->get_abs_position()
                    : box->field_5C;
                const auto min_extent = origin + box->box.bbox.field_0[0];
                const auto max_extent = origin + box->box.bbox.field_0[1];
                label_position = origin + YVEC * (box->field_48 + 0.15f);
                label_color = is_water_box
                    ? color32{64, 160, 255, 255}
                    : color32{255, 192, 32, 255};
                render_debug_box(min_extent,
                                 max_extent,
                                 is_water_box
                                     ? color32{64, 160, 255, 80}
                                     : color32{255, 192, 32, 80});
                rendered = true;
            }
        } else if (base->is_entity_trigger() && show_entities) {
            auto *entity_trig = static_cast<entity_trigger *>(base);
            if (auto *target = entity_trig->get_ent()) {
                const auto radius = std::max(entity_trig->field_48, 0.05f);
                const auto &position = target->get_abs_position();
                label_position = position + YVEC * (radius + 0.15f);
                label_color = color32{255, 64, 255, 255};
                render_debug_hemisphere(position,
                                        radius,
                                        color32{255, 64, 255, 96});
                rendered = true;
            }
        }

        if (rendered) {
            auto id = base->get_id();
            print_3d_text(label_position,
                          label_color,
                          0.5f,
                          "%s",
                          id.to_string());
        }
    }
}

void render_visibility_spheres()
{
    const int requested = debug_render_get_ival(VIS_SPHERES);
    if (requested <= 0 || g_world_ptr == nullptr) {
        return;
    }

    int rendered = 0;
    for (auto *entity : g_world_ptr->ent_mgr.entities) {
        if (entity == nullptr || !entity->is_visible()) {
            continue;
        }

        const float radius = std::clamp(entity->get_visual_radius(), 0.05f, 50.0f);
        const auto &position = entity->get_abs_position();
        const color32 sphere_color{64, 192, 255, 72};
        render_debug_hemisphere(position, radius, sphere_color);

        auto id = entity->get_id();
        print_3d_text(position + YVEC * (radius + 0.1f),
                      color32{96, 224, 255, 255},
                      0.45f,
                      "%s",
                      id.to_string());

        if (++rendered >= requested) {
            break;
        }
    }
}

} // namespace
#endif

void sub_6A9863()
{
#ifdef OPENUSM_ULTIMATE_RELEASE
    render_trigger_debug_overlays();
    render_visibility_spheres();

    if ( debug_render_get_bval(SPHERES) ) {
        render_debug_spheres();
    }

    if ( debug_render_get_bval(LINES) ) {
        render_debug_lines();
    }

    if ( debug_render_get_ival(LINE_INFO) ) {
        debug_render_line_info();
    }

    if ( debug_render_get_bval(CYLINDERS) ) {
        render_debug_cylinders();
    }

    // Mission scripts enqueue timed world-space strings.  The retail build
    // kept the producer but omitted the render/update side of that queue.
    render_3d_debug_strings();
    static Var<float> frame_time_inc{0x009682D0};
    frame_advance_3d_debug_strings(frame_time_inc());
#else
    if ( debug_render_get_bval(SPHERES) ) {
        render_debug_spheres();
    }

    if ( debug_render_get_bval(LINES) ) {
        render_debug_lines();
    }

    if ( debug_render_get_ival(LINE_INFO) ) {
        debug_render_line_info();
    }

    render_debug_lines();
    render_debug_spheres();
#endif
}

void wds_render_manager::debug_render()
{
    TRACE("wds_render_manager::debug_render");

    if constexpr (0)
    {
        if (os_developer_options::instance->get_flag(mString{"SHOW_TERRAIN_INFO"}))
        {
            show_terrain_info();
        }

        if ( debug_render_get_ival((debug_render_items_e)20) || os_developer_options::instance->get_flag(mString {"SHOW_GLASS_HOUSE"}))
        {
            //glass_house_manager::show_glass_houses();
        }

        //if ( debug_render_get_ival((debug_render_items_e)21) || SHOW_OBBS || SHOW_DISTRICTS )
        {
            auto *ter= g_world_ptr->get_the_terrain();
            ter->show_obbs();
        }

        render_debug_spheres();

        debug_render_line_info();
    }

    sub_6A9863();
}

void wds_render_manager::render_region_mesh(nglMesh *a2, Float fade)
{
    TRACE("wds_render_manager::render_region_mesh");

    sp_log("fade = %f", fade);

    THISCALL(0x00537390, this, a2, fade);
}

int wds_render_manager::add_far_away_entity(vhandle_type<entity> a2) {
    return THISCALL(0x0052A470, this, a2);
}

void wds_render_manager::init_level(const char *a2)
{
    TRACE("wds_render_manager::init_level", a2);
    if constexpr (1) {
        if (this->field_5C == nullptr) {
            tlFixedString a1{"obb_shadow000"};
            this->field_5C = nglGetMesh(a1, true);
        }

        if (this->field_94 == nullptr) {
            filespec v7{mString{a2}};

            this->field_94 = new city_lod{v7.m_name.c_str()};
        }
    } else {
        THISCALL(0x00550930, this, a2);
    }
}

void wds_render_manager::create_colorvol_scene()
{
    THISCALL(0x0053DA50, this);
}

void wds_render_manager::render_lowlods(camera &)
{
    if ( os_developer_options::instance->get_flag(mString{"RENDER_LOWLODS"}) ) {
        this->field_94->render();
    }
}

static constexpr auto g_projected_fov_multiplier = 0.80000001f;

void wds_render_manager::update_occluders(camera &a2)
{
    TRACE("wds_render_manager::update_occluders");

    if constexpr (0) {
        occlusion::empty_quad_database();

        for (auto &i : this->field_30.field_0) {
            auto *reg = i.field_0;

            float v18 = reg->get_ground_level();

            auto &v5 = a2.get_abs_po();

            auto &v6 = a2.get_abs_position();

            occlusion_visitor visitor{v6, v5.get_z_facing(), v18, reg};

            ++subdivision_node_obb_base::visit_key();

            auto a4 = a2.compute_xz_projected_fov() * g_projected_fov_multiplier;

            auto &v11 = a2.get_abs_po();

            auto &v12 = a2.get_abs_po();

            vector3d a2a = a2.get_abs_position() - v12.get_z_facing() * 20.f;

            sector2d v26{a2a, v11.get_z_facing(), a4};
            ++region::visit_key2;
            auto *v16 = reg->field_98;
            if (v16 != nullptr) {
                v16->field_5C->traverse_sector_raster(v26, 100.0f, visitor);
            }
        }
    }
    else
    {
        THISCALL(0x00530500, this, &a2);
    }
}

void update_camera_teleport(camera &cam)
{
    TRACE("update_camera_teleport");

    if constexpr (0)
    {
        auto *v1 = g_cut_scene_player();
        auto v17 = ( v1->is_playing() ? 1.0 : 25.0 );

        static Var<vector3d> last_camera_position {0x00960B48};
        static Var<bool> last_camera_position_valid {0x00960B54};

        auto &abs_pos = cam.get_abs_position();
        if ( !last_camera_position_valid() )
        {
            last_camera_position() = abs_pos;
        }

        ++entity::visit_key;

        auto len2 = (last_camera_position() - abs_pos).length2();
        if ( len2 > v17 )
        {
            fixed_vector<region *, 15> a2 {};
            
            camera_teleport_update_visitor_t visitor {};
            loaded_regions_cache::get_regions_intersecting_sphere(abs_pos, culling_params::entity_traversal_distance, &a2);
            for (auto i = 0u; i < a2.size(); ++i) 
            {
                region *reg = a2.at(i);
                assert(reg != nullptr);

                reg->visibility_map->traverse_sphere(
                                                abs_pos,
                                                culling_params::entity_traversal_distance,
                                                &visitor);
                auto *bitvector_of_legos_rendered_last_frame = reg->bitvector_of_legos_rendered_last_frame;
                if ( bitvector_of_legos_rendered_last_frame != nullptr ) {
                    bitvector_of_legos_rendered_last_frame->clear();
                }

            }

            if ( traversed_entities_last_frame() != nullptr ) {
                traversed_entities_last_frame()->m_size = 0;
            }
        }

        last_camera_position() = cam.get_abs_position();
        last_camera_position_valid() = true;
    } else {
        CDECL_CALL(0x00530760, &cam);
    }
}

void sub_520E60()
{
    CDECL_CALL(0x00520E60);
}

void update_spidey_interface()
{
    if ( g_world_ptr != nullptr )
    {
        if ( g_world_ptr->get_hero_ptr(0) != nullptr ) {
            g_femanager.IGO->UpdateInScene();
        }
    }
}

#include "debug_menu.h"

void wds_render_manager::render(camera &a2, int a3)
{
    TRACE("wds_render_manager::render");

    assert(this->field_94 != nullptr);

    if constexpr (1)
    {
        sub_520E60();
        update_camera_teleport(a2);
        if ( g_disable_occlusion_culling() == 3 )
        {
            occlusion::reset_active_occluders();
        }
        else
        {
            this->update_occluders(a2);
            occlusion::init_frame(a2.get_abs_position());
        }

        auto *panel_params = comic_panels::get_panel_params();
        if ( panel_params == nullptr || (panel_params->field_0 & 0x20) != 0 )
        {
            this->create_colorvol_scene();
            
            if ( debug_render_get_bval(LOW_LODS) ) {
                this->render_lowlods(a2);
            }

            g_camera_link() = &a2;

            this->field_30.field_0.clear();
            this->field_30.field_10.clear();

            a2.compute_sector(g_world_ptr->the_terrain, false, nullptr);
            auto *prim_reg = a2.get_primary_region();

            auto *reg = g_world_ptr->the_terrain->find_region(a2.get_abs_position(), nullptr);
            if ( reg != prim_reg )
            {
                auto *v10 = g_world_ptr->get_hero_ptr(a3);
                if ( v10 != nullptr )
                {
                    if ( v10->get_primary_region() == nullptr )
                    {
                        prim_reg = reg;
                    }
                }
            }

            if ( prim_reg == nullptr )
            {
                sp_log("no camera region!!!!");
                if ( g_disable_occlusion_culling() != 3 ) {
                    occlusion::term_frame();
                }

                return;
            }

            geometry_manager::rebuild_view_frame();
            ++region::visit_key;
            this->field_30.field_0.reserve(g_world_ptr->the_terrain->get_num_regions() + 1);

            a2.get_abs_position();

            this->build_render_data_regions(this->field_30, a2);
            this->sub_53D560(a2);
        }

        if ( debug_render_get_bval(ENTITIES) )
        {
            this->build_render_data_ents(this->field_30, a2, a3);
            aeps::FrameSetupRenderAndThenRender();
            if ( panel_params == nullptr || (panel_params->field_0 & 0x20) != 0 )
            {
                motion_effect_struct::render_all_motion_fx(a2, geometry_manager::world_space_frustum());
                update_spidey_interface();
                ++entity::visit_key;
            }
        }

        if ( panel_params == nullptr || (panel_params->field_0 & 0x20) != 0 )
        {
            send_shadow_projectors();

            if ( debug_render_get_bval(OCCLUSION) )
            {
                occlusion::debug_render_occluders();
            }

#ifndef OPENUSM_ULTIMATE_RELEASE
            this->debug_render();
#endif
            this->clear_colorvol_scene();
        }

        if ( g_disable_occlusion_culling() != 3 ) {
            occlusion::term_frame();
        }

    } else {
        THISCALL(0x0054B250, this, &a2, a3);
    }

    //_populate_missions();

#ifndef OPENUSM_ULTIMATE_RELEASE
    if ( debug_render_get_bval(OCCLUSION) )
    {
        occlusion::debug_render_occluders();
    }
#endif

    this->debug_render();
}

void render_data::sub_56FCB0() {
    THISCALL(0x0056FCB0, this);
}

void wds_render_manager::frame_advance(Float a2) {
    TRACE("wds_render_manager::frame_advance");

    THISCALL(0x0054ADE0, this, a2);
}

void wds_render_manager::render_stencil_shadows(const camera &a2)
{
    TRACE("wds_render_manager::render_stencil_shadows");
    
    THISCALL(0x0053D5E0, this, &a2);
}

void wds_render_manager::build_render_data_regions(render_data &a2, camera &a3)
{
    TRACE("wds_render_manager::build_render_data_regions");

    THISCALL(0x00547000, this, &a2, &a3);
}

void wds_render_manager::build_render_data_ents(render_data &a2, camera &a3, int a4)
{
    TRACE("wds_render_manager::build_render_data_ents");

    THISCALL(0x00547250, this, &a2, &a3, a4);
}

void wds_render_manager::clear_colorvol_scene()
{
    USColorVolShaderSpace::gUSColorVolScene() = nullptr;
}

void wds_render_manager::render_meshes(camera &a2)
{
    TRACE("wds_render_manager::render_meshes");

    THISCALL(0x0053CED0, this, &a2);
}

void wds_render_manager::render_legos(camera &a2)
{
    TRACE("wds_render_manager::render_legos");

    THISCALL(0x0053D270, this, &a2);
}

static int g_region_meshes_occluded_this_frame;
static int g_region_meshes_rendered_this_frame;

void wds_render_manager::sub_53D560(camera &a2)
{
    TRACE("wds_render_manager::sub_53D560");

    if constexpr (1)
    {
        g_region_meshes_occluded_this_frame = 0;
        g_region_meshes_rendered_this_frame = 0;
        if ( debug_render_get_bval(REGION_MESHES) ) {
            this->render_meshes(a2);
        }

        if ( debug_render_get_bval(LEGOS) ) {
            this->render_legos(a2);
        }
    }
    else
    {
        THISCALL(0x0053D560, this, &a2);
    }
}

void wds_render_manager_patch()
{
    {
        FUNC_ADDRESS(address, &wds_render_manager::render_region_mesh);
        REDIRECT(0x0053D234, address);

        REDIRECT(0x00537465, FastListAddMesh);
    }

    REDIRECT(0x0054B410, debug_render_get_bval);

    {
        FUNC_ADDRESS(address, &wds_render_manager::render);
        REDIRECT(0x0054E52D, address);
    }

    REDIRECT(0x0054B265, update_camera_teleport);

    {
        FUNC_ADDRESS(address, &wds_render_manager::init_level);
        REDIRECT(0x0055B355, address);
    }

    {
        FUNC_ADDRESS(address, &wds_render_manager::render_stencil_shadows);
        REDIRECT(0x0054E585, address);
    }

    {
        FUNC_ADDRESS(address, &wds_render_manager::build_render_data_regions);
        REDIRECT(0x0054B3FB, address);
    }

    {
        FUNC_ADDRESS(address, &wds_render_manager::build_render_data_ents);
        REDIRECT(0x0054B428, address);
    }

    {
        FUNC_ADDRESS(address, &wds_render_manager::sub_53D560);
        REDIRECT(0x0054B403, address);
    }
}
