#include "pause_menu_root.h"

#include "pause_menu_goals.h"
#include "comic_panels.h"
#include "common.h"
#include "entity_base.h"
#include "fe_health_widget.h"
#include "fetext.h"
#include "femanager.h"
#include "femenusystem.h"
#include "igofrontend.h"
#include "mstring.h"
#include "panelquad.h"
#include "pausemenusystem.h"
#include "pause_menu_transition.h"
#include "pause_menu_status.h"
#include "panelfile.h"
#include "utility.h"
#include "trace.h"
#include "vtbl.h"
#include "wds.h"
#include "movie_manager.h"
#include "main.h"

#include "ngl/include/dx/ngl_dx_state.h"

#include "game.h"

#include "FileUSM.h"

#include "fe_menu_nav_bar.h"

#include "femultilinetext.h"
#include "mission_manager.h"

#include "variables.h"

#include "sound_instance_id.h"

#include "cursor.h"
#include "game_settings.h"
#include "script_object.h"

#include "variables.h"

#include "wds.h"

#include <windows.h>
#include <string>
#include <iostream>
#include <stdexcept>
#include <filesystem>
#include <vector>


#include "string_hash.h"


VALIDATE_SIZE(pause_menu_root, 0x100u);

VALIDATE_SIZE(pause_menu_root2, 0x104u);


pause_menu_root::pause_menu_root(FEMenuSystem *a2, int a3, int a4) : FEMenu(a2, 0, a3, a4, 8, 0) {
    this->field_AC = a2;

    this->field_A0 = nullptr;
    this->field_B0 = 0;
    this->field_B4 = 0;
    this->field_30 = 0;
    this->field_2C = 0;
    this->field_2D = 0;
}






