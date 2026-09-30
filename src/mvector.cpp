#include "mvector.h"

#include "ai_adv_strength_test_data.h"
#include "anim_record.h"
#include "attach_action_trigger_enum.h"
#include "attach_node.h"
#include "common.h"
#include "layer_state_machine_shared.h"
#include "param_block.h"
#include "actor.h"
#include "als_category.h"
#include "als_filter_data.h"
#include "als_post_kill_rule.h"
#include "als_post_layer_alter.h"
#include "als_state.h"
#include "alter_conditions.h"
#include "als_scripted_state.h"
#include "als_transition_rule.h"
#include "als_transition_group_base.h"
#include "als_meta_anim_base.h"
#include "als_meta_anim_swing.h"
#include "base_state.h"
#include "combo_system.h"
#include "combo_system_move.h"
#include "combo_system_weapon.h"
#include "enhanced_state.h"
#include "mashed_state.h"
#include "meta_anim_interact.h"
#include "fefloatingtext.h"
#include "fetext.h"
#include "femultilinetext.h"
#include "func_wrapper.h"
#include "interact_sound_entry.h"
#include "mash_virtual_base.h"
#include "memory.h"
#include "panelanim.h"
#include "panelanimkeyframe.h"
#include "panelanimfile.h"
#include "panelquad.h"
#include "panelquadsection.h"
#include "sound_alias_database.h"
#include "trace.h"
#include "entity_base_vhandle.h"
#include "vtbl.h"
#include "xbpack.h"

#ifdef OPENUSM_XBPACK_V10
#include "xbpack_v10_types.h"
#endif

VALIDATE_SIZE(mVector<int>, 0x14);

