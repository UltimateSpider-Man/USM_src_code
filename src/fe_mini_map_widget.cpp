#include "fe_mini_map_widget.h"

#include "camera.h"
#include "common.h"
#include "custom_math.h"
#include "entity.h"
#include "fe_mini_map_dot.h"
#include "func_wrapper.h"
#include "mini_map_dot_type.h"
#include "ngl.h"
#include "ngl_mesh.h"
#include "ngl_scene.h"
#include "oldmath_po.h"
#include "os_developer_options.h"
#include "panel_layer.h"
#include "panelfile.h"
#include "region.h"
#include "resource_manager.h"
#include "sound_instance_id.h"
#include "terrain.h"
#include "trace.h"
#include "utility.h"
#include "vtbl.h"
#include "wds.h"

#include "resource_directory.h"

VALIDATE_SIZE(fe_mini_map_widget, 0x3B8u);

fe_mini_map_widget::fe_mini_map_widget()
{
    if constexpr (0)
    {
    }
    else
    {
        THISCALL(0x006343C0, this);
    }
}

fe_mini_map_widget::~fe_mini_map_widget()
{
    this->m_vtbl = 0x00895A00;

    operator delete(this->field_3A4);
    this->field_3A4 = nullptr;

    for ( auto &mat : this->field_4 )
    {
        mat.m_texture = nullptr;
    }
}

void fe_mini_map_widget::Init()
{
    TRACE("fe_mini_map_widget::Init");

    if constexpr (1)
    {
        assert(mini_map_icons == nullptr);

        this->mini_map_icons = PanelFile::UnmashPanelFile("mini_map_items", static_cast<panel_layer>(7));
        if (this->mini_map_icons != nullptr) {
            this->map_icon_spidey = this->mini_map_icons->GetPQ("map_icon_spidey");
            this->map_icon_others = this->mini_map_icons->GetPQ("map_icon_others");
            this->minimap_ring = this->mini_map_icons->GetPQ("minimap_ring");
        }

        assert(mini_map_frame == nullptr);

        this->mini_map_frame = PanelFile::UnmashPanelFile("mini_map_frame", static_cast<panel_layer>(7));

        if (this->mini_map_frame != nullptr) {
            this->map_frame_black = this->mini_map_frame->GetPQ("map_frame_black");
            this->map_frame_white = this->mini_map_frame->GetPQ("map_frame_white");
            this->map_frame_white_stub = this->mini_map_frame->GetPQ("map_frame_white_stub");
            this->compass_base = this->mini_map_frame->GetPQ("compass_base");
            this->compass_arrow = this->mini_map_frame->GetPQ("compass_arrow");
            this->map_frame_map_placeholder = this->mini_map_frame->GetPQ(
                "map_frame_map_placeholder");
            this->field_3A0 = this->mini_map_frame->GetAnimationPointer(0);
        }
    } else {
        THISCALL(0x006432F0, this);
    }
}

void fe_mini_map_widget::PrepareRegions()
{
    TRACE("fe_mini_map_widget::PrepareRegions");

    if constexpr (0)
    {
        for (auto &mat : this->field_4 ) {
            mat.m_texture = nullptr;
        }

        auto *hero_or_marky_cam_ptr = g_world_ptr->get_hero_or_marky_cam_ptr();

        auto abs_pos = hero_or_marky_cam_ptr->get_abs_position();
        auto *outermost_region = g_world_ptr->the_terrain->find_outermost_region(abs_pos);

        region_array v18 {};
        build_region_list_radius(&v18, outermost_region, abs_pos, 500.0f, true);

        int v14 = 0;
        for (int i = 0; i < v18.count; ++i)
        {
            auto *reg = v18[i];
            if ( reg != nullptr )
            {
                if ( reg->is_loaded() && !reg->is_interior() )
                {
                    auto scene_id = reg->get_scene_id(1);
                    auto key = create_resource_key_from_path(scene_id.c_str(), RESOURCE_KEY_TYPE_PACK);
                    auto *dir = resource_manager::get_resource_directory(key);
                    if ( dir != nullptr )
                    {
                        auto v13 = reg->get_scene_id(0);
                        tlFixedString v17 {v13.c_str()};
                        this->field_4[v14].m_texture = bit_cast<nglTexture *>(dir->get_tlresource(v17,
                                                  TLRESOURCE_TYPE_TEXTURE));
                        reg->get_region_extents(&this->field_244[v14], &this->field_2D4[v14]);
                        ++v14;
                    }
                }
            }
        }
    }
    else
    {
        THISCALL(0x00619690, this);
    }
}