void pause_menu_root::_Load()
{
    TRACE("pause_menu_root::Load");

    if constexpr (1)
    {
        auto *v2 = bit_cast<PauseMenuSystem *>(this->field_AC)->field_2C;

        this->field_3C[0] = v2->GetPQ("pm_splash_back_01a");
        this->field_3C[1] = v2->GetPQ("pm_splash_back_01b");
        this->field_3C[2] = v2->GetPQ("pm_splash_back_02a");
        this->field_3C[3] = v2->GetPQ("pm_splash_back_02b");
        this->field_3C[4] = v2->GetPQ("pm_splash_back_03a");
        this->field_3C[5] = v2->GetPQ("pm_splash_back_03b");
        this->field_3C[6] = v2->GetPQ("pm_splash_back_stub_01");
        this->field_3C[7] = v2->GetPQ("pm_splash_back_stub_02");
        this->field_3C[8] = v2->GetPQ("pm_splash_icon");

        this->field_68 = v2->GetPQ("pm_splash_dialog_box_01");
        this->field_6C = v2->GetPQ("pm_splash_dialog_box_02");
        this->field_60 = v2->GetPQ("pm_splash_hilite_text");
        this->field_64 = v2->GetPQ("pm_splash_hilite_text_01");
        this->field_70 = v2->GetPQ("pm_splash_back_04");
        this->field_74 = v2->GetPQ("pm_splash_back_venom");
		
		
		this->field_78[7] = v2->GetTextPointer("pm_splash_text_08");
        this->field_78[0] = v2->GetTextPointer("pm_splash_text_01");
        this->field_78[1] = v2->GetTextPointer("pm_splash_text_02");
        this->field_78[2] = v2->GetTextPointer("pm_splash_text_03");
        this->field_78[3] = v2->GetTextPointer("pm_splash_text_04");
        this->field_78[4] = v2->GetTextPointer("pm_splash_text_05");
        this->field_78[5] = v2->GetTextPointer("pm_splash_text_06");
        this->field_78[6] = v2->GetTextPointer("pm_splash_text_07");
		this->field_78[8] = v2->GetTextPointer("pm_splash_text_09");
       

        this->field_9C = v2->GetTextPointer("pm_splash_text_GAMEPAUSED");
        this->field_A0 = v2->GetTextPointer("pm_splash_dialog_box_text_BODY");
        this->field_A4 = v2->GetTextPointer("pm_splash_dialog_box_text_NOWAY");

        this->field_A8 = v2->GetTextPointer("pm_splash_dialog_box_text_OKAY");
        for (auto i = 0u; i < 9u; ++i)
        {
            this->field_3C[i]->TurnOn(true);
        }

        this->field_78[0]->SetShown(true);
        this->field_78[0]->SetNoFlash(color32 {0xFFE6D03F});
        this->field_78[0]->SetScale(1.2, 1.2);

        for (auto i = 0u; i < 8u; ++i) 
        {
            auto *v6 = this->field_78[i + 1];
            v6->SetShown(true);
            v6->SetNoFlash(color32 {0xFFC87238});
        }

        this->field_68->TurnOn(1);
        this->field_6C->TurnOn(1);
        this->field_60->TurnOn(1);
        this->field_64->TurnOn(1);
        this->field_9C->SetShown(true);
        this->field_9C->SetText(static_cast<global_text_enum>(253));
        this->field_9C->SetNoFlash(color32 {0xFFC8C8C8});
        this->field_A0->SetShown(1);
        this->field_A0->SetText(static_cast<global_text_enum>(271));
        this->field_A0->SetNoFlash(color32 {0xFFC8C8C8});
        this->field_A4->SetShown(1);
        this->field_A4->SetText(static_cast<global_text_enum>(254));
        this->field_A4->SetNoFlash(color32 {0xFFC87238});
        this->field_A8->SetShown(1);
        this->field_A8->SetText(static_cast<global_text_enum>(255));
        this->field_A8->SetNoFlash(color32 {0xFFC87238});

		this->field_78[8]->SetText(static_cast<global_text_enum>(263));
        this->field_78[7]->SetText(static_cast<global_text_enum>(265));
        this->field_78[0]->SetText(static_cast<global_text_enum>(275));
        this->field_78[1]->SetText(static_cast<global_text_enum>(91));
        this->field_78[2]->SetText(static_cast<global_text_enum>(92));
        this->field_78[3]->SetText(static_cast<global_text_enum>(260));
        this->field_78[4]->SetText(static_cast<global_text_enum>(258));
        this->field_78[5]->SetText(static_cast<global_text_enum>(259));
        this->field_78[6]->SetText(static_cast<global_text_enum>(273));

        auto v8 = this->field_78[0]->GetX();
        auto v9 = this->field_78[0]->GetY();
        this->field_60->GetPos(this->field_B8, this->field_C8);
        this->field_64->GetPos(this->field_D8, this->field_E8);

        for (auto i = 0u; i < 4u; ++i)
        {
            this->field_B8[i] = this->field_B8[i] - v8;
            this->field_C8[i] = this->field_C8[i] - v9;
            this->field_D8[i] = this->field_D8[i] - v8;
            this->field_E8[i] = this->field_E8[i] - v9;
        }

        this->field_F8 = false;
    }
    else
    {
        THISCALL(0x0063B2E0, this);
    }
}



void pause_menu_root2::Draw()
{

    bool isStoryActive = mission_manager::s_inst->is_story_active();
    for (int i = 0; i < 9; ++i)
    {
        if (i != 9 || !isStoryActive)
        {
            this->field_78[i]->Draw();
        }
    }


    if (this->field_B0 != 9)
    {
        this->field_60->Draw();
        this->field_64->Draw();
    }

	THISCALL(0x0061BF80, this);

}


void pause_menu_root2::update_selected()
{
    const int prev_index = this->field_B4;
    const int curr_index = this->field_B0;

    auto resolve = [this](int idx) -> FEText * {
        if (idx == 10) {
            if (!this->field_AC)            return nullptr;
            if (!this->field_AC->field_30)  return nullptr;
            return this->field_AC->field_30->text_box;
        }

        if (idx < 0 || idx >= 10) return nullptr;
        return this->field_78[idx];
    };

    if (FEText *curr_text = resolve(curr_index)) {

        if (curr_text) {
            curr_text->SetNoFlash(color32{200, 200, 200, 255});
        }
        curr_text->SetScale(1.0f, 1.0f);
    }

    if (prev_index != curr_index) {
        if (FEText *prev_text = resolve(prev_index)) {
            if (prev_text) {
                prev_text->SetNoFlash(color32{63, 208, 230, 255});
            }
            prev_text->SetScale(1.0f, 1.0f);
        }
    }

    this->field_B4 = this->field_B0;
}