#if defined(OPENUSM_XBPACK_MODE) && !defined(TARGET_XBOX)
namespace
{
constexpr int PC_OPERATOR_NEW = 0x00822046;
constexpr int PC_MEM_ALLOC = 0x0043A100;

#ifdef OPENUSM_XBPACK_V10
constexpr uint32_t XBOX_V10_STATE_064 = 0x64;
// The v10 flee pair sits one slot later than the PC numbering suggests:
// Xbox 0x97 is the PC 0xA0 goto-away state, 0x98 flee_quad_path_state and
// 0x99 flee_traffic_path_state (object sizes 0x24/0x28/0x24 against PC
// 0x30/0x34/0x30).  PC 0xA3, the ped cower state, has no v10 class.  The
// previous 0x97/0x98 values turned every v10 traffic-path flee into PC
// 0x99 (lite_idle_state), which left fleeing pedestrians standing still.
constexpr uint32_t XBOX_V10_FLEE_POI_GOTO_STATE = 0x97;
constexpr uint32_t XBOX_V10_FLEE_QUAD_PATH_STATE = 0x98;
constexpr uint32_t XBOX_V10_FLEE_TRAFFIC_PATH_STATE = 0x99;
constexpr uint32_t XBOX_V10_STATE_09C = 0x9C;
// Shocker's COMBAT state (s02_workout): picks the knockback (code 0xF) or
// ranged (code 0x1B) attack from shocker_inode.  Its PC class is 0xF6, which
// makes the same decisions; unmapped, 0xE6 became the PC Silver Sable state,
// whose activate crashed looking up a "sable" inode.  PC 0xF6 needs the
// Shocker combat_inode (PC 0xF5), see core_ai_resource.cpp.
constexpr uint32_t XBOX_V10_SHOCKER_COMBAT_STATE = 0xE6;
constexpr uint32_t XBOX_V10_PED_DEFAULT_TRANS_STATE = 0x9F;
constexpr uint32_t XBOX_V10_PED_HIT_REACT_STATE = 0xA1;
constexpr uint32_t XBOX_V10_PED_SUBDUED_STATE = 0xA4;
constexpr uint32_t XBOX_V10_PED_IDLE_STATE = 0xA6;
constexpr uint32_t XBOX_V10_PLAY_DODGE_ANIM_STATE = 0xA7;
constexpr uint32_t XBOX_V10_PLR_LOCO_CRAWL_STATE = 0xAA;
constexpr uint32_t XBOX_V10_PLR_LOCO_CRAWL_TRANS_STATE = 0xAB;
constexpr uint32_t XBOX_V10_ATTACH_STATE = 0xF1;
constexpr uint32_t XBOX_V10_STATE_0F9 = 0xF9;
constexpr uint32_t XBOX_V10_PARKER_COMBAT_STATE = 0x103;
constexpr uint32_t XBOX_V10_PLAYER_COMBAT_STATE = 0x104;
constexpr uint32_t XBOX_V10_PREPARE_COMBO_STATE = 0x105;
constexpr uint32_t XBOX_V10_SPIDEY_COMBAT_STATE = 0x106;
constexpr uint32_t XBOX_V10_VENOM_COMBAT_STATE = 0x107;
constexpr uint32_t XBOX_V10_DEBUG_STATE = 0x108;
constexpr uint32_t XBOX_V10_STATE_115 = 0x115;
constexpr uint32_t XBOX_V10_HOSTAGE_VICTIM_STATE = 0x116;
constexpr uint32_t XBOX_V10_INTERACTION_STATE = 0x11B;
constexpr uint32_t XBOX_V10_PICK_UP_STATE = 0x11D;
constexpr uint32_t XBOX_V10_PUT_DOWN_STATE = 0x11E;
constexpr uint32_t XBOX_V10_JUMP_STATE = 0x11F;
// 0x120 is the v10 pole_swing_inode (PC 0x130); throw_state is 0x132.
constexpr uint32_t XBOX_V10_THROW_STATE = 0x132;
constexpr uint32_t XBOX_V10_POLE_SWING_STATE = 0x121;
constexpr uint32_t XBOX_V10_STD_PUPPET_TRANS_STATE = 0x12C;
constexpr uint32_t XBOX_V10_RUN_STATE = 0x12D;
constexpr uint32_t XBOX_V10_SWING_STATE = 0x12F;
constexpr uint32_t XBOX_V10_AIMED_THROW_STATE = 0x130;
constexpr uint32_t XBOX_V10_SPIDEY_BASE_STATE = 0x134;
constexpr uint32_t XBOX_V10_VENOM_BASE_STATE = 0x135;
constexpr uint32_t XBOX_V10_WEB_ZIP_STATE = 0x137;
constexpr uint32_t XBOX_V10_LAUNCH_LAYER_STATE = 0x13A;
constexpr uint32_t XBOX_V10_BIPED_IDLE_STATE = 0x141;
constexpr uint32_t XBOX_V10_NONPATHED_GOTO_STATE = 0x143;
constexpr uint32_t XBOX_V10_PATHED_GOTO_STATE = 0x144;
constexpr uint32_t XBOX_V10_GENERIC_TARGET_HERO_STATE = 0x150;
constexpr uint32_t XBOX_V10_HIT_REACT_STATE = 0x161;
constexpr uint32_t XBOX_V10_STD_DEFAULT_STATE_SET_BASE = 0x162;
constexpr uint32_t XBOX_V10_SUBDUED_STATE = 0x164;
constexpr uint32_t XBOX_V10_STD_FEAR_TRANS_STATE = 0x168;
constexpr uint32_t XBOX_V10_TRAFFIC_BASE_STATE = 0x194;
constexpr uint32_t XBOX_V10_UNIVERSAL_SOLDIER_NONCOMBAT_IDLE_STATE = 0x19B;
constexpr uint32_t XBOX_V10_UNIVERSAL_SOLDIER_TRANS_STATE = 0x19C;
constexpr uint32_t XBOX_V10_META_ANIM_SWING = 0x1D3;
constexpr uint32_t PC_STATE_06C = 0x6C;
constexpr uint32_t PC_FLEE_POI_GOTO_STATE = 0xA0;
constexpr uint32_t PC_FLEE_QUAD_PATH_STATE = 0xA1;
constexpr uint32_t PC_FLEE_TRAFFIC_PATH_STATE = 0xA2;
constexpr uint32_t PC_STATE_0A6 = 0xA6;
constexpr uint32_t PC_SHOCKER_COMBAT_STATE = 0xF6;
constexpr uint32_t PC_PED_DEFAULT_TRANS_STATE = 0xA9;
constexpr uint32_t PC_PED_HIT_REACT_STATE = 0xAB;
constexpr uint32_t PC_PED_SUBDUED_STATE = 0xAE;
constexpr uint32_t PC_PED_IDLE_STATE = 0xB1;
constexpr uint32_t PC_PLAY_DODGE_ANIM_STATE = 0xB2;
constexpr uint32_t PC_PLR_LOCO_CRAWL_STATE = 0xB5;
constexpr uint32_t PC_PLR_LOCO_CRAWL_TRANS_STATE = 0xB6;
constexpr uint32_t PC_ATTACH_STATE = 0x101;
constexpr uint32_t PC_STATE_109 = 0x109;
constexpr uint32_t PC_PARKER_COMBAT_STATE = 0x113;
constexpr uint32_t PC_PLAYER_COMBAT_STATE = 0x114;
constexpr uint32_t PC_PREPARE_COMBO_STATE = 0x115;
constexpr uint32_t PC_SPIDEY_COMBAT_STATE = 0x116;
constexpr uint32_t PC_VENOM_COMBAT_STATE = 0x117;
constexpr uint32_t PC_DEBUG_STATE = 0x118;
constexpr uint32_t PC_STATE_125 = 0x125;
constexpr uint32_t PC_HOSTAGE_VICTIM_STATE = 0x126;
constexpr uint32_t PC_INTERACTION_STATE = 0x12B;
constexpr uint32_t PC_PICK_UP_STATE = 0x12D;
constexpr uint32_t PC_PUT_DOWN_STATE = 0x12E;
constexpr uint32_t PC_JUMP_STATE = 0x12F;
constexpr uint32_t PC_THROW_STATE = 0x142;
constexpr uint32_t PC_POLE_SWING_STATE = 0x131;
constexpr uint32_t PC_STD_PUPPET_TRANS_STATE = 0x13C;
constexpr uint32_t PC_RUN_STATE = 0x13D;
constexpr uint32_t PC_SWING_STATE = 0x13F;
constexpr uint32_t PC_AIMED_THROW_STATE = 0x140;
constexpr uint32_t PC_SPIDEY_BASE_STATE = 0x144;
constexpr uint32_t PC_VENOM_BASE_STATE = 0x145;
constexpr uint32_t PC_WEB_ZIP_STATE = 0x147;
constexpr uint32_t PC_LAUNCH_LAYER_STATE = 0x14A;
constexpr uint32_t PC_BIPED_IDLE_STATE = 0x151;
constexpr uint32_t PC_NONPATHED_GOTO_STATE = 0x153;
constexpr uint32_t PC_PATHED_GOTO_STATE = 0x154;
constexpr uint32_t PC_GENERIC_TARGET_HERO_STATE = 0x160;
constexpr uint32_t PC_HIT_REACT_STATE = 0x171;
constexpr uint32_t PC_STD_DEFAULT_STATE_SET_BASE = 0x172;
constexpr uint32_t PC_SUBDUED_STATE = 0x174;
constexpr uint32_t PC_STD_FEAR_TRANS_STATE = 0x178;
constexpr uint32_t PC_TRAFFIC_BASE_STATE = 0x1A5;
constexpr uint32_t PC_UNIVERSAL_SOLDIER_NONCOMBAT_IDLE_STATE = 0x1AC;
constexpr uint32_t PC_UNIVERSAL_SOLDIER_TRANS_STATE = 0x1AD;
constexpr std::intptr_t PC_STATE_06C_VTABLE = 0x008751C8;
constexpr std::intptr_t PC_PED_DEFAULT_TRANS_STATE_VTABLE = 0x00875B50;
constexpr std::intptr_t PC_STD_DEFAULT_STATE_SET_BASE_VTABLE = 0x008750B8;
constexpr std::intptr_t PC_STATE_125_VTABLE = 0x00877000;
constexpr std::intptr_t PC_STD_PUPPET_TRANS_STATE_VTABLE = 0x008771E0;
constexpr std::intptr_t PC_SPIDEY_BASE_STATE_VTABLE = 0x00877534;
constexpr std::intptr_t PC_VENOM_BASE_STATE_VTABLE = 0x00877570;
constexpr std::intptr_t PC_STD_FEAR_TRANS_STATE_VTABLE = 0x00874D18;
constexpr std::intptr_t PC_UNIVERSAL_SOLDIER_TRANS_STATE_VTABLE = 0x00877AA8;
constexpr std::intptr_t PC_META_ANIM_SWING_VTABLE = 0x0087B918;

struct xbox_v10_state
{
    uint32_t type;
    uint8_t base[0x10];
    float field_14;
    uint32_t field_18;
    ai::state_trans_messages field_1C;
    bool field_20;
    uint8_t padding[3];
};

static_assert(sizeof(xbox_v10_state) == 0x24);

struct xbox_v10_meta_anim_swing
{
    uint32_t type;
    uint8_t fields[0x38];
    uint32_t padding;
};

static_assert(sizeof(xbox_v10_meta_anim_swing) == 0x40);
static_assert(offsetof(xbox_v10_meta_anim_swing, padding) == 0x3C);

constexpr uint16_t V10_STATE_TYPES[][2] = {
    {0x02D, 0x02E}, {0x033, 0x035}, {0x054, 0x05B}, {0x055, 0x05C},
    {0x062, 0x06A}, {0x063, 0x06B}, {0x065, 0x06D}, {0x066, 0x06E},
    {0x067, 0x06F}, {0x068, 0x070}, {0x069, 0x071}, {0x06A, 0x072},
    {0x06C, 0x074}, {0x06E, 0x076}, {0x06F, 0x077}, {0x071, 0x079},
    {0x072, 0x07A}, {0x074, 0x07C}, {0x075, 0x07D}, {0x076, 0x07E},
    {0x077, 0x07F}, {0x079, 0x081}, {0x07A, 0x082}, {0x07C, 0x084},
    {0x07D, 0x085}, {0x07E, 0x086}, {0x07F, 0x087}, {0x080, 0x088},
    {0x081, 0x089}, {0x082, 0x08A}, {0x083, 0x08C}, {0x084, 0x08E},
    {0x085, 0x08B}, {0x086, 0x08F}, {0x090, 0x099}, {0x092, 0x09B},
    {0x09A, 0x0A4},
    {0x0AC, 0x0B7}, {0x0AE, 0x0B9}, {0x0AF, 0x0BA}, {0x0B0, 0x0BB},
    {0x0B2, 0x0BD}, {0x0B3, 0x0C0}, {0x0B4, 0x0C1}, {0x0B5, 0x0C2},
    {0x0B6, 0x0C3}, {0x0B7, 0x0C4}, {0x0B8, 0x0C5}, {0x0BB, 0x0C8},
    {0x0C0, 0x0CD}, {0x0C1, 0x0CE}, {0x0C3, 0x0D0}, {0x0C4, 0x0D1},
    {0x0C6, 0x0D3}, {0x0C8, 0x0D5}, {0x0C9, 0x0D6}, {0x0EB, 0x0FB},
    {0x0EC, 0x0FC}, {0x0ED, 0x0FD}, {0x0EE, 0x0FE}, {0x0EF, 0x0FF},
    {0x0F0, 0x100}, {0x0F3, 0x103}, {0x0F4, 0x104}, {0x0F5, 0x105},
    {0x0F6, 0x106}, {0x0F8, 0x108}, {0x0FB, 0x10B}, {0x0FC, 0x10C},
    {0x0FD, 0x10D}, {0x0FE, 0x10E}, {0x100, 0x110}, {0x101, 0x111},
    {0x102, 0x112}, {0x10B, 0x11B}, {0x112, 0x122}, {0x113, 0x123},
    {0x114, 0x124}, {0x11A, 0x12A}, {0x122, 0x132}, {0x123, 0x133},
    {0x124, 0x134}, {0x125, 0x135}, {0x127, 0x137}, {0x129, 0x139},
    {0x132, 0x142}, {0x138, 0x148}, {0x139, 0x149}, {0x13F, 0x14F},
    {0x158, 0x168}, {0x15A, 0x16A}, {0x15B, 0x16B}, {0x15F, 0x16F},
    {0x160, 0x170}, {0x165, 0x175}, {0x166, 0x176}, {0x169, 0x179},
    {0x16A, 0x17A}, {0x16C, 0x17C}, {0x16E, 0x17E}, {0x17B, 0x18B},
    {0x17E, 0x18E}, {0x17F, 0x190}, {0x180, 0x191}, {0x183, 0x194},
    {0x184, 0x195}, {0x186, 0x197}, {0x188, 0x199}, {0x199, 0x1AA},
    {0x19A, 0x1AB}, {0x1A0, 0x1B2}, {0x1A4, 0x1B6}, {0x1A5, 0x1B7},
    {0x1A7, 0x1B9}, {0x1AA, 0x1BC}, {0x201, 0x217},
};

enum class v10_type_source
{
    explicit_case,
    state_table,
    mash_table,
    unmapped,
};

const char *v10_type_source_name(v10_type_source source)
{
    switch (source)
    {
    case v10_type_source::explicit_case:
        return "case";
    case v10_type_source::state_table:
        return "state-table";
    case v10_type_source::mash_table:
        return "mash-table";
    case v10_type_source::unmapped:
    default:
        return "UNMAPPED";
    }
}

uint32_t translate_v10_state_type_impl(uint32_t type, v10_type_source &source);

uint32_t translate_v10_state_type(uint32_t type)
{
    v10_type_source source;
    return translate_v10_state_type_impl(type, source);
}

uint32_t translate_v10_state_type_impl(uint32_t type, v10_type_source &source)
{
    source = v10_type_source::explicit_case;

    switch (type)
    {
    case XBOX_V10_STATE_064:
        return PC_STATE_06C;
    case XBOX_V10_FLEE_POI_GOTO_STATE:
        return PC_FLEE_POI_GOTO_STATE;
    case XBOX_V10_FLEE_QUAD_PATH_STATE:
        return PC_FLEE_QUAD_PATH_STATE;
    case XBOX_V10_FLEE_TRAFFIC_PATH_STATE:
        return PC_FLEE_TRAFFIC_PATH_STATE;
    case XBOX_V10_SHOCKER_COMBAT_STATE:
        return PC_SHOCKER_COMBAT_STATE;
    case XBOX_V10_STATE_09C:
        return PC_STATE_0A6;
    case XBOX_V10_PED_DEFAULT_TRANS_STATE:
        return PC_PED_DEFAULT_TRANS_STATE;
    case XBOX_V10_PED_HIT_REACT_STATE:
        return PC_PED_HIT_REACT_STATE;
    case XBOX_V10_PED_SUBDUED_STATE:
        return PC_PED_SUBDUED_STATE;
    case XBOX_V10_PED_IDLE_STATE:
        return PC_PED_IDLE_STATE;
    case XBOX_V10_PLAY_DODGE_ANIM_STATE:
        return PC_PLAY_DODGE_ANIM_STATE;
    case XBOX_V10_PLR_LOCO_CRAWL_STATE:
        return PC_PLR_LOCO_CRAWL_STATE;
    case XBOX_V10_PLR_LOCO_CRAWL_TRANS_STATE:
        return PC_PLR_LOCO_CRAWL_TRANS_STATE;
    case XBOX_V10_ATTACH_STATE:
        return PC_ATTACH_STATE;
    case XBOX_V10_STATE_0F9:
        return PC_STATE_109;
    case XBOX_V10_PARKER_COMBAT_STATE:
        return PC_PARKER_COMBAT_STATE;
    case XBOX_V10_PLAYER_COMBAT_STATE:
        return PC_PLAYER_COMBAT_STATE;
    case XBOX_V10_PREPARE_COMBO_STATE:
        return PC_PREPARE_COMBO_STATE;
    case XBOX_V10_SPIDEY_COMBAT_STATE:
        return PC_SPIDEY_COMBAT_STATE;
    case XBOX_V10_VENOM_COMBAT_STATE:
        return PC_VENOM_COMBAT_STATE;
    case XBOX_V10_DEBUG_STATE:
        return PC_DEBUG_STATE;
    case XBOX_V10_STATE_115:
        return PC_STATE_125;
    case XBOX_V10_HOSTAGE_VICTIM_STATE:
        return PC_HOSTAGE_VICTIM_STATE;
    case XBOX_V10_INTERACTION_STATE:
        return PC_INTERACTION_STATE;
    case XBOX_V10_PICK_UP_STATE:
        return PC_PICK_UP_STATE;
    case XBOX_V10_PUT_DOWN_STATE:
        return PC_PUT_DOWN_STATE;
    case XBOX_V10_JUMP_STATE:
        return PC_JUMP_STATE;
    case XBOX_V10_THROW_STATE:
        return PC_THROW_STATE;
    case XBOX_V10_POLE_SWING_STATE:
        return PC_POLE_SWING_STATE;
    case XBOX_V10_STD_PUPPET_TRANS_STATE:
        return PC_STD_PUPPET_TRANS_STATE;
    case XBOX_V10_RUN_STATE:
        return PC_RUN_STATE;
    case XBOX_V10_SWING_STATE:
        return PC_SWING_STATE;
    case XBOX_V10_AIMED_THROW_STATE:
        return PC_AIMED_THROW_STATE;
    case XBOX_V10_SPIDEY_BASE_STATE:
        return PC_SPIDEY_BASE_STATE;
    case XBOX_V10_VENOM_BASE_STATE:
        return PC_VENOM_BASE_STATE;
    case XBOX_V10_WEB_ZIP_STATE:
        return PC_WEB_ZIP_STATE;
    case XBOX_V10_LAUNCH_LAYER_STATE:
        return PC_LAUNCH_LAYER_STATE;
    case XBOX_V10_BIPED_IDLE_STATE:
        return PC_BIPED_IDLE_STATE;
    case XBOX_V10_NONPATHED_GOTO_STATE:
        return PC_NONPATHED_GOTO_STATE;
    case XBOX_V10_PATHED_GOTO_STATE:
        return PC_PATHED_GOTO_STATE;
    case XBOX_V10_GENERIC_TARGET_HERO_STATE:
        return PC_GENERIC_TARGET_HERO_STATE;
    case XBOX_V10_HIT_REACT_STATE:
        return PC_HIT_REACT_STATE;
    case XBOX_V10_STD_DEFAULT_STATE_SET_BASE:
        return PC_STD_DEFAULT_STATE_SET_BASE;
    case XBOX_V10_SUBDUED_STATE:
        return PC_SUBDUED_STATE;
    case XBOX_V10_STD_FEAR_TRANS_STATE:
        return PC_STD_FEAR_TRANS_STATE;
    case XBOX_V10_TRAFFIC_BASE_STATE:
        return PC_TRAFFIC_BASE_STATE;
    case XBOX_V10_UNIVERSAL_SOLDIER_NONCOMBAT_IDLE_STATE:
        return PC_UNIVERSAL_SOLDIER_NONCOMBAT_IDLE_STATE;
    case XBOX_V10_UNIVERSAL_SOLDIER_TRANS_STATE:
        return PC_UNIVERSAL_SOLDIER_TRANS_STATE;
    default:
        break;
    }

    for (const auto &mapping : V10_STATE_TYPES)
    {
        if (mapping[0] == type)
        {
            source = v10_type_source::state_table;
            return mapping[1];
        }
    }

    // Complete Xbox v10 -> PC table.  Without it every state type missing from
    // the lists above was used unchanged, i.e. as the PC class that happens to
    // share the number (an idle state, an inode, ...), which froze the ped or
    // enemy that entered it.
    uint32_t pc_type = 0;
    if (xbpack::v10_types::find_pc_mash_type(type, pc_type))
    {
        source = v10_type_source::mash_table;
        return pc_type;
    }

    source = v10_type_source::unmapped;
    return type;
}

// One log line per distinct Xbox state type, so a runtime log shows which v10
// states the loaded graphs use and how each was translated.
void log_v10_state_type(const ai::mashed_state &state, uint32_t xbox_type)
{
    static bool seen[0x400] {};
    if (xbox_type < sizeof(seen) && seen[xbox_type])
        return;
    if (xbox_type < sizeof(seen))
        seen[xbox_type] = true;

    v10_type_source source;
    const auto pc_type = translate_v10_state_type_impl(xbox_type, source);
    sp_log("[xbpack] v10 state '%s' type 0x%03X -> PC 0x%03X (%s)",
           state.field_C.to_string(),
           static_cast<unsigned>(xbox_type),
           static_cast<unsigned>(pc_type),
           v10_type_source_name(source));
}
#endif

struct xbox_combo_system_move
{
#ifdef OPENUSM_XBPACK_V10
    uint8_t data[0xC8];
#else
    uint8_t through_results_keys[0x14];
    uint32_t string_size;
    uint32_t string_guts;
    uint32_t string_allocator;
    uint8_t results_tail[0x5C];
    uint8_t requirements_and_move_tail[0x48];
#endif
};

struct xbox_attach_node
{
    uint8_t field_0[0x10];
    uint32_t field_10[3];
    uint32_t field_1C[2];
};

static_assert(sizeof(xbox_attach_node) == 0x24);

#ifdef OPENUSM_XBPACK_V10
static_assert(sizeof(xbox_combo_system_move) == 0xC8);
#else
static_assert(sizeof(xbox_combo_system_move) == 0xC4);
static_assert(offsetof(xbox_combo_system_move, string_size) == 0x14);
static_assert(offsetof(xbox_combo_system_move, results_tail) == 0x20);
static_assert(offsetof(xbox_combo_system_move, requirements_and_move_tail) == 0x7C);
#endif

void *allocate_from_pc_heap(size_t size)
{
    return reinterpret_cast<void *>(CDECL_CALL(PC_OPERATOR_NEW, size));
}

void *allocate_from_pc_allocator(size_t size)
{
    return reinterpret_cast<void *>(CDECL_CALL(PC_MEM_ALLOC, size));
}

#ifdef OPENUSM_XBPACK_V10
als::als_meta_anim_base *expand_v10_meta_anim_swing(
    const xbox_v10_meta_anim_swing &source)
{
    auto *result = static_cast<als::als_meta_anim_swing *>(
        allocate_from_pc_heap(sizeof(als::als_meta_anim_swing)));
    assert(result != nullptr);
    std::memset(result, 0, sizeof(*result));
    std::memcpy(result, &source, offsetof(xbox_v10_meta_anim_swing, padding));
    result->m_vtbl = PC_META_ANIM_SWING_VTABLE;
    return result;
}

void detach_v10_meta_anim_swing(als::als_meta_anim_swing &anim)
{
    auto &keys = anim.field_28;
    if (keys.m_size <= 0)
    {
        keys.m_data = nullptr;
        keys.field_C = 0;
        keys.field_10 = false;
        keys.field_0 = 0;
        return;
    }

    assert(keys.m_data != nullptr);

    auto **data = static_cast<als::meta_key_anim **>(
        allocate_from_pc_allocator(
            sizeof(als::meta_key_anim *) * keys.m_size));
    assert(data != nullptr);
    std::memcpy(data, keys.m_data,
                sizeof(als::meta_key_anim *) * keys.m_size);

    keys.m_data = data;
    keys.field_C = keys.m_size;
    keys.field_10 = false;
    keys.field_0 = 0;
}

template<typename T>
T *expand_v10_state_base(const xbox_v10_state &source, std::intptr_t vtable)
{
    auto *result = static_cast<T *>(allocate_from_pc_allocator(sizeof(T)));
    assert(result != nullptr);
    std::memset(result, 0, sizeof(T));

    result->m_vtbl = vtable;
    std::memcpy(reinterpret_cast<uint8_t *>(result) + 4,
                source.base,
                sizeof(source.base));
    result->field_14 = nullptr;
    result->field_18 = nullptr;
    return result;
}

ai::base_state *expand_v10_state(const xbox_v10_state &source)
{
    switch (source.type)
    {
    case XBOX_V10_STATE_064:
        return expand_v10_state_base<ai::base_state>(
            source, PC_STATE_06C_VTABLE);

    case XBOX_V10_PED_DEFAULT_TRANS_STATE:
        return expand_v10_state_base<ai::base_state>(
            source, PC_PED_DEFAULT_TRANS_STATE_VTABLE);

    case XBOX_V10_STATE_115:
        return expand_v10_state_base<ai::base_state>(
            source, PC_STATE_125_VTABLE);

    case XBOX_V10_STD_PUPPET_TRANS_STATE:
        return expand_v10_state_base<ai::base_state>(
            source, PC_STD_PUPPET_TRANS_STATE_VTABLE);

    case XBOX_V10_SPIDEY_BASE_STATE:
        return expand_v10_state_base<ai::base_state>(
            source, PC_SPIDEY_BASE_STATE_VTABLE);

    case XBOX_V10_VENOM_BASE_STATE:
        return expand_v10_state_base<ai::base_state>(
            source, PC_VENOM_BASE_STATE_VTABLE);

    case XBOX_V10_STD_DEFAULT_STATE_SET_BASE:
        return expand_v10_state_base<ai::base_state>(
            source, PC_STD_DEFAULT_STATE_SET_BASE_VTABLE);

    case XBOX_V10_STD_FEAR_TRANS_STATE:
        return expand_v10_state_base<ai::base_state>(
            source, PC_STD_FEAR_TRANS_STATE_VTABLE);

    case XBOX_V10_UNIVERSAL_SOLDIER_TRANS_STATE:
        return expand_v10_state_base<ai::base_state>(
            source, PC_UNIVERSAL_SOLDIER_TRANS_STATE_VTABLE);

    default:
        assert(false && "Unsupported v10 ai::base_state type");
        return nullptr;
    }
}
#endif

combo_system_move *expand_combo_move(const xbox_combo_system_move &source)
{
    auto *storage = allocate_from_pc_heap(sizeof(combo_system_move));
    assert(storage != nullptr);

    auto *result = new (storage) combo_system_move {};
#ifdef OPENUSM_XBPACK_V10
    std::memcpy(result, source.data, sizeof(source.data));
#else
    auto *bytes = reinterpret_cast<uint8_t *>(result);

    std::memcpy(bytes, source.through_results_keys, sizeof(source.through_results_keys));
    *reinterpret_cast<uint32_t *>(bytes + 0x14) = 0;
    std::memcpy(bytes + 0x18, &source.string_size, 0x0C);
    std::memcpy(bytes + 0x24, source.results_tail, sizeof(source.results_tail));
    std::memcpy(bytes + 0x80,
                source.requirements_and_move_tail,
                sizeof(source.requirements_and_move_tail));
#endif

    return result;
}

void detach_combo_move_links_from_mash(combo_system_move &move)
{
    auto &links = move.field_80.field_30;
    if (links.m_size <= 0)
    {
        links.m_data = nullptr;
        links.field_C = 0;
        links.field_0 = 0;
        return;
    }

    assert(links.m_data != nullptr);

    auto **copies = static_cast<combo_system_move::link_info **>(
        allocate_from_pc_allocator(
            sizeof(combo_system_move::link_info *) * links.m_size));
    assert(copies != nullptr);

    for (int i = 0; i < links.m_size; ++i)
    {
        auto *source = links.m_data[i];
        if (source == nullptr)
        {
            copies[i] = nullptr;
            continue;
        }

        auto *copy = static_cast<combo_system_move::link_info *>(
            allocate_from_pc_heap(sizeof(combo_system_move::link_info)));
        assert(copy != nullptr);
        std::memcpy(copy, source, sizeof(*copy));
        copies[i] = copy;
    }

    links.m_data = copies;
    links.field_C = links.m_size;
    links.field_10 = true;
    links.field_0 = 0;
}

void detach_combo_move_string_from_mash(combo_system_move &move)
{
    auto &string = move.field_4.field_10;
    string.field_0 = 0;

    if (string.empty())
    {
        return;
    }

    assert(string.guts != nullptr);
    assert(string.size() <= static_cast<int>(MAX_MSTRING_LENGTH));

    const auto allocation_size = static_cast<size_t>(string.size()) + 1;
    auto *copy = static_cast<char *>(allocate_from_pc_heap(allocation_size));
    assert(copy != nullptr);
    std::memcpy(copy, string.guts, allocation_size);

    string.guts = copy;
    string.field_C = nullptr;
}
}
#endif

