#include "frontendmenusystem.h"

#include "common.h"
#include "femenu.h"
#include "func_wrapper.h"
#include "game.h"
#include "game_settings.h"
#include "main_menu_credits.h"
#include "main_menu_keyboard.h"
#include "main_menu_legal.h"
#include "main_menu_load.h"
#include "main_menu_memcard_check.h"
#include "main_menu_options.h"
#include "main_menu_start.h"
#include "memory.h"
#include "mission_manager.h"
#include "movie_manager.h"
#include "ngl_scene.h"
#include "os_developer_options.h"
#include "panelfile.h"
#include "utility.h"
#include "variables.h"
#include "vtbl.h"
#include "memoryunitmanager.h"

#include "sound_manager.h"

#include "game.h"

#include "fetext.h"


VALIDATE_SIZE(FrontEndMenuSystem, 0x80);

bool & already_drew_this_frame = var<bool>(0x0096B44A);

FrontEndMenuSystem::FrontEndMenuSystem() : FEMenuSystem(7, static_cast<font_index>(1)) {
    if constexpr (1) {
        this->field_50 = 0;
        this->field_51 = 0;
        this->field_52 = 0;

        static Var<bool> first_time_through{0x00937B78};

        if (first_time_through())
        {
            this->field_4[this->m_count++] = new main_menu_legal {this, 320, 240};

            this->field_4[this->m_count++] = new main_menu_start {this, 320, 240};

            this->field_4[this->m_count++] = new main_menu_memcard_check {this, 320, 240};

            this->field_4[this->m_count++] = new main_menu_options {this, 320, 240};

            this->field_4[this->m_count++] = new main_menu_load {this, 320, 240};

            this->field_4[this->m_count++] = new main_menu_keyboard {this, 320, 240};

            this->field_4[this->m_count++] = new main_menu_credits {this, 320, 240};

            first_time_through() = false;
            this->field_50 = false;
        } else {
            this->field_50 = true;
        }

        this->field_34 = 0;
        this->field_51 = 0;
        this->field_52 = 0;
        this->field_7C = PanelFile::UnmashPanelFile("main_menu", static_cast<panel_layer>(7));
        for (int i = 0; i < 7; ++i) {
            this->field_4[i]->Init();
        }

        static Var<bool> byte_96B44B{0x0096B44B};

        auto v17 = !byte_96B44B();
        auto v18 = this->m_index;
        this->field_34 = v18;
        if (v17) {
            this->field_30 = 1;
            if (v18 != -1) {
                this->field_4[v18]->OnDeactivate(this->field_4[0]);
            }

            auto **v21 = this->field_4;
            this->m_index = 0;
            (*v21)->OnActivate();
            this->UpdateButtonDown();

        } else {
            this->field_30 = 3;
            if (v18 != -1) {
                this->field_4[v18]->OnDeactivate(*(this->field_4 + 1));
            }

            auto **v19 = this->field_4;
            this->m_index = 1;
            v19[1]->OnActivate();
            this->UpdateButtonDown();
            this->sub_60C240();
        }

    } else {
		
	  THISCALL(0x00648580, this);

    }
}

void FrontEndMenuSystem::sub_60C240() {
		    void (__fastcall *func)(void *, void *) = (decltype(func)) 0x0060c240;
        func(this, nullptr);
}

bool FrontEndMenuSystem::WaitForMemCheck() {
    return this->field_30 != 10 &&
        !os_developer_options::instance->os_developer_options::get_flag(static_cast<os_developer_options::flags_t>(66));
}

bool FrontEndMenuSystem::sub_60C230() {
    return this->field_30 != 10;
}

void FrontEndMenuSystem::sub_619030(bool a2) {
	    void (__fastcall *func)(void *, void *, bool) = (decltype(func)) 0x00619030;
        func(this, nullptr, a2);
}

void FrontEndMenuSystem::MakeActive(int a2) {
    auto idx = this->m_index;
    this->field_34 = idx;
    if (idx != -1) {
        FEMenu *v4;
        if (a2 < 0) {
            v4 = nullptr;
        } else {
            v4 = this->field_4[a2];
        }

        auto *v8 = v4;
        auto *v5 = this->field_4[idx];

        v5->OnDeactivate(v8);
    }

    this->m_index = a2;

    if (a2 >= 0) {
        auto *v6 = this->field_4[a2];

        v6->OnActivate();
    }

    this->UpdateButtonDown();
}

void FrontEndMenuSystem::sub_612F40(float a2)
{
	THISCALL(0x00612F40, this, a2);
}


void FrontEndMenuSystem::Update(float a2)
{
        // Sleep(20000);
	
	      movie_manager::load_and_play_movie("attract", nullptr, false);

}

static Var<bool> sounds_paused2{0x00960044};



void FrontEndMenuSystem::RenderLoadMeter(bool a1) {
    if constexpr (1) {
        if (!os_developer_options::instance->get_flag(mString{"NO_LOAD_SCREEN"})) {
            this->sub_619030(a1);
        }
    } else {
		
	    void  (__fastcall *func)(void *, void *,bool) = (decltype(func)) 0x00619230;
        func(this, nullptr, a1);
    }
}

