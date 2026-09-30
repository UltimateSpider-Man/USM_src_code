#include <gtest/gtest.h>

#include <custom_math.h>
#include <vector3d.h>
#include <vector4d.h>

TEST(Vector4d, Equal)
{
    vector4d a2(-0.706791, 0.706905, 0.019531, 0.018715);
    vector4d a3(-0.706791, 0.706867, 0.019531, 0.020097);
    vector4d result(-0.706791, 0.706896, 0.019531, 0.019060);

    EXPECT_TRUE(approx_equals(a2, result, LARGE_EPSILON));
    EXPECT_TRUE(approx_equals(a3, result, LARGE_EPSILON));

    a2 = {0.706579, -0.706934, 0.022704, 0.021800};
    a3 = {0.706579, -0.706890, 0.022704, 0.023182};
    result = {0.706579, -0.706923, 0.022704, 0.022145};

    EXPECT_TRUE(approx_equals(a2, result, LARGE_EPSILON));
    EXPECT_TRUE(approx_equals(a3, result, LARGE_EPSILON));

}

TEST(Vector4d, Test1)
{
    vector4d a2(0.000000, 22.713797, 22.713797, 0.000000);
    vector4d a3(-0.166667, 0.008333, -0.000192, 0.000000);
    vector4d a4(0.000000, 9.306329, 9.306329, 0.000000);
    vector4d a5(-0.166667, 0.008333, -0.000192, 0.000000);
    vector4d a6(0.000000, 3.813002, 3.813002, 0.000000);
    vector4d a7(-0.166667, 0.008333, -0.000192, 0.000000);
    vector4d a8(0.000000, 1.562268, 1.562268, 0.000000);
    vector4d expectedResult(0.000000, 0.999965, 0.999965, 0.000000);

    auto result = sub_5FC6D0(a2, a3, a4, a5, a6, a7, a8);
    EXPECT_TRUE(approx_equals(expectedResult, result, LARGE_EPSILON));
}

TEST(Vector4d, Test2)
{
    vector4d a2(0.000000, 0.000000, 0.000000, 0.000000);
    vector4d a3(-0.166667, 0.008333, -0.000192, 0.000000);
    vector4d a4(0.000000, 0.000000, 0.000000, 0.000000);
    vector4d a5(-0.166667, 0.008333, -0.000192, 0.000000);
    vector4d a6(0.000015, 0.000000, 0.000015, 0.000000);
    vector4d a7(-0.166667, 0.008333, -0.000192, 0.000000);
    vector4d a8(0.024860, 0.000000, 0.024860, 0.000000);
    vector4d expectedResult(0.024857, 0.000000, 0.024857, 0.000000);

    auto result = sub_5FC6D0(a2, a3, a4, a5, a6, a7, a8);
    EXPECT_TRUE(approx_equals(expectedResult, result, LARGE_EPSILON));
}

TEST(Vector4d, Test2_)
{
    vector4d a2(212.229340, -0.097064, 1590.017090, 0.000000);
    vector4d a3(-0.166667, 0.008333, -0.000192, 0.000000);
    vector4d a4(45.920212, -0.189004, 193.515411, 0.000000);
    vector4d a5(-0.166667, 0.008333, -0.000192, 0.000000);
    vector4d a6(9.935788, -0.368029, 23.552084, 0.000000);
    vector4d a7(-0.166667, 0.008333, -0.000192, 0.000000);
    vector4d a8(2.149813, -0.716628, 2.866442, 0.000000);
    vector4d expectedResult(0.835819, -0.656847, 0.248818, 0.000000);

    auto result = sub_5FC6D0(a2, a3, a4, a5, a6, a7, a8);
    EXPECT_TRUE(approx_equals(expectedResult, result, LARGE_EPSILON));
}

TEST(Vector4d, Test3)
{
    vector4d a2(-0.597565, 0.055536, -0.579416, 0.551462);
    vector4d a3(0.999998, 0.000002, 1.000000, 0.000000);
    vector4d a4(-0.597565, 0.055536, -0.579416, 0.551462);
    vector4d a5(0.999998, 0.000002, 1.000000, 0.000000);
    vector4d expectedResult(-0.597565, 0.055536, -0.579416, 0.551462);

    auto result = sub_5FC770(a2, a3, a4, a5);
    EXPECT_TRUE(approx_equals(expectedResult, result, LARGE_EPSILON));
}

TEST(Vector4d, Test4)
{
    vector4d a2(0.000000, 0.000000, 0.000000, 1.000000);
    vector4d a3(0.000000, 1.000000, 1.000000, 0.000000);
    vector4d a4(0.000000, 0.991485, 0.130497, 0.008527);
    vector4d a5(0.000000, 1.000000, 1.000000, 0.000000);
    vector4d expectedResult(0.000000, 0.991485, 0.130497, 0.008527);

    auto result = sub_5FC770(a2, a3, a4, a5);
    EXPECT_TRUE(approx_equals(expectedResult, result, LARGE_EPSILON));
}

TEST(Vector4d, Test5)
{
    vector4d a2(-0.706791, 0.706905, 0.019531, 0.018715);
    vector4d a3(3.359164, -2.639871, 1.000000, 0.000000);
    vector4d a4(-0.706791, 0.706867, 0.019531, 0.020097);
    vector4d a5(3.359164, -2.639871, 1.000000, 0.000000);
    vector4d expectedResult(-0.508389, 0.508572, 0.014048, 0.009813);

    auto result = sub_5FC770(a2, a3, a4, a5);
    EXPECT_TRUE(approx_equals(expectedResult, result, LARGE_EPSILON));
}

TEST(ArbitraryPOCharComp, Test6)
{
    const float a2 = 0.125003f;
    vector4d a3{0.706579, -0.706934, 0.022704, 0.021800};
    vector4d a4{0.706543, -0.706543, 0.022705, -0.022850};
    const vector4d expectedResult{0.706673, -0.706983, 0.022707, 0.016219};

    const auto result = sub_5FD0C0(a2, a3, a4);
    EXPECT_TRUE(approx_equals(result, expectedResult, LARGE_EPSILON));
}

TEST(ArbitraryPOCharComp, Test7)
{
    const float a2 = 0.125003f;
    vector4d a3{0.00854492, -0.00146484, 0.00854492, 0.999926};
    vector4d a4{0.00854492, -0.00146484, 0.00854492, 0.999926};
    const vector4d expectedResult{0.00854492, -0.00146484, 0.00854492, 0.999926};

    const auto result = sub_5FD0C0(a2, a3, a4);
    EXPECT_TRUE(approx_equals(result, expectedResult, LARGE_EPSILON));
}
