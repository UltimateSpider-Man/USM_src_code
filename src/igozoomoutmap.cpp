#include "igozoomoutmap.h"

#include "panelquad.h"
#include "fe_menu_nav_bar.h"
#include "mission_manager.h"

#include "actor.h"
#include "ai_player_controller.h"
#include "ai_std_hero.h"

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

#include <cstdint>
#include <iterator>

// ---------------------------------------------------------------------------
// Helpers referenced by the two icon-setter ports (sub_621410 / sub_621860).
//
// These are external engine routines without recovered openusm wrappers in
// this TU. They are declared here with their decompiled signatures and called
// through their known PC addresses via CDECL_CALL so the ports compile and
// dispatch correctly. Replace with proper named wrappers once mapped.
//   sub_612760 — 0x00612760, returns the "mode==2"-ish legend predicate.
//   sub_5C5920 — 0x005C5920, mission-state refresh (m_script && story==4).
//   dword_968518 — mission_manager::s_inst (the mission_manager singleton).
//
// The former HIBYTE_RETADDR: SOLVED against the retail binary. In both
// functions the decompiler emitted a branch on HIBYTE(retaddr); that is not a
// recovery gap and not a caller-supplied parameter. Both prologues open with a
// bare `push ecx`, which creates a 4-byte local immediately below the return
// address, and both store the AL result of `call 0x005C5920` into its top byte:
//
//     00621431  call 0x612760            ; -> bl   (sub_612760)
//     0062143E  call 0x5C5920            ; -> al
//     00621449  mov  byte [esp+0x13], al ; <- the slot Hex-Rays calls retaddr
//     0062144D  call 0x5BAFF0            ; -> ebp  (is_story_mission_active)
//     ...
//     00621581  mov  al, byte [esp+0x17] ; same byte, esp scaled by the -1.0f push
//
// So the branch predicate is mission_manager::sub_5C5920() — "a script is
// loaded AND g_mission_type == 4" — which selects the story-stage-4 icon/label
// layout inside each mission branch. 0x00621860 stores it at the identical slot.
// ---------------------------------------------------------------------------
namespace {

inline bool legend_story_stage_4()
{
    auto *mm = mission_manager::s_inst;
    return mm != nullptr && mm->sub_5C5920();
}

} // namespace

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


// ---------------------------------------------------------------------------
// Port of PS2 beta zoom_map_ui::UpdateSpideyLegend.
//
// Faithful call-for-call reconstruction of the MIPS decompilation. The PS2
// routine drives every quad through two adjacent this-adjusting virtual
// thunks — vtable+204 and vtable+220 — both taking m_render_ctx (+536).
// Given the field naming (m_hidden_*) the mapping is +204 == show,
// +220 == hide; if the legend comes up inverted in-game, flip the `shown`
// booleans below (the structure is otherwise byte-exact against the decomp).
//
// Exact PS2 call sequence, in order:
//   1. +204 on m_icons[0..6]            (a1+32  .. a1+56)
//   2. +204 on m_labels[0..5]           (a1+120 .. a1+140)
//   3. mission active:
//        type 1/2/3 -> +204 on m_icon_slots[type].mission  (156/164/172)
//                      +220 on m_label_slots[type].mission (272/280/288)
//        default    -> +204 on m_icon_slots[0].mission     (148)
//                      +220 on m_label_slots[0].mission    (264)
//      free roam:
//        +204 on m_icon_slots[i].freeroam, i = 0..3 ONLY
//        (loop counter v5: 6 -> +2 per slot -> break at >= 14 == 4 iters,
//         offsets 144/152/160/168 — slots 4..6 are deliberately untouched)
//   4. +220 on m_hidden_icon            (a1+232)
//   5. +220 on m_hidden_labels[0..5]    (a1+236 .. a1+256)
//   6. free roam only:
//        +220 on m_label_slots[i].freeroam, i = 0..3 ONLY
//        (v11: 7 -> break at >= 15 == 4 iters, offsets 260/268/276/284)
//
// The 4-slot bound matches the 4 mission-type branches (default,1,2,3):
// only those slots have a free-roam counterpart to toggle.
//
// m_render_ctx has no equivalent in the PC visibility call, so it is not
// threaded through. On PC the visibility primitive is the real
// PanelAnimObject::SetShown(bool) (get_vfunc(m_vtbl, 0x64), __fastcall) —
// the same idiom used everywhere else in openusm.
//
// CRASH FIX (DIK_M -> zoom map -> UpdateSpideyLegend):
// The PC final zoom_map_ui layout diverges from the PS2 beta in the legend
// region. Evidence from the PC decomp ports below: sub_621410/sub_621860
// keep the live legend icon quads at dword offsets +100..103 (bytes
// 400..412, inside field_13C) and read label POINTERS out of bytes 128..172
// — the exact bytes the PS2 beta layout maps to m_labels[2..5] /
// m_icon_slots[0..3]. So on the final struct several of the PS2-offset
// fields hold label-text pointers / ids rather than PanelQuads; blindly
// doing *(*field + 0x64) on those was the crash.
//
// legend_set_shown therefore validates every candidate before dispatch:
//   1. pointer is readable (VirtualQuery) and 4-aligned;
//   2. its m_vtbl is a known PanelAnimObject-family vtable (PanelQuad
//      0x0087B990 / PanelAnimObject 0x00873898), OR lives in the retail
//      .rdata range AND its +0x64 slot points into .text.
// Anything that fails is skipped and logged (once per call) instead of
// being called through. Result: fields that really are quads behave exactly
// like PS2; fields where the final layout diverges are inert instead of
// fatal, and the log tells you which offsets need remapping against the
// final struct.
// ---------------------------------------------------------------------------

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#if __has_include("utility/log.h")
#include "utility/log.h"
#define LEGEND_LOG(...) sp_log(__VA_ARGS__)
#else
#define LEGEND_LOG(...) ((void)0)
#endif

