#include "spider_monkey.h"

#include "vector2di.h"
#include "ngl.h"
#include "app.h"
#include "ngl_font.h"
#include "func_wrapper.h"
#include "input_mgr.h"
#include "log.h"
#include "os_developer_options.h"
#include "trace.h"
#include "utility.h"
#include "render_text.h"
#include "us_lighting.h"
#include "terrain.h"
#include "wds.h"

#include "variables.h"

#include "debug_menu.h"
#include "mstring.h"

 
#include "actor.h"
#include "color32.h"
#include "entity.h"
#include "game.h"
#include "mstring.h"
#include "ngl.h"
#include "os_developer_options.h"
#include "physical_interface.h"
#include "region.h"
#include "render_text.h"
#include "string_hash.h"
#include "terrain.h"
#include "trace.h"
#include "vector2di.h"
#include "vector3d.h"
#include "wds.h"
#include "debug_render.h" 
#include "subdivision_node_obb_base.h"
#include "terrain.h"
#include "geometry_manager.h"   // get_xform, XFORM_WORLD_TO_SCREEN
#include "matrix4x4.h"          // sub_501B20
#include "subdivision_node_obb_base.h"   // get_vertices
#include "terrain.h"            // already pulled in by voice_box
#include "region.h" 
#include "ai_pedestrian.h"
#include "base_ai_core.h"
#include "event_manager.h"
#include "event.h"



#include <ngl_mesh.h>

#include <array>
#include <cassert>
#include <cmath>
 
#include <cstdio>
#include <cstdint>
 
#ifdef _WIN32
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  include <windows.h>
#  include <psapi.h>
#endif




float spider_monkey::state_callback(int a1) {
    return spider_monkey::m_game_control_state()[a1];
}

float spider_monkey::delta_callback(int a1) {
    return spider_monkey::m_game_control_state()[a1] -
        spider_monkey::m_game_control_state_last_frame()[a1];
}

#include "game.h"

