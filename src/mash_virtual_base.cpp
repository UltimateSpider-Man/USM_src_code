#include "mash_virtual_base.h"

#include "anim_record.h"
#include "enum_anim_key.h"
#include "func_wrapper.h"
#include "als_scripted_category.h"
#include "als_scripted_state.h"
#include "als_transition_group_base.h"
#include "layer_state_machine_shared.h"
#include "log.h"
#include "mash_config.h"
#include "memory.h"
#include "meta_anim_interact.h"
#include "panelquad.h"
#include "fefloatingtext.h"
#include "femultilinetext.h"
#include "spidey_base_state.h"
#include "std_puppet_trans_state.h"
#include "string_hash.h"
#include "trace.h"
#include "utility.h"
#include "vtbl.h"

#include <cassert>

#if defined(OPENUSM_XBPACK_MODE) && !defined(TARGET_XBOX)
#include <cstddef>
#include <cstdio>
#include <windows.h>
#endif

#ifdef OPENUSM_XBPACK_V10
#include "xbpack.h"
#include "xbpack_v10_types.h"
#endif

#if defined(OPENUSM_XBPACK_MODE) && !defined(TARGET_XBOX)
namespace {

struct xbox_type_mapping
{
    uint32_t xbox_type;
    uint32_t pc_type;
};

template<size_t Size>
bool translate_type(uint32_t xbox_type,
                    const xbox_type_mapping (&mappings)[Size],
                    uint32_t &pc_type)
{
    for (const auto &mapping : mappings) {
        if (mapping.xbox_type == xbox_type) {
            pc_type = mapping.pc_type;
            return true;
        }
    }

    return false;
}

constexpr xbox_type_mapping XBOX_V14_FACTORY_TYPES[] {
    {to_hash("lite_idle_state"), 0x099},
    {to_hash("attach_state"), 0x101},
    {to_hash("parker_combat_state"), 0x113},
    {to_hash("run_state"), 0x13D},
    {to_hash("jump_state"), 0x12F},
    {to_hash("hit_react_state"), 0x171},
    {to_hash("plr_loco_crawl_state"), 0x0B5},
    {to_hash("debug_state"), 0x118},
    {to_hash("aimed_throw_state"), 0x140},
    {to_hash("plr_loco_crawl_transition_state"), 0x0B6},
    {to_hash("interaction_state"), 0x12B},
    {to_hash("put_down_state"), 0x12E},
    {to_hash("pick_up_state"), 0x12D},
    {to_hash("subdued_state"), 0x174},
    {to_hash("std_default_state_set_base"), 0x172},
    {to_hash("web_zip_state"), 0x147},
    {to_hash("venom_grapple_thrown_state"), 0x1B7},
    {to_hash("spidey_combat_state"), 0x116},
    {to_hash("venom_combat_state"), 0x117},
    {to_hash("pole_swing_state"), 0x131},
    {to_hash("swing_state"), 0x13F},
    {to_hash("biped_idle_state"), 0x151},
    {to_hash("hostage_victim_state"), 0x126},
    {to_hash("launch_layer_state"), 0x14A},
    {to_hash("universal_soldier_noncombat_idle_state"), 0x1AC},
    {to_hash("flee_quad_path_state"), 0x0A1},
    {to_hash("flee_traffic_path_state"), 0x0A2},
    {to_hash("ped_hit_react_state"), 0x0AB},
    {to_hash("ped_subdued_state"), 0x0AE},
    // CH_GANG_SKIN_BOSS V14 state graph. These hash-to-PC factory IDs were
    // matched against the shipped PC V14 gang-boss graph by state signature.
    {to_hash("nonpathed_goto_state"), 0x153},
    {to_hash("cpu_combat_state"), 0x10E},
    {to_hash("play_single_anim_state"), 0x14F},
    {to_hash("go_to_car_state"), 0x108},
    {to_hash("drive_car_state"), 0x103},
    {to_hash("puppet_movement_state"), 0x135},
    {to_hash("puppet_play_anim_state"), 0x137},
    {to_hash("morale_reaction_state"), 0x1AA},
    {to_hash("pedestrian_idle_state"), 0x0B1},
    {to_hash("play_dodge_anim_state"), 0x0B2},
    {to_hash("universal_soldier_combat_idle_state"), 0x1AB},
    {to_hash("puppet_facing_state"), 0x132},
    {to_hash("falling_state"), 0x16F},
    {to_hash("exit_car_state"), 0x105},
    {to_hash("wait_for_car_stop_state"), 0x10D},
    {to_hash("generic_move_to_attack_state"), 0x111},
    {to_hash("patrol_state"), 0x190},
    {to_hash("select_patrol_node_state"), 0x191},
    {to_hash("unconscious_state"), 0x175},
    {to_hash("hit_avoid_state"), 0x170},
    {to_hash("search_state"), 0x197},
    {to_hash("puppet_interact_state"), 0x134},
    {to_hash("puppet_signal_state"), 0x139},
    {to_hash("enter_car_state"), 0x104},
    {to_hash("idle_car_state"), 0x109},
    {to_hash("get_arrested_state"), 0x0FF},
    {to_hash("enhanced_state"), 0x217},
};
static_assert(sizeof(XBOX_V14_FACTORY_TYPES) / sizeof(XBOX_V14_FACTORY_TYPES[0]) == 56);

#ifdef OPENUSM_XBPACK_V10
struct v10_type_mapping
{
    uint32_t xbox_type;
    uint32_t pc_type;
    uint32_t pc_vtable;
};

constexpr v10_type_mapping V10_TYPES[] {
    {0x02E, 0x02F, 0x0087C6F8},
    {0x057, 0x05E, 0x0087B8BC},
    {0x05C, 0x063, 0x00879FC0},
    {0x05F, 0x066, 0x0087CBA8},
    {0x060, 0x067, 0x0087CC4C},
    {0x061, 0x069, 0x0087CA78},
    {0x06B, 0x073, 0x0087CA48},
    {0x06D, 0x075, 0x0087CBE8},
    {0x073, 0x07B, 0x0087CC18},
    {0x078, 0x080, 0x0087CCE0},
    // Green Goblin's "gg" inode (V10 0x8C -> PC 0x94). The same-number
    // PC 0x7B slot is a different class with a vector at +0x1C.
    {0x07B, 0x083, 0x0087CCAC},
    {0x13D, 0x14D, 0x0087CEC0},
    {0x13E, 0x14E, 0x0087CEFC},
    {0x140, 0x150, 0x0087C048},
    {0x170, 0x180, 0x0087DAA4},
    {0x12E, 0x13E, 0x0087DB34},
    {0x12B, 0x13B, 0x0087DB04},
    {0x14E, 0x15F, 0x0087C410},
    {0x14F, 0x15F, 0x0087C410},
    {0x150, 0x160, 0x00874420},
    {0x151, 0x161, 0x0087C4B0},
    {0x154, 0x164, 0x0087C550},
    {0x16F, 0x17F, 0x0087DC44},
    {0x172, 0x182, 0x0087C68C},
    {0x136, 0x146, 0x0087DB64},
    {0x120, 0x130, 0x0087DAD4},
    {0x0E8, 0x0F8, 0x0087D700},
    {0x0EA, 0x0FA, 0x0087D8D4},
    {0x163, 0x173, 0x0087CC7C},
    {0x08D, 0x094, 0x0087CE10},
    // V10 path-node nuggets use type 0x91.  The prerelease PC packs encode
    // the same object as 0x9A; falling through to the PC 0x91 slot relinks it
    // as anim_record and calls a deleting destructor from path_graph::unmash.
    {0x091, 0x09A, 0x0087E3C8},
    {0x094, 0x09D, 0x0087CF38},
    {0x096, 0x09F, 0x0087CF80},
    {0x1B4, 0x1C8, 0x0087DDD8},
    {0x181, 0x192, 0x0087DC74},
    {0x182, 0x193, 0x0087DCA4},
    {0x185, 0x196, 0x0087DCD4},
    {0x17C, 0x18C, 0x0087C090},
    {0x189, 0x19A, 0x0087DD34},
    {0x08F, 0x098, 0x0087CE40},
    {0x093, 0x09C, 0x0087CFB4},
    {0x095, 0x09E, 0x0087CFF0},
    {0x0F2, 0x102, 0x0087D904},
    {0x10D, 0x11D, 0x0087DA74},
    {0x167, 0x177, 0x0087CA18},
    {0x156, 0x166, 0x0087D370},
    {0x157, 0x167, 0x0087DB94},
    {0x193, 0x1A4, 0x0087DD68},
    {0x187, 0x198, 0x0087DD04},
    {0x195, 0x1A6, 0x0087DDA8},
    {0x19D, 0x1AE, 0x0087DE08},
    {0x1A1, 0x1B3, 0x0087DE68},
    {0x1A6, 0x1B8, 0x0087DE38},
    {0x162, 0x172, 0x008750B8},
    {0x16D, 0x17D, 0x0087DC00},
    {0x148, 0x157, 0x0087BB9C},
    {0x146, 0x156, 0x0087BBD0},
    // Shocker's combat_inode, retagged by the AI core converter: PC 0xF5 is
    // the 0x338-byte Shocker subclass that shocker_combat_state (PC 0xF6)
    // reads at +0x32C/+0x330.
    {xbpack::v10_shocker_combat_inode_type, 0x0F5, 0x0087D5A0},
    {0x149, 0x15A, 0x0087BD00},
    {0x14A, 0x15B, 0x0087BE38},
    {0x142, 0x152, 0x0087BFDC},
    {0x145, 0x155, 0x0087C018},
    {0x155, 0x165, 0x0087D3D0},
    {0x00B, 0x00B, 0x0087BB6C},
    {0x147, 0x158, 0x0087A1FC},
    {0x20B, 0x221, 0x0087553C},
    {0x20C, 0x222, 0x0087E380},
    {0x20D, 0x223, 0x0087B8D8},
    {0x1CF, 0x1E3, 0x0087E3A4},
    {0x1D0, 0x1E4, 0x0087B8F8},
    {0x1D1, 0x1E5, 0x00878340},
    {0x1D2, 0x1E7, 0x00875494},
    {0x1D3, 0x1E8, 0x0087B918},
    {0x1D4, 0x1E9, 0x0087B954},
};

bool translate_v10_factory_type(uint32_t xbox_type, uint32_t &pc_type)
{
    for (const auto &mapping : V10_TYPES) {
        if (mapping.xbox_type == xbox_type) {
            pc_type = mapping.pc_type;
            return true;
        }
    }

    return false;
}

bool translate_v10_vtable(uint32_t xbox_type, uint32_t &pc_vtable)
{
    for (const auto &mapping : V10_TYPES) {
        if (mapping.xbox_type == xbox_type) {
            pc_vtable = mapping.pc_vtable;
            return true;
        }
    }

    return false;
}
#endif

bool translate_factory_type(uint32_t xbox_type, uint32_t &pc_type)
{
#ifdef OPENUSM_XBPACK_V10
    if (translate_v10_factory_type(xbox_type, pc_type)) {
        return true;
    }
#endif

    constexpr uint32_t MAX_PC_FACTORY_TYPE = 0x229;
    if (xbox_type <= MAX_PC_FACTORY_TYPE) {
        pc_type = xbox_type;
        return true;
    }

    return translate_type(xbox_type, XBOX_V14_FACTORY_TYPES, pc_type);
}

void report_unsupported_type(const char *category,
                             uint32_t hash,
                             const void *object = nullptr,
                             const void *caller = nullptr)
{
    char message[224];
    std::snprintf(message,
                  sizeof(message),
                  "XBPACK unsupported %s hash 0x%08X object=%p caller=%p\n",
                  category,
                  hash,
                  object,
                  caller);

#if defined(_DEBUG)
    OutputDebugStringA(message);
    DebugBreak();
#else
    MessageBox(NULL, message, "openusm", MB_OK);
#endif
    std::abort();
}

extern "C" __attribute__((noinline, used)) void *__cdecl xbpack_create_subclass(
    uint32_t xbox_type)
{
    uint32_t pc_type = 0;
    if (!translate_factory_type(xbox_type, pc_type)) {
        report_unsupported_type("factory", xbox_type);
        return nullptr;
    }

    return mash_virtual_base::create_subclass_by_enum(
        static_cast<mash::virtual_types_enum>(pc_type));
}

constexpr xbox_type_mapping XBOX_V14_MOCOMP_TYPES[] {
    {to_hash("als::null_mocomp"), 0x202},
    {to_hash("als::use_anim_only"), 0x20D},
    {to_hash("als::use_anim_only_with_invis"), 0x20E},
    {to_hash("als::simple_orientation"), 0x20A},
    {to_hash("als::simple_orient_with_playback_speed"), 0x208},
    {to_hash("als::simple_orient_with_speed_adjust"), 0x209},
    {to_hash("als::simple_orientation_ped"), 0x20B},
    {to_hash("als::strafe_mocomp"), 0x20C},
    {to_hash("als::orientated_react"), 0x204},
    {to_hash("als::crawl_orient"), 0x1F6},
    {to_hash("als::crawl_zip_mocomp"), 0x1F8},
    {to_hash("als::crawl_corner_mocomp"), 0x1F4},
    {to_hash("als::crawl_land_mocomp"), 0x1F5},
    {to_hash("als::crawl_corner_int90_mocomp"), 0x1F3},
    {to_hash("als::set_orient_mocomp"), 0x207},
    {to_hash("als::pole_swing_mocomp"), 0x205},
    {to_hash("als::bounce_mocomp"), 0x1EE},
    {to_hash("als::y_facing_fixup"), 0x211},
    {to_hash("als::fall_mocomp"), 0x1FB},
    {to_hash("als::jump_mocomp"), 0x1FE},
    {to_hash("als::feed_mocomp"), 0x201},
    {to_hash("als::webzip_mocomp"), 0x210},
    {to_hash("als::orient_adaptive_blend_xz"), 0x203},
    {to_hash("als::direct_mocomp"), 0x1F9},
    {to_hash("als::flight_mocomp"), 0x1FC},
    {to_hash("als::electro_flight_mocomp"), 0x1FA},
    {to_hash("als::beetle_flight_mocomp"), 0x1EC},
    {to_hash("als::chopper_flight_mocomp"), 0x1EF},
    {to_hash("als::johnny_storm_flight_mocomp"), 0x1FD},
    {to_hash("als::move_and_face"), 0x1FF},
    {to_hash("als::move_and_face_no_anim_movement"), 0x200},
    {to_hash("als::combat_move_and_face"), 0x1F0},
    {to_hash("als::combat_move_and_ignore_face"), 0x1F1},
    {to_hash("als::action_move_and_face"), 0x1EB},
    {to_hash("als::constant_move_and_face"), 0x1F2},
    {to_hash("als::begin_biped_physics"), 0x1ED},
    {to_hash("als::velocity_orientation"), 0x20F},
    {to_hash("als::crawl_transition"), 0x1F7},
    {to_hash("als::relative_orientation"), 0x206},
};
static_assert(sizeof(XBOX_V14_MOCOMP_TYPES) / sizeof(XBOX_V14_MOCOMP_TYPES[0]) == 39);

bool translate_mocomp_type(uint32_t xbox_type, uint32_t &pc_type)
{
#ifdef OPENUSM_XBPACK_V10
    if (xbox_type >= 0x1D5 && xbox_type <= 0x1E8) {
        pc_type = xbox_type + 0x15;
        return true;
    }

    if (xbox_type >= 0x1E9 && xbox_type <= 0x1FB) {
        pc_type = xbox_type + 0x16;
        return true;
    }

    return false;
#else
    return translate_type(xbox_type, XBOX_V14_MOCOMP_TYPES, pc_type);
#endif
}

extern "C" __attribute__((noinline, used)) void *__cdecl xbpack_create_mocomp_in_place(
    uint32_t xbox_type,
    mash_virtual_base *storage,
    int max_size)
{
    uint32_t pc_type = 0;
    if (!translate_mocomp_type(xbox_type, pc_type)) {
        report_unsupported_type("mocomp", xbox_type);
        return nullptr;
    }

    return mash_virtual_base::create_subclass_by_enum_in_place(
        static_cast<mash::virtual_types_enum>(pc_type), storage, max_size);
}

} // namespace
#endif

