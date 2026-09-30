#include "xbpack.h"

#ifdef OPENUSM_XBPACK_V10

#include "func_wrapper.h"
#include "log.h"
#include "string_hash.h"
#include "utility.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <windows.h>

// Xbox v10 combat support for enemy AI (Sable mercenaries in v03_throw_down).
//
// 1. Launched-layer interrupt scope
// ---------------------------------
// PC launch_layer_state::frame_advance (0x006AF230) moves ai_state_machine
// +0x34 (the "run default transitions" flag) when it launches a layer:
//   non-blocking layer: the layer's flag is cleared      (0x006AF344)
//   blocking layer:     the layer takes the parent's flag
//                       and the parent's is cleared      (0x006AF362)
// xbpack_v10_ped_patch.h used to NOP both hand-offs for every machine in the
// game.  Its evidence is pedestrian-specific: the v10 pedestrian layer graphs
// (ped_quad_path_layer, ped_reaction_layer, ped_lane_layer) carry no
// ped_default_trans, so with the PC hand-off a walking ped never evaluated
// its hit/threat transitions.  The same NOPs also changed every other AI.
// For a universal soldier the parent machine then keeps evaluating
// universal_soldier_trans while a blocking combat or locomotion layer runs,
// so a parent transition can end that layer before it finishes: locomotion
// restarts and the shot that follows the aim never happens.
//
// The bridge below replaces the two hand-offs with calls that pick the rule
// per launch.  openusm.ini (next to the game executable):
//   [XbpackV10]
//   LayerInterrupts=1   ; 0 = stock PC rule for every graph
//                       ; 1 = beta rule for pedestrian graphs only (default)
//                       ; 2 = beta rule for every graph (previous behaviour)
//
// 2. Combat trace (CombatTrace=1, default on)
// -------------------------------------------
// A few log lines per mission for the chain that is missing in the v10
// videos: gun::fire -> gun hit handler -> bullet damage on the target, plus
// one line per distinct parent graph showing which layer rule it received.
// Nothing is logged per frame.
namespace
{
constexpr uintptr_t PED_DEFAULT_TRANS_VTABLE = 0x00875B50;
constexpr uintptr_t UNIVERSAL_SOLDIER_TRANS_VTABLE = 0x00877AA8;

constexpr uintptr_t LAYER_NONBLOCKING_SITE = 0x006AF344;
constexpr uintptr_t LAYER_BLOCKING_SITE = 0x006AF362;

// mov byte [eax+34h],0 ; mov eax,1
constexpr uint8_t PC_NONBLOCKING[] = {
    0xC6, 0x40, 0x34, 0x00, 0xB8, 0x01, 0x00, 0x00, 0x00,
};
constexpr uint8_t BETA_NONBLOCKING[] = {
    0x90, 0x90, 0x90, 0x90, 0xB8, 0x01, 0x00, 0x00, 0x00,
};

// mov edx,[esi+0Ch] ; mov cl,[edx+34h] ; mov [eax+34h],cl ;
// mov edx,[esi+0Ch] ; mov byte [edx+34h],0
constexpr uint8_t PC_BLOCKING[] = {
    0x8B, 0x56, 0x0C, 0x8A, 0x4A, 0x34, 0x88, 0x48,
    0x34, 0x8B, 0x56, 0x0C, 0xC6, 0x42, 0x34, 0x00,
};
constexpr uint8_t BETA_BLOCKING[] = {
    0xEB, 0x0E, 0x90, 0x8A, 0x4A, 0x34, 0x88, 0x48,
    0x34, 0x8B, 0x56, 0x0C, 0xC6, 0x42, 0x34, 0x00,
};

static_assert(sizeof(PC_NONBLOCKING) == sizeof(BETA_NONBLOCKING));
static_assert(sizeof(PC_BLOCKING) == sizeof(BETA_BLOCKING));

constexpr uintptr_t GUN_FIRE = 0x005007B0;       // thiscall, 2 args, ret 8
constexpr uintptr_t GUN_FIRE_CALLS[] = {
    0x0050119F, 0x005011E3, 0x0050122C, 0x005012C1, 0x0050132F,
};
constexpr uintptr_t GUN_HIT = 0x00500140;        // thiscall, 5 args, ret 14h
constexpr uintptr_t GUN_HIT_SLOT = 0x00886D10 + 0x2F8;
constexpr uintptr_t APPLY_DAMAGE = 0x004FDBD0;   // thiscall, 13 args, ret 34h
constexpr uintptr_t BULLET_DAMAGE_CALL = 0x004FFADE;
constexpr uintptr_t GOD_MODE_CHEAT = 0x0095A6A8;

constexpr size_t GUN_FX_FIRST = 0x1AC;
constexpr size_t GUN_FX_STRIDE = 0x40;

int layer_mode = 1;
bool trace_enabled = true;
bool settings_read = false;

void read_settings()
{
    if (settings_read) {
        return;
    }
    settings_read = true;

    char path[MAX_PATH] {};
    const auto length = GetModuleFileNameA(nullptr, path, MAX_PATH);
    std::string ini(path, length < MAX_PATH ? length : 0);
    const auto slash = ini.find_last_of("\\/");
    ini.resize(slash == std::string::npos ? 0 : slash + 1);
    ini += "openusm.ini";

    layer_mode = static_cast<int>(
        GetPrivateProfileIntA("XbpackV10", "LayerInterrupts", 1, ini.c_str()));
    if (layer_mode < 0 || layer_mode > 2) {
        layer_mode = 1;
    }
    trace_enabled =
        GetPrivateProfileIntA("XbpackV10", "CombatTrace", 1, ini.c_str()) != 0;
}

uint32_t read_u32(const void *base, size_t offset)
{
    uint32_t value = 0;
    std::memcpy(&value, static_cast<const uint8_t *>(base) + offset, sizeof(value));
    return value;
}

float read_float(const void *base, size_t offset)
{
    float value = 0.0f;
    std::memcpy(&value, static_cast<const uint8_t *>(base) + offset, sizeof(value));
    return value;
}

// state_graph +0x20 is the mVector of default-transition states (size +0x24,
// data +0x28); the v10 converter gives them their PC vtables.
bool graph_has_default_state(const uint8_t *graph, uintptr_t vtable)
{
    if (graph == nullptr) {
        return false;
    }

    const auto count = *reinterpret_cast<const int *>(graph + 0x24);
    const auto *const *list = *reinterpret_cast<void *const *const *>(graph + 0x28);
    if (list == nullptr || count <= 0 || count > 256) {
        return false;
    }

    for (int i = 0; i < count; ++i) {
        if (list[i] != nullptr &&
            *static_cast<const uint32_t *>(list[i]) == vtable) {
            return true;
        }
    }

    return false;
}

const uint8_t *parent_graph(const uint8_t *launch_state)
{
    const auto *parent = *reinterpret_cast<const uint8_t *const *>(launch_state + 0x0C);
    return parent != nullptr
        ? *reinterpret_cast<const uint8_t *const *>(parent + 0x0C)
        : nullptr;
}

bool use_beta_rule(const uint8_t *launch_state)
{
    if (layer_mode == 0) {
        return false;
    }
    if (layer_mode == 2) {
        return true;
    }
    return graph_has_default_state(parent_graph(launch_state),
                                   PED_DEFAULT_TRANS_VTABLE);
}

void note_launch(const uint8_t *launch_state, bool blocking, bool beta)
{
    if (!trace_enabled) {
        return;
    }

    static uint32_t seen[64] {};
    static unsigned seen_count = 0;

    const auto *graph = parent_graph(launch_state);
    const auto name = graph != nullptr ? read_u32(graph, 0) : 0u;
    const auto key = name ^ (blocking ? 0x80000000u : 0u);
    for (unsigned i = 0; i < seen_count; ++i) {
        if (seen[i] == key) {
            return;
        }
    }
    if (seen_count < sizeof(seen) / sizeof(seen[0])) {
        seen[seen_count++] = key;
    }

    const char *kind = graph_has_default_state(graph, PED_DEFAULT_TRANS_VTABLE)
        ? "pedestrian"
        : graph_has_default_state(graph, UNIVERSAL_SOLDIER_TRANS_VTABLE)
            ? "universal soldier"
            : "other";
    sp_log("[xbpack] v10 layer launch: parent graph %s (0x%08X, %s), %s layer, "
           "%s interrupt rule",
           string_hash {static_cast<int>(name)}.to_string(),
           static_cast<unsigned>(name), kind,
           blocking ? "blocking" : "non-blocking",
           beta ? "beta" : "stock PC");
}

bool matches(const uint8_t *code, const uint8_t *expected, size_t size)
{
    return std::memcmp(code, expected, size) == 0;
}

void make_call(uint8_t *out, uintptr_t address, const void *target, size_t size)
{
    out[0] = 0xE8;
    const auto rel = static_cast<int32_t>(
        reinterpret_cast<uintptr_t>(target) - (address + 5u));
    std::memcpy(out + 1, &rel, sizeof(rel));
    std::memset(out + 5, 0x90, size - 5u);
}

void write_code(uintptr_t address, const uint8_t *bytes, size_t size)
{
    auto *code = reinterpret_cast<uint8_t *>(address);
    DWORD old_protect = 0;
    const bool unprotected =
        VirtualProtect(code, size, PAGE_EXECUTE_READWRITE, &old_protect) != 0;
    std::memcpy(code, bytes, size);
    if (unprotected) {
        VirtualProtect(code, size, old_protect, &old_protect);
    }
    FlushInstructionCache(GetCurrentProcess(), code, size);
}

// ---------------------------------------------------------------- trace

unsigned fires = 0;
unsigned hits = 0;
unsigned damage_calls = 0;
unsigned damage_applied = 0;
unsigned damage_blocked = 0;

void summary()
{
    const auto events = fires + hits + damage_calls;
    if (events != 0 && events % 256 == 0) {
        sp_log("[xbpack] v10 combat: fires %u, gun hits %u, bullet damage calls %u "
               "(applied %u, blocked %u)",
               fires, hits, damage_calls, damage_applied, damage_blocked);
    }
}

// gun::fire(target, mode): gun +0x108 owner, +0x10C flags (bit 9 tracer),
// +0x118 damage, +0x158 shots per trigger; cached effects at +0x1AC..+0x2AC
// (+0x1EC muzzle, +0x22C/+0x26C impacts), each with its entity key at +8
// and fx_cache at +0x30.
int __fastcall gun_fire_trace(void *gun, void *, int target, int mode)
{
    ++fires;
    if (trace_enabled && fires <= 24) {
        uint32_t fx_hash[5] {};
        uint32_t fx_cache[5] {};
        for (int i = 0; i < 5; ++i) {
            const auto base = GUN_FX_FIRST + GUN_FX_STRIDE * static_cast<size_t>(i);
            fx_hash[i] = read_u32(gun, base + 0x08);
            fx_cache[i] = read_u32(gun, base + 0x30);
        }

        const auto flags = read_u32(gun, 0x10C);
        sp_log("[xbpack] v10 combat: fire #%u gun=%p owner=0x%08X target=0x%08X mode=%d "
               "flags=0x%08X tracer=%u shots=%d damage=%.2f",
               fires, gun, read_u32(gun, 0x108), static_cast<unsigned>(target),
               mode, flags, (flags >> 9) & 1u,
               static_cast<int>(read_u32(gun, 0x158)), read_float(gun, 0x118));
        sp_log("[xbpack] v10 combat:   fx key/cache c0 %08X/%08X c1 %08X/%08X "
               "c2 %08X/%08X c3 %08X/%08X c4 %08X/%08X",
               fx_hash[0], fx_cache[0], fx_hash[1], fx_cache[1], fx_hash[2],
               fx_cache[2], fx_hash[3], fx_cache[3], fx_hash[4], fx_cache[4]);
    }

    summary();
    return THISCALL(GUN_FIRE, gun, target, mode);
}

// Gun vtable +0x2F8: bullet hit handler (a2 is the entity that was hit).
int __fastcall gun_hit_trace(void *gun, void *,
                             uint32_t a1, uint32_t a2, uint32_t a3,
                             uint32_t a4, uint32_t a5)
{
    ++hits;
    if (trace_enabled && hits <= 24) {
        const auto *entity = reinterpret_cast<const void *>(a2);
        sp_log("[xbpack] v10 combat: gun hit #%u gun=%p source=0x%08X entity=%p "
               "entity_vtbl=0x%08X",
               hits, gun, a1, entity,
               entity != nullptr ? read_u32(entity, 0) : 0u);
    }

    summary();
    return THISCALL(GUN_HIT, gun, a1, a2, a3, a4, a5);
}

// damage_interface::apply_damage as reached from the gun's bullet path
// (0x004FFA80).  a2 = amount (float), a3 = damage type.  It returns 0 early
// for god mode on the hero (0x0095A6A8) and for entity flag bit 14.
int __fastcall bullet_damage_trace(void *damage, void *,
                                   uint32_t a1, uint32_t a2, uint32_t a3,
                                   uint32_t a4, uint32_t a5, uint32_t a6,
                                   uint32_t a7, uint32_t a8, uint32_t a9,
                                   uint32_t a10, uint32_t a11, uint32_t a12,
                                   uint32_t a13)
{
    ++damage_calls;
    const auto *owner = reinterpret_cast<const void *>(read_u32(damage, 4));
    const auto owner_flags = owner != nullptr ? read_u32(owner, 8) : 0u;
    const bool god_mode = *reinterpret_cast<const uint8_t *>(GOD_MODE_CHEAT) != 0;

    const int result = THISCALL(APPLY_DAMAGE, damage, a1, a2, a3, a4, a5, a6,
                                a7, a8, a9, a10, a11, a12, a13);
    if (result != 0) {
        ++damage_applied;
    } else {
        ++damage_blocked;
    }

    if (trace_enabled && damage_calls <= 32) {
        float amount = 0.0f;
        std::memcpy(&amount, &a2, sizeof(amount));
        sp_log("[xbpack] v10 combat: bullet damage #%u ifc=%p owner=%p amount=%.2f "
               "type=%u god_mode=%u invulnerable_flag=%u result=%d",
               damage_calls, damage, owner, amount, a3, god_mode ? 1u : 0u,
               (owner_flags >> 14) & 1u, result);
    }

    summary();
    return result;
}

bool call_targets(uintptr_t site, uintptr_t target)
{
    const auto *code = reinterpret_cast<const uint8_t *>(site);
    if (code[0] != 0xE8) {
        return false;
    }
    int32_t rel = 0;
    std::memcpy(&rel, code + 1, sizeof(rel));
    return site + 5u + static_cast<uintptr_t>(rel) == target;
}
} // namespace

