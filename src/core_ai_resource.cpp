#include "core_ai_resource.h"

#include "combo_system.h"
#include "common.h"
#include "func_wrapper.h"
#include "resource_key.h"
#include "trace.h"
#include "utility.h"
#include "xbpack.h"

#include <algorithm>
#include <cassert>
#include <array>
#include <cstring>
#include <memory>
#include <vector>

namespace ai {

#if defined(OPENUSM_XBPACK_MODE) && !defined(TARGET_XBOX)
namespace {

std::vector<std::unique_ptr<uint8_t[]>> converted_core_ai_buffers;

uint32_t read_u32(const uint8_t *data)
{
    uint32_t value = 0;
    std::memcpy(&value, data, sizeof(value));
    return value;
}

void convert_graph_keys(mVector<resource_key> &graphs)
{
    for (auto *key : graphs) {
        if (key->m_type != RESOURCE_KEY_TYPE_AI_STATE_GRAPH) {
            key->m_type = static_cast<resource_key_type>(
                xbpack::pc_type(static_cast<int>(key->m_type)));
        }

        assert(key->m_type == RESOURCE_KEY_TYPE_AI_STATE_GRAPH);
    }
}

#ifdef OPENUSM_XBPACK_V10

struct pc_mash_info
{
    uint8_t *image;
    int used;
    int size;
    int field_C;
};

static_assert(sizeof(pc_mash_info) == 0x10);

enum class node_layout
{
    info,
    combat_target,
    hero,
    glass_house,
    swing,
    target,
    web_zip,
    combat,
    interaction,
    loco,
    std_fear,
    pedestrian,
    cpu_controller,
    ped_avoidance,
    avoidance,
    universal_soldier,
    venom,
    std_web_zip,
    green_goblin,
    std_default_trans,
    prop_throw,
};

struct xb_node
{
    uint32_t type;
    uint32_t id;
    uint16_t xbox_size;
    uint16_t pc_size;
    node_layout layout;
};

constexpr xb_node V10_NODES[] {
    {0x02E, 0x76BB3C7A, 0x058, 0x05C, node_layout::info},
    {0x05F, 0xC1B02807, 0x044, 0x048, node_layout::info},
    {0x060, 0x4312463B, 0x02C, 0x030, node_layout::info},
    {0x061, 0x5D0C49A4, 0x2F8, 0x330, node_layout::combat},
    {0x06B, 0x62030D0E, 0x0A4, 0x0A8, node_layout::info},
    {0x06D, 0xA5C90574, 0x044, 0x048, node_layout::info},
    {0x073, 0xBFF2ADD4, 0x03C, 0x040, node_layout::info},
    {0x078, 0x5D0C49A4, 0x2FC, 0x32C, node_layout::combat},
    {0x07B, 0x00000DAE, 0x08C, 0x094, node_layout::green_goblin},
    {0x13D, 0x0001AB00, 0x028, 0x02C, node_layout::info},
    {0x13E, 0x0001AB00, 0x02C, 0x030, node_layout::info},
    {0x140, 0xFAD58E58, 0x030, 0x048, node_layout::avoidance},
    {0x170, 0x003AC42E, 0x210, 0x24C, node_layout::hero},
    {0x12E, 0x08641048, 0x088, 0x094, node_layout::swing},
    {0x12B, 0x15897C0C, 0x018, 0x020, node_layout::info},
    {0x14E, 0x1754B0DC, 0x05C, 0x088, node_layout::combat_target},
    {0x14F, 0x1754B0DC, 0x05C, 0x088, node_layout::combat_target},
    {0x151, 0x1754B0DC, 0x068, 0x094, node_layout::target},
    {0x154, 0x1754B0DC, 0x068, 0x094, node_layout::target},
    {0x16F, 0x1B17CB5D, 0x030, 0x034, node_layout::glass_house},
    {0x136, 0x1CF15FD1, 0x128, 0x0E0, node_layout::web_zip},
    {0x120, 0x371268F7, 0x028, 0x02C, node_layout::info},
    {0x095, 0x5D0C49A4, 0x2F8, 0x328, node_layout::combat},
    {0x146, 0x5D0C49A4, 0x2F8, 0x328, node_layout::combat},
    {0x096, 0x76DDDD6F, 0x0A8, 0x0D8, node_layout::pedestrian},
    {0x0F2, 0x7CED570D, 0x038, 0x03C, node_layout::info},
    // S02_SHOCKER_PACK contains the beta shocker_inode (0x28 bytes).
    // PC 0xF7 uses 0x38 (vtable 0x0087D6D0, size function 0x00443050).
    // Expand the info-node base before PC unmashing reads param_array at +0x14;
    // native activate initializes every Shocker-specific field at +0x1C..+0x34.
    {0x0E7, 0x0087893D, 0x028, 0x038, node_layout::info},
    {0x0E8, 0x5D0C49A4, 0x320, 0x350, node_layout::combat},
    // The S02 police core also carries arrest/follow targeting nodes. Their
    // PC vtables share combat_target_inode's 0x88-byte layout and unmash path.
    {0x0E9, 0x39338F28, 0x05C, 0x088, node_layout::combat_target},
    {0x109, 0xEAED554A, 0x05C, 0x088, node_layout::combat_target},
    {0x0EA, 0x399785D1, 0x01C, 0x020, node_layout::info},
    {0x163, 0x94B51E64, 0x01C, 0x034, node_layout::std_default_trans},
    {0x08D, 0x9EE13B40, 0x044, 0x04C, node_layout::interaction},
    {0x093, 0x4C90D9E0, 0x054, 0x064, node_layout::loco},
    {0x094, 0xFAD58E58, 0x038, 0x054, node_layout::ped_avoidance},
    {0x10D, 0x6D4B8BFF, 0x020, 0x024, node_layout::info},
    {0x167, 0x74556656, 0x02C, 0x07C, node_layout::std_fear},
    {0x1B4, 0xA2D277FE, 0x040, 0x044, node_layout::info},
    {0x181, 0xA8E18643, 0x01C, 0x020, node_layout::info},
    {0x182, 0xDA1774E3, 0x02C, 0x030, node_layout::info},
    {0x185, 0x8AB41E64, 0x01C, 0x020, node_layout::info},
    {0x189, 0x1CF0A9AA, 0x034, 0x038, node_layout::info},
    {0x08F, 0xC8553C6E, 0x04C, 0x050, node_layout::info},
    {0x156, 0xCC62C392, 0x0E8, 0x0EC, node_layout::info},
    {0x157, 0x9317E156, 0x034, 0x03C, node_layout::info},
    {0x193, 0xCCF57218, 0x038, 0x03C, node_layout::info},
    {0x187, 0x085DE71B, 0x028, 0x02C, node_layout::info},
    {0x195, 0xF264967F, 0x0C8, 0x0CC, node_layout::info},
    {0x19D, 0xA1F4712A, 0x068, 0x078, node_layout::universal_soldier},
    {0x1A1, 0x5D0C49A4, 0x2FC, 0x32C, node_layout::combat},
    {0x1A6, 0x08909065, 0x0A8, 0x0BC, node_layout::venom},
    {0x172, 0x8345223D, 0x048, 0x050, node_layout::info},
    {0x17C, 0x973950CC, 0x028, 0x02C, node_layout::info},
    {0x148, 0xD552BA6D, 0x020, 0x024, node_layout::info},
    {0x149, 0x5D0C49A4, 0x328, 0x358, node_layout::combat},
    {0x14A, 0x5D0C49A4, 0x30C, 0x33C, node_layout::combat},
    {0x142, 0xD970BD20, 0x054, 0x064, node_layout::loco},
    {0x145, 0xF6E3EBA5, 0x01C, 0x020, node_layout::info},
    {0x155, 0xCC62C392, 0x0FC, 0x108, node_layout::cpu_controller},
    {0x00B, 0xD664A286, 0x01C, 0x020, node_layout::info},
    {0x16D, 0x0C1057FA, 0x070, 0x080, node_layout::std_web_zip},
    // Nodes that only gain the PC info-node base dword.  Left unexpanded they
    // desynchronise the PC mVector<info_node> unmash (0x006B80E0); in S07 the
    // next node's vtable id was then read from string bytes ("ltim").
    // preview_combat_inode (Green Goblin): Xbox activate 0x002AE6B0 writes
    // +0x18, PC activate 0x006FE610 writes +0x1C.
    {0x0AD, 0x7AB5E7A5, 0x01C, 0x020, node_layout::info},
    // rhino_inode: Xbox ctor 0x0003C210 / activate 0x002B0E50 fields
    // +0x28/+0x4C.. match PC ctor 0x004419C0 / activate 0x007016B0 at +4.
    {0x0BF, 0x98AC2DEE, 0x078, 0x07C, node_layout::info},
    // prop_throw (Rhino): Xbox +0x2C/+0x48 = PC +0x30/+0x4C (ctors
    // 0x0003C140 / 0x006D7B50); +0x48 is an owned pointer, see below.
    {0x0B1, 0xA817DC34, 0x04C, 0x050, node_layout::prop_throw},
};

// Shocker's combat_inode (Xbox 0x146 in a core that also has shocker_inode).
// shocker_combat_state (PC 0xF6, mapped in mvector.cpp) reads the PC Shocker
// subclass (0xF5, vtable 0x0087D5A0, 0x338 bytes) at +0x32C/+0x330, past the
// end of the generic 0x328-byte combat_inode.  The subclass only appends
// fields its activate (0x006FE8E0) initialises, so expand the generic layout
// into the larger object and retag it for fixup_vtable.
constexpr uint32_t XBOX_V10_SHOCKER_INODE = 0x0E7;
constexpr xb_node V10_SHOCKER_COMBAT_NODE {
    xbpack::v10_shocker_combat_inode_type, 0x5D0C49A4, 0x2F8, 0x338, node_layout::combat};

void expand_info(const uint8_t *source, size_t xbox_size, uint8_t *destination)
{
    std::memcpy(destination, source, 0x0C);
    std::memcpy(destination + 0x10, source + 0x0C, xbox_size - 0x0C);
}

void expand_combat_target(const uint8_t *source, uint8_t *destination)
{
    std::memcpy(destination, source, 0x0C);
    std::memcpy(destination + 0x10, source + 0x0C, 0x0C);
    std::memcpy(destination + 0x20, source + 0x1C, 4);
    std::memcpy(destination + 0x28, source + 0x20, 0x24);
    std::memcpy(destination + 0x4C, source + 0x48, 0x0C);
    destination[0x84] = source[0x58];
}

void expand_hero(const uint8_t *source, uint8_t *destination)
{
    std::memcpy(destination, source, 0x0C);
    std::memcpy(destination + 0x10, source + 0x0C, 0x44);
    std::memcpy(destination + 0x88, source + 0x50, 0x1C0);
}

void expand_glass_house(const uint8_t *source, uint8_t *destination)
{
    expand_info(source, 0x30, destination);
    std::memset(destination + 0x24, 0, 0x0C);
}

void expand_v10_swing(const uint8_t *source, uint8_t *destination)
{
    expand_info(source, 0x18, destination);
    destination[0x1C] = source[0x18];
    std::memcpy(destination + 0x20, source + 0x20, 4);
    destination[0x24] = source[0x19];
    std::memcpy(destination + 0x28, source + 0x1C, 4);
    std::memcpy(destination + 0x30, source + 0x24, 0x14);
    destination[0x44] = source[0x38];
    std::memcpy(destination + 0x48, source + 0x3C, 0x0C);
    destination[0x54] = source[0x48];
    std::memcpy(destination + 0x58, source + 0x4C, 0x3C);
}

void expand_target(const uint8_t *source, uint8_t *destination)
{
    std::memcpy(destination, source, 0x0C);
    std::memcpy(destination + 0x10, source + 0x0C, 0x14);
    std::memcpy(destination + 0x28, source + 0x20, 0x24);
    std::memcpy(destination + 0x4C, source + 0x48, 0x0C);
    std::memcpy(destination + 0x84, source + 0x54, 4);
    std::memcpy(destination + 0x88, source + 0x5C, 0x0C);
}

void expand_v10_web_zip(const uint8_t *source, uint8_t *destination)
{
    std::memcpy(destination, source, 0x0C);
    std::memcpy(destination + 0x10, source + 0x0C, 0x94);
    std::memcpy(destination + 0xA4, source + 0xA0, 0x30);
    std::memcpy(destination + 0xD8, source + 0x120, 8);
}

void expand_v10_combat(const uint8_t *source,
                       size_t xbox_size,
                       uint8_t *destination)
{
    std::memcpy(destination, source, 0x0C);
    std::memcpy(destination + 0x10, source + 0x0C, 0x50);
    std::memcpy(destination + 0x70, source + 0x5C, 0x40);
    destination[0xB0] = source[0x9C];
    destination[0xB4] = source[0xA0];
    destination[0xD0] = source[0xA1];
    std::memcpy(destination + 0xD4, source + 0xA4, 4);
    std::memcpy(destination + 0xD8, source + 0xA8, xbox_size - 0xA8);
}

void expand_interaction(const uint8_t *source, uint8_t *destination)
{
    std::memcpy(destination, source, 0x0C);
    std::memcpy(destination + 0x10, source + 0x0C, 0x2C);
    std::memcpy(destination + 0x40, source + 0x38, 0x0C);
}

void expand_loco(const uint8_t *source, uint8_t *destination)
{
    constexpr uint32_t unset_float = 0xBF800000;

    std::memcpy(destination, source, 0x0C);
    std::memcpy(destination + 0x10, source + 0x0C, 0x14);
    std::memcpy(destination + 0x24, &unset_float, sizeof(unset_float));
    std::memcpy(destination + 0x28, source + 0x20, 0x14);
    std::memcpy(destination + 0x3C, &unset_float, sizeof(unset_float));
    std::memcpy(destination + 0x40, source + 0x34, 0x14);

    std::memcpy(destination + 0x54, source + 0x48, 5);
    std::memcpy(destination + 0x5A, source + 0x4D, 3);
    std::memcpy(destination + 0x5E, source + 0x50, 2);
}

void expand_std_fear(const uint8_t *source, uint8_t *destination)
{
    expand_info(source, 0x18, destination);
    std::memcpy(destination + 0x78, source + 0x28, 2);
}

void expand_pedestrian(const uint8_t *source, uint8_t *destination)
{
    expand_info(source, 0x18, destination);
    std::memcpy(destination + 0xD0, source + 0xA4, 2);
}

void expand_cpu_controller(const uint8_t *source, uint8_t *destination)
{
    expand_info(source, 0x18, destination);
    std::memcpy(destination + 0xEC, source + 0xE8, 4);
    std::memcpy(destination + 0xF8, source + 0xEC, 8);
    destination[0x104] = source[0xF8];
}

void expand_ped_avoidance(const uint8_t *source, uint8_t *destination)
{
    expand_info(source, 0x18, destination);
}

void expand_avoidance(const uint8_t *source, uint8_t *destination)
{
    expand_info(source, 0x18, destination);
    std::memcpy(destination + 0x20, source + 0x18, 4);
    std::memcpy(destination + 0x24, source + 0x1C, 0x14);
}

void expand_universal_soldier(const uint8_t *source, uint8_t *destination)
{
    std::memcpy(destination, source, 0x0C);
    std::memcpy(destination + 0x10, source + 0x0C, 0x0C);
    std::memcpy(destination + 0x20, source + 0x18, 0x44);
    std::memcpy(destination + 0x68, source + 0x5C, 8);
    std::memcpy(destination + 0x74, source + 0x64, 4);
}

void expand_venom(const uint8_t *source, uint8_t *destination)
{
    std::memcpy(destination, source, 0x0C);
    std::memcpy(destination + 0x10, source + 0x0C, 0x14);
    std::memcpy(destination + 0x28, source + 0x20, 8);
    std::memcpy(destination + 0x34, source + 0x28, 0x70);
    std::memcpy(destination + 0xA8, source + 0x98, 0x10);
}

void expand_std_web_zip(const uint8_t *source, uint8_t *destination)
{
    expand_loco(source, destination);
    std::memcpy(destination + 0x64, source + 0x54, 0x1C);
}

void expand_prop_throw(const uint8_t *source, uint8_t *destination)
{
    expand_info(source, 0x4C, destination);

    // +0x4C (Xbox +0x48) is a pointer the destructor (PC 0x006D7BA0, Xbox
    // 0x00265690) deletes.  The serialized word is stale memory from the
    // beta's mash writer (text such as "})} "), not an object.
    std::memset(destination + 0x4C, 0, sizeof(uint32_t));
}

void expand_std_default_trans(const uint8_t *source, uint8_t *destination)
{
    expand_info(source, 0x1C, destination);

    // V10 ends at +0x1C and has no equivalents for these PC runtime
    // permissions. The normal PC constructor (0x006A1700) enables all
    // three; the in-place constructor (0x006A1790) preserves mashed values.
    // Leaving the expanded tail zero disables airborne recovery, subdued
    // and death transitions, including the hero's zero-health check at
    // 0x00478E0B. Keep serialized V10 parameters/cached flags intact.
    destination[0x24] = 1;
    destination[0x25] = 1;
    destination[0x26] = 1;
}

void expand_green_goblin(const uint8_t *source, uint8_t *destination)
{
    // V10 gg inode: Xbox 0x8C, PC 0x94. Besides the PC actor pointer
    // at +0x0C, the PC runtime adds a field at +0x44. Constructors and
    // activation code place the Xbox +0x58/+0x59/+0x60 flags at
    // PC +0x60/+0x61/+0x68, respectively.
    std::memcpy(destination, source, 0x0C);
    std::memcpy(destination + 0x10, source + 0x0C, 0x34);
    std::memcpy(destination + 0x48, source + 0x40, 0x4C);
}

const xb_node *find_v10_node(const uint8_t *source)
{
    const auto type = read_u32(source);
    const auto id = read_u32(source + 4);

    for (const auto &node : V10_NODES) {
        if (node.type == type && node.id == id) {
            return &node;
        }
    }

    return nullptr;
}

void expand_node(const xb_node &node,
                 const uint8_t *source,
                 uint8_t *destination)
{
    switch (node.layout) {
    case node_layout::info:
        expand_info(source, node.xbox_size, destination);
        break;
    case node_layout::combat_target:
        expand_combat_target(source, destination);
        break;
    case node_layout::hero:
        expand_hero(source, destination);
        break;
    case node_layout::glass_house:
        expand_glass_house(source, destination);
        break;
    case node_layout::swing:
        expand_v10_swing(source, destination);
        break;
    case node_layout::target:
        expand_target(source, destination);
        break;
    case node_layout::web_zip:
        expand_v10_web_zip(source, destination);
        break;
    case node_layout::combat:
        expand_v10_combat(source, node.xbox_size, destination);
        break;
    case node_layout::interaction:
        expand_interaction(source, destination);
        break;
    case node_layout::loco:
        expand_loco(source, destination);
        break;
    case node_layout::std_fear:
        expand_std_fear(source, destination);
        break;
    case node_layout::pedestrian:
        expand_pedestrian(source, destination);
        break;
    case node_layout::cpu_controller:
        expand_cpu_controller(source, destination);
        break;
    case node_layout::ped_avoidance:
        expand_ped_avoidance(source, destination);
        break;
    case node_layout::avoidance:
        expand_avoidance(source, destination);
        break;
    case node_layout::universal_soldier:
        expand_universal_soldier(source, destination);
        break;
    case node_layout::venom:
        expand_venom(source, destination);
        break;
    case node_layout::std_web_zip:
        expand_std_web_zip(source, destination);
        break;
    case node_layout::green_goblin:
        expand_green_goblin(source, destination);
        break;
    case node_layout::std_default_trans:
        expand_std_default_trans(source, destination);
        break;
    case node_layout::prop_throw:
        expand_prop_throw(source, destination);
        break;
    }
}

bool convert_v10_nodes(std::vector<uint8_t> &data)
{
    struct node_ref
    {
        size_t offset;
        const xb_node *node;
    };

    std::vector<node_ref> nodes;
    for (size_t offset = 0; offset + 8 <= data.size(); offset += 4)
    {
        const auto *node = find_v10_node(data.data() + offset);
        if (node == nullptr || offset + node->xbox_size > data.size()) {
            continue;
        }

        if (!nodes.empty()) {
            const auto &previous = nodes.back();
            if (offset < previous.offset + previous.node->xbox_size) {
                continue;
            }
        }

        nodes.push_back({offset, node});
        offset += node->xbox_size - 4;
    }

    if (nodes.empty()) {
        return false;
    }

    const bool shocker_core = std::any_of(nodes.begin(), nodes.end(), [](const node_ref &entry) {
        return entry.node->type == XBOX_V10_SHOCKER_INODE;
    });
    if (shocker_core) {
        for (auto &entry : nodes) {
            if (entry.node->type == 0x146 && entry.node->id == V10_SHOCKER_COMBAT_NODE.id) {
                entry.node = &V10_SHOCKER_COMBAT_NODE;
            }
        }
    }

    std::vector<uint8_t> converted;
    converted.reserve(data.size() + nodes.size() * 0x40);

    size_t source_offset = 0;
    for (const auto &entry : nodes)
    {
        converted.insert(converted.end(),
                         data.begin() + source_offset,
                         data.begin() + entry.offset);

        const auto destination_offset = converted.size();
        converted.resize(destination_offset + entry.node->pc_size, 0);
        expand_node(*entry.node,
                    data.data() + entry.offset,
                    converted.data() + destination_offset);
        if (entry.node == &V10_SHOCKER_COMBAT_NODE) {
            std::memcpy(converted.data() + destination_offset,
                        &V10_SHOCKER_COMBAT_NODE.type,
                        sizeof(uint32_t));
        }

        source_offset = entry.offset + entry.node->xbox_size;
    }

    converted.insert(converted.end(),
                     data.begin() + source_offset,
                     data.end());
    data.swap(converted);
    return true;
}

#else

bool convert_web_zip_inode(std::vector<uint8_t> &data)
{
    constexpr auto xbox_hash = to_hash("web_zip_inode");
    constexpr int xbox_object_size = 0xB4;
    constexpr int inline_array_size = 0x30;
    constexpr int pc_object_size = 0xE0;
    constexpr int xbox_total_size = xbox_object_size + inline_array_size;

    if (data.size() < xbox_total_size) {
        return false;
    }

    bool converted = false;
    for (size_t offset = 0; offset + xbox_total_size <= data.size(); offset += 4)
    {
        if (read_u32(data.data() + offset) != xbox_hash) {
            continue;
        }

        const auto pointer_offset = offset + pc_object_size - inline_array_size;
        const auto xbox_array_pointer = read_u32(data.data() + pointer_offset);
        if (xbox_array_pointer == 0) {
            continue;
        }

        data.erase(data.begin() + pointer_offset,
                   data.begin() + pointer_offset + sizeof(uint32_t));
        converted = true;
        offset += pc_object_size - 4;
    }

    return converted;
}

void expand_swing_inode(const uint8_t *source, uint8_t *destination)
{
    std::memcpy(destination, source, 0x1C);

    destination[0x1C] = source[0x87];
    std::memcpy(destination + 0x20, source + 0x1C, 4);
    destination[0x24] = source[0x85];
    std::memcpy(destination + 0x28, source + 0x20, 0x1C);

    destination[0x44] = source[0x84];
    std::memcpy(destination + 0x48, source + 0x3C, 0x0C);
    destination[0x54] = source[0x86];
    std::memcpy(destination + 0x58, source + 0x48, 0x3C);
}

bool convert_swing_inode(std::vector<uint8_t> &data)
{
    constexpr auto swing_hash = to_hash("swing_inode");
    constexpr auto following_hash = to_hash("std_puppet_inode");
    constexpr size_t xbox_object_size = 0x88;
    constexpr size_t pc_object_size = 0x94;

    if (data.size() < xbox_object_size + sizeof(uint32_t)) {
        return false;
    }

    bool converted = false;
    for (size_t offset = 0;
         offset + xbox_object_size + sizeof(uint32_t) <= data.size();
         offset += 4)
    {
        const auto *source = data.data() + offset;
        if (read_u32(source) != swing_hash
            || read_u32(source + xbox_object_size) != following_hash) {
            continue;
        }

        std::array<uint8_t, pc_object_size> expanded {};
        expand_swing_inode(source, expanded.data());

        data.erase(data.begin() + offset,
                   data.begin() + offset + xbox_object_size);
        data.insert(data.begin() + offset, expanded.begin(), expanded.end());
        converted = true;
        offset += pc_object_size - 4;
    }

    return converted;
}

void expand_results(const uint8_t *source, uint8_t *destination)
{
    constexpr size_t xbox_size = 0x78;
    constexpr size_t pc_size = 0x7C;
    constexpr size_t pc_only_string_word = 0x10;

    std::memcpy(destination, source, pc_only_string_word);
    std::memset(destination + pc_only_string_word, 0, sizeof(uint32_t));
    std::memcpy(destination + pc_only_string_word + sizeof(uint32_t),
                source + pc_only_string_word,
                xbox_size - pc_only_string_word);
    static_assert(pc_only_string_word + sizeof(uint32_t)
                      + xbox_size - pc_only_string_word
                  == pc_size);
}

void expand_incoming_move(const uint8_t *source, uint8_t *destination)
{
    constexpr size_t prefix_size = 0x14;
    constexpr size_t xbox_results_size = 0x78;
    constexpr size_t pc_results_size = 0x7C;
    constexpr size_t pc_size = 0x94;

    std::memcpy(destination, source, prefix_size);
    expand_results(source + prefix_size, destination + prefix_size);
    std::memset(destination + prefix_size + pc_results_size,
                0,
                pc_size - prefix_size - pc_results_size);
    static_assert(prefix_size + xbox_results_size == 0x8C);
}

void expand_combat_state(const uint8_t *source, uint8_t *destination)
{
    std::memcpy(destination, source, 0x4C);
    std::memset(destination + 0x4C, 0, 4);
    std::memcpy(destination + 0x50, source + 0x4C, 0x30);
    std::memset(destination + 0x80, 0, 8);
    std::memcpy(destination + 0x88, source + 0x7C, 0x10);

    const auto *xbox_state = source + 0x8C;
    auto *pc_state = destination + 0x98;
    std::memcpy(pc_state, xbox_state, 0x1C);
    std::memset(pc_state + 0x1C, 0, 4);
    std::memcpy(pc_state + 0x20, xbox_state + 0x1C, 0x18);
    pc_state[0x38] = xbox_state[0x3D];
    std::memset(pc_state + 0x39, 0, 3);

    std::memcpy(destination + 0xD4, source + 0xCC, 4);
}

bool convert_player_combat_inode(std::vector<uint8_t> &data)
{
    constexpr auto player_hash = to_hash("player_combat_inode");
    constexpr auto combat_hash = to_hash("combat_inode");
    constexpr auto incoming_hash = to_hash("combat_inode::incoming_move");

    constexpr size_t xbox_object_size = 0xDC;
    constexpr size_t pc_object_size = 0x330;
    constexpr size_t parameter_data_size = 0x28;
    constexpr size_t xbox_incoming_size = 0x90;
    constexpr size_t pc_incoming_size = 0x94;
    constexpr size_t incoming_count = 4;
    constexpr size_t xbox_array_offset = xbox_object_size + parameter_data_size;
    constexpr size_t xbox_total_size =
        xbox_array_offset + xbox_incoming_size * incoming_count;
    constexpr size_t pc_total_size = pc_object_size + parameter_data_size;

    static_assert(xbox_array_offset == 0x104);
    static_assert(xbox_total_size == 0x344);
    static_assert(pc_total_size == 0x358);

    if (data.size() < xbox_total_size) {
        return false;
    }

    bool converted = false;
    for (size_t offset = 0; offset + xbox_total_size <= data.size(); offset += 4)
    {
        const auto *source = data.data() + offset;
        if (read_u32(source) != player_hash || read_u32(source + 4) != combat_hash) {
            continue;
        }

        bool has_expected_moves = true;
        for (size_t i = 0; i < incoming_count; ++i) {
            if (read_u32(source + xbox_array_offset + i * xbox_incoming_size)
                != incoming_hash) {
                has_expected_moves = false;
                break;
            }
        }
        if (!has_expected_moves) {
            continue;
        }

        std::array<uint8_t, pc_total_size> expanded {};
        expand_combat_state(source, expanded.data());

        for (size_t i = 0; i < incoming_count; ++i) {
            expand_incoming_move(
                source + xbox_array_offset + i * xbox_incoming_size,
                expanded.data() + 0xD8 + i * pc_incoming_size);
        }

        std::memcpy(expanded.data() + 0x328, source + 0xD4, 8);
        std::memcpy(expanded.data() + pc_object_size,
                    source + xbox_object_size,
                    parameter_data_size);

        data.erase(data.begin() + offset, data.begin() + offset + xbox_total_size);
        data.insert(data.begin() + offset, expanded.begin(), expanded.end());
        converted = true;
        offset += pc_total_size - 4;
    }

    return converted;
}

bool convert_spidey_combat_inode(std::vector<uint8_t> &data)
{
    constexpr auto spidey_hash = to_hash("spidey_combat_inode");
    constexpr auto combat_hash = to_hash("combat_inode");
    constexpr auto incoming_hash = to_hash("combat_inode::incoming_move");

    constexpr size_t xbox_object_size = 0xFC;
    constexpr size_t pc_object_size = 0x350;
    constexpr size_t xbox_incoming_size = 0x90;
    constexpr size_t pc_incoming_size = 0x94;
    constexpr size_t incoming_count = 4;
    constexpr size_t xbox_total_size =
        xbox_object_size + xbox_incoming_size * incoming_count;

    static_assert(xbox_total_size == 0x33C);

    if (data.size() < xbox_total_size) {
        return false;
    }

    bool converted = false;
    for (size_t offset = 0; offset + xbox_total_size <= data.size(); offset += 4)
    {
        const auto *source = data.data() + offset;
        if (read_u32(source) != spidey_hash
            || read_u32(source + 4) != combat_hash) {
            continue;
        }

        bool has_expected_moves = true;
        for (size_t i = 0; i < incoming_count; ++i) {
            if (read_u32(source + xbox_object_size + i * xbox_incoming_size)
                != incoming_hash) {
                has_expected_moves = false;
                break;
            }
        }
        if (!has_expected_moves) {
            continue;
        }

        std::array<uint8_t, pc_object_size> expanded {};
        expand_combat_state(source, expanded.data());

        for (size_t i = 0; i < incoming_count; ++i) {
            expand_incoming_move(
                source + xbox_object_size + i * xbox_incoming_size,
                expanded.data() + 0xD8 + i * pc_incoming_size);
        }

        std::memcpy(expanded.data() + 0x328, source + 0xD4, 8);
        std::memcpy(expanded.data() + 0x330, source + 0xDC, 0x20);

        data.erase(data.begin() + offset, data.begin() + offset + xbox_total_size);
        data.insert(data.begin() + offset, expanded.begin(), expanded.end());
        converted = true;
        offset += pc_object_size - 4;
    }

    return converted;
}

#endif

void convert_core_ai_data(core_ai_resource &resource)
{
    auto *source = reinterpret_cast<const uint8_t *>(resource.field_C);
    if (source == nullptr || resource.field_40 <= 0) {
        return;
    }

    std::vector<uint8_t> converted_data(source, source + resource.field_40);
#ifdef OPENUSM_XBPACK_V10
    if (!convert_v10_nodes(converted_data)) {
        return;
    }
#else
    bool converted = convert_web_zip_inode(converted_data);
    converted = convert_swing_inode(converted_data) || converted;
    converted = convert_player_combat_inode(converted_data) || converted;
    converted = convert_spidey_combat_inode(converted_data) || converted;
    if (!converted) {
        return;
    }
#endif

    auto storage = std::make_unique<uint8_t[]>(converted_data.size());
    std::memcpy(storage.get(), converted_data.data(), converted_data.size());
    resource.field_C = reinterpret_cast<intptr_t>(storage.get());
    resource.field_40 = static_cast<int>(converted_data.size());

    converted_core_ai_buffers.push_back(std::move(storage));
}

} // namespace
#endif

VALIDATE_SIZE(core_ai_resource, 0x48);
VALIDATE_OFFSET(core_ai_resource, my_locomotion_graphs, 0x28);

core_ai_resource::core_ai_resource(from_mash_in_place_constructor *a2) {
    THISCALL(0x006D9A10, this, a2);
}

int core_ai_resource::destruct_mashed_class() {
    return THISCALL(0x006D71A0, this);
}

void core_ai_resource::unmash(mash_info_struct *a1, [[maybe_unused]] void *a3)
{
    TRACE("ai::core_ai_resource::unmash");

#if OPENUSM_XBOX_MASH_FORMAT
#if defined(OPENUSM_XBPACK_V10) && !defined(TARGET_XBOX)
    auto *pc_mash = reinterpret_cast<pc_mash_info *>(a1);
    mash_info_struct mash_ctx {pc_mash->image, pc_mash->size};
    mash_ctx.buffer_size_used[mash::NORMAL_BUFFER] = pc_mash->used;
    a1 = &mash_ctx;

    a1->unmash_class_in_place(this->field_0, this);
    a1->unmash_class_in_place(this->my_base_graphs, this);
    a1->unmash_class_in_place(this->my_locomotion_graphs, this);
    convert_graph_keys(this->my_base_graphs);
    convert_graph_keys(this->my_locomotion_graphs);

    if (this->field_10 != nullptr)
    {
        this->field_10 = bit_cast<combo_system *>(a1->read_from_buffer(
            mash::NORMAL_BUFFER, sizeof(combo_system), 4));
        a1->unmash_class_in_place(this->field_10->field_0, this->field_10);
        a1->unmash_class_in_place(this->field_10->field_14, this->field_10);
        a1->unmash_class_in_place(this->field_10->field_28, this->field_10);
        a1->unmash_class_in_place(this->field_10->field_3C, this->field_10);
    }

    a1->align_buffer(mash::NORMAL_BUFFER, 4);
    this->field_40 = *reinterpret_cast<int *>(
        a1->read_from_buffer(mash::NORMAL_BUFFER, 4, 4));
    a1->align_buffer(mash::NORMAL_BUFFER, 16);
    this->field_C = reinterpret_cast<intptr_t>(
        &a1->mash_image_ptr[mash::NORMAL_BUFFER]
                           [a1->buffer_size_used[mash::NORMAL_BUFFER]]);
    a1->advance_buffer(mash::NORMAL_BUFFER, this->field_40);
    a1->read_from_buffer(mash::NORMAL_BUFFER, 4, 4);

    pc_mash->used = a1->buffer_size_used[mash::NORMAL_BUFFER];
    convert_core_ai_data(*this);
#else
    a1->unmash_class_in_place(this->field_0, this);
    a1->unmash_class_in_place(this->my_base_graphs, this);
    a1->unmash_class_in_place(this->my_locomotion_graphs, this);

#if defined(OPENUSM_XBPACK_MODE) && !defined(TARGET_XBOX)
    convert_graph_keys(this->my_base_graphs);
    convert_graph_keys(this->my_locomotion_graphs);
#endif

    uint8_t class_mashed = *a1->read_from_buffer(mash::SHARED_BUFFER, 1, 1);
    assert(class_mashed == 0xAF || class_mashed == 0);

    if (this->field_10 != nullptr) {
        a1->unmash_class(this->field_10, this, mash::NORMAL_BUFFER);
    }

    a1->align_buffer(mash::SHARED_BUFFER, 4);
    const auto normal_data_size =
        *reinterpret_cast<int *>(a1->read_from_buffer(mash::SHARED_BUFFER, 4, 4));
    const auto shared_data_size =
        *reinterpret_cast<int *>(a1->read_from_buffer(mash::SHARED_BUFFER, 4, 4));

    a1->align_buffer(mash::NORMAL_BUFFER, 16);
    a1->align_buffer(mash::SHARED_BUFFER, 16);

    this->field_40 = normal_data_size;
    this->field_C = (int)&a1->mash_image_ptr[0][a1->buffer_size_used[0]];
    a1->advance_buffer(mash::NORMAL_BUFFER, normal_data_size);
    a1->advance_buffer(mash::SHARED_BUFFER, shared_data_size);

    const auto normal_sentry =
        *reinterpret_cast<uint32_t *>(a1->read_from_buffer(mash::NORMAL_BUFFER, 4, 4));
    const auto shared_sentry =
        *reinterpret_cast<uint32_t *>(a1->read_from_buffer(mash::SHARED_BUFFER, 4, 4));
    assert(normal_sentry == mash::CUSTOM_MASH_SENTRY);
    assert(shared_sentry == mash::CUSTOM_MASH_SENTRY);

#if defined(OPENUSM_XBPACK_MODE) && !defined(TARGET_XBOX)
    convert_core_ai_data(*this);
#endif
#endif
#else
    THISCALL(0x006D71F0, this, a1, a3);
#endif
}

bool core_ai_resource::does_base_graph_exist(resource_key the_graph) const
{
    assert(the_graph.get_type() == RESOURCE_KEY_TYPE_AI_STATE_GRAPH);

    for (auto &curr : this->my_base_graphs)
    {
        assert(curr->get_type() == RESOURCE_KEY_TYPE_AI_STATE_GRAPH);

        if (*curr == the_graph) {
            return true;
        }
    }

    return false;
}

bool core_ai_resource::does_locomotion_graph_exist(resource_key the_graph) const
{
    assert(the_graph.get_type() == RESOURCE_KEY_TYPE_AI_STATE_GRAPH);

    for (auto &curr : this->my_locomotion_graphs)
    {
        assert(curr->get_type() != RESOURCE_KEY_TYPE_AI_STATE_GRAPH);

        if (*curr == the_graph) {
            return true;
        }
    }

    return false;
}

} // namespace ai

void core_ai_resource_patch()
{
    {
        FUNC_ADDRESS(address, &ai::core_ai_resource::unmash);
        REDIRECT(0x00568A67, address);
    }
}
