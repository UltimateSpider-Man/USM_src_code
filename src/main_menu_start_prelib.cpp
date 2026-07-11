#include "main_menu_start_prelib.h"

#include "common.h"
#include "func_wrapper.h"

#include "panelquad.h"
#include "game.h"

#include "utility.h"

#include "movie_manager.h"

VALIDATE_SIZE(main_menu_start_prelib, 0x130);

main_menu_start_prelib::main_menu_start_prelib(FrontEndMenuSystem *a2, int a3, int a4) : FEMenu(a2, 0, a3, a4, 8, 0) {
    this->field_128 = 0;
    this->field_12C = a2;
    this->field_120 = 0.0;
    this->field_124 = 0.0;
    this->field_12A = 0;
    this->field_128 = 0;
}

namespace {
	
bool group_is_active(int group)
{
    return *bit_cast<char *>(group + 45) != 0;
}
 
void rearm_anim_group(int group, int dir, int loop)
{
    int count = *bit_cast<int *>(group + 4);
    if (count) {
        int *children = *bit_cast<int **>(group + 8);
        for (int i = 0; i < count; ++i) {
            // child->field_14 sub-object; PC virtual __thiscall at vtbl+0x20 -> TurnOn(true)
            int obj  = *bit_cast<int *>(children[i] + 0x14);
            int vtbl = *bit_cast<int *>(obj);
            auto fn  = bit_cast<void(__thiscall *)(void *, int)>(*bit_cast<void **>(vtbl + 0x20));
            fn(bit_cast<void *>(obj), 1);
        }
    }
    *bit_cast<int *>(group + 24)   = 0;
    *bit_cast<int *>(group + 28)   = 0;
    *bit_cast<float *>(group + 32) = *bit_cast<float *>(group + 20);
    *bit_cast<int *>(group + 36)   = dir;
    *bit_cast<int *>(group + 40)   = loop;
    *bit_cast<char *>(group + 44)  = 0;
    *bit_cast<char *>(group + 45)  = 1;
}
}
 
