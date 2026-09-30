#include <gtest/gtest.h>

#include <tlresource_location.h>

TEST(TlResourceLocation, Construct)
{
    tlresource_location loc {};
    EXPECT_EQ(loc.get_data(), nullptr);
}
