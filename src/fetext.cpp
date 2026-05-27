#include "fetext.h"

#include "common.h"
#include "fetextflashinfo.h"
#include "func_wrapper.h"
#include "mash_info_struct.h"
#include "game.h"
#include "localized_string_table.h"
#include "ngl.h"
#include "trace.h"
#include "utility.h"
#include "variables.h"
#include "vtbl.h"

VALIDATE_SIZE(FEText, 0x68);

FEText::FEText()
{
    THISCALL(0x00617360, this);
}

FEText::FEText(font_index a2,
               global_text_enum a3,
               Float a4,
               Float a5,
               int a6,
               panel_layer a7,
               Float a8,
               int a9,
               int a10,
               color32 a11) {
    THISCALL(0x00617500, this, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11);
}


void FEText::_unmash(mash_info_struct *a1, void *a3)
{
    TRACE("FEText::unmash");
    if constexpr (1)
    {
        a1->unmash_class_in_place(this->field_1C, this);
        a1->unmash_class_in_place(this->field_50, this);
    }
    else
    {
        void (__fastcall *func)(void *, void *, mash_info_struct *, void *) = CAST(func, get_vfunc(m_vtbl, 0x4));
        func(this, nullptr, a1, a3);
    }
}

int FEText::_get_mash_sizeof()
{
#ifdef TARGET_XBOX
    return 0x60;
#else
    return 0x68;
#endif
}

void FEText::Draw() {
    //sp_log("Draw:");

    if constexpr (0)
    {
        if (this->IsShown() && !(this->field_1C == mString{""})) {
            auto color = this->field_4C;
            if ((this->field_64 & 8) != 0) {
                color = this->flash_info->GetColor(this->field_4C);
            }

            auto alpha = color.get_alpha();
            uint8_t v2 = (uint64_t) ((double) alpha * this->field_4);

            color.set_alpha(v2);
            if ((this->field_64 & 1) == 0) {
                color.set_alpha(255u);
                color.set_blue(255u);
                color.set_green(255u);
                color.set_red(255u);
            }

            auto a4 = this->field_34[1];
            auto a3 = this->field_34[0];

            {
                void (__fastcall *field_108)(FEText *, void *, void *, void *) = CAST(field_108, get_vfunc(this->m_vtbl, 0x108));
                field_108(this, nullptr, &a3, &a4);
            }

            nglFont *font = g_femanager.GetFont(this->field_18);

            auto v10 = this->field_40;
            auto v9 = this->field_3C;

            auto a5 = this->GetZvalue();

            auto v14 = color32::to_int(color);
            auto *str = this->field_1C.c_str();
            nglListAddString(font, str, a3, a4, a5, v14, v9, v10);
        }
    } else {
        THISCALL(0x00617640, this);
    }
}

mString FEText::GetName() {
    return this->field_50;
}

void FEText::Update(Float a2) {
    sp_log("0x%08X", m_vtbl);

    void (__fastcall *func)(FEText *, void *, Float) = CAST(func, get_vfunc(m_vtbl, 0x18));
    func(this, nullptr, a2);
}

void FEText::SetText(global_text_enum a2)
{
    if constexpr (1)
    {
        void (__fastcall *func)(FEText *, void *, global_text_enum) = CAST(func, get_vfunc(m_vtbl, 0x88));
        func(this, nullptr, a2);
    }
    else
    {
        auto *table = g_game_ptr->field_7C;
        mString v3 {table->lookup_localized_string(a2)};
        this->SetTextNoLocalize(*bit_cast<FEText::string *>(&v3));
    }
}

void FEText::SetPos(Float a2, Float a3) {
    void (__fastcall *func)(FEText *, void *, Float, Float) = CAST(func, get_vfunc(m_vtbl, 0x90));
    func(this, nullptr, a2, a3);
}

void FEText::SetNoFlash(color32 a2) {
    this->field_4C = a2;
    this->field_64 = this->field_64 & 0xF7 | 1;
}