namespace {

constexpr int LEGEND_FREEROAM_SLOTS = 4; // PS2 loops cover slots 0..3 only

// Known PanelAnimObject-family vtables in the retail PC binary.
constexpr uintptr_t PANELQUAD_VTBL    = 0x0087B990u; // PanelQuad::PanelQuad
constexpr uintptr_t PANELANIMOBJ_VTBL = 0x00873898u; // PanelAnimObject ctor

// USM.exe section ranges (image base 0x00400000).
constexpr uintptr_t TEXT_LO  = 0x00401000u;
constexpr uintptr_t TEXT_HI  = 0x00800000u;
constexpr uintptr_t RDATA_LO = 0x00800000u;
constexpr uintptr_t RDATA_HI = 0x00990000u;

inline bool legend_mem_readable(uintptr_t p, size_t n)
{
    MEMORY_BASIC_INFORMATION mbi{};
    if (!VirtualQuery(reinterpret_cast<void *>(p), &mbi, sizeof(mbi))) {
        return false;
    }
    if (mbi.State != MEM_COMMIT || (mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD))) {
        return false;
    }
    const uintptr_t region_end =
        reinterpret_cast<uintptr_t>(mbi.BaseAddress) + mbi.RegionSize;
    return p + n <= region_end;
}

inline bool legend_quad_valid(const PanelQuad *quad)
{
    const uintptr_t p = reinterpret_cast<uintptr_t>(quad);
    if (p < 0x00010000u || (p & 3u) || !legend_mem_readable(p, sizeof(uintptr_t))) {
        return false;
    }

    const uintptr_t vtbl = *reinterpret_cast<const uintptr_t *>(p); // m_vtbl
    if (vtbl == PANELQUAD_VTBL || vtbl == PANELANIMOBJ_VTBL) {
        return true;
    }

    // Permissive path for other PanelAnimObject subclasses (FEText, ...):
    // vtable must sit in static data and its SetShown slot must be code.
    if (vtbl < RDATA_LO || vtbl >= RDATA_HI || (vtbl & 3u)) {
        return false;
    }
    if (!legend_mem_readable(vtbl + 0x64u, sizeof(uintptr_t))) {
        return false;
    }
    const uintptr_t fn = *reinterpret_cast<const uintptr_t *>(vtbl + 0x64u);
    return fn >= TEXT_LO && fn < TEXT_HI;
}

inline void legend_set_shown(PanelQuad *quad, bool shown)
{
    if (!legend_quad_valid(quad)) {
        if (quad != nullptr) {
            LEGEND_LOG("UpdateSpideyLegend: skipping non-quad field value %p", (void *)quad);
        }
        return;
    }
    // Real PC primitive: PanelAnimObject::SetShown -> get_vfunc(m_vtbl, 0x64).
    quad->SetShown(shown);
}

} // namespace

