#include "pausemenusystem.h"

#include "alternate_costumes.h"
#include "character_viewer.h"
#include "comic_panels.h"
#include "common.h"
#include "concept_art.h"
#include "concept_art2.h"
#include "covers.h"
#include "fe_dialog_text.h"
#include "fe_menu_nav_bar.h"
#include "fe_mini_map_widget.h"
#include "func_wrapper.h"
#include "game.h"
#include "igofrontend.h"
#include "landmarks.h"
#include "ltd_edition.h"
#include "memory.h"
#include "panelfile.h"
#include "pause_menu_controller.h"
#include "pause_menu_credits.h"
#include "pause_menu_message_log.h"
#include "pause_menu_options_display.h"
#include "pause_menu_root.h"
#include "pause_menu_save_load_display.h"
#include "pause_menu_save_load_hookup.h"
#include "pause_menu_status.h"
#include "pause_menu_transition.h"
#include "trace.h"
#include "unlockables_menu.h"
#include "utility.h"
#include "debug_menu.h"
#include "os_developer_options.h"

#include "memoryunitmanager.h"

#include "tlresource_directory.h"
#include "resource_directory.h"
#include "ngl.h"

#include <cassert>

VALIDATE_SIZE(PauseMenuSystem, 0x3cu);

PauseMenuSystem *& pause_menu_system_ptr = var<PauseMenuSystem*>(0x00937B1C);

PauseMenuSystem::PauseMenuSystem(font_index a2) : FEMenuSystem(17, a2) {
    if constexpr (1) {
        this->m_vtbl = 0x00893E00;
        this->field_2C = nullptr;
        this->field_30 = new menu_nav_bar{};

        this->field_4[this->m_count++] = new fe_dialog_text {this, 320, 240};

        this->field_4[this->m_count++] = new pause_menu_transition {this, 320, 240};

        this->field_4[this->m_count++] = new pause_menu_root {this, 320, 240};

        this->field_4[this->m_count++] = new pause_menu_status {this, 320, 240};

        this->field_4[this->m_count++] = new pause_menu_options_display {this, 320, 240};

        this->field_4[this->m_count++] = new pause_menu_controller {this, 320, 240};

        this->field_4[this->m_count++] = new pause_menu_save_load_display {this, 320, 240};

        this->field_4[this->m_count++] = new pause_menu_message_log {this, 320, 240};

        this->field_4[this->m_count++] = new unlockables_menu {this, 320, 240};

        this->field_4[this->m_count++] = new pause_menu_credits {this, 320, 240};

        this->field_4[this->m_count++] = new character_viewer {this, 320, 240};

        this->field_4[this->m_count++] = new alternate_costumes {this, 320, 240};

        this->field_4[this->m_count++] = new concept_art {this, 320, 240};

        this->field_4[this->m_count++] = new concept_art2 {this, 320, 240};
        this->field_4[this->m_count++] = new covers {this, 320, 240};

        this->field_4[this->m_count++] = new landmarks {this, 320, 240};

        this->field_4[this->m_count++] = new ltd_edition {this, 320, 240};

        this->LoadAll();
        this->field_34 = CAST(this->field_34, *(this->field_4 + 6));
    } else {
        THISCALL(0x00647E50, this, a2);
    }
}


bool PauseMenuSystem::IsDialogActivated() {
    return this->m_index == 0;
}

void PauseMenuSystem::LoadAll() {
    TRACE("PauseMenuSystem::LoadAll");

    if constexpr (1)
    {
        this->field_2C = PanelFile::UnmashPanelFile("pause_menu_interface", static_cast<panel_layer>(2));
        this->field_4[0]->Load();
        this->field_4[1]->Load();
        this->field_4[2]->Load();
        this->field_4[3]->Load();
        this->field_4[4]->Load();
        this->field_4[5]->Load();
        this->field_4[6]->Load();
        this->field_4[7]->Load();
        this->field_4[9]->Load();

        this->field_30->text_box = this->field_2C->GetTextPointer("pm_nav_bar_text_01");
        this->field_30->background_a = this->field_2C->GetPQ("pm_nav_bar_01");
        this->field_30->field_1C = this->field_2C->GetPQ("pm_nav_bar_02");
        this->field_30->Load();
    }
    else
    {
        THISCALL(0x006430F0, this);
    }
}

void PauseMenuSystem::Draw() {
    THISCALL(0x0060BF10, this);
}

bool sub_7B1EE0() {
    return (bool) CDECL_CALL(0x007B1EE0);
}

