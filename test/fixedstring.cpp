#include <gtest/gtest.h>

#include "character_anim_controller.h"
#include "fixedstring.h"

TEST(tlFixedString, Equal)
{
    tlFixedString a1 {"character"};
    tlFixedString a2 {"Character"};
    const tlFixedString a3 {CHARACTER_ANIMTYPE_NAME};

    EXPECT_EQ(a1, a3);
    EXPECT_EQ(a2, a3);
}

TEST(tlFixedString, NotEqual)
{
    tlFixedString a1 {"entity"};
    const tlFixedString a2 {CHARACTER_ANIMTYPE_NAME};

    EXPECT_NE(a1, a2);
}
