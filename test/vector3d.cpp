#include <gtest/gtest.h>

#include <custom_math.h>
#include <vector3d.h>

TEST(Vector3d, Construct)
{
    vector3d v {};
    EXPECT_EQ(v[0], 0.0f);
}

TEST(Vector3d, ReorientVectors)
{
    vector3d forward {0.985648, -0.0516526, 0.160719};
    vector3d up {0, 0, 0};
    vector3d a1 {0.981168, -0.0855519, 0.173175};
    vector3d a4 {0.158808, 0.867648, -0.471135};
    vector3d a10 {0.123463, 0.869854,-0.47761};

    reorient_vectors(a1,
                    a4,
                    forward,
                    a10,
                    forward, up, 0.5f);

    EXPECT_TRUE(approx_equals(forward, vector3d(0.983571, -0.0686128, 0.166977), LARGE_EPSILON));
    EXPECT_TRUE(approx_equals(up, vector3d(0.141156, 0.868903, -0.474428), LARGE_EPSILON));
}

TEST(Vector3d, sub_444A60)
{
    vector3d a2 {6.716872, -32.072525, 14.798354};
    vector3d a3 {0.606163, 0.554652, 0.570024};
    auto res = sub_444A60(a2, a3);

    EXPECT_TRUE(approx_equals(res, vector3d (9.918718, -29.142769, 17.809311), LARGE_EPSILON));

    a2 = {16.458485, -15.374539, 26.640459};
    a3 = {0.563501, 0.673320, 0.478650};

    res = sub_444A60(a2, a3);

    EXPECT_TRUE(approx_equals(res, vector3d (9.880253, -23.234779, 21.052763), LARGE_EPSILON));
}