mash_virtual_base::mash_virtual_base()
{

}

void *mash_virtual_base::create_subclass_by_enum(mash::virtual_types_enum a1)
{
    TRACE("mash_virtual_base::create_subclass_by_enum");

    return (void *) CDECL_CALL(0x0042AB60, a1);
}

void *mash_virtual_base::create_subclass_by_enum_in_place(mash::virtual_types_enum a1,
                                                          mash_virtual_base *a2,
                                                          int a3)
{
    TRACE("mash_virtual_base::create_subclass_by_enum_in_place");

    return (void *) CDECL_CALL(0x004227E0, a1, a2, a3);
}

void mash_virtual_base::destruct_mashed_class() {
    ;
}

void mash_virtual_base::unmash(mash_info_struct *a2, void *a3) {
    if constexpr (1)
    {
        void (__fastcall *func)(void *, int, mash_info_struct *, void *) = CAST(func, get_vfunc(m_vtbl, 0x4));
        func(this, 0, a2, a3);
    }
    else
    {
        ;
    }
}

uint32_t mash_virtual_base::get_virtual_type_enum() const {
    return 573;
}

bool mash_virtual_base::is_subclass_of(mash::virtual_types_enum) const {
    return false;
}

bool mash_virtual_base::is_or_is_subclass_of(mash::virtual_types_enum a2) const {
    return this->get_virtual_type_enum() == a2 || this->is_subclass_of(a2);
}

