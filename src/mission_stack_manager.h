#pragma once

#include "resource_pack_slot.h"
#include "variable.h"

struct mString;
struct resource_pack_group;

struct mission_stack_manager {
    int pack_loads_or_unloads_pending;
    int field_4;
    bool loading_started;
    bool unloading_started;
    bool field_A;
    bool field_B;

    //0x005BB5D0
    mission_stack_manager();
	
	//0x005D8430
	void sub_5D8430(const mString &a1, const mString &a2);


    //0x005D2250
    resource_pack_group *get_pack_group_for_pack(const mString &a1);

    //0x005D21A0
    resource_pack_group *get_pack_group(const mString &a1);

    //0x005BB640
    bool waiting_for_push_or_pop();

    //0x005D82D0
    void insert_mission_pack(resource_pack_group *a2, const mString &a3, int a4);

    //0x005D7FE0
    void push_mission_pack(const mString &a2,
                           const mString &a3,
                           int district_slot_override_idx,
                           bool a5);

    //0x005D5800
    void pop_mission_pack(const mString &a2, const mString &a3);

    //0x005D1F70
    static void map_directory_parent(resource_pack_slot *a1);

    //0x005D22F0
    void pop_mission_pack_internal();

    //0x005D2360
    bool is_pack_pushed(const mString &a1);

    //0x005D5940
    void pop_mission_pack_immediate(const mString &a1, const mString &a2);

    //0x005D8430
    void push_mission_pack_immediate(const mString &a1, const mString &a2);

    //0x005D56E0
    bool nonstatic_mission_stack_callback(resource_pack_slot::callback_enum a2,
                                          resource_pack_streamer *a3,
                                          resource_pack_slot *a4,
                                          limited_timer *a5);

    //0x005D1FC0
    static void unmap_directory_parent(resource_pack_slot *a1);

    //0x005D7FC0
    static bool mission_stack_callback(resource_pack_slot::callback_enum a1,
                                       resource_pack_streamer *a2,
                                       resource_pack_slot *a3,
                                       limited_timer *a5);

    //0x005BB620
    void start_streaming();

    static mission_stack_manager *get_instance();

    //0x0096851C
    static mission_stack_manager *& s_inst;
};

extern void mission_stack_manager_patch();
