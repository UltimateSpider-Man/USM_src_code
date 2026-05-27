#include "forwards.h"

#include <unordered_map>
#include "resource_versions.h"
#include "main.h"
#include "aeps.h"
#include "ai_find_best_swing_anchor.h"
#include "ai_interact_resource_handler.h"
#include "ai_interaction_data.h"
#include "ai_path.h"
#include "ai_player_controller.h"
#include "ai_state_graph_resource_handler.h"
#include "ai_state_jump.h"
#include "ai_state_run.h"
#include "ai_state_swing.h"
#include "ai_state_web_zip.h"
#include "ai_std_hero.h"
#include "als_animation_logic_system.h"
#include "als_animation_logic_system_shared.h"
#include "als_animation_logic_system_interface.h"
#include "als_basic_rule_data.h"
#include "als_category.h"
#include "als_inode.h"
#include "als_meta_anim_base.h"
#include "als_meta_anim_swing.h"
#include "als_meta_aimed_shot_vert.h"
#include "als_meta_anim_table_shared.h"
#include "als_mocomp.h"
#include "als_motion_compensator.h"
#include "als_resource_handler.h"
#include "als_res_data.h"
#include "als_simple_orientation.h"
#include "als_state.h"
#include "als_scripted_category.h"
#include "als_scripted_state.h"
#include "als_transition_group_base.h"
#include "alternate_costumes.h"
#include "anchor_query_visitor.h"
#include "animation_controller.h"
#include "animation_interface.h"
#include "anim_handle.h"
#include "anim_resource_handler.h"
#include "anim_record.h"
#include "app.h"
#include "base_ai_core.h"
#include "base_ai_res_state_graph.h"
#include "base_ai_resource_handler.h"
#include "base_ai_state_machine.h"
#include "base_tl_resource_handler.h"
#include "beam.h"
#include "box_trigger_resource_handler.h"
#include "cached_special_effect.h"
#include "camera.h"
#include "camera_mode.h"
#include "camera_target_info.h"
#include "character_anim_controller.h"
#include "character_pose_skel.h"
#include "character_viewer.h"
#include "charcomponentbase.h"
#include "chuck_callbacks.h"
#include "city_lod.h"
#include "console.h"
#include "collide_aux.h"
#include "collision_capsule.h"
#include "colmesh.h"
#include "combat_state.h"
#include "combo_system.h"
#include "combo_system_move.h"
#include "common.h"
#include "conglom.h"
#include "core_ai_resource.h"
#include "cursor.h"
#include "custom_math.h"
#include "cut_scene.h"
#include "cut_scene_player.h"
#include "cut_scene_resource_handler.h"
#include "damage_interface.h"
#include "daynight.h"
#include "debug_struct.h"
#include "eligible_pack.h"
#include "eligible_pack_streamer.h"
#include "enhanced_state.h"
#include "entity_base.h"
#include "entity_base_vhandle.h"
#include "entity_handle_manager.h"
#include "entity_mash.h"
#include "event_manager.h"
#include "fe_dialog_text.h"
#include "fe_mini_map_widget.h"
#include "fe_mission_text.h"
#include "fefloatingtext.h"
#include "femanager.h"
#include "femenuentry.h"
#include "femultilinetext.h"
#include "fe_menu_nav_bar.h"
#include "filespec.h"
#include "fileusm.h"
#include "frontendmenusystem.h"
#include "func_wrapper.h"
#include "fx_cache.h"
#include "gab_database_resource_handler.h"
#include "game.h"
#include "game_camera.h"
#include "game_data_meat.h"
#include "game_settings.h"
#include "gamepadinput.h"
#include "gamepad_menu.h"
#include "geometry_manager.h"
#include "ghetto_mash_file_header.h"
#include "glass_house_manager.h"
#include "glass_house_resource_handler.h"
#include "hero_base_state.h"
#include "hierarchical_entity_proximity_map.h"
#include "igofrontend.h"
#include "igozoomoutmap.h"
#include "item_resource_handler.h"
#include "info_node_desc_list.h"
#include "ini_parser.h"
#include "input_action.h"
#include "input_mgr.h"
#include "inputsettings.h"
#include "interactable_interface.h"
#include "interaction_inode.h"
#include "interaction_state.h"
#include "layer_state_machine_shared.h"
#include "line_anchor.h"
#include "link_system.h"
#include "localized_string_table.h"
#include "local_collision.h"
#include "lookat_target_controller.h"
#include "mAvlTree.h"
#include "main_menu_keyboard.h"
#include "main_menu_options.h"
#include "mash_info_struct.h"
#include "mash_virtual_base.h"
#include "matrix4x4.h"
#include "memory.h"
#include "mesh_file_resource_handler.h"
#include "material_file_resource_handler.h"
#include "message_board.h"
#include "meta_anim_interact.h"
#include "motion_effect_struct.h"
#include "mission_manager.h"
#include "morph_file_resource_handler.h"
#include "debug_render.h"
#include "moved_entities.h"
#include "mouselook_controller.h"
#include "mstring.h"
#include "multilinestring.h"
#include "nal_anim_controller.h"
#include "nal_system.h"
#include "nfl_driver.h"
#include "nfl_system.h"
#include "ngl.h"
#include "ngl_math.h"
#include "ngl_mesh.h"
#include "ngl_vertexdef.h"
#include "nslbank.h"
#include "oldmath_po.h"
#include "os_developer_options.h"
#include "os_file.h"
#include "panel_resource_handler.h"
#include "panelanimfile.h"
#include "panelfile.h"
#include "panelquad.h"
#include "panelquadsection.h"
#include "panelmeshsection.h"
#include "param_block.h"
#include "param_list.h"
#include "path_resource_handler.h"
#include "patrol_def_resource_handler.h"
#include "pause_menu_controller.h"
#include "pause_menu_message_log.h"
#include "pause_menu_root.h"
#include "pause_menu_save_load_display.h"
#include "pause_menu_status.h"
#include "pause_menu_awards.h"
#include "pause_menu_transition.h"
#include "pause_menu_options_display.h"
#include "pausemenusystem.h"
#include "pc_joypad_device.h"
#include "ped_spawner.h"
#include "physical_interface.h"
#include "physics_inode.h"
#include "physics_system.h"
#include "pick_up_state.h"
#include "player_controller_inode.h"
#include "plr_loco_crawl_state.h"
#include "plr_loco_crawl_transition_state.h"
#include "pole_swing_state.h"
#include "polytube.h"
#include "quaternion.h"
#include "quick_anchor_info.h"
#include "rbc_def_contact.h"
#include "region.h"
#include "resource_amalgapak_header.h"
#include "resource_directory.h"
#include "resource_manager.h"
#include "resource_pack_standalone.h"
#include "resource_pack_streamer.h"
#include "return_address.h"
#include "rigid_body.h"
#include "rumble_manager.h"
#include "scene_anim.h"
#include "scene_anim_resource_handler.h"
#include "scratchpad_stack.h"
#include "script.h"
#include "script_access.h"
#include "script_object.h"
#include "script_data_interface.h"
#include "script_executable.h"
#include "script_controller.h"
#include "script_manager.h"
#include "script_memtrack.h"
#include "script_var_container.h"
#include "settings.h"
#include "sin_container.h"
#include "skeleton_resource_handler.h"
#include "slc_manager.h"
#include "sound_alias_database.h"
#include "sound_alias_database_resource_handler.h"
#include "sound_bank_slot.h"
#include "sound_interface.h"
#include "sound_manager.h"
#include "spawnable.h"
#include "spiderman_camera.h"
#include "spider_monkey.h"
#include "spidey_base_state.h"
#include "spline.h"
#include "state_machine.h"
#include "subdivision_static_region_list.h"
#include "string_hash.h"
#include "string_hash_dictionary.h"
#include "swing_anchor_obbfilter.h"
#include "terrain.h"
#include "texture_resource_handler.h"
#include "texturehandle.h"
#include "textureinputpack.h"
#include "timer.h"
#include "tl_system.h"
#include "tl_instance_bank.h"
#include "tlresourcedirectory.h"
#include "tlresource_directory.h"
#include "traffic.h"
#include "traffic_path_lane.h"
#include "trigger_manager.h"
#include "tx_system.h"
#include "unlockables_menu.h"
#include "us_decal.h"
#include "us_frontend.h"
#include "us_lod.h"
#include "us_person.h"
#include "us_simpleshader.h"
#include "us_translucentshader.h"
#include "us_variant.h"
#include "usm_anim_player.h"
#include "utility.h"
#include "variables.h"
#include "vector3d.h"
#include "variant_interface.h"
#include "vm.h"
#include "vm_executable.h"
#include "vm_thread.h"
#include "vtbl.h"
#include "wds.h"
#include "web_interface.h"
#include "web_polytube.h"
#include "window_manager.h"
#include "worldly_pack_slot.h"
#include "script_file_loader.h"



#include "main_menu_credits.h"

#include "input.h"

#include <ngl_dx_scene.h>
#include <ctime>
#include <corecrt_startup.h>

#include "debug_menu.h"
#include "debug_menu2.h"
#include "debug_menu_extra.h"

//
#include <list.hpp>
#include <vector.hpp>

#include <cassert>
#include <cmath>
#include <cstdio>
#include <dinput.h>
#include <direct.h>

#include <float.h>
#include <windows.h>

#include <dsound.h>

#include "common.h"
#include "ngl.h"
#include "ngl_font.h"
#include "game.h"

#include "pause_menu_goals.h"


#include "fixedstring.h"
#include "mission_table_container.h"

#include "info_node.h"
#include "entity_base_vhandle.h"


namespace fs = std::filesystem;

std::map<uint32_t, Mod> Mods;
std::unordered_map<std::string, std::filesystem::path> ModFileOverrides;

void register_class_and_create_window(LPCSTR lpClassName,
                                      LPCSTR lpWindowName,
                                      int X,
                                      int Y,
                                      int a5,
                                      int a6,
                                      WNDPROC a7,
                                      HINSTANCE hInstance,
                                      int a9,
                                      DWORD dwStyle);

#define TEXT_START 0x00401000
#define TEXT_END 0x00988000

DWORD old_perms = 0;
BOOL set_text_to_writable() {
    return VirtualProtect((void *) TEXT_START, TEXT_END - TEXT_START, PAGE_READWRITE, &old_perms);
}

BOOL restore_text_perms() {
    return VirtualProtect((void*)(TEXT_START), TEXT_END - TEXT_START, old_perms, &old_perms);
}



// Toggles between windowed and exclusive-fullscreen at runtime.
//
// Three things have to happen for windowed mode to actually look right:
//   1. g_Windowed / g_config.WindowedMode must agree, because the engine
//      reads them from different places (renderer init, present-params
//      builder, input clipping, etc.).
//   2. The game's internal D3D path at 0x0076D230 has to flip the device
//      (it ultimately drives IDirect3DDevice9::Reset with present params
//      derived from g_Windowed).
//   3. The Win32 window itself has to be restyled, resized, and re-placed
//      — without this the OS window is still the popup/topmost shape we
//      built for the previous mode and the result looks broken.
//
// All entry points (init_hook on startup, ALT+ENTER at runtime, debug menu)
// route through here so the three pieces stay in sync.
void apply_windowed_mode(bool windowed)
{
    // 1) Update globals first so any callbacks pumped by the calls below
    //    (WM_SIZE, WM_ACTIVATEAPP, ...) observe the new mode.
    g_config.WindowedMode = windowed;
    g_Windowed() = windowed;

    // 2) Ask the game to flip its D3D device. Safe to call before the
    //    device exists — 0x0076D230 early-outs on null device pointer.
    ESI_CALL(0x0076D230, windowed);

    // 3) Keep the game ticking when it loses focus in windowed mode,
    //    otherwise alt-tabbing freezes audio / animation.
    if (os_developer_options::instance) {
        os_developer_options::instance->set_flag(mString{ "ALWAYS_ACTIVE" }, windowed);
    }

    // 4) Restyle / move the OS window. If the game hasn't created its
    //    window yet (early init), there's nothing to restyle and
    //    register_class_and_create_window will pick the right path
    //    based on g_config.WindowedMode set above.
    HWND hwnd = g_appHwnd();
    if (!hwnd) {
        return;
    }

    const int cx = g_cx();
    const int cy = g_cy();

    DWORD style;
    DWORD exStyle;
    int x, y, w, h;

    if (windowed) {
        // Real overlapped window: caption + sysmenu + minimize, no resize
        // (the game can't react to client-area changes anyway).
        style   = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_VISIBLE;
        exStyle = 0;

        RECT rc{ 0, 0, cx, cy };
        AdjustWindowRectEx(&rc, style, FALSE, exStyle);
        w = rc.right  - rc.left;
        h = rc.bottom - rc.top;

        // Position policy:
        //   - First entry to windowed mode (or coming from a fullscreen-shaped
        //     window): place inside the work area so the title bar isn't
        //     jammed against the screen edge or hidden by the taskbar.
        //   - Subsequent entries (init_hook re-runs, ALT+ENTER round trips):
        //     keep whatever position the user has dragged the window to.
        //     Without this, every toggle yanks the window back to center.
        const int screenW = GetSystemMetrics(SM_CXSCREEN);
        const int screenH = GetSystemMetrics(SM_CYSCREEN);

        RECT cur{};
        GetWindowRect(hwnd, &cur);
        const bool looksFullscreenShaped =
            (cur.left   <= 0 && cur.top    <= 0 &&
             cur.right  >= screenW && cur.bottom >= screenH);
        const bool firstPlacement = (cur.right - cur.left == 0);

        if (looksFullscreenShaped || firstPlacement) {
            // Center inside the work area (taskbar-aware).
            RECT work{ 0, 0, screenW, screenH };
            SystemParametersInfoA(SPI_GETWORKAREA, 0, &work, 0);
            const int workW = work.right  - work.left;
            const int workH = work.bottom - work.top;
            x = work.left + (workW - w) / 2;
            y = work.top  + (workH - h) / 2;
            if (x < work.left) x = work.left;
            if (y < work.top)  y = work.top;
        } else {
            // Preserve current top-left.
            x = cur.left;
            y = cur.top;
        }

        // Track non-client offset — the engine's mouse code reads these
        // to translate screen->client coords (same globals create_window
        // writes on initial startup).
        Var<int> dword_9874D4{ 0x009874D4 };
        Var<int> dword_955158{ 0x00955158 };
        dword_9874D4() = -rc.left;
        dword_955158() = -rc.top;
    } else {
        // Borderless popup covering the whole screen. We use WS_POPUP +
        // WS_EX_TOPMOST to match what the engine's fullscreen path builds.
        style   = WS_POPUP | WS_VISIBLE;
        exStyle = WS_EX_TOPMOST;
        x = 0;
        y = 0;
        w = GetSystemMetrics(SM_CXSCREEN);
        h = GetSystemMetrics(SM_CYSCREEN);

        Var<int> dword_9874D4{ 0x009874D4 };
        Var<int> dword_955158{ 0x00955158 };
        dword_9874D4() = 0;
        dword_955158() = 0;
    }

    // Apply style first, then move/resize with SWP_FRAMECHANGED so the
    // new non-client area is recomputed.
    SetWindowLongA(hwnd, GWL_STYLE,   static_cast<LONG>(style));
    SetWindowLongA(hwnd, GWL_EXSTYLE, static_cast<LONG>(exStyle));

    SetWindowPos(hwnd,
                 windowed ? HWND_NOTOPMOST : HWND_TOPMOST,
                 x, y, w, h,
                 SWP_FRAMECHANGED | SWP_SHOWWINDOW);

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
    SetForegroundWindow(hwnd);
    SetFocus(hwnd);

    // Re-clip the cursor to the new client rect in fullscreen, release in
    // windowed. Matches what WM_DESTROY's ClipCursor(nullptr) cleans up.
    if (windowed) {
        ClipCursor(nullptr);
    } else {
        RECT clip;
        if (GetClientRect(hwnd, &clip)) {
            POINT tl{ clip.left,  clip.top    };
            POINT br{ clip.right, clip.bottom };
            ClientToScreen(hwnd, &tl);
            ClientToScreen(hwnd, &br);
            clip.left = tl.x; clip.top = tl.y;
            clip.right = br.x; clip.bottom = br.y;
            ClipCursor(&clip);
        }
    }

    sp_log("apply_windowed_mode: %s\n", windowed ? "windowed" : "fullscreen");
}

// Back-compat shim — existing call sites (init_hook, ALT+ENTER) keep using
// this name.
void ToggleFullScreen(bool windowed)
{
    apply_windowed_mode(windowed);
}

void init_hook(HWND hwnd) {
	//  os_developer_options::instance->set_flag(mString{ "NO_LOAD_SCREEN" }, g_config.NoLoadScreen);
    bool windowedMode = g_config.WindowedMode;

    g_Windowed() = windowedMode;
    CDECL_CALL(0x0076E3E0, hwnd);
	
    g_config.WindowedMode = windowedMode;
    g_Windowed() = windowedMode;
    if (os_developer_options::instance) {
        os_developer_options::instance->set_flag(mString{"WINDOW_DEFAULT"}, windowedMode);
    }
	
					        if (os_developer_options::instance->get_flag(mString{ "ENABLE_FPU_EXCEPTION_HANDLING" }))
        {

                 os_developer_options::instance->set_flag(mString{ "ENABLE_FPU_EXCEPTION_HANDLING" }, g_config.EnableFpuExceptionHandling);

 
        }
	

    g_Windowed() = windowedMode;
    ToggleFullScreen(windowedMode);
}





HRESULT tga_hook(IDirect3DDevice9* dev, unsigned __int8* a2, unsigned int a3, IDirect3DBaseTexture9** a4)
{
    nglTexture* tex = reinterpret_cast<nglTexture*>(reinterpret_cast<uint8_t*>(a4) - offsetof(nglTexture, DXTexture));
    printf("loading TGA %s (0x%08X)\n", tex->field_60.to_string(), tex->field_60.m_hash);
    if (auto data = getModDataByHash(tex->field_60.m_hash))
        a2 = data;

    return (HRESULT)STDCALL(0x007CA291, dev, a2, a3, a4);
}

bool __cdecl readFile(const char* FileName, tlFileBuf* File, size_t Alignment, unsigned int Flags)
{
    return (bool)CDECL_CALL(0x0074A710, FileName, File, Alignment, Flags);
}

BOOL install_patches()
{
    sp_log("Installing patches\n");


    // @todo: global config
    REDIRECT(0x005AD218, init_hook);

    // @todo: debug menu
    {
        auto address = func_address(&nglStringNode::Render);
        set_vfunc(0x0088EBB4, address);
    }

    {
        auto address = func_address(&nglQuadNode::Render);
        SET_JUMP(0x00783670, address);
    }

    // SET_JUMP(0x0076D680, create_renderer);


    SET_JUMP(0x0076E050, nglListInit);

    SET_JUMP(0x0076EA10, nglListSend);
    
    
    // mod loading
    {
        // global assets
        REDIRECT(0x007700AE, readFile);    // Meshes
        REDIRECT(0x0077861E, readFile);    // ....
        REDIRECT(0x00779262, readFile);    // ....
        REDIRECT(0x0077A9A6, readFile);    // ....
        REDIRECT(0x0077AA2F, readFile);    // ....
        REDIRECT(0x0077AA69, readFile);    // ....
        REDIRECT(0x0077AADC, readFile);    // ....
        REDIRECT(0x00782CF9, readFile);    // ....
        REDIRECT(0x0078D653, readFile);    // ....
        REDIRECT(0x0078DA23, readFile);    // ....
        REDIRECT(0x0078DD5E, readFile);    // skeletons


        FUNC_ADDRESS(address, &mesh_file_resource_handler::_handle_resource);
        set_vfunc(0x00888A44, address);
        
        {
            FUNC_ADDRESS(address, &nglTexture::CreateTextureOrSurface);
            SET_JUMP(0x00775000, address);
        }

        {
            HRESULT(*func)(nglMeshSection*) = &nglSetStreamSourceAndDrawPrimitive;
            SET_JUMP(0x00771AF0, func);
        }

        REDIRECT(0x0056BDAA, nglLoadMeshFileInternal);
		        REDIRECT(0x0056C126, nglLoadMeshFileInternal);
        REDIRECT(0x0056C244, nglLoadMeshFileInternal);
        REDIRECT(0x0076FF90, nglLoadMeshFileInternal);
        REDIRECT(0x007700D9, nglLoadMeshFileInternal);
        REDIRECT(0x00778649, nglLoadMeshFileInternal);
		
		
		    



        // these funcs mysteriously are only used for TGA
        // and kinda look like it too, but I don't see em being used...
        // .. so they're here for prosperity
        REDIRECT(0x0077ABDE, tga_hook);
        REDIRECT(0x0077AB01, tga_hook);
        REDIRECT(0x0077A1D8, tga_hook);
    }
    sp_log("Patches have been installed\n");

    return TRUE;
}

static constexpr uint32_t NOP = 0x90;

void set_nop(ptrdiff_t address, size_t num_bytes) {
    for (size_t i = 0u; i < num_bytes; ++i) {
        *bit_cast<uint8_t *>(static_cast<size_t>(address) + i) = NOP;
    }
}

#define REDIRECT_WITH_NOP(addr, my_func)                                          \
    {                                                                             \
        *(uint8_t *) addr = 0xE8;                                                 \
        *(uint32_t *) ((uint8_t *) (addr + 1)) = ((uint32_t) my_func) - addr - 5; \
        *(uint8_t *) (addr + 5) = NOP;                                            \
        sp_log("Patched function sub_%08X with %s", addr, #my_func);              \
    }

#define MOVE(addr, my_func)                                            \
    {                                                                  \
        *bit_cast<uint8_t *>(addr) = 0xB9;                             \
        *(uint32_t *) ((uint8_t *) (addr + 1)) = ((uint32_t) my_func); \
        *bit_cast<uint8_t *>(addr + 5) = NOP;                          \
    }

void sub_76F320() {
    if constexpr (1) {
        struct Vtbl {
            int empty[7];
            void (__fastcall *field_1C)(void *, void *, int, int, int);
        };

        auto address = get_vtbl(nglMeshFileDirectory());

        if (bit_cast<std::ptrdiff_t>(address) != 0x008B8180) {
            sp_log("Invalid address of vtable = 0x%08X", address);

            set_vtbl(nglMeshFileDirectory(), 0x008B8180);

            address = get_vtbl(nglMeshFileDirectory());
        }

        Vtbl *vtbl = CAST(vtbl, address);

        vtbl->field_1C(nglMeshFileDirectory(), nullptr, 1, 1, 1); // 0x008B8180 -> sub_560770
    } else {
        CDECL_CALL(0x0076F320);
    }
}

bool sub_5A3AA0(const char *a1, char *a2) {
    return (bool) CDECL_CALL(0x005A3AA0, a1, a2);
}

static Var<bool> ALLOW_ERROR_POPUPS{0x00922A30};

void sub_597720(LPCSTR lpText) {
    char Dest[2048];
    char Format[2056];

    sprintf(Dest, "%s\n", lpText);
    OutputDebugStringA(Dest);

    {
        mString v1{Dest};
        CDECL_CALL(0x0051AB90, &v1);
    }

    sprintf(Format, "Error: \r\n%s", Dest);
    CDECL_CALL(0x005975C0, Format, 1, 1);
    if (ALLOW_ERROR_POPUPS()) {
        MessageBoxA(window_manager::instance()->field_4, lpText, "Error", 0x11010u);
    }

    link_system::send_command();
    _flushall();
    exit(-1);
}

LONG __stdcall TopLevelExceptionFilter(EXCEPTION_POINTERS *pExceptionInfo) {
    return (LONG) STDCALL(0x00597830, pExceptionInfo);
}

uint8_t color_ramp_function(float ratio, int period_duration, int cur_time) {

    if (cur_time <= 0 || 4 * period_duration <= cur_time)
        return 0;

    if (cur_time < period_duration) {

        float calc = ratio * cur_time;

        return std::min(calc, 1.0f) * 255;
    }


    if (period_duration <= cur_time && cur_time <= 3 * period_duration) {
        return 255;
    }


    if (cur_time <= 4 * period_duration) {

        int adjusted_time = cur_time - 3 * period_duration;
        float calc = 1.f - ratio * adjusted_time;

        return std::min(calc, 1.0f) * 255;
    }

    return 0;

}
void sub_81E130(int *a1) {
    CDECL_CALL(0x0081E130, a1);
}

void sub_81C1A0() {
    CDECL_CALL(0x0081C1A0);
}

void init_assert_handler() {
    CDECL_CALL(0x005BC9B0);
}

void sub_5C9EA0() {
    CDECL_CALL(0x005C9EA0);
}

void create_directory(const char *str) {
    if constexpr (1) {
        char Dest[260];

        strncpy(Dest, str, 260u);
        if (Dest[0]) {
            auto *v1 = Dest;
            char v2;
            do {
                if (*v1 == '\\') {
                    *v1 = 0;
                    mkdir(Dest);
                    *v1 = '\\';
                }
                v2 = (v1++)[1];
            } while (v2);
        }

        mkdir(Dest);
    } else {
        CDECL_CALL(0x0081BD80, str);
    }
}

//0x005B2240
void replace_if_find(char *begin, char *end, const char &a3, const char &a4) {
    if constexpr (0) {
        for (auto *i = begin; i != end; ++i) {
            if (*i == a3) {
                *i = a4;
            }
        }
    } else {
        std::replace(begin, end, a3, a4);
    }
}

void parse_cmd(const char *str)
{
    TRACE("parse_cmd");

    if constexpr (1) {
        char Dest[1024]{};
        strncpy(Dest, str, 1023u);
        for (auto *i = strtok(Dest, " "); i != nullptr; i = strtok(nullptr, " ")) {
            if (strnicmp(i, "pack", strlen(i)) == 0 || strnicmp(i, "repack", strlen(i)) == 0) {
                g_is_the_packer() = true;
            } else if (strnicmp(i, "smokelevel", strlen(i)) == 0 ||
                       strnicmp(i, "runlevel", strlen(i)) == 0) {
                strcpy(g_scene_name, strtok(nullptr, " "));
                if (strnicmp(i, "smokelevel", strlen(i)) == 0) {
                    os_developer_options::instance->set_flag(75, true);
                }

                os_developer_options::instance->set_flag(76, true);
            } else if (strnicmp(i, "smoketest", strlen(i)) == 0) {
                os_developer_options::instance->set_flag(75, true);
            } else if (strnicmp(i, "-entityids", strlen(i))) {
                if (strlen(i) > 2 && strnicmp(i, "-f", 2u) == 0) {
                    mString v13{i};

                    auto v2 = v13.find({2}, '=');
                    auto v3 = v2;
                    if (v2 != -1) {
                        mString v16 = v13.slice(2, v2);

                        mString v17 = v13.slice(v3 + 1, v13.size());

                        if (v16.size() && v17.size() == 1) {
                            if (*v17.c_str() == '1') {
                                os_developer_options::instance->set_flag(v16, true);
                            } else if (*v17.c_str() == '0') {
                                os_developer_options::instance->set_flag(v16, false);
                            }
                        }
                    }

                } else if (strlen(i) > 2 && strnicmp(i, "-i", 2u) == 0) {
                    mString v15{i};

                    auto v4 = v15.find({2}, '=');
                    if (v4 != -1) {
                        mString v20 = v15.slice(2, v4);

                        mString v21 = v15.slice(v4 + 1, v15.size());

                        if (v20.size() && v21.size()) {
                            auto v7 = atoi(v21.c_str());
                            os_developer_options::instance->set_int(v20, v7);
                        }
                    }

                } else if (strlen(i) > 2 && strnicmp(i, "-s", 2u) == 0) {
                    char v22[260]{};
                    copy_str(v22, i, 260u);

                    char v11 = ' ';
                    char v12 = '#';
                    replace_if_find(v22, &v22[strlen(v22)], v12, v11);
                    mString v14{v22};

                    auto v8 = v14.find({2}, '=');
                    if (v8 != -1) {
                        mString v19 = v14.slice(2, v8);

                        mString v18 = v14.slice(v8 + 1, v14.size());

                        if (v19.size() && v18.size()) {
                            os_developer_options::instance->set_string(v19, v18);
                        }
                    }
                }
            }
        }
    } else {
        CDECL_CALL(0x005948B0, str);
    }
}

void create_sound_ifc(HWND a1) {
    if constexpr (1) {
        static Var<LPDIRECTSOUND8> dword_987518 = {0x00987518};

        auto &v1 = dword_987518();
        if (dword_987518() != nullptr ||
            (DirectSoundCreate8(&IID_IDirectSound8, &dword_987518(), nullptr),
             (v1 = dword_987518()) != nullptr)) {
            v1->lpVtbl->SetCooperativeLevel(v1, a1, DISCL_NONEXCLUSIVE);
        }
    } else {
        CDECL_CALL(0x0081E2D0, a1);
    }
}

void sub_581780() {
    _controlfp(0x300u, 0x300u);
    _controlfp(_PC_24, _MCW_PC);
}

bool sub_81C2A0(unsigned int a1, unsigned int a2, char a3) {
    return (bool) CDECL_CALL(0x0081C2A0, a1, a2, a3);
}

Var<bool> byte_965BF7{0x00965BF7};

void sub_5BCA60(int a1, int a2) {
    if constexpr (1) {
        static Var<int (*)(int, int, int)> dword_9680A0{0x009680A0};

        static Var<int> dword_9682CC{0x009682CC};

        if (dword_9680A0() != nullptr) {
            sp_log("dword_9680A0 = 0x%08X", dword_9680A0());

            dword_9680A0()(a1, a2, dword_9682CC());
        }
    } else {
        CDECL_CALL(0x005BCA60, a1, a2);
    }
}

void sub_5BCA80(int a1) {
    if constexpr (1) {
        static Var<cdecl_call> dword_9680A4{0x009680A4};

        static Var<int> dword_9682D8{0x009682D8};

        if (dword_9680A4() != nullptr) {
            sp_log("dword_9680A4 = 0x%08X", dword_9680A4());

            dword_9680A4()(a1, dword_9682D8());
        }
    } else {
        CDECL_CALL(0x005BCA80, a1);
    }
}

static Var<char[]> byte_88CC68 = {0x0088CC68};

LRESULT __stdcall WindowProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam) {
    if constexpr (1) {
        LRESULT result;

        if (Msg <= WM_KEYUP) {
            if (Msg != WM_KEYUP) {
                switch (Msg) {
                case WM_DESTROY: {
                    ClipCursor(nullptr);
                    PostQuitMessage(0);
                    return 0;
                }
                case WM_CLOSE: {
                    if (byte_965BF7()) {
                        byte_922994() = true;
                        dword_922908() = 2;
                        g_cursor()->sub_5B0D70();
                        g_cursor()->field_120 = true;
                    }

                    return 0;
                }
                case WM_ACTIVATEAPP: {
                    if (byte_965BF9() == (wParam != 0)) {
                        return DefWindowProcA(hWnd, Msg, wParam, lParam);
                    }

                    byte_965BF9() = (wParam != 0);
                    if (os_developer_options::instance != nullptr &&
                        os_developer_options::instance->get_flag(static_cast<os_developer_options::flags_t>(117))) {
                        byte_965BF9() = true;
                    }

                    if (g_game_ptr == nullptr || !byte_965BF9()) {
                        return DefWindowProcA(hWnd, Msg, wParam, lParam);
                    }

                    g_timer()->sub_582180();
                    return DefWindowProcA(hWnd, Msg, wParam, lParam);
                }
                case WM_SETCURSOR: {
                    if (!hCursor()) {
                        return DefWindowProcA(hWnd, Msg, wParam, lParam);
                    }

                    SetCursor(hCursor());
                    return 0;
                }
                case WM_KEYDOWN: {
                    g_cursor()->sub_5B0D70();
                    goto LABEL_5;
                }
                default:
                    return DefWindowProcA(hWnd, Msg, wParam, lParam);
                }

                return DefWindowProcA(hWnd, Msg, wParam, lParam);
            }

            sub_5BCA60(0, byte_88CC68()[static_cast<uint16_t>(wParam)]);

            if (g_femanager.m_pause_menu_system != nullptr && g_femanager.m_pause_menu_system->m_index != -1)
            {
            LABEL_24:

                auto *pause_menu_system = g_femanager.m_pause_menu_system;

                auto *vtbl = bit_cast<fastcall_call(*)[10]>(pause_menu_system->m_vtbl);

                auto *func = (*vtbl)[9];

                func(pause_menu_system, nullptr, 257, wParam, lParam);

                return DefWindowProcA(hWnd, Msg, wParam, lParam);
            }

            if (g_femanager.m_fe_menu_system != nullptr && g_femanager.m_fe_menu_system->m_index != -1)
            {
                if (g_femanager.m_pause_menu_system->m_index == -1)
                {
                    auto *frontend_menu_system = g_femanager.m_fe_menu_system;

                    auto *vtbl = bit_cast<fastcall_call(*)[10]>(frontend_menu_system->m_vtbl);

                    auto *func = (*vtbl)[9];
                    func(frontend_menu_system, nullptr, 257, wParam, lParam);

                    return DefWindowProcA(hWnd, Msg, wParam, lParam);
                }
                goto LABEL_24;
            }

            return DefWindowProcA(hWnd, Msg, wParam, lParam);
        }

        if (Msg > WM_SYSCOMMAND) {
            switch (Msg) {
            case WM_MOUSEMOVE:
            case WM_LBUTTONDOWN:
            case WM_LBUTTONUP:
            case WM_RBUTTONDOWN:
            case WM_RBUTTONUP:
            case WM_MOUSEWHEEL: {
                if (g_femanager.m_pause_menu_system != nullptr && g_femanager.m_pause_menu_system->m_index != -1) {
                    goto LABEL_43;
                }

                if (g_femanager.m_fe_menu_system != nullptr && g_femanager.m_fe_menu_system->m_index != -1)
                {
                    if (g_femanager.m_pause_menu_system->m_index == -1)
                    {
                        auto *frontend_menu_system = g_femanager.m_fe_menu_system;

                        auto *vtbl = bit_cast<fastcall_call(*)[10]>(frontend_menu_system->m_vtbl);

                        auto *func = (*vtbl)[9];
                        //assert(bit_cast<std::intptr_t>(func) == 0x0060B6E0);

                        func(frontend_menu_system, nullptr, Msg, wParam, lParam);

                    } else {
                    LABEL_43:

                        auto *pause_menu_system = g_femanager.m_pause_menu_system;

                        auto *vtbl = bit_cast<fastcall_call(*)[10]>(pause_menu_system->m_vtbl);

                        auto *func = (*vtbl)[9];
                        //assert(bit_cast<std::intptr_t>(func) == 0x0060B6E0);

                        func(pause_menu_system, nullptr, Msg, wParam, lParam);
                    }
                }

                if (g_femanager.IGO != nullptr) {
                    auto v6 = g_femanager.IGO->field_44;
                    if (v6->sub_55F320())
                        v6->sub_638AD0(Msg, wParam, lParam);
                }
                break;
            }
            default:
                return DefWindowProcA(hWnd, Msg, wParam, lParam);
            }
            return DefWindowProcA(hWnd, Msg, wParam, lParam);
        }

        if (Msg == WM_SYSCOMMAND) {
            if ((wParam & 0xFFF0) == SC_KEYMENU || (wParam & 0xFFF0) == SC_SCREENSAVE ||
                (wParam & 0xFFF0) == SC_MONITORPOWER) {
                return 0;
            }

            return DefWindowProcA(hWnd, Msg, wParam, lParam);
        }

        switch (Msg) {
        case WM_CHAR:
            sub_5BCA80(wParam);
            result = DefWindowProcA(hWnd, Msg, wParam, lParam);
            break;
			case WM_SYSKEYDOWN:
    // ALT+ENTER → toggle windowed / fullscreen
    if (wParam == VK_RETURN && (lParam & (1 << 29)))   // 29th bit = ALT key
    {
        g_config.WindowedMode = !g_config.WindowedMode;
        const bool windowed = g_config.WindowedMode;

        g_Windowed() = windowed;
        ToggleFullScreen(windowed);

        return 0;  // handled
    }

LABEL_5:
    sub_5BCA60(1, byte_88CC68()[(uint16_t)wParam]);
    result = DefWindowProcA(hWnd, Msg, wParam, lParam);
    break;
        case WM_SYSKEYUP:
            sub_5BCA60(0, byte_88CC68()[(uint16_t) wParam]);
            result = DefWindowProcA(hWnd, Msg, wParam, lParam);
            break;
        case WM_SYSCHAR:
            if (wParam != 121) {
                return DefWindowProcA(hWnd, Msg, wParam, lParam);
            }
            result = 0;
            break;
        default:
            return DefWindowProcA(hWnd, Msg, wParam, lParam);
        }
        return result;
    } else {
        return (LRESULT) STDCALL(0x005941A0, hWnd, Msg, wParam, lParam);
    }
}

