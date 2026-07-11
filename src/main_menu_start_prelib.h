#pragma once

#include "femenu.h"

#include "float.hpp"

#include "PanelQuad.h"

#include "panelfile.h"

#include "panelanimfile.h"

#include "frontendmenusystem.h"



struct main_menu_start_prelib : FEMenu {
    // PanelQuads - offset 0x2C to 0xC4
    PanelQuad *pq_logo_main;              // 0x2C (this+11)
    PanelQuad *pq_bkg_city;               // 0x30 (this+12)
    PanelQuad *pq_bkg_city01;             // 0x34 (this+13)
    PanelQuad *pq_bkg_city02;             // 0x38 (this+14)
    PanelQuad *pq_bkg_detail02;           // 0x3C (this+15)
    PanelQuad *pq_bkg_grey_a02;           // 0x40 (this+16)
    PanelQuad *pq_bkg_grey_a04;           // 0x44 (this+17)
    PanelQuad *pq_bkg_grey_a07;           // 0x48 (this+18)
    PanelQuad *pq_bkg_detail01;           // 0x4C (this+19)
    PanelQuad *pq_bkg_grey_a01;           // 0x50 (this+20)
    PanelQuad *pq_bkg_grey_a03;           // 0x54 (this+21)
    PanelQuad *pq_bkg_grey_a06;           // 0x58 (this+22)
    PanelQuad *pq_bkg_grey_a05;           // 0x5C (this+23)
    PanelQuad *pq_bkg_grey_a08;           // 0x60 (this+24)
    PanelQuad *pq_bkg_grey_a09;           // 0x64 (this+25)
    PanelQuad *pq_pre_main_back_b;        // 0x68 (this+26)
    PanelQuad *pq_pre_main_screen;        // 0x6C (this+27)
    PanelQuad *pq_pre_main_screen_0;      // 0x70 (this+28)
    PanelQuad *pq_pre_main_screen_1;      // 0x74 (this+29)
    PanelQuad *pq_pre_main_screen_2;      // 0x78 (this+30)
    PanelQuad *pq_pre_main_spider;        // 0x7C (this+31)
    PanelQuad *pq_bkg_white01;            // 0x80 (this+32)
    PanelQuad *pq_bkg_detail_lig;         // 0x84 (this+33)
    PanelQuad *pq_bkg_detail_lig_0;       // 0x88 (this+34)
    PanelQuad *pq_bkg_detail_lig_1;       // 0x8C (this+35)
    PanelQuad *pq_bkg_detail_lig_2;       // 0x90 (this+36)
    PanelQuad *pq_bkg_detail_lig_3;       // 0x94 (this+37)
    PanelQuad *pq_bkg_detail_lig_4;       // 0x98 (this+38)
    PanelQuad *pq_bkg_detail_dar;         // 0x9C (this+39)
    PanelQuad *pq_bkg_detail_dar_0;       // 0xA0 (this+40)
    PanelQuad *pq_bkg_detail_dar_1;       // 0xA4 (this+41)
    PanelQuad *pq_bkg_detail_dar_2;       // 0xA8 (this+42)
    PanelQuad *pq_bkg_detail_dar_3;       // 0xAC (this+43)
    PanelQuad *pq_bkg_detail_dar_4;       // 0xB0 (this+44)
    PanelQuad *pq_mainmenu_box_h;         // 0xB4 (this+45)
    PanelQuad *pq_mainmenu_box_h_0;       // 0xB8 (this+46)
    PanelQuad *pq_loading_bar01;          // 0xBC (this+47)
    PanelQuad *pq_loading_bar02;          // 0xC0 (this+48)
    PanelQuad *pq_loading_bar_ga;         // 0xC4 (this+49)

    // Data from panel animations - offset 0xC8 to 0x114
    int field_C8;                         // 0xC8 (this+50)
    int field_CC;                         // 0xCC (this+51)
    int field_D0;                         // 0xD0 (this+52)
    int field_D4;                         // 0xD4 (this+53)
    int field_D8;                         // 0xD8 (this+54)
    int field_DC;                         // 0xDC (this+55)
    int field_E0;                         // 0xE0 (this+56)
    int field_E4;                         // 0xE4 (this+57)
    int field_E8;                         // 0xE8 (this+58)
    int field_EC;                         // 0xEC (this+59)
    int field_F0;                         // 0xF0 (this+60)
    int field_F4;                         // 0xF4 (this+61)
    int field_F8;                         // 0xF8 (this+62)
    int field_FC;                         // 0xFC (this+63)
    int field_100;                        // 0x100 (this+64)
    int field_104;                        // 0x104 (this+65)
    int field_108;                        // 0x108 (this+66)
    int field_10C;                        // 0x10C (this+67)
    int field_110;                        // 0x110 (this+68)
    int field_114;                        // 0x114 (this+69)

    // FEText pointers - offset 0x118 to 0x11C
    FEText *text_mainmenu;                // 0x118 (this+70)
    FEText *text_mainmenu_0;              // 0x11C (this+71)

    // Padding/unknown - offset 0x120 to 0x12A
    float field_120;
    float field_124;
    bool field_128;
    bool field_129;
    uint16_t field_12A;

    FrontEndMenuSystem *field_12C; 	// 0x12C (this+75)
	

    main_menu_start_prelib(FrontEndMenuSystem *a2, int a3, int a4);
	
void Update(Float a2);

void Update_build(Float a2);

void Init();
};


extern void main_menu_start_prelib_patch();

extern void main_menu_start_build_patch();