#ifdef OPENUSM_XBPACK_V10
uint32_t xbpack::pc_state_type(uint32_t type)
{
    return translate_v10_state_type(type);
}
#endif

template<>
void mVector<sound_alias>::destruct_mashed_class()
{
    if constexpr (0)
    {
        //this->clear();
    }
    else
    {
        THISCALL(0x005D6EE0, this);
    }
}

template<>
void mVector<als::layer_state_machine_shared>::destruct_mashed_class()
{
    THISCALL(0x004B01C0, this);
}

template<>
void mVector<als::als_meta_anim_base>::destruct_mashed_class()
{
    THISCALL(0x004B01C0, this);
}

template<>
void mVector<PanelAnim>::custom_unmash(mash_info_struct *a1, [[maybe_unused]] void *a3)
{
    TRACE("mVector<PanelAnim>::custom_unmash");
#if OPENUSM_XBOX_MASH_FORMAT
    this->field_C = this->m_size;
    if ( this->m_size <= 0 )
    {
        this->m_data = nullptr;
    }
    else
#else
    if ( this->m_data != nullptr )
#endif
    {
        this->m_data = (PanelAnim **) a1->read_from_buffer(
#if OPENUSM_XBOX_MASH_FORMAT
            mash::NORMAL_BUFFER,
#endif
                4 * this->m_size, 4);
        for ( auto i = 0; i < this->m_size; ++i )
        {
            auto &a1a = this->m_data[i];
            auto *v6 = bit_cast<PanelAnim *>(a1->read_from_buffer(
#if OPENUSM_XBOX_MASH_FORMAT
                mash::NORMAL_BUFFER,
#endif
                sizeof(PanelAnim), 4));

            a1a = v6;
            a1->unmash_class_in_place(v6->field_0, v6);
        }
    }

    this->field_0 = (int)&a1->mash_image_ptr[0][a1->buffer_size_used[0] - (uint32_t) this];
}

