#pragma once

// Explicit rest-pose assistance for unrigged custom meshes. All matrices act
// on native bind-model points, using the game's row-vector convention. This
// changes imported vertex attributes only; gameplay poses remain untouched.
#include "mod_mesh_retarget.h"
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <functional>

namespace modmesh { namespace donorpose {
namespace rt = retarget;

struct Plan {
    std::vector<rt::Matrix> deformation;
    std::vector<uint8_t> arm; // 0 body, 1 left upperarm subtree, 2 right subtree
    int upperarm[2] = {-1, -1};
};

inline std::string boneKey(std::string name)
{
    if (const auto nul = name.find('\0'); nul != std::string::npos) name.resize(nul);
    if (const auto prefix = name.rfind("::"); prefix != std::string::npos) name.erase(0, prefix + 2);
    for (auto &c : name) c = char(std::tolower(static_cast<unsigned char>(c)));
    name.erase(std::remove_if(name.begin(), name.end(), [](char c) {
        return c == ' ' || c == '_' || c == '-';
    }), name.end());
    if (name.compare(0, 5, "bip01") == 0) name.erase(0, 5);
    return name;
}

inline bool fail(std::string *why, const char *reason)
{
    if (why) *why = reason;
    return false;
}

inline bool prepareArmsDown(const std::vector<std::string> &names,
                            const std::vector<int> &parents,
                            const std::vector<float> &bindPositions,
                            Plan &out, std::string *why = nullptr,
                            double leftPitch = 0.0, double rightPitch = 0.0)
{
    if (!std::isfinite(leftPitch) || !std::isfinite(rightPitch)
        || std::abs(leftPitch) > 80.0 || std::abs(rightPitch) > 80.0)
        return fail(why, "donor arm pitches must be finite degrees in [-80,80]");
    const size_t count = names.size();
    if (!count || count > 1024 || parents.size() != count || bindPositions.size() != count * 3)
        return fail(why, "missing validated native bone names, parents or bind pivots");
    for (float v : bindPositions) if (!std::isfinite(v)) return fail(why, "non-finite native bind pivot");
    std::vector<uint8_t> state(count);
    std::function<bool(size_t)> visit = [&](size_t i) {
        if (state[i] == 1) return false;
        if (state[i] == 2) return true;
        state[i] = 1;
        if (parents[i] < -1 || parents[i] >= int(count)
            || (parents[i] >= 0 && !visit(size_t(parents[i])))) return false;
        state[i] = 2;
        return true;
    };
    for (size_t i = 0; i < count; ++i)
        if (!visit(i)) return fail(why, "invalid native arm hierarchy");
    auto descendant = [&](int child, int ancestor) {
        for (int i = child; i >= 0; i = parents[size_t(i)]) if (i == ancestor) return true;
        return false;
    };
    auto find = [&](const char *key) {
        int result = -1;
        for (size_t i = 0; i < count; ++i) if (boneKey(names[i]) == key) {
            if (result >= 0) return -1;
            result = int(i);
        }
        return result;
    };
    Plan plan;
    plan.deformation.assign(count, rt::identity());
    plan.arm.assign(count, 0);
    const char *keys[2][3] = {{"lupperarm", "lforearm", "lhand"},
                             {"rupperarm", "rforearm", "rhand"}};
    for (int side = 0; side < 2; ++side) {
        const int upper = find(keys[side][0]), elbow = find(keys[side][1]), hand = find(keys[side][2]);
        if (upper < 0 || elbow < 0 || hand < 0
            || !descendant(elbow, upper) || !descendant(hand, elbow))
            return fail(why, "missing or ambiguous upperarm/forearm/hand chain");
        plan.upperarm[side] = upper;
        const rt::Vec3 pivot{{bindPositions[size_t(upper)*3], bindPositions[size_t(upper)*3+1], bindPositions[size_t(upper)*3+2]}};
        double direction[3]; double length = 0;
        for (int c = 0; c < 3; ++c) {
            direction[c] = double(bindPositions[size_t(elbow)*3+c]) - pivot[c];
            length += direction[c]*direction[c];
        }
        length = std::sqrt(length);
        if (length < 1e-5) return fail(why, "coincident upperarm and forearm pivots");
        for (double &v : direction) v /= length;
        // Shortest proper rotation from the native upperarm direction to the
        // explicit down/forward direction. Positive pitch points toward +Z.
        // This preserves its authored twist and does not assume fixed indices
        // or a perfectly horizontal native T-pose.
        const double pitch = (side == 0 ? leftPitch : rightPitch)*3.14159265358979323846/180.0;
        const double target[3] = {0.0, -std::cos(pitch), std::sin(pitch)};
        const double cosine = std::clamp(direction[0]*target[0] + direction[1]*target[1] + direction[2]*target[2], -1.0, 1.0);
        double axis[3] = {direction[1]*target[2]-direction[2]*target[1],
            direction[2]*target[0]-direction[0]*target[2], direction[0]*target[1]-direction[1]*target[0]};
        const double sine = std::sqrt(axis[0]*axis[0] + axis[1]*axis[1] + axis[2]*axis[2]);
        rt::Matrix rotate = rt::identity();
        if (sine < 1e-8) {
            if (cosine < 0) return fail(why, "upward native arm has ambiguous arms-down rotation");
        } else {
            for (double &v : axis) v /= sine;
            for (int row = 0; row < 3; ++row) {
                double basis[3] = {0,0,0}; basis[row] = 1;
                const double cross[3] = {axis[1]*basis[2]-axis[2]*basis[1],
                    axis[2]*basis[0]-axis[0]*basis[2], axis[0]*basis[1]-axis[1]*basis[0]};
                for (int c = 0; c < 3; ++c)
                    rotate[row*4+c] = float(cosine*basis[c] + sine*cross[c]
                        + (1-cosine)*axis[row]*axis[c]);
            }
        }
        const auto movedPivot = rt::vector(pivot, rotate);
        for (int c = 0; c < 3; ++c) rotate[12+c] = pivot[c] - movedPivot[c];
        for (size_t i = 0; i < count; ++i) if (descendant(int(i), upper)) {
            if (plan.arm[i]) return fail(why, "overlapping left and right arm chains");
            plan.arm[i] = uint8_t(side + 1);
            plan.deformation[i] = rotate;
        }
    }
    out = std::move(plan);
    if (why) why->clear();
    return true;
}

inline bool blended(const Plan &plan, const float indices[4], const float weights[4],
                     rt::Matrix &out, std::string *why = nullptr)
{
    double total = 0, matrix[16] = {};
    for (int lane = 0; lane < 4; ++lane) {
        const float weight = weights[lane], index = indices[lane];
        if (!std::isfinite(weight) || weight < 0.f || !std::isfinite(index))
            return fail(why, "non-finite or negative donor-pose influence");
        if (!weight) continue;
        if (index < 0.f || index != std::floor(index) || index >= double(plan.deformation.size()))
            return fail(why, "donor-pose influence is outside the native skeleton");
        const auto &deform = plan.deformation[size_t(index)];
        for (int c = 0; c < 16; ++c) matrix[c] += double(weight)*deform[c];
        total += weight;
    }
    if (total <= 1e-8 || !std::isfinite(total)) return fail(why, "weightless donor-pose vertex");
    for (int c = 0; c < 16; ++c) out[c] = float(matrix[c]/total);
    if (!rt::affine(out)) return fail(why, "invalid blended donor-pose transform");
    return true;
}

// Pose donors with undo=false; after assigning their native weights, recover
// custom vertices' native-bind positions with undo=true. Invert the weighted
// affine matrix, not a weighted average of its inverses. The latter is wrong
// at blended shoulder joints and creates seams/stretch even in the rest pose.
inline bool transform(const Plan &plan, const float indices[4], const float weights[4],
                       const rt::Vec3 &point, const rt::Vec3 &normal, bool undo,
                       rt::Vec3 &outPoint, rt::Vec3 &outNormal, std::string *why = nullptr)
{
    for (float v : point) if (!std::isfinite(v)) return fail(why, "non-finite donor-pose position");
    for (float v : normal) if (!std::isfinite(v)) return fail(why, "non-finite donor-pose normal");
    rt::Matrix forward, inverse;
    if (!blended(plan, indices, weights, forward, why)) return false;
    if (!rt::inverse(forward, inverse)) return fail(why, "singular blended donor-pose transform");
    const auto &mapping = undo ? inverse : forward;
    const auto &normalInverse = undo ? forward : inverse;
    rt::Vec3 p = rt::point(point, mapping), n{};
    double length = 0;
    for (int c = 0; c < 3; ++c) {
        n[c] = float(double(normal[0])*normalInverse[c*4]
            + double(normal[1])*normalInverse[c*4+1] + double(normal[2])*normalInverse[c*4+2]);
        length += double(n[c])*n[c];
    }
    if (length > 1e-20) for (auto &v : n) v = float(v/std::sqrt(length));
    for (float v : p) if (!std::isfinite(v)) return fail(why, "donor-pose inverse produced invalid position");
    for (float v : n) if (!std::isfinite(v)) return fail(why, "donor-pose inverse produced invalid normal");
    outPoint = p; outNormal = n;
    return true;
}

}} // namespace modmesh::donorpose