extern "C" __attribute__((noinline, used)) void __cdecl xbpack_v10_layer_nonblocking(
    const uint8_t *launch_state, uint8_t *layer)
{
    const bool beta = use_beta_rule(launch_state);
    note_launch(launch_state, false, beta);
    if (!beta && layer != nullptr) {
        layer[0x34] = 0;
    }
}

extern "C" __attribute__((noinline, used)) void __cdecl xbpack_v10_layer_blocking(
    const uint8_t *launch_state, uint8_t *layer)
{
    const bool beta = use_beta_rule(launch_state);
    note_launch(launch_state, true, beta);
    if (beta || layer == nullptr) {
        return;
    }

    auto *parent = *reinterpret_cast<uint8_t *const *>(launch_state + 0x0C);
    layer[0x34] = parent[0x34];
    parent[0x34] = 0;
}

// Replaces the 9 bytes at 0x006AF344. eax = new layer machine, esi = the
// launch_layer_state. The replaced code ends with mov eax,1 (the result).
extern "C" __attribute__((naked, used)) void xbpack_v10_layer_nonblocking_stub()
{
    __asm__ volatile(
        "push ecx\n\t"
        "push edx\n\t"
        "push eax\n\t"
        "push esi\n\t"
        "call _xbpack_v10_layer_nonblocking\n\t"
        "add esp, 8\n\t"
        "pop edx\n\t"
        "pop ecx\n\t"
        "mov eax, 1\n\t"
        "ret\n\t");
}

