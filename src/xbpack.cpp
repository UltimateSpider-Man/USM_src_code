#include "xbpack.h"

#ifdef OPENUSM_XBPACK_MODE

#include "actor_xbpack.h"
#include "ai_interact_resource_handler.h"
#include "ai_state_graph_resource_handler.h"
#include "als_resource_handler.h"
#include "base_ai_resource_handler.h"
#include "combo_system.h"
#include "combo_system_move.h"
#include "colmesh.h"
#include "conglom.h"
#include "core_ai_resource.h"
#include "cut_scene_player.h"
#include "damage_interface.h"
#include "gab_database_resource_handler.h"
#include "log.h"
#include "lego_map.h"
#include "mission_manager.h"
#include "mash_virtual_base.h"
#include "ngl.h"
#include "param_block.h"
#include "panelfile.h"
#include "panelmeshsection.h"
#include "parse_generic_mash.h"
#include "resource_directory.h"
#include "resource_manager.h"
#include "resource_versions.h"
#include "script_manager.h"
#include "script_lib_debug_menu.h"
#include "slc_manager.h"
#include "sound_alias_database_resource_handler.h"
#include "terrain.h"
#include "texture_resource_handler.h"
#include "material_file_resource_handler.h"
#include "morph_file_resource_handler.h"
#include "func_wrapper.h"
#include "utility.h"
#include "variables.h"
#include "wds.h"

#ifdef OPENUSM_XBPACK_V10
#include <nal_generic.h>
#include <nal_skeleton.h>
#include "nal_system.h"
#include "xbpack_v10_po_patch.h"
#include "xbpack_v10_ped_patch.h"
#include "xbpack_v10_shocker.h"
#include "xbpack_v10_cut_scene.h"
#include "xbpack_v10_electro.h"
#include "xbpack_v10_green_goblin.h"
#include "xbpack_v10_s07_web.h"
#include "xbpack_v10_scene_pack.h"
#include "xbpack_v10_directory_parents.h"

void xbpack_v10_feed_trace_patch();
void xbpack_v10_info_nodes_patch();
void xbpack_v10_combat_trace_patch();
void xbpack_v10_gab_patch();
bool install_v10_layer_interrupt_bridge();
#endif

#include <cstdint>
#include <cstring>

