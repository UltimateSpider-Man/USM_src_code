#include "event.h"

struct anim_event : event {
    string_hash field_C;
    string_hash *field_10;
    int my_num_params;

    //0x004AD3F0
    anim_event(string_hash a2, string_hash a3, int count);

    //0x0043A570
    ~anim_event();
};
