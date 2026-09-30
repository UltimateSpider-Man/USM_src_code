#pragma once

#include "script_library_class.h"

struct mString;

struct slc_beam_t : script_library_class {
    slc_beam_t(const char *n, int sz, const char *p = nullptr) : script_library_class(n, sz, p, false) {
        m_vtbl = CAST(m_vtbl, 0x0089AAF8);
    }

    int _find_instance(const mString &a1) const;
};

extern slc_beam_t *slc_beam;

extern void script_lib_beam_patch();
