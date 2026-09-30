#include "ai_voice_box_inode.h"

#include "common.h"

VALIDATE_SIZE(ai::voice_box_inode, 0x44);

void ai::voice_box_inode::sub_6D7E10(const char *a2)
{
    static const string_hash speaker_id {to_hash("speaker_id")};

    this->my_param_block.set_pb_fixedstring(speaker_id, a2, true);
}