// ---------------------------------------------------------------------------
// Port of PC zoom_map_ui::UpdateSpideyLegend (0x0062CC00) — the Spider-Man /
// Venom legend dispatcher. Single call site in the retail binary: 0x006363A9.
//
// This is the routine that actually picks which legend gets built. It branches
// on the live hero type and hands off to one of the two per-hero builders:
//
//     0062CC4D  mov eax, [0x95C770]      ; g_world_ptr
//     0062CC52  mov ecx, [eax + 0x230]   ; ->field_230[0]      (get_hero_ptr(0))
//     0062CC58  mov edx, [ecx + 0x8C]    ; ->m_player_controller
//     0062CC5E  mov eax, [edx + 0x420]   ; ->m_hero_type
//     0062CC64  cmp eax, ebx             ; ebx = 2 == VENOM
//     0062CC6B  jne 0x62CC74
//     0062CC6D  call 0x621860            ; VENOM  legend, header icon id 428
//     0062CC74  call 0x621410            ; SPIDEY legend, header icon id 427
//
// Hooking 0x0062CC74 (as this file used to) only wraps the Spider-Man call
// *inside* this function: the Venom branch is never covered and the reset /
// alpha / alignment passes around the hand-off are skipped entirely. The hook
// therefore moved to the dispatcher's own call site.
//
// Field map, in dword indices (the interpreted zoom_map_ui names do not line up
// with these, so the raw form is the faithful one — same rule as sub_621410):
//     dword  2       cached hero type
//     dwords 31..44  the 14 icon quads hidden every frame  (vtable+0x5C)
//     dword  119     selected legend row
//     dword  120     visible legend line count, seeded to 2 and grown by the
//                    per-hero builder
//     dwords 110..114 live icon slots filled by the builder
//     dwords 124..129 live label slots filled by the builder
//     dwords 89..     legend text lines, used to align the icons vertically
//
// Vtable slots: 0x5C hide(int), 0x6C set_alpha(float, int), 0xAC get_x_abs(),
// 0xB0 get_y_abs(), 0x98 set_pos_abs(x, y).
// ---------------------------------------------------------------------------
void zoom_map_ui::UpdateSpideyLegend()
{
    auto *self = reinterpret_cast<char *>(this);

    auto DW = [self](int i) -> uint32_t & {
        return *reinterpret_cast<uint32_t *>(self + 4 * i);
    };
    auto VF = [](uint32_t obj, int slot) -> uint32_t {
        return *reinterpret_cast<uint32_t *>(*reinterpret_cast<uint32_t *>(obj) + slot);
    };

    DW(119) = 0;
    DW(120) = 2;

    // Hide the 14 icon quads at dwords 31..44.
    for (int i = 31; i < 45; ++i) {
        const uint32_t q = DW(i);
        if (q != 0) {
            reinterpret_cast<void(__thiscall *)(uint32_t, int)>(VF(q, 0x5C))(q, 0);
        }
    }

    DW(124) = reinterpret_cast<uint32_t>(self + 0x1E4);
    DW(125) = reinterpret_cast<uint32_t>(self + 0x1E5);

    // g_world_ptr->get_hero_ptr(0)->m_player_controller->m_hero_type.
    // The retail routine dereferences this chain unguarded; the nulls are
    // checked here for the same reason legend_set_shown validates its quads —
    // the map can be opened before the player controller exists.
    hero_type_enum hero = UNDEFINED;
    if (auto *hero_ent = g_world_ptr->get_hero_ptr(0)) {
        if (auto *pc = static_cast<actor *>(hero_ent)->m_player_controller) {
            hero = pc->m_hero_type;
        }
    }

    DW(2) = static_cast<uint32_t>(hero);

    if (hero == VENOM) {
        this->sub_621860();   // Venom legend      — header icon id 428
    } else {
        this->sub_621410();   // Spider-Man legend — header icon id 427
    }

    // Alpha pass: every legend icon after the first is drawn at 0.75.
    for (int i = 1; i < static_cast<int>(DW(120)); ++i) {
        const uint32_t q = DW(110 + i - 1);
        if (q != 0) {
            reinterpret_cast<void(__thiscall *)(uint32_t, float, int)>(VF(q, 0x6C))(q, 0.75f, 1);
        }
    }

    // Alignment pass: icon k keeps its own X and takes text line k's Y.
    for (int k = 2; k < static_cast<int>(DW(120)); ++k) {
        const uint32_t line = DW(89 + k - 2);
        const uint32_t icon = DW(111 + k - 2);
        if (line == 0 || icon == 0) {
            continue;
        }

        const float y = reinterpret_cast<float(__thiscall *)(uint32_t)>(VF(line, 0xB0))(line);
        const float x = reinterpret_cast<float(__thiscall *)(uint32_t)>(VF(icon, 0xAC))(icon);

        reinterpret_cast<void(__thiscall *)(uint32_t, float, float)>(VF(icon, 0x98))(icon, x, y);
    }
}

// The build variant shares the retail layout and addresses, so it delegates to
// the dispatcher above instead of jumping straight into the Spider-Man builder
// (which is what the old body did, making the Venom legend unreachable).
void zoom_map_ui::UpdateSpideyLegend_build()
{
    this->UpdateSpideyLegend();
}



