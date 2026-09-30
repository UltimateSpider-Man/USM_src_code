#include "femultilinetext.h"

#include "common.h"
#include "fetextflashinfo.h"
#include "func_wrapper.h"
#include "game.h"
#include "gamepadinput.h"
#include "localized_string_table.h"
#include "mash_config.h"
#include "multilinestring.h"
#include "trace.h"
#include "utility.h"
#include "variables.h"


VALIDATE_OFFSET(FEMultiLineText, lines, 0x88);
VALIDATE_SIZE(FEMultiLineText, 0xA0u);

FEMultiLineText::FEMultiLineText()
{
    THISCALL(0x00617470, this);
}

FEMultiLineText::FEMultiLineText(font_index a2,
                                 Float a4,
                                 Float a5,
                                 int a6,
                                 panel_layer a7,
                                 Float a8,
                                 int a9,
                                 int a10,
                                 color32 a11) {
    THISCALL(0x00629250, this, a2, a4, a5, a6, a7, a8, a9, a10, a11);
}

void FEMultiLineText::Draw() {
    if constexpr (0) {
    } else {
        THISCALL(0x0060A070, this);
    }
}

void FEMultiLineText::_unmash(mash_info_struct *a1, void *a3)
{
    TRACE("FEMultiLineText::unmash");
    FEText::_unmash(a1, a3);
    this->lines = nullptr;
    this->SetNumLines(this->line_avail_num);
}

int FEMultiLineText::_get_mash_sizeof()
{
#if OPENUSM_XBOX_MASH_FORMAT && !defined(OPENUSM_XBPACK_V10)
    return 0x98;
#else
    return 0xA0;
#endif
}

void FEMultiLineText::Draw(int a2, int a3) {
    if (this->IsShown()) {
        if (a2 < 0) {
            a2 = 0;
        }

        if (a3 > this->field_80) {
            a3 = this->field_80;
        }

        auto v4 = this->field_64;
        auto v17 = this->field_4C;
        if ((v4 & 8) != 0) {
            v17 = this->flash_info->GetColor(this->field_4C);
        }

        v17.set_alpha(v17.get_alpha() * this->field_4);

        if ((v4 & 1) == 0) {
            v17.set_alpha(0xFFu);
            v17.set_blue(0xFFu);
            v17.set_green(0xFFu);
            v17.set_red(0xFFu);
        }

        auto v8 = color32::to_int(v17);

        auto v9 = color32::to_int(this->field_68);
        if (a2 < a3) {
            int i = a2;
            int a3a = a3 - a2;
            do {
                auto *v11 = &this->lines[i];
                if (v11->m_font_index != 6) {
                    if (v11->field_10 != "") {
                        auto v16 = this->field_78;
                        auto v15 = this->field_6C;
                        auto v14 = this->field_40;
                        auto v13 = this->field_3C;
                        auto z_value = this->GetZvalue();
                        v11->Draw(z_value, v8, v9, v13, v14, v15, v16);
                    }
                }
                ++i;
                --a3a;
            } while (a3a);
        }
    }
}

void FEMultiLineText::GetPos(Float &a2, Float &a3) {
    a2 = this->field_34[0];
    a3 = this->field_34[1];
}

void FEMultiLineText::SetButtonColor(color32 a2) {
    this->field_68 = a2;
}

mString FEMultiLineText::ReplaceEndlines(mString a2) {
    for (auto i = a2.find("\n", 0); i > 0; i = a2.find("\n", i + 2)) {
        a2.data()[i] = ' ';
        a2.data()[i + 1] = '\n';
    }

    return a2;
}

void FEMultiLineText::SetButtonScale(Float a2) {
    this->field_6C = a2;
}

void FEMultiLineText::SetTextBox(global_text_enum a2, int a3, Float a4) {
    //sp_log("FEMultiLineText::SetTextBox: %s", g_game_ptr->field_7C->field_0->field_0[a2]);

    THISCALL(0x00618070, this, a2, a3, a4);
}

char *sub_609580(const char *a1, const char *a2, const char *a3) {
    if constexpr (1) {
        auto *v3 = a1;
        auto v4 = strlen(a1);
        auto v12 = strlen(a2);
        auto *v5 = &a3[strlen(a3) + 1];
        uint32_t v11 = v5 - (a3 + 1);
        auto *result = static_cast<char *>(malloc(v4 * (v5 - a3) + 1));
        auto *v7 = result;
        auto *v13 = result;
        auto *v8 = result;
        if (result != nullptr) {
            result[0] = '\0';
            auto *v9 = strstr(a1, a2);
            if (v9 != nullptr) {
                do {
                    std::memcpy(v8, v3, v9 - v3);
                    auto *v10 = &v8[v9 - v3];

                    std::memcpy(v10, a3, v11);
                    v8 = &v10[v11];
                    v3 = &v9[v12];
                    v10[v11] = 0;
                    v9 = strstr(&v9[v12], a2);
                } while (v9);

                v7 = v13;
            }

            strcat(v8, v3);
            result = (char *) std::realloc(v7, strlen(v7) + 1);
        }

        sp_log("a1 = %s, a2 = %s, a3 = %s -> %s", a1, a2, a3, result);

        return result;

    } else {
        return (char *) CDECL_CALL(0x00609580, a1, a2, a3);
    }
}

