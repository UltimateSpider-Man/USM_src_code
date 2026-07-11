#include "collide_aux.h"

#include "capsule.h"
#include "log.h"
#include "local_collision.h"
#include "sphere.h"
#include "trace.h"
#include "utility.h"

#include <algorithm>
#include <cassert>
#include <cmath>

void compute_bounding_sphere_for_two_capsules(const capsule &cap0, const capsule &cap1, sphere *a3) 
{
    //sp_log("compute_bounding_sphere_for_two_capsules:");

    auto tmp = (cap0.base + cap0.end + cap1.base + cap1.end) * 0.25;

    float array_dist2[4];
    array_dist2[0] = (tmp - cap0.base).length2();
    array_dist2[1] = (tmp - cap0.end).length2();
    array_dist2[2] = (tmp - cap1.base).length2();
    array_dist2[3] = (tmp - cap1.end).length2();

    auto max_dist2 = *std::max_element(std::begin(array_dist2), std::end(array_dist2));

    a3->center = tmp;

    assert(max_dist2 >= 0.0f);

    float v20 = (cap0.radius >= cap1.radius ? cap0.radius : cap1.radius);

    a3->radius = std::sqrt(max_dist2) + v20;
}

void merge_spheres(const vector3d &a1,
                    Float radius_0,
                    const vector3d &a3,
                    Float radius_1,
                    vector3d &center_result,
                    float &radius_result)
{
    TRACE("merge_spheres");

    center_result = (a1 + a3) * 0.5f;

    auto v8 = a1 - center_result;
    auto v19 = v8.length();

    auto v9 = a3 - center_result;
    auto v11 = v9.length() - v19;

    auto delta = std::abs(v11);
    assert(delta < LARGE_EPSILON);

    auto v16 = v19 + radius_1;
    auto v17 = v19 + radius_0;
    radius_result = std::max(v17, v16) + EPSILON;
}

vector3d sub_5E2070(const vector3d &a2, const vector3d &a3, const vector3d &a4)
{
    auto v1 = dot(a4, a4);
    auto v2 = dot(a2 - a3, a4);
    auto v5 = v2 * a4;
    vector3d result = a2 - (v5 / v1);
    return result;
}

void closest_point_line_segment_plane(
        const vector3d &a1,
        const vector3d &a2,
        const vector3d &a3,
        const vector3d &a4,
        vector3d *line_result,
        vector3d *plane_result)
{
    TRACE("closest_point_line_segment_plane");

    assert(line_result != nullptr && plane_result != nullptr);

    auto a1a = dot(a1 - a3, a4);
    auto a3a = dot(a2 - a3, a4);
    if ( (a1a >= 0.0f || a3a >= 0.0f)
        && (a1a <= 0.0f || a3a <= 0.0f) )
    {
        auto v8 = a2 - a1;
        auto v9 = dot(v8, a4);
        if ( std::abs(v9) <= EPSILON ) {
            *line_result = a1;
            *plane_result = sub_5E2070(a1, a3, a4);
        } else {
            float v10 = (dot(a3, a4) - dot(a1, a4)) / v9;
            vector3d v14 = a1 + v10 * (a2 - a1);
            *plane_result = v14;
            *line_result = v14;
        }
    }
    else
    {
        vector3d v8;
        if ( std::abs(a3a) <= std::abs(a1a) )
        {
            *line_result = a2;
            v8 = sub_5E2070(a2, a3, a4);
        } else {
            *line_result = a1;
            v8 = sub_5E2070(a1, a3, a4);
        }

        *plane_result = v8;
    }
}

void closest_point_line_segment_line_segment(
        const vector3d &a1,
        const vector3d &a2,
        const vector3d &a3,
        const vector3d &a4,
        float *a5,
        float *a6)
{
    TRACE("closest_point_line_segment_line_segment");

    auto v28 = a2 - a1;
    auto v25 = a4 - a3;
    auto v31 = a1 - a3;

    auto v20 = dot(v28, v28);
    auto v34 = dot(v28, v25);
    auto v39 = dot(v25, v25);
    auto v40 = dot(v28, v31);
    auto v41 = dot(v25, v31);
    auto v6 = v39 * v20 - v34 * v34;
    if ( v6 < 0.000001f ) {
        *a5 = 0.0;
        closest_point_line_segment_point(a3, a4, a1, *a6);
        return;
    }

    if ( std::abs(v39) < 0.000001f )
    {
        *a6 = 0.0;
        closest_point_line_segment_point(a1, a2, a3, *a5);
        return;
    }

    if ( std::abs(v20) >= 0.000001f )
    {
        auto v23 = v41 * v34 - v40 * v39;
        auto v7 = v41 * v20 - v40 * v34;
        if ( v7 <= 0.0f || v7 >= v6 || v23 <= 0.0f || v23 >= v6 )
        {
            float v42[4] {};
            float v35[4] {};
            v42[0] = 1.0;
            v42[1] = 0.0;
            v42[2] = (v34 - v40) / v20;
            v42[3] = -v40 / v20;

            v35[0] = (v41 + v34) / v39;
            v35[1] = v41 / v39;
            v35[2] = 1.0;
            v35[3] = 0.0;

            v35[0] = std::clamp(v35[0], 0.0f, 1.0f);
            v35[1] = std::clamp(v35[1], 0.0f, 1.0f);
            v42[2] = std::clamp(v42[2], 0.0f, 1.0f);
            v42[3] = std::clamp(v42[3], 0.0f, 1.0f);

            double v10 = 3.4028235e38;
            int best_k = -1;

            for (int i {0}; i < 4; ++i) {
                auto v8 = v25 * v35[i];
                auto v6 = v28 * v42[i];
                auto v7 = v31 + v6;
                auto v21 = v7 - v8;
                auto v20 = dot(v21, v21);
                if (v10 > v20) {
                    v10 = v20;
                    best_k = i;
                }
            }

            assert(best_k != -1);
            *a5 = v42[best_k];
            *a6 = v35[best_k];
        }
        else
        {
            auto v8 = 1.0f / v6;
            *a6 = v7 * v8;
            *a5 = v8 * v23;
        }
    }
    else
    {
        *a5 = 0.0;
        closest_point_line_segment_point(a3, a4, a1, *a6);
    }
}

bool sub_5B8F40(const vector3d &a1, const vector3d &a2, const vector3d &a3, const vector3d &a4, float *t)
{
    assert(t != nullptr);

    auto v5 = (a2[2] - a1[2]) * a4[2]
            + (a2[1] - a1[1]) * a4[1]
            + (a2[0] - a1[0]) * a4[0];
    if ( fabs(v5) < EPSILON ) {
        return false;
    }

    *t = ((a3[2] - a1[2]) * a4[2]
        + (a3[1] - a1[1]) * a4[1]
        + (a3[0] - a1[0]) * a4[0]) / v5;
    return true;
}

void collide_aux_patch()
{
    SET_JUMP(0x005C3900, compute_bounding_sphere_for_two_capsules);

    SET_JUMP(0x005B8B90, closest_point_line_segment_plane);

    SET_JUMP(0x005C32E0, closest_point_line_segment_line_segment);
}