void zoom_map_ui::UpdateSpideyLegend_beta()
{
    // PS2: v2 = mission_manager::is_mission_active(s_inst)
    //      v3 = mission_manager::get_mission_type(s_inst)
    // s_inst can legitimately be absent (map opened outside a loaded
    // mission context in the prerelease build) — treat that as free roam.
    auto *mm = mission_manager::s_inst;
    const bool mission_active = (mm != nullptr) && mm->is_mission_active();
    const int  mission_type   = mission_active ? mm->is_story_mission_active() : 0;

    // (1)/(2) — +204 on the fixed header block: all 7 icons, all 6 labels.
    for (PanelQuad *icon : this->m_icons) {
        legend_set_shown(icon, true);
    }
    for (PanelQuad *label : this->m_labels) {
        legend_set_shown(label, true);
    }

    // (3) — mission-type switch vs free-roam icon loop.
    if (mission_active) {
        // default case (0) covers every type outside 1..3.
        const int slot = (mission_type >= 1 && mission_type <= 3) ? mission_type : 0;

        legend_set_shown(this->m_icon_slots[slot].mission, true);   // +204
        legend_set_shown(this->m_label_slots[slot].mission, false); // +220
    } else {
        // PS2 loop is bounded to slots 0..3, NOT the full 7-slot array.
        for (int i = 0; i < LEGEND_FREEROAM_SLOTS; ++i) {
            legend_set_shown(this->m_icon_slots[i].freeroam, true); // +204
        }
    }

    // (4)/(5) — +220 on the always-hidden block.
    legend_set_shown(this->m_hidden_icon, false);
    for (PanelQuad *label : this->m_hidden_labels) {
        legend_set_shown(label, false);
    }

    // (6) — free roam only: +220 on the first 4 free-roam labels.
    if (!mission_active) {
        for (int i = 0; i < LEGEND_FREEROAM_SLOTS; ++i) {
            legend_set_shown(this->m_label_slots[i].freeroam, false); // +220
        }
		
		THISCALL(0x00621410, this);
    }
}