void main_menu_start_prelib::Update(Float a2)
{
    if constexpr (1)
    {
        auto *fe = this->field_12C;                                         // field_12C = FrontEndMenuSystem*
        const int fe_state = *bit_cast<int *>(bit_cast<char *>(fe) + 0x30); // fe->field_30 (FE state)
        auto &state = this->field_12A;                                      // field_12A = state (u16)
 
        // game flag at +0x4D: when set, the menu fades in fast and may advance
        const bool fast = *(bit_cast<char *>(g_game_ptr) + 0x4D) != 0;
 
        // --- intro fade-in + auto-advance (no attract movie on PC) ---
        if (fe_state == 3) {
            if (state >= 2) {
                this->field_120 += fast ? 0.02f : 0.002f;    // 0x889850 / 0x895ac8
                if (this->field_120 > 1.0f)                   // clamp (0x86f840)
                    this->field_120 = 1.0f;
                if (fast && this->field_120 >= 0.99f) {       // advance gate (0x87eb44)
                    state = 4;
                    fe->GoNextState();
                }
            }
        } else if (fe_state == 4) {
            // idle on the main menu: after 30s play the attract movie (PS2 behaviour)
            this->field_124 += a2;
            if (this->field_124 <= 30.0f) {
                this->field_128 = false;
            } else {
                this->field_124 = 0.0f;
                movie_manager::load_and_play_movie("attract", "attract", true);
                this->field_128 = true;
            }
        }
 
        // --- visual-state sequencer (each transition returns) ---
        if (state == 0 && fe_state == 3 && !group_is_active(this->field_C8)) {   // group[0]
            this->pq_bkg_grey_a05->TurnOn(false);
            this->pq_bkg_grey_a08->TurnOn(false);
            this->pq_bkg_grey_a09->TurnOn(false);
            this->pq_pre_main_back_b->TurnOn(false);
            this->pq_pre_main_screen->TurnOn(false);
            this->pq_pre_main_screen_0->TurnOn(false);
            this->pq_pre_main_screen_1->TurnOn(false);
            this->pq_pre_main_screen_2->TurnOn(false);
            this->pq_pre_main_spider->TurnOn(false);
            this->pq_bkg_white01->TurnOn(true);
            state = 1;
            rearm_anim_group(this->field_CC, 0, 0);                              // group[1]
            for (int i = 0; i < 15; ++i)
                rearm_anim_group((&this->field_DC)[i], 0, 1);                    // groups[5..19]
            return;
        }
        if (state == 1 && fe_state == 3 && !group_is_active(this->field_CC)) {   // group[1]
            state = 3;
            this->text_mainmenu_0->SetShown(true);
            this->pq_loading_bar01->TurnOn(true);
            this->pq_loading_bar02->TurnOn(true);
            this->pq_loading_bar_ga->TurnOn(true);
            this->pq_bkg_white01->TurnOn(false);
            rearm_anim_group(this->field_D0, 1, 0);                              // group[2], dir=1
            return;
        }
        if (state == 4 && fe_state == 4 && !group_is_active(this->field_D0)) {   // group[2]
            rearm_anim_group(this->field_D0, 0, 0);
            state = 5;
            return;
        }
        if (state == 5 && fe_state == 4 && !group_is_active(this->field_D0)) {   // group[2]
            this->text_mainmenu_0->SetShown(false);
            this->pq_loading_bar01->TurnOn(false);
            this->pq_loading_bar02->TurnOn(false);
            this->pq_loading_bar_ga->TurnOn(false);
            this->text_mainmenu->SetShown(true);
            this->pq_mainmenu_box_h->TurnOn(true);
            this->pq_mainmenu_box_h_0->TurnOn(true);
            rearm_anim_group(this->field_D4, 0, 0);                              // group[3]
            state = 6;
            return;
        }
        if (state == 6 && fe_state == 4 && !group_is_active(this->field_D4)) {   // group[3]
            rearm_anim_group(this->field_D8, 0, 1);                              // group[4], looping
            state = 7;
            // FE_MM_Throb. PC fires fire-and-forget (id discarded), matching every
            // other FE sound; PS2 stored the id at a1+0x12C to stop it later.
            string_hash throb_hash{"FE_PW_OUT"};
            [[maybe_unused]] sound_instance_id id = sub_60B960(throb_hash, 1.0, 1.0);
            return;
        }
    }
    else
    {
        		        void(__fastcall * func)(void*, void*, Float) = bit_cast<decltype(func)>(0x00636AA0);

        func(this, nullptr, a2);
     }
}

