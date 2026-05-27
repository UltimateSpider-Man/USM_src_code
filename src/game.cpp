#include "game.h"

#include "aeps.h"
#include "ai_find_best_swing_anchor.h"
#include "ai_pedestrian.h"
#include "ambient_audio_manager.h"
#include "app.h"
#include "audio_box_manager.h"
#include "base_ai_core.h"
#include "colmesh.h"
#include "color32.h"
#include "comic_panels.h"
#include "common.h"
#include "chuck_callbacks.h"
#include "chunk_file.h"
#include "console.h"
#include "custom_math.h"
#include "cut_scene_player.h"
#include "daynight.h"
#include "debugutil.h"
#include "debug_render.h"
#include "debug_menu.h"
#include "debug_struct.h"
#include "event_manager.h"
#include "fe_controller_disconnect.h"
#include "fe_timer_widget.h"
#include "femanager.h"
#include "fetext.h"
#include "filespec.h"
#include "frontendmenusystem.h"
#include "func_wrapper.h"
#include "gab_manager.h"
#include "game_clock.h"
#include "game_level.h"
#include "game_process.h"
#include "game_settings.h"
#include "geometry_manager.h"
#include "gravity_generator.h"
#include "hires_screenshot.h"
#include "igofrontend.h"
#include "igozoomoutmap.h"
#include "input_device.h"
#include "input_mgr.h"
#include "instance_bank.h"
#include "keyboard.h"
#include "light_manager.h"
#include "link_system.h"
#include "localized_string_table.h"
#include "log.h"
#include "marky_camera.h"
#include "matrix4x4.h"
#include "memory.h"
#include "message_board.h"
#include "mic.h"
#include "mission_stack_manager.h"
#include "mouselook_controller.h"
#include "ngl.h"
#include "ngl_font.h"
#include "ngl_dx_state.h"
#include "ngl_scene.h"
#include "occlusion.h"
#include "oldmath_po.h"
#include "osassert.h"
#include "os_developer_options.h"
#include "parse_generic_mash.h"
#include "pausemenusystem.h"
#include "proximity_map_stack.h"
#include "physical_interface.h"
#include "region.h"
#include "render_text.h"
#include "renderoptimizations.h"
#include "resource_manager.h"
#include "resource_pack_streamer.h"
#include "resource_partition.h"
#include "rumble_manager.h"
#include "scene_entity_base.h"
#include "scratchpad_stack.h"
#include "script_controller.h"
#include "script_manager.h"
#include "script_sound_manager.h"
#include "sin_container.h"
#include "slc_manager.h"
#include "smoke_test.h"
#include "sound_bank_slot.h"
#include "sound_manager.h"
#include "spider_monkey.h"
#include "spiderman_camera.h"
#include "stack_allocator.h"
#include "subtitles.h"
#include "terrain.h"
#include "terrain_types_manager.h"
#include "theta_and_psi_mcs.h"
#include "trace.h"
#include "trigger.h"
#include "trigger_manager.h"
#include "usocean2shader.h"
#include "us_lighting.h"
#include "utility.h"
#include "variables.h"
#include "vector2di.h"
#include "vtbl.h"
#include "wds.h"

#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <numeric>

VALIDATE_SIZE(game::level_load_stuff, 0x3C);
VALIDATE_SIZE(game, 0x2C4u);

VALIDATE_OFFSET(game, gamefile, 0xC0);
VALIDATE_OFFSET(game, field_161, 0x161);
VALIDATE_OFFSET(game::level_load_stuff, load_widgets_created, 0x38);

static int main_flow[] = {5, 6, 14};

static game_process main_process{"main", main_flow, 3};

static int start_flow[] = {1, 2, 3, 4, 14};
static game_process start_process{"start", start_flow, 5};

static int pause_flow[] = {7, 14};
static game_process pause_process{"pause", pause_flow, 2};

game *& g_game_ptr = var<game *>(0x009682E0);

static int & g_debug_mem_dump_frame = var<int>(0x00921DCC);

static auto & off_921DAC = var<char *[1]>(0x00921DAC);


void sub_538D10() {
    scratchpad_stack::stk().alignment = 16;

    scratchpad_stack::stk().segment = static_cast<char *>(arch_memalign(16u, 16384u));
    scratchpad_stack::stk().current = scratchpad_stack::stk().segment;
    scratchpad_stack::stk().segment_size_bytes = 16384;
}

void construct_script_controllers() {
    CDECL_CALL(0x0065F4E0);
}

void destruct_script_controllers()
{
    CDECL_CALL(0x0064E290);
}

void init_subdivision() {
    init_proximity_map_stacks();
}

void term_subdivision()
{
    CDECL_CALL(0x0052E700);
}

game::level_load_stuff::level_load_stuff()
{
    if constexpr (1) {
        this->descriptor = nullptr;
        this->name_mission_table = mString{"m0_arena"};
        this->field_14 = mString{"spiderman"};
        this->descriptor = nullptr;
        this->field_30 = 0;
        this->load_completed = false;
        this->field_3A = false;
        this->load_widgets_created = false;
    } else {
        THISCALL(0x00561EA0, this);
    }
}

game::game()
{
    TRACE("game::game");

    if constexpr (1)
    {
        static auto & setup_inputs_p = var<void (*)(game *)>(0x0095C8FC);
        setup_inputs_p = game__setup_inputs;;
    }

    if constexpr (1)
    {
        scratchpad_stack::initialize();
        script_manager::init();
        construct_script_controllers();
        scene_entity_base::initialize();
        this->field_2B4 = false;
        this->field_1 = false;
        this->field_BC = false;
        float fov = os_developer_options::instance->get_int(mString{"CAMERA_FOV"}) * 0.017453292;

        geometry_manager::set_field_of_view(fov);

        g_debug().field_0 |= 0x80u;
        if (os_developer_options::instance->get_flag(mString{"OUTPUT_WARNING_DISABLE"})) {
            g_debug().field_0 &= 0x7Fu;
        }

        g_debug().field_1 |= 4u;
        if (os_developer_options::instance->get_flag(mString{"OUTPUT_ASSERT_DISABLE"})) {
            g_debug().field_1 &= 0xFBu;
        }

        if (os_developer_options::instance->get_flag(mString{"SMOKE_TEST"})) {
            float a3 = os_developer_options::instance->get_int(mString{"SOAK_SMOKE"});
            if (equal(a3, 0.0f)) {
                a3 = 2.0;
            }

            static Var<char *[2]> smoke_test_levels { 0x00921DB0 };

            if (os_developer_options::instance->get_flag(mString{"SMOKE_TEST_LEVEL"})) {
                g_smoke_test() = new smoke_test(bit_cast<const char **>(&g_scene_name), a3);
            } else {
                g_smoke_test() = new smoke_test(bit_cast<const char **>(&smoke_test_levels()[0]),
                                                a3);

                strcpy(g_scene_name, smoke_test_levels()[0]);
            }
        } else if (!os_developer_options::instance->get_flag(mString{"SMOKE_TEST_LEVEL"})) {
            auto v23 = os_developer_options::instance->get_string(os_developer_options::strings_t::SCENE_NAME);

            if (v23)
            {
                strcpy(g_scene_name, v23->c_str());
            }
        }

        this->the_world = new world_dynamics_system();
        g_world_ptr = this->the_world;

        this->mb = nullptr;

        this->gamefile = new game_settings();

        this->field_278 = 0.0;
        this->field_27C = 0;
        this->field_280 = 0;
        this->field_284 = 1.0e10;
        this->field_288 = 0.0;
        this->field_28C = 0.1;
        this->field_270 = 0;
        this->field_274 = 0;
        this->field_15E = 0;
        this->field_15F = 0;
        this->field_160 = 0;
        this->field_164 = false;
        bExit() = false;
        this->field_165 = false;
        this->field_166 = false;
        this->field_163 = false;
        this->field_161 = false;
        this->field_162 = false;
        this->flag.level_is_loaded = 0;
        this->field_170 = 0;
        this->field_167 = 0;
        this->flag.single_step = false;
        this->flag.physics_enabled = false;
        this->flag.field_3 = false;
        this->flag.game_paused = false;
        this->field_16D = 0;
        this->field_16E = 0;
        this->m_hero_start_enabled = true;
        this->field_171 = 0;
        this->m_user_camera_enabled = false;
        this->field_2 = 0;
        this->field_148 = 0;
        this->field_149 = 0;
        this->field_13C = 9999.9502f;
        this->field_140 = 3.0;
        this->field_144 = -1;
        this->field_14A = 64;
        this->field_158 = 0;
        this->field_159 = 0;

        this->field_14C = 9999.9004f;
        this->field_150 = -3.0;
        this->field_154 = -1;
        this->field_15A = 64;
        this->field_58 = 0;
        this->current_game_camera = nullptr;
        this->field_5C = nullptr;
        this->field_64 = nullptr;
        this->field_7C = nullptr;

        this->setup_input_registrations();
        this->setup_inputs();

        static Var<bool> g_console_command {0x0095C068};
        g_console_command() = false;

        {
            this->push_process(main_process);
            this->push_process(start_process);
        }

        this->field_2B8 = 0;
        this->field_2BC = 0;

        this->field_15C = false;
        this->field_15D = false;

        this->field_2B5 = false;

        this->field_80 = game_button {
            game_button {static_cast<game_control_t>(105)},
            game_button {static_cast<game_control_t>(102)},
            4};

        occlusion::init();
        init_subdivision();

        g_debug_mem_dump_frame = os_developer_options::instance->get_int(mString {"MEM_DUMP_FRAME"});

    }
    else
    {
        THISCALL(0x00557610, this);
    }
}

game::~game()
{
    if constexpr (1)
    {
        if ( this->gamefile != nullptr ) {
            void (__fastcall *finalize)(void *, void *, bool) = CAST(finalize, get_vfunc(gamefile->m_vtbl, 0x4));
            finalize(this->gamefile, nullptr, true);
            this->gamefile = nullptr;
        }

        if ( g_smoke_test() != nullptr ) {
            auto &v3 = g_smoke_test();
            v3->~smoke_test();
            operator delete(v3);
            g_smoke_test() = nullptr;
        }

        auto &v4 = this->gamefile;
        if ( v4 != nullptr ) {
            void (__fastcall *finalize)(void *, void *, bool) = CAST(finalize, get_vfunc(v4->m_vtbl, 0x4));
            finalize(v4, nullptr, true);
        }

        subtitles_kill();
        if ( this->the_world != nullptr )
        {
            if ( !g_is_the_packer() ) {
                this->unload_current_level();
            }

            g_femanager.ReleaseIGO();
            g_femanager.ReleaseFonts();
            g_femanager.ReleaseFrontEnd();

            auto *partition = resource_manager::get_partition_pointer(RESOURCE_PARTITION_HERO);
            assert(partition != nullptr);
            assert(partition->get_streamer() != nullptr);

            auto *v5 = partition->get_streamer();
            v5->flush(nullptr);
            v5->unload_all();
            v5->flush(nullptr);

            partition = resource_manager::get_partition_pointer(RESOURCE_PARTITION_LANG);
            assert(partition != nullptr);
            assert(partition->get_streamer() != nullptr);
            auto *v6 = partition->get_streamer();
            v6->flush(nullptr);
            v6->unload_all();
            v6->flush(nullptr);

            partition = resource_manager::get_partition_pointer(RESOURCE_PARTITION_START);
            assert(partition != nullptr);
            assert(partition->get_streamer() != nullptr);

            auto *v7 = partition->get_streamer();
            v7->flush(nullptr);
            v7->unload_all();
            v7->flush(nullptr);

            term_subdivision();
            if ( g_world_ptr != nullptr ) {
                g_world_ptr->~world_dynamics_system();
                operator delete(g_world_ptr);
            }

            g_world_ptr = nullptr;
        }

        USOcean2Shader::Release();
        destruct_script_controllers();

        if (this->mb != nullptr) {
            this->message_board_clear();
        }

        script_manager::clear();
        if ( !g_is_the_packer() ) {
            this->one_time_deinit_stuff();
        }

        script_manager::kill();

        occlusion::term();

        scratchpad_stack::term();

    } else {
        THISCALL(0x00559D10, this);
    }
}

void game::begin_hires_screenshot(int a2, int a3)
{
    hires_screenshot::params::width() = a2;
    hires_screenshot::params::height() = a3;
    this->push_process(hires_screenshot::process());
}

void game::enable_marky_cam(bool a2, bool a3, Float a4, Float a5) {
    //sp_log("enable_marky_cam: 0x%08X", v6->m_vtbl);

    if constexpr (0) {
        auto *v5 = this->the_world;
        auto *marky_cam = v5->field_28.field_44;

        assert(marky_cam != nullptr);

        auto *v7 = &v5->field_28;
        marky_cam->field_1DC = a5;
        if ((a2 && a4 >= marky_cam->field_1D8) || (!a2 && a4 == marky_cam->field_1D8)) {
            if (a2) {
                if (a3) {
                    marky_cam->sync(*this->field_5C);
                }
            } else {
                this->current_game_camera = g_spiderman_camera_ptr();
                if (this->current_game_camera == nullptr) {
                    return;
                }

                this->set_current_camera(bit_cast<camera *>(this->current_game_camera), false);

                if (a3) {
                    g_spiderman_camera_ptr()->sync(*marky_cam);
                }
            }

            v7->enable_marky_cam(a2, a4);
            marky_cam->camera_set_collide_with_world(false);
        }

    } else {
        THISCALL(0x005241E0, this, a2, a3, a4, a5);
    }
}



void game::load_complete() {
    g_game_ptr->level.load_completed = true;
}

static Var<game_process> lores_game_process{0x00922074};

void game::push_lores()
{
    this->push_process(lores_game_process());
}

void game::push_process(game_process &process)
{
    void (__fastcall *sub_570FD0)(void *, void *, void *) = CAST(sub_570FD0, 0x00570FD0);

    sub_570FD0(&this->process_stack, nullptr, &process);

    auto &last_proc = this->process_stack.back();
    last_proc.index = 0;
    last_proc.field_10 = 0;
}

void game::render_intros() {
    if constexpr (0) {
    } else {
        THISCALL(0x00557B80, this);
    }
}