template<>
void mVector<PanelAnimKeyframe>::custom_unmash(mash_info_struct *a2, [[maybe_unused]] void *a3)
{
    TRACE("mVector<PanelAnimKeyframe>::custom_unmash");

#if OPENUSM_XBOX_MASH_FORMAT
    this->field_C = this->m_size;
    if ( this->m_size <= 0 )
    {
        this->m_data = nullptr;
    }
    else
#else
    if ( this->m_data != nullptr )
#endif
    {
        this->m_data = (PanelAnimKeyframe **) a2->read_from_buffer(
#if OPENUSM_XBOX_MASH_FORMAT
            mash::NORMAL_BUFFER,
#endif 
            4 * this->m_size, 4);

        for ( auto i = 0; i < this->m_size; ++i )
        {
            auto &v5 = this->m_data[i];
            v5 = (PanelAnimKeyframe *) a2->read_from_buffer(
#if OPENUSM_XBOX_MASH_FORMAT
                mash::NORMAL_BUFFER,
#endif 
                    sizeof(PanelAnimKeyframe), 4);
        }
    }

    this->field_0 = (int)&a2->mash_image_ptr[0][a2->buffer_size_used[0] - (uint32_t)this];
}

template<>
void mVector<PanelAnimFile>::custom_unmash(mash_info_struct *a2, [[maybe_unused]] void *a3)
{
#if OPENUSM_XBOX_MASH_FORMAT
    this->field_C = this->m_size;
    if ( this->m_size <= 0 )
    {
        this->m_data = nullptr;
    }
    else
#else
    if ( this->m_data != nullptr )
#endif
    {
        this->m_data = (PanelAnimFile **) a2->read_from_buffer(
#if OPENUSM_XBOX_MASH_FORMAT
            mash::NORMAL_BUFFER,
#endif 
                4 * this->m_size, 4);
        for ( auto i = 0; i < this->m_size; ++i )
        {
            auto &a1 = this->m_data[i];
            auto *v6 = a2->read_from_buffer(
#if OPENUSM_XBOX_MASH_FORMAT
            mash::NORMAL_BUFFER,
#endif 
                    sizeof(PanelAnimFile), 4);
            a1 = (PanelAnimFile *)v6;
            a2->unmash_class_in_place(a1->field_0, v6);
        }
    }

    this->field_0 = (int)&a2->mash_image_ptr[0][a2->buffer_size_used[0] - (uint32_t) this];
}

template<>
void mVector<PanelQuadSection>::custom_unmash(mash_info_struct *a2, [[maybe_unused]] void *a3)
{
    TRACE("mVector<PanelQuadSection>::custom_unmash", std::to_string(this->m_size).c_str());

#if OPENUSM_XBOX_MASH_FORMAT
    this->field_C = this->m_size;
    if ( this->m_size <= 0 )
    {
        this->m_data = nullptr;
    }
    else
#else
    if ( this->m_data != nullptr )
#endif
    {
        this->m_data = (PanelQuadSection **) a2->read_from_buffer(
#if OPENUSM_XBOX_MASH_FORMAT
                mash::NORMAL_BUFFER,
#endif 
                4 * this->m_size, 4);
        for ( auto i = 0; i < this->m_size; ++i )
        {
            auto &v5 = this->m_data[i];
            auto *v6 = (PanelQuadSection *) a2->read_from_buffer(
#if OPENUSM_XBOX_MASH_FORMAT
                mash::NORMAL_BUFFER,
#endif 
                    sizeof(PanelQuadSection), 4);

            v5 = v6;
            a2->unmash_class_in_place(v6->field_14, v6);
        }
    }

    this->field_0 = (int)&a2->mash_image_ptr[0][a2->buffer_size_used[0] - (uint32_t) this];
}

template<>
void mVector<FEText>::custom_unmash(mash_info_struct *a2, [[maybe_unused]] void *a3)
{
    TRACE("mVector<FEText>::custom_unmash");

#if OPENUSM_XBOX_MASH_FORMAT && !defined(OPENUSM_XBPACK_V10)

    this->field_C = this->m_size;
    if ( this->m_size <= 0 )
    {
        this->m_data = nullptr;
    }
    else
    {
        this->m_data =
            bit_cast<FEText **>(a2->read_from_buffer(mash::NORMAL_BUFFER, 4 * this->m_size, 4));

        sp_log("size = %d", this->size());
        for ( auto i = 0; i < this->m_size; ++i )
        {
            TRACE("mash_info_struct::unmash_class<FEText>");
            auto &v5 = this->m_data[i];

            {
                constexpr auto mash_size = 0x60;

                struct fetext {
                    struct string_t {
                        uint32_t m_size;
                        char *guts;
                        int field_8;
                    };

                    char field_0[0x1C];
                    string_t field_1C;
                    int field_28[2];
                    int field_30[2];
                    int field_38[2];
                    int field_40[2];
                    int field_48;
                    string_t field_4C;

                    struct {
                        int field_0[2];
                    } field_58;
                };

                const fetext *v6 =
                    CAST(v6, a2->read_from_buffer(mash::NORMAL_BUFFER, mash_size, 0));

                sp_log("0x%08X", v6);
                VALIDATE_SIZE(fetext, mash_size);
                VALIDATE_OFFSET(fetext, field_4C, 0x4C);

                constexpr auto xbox_text_geometry_size =
                    offsetof(fetext, field_4C) - offsetof(fetext, field_28);
                static_assert(xbox_text_geometry_size == 0x24);

                v5 = static_cast<FEText *>(calloc(1, sizeof(FEText)));

                {
                    std::memcpy(v5, v6, sizeof(v6->field_0));
                    std::memcpy(&v5->field_1C.m_size, &v6->field_1C, sizeof(v6->field_1C));
                    std::memcpy(&v5->field_2C, &v6->field_28, xbox_text_geometry_size);
                    std::memcpy(&v5->field_50.m_size, &v6->field_4C, sizeof(v6->field_4C));
                    std::memcpy(&v5->field_60, &v6->field_58, sizeof(v6->field_58));
                }

                mash_virtual_base::fixup_vtable(v5);

                {
                    //sp_log("0x%08X", tmp->m_vtbl);
                //    assert(v5->m_vtbl == 0x00879FE0 ||
                       //    v5->m_vtbl == 0x0087A0F0 ||
                       //    v5->m_vtbl == 0x0087AE58);
                }

                const auto v7 = v5->get_mash_sizeof();
                //sp_log("mash_size = 0x%X", v7);

                if (v7 == 0x7C)
                {
                    struct floatingtext
                    {
                        fetext base {};
                        char field_60[0x1C];
                    } *text = CAST(text, v6);

                    VALIDATE_SIZE(floatingtext, 0x7C);

                    auto *tmp = calloc(1, sizeof(FEFloatingText));
                    std::memcpy(tmp, v5, sizeof(FEText));
                    std::memcpy(bit_cast<char *>(tmp) + sizeof(FEText),
                                text->field_60,
                                sizeof(text->field_60));

                    free(v5);
                    v5 = static_cast<FEText *>(tmp);
                }
                else if (v7 == 0x98)
                {
                    struct multilinetext
                    {
                        fetext base {};
                        char field_60[0x38];
                    } *text = CAST(text, v6);

                    VALIDATE_SIZE(multilinetext, 0x98);

                    auto *tmp = calloc(1, sizeof(FEMultiLineText));
                    std::memcpy(tmp, v5, sizeof(FEText));
                    std::memcpy(bit_cast<char *>(tmp) + sizeof(FEText), text->field_60, sizeof(text->field_60));

                    free(v5);
                    v5 = static_cast<FEText *>(tmp);

                }

                a2->advance_buffer(mash::NORMAL_BUFFER, v7 - mash_size);
            }

            v5->unmash(a2, nullptr);
        }
    }

#else

    if ( this->m_data != nullptr )
    {
        this->m_data = bit_cast<FEText **>(a2->read_from_buffer(4 * this->m_size, 4));

        sp_log("size = %d", this->size());
        for ( auto i = 0; i < this->m_size; ++i )
        {
            TRACE("mash_info_struct::unmash_class<FEText>");
            sp_log("i = %d", i);
            auto &v5 = this->m_data[i];
            auto *v6 = a2->read_from_buffer(sizeof(FEText), 0);
            sp_log("0x%08X", v6);

            v5 = CAST(v5, v6);

            sp_log("%d %d", v5->field_1C.m_size, v5->field_50.m_size);
            mash_virtual_base::fixup_vtable(v6);

            {
                struct {
                    int m_vtbl;
                } *tmp = CAST(tmp, v6);

                //sp_log("0x%08X", tmp->m_vtbl);
                assert(tmp->m_vtbl == 0x00879FE0 || tmp->m_vtbl == 0x0087AE58);
            }

            auto v7 = v5->get_mash_sizeof();
            a2->advance_buffer(v7 - sizeof(FEText));

            v5->unmash(a2, nullptr);
        }
    }
#endif

    this->field_0 = (int)&a2->mash_image_ptr[0][a2->buffer_size_used[0] - (uint32_t) this];
}