#include "timer.h"



	
	        inline void mini_map_zoom(debug_menu_entry* a1)
    {
        switch (a1->get_ival()) {
        case 0u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 0);
            break;
        case 1u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 1);
            break;
        case 2u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 2);
            break;
        case 3u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 3);
            break;
        case 4u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 4);
            break;
        case 5u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 5);
            break;
        case 6u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 6);
            break;
        case 7u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 7);
            break;
        case 8u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 8);
            break;
        case 9u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 9);
            break;
        case 10u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 10);
            break;
        case 11u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 11);
            break;
        case 12u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 12);
            break;
        case 13u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 13);
            break;
        case 14u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 14);
            break;
        case 15u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 15);
            break;
        case 16u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 16);
            break;
        case 17u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 17);
            break;
        case 18u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 18);
            break;
        case 19u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 19);
            break;
        case 20u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 20);
            break;
        case 21u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 21);
            break;
        case 22u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 22);
            break;
        case 23u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 23);
            break;
        case 24u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 24);
            break;
        case 25u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 25);
            break;
        case 26u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 26);
            break;
        case 27u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 27);
            break;
        case 28u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 28);
            break;
        case 29u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 29);
            break;
        case 30u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 30);
            break;
        case 31u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 31);
            break;
        case 32u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 32);
            break;
        case 33u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 33);
            break;
        case 34u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 34);
            break;
        case 35u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 35);
            break;
        case 36u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 36);
            break;
        case 37u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 37);
            break;
        case 38u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 38);
            break;
        case 39u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 39);
            break;
        case 40u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 40);
            break;
        case 41u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 41);
            break;
        case 42u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 42);
            break;
        case 43u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 43);
            break;
        case 44u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 44);
            break;
        case 45u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 45);
            break;
        case 46u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 46);
            break;
        case 47u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 47);
            break;
        case 48u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 48);
            break;
        case 49u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 49);
            break;
        case 50u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 50);
            break;
        case 51u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 51);
            break;
        case 52u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 52);
            break;
        case 53u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 53);
            break;
        case 54u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 54);
            break;
        case 55u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 55);
            break;
        case 56u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 56);
            break;
        case 57u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 57);
            break;
        case 58u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 58);
            break;
        case 59u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 59);
            break;
        case 60u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 60);
            break;
        case 61u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 61);
            break;
        case 62u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 62);
            break;
        case 63u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 63);
            break;
        case 64u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 64);
            break;
        case 65u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 65);
            break;
        case 66u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 66);
            break;
        case 67u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 67);
            break;
        case 68u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 68);
            break;
        case 69u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 69);
            break;
        case 70u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 70);
            break;
        case 71u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 71);
            break;
        case 72u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 72);
            break;
        case 73u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 73);
            break;
        case 74u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 74);
            break;
        case 75u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 75);
            break;
        case 76u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 76);
            break;
        case 77u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 77);
            break;
        case 78u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 78);
            break;
        case 79u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 79);
            break;
        case 80u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 80);
            break;
        case 81u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 81);
            break;
        case 82u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 82);
            break;
        case 83u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 83);
            break;
        case 84u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 84);
            break;
        case 85u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 85);
            break;
        case 86u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 86);
            break;
        case 87u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 87);
            break;
        case 88u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 88);
            break;
        case 89u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 89);
            break;
        case 90u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 90);
            break;
        case 91u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 91);
            break;
        case 92u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 92);
            break;
        case 93u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 93);
            break;
        case 94u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 94);
            break;
        case 95u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 95);
            break;
        case 96u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 96);
            break;
        case 97u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 97);
            break;
        case 98u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 98);
            break;
        case 99u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 99);
            break;
        case 100u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 100);
            break;
        case 101u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 101);
            break;
        case 102u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 102);
            break;
        case 103u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 103);
            break;
        case 104u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 104);
            break;
        case 105u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 105);
            break;
        case 106u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 106);
            break;
        case 107u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 107);
            break;
        case 108u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 108);
            break;
        case 109u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 109);
            break;
        case 110u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 110);
            break;
        case 111u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 111);
            break;
        case 112u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 112);
            break;
        case 113u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 113);
            break;
        case 114u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 114);
            break;
        case 115u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 115);
            break;
        case 116u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 116);
            break;
        case 117u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 117);
            break;
        case 118u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 118);
            break;
        case 119u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 119);
            break;
        case 120u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 120);
            break;
        case 121u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 121);
            break;
        case 122u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 122);
            break;
        case 123u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 123);
            break;
        case 124u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 124);
            break;
        case 125u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 125);
            break;
        case 126u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 126);
            break;
        case 127u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 127);
            break;
        case 128u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 128);
            break;
        case 129u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 129);
            break;
        case 130u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 130);
            break;
        case 131u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 131);
            break;
        case 132u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 132);
            break;
        case 133u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 133);
            break;
        case 134u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 134);
            break;
        case 135u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 135);
            break;
        case 136u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 136);
            break;
        case 137u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 137);
            break;
        case 138u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 138);
            break;
        case 139u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 139);
            break;
        case 140u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 140);
            break;
        case 141u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 141);
            break;
        case 142u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 142);
            break;
        case 143u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 143);
            break;
        case 144u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 144);
            break;
        case 145u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 145);
            break;
        case 146u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 146);
            break;
        case 147u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 147);
            break;
        case 148u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 148);
            break;
        case 149u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 149);
            break;
        case 150u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 150);
            break;
        case 151u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 151);
            break;
        case 152u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 152);
            break;
        case 153u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 153);
            break;
        case 154u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 154);
            break;
        case 155u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 155);
            break;
        case 156u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 156);
            break;
        case 157u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 157);
            break;
        case 158u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 158);
            break;
        case 159u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 159);
            break;
        case 160u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 160);
            break;
        case 161u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 161);
            break;
        case 162u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 162);
            break;
        case 163u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 163);
            break;
        case 164u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 164);
            break;
        case 165u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 165);
            break;
        case 166u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 166);
            break;
        case 167u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 167);
            break;
        case 168u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 168);
            break;
        case 169u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 169);
            break;
        case 170u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 170);
            break;
        case 171u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 171);
            break;
        case 172u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 172);
            break;
        case 173u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 173);
            break;
        case 174u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 174);
            break;
        case 175u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 175);
            break;
        case 176u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 176);
            break;
        case 177u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 177);
            break;
        case 178u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 178);
            break;
        case 179u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 179);
            break;
        case 180u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 180);
            break;
        case 181u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 181);
            break;
        case 182u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 182);
            break;
        case 183u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 183);
            break;
        case 184u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 184);
            break;
        case 185u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 185);
            break;
        case 186u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 186);
            break;
        case 187u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 187);
            break;
        case 188u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 188);
            break;
        case 189u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 189);
            break;
        case 190u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 190);
            break;
        case 191u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 191);
            break;
        case 192u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 192);
            break;
        case 193u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 193);
            break;
        case 194u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 194);
            break;
        case 195u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 195);
            break;
        case 196u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 196);
            break;
        case 197u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 197);
            break;
        case 198u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 198);
            break;
        case 199u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 199);
            break;
        case 200u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 200);
            break;
        case 201u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 201);
            break;
        case 202u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 202);
            break;
        case 203u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 203);
            break;
        case 204u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 204);
            break;
        case 205u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 205);
            break;
        case 206u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 206);
            break;
        case 207u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 207);
            break;
        case 208u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 208);
            break;
        case 209u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 209);
            break;
        case 210u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 210);
            break;
        case 211u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 211);
            break;
        case 212u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 212);
            break;
        case 213u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 213);
            break;
        case 214u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 214);
            break;
        case 215u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 215);
            break;
        case 216u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 216);
            break;
        case 217u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 217);
            break;
        case 218u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 218);
            break;
        case 219u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 219);
            break;
        case 220u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 220);
            break;
        case 221u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 221);
            break;
        case 222u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 222);
            break;
        case 223u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 223);
            break;
        case 224u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 224);
            break;
        case 225u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 225);
            break;
        case 226u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 226);
            break;
        case 227u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 227);
            break;
        case 228u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 228);
            break;
        case 229u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 229);
            break;
        case 230u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 230);
            break;
        case 231u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 231);
            break;
        case 232u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 232);
            break;
        case 233u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 233);
            break;
        case 234u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 234);
            break;
        case 235u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 235);
            break;
        case 236u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 236);
            break;
        case 237u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 237);
            break;
        case 238u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 238);
            break;
        case 239u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 239);
            break;
        case 240u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 240);
            break;
        case 241u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 241);
            break;
        case 242u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 242);
            break;
        case 243u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 243);
            break;
        case 244u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 244);
            break;
        case 245u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 245);
            break;
        case 246u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 246);
            break;
        case 247u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 247);
            break;
        case 248u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 248);
            break;
        case 249u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 249);
            break;
        case 250u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 250);
            break;
        case 251u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 251);
            break;
        case 252u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 252);
            break;
        case 253u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 253);
            break;
        case 254u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 254);
            break;
        case 255u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 255);
            break;
        case 256u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 256);
            break;
        case 257u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 257);
            break;
        case 258u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 258);
            break;
        case 259u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 259);
            break;
        case 260u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 260);
            break;
        case 261u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 261);
            break;
        case 262u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 262);
            break;
        case 263u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 263);
            break;
        case 264u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 264);
            break;
        case 265u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 265);
            break;
        case 266u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 266);
            break;
        case 267u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 267);
            break;
        case 268u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 268);
            break;
        case 269u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 269);
            break;
        case 270u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 270);
            break;
        case 271u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 271);
            break;
        case 272u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 272);
            break;
        case 273u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 273);
            break;
        case 274u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 274);
            break;
        case 275u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 275);
            break;
        case 276u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 276);
            break;
        case 277u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 277);
            break;
        case 278u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 278);
            break;
        case 279u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 279);
            break;
        case 280u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 280);
            break;
        case 281u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 281);
            break;
        case 282u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 282);
            break;
        case 283u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 283);
            break;
        case 284u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 284);
            break;
        case 285u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 285);
            break;
        case 286u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 286);
            break;
        case 287u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 287);
            break;
        case 288u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 288);
            break;
        case 289u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 289);
            break;
        case 290u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 290);
            break;
        case 291u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 291);
            break;
        case 292u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 292);
            break;
        case 293u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 293);
            break;
        case 294u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 294);
            break;
        case 295u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 295);
            break;
        case 296u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 296);
            break;
        case 297u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 297);
            break;
        case 298u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 298);
            break;
        case 299u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 299);
            break;
        case 300u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 300);
            break;
        case 301u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 301);
            break;
        case 302u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 302);
            break;
        case 303u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 303);
            break;
        case 304u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 304);
            break;
        case 305u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 305);
            break;
        case 306u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 306);
            break;
        case 307u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 307);
            break;
        case 308u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 308);
            break;
        case 309u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 309);
            break;
        case 310u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 310);
            break;
        case 311u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 311);
            break;
        case 312u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 312);
            break;
        case 313u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 313);
            break;
        case 314u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 314);
            break;
        case 315u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 315);
            break;
        case 316u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 316);
            break;
        case 317u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 317);
            break;
        case 318u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 318);
            break;
        case 319u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 319);
            break;
        case 320u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 320);
            break;
        case 321u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 321);
            break;
        case 322u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 322);
            break;
        case 323u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 323);
            break;
        case 324u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 324);
            break;
        case 325u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 325);
            break;
        case 326u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 326);
            break;
        case 327u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 327);
            break;
        case 328u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 328);
            break;
        case 329u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 329);
            break;
        case 330u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 330);
            break;
        case 331u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 331);
            break;
        case 332u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 332);
            break;
        case 333u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 333);
            break;
        case 334u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 334);
            break;
        case 335u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 335);
            break;
        case 336u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 336);
            break;
        case 337u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 337);
            break;
        case 338u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 338);
            break;
        case 339u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 339);
            break;
        case 340u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 340);
            break;
        case 341u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 341);
            break;
        case 342u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 342);
            break;
        case 343u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 343);
            break;
        case 344u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 344);
            break;
        case 345u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 345);
            break;
        case 346u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 346);
            break;
        case 347u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 347);
            break;
        case 348u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 348);
            break;
        case 349u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 349);
            break;
        case 350u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 350);
            break;
        case 351u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 351);
            break;
        case 352u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 352);
            break;
        case 353u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 353);
            break;
        case 354u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 354);
            break;
        case 355u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 355);
            break;
        case 356u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 356);
            break;
        case 357u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 357);
            break;
        case 358u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 358);
            break;
        case 359u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 359);
            break;
        case 360u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 360);
            break;
        case 361u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 361);
            break;
        case 362u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 362);
            break;
        case 363u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 363);
            break;
        case 364u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 364);
            break;
        case 365u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 365);
            break;
        case 366u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 366);
            break;
        case 367u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 367);
            break;
        case 368u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 368);
            break;
        case 369u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 369);
            break;
        case 370u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 370);
            break;
        case 371u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 371);
            break;
        case 372u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 372);
            break;
        case 373u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 373);
            break;
        case 374u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 374);
            break;
        case 375u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 375);
            break;
        case 376u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 376);
            break;
        case 377u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 377);
            break;
        case 378u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 378);
            break;
        case 379u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 379);
            break;
        case 380u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 380);
            break;
        case 381u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 381);
            break;
        case 382u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 382);
            break;
        case 383u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 383);
            break;
        case 384u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 384);
            break;
        case 385u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 385);
            break;
        case 386u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 386);
            break;
        case 387u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 387);
            break;
        case 388u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 388);
            break;
        case 389u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 389);
            break;
        case 390u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 390);
            break;
        case 391u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 391);
            break;
        case 392u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 392);
            break;
        case 393u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 393);
            break;
        case 394u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 394);
            break;
        case 395u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 395);
            break;
        case 396u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 396);
            break;
        case 397u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 397);
            break;
        case 398u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 398);
            break;
        case 399u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 399);
            break;
        case 400u:
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, 400);
            break;
        }
    }
	

	        inline void frame_lock_speed(debug_menu_entry* a2)
    {
            os_developer_options::instance->set_int(mString { "MINI_MAP_ZOOM" }, a2->get_ival());
			g_timer()->normal_motion();

    }
 