void PauseMenuSystem::Activate(int a2, bool a3)
{
    if (this->m_index < 0)
    {
        this->MakeActive(a2);
        
        if (a3)
            g_game_ptr->pause();
        pause_menu_save_load_display* v4 = this->field_34;
        this->field_38 = static_cast<uint8_t>(comic_panels::game_play_panel()->field_67);
    
    MemoryUnitManager::RegisterInsertRemoveObserver(
      reinterpret_cast<MemoryUnitManager::InsertRemoveObserver*>(v4->field_2C));
	  
	  				    Cursor *cur = g_cursor();
    if (cur != nullptr) {
        cur->field_120 = 0;
		        cur->field_114 = 0;     
    }

		THISCALL(0x0060BE90, this, a2 ,a3);
    }
}

void PauseMenuSystem::Deactivate()
{
    if (this->m_index >= 0)
    {
        this->MakeActive(0);
        g_game_ptr->unpause();
        comic_panels::game_play_panel()->field_67 = this->field_38;
							static string_hash sfx_id_hash{"fe_ps_udscroll"};

        [[maybe_unused]] sound_instance_id id = sub_60B960(sfx_id_hash, 1.0, 1.0);
    }
	THISCALL(0x0060BEE0, this);
}



void PauseMenuSystem::Update(Float a2)
{
    if constexpr (1)
    {
        int idx = this->m_index;
        if (idx >= 0)
        {
            if (idx == 0)
            {
                auto *dialog_text = bit_cast<fe_dialog_text *>(this->field_4[0]);

                if (dialog_text->field_9C != 3)
                {
                    auto *mini_map_widget = g_femanager.IGO->field_4;
                    auto *vtbl = bit_cast<fastcall_call(*)[4]>(mini_map_widget->m_vtbl);

                    auto *func = (*vtbl)[3];
                    assert(bit_cast<std::intptr_t>(func) == 0x00641810);

                    mini_map_widget->Update(a2);
                }
            }

            auto *v4 = this->field_34->field_2C;
            v4->field_24 = sub_7B1EE0();
            int idx1 = this->m_index;
            if (idx1 >= 0)
            {
                auto **v6 = this->field_4;

                if (v6[idx1] != nullptr) {
                    v6[idx1]->Update(a2);
                }
            }

            {
                auto *vtbl = bit_cast<fastcall_call(*)[7]>(this->m_vtbl);

                auto *func = (*vtbl)[6];
                assert(bit_cast<std::intptr_t>(func) == 0x006298D0);

                this->UpdateButtonPresses();
            }

            this->field_2C->Update(a2);

            if ((g_game_ptr->field_165 || g_game_ptr->field_166) && this->m_index >= 0)
            {
                auto *vtbl = bit_cast<fastcall_call(*)[7]>(this->m_vtbl);

                auto *func = (*vtbl)[3];
                assert(bit_cast<std::intptr_t>(func) == 0x0060B610);

                this->MakeActive(-1);
                g_game_ptr->unpause();
                comic_panels::game_play_panel()->field_67 = this->field_38;
            }
        }
    }
    else
    {
        THISCALL(0x0062F0C0, this, a2);
    }
}

void PauseMenuSystem::SetTransition(int a1) {
    auto *pause_menu = bit_cast<pause_menu_transition *>(this->field_4[1]);
    pause_menu->set_transition(a1);
}

void PauseMenuSystem::UpdateButtonPresses() {
    FEMenuSystem::UpdateButtonPresses();
}

   void PauseMenuSystem::OnButtonPress(int a2, int a3) {
       if (this->m_index < 0) return;
       const int back_btn = 13;
       if (a2 == 1
           && (this->m_index == 1 || this->m_index == 2)
           && a3 == back_btn
           && os_developer_options::instance->get_flag("ENABLE_ZOOM_MAP"))
       {
           this->Deactivate();
           debug_menu::show();
           return;
       }
       FEMenuSystem::OnButtonPress(a2, a3);
   }





void PauseMenuSystem_patch() {

    {
        FUNC_ADDRESS(address, &PauseMenuSystem::LoadAll);
        REDIRECT(0x00648557, address);
		

    }
	{
        FUNC_ADDRESS(address, &PauseMenuSystem::OnButtonPress);
        set_vfunc(0x00893E38, address);
	
    }

	{
	FUNC_ADDRESS(address, &PauseMenuSystem::Activate);
	REDIRECT(0x00673136, address);
		REDIRECT(0x00673338, address);
			REDIRECT(0x006734D5, address);
				REDIRECT(0x006737D5, address);
	
    }	
    return;


	

}