void main_menu_start_prelib::Update_build(Float a2)
{
    if constexpr (1)
    {
        auto *fe = this->field_12C;                                         // field_12C = FrontEndMenuSystem*
        const int fe_state = *bit_cast<int *>(bit_cast<char *>(fe) + 0x30); // fe->field_30 (FE state)
        auto &state = this->field_12A;                                      // field_12A = state (u16)
 
        // game flag at +0x4D: when set, the menu fades in fast and may advance
        const bool fast = *(bit_cast<char *>(g_game_ptr) + 0x4D) != 0;
 
        // --- intro fade-in + auto-advance (no attract movie on PC) ---
        if (fe_state == 3) {
            if (state >= 2) {
                this->field_120 += fast ? 0.02f : 0.002f;    // 0x889850 / 0x895ac8
                if (this->field_120 > 1.0f)                   // clamp (0x86f840)
                    this->field_120 = 1.0f;
                if (fast && this->field_120 >= 0.99f) {       // advance gate (0x87eb44)
                    state = 4;
                    fe->GoNextState();
                }
            }
        } else if (fe_state == 4) {
            // idle on the main menu: after 30s play the attract movie (PS2 behaviour)
            this->field_124 += a2;
            if (this->field_124 <= 30.0f) {
                this->field_128 = false;
            } else {
                this->field_124 = 0.0f;
                movie_manager::load_and_play_movie("attract", "attract", true);
                this->field_128 = true;
            }
        }
 
        // --- visual-state sequencer (each transition returns) ---
        if (state == 0 && fe_state == 3 && !group_is_active(this->field_C8)) {   // group[0]
            this->pq_bkg_grey_a05->TurnOn(false);
            this->pq_bkg_grey_a08->TurnOn(false);
            this->pq_bkg_grey_a09->TurnOn(false);
            this->pq_pre_main_back_b->TurnOn(false);
            this->pq_pre_main_screen->TurnOn(false);
            this->pq_pre_main_screen_0->TurnOn(false);
            this->pq_pre_main_screen_1->TurnOn(false);
            this->pq_pre_main_screen_2->TurnOn(false);
            this->pq_pre_main_spider->TurnOn(false);
            this->pq_bkg_white01->TurnOn(true);
            state = 1;
            rearm_anim_group(this->field_CC, 0, 0);                              // group[1]
            for (int i = 0; i < 15; ++i)
                rearm_anim_group((&this->field_DC)[i], 0, 1);                    // groups[5..19]
            return;
        }
        if (state == 1 && fe_state == 3 && !group_is_active(this->field_CC)) {   // group[1]
            state = 3;
            this->text_mainmenu_0->SetShown(true);
            this->pq_loading_bar01->TurnOn(true);
            this->pq_loading_bar02->TurnOn(true);
            this->pq_loading_bar_ga->TurnOn(true);
            this->pq_bkg_white01->TurnOn(false);
            rearm_anim_group(this->field_D0, 1, 0);                              // group[2], dir=1
            return;
        }
        if (state == 4 && fe_state == 4 && !group_is_active(this->field_D0)) {   // group[2]
            rearm_anim_group(this->field_D0, 0, 0);
            state = 5;
            return;
        }
        if (state == 5 && fe_state == 4 && !group_is_active(this->field_D0)) {   // group[2]
            this->text_mainmenu_0->SetShown(false);
            this->pq_loading_bar01->TurnOn(false);
            this->pq_loading_bar02->TurnOn(false);
            this->pq_loading_bar_ga->TurnOn(false);
            this->text_mainmenu->SetShown(true);
            this->pq_mainmenu_box_h->TurnOn(true);
            this->pq_mainmenu_box_h_0->TurnOn(true);
            rearm_anim_group(this->field_D4, 0, 0);                              // group[3]
            state = 6;
            return;
        }
        if (state == 6 && fe_state == 4 && !group_is_active(this->field_D4)) {   // group[3]
            rearm_anim_group(this->field_D8, 0, 1);                              // group[4], looping
            state = 7;
            // FE_MM_Throb. PC fires fire-and-forget (id discarded), matching every
            // other FE sound; PS2 stored the id at a1+0x12C to stop it later.
            string_hash throb_hash{"FE_PW_OUT"};
            [[maybe_unused]] sound_instance_id id = sub_60B960(throb_hash, 1.0, 1.0);
            return;
        }
    }
    else
    {
        		        void(__fastcall * func)(void*, void*, Float) = bit_cast<decltype(func)>(0x00636AA0);

        func(this, nullptr, a2);
     }
}



