#pragma once

#include "entity.h"
#include "entity_base_vhandle.h"
#include "float.hpp"
#include "vector3d.h"

#include "panelfile.h"

struct PanelQuad;

struct zoom_map_ui {
    // Each legend slot holds two quads: ".freeroam" is shown while no mission
    // is active, ".mission" is the variant swapped in for a specific mission
    // type. The no-mission loops walk .freeroam across all 7 slots; the
    // per-mission switch touches .mission of one slot (index 0 = default).
    struct legend_slot {
        PanelQuad *freeroam;  // shown when not in a mission
        PanelQuad *mission;   // shown for the matching mission type
    };

    /* 0x000 */ char field_0[0x20];
    /* 0x020 */ PanelQuad *m_icons[7];       // always-on legend icons
    /* 0x03C */ char field_3C[0x3C];
    /* 0x078 */ PanelQuad *m_labels[6];      // always-on legend labels
    /* 0x090 */ legend_slot m_icon_slots[7]; // 0x090..0x0C7
    /* 0x0C8 */ char field_C8[0x20];
    /* 0x0E8 */ PanelQuad *m_hidden_icon;    // hidden each frame
    /* 0x0EC */ PanelQuad *m_hidden_labels[6];
    /* 0x104 */ legend_slot m_label_slots[7];// 0x104..0x13B
    /* 0x13C */ char field_13C[0xDC];
    /* 0x218 */ int m_render_ctx;            // arg passed to every SetShown
    /* 0x21C */ char field_21C[0x24];
    /* 0x240 */

    //0x003D2194 (PS2) — ported below
    void UpdateSpideyLegend();
	
	void UpdateSpideyLegend_build();

    void UpdateSpideyLegend_beta();


		char Draw();


void Update(Float a2);


void SetUpNormalNavBar();




void Init();

void Init_beta();

void  Init_build();


void OnSquare();


int OnX();


int sub_612820();

void __thiscall sub_621A80(Float a7);

int  sub_6222A0();

// 0x00621410 — set icon graphics + label pointers, non-mode==2 path.
int sub_621410();

// 0x00621860 — set icon graphics + label pointers, mode==2 path.
int sub_621860();



};

struct IGOZoomPOI {
    vhandle_type<entity> field_0;
    vector3d field_4;
    int *field_10;

    void UpdateInScene();
};

struct IGOZoomOutMap {
    struct internal {
        IGOZoomPOI field_0;
        int field_14;
        int field_18;

        internal() {
            this->field_14 = 5;
            this->field_18 = 3;
            this->field_0.field_10 = nullptr;
        }
    };

    internal field_0[50];
    vector3d field_578;

    int field_4[11];

    int field_5B0;
    int field_5B4;
    int field_5B8;
    char field_5BC;
    char field_5BD;
    char field_5BE;
    char field_5BF;
    char field_5C0;
    char field_5C1;
    char field_5C2;
    bool field_5C3;
    bool field_5C4;
    char field_5C5;
    char field_5C6;
    char field_5C7;
    float field_5C8;
    zoom_map_ui field_5CC;
    int field_80C;
    int field_810;
    int field_814;
    char field_818;
    int field_81C;
    int field_820;
    int field_824;
    int field_828;

    //0x006489A0
    IGOZoomOutMap();
	
	void OnSelectPress();

    void UpdateInScene();

    //0x0060C2D0
    void DoneZoomingBack();

    //0x0063A760
    void Update(Float a2);

    bool sub_55F320();

    void sub_638AD0(int a2, int a3, int a4);

    //0x00619550
    void SetZoomLevel(int a2);
};

extern void IGOZoomOutMap_patch();

extern void IGOZoomOutMap_beta_patch();

extern void IGOZoomOutMap_build_patch();