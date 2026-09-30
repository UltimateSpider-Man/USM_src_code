#include "anim_event.h"

#include "common.h"
#include "func_wrapper.h"

VALIDATE_SIZE(anim_event, 0x18);

anim_event::anim_event(string_hash a2, string_hash a3, int count) : event(a2)
{
    THISCALL(0x004AD3F0, this, a2, a3, count);
}

anim_event::~anim_event()
{
    THISCALL(0x0043A570, this);
}
