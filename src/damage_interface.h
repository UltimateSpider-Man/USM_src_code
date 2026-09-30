#pragma once

#include "generic_interface.h"

#include "float.hpp"
#include "vector3d.h"

#include <list.hpp>
#include <vector.hpp>

struct actor;
struct resource_key;

template<typename T>
struct bounded_variable {
    T field_0[4];

    void sub_48BFB0(const T &a2);
};

struct damage_interface : generic_interface {
    actor *field_4;
    char field_8[0x1F4];

    bounded_variable<float> field_1FC;
    bounded_variable<float> field_20C;
    bounded_variable<float> field_21C;
    bounded_variable<int> field_22C;

    //0x004DE8A0
    damage_interface(actor *a2);

    //0x004D9BF0
    ~damage_interface();

    /* virtual */ bool get_ifc_num(const resource_key &att, float *a3, bool is_log);

    /* virtual */ bool set_ifc_num(const resource_key &att, Float a3, bool is_log);

    //virtual
    void _un_mash(
        generic_mash_header *a2,
        void *a3,
        void *a4,
        generic_mash_data_ptrs *a5);

    //virtual
    void frame_advance(Float a3);

    //0x004D1990
    static void frame_advance_all_damage_ifc(Float a1);

    static inline Var<_std::vector<damage_interface *> *> all_damage_interfaces {0x0095A660};

    static inline Var<_std::list<damage_interface *> *> found_damageable {0x0095A5EC};
};


extern void damage_interface_patch();
extern void damage_interface_xbpack_patch();
