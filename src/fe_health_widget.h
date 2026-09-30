#pragma once


#include "config.h"

struct PanelQuad;
struct PanelFile;

struct fe_health_widget {
    PanelFile *field_0[6];
    int field_18[6];
    int field_30;
    int field_34;
    int field_38;
    int field_3C;
    PanelQuad *field_40;
    PanelQuad *field_44;
    PanelQuad *field_48;
    float field_4C;
    float field_50;
    bool field_54;
    bool field_55;
    bool field_56;

    fe_health_widget();

    //0x0061A3F0
    void SetShown(bool a2);

    //0x0061A5A0
    void UpdateMasking();

    //0x0063B170
    char clear_bars();

    void Init(int a2, const char *a3, bool a4);

    void DeInit(int a2);

    // Carnage owns a panel, but shares Venom's native six-slot HUD layout.
    bool SelectCarnagePanel(bool selected);


    void SetType(int the_type, int a3)
    {
        void(__fastcall * func)(void*, void*, int, int) = bit_cast<decltype(func)>(0x00641BC0);

        func(this, nullptr, the_type, a3);
    }
};

// Drop cached pointers before the owning pack's panel data is released.
void carnage_hud_resource_changed(fe_health_widget *widget, bool carnage, bool unloading);
