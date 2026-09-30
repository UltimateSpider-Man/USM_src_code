#include "scnanims.h"

#include "mstring.h"
#include "variable.h"
#include "utility.h"

#include <cstdlib>
#include <new>

namespace
{
    // Stock storage for the standalone scene-animation pack name.  0x8AA994
    // is the read-only "scnanims" literal, not the mString object.
    mString &g_scnanims = var<mString>(0x0096FB80);

    void sub_86DD90()
    {
        g_scnanims.~mString();
    }
}

int sub_85E1B0()
{
    new (&g_scnanims) mString("scnanims");

    return std::atexit(sub_86DD90);
}

void scnanims_patch()
{
    // 0x91D144 is the stock scnanims initializer entry.  The preceding entry
    // at 0x91D13C registers PanelEffect and must remain untouched.
    set_vfunc(0x0091D144, &sub_85E1B0);
}