void sub_79DFF0() {
    CDECL_CALL(0x0079DFF0);
}

void sub_81C230() {
    CDECL_CALL(0x0081C230);
}

void sub_81D700() {
    CDECL_CALL(0x0081D700);
}

void sub_81E300() {
    CDECL_CALL(0x0081E300);
}

//0x0081C5A0
void free_file(FileUSM *file) {
    if (file != nullptr) {
        free(file->field_0);
        free(file->field_4);
        free(file);
    }
}

void sub_4DDEC0() {
    CDECL_CALL(0x004DDEC0);
}

void bink_set_sound_system() {
    CDECL_CALL(0x0060BBA0);
    //BinkSetSoundSystem(BinkOpenDirectSound, pUnkOuter());
}



bool get_path(const char *a1, const char *a2, char *out, unsigned int str_len) {
    if constexpr (1) {
        char Buffer[260]{};

        char v12[260]{};

        char v8[260]{};

        if (GetSystemDirectoryA(Buffer, 260u)) {
            Buffer[259] = '\0';

            char Dest[268];
            std::sprintf(Dest, "%s\\shell32.dll", Buffer);
            auto hModule = LoadLibraryA(Dest);

            if (hModule != nullptr) {
                auto *SHGetFolderPathA =
                    bit_cast<HRESULT(__stdcall *)(HWND, int, HANDLE, DWORD, LPSTR)>(
                        GetProcAddress(hModule, "SHGetFolderPathA"));
                if (SHGetFolderPathA != nullptr &&
                    SHGetFolderPathA(nullptr, 5, nullptr, 0, v12) >= 0) {
                    sprintf(v8, "%s\\%s", v12, a1);
                    if (strlen(v8) >= str_len) {
                        return false;
                    }

                    strcpy(out, v8);
                }

                FreeLibrary(hModule);
            }
        }

        if (&v12[strlen(v12) + 1] == &v12[1]) {
            if (strlen(a2) >= str_len) {
                return false;
            }

            strcpy(out, a2);
        }

        return true;
    } else {
        return (bool) CDECL_CALL(0x0081BE00, a1, a2, out, str_len);
    }
}

bool sub_586FA0(unsigned char a1) {
    bool result;

    switch (a1) {
    case 224:
    case 228:
    case 231:
    case 232:
    case 233:
    case 236:
    case 241:
    case 242:
    case 246:
    case 249:
    case 252:
        result = true;
        break;
    default:
        result = false;
        break;
    }

    return result;
}

void sub_5952D0()
{
    TRACE("sub_5952D0");

    if constexpr (0)
    {
        operator delete(dword_965C24()[0]);
        operator delete(dword_965C24()[1]);
        operator delete(dword_965C24()[2]);
        operator delete(dword_965C24()[3]);
        operator delete(dword_965C24()[4]);
        operator delete(dword_965C24()[5]);
        operator delete(dword_965C24()[6]);
        operator delete(dword_965C24()[7]);
        operator delete(dword_965C24()[8]);
        operator delete(dword_965C24()[9]);
        operator delete(dword_965C24()[10]);
        operator delete(dword_965C24()[11]);
        operator delete(dword_965C24()[12]);
        operator delete(dword_965C24()[13]);

        Input::instance()->sub_821490(true);

        struct {
            char field_0[16];
            int field_10[8];
            int field_30;
            bool field_34;
            int field_38;
            bool field_3C;
            int field_40;
            int field_44;
        } v29;

        VALIDATE_SIZE(v29, 0x48);

        static constexpr decltype(v29) input_value[18] = {
            {"Forward",      {0, 0, 0, 0, 0, 0, 0, 0},  65553, false, 65736, false, 196612, 0},
            {"Backward",     {0, 0, 0, 0, 0, 0, 0, 0},  65567, false, 65744, false, 196611, 0},
            {"TurnLeft",     {0, 0, 0, 0, 0, 0, 0, 0},  65566, false, 65739, false, 196610, 0},
            {"TurnRight",    {0, 0, 0, 0, 0, 0, 0, 0},  65568, false, 65741, false, 196609, 0},
            {"Jump",         {0, 0, 0, 0, 0, 0, 0, 0}, 0x10000 + DIK_SPACE, false, 65535, false, 196629, 0},
            {"StickToWalls", {0, 0, 0, 0, 0, 0, 0, 0},  65569, false, 65578, false, 196630, 0},
            {"Punch",        {0, 0, 0, 0, 0, 0, 0, 0}, 0x10000 + DIK_Q, false, 65565, false, 196631, 0},
            {"Kick",         {0, 0, 0, 0, 0, 0, 0, 0}, 0x10000 + DIK_E, false, 65592, false, 196632, 0},
            {"ThrowWeb",     {0, 0, 0, 0, 0, 0, 0, 0}, 131082, false, 65582, false,  65535, 0},
            {"BlackButton",  {0, 0, 0, 0, 0, 0, 0, 0}, 131081, false, 65583, false,  65535, 0},
            {"CameraUp",     {0, 0, 0, 0, 0, 0, 0, 0},  65608, false, 65737, false,  65535, 0},
            {"CameraDown",   {0, 0, 0, 0, 0, 0, 0, 0},  65616, false, 65745, false,  65535, 0},
            {"CameraLeft",   {0, 0, 0, 0, 0, 0, 0, 0},  65611, false, 65535, false,  65535, 0},
            {"CameraRight",  {0, 0, 0, 0, 0, 0, 0, 0},  65613, false, 65535, false,  65535, 0},
            {"CameraCenter", {0, 0, 0, 0, 0, 0, 0, 0}, 0x10000 + 48, false, 65535, false,  65535, 0},
            {"Pause",        {0, 0, 0, 0, 0, 0, 0, 0},  65537, true,  65535, false,  65535, 0},
            {"BackButton",   {0, 0, 0, 0, 0, 0, 0, 0},  65586, false, 65535, false,  65535, 0},
            {"ScreenShot",   {0, 0, 0, 0, 0, 0, 0, 0},  65623, false, 65535, false,  65535, 0}
        };

        //0x00922940
        static constexpr InputAction input_actions[18] = {
                       InputAction::Forward,
                       InputAction::Backward,
                       InputAction::TurnLeft,
                       InputAction::TurnRight,
                       InputAction::Jump,
                       InputAction::StickToWalls,
                       InputAction::Punch,
                       InputAction::Kick,
                       InputAction::ThrowWeb,
                       InputAction::BlackButton,
                       InputAction::CameraUp,
                       InputAction::CameraDown,
                       InputAction::CameraLeft,
                       InputAction::CameraRight,
                       InputAction::CameraCenter,
                       InputAction::Pause,
                       InputAction::BackButton,
                       InputAction::ScreenShot
        };

        for (auto i = 0u; i < std::size(input_actions); ++i)
        {
            auto &v22 = input_actions[i];

            v29 = input_value[i];

            char Dest[264];
            sprintf(Dest, "Controls\\Player%d\\%s1", 1, v29.field_0);

            uint32_t v0 = ( v29.field_34 
                            ? v29.field_30
                            : g_settings()->sub_81D010(Dest, v29.field_30)
                            );

            const uint16_t v23 = v0;

            sprintf(Dest, "Controls\\Player%d\\%s2", 1, v29.field_0);
            uint32_t v1 = ( v29.field_3C
                            ? v29.field_38
                            : g_settings()->sub_81D010(Dest, v29.field_38));

            const uint16_t v27 = v1;

            sprintf(Dest, "Controls\\Player%d\\%s3", 1, v29.field_0);
            uint32_t v2 = g_settings()->sub_81D010(Dest, 0);
            if (v2 == 0 && Input::instance()->sub_820080() > 0) {
                v2 = v29.field_40;
            }

            const uint16_t v28 = v2;

            InputType v4 = static_cast<InputType>(HIWORD(v0));
            sp_log("%d, %d, %d", v4, v0, v23);

            g_inputSettingsInGame()->field_18.set(v22, 0, v4, v23);

            InputType v5 = static_cast<InputType>(HIWORD(v1));
            g_inputSettingsInGame()->field_18.set(v22, 1u, v5, v27);

            InputType v6 = static_cast<InputType>(HIWORD(v2));
            g_inputSettingsInGame()->field_18.set(v22, 2u, v6, v28);

            if (v22 == InputAction::Pause)
            {
                g_inputSettingsMenu()->field_18.set(InputAction::Kick, 0, v4, v23);
                g_inputSettingsMenu()->field_18.set(InputAction::Kick, 1u, v5, v27);
                g_inputSettingsMenu()->field_18.set(InputAction::Kick, 2u, v6, v28);
            }

            if (v22 == InputAction::Forward && v4 == InputType::Key && v23 != 200)
            {
                g_inputSettingsMenu()->field_18.set_key(v22, 1u, v23);
            }
            else
            {
                uint16_t v8;
                if (v22 == InputAction::Backward && v4 == InputType::Key)
                {
                    v8 = v23;
                    if (v23 != 208) {
                        g_inputSettingsMenu()->field_18.set_key(v22, 1u, v23);
                    }

                } else {
                    v8 = v23;
                }

                if ((v22 == InputAction::TurnLeft && v4 == InputType::Key && v8 != 203) ||
                    (v22 == InputAction::TurnRight && v4 == InputType::Key && v8 != 205)) {
                    g_inputSettingsMenu()->field_18.set_key(v22, 1u, v23);
                }
            }

            if (v22 == InputAction::Pause)
            {
                if (v4 == InputType::Joy && v23 != 21 && v23 != 24) {
                    g_inputSettingsMenu()->field_18.set(InputAction::Pause, 1u, InputType::Joy, v23);
                }

                if (v22 == InputAction::Pause)
                {
                    if (v5 == InputType::Joy && v23 != 21 && v27 != 24) {
                        g_inputSettingsMenu()->field_18.set(InputAction::Pause,
                                                            1u,
                                                            InputType::Joy,
                                                            v27);
                    }

                    if (v22 == InputAction::Pause
                            && v6 == InputType::Joy
                            && v23 != 21
                            && v28 != 24)
                    {
                        g_inputSettingsMenu()->field_18.set(InputAction::Pause,
                                                            1u,
                                                            InputType::Joy,
                                                            v28);
                    }
                }
            }

            const char *v10;
            auto *v9 = Input::instance()->get_string(v4, v23);
            if (v9 != nullptr || (v9 = Input::instance()->get_string(v5, v27)) != nullptr ||
                (v9 = Input::instance()->get_string(v6, v28)) != nullptr) {
                v10 = v9;
            } else {
                v10 = get_msg(g_fileUSM(), "NONE");
            }

            auto *v12 = static_cast<char *>(operator new(255u));

            if (v10 != nullptr) {
                v9 = v10;
            }

            sprintf(v12, "\"%s\"", v9);
            if (strlen(v12) == 3 && sub_586FA0(v12[1])) {
                auto v13 = v12[1] - ' ';

                v12[1] = v13;

            }
            else if (strlen(v12) == 3)
            {
                unsigned char v14 = v12[1];
                if (v14 > '`' && v14 < '{')
                {
                    auto v13 = toupper(v14);
                    v12[1] = v13;
                }
            }

            switch (v22) {
            case InputAction::Jump:
                dword_965C24()[GamepadInput::Cross] = v12;
                break;
            case InputAction::StickToWalls:
                dword_965C24()[GamepadInput::Circle] = v12;
                break;
            case InputAction::Punch:
                dword_965C24()[GamepadInput::Square] = v12;
                break;
            case InputAction::Kick:
                dword_965C24()[GamepadInput::Triangle] = v12;
                break;
            case InputAction::BlackButton:
                dword_965C24()[GamepadInput::L2] = v12;
                break;
            case InputAction::ThrowWeb:
                dword_965C24()[GamepadInput::R2] = v12;
                break;
            case InputAction::Pause:
                dword_965C24()[GamepadInput::Start] = v12;
                break;
            case InputAction::BackButton:
                dword_965C24()[GamepadInput::Select] = v12;
                break;
            case InputAction::CameraCenter:
                dword_965C24()[GamepadInput::R3] = v12;
                break;
            case InputAction::Forward:
                dword_965C24()[GamepadInput::Forward] = v12;
                break;
            case InputAction::TurnLeft:
                dword_965C24()[GamepadInput::Left] = v12;
                break;
            case InputAction::TurnRight:
                dword_965C24()[GamepadInput::Right] = v12;
                break;
            default:
                break;
            }
        }

        sub_5828B0();
        g_inputSettingsMenu()->field_18.set_key(InputAction::Jump, 0, DIK_RETURN);
        g_inputSettingsMenu()->field_18.set_key(InputAction::Kick, 0, DIK_ESCAPE);
        g_inputSettingsMenu()->field_18.set_key(InputAction::Pause, 0, DIK_SPACE);
        g_inputSettingsMenu()->field_18.set_key(InputAction::Forward, 0, DIK_UP);
        g_inputSettingsMenu()->field_18.set_key(InputAction::Backward, 0, DIK_DOWN);
        g_inputSettingsMenu()->field_18.set_key(InputAction::TurnLeft, 0, DIK_LEFT);
        g_inputSettingsMenu()->field_18.set_key(InputAction::TurnRight, 0, DIK_RIGHT);

        *g_inputSettings2() = *g_inputSettingsInGame();
        g_inputSettings2()->field_18.find_and_clear(InputType::Mouse, 9);
        g_inputSettings2()->field_18.find_and_clear(InputType::Mouse, 10);

        *g_inputSettings4() = *g_inputSettingsInGame();
        g_inputSettings4()->field_18.find_and_clear(InputType::Key);
        g_inputSettings4()->field_18.set_key(InputAction::Jump, 3u, DIK_RETURN);
        g_inputSettings4()->field_18.set_key(InputAction::Kick, 3u, DIK_ESCAPE);
        g_inputSettings4()->field_18.set_key(InputAction::Pause, 3u, DIK_SPACE);
        g_inputSettings4()->field_18.set_key(InputAction::Forward, 3u, DIK_UP);
        g_inputSettings4()->field_18.set_key(InputAction::Backward, 3u, DIK_DOWN);
        g_inputSettings4()->field_18.set_key(InputAction::TurnLeft, 3u, DIK_LEFT);
        g_inputSettings4()->field_18.set_key(InputAction::TurnRight, 3u, DIK_RIGHT);
        g_inputSettings4()->field_18.find_and_clear(InputType::Mouse, 9);
        g_inputSettings4()->field_18.find_and_clear(InputType::Mouse, 10);
    } 
    else
    {
        CDECL_CALL(0x005952D0);
    }
}


	static Var<int>  dword_96B430 = {0x0096B430};

int __stdcall myWinMain(HINSTANCE hInstance,
                        [[maybe_unused]] HINSTANCE hPrevInstance,
                        LPSTR lpCmdLine,
                        [[maybe_unused]] int nShowCmd) {
    if (!CreateMutexA(nullptr, true, "USM") || GetLastError() == ERROR_ALREADY_EXISTS) {
        return 0;
    }

    g_settings() = new Settings{"Activision", "Ultimate Spider-Man"};

    char v6;
    switch (g_settings()->sub_81D010("Settings\\Language", 0)) {
    case 1:
        v6 = 'f';
        break;
    case 2:
        v6 = 'g';
        break;
    case 3:
        v6 = 's';
        break;
    case 4:
        v6 = 'i';
        break;
    default:
        v6 = 'e';
        break;
    }

    char Dest[100];
    sprintf(Dest, "data\\usm_lt%c.usm", v6);

    g_fileUSM() = create_usm_file(Dest, nullptr);

    static Var<CHAR[]> DirectoryName{0x0088FAF8};
    ULARGE_INTEGER TotalNumberOfFreeBytes;
    GetDiskFreeSpaceExA(DirectoryName(), nullptr, nullptr, &TotalNumberOfFreeBytes);
    if (TotalNumberOfFreeBytes.QuadPart < 0xA00000) {
        auto *v162 = get_msg(g_fileUSM(), "MSGBOX_ERROR");
        auto *v7 = get_msg(g_fileUSM(), "MSGBOX_SPACE");

        MessageBoxA(nullptr, v7, v162, 0x10u);
        return 0;
    }

    if (!sub_81C2A0(9u, 0, 99u)) {
        auto *v162 = get_msg(g_fileUSM(), "MSGBOX_ERROR");
        auto *v7 = get_msg(g_fileUSM(), "MSGBOX_DX9");

        MessageBoxA(nullptr, v7, v162, 0x10u);
        return 0;
    }

    static Var<char> byte_965C08 = {0x00965C08};
    byte_965C08() = 1;

    init_assert_handler();

#if 0
    _controlfp(1u, 0x8001Fu);
    sub_581780();
#endif

    static Var<int> dword_965BFC = {0x00965BFC};
    dword_965BFC() = (int) hInstance;

    create_window_handle();

    os_developer_options::os_developer_init();
    ini_parser::parse("game.ini", os_developer_options::instance);

    sub_5C9EA0();

    g_timer() = new Timer{30.0, 30.0};

    sub_81C1A0();

    sub_81E130(nullptr);

    Input::create_inst();

    char Str[12];
    g_settings()->sub_81CFA0("Settings\\Resolution", "800x600", Str, 10u);

    char *v11 = strtok(Str, "x");
    g_cx() = atoi(v11);
    char *v12 = strtok(nullptr, "x");
    g_cy() = atoi(v12);

    flt_965BDC() = (double) g_cy() * 0.011029412 - 5.29;

    byte_95C718() = g_settings()->sub_81D050("Settings\\DistanceClipping", 0);
    dword_95C2F8() = g_settings()->sub_81D010("Settings\\Distance", 50);
    g_player_shadows_enabled() = g_settings()->sub_81D050("Settings\\DetailedShadows", 1);
    g_enable_stencil_shadows() = g_player_shadows_enabled();
    ChromeEffect() = g_settings()->sub_81D050("Settings\\ChromeEffect", 1);

    register_class_and_create_window("Render Window",
                                     "Ultimate Spider-Man",
                                     0,
                                     0,
                                     g_cx(),
                                     g_cy(),
                                     WindowProc,
                                     hInstance,
                                     80,
                                     1u);

    ShowWindow(g_appHwnd(), 3);

    g_Windowed() = g_config.WindowedMode;   // honor -windowed; was hard-coded to 0
    UpdateWindow(g_appHwnd());
    SetWindowPos(g_appHwnd(), nullptr, 0, 0, g_cx(), g_cy(), 4u);

    create_sound_ifc(g_appHwnd());
    ShowCursor(0);
    os_developer_options::instance->set_int(mString {"ALLOW_SCREENSHOT"}, 1);

    window_manager::instance()->field_4 = g_appHwnd();

    parse_cmd(lpCmdLine);

    if (os_developer_options::instance->get_flag(mString {"HALT_ON_ASSERTS"})) {
        g_debug().field_1 |= 1;
    } else {
        g_debug().field_1 &= 0xFE;
    }

    if (g_is_the_packer() || !os_developer_options::instance->get_flag(mString {"SCREEN_ASSERTS"})) {
        g_debug().field_1 &= 0xFD;
    } else {
        g_debug().field_1 |= 2;
    }

    nflInitParams a1;
    a1.field_0[0] = 32;
    a1.field_0[1] = 32;
    a1.field_0[2] = 32;
    a1.field_0[0] = 1024;
    a1.field_0[3] = 3;
    a1.field_0[4] = 0;

    static Var<size_t> dword_965C04 = {0x00965C04};
    dword_965C04() = nflInit(&a1);

    static Var<char *> dword_965C00 = {0x00965C00};
    dword_965C00() = new char[dword_965C04()];

    //EnableLog l{};

    nflStart(dword_965C00());

    int v17 = os_developer_options::instance->get_int(mString {"RANDOM_SEED"});
    if (v17) {
        srand(v17);
    } else {
        auto v18 = time(nullptr);
        srand(v18);
    }

    char v173[260];

    sprintf(v173, "%s\\%s\\Screenshot", "Activision", "Ultimate Spider-Man");
    get_path(v173, "Screenshot", byte_9659B8(), 260u);
    create_directory(byte_9659B8());

    char v174[260];
    sprintf(v174, "%s\\%s\\Save", "Activision", "Ultimate Spider-Man");

    static Var<char[260]> byte_965AD0 = {0x00965AD0};
    get_path(v174, "Save", byte_965AD0(), 260u);
    create_directory(byte_965AD0());

    Input::instance()->field_129D0 = 1;

    g_inputSettingsMenu() = new InputSettings{};
    g_inputSettingsInGame() = new InputSettings{};
    g_inputSettings2() = new InputSettings{};
    g_inputSettings3() = new InputSettings{};
    g_inputSettings4() = new InputSettings{};

    g_inputSettingsInGame()->field_18.sub_821E70();

    char Source[260];
    char Type[40];

    for (int i{0}; i < 10; ++i) {
        sprintf(Source, "Controls\\Gamepads\\PadID%d", i + 1);
        g_settings()->sub_81CFA0(Source, "00000000-0000-0000-0000-000000000000", Type, 37u);

        Input::instance()->sub_81FC00(i, Type);
    }

    Input::instance()->initialize(g_appHwnd());

    auto *v30 = get_msg(g_fileUSM(), "MouseWheelDown");
    auto *v31 = get_msg(g_fileUSM(), "MouseWheelUp");
    auto *v32 = get_msg(g_fileUSM(), "MouseAxis");
    auto *v33 = get_msg(g_fileUSM(), "MouseBtn");
    auto *v34 = get_msg(g_fileUSM(), "MouseMiddle");
    auto *v35 = get_msg(g_fileUSM(), "MouseRight");
    auto *v36 = get_msg(g_fileUSM(), "MouseLeft");

    Input::instance()->set_mouse(v36, v35, v34, v33, v32, v31, v30);

    auto *v38 = get_msg(g_fileUSM(), "GamepadBtn");
    auto *v39 = get_msg(g_fileUSM(), "GamepadPoV");
    auto *v40 = get_msg(g_fileUSM(), "GamepadAxis");
    Input::instance()->set_gamepad(v40, v39, v38);

    auto *v42 = get_msg(g_fileUSM(), "ESC");

    Input::instance()->set_key(1, v42);

    auto *v44 = get_msg(g_fileUSM(), "BACK");
    Input::instance()->set_key(14, v44);

    auto *v46 = get_msg(g_fileUSM(), "TAB");
    Input::instance()->set_key(15, v46);

    auto *v48 = get_msg(g_fileUSM(), "ENTER");
    Input::instance()->set_key(28, v48);

    auto *v50 = get_msg(g_fileUSM(), "RCTRL");
    Input::instance()->set_key(157, v50);

    auto *v52 = get_msg(g_fileUSM(), "LCTRL");
    Input::instance()->set_key(29, v52);

    auto *v54 = get_msg(g_fileUSM(), "LSHIFT");
    Input::instance()->set_key(42, v54);

    auto *v56 = get_msg(g_fileUSM(), "RSHIFT");
    Input::instance()->set_key(54, v56);

    auto *v58 = get_msg(g_fileUSM(), "SPACE");
    Input::instance()->set_key(57, v58);

    auto *v60 = get_msg(g_fileUSM(), "CAPSL");
    Input::instance()->set_key(58, v60);

    auto *v62 = get_msg(g_fileUSM(), "SCROLL");
    Input::instance()->set_key(70, v62);

    auto *v64 = get_msg(g_fileUSM(), "LALT");
    Input::instance()->set_key(56, v64);

    auto *v66 = get_msg(g_fileUSM(), "RALT");
    Input::instance()->set_key(184, v66);

    auto *v68 = get_msg(g_fileUSM(), "PAUSE");
    Input::instance()->set_key(197, v68);

    auto *v70 = get_msg(g_fileUSM(), "HOME");
    Input::instance()->set_key(199, v70);

    auto *v72 = get_msg(g_fileUSM(), "PGUP");
    Input::instance()->set_key(201, v72);

    auto *v74 = get_msg(g_fileUSM(), "UP");
    Input::instance()->set_key(200, v74);

    auto *v76 = get_msg(g_fileUSM(), "LEFT");
    Input::instance()->set_key(203, v76);

    auto *v78 = get_msg(g_fileUSM(), "DOWN");
    Input::instance()->set_key(208, v78);

    auto *v80 = get_msg(g_fileUSM(), "RIGHT");
    Input::instance()->set_key(205, v80);

    auto *v82 = get_msg(g_fileUSM(), "DEL");
    Input::instance()->set_key(211, v82);

    auto *v84 = get_msg(g_fileUSM(), "INS");
    Input::instance()->set_key(210, v84);

    auto *v86 = get_msg(g_fileUSM(), "END");
    Input::instance()->set_key(207, v86);

    auto *v88 = get_msg(g_fileUSM(), "PGDWN");
    Input::instance()->set_key(209, v88);

    auto *v90 = get_msg(g_fileUSM(), "MENU");
    Input::instance()->set_key(221, v90);

    auto *v92 = get_msg(g_fileUSM(), "KP,");
    Input::instance()->set_key(179, v92);

    auto *v94 = get_msg(g_fileUSM(), "KP.");
    Input::instance()->set_key(83, v94);

    auto *v96 = get_msg(g_fileUSM(), "KPEnter");
    Input::instance()->set_key(156, v96);

    auto *v98 = get_msg(g_fileUSM(), "NumL");
    Input::instance()->set_key(69, v98);

    auto *v100 = get_msg(g_fileUSM(), "KP0");
    Input::instance()->set_key(82, v100);

    auto *v102 = get_msg(g_fileUSM(), "KP1");
    Input::instance()->set_key(79, v102);

    auto *v104 = get_msg(g_fileUSM(), "KP2");
    Input::instance()->set_key(80, v104);

    auto *v106 = get_msg(g_fileUSM(), "KP3");
    Input::instance()->set_key(81, v106);

    auto *v108 = get_msg(g_fileUSM(), "KP4");
    Input::instance()->set_key(75, v108);

    auto *v110 = get_msg(g_fileUSM(), "KP5");
    Input::instance()->set_key(76, v110);

    auto *v112 = get_msg(g_fileUSM(), "KP6");
    Input::instance()->set_key(77, v112);

    auto *v114 = get_msg(g_fileUSM(), "KP7");
    Input::instance()->set_key(71, v114);

    auto *v116 = get_msg(g_fileUSM(), "KP8");
    Input::instance()->set_key(72, v116);

    auto *v118 = get_msg(g_fileUSM(), "KP9");
    Input::instance()->set_key(73, v118);

    auto *v120 = get_msg(g_fileUSM(), "F1");
    Input::instance()->set_key(59, v120);

    auto *v122 = get_msg(g_fileUSM(), "F2");
    Input::instance()->set_key(60, v122);

    auto *v124 = get_msg(g_fileUSM(), "F3");
    Input::instance()->set_key(61, v124);

    auto *v126 = get_msg(g_fileUSM(), "F4");
    Input::instance()->set_key(62, v126);

    auto *v128 = get_msg(g_fileUSM(), "F5");
    Input::instance()->set_key(63, v128);

    auto *v130 = get_msg(g_fileUSM(), "F6");
    Input::instance()->set_key(64, v130);

    auto *v132 = get_msg(g_fileUSM(), "F7");
    Input::instance()->set_key(65, v132);

    auto *v134 = get_msg(g_fileUSM(), "F8");
    Input::instance()->set_key(66, v134);

    auto *v136 = get_msg(g_fileUSM(), "F9");
    Input::instance()->set_key(67, v136);

    auto *v138 = get_msg(g_fileUSM(), "F10");
    Input::instance()->set_key(68, v138);

    auto *v140 = get_msg(g_fileUSM(), "F11");
    Input::instance()->set_key(87, v140);

    auto *v142 = get_msg(g_fileUSM(), "F12");
    Input::instance()->set_key(88, v142);

    auto *v144 = get_msg(g_fileUSM(), "F13");
    Input::instance()->set_key(100, v144);

    auto *v146 = get_msg(g_fileUSM(), "F14");
    Input::instance()->set_key(101, v146);

    auto *v148 = get_msg(g_fileUSM(), "F15");
    Input::instance()->set_key(102, v148);

    sub_5952D0();

    static Var<RTL_CRITICAL_SECTION> CriticalSection = {0x009618F4};
    InitializeCriticalSection(&CriticalSection());

    Input::instance()->sub_8203F0(0, g_inputSettingsMenu());
    Input::instance()->sub_81FB90(1);

    Settings::MouseLook() = g_settings()->sub_81D010("Settings\\MouseLook", 1) != 0;
    if (Settings::MouseLook()) {
        Settings::InvertCameraH() = g_settings()->sub_81D050("Settings\\InvertCameraH", 0);

        g_inputSettingsInGame()->field_18.set_mouse(InputAction::CameraRight,
                                                    3,
                                                    InputMouse::LookRight);
        g_inputSettingsInGame()->field_18.set_mouse(InputAction::CameraLeft,
                                                    3,
                                                    InputMouse::LookLeft);

        Settings::InvertCameraV() = g_settings()->sub_81D050("Settings\\InvertCameraV", 0);
        g_inputSettingsInGame()->field_18.set_mouse(InputAction::CameraUp, 3, InputMouse::LookUp);
        g_inputSettingsInGame()->field_18.set_mouse(InputAction::CameraDown,
                                                    3,
                                                    InputMouse::LookDown);
    }

    Input::instance()->m_sensitivity = g_settings()->sub_81D010("Settings\\Sensitivity", 50) *
            0.001f +
        0.001f;

    Settings::SoundMode() = g_settings()->sub_81D010("Settings\\SoundMode", 2);
    Settings::GameSoundVolume() = g_settings()->sub_81D010("Settings\\GameSoundVolume",
                                                                    10) *
        0.1;
    Settings::MusicVolume() = g_settings()->sub_81D010("Settings\\MusicVolume", 10) * 0.1f;

    if (os_developer_options::instance->get_flag(mString {"EXCEPTION_HANDLER"})) {
        SetUnhandledExceptionFilter(TopLevelExceptionFilter);
    }

    ALLOW_ERROR_POPUPS() = os_developer_options::instance->get_flag(mString {"ALLOW_ERROR_POPUPS"});
    if (!ALLOW_ERROR_POPUPS()) {
        SetErrorMode(2u);
    }

    static tlSystemCallbacks ngl_callbacks{ngl_readfile_callback,
                                           ngl_releasefile_callback,
                                           0,
                                           0,
                                           ngl_memalloc_callback,
                                           ngl_memfree_callback

    };

    tlSetSystemCallbacks(ngl_callbacks);

    nWidth() = g_cx();
    nHeight() = g_cy();

    nglInit(g_appHwnd());
    nalInit(nullptr);

    g_cursor() = new Cursor(L"data\\ump.dat", g_cx(), g_cy());
    set_tl_system_directories();

    static Var<nglFrameLockType> g_frame_lock{0x00922920};
    nglSetFrameLock(g_frame_lock());

    auto list_buffer = os_developer_options::instance->get_int(mString{"PCLISTBUFFER"});
    nglSetBufferSize(static_cast<nglBufferType>(0), list_buffer << 10, true);
    auto scratch_buffer = os_developer_options::instance->get_int(mString{"PCSCRATCHBUFFER"});
    nglSetBufferSize(static_cast<nglBufferType>(1), scratch_buffer << 10, true);
    auto v154 = os_developer_options::instance->get_int(mString{"PCSCRATCHINDEXBUFFER"});
    nglSetBufferSize(static_cast<nglBufferType>(2), 2 * v154, true);
    auto v155 = os_developer_options::instance->get_int(mString{"PCSCRATCHVERTEXBUFFER"});
    nglSetBufferSize(static_cast<nglBufferType>(3), v155 << 10, true);

    aeps::Init();

    //application_startup
    {
        entity_handle_manager::create_inst();
        app::create_inst();
        bink_set_sound_system();
        cut_scene::init_stream_scene_anims();
    }

    if (g_is_the_packer()) {
        //sub_748E10();
    } else {
        if (!g_master_clock_is_up()) {
            timeBeginPeriod(1u);
        }

        auto v163 = timeGetTime();
        auto v165 = (double) os_developer_options::instance->get_int(mString{"RUN_LENGTH"});
        auto v164 = (os_developer_options::instance->get_int(mString{"RUN_LENGTH"}) != -1);

        g_timer()->sub_582180();

        g_game_ptr->gamefile->field_340.m_invert_camera_horz = Settings::InvertCameraH();
        g_game_ptr->gamefile->field_340.m_invert_camera_vert = Settings::InvertCameraV();
        auto *rumble_ptr = input_mgr::instance->rumble_ptr;
        if (rumble_ptr != nullptr) {
            rumble_ptr->disable_vibration();
        }

        if (!bExit()) {

            while (1) {
                MSG Msg;

                auto res = PeekMessageA(&Msg, nullptr, 0, 0, 1u);

                while (res != 0) {
                    if (Msg.message == WM_QUIT) {
                        bExit() = true;
                    }

                    TranslateMessage(&Msg);
                    DispatchMessageA(&Msg);
                    res = PeekMessageA(&Msg, nullptr, 0, 0, 1u);
                }

                if (v164 && v165 < 0.0) {
                    break;
                }

                if (bExit()) {
                    goto LABEL_94;
                }

                if (byte_965BF9()) {
                    if (!g_master_clock_is_up()) {
                        timeBeginPeriod(1u);
                    }

                    DWORD v158 = timeGetTime();
                    DWORD v159 = timeGetTime();
                    auto v160 = (double) (v159 - v163);
                    v163 = v159;
                    v165 = v165 - v160 * 0.001;
                    app::instance->tick();
                    DWORD v168 = timeGetTime() - v158;
                    app::instance->m_game->field_278 = v168 * 0.001f;

                    if (g_inputSettingsInGame()->field_18.get_state(InputAction::ScreenShot) <=
                        0.0)
                    {
                        if (!byte_965BF5() && byte_965BF6()) {
                            byte_965BF5() = true;
                            byte_965BF6() = false;
                        }

                    } else {
                        byte_965BF6() = true;
                    }

                    if (dword_922908()) {
                        if (dword_922908() >= 0 && !byte_965C21()) {
                            --dword_922908();
                        }
                    } else if (!byte_965C21()) {
                        if (byte_965BF8()) {
                            ClipCursor(nullptr);
                            break;
                        }

                        if (sub_5A3AA0("CONFIRMQUIT_MSG", nullptr)) {
                            ClipCursor(nullptr);
                            bExit() = true;
                            dword_922908() = -1;
                        } else {
                            g_cursor()->field_120 = 0;
                            if (!g_cursor()->field_120) {
                                g_cursor()->field_114 = 1;
                            }

                            dword_922908() = -1;
                        }
                    }

                    if (bExit()) {
                        goto LABEL_94;
                    }
                }
            }

            bExit() = true;
        }
    }
LABEL_94:

    if (app::instance != nullptr) {
        auto *vtbl = bit_cast<int(*)[1]>(app::instance->m_vtbl);

        assert((*vtbl)[0] == 0x005E99D0);

        delete app::instance;

        app::instance = nullptr;
    }

    entity_handle_manager::delete_inst();

    sub_732D60(1);

    sub_4DDEC0();

    if (g_cursor() != nullptr) {
        auto *vtbl = bit_cast<int(*)[1]>(g_cursor()->m_vtbl);

        assert((*vtbl)[0] == 0x005B7BC0);

        delete g_cursor();
    }

    sub_76DF40();
    nalExit();
    sub_77B2F0(1);

    if (g_timer() != nullptr) {
        operator delete(g_timer());
    }

    free_file(g_fileUSM());

    sub_81E300();

    if (Input::instance() != nullptr) {
        Input::instance()->sub_820C60();
        operator delete(Input::instance());
        Input::instance() = nullptr;
    }

    sub_81E300();
    sub_81D700();

    delete g_inputSettingsMenu();
    delete g_inputSettingsInGame();
    delete g_inputSettings2();
    delete g_inputSettings3();
    delete g_inputSettings4();
    delete g_settings();

    sub_81C230();

    DestroyWindow(window_manager::instance()->field_4);

    if (window_manager::instance() != nullptr) {
        delete window_manager::instance();
        window_manager::instance() = nullptr;
    }

    if (os_developer_options::instance != nullptr) {
        delete os_developer_options::instance;
        os_developer_options::instance = nullptr;
    }

    sub_79DFF0();

    delete[] dword_965C00();

    return 0;
}

