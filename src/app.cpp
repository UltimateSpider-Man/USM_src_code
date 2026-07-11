#include "app.h"

#include "actor.h"
#include "ambient_audio_manager.h"
#include "audio_box_manager.h"
#include "collide.h"
#include "comic_panels.h"
#include "common.h"
#include "custom_math.h"
#include "cut_scene_player.h"
#include "damage_morphs.h"
#include "debug_menu.h"
#include "event_manager.h"
#include "femanager.h"
#include "fixedstring.h"
#include "frontendmenusystem.h"
#include "func_wrapper.h"
#include "gab_manager.h"
#include "game.h"
#include "geometry_manager.h"
#include "input_mgr.h"
#include "link_system.h"
#include "log.h"
#include "memory.h"
#include "mission_stack_manager.h"
#include "ngl.h"
#include "nlPlatformEnum.h"
#include "os_developer_options.h"
#include "pc_input_mgr.h"
#include "physics_system.h"
#include "resource_manager.h"
#include "scratchpad_stack.h"
#include "script_memtrack.h"
#include "script_sound_manager.h"
#include "shadow.h"
#include "smoke_test.h"
#include "sound_manager.h"
#include "spider_monkey.h"
#include "string_hash_dictionary.h"
#include "timer.h"
#include "tokenizer.h"
#include "trace.h"
#include "trigger_manager.h"
#include "unit_tests.h"
#include "utility.h"
#include "window_manager.h"

#include <cassert>

GameConfig g_config;

VALIDATE_SIZE(app, 0x3Cu);

VALIDATE_OFFSET(app, m_game, 0x30);

app *& app::instance = var<app *>(0x009685D4);

nglTexture *& g_shadow_target_blurred = var<nglTexture *>(0x00966EF4);

int & dword_966F8C = var<int>(0x00966F8C);

nglTexture *& g_shadow_target_unblurred = var<nglTexture *>(0x00965F48);

void sub_592E40()
{
    for (auto &s : g_shadow())
    {
        if (s.field_44 != nullptr) {
            nglDestroyTexture(s.field_44);
        }
    }

    if (g_shadow_target_unblurred != nullptr) {
        nglDestroyTexture(g_shadow_target_unblurred);
    }
}

//0x00592E80
void init_shadow_targets()
{
    TRACE("init_shadow_targets");

#if defined(ENABLE_DEBUG_MENU) && DEBUG_MENU_REIMPL == 0
    debug_menu::init();
#endif
    
    sub_592E40();

    tlFixedString v1;
    for ( auto &s : g_shadow() )
    {
        s.field_44 = nglCreateTexture(4609, 128, 128, 0, 1);
        v1 = tlFixedString{"blurred shadow texture"};
        s.field_44->field_60 = v1;
    }

    g_shadow_target_unblurred = nglCreateTexture(4609u, 256, 256, 0, 1);

    g_shadow_target_unblurred->field_60 = tlFixedString {"unblurred shadow"};
}

void set_god_mode(int a1)
{
    bool & god_mode_cheat = var<bool>(0x0095A6A8);
    bool & ultra_god_mode_cheat = var<bool>(0x0095A6A9);
    bool & mega_god_mode_cheat = var<bool>(0x0095A6AA);

    god_mode_cheat = false;
    ultra_god_mode_cheat = false;
    mega_god_mode_cheat = false;
    switch (a1) {
    case 1:
        god_mode_cheat = true;
        return;
    case 2:
        god_mode_cheat = true;
        ultra_god_mode_cheat = true;
        break;
    case 3:
        god_mode_cheat = true;
        mega_god_mode_cheat = true;
        break;
    case 4:
        ultra_god_mode_cheat = true;
        break;
    case 5:
        mega_god_mode_cheat = true;
        break;
    }
}

void colgeom_init_lists()
{
    CDECL_CALL(0x00544E90);
}

void colgeom_destroy_lists()
{
    CDECL_CALL(0x005489A0);
}