// ---------------------------------------------------------------------------
// Port of PC zoom_map_ui::sub_621410 (0x00621410).
//
// Sets the legend icon graphics and label-text pointers for the non-mode==2
// path. The two sibling helpers (this + sub_621860) are the PC build's
// expansion of the PS2 UpdateSpideyLegend mission-type switch: each picks the
// icon texture ids (427..436) and label-slot pointers for one branch.
//
// Kept as a 1:1 transcription against the raw dword/byte offsets the
// decompiler emits — the interpreted zoom_map_ui field names do not line up
// with these indices, so the raw form is the faithful one. Constants preserved
// verbatim: -1082130432 == 0xBF800000 == -1.0f (the alpha arg to the
// vtable+276 color getter), texture ids 427..436, and panel-quad set-icon
// dispatch at vtable+328.
//
// HIBYTE(retaddr) reproduces a quirk of the original codegen (a byte read of
// an uninitialised stack slot used as a branch predicate); preserved so the
// reimplementation matches the binary. Revisit if it proves to be a genuine
// parameter once the call sites are mapped.
// ---------------------------------------------------------------------------
int zoom_map_ui::sub_621410()
{
    char *self = reinterpret_cast<char *>(this);

    int v2 = *((uint32_t *)self + 136);
    *((uint32_t *)self + 135) = *((uint32_t *)self + 137);
    *((uint32_t *)self + 134) = v2;
    // Call order is load-bearing: 0x612760, then 0x5C5920, then 0x5BAFF0.
    bool v3 = CDECL_CALL(0x00612760);
    const bool v4 = legend_story_stage_4();
    int is_story_mission_active = mission_manager::s_inst->is_story_mission_active();
    int v5 = *((uint32_t *)self + 100);
    int v6 = is_story_mission_active;
    *((uint32_t *)self + 110) = *((uint32_t *)self + 30);
    (*(void (__thiscall **)(int, int))(*(uint32_t *)v5 + 136))(v5, 427);

    if (!v3) {
        int v7_v8, v9;
        int *v7;
        *((uint32_t *)self + 111) = *((uint32_t *)self + 33);
        v7 = *((int **)self + 101);
        v7_v8 = *v7;
        v9 = (*(int (__thiscall **)(int *, int))(*v7 + 276))(v7, -1082130432);
        (*(void (__thiscall **)(uint32_t, int, int))(v7_v8 + 328))(*((uint32_t *)self + 101), 434, v9);

        int *v10 = *((int **)self + 102);
        *((uint32_t *)self + 112) = *((uint32_t *)self + 35);
        int v11 = *v10;
        int v12 = (*(int (__thiscall **)(int *, int))(*v10 + 276))(v10, -1082130432);
        (*(void (__thiscall **)(uint32_t, int, int))(v11 + 328))(*((uint32_t *)self + 102), 435, v12);

        int *v13 = *((int **)self + 103);
        *((uint32_t *)self + 113) = *((uint32_t *)self + 42);
        int v14 = *v13;
        int v15 = (*(int (__thiscall **)(int *, int))(*v13 + 276))(v13, -1082130432);
        (*(void (__thiscall **)(uint32_t, int, int))(v14 + 328))(*((uint32_t *)self + 103), 432, v15);

        *((uint32_t *)self + 114) = *((uint32_t *)self + 37);
        int *v16 = *((int **)self + 104);
        int v17 = *v16;
        int v18 = (*(int (__thiscall **)(int *, int))(*v16 + 276))(v16, -1082130432);
        (*(void (__thiscall **)(uint32_t, int, int))(v17 + 328))(*((uint32_t *)self + 104), 436, v18);

        *((uint32_t *)self + 127) = (uint32_t)(self + 489);
        int v19 = *((uint32_t *)self + 120);
        *((uint32_t *)self + 126) = (uint32_t)(self + 488);
        int result = v19 + 4;
        *((uint32_t *)self + 129) = (uint32_t)(self + 490);
        *((uint32_t *)self + 128) = (uint32_t)(self + 493);
        *((uint32_t *)self + 120) = result;
        return result;
    }

    int *v21 = *((int **)self + 101);
    if (v4) {
        int v22 = *v21;
        *((uint32_t *)self + 111) = *((uint32_t *)self + 34);
        int v23 = (*(int (__thiscall **)(int *, int))(*v21 + 276))(v21, -1082130432);
        (*(void (__thiscall **)(uint32_t, int, int))(v22 + 328))(*((uint32_t *)self + 101), 434, v23);

        *((uint32_t *)self + 112) = *((uint32_t *)self + 31);
        int *v24 = *((int **)self + 102);
        int v25 = *v24;
        int v26 = (*(int (__thiscall **)(int *, int))(*v24 + 276))(v24, -1082130432);
        (*(void (__thiscall **)(uint32_t, int, int))(v25 + 328))(*((uint32_t *)self + 102), 429, v26);

        int *v27 = *((int **)self + 103);
        *((uint32_t *)self + 113) = *((uint32_t *)self + 32);
        int v28 = *v27;
        int v29 = (*(int (__thiscall **)(int *, int))(*v27 + 276))(v27, -1082130432);
        (*(void (__thiscall **)(uint32_t, int, int))(v28 + 328))(*((uint32_t *)self + 103), 430, v29);

        int *v30 = *((int **)self + 104);
        *((uint32_t *)self + 114) = *((uint32_t *)self + 40);
        int v31 = *v30;
        int v32 = (*(int (__thiscall **)(int *, int))(*v30 + 276))(v30, -1082130432);
        (*(void (__thiscall **)(uint32_t, int, int))(v31 + 328))(*((uint32_t *)self + 104), 431, v32);

        *((uint32_t *)self + 128) = (uint32_t)(self + 487);
        int v33 = *((uint32_t *)self + 120);
        *((uint32_t *)self + 126) = (uint32_t)(self + 488);
        int result = v33 + 4;
        *((uint32_t *)self + 127) = (uint32_t)(self + 486);
        *((uint32_t *)self + 129) = (uint32_t)(self + 491);
        *((uint32_t *)self + 120) = result;
        return result;
    }

    *((uint32_t *)self + 111) = *((uint32_t *)self + 31);
    int v34 = *v21;
    int v35 = (*(int (__thiscall **)(int *, int))(*v21 + 276))(v21, -1082130432);
    (*(void (__thiscall **)(uint32_t, int, int))(v34 + 328))(*((uint32_t *)self + 101), 429, v35);

    int *v36 = *((int **)self + 102);
    *((uint32_t *)self + 112) = *((uint32_t *)self + 32);
    int v37 = *v36;
    int v38 = (*(int (__thiscall **)(int *, int))(*v36 + 276))(v36, -1082130432);
    int result = (*(int (__thiscall **)(uint32_t, int, int))(v37 + 328))(*((uint32_t *)self + 102), 430, v38);

    *((uint32_t *)self + 126) = (uint32_t)(self + 486);
    int v39 = *((uint32_t *)self + 120) + 2;
    *((uint32_t *)self + 127) = (uint32_t)(self + 487);
    *((uint32_t *)self + 120) = v39;

    switch (v6) {
    case 3:
    case 6: {
        int *v50 = *((int **)self + 103);
        *((uint32_t *)self + 113) = *((uint32_t *)self + 36);
        int v51 = *v50;
        int v52 = (*(int (__thiscall **)(int *, int))(*v50 + 276))(v50, -1082130432);
        (*(void (__thiscall **)(uint32_t, int, int))(v51 + 328))(*((uint32_t *)self + 103), 435, v52);
        char *v43 = self + 489;
        result = *((uint32_t *)self + 120) + 1;
        *((uint32_t *)self + 128) = (uint32_t)v43;
        *((uint32_t *)self + 120) = result;
        return result;
    }
    case 7: {
        int *v40 = *((int **)self + 103);
        *((uint32_t *)self + 113) = *((uint32_t *)self + 38);
        int v41 = *v40;
        int v42 = (*(int (__thiscall **)(int *, int))(*v40 + 276))(v40, -1082130432);
        (*(void (__thiscall **)(uint32_t, int, int))(v41 + 328))(*((uint32_t *)self + 103), 436, v42);
        char *v43 = self + 490;
        result = *((uint32_t *)self + 120) + 1;
        *((uint32_t *)self + 128) = (uint32_t)v43;
        *((uint32_t *)self + 120) = result;
        return result;
    }
    case 5:
    case 2: {
        *((uint32_t *)self + 113) = *((uint32_t *)self + 43);
        int *v47 = *((int **)self + 103);
        int v48 = *v47;
        int v49 = (*(int (__thiscall **)(int *, int))(*v47 + 276))(v47, -1082130432);
        (*(void (__thiscall **)(uint32_t, int, int))(v48 + 328))(*((uint32_t *)self + 103), 432, v49);
        result = *((uint32_t *)self + 120) + 1;
        *((uint32_t *)self + 128) = (uint32_t)(self + 493);
        *((uint32_t *)self + 120) = result;
        break;
    }
    case 1: {
        int *v44 = *((int **)self + 103);
        *((uint32_t *)self + 113) = *((uint32_t *)self + 39);
        int v45 = *v44;
        int v46 = (*(int (__thiscall **)(int *, int))(*v44 + 276))(v44, -1082130432);
        (*(void (__thiscall **)(uint32_t, int, int))(v45 + 328))(*((uint32_t *)self + 103), 433, v46);
        *((uint32_t *)self + 128) = (uint32_t)(self + 492);
        result = *((uint32_t *)self + 120) + 1;
        *((uint32_t *)self + 120) = result;
        break;
    }
    }
    return result;
}

