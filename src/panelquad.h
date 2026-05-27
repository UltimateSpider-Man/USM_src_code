#pragma once

#include "panelanimobject.h"

#include "float.hpp"
#include "mstring.h"
#include "mvector.h"
#include "panel_layer.h"
#include "vector2d.h"
#include "color32.h"


#include <cstdint>

struct from_mash_in_place_constructor;
struct nglTexture;
struct PanelQuadSection;
struct PanelMeshSection;

struct PanelQuad : PanelAnimObject {
    vector2d field_14;
    mVector<PanelQuadSection> pqs;
    PanelMeshSection *pmesh;
    float field_34;
    float field_38;
    mString field_3C;

    //0x00637E80
    PanelQuad();

    //0x00637F00
    PanelQuad(from_mash_in_place_constructor *a2);
	
	void SetPos(float a2, float a3);


    //0x0043F7F0
    ~PanelQuad();

    //0x00616990
    vector2d GetMax();

    //0x006168C0
    vector2d GetMin();
	


    void sub_616710(Float a2, Float a3);

    //0x00616290
    void SetTexture(nglTexture *a2);

    //0x00633A30
    //virtual
    void _destruct_mashed_class();

    /* virtual */ void _unmash(mash_info_struct *a1, void *a3);

    /* virtual */ int _get_mash_sizeof();

    /* virtual */ void Draw();

    /* virtual */ void TurnOn(bool a2);

    //virtual
    void Rotate(Float a2, Float a3, Float a4, bool a5);

    /* virtual */ void SetColor(color32 a2);

    //0x006284D0
    //virtual 
    void SetPos(float *a2, float *a3);

    /* virtual */ void SetPos(Float a2, Float a3, Float a4, Float a5);

    void Init(vector2d *a2, color32 *a3, panel_layer a4, Float a5, const char *a6);

    //0x0043F770
    //virtual
    float GetCenterX();

    /* virtual */
    float GetCenterY();

    //0x006166A0
    //virtual
    void GetPos(float *a2, float *a3);

    //0x0062E130
    //virtual
    void CopyFrom(const PanelQuad *a2);
	

};

extern void PanelQuad_patch();
