#include "fe_mini_map_dot.h"
 
#include "color32.h"
#include "common.h"
#include "func_wrapper.h"
#include "igofrontend.h"
#include "ngl.h"
#include "panelquad.h"
#include "variable.h"
#include "vtbl.h"
#include "fe_mini_map_widget.h"
 
VALIDATE_SIZE(fe_mini_map_dot, 0x2C);

// ---------------------------------------------------------------------------
// Minimap dot colours — PS2 beta palette (fe_mini_map_dot ctor, 0x0063AB90 PS2).
// ---------------------------------------------------------------------------
// The PS2 beta ctor selects a body colour from a 17-entry switch keyed by the
// dot type (field_20, cases 0..0x10) and applies it to the body PanelQuad via
// vtable slot +0x10C (SetColor). Types outside 0..0x10 fall to the default.
//
// color32(uint32_t) reads 0xAABBGGRR (red = low byte; see color32.h), so the
// raw PS2 colour words below decode as the #RRGGBB shown in the trailing
// comment. These are the genuine PS2 beta values, which differ from the PC
// retail palette (notably: distinct colours for types 11 and 12, and a
// 0xFFDCDCDC default rather than the PC 0xFFC8C8C8).
//
// Ring-bearing types on PS2 are 0x0F and 0x10 (15/16), matching PC.
static const color32 s_minimap_dot_colors[17] = {
    color32{0xFF82B4BEu}, //  0  #BEB482  POI / hero pin
    color32{0xFFA03030u}, //  1  #3030A0
    color32{0xFFA03030u}, //  2  #3030A0
    color32{0xFFC6D9F1u}, //  3  #F1D9C6
    color32{0xFF3F5877u}, //  4  #77583F
    color32{0xFFE589EBu}, //  5  #EB89E5
    color32{0xFF6D325Fu}, //  6  #5F326D
    color32{0xFFD9ED6Cu}, //  7  #6CEDD9
    color32{0xFFEFD841u}, //  8  #41D8EF
    color32{0xFF60571Au}, //  9  #1A5760
    color32{0xFF74C5C4u}, // 10  #C4C574
    color32{0xFFEC9A6Eu}, // 11  #6E9AEC
    color32{0xFF8F4B38u}, // 12  #384B8F
    color32{0xFF6ED76Eu}, // 13  #6ED76E
    color32{0xFF286228u}, // 14  #286228
    color32{0xFFD53A38u}, // 15  #383AD5  ring
    color32{0xFFC8C8C8u}, // 16  #C8C8C8  ring
};

// PS2 beta default colour for unknown dot types (v14 = 0xFFDCDCDC).
static const color32 s_minimap_dot_default{0xFFDCDCDCu};

static Var<IGOFrontEnd *> g_igo_frontend{0x00937B18};



fe_mini_map_dot::fe_mini_map_dot(mini_map_dot_type a2, vector3d a3) {
    this->field_24 = false;
    this->field_25 = false;
    this->field_26 = false;
    this->field_14 = vector3d{};

    this->field_20 = a2;

    // PS2 beta per-type tint: table lookup, default for out-of-range types.
    const int dot_type = a2;
    color32 color = (dot_type >= 0 && dot_type <= 0x10)
                        ? s_minimap_dot_colors[dot_type]
                        : s_minimap_dot_default;

    auto *widget = g_igo_frontend()->field_4;

    // Primary icon quad: hero gets map_icon_spidey, everyone else map_icon_others.
    auto *quad = new PanelQuad{};
    this->field_0 = quad;
    quad->CopyFrom(this->field_20 != 0 ? widget->map_icon_others
                                       : widget->map_icon_spidey);

    // Highlight ring, only on the two ring-bearing types.
    if (this->field_20 == 15 || this->field_20 == 16) {
        auto *ring = new PanelQuad{};
        this->field_4 = ring;
        ring->CopyFrom(widget->minimap_ring);
        this->highlight_circle_count_down = 12;
    } else {
        this->highlight_circle_count_down = 0;
    }

    this->field_0->SetColor(color);
    this->field_0->TurnOn(true);

    this->field_8 = nullptr;
    this->field_C = nullptr;
    this->field_10 = nullptr;

    // PS2 beta gate: the stock ctor only builds the line geometry when the
    // supplied position's y component is below -1.0 (decomp: v12 = a3.y;
    // if (v12 < -1.0 || ...) InitLines()). The second half of that OR tests an
    // uninitialised FPU register on PS2 and never independently fires, so the
    // y < -1.0 test is the real condition.
    if (a3.y < -1.0f) {
        this->InitLines();
    }
    widget->AddMapPOIWidget(this);

    this->field_24 = true;
    this->field_25 = true;
    this->field_26 = false;
}


