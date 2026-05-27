#include "igozoomoutmap.h"

#include "common.h"
#include "func_wrapper.h"
#include "game.h"
#include "marky_camera.h"
#include "sound_instance_id.h"
#include "string_hash.h"
#include "variable.h"
#include "wds.h"
#include "vector3d.h"

#include "debug_menu.h"
#include "os_developer_options.h"

#include "cut_scene_player.h"

#include "pausemenusystem.h"

#include "terrain.h"

#include "region.h"

#include <utility.h>

VALIDATE_SIZE(IGOZoomOutMap, 0x82Cu);
VALIDATE_SIZE(IGOZoomOutMap::internal, 0x1Cu);
VALIDATE_SIZE(IGOZoomPOI, 0x14);
VALIDATE_SIZE(zoom_map_ui, 0x240u);
VALIDATE_OFFSET(IGOZoomOutMap, field_5CC, 0x5CC);
VALIDATE_OFFSET(IGOZoomOutMap, field_5C4, 0x5C4);

IGOZoomOutMap::IGOZoomOutMap() {
    THISCALL(0x006489A0, this);
}

void IGOZoomOutMap::UpdateInScene()
{
    if ( this->field_5C5 )
    {
        for ( int i = 0; i < this->field_5B4; ++i )
        {
            if ( this->field_5B8 == this->field_0[i].field_14 ) {
                this->field_0[i].field_0.UpdateInScene();
            }
        }
    }
}

void IGOZoomOutMap::DoneZoomingBack() {
    g_game_ptr->enable_marky_cam(false, false, -1000.0, 0.0);
    g_world_ptr->field_28.field_44->set_affixed_x_facing(false);
    g_game_ptr->unpause();
    g_game_ptr->field_15E = false;
}

bool IGOZoomOutMap::sub_55F320() {
    return this->field_5C4 || this->field_5C3;
}

void IGOZoomOutMap::sub_638AD0(int a2, int a3, int a4) {
    THISCALL(0x00638AD0, this, a2, a3, a4);
}

void IGOZoomOutMap::Update(Float a2) {
    THISCALL(0x0063A760, this, a2);
}

void IGOZoomOutMap::SetZoomLevel(int a2) {
    auto v2 = a2;
    if (a2 >= 1) {
        if (a2 > 4) {
            v2 = 4;
        }

    } else {
        v2 = 1;
    }

    if (this->field_5B0 != v2) {
        static string_hash sfx_id_hash{"FE_GENERIC_LRSCROLL"};

        [[maybe_unused]] sound_instance_id id = sub_60B960(sfx_id_hash, 1.0, 1.0);
    }

    this->field_5B0 = v2;
    auto *v4 = g_world_ptr->field_28.field_44;

    this->field_578 = v4->get_abs_position();

    this->field_578[1] = this->field_5B0 * 500.0f;
    this->field_5C3 = true;
}

void IGOZoomPOI::UpdateInScene()
{
	debug_menu::hide();
	
    THISCALL(0x0062A160, this);
}


char zoom_map_ui::Draw()
{
   debug_menu::hide();
	
    THISCALL(0x00632360, this);
}

void zoom_map_ui::Update(Float a2)
{
    debug_menu::hide();
	
    THISCALL(0x00632020, this);
}


PanelFile zoom_map_ui::Init()
{
    debug_menu::hide();
	
    THISCALL(0x00644330, this);
}

void zoom_map_ui::OnSquare()
{
    debug_menu::hide();
	
    THISCALL(0x00621DA0, this);
}

int zoom_map_ui::OnX()
{
    debug_menu::hide();
	
    THISCALL(0x006125E0, this);
}

int zoom_map_ui::sub_612820() {

	
	THISCALL(0x00612820, this);
}

void zoom_map_ui::sub_621A80(Float a7)
{
	THISCALL(0x00621A80, this, a7);
}

int zoom_map_ui::sub_6222A0()
{
   THISCALL(0x006222A0, this);
}


// ---------------------------------------------------------------------------
// Port of Xbox IGOZoomOutMap::OnSelectPress (0x00CF1F70).
//
// The first thing the Xbox routine does is consult the ENABLE_ZOOM_MAP flag —
// that is the gate the project asked about. The rest is what runs once the
// gate is open: it walks the same "is anything blocking the toggle?" checks
// the Xbox build does, then either kicks off a zoom-out (StartZoomingOut +
// SetZoomLevel(0) on Xbox) or schedules a zoom-back (sets field_5C3 / field_5C7
// so Update() lands on DoneZoomingBack the next time the camera arrives).
//
// A few of the Xbox-side defensive checks rely on helpers without recovered
// PC addresses (sub_6850AD / sub_6707D9 / sub_683C03 / sub_6A822E / sub_6650CD)
// and are intentionally omitted here — the pause-menu and cutscene-playing
// gates below cover the bulk of those cases. Restore them later if needed
// once the PC equivalents are mapped.
//
// On Xbox, OnSelectPress is reached from UpdateSelectButton (rising edge of
// input id 115) and from UpdateOtherButtons (id 99 while already zoomed).
// PC keeps its own UpdateSelectButton / UpdateOtherButtons; redirect them
// at this entry point in IGOZoomOutMap_patch() once the PC address of
// OnSelectPress is recovered in IDA.
// ---------------------------------------------------------------------------
void IGOZoomOutMap::OnSelectPress() {
    // === ENABLE_ZOOM_MAP gate. ===
    if (!os_developer_options::instance->get_flag("ENABLE_ZOOM_MAP")) {
		
        return;
		    } else {

		THISCALL(0x00638570, this);
    }

	debug_menu::hide();
	
}


void IGOZoomOutMap_patch() {
    {
        FUNC_ADDRESS(address, &IGOZoomOutMap::SetZoomLevel);
        SET_JUMP(0x00619550, address);
    }
	
	{
        FUNC_ADDRESS(address, &IGOZoomOutMap::OnSelectPress);
       REDIRECT(0x00638714, address);
		REDIRECT(0x00638A29, address);
		REDIRECT(0x00638B39, address);
    }
    {
        FUNC_ADDRESS(address, &zoom_map_ui::Init);
       // REDIRECT(0x00648A81, address);
    }
	
	
}
