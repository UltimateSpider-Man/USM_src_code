#include "pause_menu_status.h"

#include "common.h"
#include "func_wrapper.h"
#include "memory.h"
#include "utility.h"
#include "trace.h"
#include "panelquad.h"
#include "pausemenusystem.h"
#include "pause_menu_goals.h"
#include "pause_menu_awards.h"
#include "pause_menu_game.h"
#include "panelfile.h"
#include "cursor.h"
#include "pause_menu_transition.h"
#include "vtbl.h"
#include "fileusm.h"

#include "sound_instance_id.h"


VALIDATE_SIZE(pause_menu_status, 0x130);

pause_menu_status::pause_menu_status(FEMenuSystem *a2, int a3, int a4)
    : FEMenu(a2, 0, a3, a4, 0, 0) {
    THISCALL(0x0060FF30, this, a2, a3, a4);
}

void pause_menu_status::OnTriangle(int a2)
{
    TRACE("pause_menu_status::OnTriangle");

    if constexpr (1) {

        this->field_108 = this->field_FC;
        this->update_selected();

        auto *transition = static_cast<pause_menu_transition *>(
            this->field_E8->field_4[1]);
        transition->set_transition(12);


        this->field_E8->MakeActive(1);

        if (this->field_E8->field_38 == 0) {
            *bit_cast<bool *>(0x0096F7D4 + 103) = false;
        }

        static string_hash sfx_id_hash{"fe_aw_back"};
        [[maybe_unused]] sound_instance_id id =
            sub_60B960(sfx_id_hash, 1.0f, 1.0f);
    }
    else {
        THISCALL(0x0061CCE0, this, a2);  
    }
}