// Replaces the 16 bytes at 0x006AF362. eax (the layer) is used afterwards.
extern "C" __attribute__((naked, used)) void xbpack_v10_layer_blocking_stub()
{
    __asm__ volatile(
        "push eax\n\t"
        "push ecx\n\t"
        "push edx\n\t"
        "push eax\n\t"
        "push esi\n\t"
        "call _xbpack_v10_layer_blocking\n\t"
        "add esp, 8\n\t"
        "pop edx\n\t"
        "pop ecx\n\t"
        "pop eax\n\t"
        "ret\n\t");
}

bool install_v10_layer_interrupt_bridge()
{
    read_settings();

    uint8_t nonblocking[sizeof(PC_NONBLOCKING)] {};
    uint8_t blocking[sizeof(PC_BLOCKING)] {};
    make_call(nonblocking, LAYER_NONBLOCKING_SITE,
              reinterpret_cast<const void *>(&xbpack_v10_layer_nonblocking_stub),
              sizeof(nonblocking));
    make_call(blocking, LAYER_BLOCKING_SITE,
              reinterpret_cast<const void *>(&xbpack_v10_layer_blocking_stub),
              sizeof(blocking));

    const auto *site_a = reinterpret_cast<const uint8_t *>(LAYER_NONBLOCKING_SITE);
    const auto *site_b = reinterpret_cast<const uint8_t *>(LAYER_BLOCKING_SITE);
    const bool a_ok = matches(site_a, PC_NONBLOCKING, sizeof(PC_NONBLOCKING)) ||
        matches(site_a, BETA_NONBLOCKING, sizeof(BETA_NONBLOCKING)) ||
        matches(site_a, nonblocking, sizeof(nonblocking));
    const bool b_ok = matches(site_b, PC_BLOCKING, sizeof(PC_BLOCKING)) ||
        matches(site_b, BETA_BLOCKING, sizeof(BETA_BLOCKING)) ||
        matches(site_b, blocking, sizeof(blocking));
    if (!a_ok || !b_ok) {
        return false;
    }

    write_code(LAYER_NONBLOCKING_SITE, nonblocking, sizeof(nonblocking));
    write_code(LAYER_BLOCKING_SITE, blocking, sizeof(blocking));

    static const char *const MODES[] = {
        "stock PC rule for every graph",
        "beta rule for pedestrian graphs, stock PC rule elsewhere",
        "beta rule for every graph",
    };
    sp_log("[xbpack] V10 layer interrupt bridge installed: %s "
           "(openusm.ini [XbpackV10] LayerInterrupts=%d)",
           MODES[layer_mode], layer_mode);
    return true;
}

void xbpack_v10_combat_trace_patch()
{
    read_settings();
    if (!trace_enabled) {
        sp_log("[xbpack] v10 combat trace disabled (CombatTrace=0)");
        return;
    }

    unsigned installed = 0;
    for (const auto site : GUN_FIRE_CALLS) {
        if (call_targets(site, GUN_FIRE)) {
            REDIRECT(site, gun_fire_trace);
            ++installed;
        }
    }

    const bool hit_hooked = read_u32(reinterpret_cast<const void *>(GUN_HIT_SLOT), 0) == GUN_HIT;
    if (hit_hooked) {
        set_vfunc(GUN_HIT_SLOT, &gun_hit_trace);
    }

    const bool damage_hooked = call_targets(BULLET_DAMAGE_CALL, APPLY_DAMAGE);
    if (damage_hooked) {
        REDIRECT(BULLET_DAMAGE_CALL, bullet_damage_trace);
    }

    sp_log("[xbpack] v10 combat trace installed: gun::fire %u/5 sites, hit handler %s, "
           "bullet damage %s",
           installed, hit_hooked ? "yes" : "no", damage_hooked ? "yes" : "no");
}

#endif