void fe_mini_map_dot::fe_mini_map_dot_hook(mini_map_dot_type a2, vector3d a3) {
    this->field_24 = false;
    this->field_25 = false;
    this->field_26 = false;
    this->field_14 = vector3d{};

    this->field_20 = a2;

    // PS2 beta per-type tint: table lookup, default for out-of-range types.
    const int dot_type = a2;
    color32 color = (dot_type >= 0 && dot_type <= 0x10)
                        ? s_minimap_dot_colors[dot_type]
                        : s_minimap_dot_default;

    auto *widget = g_igo_frontend()->field_4;

    // Primary icon quad: hero gets map_icon_spidey, everyone else map_icon_others.
    auto *quad = new PanelQuad{};
    this->field_0 = quad;
    quad->CopyFrom(this->field_20 != 0 ? widget->map_icon_others
                                       : widget->map_icon_spidey);

    // Highlight ring, only on the two ring-bearing types.
    if (this->field_20 == 15 || this->field_20 == 16) {
        auto *ring = new PanelQuad{};
        this->field_4 = ring;
        ring->CopyFrom(widget->minimap_ring);
        this->highlight_circle_count_down = 12;
    } else {
        this->highlight_circle_count_down = 0;
    }

    this->field_0->SetColor(color);
    this->field_0->TurnOn(true);

    this->field_8 = nullptr;
    this->field_C = nullptr;
    this->field_10 = nullptr;

    this->InitLines();
    widget->AddMapPOIWidget(this);

    this->field_24 = true;
    this->field_25 = true;
    this->field_26 = false;
	
	// NOTE (PS2 beta): the reimplemented body above already fully constructs the
	// dot; the call below re-enters the *PC* stock ctor (0x0063AB90), which
	// rebuilds the quads and leaks the first set. For a PS2-beta target this PC
	// address is also wrong. Use the canonical fe_mini_map_dot ctor instead, or
	// drop this thunk and keep only the reimplemented body.
		    void(__fastcall * func)(void*, void*, mini_map_dot_type, vector3d) = bit_cast<decltype(func)>(0x0063AB90);

        func(this, nullptr, a2, a3);
}

void fe_mini_map_dot::Draw()
{
    if (this->field_24 && this->field_25)
    {
        this->field_0->Draw();

        if (this->field_8 != nullptr) {
            nglListAddQuad(this->field_8);
        }

        if (this->field_C != nullptr) {
            nglListAddQuad(this->field_C);
        }

        if (this->field_10 != nullptr) {
            nglListAddQuad(this->field_10);
        }

        if (this->highlight_circle_count_down != 0) {
            this->field_4->Draw();
        }
    }
}




void fe_mini_map_dot::SetZvalueAbs(Float a2)
{

	
         void(__fastcall * func)(void*, void*, Float) = bit_cast<decltype(func)>(0x0060C4E0);

        func(this, nullptr, a2);
}

void fe_mini_map_dot::InitLines()
{

		    void(__fastcall * func)(void*, void*) = bit_cast<decltype(func)>(0x0060C4E0);

        func(this, nullptr);
}

void fe_mini_map_dot_patch()
{
    // Redirect the two stock call sites that construct minimap dots to our
    // thunk. Both are clean 5-byte `E8 rel32` calls, so REDIRECT's 5-byte
    // overwrite is byte-exact, and the thunk's calling convention / stack
    // cleanup (`ret 0x10`) matches the stock ctor precisely.
    //   0x005C9D84: constructs a fixed-type dot (push 3)  — e.g. waypoint
    //   0x00641215: constructs POI/player dots inside fe_mini_map_widget::UpdatePOIs
	FUNC_ADDRESS(address, &fe_mini_map_dot::fe_mini_map_dot_hook);
    REDIRECT(0x005C9D84, &address);
    REDIRECT(0x00641215, &address);
}