void redirect_winmain() {
    REDIRECT(0x00822556, myWinMain);
}

HANDLE __stdcall HookCreateFileA(LPCSTR lpFileName,
                                 DWORD dwDesiredAccess,
                                 DWORD dwShareMode,
                                 LPSECURITY_ATTRIBUTES lpSecurityAttributes,
                                 DWORD dwCreationDisposition,
                                 DWORD dwFlagsAndAttributes,
                                 HANDLE hTemplateFile) {
    //sp_log("HookCreateFileA %s, return_address = 0x%08X", lpFileName, getReturnAddress);

    return CreateFileA(lpFileName,
                       dwDesiredAccess,
                       dwShareMode,
                       lpSecurityAttributes,
                       dwCreationDisposition,
                       dwFlagsAndAttributes,
                       hTemplateFile);
}

BOOL __stdcall HookReadFile(HANDLE hFile,
                            LPVOID lpBuffer,
                            DWORD nNumberOfBytesToRead,
                            LPDWORD lpNumberOfBytesRead,
                            LPOVERLAPPED lpOverlapped) {
    //sp_log("HookReadFile 0x%08X, return_address = 0x%08X", hFile, getReturnAddress);

    return ReadFile(hFile, lpBuffer, nNumberOfBytesToRead, lpNumberOfBytesRead, lpOverlapped);
}

//0x0081BFB0
void register_class(LPCSTR lpClassName, WNDPROC windowProc, HINSTANCE hInstance, int a4) {
    static ATOM g_wndClass{};

    if (g_wndClass) {
        UnregisterClassA(lpClassName, hInstance);
    }

#if 0
    WNDCLASSEXA desc;
    memset(&desc, 0, sizeof(desc));
#else
    WNDCLASSEXA desc{};
#endif

    desc.lpfnWndProc = windowProc;
    desc.cbSize = 48;
    desc.style = 64;
    desc.hInstance = hInstance;
    desc.lpszClassName = lpClassName;
    if (a4) {
        desc.hIcon = LoadIconA(hInstance, MAKEINTRESOURCE(a4));
    }

    g_wndClass = RegisterClassExA(&desc);
}

//0x0081C030
void create_window(LPCSTR lpClassName,
                   LPCSTR lpWindowName,
                   HINSTANCE hInstance,
                   DWORD dwExStyle,
                   int p_y,
                   int a6,
                   int a7,
                   DWORD dwStyle) {
					   
    int w;
    int h;
    DWORD dwStylea;

    int y = p_y;
    int x = dwExStyle;
    DWORD dwExStylea = 0;

    if ((uint8_t)dwStyle)               // FULLSCREEN PATH
    {
        dwStylea   = 0x80000000;        // WS_POPUP
        dwExStylea = 8;                 // WS_EX_TOPMOST
        x = 0;
        y = 0;
        w = GetSystemMetrics(SM_CXSCREEN);
        h = GetSystemMetrics(SM_CYSCREEN);
    }
    else                                // WINDOWED PATH
    {
        RECT Rect{0,0,a6,a7};
        // Real overlapped window: caption + sysmenu + minimize, no resize.
        // (Original game used 0x80C80000 = WS_POPUP|WS_CAPTION|WS_SYSMENU,
        //  which is a valid but odd hybrid — Alt-Tab and dragging behave
        //  poorly. WS_OVERLAPPED is what users expect.)
        dwStylea = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_VISIBLE;
        AdjustWindowRectEx(&Rect, dwStylea, FALSE, 0);

        w = Rect.right  - Rect.left;
        h = Rect.bottom - Rect.top;

        Var<int> dword_9874D4{0x009874D4};
        Var<int> dword_955158{0x00955158};
        dword_9874D4() = -Rect.left;
        dword_955158() = -Rect.top;

        if (x == -1 || x == 0)
            x = (GetSystemMetrics(SM_CXSCREEN) - w) / 2;
        if (y == -1 || y == 0)
            y = (GetSystemMetrics(SM_CYSCREEN) - h) / 2;
        if (x < 0) x = 0;
        if (y < 0) y = 0;
    }

    SetLastError(0);
    sp_log("create_window");

g_appHwnd() = CreateWindowExA(
    dwExStylea,
    lpClassName,
    lpWindowName,
    dwStylea,
    x, y, w, h,
    nullptr,
    nullptr,
    hInstance,
    nullptr);
}





//0x0081C140
void register_class_and_create_window(LPCSTR lpClassName,
                                      LPCSTR lpWindowName,
                                      int X,
                                      int Y,
                                      int a5,
                                      int a6,
                                      WNDPROC windowProc,
                                      HINSTANCE hInstance,
                                      int a9,
                                      DWORD /*dwStyle*/) {
    if (g_appHwnd()) {
        DestroyWindow(g_appHwnd());
        g_appHwnd() = nullptr;
    }

    // Class must exist before CreateWindowExA can use it.
    register_class(lpClassName, windowProc, hInstance, a9);

    // create_window's last param: nonzero = fullscreen path, zero = windowed path.
    // -windowed (g_config.WindowedMode == true) -> 0 -> windowed path.
    const DWORD fullscreen = g_config.WindowedMode ? 0u : 1u;
    create_window(lpClassName, lpWindowName, hInstance, X, Y, a5, a6, fullscreen);
}

unsigned int hook_controlfp(unsigned int, unsigned int) {
    return {};
}

void initterm(const _PVFV *ppfn, const _PVFV *end) {
    if constexpr (1) {
        do {
            if (auto pfn = *++ppfn) {
                pfn();
            }

        } while (ppfn < end);

    } else {
        CDECL_CALL(0x00822840, ppfn, end);
    }
}

void start() {
    __asm("add esp, 4\n");

    int v22;
    int v3;
    bool v4;
    int v2;

    IMAGE_NT_HEADERS32 *nt_header;

    BOOL v21 = 0;
    auto module_handle = GetModuleHandleA(nullptr);

    uint8_t *module_base = CAST(module_base, module_handle);
    IMAGE_DOS_HEADER *dos_header = CAST(dos_header, module_base);
    if (dos_header->e_magic != IMAGE_DOS_SIGNATURE) {
        goto LABEL_5;
    }

    nt_header = CAST(nt_header, module_base + dos_header->e_lfanew);

    if (nt_header->Signature != IMAGE_NT_SIGNATURE) {
        goto LABEL_5;
    }

    v2 = nt_header->OptionalHeader.Magic;
    if (v2 == 0x10B) {
        if (nt_header->OptionalHeader.NumberOfRvaAndSizes > 14) {
            v3 = 0;
            v4 = nt_header->OptionalHeader.DataDirectory[14].VirtualAddress == 0;
            goto LABEL_10;
        }
    LABEL_5:
        v22 = 0;
        goto LABEL_11;
    }

    if (v2 != 0x20B || nt_header->OptionalHeader.DataDirectory[1].Size <= 14) {
        goto LABEL_5;
    }

    v3 = 0;
    v4 = nt_header[1].Signature == 0;
LABEL_10:
    v3 = !v4;
    v22 = v3;
LABEL_11:
    //ms_exc.registration.TryLevel = 0;

    Var<int (*)(DWORD)> _set_app_type = {0x0086F254};
    _set_app_type()(2);

    static Var<int> dword_987BE0{0x00987BE0};
    static Var<int> dword_987BE4{0x00987BE4};

    dword_987BE0() = -1;
    dword_987BE4() = -1;

    static Var<int> dword_987BD4{0x00987BD4};
    static Var<int> dword_987BD8{0x00987BD8};
    static Var<int> dword_987BDC{0x00987BDC};

    _fmode = dword_987BD8();

    Var<int *(*) (void)> _p__commode{0x0086F240};
    *_p__commode()() = dword_987BD4();

    Var<DWORD> adjust_fdiv{0x0086F2BC};
    dword_987BDC() = adjust_fdiv();
    CDECL_CALL(0x008227B8);

    int (*sub_822858)() = CAST(sub_822858, 0x00822858);
    CDECL_CALL(0x00822858);

    static Var<int> dword_956284{0x00956284};
    if (!dword_956284()) {
        Var<int (*)(DWORD)> _setusermatherr{0x0086F238};

        _setusermatherr()((DWORD) sub_822858);
    }

    auto _setdefaultprecision = []() -> unsigned int { return _controlfp(0x10000u, 0x30000u); };

    _setdefaultprecision();

    Var<_PVFV> dword_91D934{0x0091D934};
    Var<_PVFV> dword_91D938{0x0091D938};
    initterm(&dword_91D934(), &dword_91D938());

    auto sub_8227FC = []() -> void { CDECL_CALL(0x008227FC); };

    atexit(sub_8227FC);

    Var<int> dword_987BD0{0x00987BD0};
    auto v19 = dword_987BD0();

    Var<int (*)(int *, char ***, char ***, int, void *)> _getmainargs{0x0086F230};

    int v16;
    char **v17;
    char **v18;
    Var<int> dword_987BCC{0x00987BCC};

    auto v15 = _getmainargs()(&v16, &v17, &v18, dword_987BCC(), &v19);
    if (v15 < 0) {
        auto amsg_exit = [](int a1) -> int {
            //
            return CDECL_CALL(0x008227B2, a1);
        };

        amsg_exit(8);
    }

    Var<_PVFV> dword_91B000{0x0091B000};
    Var<_PVFV> dword_91D930{0x0091D930};

    initterm(&dword_91B000(), &dword_91D930());

    char *i;
    char *v20;
    for (i = _acmdln;; ++i) {
        v20 = i;
        auto v6 = *i;
        if ((uint8_t) *i <= ' ' && (!v6 || !v21)) {
            break;
        }

        if (v6 == 34) {
            v21 = !v21;
        }

        Var<int (*)(unsigned int)> ismbblead{0x0086F2B0};
        if (ismbblead()(v6)) {
            v20 = ++i;
        }
    }

    while (*i != '\0' && *i <= ' ') {
        v20 = ++i;
    }

    STARTUPINFOA StartupInfo;
    StartupInfo.dwFlags = 0;
    GetStartupInfoA(&StartupInfo);

    int nShowCmd = (((StartupInfo.dwFlags & 1) != 0) ? StartupInfo.wShowWindow : 10);

    auto v8 = GetModuleHandleA(nullptr);

    int __stdcall (*WinMain)(HINSTANCE, HINSTANCE, LPSTR, int) = CAST(WinMain, 0x005AC1E0);

    auto v9 = WinMain(v8, nullptr, i, nShowCmd);

    if (!v22) {
        exit(v9);
    }

    Var<void (*)()> cexit{0x0086F2B4};

    cexit()();
    //ms_exc.registration.TryLevel = -1;
}

static void *HookVTableFunction(void *pVTable, void *fnHookFunc, int nOffset) {
    intptr_t ptrVtable = *((intptr_t *) pVTable); // Pointer to our chosen vtable
    intptr_t ptrFunction = ptrVtable +
        sizeof(intptr_t) *
            nOffset; // The offset to the function (remember it's a zero indexed array with a size of four bytes)
    intptr_t ptrOriginal = *((intptr_t *) ptrFunction); // Save original address

    // Edit the memory protection so we can modify it
    MEMORY_BASIC_INFORMATION mbi;
    VirtualQuery((LPCVOID) ptrFunction, &mbi, sizeof(mbi));
    VirtualProtect(mbi.BaseAddress, mbi.RegionSize, PAGE_EXECUTE_READWRITE, &mbi.Protect);

    // Overwrite the old function with our new one
    *((intptr_t *) ptrFunction) = (intptr_t) fnHookFunc;

    // Restore the protection
    VirtualProtect(mbi.BaseAddress, mbi.RegionSize, mbi.Protect, &mbi.Protect);

    // Return the original function address incase we want to call it
    return (void *) ptrOriginal;
}

uint32_t controllerKeys[MENU_KEY_MAX];
uint32_t keys[256];

void GetDeviceStateHandleKeyboardInput(LPVOID lpvData) {
	BYTE* keysCurrent = (BYTE *) lpvData;

	for (auto i = 0u; i < 256u; ++i) {

		if (keysCurrent[i] != 0) {
			++keys[i];
		} else {
			keys[i] = 0;
		}
	}
}

int debug_enabled = 0;

int debug_disabled = 0;


int get_menu_key_value(MenuKey key, int keyboard) {
    if (keyboard) {

        int i = 0;
        switch (key) {
        case MENU_SELECT:
            i = DIK_Z;
            break;
        case MENU_START:
            i = DIK_Y;
            break;
        case MENU_ACCEPT:
            i = DIK_RETURN;
            break;
        case MENU_BACK:
            i = DIK_ESCAPE;
            break;

        case MENU_UP:
            i = DIK_UPARROW;
            break;
        case MENU_DOWN:
            i = DIK_DOWNARROW;
            break;
        case MENU_LEFT:
            i = DIK_LEFTARROW;
            break;
        case MENU_RIGHT:
            i = DIK_RIGHTARROW;
            break;
        }
        return keys[i];
    }
    return controllerKeys[key];
}
static constexpr DWORD MAX_ELEMENTS_PAGE = 18;

void menu_setup(int game_state, int keyboard);
void menu_input_handler(int keyboard, int SCROLL_SPEED);
void menu_input_handler_empty_dbg_controls(int keyboard, int SCROLL_SPEED);

// Defined further down next to apply_freeze_toggle(); forward-declared here
// because GetDeviceStateHook (below) calls it when handling the TAB / L3
// debug-menu focus toggle.
static void set_hero_frozen(bool frozen);

int is_menu_key_pressed(MenuKey key, int keyboard) {
    return (get_menu_key_value(key, keyboard) == 2);
}

int is_menu_key_clicked(MenuKey key, int keyboard) {
    return get_menu_key_value(key, keyboard);
}


void read_and_update_controller_key_button(LPDIJOYSTATE2 joy, int index, MenuKey key) {
	int res = 0;
	if (joy->rgbButtons[index]) {
		++controllerKeys[key];
	}
	else {
		controllerKeys[key] = 0;
	}
}


void read_and_update_controller_key_dpad(LPDIJOYSTATE2 joy, int angle, MenuKey key) {
	
	if (joy->rgdwPOV[0] == 0xFFFFFFFF)
		controllerKeys[key] = 0;
	else
		controllerKeys[key] = (joy->rgdwPOV[0] == angle) ? controllerKeys[key] + 1 : 0;
}


// Helper: treat a DirectInput axis as a digital direction.
//   axis_value  : raw DIJOYSTATE2 axis (default DInput range 0..65535, centered at 32768)
//   positive    : true => fires when axis is past center + threshold
//                 false => fires when axis is below center - threshold
//   key         : MenuKey slot to update
static void read_and_update_controller_key_axis(LONG axis_value, bool positive, MenuKey key) {
    constexpr LONG CENTER    = 32768;
    constexpr LONG THRESHOLD = 8192;     // ~25% deflection

    bool active = positive
        ? (axis_value > CENTER + THRESHOLD)
        : (axis_value < CENTER - THRESHOLD);

    if (active) {
        ++controllerKeys[key];
    } else {
        controllerKeys[key] = 0;
    }
}


void GetDeviceStateHandleControllerInput(LPVOID lpvData) {
	LPDIJOYSTATE2 joy = (decltype(joy)) lpvData;

	read_and_update_controller_key_button(joy, 1, MENU_ACCEPT);
	read_and_update_controller_key_button(joy, 2, MENU_BACK);
        read_and_update_controller_key_button(joy, 9, MENU_SELECT);
		read_and_update_controller_key_button(joy, 8, MENU_START);

	read_and_update_controller_key_dpad(joy, 0, MENU_UP);
	read_and_update_controller_key_dpad(joy, 9000, MENU_RIGHT);
	read_and_update_controller_key_dpad(joy, 18000, MENU_DOWN);
	read_and_update_controller_key_dpad(joy, 27000, MENU_LEFT);

	// L3 / R3 stick clicks (DS4 buttons 10 and 11 via DirectInput).
	read_and_update_controller_key_button(joy, 10, MENU_L3);
	read_and_update_controller_key_button(joy, 11, MENU_R3);

	// Left analog stick directions (lX = horizontal, lY = vertical, up = small Y).
	read_and_update_controller_key_axis(joy->lX, 13, MENU_L3_LEFT);
	read_and_update_controller_key_axis(joy->lX, 14,  MENU_L3_RIGHT);
	read_and_update_controller_key_axis(joy->lY, 15, MENU_L3_UP);
	read_and_update_controller_key_axis(joy->lY, 16,  MENU_L3_DOWN);

	// Right analog stick directions (DS4: lZ = horizontal, lRz = vertical, up = small).
	read_and_update_controller_key_axis(joy->lZ,  17, MENU_R3_LEFT);
	read_and_update_controller_key_axis(joy->lZ,  18,  MENU_R3_RIGHT);
	read_and_update_controller_key_axis(joy->lRz, 19, MENU_R3_UP);
	read_and_update_controller_key_axis(joy->lRz, 20,  MENU_R3_DOWN);
}


DWORD modulo(int num, DWORD mod) {
    if (num >= 0) {
        return num % mod;
    }

    int absolute = abs(num);
    if (absolute % mod == 0)
        return 0;

    return mod - absolute % mod;
}

void menu_go_down() {


    if ((current_menu->window_start + MAX_ELEMENTS_PAGE) < current_menu->used_slots) {

        if (current_menu->cur_index < MAX_ELEMENTS_PAGE / 2)
            ++current_menu->cur_index;
        else
            ++current_menu->window_start;
    }
    else {

        int num_elements = std::min((DWORD)MAX_ELEMENTS_PAGE, current_menu->used_slots - current_menu->window_start);
        current_menu->cur_index = modulo(current_menu->cur_index + 1, num_elements);
        if (current_menu->cur_index == 0)
            current_menu->window_start = 0;
    }
}

void menu_go_up() {

    int num_elements = std::min((DWORD)MAX_ELEMENTS_PAGE, current_menu->used_slots - current_menu->window_start);
    if (current_menu->window_start) {


        if (current_menu->cur_index > MAX_ELEMENTS_PAGE / 2)
            current_menu->cur_index--;
        else
            current_menu->window_start--;

    }
    else {

        int num_elements = std::min(MAX_ELEMENTS_PAGE, current_menu->used_slots - current_menu->window_start);
        current_menu->cur_index = modulo(current_menu->cur_index - 1, num_elements);
        if (current_menu->cur_index == (num_elements - 1))
            current_menu->window_start = current_menu->used_slots - num_elements;

    }

}




void menu_input_handler(int keyboard, int SCROLL_SPEED) {
    if (is_menu_key_clicked(MENU_DOWN, keyboard)) {

        int key_val = get_menu_key_value(MENU_DOWN, keyboard);
        if (key_val == 1) {
            menu_go_down();
        }
        else if ((key_val >= SCROLL_SPEED) && (key_val % SCROLL_SPEED == 0)) {
            menu_go_down();
        }
    }
    else if (is_menu_key_clicked(MENU_UP, keyboard)) {

        int key_val = get_menu_key_value(MENU_UP, keyboard);
        if (key_val == 1) {
            menu_go_up();
        }
        else if ((key_val >= SCROLL_SPEED) && (key_val % SCROLL_SPEED == 0)) {
            menu_go_up();
        }
    }
    else if (is_menu_key_pressed(MENU_ACCEPT, keyboard))
    {
        auto* entry = &current_menu->entries[current_menu->window_start + current_menu->cur_index];
        assert(entry != nullptr);
        entry->on_select(1.0);

        //current_menu->handler(entry, ENTER);
    }
    else if (is_menu_key_pressed(MENU_BACK, keyboard)) {
        current_menu->go_back();
    }
else if (is_menu_key_clicked(MENU_LEFT, keyboard)) { // Use is_menu_key_clicked here
        debug_menu_entry* cur = &current_menu->entries[current_menu->window_start + current_menu->cur_index];
        int key_val = get_menu_key_value(MENU_LEFT, keyboard);
        if (key_val == 1) {
            cur->on_change(-1.0, false);
        } else if ((key_val >= SCROLL_SPEED) && (key_val % SCROLL_SPEED == 0)) {
            cur->on_change(-1.0, false);
        }
    } else if (is_menu_key_clicked(MENU_RIGHT, keyboard)) { // Use is_menu_key_clicked here
        debug_menu_entry* cur = &current_menu->entries[current_menu->window_start + current_menu->cur_index];
        int key_val = get_menu_key_value(MENU_RIGHT, keyboard);
        if (key_val == 1) {
            cur->on_change(1.0, true);
        } else if ((key_val >= SCROLL_SPEED) && (key_val % SCROLL_SPEED == 0)) {
            cur->on_change(1.0, true);
        }
    }

    debug_menu_entry* highlighted = &current_menu->entries[current_menu->window_start + current_menu->cur_index];
    assert(highlighted->frame_advance_callback != nullptr);
    highlighted->frame_advance_callback(highlighted);
}


void menu_input_handler_empty_dbg_controls(int keyboard, int SCROLL_SPEED) {
    if (is_menu_key_clicked(MENU_DOWN, keyboard)) {

        int key_val = get_menu_key_value(MENU_DOWN, keyboard);
        if (key_val == 1) {
        }
        else if ((key_val >= SCROLL_SPEED) && (key_val % SCROLL_SPEED == 0)) {

        }
    }
    else if (is_menu_key_clicked(MENU_UP, keyboard)) {

        int key_val = get_menu_key_value(MENU_UP, keyboard);
        if (key_val == 1) {

        }
        else if ((key_val >= SCROLL_SPEED) && (key_val % SCROLL_SPEED == 0)) {
        }
    }
    else if (is_menu_key_pressed(MENU_ACCEPT, keyboard))
    {


        //current_menu->handler(entry, ENTER);
    }
    else if (is_menu_key_pressed(MENU_BACK, keyboard)) {
    }
else if (is_menu_key_clicked(MENU_LEFT, keyboard)) { // Use is_menu_key_clicked here
        debug_menu_entry* cur = &current_menu->entries[current_menu->window_start + current_menu->cur_index];
        int key_val = get_menu_key_value(MENU_LEFT, keyboard);
        if (key_val == 1) {
        } else if ((key_val >= SCROLL_SPEED) && (key_val % SCROLL_SPEED == 0)) {
        }
    } else if (is_menu_key_clicked(MENU_RIGHT, keyboard)) { // Use is_menu_key_clicked here
        debug_menu_entry* cur = &current_menu->entries[current_menu->window_start + current_menu->cur_index];
        int key_val = get_menu_key_value(MENU_RIGHT, keyboard);
        if (key_val == 1) {
        } else if ((key_val >= SCROLL_SPEED) && (key_val % SCROLL_SPEED == 0)) {

        }
    }


}





