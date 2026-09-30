#pragma once

#include "mstring.h"

struct FEText;
struct PanelAnimFile;
struct PanelFile;
struct PanelQuad;

struct fe_mission_text {
    void *m_vtbl;
    PanelFile *panel;
    PanelQuad *back_1;
    PanelQuad *back_2;
    PanelQuad *detail;
    PanelQuad *back_panel;
    char field_18[0x84];
    PanelQuad *gradient_text;
    PanelQuad *gradient_hints;
    PanelQuad *gradient_reward;
    FEText *text;
    PanelAnimFile *anim;
    int field_B0;
    bool shown;
    bool positioned;

    fe_mission_text();

    void draw_v10();

    //0x0060D560
    void set_flavor(int a2);

    struct string {
        int field_0[2];
        char *data;
        int field_C;
    };

    //0x0060D440
    void set_text(string a2);

    //0x0061AA00
    void SetShown(bool a2);
};

extern void fe_mission_text_patch();

#ifdef OPENUSM_XBPACK_V10
extern void fe_mission_text_v10_patch();
#endif