void pause_menu_root2::OnDown(int a2)
{
    if (byte_965C21() || this->field_30 || this->field_2C || this->field_F8) {
        return;
    }

    if (++this->field_B0 > 9) {
        this->field_B0 = 0;
    }

    if (this->field_B0 == 9 && mission_manager::s_inst->is_story_active()) {
        if (++this->field_B0 > 9) {
            this->field_B0 = 0;
        }
    }

    this->update_selected();

    static string_hash sfx{"fe_ps_udscroll"};
    [[maybe_unused]] sound_instance_id id = sub_60B960(sfx, 1.0f, 1.0f);
}

void pause_menu_root2::OnUp(int a2)
{
    sp_log("pause_menu_root::OnUp(): %d", a2);

    if (byte_965C21() || this->field_30 || this->field_2C || this->field_F8) {
        return;
    }

    if (--this->field_B0 < 0) {
        this->field_B0 = 9;
    }

    if (this->field_B0 == 9 && mission_manager::s_inst->is_story_active()) {
        if (--this->field_B0 < 0) {
            this->field_B0 = 9;
        }
    }

    this->update_selected();

    static string_hash sfx{"fe_ps_udscroll"};
    [[maybe_unused]] sound_instance_id id = sub_60B960(sfx, 1.0f, 1.0f);
}

void pause_menu_root::sub_61C610()
{
     CDECL_CALL(0x0061C610);
}

void sub_648F40() {
    CDECL_CALL(0x00648F40);
}

void pause_menu_root2::OnDeactivate(FEMenu *a2)
{
    this->field_28 &= ~0x80;

    Cursor *cur = g_cursor();
    if (cur != nullptr) {
        cur->field_120 = 0;
    }
}

void pause_menu_root::Update(Float a2) {
    if constexpr (1) {
        if (this->field_2D) {
            sub_648F40();
        }

        FEMenu::Update(a2);
        if (this->field_30) {
            this->update_switching_heroes();
        }

        if (this->field_2C) {
            if (!mission_stack_manager::s_inst->waiting_for_push_or_pop()) {
                auto *v3 = this->field_AC;
                this->field_2C = false;

                v3->MakeActive(9);

                comic_panels::game_play_panel()->field_67 = true;
            }
        }
    } else {
        THISCALL(0x006490A0, this, a2);
    }
}

void  sub_582AD0()
{

CDECL_CALL(0x00582AD0);
  
}

void pause_menu_root2::sub_62A840()
{
    menu_nav_bar *nav_bar = this->field_AC->field_30;

    nav_bar->field_4 = "Source";
    nav_bar->field_28 = 0;
    nav_bar->AddButtons(menu_nav_bar::button_type{15},
                                   menu_nav_bar::button_type{17},
                                   static_cast<global_text_enum>(3));



    mString text;

    text = mString(nav_bar->field_4.guts);
    nav_bar->text_box->SetTextNoLocalize(text);

    text = mString(get_msg(g_fileUSM(), "Resume"));
    nav_bar->text_box->SetTextNoLocalize(text);
}

void pause_menu_root2::OnActivate()
{
	    Cursor *cur = g_cursor();
    if (cur != nullptr) {
        cur->sub_5A6790();      // clear all registered hitboxes
        cur->field_120 = 0;     // suppress per-frame visibility flip
        cur->field_114 = 0;     // hide for current frame
    }

    THISCALL(0x006302D0, this);

}