typedef int (__stdcall* GetDeviceState_ptr)(IDirectInputDevice8*, DWORD, LPVOID);
GetDeviceState_ptr GetDeviceStateOriginal = nullptr;

HRESULT __stdcall GetDeviceStateHook(IDirectInputDevice8* self, DWORD cbData, LPVOID lpvData) {

	HRESULT res = GetDeviceStateOriginal(self, cbData, lpvData);

	printf("cbData %d %d %d\n", cbData, sizeof(DIJOYSTATE), sizeof(DIJOYSTATE2));

	//keyboard time babyyy

	if (cbData == 256 || cbData == sizeof(DIJOYSTATE2)) {

		
		if (cbData == 256)
			GetDeviceStateHandleKeyboardInput(lpvData);
		else if (cbData == sizeof(DIJOYSTATE2))
			GetDeviceStateHandleControllerInput(lpvData);

		// Global windowed-mode toggle on DIK_X.
		//
		// Placed here (before the g_state guard below) so the hotkey works
		// in menus, loading screens, and frontends — not just during
		// RUNNING. Edge-triggered with the same `keys[i] == 2; keys[i] = 0`
		// pattern the debug menu uses, so a held key fires exactly once.
		//
		// This is parallel to the ALT+ENTER handler in WindowProc; both
		// route through apply_windowed_mode so the three pieces (globals,
		// D3D device, OS window) stay in sync.
		if (cbData == 256 && keys[DIK_X] == 2) {
			keys[DIK_X] = 0;
			apply_windowed_mode(!g_config.WindowedMode);
		}

		int game_states = 0;
		if (g_game_ptr)
        {
			//game_states = game_get_cur_state(g_game_ptr);
        }
     
    auto g_state = []() -> game_state {
        if (g_game_ptr != nullptr)
        {
            return g_game_ptr->get_cur_state();
        }

        return static_cast<game_state>(0);
    }();

    if (g_state != game_state::RUNNING && g_console == nullptr)
    {
        return res;
    }

    if (cbData == 256 || cbData == sizeof(DIJOYSTATE2)) {
        int keyboard = cbData == 256;
        menu_setup((int)g_state, keyboard);

        // --- Debug-menu focus toggle ---
        // While the debug menu is on-screen (debug_enabled || debug_disabled),
        // edge-detect TAB on the keyboard or L3 on the gamepad. Each press
        // flips debug_menu::has_focus and re-syncs the hero-frozen flag so
        // the two stay coupled:
        //   has_focus == true  -> menu eats input, hero is frozen
        //   has_focus == false -> menu still drawn (dimmed), input passes
        //                         through to the game, hero can move
        if (debug_enabled || debug_disabled) {
            bool toggle = false;
            if (keyboard && keys[DIK_TAB] == 2) {
                keys[DIK_TAB] = 0;
                toggle = true;
            }
            if (!keyboard && controllerKeys[MENU_SELECT] == 2) {
                controllerKeys[MENU_SELECT] = 0;
                toggle = true;
            }
            if (toggle) {
                debug_menu::has_focus = !debug_menu::has_focus;
                set_hero_frozen(debug_menu::has_focus);
            }
        }

        // Route nav input to the menu ONLY when it owns focus. The original
        // guard here was `if (debug_enabled, debug_disabled)` which is a
        // comma operator -- it silently evaluated only `debug_disabled` and
        // ignored `debug_enabled` entirely. Using the proper boolean OR plus
        // the focus check fixes both bugs.
        if ((debug_enabled || debug_disabled) && debug_menu::has_focus) {
            menu_input_handler(keyboard, 5);
        }
    }

    // Blank game input ONLY while the menu has focus. When the user has
    // toggled focus off (TAB / L3) the menu stays drawn but input is
    // forwarded to the hero -- this is the new "control the hero with
    // the menu still up" behavior.
    if ((debug_enabled || debug_disabled) && debug_menu::has_focus) {
        memset(lpvData, 0, cbData);
    }



    static constexpr struct {
        int key;
        char sym;
    } char_keys[] = {{DIK_A, 'a'}, {DIK_B, 'b'}, {DIK_C, 'c'}, {DIK_D, 'd'}, {DIK_E, 'e'}, {DIK_F, 'f'},
                     {DIK_G, 'g'}, {DIK_H, 'h'}, {DIK_I, 'i'}, {DIK_J, 'j'}, {DIK_K, 'k'}, {DIK_L, 'l'},
                     {DIK_M, 'm'}, {DIK_N, 'n'}, {DIK_O, 'o'}, {DIK_P, 'p'}, {DIK_Q, 'q'}, {DIK_R, 'r'},
                     {DIK_S, 's'}, {DIK_T, 't'}, {DIK_U, 'u'}, {DIK_V, 'v'}, {DIK_W, 'w'}, {DIK_X, 'x'},
                     {DIK_Y, 'y'}, {DIK_Z, 'z'}};

    static constexpr struct {
        int key;
        char sym;
    } num_keys[] = {{DIK_1, '1'},
                    {DIK_2, '2'},
                    {DIK_3, '3'},
                    {DIK_4, '4'},
                    {DIK_5, '5'},
                    {DIK_6, '6'},
                    {DIK_7, '7'},
                    {DIK_8, '8'},
                    {DIK_9, '9'},
                    {DIK_0, '0'}
    };

    auto key_is_pressed = [](int i) -> bool {
        auto res = (keys[i] == 2);
        if (res) keys[i] = 0;

        return res;
    };

    auto char_key_is_pressed = []() -> char {
        for (auto &k : char_keys) {
            if (keys[k.key] == 2) {
                keys[k.key] = 0;
                return k.sym;
            }
        }

        return 0;
    };

    auto num_key_is_pressed = []() -> char {
        for (auto &k : num_keys) {
            if (keys[k.key] == 2) {
                keys[k.key] = 0;
                return k.sym;
            }
        }

        return 0;
    };

    if (key_is_pressed(DIK_GRAVE)) {
        _kbevcb(KeyEvent::Press, KB_TILDE);
    }

    if (g_console == nullptr) {
        return res;
    }

    if (g_console->isVisible())
    {
        if (key_is_pressed(DIK_TAB))
        {
            _kbevcb(KeyEvent::Press, KB_TAB);
        }
        else if (key_is_pressed(DIK_RETURN)) {
            _kbevcb(KeyEvent::Press, KB_RETURN);
        } else if (key_is_pressed(DIK_HOME)) {
            _kbevcb(KeyEvent::Press, KB_HOME);
        } else if (key_is_pressed(DIK_END)) {
            _kbevcb(KeyEvent::Press, KB_END);
        } else if (key_is_pressed(DIK_PGUP)) {
            _kbevcb(KeyEvent::Press, KB_PAGEUP);
        } else if (key_is_pressed(DIK_PGDN)) {
            _kbevcb(KeyEvent::Press, KB_PAGEDOWN);
        } else if (key_is_pressed(DIK_UP)) {
            _kbevcb(KeyEvent::Press, KB_UP);
        } else if (key_is_pressed(DIK_DOWN)) {
            _kbevcb(KeyEvent::Press, KB_DOWN);
        }
        else if(key_is_pressed(DIK_BACKSPACE))
        {
            _kbevcb(KeyEvent::Press, KB_BACKSPACE);
        }
        else if (key_is_pressed(DIK_MINUS)) {
            _kbchcb('_');
        } else if (key_is_pressed(DIK_SPACE)) {
            _kbchcb(' ');
        }
        else if (char ch = char_key_is_pressed(); ch != 0)
        {
            if (GetKeyState(VK_SHIFT) & 0x8000) {
                _kbchcb(toupper(ch));
            } else {
                _kbchcb(ch);
            }
        }
        else if (char ch = num_key_is_pressed(); ch != 0)
        {
            if ((GetKeyState(VK_SHIFT) & 0x8000))
            {
                if (ch == '9')
                {
                    _kbchcb('(');
                }
                else if (ch == '0')
                {
                    _kbchcb(')');
                }
            }
            else
            {
                _kbchcb(ch);
            }

        }
    }

	if (g_console->isVisible()) {
        memset(lpvData, 0, cbData);
	}

	//printf("Device State called %08X %d\n", this, cbData);
}
	return res;
}

typedef HRESULT(__stdcall* GetDeviceData_ptr)(IDirectInputDevice8*, DWORD, LPDIDEVICEOBJECTDATA, LPDWORD, DWORD);
GetDeviceData_ptr GetDeviceDataOriginal = nullptr;

HRESULT __stdcall GetDeviceDataHook(IDirectInputDevice8* self, DWORD cbObjectData, LPDIDEVICEOBJECTDATA rgdod, LPDWORD pdwInOut, DWORD dwFlags) {

	HRESULT res = GetDeviceDataOriginal(self, cbObjectData, rgdod, pdwInOut, dwFlags);

	printf("data\n");
	if (res == DI_OK) {

		printf("All gud\n");
		for (auto i = 0u; i < *pdwInOut; ++i) {

			if (LOBYTE(rgdod[i].dwData) > 0) {

				if (rgdod[i].dwOfs == DIK_ESCAPE) {

					printf("Pressed escaped\n");
					__debugbreak();
				}
			}
		}
	}
	//printf("Device Data called %08X\n", this);

	return res;
}

typedef HRESULT(__stdcall* IDirectInput8CreateDevice_ptr)(IDirectInput8W*, const GUID*, LPDIRECTINPUTDEVICE8W*, LPUNKNOWN);
IDirectInput8CreateDevice_ptr createDeviceOriginal = nullptr;

HRESULT  __stdcall IDirectInput8CreateDeviceHook(IDirectInput8W* self, const GUID* guid, LPDIRECTINPUTDEVICE8W* device, LPUNKNOWN unk) {

	//printf("CreateDevice %d %d %d %d %d %d %d\n", *guid, GUID_SysMouse, GUID_SysKeyboard, GUID_SysKeyboardEm, GUID_SysKeyboardEm2, GUID_SysMouseEm, GUID_SysMouseEm2);
	sp_log("Guid = {%08lX-%04hX-%04hX-%02hhX%02hhX-%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX}\n",
		guid->Data1, guid->Data2, guid->Data3,
		guid->Data4[0], guid->Data4[1], guid->Data4[2], guid->Data4[3],
		guid->Data4[4], guid->Data4[5], guid->Data4[6], guid->Data4[7]);

	HRESULT res = createDeviceOriginal(self, guid, device, unk);

	if (IsEqualGUID(GUID_SysMouse, *guid))
		return res; // ignore mouse

	if (IsEqualGUID(GUID_SysKeyboard, *guid))
    {
		sp_log("Found the keyboard");
    }
	else
    {
		sp_log("Hooking something different...maybe a controller");
    }

    if (GetDeviceStateOriginal == nullptr) {
        GetDeviceStateOriginal = (GetDeviceState_ptr)
            HookVTableFunction((void *) *device, (void *) GetDeviceStateHook, 9);
    }

    if (GetDeviceDataOriginal == nullptr) {
        GetDeviceDataOriginal = (GetDeviceData_ptr) HookVTableFunction((void *) *device,
                                                                       (void *) GetDeviceDataHook,
                                                                       10);
    }

	return res;
}

typedef HRESULT(__stdcall* DirectInput8Create_ptr)(HINSTANCE hinst, DWORD dwVersion, REFIID riidltf, LPVOID* ppvOut, LPUNKNOWN punkOuter);
HRESULT __stdcall HookDirectInput8Create(HINSTANCE hinst, DWORD dwVersion, REFIID riidltf, LPVOID* ppvOut, LPUNKNOWN punkOuter)
{
	DirectInput8Create_ptr caller = (decltype(caller)) *(void**)0x00987944;
	HRESULT res = caller(hinst, dwVersion, riidltf, ppvOut, punkOuter);

	IDirectInput8* iDir = (IDirectInput8 *) (*ppvOut);

    if (createDeviceOriginal == nullptr) {
        createDeviceOriginal = (IDirectInput8CreateDevice_ptr)
            HookVTableFunction((void *) iDir, (void *) IDirectInput8CreateDeviceHook, 3);
    }

	return res;
}

typedef void (*aeps_RenderAll_ptr)();
aeps_RenderAll_ptr aeps_RenderAll_orig = (aeps_RenderAll_ptr)0x004D9310;


unsigned int nglColor(int r, int g, int b, int a)
{
    return ((a << 24) | (r << 16) | (g << 8) | (b & 255));
}



// ----------------------------------------------------------------------------------------------------------------------------
// 
// 
//typedef void (*nglListEndScene_ptr)();
//nglListEndScene_ptr nglListEndScene = (nglListEndScene_ptr)0x00742B50;

//typedef void (*nglSetClearFlags_ptr)(int);
//nglSetClearFlags_ptr nglSetClearFlags = (nglSetClearFlags_ptr)0x00769DB0;

void aeps_RenderAll() {
    static int cur_time = 0;
    int period = 60;
    int duration = 6 * period;
    float ratio = 1.f / period;

    uint8_t red = color_ramp_function(ratio, period, cur_time + 2 * period) + color_ramp_function(ratio, period, cur_time - 4 * period);
    uint8_t green = color_ramp_function(ratio, period, cur_time);
    uint8_t blue = color_ramp_function(ratio, period, cur_time - 2 * period);

    nglListAddString(*nglSysFont, 0.1f, 0.2f, 0.2f, nglColor(red, green, blue, 255), 1.f, 1.f, "");

    cur_time = (cur_time + 1) % duration;


    aeps_RenderAll_orig();
}


void HookFunc(DWORD callAdd, DWORD funcAdd, BOOLEAN jump, const char* reason) {

    //Only works for E8/E9 hooks	
    DWORD jmpOff = funcAdd - callAdd - 5;

    BYTE shellcode[] = { 0, 0, 0, 0, 0 };
    shellcode[0] = jump ? 0xE9 : 0xE8;

    memcpy(&shellcode[1], &jmpOff, sizeof(jmpOff));
    memcpy((void*)callAdd, shellcode, sizeof(shellcode));

    if (reason)
        printf("Hook: %08X -  %s\n", callAdd, reason);

}

#include "script_lib_debug_menu.h"
typedef int (*script_manager_register_allocated_stuff_callback_ptr)(void* func);
script_manager_register_allocated_stuff_callback_ptr script_manager_register_allocated_stuff_callback = (script_manager_register_allocated_stuff_callback_ptr)0x005AFE40;

typedef int (*construct_client_script_libs_ptr)();
construct_client_script_libs_ptr construct_client_script_libs = (construct_client_script_libs_ptr)0x0058F9C0;
typedef struct _list {
    struct _list* next;
    struct _list* prev;
    void* data;
}list;

#include "levelmenu.h"
debug_menu* progression_menu = nullptr;
debug_menu* script_menu = nullptr;
debug_menu* level_select_menu = nullptr;
#ifdef TARGET_XBOX
#endif

debug_menu** all_menus[] = {
    &debug_menu::root_menu,
    &progression_menu,
	&script_menu,
    &level_select_menu,
#ifdef TARGET_XBOX
#endif
};

void remove_debug_menu_entry(debug_menu_entry* entry) {

    DWORD to_be = (DWORD)entry;
    for (auto i = 0u; i < (sizeof(all_menus) / sizeof(void*)); ++i) {

        debug_menu* cur = *all_menus[i];

        DWORD start = (DWORD)cur->entries;
        DWORD end = start + cur->used_slots * sizeof(debug_menu_entry);

        if (start <= to_be && to_be < end) {

            int index = (to_be - start) / sizeof(debug_menu_entry);

            memcpy(&cur->entries[index], &cur->entries[index + 1], cur->used_slots - (index + 1));
            memset(&cur->entries[cur->used_slots - 1], 0, sizeof(debug_menu_entry));
            cur->used_slots--;
            return;
        }

    }

    printf("FAILED TO DEALLOCATE AN ENTRY :S %08X\n", entry);

}

void vm_debug_menu_entry_garbage_collection_callback(void* a1, list* lst) {

    list* end = lst->prev;

    for (list* cur = end->next; cur != end; cur = cur->next) {

        debug_menu_entry* entry = ((debug_menu_entry*)cur->data);
        //printf("Will delete %s %08X\n", entry->text, entry);
        remove_debug_menu_entry(entry);
    }
}

int construct_client_script_libs_hook() {
    if (vm_debug_menu_entry_garbage_collection_id == -1) {
        int res = script_manager_register_allocated_stuff_callback((void*)vm_debug_menu_entry_garbage_collection_callback);
        vm_debug_menu_entry_garbage_collection_id = res;
    }
    return construct_client_script_libs();
}





void handle_debug_entry(debug_menu_entry* entry, custom_key_type) {
    current_menu = entry->m_value.p_menu;
}

// AI

static ai::ai_core* debug_menu_ai_core = nullptr;

static const char* TYPE_NAME_ARRAY[8]{
    "float",
    "int",
    "string_hash",
    "fixedstring",
    "vector3d",
    "float_variance",
    "entity",
    "pointer"
};


void handle_export_block(debug_menu_entry* arg0)
{
    auto* the_pb = static_cast<ai::param_block*>(arg0->get_data());
    assert(the_pb != nullptr);

    auto* v2 = debug_menu_ai_core->get_actor(0);
    auto id = v2->get_id();
    auto v4 = id.to_string();
    mString a1{ 0, "param_dump_%s.txt", v4 };
    os_file file{ a1, 2 };
    mString v32{};

    auto* v31 = the_pb->param_array;
    if (v31 != nullptr)
    {
        auto* v8 = id.to_string();
        v32 = mString{ 0, "// Parameter list for %s\r\n", v8 };
        auto v14 = v32.length();
        auto* v9 = v32.c_str();
        file.write(v9, v14);

        for (auto& v28 : v31->field_0)
        {
            auto v21 = v28->get_value_in_string_form();
            auto v19 = v28->m_name;

            auto v15 = v21.c_str();
            auto* v13 = v19.to_string();
            auto data_type = v28->get_data_type();
            v32 = mString{ 0, "%s %s %s\r\n", TYPE_NAME_ARRAY[data_type], v13, v15 };
            auto v16 = v32.length();
            auto* v11 = v32.c_str();
            file.write(v11, v16);
        }
    }
    else
    {
        v32 = mString{ "// no parameters defined in this block.\r\n" };
        auto v17 = v32.length();
        auto* v12 = v32.c_str();
        file.write(v12, v17);
    }

    debug_menu::hide();
}

std::string ai_param_render_callback(debug_menu_entry* a2)
{
    using namespace ai;

    mString result{};

    auto* the_data = static_cast<ai::param_block::param_data*>(a2->get_data());
    assert(the_data != nullptr);

    switch (the_data->get_data_type())
    {
    case PT_STRING_HASH: {
        auto v19 = the_data->get_data_hash();
        auto v18 = the_data->m_name;
        auto* v15 = v19.to_string();
        auto* v2 = v18.to_string();
        result = mString{ 0, "%s %s (hash)", v2, v15 };
        break;
    }
    case PT_FIXED_STRING: {
        auto v20 = the_data->m_name;
        auto* v16 = the_data->get_data_string();
        auto v4 = v20.to_string();
        result = mString{ 0, "%s %s (fixedstring)", v4, v16 };
        break;
    }
    case PT_VECTOR_3D: {
        vector3d v32 = *the_data->get_data_vector3d();
        auto v21 = the_data->m_name;
        auto v13 = v32[2];
        auto v11 = v32[1];
        auto v10 = v32[0];
        auto* v6 = v21.to_string();
        result = mString{ 0, "%s (%.2f %.2f %.2f) (vector3d)", v6, v10, v11, v13 };
        break;
    }
    case PT_FLOAT_VARIANCE: {
        auto v31 = *the_data->get_data_float_variance();
        auto v22 = the_data->m_name;
        auto v14 = v31.field_4;
        auto v12 = v31.field_0;
        auto* v8 = v22.to_string();
        result = mString{ 0, "%s (b%.2f v%.2f) (float variance)", v8, v12, v14 };
        break;
    }
    default: {
        auto v23 = the_data->m_name;
        auto data_type = the_data->get_data_type();
        auto* v9 = v23.to_string();
        printf("Unsupported param data type for %s %d.  This code needs to be updated\n", v9, data_type);
        assert(0);
        return result.c_str();
    }
    }

    return result.c_str();
}

void populate_param_block(debug_menu_entry* a2)
{
    using namespace ai;

    auto* the_pb = static_cast<ai::param_block*>(a2->get_data());
    assert(the_pb != nullptr);

    auto name_menu = a2->get_script_handler();
    auto* v26 = create_menu(name_menu.c_str(), debug_menu::sort_mode_t::ascending);
    a2->set_submenu(v26);

    debug_menu_entry* v27 = nullptr;
    auto* v25 = the_pb->param_array;
    if (v25 != nullptr)
    {
        for (auto& v22 : v25->field_0)
        {
            auto data_hash = v22->m_name;
            auto* v2 = data_hash.to_string();
            mString a1{ 0, "%s", v2 };

            debug_menu_entry v27{ a1 };

            switch (v22->get_data_type())
            {
            case PT_FLOAT: {
                auto* v3 = bit_cast<float*>(v22);

                v27.set_pt_fval(v3);
                v27.set_step_size(0.30000001);
                v27.set_step_scale(30.0);
                v27.set_max_value(3.4028235e38);
                v27.set_min_value(-3.4028235e38);
                break;
            }
            case PT_INTEGER: {
                auto* v4 = bit_cast<int*>(v22);

                v27.set_p_ival(v4);
                v27.set_max_value(2147483600.0);
                v27.set_min_value(-2147483600.0);
                break;
            }
            case PT_STRING_HASH:
            case PT_FIXED_STRING:
            case PT_VECTOR_3D:
            case PT_FLOAT_VARIANCE:
                v27.set_render_cb(ai_param_render_callback);
                break;
            default:
                auto data_type = v22->get_data_type();
                printf("Unknown param data type %d.  This code needs to be updated\n", data_type);
                assert(0);
                return;
            }

            v27.set_data(v22);
            v26->add_entry(&v27);
        }

        v27 = create_menu_entry(mString{ "--Export this block--" });
        v27->set_game_flags_handler(handle_export_block);
        v27->set_data(the_pb);
    }
    else
    {
        v27 = create_menu_entry(mString{ "--None defined--" });
    }

    v26->add_entry(v27);
}

void ai_core_menu_handler(debug_menu_entry* a2)
{
    auto* the_core = static_cast<ai::ai_core*>(a2->get_data());
    assert(the_core != nullptr);

    debug_menu_ai_core = the_core;

    auto name_menu = a2->get_script_handler();
    auto* v21 = create_menu(name_menu.c_str(), debug_menu::sort_mode_t::ascending);
    a2->set_submenu(v21);

    debug_menu_entry v20{ mString {"-Core params"} };
    v20.set_submenu(nullptr);
    v20.set_game_flags_handler(populate_param_block);
    auto* v2 = the_core->get_param_block();
    v20.set_data(v2);
    v21->add_entry(&v20);

    auto* v19 = the_core->field_60;
    if (v19 != nullptr)
    {
        for (auto& v16 : (*v19))
        {
            auto v5 = v16->field_4;
            auto* v3 = v5.to_string();

            debug_menu_entry v20{ mString {0, "%s inode params", v3} };
            v20.set_submenu(nullptr);
            v20.set_game_flags_handler(populate_param_block);
            auto& v4 = v16->field_10;
            v20.set_data(&v4);
            v21->add_entry(&v20);
        }
    }
}


#include "entity_base.h"
void populate_ai_root(debug_menu_entry* arg0)
{
    auto name_menu = arg0->get_script_handler();
    debug_menu* v20 = create_menu(name_menu.c_str(), debug_menu::sort_mode_t::undefined);
    arg0->set_submenu(v20);

    static auto* g_the_ai_core_list = ai::ai_core::the_ai_core_list_high();

    if (g_the_ai_core_list != nullptr)
    {
        for (auto& v17 : (*g_the_ai_core_list))
        {
            auto* v3 = v17->get_actor(0);
            if (!v3->is_flagged(0x800u))
            {
                auto id = v3->get_id();
                auto* v7 = id.to_string();
                debug_menu_entry v16{ mString {v7} };

                v16.set_data(v17);
                v16.set_submenu(nullptr);
                v16.set_game_flags_handler(ai_core_menu_handler);
                v20->add_entry(&v16);
            }
        }
    }
}

void create_ai_root_menu(debug_menu* parent)
{
    assert(parent != nullptr);

    debug_menu_entry v5{ mString {"AI"} };
    v5.set_submenu(nullptr);
    v5.set_game_flags_handler(populate_ai_root);
    parent->add_entry(&v5);
}


// Memory

int g_mem_checkpoint_debug_0{ -1 };

void set_memtrack_checkpoint(debug_menu_entry*)
{
    g_mem_checkpoint_debug_0 = mem_set_checkpoint();
    debug_menu::hide();
}

void dump_memtrack_data(debug_menu_entry*)
{
    mem_check_leaks_since_checkpoint(g_mem_checkpoint_debug_0, 1u);
    mem_print_stats("\nMemory log\n");
    debug_menu::hide();
}

void create_memory_menu(debug_menu* parent)
{
    auto* memory_menu = create_menu("Memory", debug_menu::sort_mode_t::undefined);
    auto* v2 = create_menu_entry(memory_menu);
    parent->add_entry(v2);

    script_memtrack::create_debug_menu(memory_menu);

    auto* entry = create_menu_entry(mString{ "Dump MemTrack Data Since Last Checkpoint" });
    entry->set_game_flags_handler(dump_memtrack_data);
    memory_menu->add_entry(entry);

    entry = create_menu_entry(mString{ "Set MemTrack Checkpoint" });
    entry->set_game_flags_handler(set_memtrack_checkpoint);
    memory_menu->add_entry(entry);

    slab_allocator::create_slab_debug_menu(memory_menu);
}


// Entity Animation 

void entity_animation_handler(debug_menu_entry* entry)
{
    printf("handle_animation_entry\n");

    auto* v7 = static_cast<entity*>(entry->data1);
    if (v7 != nullptr && v7->is_an_actor())
    {
        auto* v6 = (actor*)v7;
        auto* context = v6->m_resource_context;
        assert(context != nullptr);

        string_hash v4{ entry->text };
        resource_manager::push_resource_context(context);

        auto v3 = v6->play_anim(v4);
        resource_manager::pop_resource_context();
        v3.set_anim_speed(1.0);
        debug_menu::hide();
    }
}

void sub_6918AD(debug_menu_entry* entry)
{
    auto* e = static_cast<entity*>(entry->data1);
    if (e->is_an_actor())
    {
        auto* a1 = create_menu(entry->text);
        entry->set_submenu(a1);
        auto* v18 = (actor*)e;


        std::list<nalAnimClass<nalAnyPose>*> v17;
        actor::get_animations(v18,v17);
        
        for (auto* v15 : v17)
        {
            auto& v3 = v15->field_8;
            auto* v4 = v3.to_string();
            mString v14{ v4 };

            debug_menu_entry v13{ v14 };
            v13.data1 = entry->data1;
            v13.set_game_flags_handler(entity_animation_handler);
            a1->add_entry(&v13);
        }
    }
}

void populate_entity_animation_menu(debug_menu_entry* entry)
{
    auto* v26 = create_menu(entry->text);
    entry->set_submenu(v26);
    entity::find_entities(1);

    Var<_std::list<entity*>*> found_entities_{ 0x0095A6E0 };
    auto& found_entities = (*found_entities_());
    for (auto* ent : found_entities)
    {
        if (ent->is_an_actor())
        {
            auto* v23 = (actor*)ent;
            std::list<nalAnimClass<nalAnyPose>*> v22;
            actor::get_animations(v23, v22);
            if (!v22.empty())
            {
                auto id = v23->get_id();
                auto* v4 = id.to_string();
                mString v14{ v4 };

                debug_menu_entry v21{ v14 };

                v21.set_game_flags_handler(sub_6918AD);
                v21.data1 = ent;
                v21.set_submenu(nullptr);
                v26->add_entry(&v21);
            }
        }
    }
}

void create_entity_animation_menu(debug_menu* parent)
{
    debug_menu_entry v5{ mString{"Entity Animations"} };

    v5.set_submenu(nullptr);
    v5.set_game_flags_handler(populate_entity_animation_menu);
    parent->add_entry(&v5);
}

// Replays

#ifdef TARGET_XBOX
#include "rtdt_replay_mgr.h"
#include "rtdt_replay_mgr.h"


bool sub_6694CA(rtdt_replay_mgr* instance)
{
    g_game_ptr->enable_physics(false);
    return true;
}

void sub_66ADD4(rtdt_replay_mgr* instance, int flag)
{
}

void replay_handler(debug_menu_entry* entry)
{

    if (entry->get_id() == 0) {
        auto instance = rtdt_replay_mgr::m_instance;

        if (sub_6694CA(instance)) {
            auto replay_instance = rtdt_replay_mgr::m_instance;

            sub_66ADD4(replay_instance, 1);

            g_game_ptr->enable_physics(true);
            debug_menu::physics_state_on_exit = true;
        }

        debug_menu::hide();
    }
}


void populate_replay_menu(debug_menu_entry* entry)
{

    auto head_menu = create_menu("Replay", debug_menu::sort_mode_t::ascending);
    entry->set_submenu(head_menu);

    mString v25{ "Start" };
    debug_menu_entry v38{ v25.c_str() };

    v38.set_game_flags_handler(replay_handler);

    head_menu->add_entry(&v38);
}

void create_replay_menu(debug_menu* parent)
{
    auto* replay_menu = create_menu("Replay");
		            debug_menu_entry v1;
    debug_menu_entry* block = v1.alloc_block(replay_menu, 4);
    block[0] = debug_menu_entry { replay_menu };
    auto* v2 = create_menu_entry(replay_menu);
    v2->set_game_flags_handler(populate_replay_menu);
    parent->add_entry(v2);
}
#endif



// Entity Variants
// ----------------------------------------------------------------------------------
#include "entity_base.h"
#include "list.hpp"
#include "entity.h"
#include "variant_interface.h"

void apply_variant_handler(debug_menu_entry* entry)
{
    if (auto c = static_cast<conglomerate*>(entry->m_data)) {
        if (auto ifc = c->m_variant_interface) {
            string_hash hash{ entry->text };
            ifc->apply_variant(hash);
        }
    }
}
void populate_variants_menu(debug_menu_entry* entry)
{
    auto* menu = create_menu(entry->text, debug_menu::sort_mode_t::ascending);
	
			            debug_menu_entry v1;
    debug_menu_entry* block = v1.alloc_block(menu, 4);
    block[0] = debug_menu_entry { menu };
    entry->set_submenu(menu);

    auto c = static_cast<conglomerate*>(entry->m_data);
    auto vi = c->m_variant_interface;
    for (int i = 0; i < vi->variants.size(); ++i) {
        string_hash sss(vi->variants.m_data[i].hash);
        auto variant_name = sss.to_string();

        auto variant_menu = create_menu_entry(variant_name);
        variant_menu->set_game_flags_handler(apply_variant_handler);
        variant_menu->set_data(entry->m_data);
        menu->add_entry(variant_menu);
    }
}