void game::one_time_deinit_stuff()
{
    {
        tlFixedString a1{"dropshadow"};
        nglReleaseMeshFile(a1);
    }

    {
        tlFixedString a1{"vcl_car_shadow"};
        nglReleaseMeshFile(a1);
    }

    {
        if (g_console != nullptr) {
            mem_dealloc(g_console, sizeof(Console));
        }

        g_console = nullptr;
    }

    this->field_B4 = nullptr;
    this->field_B8 = nullptr;
    trigger_manager::instance->deinit();
    terrain_types_manager::delete_inst();
}

game_settings *game::get_game_settings() {
    assert(gamefile != nullptr);

    return this->gamefile;
}

game_state game::get_cur_state() const
{
    assert(process_stack.size() != 0);

    auto &last = this->process_stack.back();

    return static_cast<game_state>(last.field_4[last.index]);
}

static Var<int> dword_92255C{0x0092255C};

static Var<bool> byte_922558{0x00922558};

static Var<bool> byte_921D79{0x00921D79};

void sub_5935D0() {
    if (g_shadow_scene() == nullptr) {
        g_shadow_scene() = nglCurScene();
    }
}

void UploadLightConsts(unsigned int *&, void *) {}

void sub_510770(unsigned int *&, void *) {}

void game::render_world()
{
    TRACE("game::render_world");

    if constexpr (0)
    {
        auto *v3 = this->get_current_view_camera(0);
        if (v3 != nullptr)
        {
            auto fov = v3->get_fov();

            if (fov >= 0.0024999999f) {
                if (dword_92255C() != g_TOD()) {
                    ++dword_91E1D8();
                    dword_92255C() = g_TOD();
                }

                auto fpf = DEG_TO_RAD(os_developer_options::instance->get_int(mString {"CAMERA_FOV"}));
                if (fov == fpf) {
                    g_tan_half_fov_ratio() = 1.0f;
                } else {
                    fpf = std::tan(fov * 0.5f) / std::tan(fpf * 0.5f);
                    g_tan_half_fov_ratio() = fpf;
                }
				

                g_indoors() = v3->is_indoors();
                nglSetClearFlags(0);
                nglListBeginScene(static_cast<nglSceneParamType>(1));
                sub_5935D0();
                nglListEndScene();
                auto *v6 = comic_panels::get_panel_params();
                auto *v7 = v6;
                if ((!v6 || (v6->field_0 & 0x40) != 0) && !g_indoors() && byte_922558())
                {
                    nglListBeginScene(static_cast<nglSceneParamType>(1));
                    nglCurScene()->field_408 = true;
                    nglSetSceneCallBack(static_cast<nglSceneCallbackType>(0), UploadLightConsts, nullptr);
                    nglSetSceneCallBack(static_cast<nglSceneCallbackType>(2), sub_510770, nullptr);
                    v3->adjust_geometry_pipe(false);
                    geometry_manager::set_far_plane(12000.0);
                    nglCalculateMatrices(0);

                    for (auto &v10 : this->the_world->field_23C)
                    {
                        if (v10->is_visible()) {
                            v10->render(1.0f);
                        }
                    }

                    nglListEndScene();
                }

                nglListBeginScene(static_cast<nglSceneParamType>(1));
                fpf = v3->get_far_plane_factor();

                assert(fpf >= 0.0001f && fpf <= 1.0f);

                auto v11 = fpf * 10000.0f;

                geometry_manager::set_far_plane(v11);

                if ( (byte_921D79()
                        && !this->field_16E)
                        || g_cut_scene_player()->is_playing())
                {
                    v3->adjust_geometry_pipe(false);
                    auto *v13 = g_femanager.IGO->field_44;
                    auto v14 = v13->field_5C4 || v13->field_5C3;
                    if (!g_distance_clipping_enabled() || v14) {
                        if (g_renderState().field_88) {
                            g_Direct3DDevice()->lpVtbl->SetRenderState(g_Direct3DDevice(),
                                                                       D3DRS_FOGENABLE,
                                                                       0);
                            g_renderState().field_88 = false;
                        }

                    } else {
                        auto v15 = g_distance_clipping() * LARGE_EPSILON * 1900.0f + 100.0f;
                        float a2 = v15;
                        auto v16 = v15 - 200.0f;
                        auto fov_1 = v16;
                        if (v16 < 100.0f) {
                            fov_1 = 100.0;
                        }

                        g_renderState().setFogEnable(true);

                        static Var<uint32_t[4]> dword_922548{0x00922548};

                        g_renderState().setFogColor(dword_922548()[g_TOD()]);
                        if (g_renderState().field_90 != 3) {
                            g_Direct3DDevice()->lpVtbl->SetRenderState(g_Direct3DDevice(),
                                                                       D3DRS_FOGTABLEMODE,
                                                                       3);
                            g_renderState().field_90 = 3;
                        }

                        g_renderState().sub_55DFE0(fov_1);
                        g_renderState().sub_55E010(a2);
                    }

                    nglCalculateMatrices(0);
                    auto v17 = g_disable_occlusion_culling();
                    if (g_indoors()) {
                        auto *v18 = g_cut_scene_player();
                        if (v18->field_E1 || v18->field_E2) {
                            g_disable_occlusion_culling() |= 3u;
                        }
                    }

                    this->the_world->field_A0.render(*v3, 0);
                    g_disable_occlusion_culling() = v17;
                    if ((!v7 || (v7->field_0 & 0x20) != 0) && !g_indoors()) {
                        USOcean2Shader::Draw(v3->get_abs_po().m[3]);
                    }
                }

                nglListEndScene();
                if (!v7 || (v7->field_0 & 0x20) != 0) {
                    this->the_world->field_A0.render_stencil_shadows(*v3);
                }
            }
        }

    }
    else
    {
        THISCALL(0x0054E1B0, this);
    }
}

void game::advance_state_legal(Float a2)
{
    //sp_log("advance_state_legal: start");

    if constexpr (1)
    {
        mString v12 {"legalscreen"};

        resource_key v11{string_hash{"legalscreen"}, RESOURCE_KEY_TYPE_PACK};

        this->clear_screen();

        limited_timer_base v10{};

        auto *my_partition = resource_manager::get_partition_pointer(RESOURCE_PARTITION_START);
        assert(my_partition != nullptr);

        resource_pack_streamer *streamer = my_partition->get_streamer();
        assert(streamer != nullptr);

        streamer->load("game", 0, nullptr, nullptr);
        streamer->flush(RenderLoadMeter);

        auto *resource_context = resource_manager::get_best_context(RESOURCE_PARTITION_START);

        resource_manager::push_resource_context(resource_context);
        localized_string_table::load_localizer();
        this->one_time_init_stuff();

        {
            auto &process = this->process_stack.back();
            assert(++process.index < process.num_states);
        }

    } else {
        THISCALL(0x00558100, this, a2);
    }

    //sp_log("advance_state_legal: end");
}

void game::handle_frame_locking(float *time_inc) {
    auto frame_lock = os_developer_options::instance->get_int(mString {"FRAME_LOCK"});
    if (frame_lock > 0) {
        *time_inc = 0.5 / frame_lock;
    }
    
    // Slow motion implementation
    auto slow_motion_enabled = os_developer_options::instance->get_int(mString {"SLOW_MOTION_ENABLED"});
    if (slow_motion_enabled) {
        *time_inc *= 0.25f; // Quarter speed (adjust as needed)
    }
}


float game::handle_frame_locking2(float time_inc) {
    // Get frame lock setting (target FPS)
    auto frame_lock = os_developer_options::instance->get_int(mString{"FRAME_LOCK"});
    
    // Get slow motion factor (1.0 = normal speed, 0.5 = half speed, 2.0 = double speed)
    auto slow_motion_factor = os_developer_options::instance->get_int(mString{"SLOW_MOTION_FACTOR"});
    
    // Default to normal speed if not set
    if (slow_motion_factor <= 0.0f) {
        slow_motion_factor = 1.0f;
    }
    
    if (frame_lock > 0) {
        // Calculate base time increment (assuming 60 FPS base or 0.0166667 seconds per frame)
        // Using 1.0 instead of 0.5 for standard frame time calculation
        float base_time_inc = 1.0f / static_cast<float>(frame_lock);
        
        // Apply slow motion factor
        return base_time_inc * slow_motion_factor;
    } else {
        // If frame lock is disabled, still apply slow motion to the current time increment
        return time_inc * slow_motion_factor;
    }
}



// Alternative implementation with more control options
float game::handle_frame_locking_advanced(float time_inc) {
    // Get frame lock setting
    auto frame_lock = os_developer_options::instance->get_int(mString{"FRAME_LOCK"});
    
    // Get slow motion settings
    auto slow_motion_enabled = os_developer_options::instance->get_int(mString{"FRAME_LOCK"});
    auto slow_motion_factor = os_developer_options::instance->get_int(mString{"FRAME_LOCK"});
    auto slow_motion_smooth = os_developer_options::instance->get_int(mString{"FRAME_LOCK"});
    
    // Static variable for smooth transitions
    static float current_time_scale = 1.0f;
    
    // Determine target time scale
    float target_time_scale = slow_motion_enabled ? slow_motion_factor : 1.0f;
    
    // Smooth transition if enabled
    if (slow_motion_smooth) {
        float lerp_speed = 0.1f; // Adjust for smoother/faster transitions
        current_time_scale += (target_time_scale - current_time_scale) * lerp_speed;
    } else {
        current_time_scale = target_time_scale;
    }
    
    // Clamp time scale to reasonable values
    current_time_scale = std::max(0.01f, std::min(10.0f, current_time_scale));
    
    float result_time_inc;
    if (frame_lock > 0) {
        // Calculate frame-locked time increment
        float base_time_inc = 1.0f / static_cast<float>(frame_lock);
        result_time_inc = base_time_inc * current_time_scale;
    } else {
        // Apply time scale to existing time increment
        result_time_inc = time_inc * current_time_scale;
    }
    
    // Optional: Debug output
    #ifdef DEBUG_FRAME_TIMING
    static int frame_counter = 0;
    if (++frame_counter % 60 == 0) {
        printf("Frame Lock: %d, Time Scale: %.2f, Time Inc: %.6f\n", 
               frame_lock, current_time_scale, result_time_inc);
    }
    #endif
    
    return result_time_inc;
}

// Simple version matching your exact request
float game::handle_frame_locking_simple(float time_inc) {
    auto frame_lock = os_developer_options::instance->get_int(mString{"FRAME_LOCK"});
    
    // Simple slow motion: get a multiplier value
    // 1.0 = normal, 0.5 = half speed, 2.0 = double speed
    auto time_scale = os_developer_options::instance->get_int(mString{"TIME_SCALE"});
    if (time_scale <= 0.0f) time_scale = 1.0f;
    
    if (frame_lock > 0) {
        return (0.5f / static_cast<float>(frame_lock)) * time_scale;
    } else {
        // If no frame lock, just apply time scale
        return time_inc * time_scale;
    }
}


void game_packs_modified_callback(_std::vector<resource_key> &a1) {
    CDECL_CALL(0x0054F6D0, &a1);
}

void game::one_time_init_stuff()
{
    TRACE("game::one_time_init_stuff");

    if constexpr (1)
    {
        comic_panels::init();
        g_femanager.LoadFonts();
        g_femanager.InitIGO();
        subtitles_init();

        terrain_types_manager::create_inst();
        this->message_board_init();

        resource_manager::add_resource_pack_modified_callback(game_packs_modified_callback);

        if (g_console == nullptr) {
            g_console = new Console {};
        }

        tlFixedString a1 {"dropshadow"};
        this->field_B4 = nglGetFirstMeshInFile(a1);

        a1 = tlFixedString {"vcl_car_shadow"};
        this->field_B8 = nglGetFirstMeshInFile(a1);
    }
    else
    {
        THISCALL(0x00552E50, this);
    }
}

void game::pop_process()
{
    assert(process_stack.size() != 0);

    if constexpr (1) {
        this->process_stack.pop_back();
    } else {
        auto v1 = this->process_stack.begin();
        if (v1 != this->process_stack.end()) {
            if ((this->process_stack.end() - v1)) {
                --this->process_stack.m_last;
            }
        }
    }
}

void game::set_current_camera(camera *a2, bool a3)
{
    if constexpr (1)
    {
        this->field_5C = a2;
        if (a3 && a2->is_a_game_camera()) {
            bit_cast<game_camera *>(a2)->field_12C = false;
        }

        this->field_64 = this->field_5C->field_C0;

    } else {
        THISCALL(0x00514FD0, this, a2, a3);
    }
}

bool game::level_is_loaded() const
{
    return this->flag.level_is_loaded;
}

bool game::is_physics_enabled() const
{
    return this->flag.physics_enabled;
}

bool game::is_paused() const
{
    return this->flag.game_paused;
}

bool game::is_marky_cam_enabled() const
{
    return this->the_world->field_28.is_marky_cam_enabled();
}
#include "cursor.h"
static Var<bool> sounds_paused{0x00960044};

void game::pause() {
    TRACE("game::pause");

    if (!this->field_15F || !fe_controller_disconnect::get_currently_plugged_in()) {
        if (this->flag.game_paused) {
            this->unpause();
        } else {
            IGOZoomOutMap *v2 = nullptr;

            if (!fe_controller_disconnect::get_currently_plugged_in() ||
                g_femanager.m_pause_menu_system->IsDialogActivated() ||
                (v2 = g_femanager.IGO->field_44, v2->field_5C4) || v2->field_5C3) {
                if (!g_femanager.m_pause_menu_system->IsDialogActivated() &&
                    fe_controller_disconnect::get_currently_plugged_in()) {
                    auto *v6 = g_femanager.IGO->field_44;
                    if (!v6->field_5C4 && !v6->field_5C3) {
                        sound_manager::fade_sounds_by_type(15u, 0.5, 0.5, 0);
                    }
                }


                sounds_paused() = false;
            } else {
                auto *v3 = g_cut_scene_player();
                if (v3->is_playing()) {
                    Var<float> dword_9682D0{0x009682D0};
                    auto v9 = dword_9682D0();
                    dword_9682D0() = 0.06666667;
                    auto *v4 = g_cut_scene_player();
                    v4->frame_advance(0.06666667);

                    auto v8 = dword_9682D0();
                    auto *v5 = g_cut_scene_player();
                    v5->frame_advance(v8);
                    dword_9682D0() = v9;
                }

                sound_manager::fade_sounds_by_type(127u, 0.0, 0.13333334, 1);
                sounds_paused() = true;
            }

            this->push_process(pause_process);

            this->flag.game_paused = true;
            auto *v7 = input_mgr::instance->rumble_ptr;
            if (v7 != nullptr)
            {
                if (v7->field_5C) {
                    v7->get_current_rumble_info(this->field_290);
                    v7->stop_vibration();
                    this->field_2B5 = true;
                } else {
                    this->field_2B5 = false;
					            if (sounds_paused()) {
                sound_manager::unpause_all_sounds();
            }
                }
            }
        }
    }
}