namespace
{
constexpr uintptr_t PLATFORM_INITIALIZER = 0x005E10BB;
constexpr uintptr_t EXPECTED_RESOURCE_VERSIONS = 0x00937440;

#ifdef OPENUSM_XBPACK_V10
constexpr uintptr_t PO_VTABLES[] = {
    0x008BDA58,
    0x008BDAB8,
    0x008BDB18,
    0x008BDB78,
};

constexpr uintptr_t QUAT_VTABLES[] = {
    0x008BD8D8,
    0x008BD938,
    0x008BD998,
    0x008BD9F8,
};

constexpr uintptr_t IKSPIN_VTABLE = 0x008BDBD8;

							  
					  
					 
					   
						 
  

																	 
																		  
																		   
																			 
																		  
					
																
																	   
															 
																  
														  
															   
																	
															
															  
																   
														 
															  
															  
																   
														   
														   
																
															 
															   
																	

																			  
																  
																	   
															   
															
															
															

											 
																	
																		 
																   
																		
																   
																   
																   
																 
																

																	  
															 
																  
														  
															   
																	
																 
															
																	
																		 
																 

																		  
															   
																	
																
																	 
															   
															
																  
																	   
																 
																	  
															   
																 
															   
															  
															
																 
																 
																	  
																
																	 
															  
																   
																  
																	   
																
																  
																	   
															
																 
															  
															  
															
																 
															  

																   
																	 
																		  
																	
																		 
																  
																 
																	   
																			
																	  
																		   
																		  
																			   
																	  
																	  
																	  
																	
																   
																	   

																  
																
																	 
																  
																	  
																  
																	   
																	  
																		   
																	
																	
																		
																
																	 
																  
																  
																
																	 
																  
																	
  

constexpr uintptr_t PO_ALIGN = 0x10;
constexpr uintptr_t PO_SIZE = xbpack::v10_po::source_pose_size;
																		  
																		
																			
											 
									  
constexpr uintptr_t QUAT_SIZE = 0x10;
constexpr uintptr_t PC_QUAT_STATE_SIZE = 0x34;
constexpr uintptr_t XB_QUAT_STATE_SIZE = 0x40;
constexpr uintptr_t IKSPIN_SIZE = 0x50;
constexpr uintptr_t PC_IKSPIN_STATE_SIZE = 0xA8;
constexpr uintptr_t XB_IKSPIN_STATE_SIZE = 0xC0;


uint8_t *align_data(void *data, uintptr_t alignment)
{
    auto address = reinterpret_cast<uintptr_t>(data);
    address = (address + alignment - 1) & ~(alignment - 1);
    return reinterpret_cast<uint8_t *>(address);
}

uint8_t *align16(void *data)
{
    return align_data(data, PO_ALIGN);
}

							   
 
										
 

int __fastcall po_size(void *, void *)
{
    return PO_SIZE;
}

void __fastcall layout_po(
    void *, void *, const nalGeneric::nalComponentInfo *info,
    void **cursor, void **)
{
    *cursor = align16(*cursor) + PO_SIZE * info->field_28;
}

void __fastcall advance_po(
    void *, void *, const nalGeneric::nalComponentInfo *info,
    void **cursor)
{
    *cursor = align16(*cursor) + PO_SIZE * info->field_28;
}

// Track lookups (event data 0x004AD9F0 among others) walk the source stream
// through slots 0x4C..0x58. The two trajectory PO components keep one base
// pose per active track there: Xbox v10 0x0036B310/0x0036AE40 align it to 16
// and step 0x20, native PC 0x00784D30/0x00788CA0 use 4 and 0x1C. With the
// native steps every track behind a trajectory (USMEvent, signal counter...)
// was read from the wrong address, so root-motion animations lost their
// ATTACK/FX events.
constexpr uintptr_t PC_TRACK_ALIGN4 = 0x00784D30;
constexpr uintptr_t PC_TRACK_SKIP_CACHE_POSE = 0x00788CA0;

void __fastcall align_po_track(void *, void *, void **cursor)
{
    *cursor = align16(*cursor);
}

void __fastcall skip_po_track(void *, void *, void **cursor)
{
    *cursor = static_cast<uint8_t *>(*cursor) + PO_SIZE;
}

bool patch_xbox_v10_trajectory_tracks()
{
    constexpr uintptr_t TRAJECTORY_PO_VTABLES[] = {PO_VTABLES[2], PO_VTABLES[3]};
    for (auto vtable : TRAJECTORY_PO_VTABLES) {
        const auto align = *reinterpret_cast<const uintptr_t *>(vtable + 0x54);
        const auto skip = *reinterpret_cast<const uintptr_t *>(vtable + 0x58);
        if (align != PC_TRACK_ALIGN4 || skip != PC_TRACK_SKIP_CACHE_POSE) {
            sp_log("[xbpack] V10 trajectory track walk rejected: vtable 0x%08X "
                   "has 0x%08X/0x%08X",
                   static_cast<unsigned>(vtable),
                   static_cast<unsigned>(align),
                   static_cast<unsigned>(skip));
            return false;
        }
    }

    for (auto vtable : TRAJECTORY_PO_VTABLES) {
        set_vfunc(vtable + 0x54, &align_po_track);
        set_vfunc(vtable + 0x58, &skip_po_track);
    }

    sp_log("[xbpack] V10 trajectory track walk installed: base pose align=16 stride=0x%X",
           static_cast<unsigned>(PO_SIZE));
    return true;
}

void __fastcall copy_po(
    void *, void *, const nalGeneric::nalComponentInfo *info,
    void **dst_cursor, const void **src_cursor)
{
    auto *dst = align16(*dst_cursor);
    auto *src = align16(const_cast<void *>(*src_cursor));
    auto size = PO_SIZE * info->field_28;

    std::memcpy(dst, src, size);
    *dst_cursor = dst + size;
    *src_cursor = src + size;
}

void __fastcall layout_quat(
    void *, void *, const nalGeneric::nalComponentInfo *info,
    void **cursor, void **)
{
    *cursor = align16(*cursor) + QUAT_SIZE * info->field_28;
}

void __fastcall advance_quat(
    void *, void *, const nalGeneric::nalComponentInfo *info,
    void **cursor)
{
    *cursor = align16(*cursor) + QUAT_SIZE * info->field_28;
}

void __fastcall copy_quat(
    void *, void *, const nalGeneric::nalComponentInfo *info,
    void **dst_cursor, const void **src_cursor)
{
    auto *dst = align16(*dst_cursor);
    auto *src = align16(const_cast<void *>(*src_cursor));
    auto size = QUAT_SIZE * info->field_28;

    std::memcpy(dst, src, size);
    *dst_cursor = dst + size;
    *src_cursor = src + size;
}

bool component_active(
    const uintptr_t *context, const nalGeneric::nalComponentInfo *info, int i)
{
    auto *bits = *reinterpret_cast<const uint32_t *const *>(context[0] + 0x60);
    auto index = info->field_24 + i;
    return ((bits[index / 32] >> (index & 31)) & 1) != 0;
}

int active_quat_count(const uintptr_t *context)
{
    auto *info = reinterpret_cast<const nalGeneric::nalComponentInfo *>(context[1]);
    int count = 0;

    for (int i = 0; i < info->field_28; ++i) {
        count += component_active(context, info, i);
    }

    return count;
}

void compact_quat_states(uint8_t *data, int count)
{
    for (int i = 1; i < count; ++i) {
        std::memmove(data + i * PC_QUAT_STATE_SIZE,
                     data + i * XB_QUAT_STATE_SIZE,
                     PC_QUAT_STATE_SIZE);
    }
}

void expand_quat_states(uint8_t *data, int count)
{
    for (int i = count - 1; i >= 0; --i) {
        auto *dst = data + i * XB_QUAT_STATE_SIZE;
        std::memmove(dst, data + i * PC_QUAT_STATE_SIZE,
                     PC_QUAT_STATE_SIZE);
        std::memset(dst + PC_QUAT_STATE_SIZE, 0,
                    XB_QUAT_STATE_SIZE - PC_QUAT_STATE_SIZE);
    }
}

int __fastcall read_quat_control(
    void *component, void *, uintptr_t *context,
    void **state_cursor, void **source_cursor, unsigned int n2)
{
    auto count = active_quat_count(context);
    auto *state = align16(*state_cursor);
    *state_cursor = state;

    auto result = THISCALL(0x00787B50, component, context,
                           state_cursor, source_cursor, n2);
    if (state != nullptr) {
        expand_quat_states(state, count);
    }
    *state_cursor = reinterpret_cast<void *>(
        reinterpret_cast<uintptr_t>(state) + count * XB_QUAT_STATE_SIZE);
    return result;
}

int update_quat(
    uintptr_t address, void *component, uintptr_t *context,
										
    void **data_cursor, void **state_cursor, void *a4,
    int a5, unsigned int a6, int a7)
{
    auto count = active_quat_count(context);
							
												
	 
    auto *state = align16(*state_cursor);
    if (state != nullptr) {
        compact_quat_states(state, count);
    }
    *state_cursor = state;

    auto result = THISCALL(address, component, context, data_cursor,
                           state_cursor, a4, a5, a6, a7);
    if (state != nullptr) {
        expand_quat_states(state, count);
    }
    *state_cursor = reinterpret_cast<void *>(
        reinterpret_cast<uintptr_t>(state) + count * XB_QUAT_STATE_SIZE);
    return result;
}

							   
																	
																	 
 
																			
																	
											
																
													
 

int __fastcall update_entropy_quat(
    void *component, void *, uintptr_t *context, void **data_cursor,
    void **state_cursor, void *a4, int a5, unsigned int a6, int a7)
{
    return update_quat(0x00787890, component, context, data_cursor,
                       state_cursor, a4, a5, a6, a7);
}

int __fastcall update_packed8_quat(
    void *component, void *, uintptr_t *context, void **data_cursor,
    void **state_cursor, void *a4, int a5, unsigned int a6, int a7)
{
																			
																			 
    return update_quat(0x00787C30, component, context, data_cursor,
                       state_cursor, a4, a5, a6, a7);
}

int __fastcall update_packed16_quat(
    void *component, void *, uintptr_t *context, void **data_cursor,
    void **state_cursor, void *a4, int a5, unsigned int a6, int a7)
{
																		  
																	  
    return update_quat(0x00787DC0, component, context, data_cursor,
                       state_cursor, a4, a5, a6, a7);
}

					   
														   
															 
 
											  
															  
										  
 

							  
																	 
										
 
																		   
													
 

								  
																	 
										
 
																		   
													
 

					   
																		 
																		 
 
											  
																   
									   
 

							  
															  
																		 
 
																  
															   
 

								  
															  
																		 
 
																  
															   
 

								  
																	 
 
											  
																  
 

int __fastcall component_alignment(void *, void *)
{
    return PO_ALIGN;
}

bool patch_xbox_v10_po_layout()
{
    const xbpack::v10_po::instruction_patch *failed = nullptr;
    auto *code = reinterpret_cast<uint8_t *>(xbpack::v10_po::code_begin);
    if (!xbpack::v10_po::apply(code, xbpack::v10_po::code_size, &failed)) {
        sp_log("[xbpack] V10 PO patch rejected: unexpected instruction for "
               "%s at 0x%08X; PO instructions left unchanged",
               failed != nullptr ? failed->purpose : "unknown",
               failed != nullptr ? static_cast<unsigned>(failed->address) : 0u);
        return false;
    }

    FlushInstructionCache(GetCurrentProcess(), code, xbpack::v10_po::code_size);
    sp_log("[xbpack] V10 PO animation bridge installed: "
           "source/live stride=32, native cache stride=28 (%u checks)",
           static_cast<unsigned>(xbpack::v10_po::patch_count));
    return true;
}

bool patch_xbox_v10_pedestrian_flow()
{
    const xbpack::v10_ped::instruction_patch *failed = nullptr;
    auto *code = reinterpret_cast<uint8_t *>(xbpack::v10_ped::code_begin);
    if (!xbpack::v10_ped::apply(code, xbpack::v10_ped::code_size, &failed)) {
        sp_log("[xbpack] V10 pedestrian patch rejected: unexpected instruction "
               "for %s at 0x%08X; pedestrian instructions left unchanged",
               failed != nullptr ? failed->purpose : "unknown",
               failed != nullptr ? static_cast<unsigned>(failed->address) : 0u);
        return false;
    }

    FlushInstructionCache(GetCurrentProcess(), code, xbpack::v10_ped::code_size);
    sp_log("[xbpack] V10 beta pedestrian flow installed: "
           "threat selection and flee repathing restored (%u checks)",
           static_cast<unsigned>(xbpack::v10_ped::patch_count));

    auto *lane_code = reinterpret_cast<uint8_t *>(xbpack::v10_ped::lane_code_begin);
    if (!xbpack::v10_ped::apply_lanes(lane_code, xbpack::v10_ped::lane_code_size,
                                   &failed)) {
        sp_log("[xbpack] V10 lane patch rejected: unexpected instruction at 0x%08X",
               failed != nullptr ? static_cast<unsigned>(failed->address) : 0u);
        return false;
    }
    FlushInstructionCache(GetCurrentProcess(), lane_code,
                          xbpack::v10_ped::lane_code_size);
    sp_log("[xbpack] V10 pedestrian lane goals use beta success transitions");

    // Launched-layer interrupt hand-off (launch_layer_state 0x006AF344 /
    // 0x006AF362): the beta rule is applied to pedestrian graphs only; every
    // other graph (universal soldiers, bosses, heroes) keeps the stock PC
    // hand-off.  See xbpack_v10_combat.cpp and openusm.ini LayerInterrupts.
    if (!install_v10_layer_interrupt_bridge()) {
        sp_log("[xbpack] V10 layer interrupt bridge rejected: unexpected "
               "instructions at 0x006AF344/0x006AF362; launch_layer_state "
               "left unchanged");
        return false;
    }
    return true;
}

void __fastcall layout_ikspin(
    void *, void *, const nalGeneric::nalComponentInfo *info,
    void **cursor, void **)
{
    *cursor = align16(*cursor) + IKSPIN_SIZE * info->field_28;
}









void __fastcall advance_ikspin(
    void *, void *, const nalGeneric::nalComponentInfo *info,
    void **cursor)
{
    *cursor = align16(*cursor) + IKSPIN_SIZE * info->field_28;
}

void __fastcall copy_ikspin(
    void *, void *, const nalGeneric::nalComponentInfo *info,
    void **dst_cursor, const void **src_cursor)
{
    auto *dst = align16(*dst_cursor);
    auto *src = align16(const_cast<void *>(*src_cursor));
    auto size = IKSPIN_SIZE * info->field_28;

    std::memcpy(dst, src, size);
    *dst_cursor = dst + size;
    *src_cursor = src + size;
}

void compact_ikspin_states(uint8_t *data, int count)
{
    for (int i = 0; i < count; ++i) {
        auto *src = data + i * XB_IKSPIN_STATE_SIZE;
        auto *dst = data + i * PC_IKSPIN_STATE_SIZE;
        std::memmove(dst, src, PC_QUAT_STATE_SIZE);
        std::memmove(dst + PC_QUAT_STATE_SIZE,
                     src + XB_QUAT_STATE_SIZE,
                     PC_QUAT_STATE_SIZE);
        std::memmove(dst + 2 * PC_QUAT_STATE_SIZE,
                     src + 2 * XB_QUAT_STATE_SIZE,
                     0x40);
    }
}

void expand_ikspin_states(uint8_t *data, int count)
{
    for (int i = count - 1; i >= 0; --i) {
        auto *src = data + i * PC_IKSPIN_STATE_SIZE;
        auto *dst = data + i * XB_IKSPIN_STATE_SIZE;
        std::memmove(dst + 2 * XB_QUAT_STATE_SIZE,
                     src + 2 * PC_QUAT_STATE_SIZE,
                     0x40);
        std::memmove(dst + XB_QUAT_STATE_SIZE,
                     src + PC_QUAT_STATE_SIZE,
                     PC_QUAT_STATE_SIZE);
        std::memmove(dst, src, PC_QUAT_STATE_SIZE);
        std::memset(dst + PC_QUAT_STATE_SIZE, 0,
                    XB_QUAT_STATE_SIZE - PC_QUAT_STATE_SIZE);
        std::memset(dst + XB_QUAT_STATE_SIZE + PC_QUAT_STATE_SIZE, 0,
                    XB_QUAT_STATE_SIZE - PC_QUAT_STATE_SIZE);
    }
}

int __fastcall read_ikspin_control(
    void *component, void *, uintptr_t *context, void **state_cursor,
    void **source_cursor, unsigned int n2)
{
    auto count = active_quat_count(context);
    auto *state = align16(*state_cursor);
    *state_cursor = state;

    auto result = THISCALL(0x00789340, component, context,
                           state_cursor, source_cursor, n2);
    if (state != nullptr) {
        expand_ikspin_states(state, count);
    }
    *state_cursor = reinterpret_cast<void *>(
        reinterpret_cast<uintptr_t>(state) + count * XB_IKSPIN_STATE_SIZE);
    return result;
}

int __fastcall update_ikspin(
    void *component, void *, uintptr_t *context, void **data_cursor,
    void **state_cursor, void *a4, int a5, unsigned int a6, int a7)
{
    auto count = active_quat_count(context);
    auto *state = align16(*state_cursor);
    if (state != nullptr) {
        compact_ikspin_states(state, count);
    }
    *state_cursor = state;

    auto result = THISCALL(0x007895B0, component, context, data_cursor,
                           state_cursor, a4, a5, a6, a7);
    if (state != nullptr) {
        expand_ikspin_states(state, count);
    }
    *state_cursor = reinterpret_cast<void *>(
        reinterpret_cast<uintptr_t>(state) + count * XB_IKSPIN_STATE_SIZE);
    return result;
}

int __fastcall apply_ikspin(
    void *, void *, uintptr_t *context, uint8_t *dst,
    void **cache_cursor, const uint8_t *fallback, const int32_t *offsets)
{
    auto *info = reinterpret_cast<const nalGeneric::nalComponentInfo *>(context[1]);
    auto *cache = align16(*cache_cursor);

    for (int i = 0; i < info->field_28; ++i) {
        auto index = info->field_24 + i;
        auto offset = offsets[index];
        if (offset >= 0) {
            if (component_active(context, info, i)) {
                std::memcpy(dst + offset, cache, IKSPIN_SIZE);
                cache += IKSPIN_SIZE;
            } else {
                std::memcpy(dst + offset, fallback + offset, IKSPIN_SIZE);
            }
        } else if (component_active(context, info, i)) {
            cache += IKSPIN_SIZE;
        }
    }

    *cache_cursor = cache;
    return context[1];
}

int __fastcall skip_ikspin_cache(
    void *, void *, uintptr_t *context, void **cache_cursor,
    void **state_cursor, int, int)
{
    auto *info = reinterpret_cast<const nalGeneric::nalComponentInfo *>(context[1]);
    auto *cache = align16(*cache_cursor);
    *state_cursor = align_data(*state_cursor, alignof(void *));
    for (int i = 0; i < info->field_28; ++i) {
        if (component_active(context, info, i)) {
            cache += IKSPIN_SIZE;
        }
    }
    *cache_cursor = cache;
    return info->field_28;
}

int __fastcall advance_ikspin_cache(
    void *, void *, uintptr_t *context, void **cache_cursor)
{
    auto *cache = align16(*cache_cursor);
    *cache_cursor = cache + active_quat_count(context) * IKSPIN_SIZE;
    return context[1];
}

void *__fastcall find_event_data(void *anim, void *, void *handle)
{
    auto *data = reinterpret_cast<uint8_t *>(
        THISCALL(0x004AD9F0, anim, handle));

    if (data != nullptr && *reinterpret_cast<uint16_t *>(data + 2) == 0) {
        return nullptr;
    }

    return data;
}

#endif
}