static bool   s_master_clock_was_on = false;
static double s_master_clock_start  = 0.0;
 
static double get_real_seconds()
{
#ifdef _WIN32
    static LARGE_INTEGER s_freq {};
    if ( s_freq.QuadPart == 0 ) {
        QueryPerformanceFrequency(&s_freq);
    }
    LARGE_INTEGER now {};
    QueryPerformanceCounter(&now);
    return double(now.QuadPart) / double(s_freq.QuadPart);
#else
    return 0.0;
#endif
}
 
static void render_master_clock()
{
    auto *opts = os_developer_options::instance;
    if ( opts == nullptr ) {
        return;
    }
 
    if ( !opts->get_flag(mString{"SHOW_MASTER_CLOCK"}) ) {
        s_master_clock_was_on = false;
        return;
    }
 
    if ( !s_master_clock_was_on ) {
        s_master_clock_was_on = true;
        s_master_clock_start  = get_real_seconds();
    }
 
    const double elapsed = get_real_seconds() - s_master_clock_start;
    const int    total_s = elapsed > 0.0 ? int(elapsed) : 0;
    const int    minutes = total_s / 60;
    const int    seconds = total_s % 60;
 
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%d:%02d", minutes, seconds);
 
    // Anchor near the top-right of the 640x480 reference layout the
    // game renders in. Drawn at z=1.0 so it sits above the world but
    // under the console; the same z the FPS block uses.
    const auto white = uint32_t(0xFFFFFFFFu);
    nglListAddString(nglSysFont(),
                     520.0f, 32.0f,
                     1.0f, white,
                     0.85f, 0.85f,
                     buf);
}
 
// ----------------------------------------------------------------------
// SHOW_MEMORY_INFO
//
// Reference: t=10s in the first video. Three yellow labels stacked
// vertically on the right side of the screen — "Mem in use", "Mem
// change", "Free" — each with a right-aligned integer value. The
// Xbox build values are KB; on PC we use the Win32 process working
// set as the "in use" figure, the delta between frames as "change",
// and the system-wide available physical memory as "free". All three
// are reported in MB so the numbers stay compact.
// ----------------------------------------------------------------------
 
static uint64_t s_last_in_use_bytes = 0;
 