void game::unpause()
{
    if constexpr (1)
    {
        if (this->flag.game_paused && !g_femanager.m_pause_menu_system->IsDialogActivated())
        {
            if (sounds_paused()) {
                sound_manager::unpause_all_sounds();
            }

            sound_manager::fade_sounds_by_type(255u, 1.0, 0.5, 0);
            auto &v2 = this->process_stack.back();
            auto v3 = v2.index;
            if (v2.field_4[v3] == 7) {
                v2.index = v3 + 1;
            }

            if (this->field_2B5) {
                auto *v4 = input_mgr::instance->rumble_ptr;
                if (v4 != nullptr) {
                    if (this->field_290.field_1D) {
                        rumble_struct v5{this->field_290};
                        v4->vibrate(v5);
                        this->field_2B5 = false;
                    }
                }
            }
            this->flag.game_paused = 0;
        }

    } else {
        THISCALL(0x0053A880, this);
    }
}

void game::advance_state_paused(Float a1)
{
    TRACE("game::advance_state_paused():");

    if constexpr (1) {
        auto *v3 = input_mgr::instance->rumble_ptr;
        if (v3 != nullptr && g_femanager.m_pause_menu_system->m_index != 4) {
            v3->stop_vibration();
        }

        auto *v4 = g_cut_scene_player();
        if (v4->field_E1 || v4->field_E2) {
            auto *v5 = g_cut_scene_player();
            v5->frame_advance_lite(a1);
        }

        this->frame_advance_game_overlays(a1);

        this->the_world->update_ai_and_visibility_proximity_maps_for_moved_entities(a1);
        this->the_world->update_collision_proximity_maps_for_moved_entities(a1);

        this->the_world->the_terrain->frame_advance(a1);

        spider_monkey::frame_advance(a1);

        auto *v6 = static_cast<Float *>(
            script_manager::get_game_var_address(mString{"g_game_paused"}, nullptr, nullptr));

        *v6 = 1.0;
    } else {
        THISCALL(0x00558220, this, a1);
    }

    //sp_log("game::advance_state_paused(): end");
}

void game::clear_screen()
{
    TRACE("game::clear_screen");

    if constexpr (1) {
        world_dynamics_system *world_dyn_system = this->the_world;
        auto red = world_dyn_system->field_A0.field_60.r;
        auto green = world_dyn_system->field_A0.field_60.g;
        auto blue = world_dyn_system->field_A0.field_60.b;

        for (auto i = 0u; i < 2u; ++i) {
            nglListInit();
            nglSetClearFlags(7u);
            nglSetClearColor(red, green, blue, 1.0);
            nglListSend(true);
        }
    } else {
        THISCALL(0x00515140, this);
    }
}

static Var<float> waiting_time{0x00960B60};

void game::advance_state_wait_link(Float a2) {
    if constexpr (1) {
        if (!link_system::use_link_system() ||
            !os_developer_options::instance->get_flag(mString{"WAIT_FOR_LINK"})) {
            ++this->process_stack.back().index;
        }

        auto v2 = waiting_time() > 10.0f;

        if (v2 || this->field_171) {
            ++this->process_stack.back().index;
        }

        waiting_time() += a2;

    } else {
        THISCALL(0x00536410, this, a2);
    }
}

static Var<bool> byte_96852C {0x0096852C};

static Var<int> dword_95C8E8 {0x0095C8E8};

static Var<mString> stru_969DE0 {0x00969DE0};

void sub_5C6700([[maybe_unused]] chunk_file &a1, const mString &a2)
{
    mString result;

    mString v4 = a2;
    v4.remove_leading(" \n\t\r");
    v4.remove_trailing(" \n\t\r");
    if ( v4.size() != 0 )
    {
        if ( byte_96852C() || a2.find(" ", 0) == -1 && a2.find("\t", 0) == -1 )
        {
            result = a2 + stru_969DE0();
        }
        else
        {
            auto v2 = "\"" + a2;
            mString v3 = v2 + "\"";
            [[maybe_unused]] mString v6 = v3 + stru_969DE0();
        }
    }
    else
    {
        result = "\"\""+ stru_969DE0();
    }
}

camera * get_scene_analyzer_cam()
{
    return (camera *) entity_handle_manager::find_entity(string_hash {"SCENE_ANALYZER_CAM"}, entity_flavor_t::CAMERA, false);
}

void sub_5975C0(const char *Format, bool a2, bool a3)
{
    if ( os_developer_options::instance->get_flag(static_cast<os_developer_options::flags_t>(128)) && OpenClipboard(0) )
    {
        char Dest[4096] {};
        if ( a2 )
        {
            char Filename[264] {};
            GetModuleFileNameA(nullptr, Filename, 260u);
            auto *v3 = strrchr(Filename, 92);
            char *v4 = nullptr;
            if ( v3 != nullptr ) {
                v4 = v3 + 1;
            } else {
                v4 = Filename;
            }

            if ( a3 ) {
                sprintf(Dest, "Final Application: %s\r\n\r\n%s\r\n", v4, Format);
            } else {
                sprintf(Dest, Format);
            }
        }
        else
        {
            strncpy(Dest, Format, 4095u);
        }

        Dest[4095] = '\0';
        auto v5 = GlobalAlloc(8258u, strlen(Dest) + 1);
        auto *v6 = v5;
        if ( v5 )
        {
            strcpy((char *)GlobalLock(v5), Dest);
            GlobalUnlock(v6);
            EmptyClipboard();
            SetClipboardData(1u, v6);
        }

        CloseClipboard();
        GlobalFree(v6);
    }
}

static Var<bool> g_debug_cam_get_prev_target {0x0095C75D};

void game::handle_cameras(input_mgr *a2, const Float &time_inc)
{
    TRACE("game::handle_cameras");

    if (this->field_5C != nullptr) {

        sp_log("0x%08X", this->field_5C->m_vtbl);
    }

    sp_log("%d %d", this->is_user_camera_enabled(),
            os_developer_options::instance->get_int(static_cast<os_developer_options::ints_t>(2)));

    if constexpr (1)
    {
        if ( !this->flag.level_is_loaded ) {
            return;
        }

        auto CAMERA_STATE = os_developer_options::instance->get_int(mString {"CAMERA_STATE"});
        if ( this->is_user_camera_enabled() && CAMERA_STATE != 1 ) {
            this->set_camera(1);
        }

        if ( !this->is_user_camera_enabled() && CAMERA_STATE != 0
                && !geometry_manager::is_scene_analyzer_enabled() )
        {
            this->set_camera(0);
        }

        if ( AXIS_MAX == a2->get_control_delta(30, INVALID_DEVICE_ID)
            && AXIS_MAX == a2->get_control_state(30, INVALID_DEVICE_ID) )
        {
            enum {
                CHASE_CAM = 0,
                USER_CAM = 1,
            };

            auto CAMERA_STATE = os_developer_options::instance->get_int(mString {"CAMERA_STATE"});
            if ( CAMERA_STATE != 0 )
            {
                if ( CAMERA_STATE == USER_CAM )
                {
                    if ( geometry_manager::is_scene_analyzer_enabled() ) {
                        geometry_manager::enable_scene_analyzer(false);
                    }

                    g_game_ptr->enable_user_camera(false);
                }
            }
            else
            {
                g_game_ptr->enable_user_camera(true);
            }
        }

        if ( AXIS_MAX == a2->get_control_delta(GRAVITY_TOGGLE, INVALID_DEVICE_ID) ) {
            os_developer_options::instance->toggle_flag(static_cast<os_developer_options::flags_t>(23u));
        }

        if ( AXIS_MAX == a2->get_control_delta(STOP_PHYSICS, INVALID_DEVICE_ID) ) {
            this->flag.physics_enabled = !this->flag.physics_enabled;
        }

        if ( AXIS_MAX == a2->get_control_delta(SINGLE_STEP, INVALID_DEVICE_ID)
            && AXIS_MID == a2->get_control_state(PLANE_BOUNDS_MOD, INVALID_DEVICE_ID) )
        {
            this->flag.single_step = true;
        }

        if ( AXIS_MAX == a2->get_control_delta(119, INVALID_DEVICE_ID) )
        {
            auto *v8 = g_world_ptr->get_hero_ptr(0);

            auto v11 = v8->get_abs_position();

            auto v15 = this->field_5C->get_abs_position();

            char Dest[256] {};
            _snprintf(
                Dest,
                255u,
                "/* hero */ vector3d( %f, %f, %f )\n/* cam */ vector3d( %f, %f, %f )\n",
                v11[0],
                v11[1],
                v11[2],
                v15[0],
                v15[1],
                v15[2]);
            sub_5975C0(Dest, false, true);
        }

        if ( AXIS_MAX == a2->get_control_delta(SHOW_DEBUG_INFO, INVALID_DEVICE_ID) ) {
            os_developer_options::instance->toggle_flag(static_cast<os_developer_options::flags_t>(20));
        }

        if ( AXIS_MAX == a2->get_control_delta(USERCAM_EQUALS_CHASECAM, INVALID_DEVICE_ID)
                && !this->flag.game_paused )
        {
            auto *ent = entity_handle_manager::find_entity(string_hash {"USER_CAM"}, IGNORE_FLAVOR, false);
            auto *v18 = entity_handle_manager::find_entity(string_hash {"CHASE_CAM"}, IGNORE_FLAVOR, false);
            if ( ent != nullptr && v18 != nullptr )
            {
                auto &abs_po = v18->get_abs_po();
                ent->set_abs_po(abs_po);
            }

            if ( g_mouselook_controller() != nullptr ) {
                g_mouselook_controller()->reset();
            }

            cam_target_locked() = false;
        }

        if ( AXIS_MAX == a2->get_control_delta(33, INVALID_DEVICE_ID) ) {
            g_debug_cam_get_next_target() = true;
        }

        if ( AXIS_MAX == a2->get_control_delta(34, INVALID_DEVICE_ID) ) {
            g_debug_cam_get_prev_target() = true;
        }

        if ( this->flag.game_paused )
        {
            g_debug_cam_get_next_target() = false;
            g_debug_cam_get_prev_target() = false;
        }

        if ( g_debug_cam_get_next_target() || g_debug_cam_get_prev_target() )
        {
            int v5 = 0;

            camera * arr_camera[64] {};

            if ( ai::ai_core::the_ai_core_list_high() != nullptr )
            {
                for ( auto &the_core : (*ai::ai_core::the_ai_core_list_high()) )
                {
                    if (the_core == nullptr) {
                        continue;
                    }

                    auto *v24 = the_core->field_64;
                    if ( v24 == nullptr ) {
                        continue;
                    }

                    auto *v25 = v24->get_ai_core();
                    if ( ! ai::pedestrian_inode::is_a_pedestrian(v25) && v5 < 64 ) {
                        arr_camera[v5++] = bit_cast<camera *>(v24);
                    }
                }
            }
            else
            {
                g_debug_cam_target_actor() = nullptr;
                g_debug_cam_get_next_target() = false;
                g_debug_cam_get_prev_target() = false;
            }

            if ( v5 > 0 )
            {
                auto *v26 = g_debug_cam_target_actor();
                if ( g_debug_cam_target_actor() == nullptr )
                {
                    g_debug_cam_target_actor() = arr_camera[0];
                }

                if ( g_debug_cam_get_next_target() )
                {
                    int i = 0;
                    for (; v26 != arr_camera[i] && i < v5; ++i) {
                        ;
                    }

                    if ( ++i >= v5 ) {
                        i = 0;
                    }

                    g_debug_cam_target_actor() = arr_camera[i];
                    g_debug_cam_get_next_target() = false;
                }

                if ( g_debug_cam_get_prev_target() )
                {
                    int i = v5 - 1;
                    for (; v26 != arr_camera[i] && i >= 0; --i) {
                        ;
                    }

                    if ( --i < 0 ) {
                        i = v5 - 1;
                    }

                    g_debug_cam_target_actor() = arr_camera[i];
                    g_debug_cam_get_prev_target() = false;
                }
            }
        }

        if ( AXIS_MAX == a2->get_control_delta(51, INVALID_DEVICE_ID) )
        {
            static char byte_960C08[128] {};

            byte_960C08[0] = '\0';
            auto *v34 = entity_handle_manager::find_entity(string_hash {"USER_CAM"}, entity_flavor_t::CAMERA, false);

            auto abs_pos = v34->get_abs_position();

            auto *v37 = &byte_960C08[strlen(byte_960C08)];

            static Var<vector3d> dword_88ACD4 {0x0088ACD4};
            auto v39 = dword_88ACD4()[1];
            auto v40 = dword_88ACD4()[2];
            *((float *)v37 + 0) = dword_88ACD4()[0];

            static Var<int16_t> word_88ACE0 {0x0088ACE0};
            auto v41 = word_88ACE0();
            *((float *)v37 + 1) = v39;
            *((float *)v37 + 2) = v40;
            *((int16_t *)v37 + 6) = v41;


            strcat(byte_960C08, mString {0, "%f ", abs_pos[0]}.c_str());
            strcat(byte_960C08, mString {0, "%f ", 1.0f}.c_str());
            strcat(byte_960C08, mString {0, "%f ", abs_pos[2]}.c_str());

            auto *v42 = &byte_960C08[strlen(byte_960C08)];

            static Var<int> dword_88ACC4 {0x0088ACC4};
            static Var<int> dword_88ACC8 {0x0088ACC8};

            auto v43 = dword_88ACC8();

            static Var<char> byte_88ACCC {0x0088ACCC};
            auto v44 = byte_88ACCC();
            *((int *)v42 + 0) = dword_88ACC4();
            *((int *)v42 + 1) = v43;
            v42[8] = v44;
            sub_5975C0(byte_960C08, true, false);
        }

        if ( AXIS_MAX == a2->get_control_delta(50, INVALID_DEVICE_ID) ) {
            GetAsyncKeyState(VK_MENU);
        }

        if ( os_developer_options::instance->get_flag(mString {"CAMERA_EDITOR"})
            && AXIS_MAX == a2->get_control_delta(49, INVALID_DEVICE_ID) )
        {
            chunk_file file {};
            mString v147 = this->level.name_mission_table + "_caminfo.txt";
            file.open(v147, os_file::FILE_MODIFY);
            if ( file.is_open() )
            {
                auto v143 = "Unable to open file " + v147 + " for output.";
                sp_log("%s", v143);
            }
            else
            {
                file.set_fp(0, os_file::FP_END);
                byte_96852C() = true;
                mString v46 {dword_95C8E8()};
                mString v142 = "camera_sequence_" + v46 + "()";
                sub_5C6700(file, v142);

                mString v48 {"{"};
                sub_5C6700(file, v48);

                mString v49 {this->field_1F8[0][2]};
                mString v50 {this->field_1F8[0][1]};
                mString v51 {this->field_1F8[0][0]};
                mString v140 {this->field_180[0][2]};
                mString v141 {this->field_180[0][1]};
                mString v52 {this->field_180[0][0]};
                mString v53 = "  cut_to(vector3d(" + v52;
                auto v55 = v53 + "," + v141;
                auto v57 = v55 + "," + v140;
                auto v60 = v57 + "), " + "vector3d(" + v51;
                auto v61 = v60 + ",";
                auto v63 = v61 + v50 + ",";
                mString v64 = v63 + v49;
                mString v149 = v64 + "));";
                sub_5C6700(file, v149);

                int v65 = 1;
                if ( this->field_270 > 1 )
                {
                    auto *v66 = &this->field_180[1][0];
                    do
                    {
                        mString v67 {2.0f};
                        mString v140 {v66[2]};
                        mString v141 {v66[1]};
                        mString v68 {v66[0]};
                        auto v69 = "  spawn dolly(vector3d(" + v68;
                        auto v70 = v69 + ",";
                        auto v71 = v70 + v141;
                        auto v72 = v71 + ",";
                        auto v73 = v72 + v140;
                        auto v74 = v73 + "), ";
                        auto v75 = v74 + v67;
                        mString var404 = v75 + ");";

                        auto v79 = "  pan(vector3d(" + mString {v66[30]} + ",";
                        auto v81 = v79 + mString {v66[31]} + ",";
                        auto v82 = v81 + mString {v66[32]};
                        auto v84 = v82 + "), " + mString {2.0f};
                        mString v146 = v84 + ");";
                        sub_5C6700(file, var404);
                        sub_5C6700(file, v146);
                        ++v65;
                        v66 += 3;
                    }
                    while ( v65 < this->field_270 );
                }

                mString v85 {"}"};
                sub_5C6700(file, v85);

                byte_96852C() = false;
                file.close();

                auto v143 = "Dump Dolly info: " + v142;
            }

            ++dword_95C8E8();
        }

        if ( os_developer_options::instance->get_flag(mString {"CAMERA_EDITOR"}) )
        {
            if ( AXIS_MAX == a2->get_control_delta(47, INVALID_DEVICE_ID) )
            {
                mString v86 {this->field_270};
                mString v143 = v86 + " Recorded.";
                mString v138 {v143.c_str()};
                this->mb->post(*bit_cast<message_board::string *>(&v138), 2.0f, color32 {0xFFFFFFFF});
            }

            if ( AXIS_MAX == a2->get_control_state(48, INVALID_DEVICE_ID) )
            {
                static uint32_t arr[10] {};
                std::iota(std::begin(arr), std::end(arr), 0);
                for (auto &i : arr)
                {
                    if ( AXIS_MAX == a2->get_control_delta(35 + i, INVALID_DEVICE_ID) && this->field_270 > i )
                    {
                        this->field_5C->set_abs_position(this->field_180[i]);
                        mString v138 = "Cam Position " + mString {static_cast<int>(i + 1)};
                        this->mb->post(*bit_cast<message_board::string *>(&v138), 2.0f, color32 {0xFFFFFFFF});
                    }
                }
            }
            else
            {
                for ( uint32_t i {0}; i < 10; ++i )
                {
                    if ( AXIS_MAX == a2->get_control_delta(35 + i, INVALID_DEVICE_ID) )
                    {
                        this->field_180[i] = this->field_5C->get_abs_position();

                        vector3d v142 {0.0, 0.0, 2.0f};

                        auto *v90 = this->field_5C;
                        this->field_1F8[i] = v90->get_abs_po().slow_xform(v142);
                        if ( this->field_270 < i + 1 ) {
                            this->field_270 = i + 1;
                        }

                        mString v138 = "Mark Dolly State " + mString {static_cast<int>(i + 1)};
                        this->mb->post(*bit_cast<message_board::string *>(&v138), 2.0f, color32 {0xFFFFFFFF});
                    }
                }
            }

            if ( AXIS_MAX == a2->get_control_delta(45, INVALID_DEVICE_ID) )
            {
                auto v118 = this->field_270;
                if ( v118 > 1 )
                {
                    this->field_274 = (v118 - 1) + (v118 - 1);
                    mString v138 {"Do Dolly"};
                    this->mb->post(*bit_cast<message_board::string *>(&v138), 2.0f, color32 {0xFFFFFFFF});
                }
            }

            if ( AXIS_MAX == a2->get_control_delta(46, INVALID_DEVICE_ID)
                && AXIS_MAX == a2->get_control_state(48, INVALID_DEVICE_ID) )
            {
                this->field_270 = 0;
                mString v138 {"Dolly Clear"};
                this->mb->post(*bit_cast<message_board::string *>(&v138), 2.0f, color32 {0xFFFFFFFF});
            }
        }

        if ( time_inc != 0.0f && (!this->flag.physics_enabled || this->flag.single_step) )
        {
            auto *the_world = this->the_world;
            if ( the_world->field_28.is_marky_cam_enabled() )
            {
                auto *v120 = the_world->field_28.field_44;
                if ( this->current_game_camera != v120 )
                {
                    this->current_game_camera = v120;
                    this->set_current_camera(bit_cast<camera *>(this->current_game_camera), true);
                }

                this->field_5C->frame_advance(time_inc);
            }
            else
            {
                assert(time_inc > 0 && time_inc < 10.0f);

                auto *v121 = this->field_5C;
                if ( v121 != the_world->get_chase_cam_ptr(0) ) {
                    v121->frame_advance(time_inc);
                }

                if ( !v121->is_externally_controlled() )
                {
                    auto *v123 = this->the_world;
                    auto *v124 = v123->get_chase_cam_ptr(0);
                    if ( this->current_game_camera != v124 )
                    {
                        if ( v121 == this->current_game_camera ) {
                            this->set_current_camera(this->the_world->get_chase_cam_ptr(0), false);
                        }

                        this->current_game_camera = this->the_world->get_chase_cam_ptr(0);
                    }
                }

                if ( g_world_ptr->get_num_players() == 0 )
                {
                    auto *v126 = this->the_world->get_chase_cam_ptr(0);
                    if ( v126 != nullptr ) {
                        v126->frame_advance(time_inc);
                    }
                }

                for (int i {0}; i < g_world_ptr->get_num_players(); ++i)
                {
                    auto *v129 = this->the_world->get_chase_cam_ptr(i);
                    if ( v129 != nullptr ) {
                        v129->frame_advance(time_inc);
                    }
                }
            }
        }

        this->field_5C->adjust_geometry_pipe(false);
        if ( geometry_manager::is_scene_analyzer_enabled() )
        {
            auto *scene_analyzer_cam = get_scene_analyzer_cam();
            scene_analyzer_cam->adjust_geometry_pipe(true);
        }
    }
    else
    {
        THISCALL(0x00552F50, this, a2, &time_inc);
    }
}