void populate_entity_variants_menu(debug_menu_entry* entry) {
    auto* submenu = create_menu("Entity Variants", debug_menu::sort_mode_t::ascending);
				            debug_menu_entry v1;
	    debug_menu_entry* block = v1.alloc_block(submenu, 4);
    block[0] = debug_menu_entry { submenu };
    entry->set_submenu(submenu);

    entity::find_entities(256);

    _std::list<entity*> entities = *entity::found_entities;
    for (auto& entity : entities) {
        auto entityName = entity->get_id().to_string();
        auto entityEntry = create_menu_entry(entityName);
        entityEntry->set_game_flags_handler(populate_variants_menu);
        entityEntry->set_data(entity);
        entityEntry->set_submenu(nullptr);
        submenu->add_entry(entityEntry);
    }
}

void create_entity_variants_menu(debug_menu* parent) {
    auto* entity_variants_menu = create_menu("Entity Variants");
	            debug_menu_entry v1;
    debug_menu_entry* block = v1.alloc_block(entity_variants_menu, 4);
    block[0] = debug_menu_entry { entity_variants_menu };
    auto* entry = create_menu_entry(entity_variants_menu);
    entry->set_submenu(nullptr);
    entry->set_game_flags_handler(populate_entity_variants_menu);
    parent->add_entry(entry);
}


// Dvars
// ----------------------------------------------------------------------------------
#pragma region "Dvars"

static const float flt_881AC0 = 0.5;
static const float flt_882098 = 2.5;
static const float flt_87EA34 = 0.75;
static const float flt_8820A0 = 0.66000003;
static const float flt_87EEDC = 0.69999999;

auto _g_camera_min_dist = Var<float>(0x00881AB4);

auto _g_camera_max_dist = Var<float>(0x00881AB8);

auto _g_camera_supermax_dist = Var<float>(0x00881ABC);

static auto _g_base_factor = Var<float>(0x0091F6D8);

static auto _g_snow_balling = Var<float>(0x0091F6DC);

static auto _g_jump_cap_vel = Var<float>(0x0091F6E0);

void populate_dvars(debug_menu_entry* entry)
{

    auto* v7 = create_menu("Dvars", debug_menu::sort_mode_t::undefined);
    entry->set_submenu(v7);

    auto* base_factor = create_menu_entry(mString{ "base_factor" });
    const float v10[4]{ -1000.0, 1000.0, 0.5, 10.0 };
    base_factor->set_fl_values(v10);
    base_factor->set_pt_fval(&_g_base_factor());
    v7->add_entry(base_factor);

    auto* camera_min_dist = create_menu_entry(mString{ "camera_min_dist" });
    const float v5[4]{ -1000.0, 1000.0, 0.5, 10.0 };
    camera_min_dist->set_fl_values(v5);
    camera_min_dist->set_pt_fval(&_g_camera_min_dist());
    v7->add_entry(camera_min_dist);

    auto* camera_max_dist = create_menu_entry(mString{ "camera_max_dist" });
    const float v6[4]{ -1000.0, 1000.0, 0.5, 10.0 };
    camera_max_dist->set_fl_values(v6);
    camera_max_dist->set_pt_fval(&_g_camera_max_dist());
    v7->add_entry(camera_max_dist);

    auto* camera_supermax_dist = create_menu_entry(mString{ "camera_supermax_dist" });
    const float v9[4]{ -1000.0, 1000.0, 0.5, 10.0 };
    camera_supermax_dist->set_fl_values(v9);
    camera_supermax_dist->set_pt_fval(&_g_camera_supermax_dist());
    v7->add_entry(camera_supermax_dist);

    auto* jump_cap_vel = create_menu_entry(mString{ "jump_cap_vel" });
    const float v11[4]{ -1000.0, 1000.0, 0.5, 10.0 };
    jump_cap_vel->set_fl_values(v11);
    jump_cap_vel->set_pt_fval(&_g_jump_cap_vel());
    v7->add_entry(jump_cap_vel);

    auto* snow_balling = create_menu_entry(mString{ "snow_balling" });
    const float v12[4]{ -1000.0, 1000.0, 0.5, 10.0 };
    snow_balling->set_fl_values(v12);
    snow_balling->set_pt_fval(&_g_snow_balling());
    v7->add_entry(snow_balling);
}

void create_dvars_menu(debug_menu* arg0)
{
    auto* dvars_menu = create_menu("Dvars");
		            debug_menu_entry v1;
    debug_menu_entry* block = v1.alloc_block(dvars_menu, 4);
    block[0] = debug_menu_entry { dvars_menu };
    auto* v2 = create_menu_entry(dvars_menu);
    v2->set_game_flags_handler(populate_dvars);
    arg0->add_entry(v2);
}



// Missions
// ----------------------------------------------------------------------------------


struct mission_t {
    std::string field_0;
    const char *field_C;
    int m_district_id;
    int field_14;
};

std::vector<mission_t> menu_missions;

void mission_unload_handler(debug_menu_entry *)
{
    auto *v1 = mission_manager::s_inst;
    v1->prepare_unload_script();
    debug_menu::hide();
}

void mission_select_handler(debug_menu_entry *entry)
{
    auto v1 = entry->m_id;
    const auto &v7 = menu_missions[v1];
    auto v6 = v7.field_C;
    auto v5 = v7.field_14;
    auto *v4 = v7.field_0.c_str();
    auto v3 = v7.m_district_id;
    auto *v2 = mission_manager::s_inst;
    v2->force_mission(v3, v4, v5, v6);
    debug_menu::hide();
}

typedef int (*resource_manager_can_reload_amalgapak_ptr)(void);
resource_manager_can_reload_amalgapak_ptr resource_manager_can_reload_amalgapak = (resource_manager_can_reload_amalgapak_ptr)0x0053DE90;

typedef void (*resource_manager_reload_amalgapak_ptr)(void);
resource_manager_reload_amalgapak_ptr resource_manager_reload_amalgapak = (resource_manager_reload_amalgapak_ptr)0x0054C2E0;


// Helper function to add a mission entry
void add_mission_entry(const mission_table_container::script_info &info, int district_id, debug_menu *target_menu)
{
    auto v50 = menu_missions.size();
    const auto v19 = std::string {"pk_"} + info.field_0;
    auto *v11 = v19.c_str();
    auto key = create_resource_key_from_path(v11, RESOURCE_KEY_TYPE_PACK);
    
    if ( resource_manager::get_pack_file_stats(key, nullptr, nullptr, nullptr) )
    {
        mission_t mission {};
        mission.field_0 = info.field_0;
        mission.m_district_id = district_id;
        mission.field_14 = info.field_8;
        mission.field_C = info.field_4->get_script_data_name();
        menu_missions.push_back(mission);
        
        mString v47{};
        if ( mission.field_C != nullptr )
        {
            auto *v17 = mission.field_C;
            auto *v14 = mission.field_0.c_str();
            v47 = mString {0, "%s (%s)", v14, v17};
        }
        else
        {
            auto v18 = mission.field_14;
            auto *v15 = mission.field_0.c_str();
            v47 = mString {0, "%s (%d)", v15, v18};
        }
        
        auto *v46 = create_menu_entry(v47);
        v46->set_id(v50);
        v46->set_game_flags_handler(mission_select_handler);
        target_menu->add_entry(v46);
    }
}

void populate_missions_menu(debug_menu_entry *entry)
{
    menu_missions = {};
    if ( resource_manager_can_reload_amalgapak() )
    {
        resource_manager_can_reload_amalgapak();
    }
    auto *head_menu = create_menu("Missions", debug_menu::sort_mode_t::ascending);
    entry->set_submenu(head_menu);
    debug_menu_entry v4;
    debug_menu_entry* block1 = v4.alloc_block(head_menu, 4);
    block1[0] = debug_menu_entry{ head_menu };
    
    auto *mission_unload_entry = create_menu_entry(mString{"UNLOAD CURRENT MISSION"});
    mission_unload_entry->set_game_flags_handler(mission_unload_handler);
    head_menu->add_entry(mission_unload_entry);
    
    auto *v2 = mission_manager::s_inst;
    auto v58 = v2->get_district_table_count();
    
    for ( auto i = -1; i < v58; ++i )
    {
        fixedstring<8> v53 {};
        int district_id;
        mission_table_container *table = nullptr;
        debug_menu *current_district_menu = nullptr;
        
        if ( i == -1 )
        {
            table = v2->get_global_table();
            v53 = fixedstring<8> {"global"};
            district_id = 0;
            current_district_menu = head_menu; // Put global missions directly in main menu
        }
        else
        {
            table = v2->get_district_table(i);
            auto *reg = table->get_region();
            v53 = reg->get_name();
            district_id = reg->get_district_id();
            
            // Create district folder
            auto *district_menu = create_menu(v53.to_string(), debug_menu::sort_mode_t::ascending);
            debug_menu_entry v4_district;
            debug_menu_entry* block_district = v4_district.alloc_block(district_menu, 4);
            block_district[0] = debug_menu_entry{ district_menu };
            auto *district_entry = create_menu_entry(district_menu);
            head_menu->add_entry(district_entry);
            current_district_menu = district_menu;
        }
        
        _std::vector<mission_table_container::script_info> script_infos;
        if ( table != nullptr )
        {
            auto res = table->append_script_info(&script_infos);
            // assert(res);
        }
        
        // Separate missions by type
        std::vector<mission_table_container::script_info> regular_missions;
        std::vector<mission_table_container::script_info> trick_races;
        std::vector<mission_table_container::script_info> venom_trick_races;
        
        for ( auto &info : script_infos)
        {
            std::string mission_name = info.field_0;
            
            // Check if it's a venom trick race (check for "venom" first since it might also contain "trick")
            if (mission_name.find("venom") != std::string::npos && 
                mission_name.find("trick") != std::string::npos)
            {
                venom_trick_races.push_back(info);
            }
            // Check if it's a regular trick race
            else if (mission_name.find("trick") != std::string::npos)
            {
                trick_races.push_back(info);
            }
            // Regular mission
            else
            {
                regular_missions.push_back(info);
            }
        }
        
        // Add regular missions directly to district menu
        for ( auto &info : regular_missions)
        {
            add_mission_entry(info, district_id, current_district_menu);
        }
        
        // Create Trick Races submenu if there are trick races
        if (!trick_races.empty())
        {
            auto *trick_menu = create_menu("Trick Races", debug_menu::sort_mode_t::ascending);
            debug_menu_entry v4_trick;
            debug_menu_entry* block_trick = v4_trick.alloc_block(trick_menu, 4);
            block_trick[0] = debug_menu_entry{ trick_menu };
            auto *trick_entry = create_menu_entry(trick_menu);
            current_district_menu->add_entry(trick_entry);
            
            for ( auto &info : trick_races)
            {
                add_mission_entry(info, district_id, trick_menu);
            }
        }
        
        // Create Venom Trick Races submenu if there are venom trick races
        if (!venom_trick_races.empty())
        {
            auto *venom_trick_menu = create_menu("Venom Trick Races", debug_menu::sort_mode_t::ascending);
            debug_menu_entry v4_venom;
            debug_menu_entry* block_venom = v4_venom.alloc_block(venom_trick_menu, 4);
            block_venom[0] = debug_menu_entry{ venom_trick_menu };
            auto *venom_trick_entry = create_menu_entry(venom_trick_menu);
            current_district_menu->add_entry(venom_trick_entry);
            
            for ( auto &info : venom_trick_races)
            {
                add_mission_entry(info, district_id, venom_trick_menu);
            }
        }
    }
}


void _populate_missions()
{
    auto handle_table = [](mission_table_container *table, int district_id) -> void
    {

        _std::vector<mission_table_container::script_info> script_infos{};

        if ( table != nullptr )
        {
            table->append_script_info(&script_infos);
        }

        for ( auto &info : script_infos )
        {
                std::string a2{ "pk_" };
                auto v19 = a2 + info.field_0;
                auto* v11 = v19.c_str();
            auto key = create_resource_key_from_path(v11, RESOURCE_KEY_TYPE_PACK);
            if ( resource_manager::get_pack_file_stats(key, nullptr, nullptr, nullptr) )
            {
                mission_t mission{};
                mission.field_0 = info.field_0;
                mission.m_district_id = district_id;
                mission.field_14 = info.field_8;

                mission.field_C = info.field_4->get_script_data_name();
                menu_missions.push_back(mission);

                auto v47 = [](mission_t &mission) -> mString {
                    if ( mission.field_C != nullptr )
                    {
                        auto *v17 = mission.field_C;
                        auto *v14 = mission.field_0.c_str();
                        mString str{0, "%s (%s)", v14, v17};
                        return str;
                    }

                    auto v18 = mission.field_14;
                    auto *v15 = mission.field_0.c_str();
                    mString str{0, "%s (%d)", v15, v18};
                    return str;

                }(mission);

                sp_log(v47.c_str());
            }
        }
    };

    auto *v2 = mission_manager::s_inst;
    auto count = v2->get_district_table_count();
    sp_log("%s %d", "table_count = ", count);

    {
        auto *v3 = mission_manager::s_inst;
        auto *table = v3->get_global_table();

        handle_table(table, 0);
    }


    std::for_each(&v2->m_district_table_containers[0], &v2->m_district_table_containers[0] + count, [&handle_table](auto *table) {

        auto *reg = table->get_region();
        auto &v6 = reg->get_name();
        fixedstring<8> v53{v6.to_string()};

        auto district_id = reg->get_district_id();

        //sp_log("%d %s", i, v53.to_string());

        handle_table(table, district_id);
    });

    assert(0);
}





void create_missions_menu(debug_menu* parent)
{

    auto* missions_menu = create_menu("Missions");
    auto* v2 = create_menu_entry(missions_menu);
   debug_menu_entry v1;
    debug_menu_entry* block = v1.alloc_block(missions_menu, 4);
    block[0] = debug_menu_entry{ missions_menu };
    v2->set_game_flags_handler(populate_missions_menu);
    parent->add_entry(v2);
}


// Devopts 

#include "os_developer_options.h"
#include "devopt.h"
inline constexpr auto NUM_OPTIONS = 150u + 76u;

typedef bool(__fastcall* entity_tracker_manager_get_the_arrow_target_pos_ptr)(void*, void*, vector3d*);
entity_tracker_manager_get_the_arrow_target_pos_ptr entity_tracker_manager_get_the_arrow_target_pos = (entity_tracker_manager_get_the_arrow_target_pos_ptr)0x0062EE10;






void devopt_flags_handler(debug_menu_entry *a1)
{
    switch ( a1->get_id() )
    {
    case 0u: //CD_ONLY
    {
        os_developer_options::instance->set_flag(mString { "CD_ONLY" }, a1->get_bval());

		
        break;
    }
    case 1u: //ENVMAP_TOOL
    {
        os_developer_options::instance->set_flag(mString { "ENVMAP_TOOL" }, a1->get_bval());

        break;
    }
    case 2u: //NO_CD
    {
        os_developer_options::instance->set_flag(mString { "NO_CD" }, a1->get_bval());
        break;
    }
    case 3u: //CHATTY_LOAD
    {
        os_developer_options::instance->set_flag(mString { "CHATTY_LOAD" }, a1->get_bval());
        break;
    }
    case 4u: //WINDOW_DEFAULT
    {
        os_developer_options::instance->set_flag(mString { "WINDOW_DEFAULT" }, a1->get_bval());
        break;
    }
    case 5u: //SHOW_FPS
    {
        os_developer_options::instance->set_flag(mString { "SHOW_FPS" }, a1->get_bval());
        break;
    }
    case 6u: //SHOW_STREAMER_INFO
    {
        os_developer_options::instance->set_flag(mString { "SHOW_STREAMER_INFO" }, a1->get_bval());
        break;
    }
    case 7u: //SHOW_STREAMER_SPAM
    {
        os_developer_options::instance->set_flag(mString { "SHOW_STREAMER_SPAM" }, a1->get_bval());
        break;
    }
    case 8u: //SHOW_RESOURCE_SPAM
    {
        os_developer_options::instance->set_flag(mString { "SHOW_RESOURCE_SPAM" }, a1->get_bval());
        break;
    }
    case 9u: //SHOW_MEMORY_INFO
    {
        os_developer_options::instance->set_flag(mString { "SHOW_MEMORY_INFO" }, a1->get_bval());
        break;
    }
    case 10u: //SHOW_SPIDEY_SPEED
    {
        os_developer_options::instance->set_flag(mString { "SHOW_SPIDEY_SPEED" }, a1->get_bval());
        break;
    }
    case 11u: //TRACE_MISSION_MANAGER
    {
        os_developer_options::instance->set_flag(mString { "TRACE_MISSION_MANAGER" }, a1->get_bval());
        break;
    }
    case 12u: //DUMP_MISSION_HEAP
    {
        os_developer_options::instance->set_flag(mString { "DUMP_MISSION_HEAP" }, a1->get_bval());
        break;
    }
    case 13u: //CAMERA_CENTRIC_STREAMER
    {
        os_developer_options::instance->set_flag(mString { "CAMERA_CENTRIC_STREAMER" }, a1->get_bval());
        break;
    }
    case 14u: //RENDER_LOWLODS
    {
        os_developer_options::instance->set_flag(mString { "RENDER_LOWLODS" }, a1->get_bval());
        break;
    }
    case 15u: //LOAD_STRING_HASH_DICTIONARY
    {
        os_developer_options::instance->set_flag(mString { "LOAD_STRING_HASH_DICTIONARY" }, a1->get_bval());
        break;
    }
    case 16u: //LOG_RUNTIME_STRING_HASHES
    {
        os_developer_options::instance->set_flag(mString { "LOG_RUNTIME_STRING_HASHES" }, a1->get_bval());
        break;
    }
    case 17u: //SHOW_WATERMARK_VELOCITY
    {
        os_developer_options::instance->set_flag(mString { "SHOW_WATERMARK_VELOCITY" }, a1->get_bval());
        break;
    }
    case 18u: //SHOW_STATS
    {
        os_developer_options::instance->set_flag(mString { "SHOW_STATS" }, a1->get_bval());
        break;
    }
    case 19u: //ENABLE_ZOOM_MAP
    {
        os_developer_options::instance->set_flag(mString { "ENABLE_ZOOM_MAP" }, a1->get_bval());
        break;
    }
    case 20u: //SHOW_DEBUG_INFO
    {
        os_developer_options::instance->set_flag(mString { "SHOW_DEBUG_INFO" }, a1->get_bval());
        break;
    }
    case 21u: //SHOW_PROFILE_INFO
    {
        os_developer_options::instance->set_flag(mString { "SHOW_PROFILE_INFO" }, a1->get_bval());
        break;
    }
    case 22u: //SHOW_LOCOMOTION_INFO
    {
        os_developer_options::instance->set_flag(mString { "SHOW_LOCOMOTION_INFO" }, a1->get_bval());
        break;
    }
    case 23u: //GRAVITY
    {
        os_developer_options::instance->set_flag(mString { "GRAVITY" }, a1->get_bval());
        break;
    }
    case 24u: //TEST_MEMORY_LEAKS
    {
        os_developer_options::instance->set_flag(mString { "TEST_MEMORY_LEAKS" }, a1->get_bval());
        break;
    }
    case 25u: //HALT_ON_ASSERTS
    {
        os_developer_options::instance->set_flag(mString { "HALT_ON_ASSERTS" }, a1->get_bval());
        break;
    }
    case 26u: //SCREEN_ASSERTS
    {
        os_developer_options::instance->set_flag(mString { "SCREEN_ASSERTS" }, a1->get_bval());
        break;
    }
    case 27u: //NO_ANIM_WARNINGS
    {
        os_developer_options::instance->set_flag(mString { "NO_ANIM_WARNINGS" }, a1->get_bval());
        break;
    }
    case 28u: //PROFILING_ON
    {
        os_developer_options::instance->set_flag(mString { "PROFILING_ON" }, a1->get_bval());
        break;
    }
    case 29u: //MONO_AUDIO
    {
        os_developer_options::instance->set_flag(mString { "MONO_AUDIO" }, a1->get_bval());
        break;
    }
    case 30u: //NO_MESSAGES
    {
        os_developer_options::instance->set_flag(mString { "NO_MESSAGES" }, a1->get_bval());
        break;
    }
    case 31u: //LOCK_STEP
    {
        os_developer_options::instance->set_flag(mString { "LOCK_STEP" }, a1->get_bval());
        break;
    }
    case 32u: //TEXTURE_DUMP
    {
        os_developer_options::instance->set_flag(mString { "TEXTURE_DUMP" }, a1->get_bval());
        break;
    }
    case 33u: //DISABLE_SOUND_WARNINGS
    {
        os_developer_options::instance->set_flag(mString { "DISABLE_SOUND_WARNINGS" }, a1->get_bval());
        break;
    }
    case 34u: //DISABLE_SOUND_DEBUG_OUTPUT
    {
        os_developer_options::instance->set_flag(mString { "DISABLE_SOUND_DEBUG_OUTPUT" }, a1->get_bval());
        break;
    }
    case 35u: //DELETE_UNUSED_SOUND_BANKS_ON_PACK
    {
        os_developer_options::instance->set_flag(mString { "DELETE_UNUSED_SOUND_BANKS_ON_PACK" }, a1->get_bval());
        break;
    }
    case 36u: //LOCKED_HERO
    {
        os_developer_options::instance->set_flag(mString { "LOCKED_HERO" }, a1->get_bval());
        break;
    }
    case 37u: //FOG_OVERR IDE
    {
        os_developer_options::instance->set_flag(mString { "FOG_OVERR IDE" }, a1->get_bval());
        break;
    }
    case 38u: //FOG_DISABLE
    {
        os_developer_options::instance->set_flag(mString { "FOG_DISABLE" }, a1->get_bval());
        break;
    }
    case 39u: //MOVE_EDITOR
    {
        os_developer_options::instance->set_flag(mString { "MOVE_EDITOR" }, a1->get_bval());
        break;
    }
    case 40u: //AI_PATH_DEBUG
    {
        os_developer_options::instance->set_flag(mString { "AI_PATH_DEBUG" }, a1->get_bval());
        break;
    }
    case 41u: //AI_PATH_COLOR
    {
        os_developer_options::instance->set_flag(mString { "AI_PATH_COLOR" }, a1->get_bval());
        break;
    }
    case 42u: //AI_CRITTER_ACTIVITY
    {
        os_developer_options::instance->set_flag(mString { "AI_CRITTER_ACTIVITY" }, a1->get_bval());
        break;
    }
    case 43u: //AI_PATH_COLOR_CRITTER
    {
        os_developer_options::instance->set_flag(mString { "AI_PATH_COLOR_CRITTER" }, a1->get_bval());
        break;
    }
    case 44u: //AI_PATH_COLOR_HERO
    {
        os_developer_options::instance->set_flag(mString { "AI_PATH_COLOR_HERO" }, a1->get_bval());
        break;
    }
    case 45u: //NO_PARTICLES
    {
        os_developer_options::instance->set_flag(mString { "NO_PARTICLES" }, a1->get_bval());
        break;
    }
    case 46u: //NO_PARTICLE_PUMP
    {
        os_developer_options::instance->set_flag(mString { "NO_PARTICLE_PUMP" }, a1->get_bval());
        break;
    }
    case 47u: //SHOW_NORMALS
    {
        os_developer_options::instance->set_flag(mString { "SHOW_NORMALS" }, a1->get_bval());
        break;
    }
    case 48u: //SHOW_SHADOW_BALL
    {
        os_developer_options::instance->set_flag(mString { "SHOW_SHADOW_BALL" }, a1->get_bval());
        break;
    }
    case 49u: //SHOW_LIGHTS
    {
        os_developer_options::instance->set_flag(mString { "SHOW_LIGHTS" }, a1->get_bval());
        break;
    }
    case 50u: //SHOW_PLRCTRL
    {
        os_developer_options::instance->set_flag(mString { "SHOW_PLRCTRL" }, a1->get_bval());
        break;
    }
    case 51u: //SHOW_PSX_INFO
    {
        os_developer_options::instance->set_flag(mString { "SHOW_PSX_INFO" }, a1->get_bval());
        break;
    }
    case 52u: //FLAT_SHADE
    {
        os_developer_options::instance->set_flag(mString { "FLAT_SHADE" }, a1->get_bval());
        break;
    }
    case 53u: //INTERFACE_DISABLE
    {
        os_developer_options::instance->set_flag(mString { "INTERFACE_DISABLE" }, a1->get_bval());
        break;
    }
    case 54u: //WIDGET_TOOLS
    {
        os_developer_options::instance->set_flag(mString { "WIDGET_TOOLS" }, a1->get_bval());
        break;
    }
    case 55u: //LIGHTING
    {
        os_developer_options::instance->set_flag(mString { "LIGHTING" }, a1->get_bval());
        break;
    }
    case 56u: //FAKE_POINT_LIGHTS
    {
        os_developer_options::instance->set_flag(mString { "FAKE_POINT_LIGHTS" }, a1->get_bval());
        break;
    }
    case 57u: //BSP_SPRAY_PAINT
    {
        os_developer_options::instance->set_flag(mString { "BSP_SPRAY_PAINT" }, a1->get_bval());
        break;
    }
    case 58u: //CAMERA_EDITOR
    {
        os_developer_options::instance->set_flag(mString { "CAMERA_EDITOR" }, a1->get_bval());
        break;
    }
    case 59u: //IGNORE_RAMPING
    {
        os_developer_options::instance->set_flag(mString { "IGNORE_RAMPING" }, a1->get_bval());
        break;
    }
    case 60u: //POINT_SAMPLE
    {
        os_developer_options::instance->set_flag(mString { "POINT_SAMPLE" }, a1->get_bval());
        break;
    }
    case 61u: //DISTANCE_FADING
    {
        os_developer_options::instance->set_flag(mString { "DISTANCE_FADING" }, a1->get_bval());
        break;
    }
    case 62u: //OVERRIDE_CONTROLLER_OPTIONS
    {
        os_developer_options::instance->set_flag(mString { "OVERRIDE_CONTROLLER_OPTIONS" }, a1->get_bval());
        break;
    }
    case 63u: //DISABLE_MOUSE_PLAYER_CONTROL
    {
        os_developer_options::instance->set_flag(mString { "DISABLE_MOUSE_PLAYER_CONTROL" }, a1->get_bval());
        break;
    }
    case 64u: //NO_MOVIES
    {
        os_developer_options::instance->set_flag(mString { "NO_MOVIES" }, a1->get_bval());
        break;
    }
    case 65u: //XBOX_USER_CAM
    {
        os_developer_options::instance->set_flag(mString { "XBOX_USER_CAM" }, a1->get_bval());
        break;
    }
    case 66u: //NO_LOAD_SCREEN
    {
        os_developer_options::instance->set_flag(mString { "NO_LOAD_SCREEN" }, a1->get_bval());
        break;
    }
    case 67u: //EXCEPTION_HANDLER
    {
        os_developer_options::instance->set_flag(mString { "EXCEPTION_HANDLER" }, a1->get_bval());
        break;
    }
    case 68u: //NO_EXCEPTION_HANDLER
    {
        os_developer_options::instance->set_flag(mString { "NO_EXCEPTION_HANDLER" }, a1->get_bval());
        break;
    }
    case 69u: //NO_CD_CHECK
    {
        os_developer_options::instance->set_flag(mString { "NO_CD_CHECK" }, a1->get_bval());
        break;
    }
    case 70u: //NO_LOAD_METER
    {
        os_developer_options::instance->set_flag(mString { "NO_LOAD_METER" }, a1->get_bval());
        break;
    }
    case 71u: //NO_MOVIE_BUFFER_WARNING
    {
        os_developer_options::instance->set_flag(mString { "NO_MOVIE_BUFFER_WARNING" }, a1->get_bval());
        break;
    }
    case 72u: //LIMITED_EDITION_DISC
    {
        os_developer_options::instance->set_flag(mString { "LIMITED_EDITION_DISC" }, a1->get_bval());
        break;
    }
    case 73u: //NEW_COMBAT_LOCO
    {
        os_developer_options::instance->set_flag(mString { "NEW_COMBAT_LOCO" }, a1->get_bval());
        break;
    }
    case 74u: //LEVEL_WARP
    {
        os_developer_options::instance->set_flag(mString { "LEVEL_WARP" }, a1->get_bval());
        break;
    }
    case 75u: //SMOKE_TEST
    {
        os_developer_options::instance->set_flag(mString { "SMOKE_TEST" }, a1->get_bval());
        break;
    }
    case 76u: //SMOKE_TEST_LEVEL
    {
        os_developer_options::instance->set_flag(mString { "SMOKE_TEST_LEVEL" }, a1->get_bval());
        break;
    }
    case 77u: //COMBO_TESTER
    {
        os_developer_options::instance->set_flag(mString { "COMBO_TESTER" }, a1->get_bval());
        break;
    }
    case 78u: //DROP _SHADOWS_ALWAYS_RAYCAST
    {
        os_developer_options::instance->set_flag(mString { "DROP _SHADOWS_ALWAYS_RAYCAST" }, a1->get_bval());
        break;
    }
    case 79u: //DISABLE_DROP_SHADOWS
    {
        os_developer_options::instance->set_flag(mString { "DISABLE_DROP_SHADOWS" }, a1->get_bval());
        break;
    }
    case 80u: //DISABLE_HIRES_SHADOWS
    {
        os_developer_options::instance->set_flag(mString { "DISABLE_HIRES_SHADOWS" }, a1->get_bval());
        break;
    }
    case 81u: //DISABLE_STENCIL_SHADOWS
    {
        os_developer_options::instance->set_flag(mString { "DISABLE_STENCIL_SHADOWS" }, a1->get_bval());
        break;
    }
    case 82u: //DISABLE_COLORVOLS
    {
        os_developer_options::instance->set_flag(mString { "DISABLE_COLORVOLS" }, a1->get_bval());
        break;
    }
    case 83u: //DISABLE_RENDER_ENTS
    {
        os_developer_options::instance->set_flag(mString { "DISABLE_RENDER_ENTS" }, a1->get_bval());
        break;
    }
    case 84u: //DISABLE_FULLSCREEN_BLUR
    {
        os_developer_options::instance->set_flag(mString { "DISABLE_FULLSCREEN_BLUR" }, a1->get_bval());
        break;
    }
    case 85u: //FORCE_TIGHTCRAWL
    {
        os_developer_options::instance->set_flag(mString { "FORCE_TIGHTCRAWL" }, a1->get_bval());
        break;
    }
    case 86u: //FORCE_NONCRAWL
    {
        os_developer_options::instance->set_flag(mString { "FORCE_NONCRAWL" }, a1->get_bval());
        break;
    }
    case 87u: //SHOW_DEBUG_TEXT
    {
        os_developer_options::instance->set_flag(mString { "SHOW_DEBUG_TEXT" }, a1->get_bval());
        break;
    }
    case 88u: //SHOW_STYLE_POINTS
    {
        os_developer_options::instance->set_flag(mString { "SHOW_STYLE_POINTS" }, a1->get_bval());
        break;
    }
    case 89u: //CAMERA_MOUSE_MODE
    {
        os_developer_options::instance->set_flag(mString { "CAMERA_MOUSE_MODE" }, a1->get_bval());
        break;
    }
    case 90u: //USERCAM_ON_CONTROLLER2
    {
        os_developer_options::instance->set_flag(mString { "USERCAM_ON_CONTROLLER2" }, a1->get_bval());
        break;
    }
    case 91u: //DISABLE_ANCHOR_RETICLE_RENDERING
    {
        os_developer_options::instance->set_flag(mString { "DISABLE_ANCHOR_RETICLE_RENDERING" }, a1->get_bval());
        break;
    }
    case 92u: //SHOW_ANCHOR_LINE
    {
        os_developer_options::instance->set_flag(mString { "SHOW_ANCHOR_LINE" }, a1->get_bval());
        break;
    }
    case 93u: //FREE_SPIDER_REFLEXES
    {
        os_developer_options::instance->set_flag(mString { "FREE_SPIDER_REFLEXES" }, a1->get_bval());
        break;
    }
    case 94u: //SHOW_BAR_OF_SHAME
    {
        os_developer_options::instance->set_flag(mString { "SHOW_BAR_OF_SHAME" }, a1->get_bval());
        break;
    }
    case 95u: //SHOW_ENEMY_HEALTH_WIDGETS
    {
        os_developer_options::instance->set_flag(mString { "SHOW_ENEMY_HEALTH_WIDGETS" }, a1->get_bval());
        break;
    }
    case 96u: //ALLOW_IGC_PAUSE
    {
        os_developer_options::instance->set_flag(mString { "ALLOW_IGC_PAUSE" }, a1->get_bval());
        break;
    }
    case 97u: //SHOW_OBBS
    {
        os_developer_options::instance->set_flag(mString { "SHOW_OBBS" }, a1->get_bval());
        break;
    }
    case 98u: //SHOW_DISTRICTS
    {
        os_developer_options::instance->set_flag(mString { "SHOW_DISTRICTS" }, a1->get_bval());
        break;
    }
    case 99u: //SHOW_CHUCK_DEBUGGER
    {
        os_developer_options::instance->set_flag(mString { "SHOW_CHUCK_DEBUGGER" }, a1->get_bval());
        break;
    }
    case 100u: //CHUCK_OLD_FASHIONED
    {
        os_developer_options::instance->set_flag(mString { "CHUCK_OLD_FASHIONED" }, a1->get_bval());
        break;
    }
    case 101u: //CHUCK_DISABLE_BREAKPOINTS
    {
        os_developer_options::instance->set_flag(mString { "CHUCK_DISABLE_BREAKPOINTS" }, a1->get_bval());
        break;
    }
    case 102u: //SHOW_AUDIO_BOXES
    {
        os_developer_options::instance->set_flag(mString { "SHOW_AUDIO_BOXES" }, a1->get_bval());
        break;
    }
    case 103u: //DISABLE_SOUNDS
    {
        os_developer_options::instance->set_flag(mString { "DISABLE_SOUNDS" }, a1->get_bval());
        break;
    }
    case 104u: //SHOW_TERRAIN_INFO
    {
        os_developer_options::instance->set_flag(mString { "SHOW_TERRAIN_INFO" }, a1->get_bval());
        break;
    }
    case 105u: //DISABLE_AUDIO_BOXES
    {
        os_developer_options::instance->set_flag(mString { "DISABLE_AUDIO_BOXES" }, a1->get_bval());
        break;
    }
    case 106u: //NSL_OLD_FASHIONED
    {
        os_developer_options::instance->set_flag(mString { "NSL_OLD_FASHIONED" }, a1->get_bval());
        break;
    }
    case 107u: //SHOW_MASTER_CLOCK
    {
        os_developer_options::instance->set_flag(mString { "SHOW_MASTER_CLOCK" }, a1->get_bval());
        break;
    }
    case 108u: //LOAD_GRADIENTS
    {
        os_developer_options::instance->set_flag(mString { "LOAD_GRADIENTS" }, a1->get_bval());
        break;
    }
    case 109u: //BONUS_LEVELS_AVAILABLE
    {
        os_developer_options::instance->set_flag(mString { "BONUS_LEVELS_AVAILABLE" }, a1->get_bval());
        break;
    }
    case 110u: //COMBAT_DISPLAY
    {
        os_developer_options::instance->set_flag(mString { "COMBAT_DISPLAY" }, a1->get_bval());
        break;
    }
    case 111u: //COMBAT_DEBUGGER
    {
        os_developer_options::instance->set_flag(mString { "COMBAT_DEBUGGER" }, a1->get_bval());
        break;
    }
    case 112u: //ALLOW_ERROR_POPUPS
    {
        os_developer_options::instance->set_flag(mString { "ALLOW_ERROR_POPUPS" }, a1->get_bval());
        break;
    }
    case 113u: //ALLOW_WARNING_POPUPS
    {
        os_developer_options::instance->set_flag(mString { "ALLOW_WARNING_POPUPS" }, a1->get_bval());
        break;
    }
    case 114u: //OUTPUT_WARNING_DISABLE
    {
        os_developer_options::instance->set_flag(mString { "OUTPUT_WARNING_DISABLE" }, a1->get_bval());
        break;
    }
    case 115u: //OUTPUT_ASSERT_DISABLE
    {
        os_developer_options::instance->set_flag(mString { "OUTPUT_ASSERT_DISABLE" }, a1->get_bval());
        break;
    }
    case 116u: //ASSERT_ON_WARNING
    {
        os_developer_options::instance->set_flag(mString { "ASSERT_ON_WARNING" }, a1->get_bval());
        break;
    }
    case 117u: //ALWAYS_ACTIVE
    {
        os_developer_options::instance->set_flag(mString { "ALWAYS_ACTIVE" }, a1->get_bval());
        break;
    }
    case 118u: //FORCE_PROGRESSION
    {
        os_developer_options::instance->set_flag(mString { "FORCE_PROGRESSION" }, a1->get_bval());
        break;
    }
    case 119u: //LINK
    {
        os_developer_options::instance->set_flag(mString { "LINK" }, a1->get_bval());
        break;
    }
    case 120u: //WAIT_FOR_LINK
    {
        os_developer_options::instance->set_flag(mString { "WAIT_FOR_LINK" }, a1->get_bval());
        break;
    }
    case 121u: //SHOW_END_OF_PACK
    {
        os_developer_options::instance->set_flag(mString { "SHOW_END_OF_PACK" }, a1->get_bval());
        break;
    }
    case 122u: //LIVE_IN_GLASS_HOUSE
    {
        os_developer_options::instance->set_flag(mString { "LIVE_IN_GLASS_HOUSE" }, a1->get_bval());
        break;
    }
    case 123u: //SHOW_GLASS_HOUSE
    {
        os_developer_options::instance->set_flag(mString { "SHOW_GLASS_HOUSE" }, a1->get_bval());
        break;
    }
    case 124u: //DISABLE_RACE_PREVIEW
    {
        os_developer_options::instance->set_flag(mString { "DISABLE_RACE_PREVIEW" }, a1->get_bval());
        break;
    }
    case 125u: //FREE_MINI_MAP
    {
        os_developer_options::instance->set_flag(mString { "FREE_MINI_MAP" }, a1->get_bval());
        break;
    }
    case 126u: //SHOW_LOOPING_ANIM_WARNINGS
    {
        os_developer_options::instance->set_flag(mString { "SHOW_LOOPING_ANIM_WARNINGS" }, a1->get_bval());
        break;
    }
    case 127u: //SHOW_PERF_INFO
    {
        os_developer_options::instance->set_flag(mString { "SHOW_PERF_INFO" }, a1->get_bval());
        break;
    }
    case 128u: //COPY_ERROR_TO_CLIPBOARD
    {
        os_developer_options::instance->set_flag(mString { "COPY_ERROR_TO_CLIPBOARD" }, a1->get_bval());
        break;
    }
    case 129u: //BOTH_HANDS_UP_REORIENT
    {
        os_developer_options::instance->set_flag(mString { "BOTH_HANDS_UP_REORIENT" }, a1->get_bval());
        break;
    }
    case 130u: //SHOW_CAMERA_PROJECTION
    {
        os_developer_options::instance->set_flag(mString { "SHOW_CAMERA_PROJECTION" }, a1->get_bval());
        break;
    }
    case 131u: //OLD_DEFAULT_CONTROL_SETTINGS
    {
        os_developer_options::instance->set_flag(mString { "OLD_DEFAULT_CONTROL_SETTINGS" }, a1->get_bval());
        break;
    }
    case 132u: //IGC_SPEED_CONTROL
    {
        os_developer_options::instance->set_flag(mString { "IGC_SPEED_CONTROL" }, a1->get_bval());
        break;
    }
    case 133u: //RTDT_ENABLED
    {
        os_developer_options::instance->set_flag(mString { "RTDT_ENABLED" }, a1->get_bval());
        break;
    }
    case 134u: //ENABLE_DECALS
    {
        os_developer_options::instance->set_flag(mString { "ENABLE_DECALS" }, a1->get_bval());
        break;
    }
    case 135u: //AUTO_STICK_TO_WALLS
    {
        os_developer_options::instance->set_flag(mString { "AUTO_STICK_TO_WALLS" }, a1->get_bval());
        break;
    }
    case 136u: //ENABLE_PEDESTRIANS
    {
        os_developer_options::instance->set_flag(mString { "ENABLE_PEDESTRIANS" }, a1->get_bval());
        break;
    }
    case 137u: //CAMERA_INVERT_LOOKAROUND
    {
        os_developer_options::instance->set_flag(mString { "CAMERA_INVERT_LOOKAROUND" }, a1->get_bval());
        break;
    }
    case 138u: //CAMERA_INVERT_LOOKAROUND_X
    {
        os_developer_options::instance->set_flag(mString { "CAMERA_INVERT_LOOKAROUND_X" }, a1->get_bval());
        break;
    }
    case 139u: //CAMERA_INVERT_LOOKAROUND_Y
    {
        os_developer_options::instance->set_flag(mString { "CAMERA_INVERT_LOOKAROUND_Y" }, a1->get_bval());
        break;
    }
    case 140u: //FORCE_COMBAT_CAMERA_OFF
    {
        os_developer_options::instance->set_flag(mString { "FORCE_COMBAT_CAMERA_OFF" }, a1->get_bval());
        break;
    }
    case 141u: //DISPLAY_THOUGHT_BUBBLES
    {
        os_developer_options::instance->set_flag(mString { "DISPLAY_THOUGHT_BUBBLES" }, a1->get_bval());
        break;
    }
    case 142u: //ENABLE_DEBUG_LOG
    {
        os_developer_options::instance->set_flag(mString { "ENABLE_DEBUG_LOG" }, a1->get_bval());
        break;
    }
    case 143u: //ENABLE_LONG_CALLSTACK
    {
        os_developer_options::instance->set_flag(mString { "ENABLE_LONG_CALLSTACK" }, a1->get_bval());
        break;
    }
    case 144u: //RENDER_FE_UI
    {
        os_developer_options::instance->set_flag(mString { "RENDER_FE_UI" }, a1->get_bval());
        break;
    }
    case 145u: //LOCK_INTERIORS
    {
        os_developer_options::instance->set_flag(mString { "LOCK_INTERIORS" }, a1->get_bval());
        break;
    }
    case 146u: //MEMCHECK_ON_LOAD
    {
        os_developer_options::instance->set_flag(mString { "MEMCHECK_ON_LOAD" }, a1->get_bval());
        break;
    }
    case 147u: //DISPLAY_ALS_USAGE_PROFILE
    {
        os_developer_options::instance->set_flag(mString { "DISPLAY_ALS_USAGE_PROFILE" }, a1->get_bval());
        break;
    }
    case 148u: //ENABLE_FPU_EXCEPTION_HANDLING
    {
        os_developer_options::instance->set_flag(mString { "ENABLE_FPU_EXCEPTION_HANDLING" }, false);
        break;
    }
    case 149u: //UNLOCK_ALL_UNLOCKABLES
    {
        os_developer_options::instance->set_flag(mString { "UNLOCK_ALL_UNLOCKABLES" }, a1->get_bval());
        break;
    }
    default:
        return;
    }
}