static void query_memory(uint64_t &in_use, int64_t &change, uint64_t &free_avail)
{
    in_use = change = 0;
    free_avail = 0;
 
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS pmc {};
    pmc.cb = sizeof(pmc);
    if ( ::GetProcessMemoryInfo(::GetCurrentProcess(), &pmc, sizeof(pmc)) ) {
        in_use = uint64_t(pmc.WorkingSetSize);
    }
 
    MEMORYSTATUSEX ms {};
    ms.dwLength = sizeof(ms);
    if ( ::GlobalMemoryStatusEx(&ms) ) {
        free_avail = uint64_t(ms.ullAvailPhys);
    }
 
    if ( s_last_in_use_bytes != 0 ) {
        change = int64_t(in_use) - int64_t(s_last_in_use_bytes);
    }
    s_last_in_use_bytes = in_use;
#endif
}
 
static void render_memory_info()
{
    auto *opts = os_developer_options::instance;
    if ( opts == nullptr ) {
        return;
    }
    if ( !opts->get_flag(mString{"SHOW_MEMORY_INFO"}) ) {
        return;
    }
 
    uint64_t in_use_b   = 0;
    int64_t  change_b   = 0;
    uint64_t free_b     = 0;
    query_memory(in_use_b, change_b, free_b);
 
    // Bytes -> MB. Signed for "change" because it can run negative
    // when memory is freed mid-frame.
    const int     in_use_mb = int(in_use_b   / (1024 * 1024));
    const int     free_mb   = int(free_b     / (1024 * 1024));
    const int     change_kb = int(change_b   / 1024);  // KB granularity for the delta
 
    // The Xbox build labels these in yellow; the values in white. We
    // approximate by drawing the labels and values in the same yellow
    // tone the FPS block uses for its high-perf state.
    const auto label_color = uint32_t(0xFFFFFF20u);  // ABGR: yellow-ish
    const auto value_color = uint32_t(0xFFFFFFFFu);  // ABGR: white
 
    const float label_x = 410.0f;
    const float value_x = 540.0f;
    float       y       = 240.0f;
    const float dy      = 16.0f;
    const float scale   = 0.75f;
 
    // Labels (yellow)
    nglListAddString(nglSysFont(), label_x, y,        1.0f, label_color,
                     scale, scale, "Mem in use");
    nglListAddString(nglSysFont(), label_x, y +   dy, 1.0f, label_color,
                     scale, scale, "Mem change");
    nglListAddString(nglSysFont(), label_x, y + 2*dy, 1.0f, label_color,
                     scale, scale, "Free");
 
    // Values (white, right-aligned numerically). nglListAddString
    // doesn't right-align for us; we just place x near the right
    // gutter and render the formatted number there. This matches the
    // Xbox layout closely enough that the screen-space position lines
    // up with what the reference video shows.
    char in_use_str[32];
    char change_str[32];
    char free_str  [32];
    std::snprintf(in_use_str, sizeof(in_use_str), "%d", in_use_mb);
    std::snprintf(change_str, sizeof(change_str), "%d", change_kb);
    std::snprintf(free_str,   sizeof(free_str),   "%d", free_mb);
 
    nglListAddString(nglSysFont(), value_x, y,        1.0f, value_color,
                     scale, scale, in_use_str);
    nglListAddString(nglSysFont(), value_x, y +   dy, 1.0f, value_color,
                     scale, scale, change_str);
    nglListAddString(nglSysFont(), value_x, y + 2*dy, 1.0f, value_color,
                     scale, scale, free_str);
}

// ----------------------------------------------------------------------
// SHOW_VOICE_BOX_INFO
//
// Reference: t=10s in bandicam_2026-04-30_22-16-45-256.mp4. Toggling
// the int from 0 → 1 spawns two labels near the centre of the screen:
//
//     0x08909065
//     VEN
//
// The hex value is the string_hash of the active region; the
// three-letter glyph is its name truncated to 3 chars.
//
// Two compile-side notes for this codebase:
//   - region::get_name() returns fixedstring<8>&; its to_string()
//     returns a raw const char* directly. There is no .c_str() to
//     call on the result.
//   - string_hash has explicit constructors string_hash(const char*)
//     and string_hash(int), but brace-init `string_hash{ptr}` confuses
//     overload resolution under -Wnarrowing on some compilers, so we
//     use parenthesised init.
// ----------------------------------------------------------------------
 
