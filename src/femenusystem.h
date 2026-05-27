#pragma once

#include "font_index.h"
#include "fe_menu_nav_bar.h"
#include <cstdint>

struct FEMenu;


struct FEMenuSystem {
    std::intptr_t m_vtbl;

    FEMenu **field_4;
    font_index field_8;
    int m_index;

    int field_10;
    int m_count;
    uint16_t field_18[8];

    char field_28;
    char field_29;
    short field_2A;

    //0x00618760
    FEMenuSystem(int a2, font_index a3);

    //0x00629960
    //virtual
    void UpdateButtonDown();

    //0x0060B610
    //virtual
    void MakeActive(int idx);
	
	
	void MakeDeactive(int idx);

    //virtual
    char GetDefaultColorScheme();

    //virtual
    bool GetSingleInput();

    //0x006187D0
    //virtual
    void OnButtonPress(int a2, int a3);

    //0x0060B6B0
    //virtual
    void OnButtonRelease(int a2, int a3);

    //0x006298D0
    //virtual
    void UpdateButtonPresses();
};

struct FEMenuSystem2 {
    std::intptr_t m_vtbl;

    FEMenu **field_4;
    font_index field_8;
    int m_index;

    int field_10;
    int m_count;
    uint16_t field_18[9];

    char field_28;
    char field_29;
    menu_nav_bar *field_30;

};

extern void FEMenuSystem_patch();
