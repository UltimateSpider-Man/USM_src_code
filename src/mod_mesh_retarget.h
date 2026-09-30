#pragma once

// Render-only retargeting. Matrices use the game's row-vector convention:
// point * local * parent, with translation at elements 12..14.
// No animation controller, gameplay skeleton or target pose is modified.
#include <array>
#include <cmath>
#include <cstddef>
#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace modmesh { namespace retarget {

using Matrix = std::array<float, 16>;
using Vec3 = std::array<float, 3>;

inline Matrix identity()
{
    return {{1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1}};
}

inline bool affine(const Matrix &m)
{
    for (float v : m) if (!std::isfinite(v)) return false;
    return std::abs(m[3]) < 1e-4f && std::abs(m[7]) < 1e-4f
        && std::abs(m[11]) < 1e-4f && std::abs(m[15] - 1.f) < 1e-4f;
}

inline Matrix multiply(const Matrix &a, const Matrix &b)
{
    Matrix result{};
    for (int r = 0; r < 4; ++r)
        for (int c = 0; c < 4; ++c) {
            double value = 0;
            for (int k = 0; k < 4; ++k) value += double(a[r*4+k]) * b[k*4+c];
            result[r*4+c] = float(value);
        }
    return result;
}

inline Vec3 vector(const Vec3 &v, const Matrix &m)
{
    Vec3 result{};
    for (int c = 0; c < 3; ++c)
        result[c] = float(double(v[0])*m[c] + double(v[1])*m[4+c]
                        + double(v[2])*m[8+c]);
    return result;
}

inline Vec3 position(const Matrix &m) { return {{m[12], m[13], m[14]}}; }

inline Vec3 point(const Vec3 &v, const Matrix &m)
{
    Vec3 result = vector(v, m);
    for (int c = 0; c < 3; ++c) result[c] += m[12+c];
    return result;
}

inline bool inverse(const Matrix &m, Matrix &result)
{
    if (!affine(m)) return false;
    const double a=m[0], b=m[1], c=m[2], d=m[4], e=m[5], f=m[6],
                 g=m[8], h=m[9], i=m[10];
    const double det = a*(e*i-f*h) - b*(d*i-f*g) + c*(d*h-e*g);
    if (!std::isfinite(det) || std::abs(det) < 1e-12) return false;
    result = identity();
    result[0]=float((e*i-f*h)/det); result[1]=float((c*h-b*i)/det); result[2]=float((b*f-c*e)/det);
    result[4]=float((f*g-d*i)/det); result[5]=float((a*i-c*g)/det); result[6]=float((c*d-a*f)/det);
    result[8]=float((d*h-e*g)/det); result[9]=float((b*g-a*h)/det); result[10]=float((a*e-b*d)/det);
    Vec3 translated = vector(position(m), result);
    for (int c0 = 0; c0 < 3; ++c0) result[12+c0] = -translated[c0];
    return affine(result);
}

struct Plan {
    std::vector<Matrix> sourceBind, sourceInverse, targetBind, targetInverse;
    std::vector<int> parent, targetBone, driverParent;
    std::vector<std::size_t> order;
    int rootDriver = -1;
    // Explicit posed-scan rigs whose bind axes are aligned to native bone axes.
    // Ordinary FBX/native retarget plans retain their original delta behavior.
    bool absoluteBoneAxes = false;
};

inline bool fail(std::string *why, const char *message)
{
    if (why) *why = message;
    return false;
}