// ---------------------------------------------------------------------------
// Port of PC zoom_map_ui::sub_621860 (0x00621860).
//
// Sibling of sub_621410: sets icon graphics + label pointers for the mode==2
// branch. Same 1:1 transcription rules apply (raw offsets, -1.0f alpha,
// texture ids, vtable+276 / vtable+328 dispatch). header-icon id here is 428.
// ---------------------------------------------------------------------------
int zoom_map_ui::sub_621860()
{
    int **self = reinterpret_cast<int **>(this);

    int *v2 = *(self + 138);
    *(self + 135) = *(self + 139);
    *(self + 134) = v2;
    // Call order is load-bearing: 0x612760, then 0x5C5920, then 0x5BAFF0.
    bool v3 = CDECL_CALL(0x00612760);
    const bool v4 = legend_story_stage_4();
    int is_story_mission_active = mission_manager::s_inst->is_story_mission_active();
    int v5 = (int)*(self + 100);
    int v6 = is_story_mission_active;
    *(self + 110) = *(self + 41);
    (*(void (__thiscall **)(int, int))(*(uint32_t *)v5 + 136))(v5, 428);

    int result;

    if (v3) {
        int *v11 = *(self + 101);
        if (v4) {
            *(self + 111) = *(self + 34);
            int v12 = *v11;
            int v13 = (*(int (__thiscall **)(int *, int))(*v11 + 276))(v11, -1082130432);
            (*(void (__thiscall **)(uint32_t, int, int))(v12 + 328))((uint32_t)*(self + 101), 434, v13);

            *(self + 112) = *(self + 32);
            int *v14 = *(self + 102);
            int v15 = *v14;
            int v16 = (*(int (__thiscall **)(int *, int))(*v14 + 276))(v14, -1082130432);
            (*(void (__thiscall **)(uint32_t, int, int))(v15 + 328))((uint32_t)*(self + 102), 430, v16);

            int *v17 = *(self + 103);
            *(self + 113) = *(self + 40);
            int v18 = *v17;
            int v19 = (*(int (__thiscall **)(int *, int))(*v17 + 276))(v17, -1082130432);
            (*(void (__thiscall **)(uint32_t, int, int))(v18 + 328))((uint32_t)*(self + 103), 431, v19);

            *(self + 126) = (int *)(self + 122);
            result = (int)*(self + 120) + 3;
            *(self + 127) = (int *)((char *)self + 487);
            *(self + 128) = (int *)((char *)self + 491);
            *(self + 120) = (int *)result;
        } else {
            *(self + 111) = *(self + 32);
            int v20 = *v11;
            int v21 = (*(int (__thiscall **)(int *, int))(*v11 + 276))(v11, -1082130432);
            result = (*(int (__thiscall **)(uint32_t, int, int))(v20 + 328))((uint32_t)*(self + 101), 430, v21);

            *(self + 126) = (int *)((char *)self + 487);
            *(self + 120) = (int *)((char *)*(self + 120) + 1);

            if (v6 == 3 || v6 == 6) {
                int *v22 = *(self + 102);
                *(self + 112) = *(self + 36);
                int v23 = *v22;
                int v24 = (*(int (__thiscall **)(int *, int))(*v22 + 276))(v22, -1082130432);
                (*(void (__thiscall **)(uint32_t, int, int))(v23 + 328))((uint32_t)*(self + 102), 435, v24);

                *(self + 127) = (int *)((char *)self + 489);
                *(self + 120) = (int *)((char *)*(self + 120) + 1);
                return (int)self + 489;
            }
        }
    } else {
        *(self + 111) = *(self + 35);
        int *v7 = *(self + 101);
        int v8 = *v7;
        int v9 = (*(int (__thiscall **)(int *, int))(*v7 + 276))(v7, -1082130432);
        (*(void (__thiscall **)(uint32_t, int, int))(v8 + 328))((uint32_t)*(self + 101), 435, v9);

        result = (int)*(self + 120) + 1;
        *(self + 126) = (int *)((char *)self + 489);
        *(self + 120) = (int *)result;
    }
    return result;
}

