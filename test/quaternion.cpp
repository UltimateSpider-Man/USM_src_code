#include <gtest/gtest.h>

#include <quaternion.h>

TEST(Quaternion, Construct)
{
    quaternion q {};
    EXPECT_EQ(q[0], 0.0f);
}

TEST(Quaternion, Slerp)
{
    quaternion a2 {0.867351, -0.396007, -0.089976, 0.287723};
    quaternion a3 {0.856533, -0.408080, -0.231215, 0.215320};
    float lambda = 0.0f;

    quaternion res = slerp(a2, a3, lambda);

    EXPECT_EQ(res, quaternion(0.867352, -0.396084, -0.090612, 0.287417));
}