void FEText::SetScale(Float a2, Float a3)
{
    if constexpr (1)
    {
        float (__fastcall *func)(FEText *, void *, Float, Float) = CAST(func, get_vfunc(m_vtbl, 0x78));
        func(this, nullptr, a2, a3);
    }
    else
    {
        this->field_3C = a2;
        this->field_40 = a3;
    }
}

void FEText::SetScale(Float a2) {
    this->field_3C = a2;
    this->field_40 = a2;
}

void FEText::SetTextNoLocalize(string a1) {
    TRACE("FEText::SetTextNoLocalize");

    if constexpr (0)
    {
        this->field_C = *(decltype(field_C) *)&a1;
    }
    else
    {
        THISCALL(0x0043C410, this, a1);
    }
}

void FEText::SetX(Float a2) {
    this->field_34[0] = a2;
}

void FEText::SetY(Float a2) {
    this->field_34[1] = a2 - flt_965BDC();
}

bool FEText::GetFlag(int a2) {
    return (this->field_64 & a2) != 0;
}

void FEText::AdjustForJustification(float *a2, float *a3)
{
    if constexpr (1) {
        auto *str = this->field_1C.c_str();
        nglFont *font = g_femanager.GetFont(this->field_18);

        uint32_t v13, v14;
        nglGetStringDimensions(font, (char *) str, &v14, &v13, this->field_3C, this->field_40);
        float v8 = v13;
        float v7 = v14;

        //sp_log("%f %f", v8, v7);

        if (this->GetFlag(32)) {
            *a2 = *a2 - v7;
        } else if (!this->GetFlag(16)) {
            *a2 = *a2 - (v7 * 0.5);
        }

        if (this->GetFlag(128)) {
            *a3 = *a3 - v8;
        } else if (!this->GetFlag(64)) {
            *a3 = *a3 - (v8 * 0.5);
        }
    } else {
        THISCALL(0x00617860, this, a2, a3);
    }
}

void FEText::SetFlash(color32 a2, color32 a3, Float a4) {
    this->field_4C = a2;
    auto *v5 = this->flash_info;
    this->field_64 |= 10u;
    if (v5 != nullptr) {
        v5->field_0 = a3;
        this->flash_info->field_C = a4;
    } else {
        this->flash_info = new FETextFlashInfo{a3, a4};
    }
}

float FEText::GetX() {
    float (__fastcall *func)(void *) = CAST(func, get_vfunc(m_vtbl, 0xD4));

    return func(this);
}

float FEText::GetY() {
    float (__fastcall *func)(void *) = CAST(func, get_vfunc(m_vtbl, 0xD8));

    return func(this);
}

void FEText_patch() {
    {
        FUNC_ADDRESS(address, &FEText::_unmash);
        SET_JUMP(0x0062E540, address);
    }

    {
        FUNC_ADDRESS(address, &FEText::_get_mash_sizeof);
        set_vfunc(0x0087A02C, address);
    }

    {
        FUNC_ADDRESS(address, &FEText::SetTextNoLocalize);
        set_vfunc(0x0087A06C, address);
        set_vfunc(0x0087A17C, address);
    }
	
	    {
        FUNC_ADDRESS(address, &FEText::SetFlash);
        set_vfunc(0x0087A088, address);
        set_vfunc(0x0087A198, address);
		set_vfunc(0x0087AF00, address);
    }


    return;

    if constexpr (0) {
        {
            FUNC_ADDRESS(address, &FEText::Update);
            SET_JUMP(0x006098D0, address);
        }

        {
            FUNC_ADDRESS(address, &FEText::Draw);
            set_vfunc(0x0087A034, address);

            void (*p_func)(nglFont *, const char *,
                    Float, Float, Float, uint32_t, Float, Float) = &nglListAddString;
            REDIRECT(0x00617745, p_func);
        }

        {
            FUNC_ADDRESS(address, &FEText::AdjustForJustification);
            set_vfunc(0x0087A0E8, address);
        }
    }
}
