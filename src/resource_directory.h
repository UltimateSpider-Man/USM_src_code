#pragma once

#include "mashable_vector.h"
#include "resource_allocation_pool.h"
#include "resource_location.h"
#include "resource_pack_group.h"
#include "tlresource_location.h"

struct generic_mash_header;
struct tlHashString;
struct generic_mash_data_ptrs;
struct resource_pack_slot;

struct resource_directory {
    mashable_vector<resource_directory *> parents;
    mashable_vector<resource_location> resource_locations;
    mashable_vector<tlresource_location> texture_locations;
    mashable_vector<tlresource_location> mesh_file_locations;
    mashable_vector<tlresource_location> mesh_locations;
    mashable_vector<tlresource_location> morph_file_locations;
    mashable_vector<tlresource_location> morph_locations;
    mashable_vector<tlresource_location> material_file_locations;
    mashable_vector<tlresource_location> material_locations;
    mashable_vector<tlresource_location> anim_file_locations;
    mashable_vector<tlresource_location> anim_locations;
    mashable_vector<tlresource_location> scene_anim_locations;
    mashable_vector<tlresource_location> skeleton_locations;
    mashable_vector<resource_pack_group> field_68;
    mashable_vector<resource_allocation_pool> field_70;
    resource_pack_slot *pack_slot;

private:
    int base;

public:
    uint8_t *field_80;

    int field_84;
    int field_88;

#ifndef TARGET_XBOX
    int type_start_idxs[70];
#else
    int type_start_idxs[71];
#endif

#ifndef TARGET_XBOX
    int type_end_idxs[70];
#else
    int type_end_idxs[71];
#endif

    uint8_t * get_mash_data(uint32_t offset);

    void debug_print() const;

    //0x00563070
    int get_tlresource_count(tlresource_type a1);

    //0x00563090
    tlresource_location *get_tlresource_location(int idx, tlresource_type a2);

    char *add_tlresource(tlFixedString *arg0, tlresource_type tlres_type, char *data);

    int get_resource_count(resource_key_type type);

    int get_type_start_idxs(resource_key_type type);

    resource_location *get_resource_location(int idx);

    char *get_tlresource(tlresource_location *loc, resource_pack_slot **a3);

    //0x0052AAE0
    char *get_tlresource(uint32_t a2, tlresource_type tlres_type);

    char *get_tlresource(const tlHashString &a1, tlresource_type a2);

    char *get_tlresource(const tlFixedString &a1, tlresource_type a2);

    //0x0051F350
    bool find_tlresource(uint32_t a1,
                         tlresource_type tlres_type,
                         resource_directory **out_dir,
                         tlresource_location **out_loc);

    //0x0051F6E0
    void un_mash_start(generic_mash_header *header, void *a3, generic_mash_data_ptrs *a4, void *a5);

    //0x0050E380
    mashable_vector<tlresource_location> *tlresource_type_to_vector(tlresource_type a2);

    //0x0051F0D0
    void constructor_common(
        resource_pack_slot *a2, uint8_t *a3, uint8_t *a4, uint32_t a5, uint32_t a6);

    //0x00537D30
    void remove_parent(resource_directory *a2);

    //0x00537CC0
    void add_parent(resource_directory *new_dir);

    //0x0051F550
    bool find_resource(const resource_key &a2,
                       resource_directory **out_dir,
                       resource_location **out_loc);

    uint8_t *get_resource(const resource_location *loc, resource_pack_slot **a3);

    //0x0052AA70
    uint8_t *get_resource(const resource_key &resource_id, int *a3, resource_pack_slot **a4);

    //0x0051F620
    void *allocate_from_pool(int a2, int a3);

    //0x0051F7D0
    void release_mem();

    bool sub_9C2EE0();
};

extern void resource_directory_patch();
extern void resource_directory_xbpack_patch();