void game::handle_game_states(const Float &a2)
{
    TRACE("game::handle_game_states");

    if constexpr (0)
    {
        switch (this->get_cur_state()) {
        case game_state::LEGAL: {
            this->advance_state_legal(a2);
            break;
        }
        case static_cast<game_state>(2):
            this->go_next_state();
            break;
        case static_cast<game_state>(3):
            this->go_next_state();
            break;
        case game_state::WAIT_LINK: {
            this->advance_state_wait_link(a2);
            break;
        }
        case game_state::LOAD_LEVEL: {
            this->advance_state_load_level(a2);
            break;
        }
        case game_state::RUNNING:
            this->advance_state_running(a2);
            break;
        case game_state::PAUSED:
            this->advance_state_paused(a2);
            break;
        case static_cast<game_state>(8): {
            nglBeginHiresScreenShot(hires_screenshot::params::width(),
                                    hires_screenshot::params::height());
            this->go_next_state();
            break;
        }
        case static_cast<game_state>(9): {
            if (!nglSaveHiresScreenshot()) {
                this->go_next_state();
            }

            this->advance_state_paused(a2);
            break;
        }
        case static_cast<game_state>(10):
        case static_cast<game_state>(11):
            this->advance_state_paused(a2);
            this->go_next_state();
            break;
        case static_cast<game_state>(12):
        case static_cast<game_state>(13): {
            this->go_next_state();
            break;
        }
        case static_cast<game_state>(14):
            this->pop_process();
            break;
        default:
            assert(0);
            return;
        }
    }
    else
    {
        THISCALL(0x0055D510, this, &a2);
    }
}

void game::go_next_state()
{
    assert(process_stack.size() != 0);
    this->process_stack.front().go_next_state();
}

void game::set_camera(int a2)
{
    TRACE("game::set_camera");

    if constexpr (1)
    {
        enum {
            CHASE_CAM = 0,
            USER_CAM = 1,
        };

        os_developer_options::instance->set_int(mString{"CAMERA_STATE"}, a2);
        if (a2 == USER_CAM)
        {
            auto *cam = bit_cast<game_camera *>(
                entity_handle_manager::find_entity(string_hash{"USER_CAM"},
                                                   entity_flavor_t::CAMERA,
                                                   false));

            cam->set_abs_position(this->current_game_camera->get_rel_position());
            this->set_current_camera(cam, true);

            {
                color32 v13 {0xFF, 0xFF, 0xFF, 0xFF};
                mString v11{"New Camera: User Cam"};
                message_board::string str = *bit_cast<message_board::string *>(&v11);
                this->mb->post(str, 2.0, v13);
            }

            if (g_world_ptr->get_hero_ptr(0) != nullptr) {
                auto *v7 = entity_handle_manager::find_entity(string_hash{"USER_CAM"},
                                                             entity_flavor_t::IGNORE_FLAVOR,
                                                             false);

                auto *v8 = entity_handle_manager::find_entity(string_hash{"CHASE_CAM"},
                                                              entity_flavor_t::IGNORE_FLAVOR,
                                                              false);

                entity_set_abs_po(v7, v8->get_abs_po());
            }

            if (g_mouselook_controller() != nullptr) {
                auto *v10 = g_mouselook_controller()->field_C;
                if (v10 != nullptr) {
                    v10->reset_angles();
                }
            }

            cam_target_locked() = false;
        }
        else if (a2 == CHASE_CAM)
        {
            auto *v3 = bit_cast<game_camera *>(this->current_game_camera);

            this->set_current_camera(v3, true);

            {
                mString v11{"New Camera: Chase Cam"};
                message_board::string str = *bit_cast<message_board::string *>(&v11);
                this->mb->post(str, 2.0f, color32 {0xFFFFFFFF});
            }
        }
    }
    else
    {
        THISCALL(0x0054F8C0, this, a2);
    }
}

void game::setup_input_registrations() {
    setup_input_registrations_p(this);
}

void game::setup_inputs()
{
    setup_inputs_p(this);
}

void game::reset_control_mappings()
{
    input_mgr::instance->clear_mapping();
    input_mgr::instance->scan_devices();

    this->setup_inputs();
}

static int & g_mem_checkpoint_level = var<int>(0x00921DC4);

void game::init_motion_blur()
{
    TRACE("game::init_motion_blur");

    if constexpr (0)
    {}
    else
    {
        THISCALL(0x00514AB0, this);
    }
}

void glow_init() {
    ;
}

void game::freeze_hero(bool a2)
{

        THISCALL(0x00514F00, this, a2);

}



void game::soft_reset_process()
{
    if constexpr (1)
    {
        this->process_stack.clear();

        this->push_process(main_process);
		
    }
    else
    {
        THISCALL(0x00548C70, this);
    }
}


void game::message_board_init()
{
    this->mb = new message_board {};
}


					