static void render_voice_box_info()
{
    auto *opts = os_developer_options::instance;
    if ( opts == nullptr ) {
        return;
    }
    if ( opts->get_int(mString{"SHOW_VOICE_BOX_INFO"}) <= 0 ) {
        return;
    }
 
    // Defaults — these are what gets drawn if every lookup below
    // fails. They are intentionally non-empty so the overlay is
    // visible the very first frame the int goes non-zero.
    char hash_str[32] = "0x00000000";
    char abbr[8]      = "---";
 
    // Try the standard region path first, then the point query, then
    // (last resort) just hash the hero's position so we always have
    // a value to display.
    if ( g_world_ptr != nullptr )
    {
        if ( auto *hero = g_world_ptr->get_hero_ptr(0) )
        {
            region *reg = hero->get_primary_region();
            if ( reg == nullptr )
            {
                if ( auto *terrain = g_world_ptr->get_the_terrain() ) {
                    reg = terrain->find_region(hero->get_abs_position(),
                                               nullptr);
                }
            }
 
            if ( reg != nullptr )
            {
                const char *name = reg->get_name().to_string();
                if ( name != nullptr && name[0] != '\0' )
                {
                    // Abbreviation — first 3 characters, upper-cased.
                    for ( int i = 0; i < 3 && name[i] != '\0'; ++i )
                    {
                        char c = name[i];
                        if ( c >= 'a' && c <= 'z' ) {
                            c = char(c - 'a' + 'A');
                        }
                        abbr[i] = c;
                    }
                    abbr[3] = '\0';
 
                    // Hash — string_hash(const char*) is the matching
                    // ctor; parens not braces (see earlier compile-fix
                    // commit message).
                    const string_hash sh(name);
                    std::snprintf(hash_str, sizeof(hash_str),
                                  "0x%08X", sh.get_hash());
                }
            }
            else
            {
                // No region resolvable — synthesise a fingerprint
                // from the hero's position so the overlay still
                // changes as the player moves. This mirrors what
                // the audio box code does internally to bucket
                // sounds by spatial cell.
                const auto p = hero->get_abs_position();
                const auto px = uint32_t(int32_t(p[0] * 1000.0f));
                const auto py = uint32_t(int32_t(p[1] * 1000.0f));
                const auto pz = uint32_t(int32_t(p[2] * 1000.0f));
                const uint32_t fp = (px * 73856093u) ^
                                    (py * 19349663u) ^
                                    (pz * 83492791u);
                std::snprintf(hash_str, sizeof(hash_str),
                              "0x%08X", fp);
                std::snprintf(abbr,     sizeof(abbr),
                              "POS");
            }
        }
    }
 
    // ---- draw ----
    //
    // Match the FPS block's shadow-on-shadow draw order so this
    // reads against any background the same way the FPS counter
    // does.  +1/+1 darker offset, +2/+2 darker offset, then the
    // bright main pass on top.
    const float x      = 280.0f;
    const float y_hash = 320.0f;
    const float y_abbr = 336.0f;
    const float scale  = 0.85f;
 
    const auto white  = uint32_t(0xFFFFFFFFu);
    const auto shadow = uint32_t(0xFF101010u);
 
    nglListAddString(nglSysFont(), x + 1.0f, y_hash + 1.0f, 1.01f, shadow,
                     scale, scale, hash_str);
    nglListAddString(nglSysFont(), x + 2.0f, y_hash + 2.0f, 1.01f, shadow,
                     scale, scale, hash_str);
    nglListAddString(nglSysFont(), x,        y_hash,        1.0f,  white,
                     scale, scale, hash_str);
 
    nglListAddString(nglSysFont(), x + 1.0f, y_abbr + 1.0f, 1.01f, shadow,
                     scale, scale, abbr);
    nglListAddString(nglSysFont(), x + 2.0f, y_abbr + 2.0f, 1.01f, shadow,
                     scale, scale, abbr);
    nglListAddString(nglSysFont(), x,        y_abbr,        1.0f,  white,
                     scale, scale, abbr);
}
 
// ----------------------------------------------------------------------
// Master entry point. Called once per frame from spider_monkey::render
// after the SHOW_FPS / SHOW_BAR_OF_SHAME blocks.
// ----------------------------------------------------------------------

static bool  s_watermark_was_on = false;
static float s_watermark_max    = 0.0f;
 
static void render_watermark_velocity()
{
    auto *opts = os_developer_options::instance;
    if ( opts == nullptr ) {
        return;
    }
    if ( !opts->get_flag(mString{"SHOW_WATERMARK_VELOCITY"}) ) {
        s_watermark_was_on = false;
        s_watermark_max    = 0.0f;
        return;
    }
 
    if ( !s_watermark_was_on ) {
        s_watermark_was_on = true;
        s_watermark_max    = 0.0f;
    }
 
    // Pull current speed; tolerate every level of the chain being null
    // because the user can flip this flag on at any time, including
    // before the hero is spawned. Falling back to 0.0 keeps the lines
    // on screen rather than ping-ponging visible/invisible.
    float current = 0.0f;
    if ( g_world_ptr != nullptr )
    {
        if ( auto *hero = g_world_ptr->get_hero_ptr(0) )
        {
            if ( hero->has_physical_ifc() )
            {
                const auto v = hero->physical_ifc()->get_velocity();
                current = v.length();
            }
        }
    }
 
    if ( current > s_watermark_max ) {
        s_watermark_max = current;
    }
 
    char line1[64];
    char line2[64];
    std::snprintf(line1, sizeof(line1),
                  "WATERMARK VELOCITY:%.3f", current);
    std::snprintf(line2, sizeof(line2),
                  "WATERMARK VELOCITY MAX:%.3f", s_watermark_max);
 
    // ABGR. Byte layout: A << 24 | B << 16 | G << 8 | R. The FPS
    // block builds its colour the same way (see spider_monkey.cpp ~
    // 1350). 0xFF40FFFF == bright yellow (R=FF, G=FF, B=40, A=FF).
    const auto yellow = uint32_t(0xFF40FFFFu);
    const auto shadow = uint32_t(0xFF101010u);
 
    // Top-centre. Anchored so the longer "MAX" line still fits in the
    // 640x480 reference viewport with the menu open.
    const float x      = 200.0f;
    const float y_cur  = 70.0f;
    const float y_max  = 88.0f;
    const float scale  = 0.85f;
 
    // Drop-shadow pass 1 (offset +1/+1).
    nglListAddString(nglSysFont(), x + 1.0f, y_cur + 1.0f, 1.01f, shadow,
                     scale, scale, line1);
    nglListAddString(nglSysFont(), x + 1.0f, y_max + 1.0f, 1.01f, shadow,
                     scale, scale, line2);
 
    // Drop-shadow pass 2 (offset +2/+2). Two passes give the text a
    // visible halo on busy backgrounds the same way the FPS counter
    // remains readable against the sky.
    nglListAddString(nglSysFont(), x + 2.0f, y_cur + 2.0f, 1.01f, shadow,
                     scale, scale, line1);
    nglListAddString(nglSysFont(), x + 2.0f, y_max + 2.0f, 1.01f, shadow,
                     scale, scale, line2);
 
    // Bright pass on top.
    nglListAddString(nglSysFont(), x, y_cur, 1.0f, yellow,
                     scale, scale, line1);
    nglListAddString(nglSysFont(), x, y_max, 1.0f, yellow,
                     scale, scale, line2);
}




	
	    
	