template<>
void mVector<PanelQuad>::custom_unmash(mash_info_struct *a2, [[maybe_unused]] void *a3)
{
    TRACE("mVector<PanelQuad>::custom_unmash", std::to_string(this->m_size).c_str());

#if OPENUSM_XBOX_MASH_FORMAT && !defined(OPENUSM_XBPACK_V10)
    this->field_C = this->m_size;
    if ( this->m_size <= 0 )
    {
        this->m_data = nullptr;
    }
    else
    {
        this->m_data =
            bit_cast<PanelQuad **>(a2->read_from_buffer(mash::NORMAL_BUFFER, 4 * this->m_size, 4));

        for ( auto i = 0; i < this->m_size; ++i )
        {
            sp_log("i = %d", i);
            TRACE("mash_info_struct::unmash_class<PanelQuad>");
            auto unmash_class = [](mash_info_struct *self, PanelQuad *&a2)
            {
                [](mash_info_struct *a2, PanelQuad *&v5)
                {
                    constexpr auto mash_size = 0x48;

                    struct {
                        char field_0[0x3C];
                        struct {
                            int m_size;
                            char *guts;
                            int field_8;
                        } field_3C;
                    } *v6 = CAST(v6, a2->read_from_buffer(mash::NORMAL_BUFFER, mash_size, 0));

                    v5 = static_cast<PanelQuad *>(calloc(1, sizeof(PanelQuad)));

                    {
                        //sp_log("0x%08X", v6->field_3C.guts);
                        std::memcpy(v5, v6, sizeof(v6->field_0));
                        std::memcpy(&v5->field_3C.m_size, &v6->field_3C, sizeof(v6->field_3C));
                    }
                   
                    mash_virtual_base::fixup_vtable(v5);

                    const auto v7 = v5->get_mash_sizeof();

                    a2->advance_buffer(mash::NORMAL_BUFFER, v7 - mash_size);
                }(self, a2);

                a2->unmash(self, nullptr);
            };

            unmash_class(a2, this->m_data[i]);
        }
    }

#else
    if ( this->m_data != nullptr )
    {
        this->m_data = bit_cast<PanelQuad **>(a2->read_from_buffer(4 * this->m_size, 4));

        for ( auto i = 0; i < this->m_size; ++i )
        {
            sp_log("i = %d", i);
 
            auto unmash_class = [](mash_info_struct *self, PanelQuad *&a2)
            {
                [](mash_info_struct *a2, PanelQuad *&v5)
                {
                    constexpr auto mash_size = sizeof(PanelQuad);

                    auto *v6 = a2->read_from_buffer(mash_size, 0);

                    v5 = CAST(v5, v6);
                   
                    mash_virtual_base::fixup_vtable(v6);

                    {
                        struct {
                            int m_vtbl;
                        } *tmp = CAST(tmp, v6);

                        assert(tmp->m_vtbl == 0x0087B990);
                    }

                    const auto v7 = v5->get_mash_sizeof();

                    a2->advance_buffer(v7 - mash_size);
                }(self, a2);

                a2->unmash(self, nullptr);
            };

            unmash_class(a2, this->m_data[i]);
        }
    }
#endif

    this->field_0 = (int)&a2->mash_image_ptr[0][a2->buffer_size_used[0] - (uint32_t) this];
}

template<>
void mVector<sound_alias>::custom_unmash(mash_info_struct *a2, [[maybe_unused]] void *a3)
{
    TRACE("mVector<sound_alias>::custom_unmash");

#if OPENUSM_XBOX_MASH_FORMAT
    this->field_C = this->m_size;
    if ( this->m_size <= 0 )
    {
        this->m_data = nullptr;
    }
    else
#else
    if ( this->m_data != nullptr )
#endif
    {
        this->m_data = (sound_alias **) a2->read_from_buffer(
#if OPENUSM_XBOX_MASH_FORMAT
            mash::NORMAL_BUFFER,
#endif
            4 * this->m_size, 4);

        for (auto i = 0; i < this->m_size; ++i )
        {
            auto &a2a = this->m_data[i];
            auto *v6 = (sound_alias *) a2->read_from_buffer(
#if OPENUSM_XBOX_MASH_FORMAT
                mash::NORMAL_BUFFER,
#endif 
                sizeof(sound_alias), 4);

            a2a = v6;
            a2->unmash_class_in_place(v6->field_0, v6);
            a2->unmash_class_in_place(v6->field_4, v6);
        }
    }

    this->field_0 = (int)&a2->mash_image_ptr[0][a2->buffer_size_used[0] - (uint32_t) this];
}

template<>
void mVector<als::layer_state_machine_shared>::custom_unmash(mash_info_struct *a2, [[maybe_unused]] void *a3)
{
    TRACE("mVector<als::layer_state_machine_shared>::custom_unmash");

#if OPENUSM_XBOX_MASH_FORMAT
    this->field_C = this->m_size;
    if (this->m_size <= 0)
    {
        this->m_data = nullptr;
    }
    else
#else
    if ( this->m_data != nullptr )
#endif

    {
        this->m_data = (als::layer_state_machine_shared **) a2->read_from_buffer(
#if OPENUSM_XBOX_MASH_FORMAT
            mash::NORMAL_BUFFER,
#endif 
                4 * this->m_size, 4);
        for ( auto i = 0; i < this->m_size; ++i )
        {

            auto &v5 = this->m_data[i];

            {
                auto *v6 = a2->read_from_buffer(
#if OPENUSM_XBOX_MASH_FORMAT
                    mash::NORMAL_BUFFER,
#endif
                    sizeof(als::layer_state_machine_shared), 0);

                v5 = (als::layer_state_machine_shared *)v6;
                mash_virtual_base::fixup_vtable(v6);

                {
                    struct {
                        int m_vtbl;
                    } *tmp = CAST(tmp, v6);

                    assert(tmp->m_vtbl == 0x0087E3A4);
                }

                auto v7 = v5->get_mash_sizeof();
                a2->advance_buffer(
#if OPENUSM_XBOX_MASH_FORMAT
                    mash::NORMAL_BUFFER,
#endif 
                    v7 - sizeof(als::layer_state_machine_shared));
            }

            v5->unmash(a2, nullptr);
        }
    }

    this->field_0 = (int)&a2->mash_image_ptr[0][a2->buffer_size_used[0] - (uint32_t)this];
}

template<>
void mVector<als::state>::custom_unmash(mash_info_struct *a2, [[maybe_unused]] void *a3)
{
    TRACE("mVector<als::state>::custom_unmash");

#if OPENUSM_XBOX_MASH_FORMAT
    this->field_C = this->m_size;
    if (this->m_size <= 0)
    {
        this->m_data = nullptr;
    }
    else
#else
    if ( this->m_data != nullptr )
#endif
    {
        this->m_data = (als::state **) a2->read_from_buffer(
#if OPENUSM_XBOX_MASH_FORMAT
                mash::NORMAL_BUFFER,
#endif 
                4 * this->m_size, 4);

        for ( auto i = 0; i < this->m_size; ++i )
        {
            auto &v5 = this->m_data[i];
            auto *v6 = a2->read_from_buffer(
#if OPENUSM_XBOX_MASH_FORMAT
                mash::NORMAL_BUFFER,
#endif 
                sizeof(als::state), 0);
            v5 = (als::state *)v6;

            mash_virtual_base::fixup_vtable(v5);

            assert(v5->m_vtbl == 0x0087E1D8 || v5->m_vtbl == 0x0087E214);
            auto v7 = v5->get_mash_sizeof();
            a2->advance_buffer(
#if OPENUSM_XBOX_MASH_FORMAT
                    mash::NORMAL_BUFFER,
#endif 
                    v7 - 0x14);

            v5->unmash(a2, nullptr);
        }
    }

    this->field_0 = (int)&a2->mash_image_ptr[0][a2->buffer_size_used[0] - (DWORD) this];
}

template<>
void mVector<als::als_meta_anim_base>::custom_unmash(mash_info_struct *a2, void *a3)
{
    TRACE("mVector<als::als_meta_anim_base>::custom_unmash");

#if OPENUSM_XBOX_MASH_FORMAT
    this->field_C = this->m_size;
    if (this->m_size <= 0)
    {
        this->m_data = nullptr;
    }
    else
#else
    if ( this->m_data != nullptr )
#endif
    {
        this->m_data = (als::als_meta_anim_base **) a2->read_from_buffer(
#if OPENUSM_XBOX_MASH_FORMAT
            mash::NORMAL_BUFFER,
#endif
            4 * this->m_size, 4);
        for ( auto i = 0; i < this->m_size; ++i )
        {
#ifdef OPENUSM_XBPACK_V10
            auto *source = bit_cast<xbox_v10_meta_anim_swing *>(
                a2->read_from_buffer(mash::NORMAL_BUFFER,
                                     sizeof(als::als_meta_anim_base),
                                     0));

            if (source->type == XBOX_V10_META_ANIM_SWING)
            {
                a2->advance_buffer(
                    mash::NORMAL_BUFFER,
                    sizeof(xbox_v10_meta_anim_swing) - sizeof(als::als_meta_anim_base));
                this->m_data[i] = expand_v10_meta_anim_swing(*source);
                this->m_data[i]->unmash(a2, nullptr);
                detach_v10_meta_anim_swing(
                    *static_cast<als::als_meta_anim_swing *>(this->m_data[i]));
            }
            else
            {
                auto *anim = bit_cast<als::als_meta_anim_base *>(source);
                this->m_data[i] = anim;
                mash_virtual_base::fixup_vtable(anim);
                a2->advance_buffer(mash::NORMAL_BUFFER,
                                   anim->get_mash_sizeof() - sizeof(*anim));
                anim->unmash(a2, nullptr);
            }
#else
            a2->unmash_class(this->m_data[i], a3
#if OPENUSM_XBOX_MASH_FORMAT
                , mash::NORMAL_BUFFER
#endif
                    );
#endif
        }
    }

    this->field_0 = (int)&a2->mash_image_ptr[0][a2->buffer_size_used[0] - (uint32_t) this];
}

template<>
void mVector<combo_system_move>::custom_unmash(mash_info_struct *a2, void *a3)
{
#if OPENUSM_XBOX_MASH_FORMAT
    this->field_C = this->m_size;
    if (this->m_size <= 0)
    {
        this->m_data = nullptr;
    }
    else
#else
    if ( this->m_data != nullptr )
#endif
    {
        this->m_data = (combo_system_move **) a2->read_from_buffer(
#if OPENUSM_XBOX_MASH_FORMAT
            mash::NORMAL_BUFFER, 
#endif
                4 * this->m_size, 4);
        for ( auto i = 0; i < this->m_size; ++i )
        {
#if defined(OPENUSM_XBPACK_MODE) && !defined(TARGET_XBOX)
            const auto *source = bit_cast<const xbox_combo_system_move *>(
                a2->read_from_buffer(mash::NORMAL_BUFFER, sizeof(xbox_combo_system_move), 0));

            auto *move = expand_combo_move(*source);
            this->m_data[i] = move;

            mash_virtual_base::fixup_vtable(move);
            move->unmash(a2, a3);
            detach_combo_move_links_from_mash(*move);
            detach_combo_move_string_from_mash(*move);
#else
            a2->unmash_class(this->m_data[i], a3
#if OPENUSM_XBOX_MASH_FORMAT
                , mash::NORMAL_BUFFER
#endif
                    );
#endif
        }
    }

    this->field_0 = (int)&a2->mash_image_ptr[0][a2->buffer_size_used[0] - (uint32_t) this];
}

template<>
void mVector<combo_system_chain>::custom_unmash(mash_info_struct *a1, void *a3)
{
#if OPENUSM_XBOX_MASH_FORMAT
    this->field_C = this->m_size;
    if (this->m_size <= 0)
    {
        this->m_data = nullptr;
    }
    else
#else
    if ( this->m_data != nullptr )
#endif
    {
        this->m_data = (combo_system_chain **) a1->read_from_buffer(
#if OPENUSM_XBOX_MASH_FORMAT
            mash::NORMAL_BUFFER,
#endif
                4 * this->m_size, 4);
        for ( auto i = 0; i < this->m_size; ++i )
        {
            a1->unmash_class(this->m_data[i], a3
#if OPENUSM_XBOX_MASH_FORMAT
                , mash::NORMAL_BUFFER
#endif
                    );
        }
    }

    this->field_0 = (int)&a1->mash_image_ptr[0][a1->buffer_size_used[0] - (uint32_t) this];
}