void fe_mini_map_widget::RenderMeshes(matrix4x4 *a2, float &a4)
{
    TRACE("fe_mini_map_widget::RenderMeshes");

    if constexpr (0)
    {
        uint32_t v5 = 0;
        for ( int i = 0; i < 12; ++i )
        {
            if ( this->field_4[i].m_texture != nullptr ) {
                ++v5;
            }
        }

        nglMesh *mesh = nullptr;
        if ( v5 != 0 )
        {
            nglCreateMesh(0x40000u, v5, 0, nullptr);
            auto *v8 = this->field_4;
            for (int i = 0; i < 12; ++i)
            {
                if ( this->field_4[i].m_texture != nullptr )
                {
                    nglMaterialBase *v10 = ( v8 != nullptr
                                                ? bit_cast<nglMaterialBase *>(&v8->field_4)
                                                : nullptr );

                    auto *v11 = sub_507920(v10, 4, 1, 0, nullptr, D3DPT_TRIANGLESTRIP, true);
                    auto iter = v11->CreateIterator();
                    iter.BeginStrip(4u);

                    auto *v14 = g_world_ptr->get_hero_or_marky_cam_ptr();

                    vector3d v47[3] {};
                    v47[2] = v14->get_abs_position();
                    v47[1] = this->field_244[i] - v47[2];
                    v47[0] = this->field_2D4[i] - v47[2];

                    iter.Write(v47[1], -1, vector2d {1.0, 1.0});
                    ++iter;

                    iter.Write(v47[1], -1, vector2d {1.0, 0.0});
                    ++iter;

                    iter.Write(v47[0], -1, vector2d {0.0, 1.0});
                    ++iter;

                    iter.Write(v47[0], -1, vector2d {0.0, 0.0});
                    ++iter;

                    auto *v36 = iter.field_4->field_4;
                    if ( (v36->Flags & 0x40000) == 0 ) {
                        v36->field_3C.m_vertexBuffer->lpVtbl->Unlock(v36->field_3C.m_vertexBuffer);
                    }
                }
            }

            mesh = nglCloseMesh();
        }

        auto *v39 = g_world_ptr->get_chase_cam_ptr(0);
        auto &abs_po = v39->get_abs_po();
        auto y_facing = abs_po.get_y_facing();
        auto z_facing = abs_po.get_z_facing();

        auto v68 = -dot(z_facing, YVEC);
        float v41 = 0.0;
        vector3d v74 {z_facing[0], 0.0, z_facing[2]};
        if ( std::abs(v74[0]) > 0.0f && std::abs(v74[2]) > 0.0f ) {
            v41 = (dot(z_facing, v74) + 1.0f) * 0.5f;
        }

        vector3d v40 = v41 * z_facing + v68 * y_facing;
        v40[1] = 0.0f;
        v40.normalize();

        auto v69 = -sub_48A720(v40[0], v40[2]);
        auto CenterY = this->compass_base->GetCenterY();
        auto CenterX = this->compass_base->GetCenterX();
        this->compass_arrow->Rotate(CenterX, CenterY, v69 + 3.1415927, true);

        matrix4x4 v81 {};
        v81.make_rotate(YVEC, v69);
        auto *v55 = g_world_ptr->get_chase_cam_ptr(0);

        auto v56 = dot(YVEC, v55->get_abs_po().get_y_facing());

        auto v70 = v56;
        auto v63 = std::abs(v56);
        float v57 = sub_4ADC40(v63);
        if ( v70 < 0.0f ) {
            v57 = -v57;
        }

        a4 = (3.1415927 / 2.0) - v57;
        if ( a4 >= (3.1415927 / 4.0) )
        {
            if ( a4 > (3.1415927 / 2.0) ) {
                a4 = (3.1415927 / 2.0);
            }
        }
        else
        {
            a4 = 3.1415927 / 4.0;
        }

        float v64 = -a4;

        matrix4x4 a3 {};
        a3.make_rotate(XVEC, v64);
        float MINI_MAP_ZOOM = os_developer_options::instance->get_int(mString {"MINI_MAP_ZOOM"}); 
        if ( MINI_MAP_ZOOM >= 50.0f )
        {
            if ( MINI_MAP_ZOOM > 1000.0f ) {
                MINI_MAP_ZOOM = 1000.0f;
            }
        }
        else
        {
            MINI_MAP_ZOOM = 50.0f;
        }

        matrix4x4 v83 {};
        v83.make_translate(vector3d {0, 0, MINI_MAP_ZOOM});
        auto v60 = v81 * a3;
        matrix4x4 a2 = v60 * v83;
        if ( mesh != nullptr) {
            nglListAddMesh(mesh, *bit_cast<math::MatClass<4, 3> *>(&a2), nullptr, nullptr);
        }

    }
    else
    {
        THISCALL(0x00638C30, this, a2, &a4);
    }
}

