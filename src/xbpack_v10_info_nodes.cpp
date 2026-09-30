#include "xbpack.h"

#ifdef OPENUSM_XBPACK_V10

#include "func_wrapper.h"
#include "log.h"
#include "mash_virtual_base.h"
#include "string_hash.h"
#include "utility.h"
#include "vtbl.h"

#include <cstdint>
#include <cstring>
#include <vector>

// PC AI code assumes every character owns a std_default_trans_inode: the
// hit-react states write it without a null check (hit_react_state::activate
// 0x006AF424, deactivate 0x006A3DF6) and about forty other sites read it.
// Retail PC pedestrians carry one; the Xbox v10 pedestrians do not, because
// the beta's hit_react_state registered THROW_START/THROW_END itself (Xbox
// 0x00266490) while PC moved that into std_default_trans_inode::activate
// (0x00697DA0).  Once v10 peds reach ped_hit_react again (see the layer
// interrupt patch) the missing node crashes the game.
//
// The same holds for ai_std_jump_inode: PC jump states read it without a null
// check (e.g. 0x006A95E0 passes node+0x20 to the vector setter 0x00499690,
// and ~55 other sites), but v10 enemy cores such as the v03 Sable mercenaries
// carry none.  A v10 core entering such a state crashed at 0x004996A0.  Its
// PC class is type 0x182 (0x50 bytes, ctor 0x00440910); activate 0x0068C4C0
// initialises every derived field.
//
// Give each v10 AI core that lacks one of these nodes a fallback instance of
// the native PC class, created and activated on first lookup.  Freed cores are
// reused by address, so one registry slot per address and kind is
// reinitialised instead of allocating again; no native object or list is
// modified.
namespace
{
constexpr uintptr_t GET_INFO_NODE = 0x006A3390;
constexpr uintptr_t AI_CORE_CTOR = 0x006AEA90;
constexpr uintptr_t AI_CORE_CTOR_CALL = 0x006BDAAA;
constexpr uintptr_t NODE_ACTIVATE = 0x20; // info_node::activate(ai_core *)
constexpr size_t MAX_FALLBACK_SIZE = 0x50;

struct fallback_kind
{
    const char *name;
    uint32_t id;
    uint32_t pc_type;
    size_t size;
};

constexpr fallback_kind FALLBACK_KINDS[] = {
    {"std_default_trans_inode", to_hash("std_default_trans_inode"), 0x173, 0x34},
    {"ai_std_jump_inode", to_hash("ai_std_jump_inode"), 0x182, 0x50},
};
constexpr size_t FALLBACK_KIND_COUNT = sizeof(FALLBACK_KINDS) / sizeof(FALLBACK_KINDS[0]);

struct fallback_node
{
    const void *core;
    size_t kind;
    uint8_t *node;
    bool active;
};

std::vector<fallback_node> fallbacks;
uint8_t pristine[FALLBACK_KIND_COUNT][MAX_FALLBACK_SIZE] {};
bool have_pristine[FALLBACK_KIND_COUNT] {};
bool logged[FALLBACK_KIND_COUNT] {};

const fallback_kind *find_kind(uint32_t name, size_t &index)
{
    for (size_t i = 0; i < FALLBACK_KIND_COUNT; ++i) {
        if (FALLBACK_KINDS[i].id == name) {
            index = i;
            return &FALLBACK_KINDS[i];
        }
    }
    return nullptr;
}

// ai_core +0x60 -> sorted mVector<info_node>: size at +4, data at +8.
void *find_node(const uint8_t *core, uint32_t name)
{
    const auto *list = *reinterpret_cast<const uint8_t *const *>(core + 0x60);
    if (list == nullptr) {
        return nullptr;
    }

    const auto count = *reinterpret_cast<const int *>(list + 4);
    auto *const *data = *reinterpret_cast<void *const *const *>(list + 8);
    for (int i = 0; i < count && data != nullptr; ++i) {
        if (*reinterpret_cast<const uint32_t *>(
                static_cast<const uint8_t *>(data[i]) + 4) == name) {
            return data[i];
        }
    }

    return nullptr;
}

void activate(uint8_t *node, void *core)
{
    const auto vtable = *reinterpret_cast<std::intptr_t *>(node);
    void(__fastcall * func)(void *, void *, void *) =
        CAST(func, get_vfunc(vtable, NODE_ACTIVATE));
    func(node, nullptr, core);
}

void *fallback_for(void *core, size_t index)
{
    const auto &kind = FALLBACK_KINDS[index];
    for (auto &entry : fallbacks) {
        if (entry.core == core && entry.kind == index) {
            if (!entry.active) {
                std::memcpy(entry.node, pristine[index], kind.size);
                *reinterpret_cast<uint32_t *>(entry.node + 4) = kind.id;
                activate(entry.node, core);
                entry.active = true;
            }
            return entry.node;
        }
    }

    auto *node = static_cast<uint8_t *>(mash_virtual_base::create_subclass_by_enum(
        static_cast<mash::virtual_types_enum>(kind.pc_type)));
    if (node == nullptr) {
        return nullptr;
    }

    if (!have_pristine[index]) {
        std::memcpy(pristine[index], node, kind.size);
        have_pristine[index] = true;
    }

    *reinterpret_cast<uint32_t *>(node + 4) = kind.id;
    activate(node, core);
    fallbacks.push_back({core, index, node, true});

    if (!logged[index]) {
        logged[index] = true;
        sp_log("[xbpack] V10 AI core without %s: using a fallback node (first core %p)",
               kind.name, core);
    }
    return node;
}

// ai_core::get_info_node(string_hash, bool warn), __thiscall, ret 8.
void *__fastcall get_info_node(void *core, void *, uint32_t name, int warn)
{
    if (auto *node = find_node(static_cast<const uint8_t *>(core), name)) {
        return node;
    }

    size_t index = 0;
    if (find_kind(name, index) != nullptr) {
        return fallback_for(core, index);
    }

    if ((warn & 0xFF) != 0) {
        static unsigned warnings = 0;
        if (warnings++ < 32) {
            sp_log("unknown ai info-node name 0x%08X for core %p",
                   static_cast<unsigned>(name), core);
        }
    }

    return nullptr;
}

// A new core may reuse the address of a destroyed one: its fallback is
// rebuilt on the next lookup.
void *__fastcall construct_ai_core(void *core, void *, int resource, int params, int owner)
{
    for (auto &entry : fallbacks) {
        if (entry.core == core) {
            entry.active = false;
        }
    }

    THISCALL(AI_CORE_CTOR, core, resource, params, owner);
    return core;
}
}

void xbpack_v10_info_nodes_patch()
{
    SET_JUMP(GET_INFO_NODE, get_info_node);
    REDIRECT(AI_CORE_CTOR_CALL, construct_ai_core);
}

#endif