// Parent and targetBone are indexed by SOURCE mesh bone. targetBone indexes
// the TARGET mesh's skin-pose array, not the larger PCSKEL component array.
// A missing source helper (-1) keeps its source rest transform under its parent.
inline bool prepare(const std::vector<Matrix> &sourceBind,
                    const std::vector<int> &parent,
                    const std::vector<int> &targetBone,
                    const std::vector<Matrix> &targetBind,
                    Plan &out, std::string *why = nullptr)
{
    if (sourceBind.empty() || sourceBind.size() > 1024 || targetBind.empty()
        || targetBind.size() > 1024 || parent.size() != sourceBind.size()
        || targetBone.size() != sourceBind.size())
        return fail(why, "invalid retarget bone counts");
    Plan plan;
    plan.sourceBind=sourceBind; plan.parent=parent; plan.targetBone=targetBone;
    plan.targetBind=targetBind;
    plan.sourceInverse.resize(sourceBind.size());
    plan.targetInverse.resize(targetBind.size());
    plan.driverParent.assign(sourceBind.size(), -1);
    for (std::size_t j=0; j<sourceBind.size(); ++j) {
        if (!inverse(sourceBind[j], plan.sourceInverse[j]))
            return fail(why, "invalid source bind matrix");
        if (parent[j] < -1 || parent[j] >= int(sourceBind.size()) || parent[j] == int(j)
            || targetBone[j] < -1 || targetBone[j] >= int(targetBind.size()))
            return fail(why, "invalid source parent or target bone index");
    }
    for (std::size_t j=0; j<targetBind.size(); ++j)
        if (!inverse(targetBind[j], plan.targetInverse[j]))
            return fail(why, "invalid target bind matrix");
    std::vector<unsigned char> state(sourceBind.size(), 0);
    std::function<bool(std::size_t)> visit = [&](std::size_t j) {
        if (state[j] == 1) return false;
        if (state[j] == 2) return true;
        state[j] = 1;
        if (parent[j] >= 0 && !visit(std::size_t(parent[j]))) return false;
        state[j] = 2;
        plan.order.push_back(j);
        return true;
    };
    for (std::size_t j=0; j<sourceBind.size(); ++j)
        if (!visit(j)) return fail(why, "source parent cycle");
    for (std::size_t j : plan.order) {
        int ancestor = parent[j];
        while (ancestor >= 0 && targetBone[std::size_t(ancestor)] < 0)
            ancestor = parent[std::size_t(ancestor)];
        plan.driverParent[j] = ancestor;
        if (plan.rootDriver < 0 && targetBone[j] >= 0) plan.rootDriver = targetBone[j];
    }
    if (plan.rootDriver < 0) return fail(why, "no mapped source bones");
    out = std::move(plan);
    if (why) why->clear();
    return true;
}

// Read target world poses and emit independent source world poses. Keeping the
// target's changes in local translation preserves authored IK/stretch motion;
// replacing only the rest offsets prevents Venom's longer limbs stretching a
// human source mesh. Identical rigs reproduce the incoming animated pose.
inline bool evaluate(const Plan &plan, const Matrix *targetCurrent,
                     std::size_t targetCount, std::vector<Matrix> &out,
                     std::string *why = nullptr)
{
    if (!targetCurrent || targetCount < plan.targetBind.size()
        || plan.rootDriver < 0 || plan.order.size() != plan.sourceBind.size())
        return fail(why, "missing target pose or retarget plan");
    if (plan.absoluteBoneAxes) {
        // Rotation-only retarget of a reconstructed posed scan. Every local
        // offset is measured in its reconstructed parent bind basis. Native
        // world axes animate that chain without importing a larger donor's
        // limb lengths, and without double-applying the scan's lowered arms.
        std::vector<Matrix> result(plan.sourceBind.size());
        for (std::size_t j : plan.order) {
            if (j >= plan.sourceBind.size() || j >= plan.targetBone.size()
                || j >= plan.parent.size()) return fail(why, "invalid posed-scan plan index");
            const int target = plan.targetBone[j], parent = plan.parent[j];
            if (target < 0) {
                if (parent < 0 || std::size_t(parent) >= result.size()
                    || std::size_t(parent) >= plan.sourceInverse.size())
                    return fail(why, "unmapped source helper has no driven parent");
                result[j] = multiply(multiply(plan.sourceBind[j], plan.sourceInverse[std::size_t(parent)]),
                                     result[std::size_t(parent)]);
                if (!affine(result[j])) return fail(why, "nonfinite source helper pose");
                continue;
            }
            if (target < 0 || std::size_t(target) >= targetCount
                || !affine(targetCurrent[std::size_t(target)]))
                return fail(why, "missing posed-scan native bone axes");
            Matrix check;
            if (!inverse(targetCurrent[std::size_t(target)], check))
                return fail(why, "singular posed-scan animated bone");
            result[j] = targetCurrent[std::size_t(target)];
            if (parent >= 0) {
                const std::size_t p = std::size_t(parent);
                if (p >= result.size() || p >= plan.sourceInverse.size())
                    return fail(why, "invalid posed-scan parent");
                const Vec3 local = point(position(plan.sourceBind[j]), plan.sourceInverse[p]);
                const Vec3 translated = point(local, result[p]);
                for (int c = 0; c < 3; ++c) result[j][12+c] = translated[c];
            }
            if (!affine(result[j])) return fail(why, "nonfinite posed-scan pose");
        }
        out = std::move(result);
        if (why) why->clear();
        return true;
    }
    std::vector<Matrix> targetCurrentInverse(plan.targetBind.size());
    for (std::size_t j=0; j<plan.targetBind.size(); ++j)
        if (!inverse(targetCurrent[j], targetCurrentInverse[j]))
            return fail(why, "invalid animated target matrix");
    const Matrix rootMotion = multiply(plan.targetInverse[std::size_t(plan.rootDriver)],
                                       targetCurrent[std::size_t(plan.rootDriver)]);
    std::vector<Matrix> result(plan.sourceBind.size());
    for (std::size_t j : plan.order) {
        const int target = plan.targetBone[j];
        if (target < 0) {
            const int parent = plan.parent[j];
            result[j] = parent < 0 ? multiply(plan.sourceBind[j], rootMotion)
                : multiply(multiply(plan.sourceBind[j], plan.sourceInverse[std::size_t(parent)]),
                           result[std::size_t(parent)]);
        } else {
            const std::size_t t = std::size_t(target);
            result[j] = multiply(multiply(plan.sourceBind[j], plan.targetInverse[t]), targetCurrent[t]);
            const int ancestor = plan.driverParent[j];
            if (ancestor >= 0) {
                const std::size_t p = std::size_t(ancestor);
                const std::size_t tp = std::size_t(plan.targetBone[p]);
                Vec3 localSource = point(position(plan.sourceBind[j]), plan.sourceInverse[p]);
                const Vec3 localTargetBind = point(position(plan.targetBind[t]), plan.targetInverse[tp]);
                const Vec3 localTargetNow = point(position(targetCurrent[t]), targetCurrentInverse[tp]);
                Vec3 delta{};
                for (int c=0; c<3; ++c) delta[c] = localTargetNow[c] - localTargetBind[c];
                // Express the native animation's translation delta in the
                // source parent's bind basis before applying its new pose.
                delta = vector(vector(delta, plan.targetBind[tp]), plan.sourceInverse[p]);
                for (int c=0; c<3; ++c) localSource[c] += delta[c];
                const Vec3 translated = point(localSource, result[p]);
                for (int c=0; c<3; ++c) result[j][12+c] = translated[c];
            }
        }
        if (!affine(result[j])) return fail(why, "nonfinite retargeted pose");
    }
    out = std::move(result);
    if (why) why->clear();
    return true;
}

