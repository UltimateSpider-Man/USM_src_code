#include "debug_string.h"

#include "debug_render.h"

fixed_vector<debug_string_t, 25> *debug_strings{nullptr};

debug_string_t::debug_string_t(
    bool a2, const vector3d &a3, color32 a4, float a5, const mString &a1, float a7) {
    this->field_0 = a2;

    this->field_4 = a1;
    this->field_10 = a4;
    this->field_14 = a3;
    this->field_20 = a5;
    this->field_24 = a7;
}

void add_3d_debug_str(const vector3d &arg0, color32 a2, Float a3, const mString &a1)
{
    if (debug_strings != nullptr && 1) { //SHOW_DEBUG_TEXT
        debug_string_t *it = debug_strings->m_data;
        auto *end = &debug_strings->m_data[debug_strings->m_size];
        while (it != end) {
            if (it->field_20 <= 0.0) {
                it->field_0 = true;
                it->field_4 = a1;

                it->field_14 = arg0;
                it->field_10 = a2;
                it->field_20 = a3;
                it->field_24 = 0.5f;
                return;
            }

            ++it;
        }

        if (debug_strings->size() == 24) {
            debug_strings->sub_CBF970(23);
        }

        debug_string_t v4{true, arg0, a2, a3, a1, 0.5};

        debug_strings->push_back(v4);
    }
}

void render_3d_debug_strings()
{
    if (debug_strings == nullptr) {
        return;
    }

    for (auto &entry : *debug_strings) {
        if (entry.field_0 && entry.field_20 > 0.0f && !entry.field_4.empty()) {
            render_billboard(entry.field_14,
                             entry.field_10,
                             entry.field_24,
                             entry.field_4.c_str());
        }
    }
}

void frame_advance_3d_debug_strings(Float time_inc)
{
    if (debug_strings == nullptr || time_inc <= 0.0f) {
        return;
    }

    for (auto &entry : *debug_strings) {
        if (!entry.field_0) {
            continue;
        }

        entry.field_20 -= time_inc;
        if (entry.field_20 <= 0.0f) {
            entry.field_20 = 0.0f;
            entry.field_0 = false;
        }
    }
}

void clear_3d_debug_strings()
{
    if (debug_strings == nullptr) {
        return;
    }

    for (auto &entry : *debug_strings) {
        entry.field_0 = false;
        entry.field_20 = 0.0f;
    }

    debug_strings->m_size = 0;
}
