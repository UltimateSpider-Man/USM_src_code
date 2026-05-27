#include "unlockables_menu.h"

#include "common.h"
#include "func_wrapper.h"
#include "log.h"
#include "trace.h"
#include "utility.h"

#include "panelfile.h"
#include "panelquad.h"
#include "fetext.h"
#include "fixedstring.h"
#include "mstring.h"
#include "color32.h"

#include "femenusystem.h"
#include "fe_menu_nav_bar.h"
#include "femanager.h"

#include "comic_panels.h"
#include "movie_manager.h"
#include "mission_manager.h"
#include "mission_stack_manager.h"
#include "resource_manager.h"
#include "resource_partition.h"
#include "os_developer_options.h"
#include "sound_instance_id.h"
#include "string_hash.h"
#include "ngl.h"
#include "game.h"
#include "cursor.h"
#include "fileusm.h"
#include "global_text_enum.h"
#include "variables.h"

#include <windows.h>
#include <string>
#include <stdexcept>
#include <filesystem>
#include <vector>

namespace fs = std::filesystem;

VALIDATE_SIZE(unlockables_menu, 0x100u);
VALIDATE_SIZE(unlockable_item, 0x10u);

VALIDATE_OFFSET(unlockables_menu, field_2C,    0x2C);
VALIDATE_OFFSET(unlockables_menu, panel_file,  0x30);
VALIDATE_OFFSET(unlockables_menu, field_34,    0x34);
VALIDATE_OFFSET(unlockables_menu, field_38,    0x38);
VALIDATE_OFFSET(unlockables_menu, items,       0x74);
VALIDATE_OFFSET(unlockables_menu, field_E4,    0xE4);
VALIDATE_OFFSET(unlockables_menu, field_E8,    0xE8);
VALIDATE_OFFSET(unlockables_menu, field_EC,    0xEC);
VALIDATE_OFFSET(unlockables_menu, field_F0,    0xF0);
VALIDATE_OFFSET(unlockables_menu, field_F4,    0xF4);
VALIDATE_OFFSET(unlockables_menu, field_F8,    0xF8);

// ─────────────────────────────────────────────────────────────────────────────
// Boot-into-DEMO helpers (custom case 5 of OnCross)
// ─────────────────────────────────────────────────────────────────────────────




// ─────────────────────────────────────────────────────────────────────────────
// Constructor (0x00614020)
// ─────────────────────────────────────────────────────────────────────────────

unlockables_menu::unlockables_menu(FEMenuSystem *a2, int a3, int a4)
    : FEMenu(a2, 0, a3, a4, 8, 0)
{
    THISCALL(0x00614020, this, a2, a3, a4);
}

// ─────────────────────────────────────────────────────────────────────────────
// (0x0064BF80) load icon / highlight / full / big textures for items[a2-1]
//
// Original IDA strings:
//   icon name      = "u_mm_icon_0"        + index
//   highlight name = icon_name            + "_question_mark"
//   full icon name = "u_mm_full_icon_0"   + index
//   big icon name  = "u_mm_big_icon_0"    + index
// ─────────────────────────────────────────────────────────────────────────────