void create_devopt_menu(debug_menu* parent)
{
    assert(parent != nullptr);
 
    auto* game_menu = create_menu("Devopts");
    auto* v92       = game_menu;
    auto* v4        = create_menu_entry(game_menu);
 
    parent->add_entry(v4);
 
    debug_menu_entry v89;
 
    // === BEGIN GENERATED FLAG ENTRIES (150 total, indices 0..149) =========
    // Generated from os_developer_options.cpp::g_flag_names; do not hand-edit
    // individual entries — regenerate the whole block if a flag is added or
    // renamed in g_flag_names. Two flag names contain embedded spaces
    // ("FOG_OVERR IDE", "DROP _SHADOWS_ALWAYS_RAYCAST"); these are typos in
    // the original game data preserved verbatim because flag lookups are
    // exact-string matches.
 
    v89 = debug_menu_entry(mString{ "CD_ONLY" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "CD_ONLY" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(0);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "ENVMAP_TOOL" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "ENVMAP_TOOL" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(1);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "NO_CD" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "NO_CD" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(2);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "CHATTY_LOAD" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "CHATTY_LOAD" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(3);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "WINDOW_DEFAULT" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "WINDOW_DEFAULT" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(4);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "SHOW_FPS" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "SHOW_FPS" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(5);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "SHOW_STREAMER_INFO" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "SHOW_STREAMER_INFO" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(6);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "SHOW_STREAMER_SPAM" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "SHOW_STREAMER_SPAM" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(7);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "SHOW_RESOURCE_SPAM" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "SHOW_RESOURCE_SPAM" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(8);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "SHOW_MEMORY_INFO" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "SHOW_MEMORY_INFO" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(9);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "SHOW_SPIDEY_SPEED" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "SHOW_SPIDEY_SPEED" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(10);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "TRACE_MISSION_MANAGER" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "TRACE_MISSION_MANAGER" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(11);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "DUMP_MISSION_HEAP" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "DUMP_MISSION_HEAP" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(12);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "CAMERA_CENTRIC_STREAMER" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "CAMERA_CENTRIC_STREAMER" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(13);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "RENDER_LOWLODS" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "RENDER_LOWLODS" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(14);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "LOAD_STRING_HASH_DICTIONARY" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "LOAD_STRING_HASH_DICTIONARY" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(15);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "LOG_RUNTIME_STRING_HASHES" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "LOG_RUNTIME_STRING_HASHES" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(16);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "SHOW_WATERMARK_VELOCITY" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "SHOW_WATERMARK_VELOCITY" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(17);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "SHOW_STATS" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "SHOW_STATS" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(18);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "ENABLE_ZOOM_MAP" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "ENABLE_ZOOM_MAP" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(19);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "SHOW_DEBUG_INFO" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "SHOW_DEBUG_INFO" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(20);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "SHOW_PROFILE_INFO" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "SHOW_PROFILE_INFO" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(21);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "SHOW_LOCOMOTION_INFO" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "SHOW_LOCOMOTION_INFO" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(22);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "GRAVITY" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "GRAVITY" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(23);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "TEST_MEMORY_LEAKS" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "TEST_MEMORY_LEAKS" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(24);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "HALT_ON_ASSERTS" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "HALT_ON_ASSERTS" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(25);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "SCREEN_ASSERTS" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "SCREEN_ASSERTS" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(26);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "NO_ANIM_WARNINGS" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "NO_ANIM_WARNINGS" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(27);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "PROFILING_ON" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "PROFILING_ON" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(28);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "MONO_AUDIO" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "MONO_AUDIO" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(29);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "NO_MESSAGES" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "NO_MESSAGES" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(30);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "LOCK_STEP" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "LOCK_STEP" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(31);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "TEXTURE_DUMP" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "TEXTURE_DUMP" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(32);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "DISABLE_SOUND_WARNINGS" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "DISABLE_SOUND_WARNINGS" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(33);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "DISABLE_SOUND_DEBUG_OUTPUT" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "DISABLE_SOUND_DEBUG_OUTPUT" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(34);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "DELETE_UNUSED_SOUND_BANKS_ON_PACK" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "DELETE_UNUSED_SOUND_BANKS_ON_PACK" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(35);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "LOCKED_HERO" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "LOCKED_HERO" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(36);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "FOG_OVERR IDE" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "FOG_OVERR IDE" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(37);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "FOG_DISABLE" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "FOG_DISABLE" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(38);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "MOVE_EDITOR" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "MOVE_EDITOR" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(39);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "AI_PATH_DEBUG" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "AI_PATH_DEBUG" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(40);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "AI_PATH_COLOR" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "AI_PATH_COLOR" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(41);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "AI_CRITTER_ACTIVITY" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "AI_CRITTER_ACTIVITY" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(42);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "AI_PATH_COLOR_CRITTER" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "AI_PATH_COLOR_CRITTER" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(43);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "AI_PATH_COLOR_HERO" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "AI_PATH_COLOR_HERO" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(44);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "NO_PARTICLES" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "NO_PARTICLES" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(45);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "NO_PARTICLE_PUMP" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "NO_PARTICLE_PUMP" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(46);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "SHOW_NORMALS" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "SHOW_NORMALS" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(47);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "SHOW_SHADOW_BALL" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "SHOW_SHADOW_BALL" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(48);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "SHOW_LIGHTS" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "SHOW_LIGHTS" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(49);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "SHOW_PLRCTRL" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "SHOW_PLRCTRL" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(50);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "SHOW_PSX_INFO" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "SHOW_PSX_INFO" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(51);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "FLAT_SHADE" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "FLAT_SHADE" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(52);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "INTERFACE_DISABLE" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "INTERFACE_DISABLE" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(53);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "WIDGET_TOOLS" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "WIDGET_TOOLS" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(54);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "LIGHTING" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "LIGHTING" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(55);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "FAKE_POINT_LIGHTS" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "FAKE_POINT_LIGHTS" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(56);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "BSP_SPRAY_PAINT" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "BSP_SPRAY_PAINT" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(57);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "CAMERA_EDITOR" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "CAMERA_EDITOR" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(58);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "IGNORE_RAMPING" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "IGNORE_RAMPING" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(59);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "POINT_SAMPLE" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "POINT_SAMPLE" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(60);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "DISTANCE_FADING" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "DISTANCE_FADING" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(61);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "OVERRIDE_CONTROLLER_OPTIONS" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "OVERRIDE_CONTROLLER_OPTIONS" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(62);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "DISABLE_MOUSE_PLAYER_CONTROL" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "DISABLE_MOUSE_PLAYER_CONTROL" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(63);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "NO_MOVIES" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "NO_MOVIES" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(64);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "XBOX_USER_CAM" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "XBOX_USER_CAM" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(65);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "NO_LOAD_SCREEN" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "NO_LOAD_SCREEN" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(66);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "EXCEPTION_HANDLER" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "EXCEPTION_HANDLER" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(67);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "NO_EXCEPTION_HANDLER" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "NO_EXCEPTION_HANDLER" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(68);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "NO_CD_CHECK" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "NO_CD_CHECK" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(69);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "NO_LOAD_METER" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "NO_LOAD_METER" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(70);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "NO_MOVIE_BUFFER_WARNING" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "NO_MOVIE_BUFFER_WARNING" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(71);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "LIMITED_EDITION_DISC" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "LIMITED_EDITION_DISC" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(72);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "NEW_COMBAT_LOCO" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "NEW_COMBAT_LOCO" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(73);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "LEVEL_WARP" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "LEVEL_WARP" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(74);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "SMOKE_TEST" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "SMOKE_TEST" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(75);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "SMOKE_TEST_LEVEL" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "SMOKE_TEST_LEVEL" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(76);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "COMBO_TESTER" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "COMBO_TESTER" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(77);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "DROP _SHADOWS_ALWAYS_RAYCAST" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "DROP _SHADOWS_ALWAYS_RAYCAST" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(78);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "DISABLE_DROP_SHADOWS" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "DISABLE_DROP_SHADOWS" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(79);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "DISABLE_HIRES_SHADOWS" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "DISABLE_HIRES_SHADOWS" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(80);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "DISABLE_STENCIL_SHADOWS" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "DISABLE_STENCIL_SHADOWS" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(81);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "DISABLE_COLORVOLS" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "DISABLE_COLORVOLS" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(82);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "DISABLE_RENDER_ENTS" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "DISABLE_RENDER_ENTS" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(83);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "DISABLE_FULLSCREEN_BLUR" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "DISABLE_FULLSCREEN_BLUR" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(84);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "FORCE_TIGHTCRAWL" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "FORCE_TIGHTCRAWL" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(85);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "FORCE_NONCRAWL" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "FORCE_NONCRAWL" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(86);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "SHOW_DEBUG_TEXT" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "SHOW_DEBUG_TEXT" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(87);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "SHOW_STYLE_POINTS" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "SHOW_STYLE_POINTS" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(88);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "CAMERA_MOUSE_MODE" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "CAMERA_MOUSE_MODE" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(89);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "USERCAM_ON_CONTROLLER2" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "USERCAM_ON_CONTROLLER2" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(90);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "DISABLE_ANCHOR_RETICLE_RENDERING" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "DISABLE_ANCHOR_RETICLE_RENDERING" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(91);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "SHOW_ANCHOR_LINE" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "SHOW_ANCHOR_LINE" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(92);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "FREE_SPIDER_REFLEXES" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "FREE_SPIDER_REFLEXES" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(93);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "SHOW_BAR_OF_SHAME" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "SHOW_BAR_OF_SHAME" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(94);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "SHOW_ENEMY_HEALTH_WIDGETS" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "SHOW_ENEMY_HEALTH_WIDGETS" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(95);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "ALLOW_IGC_PAUSE" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "ALLOW_IGC_PAUSE" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(96);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "SHOW_OBBS" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "SHOW_OBBS" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(97);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "SHOW_DISTRICTS" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "SHOW_DISTRICTS" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(98);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "SHOW_CHUCK_DEBUGGER" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "SHOW_CHUCK_DEBUGGER" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(99);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "CHUCK_OLD_FASHIONED" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "CHUCK_OLD_FASHIONED" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(100);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "CHUCK_DISABLE_BREAKPOINTS" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "CHUCK_DISABLE_BREAKPOINTS" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(101);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "SHOW_AUDIO_BOXES" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "SHOW_AUDIO_BOXES" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(102);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "DISABLE_SOUNDS" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "DISABLE_SOUNDS" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(103);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "SHOW_TERRAIN_INFO" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "SHOW_TERRAIN_INFO" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(104);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "DISABLE_AUDIO_BOXES" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "DISABLE_AUDIO_BOXES" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(105);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "NSL_OLD_FASHIONED" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "NSL_OLD_FASHIONED" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(106);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "SHOW_MASTER_CLOCK" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "SHOW_MASTER_CLOCK" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(107);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "LOAD_GRADIENTS" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "LOAD_GRADIENTS" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(108);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "BONUS_LEVELS_AVAILABLE" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "BONUS_LEVELS_AVAILABLE" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(109);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "COMBAT_DISPLAY" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "COMBAT_DISPLAY" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(110);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "COMBAT_DEBUGGER" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "COMBAT_DEBUGGER" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(111);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "ALLOW_ERROR_POPUPS" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "ALLOW_ERROR_POPUPS" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(112);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "ALLOW_WARNING_POPUPS" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "ALLOW_WARNING_POPUPS" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(113);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "OUTPUT_WARNING_DISABLE" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "OUTPUT_WARNING_DISABLE" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(114);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "OUTPUT_ASSERT_DISABLE" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "OUTPUT_ASSERT_DISABLE" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(115);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "ASSERT_ON_WARNING" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "ASSERT_ON_WARNING" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(116);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "ALWAYS_ACTIVE" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "ALWAYS_ACTIVE" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(117);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "FORCE_PROGRESSION" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "FORCE_PROGRESSION" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(118);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "LINK" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "LINK" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(119);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "WAIT_FOR_LINK" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "WAIT_FOR_LINK" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(120);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "SHOW_END_OF_PACK" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "SHOW_END_OF_PACK" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(121);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "LIVE_IN_GLASS_HOUSE" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "LIVE_IN_GLASS_HOUSE" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(122);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "SHOW_GLASS_HOUSE" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "SHOW_GLASS_HOUSE" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(123);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "DISABLE_RACE_PREVIEW" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "DISABLE_RACE_PREVIEW" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(124);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "FREE_MINI_MAP" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "FREE_MINI_MAP" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(125);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "SHOW_LOOPING_ANIM_WARNINGS" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "SHOW_LOOPING_ANIM_WARNINGS" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(126);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "SHOW_PERF_INFO" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "SHOW_PERF_INFO" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(127);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "COPY_ERROR_TO_CLIPBOARD" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "COPY_ERROR_TO_CLIPBOARD" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(128);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "BOTH_HANDS_UP_REORIENT" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "BOTH_HANDS_UP_REORIENT" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(129);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "SHOW_CAMERA_PROJECTION" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "SHOW_CAMERA_PROJECTION" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(130);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "OLD_DEFAULT_CONTROL_SETTINGS" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "OLD_DEFAULT_CONTROL_SETTINGS" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(131);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "IGC_SPEED_CONTROL" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "IGC_SPEED_CONTROL" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(132);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "RTDT_ENABLED" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "RTDT_ENABLED" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(133);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "ENABLE_DECALS" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "ENABLE_DECALS" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(134);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "AUTO_STICK_TO_WALLS" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "AUTO_STICK_TO_WALLS" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(135);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "ENABLE_PEDESTRIANS" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "ENABLE_PEDESTRIANS" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(136);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "CAMERA_INVERT_LOOKAROUND" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "CAMERA_INVERT_LOOKAROUND" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(137);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "CAMERA_INVERT_LOOKAROUND_X" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "CAMERA_INVERT_LOOKAROUND_X" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(138);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "CAMERA_INVERT_LOOKAROUND_Y" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "CAMERA_INVERT_LOOKAROUND_Y" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(139);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "FORCE_COMBAT_CAMERA_OFF" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "FORCE_COMBAT_CAMERA_OFF" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(140);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "DISPLAY_THOUGHT_BUBBLES" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "DISPLAY_THOUGHT_BUBBLES" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(141);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "ENABLE_DEBUG_LOG" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "ENABLE_DEBUG_LOG" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(142);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "ENABLE_LONG_CALLSTACK" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "ENABLE_LONG_CALLSTACK" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(143);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "RENDER_FE_UI" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "RENDER_FE_UI" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(144);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "LOCK_INTERIORS" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "LOCK_INTERIORS" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(145);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "MEMCHECK_ON_LOAD" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "MEMCHECK_ON_LOAD" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(146);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "DISPLAY_ALS_USAGE_PROFILE" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "DISPLAY_ALS_USAGE_PROFILE" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(147);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "ENABLE_FPU_EXCEPTION_HANDLING" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "ENABLE_FPU_EXCEPTION_HANDLING" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(148);
    v92->add_entry(&v89);
 
    v89 = debug_menu_entry(mString{ "UNLOCK_ALL_UNLOCKABLES" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "UNLOCK_ALL_UNLOCKABLES" }));
    v89.set_game_flags_handler(devopt_flags_handler);
    v89.set_id(149);
    v92->add_entry(&v89);
    // === END GENERATED FLAG ENTRIES ======================================
 
    // Trailing INT_OPTION pass — picks up integer-typed devopts (the bool
    // ones are already covered by the 150-entry block above).
    for (auto idx = 0u; idx < NUM_OPTIONS; ++idx)
    {
        auto* v21 = get_option(idx);
        switch (v21->m_type)
        {
        case game_option_t::INT_OPTION:
        {
            v89 = debug_menu_entry(mString{ v21->m_name });
            v89.set_p_ival(v21->m_value.p_ival);
            v89.set_min_value(-1000.0);
            v89.set_max_value(1000.0);
            v92->add_entry(&v89);
            break;
        }
        default:
            break;
        }
    }
}
#include "timer.h"

    void slow_motion_handler(debug_menu_entry* a1)
    {
        switch (a1->get_bval()) {
        case 0u:
		os_developer_options::instance->set_int(mString{ "FRAME_LOCK" } ,false);			
        g_timer()->normal_motion();
		debug_menu::hide();
            break;
        case 1u:
        os_developer_options::instance->set_int(mString{ "FRAME_LOCK" } ,true);			
        g_timer()->slow_motion();
		debug_menu::hide();
            break;

    }
	
	    }
		
		

void create_game_flags_menu(debug_menu* parent);



void create_game_flags_menu(debug_menu *parent)
{
    assert(parent != nullptr);
    
    auto *v3 =  create_menu("Game", handle_game_entry, 300);
	
		debug_menu_entry v5;
   debug_menu_entry* block = v5.alloc_block(v3, 4);
   block[0] = debug_menu_entry{ v3 };
	
	    auto* game_menu = create_menu("Game");
		    auto *v92 = game_menu;
    auto* v4 = create_menu_entry(game_menu);

    parent->add_entry(v4);

    debug_menu_entry v89;




    v89 = debug_menu_entry(mString{ "Report SLF Recall Timeouts" });
    static bool byte_1597BC0 = false;
    v89.set_pt_bval(&byte_1597BC0);
    v92->add_entry(&v89);


    v89 = debug_menu_entry(mString{ "Physics Enabled" });
    v89.set_bval(true);
    v89.set_game_flags_handler(game_flags_handler);
    v89.set_id(0);
    v92->add_entry(&v89);

    v89 = debug_menu_entry(mString{ "Single Step" });
    v89.set_game_flags_handler(game_flags_handler);
    v89.set_id(1);
    v92->add_entry(&v89);

    v89 = debug_menu_entry(mString{ "Slow Motion Enabled" });
    v89.set_bval(false);
    v89.set_game_flags_handler(slow_motion_handler);
    v92->add_entry(&v89);


    v89 = debug_menu_entry{ mString{"Monkey Enabled"} };

    auto v1 = spider_monkey::is_running();
    v89.set_bval(v1);
    v89.set_game_flags_handler(game_flags_handler);
    v89.set_id(3);
    v92->add_entry(&v89);

    v89 = debug_menu_entry{ mString{"Rumble Enabled"} };
    v89.set_bval(true);
    v89.set_game_flags_handler(game_flags_handler);
    v89.set_id(4);
    v92->add_entry(&v89);

    v89 = debug_menu_entry(mString{ "God Mode" });
    v89.set_ival(os_developer_options::instance->get_int(mString{ "GOD_MODE" }));

    const float v2[4] = { 0, 5, 1, 1 };
    v89.set_fl_values(v2);
    v89.set_game_flags_handler(game_flags_handler);
    v89.set_id(5);
    v92->add_entry(&v89);

    v89 = debug_menu_entry(mString{ "Show Districts" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "SHOW_STREAMER_INFO" }));
    v89.set_game_flags_handler(game_flags_handler);
    v89.set_id(6);
    v92->add_entry(&v89);

    v89 = debug_menu_entry(mString{ "Show Hero Position" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "SHOW_DEBUG_INFO" }));
    v89.set_game_flags_handler(game_flags_handler);
    v89.set_id(7);
    v92->add_entry(&v89);

    v89 = debug_menu_entry(mString{ "Show FPS" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "SHOW_FPS" }));
    v89.set_game_flags_handler(game_flags_handler);
    v89.set_id(8);
    v92->add_entry(&v89);

    v89 = debug_menu_entry(mString{ "User Camera on Controller 2" });
    v89.set_bval(os_developer_options::instance->get_flag(mString{ "USERCAM_ON_CONTROLLER2" }));
    v89.set_game_flags_handler(game_flags_handler);
    v89.set_id(9);
    v92->add_entry(&v89);

    v89 = debug_menu_entry(mString{ "Toggle Unload All Districts" });
    v89.set_game_flags_handler(game_flags_handler);
    v89.set_id(13);
    v92->add_entry(&v89);

    {
        auto* v88 = create_menu("Save/Load", handle_game_entry, 10);
        auto v18 = debug_menu_entry(v88);
        v92->add_entry(&v18);

        v89 = debug_menu_entry(mString{ "Save Game" });
        v89.set_game_flags_handler(game_flags_handler);
        v89.set_id(14);
        v88->add_entry(&v89);

        v89 = debug_menu_entry(mString{ "Load Game" });
        v89.set_game_flags_handler(game_flags_handler);
        v89.set_id(15);
        v88->add_entry(&v89);

        v89 = debug_menu_entry(mString{ "Attemp Auto Load" });
        v89.set_game_flags_handler(game_flags_handler);
        v89.set_id(16);
        v88->add_entry(&v89);
    }

    {
        auto* v87 = create_menu("Screenshot", handle_game_entry, 10);
        auto v23 = debug_menu_entry(v87);
        v92->add_entry(&v23);

        v89 = debug_menu_entry(mString{ "Hires Screenshot" });
        v89.set_game_flags_handler(game_flags_handler);
        v89.set_id(11);
        v87->add_entry(&v89);

        v89 = debug_menu_entry(mString{ "Lores Screenshot" });
        v89.set_game_flags_handler(game_flags_handler);
        v89.set_id(12);
        v87->add_entry(&v89);
    }

    create_devopt_menu(v92);
    create_gamefile_menu(v92);
}







