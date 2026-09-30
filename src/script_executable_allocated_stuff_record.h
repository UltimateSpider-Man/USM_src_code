#pragma once

#include <list.hpp>

#include <cstdint>

struct script_executable;
struct mString;

struct script_executable_allocated_stuff_record {
    void (*field_0)(script_executable *, _std::list<uint32_t> &, _std::list<mString> &);
    _std::list<uint32_t> stuff;
    _std::list<mString> debug_stuff_descriptions;

    script_executable_allocated_stuff_record() = default;
};