struct poi_sort_record_t {
    float field_0;
    fe_mini_map_dot *field_4;
};

void sort__poi_sort_record_t(poi_sort_record_t *a1, poi_sort_record_t *a2, int a3)
{
    TRACE("std::sort<poi_sort_record_t>");

    sp_log("%d %f", a3, a1->field_0);

    CDECL_CALL(0x0064BCA0, a1, a2, a3);

    sp_log("%f", a1->field_0);
}

void fe_mini_map_widget::UpdatePOIs(matrix4x4 *a2,
                                    Float a3,
                                    Float a4,
                                    Float a5,
                                    Float a6,
                                    Float a7)
{
    TRACE("fe_mini_map_widget::UpdatePOIs");

    if constexpr (0)
    {

    }
    else
    {
        THISCALL(0x0063AEC0, this, a2, a3, a4, a5, a6, a7);
    }
}

static Var<int> cur_z_value{0x00937B74};

// ---------------------------------------------------------------------------
// Minimap blip SFX gating
// ---------------------------------------------------------------------------
// Every dot registration funnels through here (the patch REDIRECTs all three
// stock call sites: 0x0063AE3A ctor, 0x006410E5 / 0x006436C6 inlined ctors).
// Dot-type provenance, verified against USM - Original.exe:
//
//   type 0   hero dot         fe_mini_map_widget::Update   (0x6418AE / 0x641959)
//   type 2   arrow-target     ensure-dot helper 0x640F10   (inlined ctor @ 0x6410E5)
//   type 3   token reveal     token_def::show_dot 0x5C9CE0 (push 3 @ 0x5C9D80)
//   type 5   fixed marker     widget setup                 (inlined ctor @ 0x6436C6)
//   type N   mission POIs     SLF "set_poi_icon(num)" 0x677480 -> 0x641120
//                             (icon id chosen by mission scripts)
//
// The blip fires ONLY for the dedicated openusm enemy/boss ids
// (mini_map_dot_type.h, 20 / 21), which are spawned exclusively by the new
// entity_tracker.set_enemy_icon(num) script function (slc_manager.cpp).
// Everything stock stays silent by design: mission set_poi_icon dots
// (types 3 / 4), token_def::show_dot reveals (type 3), hero / arrow /
// objective dots.
//
// Reference behaviour (bandicam capture 2026-06-10, combat-tour spawn wave):
// every enemy dot added gets its own immediate blip, even back-to-back --
// six enemies registering over ~0.2 s produced six chirps ~37 ms apart, and
// a seventh enemy whose dot overlapped the cluster still got its own chirp.
// So: no rate limiting, no coalescing, default volume/pitch.
//
// NOTE: if a per-frame enemy-reticle pass (place_enemy_poi_reticles) is ever
// merged, it re-adds its dots continuously; the blip must then move to that
// pass's spawn detection instead of firing here unconditionally.

