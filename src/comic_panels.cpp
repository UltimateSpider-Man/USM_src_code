#include "comic_panels.h"

#include "comic_page_camera.h"
#include "common.h"
#include "entity_base.h"
#include "func_wrapper.h"
#include "game.h"
#include "geometry_manager.h"
#include "ngl.h"
#include "ngl_params.h"
#include "ngl_scene.h"
#include "cut_scene_player.h"
#include "trace.h"
#include "utility.h"
#include "variables.h"
#include "vector3d.h"
#include "vtbl.h"
#include "wds.h"

VALIDATE_SIZE(comic_panels::panel_component_camera, 0x4C);

VALIDATE_OFFSET(comic_panels::panel, field_64, 0x64u);

VALIDATE_SIZE(comic_panels::panel_component_base, 0x10u);

VALIDATE_SIZE(comic_panels::panel_component::render_info, 0x148u);

VALIDATE_SIZE(comic_panels::panel_params_t, 0xD4);

namespace comic_panels {

Var<camera *> current_view_camera{0x0096F7B4};

Var<panel *> game_play_panel{0x0096F7D4};

Var<fixed_vector<panel *, 48>> panels{0x0096F9F8};

Var<bool> world_has_been_rendered {0x0096F7A0};

void clear_color_rect(
        aarect<float, vector2d> &a1,
        color &a2,
        math::MatClass<4, 3> &a3)
{
    CDECL_CALL(0x0073ADB0, &a1, &a2, &a3);
}

void draw_textured_quad(
        math::MatClass<4, 3> &a1,
        aarect<float, vector2d> &a2,
        aarect<float, vector2d> &a3,
        nglTexture *a4,
        color32 a5,
        bool a6)
{
    CDECL_CALL(0x0073D890, &a1, &a2, &a3, a4, a5, a6);
}

void sub_735C80(void *, aarect<float, vector2d> &a1, void *)
{
    a1.field_0[0][0] = 0.0;
    a1.field_0[0][1] = 0.0;
    a1.field_0[1][0] = 1.0;
    a1.field_0[1][1] = 1.0;
}

aarect<float, vector2d> sub_742E00(aarect<float, vector2d> a2, const vector2d &a3)
{
    aarect<float, vector2d> result;
    CDECL_CALL(0x00742E00, &result, &a2, &a3);

    return result;
}

aarect<float, vector2d> sub_744B00()
{
    aarect<float, vector2d> result {vector2d {-1.0f, -1.0f}, vector2d {1.0f, 1.0f}};
    return result;
}

void set_default_bgcolor(const color &a1)
{
    default_bgcol() = a1;
}

void sub_7315A0()
{
    TRACE("sub_7315A0");

    CDECL_CALL(0x007315A0);
}

void sub_742B20(void *, bool a2, bool a3)
{
    nglListBeginScene(static_cast<nglSceneParamType>(1));
    nglSetZTestEnable(a2);
    nglSetZWriteEnable(a3);
}

void init()
{
    TRACE("comic_panels::init");

    CDECL_CALL(0x00736A60);
}

void render()
{
    TRACE("comic_panels::render");
    
    CDECL_CALL(0x0073EA70);
}

void frame_advance(Float a1)
{
    TRACE("comic_panels::frame_advance");

    CDECL_CALL(0x0073E160, a1);
}

bool render_panels()
{
    TRACE("comic_panels::render_panels");

    if constexpr (0)
    {
    }
    else
    {
        bool (__cdecl *func)() = CAST(func, 0x0073E710);
        return func();
    }
}

panel *acquire_panel(const char *a1) {
    return (comic_panels::panel *) CDECL_CALL(0x00733AD0, a1);
}

panel_params_t *get_panel_params()
{
    if constexpr (0)
    {
        if ( nglCurScene() == nullptr ) {
            return nullptr;
        }

        if ( !nglCurScene()->field_404.IsSetParam<SMPanelParams>() ) {
            return nullptr;
        }

        SMPanelParams v1 {};
        return nglCurScene()->field_404.GetOrDefault<SMPanelParams>(v1)->field_0;
    }
    else
    {
        return (panel_params_t *) CDECL_CALL(0x00738CB0);
    }
}

camera *get_current_view_camera(int) {
    return current_view_camera();
}

void panel::init_anim(nalPanel::nalPanelAnim *a2)
{
    TRACE("comic_panels::panel::init_anim");

    THISCALL(0x00736220, this, a2);
}

void panel::add_camera_component(const char *a2, bool a3, int a4) {
    // Keep the stock camera component creation.  The previous empty
    // replacement left camera-backed comic panels with no component to
    // capture or render (most visibly in the alternate-costume viewer).
    THISCALL(0x007360B0, this, a2, a3, a4);
}

void panel::capture()
{
    if ( !this->field_67 && (this->field_50 > 0.0f || !this->field_5C) )
    {
        panel_component::render_info v3 {*this, 1};
        for ( auto *i = this->field_60; i != nullptr; i = i->field_4 ) {
            i->capture(v3);
        }
    }
}

bool panel::render()
{
    TRACE("comic_panels::panel::render");

    if constexpr (0)
    {
    }
    else
    {
        bool (__fastcall *func)(void *) = CAST(func, 0x00738B50);
        return func(this);
    }
}

bool sub_731790(const matrix4x4 &a1)
{
    auto sub_F3F4D0 = [](float a1, float a2) -> bool
    {
        return std::abs(a2 - a1) < 0.050000001;
    };

    return sub_F3F4D0(a1[0][0], 1.0)
      && sub_F3F4D0(a1[1][1], 1.0)
      && sub_F3F4D0(a1[2][2], -1.0);
}

bool panel::is_square() const
{
    auto v3 = this->get_transform();
    v3[3] = vector4d {0.0, 0.0, 0.0, 1.0};
    matrix4x4 v1;
    if ( cur_page_camera() != nullptr )
    {
        v1 = cur_page_camera()->get_transform();
    }
    else
    {
        auto v5 = identity_matrix;
        v5[2][2] = -1.0;
        v1 = v5;
    }

    auto v4 = v1;
    v4[3] = {0.0, 0.0, 0.0, 1.0};
    v3 = v4 * v3;
    return sub_731790(v3);
}

matrix4x4 panel::get_transform() const
{
    return this->field_4;
}

aarect<float, vector2d> panel::get_rect() const
{
    auto a2 = this->m_size * 0.5;
    auto v2 = a2 * -1.0f;
    aarect<float, vector2d> arg0 {v2, a2};
    return arg0;
}

vector3d panel::get_loc() const
{
    vector3d result { 
        this->field_4[3][0],
        this->field_4[3][1],
        this->field_4[3][2]};
    return result;
}

void panel::set_size(const vector2d &a2) {
    this->m_size = a2;
}

void panel::set_loc(const vector3d &a2) {
    this->field_4[3] = a2;
}

camera * panel_component_camera::get_default_camera() const
{
    auto v1 = this->field_2C;
    if ( v1 >= 4 ) {
        return g_game_ptr->get_current_view_camera(0);
    }

    auto *result = this->field_34[v1].get_volatile_ptr();
    if ( result == nullptr ) {
        return g_game_ptr->get_current_view_camera(0);
    }

    return result;
}

void panel_component_camera::register_camera(uint32_t a2, const char *a3) {
    if (a3 != nullptr && a3[0] != '\0') {
        auto *v4 = g_world_ptr->ent_mgr.get_entity(string_hash{a3});
        if (v4 != nullptr) {
            this->field_34[a2].field_0 = v4->get_my_handle();
        } else {
            this->field_34[a2].field_0 = {0};
        }

    } else {
        this->field_34[a2].field_0 = {0};
    }
}

void panel_component::setup_geomgr(panel_component::render_info &a3)
{
    void (__fastcall *func)(void *, void *, render_info *) = CAST(func, get_vfunc(m_vtbl, 0x1C));
    func(this, nullptr, &a3);
}

panel_component::render_info::render_info(comic_panels::panel &a2, int a3)
{
    THISCALL(0x00745F50, this, &a2, a3);
}

void panel_component_camera::set_scene_params(panel_component::render_info &a2)
{
    if constexpr (0)
    {
        if ( nglCurScene() != nullptr )
        {
            auto *v2 = &a2;
            if ( a2.field_40.field_4 != game_play_panel() || a2.field_143 || a2.field_13C->field_4C != nullptr )
            {
                auto *mem = nglListAlloc(sizeof(panel_params_t), 16u);
                auto *v5 = new (mem) panel_params_t {a2.field_40};

                v5->field_0 = 0;
                if ( (this->field_48 & 8) != 0 ) {
                    v5->field_0 = 0x40;
                }

                if ( (this->field_48 & 0x10) != 0 ) {
                    v5->field_0 |= 0x20u;
                }

                for ( int i = 0; i < 5; ++i )
                {
                    if ( (this->field_30 & (1 << i)) != 0 ) {
                        v5->field_0 |= (1 << i);
                    }
                }

                v5->field_D1 ^= (v5->field_D1 ^ v2->field_143) & 1;

                SMPanelParams params {v5};
                auto *SceneParams = nglGetSceneParams();
                SceneParams->SetParam(params);
            }
            else
            {
                SMPanelParams params {nullptr};
                auto *v3 = nglGetSceneParams();
                v3->SetParam(params);
            }
        }
    }
    else
    {
        THISCALL(0x007388A0, this, &a2);
    }
}

void panel_component_camera::_render(comic_panels::panel_component::render_info &a2)
{
    TRACE("comic_panels::panel_component_camera::render");

    if constexpr (0)
    {
        if ( this->field_2C < 4u )
        {
            auto *v3 = &a2;
            if ( !a2.field_142 )
            {
                if ( !a2.field_114.sub_560880() )
                {
                    const auto v5 = v3->field_138 * this->field_24;
                    if ( v5 > 0.0f )
                    {
                        if ( this->field_44 == nullptr || v3->field_143 )
                        {
                            auto v13 = this->field_48;
                            if ( (v13 & 8) != 0 && (v13 & 0x10) != 0 && v3->field_40.field_4 != game_play_panel() )
                            {
                                auto *v14 = g_cut_scene_player();
                                if ( v14->is_playing() )
                                {
                                    sub_742B20(nullptr, false, false);
                                    nglSetClearFlags(0);
                                    clear_color_rect(v3->field_114, default_bgcol(), v3->field_0);
                                    nglListEndScene();
                                }
                            }

                            auto *def_cam = this->get_default_camera();
                            struct {
                                camera *&field_0;
                                camera *field_4;
                            } a2a = {current_view_camera(), current_view_camera()};
                            current_view_camera() = def_cam;

                            int v22 = 3;
                            struct stru {
                                int *field_0;
                                int field_4;

                                stru(bool a2, int *a3, int *a4)
                                {
                                    this->field_0 = (a2 ? a3 : nullptr);
                                    this->field_4 = *a3;
                                    if ( a2 ) {
                                        *a3 = *a4;
                                    }
                                }


                                ~stru()
                                {
                                    if ( this->field_0 != nullptr ) {
                                        *this->field_0 = this->field_4;
                                    }
                                }



                            } a1 {world_has_been_rendered(), &g_disable_occlusion_culling(), &v22};

                            geometry_manager::get_xform(geometry_manager::xform_t::XFORM_VIEW_TO_PROJECTION);

                            bool v16 = geometry_manager::get_auto_rebuild_view_frame();
                            geometry_manager::set_auto_rebuild_view_frame(false);
                            sub_742B20(nullptr, true, true);
                            geometry_manager::set_aspect_ratio(1.0);
                            this->setup_geomgr(*v3);
                            if ( !v3->field_142 )
                            {
                                geometry_manager::set_auto_rebuild_view_frame(v16);
                                geometry_manager::rebuild_view_frame();
                                nglSetClearFlags(0);
                                auto &xform = geometry_manager::get_xform(geometry_manager::XFORM_EFFECTIVE_WORLD_TO_VIEW);
                                nglSetWorldToViewMatrix(xform);

                                struct stru {
                                    bool *field_0;
                                    bool field_4;

                                    stru(bool a2, bool *a3, bool *a4)
                                    {
                                        this->field_0 = (a2 ? a3 : nullptr);
                                        this->field_4 = *a3;
                                        if ( a2 ) {
                                            *a3 = *a4;
                                        }
                                    }

                                    ~stru() 
                                    {
                                        if ( this->field_0 != nullptr ) {
                                            *this->field_0 = this->field_4;
                                        }
                                    }
                                };
                                
                                bool a6 = (this->field_48 & 0x10) == 0;
                                bool v23 = false;
                                stru v28 {a6, &g_player_shadows_enabled(), &v23};

                                static Var<bool> byte_922C5D {0x00922C5D};
                                bool v24 = false;
                                stru v27 {a6, &byte_922C5D(), &v24};
                                g_game_ptr->render_world();
                            }

                            nglListEndScene();
                            geometry_manager::set_auto_rebuild_view_frame(false);
                            auto v18 = sub_744B00();
                            geometry_manager::set_viewport(v18);
                            if ( v3->field_141 )
                            {
                                geometry_manager::set_scissor(v3->field_124);
                            }
                            else
                            {
                                auto v19 = sub_744B00();
                                geometry_manager::set_scissor(v19);
                            }

                            geometry_manager::set_auto_rebuild_view_frame(v16);

                            a2a.field_0 = a2a.field_4;
                        }
                        else
                        {
                            bool v6 = ( (v5 * 255.0f) == 0xFF &&  (this->field_48 & 0x20) != 0 );

                            sub_742B20(nullptr, (this->field_48 & 1) == 0, (this->field_48 & 1) != 0);

                            vector2d a3 {};
                            a3[0] = v3->field_0[3][0];
                            a3[1] = v3->field_0[3][1];

                            aarect<float, vector2d> a2a = v3->field_114;
                            aarect<float, vector2d> a1 = (a2a += a3);
                            sub_735C80(this, a1, nullptr);

                            {
                                auto *v12 = this->field_44;
                                color32 a2 {0xFF, 0xFF, 0xFF, 0xFF};
                                draw_textured_quad(v3->field_0, a2a, a1, v12, a2, v6);
                            }

                            nglListEndScene();
                        }
                    }
                }
            }
        }
    }
    else
    {
        THISCALL(0x007419C0, this, &a2);
    }
}

void panel_component_camera::_capture(panel_component::render_info &a2)
{
    TRACE("comic_panels::panel_component_camera::capture");

    THISCALL(0x0073CB20, this, &a2);
}

void panel_component_base::_render(panel_component::render_info &a2)
{
    TRACE("comic_panels::panel_component_base::render");

    THISCALL(0x0073AF90, this, &a2);
}

void panel_component_base::capture(panel_component::render_info &a2)
{
    if constexpr (0)
    {
        a2.field_138 = this->field_8 * a2.field_138;
    }
    else
    {
        void (__fastcall *func)(void *, void *, panel_component::render_info *) = CAST(func, get_vfunc(m_vtbl, 0x18));
        func(this, nullptr, &a2);
    }
}

    

} // namespace comic_panels