#endif

bool install_xbpack_support()
{
#ifdef OPENUSM_XBPACK_MODE
    auto *platform = reinterpret_cast<uint8_t *>(PLATFORM_INITIALIZER);
    if (*platform != NL_PLATFORM_PC && *platform != NL_PLATFORM_XBOX) {
        sp_log("Unexpected g_platform initializer at 0x%08X: 0x%02X",
               static_cast<unsigned>(PLATFORM_INITIALIZER),
               *platform);
        return false;
    }

    *platform = NL_PLATFORM_XBOX;
    g_platform = NL_PLATFORM_XBOX;
    *reinterpret_cast<resource_versions *>(EXPECTED_RESOURCE_VERSIONS) =
        expected_resource_versions(NL_PLATFORM_XBOX);

    resource_manager_xbpack_patch();
    resource_directory_xbpack_patch();
    texture_resource_handler_xbpack_patch();
    material_file_resource_handler_xbpack_patch();
    morph_file_resource_handler_xbpack_patch();

 //   ngl_xbpack_patch();
    PanelMeshSection_patch();
    PanelFile_xbpack_patch();
    slc_manager_patch();
    script_manager_xbpack_patch();
    terrain_xbpack_patch();
    if (!wds_xbpack_patch())
        return false;

    if constexpr (xbpack::v10) {
#ifdef OPENUSM_XBPACK_V10
        // Complete and validate the native PO source/live consumers before
        // changing their vtables. The private PC cache and compressed quat/
        // IK-spin formats keep their native sizes and alignment.
        if (!patch_xbox_v10_po_layout())
            return false;

        if (!patch_xbox_v10_pedestrian_flow())
            return false;

        for (auto vtable : PO_VTABLES) {
            set_vfunc(vtable + 0x04, &po_size);
            set_vfunc(vtable + 0x10, &layout_po);
            set_vfunc(vtable + 0x2C, &advance_po);
            set_vfunc(vtable + 0x30, &advance_po);
            set_vfunc(vtable + 0x34, &copy_po);
            set_vfunc(vtable + 0x5C, &component_alignment);
        }

        if (!patch_xbox_v10_trajectory_tracks())
            return false;

																		   
																			   
																	
															  
															  

        for (auto vtable : QUAT_VTABLES) {
            set_vfunc(vtable + 0x10, &layout_quat);
            set_vfunc(vtable + 0x2C, &advance_quat);
            set_vfunc(vtable + 0x30, &advance_quat);
            set_vfunc(vtable + 0x34, &copy_quat);
            set_vfunc(vtable + 0x5C, &component_alignment);
        }

        set_vfunc(QUAT_VTABLES[1] + 0x14, &read_quat_control);
        set_vfunc(QUAT_VTABLES[2] + 0x14, &read_quat_control);
        set_vfunc(QUAT_VTABLES[3] + 0x14, &read_quat_control);
															
        set_vfunc(QUAT_VTABLES[1] + 0x18, &update_entropy_quat);
        set_vfunc(QUAT_VTABLES[2] + 0x18, &update_packed8_quat);
        set_vfunc(QUAT_VTABLES[3] + 0x18, &update_packed16_quat);
		
															   
														   
															   
															   
															   

        // Xbox v10 stores the same two decoder states as PC, but pads each
        // quaternion state from 0x34 to 0x40 bytes.  Bridge just those two
        // state records around the stock PC decoder; the 0x40-byte tail,
        // 0x50-byte pose and 0x16-byte encoded stride are otherwise shared.
        set_vfunc(IKSPIN_VTABLE + 0x10, &layout_ikspin);
        set_vfunc(IKSPIN_VTABLE + 0x14, &read_ikspin_control);
        set_vfunc(IKSPIN_VTABLE + 0x18, &update_ikspin);
        set_vfunc(IKSPIN_VTABLE + 0x2C, &advance_ikspin);
        set_vfunc(IKSPIN_VTABLE + 0x30, &advance_ikspin);
        set_vfunc(IKSPIN_VTABLE + 0x34, &copy_ikspin);
        set_vfunc(IKSPIN_VTABLE + 0x5C, &component_alignment);

        REDIRECT(0x0049C94B, find_event_data);
        SET_JUMP(0x005F6DA0, inverse_kinematics::LegsIK_BuildBoneMatrices_v10);
        damage_interface_xbpack_patch();
        cut_scene_player_xbpack_v10_patch();
        if (!cut_scene_segment_xbpack_v10_patch())
            return false;
        xbpack_v10_info_nodes_patch();
        if (!xbpack_v10_s03_patch())
            return false;
        if (!lego_map_xbpack_v10_patch())
            return false;
        if (!xbpack_v10_shocker_patch())
            return false;
        if (!xbpack_v10_electro_patch())
            return false;
        if (!xbpack_v10_green_goblin_patch())
            return false;
        if (!xbpack_v10_s07_web_patch())
            return false;
        if (!xbpack_v10_scene_pack_patch())
            return false;
        if (!xbpack_v10_directory_parents_patch())
            return false;
        xbpack_v10_gab_patch();
        if (!mission_manager_v10_patch())
            return false;
        xbpack_v10_feed_trace_patch();
        xbpack_v10_combat_trace_patch();

        constexpr uint8_t missing_exit_layer_fallback[] = {
            0xE9, 0x72, 0x00, 0x00, 0x00, // jmp 0x006CE393
            0x90, 0x90, 0x90, 0x90,
        };
        std::memcpy(reinterpret_cast<void *>(0x006CE31C),
                    missing_exit_layer_fallback,
                    sizeof(missing_exit_layer_fallback));
#endif

        *reinterpret_cast<uint8_t *>(0x0053D010) = 0xE9;
        *reinterpret_cast<uint32_t *>(0x0053D011) = 0x0053D02F - 0x0053D015;

        construct_debug_menu_lib();
        REDIRECT(0x00509DAC, parse_generic_mash_init);
        cg_mesh_patch();
        combo_system_patch();
        core_ai_resource_patch();
#ifdef OPENUSM_XBPACK_V10
        param_block_v10_patch();
#endif
    }

    als_resource_handler_xbpack_patch();
    ai_state_graph_resource_handler_xbpack_patch();
    mash_virtual_base_xbpack_patch();
    combo_system_move_patch();
    conglomerate_xbpack_patch();
    actor_xbpack_patch();
    ai_interact_resource_handler_xbpack_patch();

    if constexpr (!xbpack::v10) {
        gab_database_resource_handler_patch();
        sound_alias_database_resource_handler_xbpack_patch();
        base_ai_resource_handler_xbpack_patch();
    }
#endif

    return true;
}