void main_menu_start_prelib::Init()
{
    PanelFile *panel_file = this->field_12C->field_7C;

    auto load_pq = [panel_file](const char *name) -> PanelQuad* {
        PanelQuad *pq = panel_file->GetPQ(name);
        pq->TurnOn(false);
        return pq;
    };

    this->pq_bkg_city           = load_pq("mm_bkg_city");
    this->pq_bkg_detail01       = load_pq("mm_bkg_detail01");
    this->pq_bkg_detail02       = load_pq("mm_bkg_detail02");
    this->pq_bkg_grey_a01       = load_pq("mm_bkg_grey_a01");
    this->pq_bkg_grey_a02       = load_pq("mm_bkg_grey_a02");
    this->pq_logo_main          = load_pq("mm_logo_main");
    this->pq_bkg_city01         = load_pq("mm_bkg_city01");
    this->pq_bkg_city02         = load_pq("mm_bkg_city02");
    this->pq_bkg_grey_a03       = load_pq("mm_bkg_grey_a03");
    this->pq_bkg_grey_a04       = load_pq("mm_bkg_grey_a04");
    this->pq_bkg_grey_a05       = load_pq("mm_bkg_grey_a05");
    this->pq_bkg_grey_a06       = load_pq("mm_bkg_grey_a06");
    this->pq_bkg_grey_a07       = load_pq("mm_bkg_grey_a07");
    this->pq_bkg_grey_a08       = load_pq("mm_bkg_grey_a08");
    this->pq_bkg_grey_a09       = load_pq("mm_bkg_grey_a09");
    this->pq_pre_main_back_b    = load_pq("mm_pre_main_back_b");
    this->pq_pre_main_screen    = load_pq("mm_pre_main_screen");
    this->pq_pre_main_screen_0  = load_pq("mm_pre_main_screen_0");
    this->pq_pre_main_screen_1  = load_pq("mm_pre_main_screen_1");
    this->pq_pre_main_screen_2  = load_pq("mm_pre_main_screen_2");
    this->pq_pre_main_spider    = load_pq("mm_pre_main_spider");
    this->pq_bkg_white01        = load_pq("mm_bkg_white01");
    this->pq_bkg_detail_lig     = load_pq("mm_bkg_detail_lig");
    this->pq_bkg_detail_lig_0   = load_pq("mm_bkg_detail_lig_0");
    this->pq_bkg_detail_lig_1   = load_pq("mm_bkg_detail_lig_1");
    this->pq_bkg_detail_lig_2   = load_pq("mm_bkg_detail_lig_2");
    this->pq_bkg_detail_lig_3   = load_pq("mm_bkg_detail_lig_3");
    this->pq_bkg_detail_lig_4   = load_pq("mm_bkg_detail_lig_4");
    this->pq_bkg_detail_dar     = load_pq("mm_bkg_detail_dar");
    this->pq_bkg_detail_dar_0   = load_pq("mm_bkg_detail_dar_0");
    this->pq_bkg_detail_dar_1   = load_pq("mm_bkg_detail_dar_1");
    this->pq_bkg_detail_dar_2   = load_pq("mm_bkg_detail_dar_2");
    this->pq_bkg_detail_dar_3   = load_pq("mm_bkg_detail_dar_3");
    this->pq_bkg_detail_dar_4   = load_pq("mm_bkg_detail_dar_4");
    this->pq_mainmenu_box_h     = load_pq("mm_mainmenu_box_h");
    this->pq_mainmenu_box_h_0   = load_pq("mm_mainmenu_box_h_0");
    this->pq_loading_bar01      = load_pq("mm_loading_bar01");
    this->pq_loading_bar02      = load_pq("mm_loading_bar02");
    this->pq_loading_bar_ga     = load_pq("mm_loading_bar_ga");

    // Load text elements
    this->text_mainmenu = panel_file->GetTextPointer("mm_mainmenu_text");
    if (this->text_mainmenu) {
        this->text_mainmenu->SetShown(false);
    }

    this->text_mainmenu_0 = panel_file->GetTextPointer("mm_mainmenu_text_0");
    if (this->text_mainmenu_0) {
        this->text_mainmenu_0->SetShown(false);
    }

    // Load animation/position data from panel file sub-structure

}



void main_menu_start_prelib_patch()   
{

{


  FUNC_ADDRESS(address, &main_menu_start_prelib::Update);
  set_vfunc(0x00894668, address);

}

}

void main_menu_start_build_patch()   
{

{


  FUNC_ADDRESS(address, &main_menu_start_prelib::Update_build);
  set_vfunc(0x00894668, address);

}

}