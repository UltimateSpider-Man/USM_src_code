#include "render_text.h"

#include "fetext.h"
#include "mstring.h"
#include "os_developer_options.h"

#include "vector2di.h"

void render_text_unchecked(const mString &a1, const vector2di &a2, color32 a3, Float a4, Float a5)
{
    FEText fe_text{static_cast<font_index>(0),
                   static_cast<global_text_enum>(0),
                   static_cast<float>(a2.x),
                   static_cast<float>(a2.y),
                   static_cast<int>(a4),
                   static_cast<panel_layer>(0),
                   a5,
                   16,
                   0,
                   a3};

    fe_text.field_1C = a1;
    fe_text.Draw();
}

void render_text(const mString &a1, const vector2di &a2, color32 a3, Float a4, Float a5)
{
    auto *options = os_developer_options::instance;
    if (options != nullptr && options->get_flag(mString{"SHOW_DEBUG_TEXT"})) {
        render_text_unchecked(a1, a2, a3, a4, a5);
    }
}
