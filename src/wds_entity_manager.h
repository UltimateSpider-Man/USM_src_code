#pragma once

#include "float.hpp"
#include "multi_vector.h"
#include "string_hash.h"

#include <list.hpp>
#include <vector.hpp>

struct box_trigger;
struct camera;
struct convex_box;
struct entity;
struct po;
struct mString;
struct region;
struct item;
struct vector3d;

struct wds_entity_manager {
    multi_vector<entity *> entities;
    multi_vector<item *> items;
    int field_18;
    void *field_1C;
    int field_20;
    int field_24;
    entity *field_28;

    //0x005DF4C0
    wds_entity_manager();

    inline auto *get_entities() {
        return &this->entities;
    }

    inline auto *get_items() {
        return &this->items;
    }

    bool is_entity_valid(entity *a1);

    bool is_item_valid(item *a2);

    entity *acquire_entity(
        string_hash a2,
        string_hash a3,
        uint32_t a6);

    //0x005E0D40
    entity *acquire_entity(string_hash a1, uint32_t a2);

    //0x005B85A0
    entity_base *get_entity(string_hash a1);

    //0x005E0760
    void add_dynamic_instanced_entity(entity *a2);

    void destroy_all_entities_and_items();

    //0x005DFAB0
    entity *add_to_entities(_std::vector<entity *> *vec, entity *a3);

    //0x005DFB10
    int add_entity_internal(_std::vector<entity *> *vec, entity *cam);

    //0x005DBBC0
    void make_time_limited(entity *a1, Float a2);

    //0x005B8590
    void remove_entity_from_misc_lists(entity *a1);

    //0x005DF7D0
    item *add_item(_std::vector<item *> *a2, item *a3);

    //0x005D5410
    bool remove_item(item *a2);

    //0x005D5350
    bool remove_entity(entity *a2);

    //0x005D6F70
    void release_entity(entity *e);

    //0x005D6F20
    void destroy_entity(entity *e);

    //0x005E0970
    int add_ent_to_lists(_std::vector<entity *> *a2, _std::vector<item *> *a3, entity *ent);

    //0x005E0A10
    entity *create_and_add_entity_or_subclass(string_hash a2,
                                          string_hash a3,
                                          const po &a4,
                                          const mString &a5,
                                          uint32_t a6,
                                          const _std::list<region *> *a7);

    //0x005C2D80
    box_trigger *create_and_add_box_trigger(
            string_hash a1,
            const vector3d &a3,
            const convex_box &a4);

    //0x005D92D0
    void process_time_limited_entities(Float a2);

    //0x005DFC80
    void add_camera(_std::vector<entity *> *vec, camera *a2);

    //0x005DFC90
    void add_mic(_std::vector<entity *> *a1, mic *a2);

    void check_water(Float);

    //0x005DBC00
    void frame_advance(Float a2);
};

extern void wds_entity_manager_patch();

void __fastcall debug_character_destroy_all_entities(
    wds_entity_manager *self,
    void *);

#ifdef OPENUSM_XBPACK_MODE
void __fastcall xbpack_destroy_all_entities(wds_entity_manager *self, void *);
#endif