void pause_menu_status::_Load() {
    TRACE("pause_menu_status::Load");

    if constexpr (1)
    {
        auto *v2 = this->field_E8;
        this->field_12C = 0;
        this->field_12D = 0;
        auto *v3 = v2->field_2C;
        this->field_2C[0] = v3->GetPQ("pm_all_back_01");
        this->field_2C[1] = v3->GetPQ("pm_all_back_02");
        this->field_2C[2] = v3->GetPQ("pm_all_back_02a");
        this->field_2C[3] = v3->GetPQ("pm_all_back_02b");
        this->field_2C[4] = v3->GetPQ("pm_all_back_03");
        this->field_2C[5] = v3->GetPQ("pm_all_back_04");
        this->field_2C[6] = v3->GetPQ("pm_all_back_05");
        this->field_2C[7] = v3->GetPQ("pm_all_detail_02");
        this->field_2C[8] = v3->GetPQ("pm_all_detail_03");
        this->field_2C[9] = v3->GetPQ("pm_all_detail_04");
        this->field_2C[10] = v3->GetPQ("pm_all_detail_05");
        this->field_2C[11] = v3->GetPQ("pm_all_detail_06");
        this->field_2C[12] = v3->GetPQ("pm_all_detail_07");
        this->field_2C[13] = v3->GetPQ("pm_all_detail_08");
        this->field_2C[14] = v3->GetPQ("pm_all_detail_09");
        this->field_2C[15] = v3->GetPQ("pm_all_detail_10");
        this->field_2C[16] = v3->GetPQ("pm_all_detail_11");
        this->field_2C[17] = v3->GetPQ("pm_all_box_01");
        this->field_2C[18] = v3->GetPQ("pm_all_box_02");
        this->field_2C[19] = v3->GetPQ("pm_all_box_03");
        this->field_2C[20] = v3->GetPQ("pm_all_icon");
        this->field_2C[21] = v3->GetPQ("pm_status_explanation_box_01");
        this->field_2C[22] = v3->GetPQ("pm_status_explanation_box_02");
        this->field_2C[23] = v3->GetPQ("pm_status_hilite_text");
        this->field_2C[24] = v3->GetPQ("pm_status_hilite_text_01");
        this->field_90[0] = v3->GetPQ("pm_scroll_arrow_down");
        this->field_90[1] = v3->GetPQ("pm_scroll_arrow_up");
        this->field_90[2] = v3->GetPQ("pm_scroll_bar_01");
        this->field_90[3] = v3->GetPQ("pm_scroll_bar_02");
        this->field_90[4] = v3->GetPQ("pm_scroll_spider_icon");
        this->field_A4[0] = v3->GetTextPointer("pm_header_text_AWARDS");
        this->field_A4[1] = v3->GetTextPointer("pm_status_explanation_box_text_BLANK");
        this->field_A4[2] = v3->GetTextPointer("pm_status_text_left_01_BLANK");
        this->field_A4[3] = v3->GetTextPointer("pm_status_text_left_02_BLANK");
        this->field_A4[4] = v3->GetTextPointer("pm_status_text_left_03_BLANK");
        this->field_A4[5] = v3->GetTextPointer("pm_status_text_left_04_BLANK");
        this->field_A4[6] = v3->GetTextPointer("pm_status_text_left_05_BLANK");
        this->field_A4[7] = v3->GetTextPointer("pm_status_text_left_06_BLANK");
        this->field_A4[8] = v3->GetTextPointer("pm_status_text_right_01_BLANK");
        this->field_A4[9] = v3->GetTextPointer("pm_status_text_right_02_BLANK");
        this->field_A4[10] = v3->GetTextPointer("pm_status_text_right_03_BLANK");
        this->field_A4[11] = v3->GetTextPointer("pm_status_text_right_04_BLANK");
        this->field_A4[12] = v3->GetTextPointer("pm_status_text_right_05_BLANK");
        this->field_A4[13] = v3->GetTextPointer("pm_status_text_right_06_BLANK");

        auto *v5 = new PanelQuad {};
        auto *v6 = this->field_2C[21];
        this->field_DC = v5;
        v5->CopyFrom(v6);

        float v34[4] {};
        float v30[4] {};
        this->field_DC->GetPos(v34, v30);

        v30[0] = v30[0] - 50.f;
        v30[1] = v30[1] - 70.f;
        v30[2] = v30[2] - 50.f;
        v30[3] = v30[3] - 70.f;

        this->field_DC->SetPos(v34, v30);

        auto *v10 = new PanelQuad {};
        auto *v11 = this->field_2C[22];
        this->field_E0 = v10;
        v10->CopyFrom(v11);
        this->field_E0->GetPos(v34, v30);

        v30[0] = v30[0] - 50.f;
        v30[1] = v30[1] - 70.f;
        v30[2] = v30[2] - 50.f;
        v30[3] = v30[3] - 70.f;

        this->field_E0->SetPos(v34, v30);
        int v27 = this->field_A4[1]->GetX();
        int v26 = this->field_A4[1]->GetY();

        float a5 = v26 - 45.f;
        float a4 = v27;
        auto *v15 = new FEText {
                static_cast<font_index>(1),
                static_cast<global_text_enum>(292),
                a4,
                a5,
                5,
                static_cast<panel_layer>(1),
                1.0,
                16,
                0,
                color32 {0xFFC8C8C8}};

        auto *v16 = this->field_90[4];
        this->field_E4 = v15;
        this->field_120 = v16->GetCenterY();
        auto *v18 = v3->GetPQ("pm_scroll_spider_icon_end_marker");
        this->field_124 = v18->GetCenterY();
        auto *v19 = this->field_A4[2];
        auto v28 = this->field_2C[23]->GetCenterY();
        auto v20 = v19->GetY();
        auto *v21 = this->field_2C[24];
        auto *v22 = this->field_A4[2];
        this->field_118 = v28 - v20;
        auto v29 = v21->GetCenterY();
        auto v23 = v22->GetY();
        this->field_128 = 0;
        this->field_11C = v29 - v23;
        this->update_selected();
    }
    else
    {
        THISCALL(0x0063B890, this);
    }
}

mString *pause_menu_status::get_element_desc(mString *out, int a3)
{
    TRACE("pause_menu_status::get_element_desc");

    if constexpr (1)
    {
        const auto mode = this->field_EC;

        switch (mode)
        {
        case 0:
            // Default: descriptions array at field_F0 + 0x140
            new (out) mString(this->field_F0->field_140[a3]);
            break;

        case 1:
            // Alternate: descriptions array at field_F4 + 0x160
            new (out) mString(this->field_F4->field_160[a3]);
            break;

        case 2:
            // Goals submenu owns the string; it constructs into `out` itself.
            // Matches original: pause_menu_goals::get_element_desc(field_F8, out, a3)
            this->field_F8->get_element_desc(out, a3);
            break;

        default:
            // Fallback (mode >= 3): single string stored at field_F0->field_0
            new (out) mString(this->field_F0->field_0);
            break;
        }
    }
    else
    {
        THISCALL(0x00610290, this, out, a3);
    }

    return out;
}

