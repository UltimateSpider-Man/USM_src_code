#pragma once

#include "info_node.h"

#include <vector.hpp>

namespace ai {

struct pedestrian_inode;

struct avoidance_inode : info_node {
    bool field_1C;
    _std::vector<void *> *field_20;
    int field_24;
    int field_28;
    int field_2C;
    int field_30;
    int field_34;
    int field_38;
    int field_3C;
    int field_40;
    bool field_44;
    char empty[3];

    //0x006D74E0
    void set_respect_obbs(bool a2);
};

struct ped_avoidance_inode : avoidance_inode {
    bool field_48;
    pedestrian_inode *field_4C;
    float field_50;

    static inline const string_hash default_id {int(to_hash("AVOIDANCE_INODE"))};
};

}