void create_game_flags_menu(debug_menu* parent);

// Character List (Script > Pop Character / Pop All Characters)
// ----------------------------------------------------------------------------------
// Implements the debug menu character spawning functionality:
//   - "Pop Character"     : spawns the currently selected character near the hero
//   - "Pop All Characters": spawns every character in the list near the hero
//   - [character names]   : selecting one directly spawns that character
//
// Characters are spawned via wds_entity_manager::create_and_add_entity_or_subclass
// at the hero's current position with a small offset.
// ----------------------------------------------------------------------------------


#include "entity_base.h"
#include "entity.h"
#include "oldmath_po.h"
#include "vector3d.h"
#include "wds.h"
#include "terrain.h"
#include "string_hash.h"
#include "mstring.h"

static const char *g_character_names[] = {
    "alex_ohirn",
    "alex_ohirn_prison",
    "arachno_man",
    "beetle",
    "bolivar_trask_old",
    "boomerang",
    "carnage",
    "eddie_brock",
    "eddie_brock_sr",
    "electro_naked",
    "electro_nosuit",
    "electro_suit",
    "gang_ftb_base",
    "gang_ftb_boss",
    "gang_ftb_lt",
    "gang_mercs_base",
    "gang_mercs_boss",
    "gang_mercs_lt",
    "gang_skin_base",
    "gang_skin_boss",
    "gang_skin_lt",
    "gang_srk_base",
    "gang_srk_boss",
    "gang_srk_lt",
    "gmu_businessman",
    "gmu_child_male",
    "gmu_cop_fat",
    "gmu_cop_thin",
    "gmu_cop_thin_igc",
    "gmu_jogger_fem",
    "gmu_lab_fem",
    "gmu_man",
    "gmu_medic_fem",
    "gmu_sable_merc",
    "gmu_storeowner",
    "gmu_street_vendor",
    "gmu_student_female",
    "gmu_student_male",
    "gmu_utilityworker_male",
    "gmu_woman",
    "johnny_storm",
    "kong",
    "liz_allen",
    "mary_jane",
    "nick_fury",
    "origin_spider",
    "ped_fem",
    "ped_male",
    "peter_hooded",
    "peter_parker",
    "peter_shirtless",
    "rhino",
    "rhino_igc",
    "richard_parker",
    "sable",
    "sharon_carter",
    "shield_agent",
    "shield_agent_jetpack",
    "shield_agent_test",
    "shocker",
    "toomes",
    "usm_blacksuit",
    "usm_peterhead",
    "usm_venomhand",
    "usm_wrestling",
    "venom_eddie",
    "venom_spider",
    "venom_viewer",
    "wolverine",
};

static constexpr int g_character_count = sizeof(g_character_names) / sizeof(g_character_names[0]);

// Tracks which character entry was last highlighted/selected in the list
static int g_selected_character_index = 0;

// Spawns a single character by name at the hero's position + offset
static entity *spawn_character_at_hero(const char *char_name, float offset_x = 2.0f, float offset_z = 0.0f)
{
    if (g_world_ptr == nullptr) {
        return nullptr;
    }

    auto *hero = g_world_ptr->get_hero_ptr(0);
    if (hero == nullptr) {
        return nullptr;
    }

    // Build spawn transform from hero's current position
    po spawn_po {identity_matrix};
    auto &hero_po = hero->get_abs_po();
    spawn_po = hero_po;

    // Offset the spawn position so the character doesn't overlap the hero
    auto hero_pos = hero->get_abs_position();
    vector3d spawn_pos = hero_pos;
    spawn_pos[0] += offset_x;
    spawn_pos[2] += offset_z;
    spawn_po.set_position(spawn_pos);

    // Create entity using type_hash == id_hash (same name for both)
    string_hash type_hash {char_name};
    string_hash id_hash {char_name};
    mString empty_str {};

    auto *ent = g_world_ptr->ent_mgr.create_and_add_entity_or_subclass(
        type_hash,
        id_hash,
        spawn_po,
        empty_str,
        129,
        nullptr);

    if (ent != nullptr) {
        ent->set_visible(true, false);

        auto *the_terrain = g_world_ptr->get_the_terrain();
        if (the_terrain != nullptr) {
            ent->compute_sector(the_terrain, 0, ent);
        }

        printf("[CharList] Spawned character: %s\n", char_name);
    } else {
        printf("[CharList] Failed to spawn character: %s\n", char_name);
    }

    return ent;
}

// Handler: spawn a single character when its menu entry is selected
static void character_select_handler(debug_menu_entry *entry)
{
    if (entry == nullptr) {
        return;
    }

    // entry->text contains the character name
    const char *char_name = entry->text;

    // Update the selected index based on the entry's ID
    g_selected_character_index = entry->get_id();

    spawn_character_at_hero(char_name, 2.0f, 0.0f);
    debug_menu::hide();
}

// Handler: "Pop Character" — spawns the last selected/highlighted character
static void pop_character_handler(debug_menu_entry *entry)
{
    if (g_selected_character_index >= 0 && g_selected_character_index < g_character_count) {
        const char *char_name = g_character_names[g_selected_character_index];
        spawn_character_at_hero(char_name, 2.0f, 0.0f);
    }

    debug_menu::hide();
}

// Handler: "Pop All Characters" — spawns every character in the list
static void pop_all_characters_handler(debug_menu_entry *entry)
{
    // Arrange characters in a grid pattern around the hero
    float offset_x = 3.0f;
    float offset_z = 0.0f;
    const float spacing = 3.0f;
    const int chars_per_row = 8;

    for (int i = 0; i < g_character_count; ++i) {
        float row = static_cast<float>(i / chars_per_row);
        float col = static_cast<float>(i % chars_per_row);

        float ox = (col - chars_per_row / 2.0f) * spacing;
        float oz = (row + 1.0f) * spacing;

        spawn_character_at_hero(g_character_names[i], ox, oz);
    }

    debug_menu::hide();
}

// Populates the character list submenu with Pop Character, Pop All, and all names
static void populate_character_list_menu(debug_menu_entry *entry)
{
    auto *submenu = create_menu("Script", debug_menu::sort_mode_t::ascending);

    debug_menu_entry v1;
    debug_menu_entry *block = v1.alloc_block(submenu, 4);
    block[0] = debug_menu_entry{submenu};
    entry->set_submenu(submenu);

    // --- "Pop Character" entry ---
    {
        auto *pop_entry = create_menu_entry(mString{"Pop Character"});
        pop_entry->set_game_flags_handler(pop_character_handler);
        pop_entry->set_id(0);
        submenu->add_entry(pop_entry);
    }

    // --- "Pop All Characters" entry ---
    {
        auto *pop_all_entry = create_menu_entry(mString{"Pop All Characters"});
        pop_all_entry->set_game_flags_handler(pop_all_characters_handler);
        pop_all_entry->set_id(1);
        submenu->add_entry(pop_all_entry);
    }

    // --- Individual character entries ---
    for (int i = 0; i < g_character_count; ++i) {
        auto *char_entry = create_menu_entry(mString{g_character_names[i]});
        char_entry->set_game_flags_handler(character_select_handler);
        char_entry->set_id(static_cast<uint16_t>(i));
        submenu->add_entry(char_entry);
    }
}

// Creates the "Script" submenu and adds it to the parent (root) debug menu
void create_character_list_menu(debug_menu *parent)
{
    auto *char_list_menu = create_menu("Script");

    debug_menu_entry v1;
    debug_menu_entry *block = v1.alloc_block(char_list_menu, 4);
    block[0] = debug_menu_entry{char_list_menu};

    auto *entry = create_menu_entry(char_list_menu);
    entry->set_submenu(nullptr);
    entry->set_game_flags_handler(populate_character_list_menu);
    parent->add_entry(entry);
}




void create_script_menu()
{
    if ( script_menu == nullptr )
    {
        script_menu =  create_menu ("Script", debug_menu::sort_mode_t::undefined);
        debug_menu_entry v1;
        debug_menu_entry* block = v1.alloc_block(script_menu, 4);
        block[0] = debug_menu_entry{ script_menu };
        debug_menu::root_menu->add_entry(script_menu);
		  create_character_list_menu(script_menu);
    }
}




#pragma endregion

// Debug Menu
// ----------------------------------------------------------------------------------

void debug_menu::init() {
    root_menu = create_menu("Debug Menu", handle_debug_entry, 10);

    progression_menu = create_menu("Progression");
    level_select_menu = create_menu("Level Select");

    debug_menu_entry progression_entry{ progression_menu };
    debug_menu_entry level_select_entry{ level_select_menu };
    
			            debug_menu_entry v0;
    debug_menu_entry* block0 = v0.alloc_block(root_menu, 4);
    block0[0] = debug_menu_entry { root_menu };
		debug_menu_entry v1;
   debug_menu_entry* block = v1.alloc_block(level_select_menu, 4);
   block[0] = debug_menu_entry{ level_select_menu };
   

   		debug_menu_entry v3;
   debug_menu_entry* block2 = v3.alloc_block(progression_menu, 4);
   block2[0] = debug_menu_entry{ progression_menu };
   

	
	    create_dvars_menu(root_menu);
    create_warp_menu(root_menu);
	create_game_flags_menu(root_menu);
    create_missions_menu(root_menu);
	create_debug_render_menu(root_menu);
    create_debug_district_variants_menu(root_menu);
#   ifdef TARGET_XBOX
        create_replay_menu(root_menu);
#   endif
	create_ai_root_menu(root_menu);
	create_memory_menu(root_menu);
    create_entity_variants_menu(root_menu);
	create_entity_animation_menu(root_menu);
	add_debug_menu_entry(root_menu, &level_select_entry);
	create_script_menu();
    add_debug_menu_entry(root_menu, &progression_entry);
	create_camera_menu_items(root_menu);




    /*
    for (int i = 0; i < 5; i++) {

        debug_menu_entry asdf;
        sprintf(asdf.text, "entry %d", i);
        printf("AQUI %s\n", asdf.text);

        add_debug_menu_entry(debug_menu::root_menu, &asdf);
    }
    add_debug_menu_entry(debug_menu::root_menu, &teste);
    */
}
#ifdef _WIN32
#define _USE_MATH_DEFINES
#define NOMINMAX
#endif



void debug_nglListEndScene_hook() {
    g_console->render();

    if (debug_enabled) 
      //  render_current_debug_menu();
	debug_menu::active_menu->render_current_debug_menu();
	
	
	    if (debug_disabled) 
    debug_menu::active_menu->render_current_debug_menu();


    nglListEndScene();
}
static bool s_hero_frozen = false;

void close_debug()
{
    debug_enabled = 0;
    debug_disabled = 0;
    g_game_ptr->unpause();
    g_game_ptr->enable_physics(true);
	g_game_ptr->freeze_hero(false);
	// menu fully closed -> drop focus and reset hero-frozen tracker
	debug_menu::has_focus = false;
	s_hero_frozen = false;
}

static void apply_freeze_toggle()
{
    s_hero_frozen = !s_hero_frozen;
    g_game_ptr->unpause();
    g_game_ptr->freeze_hero(s_hero_frozen);
}

// Non-toggling counterpart to apply_freeze_toggle(): drives s_hero_frozen
// and the in-game hero-freeze flag to an exact requested state. Used by
// the new debug-menu focus toggle (TAB / L3) so freezing the hero stays
// in lock-step with debug_menu::has_focus, instead of just flipping
// whatever the previous value happened to be.
static void set_hero_frozen(bool frozen)
{
    s_hero_frozen = frozen;
    if (g_game_ptr != nullptr) {
        g_game_ptr->unpause();
        g_game_ptr->freeze_hero(frozen);
    }
}

void disable_physics()
{
    debug_enabled  = 1; apply_freeze_toggle();
    g_game_ptr->unpause();
    current_menu = current_menu;
  //  g_game_ptr->enable_physics(false);
    debug_menu::active_menu->has_focus = true;   // menu just opened -> grab focus
	
	debug_menu::active_menu->had_menu_this_frame = true;


}


void enable_physics()
{

	debug_disabled  = 1; apply_freeze_toggle();
    g_game_ptr->unpause();
    current_menu = current_menu;
 //   g_game_ptr->enable_physics(true);
    debug_menu::active_menu->has_focus = true;   // menu just opened -> grab focus
	
	debug_menu::active_menu->had_menu_this_frame = true;


}	

void custom()
{
    debug_disabled  = 1; apply_freeze_toggle();
    g_game_ptr->unpause();
    current_menu = current_menu;
  //  g_game_ptr->enable_physics(false);
    debug_menu::active_menu->has_focus = true;   // menu just opened -> grab focus
	
	debug_menu::active_menu->had_menu_this_frame = true;

}	







#include "game_level.h"
#include "fe_health_widget.h"


constexpr auto NUM_HEROES = 24u;

const char* hero_list[NUM_HEROES] = {
    "ultimate_spiderman",
	"usm_blacksuit_costume",
	"usm_wrestling_costume",
	"arachno_man_costume",
	"peter_parker_costume",
	"peter_hooded_costume",
    "venom",
    "peter_parker",
    "peter_hooded",
    "venom_spider",
    "carnage",
    "rhino",
    "green_goblin",
    "mary_jane",
    "venarge",
    "electro_suit",
    "electro_nosuit",
    "wolverine",
    "beetle",
    "shocker",
    "silver_sable",
    "johnny_storm",
	"black_suit",
	"spider-man",
};

enum class hero_status_e {
    UNDEFINED = 0,
    REMOVE_PLAYER = 1,
    ADD_PLAYER = 2,
    CHANGE_HEALTH_TYPE = 3,
} hero_status;

int hero_selected;
int frames_to_skip = 2;

struct level_descriptor_v2_t
{
    fixedstring<32> field_0;
    fixedstring<64> field_20;
    fixedstring<16> field_60;
    int field_70;
    int field_74;
    int field_78;
    int field_7C;
    int field_80;
    int field_84;
    int field_88;
    int field_8C;
};

#include "game_process.h"

level_descriptor_t* get_level_descriptors(int* arg0)
{
    auto* game_partition = resource_manager::get_partition_pointer(resource_partition_enum(0));
    assert(game_partition != nullptr);

    assert(game_partition->get_pack_slots().size() == 1);

    auto& v2 = game_partition->get_pack_slots();
    auto* game_slot = v2.front();
    assert(game_slot != nullptr);

    auto v6 = 9;
    string_hash v5{ "level" };
    resource_key a1{ v5, (resource_key_type)v6 };
    int a2 = 0;
    auto* v11 = (level_descriptor_t*)game_slot->get_resource(a1, &a2, nullptr);

    if (arg0 != nullptr)
    {
        *arg0 = a2 / sizeof(level_descriptor_t);
    }

    return v11;
}

static int main_flow[] = { 5, 6, 14 };
game_process mainflow_proc{ "main", main_flow, 3 };

#pragma once
#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#if defined(_WIN32)
#define NOMINMAX
#include <shellapi.h>
#include <windows.h>
#else // POSIX (including WSL)
#include <limits.h>
#include <unistd.h>
#endif



// Global constants
constexpr const char* kIniPath = "data/game.ini"; // default INI location

// INI helpers – cross-platform read / write of single values
inline std::string ReadINI(const char* section,
    const char* key,
    const char* defaultValue = "")
{
#if defined(_WIN32)
    char buf[256] {}; // zero-initialise
    GetPrivateProfileStringA(section,
        key,
        defaultValue,
        buf,
        static_cast<DWORD>(sizeof(buf)),
        kIniPath);
    return buf;
#else // POSIX (including WSL)
    std::ifstream file(kIniPath);
    if (!file)
        return defaultValue;

    std::string line, currentSection;
    auto trim = [](std::string& s) {
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char c) { return !std::isspace(c); }));
        s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char c) { return !std::isspace(c); }).base(), s.end());
    };
    std::string targetSec(section), targetKey(key);
    std::transform(targetSec.begin(), targetSec.end(), targetSec.begin(), ::toupper);
    std::transform(targetKey.begin(), targetKey.end(), targetKey.begin(), ::toupper);

    while (std::getline(file, line)) {
        trim(line);
        if (line.empty() || line[0] == ';')
            continue;
        if (line.front() == '[' && line.back() == ']') {
            currentSection = line.substr(1, line.size() - 2);
            std::transform(currentSection.begin(), currentSection.end(), currentSection.begin(), ::toupper);
            continue;
        }
        if (currentSection != targetSec)
            continue;
        auto pos = line.find('=');
        if (pos == std::string::npos)
            continue;
        std::string k = line.substr(0, pos);
        trim(k);
        std::transform(k.begin(), k.end(), k.begin(), ::toupper);
        if (k != targetKey)
            continue;
        std::string value = line.substr(pos + 1);
        trim(value);
        return value;
    }
    return defaultValue;
#endif
}

inline void WriteINI(const char* section,
    const char* key,
    const char* value)
{
#if defined(_WIN32)
    if (!WritePrivateProfileStringA(section, key, value, kIniPath))
        throw std::runtime_error("WritePrivateProfileString failed");
#else // POSIX (including WSL)
    // Very small, naïve re-write: load file into memory, modify or append key.
    std::ifstream in(kIniPath);
    std::vector<std::string> lines;
    std::string currentSection, targetSec(section);
    std::transform(targetSec.begin(), targetSec.end(), targetSec.begin(), ::toupper);

    bool keyWritten = false;
    if (in) {
        std::string l;
        while (std::getline(in, l))
            lines.push_back(l);
    }

    auto trim = [](std::string& s) {
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char c) { return !std::isspace(c); }));
        s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char c) { return !std::isspace(c); }).base(), s.end());
    };

    for (auto& l : lines) {
        std::string line = l;
        trim(line);
        if (line.empty() || line[0] == ';')
            continue;
        if (line.front() == '[' && line.back() == ']') {
            currentSection = line.substr(1, line.size() - 2);
            std::transform(currentSection.begin(), currentSection.end(), currentSection.begin(), ::toupper);
            continue;
        }
        if (currentSection != targetSec)
            continue;
        auto pos = line.find('=');
        if (pos == std::string::npos)
            continue;
        std::string k = line.substr(0, pos);
        trim(k);
        std::transform(k.begin(), k.end(), k.begin(), ::toupper);
        if (k == key) {
            l = std::string(key) + "=" + value;
            keyWritten = true;
            break;
        }
    }

    if (!keyWritten) {
        // append [section] + key if missing
        bool sectionExists = false;
        for (auto& l : lines) {
            std::string chk = l;
            trim(chk);
            if (chk.front() == '[' && chk.back() == ']') {
                std::string s = chk.substr(1, chk.size() - 2);
                std::transform(s.begin(), s.end(), s.begin(), ::toupper);
                if (s == targetSec) {
                    sectionExists = true;
                    break;
                }
            }
        }
        if (!sectionExists) {
            lines.push_back("[" + std::string(section) + "]");
        }
        lines.push_back(std::string(key) + "=" + value);
    }

    std::ofstream out(kIniPath, std::ios::trunc);
    for (auto& l : lines)
        out << l << "\n";
#endif
}

// Convenience: switch active scene by overwriting the value in [Strings]
inline void SelectScene(const std::string& sceneName)
{
    WriteINI("Strings", "SCENE_NAME", sceneName.c_str());
}


// Process-restart helpers

    inline std::string currentExecutablePath()
    {
#if defined(_WIN32)
        char buf[MAX_PATH];
        DWORD len = GetModuleFileNameA(nullptr, buf, MAX_PATH);
        if (len == 0 || len == MAX_PATH)
            throw std::runtime_error("GetModuleFileName failed");
        return std::string(buf, len);
#elif defined(__APPLE__)
        uint32_t size = 0;
        _NSGetExecutablePath(nullptr, &size);
        std::string path(size, '\\0');
        if (_NSGetExecutablePath(path.data(), &size) != 0)
            throw std::runtime_error("_NSGetExecutablePath failed");
        return path;
#else
        char buf[PATH_MAX];
        ssize_t len = ::readlink("/proc/self/exe", buf, sizeof(buf) - 1);
        if (len == -1)
            throw std::runtime_error("readlink(/proc/self/exe) failed");
        buf[len] = '\\0';
        return std::string(buf, len);
#endif
    }

    inline void launch(const std::string& exePath,
        const std::string& args = {})
    {
#if defined(_WIN32)
        std::string cmd = "\"" + exePath + "\" " + args;
        STARTUPINFOA si { sizeof(si) };
        PROCESS_INFORMATION pi {};
        if (!CreateProcessA(nullptr, cmd.data(), nullptr, nullptr, FALSE,
                0, nullptr, nullptr, &si, &pi))
            throw std::runtime_error("CreateProcess failed");
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
#else
        if (args.empty())
            execl(exePath.c_str(), exePath.c_str(), static_cast<char*>(nullptr));
        else
            execl("/bin/sh", "sh", "-c", (exePath + " " + args).c_str(), static_cast<char*>(nullptr));
        throw std::runtime_error("execl failed");
#endif
    }
 // namespace detail
 


[[noreturn]] inline void restart(const std::string& extraArgs = {})
{
    launch(currentExecutablePath(), extraArgs);
    std::exit(EXIT_SUCCESS);
}

[[noreturn]] inline void restartWith(const std::string& exePath,
    const std::string& args = {})
{
    launch(exePath, args);
    std::exit(EXIT_SUCCESS);
}



int wait_loop0 = 1000;
int wait_loop1 = 500;

// for microprocessor without timer, if it has a timer refer to vendor documentation and use it instead.
void
wait( int seconds )
{   // this function needs to be finetuned for the specific microprocessor
    int i, j, k;
    for(i = 0; i < seconds; i++)
    {
        for(j = 0; j < wait_loop0; j++)
        {
            for(k = 0; k < wait_loop1; k++)
            {   // waste function, volatile makes sure it is not being optimized out by compiler
                int volatile t = 120 * j * i + k;
                t = t + 5;
            }
        }        }        }
 

struct RebootRequest {
    bool        pending = false;
    std::string scene;
    std::string args;
};

inline RebootRequest g_reboot{};

// =====================================================================
// Teardown helpers
//
// Fill these in with the actual reversed engine calls you have.
// They are separated so you can enable/disable subsystems as you
// discover them.
// =====================================================================
inline void TeardownCurrentLevel()
{
    // --- Graphics / world ------------------------------------------
    if (g_game_ptr) {
        g_game_ptr->clear_screen();
    }

    // --- Sound -----------------------------------------------------
    sound_manager::delete_inst();

    // --- Add more subsystem teardowns here as you reverse them -----
    // resource_manager::flush();
    // entity_manager::destroy_all();
    // physics::shutdown();
    // mission_manager::unload();
    // world::unload_current_level();
}

inline void LoadLevel(const std::string& sceneName)
{
    // --- Option A: let the engine re-read from INI -----------------
    // The game's own startup path already reads SCENE_NAME from the INI.
    // If you can invoke that same path:
    //
    //     g_game_ptr->load_level(sceneName.c_str());
    //
    // --- Option B: re-init subsystems manually ---------------------
    //     sound_manager::create_inst();
    //     resource_manager::load_scene(sceneName.c_str());
    //     g_game_ptr->begin_scene();
    //
    // --- Option C: call the game's full init sequence ---------------
    //     game_init();          // your reversed init function
    //
    // Uncomment / replace with whatever you have reversed.
    // For now this is a placeholder — fill in the actual call:

    SelectScene(sceneName.c_str());
}

inline void PollReboot()
{
    if (!g_reboot.pending)
        return;

    g_reboot.pending = false;

    TeardownCurrentLevel();
    LoadLevel(g_reboot.scene);

    g_reboot.scene.clear();
    g_reboot.args.clear();
}


[[noreturn]] inline void RestartWithScene(const std::string& sceneName,
                                          const std::string& extraArgs = {})
{

		g_game_ptr->clear_screen();
	  sound_manager::delete_inst();  
    #if defined(_WIN32)
        Sleep(5000); 


    #else 
        sleep(500);	
    #endif
    SelectScene(sceneName);	
    restart(extraArgs);          
	


	

}



// Cycles through every entry in kLevelList sequentially.
// The index is stored back into the INI so the state survives a full
// process restart on every platform (static variables won’t).


// ---------------------------------------------------------------------
// app_rebooter.hpp  (or wherever the globals live)
// ---------------------------------------------------------------------
#include <array>   // new

constexpr std::array<const char*, 10> kLevelList{
    "ai_arena",
	"blank",
	"charactera_arena",
	"characterb_arena",
	"city_arena",
	"combata_arena",
	"combatb_arena",
	"mockupj_arena",
	"blank_arena",
	"traffic_arena",
	

};

[[noreturn]] inline void RestartNextScene(const std::string& extraArgs = {})
{
    // Persisted cursor lives in the INI right next to SCENE_NAME
    const std::string cursorStr = ReadINI("Strings", "SCENE_CURSOR", "0");
	std::size_t cursor = std::stoul(cursorStr) % std::size(kLevelList);


    const char* nextScene = kLevelList[cursor];
    SelectScene(nextScene);                          // update SCENE_NAME

    // advance & wrap
    std::size_t nextCursor = (cursor + 1) % std::size(kLevelList);
    WriteINI("Strings", "SCENE_CURSOR", std::to_string(nextCursor).c_str());

    restart(extraArgs);                              // will not return
}



void level_select_handler(debug_menu_entry* entry)
{
    // entry->m_id was set to the index when you built the menu
    const std::size_t idx = static_cast<std::size_t>(entry->m_id);

    // Safety net – in case someone hand‑edits game.ini
    if (idx >= std::size(kLevelList))
        return;      


	PollReboot();
	RestartWithScene(kLevelList[idx]);
}


void reboot_handler(debug_menu_entry* a1)
{
}

void handle_hero_select_menu(debug_menu_entry* entry, custom_key_type)
{
    entry->m_game_flags_handler(entry);
}

void hero_entry_callback(debug_menu_entry*);

void hero_toggle_handler(debug_menu_entry* entry);

bool is_hero_available(const char* hero_name) {
        try {
            resource_key hero_key;
            string_hash hero_hash{hero_name};
            hero_key.set(hero_hash, RESOURCE_KEY_TYPE_PACK);
            return resource_manager::get_pack_file_stats(hero_key, nullptr, nullptr, nullptr);
        }
        catch (const std::exception&) {
            return false;
        }
    }
	
	
	constexpr auto HERO_PACKLIST = 22;

	
	
	static bool hero_select_list[HERO_PACKLIST] = {
true, false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false
};




void create_level_select_menu(debug_menu* level_select_menu)
{
    //assert(debug_menu::root_menu != nullptr);

    static debug_menu* hero_select_menu = create_menu("Hero Select");

    debug_menu_entry v28{ hero_select_menu };

    level_select_menu->add_entry(&v28);
    for (auto i = 0u; i < NUM_HEROES; ++i)
    {
        string_hash v5{ hero_list[i] };
        auto v11 = resource_key{ v5, RESOURCE_KEY_TYPE_PACK };
        auto v30 = resource_manager::get_pack_file_stats(v11, nullptr, nullptr, nullptr);
        if (v30)
        {
            mString v35{ hero_list[i] };

            debug_menu_entry v37{ v35.c_str() };

            v37.set_game_flags_handler(hero_toggle_handler);
            v37.m_id = i;
            v37.set_frame_advance_cb(hero_entry_callback);
            hero_select_menu->add_entry(&v37);
		        }
    }	
			
    int arg0;
    auto* level_descriptors = get_level_descriptors(&arg0);
    printf("num_descriptors = %d\n", arg0);
    for (auto i = 0; i < arg0; ++i)
    {
        auto* v1 = level_descriptors[i].field_0.to_string();
        string_hash v5{ v1 };
        auto v11 = resource_key{ v5, RESOURCE_KEY_TYPE_PACK };
        auto v17 = resource_manager::get_pack_file_stats(v11, nullptr, nullptr, nullptr);
        if (v17)
        {
            mString v22{ level_descriptors[i].field_60.to_string() };
            debug_menu_entry v39{ v22.c_str() };

            v39.set_game_flags_handler(level_select_handler);
            v39.m_id = i;
            level_select_menu->add_entry(&v39);
        }
    }

    mString v25{ "-- REBOOT --" };
    debug_menu_entry v38{ v25.c_str() };

    v38.set_game_flags_handler(reboot_handler);

    level_select_menu->add_entry(&v38);


}




   void remove_player(int player_num)
    {
resource_manager::set_active_district(false);
    
        void (__fastcall *func)(void *, void *, int) = bit_cast<decltype(func)>(0x00558550);

        func(g_world_ptr, nullptr, player_num);
    }


    
    int add_player(const mString &a2)
    {

            resource_manager::set_active_district(true);

        int (__fastcall *func)(void *, void *, const mString *) = bit_cast<decltype(func)>(0x0055B400);

        return func(g_world_ptr, nullptr, &a2);
    }



void hero_toggle_handler(debug_menu_entry* entry)
{
    printf("hero_toggle_handler\n");
    assert(entry->get_id() < NUM_HEROES);
    hero_selected = entry->get_id();
	    int hero_index = -1;
    if (hero_select_list[0] != 0) hero_index = 0;
    if (hero_select_list[1] != 0) hero_index = 1;
	    if (hero_select_list[2] != 0) hero_index = 2;
		    if (hero_select_list[3] != 0) hero_index = 3;
			    if (hero_select_list[4] != 0) hero_index = 4;
				    if (hero_select_list[5] != 0) hero_index = 5;
					    if (hero_select_list[6] != 0) hero_index = 6;
						    if (hero_select_list[7] != 0) hero_index = 7;
							    if (hero_select_list[0] != 0) hero_index = 0;
    if (hero_select_list[8] != 0) hero_index = 8;
	    if (hero_select_list[9] != 0) hero_index = 9;
		    if (hero_select_list[10] != 0) hero_index = 10;



    // Get hero name
    mString* default_hero = os_developer_options::instance->get_hero_name();
    mString hero_name(*default_hero);
    
    if (hero_index != -1) {
        hero_name = hero_list[hero_index];
    }
    hero_status = hero_status_e::REMOVE_PLAYER;
}

void hero_entry_callback(debug_menu_entry*)
{
    printf("hero_entry_callback: hero_status = %d\n", hero_status);

    auto v18 = g_world_ptr->num_players;
    switch (hero_status)
    {
    case hero_status_e::REMOVE_PLAYER:
    {
        remove_player(v18 - 1);
        hero_status = hero_status_e::ADD_PLAYER;
        frames_to_skip = 2;
        g_game_ptr->enable_marky_cam(true, true, -1000.0, 0.0);
        break;
    }
    case hero_status_e::ADD_PLAYER:
    {
        auto v1 = frames_to_skip--;
        if (v1 <= 0)
        {
            assert(hero_selected > -1 && hero_selected < NUM_HEROES);

            [[maybe_unused]] auto v2 = add_player(mString{ hero_list[hero_selected] });
            Sleep(1000);
            /*
            auto v10 = v2 <= v18;

            assert(v10 && "Cannot add another_player");
            */

            g_game_ptr->enable_marky_cam(false, true, -1000.0, 0.0);
            frames_to_skip = 2;
            hero_status = hero_status_e::CHANGE_HEALTH_TYPE;
        }
        break;
    }
    case hero_status_e::CHANGE_HEALTH_TYPE: {
        auto v3 = frames_to_skip--;
        if (v3 <= 0)
        {
            auto v17 = 0;
            auto* v5 = (actor*)g_world_ptr->get_hero_ptr(0);
            auto* v6 = v5->get_player_controller();
            auto v9 = v6->m_hero_type;
            switch (v9)
            {
                case hero_type_enum::SPIDEY:
                    v17 = 0;
                    break;
                case hero_type_enum::VENOM:
                    v17 = 4;
                    break;
                case hero_type_enum::PARKER:
                    v17 = 5;
                    break;
            }

            auto* v7 = g_world_ptr->get_hero_ptr(0);
            auto v8 = v7->my_handle;
            g_femanager.IGO->hero_health->SetType(v17, v8.field_0);
            g_femanager.IGO->hero_health->SetShown(true);
            close_debug();
            hero_status = hero_status_e::UNDEFINED;
        }
        break;
    }
    default:
        break;
    }
}