template<>
void mVector<combo_system_chain::telegraph_info>::custom_unmash(mash_info_struct *a2, void *a3)
{
    TRACE("mVector<combo_system_chain::telegraph_info>::custom_unmash");

#if OPENUSM_XBOX_MASH_FORMAT
    this->field_C = this->m_size;
    if (this->m_size <= 0)
    {
        this->m_data = nullptr;
    }
    else
#else
    if ( this->m_data != nullptr )
#endif
    {
        this->m_data = (combo_system_chain::telegraph_info **) a2->read_from_buffer(
#if OPENUSM_XBOX_MASH_FORMAT
            mash::NORMAL_BUFFER,
#endif
                4 * this->m_size, 4);
        for ( auto i = 0; i < this->m_size; ++i )
        {
            a2->unmash_class(this->m_data[i], a3
#if OPENUSM_XBOX_MASH_FORMAT
                , mash::NORMAL_BUFFER
#endif
                    );
        }
    }

    this->field_0 = (int)&a2->mash_image_ptr[0][a2->buffer_size_used[0] - (uint32_t) this];
}

template<>
void mVector<combo_system_move::link_info>::custom_unmash(mash_info_struct *a2, void *a3)
{
#if OPENUSM_XBOX_MASH_FORMAT
    this->field_C = this->m_size;
    if (this->m_size <= 0)
    {
        this->m_data = nullptr;
    }
    else
#else
    if ( this->m_data != nullptr )
#endif
    {
        this->m_data = (combo_system_move::link_info **) a2->read_from_buffer(
#if OPENUSM_XBOX_MASH_FORMAT
            mash::NORMAL_BUFFER,
#endif
                4 * this->m_size, 4);
        for ( auto i = 0; i < this->m_size; ++i )
        {
            a2->unmash_class(this->m_data[i], a3
#if OPENUSM_XBOX_MASH_FORMAT
                , mash::NORMAL_BUFFER
#endif
                    );
        }
    }

    this->field_0 = (int)&a2->mash_image_ptr[0][a2->buffer_size_used[0] - (uint32_t) this];
}

template<>
void mVector<combo_system_weapon>::custom_unmash(mash_info_struct *a1, void *a3)
{
    TRACE("mVector<combo_system_weapon>::custom_unmash");

#if OPENUSM_XBOX_MASH_FORMAT
    this->field_C = this->m_size;
    if (this->m_size <= 0)
    {
        this->m_data = nullptr;
    }
    else
#else
    if ( this->m_data != nullptr )
#endif
    {
        this->m_data = (combo_system_weapon **) a1->read_from_buffer(
#if OPENUSM_XBOX_MASH_FORMAT
            mash::NORMAL_BUFFER,
#endif
                4 * this->m_size, 4);
        for ( auto i = 0; i < this->m_size; ++i )
        {
            a1->unmash_class(this->m_data[i], a3
#if OPENUSM_XBOX_MASH_FORMAT
                , mash::NORMAL_BUFFER
#endif
                    );
        }
    }

    this->field_0 = (int)&a1->mash_image_ptr[0][a1->buffer_size_used[0] - (uint32_t) this];
}

template<>
void mVector<string_hash>::custom_unmash(mash_info_struct *a2, void *a3)
{
    TRACE("mVector<string_hash>::custom_unmash");

#if OPENUSM_XBOX_MASH_FORMAT
    this->field_C = this->m_size;
    if (this->m_size <= 0)
    {
        this->m_data = nullptr;
    }
    else
#else
    if ( this->m_data != nullptr )
#endif
    {
        this->m_data = (string_hash **) a2->read_from_buffer(
#if OPENUSM_XBOX_MASH_FORMAT
            mash::NORMAL_BUFFER,
#endif
                4 * this->m_size, 4);
        for ( auto i = 0; i < this->m_size; ++i )
        {
            a2->unmash_class(this->m_data[i], a3
#if OPENUSM_XBOX_MASH_FORMAT
                , mash::NORMAL_BUFFER
#endif
                    );
        }
    }

    this->field_0 = (int)&a2->mash_image_ptr[0][a2->buffer_size_used[0] - (uint32_t) this];
}

template<>
void mVector<resource_key>::custom_unmash(mash_info_struct *a2, void *a3)
{
    TRACE("mVector<resource_key>::custom_unmash");

#if OPENUSM_XBOX_MASH_FORMAT
    this->field_C = this->m_size;
    if (this->m_size <= 0)
    {
        this->m_data = nullptr;
    }
    else
#else
    if ( this->m_data != nullptr )
#endif
    {
#if defined(OPENUSM_XBPACK_MODE) && !defined(TARGET_XBOX)
        static_assert(sizeof(resource_key) == 0x8);

#ifdef OPENUSM_XBPACK_V10
        a2->read_from_buffer(
            mash::NORMAL_BUFFER, sizeof(resource_key *) * this->m_size, 4);
#endif
        auto *records = reinterpret_cast<resource_key *>(a2->read_from_buffer(
            mash::NORMAL_BUFFER, sizeof(resource_key) * this->m_size, 4));
        auto **pointer_table = static_cast<resource_key **>(
            allocate_from_pc_allocator(sizeof(resource_key *) * this->m_size));
        assert(pointer_table != nullptr);

        for (auto i = 0; i < this->m_size; ++i)
        {
            pointer_table[i] = &records[i];
        }

        this->m_data = pointer_table;
#else
        this->m_data = (resource_key **) a2->read_from_buffer(
#if OPENUSM_XBOX_MASH_FORMAT
            mash::NORMAL_BUFFER,
#endif
                4 * this->m_size, 4);

#if !OPENUSM_XBOX_MASH_FORMAT
        for ( auto i = 0; i < this->m_size; ++i )
        {
            a2->unmash_class(this->m_data[i], a3);
        }
#endif
#endif
    }

    this->field_0 = (int)&a2->mash_image_ptr[0][a2->buffer_size_used[0] - (uint32_t) this];
}

template<>
void mVector<als::meta_key_anim>::custom_unmash(mash_info_struct *a1, void *a3)
{
    TRACE("mVector<als::meta_key_anim>::custom_unmash");

#if OPENUSM_XBOX_MASH_FORMAT
    this->field_C = this->m_size;
    if (this->m_size <= 0)
    {
        this->m_data = nullptr;
    }
    else
#else
    if ( this->m_data != nullptr )
#endif
    {
        this->m_data = (als::meta_key_anim **) a1->read_from_buffer(
#if OPENUSM_XBOX_MASH_FORMAT
            mash::NORMAL_BUFFER,
#endif
                4 * this->m_size, 4);
        for ( auto i = 0; i < this->m_size; ++i )
        {
            a1->unmash_class(this->m_data[i], a3 
#if OPENUSM_XBOX_MASH_FORMAT
                , mash::NORMAL_BUFFER
#endif
                    );

            //sp_log("%s", this->m_data[i]->field_0.to_string());
        }
    }

    this->field_0 = (int)&a1->mash_image_ptr[0][a1->buffer_size_used[0] - (uint32_t) this];
}

template<>
void mVector<als::category>::custom_unmash(mash_info_struct *a2, [[maybe_unused]] void *a3)
{
    TRACE("mVector<als::category>::custom_unmash");

#if OPENUSM_XBOX_MASH_FORMAT
    this->field_C = this->m_size;
    if (this->m_size <= 0)
    {
        this->m_data = nullptr;
    }
    else
#else
    if ( this->m_data != nullptr )
#endif
    {
        this->m_data = (als::category **) a2->read_from_buffer(
#if OPENUSM_XBOX_MASH_FORMAT
                    mash::NORMAL_BUFFER,
#endif 
                4 * this->m_size, 4);
        for ( auto i = 0; i < this->m_size; ++i )
        {
            auto &v5 = this->m_data[i];
            auto *v6 = a2->read_from_buffer(
#if OPENUSM_XBOX_MASH_FORMAT
                    mash::NORMAL_BUFFER,
#endif 
                    sizeof(als::category), 0);
            v5 = (als::category *)v6;
            mash_virtual_base::fixup_vtable(v5);
            assert(v5->m_vtbl == 0x0087E250);

            auto v7 = v5->get_mash_sizeof();
            a2->advance_buffer(
#if OPENUSM_XBOX_MASH_FORMAT
                    mash::NORMAL_BUFFER,
#endif 
                    v7 - sizeof(als::category));
            v5->unmash(a2, nullptr);
        }
    }

    this->field_0 = (int)&a2->mash_image_ptr[0][a2->buffer_size_used[0] - (DWORD) this];
}

template<>
void mVector<als::transition_group_base>::custom_unmash(mash_info_struct *a2, void *)
{
    TRACE("mVector<als::transition_group_base>::custom_unmash");
#if OPENUSM_XBOX_MASH_FORMAT
    this->field_C = this->m_size;
    if (this->m_size <= 0)
    {
        this->m_data = nullptr;
    }
    else
#else
    if ( this->m_data != nullptr )
#endif
    {
        this->m_data = (als::transition_group_base **) a2->read_from_buffer(
#if OPENUSM_XBOX_MASH_FORMAT
                mash::NORMAL_BUFFER,
#endif 
                4 * this->m_size, 4);
        for ( auto i = 0; i < this->m_size; ++i )
        {
            auto &v5 = this->m_data[i];
            auto *v6 = a2->read_from_buffer(
#if OPENUSM_XBOX_MASH_FORMAT
                mash::NORMAL_BUFFER,
#endif 
                    4, 0);
            v5 = (als::transition_group_base *)v6;
            mash_virtual_base::fixup_vtable(v5);

            auto v7 = v5->get_mash_sizeof();
            a2->advance_buffer(
#if OPENUSM_XBOX_MASH_FORMAT
                mash::NORMAL_BUFFER,
#endif 
                    v7 - 4);
            v5->unmash(
                a2,
                nullptr);
        }
    }

    this->field_0 = (int)&a2->mash_image_ptr[0][a2->buffer_size_used[0] - (DWORD)this];
}

template<>
void mVector<ai::param_block::param_data>::initialize(mash::allocation_scope scope)
{
    if ( scope )
    {
        assert(scope == mash::FROM_MASH);

        if ( this->m_data != nullptr )
        {
            assert(m_size > 0);
            for ( int i = 0; i < this->m_size; ++i ) {
                new (this->m_data[i]) ai::param_block::param_data {};
            }
        }
    }
    else
    {
        this->m_data = nullptr;
        this->field_C = 0;
        this->field_10 = true;
    }
}

template<>
void mVector<ai::param_block::param_data>::destroy_element(ai::param_block::param_data **a2)
{
    if ( bit_cast<mContainer_base *>(this)->is_pointer_in_mash_image(*a2) )
    {
        (*a2)->destruct_mashed_class();
    }
    else if ( (*a2) != nullptr )
    {
        delete (*a2);
    }

    *a2 = nullptr;
}

template<>
void mVector<ai::param_block::param_data>::clear()
{
    if constexpr (0)
    {
        if ( this->field_10 )
        {
            for ( int i = 0; i < this->m_size; ++i )
            {
                this->destroy_element(&this->m_data[i]);
            }
        }

        if ( !this->is_pointer_in_mash_image(this->m_data) )
        {
            mem_dealloc(this->m_data, 4 * this->field_C);
        }

        this->m_data = nullptr;
        this->field_C = 0;

        mContainer_base::clear();
    }
    else
    {
        THISCALL(0x0043E400, this);
    }
}