void game::load_this_level()
{
    TRACE("game::load_this_level");

    if constexpr (1)
    {
        assert(!flag.level_is_loaded);

        this->field_2C0.reset();
        this->field_15F = false;
        this->field_15E = false;
        this->field_160 = false;
        this->flag.game_paused = false;
        this->clear_screen();
        app::instance->field_38 += 10;
        mem_print_stats("at beginning of load_this_level()");

        g_mem_checkpoint_level = 0; // mem_set_checkpoint()
        this->init_motion_blur();
        glow_init();
        if (g_femanager.m_fe_menu_system != nullptr)
        {
            g_femanager.m_fe_menu_system->RenderLoadMeter(false);
            if (g_femanager.m_fe_menu_system != nullptr) {
                g_femanager.m_fe_menu_system->RenderLoadMeter(false);
            }
        }

        aeps::Reset();
        ambient_audio_manager::reset();

        if (g_femanager.m_fe_menu_system != nullptr) {
            g_femanager.m_fe_menu_system->RenderLoadMeter(false);
        }

        assert(level.descriptor != nullptr);

        auto *common_partition = resource_manager::get_partition_pointer(RESOURCE_PARTITION_COMMON);
        assert(common_partition != nullptr);

        assert(common_partition->get_pack_slots().size() == 1);

        auto *common_slot = common_partition->get_pack_slots().front();
        assert(common_slot != nullptr);
        assert(common_partition->get_streamer() != nullptr);

        mString v90{};
        resource_key_type v88 {RESOURCE_KEY_TYPE_NONE};
        resource_key::calc_resource_string_and_type_from_path(this->level.descriptor->field_0.to_string(),
                                                              &v90,
                                                              &v88);
        auto *common_streamer = common_partition->get_streamer();
        common_streamer->load(v90.c_str(), 0, nullptr, nullptr);
        common_streamer->flush(RenderLoadMeter);
        resource_manager::get_and_push_resource_context(RESOURCE_PARTITION_COMMON);

        slc_manager::init();

        for (int i = 0; off_921DAC[i][0] != 0; ++i) {
            tlFixedString v55(off_921DAC[i]);
            nglLoadTexture(v55);
        }

        register_chuck_callbacks();
        daynight::init();
        script_manager::init_game_var();

        assert(script_manager::get_total_loaded() == 0);

        resource_key v14 = create_resource_key_from_path("init_gv", RESOURCE_KEY_TYPE_NONE);
        if ( script_manager::is_loadable(v14) )
		{
            resource_key v69{};

            resource_key v73 {string_hash {"init_gv"}, RESOURCE_KEY_TYPE_SCRIPT};
            script_manager::load(v73, 0u, common_slot, v69);

            v73 = resource_key {string_hash {"init_sv"}, RESOURCE_KEY_TYPE_SCRIPT};
            script_manager::load(v73, 0u, common_slot, v69);

            script_manager::link();
            script_manager::run(0.0, false);
            script_manager::clear();
            register_chuck_callbacks();
            this->gamefile->init_script_buffer();
        }

        resource_key v77 = create_resource_key_from_path(this->level.name_mission_table.c_str(),
                                                         RESOURCE_KEY_TYPE_SIN);

        auto *the_sin_res = common_slot->get_resource(v77, nullptr, nullptr);
        assert(the_sin_res != nullptr);

        sin_container *the_sin = nullptr;
        auto allocated_mem = parse_generic_object_mash(the_sin,
                                                       the_sin_res,
                                                       nullptr,
                                                       nullptr,
                                                       nullptr,
                                                       0,
                                                       0,
                                                       nullptr);
        assert(!allocated_mem);

        assert(the_sin != nullptr && "we have no sin!!!");

        the_sin->setup_world();
        if (g_femanager.m_fe_menu_system != nullptr) {
            g_femanager.m_fe_menu_system->RenderLoadMeter(false);
        }

        this->flag.field_3 = false;
        if (g_femanager.m_fe_menu_system != nullptr) {
            g_femanager.m_fe_menu_system->RenderLoadMeter(false);
        }

        auto *v28 = this->the_world->field_A0.field_0;
        v28->init_defaults();

        {
            auto time = this->field_2C0.elapsed();
            sp_log("Pre-load-scene load time: %f seconds", time);
        }

        gravity_generator *v30 = new gravity_generator{};
        g_world_ptr->add_generator(v30);

        if (g_femanager.m_fe_menu_system != nullptr) {
            g_femanager.m_fe_menu_system->RenderLoadMeter(false);
        }

        system_idle();
        debug_render_init();

        geometry_manager::set_zoom(1.0);

        filespec v92{};
        v92.extract(this->level.name_mission_table);

        resource_key v89 {string_hash {v92.m_name.c_str()}, RESOURCE_KEY_TYPE_SCN_ENTITY};

        this->the_world->load_scene(v89,
                                    true,
                                    v92.m_name.c_str(),
                                    nullptr,
                                    (worldly_pack_slot *) common_slot,
                                    nullptr);

        resource_key v93 = create_resource_key_from_path(this->level.name_mission_table.c_str(),
                                                         RESOURCE_KEY_TYPE_MISSION_TABLE);

        mission_manager::s_inst->add_global_table(v93);

        g_world_ptr->field_28.setup_cameras();
        script_manager::link();
        this->the_world->field_140.hook_up_global_script_object();
        mString hero_name {this->gamefile->field_340.m_hero_name.to_string()};

        this->the_world->add_player(hero_name);

        po v86;

        if ( g_game_ptr->m_hero_start_enabled )
		{
            auto *marker_hero_start = find_marker(string_hash{"HERO_START"});

            v86 = marker_hero_start->get_abs_po();

            auto v73 = os_developer_options::instance->get_string(os_developer_options::HERO_START_DISTRICT);
            if (v73)
			{
                auto *ter = g_world_ptr->get_the_terrain();
                assert(ter != nullptr);

                fixedstring<4> v77{v73->c_str()};
                auto v39 = ter->get_region_index_by_name(v77);
                if (v39 != 65535)
				{
                    region *reg = ter->get_region(v39);
                    if ( reg->is_locked() ) {
                        g_world_ptr->the_terrain->unlock_district(reg->get_district_id());
                    }

                    v86.set_position(reg->field_A4);
                }
            }

            auto hero_start_x = os_developer_options::instance->get_int(mString{"HERO_START_X"}); 
            auto hero_start_y = os_developer_options::instance->get_int(mString{"HERO_START_Y"});
            auto hero_start_z = os_developer_options::instance->get_int(mString{"HERO_START_Z"});

            if (hero_start_x != 0 || hero_start_y != 0 || hero_start_z != 0) {
                v86.set_position(
                    vector3d{(float) hero_start_x, (float) hero_start_y, (float) hero_start_z});
            }
        } else {
            v86.set_position(g_game_ptr->level.field_24);
        }

        auto *the_terrain = g_world_ptr->the_terrain;
        if ( auto *reg = the_terrain->find_region(v86.get_position(), nullptr);
                reg != nullptr && (reg->flags & 0x4000) != 0)
        {
            g_world_ptr->the_terrain->unlock_district(reg->district_id);
        }
        else
        {
            for (auto j = 0; j < the_terrain->get_num_regions(); ++j)
            {
                auto *reg = the_terrain->get_region(j);
                if (reg != nullptr && (reg->flags & 0x4000) != 0)
                {
                    g_world_ptr->the_terrain->unlock_district(reg->get_district_id());
                    v86.set_position(reg->field_A4);
                    break;
                }
            }
        }


        if (v86.m[1][0] != YVEC[0] || v86.m[1][1] != YVEC[1] ||
            v86.m[1][2] != YVEC[2])
		{
            po v77;

            v77.m[0] = vector3d {1.0};
            v77.m[1] = YVEC;
            v77.m[2] = ZVEC;
            v77.m[0][3] = 0.0;
            v77.m[1][3] = 0.0;
            v77.m[2][3] = 0.0;
            v77.m[3] = v86.m[3];
            v77.m[3][3] = 1.0;

            v86 = v77;
        }

        auto *hero_ptr = g_world_ptr->get_hero_ptr(0);
        if ( hero_ptr != nullptr )
		{
            *hero_ptr->my_rel_po = v86;

            hero_ptr->dirty_family(false);

            if (hero_ptr->is_conglom_member() || hero_ptr->is_a_conglomerate()) {
                hero_ptr->dirty_model_po_family();
            }

            hero_ptr->po_changed();

            g_spiderman_camera_ptr()->autocorrect(Float{0});

        }
		else
		{
            auto *cam_ptr = g_world_ptr->field_28.field_44;

            if ( cam_ptr != nullptr )
			{
                *cam_ptr->my_rel_po = v86;

                cam_ptr->dirty_family(false);
                if (cam_ptr->is_conglom_member() || cam_ptr->is_a_conglomerate()) {
                    cam_ptr->dirty_model_po_family();
                }

                cam_ptr->po_changed();
            }
        }

        this->the_world->the_terrain->start_streaming(game::load_complete);
        g_world_ptr->the_terrain->force_streamer_refresh();
        USOcean2Shader::Init();

        {
            auto time = this->field_2C0.elapsed();
            sp_log("Post-load-scene load time: %f seconds\n", time);
        }

        if (g_femanager.m_fe_menu_system != nullptr) {
            g_femanager.m_fe_menu_system->RenderLoadMeter(false);
        }

        this->field_64 = find_mic(string_hash{"BOOM_MIC"});

        this->current_game_camera = this->the_world->get_chase_cam_ptr(0);

        this->set_current_camera(this->the_world->get_chase_cam_ptr(0), true);

        if (g_femanager.m_fe_menu_system != nullptr) {
            g_femanager.m_fe_menu_system->RenderLoadMeter(false);
        }

        script_manager::run(0.0, true);
        if (g_femanager.m_fe_menu_system != nullptr) {
            g_femanager.m_fe_menu_system->RenderLoadMeter(false);
        }

        if (this->field_16E) {
            this->freeze_hero(true);
        }

        spider_monkey::on_level_load();
        this->level.field_34.reset();

        mem_print_stats("at very end of load_this_level()");

    } else {
        THISCALL(0x0055C6E0, this);
    }
}

void game::level_load_stuff::reset_level_load_data()
{
    this->descriptor = nullptr;
    this->field_30 = 0;
    this->load_completed = false;
    this->field_3A = false;
    this->load_widgets_created = false;
}

void game::level_load_stuff::look_up_level_descriptor()
{
    TRACE("game::level_load_stuff::look_up_level_descriptor");

    if constexpr (1)
    {
        this->descriptor = nullptr;
        auto *game_partition = resource_manager::get_partition_pointer(RESOURCE_PARTITION_START);
        assert(game_partition != nullptr);

        assert(game_partition->get_pack_slots().size() == 1);

        auto &v2 = game_partition->get_pack_slots();
        auto *game_slot = v2.front();
        assert(game_slot != nullptr);

        resource_key a1 {string_hash {"level"}, RESOURCE_KEY_TYPE_DESCRIPTOR};

        filespec v28 {this->name_mission_table};
        v28.m_name.to_upper();
        fixedstring<8> v27 {v28.m_name.c_str()};

        int lookup_size = 0;
        auto *lvl_descriptors = (level_descriptor_t *) game_slot->get_resource(a1, &lookup_size, nullptr);
        if ( lvl_descriptors == nullptr )
        {
            sp_log("Game common pack file missing. Please run the packer.");
            assert(0);
        }

        constexpr auto sizeof_level_descriptor = sizeof(level_descriptor_t);
        int v23 = lookup_size / sizeof_level_descriptor;
        assert((lookup_size % sizeof_level_descriptor) == 0);

        if constexpr (1)
        {
            printf("lookup_size = %d, num_levels_desc = %d\n", lookup_size, v23);
            for (auto i = 0; i < v23; ++i)
            {
                printf("\t%s\n", lvl_descriptors[i].field_0.to_string());
            }
        }

        {
            auto *begin = lvl_descriptors;
            auto *end = begin + v23;
            auto *it_find = std::find_if(begin, end, [&v27](const auto &desc) {
                return (v27 == desc.field_0);
            });

            if ( it_find == end )
            {
                auto *v5 = v27.to_string();
                error("Couldn't find level descriptor for %s", v5);
            }

            auto *v6 = v27.to_string();
            debug_print_va("Found level descriptor for %s", v6);
            this->descriptor = it_find;
        }

        resource_key v16 {string_hash {this->descriptor->field_0.to_string()}, RESOURCE_KEY_TYPE_PACK};
        auto v19 = resource_manager::get_pack_file_stats(v16, nullptr, nullptr, nullptr);
        if ( !v19 )
        {
            auto *v9 = this->descriptor->field_0.to_string();
            sp_log("Couldn't find packfile for level '%s'. Make sure you packed this level.", v9);
            assert(0);
        }

        resource_manager::configure_packs_by_memory_map(this->descriptor->m_index_memory_map);
    }
    else
    {
        THISCALL(0x0055A1C0, this);
    }
}

void game::level_load_stuff::construct_loading_widgets()
{
    mString a1{"spidermanlogo"};

    mission_manager::s_inst->lock();
    mission_stack_manager::s_inst->push_mission_pack_immediate(a1, a1);

    assert(!load_widgets_created);
    load_widgets_created = true;

    if (!os_developer_options::instance->get_flag(mString{"NO_LOAD_SCREEN"})) {
        g_femanager.LoadFrontEnd();
    }
}

void game::level_load_stuff::destroy_loading_widgets()
{
    if constexpr (1)
    {
        this->load_widgets_created = false;
        g_femanager.ReleaseFrontEnd();
        mString a1{"spidermanlogo"};

        mission_stack_manager::s_inst->pop_mission_pack_immediate(a1, a1);
        mission_manager::s_inst->unlock();
    }
    else
    {
        THISCALL(0x0050B560, this);
    }
}

bool game::level_load_stuff::wait_for_mem_check()
{
    bool result = false;

    if (g_femanager.m_fe_menu_system != nullptr) {
        result = g_femanager.m_fe_menu_system->WaitForMemCheck();
    }

    return result;
}

void sub_405CC0() {
    CDECL_CALL(0x00405CC0);
}

void system_idle() {
    MSG Msg;

    while (PeekMessageA(&Msg, nullptr, 0, 0, 1u)) {
        if (Msg.message == WM_QUIT) {
            break;
        }

        if (Msg.message == WM_CLOSE) {
            break;
        }

        if (Msg.message == WM_DESTROY) {
            break;
        }

        TranslateMessage(&Msg);
        DispatchMessageA(&Msg);
    }
}

camera *game::get_current_view_camera(int a2)
{
    if constexpr (1)
    {
        auto *cam = comic_panels::get_current_view_camera(a2);
        if (cam == nullptr)
        {
            auto v4 = this->the_world;
            cam = this->field_5C;
            if (cam == (camera *) v4->get_chase_cam_ptr(0)) {
                cam = (camera *) v4->get_chase_cam_ptr(a2);
            }
        }

        return cam;
    } else {
        return (camera *) THISCALL(0x00514A50, this, a2);
    }
}

void game::_load_new_level(const mString &a2)
{
    if (!a2.empty()) {
        strcpy(g_scene_name, a2.c_str());
    }

    this->field_15D = true;
    this->field_15C = false;
    this->field_167 = true;
}

void game::load_new_level(const mString &a2, const vector3d &a3)
{
    if constexpr (1)
    {
        this->_load_new_level(a2);
        this->level.field_24 = a3;
        this->m_hero_start_enabled = false;

    } else {
        THISCALL(0x00514C40, this, &a2, &a3);
    }
}

void game::load_new_level(const mString &a1, int a2)
{
    if constexpr (1)
    {
        this->_load_new_level(a1);
        this->m_hero_start_enabled = true;

    }
    else
    {
        THISCALL(0x00514C70, this, &a1, a2);
    }
}