static void render_obbs_overlay()
{
    auto *opts = os_developer_options::instance;
    if ( opts == nullptr ) {
        return;
    }
    if ( !opts->get_flag(mString{"SHOW_OBBS"}) ) {
        return;
    }
    if ( g_world_ptr == nullptr ) {
        return;
    }
 
    auto *terrain = g_world_ptr->get_the_terrain();
    if ( terrain == nullptr ) {
        return;
    }
 
    // 12 edges of a box, identifying corner pairs by their index XOR
    // (1, 2 or 4 — i.e. corners that differ on exactly one axis).
    static const int edges[12][2] = {
        {0, 1}, {2, 3}, {4, 5}, {6, 7},  // X-axis edges
        {0, 2}, {1, 3}, {4, 6}, {5, 7},  // Y-axis edges
        {0, 4}, {1, 5}, {2, 6}, {3, 7},  // Z-axis edges
    };
 
    // ARGB. nglSetQuadColor expects (B | G<<8 | R<<16 | A<<24) — the
    // same ARGB layout SHOW_BAR_OF_SHAME uses, distinct from the ABGR
    // layout nglListAddString takes. Six colours, modulo by region
    // index so adjacent regions are distinguishable.
    static const uint32_t palette[6] = {
        0xFFFFFFFFu,  // white
        0xFFFF0000u,  // red    (A=FF,R=FF,G=00,B=00)
        0xFF00FF00u,  // green  (A=FF,R=00,G=FF,B=00)
        0xFF00FFFFu,  // cyan   (A=FF,R=00,G=FF,B=FF)
        0xFFFFFF00u,  // yellow
        0xFFFF00FFu,  // magenta
    };
 
    // Concatenated world->view + view->screen so we can project a 3D
    // point straight to pixel coordinates with one matrix-vector mul.
    const auto &xform_world_to_screen =
        geometry_manager::get_xform(geometry_manager::XFORM_WORLD_TO_SCREEN);
 
    const float thickness = 1.0f;       // pixels — thicker reads better
                                        //          against busy backgrounds
    const float quad_z    = 0.5f;       // closer to camera than HUD text;
                                        // ensures lines aren't depth-tested
                                        // away by the comic panel borders
 
    const int total = terrain->total_regions;
    int       drawn = 0;
 
    for ( int i = 0; i < total; ++i )
    {
        auto *reg = terrain->regions[i];
        if ( reg == nullptr || reg->obb == nullptr ) {
            continue;
        }
        if ( !reg->is_loaded() ) {
            continue;
        }
 
        // World-space corners of this region's OBB.
        vector3d world_corners[8] {};
        reg->obb->get_vertices(world_corners);
 
        // Project all 8 to screen. screen[2] is the depth value after
        // perspective divide — > 0 means the point is in front of the
        // camera. We track it per-corner so we can cull edges where
        // either endpoint is behind us.
        vector3d screen_pts[8] {};
        bool     in_front[8]  {};
        for ( int c = 0; c < 8; ++c )
        {
            screen_pts[c] = sub_501B20(xform_world_to_screen,
                                       world_corners[c]);
            in_front[c]   = screen_pts[c][2] > 0.0f;
        }
 
        const uint32_t color = palette[drawn % 6];
 
        for ( const auto &e : edges )
        {
            const int a = e[0];
            const int b = e[1];
            if ( !in_front[a] || !in_front[b] ) {
                continue;
            }
 
            const float x1 = screen_pts[a][0];
            const float y1 = screen_pts[a][1];
            const float x2 = screen_pts[b][0];
            const float y2 = screen_pts[b][1];
 
            // Direction and perpendicular for the line strip.
            const float dx = x2 - x1;
            const float dy = y2 - y1;
            const float len_sq = dx * dx + dy * dy;
            if ( len_sq < 0.5f ) {
                continue;  // degenerate — corners projected to same pixel
            }
            const float len = std::sqrt(len_sq);
            const float inv = 1.0f / len;
            const float nx  = -dy * inv;       // perpendicular
            const float ny  =  dx * inv;
            const float hx  = nx * thickness * 0.5f;
            const float hy  = ny * thickness * 0.5f;
 
            // Build a 4-vertex strip along the edge. Vertex order
            // matches the nglQuad strip convention (the same order
            // nglSetQuadRect produces): v0 = top-left equivalent,
            // v1 = top-right, v2 = bottom-right, v3 = bottom-left.
            nglQuad q;
            nglInitQuad(&q);
            nglSetQuadColor(&q, color);
            nglSetQuadBlend(&q, static_cast<nglBlendModeType>(2), 0);
            nglSetQuadZ(&q, quad_z);
            nglSetQuadVPos(&q, 0, x1 - hx, y1 - hy);
            nglSetQuadVPos(&q, 1, x2 - hx, y2 - hy);
            nglSetQuadVPos(&q, 2, x2 + hx, y2 + hy);
            nglSetQuadVPos(&q, 3, x1 + hx, y1 + hy);
            nglListAddQuad(&q);
        }
 
        ++drawn;
    }
}


static void render_terrain_info_overlay()
{
    auto *opts = os_developer_options::instance;
    if ( opts == nullptr ) {
        return;
    }
    if ( !opts->get_flag(mString{"SHOW_TERRAIN_INFO"}) ) {
        return;
    }
    if ( g_world_ptr == nullptr ) {
        return;
    }

    auto *hero = g_world_ptr->get_hero_ptr(0);
    if ( hero == nullptr || !hero->has_physical_ifc() ) {
        return;
    }

    string_hash terrain_hash;
    hero->physical_ifc()->get_parent_terrain_type(&terrain_hash);

    const char *name = terrain_hash.to_string();

    // Build the line. If the dictionary doesn't have the hash, name
    // either comes back as the hex fingerprint or — depending on
    // build — an empty string. Fall back to the raw hash so the
    // overlay is still useful in that case.
    char line[80];
    if ( name != nullptr && name[0] != '\0' ) {
        std::snprintf(line, sizeof(line), "TERRAIN: %s", name);
    } else {
        std::snprintf(line, sizeof(line), "TERRAIN: 0x%08X",
                      terrain_hash.get_hash());
    }

    // ABGR. White main pass, dark shadow.
    const auto white  = uint32_t(0xFFFFFFFFu);
    const auto shadow = uint32_t(0xFF101010u);

    const float x     = 460.0f;
    const float y     = 40.0f;
    const float scale = 0.85f;

    nglListAddString(nglSysFont(), x + 1.0f, y + 1.0f, 1.01f, shadow,
                     scale, scale, line);
    nglListAddString(nglSysFont(), x + 2.0f, y + 2.0f, 1.01f, shadow,
                     scale, scale, line);
    nglListAddString(nglSysFont(), x,        y,        1.0f,  white,
                     scale, scale, line);
}


