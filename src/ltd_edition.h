#pragma once

#include "femenu.h"

#include "fe_menu_nav_bar.h"


#include "femenusystem.h"



 struct ltd_edition : FEMenu // sizeof=0x120
 {
     ltd_edition *m_vtbl;
     int field_4;
     int field_8;
     int field_C;
     int field_10;
     int field_14;
     int field_18;
     int field_1C;
     int field_20;
     int field_24;
     char field_28;
     char field_29;
     char field_2A;
     // padding byte
     FEMenuSystem *field_2C;
     int field_30;
     menu_nav_bar *field_34;
     int field_38;
     int field_3C;
     int field_40;
     int field_44;
     int field_48;
     int field_4C;
     int field_50;
     int field_54;
     int field_58;
     int field_5C;
     int field_60;
     int field_64;
     int field_68;
     int field_6C;
     int field_70;
     int field_74[1];
     int field_78;
     int field_7C[1];
     int field_80[1];
     int field_84;
     int field_88;
     int field_8C;
     int field_90;
     int field_94;
     int field_98;
     int field_9C;
     int field_A0;
     int field_A4;
     int field_A8;
     int field_AC;
     int field_B0;
     int field_B4;
     int field_B8;
     int field_BC;
     int field_C0;
     int field_C4;
     int field_C8;
     int field_CC;
     int field_D0;
     int field_D4;
     int field_D8;
     int field_DC;
     int field_E0;
     int field_E4;
     int field_E8;
     int field_EC;
     int field_F0;
     int field_F4;
     int field_F8;
     int field_FC;
     int field_100;
     int field_104;
     int field_108;
     int field_10C;
     int field_110;
     int field_114;
     int field_118;
     char field_11C;
    char empty[3];
	
	    //0x00614570
    ltd_edition(FEMenuSystem *a2, int a3, int a4);
	
	void Deactivate(int a2);

void scroll(int a1);

void scroll_left(int a2);

void scroll_right(int a2);

void on_back(int a2);



void on_resume_game(int a2);


void on_select(int a2);


 };