app::app()
{
    TRACE("app::app");
    this->m_vtbl = 0x00891634;

    unit_tests();
    mem_print_stats("after unit tests");
    g_platform = NL_PLATFORM_PC;
    if (link_system::use_link_system())
    {
        /*
        link_system::init();
        link_system::add_recipient(spider_monkey::link_receive);
        link_system::add_recipient(game::link_receive);
        */
        mem_print_stats("after link_system::init()");
    }

#if defined(ENABLE_DEBUG_MENU) && !DEBUG_MENU_REIMPL
    //debug_menu::init();
#endif

    init_shadow_targets();
    mem_print_stats("after init_shadow_targets()");

    this->field_34.reset();
    this->field_38 = 0;
    geometry_manager::create_inst();
    string_hash_dictionary::create_inst();
    event_manager::create_inst();

    trigger_manager::create_inst();
    pc_input_mgr::create_inst();
    input_mgr::create_inst();
    sound_manager::create_inst();
    script_sound_manager::create_inst();
    ambient_audio_manager::create_inst();
    if (!os_developer_options::instance->get_flag(mString {"DISABLE_AUDIO_BOXES"})) {
        audio_box_manager::create_inst();
    }

    gab_manager::create_inst();

    set_god_mode(os_developer_options::instance->get_int(mString {"GOD_MODE"}));

    colgeom_init_lists();
    physics_system_init();

    this->m_game = new game{};
    g_game_ptr = this->m_game;

    resource_manager::create_inst();
    if (os_developer_options::instance->get_int(mString {"MONKEY_MODE"}) > 0) {
        spider_monkey::start();
    }

    damage_morphs::init_memory_pools();

    {
        auto *inst = mission_stack_manager::get_instance();
        inst->start_streaming();
    }
}


void app::app1()
{
    TRACE("app::app");
    this->m_vtbl = 0x00891634;

    unit_tests();
    mem_print_stats("after unit tests");
    g_platform = NL_PLATFORM_PC;
    if (link_system::use_link_system())
    {
        /*
        link_system::init();
        link_system::add_recipient(spider_monkey::link_receive);
        link_system::add_recipient(game::link_receive);
        */
        mem_print_stats("after link_system::init()");
    }

#if defined(ENABLE_DEBUG_MENU) && !DEBUG_MENU_REIMPL
    //debug_menu::init();
#endif

    init_shadow_targets();
    mem_print_stats("after init_shadow_targets()");

    this->field_34.reset();
    this->field_38 = 0;
    geometry_manager::create_inst();
    string_hash_dictionary::create_inst();
    event_manager::create_inst();

    trigger_manager::create_inst();
    pc_input_mgr::create_inst();
    input_mgr::create_inst();
    sound_manager::create_inst();
    script_sound_manager::create_inst();
    ambient_audio_manager::create_inst();
    if (!os_developer_options::instance->get_flag(mString {"DISABLE_AUDIO_BOXES"})) {
        audio_box_manager::create_inst();
    }

    gab_manager::create_inst();

    set_god_mode(os_developer_options::instance->get_int(mString {"GOD_MODE"}));

    colgeom_init_lists();
    physics_system_init();

    this->m_game = new game{};
    g_game_ptr = this->m_game;

    resource_manager::create_inst();
    if (os_developer_options::instance->get_int(mString {"MONKEY_MODE"}) > 0) {
        spider_monkey::start();
    }

    damage_morphs::init_memory_pools();

    {
        auto *inst = mission_stack_manager::get_instance();
        inst->start_streaming();
    }
}


app::~app()
{
    this->m_vtbl = 0x00891634;

    if (this->m_game != nullptr) {
        delete this->m_game;
    }

    g_game_ptr = nullptr;
    this->m_game = nullptr;

    this->cleanup();
    //debug_menu::deinit(); // link_system::un_init()
    
    physics_system_shutdown();

    this->m_vtbl = 0x0088E4C8;
}

void app::internal::begin_screen_recording(const mString &a2, int a3)
{
    if (this->field_18 != 2)
    {
        this->field_18 = 2;
        this->field_0 = a2;
        this->field_14 = 0;
        os_developer_options::instance->set_int(mString{"CAMERA_CENTRIC_STREAMER"}, a3);
    }
}

void app::internal::end_screen_recording() {
    this->field_18 = 0;
    os_developer_options::instance->set_int(mString{"CAMERA_CENTRIC_STREAMER"}, 0);
}

void app::internal::sub_5B8670()
{
    if constexpr (1) {
        if (g_smoke_test() != nullptr) {
            g_smoke_test()->frame_advance();
        }
    }

    char Dest[64];

    auto v2 = this->field_18;
    if (v2 != 0)
    {
        if (v2 == 1) {
            sprintf(Dest, "screenshot%.4d", this->field_10++);
        } else {
            sprintf(Dest, "%s%.4d", this->field_0.c_str(), this->field_14++);
        }

        nglTexture *v6 = nglGetFrontBufferTex();
        nglSaveTexture(v6, Dest);
        if (this->field_18 == 1) {
            this->field_18 = 0;
        }
    }
}