// Match the lowest selected source foot/toe pivot to its native counterpart
// along the actor's up axis. This preserves native jumps/root movement without
// stretching source limbs or changing any rotation. It aligns support height,
// not both feet's full XYZ contacts (which require an IK/contact solution).
// Pair indices are {source bone, target bone}; target storage is read-only.
inline bool alignSupportHeight(std::vector<Matrix> &sourceWorld,
                               const Matrix *targetWorld, std::size_t targetCount,
                               const std::vector<std::pair<std::size_t, std::size_t>> &pairs,
                               const Vec3 &up, float *appliedShift = nullptr,
                               std::string *why = nullptr)
{
    if (sourceWorld.empty() || !targetWorld || pairs.empty())
        return fail(why, "missing support pose or foot mapping");
    double lengthSquared = 0;
    for (float value : up) {
        if (!std::isfinite(value)) return fail(why, "invalid support up axis");
        lengthSquared += double(value) * value;
    }
    if (lengthSquared < 1e-16 || !std::isfinite(lengthSquared))
        return fail(why, "invalid support up axis");
    const double length = std::sqrt(lengthSquared);
    double unit[3] = {up[0] / length, up[1] / length, up[2] / length};
    double sourceHeight = 0, targetHeight = 0;
    bool first = true;
    for (const auto &pair : pairs) {
        if (pair.first >= sourceWorld.size() || pair.second >= targetCount
            || !affine(sourceWorld[pair.first]) || !affine(targetWorld[pair.second]))
            return fail(why, "invalid support bone mapping or matrix");
        double a = 0, b = 0;
        for (int axis = 0; axis < 3; ++axis) {
            a += double(sourceWorld[pair.first][12 + axis]) * unit[axis];
            b += double(targetWorld[pair.second][12 + axis]) * unit[axis];
        }
        if (first || a < sourceHeight) sourceHeight = a;
        if (first || b < targetHeight) targetHeight = b;
        first = false;
    }
    const double shift = targetHeight - sourceHeight;
    if (!std::isfinite(float(shift))) return fail(why, "invalid support height shift");
    // Validate the complete result before publishing any translated joint.
    for (const auto &bone : sourceWorld) {
        if (!affine(bone)) return fail(why, "invalid source support pose");
        for (int axis = 0; axis < 3; ++axis)
            if (!std::isfinite(float(double(bone[12 + axis]) + unit[axis] * shift)))
                return fail(why, "support height translation overflow");
    }
    for (auto &bone : sourceWorld)
        for (int axis = 0; axis < 3; ++axis)
            bone[12 + axis] = float(double(bone[12 + axis]) + unit[axis] * shift);
    if (appliedShift) *appliedShift = float(shift);
    if (why) why->clear();
    return true;
}

}} // namespace modmesh::retarget