void pause_menu_root::update_switching_heroes() {
    int v2 = this->field_30;
    if (v2 == 4) {
        g_world_ptr->remove_player(g_world_ptr->num_players - 1);
    } else if (v2 == 2) {
        int v3;
        if (this->field_34) {
            g_world_ptr->add_player(mString{"venom"});

            v3 = 4;
        } else {
            g_world_ptr->add_player(mString{"ultimate_spiderman"});

            v3 = 0;
        }

        auto *v4 = g_femanager.IGO->hero_health;
        if (v4->field_0[v3] != nullptr)
        {
            v4->field_30 = g_world_ptr->get_hero_ptr(0)->my_handle.field_0;
            v4->field_38 = v3;
            v4->UpdateMasking();
            v4->clear_bars();
        }

        v4->SetShown(this->field_38);
    }

    --this->field_30;
}


 void pause_menu_root::run_script(const char* func_name)
{
    if (!this->field_F9)
    {
        // "Yes" selected
        int selection = this->field_B0;

        if (selection == 9)
        {
            // Initiate hero switch
          //  this->field_F8 = is_spidey;

            string_hash toggle_hash;
            toggle_hash.initialize(0, func_name, 0);
			    auto* v1 = script::gsoi()->parent;
			int function = script::find_function(toggle_hash, v1, 0);
			script::new_thread(function, reinterpret_cast<script_instance*>(script::gsoi()));
    

            script::sub_5028B0(toggle_hash, script::gsoi());
            script::exec_thread(0);
			
			    auto* v5 = this->field_AC;
    if (v5->m_index >= 0) {
        v5->MakeActive(0);
        comic_panels::game_play_panel()->field_67 = v5->m_count;
    }
		Sleep(500);		
pause_menu_system_ptr->Deactivate();
static string_hash sfx_id_hash{"FE_PS_ACCEPT"};
[[maybe_unused]] sound_instance_id id = sub_60B960(sfx_id_hash, 1.0, 1.0);
}

        if (selection == 8)
        {
            // Initiate hero switch
			bool is_spidey = false;
            this->field_F8 = is_spidey;

            string_hash toggle_hash;
            toggle_hash.initialize(0, func_name, 0);
			    auto* v1 = script::gsoi()->parent;
			int function = script::find_function(toggle_hash, v1, 0);
			script::new_thread(function, reinterpret_cast<script_instance*>(script::gsoi()));
    

            script::sub_5028B0(toggle_hash, script::gsoi());
            script::exec_thread(0);
			
			    auto* v5 = this->field_AC;
    if (v5->m_index >= 0) {
        v5->MakeActive(1);
        comic_panels::game_play_panel()->field_67 = v5->m_count;
    }
			
    pause_menu_system_ptr->Deactivate();
	Sleep(500);

    }
static string_hash sfx_id_hash{"FE_PS_ACCEPT"};
[[maybe_unused]] sound_instance_id id = sub_60B960(sfx_id_hash, 1.0, 1.0);
  
     }
}
	

struct menu_widget{
  int  field_32;
menu_widget * field_36;  
  int  field_20;
  int  field_24;
   bool field_28;
   int field_40;
  int  field_44;
   int field_45; 
int count;  


};


void pause_menu_root::set_menu_state(int state) {

    auto *transition = bit_cast<pause_menu_transition *>(this->field_AC->field_4[1]);
    transition->field_48 = state;
}

void pause_menu_root::transition_to_submenu(int target_state) {
    this->set_menu_state(target_state);

    auto *menu_sys = bit_cast<PauseMenuSystem *>(this->field_AC);
    menu_sys->MakeActive(1);

    if (!menu_sys->field_38) {
        comic_panels::game_play_panel()->field_67 = 0;
    }

    static string_hash sfx_id_hash{"FE_PS_ACCEPT"};
    [[maybe_unused]] sound_instance_id id = sub_60B960(sfx_id_hash, 1.0f, 1.0f);
}

void pause_menu_root::activate_menu(int mode) {
    auto *menu_system = this->field_AC;
    menu_system->MakeActive(mode);
    
    if (!menu_system->field_10) {
        comic_panels::game_play_panel()->field_67 = 0;
    }
}

void pause_menu_root::play_accept_sound(int flag, string_hash &hash) {
    static string_hash sfx_id_hash{"FE_PS_ACCEPT"};
    [[maybe_unused]] sound_instance_id id = sub_60B960(sfx_id_hash, 1.0f, 1.0f);
}

void pause_menu_root::reset_widget_state(int widget_ptr) {
    auto *widget = reinterpret_cast<menu_widget *>(widget_ptr);
    
    for (unsigned int i = 0; i < widget->count; ++i) {

    }
    
    widget->field_32 = widget->field_20;
    widget->field_24 = 0;
    widget->field_28 = 0;
    widget->field_40 = 0;
    widget->field_44 = false;
    widget->field_45 = true;
    widget->field_36 = 0;
}

bool LaunchProcess(const std::wstring& exePath, const std::wstring& args = L"") {
    std::wstring cmdLine = L"\"" + exePath + L"\"";
    if (!args.empty()) {
        cmdLine += L" " + args;
    }

    STARTUPINFOW si{};
    si.cb = sizeof(si);

    PROCESS_INFORMATION pi{};


    std::wstring mutableCmd = cmdLine;

    BOOL ok = CreateProcessW(
        nullptr,                   
        mutableCmd.data(),          
        nullptr,                    
        nullptr,                    
        FALSE,                      
        0,                          
        nullptr,                    
        nullptr,                    
        &si,
        &pi
    );

    if (!ok) {
        std::wcerr << L"CreateProcessW failed. Error: " << GetLastError() << L"\n";
        return false;
    }

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return true;
} 