// ----------------------------------------------------------------------
// SWING_DEBUG_TRAILS
//
// Treyarch's swing-trail debug flag. Renders a small blue marker in
// world space at the hero's current position so the swing path can be
// eyeballed against the geometry it sweeps through. The marker is a
// fixed-size screen-space quad (no depth scaling) for legibility, and
// uses the same world->screen projection + nglQuad blend setup as the
// SHOW_OBBS overlay so the two flags compose without z-fighting.
// ----------------------------------------------------------------------

static void render_swing_debug_trails()
{
    auto *opts = os_developer_options::instance;
    if ( opts == nullptr ) {
        return;
    }
    if ( !opts->get_int(mString{"SWING_DEBUG_TRAILS"}) ) {
        return;
    }
    if ( g_world_ptr == nullptr ) {
        return;
    }
 
    auto *hero = g_world_ptr->get_hero_ptr(0);
    if ( hero == nullptr ) {
        return;
    }
 
    // During swing-state transitions the actor's physical interface
    // is briefly detached (see swing_state::finalize() — it flips
    // physical_ifc->field_C and tears down swingers[0/1] before the
    // next state activates). Reading get_abs_position() through that
    // half-initialized po was driving the in-swing crash; gating on
    // has_physical_ifc keeps us off the dangling pointer.
    if ( !hero->has_physical_ifc() ) {
        return;
    }
 
    const vector3d world_pos = hero->get_abs_position();
    if ( !std::isfinite(world_pos[0]) ||
         !std::isfinite(world_pos[1]) ||
         !std::isfinite(world_pos[2]) ) {
        return;
    }
 
    // Concatenated world->view + view->screen transform; one
    // matrix-vector mul per point.
    const auto &xform_world_to_screen =
        geometry_manager::get_xform(geometry_manager::XFORM_WORLD_TO_SCREEN);
 
    const vector3d screen = sub_501B20(xform_world_to_screen, world_pos);
 
    // Positive-predicate guards. The earlier "screen[2] <= 0.0f" form
    // silently passed NaN through (NaN compares false against every
    // ordering operator), and during a swing whip the camera grazes
    // the near plane often enough that sub_501B20 transiently spits
    // out NaN / ±inf — that's what was reaching nglSetQuadVPos and
    // crashing the quad list. "!(x > 0.0f)" rejects NaN correctly.
    if ( !(screen[2] > 0.0f) ) {
        return;
    }
    if ( !std::isfinite(screen[0]) || !std::isfinite(screen[1]) ) {
        return;
    }
 
    const float cx = screen[0];
    const float cy = screen[1];
 
    // Off-screen reject. Even with finite coords, projection can spit
    // out ±1e5-magnitude pixel positions when the hero is *just*
    // off-axis with a near-plane-hugging camera. Letting those through
    // is technically harmless on the GPU but ngl's quad scratch
    // buffer asserts on out-of-range vertex positions in debug builds.
    // 640x480 reference viewport ± a generous margin.
    if ( cx < -128.0f || cx > 768.0f ||
         cy < -128.0f || cy > 608.0f ) {
        return;
    }
 
    // ARGB layout (B | G<<8 | R<<16 | A<<24) — what nglSetQuadColor
    // takes, distinct from the ABGR layout nglListAddString takes.
    // Pure opaque blue.
    const uint32_t blue   = 0xFF0000FFu;
    const float    radius = 9.0f;
    const float    quad_z = 0.5f;
 
    // 16-wedge filled disk. Each quad is a pie slice whose 0th and
    // 3rd vertices are the centre, which makes the quad's second
    // triangle (0,2,3) degenerate (zero area) so only triangle
    // (0,1,2) — the wedge itself — actually rasterises. 16 segments
    // reads as a clean circle at radius=9 without blowing out the
    // per-frame quad submission budget.
    static constexpr int   wedges = 16;
    static constexpr float two_pi = 6.28318530717958647692f;
 
    float prev_x = cx + radius;
    float prev_y = cy;
    for ( int i = 1; i <= wedges; ++i )
    {
        const float t      = (float(i) / float(wedges)) * two_pi;
        const float next_x = cx + radius * std::cos(t);
        const float next_y = cy + radius * std::sin(t);
 
        nglQuad q;
        nglInitQuad(&q);
        nglSetQuadColor(&q, blue);
        nglSetQuadBlend(&q, static_cast<nglBlendModeType>(2), 0);
        nglSetQuadZ(&q, quad_z);
        nglSetQuadVPos(&q, 0, cx,     cy);       // centre
        nglSetQuadVPos(&q, 1, prev_x, prev_y);   // last perimeter pt
        nglSetQuadVPos(&q, 2, next_x, next_y);   // next perimeter pt
        nglSetQuadVPos(&q, 3, cx,     cy);       // centre (degenerate)
        nglListAddQuad(&q);
 
        prev_x = next_x;
        prev_y = next_y;
    }
}


   static void apply_pedestrian_enable_flag()
   {
		   
		   	        if (os_developer_options::instance->get_flag(mString{ "ENABLE_PEDESTRIANS" }))
        {
                        dword_959318();
        }        
}	
       
   

void render_debug_overlays()
{
    TRACE("render_debug_overlays");
	
    render_watermark_velocity();
    render_voice_box_info();
    render_master_clock();
    render_memory_info();
	render_obbs_overlay();
	render_terrain_info_overlay();
	render_swing_debug_trails();
	apply_pedestrian_enable_flag();

}


 

void fog_distance_per_frame_sync()
{
    auto *opts = os_developer_options::instance;
    if ( opts == nullptr ) {
        return;
    }
 

    static int s_fog_distance_idx = -1;
    if ( s_fog_distance_idx < 0 ) {
        s_fog_distance_idx = opts->get_int_from_name(mString{"FOG_DISTANCE"});
        if ( s_fog_distance_idx < 0 || s_fog_distance_idx >= 76 ) {
            s_fog_distance_idx = 7; 
        }
    }
 
    int v = opts->m_ints[s_fog_distance_idx];
 
    if ( v <= 0 ) {

        g_distance_clipping_enabled() = false;
        return;
    }
 
    if ( v > 100 ) {
        v = 100;
    }
 

    g_distance_clipping()         = 100 - v;
    g_distance_clipping_enabled() = true;
}