const mString *pause_menu_status::get_element_value(int index)
{
    TRACE("pause_menu_status::get_element_value");

    if constexpr (1)
    {
        switch (this->field_EC)
        {
        case 0:
            // awards: array of mStrings starting at +0x30
            return &this->field_F0->field_30[index];
        case 1:
            // game: array of mStrings starting at +0x10
			return &this->field_F4->field_10 + index;

        case 2:
            // goals: array of mStrings starting at +0x10
            return &this->field_F8->field_10[index];
        default:
            // mode >= 3: single mString stored at +0 of awards struct
            return &this->field_F0->field_0;
        }
    }
    else
    {
        return reinterpret_cast<const mString*>(
            THISCALL(0x006101B0, this, index));
    }
}


mString *pause_menu_status::get_element_value_0(mString *out, int index)
{
    TRACE("pause_menu_status::get_element_value_0");

    if constexpr (1)
    {
        switch (this->field_EC)
        {
        case 0:
            // awards owns the value string for this row
            this->field_F0->get_element_value(out, index);
            break;
        case 1:
            // game owns the value string for this row
            this->field_F4->get_element_value(out, index);
            break;
        case 2:
            // goals owns it — this is the function we rewrote earlier
            this->field_F8->get_element_value(out, index);
            break;
        default:
            // mode >= 3: copy-construct from field_F0->field_0
            new (out) mString(this->field_F0->field_0);
            break;
        }
    }
    else
    {
        THISCALL(0x00610210, this, out, index);
    }
    return out;
}

void pause_menu_status::update_selected()
{
    TRACE("pause_menu_status::update_selected");

    int max_data_row;
    switch (this->field_EC) {
        case 0:  max_data_row = 16; break;
        case 1:  max_data_row = 20; break;
        case 2:  max_data_row = 3;  break;
        default: max_data_row = 0;  break;
    }

    auto clamp_int = [](int v, int lo, int hi) -> int {
        return v < lo ? lo : (v > hi ? hi : v);
    };


    this->field_128 = clamp_int(this->field_128, 0, max_data_row);
    this->field_110 = clamp_int(this->field_110, 0, max_data_row);
    this->field_114 = clamp_int(this->field_114, 0, max_data_row);

    constexpr int kLeftColMin  = 2;
    constexpr int kLeftColMax  = 7;   // <-- restored from 21
    constexpr int kRightColOff = 6;


    if (this->field_104 != kRightColOff) {
        this->field_104 = kRightColOff;
    }

    this->field_FC  = clamp_int(this->field_FC,  kLeftColMin,    kLeftColMax);
    this->field_100 = clamp_int(this->field_100, this->field_FC, kLeftColMax);
    this->field_108 = clamp_int(this->field_108, this->field_FC, this->field_100);
    this->field_10C = clamp_int(this->field_10C, this->field_FC, this->field_100);


    if (this->field_F0 == nullptr ||
        this->field_F4 == nullptr ||
        this->field_F8 == nullptr)
    {
        return;
    }

    THISCALL(0x0061CC30, this);
}

void pause_menu_status::Init()
{
     TRACE("pause_menu_status::Init");

    if constexpr (1) {

        this->field_12C = false;
        this->field_12D = false;


        for (int i = 0; i < 14; ++i) {
            FEText *t = this->field_A4[i];
            if (t == nullptr) continue;
            t->SetScale(1.0f, 1.0f);
            t->SetNoFlash(color32{0xFFC87538});   // orange-brown idle
        }


        this->update_selected();


        if (this->field_A4[0] != nullptr) {
            this->field_A4[0]->SetNoFlash(color32{0xFFC8C848});
        }
        if (this->field_A4[1] != nullptr) {
            this->field_A4[1]->SetNoFlash(color32{0xFFC8C848});
        }

        if (this->field_E8 != nullptr) {
            menu_nav_bar *nav_bar = this->field_E8->field_30;
            if (nav_bar != nullptr) {
                nav_bar->field_4  = "Source";
                nav_bar->field_28 = 0;
                nav_bar->AddButtons(menu_nav_bar::button_type{15},
                                    menu_nav_bar::button_type{17},
                                    static_cast<global_text_enum>(3));

                if (nav_bar->text_box != nullptr) {

                    mString text{nav_bar->field_4.guts};
                    nav_bar->text_box->SetTextNoLocalize(text);

                    text = mString{get_msg(g_fileUSM(), "Resume")};
                    nav_bar->text_box->SetTextNoLocalize(text);
                }
            }
        }


        this->field_28 |= 0x80;
        this->field_128 = 0;

        Cursor *cur = g_cursor();
        if (cur != nullptr) {
            cur->sub_5A6790();                          
            cur->sub_5A67D0(297, 420, 375, 445);        
            cur->field_120 = 1;                         
            cur->field_114 = 0;                         
        }
    }
    else {
        THISCALL(0x0062AB20, this);
    }
}


