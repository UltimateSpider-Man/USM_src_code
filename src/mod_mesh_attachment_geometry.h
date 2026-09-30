#pragma once

// Render attachment correspondence from pristine PCMESH skin geometry. Bounds
// are signed offsets from each joint in common MODEL axes, not bone-local axes:
// source and target bind rotations need not agree. A vertex contributes only to
// its largest positive skin influence (lowest bone index breaks equal weights),
// so a tiny stray influence cannot inflate an unrelated joint's envelope.
#include "mod_mesh_retarget.h"
#include "mod_pcmesh_source.h"
#include <algorithm>
#include <cstring>
#include <limits>

namespace modmesh { namespace attachment {

using retarget::Matrix;
using retarget::Vec3;

struct Bounds {
    Vec3 negative{{0,0,0}}, positive{{0,0,0}};
    std::size_t samples = 0;
};

struct SkinSample {
    Vec3 position{};
    std::array<std::size_t,4> bone{};
    std::array<float,4> weight{};
};

struct Anchor { std::size_t source = 0, target = 0; };

struct Geometry {
    std::vector<Bounds> sourceBounds, targetBounds;
    std::vector<Matrix> sourceBind, sourceInverse, targetBind, targetInverse;
    std::vector<int> sourceToTarget, targetToSource;
    std::vector<SkinSample> sourceSamples, targetSamples;
};

inline bool finite(const Vec3 &value)
{
    return std::isfinite(value[0]) && std::isfinite(value[1]) && std::isfinite(value[2]);
}

inline bool readSkin(const pcmeshsource::Source &file, const pcmeshsource::Mesh &mesh,
                     const std::vector<Matrix> &bind, std::vector<Bounds> &bounds,
                     std::vector<SkinSample> &samples, std::string *why)
{
    if (mesh.nbones != bind.size() || bind.empty() || mesh.sections.empty())
        return retarget::fail(why, "attachment skin bone/section mismatch");
    bounds.resize(bind.size());
    for (const auto &section : mesh.sections) {
        if (!section.vertexCount) continue;
        if (section.stride != 64 || section.palette.empty())
            return retarget::fail(why, "unsupported attachment skin vertex layout");
        const std::size_t start = section.vertexOffset, count = section.vertexCount;
        if (start > file.bytes.size() || count > (file.bytes.size() - start) / 64
            || count > std::size_t(section.vertexBytes) / 64
            || count > 2000000 || samples.size() > 2000000 - count)
            return retarget::fail(why, "invalid attachment vertex range");
        for (auto bone : section.palette) if (bone >= bind.size())
            return retarget::fail(why, "invalid attachment skin palette");
        for (std::size_t vertex = 0; vertex < count; ++vertex) {
            float row[16];
            std::memcpy(row, file.bytes.data() + start + vertex * 64, sizeof(row));
            SkinSample sample;
            sample.position = {{row[0],row[1],row[2]}};
            if (!finite(sample.position)) return retarget::fail(why, "nonfinite attachment vertex");
            double total = 0;
            std::size_t dominant = 0;
            float greatest = -1;
            for (int lane = 0; lane < 4; ++lane) {
                const float weight = row[12 + lane];
                if (!std::isfinite(weight) || weight < 0 || weight > 1.001f)
                    return retarget::fail(why, "invalid attachment skin weight");
                if (!weight) continue; // Native unused indices can contain sentinels.
                const float index = row[8 + lane];
                if (!std::isfinite(index) || index < 0 || index >= section.palette.size()
                    || std::floor(index) != index)
                    return retarget::fail(why, "invalid attachment vertex palette index");
                const std::size_t bone = section.palette[std::size_t(index)];
                sample.bone[lane] = bone;
                sample.weight[lane] = weight;
                total += weight;
                if (weight > greatest || (weight == greatest && bone < dominant)) {
                    greatest = weight; dominant = bone;
                }
            }
            if (!std::isfinite(total) || std::abs(total - 1.0) > 0.02)
                return retarget::fail(why, "attachment skin weights do not sum to one");
            for (float &weight : sample.weight) weight = float(weight / total);
            auto &box = bounds[dominant];
            for (int axis = 0; axis < 3; ++axis) {
                const float offset = sample.position[axis] - bind[dominant][12 + axis];
                if (!std::isfinite(offset)) return retarget::fail(why, "invalid attachment skin extent");
                box.negative[axis] = std::max(box.negative[axis], -offset);
                box.positive[axis] = std::max(box.positive[axis], offset);
            }
            ++box.samples;
            samples.push_back(sample);
        }
    }
    if (samples.empty()) return retarget::fail(why, "empty attachment skin geometry");
    return true;
}

inline bool prepare(const pcmeshsource::Source &sourceFile, const pcmeshsource::Mesh &sourceMesh,
                    const pcmeshsource::Source &targetFile, const pcmeshsource::Mesh &targetMesh,
                    const retarget::Plan &plan, Geometry &out, std::string *why = nullptr)
{
    if (plan.sourceBind.empty() || plan.targetBind.empty()
        || plan.sourceBind.size() != plan.sourceInverse.size()
        || plan.targetBind.size() != plan.targetInverse.size()
        || plan.sourceBind.size() != plan.targetBone.size())
        return retarget::fail(why, "missing attachment retarget plan");
    Geometry result;
    result.sourceBind = plan.sourceBind; result.sourceInverse = plan.sourceInverse;
    result.targetBind = plan.targetBind; result.targetInverse = plan.targetInverse;
    result.sourceToTarget = plan.targetBone;
    for (const auto *matrices : {&result.sourceBind, &result.sourceInverse,
                                &result.targetBind, &result.targetInverse})
        for (const auto &matrix : *matrices) {
            Matrix inverse;
            if (!retarget::inverse(matrix, inverse))
                return retarget::fail(why, "invalid attachment bind matrix");
        }
    result.targetToSource.assign(plan.targetBind.size(), -1);
    for (std::size_t source = 0; source < plan.targetBone.size(); ++source) {
        const int target = plan.targetBone[source];
        if (target < -1 || target >= int(result.targetToSource.size()))
            return retarget::fail(why, "invalid attachment bone correspondence");
        if (target >= 0) {
            auto &mapped = result.targetToSource[std::size_t(target)];
            mapped = mapped == -1 ? int(source) : -2; // Ambiguous targets cannot choose an anchor.
        }
    }
    if (!readSkin(sourceFile, sourceMesh, result.sourceBind, result.sourceBounds, result.sourceSamples, why)
        || !readSkin(targetFile, targetMesh, result.targetBind, result.targetBounds,
                     result.targetSamples, why)) return false;
    bool usable = false;
    for (std::size_t target = 0; target < result.targetToSource.size(); ++target) {
        const int source = result.targetToSource[target];
        if (source >= 0 && result.targetBounds[target].samples
            && result.sourceBounds[std::size_t(source)].samples) usable = true;
    }
    if (!usable) return retarget::fail(why, "no mapped attachment skin surfaces");
    out = std::move(result);
    if (why) why->clear();
    return true;
}

// Selection uses the native animated skin surface, including blended vertices,
// rather than the closest pivot. Re-evaluate when the native animation moves
// a root between body regions; attachment helpers need not have a fixed socket.
inline bool chooseAnchor(const Geometry &geometry, const Matrix *targetCurrent,
                         std::size_t targetCount, const Vec3 &worldPoint,
                         Anchor &out, std::string *why = nullptr)
{
    if (!finite(worldPoint) || !targetCurrent || targetCount < geometry.targetBind.size()
        || geometry.targetBind.empty() || geometry.targetToSource.size() != geometry.targetBind.size())
        return retarget::fail(why, "missing attachment anchor pose");
    std::vector<Matrix> skin(geometry.targetBind.size());
    for (std::size_t bone = 0; bone < skin.size(); ++bone) {
        if (!retarget::affine(targetCurrent[bone]))
            return retarget::fail(why, "invalid attachment anchor pose");
        skin[bone] = retarget::multiply(geometry.targetInverse[bone], targetCurrent[bone]);
    }
    double bestDistance = std::numeric_limits<double>::infinity();
    Anchor selected;
    bool found = false;
    for (const auto &sample : geometry.targetSamples) {
        Vec3 moved{};
        int bestTarget = -1;
        float greatest = -1;
        for (int lane = 0; lane < 4; ++lane) {
            if (!sample.weight[lane]) continue;
            const std::size_t target = sample.bone[lane];
            if (target >= skin.size()) return retarget::fail(why, "invalid attachment sample bone");
            const auto point = retarget::point(sample.position, skin[target]);
            for (int axis = 0; axis < 3; ++axis) moved[axis] += sample.weight[lane] * point[axis];
            const int source = geometry.targetToSource[target];
            if (source < 0 || !geometry.targetBounds[target].samples
                || !geometry.sourceBounds[std::size_t(source)].samples) continue;
            if (sample.weight[lane] > greatest
                || (sample.weight[lane] == greatest && int(target) < bestTarget)) {
                greatest = sample.weight[lane]; bestTarget = int(target);
            }
        }
        if (!finite(moved)) return retarget::fail(why, "nonfinite deformed attachment sample");
        if (bestTarget < 0) continue;
        double distance = 0;
        for (int axis = 0; axis < 3; ++axis) {
            const double delta = double(moved[axis]) - worldPoint[axis];
            distance += delta * delta;
        }
        if (distance < bestDistance) {
            bestDistance = distance;
            selected = {std::size_t(geometry.targetToSource[std::size_t(bestTarget)]),
                        std::size_t(bestTarget)};
            found = true;
        }
    }
    if (!found) return retarget::fail(why, "no corresponding attachment skin sample");
    out = selected;
    if (why) why->clear();
    return true;
}

// Corresponding signed extents preserve where a point lies relative to a
// body's surface. Complete bind-frame conversion avoids copying local axes
// between differently oriented source/target joints. This is an envelope
// correspondence, not an authored mouth socket or exact triangle projection.
inline bool transfer(const Geometry &geometry, const Vec3 &worldPoint,
                     const Matrix &targetCurrent, const Matrix &sourceCurrent,
                     std::size_t source, std::size_t target, Vec3 &out,
                     std::string *why = nullptr)
{
    if (!finite(worldPoint) || source >= geometry.sourceBind.size()
        || target >= geometry.targetBind.size() || source >= geometry.sourceToTarget.size()
        || geometry.sourceToTarget[source] != int(target)
        || !geometry.sourceBounds[source].samples || !geometry.targetBounds[target].samples)
        return retarget::fail(why, "invalid attachment transfer mapping");
    Matrix targetNowInverse;
    if (!retarget::inverse(targetCurrent, targetNowInverse) || !retarget::affine(sourceCurrent))
        return retarget::fail(why, "invalid attachment transfer pose");
    const auto bindPoint = retarget::point(retarget::point(worldPoint, targetNowInverse),
                                          geometry.targetBind[target]);
    Vec3 sourcePoint = retarget::position(geometry.sourceBind[source]);
    bool identical = true;
    for (int axis = 0; axis < 3; ++axis)
        identical = identical
            && geometry.sourceBounds[source].negative[axis] == geometry.targetBounds[target].negative[axis]
            && geometry.sourceBounds[source].positive[axis] == geometry.targetBounds[target].positive[axis];
    identical = identical && geometry.sourceBind[source] == geometry.targetBind[target];
    for (int axis = 0; axis < 3; ++axis) {
        const float offset = bindPoint[axis] - geometry.targetBind[target][12 + axis];
        const float from = offset < 0 ? geometry.targetBounds[target].negative[axis]
                                      : geometry.targetBounds[target].positive[axis];
        const float to = offset < 0 ? geometry.sourceBounds[source].negative[axis]
                                    : geometry.sourceBounds[source].positive[axis];
        // Missing support on an axis cannot establish a size ratio. Preserve
        // that model-axis offset; equal/flat geometry still maps identically.
        const double ratio = from > 1e-6f ? double(to) / from : 1.0;
        float mappedOffset = float(double(offset) * ratio);
        // Differently shaped bodies keep the visual anchor within the source
        // skin envelope. Identical geometry preserves every native offset,
        // including authored points outside that envelope.
        if (!identical) mappedOffset = std::max(-geometry.sourceBounds[source].negative[axis],
            std::min(geometry.sourceBounds[source].positive[axis], mappedOffset));
        sourcePoint[axis] += mappedOffset;
    }
    const auto moved = retarget::point(retarget::point(sourcePoint, geometry.sourceInverse[source]),
                                      sourceCurrent);
    if (!finite(moved)) return retarget::fail(why, "nonfinite attachment transfer result");
    out = moved;
    if (why) why->clear();
    return true;
}

}} // namespace modmesh::attachment
