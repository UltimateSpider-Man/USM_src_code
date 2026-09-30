#include "xbpack.h"

#ifdef OPENUSM_XBPACK_V10

#include "comic_panels.h"
#include "func_wrapper.h"
#include "log.h"
#include "string_hash.h"
#include "utility.h"

#include <cstdint>
#include <initializer_list>

// V10 pedestrian hit / Venom feed diagnostics.  Each line marks one step of
// the chain as seen by the PC runtime, so a single attack shows where it
// stops:
//   1. ped_default_trans_state sends a hit pedestrian to ped_hit_react,
//   2. ped_hit_react activates with the requested reaction (Fd_Feed_* for a
//      feed),
//   3. Venom's claw animation raises GRAB_START / GRAB_END,
//   4. a comic close-up panel is added.
// All of them fire on rare events only; nothing is logged per frame.
namespace
{
constexpr uintptr_t PED_HIT_REACT_VTABLE = 0x0087A598;
constexpr uintptr_t PED_HIT_REACT_ACTIVATE = 0x006ECE40;
constexpr uintptr_t PED_DEFAULT_TRANS_VTABLE = 0x00875B50;
constexpr uintptr_t PED_DEFAULT_TRANS_CHECK = 0x006ECD90;
constexpr uintptr_t ANIM_EVENT_DISPATCH = 0x004980D0;
constexpr uintptr_t ANIM_EVENT_DISPATCH_CALL = 0x0049C9CF;
constexpr uintptr_t PANEL_FRAME_ADVANCE_CALL = 0x0055D8CC;
constexpr uintptr_t PROCESS_MODE = 0x006A1590;
constexpr uintptr_t PROCESS_MODE_CALL = 0x006AF148;
constexpr uintptr_t GET_ABS_POSITION = 0x0048AC00;
constexpr uintptr_t UNIVERSAL_SOLDIER_TRANS_VTABLE = 0x00877AA8;

unsigned frame = 0;

bool graph_has_default_state(const uint8_t *graph, uint32_t vtable)
{
    if (graph == nullptr) {
        return false;
    }

    const auto count = *reinterpret_cast<const int *>(graph + 0x24);
    const auto *const *list = *reinterpret_cast<void *const *const *>(graph + 0x28);
    for (int i = 0; i < count && list != nullptr; ++i) {
        if (*static_cast<const uint32_t *>(list[i]) == vtable) {
            return true;
        }
    }

    return false;
}

bool is_ped_graph(const uint8_t *graph)
{
    return graph_has_default_state(graph, PED_DEFAULT_TRANS_VTABLE);
}

// Sable mercenaries and other universal soldiers.
bool is_soldier_graph(const uint8_t *graph)
{
    return graph_has_default_state(graph, UNIVERSAL_SOLDIER_TRANS_VTABLE);
}

uint32_t current_state_name(const uint8_t *machine)
{
    const auto *state = *reinterpret_cast<const uint8_t *const *>(machine + 0x10);
    if (state == nullptr) {
        return 0;
    }

    const auto *mashed = *reinterpret_cast<const uint8_t *const *>(state + 8);
    return mashed != nullptr ? *reinterpret_cast<const uint32_t *>(mashed + 0xC) : 0;
}

struct state_count
{
    uint32_t state;
    unsigned count;
};

void count_state(state_count (&table)[16], uint32_t state)
{
    for (auto &entry : table) {
        if (entry.count == 0 || entry.state == state) {
            entry.state = state;
            ++entry.count;
            return;
        }
    }
}

struct ped_sample
{
    const void *actor;
    float pos[3];
    unsigned frame;
    bool moved;
};

ped_sample peds[96] {};
state_count base_states[16] {};
state_count layer_states[16] {};
state_count soldier_states[16] {};
state_count soldier_layer_states[16] {};

struct still_ped
{
    uint32_t base;
    uint32_t layer;
    uint32_t layer_mode;
    int children;
    unsigned count;
};

still_ped still[16] {};

void count_still(const uint8_t *machine)
{
    const auto base = current_state_name(machine);
    const auto *first = *reinterpret_cast<const uint8_t *const *const *>(machine + 0x20);
    const auto *last = *reinterpret_cast<const uint8_t *const *const *>(machine + 0x24);
    const int children = (first != nullptr && last != nullptr) ? int(last - first) : 0;
    uint32_t layer = 0xFFFFFFFF;
    uint32_t layer_mode = 0xFFFFFFFF;
    if (children > 0 && first[0] != nullptr) {
        layer = current_state_name(first[0]);
        layer_mode = *reinterpret_cast<const uint32_t *>(first[0]);
    }

    for (auto &entry : still) {
        if (entry.count == 0 ||
            (entry.base == base && entry.layer == layer && entry.layer_mode == layer_mode &&
             entry.children == children)) {
            entry = {base, layer, layer_mode, children, entry.count + 1};
            return;
        }
    }
}

// Temporary walk probe: every ~3 s per ped, did it move more than 0.75 m?
void sample_ped(const uint8_t *machine)
{
    const auto *actor = *reinterpret_cast<void *const *>(machine + 8);
    if (actor == nullptr) {
        return;
    }

    const auto *pos = reinterpret_cast<const float *>(
        THISCALL(GET_ABS_POSITION, actor));
    ped_sample *slot = nullptr;
    for (auto &sample : peds) {
        if (sample.actor == actor) {
            slot = &sample;
            break;
        }
        if (slot == nullptr && sample.actor == nullptr) {
            slot = &sample;
        }
    }
    if (slot == nullptr) {
        return;
    }

    if (slot->actor != actor) {
        *slot = {actor, {pos[0], pos[1], pos[2]}, frame, false};
        return;
    }

    if (frame - slot->frame >= 90) {
        const float dx = pos[0] - slot->pos[0];
        const float dz = pos[2] - slot->pos[2];
        slot->moved = dx * dx + dz * dz > 0.75f * 0.75f;
        if (!slot->moved) {
            count_still(machine);
        }
        slot->pos[0] = pos[0];
        slot->pos[1] = pos[1];
        slot->pos[2] = pos[2];
        slot->frame = frame;
    }
}

struct layer_life
{
    const void *machine;
    uint32_t graph;
    uint32_t last_state;
    unsigned first;
    unsigned last;
    uint32_t history[6];
    unsigned history_size;
    uint32_t exit_message;
    uint32_t mode;
};

layer_life layers[128] {};

struct layer_exit
{
    uint32_t graph;
    uint32_t last_state;
    bool short_lived;
    unsigned count;
};

layer_exit exits[24] {};

void finish_layer(layer_life &entry);

void track_layer(const uint8_t *machine)
{
    const auto *graph = *reinterpret_cast<const uint8_t *const *>(machine + 0xC);
    const auto graph_name = graph != nullptr ? *reinterpret_cast<const uint32_t *>(graph) : 0u;
    layer_life *slot = nullptr;
    for (auto &entry : layers) {
        if (entry.machine == machine && entry.graph == graph_name) {
            slot = &entry;
            break;
        }
        if (slot == nullptr && entry.machine == nullptr) {
            slot = &entry;
        }
    }
    if (slot == nullptr) {
        return;
    }
    const bool relaunched = slot->machine == machine && slot->history_size != 0 &&
        *reinterpret_cast<const void *const *>(machine + 0x10) == nullptr;
    if (relaunched) {
        finish_layer(*slot);
    }
    if (slot->machine != machine || slot->graph != graph_name) {
        *slot = {machine, graph_name, 0, frame, frame, {}, 0, 0, 0};
    }
    slot->last = frame;
    slot->exit_message = *reinterpret_cast<const uint32_t *>(machine + 0x44);
    slot->mode = *reinterpret_cast<const uint32_t *>(machine);
    const auto state = current_state_name(machine);
    if (state != 0) {
        if (state != slot->last_state && slot->history_size < 6) {
            slot->history[slot->history_size++] = state;
        }
        slot->last_state = state;
    } else {
        static unsigned dumps = 0;
        const auto *curr = *reinterpret_cast<const uint8_t *const *>(machine + 0x10);
        if (curr != nullptr && dumps < 4) {
            ++dumps;
            const auto *mashed = *reinterpret_cast<const uint8_t *const *>(curr + 8);
            const auto *initial = *reinterpret_cast<const uint8_t *const *>(graph + 0x1C);
            sp_log("[xbpack] feed-trace: unnamed layer state: graph 0x%08X mode %u "
                   "state %p vtbl 0x%08X mashed %p graph+1C %p",
                   static_cast<unsigned>(graph_name),
                   *reinterpret_cast<const uint32_t *>(machine),
                   curr, *reinterpret_cast<const uint32_t *>(curr), mashed, initial);
            for (const auto *p : {mashed, initial}) {
                if (p == nullptr) {
                    continue;
                }
                const auto *w = reinterpret_cast<const uint32_t *>(p);
                sp_log("[xbpack] feed-trace:   %p: %08X %08X %08X %08X %08X %08X %08X %08X",
                       p, w[0], w[1], w[2], w[3], w[4], w[5], w[6], w[7]);
            }
        }
    }
}

void collect_layer_exits()
{
    for (auto &entry : layers) {
        if (entry.machine != nullptr && frame - entry.last >= 10) {
            finish_layer(entry);
        }
    }
}

void finish_layer(layer_life &entry)
{
    {
        const bool short_lived = entry.last - entry.first < 30;
        static unsigned histories = 0;
        if (short_lived && histories < 12) {
            ++histories;
            sp_log("[xbpack] feed-trace:   layer 0x%08X lived %u frames, mode %u, "
                   "exit msg %u, states %08X %08X %08X %08X %08X %08X",
                   static_cast<unsigned>(entry.graph), entry.last - entry.first,
                   static_cast<unsigned>(entry.mode),
                   static_cast<unsigned>(entry.exit_message),
                   entry.history[0], entry.history[1], entry.history[2],
                   entry.history[3], entry.history[4], entry.history[5]);
        }
        for (auto &exit : exits) {
            if (exit.count == 0 ||
                (exit.graph == entry.graph && exit.last_state == entry.last_state &&
                 exit.short_lived == short_lived)) {
                exit = {entry.graph, entry.last_state, short_lived, exit.count + 1};
                break;
            }
        }
        entry = {};
    }
}

void __fastcall process_mode(void *machine, void *, int dt, int a3)
{
    const auto *bytes = static_cast<const uint8_t *>(machine);
    const auto *graph = *reinterpret_cast<const uint8_t *const *>(bytes + 0xC);
    if (is_ped_graph(graph)) {
        count_state(base_states, current_state_name(bytes));
        sample_ped(bytes);
    } else if (is_soldier_graph(graph)) {
        count_state(soldier_states, current_state_name(bytes));
    } else {
        const auto *parent = *reinterpret_cast<const uint8_t *const *>(bytes + 0x18);
        const auto *parent_graph = parent != nullptr
            ? *reinterpret_cast<const uint8_t *const *>(parent + 0xC)
            : nullptr;
        if (is_ped_graph(parent_graph)) {
            count_state(layer_states, current_state_name(bytes));
            track_layer(bytes);
        } else if (is_soldier_graph(parent_graph)) {
            count_state(soldier_layer_states, current_state_name(bytes));
            track_layer(bytes);
        }
    }

    THISCALL(PROCESS_MODE, machine, dt, a3);
}

void report_walk_probe()
{
    unsigned tracked = 0;
    unsigned moving = 0;
    for (auto &sample : peds) {
        if (sample.actor != nullptr && frame - sample.frame < 200) {
            ++tracked;
            moving += sample.moved ? 1u : 0u;
        } else {
            sample = {};
        }
    }

    sp_log("[xbpack] feed-trace: peds tracked %u, moving %u, still %u",
           tracked, moving, tracked - moving);
    for (auto *table : {&base_states, &layer_states, &soldier_states,
                        &soldier_layer_states}) {
        const char *label = table == &base_states ? "base "
            : table == &layer_states              ? "layer"
            : table == &soldier_states            ? "soldier"
                                                  : "soldier layer";
        for (auto &entry : *table) {
            if (entry.count != 0) {
                sp_log("[xbpack] feed-trace:   %s state %s (0x%08X) x%u", label,
                       string_hash {static_cast<int>(entry.state)}.to_string(),
                       static_cast<unsigned>(entry.state), entry.count);
            }
            entry = {};
        }
    }

    collect_layer_exits();
    for (auto &exit : exits) {
        if (exit.count != 0) {
            sp_log("[xbpack] feed-trace:   layer graph 0x%08X ended in state 0x%08X "
                   "%s x%u", static_cast<unsigned>(exit.graph),
                   static_cast<unsigned>(exit.last_state),
                   exit.short_lived ? "within 1s" : "after running", exit.count);
        }
        exit = {};
    }

    for (auto &entry : still) {
        if (entry.count != 0) {
            sp_log("[xbpack] feed-trace:   still ped: base 0x%08X, %d layer(s), "
                   "layer mode %d state 0x%08X x%u",
                   static_cast<unsigned>(entry.base), entry.children,
                   static_cast<int>(entry.layer_mode),
                   static_cast<unsigned>(entry.layer), entry.count);
        }
        entry = {};
    }
}

struct named_hash
{
    uint32_t hash;
    const char *name;
};

constexpr named_hash FEED_NAMES[] {
    {to_hash("Fd_Feed_Close_By_Ven"), "Fd_Feed_Close_By_Ven"},
    {to_hash("FdFeedCloseEndByVen"), "FdFeedCloseEndByVen"},
    {to_hash("FdFeedCloseEndByVenHack"), "FdFeedCloseEndByVenHack"},
    {to_hash("Feed_Loop_By_Venom"), "Feed_Loop_By_Venom"},
    {to_hash("Feed_Loop_By_Venom_Hack"), "Feed_Loop_By_Venom_Hack"},
    {to_hash("GRAB_START"), "GRAB_START"},
    {to_hash("GRAB_END"), "GRAB_END"},
};

const char *feed_name(uint32_t hash)
{
    for (const auto &entry : FEED_NAMES) {
        if (entry.hash == hash) {
            return entry.name;
        }
    }

    return nullptr;
}

// ped_hit_react_state::activate: the requested reaction is at +0x34.
int __fastcall ped_hit_react_activate(
    void *self, void *, int a1, int a2, int a3, int a4, int a5)
{
    const int result = THISCALL(PED_HIT_REACT_ACTIVATE, self, a1, a2, a3, a4, a5);

    const auto anim = *reinterpret_cast<const uint32_t *>(
        static_cast<const uint8_t *>(self) + 0x34);
    const char *name = feed_name(anim);
    sp_log("[xbpack] feed-trace: ped_hit_react state=%p anim=%s (0x%08X)",
           self, name != nullptr ? name : "other", static_cast<unsigned>(anim));
    return result;
}

// ped_default_trans_state::check_transition(Float) asks the ped's
// combat_inode whether it was hit.  The result is a state_trans_action
// {action, state, message, param_block}.
void *__fastcall ped_default_trans_check(void *self, void *, uint32_t *out, int dt)
{
    static unsigned calls = 0;
    static unsigned hits = 0;

    auto *result = reinterpret_cast<void *>(
        THISCALL(PED_DEFAULT_TRANS_CHECK, self, out, dt));

    ++calls;
    if (out[1] == to_hash("ped_hit_react")) {
        ++hits;
        sp_log("[xbpack] feed-trace: ped_default_trans -> ped_hit_react "
               "(hit %u, %u checks)", hits, calls);
    } else if (calls == 1 || calls % 50000 == 0) {
        sp_log("[xbpack] feed-trace: ped_default_trans checks=%u hits=%u", calls, hits);
    }

    return result;
}

// ecx = event handler, arg = { ?, const event_entry * }; the entry keeps
// its name hash at +8 (see the native reader at 0x0049C8F0).
int __fastcall anim_event_dispatch(void *self, void *, void *event)
{
    const auto *entry = *reinterpret_cast<const uint8_t *const *>(
        static_cast<const uint8_t *>(event) + 4);
    if (entry != nullptr) {
        const auto hash = *reinterpret_cast<const uint32_t *>(entry + 8);
        if (const char *name = feed_name(hash)) {
            sp_log("[xbpack] feed-trace: anim event %s handler=%p", name, self);
        }
    }

    return THISCALL(ANIM_EVENT_DISPATCH, self, event);
}

void panel_frame_advance(Float dt)
{
    static uint32_t last_count = 0;

    comic_panels::frame_advance(dt);

    if (++frame % 300 == 0) {
        report_walk_probe();
    }

    const auto count = comic_panels::panels().m_size;
    if (count != last_count) {
        sp_log("[xbpack] feed-trace: comic panels %u -> %u",
               static_cast<unsigned>(last_count), static_cast<unsigned>(count));
        last_count = count;
    }
}
}

void xbpack_v10_feed_trace_patch()
{
    set_vfunc(PED_HIT_REACT_VTABLE + 0x18, &ped_hit_react_activate);
    set_vfunc(PED_DEFAULT_TRANS_VTABLE + 0x2C, &ped_default_trans_check);
    REDIRECT(ANIM_EVENT_DISPATCH_CALL, anim_event_dispatch);
    REDIRECT(PANEL_FRAME_ADVANCE_CALL, panel_frame_advance);
    REDIRECT(PROCESS_MODE_CALL, process_mode);
    sp_log("[xbpack] V10 feed trace installed");
}

#endif
