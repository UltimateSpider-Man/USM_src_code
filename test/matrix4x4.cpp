#include <gtest/gtest.h>

#include <custom_math.h>
#include <matrix4x4.h>
#include <ngl.h>
#include <ngl_math.h>
#include <vector3d.h>

TEST(Matrix4x4, Construct)
{
    matrix4x4 m {};
    EXPECT_EQ(m[0][0], 0.0f);
}

TEST(Matrix4x4, sub_771190)
{
    math::MatClass<4, 3> bones[2] = {
        matrix4x4 {
            1.00, 0.09, 0.02, 0.00,
            -0.02, 0.01, 1.00, 0.00,
            0.09, -1.00, 0.01, 0.00,
            0.70, 0.02, -0.37, 0.00
        },

        matrix4x4 {
            -0.04, 0.08, -1.00, 0.00,
            -0.85, -0.52, -0.01, -0.00,
            -0.52, 0.85, 0.09, 0.00,
            -1280.34, 16.76, -786.38, 1.00
        }
    };

    MatrixPair v6 {bones[0], bones[1]};

    matrix4x4 matrixFromMeshNode {
        1.00, 0.00, 0.00, 0.00,
        0.00, 1.00, 0.00, 0.00,
        0.00, 0.00, 1.00, 0.00,
        1280.00, -16.54, 786.72, 0.00
    };

    ComplexMatrixPair v7 {v6, matrixFromMeshNode};

    matrix4x4 mat {};
    mat.sub_771190(v7);

    const matrix4x4 expectedMat {
        -0.12, 0.05, -0.99, -0.00,
        -0.53, 0.84, 0.11, -0.00,
        0.84, 0.54, -0.07, 0.00,
        -0.19, -0.04, -0.39, -0.00
    };

    EXPECT_TRUE(approx_equals(mat, expectedMat, 0.1));
}