void pause_menu_status::OnDown(int a2)
{
    TRACE("pause_menu_status::OnDown");

    if constexpr (1) {

        int count;
        switch (this->field_EC) {
            case 0:  count = 17;
			break;   
            case 1:  count = 21;
			break;   
            case 2:  count = 4;  
			break;   
            default: count = 0;  
			break;
        }
        if (count <= 0) {
            this->update_selected();
            return;
        }


        const int top         = this->field_FC;
        const int bottom      = this->field_100;
        const int window_size = bottom - top + 1;
        const int max_scroll  = (count > window_size) ? (count - window_size) : 0;


        int next_row = this->field_110 + 1;

        if (next_row >= count) {

            next_row         = 0;
            this->field_128  = 0;
            this->field_108  = top;
        }
        else if (next_row >= this->field_128 + window_size) {

            this->field_128  = next_row - (window_size - 1);
            if (this->field_128 > max_scroll) {
                this->field_128 = max_scroll;
            }
            this->field_108  = bottom;
        }
        else {

            this->field_108  = top + (next_row - this->field_128);
        }

        this->field_110 = next_row;

        if (this->field_110 != this->field_114) {
            static string_hash sfx_id_hash{"fe_gs_udscroll"};
            [[maybe_unused]] sound_instance_id id =
                sub_60B960(sfx_id_hash, 1.0f, 1.0f);
        }

        this->update_selected();
    }
    else {
        THISCALL(0x0061D250, this, a2);
    }
}

void pause_menu_status::OnUp(int a2)
{
    TRACE("pause_menu_status::OnUp");

    if constexpr (1) {
        int count;
        switch (this->field_EC) {
            case 0:  count = 17; break;
            case 1:  count = 21; break;
            case 2:  count = 4;  break;
            default: count = 0;  break;
        }
        if (count <= 0) {
            this->update_selected();
            return;
        }

        const int top         = this->field_FC;
        const int bottom      = this->field_100;
        const int window_size = bottom - top + 1;
        const int max_scroll  = (count > window_size) ? (count - window_size) : 0;


        int prev_row = this->field_110 - 1;

        if (prev_row < 0) {

            prev_row         = count - 1;
            this->field_128  = max_scroll;
            this->field_108  = top + (prev_row - this->field_128);
        }
        else if (prev_row < this->field_128) {

            this->field_128  = prev_row;
            this->field_108  = top;
        }
        else {

            this->field_108  = top + (prev_row - this->field_128);
        }

        this->field_110 = prev_row;

        if (this->field_110 != this->field_114) {
            static string_hash sfx_id_hash{"fe_gs_udscroll"};
            [[maybe_unused]] sound_instance_id id =
                sub_60B960(sfx_id_hash, 1.0f, 1.0f);
        }

        this->update_selected();
    }
    else {
        THISCALL(0x0061D0F0, this, a2);
    }
}


void pause_menu_status::Down() {
    TRACE("pause_menu_status::Down");
    void (__fastcall *on_down)(pause_menu_status *, void *, int)
        = CAST(on_down, get_vfunc(this->m_vtbl, 0x40));
    on_down(this, nullptr, 0);
}