static constexpr uintptr_t ZOOM_MAP_UI_INIT_ORIG = 0x00644330u; // <-- set me

void zoom_map_ui::Init()       { THISCALL(ZOOM_MAP_UI_INIT_ORIG, this); }
void zoom_map_ui::Init_beta() 
{     



    auto *self = reinterpret_cast<char *>(this);
	
	
    auto AT = [self](int dw) -> uint32_t & {
        return *reinterpret_cast<uint32_t *>(self + 4 * dw);
    };

    // Init once. a1[3] (dword 3) is the panel-file guard.
    if (AT(3) != 0) {
        return;
    }

    PanelFile *pf = PanelFile::UnmashPanelFile("zoom_map", static_cast<panel_layer>(7));
    AT(3) = reinterpret_cast<uint32_t>(pf);

    // a1[4] = field_28.m_data[0], a1[5] = field_28.m_data[1] (the two anim pages).
    AT(4) = reinterpret_cast<uint32_t>(pf->field_28.m_data[0]);
    AT(5) = reinterpret_cast<uint32_t>(pf->field_28.m_data[1]);

    // -- Named quads: a1[6..57] (interface frames, legend backs, alt widgets,
    //    0500 icon set, 2000 icon set). GetPQ warns + returns the default PQ on
    //    a miss, exactly like the beta inline loop. --
    static const char *const kQuadNames[] = {
        "zm_interface_frame", "zm_interface_frame2",                     // 6, 7
        "zm_legend_back", "zm_legend_back2", "zm_legend_back3",          // 8..10
        "zm_legend_back_gradient", "zm_legend_back_darken",             // 11, 12
        "zm_legend_back_detail", "zm_legend_text_hilite",              // 13, 14
        "zm_alt_arrow_down", "zm_alt_arrow_up",                         // 15, 16
        "zm_alt_click_level_0500_ref", "zm_alt_click_level_1000_ref",   // 17, 18
        "zm_alt_click_level_1500_ref", "zm_alt_click_level_2000_ref",   // 19, 20
        "zm_alt_clicks", "zm_alt_compass_arrow", "zm_alt_compass_base", // 21..23
        "zm_alt_frame", "zm_alt_frame2", "zm_alt_frame3",              // 24..26
        "zm_alt_gradient", "zm_alt_indicator", "zm_alt_indicator_splash", // 27..29
        "zm_icon_0500_hero", "zm_icon_0500_citizen", "zm_icon_0500_enemy",     // 30..32
        "zm_icon_0500_actionable", "zm_icon_0500_TAM", "zm_icon_0500_minigame", // 33..35
        "zm_icon_0500_mission_start", "zm_icon_0500_mission_waypoint",  // 36, 37
        "zm_icon_0500_race_start", "zm_icon_0500_race_waypoint",        // 38, 39
        "zm_icon_0500_taxi_start", "zm_icon_0500_taxi_waypoint",        // 40, 41
        "zm_icon_0500_storm_start", "zm_icon_0500_storm_waypoint",      // 42, 43
        "zm_icon_2000_hero", "zm_icon_2000_citizen", "zm_icon_2000_enemy",     // 44..46
        "zm_icon_2000_actionable", "zm_icon_2000_TAM", "zm_icon_2000_minigame", // 47..49
        "zm_icon_2000_mission_start", "zm_icon_2000_mission_waypoint",  // 50, 51
        "zm_icon_2000_race_start", "zm_icon_2000_race_waypoint",        // 52, 53
        "zm_icon_2000_taxi_start", "zm_icon_2000_taxi_waypoint",        // 54, 55
        "zm_icon_2000_storm_start", "zm_icon_2000_storm_waypoint",      // 56, 57
    };
    for (int i = 0; i < static_cast<int>(std::size(kQuadNames)); ++i) {
        AT(6 + i) = reinterpret_cast<uint32_t>(pf->GetPQ(kQuadNames[i]));
    }

    // -- Legend text lines: a1[58..72]. --
    static const char *const kTextNames[] = {
        "zm_legend_text_line_01", "zm_legend_text_line_02", "zm_legend_text_line_03",
        "zm_legend_text_line_04", "zm_legend_text_line_05", "zm_legend_text_line_06",
        "zm_legend_text_line_07", "zm_legend_text_line_08", "zm_legend_text_line_08b",
        "zm_legend_text_line_09", "zm_legend_text_line_09b", "zm_legend_text_line_10",
        "zm_legend_text_line_10b", "zm_legend_text_line_11", "zm_legend_text_line_11b",
    };
    for (int i = 0; i < static_cast<int>(std::size(kTextNames)); ++i) {
        AT(58 + i) = reinterpret_cast<uint32_t>(pf->GetTextPointer(kTextNames[i]));
    }

    // -- Hero icons: two dynamically-created quads that mirror the hero markers,
    //    shown on top. a1[117] copies the 0500 hero, a1[118] the 2000 hero. --
    {
        auto *big = new PanelQuad{};
        AT(117) = reinterpret_cast<uint32_t>(big);
        big->CopyFrom(reinterpret_cast<PanelQuad *>(AT(30)));
        big->SetShown(true);

        auto *small = new PanelQuad{};
        AT(118) = reinterpret_cast<uint32_t>(small);
        small->CopyFrom(reinterpret_cast<PanelQuad *>(AT(44)));
        small->SetShown(true);
    }

    // -- Zoom-level indicator text (a1[73]): solid colour 0xFFE6D03F, shown. --
    {
        auto *indicator = pf->GetTextPointer("zm_alt_indicator_text");
        AT(73) = reinterpret_cast<uint32_t>(indicator);
        indicator->SetNoFlash(color32(0xFFE6D03Fu));
        indicator->SetShown(true);
    }

    // -- Legend line string ids. The beta set a1[58..64] via a single-arg call
    //    and a1[65..72] via a 2-arg call; on PC every legend line goes through
    //    the vtable+276 (compute justification arg from -1.0f) / vtable+328
    //    (set text id) pair, which is exactly what sub_621410 does here. Ids are
    //    the beta values 412..423 (note 08b/09b reuse 08/09's id, 11b reuses 11). --
    static const int kLineIds[] = {
        412, 413, 414, 415, 416, 417, 418, // line 01..07  -> a1[58..64]
        419, 419,                          // line 08, 08b -> a1[65..66]
        420, 420,                          // line 09, 09b -> a1[67..68]
        421, 422,                          // line 10, 10b -> a1[69..70]
        423, 423,                          // line 11, 11b -> a1[71..72]
    };
    for (int i = 0; i < static_cast<int>(std::size(kLineIds)); ++i) {
        uint32_t txt = AT(58 + i);
        if (!txt) {
            continue;
        }
        int extra = (*(int(__thiscall **)(uint32_t, int))(*reinterpret_cast<uint32_t *>(txt) + 276))(
            txt, 0xBF800000 /* -1.0f */);
        (*(void(__thiscall **)(uint32_t, int, int))(*reinterpret_cast<uint32_t *>(txt) + 328))(
            txt, kLineIds[i], extra);
    }

    // -- Nav bar (a1[1]): allocate, wire text + four backgrounds, load, then let
    //    SetUpNormalNavBar install the button prompts. --
    {

    }

    this->SetUpNormalNavBar();
}

	
void zoom_map_ui::Init_build() { THISCALL(ZOOM_MAP_UI_INIT_ORIG, this); }



