#include <gtest/gtest.h>

#include <ngl.h>
#include <tlresource_directory.h>

TEST(TlResourceDirectory, Construct)
{
    tlresource_directory<nglTexture, tlFixedString> dir {};
    EXPECT_TRUE(dir.field_4 == nullptr);
}