void __fastcall sub_742C50(void *self, int, comic_panels::panel_component_base *a2)
{
    sp_log("0x%08X", a2->m_vtbl);

    THISCALL(0x00742C50, self, a2);
}

void comic_panels_patch()
{
    {
        FUNC_ADDRESS(address, &comic_panels::panel::init_anim);
        REDIRECT(0x0073661A, address);
    }

    FUNC_ADDRESS(address, &comic_panels::panel::add_camera_component);
    REDIRECT(0x006424BB, address);

    {
        FUNC_ADDRESS(address, &comic_panels::panel::render);
        REDIRECT(0x0073E84C, address);
        REDIRECT(0x0073EA2E, address);
    }

    REDIRECT(0x00736AE5, comic_panels::sub_7315A0);

    {
        REDIRECT(0x005D7112, comic_panels::render);
    }

    REDIRECT(0x0055D8CC, comic_panels::frame_advance);

    REDIRECT(0x0073EB04, comic_panels::render_panels);

    {
        FUNC_ADDRESS(address, &comic_panels::panel_component_camera::_render);
        set_vfunc(0x008AA340, address);
    }

    {
        FUNC_ADDRESS(address, &comic_panels::panel_component_camera::_capture);
        set_vfunc(0x008AA344, address);
    }

    {
        FUNC_ADDRESS(address, &comic_panels::panel_component_base::_render);
        set_vfunc(0x008A9E04, address);
    }

    REDIRECT(0x00743453, sub_742C50);
}