template<>
void mVector<ai::param_block::param_data>::destruct_mashed_class()
{
    this->finalize(mash::FROM_MASH);
    //mContainer_base::destruct_mashed_class();
}

template<>
void mVector<ai::param_block::param_data>::custom_unmash(mash_info_struct *a2, void *a3)
{
#if OPENUSM_XBOX_MASH_FORMAT
    this->field_C = this->m_size;
    if ( this->m_size <= 0 )
    {
        this->m_data = nullptr;
    }
    else
#else
    if ( this->m_data != nullptr )
#endif
    {
        this->m_data = (ai::param_block::param_data **) a2->read_from_buffer(
#if OPENUSM_XBOX_MASH_FORMAT
                mash::NORMAL_BUFFER,
#endif 
                4 * this->m_size, 4);

        for ( auto i = 0; i < this->m_size; ++i )
        {
            auto &a1 = this->m_data[i];
            auto *v6 = (ai::param_block::param_data *) a2->read_from_buffer(
#if OPENUSM_XBOX_MASH_FORMAT
                mash::NORMAL_BUFFER,
#endif 
                12, 4);
            a1 = v6;
            a1->unmash(a2, a3);
        }
    }

    this->field_0 = (int)&a2->mash_image_ptr[0][a2->buffer_size_used[0] - (DWORD)this];
}

template<>
void mVector<als::implicit_transition_rule>::custom_unmash(mash_info_struct *a2, void *)
{
#if OPENUSM_XBOX_MASH_FORMAT
    this->field_C = this->m_size;
    if ( this->m_size <= 0 )
    {
        this->m_data = nullptr;
    }
    else
#else
    if ( this->m_data != nullptr )
#endif
    {
        this->m_data = (als::implicit_transition_rule **) a2->read_from_buffer(
#if OPENUSM_XBOX_MASH_FORMAT
                mash::NORMAL_BUFFER,
#endif
                4 * this->m_size, 4);
        for ( auto i = 0; i < this->m_size; ++i )
        {
            auto &v5 = this->m_data[i];
            auto *v6 = (als::implicit_transition_rule *) a2->read_from_buffer(
#if OPENUSM_XBOX_MASH_FORMAT
                mash::NORMAL_BUFFER,
#endif
                    0x24, 4);
            v5 = v6;
            v5->unmash(a2, nullptr);
        }
    }

    this->field_0 = (int)&a2->mash_image_ptr[0][a2->buffer_size_used[0] - (DWORD)this];
}

template<>
void mVector<als::layer_transition_rule>::custom_unmash(mash_info_struct *a1, void *a3)
{

#if OPENUSM_XBOX_MASH_FORMAT
    this->field_C = this->m_size;
    if ( this->m_size <= 0 )
    {
        this->m_data = nullptr;
    }
    else
#else
    if ( this->m_data != nullptr )
#endif
    {
        this->m_data = (als::layer_transition_rule **) a1->read_from_buffer(
#if OPENUSM_XBOX_MASH_FORMAT
            mash::NORMAL_BUFFER,
#endif
            4 * this->m_size, 4);
        for ( auto i = 0; i < this->m_size; ++i )
        {
            a1->unmash_class(this->m_data[i], a3
#if OPENUSM_XBOX_MASH_FORMAT
                , mash::NORMAL_BUFFER
#endif
                    );
        }
    }

    this->field_0 = (int)&a1->mash_image_ptr[0][a1->buffer_size_used[0] - (DWORD)this];
}

template<>
void mVector<als::dest_weight_data>::custom_unmash(mash_info_struct *a1, void *)
{
#if OPENUSM_XBOX_MASH_FORMAT
    this->field_C = this->m_size;
    if ( this->m_size <= 0 )
    {
        this->m_data = nullptr;
    }
    else
#else
    if ( this->m_data != nullptr )
#endif
    {
        this->m_data = (als::dest_weight_data **) a1->read_from_buffer(
#if OPENUSM_XBOX_MASH_FORMAT
                mash::NORMAL_BUFFER,
#endif
                4 * this->m_size, 4);
        for ( auto i = 0; i < this->m_size; ++i )
        {
            auto &v5 = this->m_data[i];
            auto *v6 = (als::dest_weight_data *) a1->read_from_buffer(
#if OPENUSM_XBOX_MASH_FORMAT
                mash::NORMAL_BUFFER,
#endif
                8, 4);
            v5 = v6;
            a1->unmash_class_in_place(v5->field_0, v6);
        }
    }

    this->field_0 = (int)&a1->mash_image_ptr[0][a1->buffer_size_used[0] - (DWORD)this];
}

template<>
void mVector<als::explicit_transition_rule>::custom_unmash(mash_info_struct *a1, void *)
{
#if OPENUSM_XBOX_MASH_FORMAT
    this->field_C = this->m_size;
    if ( this->m_size <= 0 )
    {
        this->m_data = nullptr;
    }
    else
#else
    if ( this->m_data != nullptr )
#endif
    {
        this->m_data = (als::explicit_transition_rule **) a1->read_from_buffer(
#if OPENUSM_XBOX_MASH_FORMAT
            mash::NORMAL_BUFFER,
#endif
                4 * this->m_size, 4);
        for ( auto i = 0; i < this->m_size; ++i )
        {
            auto &a1a = this->m_data[i];
            auto *v6 = (als::explicit_transition_rule *) a1->read_from_buffer(
#if OPENUSM_XBOX_MASH_FORMAT
                mash::NORMAL_BUFFER,
#endif
                    40, 4);
            a1a = v6;
            a1a->unmash(a1, nullptr);
            a1->unmash_class_in_place(a1a->field_24, a1a);
        }
    }

    this->field_0 = (int)&a1->mash_image_ptr[0][a1->buffer_size_used[0] - (DWORD)this];
}

template<>
void mVector<als::alter_conditions>::custom_unmash(mash_info_struct *a1, void *a3)
{
#if OPENUSM_XBOX_MASH_FORMAT
    this->field_C = this->m_size;
    if ( this->m_size <= 0 )
    {
        this->m_data = nullptr;
    }
    else
#else
    if ( this->m_data != nullptr )
#endif
    {
        this->m_data = (als::alter_conditions **) a1->read_from_buffer(
#if OPENUSM_XBOX_MASH_FORMAT
            mash::NORMAL_BUFFER,
#endif
            4 * this->m_size, 4);

        for ( auto i = 0; i < this->m_size; ++i )
        {
            a1->unmash_class(this->m_data[i], a3
#if OPENUSM_XBOX_MASH_FORMAT
                , mash::NORMAL_BUFFER
#endif
                    );
        }
    }

    this->field_0 = (int)&a1->mash_image_ptr[0][a1->buffer_size_used[0] - (uint32_t) this];
}

template<>
void mVector<als::incoming_transition_rule>::custom_unmash(mash_info_struct *a1, void *)
{
#if OPENUSM_XBOX_MASH_FORMAT
    this->field_C = this->m_size;
    if ( this->m_size <= 0 )
    {
        this->m_data = nullptr;
    }
    else
#else
    if ( this->m_data != nullptr )
#endif
    {
        this->m_data = (als::incoming_transition_rule **) a1->read_from_buffer(
#if OPENUSM_XBOX_MASH_FORMAT
                mash::NORMAL_BUFFER,
#endif
                4 * this->m_size, 4);
        for ( auto i = 0; i < this->m_size; ++i )
        {
            auto &v5 = this->m_data[i];
            auto *v6 = (als::incoming_transition_rule *) a1->read_from_buffer(
#if OPENUSM_XBOX_MASH_FORMAT
                mash::NORMAL_BUFFER,
#endif
                    44, 4);
            v5 = v6;
            v5->unmash(a1, nullptr);
        }
    }

    this->field_0 = (int)&a1->mash_image_ptr[0][a1->buffer_size_used[0] - (uint32_t) this];
}

template<>
void mVector<als::post_kill_rule>::custom_unmash(mash_info_struct *a2, void *a3)
{
#if OPENUSM_XBOX_MASH_FORMAT
    this->field_C = this->m_size;
    if ( this->m_size <= 0 )
    {
        this->m_data = nullptr;
    }
    else 
#else
    if ( this->m_data != nullptr )
#endif
    {
        this->m_data = (als::post_kill_rule **) a2->read_from_buffer(
#if OPENUSM_XBOX_MASH_FORMAT
            mash::NORMAL_BUFFER,
#endif
                4 * this->m_size, 4);
        for ( auto i = 0; i < this->m_size; ++i )
        {
            a2->unmash_class(this->m_data[i], a3
#if OPENUSM_XBOX_MASH_FORMAT
                , mash::NORMAL_BUFFER
#endif
                    );
        }
    }

    this->field_0 = (int)&a2->mash_image_ptr[0][a2->buffer_size_used[0] - (uint32_t) this];
}

template<>
void mVector<als::post_layer_alter>::custom_unmash(mash_info_struct *a1, void *a3)
{
#if OPENUSM_XBOX_MASH_FORMAT
    this->field_C = this->m_size;
    if ( this->m_size <= 0 )
    {
        this->m_data = nullptr;
    }
    else 
#else
    if ( this->m_data != nullptr )
#endif
    {
        this->m_data = (als::post_layer_alter **) a1->read_from_buffer(
#if OPENUSM_XBOX_MASH_FORMAT
            mash::NORMAL_BUFFER,
#endif
                4 * this->m_size, 4);
        for ( auto i = 0; i < this->m_size; ++i )
        {
            a1->unmash_class(this->m_data[i], a3
#if OPENUSM_XBOX_MASH_FORMAT
                , mash::NORMAL_BUFFER
#endif
                    );
        }
    }

    this->field_0 = (int)&a1->mash_image_ptr[0][a1->buffer_size_used[0] - (uint32_t) this];
}

template<>
void mVector<als::filter_data>::custom_unmash(mash_info_struct *a2, void *)
{
#if OPENUSM_XBOX_MASH_FORMAT
    this->field_C = this->m_size;
    if ( this->m_size <= 0 )
    {
        this->m_data = nullptr;
    }
    else 
#else
    if ( this->m_data != nullptr )
#endif
    {
        this->m_data = (als::filter_data **) a2->read_from_buffer(
#if OPENUSM_XBOX_MASH_FORMAT
            mash::NORMAL_BUFFER,
#endif
                4 * this->m_size, 4);
        for ( auto i = 0; i < this->m_size; ++i )
        {
            a2->unmash_class(this->m_data[i], this
#if OPENUSM_XBOX_MASH_FORMAT
                , mash::NORMAL_BUFFER
#endif
                    );
        }
    }

    this->field_0 = (int)&a2->mash_image_ptr[0][a2->buffer_size_used[0] - (uint32_t) this];
}