void zoom_map_ui::Update(Float a2)
{
    debug_menu::hide();
	
    THISCALL(0x00632020, this);
}

void zoom_map_ui::SetUpNormalNavBar()
{
    // TODO: THISCALL(0x00XXXXXX, this); once the PC address is recovered.
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
        REDIRECT(0x00648A81, address);
    }
    {
        // 0x006363A9 is the only call site of the dispatcher 0x0062CC00.
        // The old hook at 0x0062CC74 was the internal `call sub_621410`, i.e.
        // the Spider-Man branch only.
        FUNC_ADDRESS(address, &zoom_map_ui::UpdateSpideyLegend);
        REDIRECT(0x006363A9, address);
    }

	{
        FUNC_ADDRESS(address, &zoom_map_ui::sub_621410);
        SET_JUMP(0x00621410, address);
    }
	{
        FUNC_ADDRESS(address, &zoom_map_ui::sub_621860);
        SET_JUMP(0x00621860, address);
    }
	
}


void IGOZoomOutMap_beta_patch() {
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
        FUNC_ADDRESS(address, &zoom_map_ui::Init_beta);
        REDIRECT(0x00648A81, address);
    }
    {
	FUNC_ADDRESS(address, &zoom_map_ui::UpdateSpideyLegend_beta);
	//REDIRECT(0x062CC74, address);
	}

	{
        FUNC_ADDRESS(address, &zoom_map_ui::sub_621410);
        SET_JUMP(0x00621410, address);
    }
	{
        FUNC_ADDRESS(address, &zoom_map_ui::sub_621860);
        SET_JUMP(0x00621860, address);
    }
	
}


void IGOZoomOutMap_build_patch() {
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
        FUNC_ADDRESS(address, &zoom_map_ui::Init_build);
       REDIRECT(0x00648A81, address);
    }
    {
        FUNC_ADDRESS(address, &zoom_map_ui::UpdateSpideyLegend_build);
        REDIRECT(0x006363A9, address);
    }

	{
        FUNC_ADDRESS(address, &zoom_map_ui::sub_621410);
        SET_JUMP(0x00621410, address);
    }
	{
        FUNC_ADDRESS(address, &zoom_map_ui::sub_621860);
        SET_JUMP(0x00621860, address);
    }
	
}