void pause_menu_root::OnStart(int a2)
{

  THISCALL(0x0061BF40, this, a2);
}


namespace fs = std::filesystem;

static fs::path GetSelfDir()
{
    wchar_t buf[MAX_PATH]{};
    if (!GetModuleFileNameW(nullptr, buf, MAX_PATH))
        throw std::runtime_error("GetModuleFileNameW failed: " + std::to_string(GetLastError()));
    return fs::path(buf).parent_path();
}

static const std::vector<std::wstring> modExtensions = {
    L".asi", L".dll"
};

static bool IsModFile(const fs::path& p)
{
    std::wstring ext = p.extension().wstring();
    // lowercase compare
    for (auto& c : ext) c = towlower(c);
    for (auto& e : modExtensions)
        if (ext == e) return true;
    return false;
}

void UnlinkAllMods()
{
    fs::path dir = GetSelfDir();

    // Collect our own module path so we don't delete ourselves
    wchar_t selfBuf[MAX_PATH]{};
    GetModuleFileNameW(nullptr, selfBuf, MAX_PATH);
    fs::path selfPath = fs::canonical(selfBuf);

    for (auto& entry : fs::directory_iterator(dir))
    {
        if (!entry.is_regular_file()) continue;
        if (!IsModFile(entry.path())) continue;

        // Skip our own exe (shouldn't be .asi/.dll, but be safe)
        std::error_code ec;
        fs::path canonical = fs::canonical(entry.path(), ec);
        if (!ec && canonical == selfPath) continue;

        fs::remove(entry.path(), ec); // non-throwing remove
        // Optional: log failures
        // if (ec) OutputDebugStringW((L"Failed to remove: " + entry.path().wstring()).c_str());
    }
}

void LaunchExeInDir(const std::wstring& exePath,const fs::path& workDir, bool waitForProcess = false)
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

void ExitHACKROMAndBootToUSMPC()
{

    fs::path hackromDir = GetSelfDir();
    fs::path usmpcDir   = hackromDir.parent_path();
    fs::path usmExe     = usmpcDir / L"USM.exe";

    if (!fs::exists(usmExe))
        throw std::runtime_error("USM.exe not found: " + usmExe.string());

    EnsureFolders(usmpcDir, { L"data", L"docs" });

    LaunchExeInDir(usmExe, usmpcDir, false);
    ExitProcess(0);
}

void pause_menu_root::OnCross(int a2)
{
    const int type = this->field_B0;
 
    switch (type) {
        case 0:  // EXIT MISSION
            this->transition_to_submenu(3);
            return;
 
        case 1:  // CITY GOALS

            this->transition_to_submenu(1);
            return;
 
        case 2:  // AWARDS  
            this->transition_to_submenu(2);
            return;
 
        case 3:  // GAME STATS  
            this->transition_to_submenu(4);
            return;
 
        case 4:  // SAVE GAME
            this->transition_to_submenu(5);
            return;
 
        case 5:  // LOAD GAME
            this->transition_to_submenu(6);
            return;
 
        case 6:  // OPTIONS
            this->transition_to_submenu(7);
            return;
 
        case 7:  // MESSAGE LOG  -- engine fallback

            THISCALL(0x00630460, this, a2);
            return;
 
        case 8:  // UNLOCKABLES
            this->field_AC->MakeDeactive(5);
            {
                static string_hash sfx_id_hash{"FE_PS_ACCEPT"};
                [[maybe_unused]] sound_instance_id id =
                    sub_60B960(sfx_id_hash, 1.0f, 1.0f);
            }
            return;
 
        case 9:  // debug-menu entry
            run_script("start_to_level_s01(debug_menu_entry)");
            return;
 
        default:
            return;
    }
}