void FEMultiLineText::sub_60A4A0(mString &a1) {
    if (strchr(a1.c_str() , '~') != nullptr) {
        std::string str{a1.c_str()};
        auto *v2 = str.c_str();
        //auto *v2 = static_cast<char *>(operator new(strlen(a1.c_str()) + 1));
        //strcpy(v2, a1.c_str());
        if (strstr(v2, "~cross")) {
            auto *v3 = sub_609580(v2, "~cross", dword_965C24()[GamepadInput::Cross]);
            v2 = v3;
        }

        if (strstr(v2, "~triangle")) {
            auto *v4 = sub_609580(v2, "~triangle", dword_965C24()[GamepadInput::Triangle]);
            v2 = v4;
        }

        if (strstr(v2, "~square")) {
            auto *v5 = sub_609580(v2, "~square", dword_965C24()[GamepadInput::Square]);
            v2 = v5;
        }

        if (strstr(v2, "~circle")) {
            auto *v6 = sub_609580(v2, "~circle", dword_965C24()[GamepadInput::Circle]);
            v2 = v6;
        }

        if (strstr(v2, "~updown")) {
            auto *v7 = sub_609580(v2, "~updown", "UP & DOWN");
            v2 = v7;
        }

        if (strstr(v2, "~l2")) {
            auto *v8 = sub_609580(v2, "~l2", dword_965C24()[GamepadInput::L2]);
            v2 = v8;
        }

        if (strstr(v2, "~r2")) {
            auto *v9 = sub_609580(v2, "~r2", dword_965C24()[GamepadInput::R2]);
            v2 = v9;
        }

        if (strstr(v2, "~r3")) {
            auto *v10 = sub_609580(v2, "~r3", dword_965C24()[GamepadInput::R3]);
            v2 = v10;
        }

        if (strstr(v2, "~both_lr")) {
            char Dest[256]{};
            sprintf(Dest,
                    "\"%s & %s\"",
                    dword_965C24()[GamepadInput::L2],
                    dword_965C24()[GamepadInput::R2]);
            auto *v11 = sub_609580(v2, "~both_lr", Dest);
            v2 = v11;
        }

        if (strstr(v2, "~right")) {
            auto *v12 = sub_609580(v2, "~r2", dword_965C24()[GamepadInput::Right]);
            v2 = v12;
        }

        if (strstr(v2, "~select")) {
            auto *v13 = sub_609580(v2, "~select", dword_965C24()[GamepadInput::Select]);
            v2 = v13;
        }

        if (strstr(v2, "~forward")) {
            auto *v14 = sub_609580(v2, "~forward", dword_965C24()[GamepadInput::Forward]);
            v2 = v14;
        }

        if (strstr(v2, "~left")) {
            auto *v15 = sub_609580(v2, "~left", dword_965C24()[GamepadInput::Left]);
            v2 = v15;
        }

        if (strstr(v2, "~start")) {
            auto *v16 = sub_609580(v2, "~start", dword_965C24()[GamepadInput::Start]);
            v2 = v16;
        }

        a1.update_guts(v2, -1);
    }
}


int FEMultiLineText::MakeBox(char *a2, int a3, int a4, Float a5, Float a6, bool a7)
{
    TRACE("FEMultiLineText::MakeBox");

    return THISCALL(0x0062EAD0, this, a2, a3, a4, a5, a6, a7);
}

bool FEMultiLineText::CheckIfNotTooLong(int a2)
{
    if ( a2 < this->line_avail_num )
    {
        return true;
    }

    if ( this->field_9E )
    {
        sp_log("MultiLineString is too long (cut off).  Number allocated lines: %d\n", this->line_avail_num);
        auto *v4 = this->lines->field_10.c_str();
        sp_log("Start of text: %s\n", v4);
    }
    else
    {
        sp_log("MultiLineString is too long (not cut off).  Number allocated lines: %d\n", this->line_avail_num);
        auto *v3 = this->lines->field_10.c_str();
        sp_log("Start of text: %s\n", v3);

        assert(0 && "MultiLineString is too long");
    }

    return false;
}