char pause_menu_status::OnMouseEvent(int msg, int a3, int a4)
{
    TRACE("pause_menu_status::OnMouseEvent");

    if constexpr (1) {

        int count;
        switch (this->field_EC) {
            case 0:  count = 17;
			break;   
            case 1:  count = 21;
			break; 
            case 2:  count = 4; 
			break;  
            default: count = 0;  
			break;
        }


        int hit_count = count + 2;
        if (hit_count > 14) hit_count = 14;

        Cursor *cur = g_cursor();
        char result = 0;

        if (msg == 512) {
            if (cur != nullptr) {
                cur->sub_581C60();
            }
            this->field_10C = this->field_108;


            int hit = static_cast<int>(THISCALL(
                0x00581CD0, cur,
                &this->field_A4[0], hit_count, 450, 2, 10));

            if (hit == -1) {
                // Mouse not over any row.
                bool in_valid_zone = static_cast<int>(
                    THISCALL(0x005A0330, cur)) != 0;

                if (in_valid_zone) {

                    if (this->field_12C) {
                        this->field_12C = false;
                        this->update_selected();
                    }
                } else {

                    this->field_12C = true;
                    this->field_12D = false;
                    this->update_selected();
                }
            } else {

                if (this->field_12C) {
                    this->field_12C = false;
                    this->update_selected();
                }
                const bool unchanged = (this->field_10C == hit);


                this->field_110 = this->field_128 + hit - this->field_FC;
                this->field_108 = hit;

                if (!unchanged) {
                    this->update_selected();
                }
            }
        }


        else if (msg == 514) {
            int hit = static_cast<int>(THISCALL(
                0x00581CD0, cur,
                &this->field_A4[0], hit_count, 450, 2, 10));

            if (hit == -1) {

                bool in_valid_zone = static_cast<int>(
                    THISCALL(0x005A0330, cur)) != 0;

                if (in_valid_zone) {

                    int needed_rows = static_cast<int>(
                        THISCALL(0x00610180, this));
                    int window_size =
                        this->field_100 - this->field_FC + 1;

                    if (needed_rows > window_size) {

                        int sw = static_cast<int>(THISCALL(
                            0x00581F00, cur,
                            this->field_90[4],
                            this->field_90[1],
                            this->field_90[0]));

                        if (sw != 0) {

                            int kind = sw - 1;
                            if (kind == 0) {
                                THISCALL(0x0061D880, this);  // Up()
                            } else if (kind == 2) {
                                this->Down();
                            }
                        }
                    }
                } else {

                    void (__fastcall *on_start)(
                        pause_menu_status *, void *, int)
                        = CAST(on_start, get_vfunc(this->m_vtbl, 0x38));
                    on_start(this, nullptr, 0);
                }
            } else {

                void (__fastcall *on_cross)(
                    pause_menu_status *, void *, int)
                    = CAST(on_cross, get_vfunc(this->m_vtbl, 0x4C));
                on_cross(this, nullptr, 0);
            }
        }


        else if (msg == 517) {
            void (__fastcall *on_triangle)(
                pause_menu_status *, void *, int)
                = CAST(on_triangle, get_vfunc(this->m_vtbl, 0x50));
            on_triangle(this, nullptr, 0);
        }


        else {
            result = static_cast<char>(msg - 5);
        }

        return result;
    } else {
        return static_cast<char>(
            THISCALL(0x0062AC90, this, msg, a3, a4));
    }
}

void pause_menu_status_patch()
{
    {
        FUNC_ADDRESS(address, &pause_menu_status::_Load);
        set_vfunc(0x008940B0, address);
    }

    {
        FUNC_ADDRESS(address, &pause_menu_status::get_element_desc);
     //   REDIRECT(0x0061CD64, address);
    }
	    {
        FUNC_ADDRESS(address, &pause_menu_status::update_selected);
        REDIRECT(0x0061D235, address);
    }
	
    {
        FUNC_ADDRESS(address, &pause_menu_status::get_element_value_0);
     //   REDIRECT(0x0061CCDA, address);
    }
	
    {
	FUNC_ADDRESS(address, &pause_menu_status::OnTriangle);
	set_vfunc(0x008940F0, address);
    }	
    {
	FUNC_ADDRESS(address, &pause_menu_status::Init);
//	set_vfunc(0x008940CC, address);	
    }	
	
    {
	FUNC_ADDRESS(address, &pause_menu_status::OnUp);
	set_vfunc(0x008940DC, address);	
    }
	
    {
	FUNC_ADDRESS(address, &pause_menu_status::OnDown);
	set_vfunc(0x008940E0, address);	
    }
    {
	FUNC_ADDRESS(address, &pause_menu_status::Down);
	//REDIRECT(0x0062AD74, address);	
    }
    {
	FUNC_ADDRESS(address, &pause_menu_status::OnMouseEvent);
	set_vfunc(0x00894140, address);	
    }		
	 
}