void mash_virtual_base::generate_vtable()
{
    CDECL_CALL(0x00432B60);

#ifdef TARGET_XBOX
    {
        auto *v1 = new PanelQuad{};
        map_vtable.insert_or_assign(to_hash("PanelQuad"), v1); 
    }

    {
        auto *v1 = new FEText{};
        map_vtable.insert_or_assign(to_hash("FEText"), v1); 
    }

    {
        auto *v1 = new FEMultiLineText{};
        map_vtable.insert_or_assign(to_hash("FEMultiLineText"), v1); 
    }

    {
        auto *v1 = new FEFloatingText {};
        map_vtable.insert_or_assign(to_hash("FEFloatingText"), v1); 
    }

    {
        auto *v1 = new als::state_machine_shared {};
        map_vtable.insert_or_assign(to_hash("als::state_machine_shared"), v1); 
    }

    {
        auto *v1 = new als::layer_state_machine_shared {};
        map_vtable.insert_or_assign(to_hash("als::layer_state_machine_shared"), v1); 
    }

    {
        auto *v1 = new als::scripted_state {};
        map_vtable.insert_or_assign(to_hash("als::scripted_state"), v1); 
    }

    {
        auto *v1 = new als::base_layer_scripted_state {};
        map_vtable.insert_or_assign(to_hash("als::base_layer_scripted_state"), v1); 
    }

    {
        auto *v1 = new als::scripted_category {};
        map_vtable.insert_or_assign(to_hash("als::scripted_category"), v1); 
    }

    {
        auto *v1 = new als::scripted_trans_group{};
        map_vtable.insert_or_assign(to_hash("als::scripted_trans_group"), v1); 
    }

    {
        auto *v1 = new ai::meta_anim_interact{};
        map_vtable.insert_or_assign(to_hash("als::meta_anim_interact"), v1); 
    }

    {
        auto *v1 = new ai::meta_anim_strength_test{};
        map_vtable.insert_or_assign(to_hash("als::meta_anim_strength_test"), v1); 
    }

    {
        auto *v1 = new als::als_meta_linear_blend{};
        map_vtable.insert_or_assign(to_hash("als::als_meta_linear_blend"), v1); 
    }

    {
        auto *v1 = new als::als_meta_anim_swing{};
        map_vtable.insert_or_assign(to_hash("als::als_meta_anim_swing"), v1); 
    }

    {
        auto *v1 = new ai::spidey_base_state {};
        map_vtable.insert_or_assign(to_hash("spidey_base_state"), v1); 
    }

    {
        auto *v1 = new ai::std_puppet_trans_state {};
        map_vtable.insert_or_assign(to_hash("std_puppet_trans_state"), v1); 
    }

    {
        auto *v1 = new anim_key {};
        map_vtable.insert_or_assign(to_hash("anim_key"), v1); 
    }

    {
        auto *v1 = new anim_record {};
        map_vtable.insert_or_assign(to_hash("anim_record"), v1); 
    }
#endif
}