void app::tick()
{
    TRACE("app::tick");

    {
        float v6 = this->field_34.elapsed();
        sp_log("%f", v6);

        auto frame_lock = os_developer_options::instance->get_int(mString {"FRAME_LOCK"});
        sp_log("frame_lock = %d", frame_lock);

        float time_inc = 0.0f;
        do
        {
            time_inc = this->field_34.elapsed();
            g_game_ptr->handle_frame_locking(&time_inc);

            assert(time_inc >= 0 && time_inc < 1e9f);

            const float v4 = 0.066733405f;
            if ( time_inc > v4 ) {
                time_inc = v4;
            }
        }
        while ( 0 /* time_inc < 0.0f */ );

        this->field_34.reset();
    }

    if constexpr (0)
    {
        limited_timer_base local_timer;
        local_timer.reset();

        scratchpad_stack::reset();

        limited_timer_base total_timer;
        total_timer.reset();

        sub_77B2F0(0);

        float time_inc;
        for (time_inc = g_timer()->sub_5821D0(); equal(time_inc, 0.0f);
             time_inc = g_timer()->sub_5821D0()) {
            Sleep(0);
        }

        static bool & byte_9682F0 = var<bool>(0x009682F0);

        if (time_inc <= 0.0f)
        {
            byte_9682F0 = true;

            if (g_smoke_test() != nullptr) {
                g_smoke_test()->frame_advance();
            }

            if ( (g_game_ptr->flag.level_is_loaded && !g_game_ptr->field_165) ||
                    g_femanager.m_fe_menu_system != nullptr
                    && g_femanager.m_fe_menu_system->sub_60C230()
                    && g_cut_scene_player()->is_playing()) {
                comic_panels::render();
            } else if (g_femanager.m_fe_menu_system == nullptr || !g_femanager.m_fe_menu_system->sub_60C230()) {
                game::render_empty_list();
            }

            this->field_4.sub_5B8670();
            actor::swap_all_mesh_buffers();

        }
        else
        {
            slab_allocator::process_lists();
            script_memtrack::frame_advance();
            if (!IsWindow(window_manager::instance()->field_4)) {
                return;
            }

            event_manager::garbage_collect();
            input_mgr::instance->poll_devices();

            assert(time_inc >= 0 && time_inc < 10.0f);

            static float & dword_9682D0 = var<float>(0x009682D0);
            static float & dword_9680A8 = var<float>(0x009680A8);

            dword_9682D0 = time_inc;
            dword_9680A8 = time_inc;
            nflUpdate();
            resource_manager::frame_advance(time_inc);
            link_system::frame_advance(time_inc);
            this->m_game->frame_advance(time_inc);
            byte_9682F0 = false;
        }

        if (os_developer_options::instance->get_int(mString{"FRAME_LIMIT"}))
        {
            while (local_timer.elapsed() < 0.033333335) {
                ;
            }
        }

        this->m_game->field_278 = total_timer.elapsed();
        this->m_game->field_280 = 0;

    }
    else
    {
        THISCALL(0x005D6FC0, this);
    }
}




void app::create_inst()
{
    TRACE("app::create_inst");

    assert(instance == nullptr);

    if constexpr (1) {
        instance = new app {};
    } else {
        CDECL_CALL(0x005B2450);
    }
}

void app::cleanup()
{
    TRACE("app::cleanup");

    if constexpr (1)
    {
        gab_manager::delete_inst();

        if ( !os_developer_options::instance->get_flag(mString {"DISABLE_AUDIO_BOXES"}) ) { 
            audio_box_manager::delete_inst();
        }

        ambient_audio_manager::delete_inst();
        script_sound_manager::delete_inst();
        sound_manager::delete_inst();

        if ( input_mgr::instance != nullptr )
        {
            delete input_mgr::instance;
            input_mgr::instance = nullptr;
        }

        string_hash_dictionary::delete_inst();

        if ( pc_input_mgr::instance() != nullptr )
        {
            delete pc_input_mgr::instance();
            pc_input_mgr::instance() = nullptr;
        }

        if ( trigger_manager::instance != nullptr )
        {
            delete trigger_manager::instance;
            trigger_manager::instance = nullptr;
        }

        event_manager::delete_inst();
        colgeom_destroy_lists();
        resource_manager::delete_inst();
        nglReleaseAllMeshFiles();
        nglReleaseAllTextures();
    } else {
        CDECL_CALL(0x005D9430);
    }
}

void app_patch()
{
    if constexpr (1)
    {
        REDIRECT(0x005E10BF, unit_tests);
    }

    REDIRECT(0x005AD2E9, app::create_inst);

    {
        FUNC_ADDRESS(address, &app::tick);
        REDIRECT(0x005AD495, address);
    }

    {
        FUNC_ADDRESS(address, &app::internal::sub_5B8670);
        SET_JUMP(0x005B8670, address);
    }

    REDIRECT(0x005E10EE, init_shadow_targets);

}

void app1_patch()
{


    {
        FUNC_ADDRESS(address, &app::app1);
        REDIRECT(0x005AD495, address);
    }



}
