#include <gtest/gtest.h>

#include <resource_key.h>

TEST(ResourceKey, Construct)
{
    resource_key key {};
    assert(key.get_type() == RESOURCE_KEY_TYPE_NONE);
}