void spider_monkey::render()
{
    CDECL_CALL(0x004B6890);

    {
        g_game_ptr->mb->render();
    }

    {

		
	        if (os_developer_options::instance->get_flag(mString{ "SHOW_OBBS" }))
        {
                    auto *the_terrain = g_world_ptr->get_the_terrain();
        the_terrain->show_obbs();
        }	
					 
		if (os_developer_options::instance->get_flag(mString{ "SHOW_DEBUG_TEXT" }))
        {
            g_game_ptr->render_district_labels();
        }
         
        if (os_developer_options::instance->get_flag(mString{ "SHOW_DEBUG_INFO" }))
        {
            g_game_ptr->show_debug_info();
        }
		
		        if (os_developer_options::instance->get_int(mString{ "FOG_DISTANCE" }))
        {
            fog_distance_per_frame_sync();
        }
		        if (os_developer_options::instance->get_flag(mString{ "SHOW_BAR_OF_SHAME" }))
        {
            g_game_ptr->render_bar_of_shame();
        }
		


		
		      if (os_developer_options::instance->get_int(mString { "MINI_MAP_ZOOM" })) {
                    debug_menu_entry a1;
                a1.set_game_flags_handler(mini_map_zoom);
            }
			
					      switch (os_developer_options::instance->get_int(mString { "FRAME_LOCK" }))
            {
            case 0:
                g_timer()->normal_motion();
                break;
            case 1:
                
                break;
             case 2: 
               g_timer()->super_speed_motion();
            default:
                break;
            }
			
		
	render_debug_overlays();

            
			
		    int TOD = os_developer_options::instance->get_int(mString { "TIME_OF_DAY" });
            if (TOD == -1) {
                TOD = g_TOD();
            }

            us_lighting_switch_time_of_day(TOD);

        if (os_developer_options::instance->get_flag(mString{ "SHOW_FPS" }))
        {
            auto v40 = (g_game_ptr->field_278 == 0 ? 0.f : (1.f / g_game_ptr->field_278));
            auto v39 = (
                ((g_game_ptr->field_278 - g_game_ptr->field_27C) - g_game_ptr->field_280) >= 0.000099999997
                ? (1.0 / (g_game_ptr->field_278 - g_game_ptr->field_27C - g_game_ptr->field_280))
                : 10000.0f);

            static float dword_1584E70 = 0;
            if (v39 > dword_1584E70)
            {
                dword_1584E70 = v39;
            }

            static float dword_14EEB94 = 500;
            if (dword_14EEB94 > v39)
            {
                dword_14EEB94 = v39;
            }

            color32 v38{ 0 };
            if (v40 >= 14.2)
            {
                if (v40 >= 29.4)
                {
                    if (v40 >= 59.900002)
                    {
                        color32 v5{ 80, 255, 80, 255 };
                        v38 = v5;
                    }
                    else
                    {
                        color32 v4{ 255, 255, 32, 255 };
                        v38 = v4;
                    }
                }
                else
                {
                    color32 v3{ 255, 128, 40, 255 };
                    v38 = v3;
                }
            }
            else
            {
                color32 v2{ 255, 33, 44, 255 };
                v38 = v2;
            }

            char a1a[200]{};
            sprintf(a1a, "%2d fps (%2d.%1d)", (int)v40, (int)v39, (int)(float)(v39 * 10.0) % 10);
            auto a5 = ((int)v38.field_0[0] >> 4) | ((int)v38.field_0[1] >> 4 << 8) | ((int)v38.field_0[2] >> 4 << 16) | 0xFF000000;
            nglListAddString(nglSysFont(), 19.0, 424.0, 1.01, a5, 0.80000001, 0.80000001, a1a);
            nglListAddString(nglSysFont(), 21.0, 426.0, 1.01, a5, 0.80000001, 0.80000001, a1a);
            nglListAddString(
                nglSysFont(),
                20.0,
                425.0,
                1.0,
                v38.field_0[0] | (v38.field_0[1] << 8) | (v38.field_0[2] << 16) | 0xFF000000,
                0.80000001,
                0.80000001,
                a1a);

            if (god_mode_cheat())
            {
                if (ultra_god_mode_cheat())
                {
                    mString v16{ "Ultra-God mode" };
                    auto v11 = v38;
                    vector2di v6{ 20, 110 };
                    render_text(v16, v6, v11, 1.0, 0.5);
                }
                else if (mega_god_mode_cheat())
                {
                    mString v17{ "Mega-God mode" };
                    auto v12 = v38;
                    vector2di v7{ 20, 110 };
                    render_text(v17, v7, v12, 1.0, 0.5);
                }
                else
                {
                    mString v18{ "God mode" };
                    auto v13 = v38;
                    vector2di v8{ 20, 110 };
                    render_text(v18, v8, v13, 1.0, 0.5);
                }
            }
            else if (ultra_god_mode_cheat())
            {
                mString v19{ "Ultra-Mortal mode" };
                auto v14 = v38;
                vector2di v9{ 20, 110 };
                render_text(v19, v9, v14, 1.0, 0.5);
            }
            else if (mega_god_mode_cheat())
            {
                mString v20{ "Mega-Mortal mode" };
                auto v15 = v38;
                vector2di v10{ 20, 110 };
                render_text(v20, v10, v15, 1.0, 0.5);
            }
        }
    }
}

void spider_monkey::on_level_load() {
    CDECL_CALL(0x004B3910);
}

void spider_monkey::on_level_unload() {
    CDECL_CALL(0x004B3B20);
}


void spider_monkey::start()
{

        CDECL_CALL(0x004B6690);

}

void spider_monkey::stop()     {
        CDECL_CALL(0x004B6700);
    }

void spider_monkey::frame_advance(Float a1)
{
    TRACE("spider_monkey::frame_advance");

    CDECL_CALL(0x004B6770, a1);
}

bool spider_monkey::is_running() {
    //sp_log("spider_monkey::is_running(): %d", spider_monkey::m_running());

    return (bool) CDECL_CALL(0x004B3B60);
}

void spider_monkey_patch()
{
    {
        REDIRECT(0x0055D761, spider_monkey::frame_advance);
    }

    REDIRECT(0x0052B4BF, spider_monkey::render);

    REDIRECT(0x0052B5DC, spider_monkey::is_running);
}