void pause_menu_root::handle_restart_mission(Float a2) {
    if (!mission_manager::s_inst->is_mission_active()) {
		
        this->field_AC->MakeActive(8);
        comic_panels::game_play_panel()->field_67 = 1;
        
    static string_hash sfx_id_hash{"FE_PS_ACCEPT"};
    [[maybe_unused]] sound_instance_id id = sub_60B960(sfx_id_hash, 1.0f, 1.0f);
        return;
    }

    if (!mission_manager::s_inst->is_story_mission_active()) {
        return;
    }

    if (mission_manager::s_inst->is_story_active()) {
        sub_61C520();
    } else {
        setup_confirmation_dialog();
        
        this->field_F8 = 1;
        sub_61C610();
        this->field_F9 = 0;
        
        this->field_A8->SetNoFlash(color32 {0xFFC87238});
        this->field_A8->SetScale(1.0f, 1.0f);
        
        this->field_A4->SetNoFlash(color32{0xFFE6871F});
        this->field_A4->SetScale(1.157f, 1.157f);
        
		this->field_A0->SetText(static_cast<global_text_enum>(270));
		
    }
    
    static string_hash sfx_id_hash{"FE_PS_ACCEPT"};
    [[maybe_unused]] sound_instance_id id = sub_60B960(sfx_id_hash, 1.0f, 1.0f);
}

void pause_menu_root::handle_skip_cutscene(Float a2) {
    bool can_skip = mission_manager::s_inst->is_mission_active() &&
                    !mission_manager::s_inst->is_story_mission_active() &&
                    mission_manager::s_inst->is_story_active();
    
    if (can_skip) {
        setup_confirmation_dialog();
        
        this->field_F8 = 1;
        sub_61C610();
        this->field_F9 = 0;
        
        this->field_A8->SetNoFlash(color32{0xFFC87238});
        this->field_A8->SetScale(1.0f, 1.0f);
        
        this->field_A4->SetNoFlash(color32{0xFFE6871F});
        this->field_A4->SetScale(1.157f, 1.157f);
    }

    if (can_skip) {

		this->field_A0->SetText(static_cast<global_text_enum>(267));
        
    static string_hash sfx_id_hash{"FE_PS_ACCEPT"};
    [[maybe_unused]] sound_instance_id id = sub_60B960(sfx_id_hash, 1.0f, 1.0f);
    } else {
        dword_922908() = 2;
        g_cursor()->sub_5B0D70();
        g_cursor()->field_120 = 1;
        byte_922994() = 1;
        
    static string_hash sfx_id_hash{"FE_PS_ACCEPT"};
    [[maybe_unused]] sound_instance_id id = sub_60B960(sfx_id_hash, 1.0f, 1.0f);
    }
}

void pause_menu_root::handle_switch_hero(Float a2) {
    if (mission_manager::s_inst->is_story_active()) {
        return;
    }

    setup_confirmation_dialog();
    
    this->field_F8 = 1;
    sub_61C610();
    this->field_F9 = 1;
    
    this->field_A8->SetNoFlash(color32{0xFFE6871F});
    this->field_A8->SetScale(1.157f, 1.157f);
    
    this->field_A4->SetNoFlash(color32{0xFFC87238});
    this->field_A4->SetScale(1.0f, 1.0f);
    

	this->field_A0->SetText(static_cast<global_text_enum>(271));
    
    static string_hash sfx_id_hash{"FE_PS_ACCEPT"};
    [[maybe_unused]] sound_instance_id id = sub_60B960(sfx_id_hash, 1.0f, 1.0f);
}

void pause_menu_root::handle_confirmation_state(Float a2, int a3) {
    int menu_index = this->field_B0;
    
    if (this->field_F9) {
        if (menu_index == 5) {
            sub_61C520();
        } else if (menu_index == 7) {
            handle_skip_confirmation();
        } else if (menu_index == 8) {
            handle_hero_toggle();
        }
    }

    finalize_confirmation();
}

void pause_menu_root::handle_skip_confirmation() {
    bool can_skip = mission_manager::s_inst->is_mission_active() &&
                    !mission_manager::s_inst->is_story_active() &&
                    mission_manager::s_inst->is_story_mission_active();
    
    if (can_skip) {
	            string_hash s_progression_mission_aborted;
            s_progression_mission_aborted.initialize(0, "progression_mission_aborted()", 0);
		
					    auto* v1 = script::gsoi()->parent;
			int function = script::find_function(s_progression_mission_aborted, v1, 0);
			script::new_thread(function, reinterpret_cast<script_instance*>(script::gsoi()));
    

            script::sub_5028B0(s_progression_mission_aborted, script::gsoi());
            script::exec_thread(0);
        
        set_menu_state(21);
        this->field_AC->MakeActive(1);
        
        if (!this->field_AC->field_10) {
            comic_panels::game_play_panel()->field_67 = 0;
        }
    } else {
	    this->field_A0->SetText(static_cast<global_text_enum>(269));
        
        byte_965BF8() = 1;
        byte_922994() = 1;
        dword_922908() = 1;
    }
}

