#pragma once

#include "entity.h"
#include "entity_base_vhandle.h"
#include "float.hpp"
#include "vector3d.h"

#include "panelfile.h"

struct zoom_map_ui {
    int field_0[144];
	
		char Draw();


void Update(Float a2);



PanelFile Init();


void OnSquare();


int OnX();


int sub_612820();

void __thiscall sub_621A80(Float a7);

int  sub_6222A0();



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
