#include "../src/mod_mesh_axes.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <stdexcept>

namespace {

using Vector = std::array<double, 3>;
using Matrix = std::array<double, 16>;
using modmesh::axes::FbxAxes;
using modmesh::axes::Transform;

void require(bool condition, const char *message)
{
    if (!condition) throw std::runtime_error(message);
}

void equal(const Vector &actual, const Vector &expected, const char *message)
{
    for (int i = 0; i < 3; ++i)
        require(std::abs(actual[i] - expected[i]) < 1e-12, message);
}

Vector cross(const Vector &a, const Vector &b)
{
    return {{a[1]*b[2] - a[2]*b[1], a[2]*b[0] - a[0]*b[2], a[0]*b[1] - a[1]*b[0]}};
}

double dot(const Vector &a, const Vector &b)
{
    return a[0]*b[0] + a[1]*b[1] + a[2]*b[2];
}

Vector columnPoint(const Matrix &m, const Vector &p)
{
    return {{m[0]*p[0] + m[1]*p[1] + m[2]*p[2] + m[3],
             m[4]*p[0] + m[5]*p[1] + m[6]*p[2] + m[7],
             m[8]*p[0] + m[9]*p[1] + m[10]*p[2] + m[11]}};
}

Vector rowPoint(const Matrix &m, const Vector &p)
{
    return {{p[0]*m[0] + p[1]*m[4] + p[2]*m[8] + m[12],
             p[0]*m[1] + p[1]*m[5] + p[2]*m[9] + m[13],
             p[0]*m[2] + p[1]*m[6] + p[2]*m[10] + m[14]}};
}

Transform make(const FbxAxes &source)
{
    Transform result;
    require(modmesh::axes::fromFbx(source, result), "valid FBX frame rejected");
    return result;
}

void knownFrames()
{
    // Metadata read directly from the native Spider-Man/black-suit and Miles
    // assets. They must remain bit-identical, including global bind matrices.
    const Transform native = make(FbxAxes{});
    require(native.identity() && !native.reflected(), "native FBX changed frame");
    const Vector p{{7.0, 11.0, 13.0}};
    equal(native.vector(p), p, "native coordinates changed");
    const Matrix bind{{0, 2, 0, 0, -3, 0, 0, 0, 0, 0, 4, 0, 7, 11, 13, 1}};
    require(native.convertFbxWorldMatrix(bind) == bind, "native bind changed");

    // Autodesk MayaZUp/Max: +X right, +Z up, -Y front.
    const auto zup = make(FbxAxes{2, 1, 1, -1, 0, 1});
    equal(zup.vector(p), {{7.0, 13.0, -11.0}}, "Z-up frame failed");
    require(!zup.reflected(), "Z-up rotation reported as reflection");

    // A cyclic right-handed X-up system: +Y right, +X up, -Z front.
    const auto xup = make(FbxAxes{0, 1, 2, -1, 1, 1});
    equal(xup.vector(p), {{11.0, 7.0, -13.0}}, "X-up frame failed");
    require(!xup.reflected(), "X-up rotation reported as reflection");

    // Autodesk DirectX/Lightwave: -X right, +Y up, +Z front.
    const auto left = make(FbxAxes{1, 1, 2, 1, 0, -1});
    equal(left.vector(p), {{-7.0, 11.0, 13.0}}, "left-handed frame failed");
    require(left.reflected(), "left-handed frame lost reflection");
}

void allFrames()
{
    std::array<int, 3> axes{{0, 1, 2}};
    unsigned count = 0;
    unsigned reflected = 0;
    const Vector local{{2, -3, 5}};
    const Vector tangentA{{2, 1, 0}}, tangentB{{-1, 2, 3}};
    const Vector normal = cross(tangentA, tangentB);
    // Nonuniform scale, rotated basis, and a nonzero translated pivot expose
    // mistaken matrix transposes, lost bind bases, and local-frame changes.
    const Matrix bind{{0, 2, 0, 0, -3, 0, 0, 0, 0, 0, 4, 0, 7, 11, 13, 1}};
    do {
        for (int bits = 0; bits < 8; ++bits) {
            const int sx = (bits & 1) ? -1 : 1;
            const int sy = (bits & 2) ? -1 : 1;
            const int sz = (bits & 4) ? -1 : 1;
            const auto frame = make(FbxAxes{axes[1], sy, axes[2], sz, axes[0], sx});
            ++count;
            reflected += frame.reflected() ? 1 : 0;
            Vector right{}, up{}, front{};
            right[axes[0]] = sx; up[axes[1]] = sy; front[axes[2]] = sz;
            equal(frame.vector(right), {{1, 0, 0}}, "right basis changed");
            equal(frame.vector(up), {{0, 1, 0}}, "up basis changed");
            equal(frame.vector(front), {{0, 0, 1}}, "front basis changed");
            equal(columnPoint(frame.rowMajorMatrix(), local), frame.vector(local),
                  "M4 representation disagrees with axis conversion");
            const auto convertedBind = frame.convertFbxWorldMatrix(bind);
            equal(rowPoint(convertedBind, local), frame.vector(rowPoint(bind, local)),
                  "geometry and bone bind are in different world frames");
            equal({{convertedBind[12], convertedBind[13], convertedBind[14]}},
                  frame.vector(Vector{{7, 11, 13}}), "bind translation failed");
            const Vector a = frame.vector(tangentA), b = frame.vector(tangentB);
            const Vector n = frame.vector(normal);
            require(std::abs(dot(a, n)) < 1e-12 && std::abs(dot(b, n)) < 1e-12,
                    "converted normal is not perpendicular to its surface");
            // A reflection changes the cross-product sign, not the outward
            // normal sign. Swapping triangle corners restores front winding.
            require((dot(cross(a, b), n) < 0) == frame.reflected(),
                    "reflection parity disagrees with triangle orientation");
            require(dot(frame.reflected() ? cross(b, a) : cross(a, b), n) > 0,
                    "reflected winding does not preserve outward normals");
        }
    } while (std::next_permutation(axes.begin(), axes.end()));
    require(count == 48 && reflected == 24, "did not verify all signed permutations");
}

void invalidFrames()
{
    for (const auto &bad : {FbxAxes{-1, 1, 2, 1, 0, 1}, FbxAxes{3, 1, 2, 1, 0, 1},
                           FbxAxes{0, 1, 2, 1, 0, 1}, FbxAxes{1, 0, 2, 1, 0, 1},
                           FbxAxes{1, 1, 2, 2, 0, 1}}) {
        auto output = make(FbxAxes{2, 1, 1, -1, 0, 1});
        require(!modmesh::axes::fromFbx(bad, output), "invalid axis metadata accepted");
        require(output.identity(), "invalid metadata left a stale axis transform");
    }
}

} // namespace

int main()
{
    try {
        knownFrames();
        allFrames();
        invalidFrames();
        std::puts("PASS: native identity, Y/Z/X-up, 48 signed frames, binds, normals, winding, invalid metadata");
        return 0;
    } catch (const std::exception &error) {
        std::fprintf(stderr, "FAIL: %s\n", error.what());
        return 1;
    }
}
