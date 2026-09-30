#include "ai_std_avoidance.h"

#include "common.h"

namespace ai {

VALIDATE_SIZE(avoidance_inode, 0x48);

VALIDATE_SIZE(ped_avoidance_inode, 0x54);

void avoidance_inode::set_respect_obbs(bool a2)
{
    static const string_hash respect_obbs_hash {int(to_hash("respect_obbs"))};

    this->my_param_block.set_pb_int(respect_obbs_hash, a2, true);
    if ( a2 != this->field_1C ) {
        this->field_1C = a2;
    }
}

}