void *mash_virtual_base::construct_class_helper(void *a1) {

    if constexpr (0) {
        auto *v1 = static_cast<mash_virtual_base *>(a1);

        auto v2 = v1->get_virtual_type_enum();

        sp_log("mash::virtual_types_enum = %u", v2);

        return mash_virtual_base::create_subclass_by_enum_in_place(static_cast<mash::virtual_types_enum>(v2),
                                                                   v1,
                                                                   0x7FFFFFFF);
    } else {
        auto *v1 = static_cast<mash_virtual_base *>(a1);
        auto v2 = v1->get_virtual_type_enum();

        sp_log("mash::virtual_types_enum = %u", v2);
        return (void *) CDECL_CALL(0x0042A7C0, a1);
    }
}

void mash_virtual_base::fixup_vtable(void *a1)
{
    TRACE("mash_virtual_base::fixup_vtable");

#if defined(OPENUSM_XBPACK_MODE) && !defined(TARGET_XBOX)
    const auto hash = static_cast<uint32_t *>(a1)[0];
    uint32_t pc_vtable = 0;

#ifdef OPENUSM_XBPACK_V10
    if (translate_v10_vtable(hash, pc_vtable)) {
        static_cast<uint32_t *>(a1)[0] = pc_vtable;
        return;
    }

    switch (hash) {
    case 0x56:
        pc_vtable = 0x008737C0;
        break;
    case 0x57:
        pc_vtable = 0x0087B8BC;
        break;
    case 0x58:
        pc_vtable = 0x00873734;
        break;
    case 0x59:
        pc_vtable = 0x008737A4;
        break;
    case 0x5A:
        pc_vtable = 0x00873788;
        break;
    case 0x5B:
        pc_vtable = 0x0087B8A0;
        break;
    case 0x5C:
        pc_vtable = 0x00879FC0;
        break;
    case 0x5D:
        pc_vtable = 0x0087376C;
        break;
    case 0x5E:
        pc_vtable = 0x00873750;
        break;
    case 0x87:
        pc_vtable = 0x008738E8;
        break;
    case 0x88:
        pc_vtable = 0x00873928;
        break;
    case 0x89:
        pc_vtable = 0x00873948;
        break;
    case 0x8A:
        pc_vtable = 0x00873908;
        break;
    case 0x8B:
        pc_vtable = 0x00875560;
        break;
    case 0x8C:
        pc_vtable = 0x0087559C;
        break;
    case 0x1FC:
        pc_vtable = 0x0087E214;
        break;
    case 0x1FD:
        pc_vtable = 0x0087E250;
        break;
    case 0x1FE:
        pc_vtable = 0x0087E1D8;
        break;
    case 0x1FF:
        pc_vtable = 0x0087E1B8;
        break;
    case 0x207:
        pc_vtable = 0x0087B990;
        break;
    case 0x208:
        pc_vtable = 0x0087A0F0;
        break;
    case 0x209:
        pc_vtable = 0x0087AE58;
        break;
    case 0x20A:
        pc_vtable = 0x00879FE0;
        break;
    default: {
        // Xbox v10 numbers classes differently from the PC prerelease (the
        // offset grows from +1 to +0x16 across the enum), so using the PC
        // class with the same number silently picks an unrelated class.
        uint32_t pc_type = hash;
        const bool mapped = xbpack::v10_types::find_pc_mash_type(hash, pc_type);
        if (pc_type > 0x23D || vtable()[pc_type] == nullptr) {
            report_unsupported_type(
                "vtable", hash, a1, __builtin_return_address(0));
            return;
        }

        if (!mapped) {
            static bool warned[0x240] {};
            if (hash < sizeof(warned) && !warned[hash]) {
                warned[hash] = true;
                sp_log("[xbpack] v10 vtable type 0x%03X has no verified PC "
                       "type; keeping PC type 0x%03X",
                       static_cast<unsigned>(hash),
                       static_cast<unsigned>(pc_type));
            }
        }

        pc_vtable = bit_cast<uint32_t>(vtable()[pc_type]);
        break;
    }
    }
#else
    switch (hash) {
    case to_hash("PanelQuad"):
        pc_vtable = 0x0087B990;
        break;
    case to_hash("FEFloatingText"):
        pc_vtable = 0x0087A0F0;
        break;
    case to_hash("FEMultiLineText"):
        pc_vtable = 0x0087AE58;
        break;
    case to_hash("FEText"):
        pc_vtable = 0x00879FE0;
        break;
    case to_hash("als::state_machine_shared"):
        pc_vtable = 0x0087B8F8;
        break;
    case to_hash("als::layer_state_machine_shared"):
        pc_vtable = 0x0087E3A4;
        break;
    case to_hash("als::scripted_state"):
        pc_vtable = 0x0087E1D8;
        break;
    case to_hash("als::base_layer_scripted_state"):
        pc_vtable = 0x0087E214;
        break;
    case to_hash("als::scripted_category"):
        pc_vtable = 0x0087E250;
        break;
    case to_hash("als::scripted_trans_group"):
        pc_vtable = 0x0087E1B8;
        break;
    case to_hash("als::meta_anim_interact"):
        pc_vtable = 0x00875560;
        break;
    case to_hash("als::meta_anim_strength_test"):
        pc_vtable = 0x0087559C;
        break;
    case to_hash("als::als_meta_linear_blend"):
        pc_vtable = 0x0087B954;
        break;
    case to_hash("als::als_meta_anim_swing"):
        pc_vtable = 0x0087B918;
        break;
    case to_hash("spidey_base_state"):
        pc_vtable = 0x00877534;
        break;
    case to_hash("venom_base_state"):
        pc_vtable = 0x00877570;
        break;
    case to_hash("std_puppet_trans_state"):
        pc_vtable = 0x008771E0;
        break;
    case to_hash("std_default_state_set_base"):
        pc_vtable = 0x008750B8;
        break;
    case to_hash("anim_key"):
        pc_vtable = 0x008738E8;
        break;
    case to_hash("anim_record"):
        pc_vtable = 0x00873928;
        break;
    case to_hash("als_inode"):
        pc_vtable = 0x0087CEC0;
        break;
    case to_hash("parker_als_inode"):
        pc_vtable = 0x0087CEFC;
        break;
    case to_hash("hero_inode"):
        pc_vtable = 0x0087DAA4;
        break;
    case to_hash("std_puppet_inode"):
        pc_vtable = 0x0087DB04;
        break;
    case to_hash("slave_inode"):
        pc_vtable = 0x0087DD04;
        break;
    case to_hash("player_combat_target_inode"):
        pc_vtable = 0x0087C4B0;
        break;
    case to_hash("combat_target_inode"):
        pc_vtable = 0x0087C410;
        break;
    case to_hash("venom_combat_target_inode"):
        pc_vtable = 0x0087C550;
        break;
    case to_hash("glass_house_inode"):
        pc_vtable = 0x0087DC44;
        break;
    case to_hash("web_zip_inode"):
        pc_vtable = 0x0087DB64;
        break;
    case to_hash("swing_inode"):
        pc_vtable = 0x0087DB34;
        break;
    case to_hash("pole_swing_inode"):
        pc_vtable = 0x0087DAD4;
        break;
    case to_hash("player_combat_inode"):
        pc_vtable = 0x0087BD00;
        break;
    case to_hash("thug_combat_inode"):
        pc_vtable = 0x0087BE38;
        break;
    case to_hash("spidey_combat_inode"):
        pc_vtable = 0x0087D700;
        break;
    case to_hash("combat_inode"):
        pc_vtable = 0x0087BBD0;
        break;
    case to_hash("combat_inode::incoming_move"):
        pc_vtable = 0x0087A1FC;
        break;
    case to_hash("std_default_trans_inode"):
        pc_vtable = 0x0087CC7C;
        break;
    case to_hash("interaction_inode"):
        pc_vtable = 0x0087CE10;
        break;
    case to_hash("interaction"):
        pc_vtable = 0x0087B8D8;
        break;
    case to_hash("voice_box_inode"):
        pc_vtable = 0x0087DDD8;
        break;
    case to_hash("weapon_inode"):
        pc_vtable = 0x0087DD34;
        break;
    case to_hash("physics_inode"):
        pc_vtable = 0x0087DC74;
        break;
    case to_hash("strength_test_inode"):
        pc_vtable = 0x0087CE40;
        break;
    case to_hash("player_controller_inode"):
        pc_vtable = 0x0087D370;
        break;
    case to_hash("cpu_controller_inode"):
        pc_vtable = 0x0087D3D0;
        break;
    case to_hash("controller_inode"):
        pc_vtable = 0x0087D310;
        break;
    case to_hash("arrested_inode"):
        pc_vtable = 0x0087D8D4;
        break;
    case to_hash("std_fear_inode"):
        pc_vtable = 0x0087CA18;
        break;
    case to_hash("pedestrian_inode"):
        pc_vtable = 0x0087CF80;
        break;
    case to_hash("nonpath_loco_inode"):
        pc_vtable = 0x0087CFB4;
        break;
    case to_hash("ai_hostage_victim_inode"):
        pc_vtable = 0x0087DA74;
        break;
    case to_hash("biped_layer_inode"):
        pc_vtable = 0x0087BFDC;
        break;
    case to_hash("avoidance_inode"):
        pc_vtable = 0x0087C048;
        break;
    case to_hash("ai_car_inode"):
        pc_vtable = 0x0087D904;
        break;
    case to_hash("search_inode"):
        pc_vtable = 0x0087DCD4;
        break;
    case to_hash("path_graph_inode"):
        pc_vtable = 0x0087C090;
        break;
    case to_hash("universal_soldier_inode"):
        pc_vtable = 0x0087DE08;
        break;
    case to_hash("quad_path_inode"):
        pc_vtable = 0x0087C018;
        break;
    case to_hash("ped_avoidance_inode"):
        pc_vtable = 0x0087CF38;
        break;
    case to_hash("track_field_inode"):
        pc_vtable = 0x0087DD68;
        break;
    case to_hash("damage_inode"):
        pc_vtable = 0x0087BB9C;
        break;
    case to_hash("ai_action_processor_inode"):
        pc_vtable = 0x0087BB6C;
        break;
    case to_hash("prop_physics_inode"):
        pc_vtable = 0x0087DCA4;
        break;
    case to_hash("combo_system_move"):
        pc_vtable = 0x0087B8BC;
        break;
    case to_hash("combo_system_chain::telegraph_info"):
        pc_vtable = 0x008737C0;
        break;
    case to_hash("combo_system_move::results"):
        pc_vtable = 0x00879FC0;
        break;
    case to_hash("combo_system_move::dialation_info"):
        pc_vtable = 0x00873734;
        break;
    case to_hash("combo_system_move::requirements"):
        pc_vtable = 0x0087B8A0;
        break;
    case to_hash("combo_system_move::trigger_info"):
        pc_vtable = 0x00873750;
        break;
    case to_hash("combo_system_move::target_info"):
        pc_vtable = 0x0087376C;
        break;
    case to_hash("combo_system_move::range_info"):
        pc_vtable = 0x00873788;
        break;
    case to_hash("combo_system_move::link_info"):
        pc_vtable = 0x008737A4;
        break;
    default:
        sp_log("Unsupported Xbox mash vtable hash 0x%08X", hash);
   //     assert(0 && "Unsupported Xbox mash vtable hash");
        return;
    }
#endif

    static_cast<uint32_t *>(a1)[0] = pc_vtable;

#elif defined(TARGET_XBOX)
    const auto hash = static_cast<uint32_t *>(a1)[0];

    sp_log("0x%08X", hash);

    static_cast<uint32_t *>(a1)[0] = map_vtable.at(hash)->m_vtbl;

#else

    const auto idx = static_cast<uint32_t *>(a1)[0];
    static_cast<uint32_t *>(a1)[0] = bit_cast<uint32_t>(vtable()[idx]);
#endif

    sp_log("0x%08X", static_cast<uint32_t *>(a1)[0]);
}

void mash_virtual_base_patch() {

    REDIRECT(0x00555726, mash_virtual_base::generate_vtable);

    REDIRECT(0x004B157A, mash_virtual_base::construct_class_helper);

    return;


}

#if defined(OPENUSM_XBPACK_MODE) && !defined(TARGET_XBOX)
void mash_virtual_base_xbpack_patch()
{
    SET_JUMP(0x0041F820, mash_virtual_base::fixup_vtable);

    REDIRECT(0x00498F5B, xbpack_create_mocomp_in_place);

#ifndef OPENUSM_XBPACK_V10
    REDIRECT(0x005BF5A6, xbpack_create_subclass);
    REDIRECT(0x005D328C, xbpack_create_subclass);
    REDIRECT(0x00687FF8, xbpack_create_subclass);
    REDIRECT(0x0069BC15, xbpack_create_subclass);
    REDIRECT(0x006A15E4, xbpack_create_subclass);
    REDIRECT(0x006C4C80, xbpack_create_subclass);
    REDIRECT(0x006C8780, xbpack_create_subclass);
#endif
}
#endif
