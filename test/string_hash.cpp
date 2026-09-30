#include <gtest/gtest.h>

#include <os_developer_options.h>
#include <string_hash_dictionary.h>

TEST(StringHash, Construct)
{
    string_hash hash {};
    EXPECT_EQ(0, hash.source_hash_code);
}

#ifdef SETUP_STANDALONE_SYSTEM

TEST(StringHash, ConstructWithString)
{
    os_developer_options::os_developer_init();

    g_platform = NL_PLATFORM_PC;

    string_hash_dictionary::create_inst();

    string_hash hash {"ultimate_spiderman"};
    EXPECT_TRUE(hash.source_hash_code != 0);
}

#endif