void unlockables_menu::sub_64BF80(PanelFile *a1, int a2)
{
    TRACE("unlockables_menu::sub_64BF80");

    if constexpr (1)
    {
        assert(a1 != nullptr);
        assert(a2 >= 1 && a2 <= 7);

        const auto index_str = mString::from_int(a2);

        const auto icon_name           = mString{"u_mm_icon_0"}      + index_str;
        const auto icon_highlight_name = icon_name                   + "_question_mark";
        const auto full_icon_name      = mString{"u_mm_full_icon_0"} + index_str;
        const auto big_icon_name       = mString{"u_mm_big_icon_0"}  + index_str;

        unlockable_item *item = &this->items[a2 - 1];

        item->icon              = a1->GetPQ(icon_name.c_str());
        item->icon_highlight    = a1->GetPQ(icon_highlight_name.c_str());
        item->full_icon_texture = nglGetTexture(tlFixedString{full_icon_name.c_str()});
        item->big_icon_texture  = nglGetTexture(tlFixedString{big_icon_name.c_str()});
    }
    else
    {
        THISCALL(0x0064BF80, this, a1, a2);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// (0x0062D6D0) scroll / re-layout helper
// Used during Init's wrap-around restore + by OnUp/OnDown/OnL2/OnR2.
// Touches every items[i].icon vfunc +192 (SetPos) plus title/text refresh.
// ─────────────────────────────────────────────────────────────────────────────

void unlockables_menu::sub_62D6D0(int a2)
{
    THISCALL(0x0062D6D0, this, a2);
}


void unlockables_menu::Init()
{
    TRACE("unlockables_menu::Init");

       if constexpr (1)
       {
        PanelFile *pf = PanelFile::UnmashPanelFile("unlock_menu", static_cast<panel_layer>(7));
        assert(pf != nullptr);
        this->panel_file = pf;

        menu_nav_bar *nav = this->field_34;
        assert(nav != nullptr);

        nav->text_box     = pf->GetTextPointer("u_mm_nav_bar_text");
        nav->background_a = pf->GetPQ("u_mm_nav_bar_01");
        nav->field_1C     = pf->GetPQ("u_mm_nav_bar_02");
        nav->Load();
        this->field_38[ 0] = pf->GetPQ("u_mm_bkg_01");
        this->field_38[ 1] = pf->GetPQ("u_mm_bkg_02");
        this->field_38[ 2] = pf->GetPQ("u_mm_bkg_03");
        this->field_38[ 3] = pf->GetPQ("u_mm_title_box_01");
        this->field_38[ 4] = pf->GetPQ("u_mm_title_box_02");
        this->field_38[ 5] = pf->GetPQ("u_mm_title_box_03");
        this->field_38[ 6] = pf->GetPQ("u_mm_title_box_04");
        this->field_38[ 9] = pf->GetPQ("u_mm_title_box_spidey_icon");
        this->field_38[10] = pf->GetPQ("u_mm_main_icon_detail");
        this->field_38[ 7] = pf->GetPQ("u_mm_main_icon");
        this->field_38[ 8] = pf->GetPQ("u_mm_logo_small");
        this->field_38[11] = pf->GetPQ("u_mm_main_window_detail_01");
        this->field_38[12] = pf->GetPQ("u_mm_main_window_detail_02");
        this->field_38[13] = pf->GetPQ("u_mm_main_window_detail_03");

        for (int i = 1; i <= 7; ++i) {
            sub_64BF80(pf, i);
        }

        const float center_diff = this->items[1].icon->GetCenterY()
                                - this->items[0].icon->GetCenterY();
        this->field_E4 = static_cast<int>(center_diff);

        // ── Visible-slot count: 8 for LTD edition, 6 otherwise ──────────────
        const bool is_ltd =
              os_developer_options::instance->get_flag(mString{"LIMITED_EDITION_DISC"})
           || mission_manager::s_inst->sub_5C24C0();
        const int  span_n = is_ltd ? 8 : 6;
        this->field_E8 = this->field_E4 * (span_n - 1);


        {
            PanelQuad *wrap_icon = this->items[span_n - 2].icon;
            assert(wrap_icon != nullptr);
            wrap_icon->sub_616710(
                Float{0.0f},
                Float{-static_cast<float>(this->field_E8)});
        }


        auto &flt_87EBC4 = var<float>(0x0087EBC4);   // = 3.0f
        auto &flt_965BDC = var<float>(0x00965BDC);   // global font scale

        // Function-pointer aliases for FEText vtable entries with no public
        // wrapper in the project yet.
        using fe_int_fn   = void  (__fastcall *)(FEText *, void *, int);
        using fe_color_fn = void  (__fastcall *)(FEText *, void *, color32);
        using fe_setx_fn  = void  (__fastcall *)(FEText *, void *, float);
        using fe_getx_fn  = float (__fastcall *)(FEText *, void *);
        using fe_adj_fn   = float (__fastcall *)(FEText *, void *, float);
        using fe_set2_fn  = void  (__fastcall *)(FEText *, void *, int, int);
        using fe_scl_fn   = int   (__fastcall *)(FEText *, void *, float);


        {
            FEText *txt = pf->GetTextPointer("u_mm_unlockables_text");
            assert(txt != nullptr);
            this->field_EC = txt;

            auto *vtbl = bit_cast<std::intptr_t *>(txt->m_vtbl);

            // mark text active
            bit_cast<fe_int_fn>(vtbl[100 / 4])(txt, nullptr, 1);


            bit_cast<fe_int_fn>(vtbl[136 / 4])(txt, nullptr, 309);


            bit_cast<fe_color_fn>(vtbl[164 / 4])(
                txt, nullptr, color32{0xFFC8773F});

            // x' = AdjustForJustification(GetX() - flt_965BDC * 3.0); SetX(x')
            const float current_x = bit_cast<fe_getx_fn>(vtbl[216 / 4])(txt, nullptr);
            const float target_x  = current_x - flt_965BDC * flt_87EBC4;
            const float adjusted  = bit_cast<fe_adj_fn>(vtbl[212 / 4])(txt, nullptr, target_x);
            bit_cast<fe_setx_fn>(vtbl[144 / 4])(txt, nullptr, adjusted);
        }


        {
            FEText *txt = pf->GetTextPointer("u_mm_title_box_text");
            assert(txt != nullptr);
            this->field_F0 = txt;

            auto *vtbl = bit_cast<std::intptr_t *>(txt->m_vtbl);


            bit_cast<fe_int_fn>(vtbl[100 / 4])(txt, nullptr, 1);


            const int handle = bit_cast<fe_scl_fn>(vtbl[276 / 4])(
                txt, nullptr, -1.0f);


            bit_cast<fe_set2_fn>(vtbl[328 / 4])(
                txt, nullptr, 310, handle);


            bit_cast<fe_color_fn>(vtbl[164 / 4])(
                txt, nullptr, color32{0xFFC8773F});


            bit_cast<fe_color_fn>(vtbl[288 / 4])(
                txt, nullptr, color32{0xFFF3D5B7});

            bit_cast<fe_setx_fn>(vtbl[284 / 4])(txt, nullptr, 1.0f);
        }


        const int saved = this->field_F4;
        this->field_F4 = 0;
        while (this->field_F4 < saved) {
            ++this->field_F4;
            this->sub_62D6D0(-1);
        }
    }
    else
    {
        THISCALL(0x00645C20, this);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// _Load (0x00614110) - virtual: TurnOn(true) on all 14 background quads.
// ─────────────────────────────────────────────────────────────────────────────

void unlockables_menu::_Load()
{
    TRACE("unlockables_menu::_Load");

    if constexpr (1)
    {
        for (auto i = 0u; i < 14u; ++i)
        {
            PanelQuad *quad = this->field_38[i];
            assert(quad != nullptr);
            quad->TurnOn(true);
        }
    }
    else
    {
        THISCALL(0x00614110, this);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// OnActivate (0x0062D510) - virtual
// Pushes "unlockables" mission pack, calls vtable Init, builds nav bar buttons,
// and registers cursor regions for each visible item slot.
// ─────────────────────────────────────────────────────────────────────────────

void unlockables_menu::OnActivate()
{
    sp_log("unlockables_menu::OnActivate():");

    if constexpr (1)
    {
        mString pack_name{"unlockables"};

        // ── 1. First-time push: only run Init in a fresh resource context
        //       if the unlockables pack hasn't already been pushed. ────────
        auto *msm = mission_stack_manager::s_inst;
        if (!msm->is_pack_pushed(pack_name))
        {
            msm->push_mission_pack_immediate(pack_name, pack_name);

            auto *ctx = resource_manager::get_best_context(
                            RESOURCE_PARTITION_MISSION);
            resource_manager::push_resource_context(ctx);

            // virtual call: vtable[+12] = vtable[3] = Init()
            {
                auto *vtbl = bit_cast<fastcall_call(*)[4]>(this->m_vtbl);
                void (__fastcall *func)(void *) = CAST(func, (*vtbl)[3]);
                func(this);
            }

            resource_manager::pop_resource_context();
        }

        // ── 2. Configure the nav bar: store the pack name in field_4 so
        //       the FEText vfunc below can pick it up, then rebuild
        //       buttons (Cross / Triangle / global_text 3 = "RESUME"-ish) ──
        menu_nav_bar *nav = this->field_34;
        nav->field_4    = pack_name.c_str();
        nav->field_28   = 0;
        nav->AddButtons(menu_nav_bar::button_type{15},
                        menu_nav_bar::button_type{17},
                        static_cast<global_text_enum>(3));

        // ── 3. FEText vfunc[+140] (= vtable index 35) on nav->text_box
        //       takes a 16-byte mString-compatible struct by value.
        //       We call it twice: once with the pack name "unlockables",
        //       once with the localized "RESUME" string. ──────────────────
        using fe_settext_fn = void (__fastcall *)(FEText *, void *,
                                                  FEText::string);

        {
            auto *vtbl = bit_cast<std::intptr_t *>(nav->text_box->m_vtbl);
            auto func  = bit_cast<fe_settext_fn>(vtbl[140 / 4]);

            // 3a. menu_nav_bar->field_4 (= "unlockables")
            {
                mString tmp{nav->field_4.c_str()};
                func(nav->text_box, nullptr, FEText::string{tmp});
            }

            // 3b. localized "RESUME" string from FileUSM
            {
                const char *resume = get_msg(g_fileUSM(), "RESUME");
                mString tmp{resume};
                func(nav->text_box, nullptr, FEText::string{tmp});
            }
        }

        // ── 4. Mark menu as active + reset highlight cursor ─────────────
        this->field_28 |= 0x80u;
        this->field_2A  = -1;

        // ── 5. Rebuild cursor-pickable regions ─────────────────────────
        Cursor *cur = g_cursor();
        cur->sub_5A6790();                              // clear
        cur->sub_5A67D0(278, 420, 355, 445);            // resume button hitbox

        // Per-item rows (icon list down the left side).
        const bool is_ltd =
              os_developer_options::instance->get_flag(mString{"LIMITED_EDITION_DISC"})
           || mission_manager::s_inst->sub_5C24C0();
        const int  span_n = is_ltd ? 8 : 6;

        int y = 40;
        for (int i = 0; i < span_n - 1; ++i)
        {
            cur->sub_5A67D0(50, y, 98, y + 50);
            y += 54;
        }

        // Big detail-panel hitbox on the right.
        cur->sub_5A67D0(170, 100, 600, 400);
    }
    else
    {
        THISCALL(0x0062D510, this);
    }
}


static fs::path GetSelfDir()
{
    wchar_t buf[MAX_PATH]{};
    if (!GetModuleFileNameW(nullptr, buf, MAX_PATH))
        throw std::runtime_error("GetModuleFileNameW failed: " + std::to_string(GetLastError()));
    return fs::path(buf).parent_path();
}

// ─────────────────────────────────────────────
//  Unlink all mods
//  Removes every .asi / .dll (except the host
//  exe itself) from the current exe's directory.
//  Extend modExtensions for other mod types.
// ─────────────────────────────────────────────


// ─────────────────────────────────────────────
//  Launch exe with a specific working directory
//  so it resolves "data\" and "docs\" correctly
// ─────────────────────────────────────────────



inline void LaunchExeInDir2(const std::wstring& exePath,const fs::path& workDir, bool waitForProcess = false)
{
    STARTUPINFOW si{};
    PROCESS_INFORMATION pi{};
    si.cb = sizeof(si);

    std::wstring cmdLine = L"\"" + exePath + L"\"";

    if (!CreateProcessW(
            nullptr,
            cmdLine.data(),
            nullptr, nullptr,
            FALSE,
            0,
            nullptr, workDir.wstring().c_str(),
            &si, &pi))
    {
        throw std::runtime_error("CreateProcessW failed: " + std::to_string(GetLastError()));
    }

    if (waitForProcess)
        WaitForSingleObject(pi.hProcess, INFINITE);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
}

static void EnsureFolders(const fs::path& baseDir, const std::vector<std::wstring>& subFolders)
{
    for (auto& sub : subFolders)
    {
        fs::path p = baseDir / sub;
        if (!fs::exists(p))
            fs::create_directories(p);
    }
}

void EnterDemoAndReboot()
{
    fs::path usmpcDir = GetSelfDir();                    // ..\USMPC
    fs::path demoDir  = usmpcDir / L"DEMO";              // ..\USMPC\DEMO
    fs::path demoExe  = demoDir  / L"USM.exe";

    if (!fs::exists(demoExe))
        throw std::runtime_error("USM.exe not found in DEMO: " + demoExe.string());

    // Guarantee all required folders exist inside DEMO\
    EnsureFolders(demoDir, { L"data", L"docs", L"shaders", L"mods", L"scripts" });

    // Launch DEMO\USM.exe with DEMO\ as CWD so all
    // relative paths (data\, docs\, shaders\, mods\, scripts\) resolve correctly
    LaunchExeInDir2(demoExe, demoDir, false);

    ExitProcess(0);
}




void unlockables_menu::OnCross(int a2) {
	sp_log("unlockables_menu::OnCross():");

    static bool initialized = false;
    static string_hash fe_ps_accept;
    
    if (!initialized) {
        initialized = true;
    }
    		    static string_hash sound_id ("fe_ps_accept");

    sound_instance_id id = sub_60B960(sound_id, 1.0, 1.0);
    
    int panel_id;
    
    switch (this->field_F4) {
    case 2:
        this->field_2C->MakeActive(10);
        comic_panels::game_play_panel()->field_67 = 1;
		panel_id = 11;
        return;
        
    case 1:
        panel_id = 14;
        break;
        
    case 0:
        panel_id = 12;
        break;
        
    case 3:
        panel_id = 10;
        break;
        
    case 4:
        panel_id = 15;
        break;
        
    case 5:

        EnterDemoAndReboot();
        
    case 6:
        panel_id = 16;
        break;
	case 7:
        panel_id = 17;
        break;
    case 8:
        panel_id = 17;
        break;
	case 9:
        panel_id = 17;
        break;
        
    default:
        return;
    }
    
    this->field_2C->MakeActive(panel_id);
    comic_panels::game_play_panel()->field_67 = 1;
	THISCALL(0x0062DB20, this, a2);
}

// ─────────────────────────────────────────────────────────────────────────────
// OnTriangle (0x006253C0) - virtual: pop "unlockables" pack, back to root.
// ─────────────────────────────────────────────────────────────────────────────

void unlockables_menu::OnTriangle(int a2)
{
    sp_log("unlockables_menu::OnTriangle():");

    if constexpr (1)
    {
        mString pack_name{"unlockables"};

        mission_stack_manager::s_inst->pop_mission_pack_immediate(pack_name, pack_name);

        FEMenuSystem *sys = this->field_2C;
        sys->MakeActive(2);

        // game_play_panel.field_67 = 0  (matches IDA: !v3[56] guard)
        comic_panels::game_play_panel()->field_67 = 0;

        static string_hash sfx_id_hash{"FE_UL_BACK"};
        [[maybe_unused]] sound_instance_id id = sub_60B960(sfx_id_hash, 1.0, 1.0);
    }
    else
    {
        THISCALL(0x006253C0, this, a2);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// OnDeactivate (0x00614140) - virtual
// ─────────────────────────────────────────────────────────────────────────────

void unlockables_menu::OnDeactivate(FEMenu *a2)
{
    if constexpr (1)
    {
        mString pack_name{"unlockables"};

        auto *msm = mission_stack_manager::s_inst;
        if (msm->is_pack_pushed(pack_name)) {
            msm->pop_mission_pack_immediate(pack_name, pack_name);
        }

        this->field_28 &= ~0x80u;
    }
    else
    {
        THISCALL(0x00614140, this, a2);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Vtable patches
// Vtable base for unlockables_menu = 0x00894908.
//   +0x0C = 0x894914 -> Init
//   +0x10 = 0x894918 -> _Load
//   +0x2C = 0x894934 -> OnActivate
//   +0x4C = 0x894954 -> OnCross
//   +0x50 = 0x894958 -> OnTriangle
// ─────────────────────────────────────────────────────────────────────────────

void unlockables_menu_patch()
{
    {
        FUNC_ADDRESS(address, &unlockables_menu::_Load);
      //  set_vfunc(0x00894918, address);
    }
    {
        FUNC_ADDRESS(address, &unlockables_menu::Init);
      //  set_vfunc(0x00894914, address);
    }
    {
        FUNC_ADDRESS(address, &unlockables_menu::OnCross);
        set_vfunc(0x00894954, address);
    }
    {
        FUNC_ADDRESS(address, &unlockables_menu::OnTriangle);
        // set_vfunc(0x00894958, address);   // disabled: keep original handler
    }
    {
        FUNC_ADDRESS(address, &unlockables_menu::OnActivate);
       // set_vfunc(0x00894934, address);
    }
}