void FEMultiLineText::SetTextBoxNoLocalize(FEMultiLineText::string a2, int a7, Float a8) {
    TRACE("FEMultiLineText::SetTextBoxNoLocalize");

    assert(line_avail_num != 0);

    if constexpr (0)
    {
        this->sub_60A4A0(*bit_cast<mString *>(&a2));
        auto v12 = *bit_cast<mString *>(&a2);
        auto v5 = FEMultiLineText::ReplaceEndlines(v12);
        a2 = *bit_cast<string *>(&v5);
        auto v6 = a7;
        auto v7 = a8 == -1.f;
        auto v8 = this->field_3C;
        a7 = this->field_40;
        this->field_7C = v6;
        auto a5 = v8;
        if ( !v7 )
        {
            a5 = a8;
            a7 = a8;
        }
        
        auto v9 = this->MakeBox(a2.guts, a2.m_size, v6, a5, a7, true);
        if ( this->CheckIfNotTooLong(v9 - 1))
            this->field_80 = v9;
        else
            this->field_80 = this->line_avail_num;

        auto *v11 = this->lines->field_10.c_str();
        this->SetTextNoLocalize( mString {v11});
        this->AdjustForJustification();
    }
    else
    {
        THISCALL(0x00633AB0, this, a2, a7, a8);
    }
}

void FEMultiLineText::SetTextAlloc(global_text_enum a2) {
    sp_log("FEMultiLineText::SetTextAlloc: ");

    THISCALL(0x006180B0, this, a2);
}

void FEMultiLineText::SetText(global_text_enum a2) {
    sp_log("FEMultiLineText::SetText: %s", g_game_ptr->field_7C->lookup_localized_string(a2));

    THISCALL(0x00618030, this, a2);
}

void FEMultiLineText::AdjustForJustification()
{
    TRACE("FEMultiLineText::AdjustForJustification");
    THISCALL(0x006182D0, this);
}

void FEMultiLineText::_SetTextNoLocalize(FEMultiLineText::string a1) {
    TRACE("FEMultiLineText::SetTextNoLocalize");

    assert(line_avail_num != 0);

    THISCALL(0x0062E720, this, a1);
}

int FEMultiLineText::SetTextAllocNoLocalize(const char *a2, int a3) {
    sp_log("FEMultiLineText::SetTextAllocNoLocalize:");
    return THISCALL(0x0062E8D0, this, a2, a3);
}

void FEMultiLineText::SetTextBoxAlloc(global_text_enum a1, int a3, Float a4) {
    sp_log("FEMultiLineText::SetTextBoxAlloc:");

    THISCALL(0x00618140, this, a1, a3, a4);
}

void FEMultiLineText::SetTextBoxAllocNoLocalize(mString a2, int a6, Float a7) {
    sp_log("FEMultiLineText::SetTextBoxAllocNoLocalize:");

    THISCALL(0x00633C00, this, a2, a6, a7);
}

void FEMultiLineText::SetNumLines(int n) {
    TRACE("FEMultiLineText::SetNumLines", std::to_string(n).c_str());

    assert(n != 0);

    THISCALL(0x00617F30, this, n);
}

void FEMultiLineText_patch() {

    {
        FUNC_ADDRESS(address, &FEMultiLineText::_get_mash_sizeof);
        set_vfunc(0x0087AEA4, address);
    }

    {
        FUNC_ADDRESS(address, &FEMultiLineText::AdjustForJustification);
        REDIRECT(0x0062E878, address);
    }

    {
        FUNC_ADDRESS(address, &FEMultiLineText::_unmash);
        set_vfunc(0x0087AE5C, address);
    }

    {
        FUNC_ADDRESS(address, &FEMultiLineText::_SetTextNoLocalize);
        set_vfunc(0x0087AEE4, address);
    }


    {
        FUNC_ADDRESS(address, &FEMultiLineText::SetTextBoxNoLocalize);
        set_vfunc(0x0087AFA4, address);
    }

    return;

    SET_JUMP(0x00609580, sub_609580);

    if constexpr (0) {
        {
            FUNC_ADDRESS(address, &FEMultiLineText::SetText);
            //set_vfunc(0x0087AEE0, address);
        }

        auto addr = 0x0087AFA0;

        {
            FUNC_ADDRESS(address, &FEMultiLineText::SetTextBox);
            set_vfunc(addr, address);
        }

        addr += 0x4;

        addr += 0x4;

        {
            FUNC_ADDRESS(address, &FEMultiLineText::SetTextAlloc);
            set_vfunc(addr, address);
        }

        addr += 0x4;

        {
            FUNC_ADDRESS(address, &FEMultiLineText::SetTextAllocNoLocalize);
            set_vfunc(addr, address);
        }

        addr += 0x4;

        {
            FUNC_ADDRESS(address, &FEMultiLineText::SetTextBoxAlloc);
            set_vfunc(addr, address);
        }

        addr += 0x4;

        {
            FUNC_ADDRESS(address, &FEMultiLineText::SetTextBoxAllocNoLocalize);
            set_vfunc(addr, address);
        }

        {
            FUNC_ADDRESS(address, &MultiLineString::Draw);
            REDIRECT(0x00617E5F, address);
        }
    }
}