static bool is_enemy_poi_dot(const fe_mini_map_dot *dot)
{
    const int type = dot->field_20;
    return type == 2;
}

void fe_mini_map_widget::AddMapPOIWidget(fe_mini_map_dot *a2)
{
    if (a2 != nullptr && is_enemy_poi_dot(a2))
    {
        static string_hash sfx_id_hash{"FE_MINIMAP_BLIP"};
        [[maybe_unused]] sound_instance_id id = sub_60B960(sfx_id_hash, 1.0, 1.0);
    }

    THISCALL(0x006391F0, this, a2);
}


void fe_mini_map_widget::Draw()
{
    auto *v2 = this->field_3A0;
    auto v3 = v2 && v2->field_2D;
    if (this->field_3A8 || v3)
    {
        auto *hero_ptr = g_world_ptr->get_hero_ptr(0);

        region *reg;

        if (hero_ptr == nullptr ||
            (reg = g_world_ptr->the_terrain->find_outermost_region(hero_ptr->get_abs_position()),
             reg == nullptr) ||
            !reg->is_interior())
        {
            if (v3)
            {
                this->map_frame_map_placeholder->Draw();
            }
            else
            {
                this->PrepareRegions();
                nglListBeginScene(static_cast<nglSceneParamType>(0));
                nglSetClearFlags(1u);
                nglCurScene()->field_3BA = true;
                nglCurScene()->m_farz = 10000.0;

                float local_vec0[4], local_vec1[4];

                this->map_frame_map_placeholder->GetPos(local_vec0, local_vec1);
                auto v10 = local_vec0[0];
                auto v13 = local_vec0[3];

                auto v14 = local_vec1[0];
                auto v12 = local_vec1[3];

                nglSetViewport(local_vec0[0], local_vec1[0], local_vec0[3], local_vec1[3]);

                static const vector4d stru_892F80 {0, 0, 0, 1};

                char v9 = 0;
                matrix4x4 v19;
                v19.sub_415740(&v9);
                v19.arr[3] = stru_892F80;

                nglSetWorldToViewMatrix({v19});
                nglSetZTestEnable(false);

                matrix4x4 v20;
                float a4;
                this->RenderMeshes(&v20, a4);
                this->UpdatePOIs(&v20, a4, v10, v13, v14, v12);
                nglListEndScene();

                for (auto i = 0u; i < this->field_364.size(); ++i) {
                    this->field_364[i]->Draw();
                }

                this->compass_arrow->Draw();
            }

            this->map_frame_black->Draw();

            this->map_frame_white->Draw();

            this->map_frame_white_stub->Draw();

            this->compass_base->Draw();
        }
    }
}

void fe_mini_map_widget::Update(Float a2)
{
    THISCALL(0x00641810, this, a2);
}

void fe_mini_map_widget_patch()
{
    {
        FUNC_ADDRESS(address, &fe_mini_map_widget::Init);
        set_vfunc(0x00895A04, address);
    }

    {
        FUNC_ADDRESS(address, &fe_mini_map_widget::RenderMeshes);
        REDIRECT(0x00641B06, address);
    }

    {
        FUNC_ADDRESS(address, &fe_mini_map_widget::UpdatePOIs);
        REDIRECT(0x00641B2E, address);
    }
	{
        FUNC_ADDRESS(address, &fe_mini_map_widget::AddMapPOIWidget);
        REDIRECT(0x0063AE3A, address);
		REDIRECT(0x006410E5, address);
		REDIRECT(0x006436C6, address);
    }
	

    REDIRECT(0x0063B09C, sort__poi_sort_record_t);

    return;

    {
        FUNC_ADDRESS(address, &fe_mini_map_widget::Draw);
        set_vfunc(0x00895A08, address);
    }
}