void game::advance_state_load_level(Float a2)
{
    TRACE("game::advance_state_load_level");

    if constexpr (0)
    {
        static bool & loading_a_level = var<bool>(0x00960CB5);

        this->level.name_mission_table = g_scene_name;
        input_mgr::instance->field_26 = false;
        if (!loading_a_level)
        {
            this->level.reset_level_load_data();
            loading_a_level = true;
            this->level.look_up_level_descriptor();
            if (!g_is_the_packer()) {
                sound_manager::load_common_sound_bank(true);
            }

            mission_stack_manager::s_inst->start_streaming();

            this->level.construct_loading_widgets();

            this->load_this_level();
        }

        this->the_world->the_terrain->frame_advance(a2);
        if (this->level.load_completed && !this->level.wait_for_mem_check())
        {
            this->level.destroy_loading_widgets();
            sub_405CC0();

            int TOD = os_developer_options::instance->get_int(mString {"TIME_OF_DAY"});
            if (TOD == -1) {
                TOD= g_TOD();
            }

            us_lighting_switch_time_of_day(TOD);
            app::instance->field_38 += 2;
            this->flag.level_is_loaded = true;
            this->field_167 = false;
            loading_a_level = false;
            this->go_next_state();
        }

        cut_scene_player *v13 = g_cut_scene_player();
        if ( v13->is_playing() )
        {
            v13->frame_advance(a2);
            this->the_world->update_ai_and_visibility_proximity_maps_for_moved_entities(a2);

            this->the_world->update_collision_proximity_maps_for_moved_entities(a2);

            this->the_world->update_light_proximity_maps_for_moved_entities(a2);
            this->the_world->the_terrain->frame_advance(a2);
            light_manager::frame_advance_all_light_managers(a2);
        }

        if (g_femanager.m_fe_menu_system != nullptr)
        {
            void (__fastcall *Update)(void *, void *, Float) = CAST(Update, get_vfunc(g_femanager.m_fe_menu_system->m_vtbl, 0x14));

            Update(g_femanager.m_fe_menu_system, nullptr, a2);

            g_femanager.m_fe_menu_system->RenderLoadMeter(false);
        }
    }
    else
    {
        THISCALL(0x0055D3A0, this, a2);
    }
}

void game::frame_advance_game_overlays(Float a1)
{
    g_femanager.Update(a1);

    g_console->frame_advance(a1);
}



void game::message_board_clear()
{
    if ( this->mb != nullptr ) {
        this->mb->~message_board();
        this->mb = nullptr;
    }
}

void game::render_motion_blur() {
    THISCALL(0x00521610, this);
}

void sub_5BC870()
{
    CDECL_CALL(0x005BC870);

    g_game_ptr->mb->render();
}

void game::render_interface()
{
    TRACE("game::render_interface");

    if constexpr (1)
    {
        if (this->flag.level_is_loaded)
        {
            sub_5BC870();

            spider_monkey::render();
            this->mb->render();

            if (os_developer_options::instance->get_flag(mString{"SHOW_DEBUG_INFO"}))
            {
                this->show_debug_info();
            }

            if (os_developer_options::instance->get_flag(mString{"SHOW_DISTRICTS"}))
            {
                this->render_district_labels();
            }

            if (os_developer_options::instance->get_flag(mString{"SHOW_WATERMARK_VELOCITY"}))
            { 
                this->show_max_velocity();
            }
        }
    }
    else
    {
        THISCALL(0x00524290, this);
    }
}

mString to_string(const vector3d &a2)
{
    mString a1 {0, "<%.3f,%.3f,%.3f>", a2[0], a2[1], a2[2]};
    return a1;
}

mString get_region_name(region* reg) {
    return reg ? reg->get_name().to_string() : mString{"none"};
}

// Helper function to get entity info
mString get_entity_info(entity* ent, const char* prefix) {
    auto region_name = get_region_name(ent->get_primary_region());
    auto position = to_string(ent->get_abs_position());
    auto facing = to_string(ent->get_abs_po().get_z_facing());

    mString info{0, "%s @ %s, f = %s", prefix, position.c_str(), facing.c_str()};
    info += " " + region_name;
    return info;
}

// Refactored get_camera_info function
mString game::get_camera_info() const {
    auto* camera = this->field_5C;
    auto region_name = get_region_name(camera->get_primary_region());
    auto position = to_string(camera->get_abs_position());
    auto facing = to_string(camera->get_abs_po().get_z_facing());

    // Find region based on absolute position
    auto* terrain = g_world_ptr->get_the_terrain();
    auto* region = terrain->find_region(camera->get_abs_position(), nullptr);
    if (region) {
        region_name = region->get_name().to_string();
    }

    mString info{0, "CAMERA @ %s %s, f = %s", region_name.c_str(), position.c_str(), facing.c_str()};
    return info;
}


mString game::get_analyzer_info() const {
    string_hash hash{to_hash("SCENE_ANALYZER_CAM")};
    auto* analyzer = entity_handle_manager::find_entity(hash, entity_flavor_t::CAMERA, false);
    auto region = get_region_name(g_world_ptr->get_the_terrain()->find_region(analyzer->get_abs_position(), nullptr));
    auto position = to_string(analyzer->get_abs_position());
    auto facing = to_string(analyzer->get_abs_po().get_z_facing());

    mString info{0, "ANALYZER @ %s, f = %s", position.c_str(), facing.c_str()};
    if (!region.empty()) {
        mString info{0, "ANALYZER @ %s, f = %s%s%s", 
             position.c_str(), 
             facing.c_str(), 
             region.empty() ? "" : " ", 
             region.empty() ? "" : region.c_str()};
    }
    return info;
}
mString game::get_hero_info() const {
    auto* hero = g_world_ptr->get_hero_ptr(0);
    if (!hero) {
        return mString{"(hero does not exist!)"};
    }

    auto region = get_region_name(hero->get_primary_region());
    auto position = to_string(hero->get_abs_position());
    auto velocity = to_string(hero->physical_ifc()->get_velocity());

    mString info{0, "HERO @ %s, %s, v = %s", region.c_str(), position.c_str(), velocity.c_str()};
    return info;
}

void game::show_debug_info() const {
    TRACE("game::show_debug_info");

    auto fontSize = os_developer_options::instance->get_int(mString{"DEBUG_INFO_FONT_PCT"}) / 100.0f;
    vector2di position{50, 40};

    auto debugInfos = {
        get_hero_info(),
        get_camera_info(),
        get_analyzer_info()
    };

    for (const auto& info : debugInfos) {
        nglListAddString(nglSysFont(), position.x, position.y, 1.0, fontSize, fontSize, info.c_str());
        position.y += 20;
    }
}

// Replacement for the original game's sub_66C242 (the "Show Districts"
// renderer). Draws the Xbox debug-build HUD overlay: a white
// "DISTRICTS" header with the hero's current district id rendered in
// green directly underneath, anchored near the bottom-centre of the
// screen. The flag is toggled by the "Show Districts" entry in the
// Game flags menu.
//
// Reference: bandicam_2026-04-29_23-12-55-697.mp4 frames at t=5s,
// t=60s, t=80s, t=100s show the layout — header line on top, single
// id glyph (e.g. "0") in green just below, fixed screen-space and
// independent of camera/hero motion. The Xbox shows "DISTRICT"; the
// user's spec ("DISTRICTS 0") asks for the plural form, so that's
// what we render here.
void game::render_district_labels() {
    TRACE("game::render_district_labels");

    if ( g_world_ptr == nullptr ) {
        return;
    }

    // render_text gates on SHOW_DEBUG_TEXT internally, so make sure
    // the flag is on whenever we get this far. The Show Districts
    // toggle in debug_menu_extra also forces it on, but flags can
    // drift across saved-game loads.
    auto *opts = os_developer_options::instance;
    if ( opts != nullptr && !opts->get_flag(mString{"SHOW_DEBUG_TEXT"}) ) {
        opts->set_flag(mString{"SHOW_DEBUG_TEXT"}, true);
    }

    // Resolve the hero's current district id. If we can't find it for
    // any reason (hero not spawned yet, hero outside of any region)
    // fall back to 0 so the user still sees the HUD and knows the
    // toggle is taking effect.
    int district_id = 0;
    if ( auto *hero = g_world_ptr->get_hero_ptr(0) )
    {
        if ( auto *reg = hero->get_primary_region() )
        {
            district_id = reg->get_district_id();
        }
        else if ( auto *terrain = g_world_ptr->get_the_terrain() )
        {
            // Hero's primary region cache can be stale just after
            // a warp; do a fresh point query as a backstop.
            if ( auto *reg2 = terrain->find_region(
                     hero->get_abs_position(), nullptr) )
            {
                district_id = reg2->get_district_id();
            }
        }
    }

    // Anchor at ~42% width / ~77% height — this matches the Xbox
    // build's placement (verified visually against the reference
    // video). Works for any backbuffer resolution because we scale
    // off nglGetScreenWidth/Height.
    const int sw = nglGetScreenWidth();
    const int sh = nglGetScreenHeight();
    const int header_x = static_cast<int>(sw * 0.42f);
    const int header_y = static_cast<int>(sh * 0.77f);
    const int id_x     = static_cast<int>(sw * 0.37f);
    const int id_y     = header_y + 24;  // one line below the header

    const color32 white {255, 255, 255, 255};
    const color32 green { 32, 224,  64, 255};

    mString header {"DISTRICTS"};
    mString id_text {0, "%d", district_id};

    render_text(header,  vector2di{header_x, header_y}, white, 0.0f, 1.0f);
    render_text(id_text, vector2di{id_x,     id_y    }, green, 0.0f, 1.0f);
}


void game::show_max_velocity()
{

}

float game::get_script_game_clock_timer() const
{
    if constexpr (1)
    {
        static mString s_game_clock_timer_str{"game_clock_timer"};

        return *static_cast<float *>(
            script_manager::get_game_var_address(s_game_clock_timer_str, nullptr, nullptr));
    }
    else
    {
        float (__fastcall *func)(const game *) = CAST(func, 0x005244E0);
        return func(this);
    }
}

//TODO
void game::render_ui()
{
    if constexpr (0)
    {
        nglListBeginScene(static_cast<nglSceneParamType>(1));
        render_motion_blur();
        nglListEndScene();

        nglListBeginScene(static_cast<nglSceneParamType>(1));

        static bool & g_preserve_z_buffer = var<bool>(0x0095C878);
        nglSetClearFlags(g_preserve_z_buffer ? 0 : 6);
        nglListEndScene();

        static bool & g_disable_interface = var<bool>(0x0095C879);

        if ( g_disable_interface || !this->flag.level_is_loaded ||
            os_developer_options::instance->get_flag(mString {"INTERFACE_DISABLE"}) )
        {
            if ( this->flag.level_is_loaded )
            {
                if (!EnableShader())
                {
                    matrix4x4 a1;
                    a1.arr[0][0] = 0.003125;
                    a1.arr[0][1] = 0.0;
                    a1.arr[0][2] = 0.0;
                    a1.arr[0][3] = 0.0;
                    a1.arr[1][0] = 0.0;
                    a1.arr[1][1] = 0.004166666;
                    a1.arr[1][2] = 0.0;
                    a1.arr[1][3] = 0.0;
                    a1.arr[2][0] = 0.0;
                    a1.arr[2][1] = 0.0;
                    a1.arr[2][2] = -1.0;
                    a1.arr[2][3] = 0.0;
                    a1.arr[3][0] = -1.0;
                    a1.arr[3][1] = -1.0;
                    a1.arr[3][2] = 0.0;
                    a1.arr[3][3] = 1.0;
                    nglSetWorldToViewMatrix({a1});
                    nglSetAspectRatio(1.0);
                    nglSetOrthoMatrix(1000.0, 10000.0);
                    nglCalculateMatrices(false);
                }

                mission_manager::s_inst->render_fade();
            }
            else
            {
                g_femanager.RenderLoadMeter(true);
            }
        }
        else
        {
            g_femanager.Draw();
        }

        nglListBeginScene(static_cast<nglSceneParamType>(1));
        nglSetClearFlags(g_preserve_z_buffer ? 0 : 6);
        sub_769DE0(7);
        if (!EnableShader())
        {
            matrix4x4 v5;
            v5.arr[0][0] = 0.003125;
            v5.arr[0][1] = 0.0;
            v5.arr[0][2] = 0.0;
            v5.arr[0][3] = 0.0;
            v5.arr[1][0] = 0.0;
            v5.arr[1][1] = 0.004166666;
            v5.arr[1][2] = 0.0;
            v5.arr[1][3] = 0.0;
            v5.arr[2][0] = 0.0;
            v5.arr[2][1] = 0.0;
            v5.arr[2][2] = -1.0;
            v5.arr[2][3] = 0.0;
            v5.arr[3][0] = -1.0;
            v5.arr[3][1] = -1.0;
            v5.arr[3][2] = 0.0;
            v5.arr[3][3] = 1.0;
            nglSetWorldToViewMatrix({v5});
            nglSetAspectRatio(1.0);
            nglSetOrthoMatrix(1000.0, 10000.0);
            nglCalculateMatrices(0);
        }

        if (!g_disable_interface) {
            render_interface();
        }

        if (os_developer_options::instance->get_flag(static_cast<os_developer_options::flags_t>(107)))
        {
            auto v2 = g_game_ptr->get_script_game_clock_timer();

            mString v8{0, "%d:%.02d", (int) v2 / 3600, (int) v2 / 60 % 60};
            auto *v3 = v8.c_str();

            mString v6{v3};

            render_text(v8, vector2di{520, 30}, color32{255, 255, 255, 255}, 1.0, 0.80000001);
        }

        if constexpr (1) {
            g_console->render();

#if defined(ENABLE_DEBUG_MENU) && DEBUG_MENU_REIMPL != 1
           debug_menu::render_active();
#endif
        }

        nglListEndScene();
        if (!spider_monkey::is_running())
        {
            if (os_developer_options::instance->get_int(static_cast<os_developer_options::ints_t>(26)) == 1 && byte_965BF5())
            {
                SYSTEMTIME SystemTime;
                GetLocalTime(&SystemTime);

                char Dest[256];
                sprintf(Dest,
                        "%s\\screenshot_%4d-%02d-%02d-%02d%02d%02d",
                        byte_9659B8(),
                        SystemTime.wYear,
                        SystemTime.wMonth,
                        SystemTime.wDay,
                        SystemTime.wHour,
                        SystemTime.wMinute,
                        SystemTime.wSecond);
                nglScreenShot(Dest);
                byte_965BF5() = false;
            }

            auto ALLOW_SCREENSHOT = os_developer_options::instance->get_int(mString {"ALLOW_SCREENSHOT"});
            if ( ALLOW_SCREENSHOT == 2
                    && this->field_80.is_triggered() )
            {
                static bool & capturing = var<bool>(0x00960B47);

                if (capturing) {
                    app::instance->field_4.end_screen_recording();
                } else {
                    mString v7 {"L3ScreenShot"};

                    app::instance->field_4.begin_screen_recording(v7, 30);
                }

                capturing = !capturing;
            }
        }
    }
    else
    {
        THISCALL(0x0052B250, this);
    }
}