void pause_menu_root::handle_hero_toggle() {
    bool is_spidey = g_world_ptr->get_hero_ptr(0);
    this->field_34 = is_spidey;
    
	            string_hash s_toggle_hero;
            s_toggle_hero.initialize(0, "switch_to_venom()", 0);
			    auto* v1 = script::gsoi()->parent;
			int function = script::find_function(s_toggle_hero, v1, 0);
			script::new_thread(function, reinterpret_cast<script_instance*>(script::gsoi()));
    

            script::sub_5028B0(s_toggle_hero, script::gsoi());
            script::exec_thread(0);
    
    set_menu_state(21);
    this->field_AC->MakeActive(1);
    
    if (!this->field_AC->field_10) {
        comic_panels::game_play_panel()->field_67 = 0;
    }
	//Sleep(500);
	pause_menu_system_ptr->Deactivate();
	
}

void sub_62A840()
{

     CDECL_CALL(0x0062A840);
}

void pause_menu_root::finalize_confirmation() {
    if (this->field_2D) {
        return;
    }

    auto *widget = get_current_widget();
    reset_widget_state(0);
    
    this->field_F8 = 0;
    sub_62A840();
    
	    static string_hash sfx_id_hash{"fe_wb_accept"};
    [[maybe_unused]] sound_instance_id id = sub_60B960(sfx_id_hash, 1.0f, 1.0f);
}

void pause_menu_root::setup_confirmation_dialog() {
    auto *widget = get_current_widget();
    reset_widget_state(0);
}

int *pause_menu_root::get_current_widget() {
    return reinterpret_cast<int *>(
        *reinterpret_cast<int *>(
            reinterpret_cast<int *>(this->field_AC->field_4)[1] + 68
        )
    );
}




void pause_menu_root::handle_objectives(float* a2) {

    if (mission_manager::s_inst->is_story_active() && !mission_manager::s_inst->is_story_mission_active()) {
        return; 
    }

    this->field_AC->MakeActive(1);

    if (!this->field_AC->m_count) {
        comic_panels::game_play_panel()->field_67 = 0;
    }

    static string_hash sfx_id_hash{"FE_PS_ACCEPT"};
    [[maybe_unused]] sound_instance_id id = sub_60B960(sfx_id_hash, 1.0f, 1.0f);
}




void pause_menu_root::sub_61C520()
{
	 THISCALL(0x0061C520,this);
}




void pause_menu_root_patch() {

    {
        FUNC_ADDRESS(address, &pause_menu_root::_Load);
        set_vfunc(0x00893F48, address);
    }

    {
        FUNC_ADDRESS(address, &pause_menu_root::OnCross);
        set_vfunc(0x00893F84, address);
    }


    return;

	    {
        FUNC_ADDRESS(address, &pause_menu_root2::Draw);
        set_vfunc(0x00893F50, address);
    }
	    {
        FUNC_ADDRESS(address, &pause_menu_root::Update);
        set_vfunc(0x00893F58, address);
    }
	{
        FUNC_ADDRESS(address, &pause_menu_root2::OnUp);
        set_vfunc(0x00893F74, address);
    }

    {
        FUNC_ADDRESS(address, &pause_menu_root2::OnDown);
       set_vfunc(0x00893F78, address);
    }
	    {
        FUNC_ADDRESS(address, &pause_menu_root2::update_selected);
        REDIRECT(0x0060E894, address);
    }
	{
        FUNC_ADDRESS(address, &pause_menu_root2::OnActivate);
        set_vfunc(0x00893F64, address);
    }
	{
        FUNC_ADDRESS(address, &pause_menu_root2::OnDeactivate);
        set_vfunc(0x00893F68, address);
    }
	{
	    FUNC_ADDRESS(address, &pause_menu_root::update_switching_heroes);
	    REDIRECT(0x006490C4, address);
    }
	

}


pause_menu_root *& pause_menu_root_ptr = var<pause_menu_root*>(0x0965C21);


pause_menu_root2 *& pause_menu_root2_ptr = var<pause_menu_root2*>(0x0965C21);