#include <gtest/gtest.h>

#include <custom_math.h>
#include <mstring.h>
#include <vector3d.h>

TEST(MString, Construct)
{
    mString s {};
    EXPECT_EQ(s.size(), 0);
    EXPECT_EQ(s.c_str(), mString::null);
    EXPECT_EQ(s.field_C, nullptr);
}

TEST(MString, ConstructFromInt)
{
    mString s {1};
    EXPECT_EQ(s.size(), 1);
    EXPECT_NE(s.c_str(), mString::null);
    EXPECT_NE(s.field_C, nullptr);
}

TEST(MString, Test)
{
    for (int i = 0; i < 10; ++i)
    {
        EXPECT_EQ(mString(1).to_int(), 1);

        EXPECT_TRUE(approx_equals(mString(1.0f).to_float(), 1.0f, EPSILON));

        EXPECT_EQ(mString("   blah    ").remove_surrounding_whitespace(), mString("blah"));

        EXPECT_EQ(mString("blah").remove_leading("bl"), mString("ah"));

        EXPECT_EQ(mString("blah").remove_leading("alb"), mString("h"));

        EXPECT_EQ(mString("blah").remove_trailing("a"), mString("blah"));

        EXPECT_EQ(mString("blah").remove_trailing("ahl"), mString("b"));

        EXPECT_EQ(mString("blah").remove_trailing("ahl"), mString("b"));

        EXPECT_EQ(mString("qqq"), mString("qqq").data());

        EXPECT_EQ(mString("qqq"), mString("qqq").c_str());

        EXPECT_EQ(mString("qwerty").compare("qwerty"), 0);

        EXPECT_TRUE(mString("").empty());

        EXPECT_TRUE(mString("qwerty").is_equal(mString("QwERtY").to_lower().c_str()));

        EXPECT_TRUE(mString("QWERTY").is_equal(mString("qwErTy").to_upper().c_str()));

        EXPECT_EQ(mString("012345").substr(0, 6), mString("012345"));

        EXPECT_EQ(mString("012345").substr(1, 2), mString("12"));

        EXPECT_EQ(mString("012345").truncate(2), mString("01"));

        EXPECT_EQ(mString("foobar").slice(2, 4), "ob");

        EXPECT_EQ(mString("foobar").find("oo"), 1);

        EXPECT_EQ(mString("foobar").find("oo", 4), mString::npos);

        EXPECT_EQ(mString("foobar").find({4}, 'o'), mString::npos);

        EXPECT_EQ(mString("foobar").find(mString::pos_t{0}, 'b'), 3);

        EXPECT_EQ(mString("foobar").rfind('b'), 3);

        EXPECT_EQ(mString("foobar").rfind('b', 2), mString::npos);

        EXPECT_EQ(mString("foobar").rfind("oo"), 1);

        EXPECT_EQ(mString("foobarboobar").rfind("oo"), 7);

        EXPECT_EQ(mString("a") + mString("b"), mString("ab"));

        EXPECT_EQ("a" + mString("b"), mString("ab"));

        EXPECT_EQ(mString("a") + "b", mString("ab"));

        EXPECT_EQ(mString("a") + "b", "ab");

        EXPECT_NE(mString("a") + "b", "abb");

        EXPECT_NE(mString("a") + "b", mString("abb"));

        assert(mString("a") < mString("b"));

        assert(mString("b") > mString("a"));

        mString s1{};
        EXPECT_EQ(s1, "");

        static const char *test_strings[10] = {
            "",
            "1",
            "1234567",
            "12345678",
            "1234567890abcdef1234567890abcde",
            "1234567890abcdef1234567890abcdef",
            "1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef123456"
            "7890abcdef1234567890abcdef1234567890abcde",
            "1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef123456"
            "7890abcdef1234567890abcdef1234567890abcdef",
            "1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef123456"
            "7890abcdef1234567890abcdef1234567890abcde",
            "1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef123456"
            "7890abcdef1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef1234567890ab"
            "cdef1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcde"};

        mString s2{test_strings[i]};
        EXPECT_EQ(s2, test_strings[i]);

        auto s3 = s2;
        EXPECT_EQ(s3, test_strings[i]);

        mString s4 {0, "%s%s", test_strings[i], ""};
        EXPECT_EQ(s4, test_strings[i]);

        mString s41 {0, "%s%s", "", test_strings[i]};
        EXPECT_EQ(s41, test_strings[i]);

        mString s5 {0.1f};
        EXPECT_TRUE(approx_equals(atof(s5.c_str()), 0.1f, EPSILON));

        mString s6 {123123};
        EXPECT_EQ(s6, "123123");

        mString s7{-123123};
        EXPECT_EQ(s7, "-123123");

        mString s8 {};
        s8.copy(s4);

        EXPECT_EQ(s8, test_strings[i]);

        s8.copy("", -1);
        s8 += s4;

        EXPECT_EQ(s8, test_strings[i]);

        if (std::strlen(test_strings[i]) < 512)
        {
            s8.copy(test_strings[i], -1);
            s8.append(test_strings[i], -1);

            char buf[2048];
            std::sprintf(buf, "%s%s", test_strings[i], test_strings[i]);
            EXPECT_EQ(s8, buf);
        }

        if (std::strlen(test_strings[i]) < 1023)
        {
            s8.copy(test_strings[i], -1);
            s8.append('a');

            char buf[2048];
            std::sprintf(buf, "%s%c", test_strings[i], 'a');
            EXPECT_EQ(s8, buf);

            if (s8.length() > 0) {
                EXPECT_EQ(s8[s8.length() - 1], 'a');
            }
        }

        s8.update_guts("a", -1);
        EXPECT_EQ(s8, "a");

        s8 = s2;
        EXPECT_EQ(s8, test_strings[i]);

        s8 = test_strings[i];
        EXPECT_EQ(s8, test_strings[i]);
    }
}