template<>
void mVector<ai::mashed_state>::custom_unmash(mash_info_struct *a1, void *a3)
{
    TRACE("mVector<ai::mashed_state>::custom_unmash");

#if OPENUSM_XBOX_MASH_FORMAT
    this->field_C = this->m_size;
    if ( this->m_size <= 0 )
    {
        this->m_data = nullptr;
    }
    else 
#else
    if ( this->m_data != nullptr )
#endif
    {
        this->m_data = (ai::mashed_state **) a1->read_from_buffer(
#if OPENUSM_XBOX_MASH_FORMAT
            mash::NORMAL_BUFFER,
#endif
                4 * this->m_size, 4);
        for ( auto i = 0; i < this->m_size; ++i )
        {
            a1->unmash_class(this->m_data[i], a3
#if OPENUSM_XBOX_MASH_FORMAT
                , mash::NORMAL_BUFFER
#endif
                    );
#ifdef OPENUSM_XBPACK_V10
            auto &type = this->m_data[i]->field_14;
            log_v10_state_type(*this->m_data[i], static_cast<uint32_t>(type));
            type = static_cast<mash::virtual_types_enum>(
                xbpack::pc_state_type(static_cast<uint32_t>(type)));
#endif
        }
    }

    this->field_0 = (int)&a1->mash_image_ptr[0][a1->buffer_size_used[0] - (uint32_t)this];
}

template<>
void mVector<ai::base_state>::custom_unmash(mash_info_struct *a2,
                                            [[maybe_unused]] void *a3)
{
    TRACE("mVector<ai::base_state>::custom_unmash");

#if OPENUSM_XBOX_MASH_FORMAT
    this->field_C = this->m_size;
    if ( this->m_size <= 0 )
    {
        this->m_data = nullptr;
    }
    else 
#else
    if ( this->m_data != nullptr )
#endif
    {
        this->m_data = (ai::base_state **) a2->read_from_buffer(
#if OPENUSM_XBOX_MASH_FORMAT
            mash::NORMAL_BUFFER,
#endif
                4 * this->m_size, 4);
        for ( auto i = 0; i < this->m_size; ++i )
        {
#ifdef OPENUSM_XBPACK_V10
            const auto *source = bit_cast<const xbox_v10_state *>(
                a2->read_from_buffer(
                    mash::NORMAL_BUFFER, sizeof(xbox_v10_state), 0));
            this->m_data[i] = expand_v10_state(*source);
#else
            a2->unmash_class(this->m_data[i], a3
#if OPENUSM_XBOX_MASH_FORMAT
                , mash::NORMAL_BUFFER
#endif
                    );
#endif
        }
    }

    this->field_0 = (int)&a2->mash_image_ptr[0][a2->buffer_size_used[0] - (uint32_t)this];
}

template<>
void mVector<anim_record>::custom_unmash(mash_info_struct *a2, void *a3)
{
    TRACE("mVector<anim_record>::custom_unmash");

#if OPENUSM_XBOX_MASH_FORMAT
    this->field_C = this->m_size;
    if ( this->m_size <= 0 )
    {
        this->m_data = nullptr;
    }
    else 
#else
    if ( this->m_data != nullptr )
#endif
    {
        this->m_data = (anim_record **) a2->read_from_buffer(
#if OPENUSM_XBOX_MASH_FORMAT
            mash::NORMAL_BUFFER,
#endif
                4 * this->m_size, 4);
        for ( auto i = 0; i < this->m_size; ++i )
        {
            a2->unmash_class(this->m_data[i], a3
#if OPENUSM_XBOX_MASH_FORMAT
                , mash::NORMAL_BUFFER
#endif
                    );
        }
    }

    this->field_0 = (int)&a2->mash_image_ptr[0][a2->buffer_size_used[0] - (uint32_t)this];
}

template<>
void mVector<interact_sound_entry>::custom_unmash(mash_info_struct *a1, void *a3)
{
    TRACE("mVector<interact_sound_entry>::custom_unmash");

#if OPENUSM_XBOX_MASH_FORMAT
    this->field_C = this->m_size;
    if ( this->m_size <= 0 )
    {
        this->m_data = nullptr;
    }
    else 
#else
    if ( this->m_data != nullptr )
#endif
    {
        this->m_data = (interact_sound_entry **) a1->read_from_buffer(
#if OPENUSM_XBOX_MASH_FORMAT
            mash::NORMAL_BUFFER,
#endif
                4 * this->m_size, 4);
        for ( auto i = 0; i < this->m_size; ++i )
        {
            a1->unmash_class(this->m_data[i], a3
#if OPENUSM_XBOX_MASH_FORMAT
                , mash::NORMAL_BUFFER
#endif
                );
        }
    }

    this->field_0 = (int)&a1->mash_image_ptr[0][a1->buffer_size_used[0] - (uint32_t)this];
}

template<>
void mVector<ai_adv_strength_test_data>::custom_unmash(mash_info_struct *a2, void *a3)
{
    TRACE("mVector<ai_adv_strength_test_data>::custom_unmash");

#if OPENUSM_XBOX_MASH_FORMAT
    this->field_C = this->m_size;
    if ( this->m_size <= 0 )
    {
        this->m_data = nullptr;
    }
    else 
#else
    if ( this->m_data != nullptr )
#endif
    {
        this->m_data = (ai_adv_strength_test_data **) a2->read_from_buffer(
#if OPENUSM_XBOX_MASH_FORMAT
            mash::NORMAL_BUFFER,
#endif
                4 * this->m_size, 4);
        for ( auto i = 0; i < this->m_size; ++i )
        {
            a2->unmash_class(this->m_data[i], a3
#if OPENUSM_XBOX_MASH_FORMAT
                , mash::NORMAL_BUFFER
#endif
                    );
        }
    }

    this->field_0 = (int)&a2->mash_image_ptr[0][a2->buffer_size_used[0] - (uint32_t)this];
}

template<>
void mVector<attach_node>::custom_unmash(mash_info_struct *a1, void *a3)
{
    TRACE("mVector<attach_node>::unmash");

#if OPENUSM_XBOX_MASH_FORMAT
    this->field_C = this->m_size;
    if ( this->m_size <= 0 )
    {
        this->m_data = nullptr;
    }
    else 
#else
    if ( this->m_data != nullptr )
#endif
    {
        this->m_data = (attach_node **) a1->read_from_buffer(
#if OPENUSM_XBOX_MASH_FORMAT
            mash::NORMAL_BUFFER,
#endif
                4 * this->m_size, 4);
        for ( auto i = 0; i < this->m_size; ++i )
        {
#if OPENUSM_XBOX_MASH_FORMAT
            auto &a1a = this->m_data[i];
            auto *temp = bit_cast<xbox_attach_node *>(
                a1->read_from_buffer(mash::NORMAL_BUFFER,
                                     sizeof(xbox_attach_node), 4));

            a1a = static_cast<attach_node *>(
                allocate_from_pc_allocator(sizeof(attach_node)));
            assert(a1a != nullptr);
            std::memset(a1a, 0, sizeof(*a1a));

            std::memcpy(&a1a->field_0, temp->field_0, sizeof(temp->field_0));
#ifdef OPENUSM_XBPACK_V10
            std::memcpy(&a1a->field_10.field_0, temp->field_10,
                        sizeof(temp->field_10));
#else
            std::memcpy(&a1a->field_10.m_size, temp->field_10,
                        sizeof(temp->field_10));
#endif
            a1a->field_10.field_C = nullptr;
            std::memcpy(&a1a->field_20, temp->field_1C,
                        sizeof(temp->field_1C));

            a1a->unmash(a1, a3);
#else
            a1->unmash_class(this->m_data[i], a3);
#endif
        }
    }

    this->field_0 = (int)&a1->mash_image_ptr[0][a1->buffer_size_used[0] - (uint32_t)this];
}

template<>
void mVectorBasic<attach_action_trigger_enum>::custom_unmash(mash_info_struct *a1, void *)
{
    TRACE("mVectorBasic<attach_action_trigger_enum>::custom_unmash");

#if OPENUSM_XBOX_MASH_FORMAT
    this->field_C = this->m_size;
    if ( this->m_size <= 0 )
    {
        this->m_data = nullptr;
    }
    else
#else
    if ( this->m_data != nullptr )
#endif
    {
#ifdef OPENUSM_XBPACK_V10
        a1->read_from_buffer(mash::NORMAL_BUFFER, 4, 4);
#endif
        this->m_data = CAST(this->m_data, a1->read_from_buffer(
#if OPENUSM_XBOX_MASH_FORMAT
            mash::NORMAL_BUFFER,
#endif
                4 * this->m_size, 4));
    }

    this->field_0 = (int)&a1->mash_image_ptr[0][a1->buffer_size_used[0] - (DWORD) this];
}

template<>
void mVectorBasic<attach_action_trigger_enum>::unmash(mash_info_struct *a1, void *a2)
{
#if OPENUSM_XBOX_MASH_FORMAT && !defined(OPENUSM_XBPACK_V10)
    [](mash_info_struct *a1, mash::buffer_type a2, uint32_t &a3)
    {
        a3 = *bit_cast<int *>(a1->read_from_buffer(a2, 4, 4));
    }(a1, mash::SHARED_BUFFER, m_size);
#endif

    this->custom_unmash(a1, a2);
}

template<>
void mVectorBasic<int>::custom_unmash(mash_info_struct *a1, void *)
{
    TRACE("mVectorBasic<int>::custom_unmash");

#if OPENUSM_XBOX_MASH_FORMAT
    this->field_C = this->m_size;
    if ( this->m_size <= 0 )
    {
        this->m_data = nullptr;
    }
    else
#else
    if ( this->m_data != nullptr )
#endif
    {
        this->m_data = (int *) a1->read_from_buffer(
#if OPENUSM_XBOX_MASH_FORMAT
            mash::NORMAL_BUFFER,
#endif
                4 * this->m_size, 4);
    }

    this->field_0 = (int)&a1->mash_image_ptr[0][a1->buffer_size_used[0] - (DWORD) this];
}

template<>
void mVectorBasic<int>::unmash(mash_info_struct *a1, void *a2)
{
#if OPENUSM_XBOX_MASH_FORMAT && !defined(OPENUSM_XBPACK_V10)
    [](mash_info_struct *a1, mash::buffer_type a2, uint32_t &a3)
    {
        a3 = *bit_cast<int *>(a1->read_from_buffer(a2, 4, 4));
    }(a1, mash::SHARED_BUFFER, m_size);
#endif

    this->custom_unmash(a1, a2);
}

template<>
void mVectorBasic<vhandle_type<actor>>::custom_unmash(mash_info_struct *a1, void *)
{
    TRACE("mVectorBasic<int>::custom_unmash");

#if OPENUSM_XBOX_MASH_FORMAT
    this->field_C = this->m_size;
    if ( this->m_size <= 0 )
    {
        this->m_data = nullptr;
    }
    else
#else
    if ( this->m_data != nullptr )
#endif
    {
        this->m_data = CAST(this->m_data, a1->read_from_buffer(
#if OPENUSM_XBOX_MASH_FORMAT
            mash::NORMAL_BUFFER,
#endif
                4 * this->m_size, 4));
    }

    this->field_0 = (int)&a1->mash_image_ptr[0][a1->buffer_size_used[0] - (DWORD) this];
}

template<>
void mVectorBasic<vhandle_type<actor>>::unmash(mash_info_struct *a1, void *a2)
{
#if OPENUSM_XBOX_MASH_FORMAT && !defined(OPENUSM_XBPACK_V10)
    [](mash_info_struct *a1, mash::buffer_type a2, uint32_t &a3)
    {
        a3 = *bit_cast<int *>(a1->read_from_buffer(a2, 4, 4));
    }(a1, mash::SHARED_BUFFER, m_size);
#endif

    this->custom_unmash(a1, a2);
}