inline double sub_A26B70()
{
    if (g_game_ptr->field_28C == 0.0) {
        return 0.0;
    } else {
        return 1.0 / g_game_ptr->field_28C;
    }
}

inline double sub_690BF1()
{
    return sub_A26B70();
}




void game::render_bar_of_shame()
{
    if (os_developer_options::instance->get_flag(mString { "SHOW_BAR_OF_SHAME" })) {
        auto v39 = g_game_ptr->field_284 * 1000.0;

        int dword_1568744 = 0;
        if (v39 > dword_1568744) {
            dword_1568744 = v39;
        }

        color v38 { 0 };
        if (sub_690BF1() >= 25.0) {
            if (sub_690BF1() >= 30.0) {
                color v5 { 0.0, 1.0, 0.0, 1.0 };
                v38 = v5;
            } else {
                color v4 { 1.0, 1.0, 0.0, 1.0 };
                v38 = v4;
            }
        } else {
            color v3 { 0.0, 1.0, 0.0, 1.0 };
            v38 = v3;
        }

        nglQuad v1;
        nglInitQuad(&v1);
        nglSetQuadColor(&v1, (int)(v38.b * 255.0) | ((int)(v38.g * 255.0) << 8) | ((int)(v38.r * 255.0) << 16) | ((int)(v38.a * 255.0) << 24));
        nglSetQuadBlend(&v1, (nglBlendModeType)2, 0);
        nglSetQuadRect(&v1, 32.0, 16.0, (v39 * 3.0) + 32.0, 16.0 + 16.0);
        nglSetQuadZ(&v1, 1.0);
        nglListAddQuad(&v1);
    }
}

void nglListEndScene_hook()
{
    {
        if (os_developer_options::instance->get_flag(mString{"SHOW_DEBUG_INFO"}))
        {
            g_game_ptr->show_debug_info();
        }
    }

    g_console->render();

    nglCurScene() = nglCurScene()->field_30C;
}

void game::advance_state_running(Float a2)
{
    TRACE("game::advance_state_running");

    if constexpr (1)
    {
        if (!this->field_162
                && !this->field_161
                && os_developer_options::instance->get_int(mString{"FORCE_WIN"}) == 0)
        {
            if (this->field_164) {
                this->unload_current_level();
                this->soft_reset_process();
            } else if (this->field_167) {
                this->field_167 = false;
                this->unload_current_level();
                this->soft_reset_process();
            } else if (this->field_16D) {
                this->field_16D = false;
                this->unload_current_level();
                this->soft_reset_process();
                this->render_intros();
            } else {
                g_cut_scene_player()->frame_advance(a2);
                this->the_world->update_light_proximity_maps_for_moved_entities(a2);
                light_manager::frame_advance_all_light_managers(a2);
                if (!this->flag.physics_enabled || this->flag.single_step)
                {
                    this->the_world->frame_advance(a2);
                    this->mb->frame_advance(a2);
                    this->frame_advance_game_overlays(a2);
                    this->flag.single_step = false;
                } else {
                    this->the_world->update_ai_and_visibility_proximity_maps_for_moved_entities(a2);
                    this->the_world->update_collision_proximity_maps_for_moved_entities(a2);
                    this->the_world->the_terrain->frame_advance(a2);

                    if (g_cut_scene_player()->is_playing()) {
                        script_manager::run(a2, false);
                        aeps::FrameAdvance(a2);
                    }

                    this->the_world->field_28.usercam_frame_advance(a2);
                    this->the_world->field_28.scene_analyzer_frame_advance(a2);
                }

                static bool console_exec_completed = false;

                if (!console_exec_completed) {
                    console_exec_completed = true;

                    [[maybe_unused]] auto a3 = os_developer_options::instance
                        ->get_string(os_developer_options::strings_t::CONSOLE_EXEC);
                }

                this->field_64->frame_advance(Float{a2});
                mString v7{"g_game_paused"};

                auto *v5 = (int *) script_manager::get_game_var_address(v7, nullptr, nullptr);
                *v5 = 0;
            }
        }
    } else {

				       void (__fastcall *func)(void *, void *, Float ) = bit_cast<decltype(func)>(0x00559FA0);

         func(this, nullptr, a2);
		
    }
}

void game::load_hero_packfile(const char *str, bool a3)
{
    TRACE("load_hero_packfile", str);

    if constexpr (1)
    {
        game_settings *v8 = this->gamefile;
        v8->field_340.m_hero_name = fixedstring<8> {str};

        //sp_log("%d", resource_manager::partitions()->size());

        resource_partition *partition = resource_manager::get_partition_pointer(RESOURCE_PARTITION_HERO);
        assert(partition != nullptr);

        resource_pack_streamer *streamer = partition->get_streamer();
        assert(streamer != nullptr);

        if (a3) {
            this->unload_hero_packfile();
        }

        {
            //streamer->dump();

            //sp_log("\npreload");
            streamer->load(str, 0, nullptr, nullptr);
            //sp_log("postload\n");

            //streamer->dump();
        }

        streamer->flush(RenderLoadMeter);

        sound_manager::load_hero_sound_bank(str, true);

    }
    else
    {

		       void (__fastcall *func)(void *, void *, const char *, int ) = bit_cast<decltype(func)>(0x0055A420);

         func(this, nullptr, str, a3);
    }
}

void game::render_empty_list()
{
    if constexpr (0)
    {}
    else
    {

					  		         CDECL_CALL(0x00510780);

    }
}

void game::frame_advance(Float a2)
{
    TRACE("game::frame_advance");
 
    sp_log("%f", float(a2));
 
#if FPS_UNLOCK_60
    // ---------------------------------------------------------------
    // Sim-speed compensation for the 60 FPS patch.
    //
    // When the engine is ticking at TargetFPS (e.g. 60) instead of the
    // stock 30, frame-counter-based logic (animation frame counts,
    // "field_140 = 3.0" style cooldowns, hard-coded ticks like
    // frame_advance(0.06666667)) would otherwise advance at TargetFPS/30
    // times its intended wall-clock speed. Scaling time_inc by 30/Target
    // counter-acts that so those systems keep their original feel.
    //
    // dt-based subsystems (physics, audio fades, game_clock) will then
    // run at (30/Target) wall-clock speed when this is enabled. Set
    // g_config.CompensateSimDt = false to leave dt untouched if you'd
    // rather have those at full speed and live with frame-counter logic
    // running fast.
    // ---------------------------------------------------------------
    if (g_config.EnableHighFPS &&
        g_config.CompensateSimDt &&
        g_config.TargetFPS > 30)
    {
        const float scale = 30.0f / static_cast<float>(g_config.TargetFPS);
        a2 = Float(float(a2) * scale);
    }
#endif
 
    if constexpr (0)
    {}
    else
    {
        void (__fastcall *func)(void *, void *, Float ) = bit_cast<decltype(func)>(0x0055D780);
 
         func(this, nullptr, a2);
    }
}

void game::frame_advance_level(Float time_inc)
{
    TRACE("game::frame_advance_level");

    if constexpr (0)
    {
        static bool & gimme_the_lowdown = var<bool>(0x0095C8EE);

        auto *v2 = input_mgr::instance;
        if (gimme_the_lowdown) {
            gimme_the_lowdown = false;
        }

        this->sub_524170();

        if (this->flag.field_3 && AXIS_MAX != v2->get_control_state(54, INVALID_DEVICE_ID)) {
            this->flag.field_3 = false;
        }

        this->sub_559F50(&time_inc);

        this->sub_5241D0(time_inc);

        if (time_inc != 0.0f) {
            game_clock::frame_advance(time_inc);
        }

        this->sub_5580F0();

        assert(time_inc >= 0 && time_inc < 10.0f);

        this->handle_game_states(time_inc);
        this->handle_cameras(v2, time_inc);
        mission_manager::s_inst->frame_advance(time_inc);
        spider_monkey::frame_advance(time_inc);
        this->gamefile->frame_advance(time_inc);
    }
    else
    {

								       void (__fastcall *func)(void *, void *, Float ) = bit_cast<decltype(func)>(0x0055D650);

         func(this, nullptr, time_inc);
    }
}

void sub_65F200()
{
    for (auto i = 0; i < 128; ++i)
    {
        auto *v1 = &s_script_sound_instance_slots()[i];
        if (s_script_sound_instance_slots()[i].field_3C)
        {
            auto *v2 = v1->field_0.get_sound_instance_ptr();

            if (v2 != nullptr) {
                v2->stop();
            }
        }
    }
}

void sub_579290() {
    ;
}

static instance_bank<cg_mesh> & cg_mesh_bank = var<instance_bank<cg_mesh>>(0x00960494);

void game::unload_current_level()
{
    TRACE("game::unload_current_level");

    if constexpr (1)
    {
        mem_print_stats("unload_current_level() start");

        this->field_170 = true;
        if (g_femanager.IGO != nullptr) {
            g_femanager.IGO->field_0->SetShown(false);
        }

        mission_manager::s_inst->unload_script_now();
        auto *v2 = this->the_world->the_terrain;
        if (v2 != nullptr) {
            v2->sub_557130();
        }

        daynight::lights() = nullptr;
        daynight::gradients() = nullptr;
        daynight::initialized() = false;
        if (*off_921DAC[0]) {
            char **v3 = off_921DAC;
            char **v4 = off_921DAC;

            char v6;
            do {
                tlFixedString a1{*v3};
                auto *v5 = nglGetTexture(a1);
                if (v5 != nullptr) {
                    nglReleaseTexture(v5);
                }

                v6 = *v4[1];
                v3 = ++v4;
            } while (v6);
        }

        this->the_world->ent_mgr.destroy_all_entities_and_items();

        auto *my_partition = resource_manager::get_partition_pointer(RESOURCE_PARTITION_COMMON);

        assert(my_partition != nullptr);
        assert(my_partition->get_streamer() != nullptr);

        auto *v7 = my_partition->get_streamer();
        v7->flush(game::render_empty_list, 0.02f);
        v7->unload_all();
        v7->flush(game::render_empty_list, 0.02);
        this->flag.level_is_loaded = false;
        sub_65F200();
        if (!g_is_the_packer()) {
            sub_79A160();

            for (int i = 0; i < 128; ++i) {
                s_sound_instance_slots()[i].field_50 = 0;
            }
        }

        sound_manager::frame_advance(0.1);

        assert(the_world != nullptr);

        spider_monkey::on_level_unload();
        auto *v10 = input_mgr::instance->rumble_ptr;
        if (v10 != nullptr) {
            v10->stop_vibration();
            v10->field_60 = true;
        }

        if (!bExit()) {
            this->freeze_hero(false);
        }

        trigger_manager::instance->purge();

        event_manager::clear();

        if (this->the_world != nullptr) {
            this->the_world->~world_dynamics_system();
            operator delete(this->the_world);
        }

        this->the_world = nullptr;
        g_world_ptr = nullptr;

        script_manager::clear();
        script_manager::destroy_game_var();
        entity::destroy_static_entity_pointers();
        swing_anchor_finder::remove_all_anchors();
        this->field_58 = 0;
        slc_manager::kill();
        cg_mesh_bank.purge();
        this->flag.level_is_loaded = 0;
        input_mgr::instance->field_24 = false;

        mem_print_stats("unloading level");

        auto *mem = operator new(0x400u);

        auto *v16 = new (mem) world_dynamics_system{};
        this->the_world = v16;

        g_world_ptr = v16;
        debug_render_items()[34] = 1;
        sub_579290();
        this->field_170 = false;

        mem_print_stats("unload level end");

    } else {
        THISCALL(0x00557E10, this);
    }
}

void game::enable_physics(bool a2) {
    this->flag.physics_enabled = !a2;
}

void terrain::unload_all_districts_immediate()
{
    auto *district_partition = resource_manager::get_partition_pointer(RESOURCE_PARTITION_DISTRICT);
    assert(district_partition != nullptr);

    auto *district_streamer = district_partition->get_streamer();
    assert(district_streamer != nullptr);
    
    auto *strip_partition = resource_manager::get_partition_pointer(RESOURCE_PARTITION_STRIP);
    assert(strip_partition != nullptr);

    auto *strip_streamer = strip_partition->get_streamer();
    assert(strip_streamer != nullptr);

    do
    {
        do
        {
            district_streamer->flush(game::render_empty_list);
            strip_streamer->flush(game::render_empty_list);
        }
        while ( !district_streamer->is_idle() );
    }
    while ( !strip_streamer->is_idle() );

    auto *district_slots = district_streamer->get_pack_slots();
    assert(district_slots != nullptr);

    for ( auto i = 0u; i < district_slots->size(); ++i )
    {
        district_streamer->unload(i);
        district_streamer->flush(game::render_empty_list);
        this->force_streamer_refresh();
    }
}

void game::unload_hero_packfile()
{
    resource_partition *partition = resource_manager::get_partition_pointer(RESOURCE_PARTITION_HERO);
    assert(partition != nullptr);
    assert(partition->get_streamer());

    resource_pack_streamer *my_streamer = partition->get_streamer();
    my_streamer->flush(game::render_empty_list);
    my_streamer->unload_all();
    my_streamer->flush(game::render_empty_list);

    sound_manager::unload_hero_sound_bank();
}

bool game::is_button_pressed(int a4) const
{
    auto id = input_mgr::instance->field_58;
    if ( id == INVALID_DEVICE_ID ) {
        return false;
    }

    bool result = false;
    auto *device = input_mgr::instance->get_device_from_map(id);

    if ( device != nullptr )
    {
        if ( 1.0f != device->get_axis_state(22, 0) )
        {
            auto v7 = device->get_axis_id(a4);
            if ( 1.0f == device->get_axis_delta(v7, 0) ) {
                return true;
            }
        }
    }

    return result;
}