debug_menu_root *& menu_ptr = var<debug_menu_root*>(0);



void menu_setup(int game_state, int keyboard) {
	if (is_menu_key_pressed(MENU_START, keyboard) && (game_state == 6)) {

    //debug menu stuff
        if (!debug_enabled && game_state == 6) {
                g_game_ptr->unpause();
                debug_enabled = !debug_enabled;
                current_menu = debug_menu::root_menu;
                custom();
		
				
            }

 }
        if (is_menu_key_pressed(MENU_SELECT, keyboard) && (game_state == 7)) {



            if (!debug_enabled && game_state == 7) {
                g_game_ptr->unpause();
                debug_enabled = !debug_enabled;
                current_menu = debug_menu::root_menu;


                custom();

            }

 


        if (level_select_menu->used_slots == 0)
        {
            create_level_select_menu(level_select_menu);
        }


    }
}







void init_shadow_targets2()
{
    debug_menu::init();
	

    CDECL_CALL(0x00592E80);
}



uint8_t __stdcall slf__debug_menu_entry__set_handler__str(vm_stack* stack, void* unk) {

    stack->pop(8);

    void** params = (void**)stack->SP;

    debug_menu_entry* entry = static_cast<decltype(entry)>(params[0]);
    const char* scrpttext = static_cast<char*>(params[1]);

    string_hash strhash{ scrpttext };

    script_instance* instance = stack->my_thread->inst;
    entry->set_script_handler(instance, mString{ scrpttext });

    return true;
}

uint8_t __stdcall slf__destroy_debug_menu_entry__debug_menu_entry(vm_stack* function, void* unk) {

    function->pop(4);

    debug_menu_entry** entry = (decltype(entry))function->SP;

    remove_debug_menu_entry(*entry);

    return 1;
}

void sub_65BB36(script_library_class::function* func, vm_stack* stack, char* a3, int a4)
{
    for (auto i = 0; i < a4; ++i)
    {
        if (*bit_cast<DWORD*>(&a3[4 * i]) == 0x7BAD05CF)
        {
            printf("uninitialized parameters in call to 0x%08X", func->m_vtbl);

            //v5 = j_vm_stack::get_thread(stack);
            //vm_thread::slf_error(v5, v6);

            assert(0 && "uninitialized parameters in call to script library function");
        }
    }
}

uint8_t __fastcall slf__create_progression_menu_entry(script_library_class::function* func, void*, vm_stack* stack, void* unk) {

    stack->pop(8);

    auto* stack_ptr = bit_cast<char*>(stack->SP);
    sub_65BB36(func, stack, stack_ptr, 2);

    char** strs = (char**)stack->SP;

    printf("Entry: %s -> %s\n", strs[0], strs[1]);

    string_hash strhash{ strs[1] };

    script_instance* instance = stack->my_thread->inst;

    debug_menu_entry entry{ strs[0] };
    entry.set_script_handler(instance, { strs[1] });

    progression_menu->add_entry(&entry);

    int push = 0;
    auto sz = sizeof(push);
    memcpy((void*)stack->SP, &push, sz);
    stack->SP += sz;
    return true;
}

string_hash names[120] = {
    to_hash("_city_arena(num)"),
    to_hash("daytime_tint_effect(entity)"),
    to_hash("mission_fail_venom()"),
    to_hash("mission_fail_spidey()"),
    to_hash("do_it(entity)"),
    to_hash("generic_spidey_lose_panel()"),
    to_hash("failure_dialog(num)"),
    to_hash("generic_venom_lose_panel()"),
    to_hash("clear_all_scripts()"),
    to_hash("ambient_music_start()"),
    to_hash("fade_out_ambient_music()"),
    to_hash("show_car_combat_help_text(num)"),
    to_hash("set_hero(str)"),
    to_hash("set_spiderman_health()"),
    to_hash("run_hot_pursuit()"),
    to_hash("prepare_open_city_vars()"),
    to_hash("cycle_tod(num)"),
    to_hash("switch_to_spiderman()"),
    to_hash("switch_to_venom()"),
    to_hash("hot_pursuit_checker()"),
    to_hash("update_open_city_times()"),
    to_hash("adjust_mission_num(num)"),
    to_hash("set_TOD_and_shoreline(num)"),
    to_hash("enable_mission_tokens_by_hero()"),
    to_hash("go_to_my_neighborhood(vector3d)"),
    to_hash("city_tutorial_manager(num,num,num)"),
    to_hash("handle_objective_list(num)"),
    to_hash("launch_trick_race(num)"),
    to_hash("launch_venom_trick_race(num)"),
    to_hash("launch_torch_race(num)"),
    to_hash("set_up_tokens(num,num)"),
    to_hash("kill_shenanigans()"),
    to_hash("disable_mission_tokens()"),
    to_hash("launch_combat_tour(num)"),
    to_hash("show_mission_text(num)"),
    to_hash("update_open_city_shen_times()"),
    to_hash("convert_to_progression_step(num)"),
    to_hash("disable_all_tokens()"),
    to_hash("enable_pickup_tokens()"),
    to_hash("set_torch_token(num)"),
    to_hash("set_tour_tokens()"),
    to_hash("set_race_tokens()"),
    to_hash("reset_all_tours()"),
    to_hash("process_neighborhood_tours()"),
    to_hash("handle_health_increase(num)"),
    to_hash("handle_race_unlocking(num)"),
    to_hash("handle_tour_unlocking(num)"),
    to_hash("handle_combo_unlocking(num,num)"),
    to_hash("unlock_chars(num,num)"),
    to_hash("unlock_concept(num,num)"),
    to_hash("handle_costume_unlocking(num,num)"),
    to_hash("city_goals_hint()"),
    to_hash("process_master_objective()"),
    to_hash("set_progression_display_vars()"),
    to_hash("open_city_music_thread()"),
    to_hash("set_force_shen_vars(num)"),
    to_hash("set_force_can_vars(num)"),
    to_hash("get_obj_list(num)"),
    to_hash("set_current_progression(num)"),
    to_hash("auto_progress_objectives(num)"),
    to_hash("handle_unlocking(num,num)"),
    to_hash("setup_progression_mission(num)"),
    to_hash("setup_autoplay_mission(num)"),
    to_hash("do_open_mission(num)"),
    to_hash("prepare_endgame()"),
    to_hash("progression_mission_failed()"),
    to_hash("progression_mission_aborted()"),
    to_hash("fail_in_water()"),
    to_hash("get_hero_start_point(num)"),
    to_hash("set_master_objective()"),
    to_hash("enable_getaways(num)"),
    to_hash("kill_mission_text(num)"),
    to_hash("setup_progression_debug_menu()"),
    to_hash("set_up_traffic()"),
    to_hash("set_up_peds()"),
    to_hash("hoodname()"),
    to_hash("anybody_sinks(entity)"),
    to_hash("entity_sinks(entity)"),
    to_hash("kill_hero(debug_menu_entry)"),
    to_hash("teleport_04(debug_menu_entry)"),
    to_hash("teleport_03(debug_menu_entry)"),
    to_hash("teleport_02(debug_menu_entry)"),
    to_hash("teleport_01(debug_menu_entry)"),
    to_hash("toggle_strafe(debug_menu_entry)"),
    to_hash("toggle_progression_off(debug_menu_entry)"),
    to_hash("toggle_progression_on(debug_menu_entry)"),
    to_hash("start_to_level_V12(debug_menu_entry)"),
    to_hash("start_to_level_V10(debug_menu_entry)"),
    to_hash("start_to_level_V09(debug_menu_entry)"),
    to_hash("start_to_level_V08(debug_menu_entry)"),
    to_hash("start_to_level_V07(debug_menu_entry)"),
    to_hash("start_to_level_V03(debug_menu_entry)"),
    to_hash("start_to_level_V02(debug_menu_entry)"),
    to_hash("start_to_level_V01(debug_menu_entry)"),
    to_hash("game_finished(debug_menu_entry)"),
    to_hash("start_to_level_S13(debug_menu_entry)"),
    to_hash("start_to_level_S11(debug_menu_entry)"),
    to_hash("start_to_level_S10(debug_menu_entry)"),
    to_hash("start_to_level_S09(debug_menu_entry)"),
    to_hash("start_to_level_S08(debug_menu_entry)"),
    to_hash("start_to_level_S07(debug_menu_entry)"),
    to_hash("start_to_level_S06(debug_menu_entry)"),
    to_hash("start_to_level_S05(debug_menu_entry)"),
    to_hash("start_to_level_S04(debug_menu_entry)"),
    to_hash("start_to_combat_tour(debug_menu_entry)"),
    to_hash("start_to_level_S03(debug_menu_entry)"),
    to_hash("start_to_johnny_storm(debug_menu_entry)"),
    to_hash("start_to_level_S02(debug_menu_entry)"),
    to_hash("start_to_level_S01(debug_menu_entry)"),
    to_hash("global_turnon_tokens(debug_menu_entry)"),
    to_hash("on_combat_tour_enter(entity,num,str)"),
    to_hash("on_storm_race_enter(entity,num,str)"),
    to_hash("on_venom_race_enter(entity,num,str)"),
    to_hash("on_trick_race_enter(entity,num,str)"),
    to_hash("on_token_pickup(entity,num,num,num,str)"),
    to_hash("clear_missions_for_unlockables()"),
    to_hash("toggle_hero()"),
    to_hash("set_hero_costume(str)"),
    to_hash("left_token_trigger()"),
    to_hash("display_car_combat_warning_panel(num)")
};



bool __fastcall slf__create_debug_menu_entry(script_library_class::function* func, void*, vm_stack* stack, void* unk)
{
    stack->pop(4);

    auto* stack_ptr = bit_cast<char*>(stack->SP);
    sub_65BB36(func, stack, stack_ptr, 1);
    char** strs = bit_cast<char**>(stack->SP);

    debug_menu_entry entry{};
    entry.entry_type = debug_menu_entry_type::dUNDEFINED;
    strcpy(entry.text, strs[0]);

    printf("entry.text = %s\n", entry.text);
    
    script_instance* instance = stack->my_thread->inst;
    printf("Total funcs: %d\n", instance->get_parent()->total_funcs);
    void* res = add_debug_menu_entry(script_menu, &entry);

    script_executable* se = stack->my_thread->ex->owner->parent;
    printf("total_script_objects = %d\n", se->total_script_objects);
    
    for (auto i = 0; i < se->total_script_objects; ++i) {
        auto* so = se->script_objects[i];
        printf("Name of script_object = %s\n", so->name.to_string());

        auto* so_menu = create_menu(so->name.to_string(), debug_menu::sort_mode_t::ascending);
        auto* so_entry = create_menu_entry(so->name.to_string());
        so_entry->set_data(so);
        so_entry->set_submenu(so_menu);
        script_menu->add_entry(so_entry);

        for (auto j = 0; j < so->total_funcs; ++j) {
            auto* fn = so->funcs[j];
            printf("Func name: %s\n", fn->fullname.to_string());

            debug_menu_entry fn_entry{ fn->fullname.to_string() };
            script_instance* instance = stack->my_thread->inst;

            fn_entry.set_data(nullptr);
            fn_entry.set_submenu(nullptr);
            fn_entry.m_id = j;
            fn_entry.set_script_handler_from_char(instance, fn->fullname.to_string());
            add_debug_menu_entry(so_menu, &fn_entry);
        }

        printf("\n");
    }

    se->add_allocated_stuff_for_debug_menu(vm_debug_menu_entry_garbage_collection_id, (int)res, 0);

    int push = (int)res;
    auto sz = sizeof(push);
    memcpy((void*)stack->SP, &push, sz);
    stack->SP += sz;
    return 1;
}




// ---------------------------------------------------------------------------------------------------

BOOL install_redirects()
{
    sp_log("Installing redirects\n");

    //fix invalid float operation
    {
        set_nop(0x005AC34C, 6);

        REDIRECT(0x005AC347, hook_controlfp);
    }
	

    REDIRECT(0x005AC52F, parse_cmd);

    {
        DWORD hookDirectInputAddress = (DWORD) HookDirectInput8Create;
        REDIRECT(0x008218B0, hookDirectInputAddress);
        set_nop(0x008218B5, 1);
        sp_log("Patching the DirectInput8Create call\n");
    }
	
	FEMultiLineText_patch();
	
	    
		
		
	
	resource_amalgapak_header_patch();
	
	        cursor_patch();
	
	        FrontEndMenuSystem_patch();
			
			script_file_loader_patch();
			
			pause_menu_root_patch();
			
			PauseMenuSystem_patch();
			
			IGOZoomOutMap_patch();
			
			pause_menu_awards_patch();	

            pause_menu_status_patch();			
			
			pause_menu_goals_patch();
			
			
			unlockables_menu_patch();
			
		    pause_menu_message_log_patch();
			
		

          FEText_patch();

        alternate_costumes_patch();

    Timer_patch();
	
	script_lib_debug_menu_patch();
	
		debug_render_patch();

    //REDIRECT(0, sub_5952D0);

    if constexpr (1)
    {
        moved_entities_patch();

        pc_joypad_device_patch();

        player_controller_inode_patch();

        camera_mode_patch();

        game_button_patch();

        mouselook_controller_patch();

        web_zip_state_patch();

        lookat_target_controller_patch();

        controller_patch();

        scene_anim_patch();

        camera_patch();

        comic_panels_patch();

        us_person_patch();

        script_controller_patch();

        nal_anim_controller_patch();

        geometry_manager_patch();

        input_mgr_patch();
		
    }


    if constexpr (1) {
        console_patch();
    } else {
        game_patch();
    }


    // @todo: windowed
    REDIRECT(0x005AC4A9, register_class_and_create_window);


    // @todo: debug menu
    if constexpr (DEBUG_MENU_REIMPL)
    {
        REDIRECT(0x0052B4BF, spider_monkey::render);

        HookFunc(0x004EACF0, (DWORD)aeps_RenderAll, 0, "Patching call to aeps::RenderAll");
        HookFunc(0x0052B5D7, (DWORD)debug_nglListEndScene_hook, 0, "Hooking nglListEndScene to inject debug menu");
        HookFunc(0x005AD77D, (DWORD)construct_client_script_libs_hook, 0, "Hooking construct_client_script_libs to inject my vm");
        REDIRECT(0x005E10EE, init_shadow_targets2);

        auto writeDWORD = [](int address, DWORD newValue, [[maybe_unused]] const char* reason) -> void {
            *((DWORD*)address) = newValue;
        };
        writeDWORD(0x0089C710, (DWORD)slf__create_progression_menu_entry, "Hooking first ocurrence of create_progession_menu_entry");
        writeDWORD(0x0089C718, (DWORD)slf__create_progression_menu_entry, "Hooking second  ocurrence of create_progession_menu_entry");
        writeDWORD(0x0089AF70, (DWORD)slf__create_debug_menu_entry, "Hooking first ocurrence of create_debug_menu_entry");
        writeDWORD(0x0089C708, (DWORD)slf__create_debug_menu_entry, "Hooking second  ocurrence of create_debug_menu_entry");
        writeDWORD(0x0089C720, (DWORD)slf__destroy_debug_menu_entry__debug_menu_entry, "Hooking destroy_debug_menu_entry");
        writeDWORD(0x0089C750, (DWORD)slf__debug_menu_entry__set_handler__str, "Hooking set_handler");
    }
	

    //SET_JUMP(0x0077A870, nglLoadTextureTM2);
		

    return true;

    wds_render_manager_patch();

    wds_camera_manager_patch();

    resource_directory_patch();

    tlresource_directory_patch();


    input_settings_patch();

    game_settings_patch();

    ngl_patch();
	
	REDIRECT(0x00822556, myWinMain);
	
	
			

    //standalone patches
    if constexpr (1)
    {
        slc_manager_patch();
    
        tl_patch();

        resource_manager_patch();

        script_manager_patch();

        slab_allocator_patch();

        nfl_system_patch();
		
    }

    if constexpr (0)
    {
        vm_patch();

        vm_thread_patch();

        vm_executable_patch();
    }

    if constexpr (0)
    {
        us_decal_patch();

        us_lod_patch();

        us_translucentshader_patch();

        us_simpleshader_patch();
    }

    if constexpr (1)
    {
        sound_manager_patch();

        string_hash_dictionary_patch();

        fx_cache_patch();

        cached_special_effect_patch();

        damage_interface_patch();

        mashable_vector_patch();

        core_ai_resource_patch();

        sound_bank_slot_patch();

        settings_patch();

        static_region_list_methods_patch();

        aeps_patch();

        game_camera_patch();

        region_patch();
        
        matrix4x4_patch();

        ai_interaction_data_patch();

        animation_controller_patch();

        collision_capsule_patch();

        city_lod_patch();

        swing_inode_patch();

        swing_state_patch();

        jump_state_patch();

        polytube_patch();

        web_interface_patch();

        web_polytube_patch();

        entity_patch();

        wds_token_manager_patch();

        beam_patch();

        motion_effect_struct_patch();

        ngl_vertexdef_patch();

        actor_patch();

        app_patch();

        camera_target_info_patch();

        spiderman_camera_patch();

        script_data_interface_patch();

        animation_interface_patch();

        conglomerate_patch();

        entity_base_patch();

        entity_mash_patch();

        anim_record_patch();

        sound_interface_patch();
    }

    if constexpr (0)
    {
        script_executable_patch();

        script_patch();

        script_instance_patch();

        script_access_patch();
    }

    if constexpr (1)
    {
        combo_system_patch();

        combo_system_move_patch();

        meta_anim_interact_patch();

        Input_patch();

        sin_container_patch();

        daynight_patch();

        FrontEnd_Shader_patch();

        nglShader_patch();

        mission_manager_patch();
    }

    if constexpr (0)
    {
        resource_pack_streamer_patch();

        resource_partition_patch();

        resource_pack_slot_patch();

        resource_pack_standalone_patch();

        resource_key_patch();
    }

    if constexpr (0)
    {
        tlResourceDirectory_patch();
        
        PanelMeshSection_patch();

        fe_mini_map_widget_patch();

        IGOFrontEnd_patch();

        spider_monkey_patch();

        FEManager_patch();

        MultiLineString_patch();

        menu_nav_bar_patch();
    }

    if constexpr (0)
    {

        pause_menu_save_load_display_patch();

        pause_menu_controller_patch();

        pause_menu_options_display_patch();

        pause_menu_transition_patch();

        pause_menu_status_patch();


        unlockables_menu_patch();
    }

    if constexpr (0)
    {
        PauseMenuSystem_patch();

        cg_mesh_patch();

        FEFloatingText_patch();

        mash_virtual_base_patch();

        PanelAnimFile_patch();

        fe_dialog_text_patch();

        PanelQuad_patch();

        FEText_patch();

        PanelFile_patch();

        localized_string_table_patch();

        animation_logic_system_shared_patch();

        mash_info_struct_patch();
    }

    if constexpr (1)
    {
        worldly_pack_slot_patch();

        nsl_patch();

        wds_script_manager_patch();

        world_dynamics_system_patch();

        entity_handle_manager_patch();

        eligible_pack_patch();

        terrain_patch();

        chuck_callbacks_patch();
    }

    if constexpr (1)
    {
        os_file_patch();

        traffic_patch();

        traffic_path_lane_patch();

        spawnable_patch();

        ai_path_patch();

        ai_core_patch();
    }

    animation_logic_system_patch();

    if constexpr (1)
    {
        interactable_interface_patch();

        event_manager_patch();

        trigger_manager_patch();

        variant_interface_patch();
    }

    if constexpr (1)
    {
        nalStreamInstance_patch();

        script_memtrack_patch();

        scratchpad_stack_patch();

        hero_inode_patch();

        cut_scene_player_patch();
    }

    if constexpr (1)
    {
        character_anim_controller_patch();

        usm_anim_player_patch();

        anim_handle_patch();

        plr_loco_crawl_state_patch();

        plr_loco_crawl_transition_state_patch();

        ai_player_controller_patch();

        ai_state_machine_patch();

        line_info_patch();
        
        local_collision_patch();

        hierarchical_entity_proximity_map_patch();

        spidey_base_state_patch();

        hero_base_state_patch();
        
        enhanced_state_patch();

        CharComponentBase_patch();
        
        nalChar_patch();

        ped_spawner_patch();
    }

    //resource handler patches
    if constexpr (0)
    {
        ai_interact_resource_handler_patch();

        cut_scene_resource_handler_patch();

        patrol_def_resource_handler_patch();

        item_resource_handler_patch();

        box_trigger_resource_handler_patch();

        path_resource_handler_patch();

        ai_state_graph_resource_handler_patch();

        scene_anim_resource_handler_patch();

        anim_resource_handler_patch();

        base_ai_resource_handler_patch();

        skeleton_resource_handler_patch();

        sound_alias_database_resource_handler_patch();

        glass_house_resource_handler_patch();

        material_file_resource_handler_patch();

        morph_file_resource_handler_patch();

        mesh_file_resource_handler_patch();

        panel_resource_handler_patch();

        texture_resource_handler_patch();

        gab_database_resource_handler_patch();

        als_resource_handler_patch();
    }

    //als
    if constexpr (0)
    {
        als_meta_anim_base_patch();

        als_meta_anim_swing_patch();

        als_meta_aimed_shot_vert_patch();

        als_animation_logic_system_interface_patch();

        als_inode_patch();

        als_state_machine_patch();

        als_meta_anim_table_shared_patch();

        als_state_machine_shared_patch();

        als_transition_group_base_patch();

        als_scripted_category_patch();

        als_category_patch();

        als_simple_orientation_patch();

        als_scripted_state_patch();

        als_state_patch();

        als_layer_state_machine_shared_patch();

        als_basic_rule_data_patch();
        
        als_res_data_patch();

        als_mocomp_patch();

        als_motion_compensator_patch();
    }

    return TRUE;

    tlInstanceBank_patch();

    mission_stack_manager_patch();

    SET_JUMP(0x008223D8, start);

    {
        //REDIRECT(0x008224B1, initterm);
        REDIRECT(0x00822500, initterm);
    }

    REDIRECT(0x005AC4A9, register_class_and_create_window);

    os_developer_options_patch();


    //rbc_contact_patch();

    //spline_patch();


    if constexpr (0) {

        anchor_query_visitor_patch();


        PanelQuadSection_patch();


        rigid_body_patch();

        swing_anchor_finder_patch();

        USVariantShaderNode_patch();

        script_library_class_patch();

        po_patch();

        wds_entity_manager_patch();

        info_node_desc_list_patch();

        FEMenu_patch();

        main_menu_keyboard_patch();

        main_menu_options_patch();

        game_data_meat_patch();

        TextureInputPack_patch();

        FEMenuEntry_patch();

        IGOZoomOutMap_patch();

        run_state_patch();
    }

#define ORIGINAL_DLL 0
#if ORIGINAL_DLL
    {
        if constexpr (0)
        {
            if constexpr (0) {
                //EnableLog l{};

                sp_log("Ints:");

                mString ints{"{"};

                for (size_t i{0}; i < std::size(flag_names()); ++i) {
                    //sp_log("%d %s", i, int_names()[i]);

                    ints += mString{0, "%d \"%s\", \n", i, flag_names()[i]};
                }

                sp_log("%s", ints.c_str());

                assert(0);

                sp_log("\nFlags:");

                for (size_t i{0}; i < std::size(flag_names()); ++i) {
                    sp_log("%d %s", i, flag_names()[i]);
                }

                sp_log("\nStrings:");
                for (size_t i{0}; i < std::size(string_names()); ++i) {
                    sp_log("%d %s", i, string_names()[i]);
                }
            }

            sp_log("\nentity_flavor_names:");

            std::string str{};
            str.reserve(400);

            {
                str += "\n";

                for (int i = 0; i < 28; ++i) {
                    str += entity_flavor_names()[i];
                    str += "\n";
                }

                sp_log(str.c_str());
            }

            sp_log("resource_key_type_ext:");

            {
                str.clear();

                str += "\n";

                //for (auto &ext : resource_key_type_ext())
                auto &ext = resource_key_type_ext()[3];
                {
                    for (size_t i{0}; i < 70u; ++i) {
                        auto Src = ext[i];

                        str += Src;
                        str += ' ';
                        str += std::to_string(i);
                        str += "\n";
                    }

                    str += "\n";
                }

                sp_log(str.c_str());
            }

            assert(0);
        }



#if 0
        

        FEMenuSystem_patch();

        character_viewer_patch();

        ghetto_mash_file_header_patch();

        string_hash_patch();

        mAvlTree_patch();

        resource_pack_header_patch();

        nfl_driver_patch();

        memory_patch();

#endif

        if constexpr (0) {
            set_nop(0x0076ECF0, 2);

            set_nop(0x0076EA57, 2);
            set_nop(0x0076EA64, 2);
        }

        collide_aux_patch();

        physical_interface_patch();


        nglRenderList_patch();

        swing_anchor_obbfilter_patch();

        quick_anchor_info_patch();

        glass_house_manager_patch();

        TextureHandle_patch();

        FileUSM_patch();

        ai_state_machine_patch();

        state_graph_patch();

        character_viewer_patch();

        filespec_patch();

        entity_base_patch();

        fe_mission_text_patch();



        combat_state_patch();

        line_anchor_patch();

        resource_partition_patch();

        eligible_pack_streamer_patch();

        nglMesh_patch();

        pole_swing_state_patch();

        physics_inode_patch();

        interaction_inode_patch();

        pick_up_state_patch();


        //message_board_patch();

#if 0

        
        

        param_block_patch();

        physics_system_patch();
#endif
    }

#endif //ORIGINAL_DLL

    sp_log("Redirects have been installed\n");

    return TRUE;
}



#include "mod.h"  // Updated mod.h with GIF support
#include <fstream>

// FBX Integration for nglLoadMeshFileInternal
// This file shows how to integrate FBX loading into the existing mesh loading system


namespace fs = std::filesystem;


std::vector<uint8_t> read_file(const fs::path& filePath) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file) 
        return std::vector<uint8_t>();

    file.seekg(0, std::ios::end);
    std::streamsize sz = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<uint8_t> buffer(sz);
    buffer.resize(sz); buffer.reserve(sz);
    file.read(reinterpret_cast<char*>(buffer.data()), sz);
    return buffer;
}


void enumerate_mods() {
    Mods.clear();
    ModFileOverrides.clear();

    const fs::path modsDir = fs::current_path() / "mods";
    if (!fs::is_directory(modsDir))
        return;

    auto normalize_rel = [](std::string s) {
        // lower case + backslashes
        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return (char)std::tolower(c); });
        std::replace(s.begin(), s.end(), '/', '\\');
        // trim leading .\\
        while (s.rfind(".\\", 0) == 0) s.erase(0, 2);
        return s;
    };

    for (const auto& entry : fs::recursive_directory_iterator(modsDir)) {
        if (!entry.is_regular_file())
            continue;

        const fs::path& path = entry.path();
        std::string ext = transformToLower(path.extension().string());

        // 1) Exact file override table (used by NFL openFile hook)
        //    Key is path relative to mods/, including extension.
        {
            const fs::path rel = fs::relative(path, modsDir);
            const std::string relKey = normalize_rel(rel.generic_string());
            ModFileOverrides[relKey] = path;

            // Convenience: if a mod is placed under "mods/data/...", allow it to override
            // the engine request "data\\..." directly.
            if (relKey.rfind("data\\", 0) == 0) {
                ModFileOverrides[relKey] = path;
                ModFileOverrides[relKey.substr(5)] = path; // also allow without the leading "data\\"
            }
        }

        // 2) In-memory override (used by NGL callback + some handlers)
        //    Keying is extension-less (like ngl_readfile_callback does).
        //    Example: mods/characters/spidey.fbx -> key "characters/spidey".
        tlresource_type resType = TLRESOURCE_TYPE_NONE;
        if (ext == ".dds" || ext == ".tga")
            resType = TLRESOURCE_TYPE_TEXTURE;
        else if (ext == ".obj" || ext == ".fbx" || ext == ".dae" || ext == ".gltf" || ext == ".glb")
            resType = TLRESOURCE_TYPE_MESH;
        else if (ext == ".pcmesh" || ext == ".pcm")
            resType = TLRESOURCE_TYPE_MESH_FILE;

        // We also allow WAV files to be tracked as raw file mods (Type stays NONE),
        // so you can still access them via ModFileOverrides without bloating memory.
        const bool shouldKeepInMemory = (resType != TLRESOURCE_TYPE_NONE);

        if (!shouldKeepInMemory)
            continue;

        std::vector<uint8_t> fileData = read_file(path);

        // Build a relative no-ext key (forward slashes -> lower-case -> hashed)
        const fs::path relNoExt = fs::relative(path, modsDir).replace_extension();
        const std::string relNoExtKey = transformToLower(relNoExt.generic_string());
        const uint32_t hash = to_hash(relNoExtKey.c_str());

        Mods[hash] = Mod{path, (int)resType, std::move(fileData)};
        //printf("mod: %s -> 0x%08X\n", relNoExtKey.c_str(), hash);
    }

#   if MOD_MESH_DBG_REPLACE_ALL
        dbgReplaceMesh = getMod(0x1189ab87, TLRESOURCE_TYPE_MESH_FILE);
		dbgReplaceMesh = getMod(0xCB5B8BA9, TLRESOURCE_TYPE_MESH_FILE);
#   endif
}






BOOL install_hooks() {
    return set_text_to_writable() && install_redirects() && install_patches() &&
        restore_text_perms();
}





BOOL WINAPI DllMain(HINSTANCE, DWORD fdwReason, [[maybe_unused]] LPVOID lpvReserved) {
    //printf("DLLMain %lu 0x%08X\n", fdwReason, (int) lpvReserved);

    if (fdwReason == DLL_PROCESS_ATTACH) {
        char *args = GetCommandLine();
        if (strstr(args, " -console")) {
            g_config.DebugMode = true;

            AllocConsole();
            if (!freopen("CONOUT$", "w", stdout)) {
                MessageBoxA(NULL, "Error", "Couldn't allocate console...Closing", 0);
                return FALSE;
            }
        }

if (strstr(args, " -windowed"))
    g_config.WindowedMode = true;


        
        if (strstr(args, " -noloadscreen"))
            g_config.NoLoadScreen = true;
		
		        if (strstr(args, " -enablefpuexceptionhandling"))
            g_config.EnableFpuExceptionHandling = false;

        bool res = install_hooks();
        if (res) 
        enumerate_mods();
		    os_developer_options::os_developer_init();
            ini_parser::parse("debug_menu.ini", os_developer_options::instance);
			    if (os_developer_options::instance->get_flag(mString{"WINDOW_DEFAULT"})) {
        g_config.WindowedMode = true;
    }

    sub_5C9EA0();
        return res;
		

    } else if (fdwReason == DLL_PROCESS_DETACH) {

        FreeConsole();

    }

    return TRUE;
}