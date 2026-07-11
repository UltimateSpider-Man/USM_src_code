#pragma once

#include "mstring.h"
#include "resource_partition.h"

#include <vector.hpp>

namespace resource_manager {

//0x0050DD70
nflFileID open_pack(const char *a2);

// openusm: standalone-pack open with platform fallback.
// Tries the Xbox pack folder/extension first (packs\xbox\<name>.XBPACK),
// then the host platform's (e.g. packs\pc\<name>.PCPACK), and reports the
// chosen file's on-disk size in bytes via out_data_size (may be null).
// Returns NFL_FILE_ID_INVALID if no candidate file exists in any folder.
nflFileID open_pack_ex(const char *name, int *out_data_size);

// openusm: choose which amalga index to load. Prefers packs\amalga_XB.PAK
// (beta), then the host platform's index (e.g. packs\amalga_PC.PAK / final),
// based on which file is actually present next to the game. Falls back to the
// native-platform name when neither is found so the existing error path is
// meaningful.
mString resolve_amalgapak_filename();

//0x0050DF70
resource_key *get_prerequisiste(int prereq_idx);

//0x0050DF60
int get_pack_location_count();

void add_resource_pack_modified_callback(void (*)(_std::vector<resource_key> &));

bool using_amalgapak();

bool is_idle();

void set_active_district(bool a1);

//0x00537650
void load_amalgapak();

//0x0053DE90
bool can_reload_amalgapak();

//0x0054C2E0
void reload_amalgapak();

//0x0055BA30
void create_inst();

//0x00547AD0
void delete_inst();

//0x00558930
void configure_packs_by_memory_map(int a1);

//0x005375A0
resource_pack_slot *get_best_context(resource_pack_slot *slot);

//0x0051ED70
bool get_pack_location(int a1, resource_pack_location *a2);

//0x00537610
resource_pack_slot *get_best_context(resource_partition_enum a1);

resource_pack_slot *get_and_push_resource_context(resource_partition_enum a1);

//0x00558D20
void frame_advance(Float a2);

//0x0052A820
extern bool get_pack_file_stats(const resource_key &a1,
                                resource_pack_location *a2,
                                mString *a3,
                                int *a4);

//0x00542740
resource_pack_slot *push_resource_context(resource_pack_slot *pack_slot);

//0x00537A10
resource_directory *get_resource_directory(const resource_key &a1);

//0x0051EC80
void set_active_resource_context(resource_pack_slot *a1);

resource_pack_slot *pop_resource_context();

resource_pack_slot *get_resource_context();

extern _std::vector<resource_partition *> *& partitions;

extern mString & amalgapak_name;

extern int & amalgapak_base_offset;

extern bool & using_amalga;

extern int & amalgapak_signature;

extern int & amalgapak_pack_location_count;

extern resource_pack_location *& amalgapak_pack_location_table;

extern int & amalgapak_prerequisite_count;

extern resource_key *& amalgapak_prerequisite_table;

extern int & resource_buffer_size;
extern int & memory_maps_count;

using modified_callback_t = void (*)(_std::vector<resource_key> &a1);

inline auto & resource_pack_modified_callbacks = var<_std::vector<modified_callback_t>>(0x0095CAC4);

struct resource_memory_map {
    char field_0;
    int field_4;
    int field_8;
    int field_C;
    struct {
        int field_0;
        int field_4;
        int field_8;
        int field_C;
    } field_10[8];

    resource_memory_map() {
        this->field_0 = 0;
        std::memset(this->field_10, 0, sizeof(this->field_10));
    }
};

extern resource_memory_map *& memory_maps;

//0x00537AA0
extern resource_partition *get_partition_pointer(resource_partition_enum which_type);

bool get_resource_if_exists(const resource_key &resource_id,
                            void *a2,
                            uint8_t **a3,
                            worldly_pack_slot *slot_ptr,
                            int *mash_data_size);

//0x00531B30
uint8_t *get_resource(const resource_key &resource_id, int *a2, resource_pack_slot **a3);

extern nflFileID & amalgapak_id;

extern int & in_use_memory_map;

extern uint8_t *& resource_buffer;

extern int & resource_buffer_used;

extern _std::vector<resource_pack_slot *> & resource_context_stack;

} // namespace resource_manager

extern void resource_manager_patch();
