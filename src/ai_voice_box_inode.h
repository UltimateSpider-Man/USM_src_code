#pragma once

#include "info_node.h"

struct ai_lip_sync;

namespace ai {

struct voice_box_inode : info_node {
    int current_sound;
    int field_20;
    string_hash field_24;
    ai_lip_sync *lip_sync;
    int field_2C;
    int field_30;
    int field_34;
    int field_38;
    short field_3C;
    char field_3E;
    char field_3F;
    int field_40;

    void sub_6D7E10(const char *a2);

    inline static const string_hash default_id {int(to_hash("VOICE_BOX"))};
};

}