void FrontEndMenuSystem::GoNextState() {
    if (this->field_30 == 10) {
        return;
    }

    int v3;
    while (2) {
        auto v2 = this->field_30;
        switch (v2) {
        case 5:
            this->field_50 = 1;
            this->field_30 = 6;
            v3 = this->field_30;
            goto LABEL_28;
        case 0:
            this->field_50 = 1;
            this->field_30 = 1;
            v3 = 1;
            goto LABEL_28;
        case 4:
            this->field_52 = 1;
            this->field_30 = 5;
            v3 = 5;
            goto LABEL_28;
        case 6: {
            switch (this->field_4[this->m_index][5].field_28) {
            case 0: {
                g_game_ptr->gamefile->load_most_recent_game();													
                this->BringUpDialogBox(10, fe_state{10}, fe_state{10});
                goto LABEL_27;
            }
            case 1:
                this->field_30 = 7;
                v3 = this->field_30;
                break;
            case 2:
                this->field_30 = 9;
                v3 = this->field_30;
                break;
            case 3:
                this->field_30 = 8;
                v3 = this->field_30;
                break;
            case 4:
                sub_5A6D70();
                goto LABEL_27;
            case 5: {
                bExit() = true;
                goto LABEL_27;
            }
            default:
                goto LABEL_27;
            }

            goto LABEL_28;
        }
        case 7: {
            this->field_30 = 10;
            v3 = 10;
            goto LABEL_28;
        }
        case 11:
            if (bit_cast<main_menu_memcard_check *>(this->field_4[2])->field_100) {
                this->field_30 = this->field_58;
            } else {
                this->field_30 = this->field_54;
            }

            if (this->field_5C.m_size > 0) {
                int v5;
                this->field_54 = *(uint32_t *) *this->field_5C.sub_64A090(&v5);

                int v6;
                this->field_58 = *(uint32_t *) *this->field_6C.sub_64A090(&v6);
                this->field_5C.sub_64A2B0();
                this->field_6C.sub_64A2B0();
            }

            break;
        case 3:
            this->field_30 = 4;
            v3 = 4;
						
            goto LABEL_28;
        default:
            this->field_30 = v2 + 1;
            break;
        }

    LABEL_27:
        v3 = this->field_30;
    LABEL_28:
        switch (v3) {
        case 0:
        case 11: {
            if (this->field_34 != 2) {
                goto LABEL_42;
            }

            auto v4 = this->field_30;
            this->sub_6342D0();
            this->field_30 = v4;
            break;
        }
        case 1: {
            this->MakeActive(false);
            break;
        }
        case 2: {
            if (nglCurScene() != nullptr) {
                --this->field_30;
            } else {

										movie_manager::load_and_play_movie("company", nullptr, false);


                if (this->field_30 != 10) {
                    continue;
				//	Sleep(10000);

                }
            }

            break;
        }
        case 3: {
            this->MakeActive(true);
            this->sub_60C240();
            break;
        }
        case 4:
            if (this->m_index != 1) {
                this->MakeActive(true);
				     static string_hash sfx_id_hash{"FE_MM_THROB"};
				        [[maybe_unused]] sound_instance_id id = sub_60B960(sfx_id_hash, 1.0, 1.0);
            }
															   

                
            this->field_52 = true;
            break;
        case 5: {						
        LABEL_42:
            this->MakeActive(2);
			                sound_manager::fade_sounds_by_type(127u, 0.0, 0.13333334, 1);
                sounds_paused2() = true;
            break;
        }
        case 6:
            this->MakeActive(3);
            break;
        case 7:
            this->MakeActive(5);
            break;
        case 8:
            this->MakeActive(6);
            break;
        case 9:
            this->MakeActive(4);
            break;
        case 10: {
            if (this->m_index != 5) {
                mission_manager::s_inst->sub_5BACA0(0.0);
            }

            this->sub_60C290();
            break;
        }
        default:
            return;
        }
        break;
    }
}

void FrontEndMenuSystem::BringUpDialogBox(int a2,
                                          FrontEndMenuSystem::fe_state a3,
                                          FrontEndMenuSystem::fe_state a4) {
	
		    void  (__fastcall *func)(void *, void *,int,  FrontEndMenuSystem::fe_state, FrontEndMenuSystem::fe_state) = (decltype(func)) 0x00634300;
        func(this, nullptr, a2 ,a3, a4);
}

void FrontEndMenuSystem::sub_60C290() {
	
			    void  (__fastcall *func)(void *, void *) = (decltype(func)) 0x0060C290;
        func(this, nullptr);
}

void FrontEndMenuSystem::sub_6342D0() {
			    void  (__fastcall *func)(void *, void *) = (decltype(func)) 0x006034D0;
        func(this, nullptr);
}

void FrontEndMenuSystem_patch() {
    FUNC_ADDRESS(address, &FrontEndMenuSystem::MakeActive);
    set_vfunc(0x0089A27C, address);

    {
        FUNC_ADDRESS(address, &FrontEndMenuSystem::GoNextState);
        SET_JUMP(0x00635BC0, address);
    }
	
    {
        FUNC_ADDRESS(address, &FrontEndMenuSystem::Update);
       // set_vfunc(0x0089A284, address);
    }
}
