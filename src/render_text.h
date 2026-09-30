#pragma once

#include "color32.h"
#include "float.hpp"

struct mString;
struct vector2di;

//0x00578830
extern void render_text(const mString &a1, const vector2di &a2, color32 a3, Float a4, Float a5);

// Debug-world labels must be drawable without changing the global
// SHOW_DEBUG_TEXT developer option.  Ordinary HUD callers should continue to
// use render_text(), which retains the original option check.
extern void render_text_unchecked(const mString &a1, const vector2di &a2, color32 a3, Float a4, Float a5);