void game::sub_5580F0()
{
    if ( this->field_163 ) {
        this->unload_current_level();
    }
}

void game::sub_5241D0(Float a1)
{
    input_mgr::instance->frame_advance(a1);
}

void game::sub_524170()
{
    static Var<int> achy_breaky_int{0x0095C734};

    if ( g_world_ptr->field_158.field_C == 13111 ) {
        ++achy_breaky_int();
    }

    script_pad()->update();
    input_mgr::instance->scan_devices();
}

void game::sub_559F50(Float *a1)
{
    script_sound_manager::frame_advance(*a1);

    if ( !os_developer_options::instance->get_flag(mString {"DISABLE_AUDIO_BOXES"}) ) {
        audio_box_manager::frame_advance(*a1);
    }

    ambient_audio_manager::frame_advance(*a1);
    sound_manager::frame_advance(*a1);
    gab_manager::frame_advance(*a1);
}

void map_spiderman_controls()
{
    auto *v1 = input_mgr::instance;
    auto id = v1->field_58;
    v1->map_control(96, id, 10);
    v1->map_control(98, id, 11);
    v1->map_control(97, id, 13);
    v1->map_control(99, id, 14);
    v1->map_control(100, id, 17);
    v1->map_control(101, id, 19);
    v1->map_control(102, id, 9);
    v1->map_control(103, id, 16);
    v1->map_control(104, id, 18);
    v1->map_control(105, id, 6);
    v1->map_control(106, id, 5);
    v1->map_control(107, id, 4);
    v1->map_control(108, id, 8);
    v1->map_control(109, id, 7);
    v1->map_control(110, id, 1);
    v1->map_control(111, id, 0);
    v1->map_control(112, id, 3);
    v1->map_control(113, id, 2);
    v1->map_control(112, id, 1);
    v1->map_control(113, id, 0);
    v1->map_control(114, id, 20);
    v1->map_control(115, id, 21);
}

void map_pc_2_playstation_inputs()
{
    auto *v0 = input_mgr::instance;
    auto id = v0->field_58;
    v0->map_control(80, id, 10);
    v0->map_control(81, id, 11);
    v0->map_control(82, id, 14);
    v0->map_control(83, id, 13);
    v0->map_control(84, id, 16);
    v0->map_control(85, id, 18);
    v0->map_control(86, id, 6);
    v0->map_control(87, id, 17);
    v0->map_control(88, id, 19);
    v0->map_control(89, id, 9);
    v0->map_control(90, id, 3);
    v0->map_control(91, id, 2);
    v0->map_control(90, id, 1);
    v0->map_control(91, id, 0);
    v0->map_control(94, id, 8);
    v0->map_control(95, id, 7);
    v0->map_control(92, id, 20);
    v0->map_control(93, id, 21);
}

void game__setup_inputs(game *a1)
{
    TRACE("game::setup_inputs");

    if constexpr (0)
    {
        auto *v2 = input_mgr::instance;
        v2->clear_mapping();
        auto id = v2->field_58;
        
        v2->map_control(1, id, 10);
        v2->map_control(2, id, 11);
        v2->map_control(3, id, 13);
        v2->map_control(4, id, 14);
        v2->map_control(5, id, 3);
        v2->map_control(6, id, 2);
        v2->map_control(5, id, 8);
        v2->map_control(6, id, 7);
        v2->map_control(61, id, 20);
        v2->map_control(67, id, 20);
        v2->map_control(63, id, 16);
        v2->map_control(64, id, 17);
        v2->map_control(65, id, 3);
        v2->map_control(66, id, 2);
        v2->map_control(61, id, 10);
        v2->map_control(62, id, 11);
        v2->map_control(62, id, 21);
        v2->map_control(70, id, 3);
        v2->map_control(71, id, 2);
        v2->map_control(78, id, 16);
        v2->map_control(79, id, 17);
        v2->map_control(70, id, 1);
        v2->map_control(71, id, 0);
        v2->map_control(68, id, 10);
        v2->map_control(69, id, 11);
        v2->map_control(69, id, 14);
        v2->map_control(72, id, 20);
        v2->map_control(73, id, 21);


        static constexpr device_id_t KEYBOARD_DEVICE = static_cast<device_id_t>(0x1E8480);

        {
            enum {
                EDITCAM_FORWARD = 14,
                EDITCAM_BACKWARD = 15,
            };

            v2->map_control(EDITCAM_FORWARD, KEYBOARD_DEVICE, KB_I);
            v2->map_control(EDITCAM_BACKWARD, KEYBOARD_DEVICE, KB_K);
        }

        if constexpr (0)
        {
            v2->map_control(74, KEYBOARD_DEVICE, 86);
            v2->map_control(75, KEYBOARD_DEVICE, 85);
            v2->map_control(76, KEYBOARD_DEVICE, 84);
            v2->map_control(77, KEYBOARD_DEVICE, 83);
            v2->map_control(68, KEYBOARD_DEVICE, 45);
            v2->map_control(69, KEYBOARD_DEVICE, 54);
            v2->map_control(53, KEYBOARD_DEVICE, 77);
            v2->map_control(10, KEYBOARD_DEVICE, 92);

            v2->map_control(16, KEYBOARD_DEVICE, 10);
            v2->map_control(17, KEYBOARD_DEVICE, 12);
            v2->map_control(16, KEYBOARD_DEVICE, 17);
            v2->map_control(17, KEYBOARD_DEVICE, 5);
            v2->map_control(18, KEYBOARD_DEVICE, 15);
            v2->map_control(19, KEYBOARD_DEVICE, 21);
            v2->map_control(18, KEYBOARD_DEVICE, 24);
            v2->map_control(19, KEYBOARD_DEVICE, 26);
            v2->map_control(22, KEYBOARD_DEVICE, 18);
            v2->map_control(23, KEYBOARD_DEVICE, 6);
            v2->map_control(24, KEYBOARD_DEVICE, 4);
            v2->map_control(25, KEYBOARD_DEVICE, 1);
            v2->map_control(26, KEYBOARD_DEVICE, 37);
            v2->map_control(26, KEYBOARD_DEVICE, 38);
            v2->map_control(27, KEYBOARD_DEVICE, 41);
            v2->map_control(27, KEYBOARD_DEVICE, 42);
            v2->map_control(29, KEYBOARD_DEVICE, 20);
            v2->map_control(30, KEYBOARD_DEVICE, 22);
            v2->map_control(31, KEYBOARD_DEVICE, 3);
            v2->map_control(32, KEYBOARD_DEVICE, 49);
            v2->map_control(28, KEYBOARD_DEVICE, 54);
            v2->map_control(33, KEYBOARD_DEVICE, 91);
            v2->map_control(34, KEYBOARD_DEVICE, 90);
            v2->map_control(35, KEYBOARD_DEVICE, 27);
            v2->map_control(36, KEYBOARD_DEVICE, 28);
            v2->map_control(37, KEYBOARD_DEVICE, 29);
            v2->map_control(38, KEYBOARD_DEVICE, 30);
            v2->map_control(39, KEYBOARD_DEVICE, 31);
            v2->map_control(40, KEYBOARD_DEVICE, 32);
            v2->map_control(41, KEYBOARD_DEVICE, 33);
            v2->map_control(42, KEYBOARD_DEVICE, 34);
            v2->map_control(43, KEYBOARD_DEVICE, 35);
            v2->map_control(44, KEYBOARD_DEVICE, 36);
            v2->map_control(45, KEYBOARD_DEVICE, 13);
            v2->map_control(46, KEYBOARD_DEVICE, 16);
            v2->map_control(47, KEYBOARD_DEVICE, 60);
            v2->map_control(48, KEYBOARD_DEVICE, 38);
            v2->map_control(49, KEYBOARD_DEVICE, 66);
            v2->map_control(50, KEYBOARD_DEVICE, 65);
            v2->map_control(51, KEYBOARD_DEVICE, 72);
            v2->map_control(7, KEYBOARD_DEVICE, 7);
            v2->map_control(8, KEYBOARD_DEVICE, 70);
            v2->map_control(9, KEYBOARD_DEVICE, 69);
            v2->map_control(119, KEYBOARD_DEVICE, 68);

            v2->map_control(116, KEYBOARD_DEVICE, 51);
            v2->map_control(117, KEYBOARD_DEVICE, 50);
            v2->map_control(118, KEYBOARD_DEVICE, 47);
        }

        {
            static constexpr device_id_t MOUSE_DEVICE = static_cast<device_id_t>(0x2DC6C0); 
            v2->map_control(20, MOUSE_DEVICE, 1);
            v2->map_control(21, MOUSE_DEVICE, 0);
            v2->map_control(26, MOUSE_DEVICE, 3);
        }

        v2->map_control(54, id, 20);
        v2->map_control(54, id, 22);
        v2->map_control(55, id, 10);

        map_pc_2_playstation_inputs();

        map_spiderman_controls();
    }
    else
    {

		 
		 THISCALL(0x00605950, a1);
    }
}

void game__setup_input_registrations(game *a1)
{
    if constexpr (0)
    {}
    else
    {

		 THISCALL(0x006063C0, a1);
    }
}

void game_patch()
{
    REDIRECT(0x0052B4BA, sub_5BC870);

    {
        REDIRECT(0x0052B5D7, nglListEndScene_hook);
    }

    {
        FUNC_ADDRESS(address, &game::load_hero_packfile);
        REDIRECT(0x0055B44E, address);
    }

    {
        FUNC_ADDRESS(address, &game::level_load_stuff::look_up_level_descriptor);
        REDIRECT(0x0055D3DE, address);
    }

    {
        FUNC_ADDRESS(address, &game::one_time_init_stuff);
        REDIRECT(0x005581D5, address);
    }

    {
        FUNC_ADDRESS(address, &game::one_time_deinit_stuff);
        REDIRECT(0x00559EB5, address);
    }

    {
        FUNC_ADDRESS(address, &game::render_ui);
        REDIRECT(0x0073E9D1, address);
    }

    {
        FUNC_ADDRESS(address, &game::render_world);
        REDIRECT(0x00741CD3, address);
    }

    {
        FUNC_ADDRESS(address, &game::advance_state_running);
        REDIRECT(0x0055D5EE, address);
    }

    {
        FUNC_ADDRESS(address, &game::load_this_level);
        REDIRECT(0x0055D409, address);
    }

    {
        FUNC_ADDRESS(address, &game::advance_state_load_level);
        REDIRECT(0x0055D55D, address);
    }

    {
        FUNC_ADDRESS(address, &game::handle_game_states);
        REDIRECT(0x0055D742, address);
    }

    {
        FUNC_ADDRESS(address, &game::frame_advance);
        REDIRECT(0x005D70B8, address);
    }

    {
        FUNC_ADDRESS(address, &game::frame_advance_level);
        REDIRECT(0x0055D8C2, address);
    }

    {
        FUNC_ADDRESS(address, &game::handle_cameras);
        REDIRECT(0x0055D74F, address);
    }

    {
        FUNC_ADDRESS(address, &game::set_camera);
        REDIRECT(0x00552FA3, address);
        REDIRECT(0x00552FC2, address);
    }

    {
        FUNC_ADDRESS(address, &game::unload_current_level);
        SET_JUMP(0x00557E10, address);
    }

    if constexpr (0)
    {
        REDIRECT(0x00514EC1, game__setup_inputs);
        REDIRECT(0x005244CA, game__setup_inputs);
        REDIRECT(0x00557A48, game__setup_inputs);
    }

    if constexpr (0) {

        {
            FUNC_ADDRESS(address, &game::advance_state_paused);
            SET_JUMP(0x00558220, address);
        }

        REDIRECT(0x0055CB44, debug_render_init);

        //game::game()
        {
            REDIRECT(0x005576B0, sub_538D10);

            REDIRECT(0x005576B5, script_manager::init);

            REDIRECT(0x005576BA, construct_script_controllers);

            REDIRECT(0x005576BF, scene_entity_base::initialize);

            REDIRECT(0x0055770E, geometry_manager::rebuild_view_frame);
        }

        {
            FUNC_ADDRESS(address, &game::frame_advance);
            REDIRECT(0x005D70B8, address);
        }

        {
            FUNC_ADDRESS(address, &game::clear_screen);
            REDIRECT(0x00557BC3, address);
            REDIRECT(0x00557BF9, address);
        }

        {
            FUNC_ADDRESS(address, &game::render_intros);
            REDIRECT(0x0055A048, address);
        }

        {
            FUNC_ADDRESS(address, &game::unload_hero_packfile);
            REDIRECT(0x0055A4C7, address);
        }

        {
            FUNC_ADDRESS(address, &game::level_load_stuff::construct_loading_widgets);
            //REDIRECT(0x0055D402, address);
        }

        {
            FUNC_ADDRESS(address, &game::advance_state_legal);
            REDIRECT(0x0055D539, address);
        }

        {
            FUNC_ADDRESS(address, &game::clear_screen);
            REDIRECT(0x00558162, address);
        }

        {
            //REDIRECT(0x005581C6, resource_manager::push_resource_context);
            REDIRECT(0x005581CE, localized_string_table::load_localizer);
        }


        {
            FUNC_ADDRESS(address, &game::level_load_stuff::destroy_loading_widgets);
            REDIRECT(0x0055D43C, address);
        }

        {
            REDIRECT(0x0052B5A6, render_text);
            REDIRECT(0x00514DA8, render_text);
            REDIRECT(0x00514E4B, render_text);
        }

        {
            FUNC_ADDRESS(address, &game::enable_marky_cam);
            REDIRECT(0x0057EB54, address);
                }

        {
		        FUNC_ADDRESS(address, &game::message_board_init);
        REDIRECT(0x00552E7C, address);
    }
	        {
			FUNC_ADDRESS(address, &game::game::pause);
	        REDIRECT(0x0060BEB4, address);
			REDIRECT(0x0061940A, address);
			REDIRECT(0x00622867, address);
			REDIRECT(0x00622AD5, address);
	        REDIRECT(0x0062A0B1, address);
			REDIRECT(0x00662F0D, address);
			REDIRECT(0x006737C6, address);
    }
    }
}




