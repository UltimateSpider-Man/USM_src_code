#pragma once

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <type_traits>

namespace modmesh { namespace attachment {

// PC retail interface layout. The word at +0x28 is zip state, not a count.
struct NativeTentacleInterface {
    std::uint32_t prefix[7];
    std::uint32_t definitions;
    std::uint16_t definitionCount;
    std::uint8_t shared, fromMash;
    std::uint32_t tubes;
    std::uint32_t zipState;
};
static_assert(offsetof(NativeTentacleInterface, definitionCount) == 0x20, "PC tentacle count");
static_assert(offsetof(NativeTentacleInterface, tubes) == 0x24, "PC tentacle tube array");

inline int matchingTubeIndex(const NativeTentacleInterface &ifc,
                             const std::uint32_t *tubes, std::uint32_t tube)
{
    if (!tube || !tubes || !ifc.definitions || !ifc.tubes ||
        !ifc.definitionCount || ifc.definitionCount > 128) return -1;
    for (unsigned i = 0; i < ifc.definitionCount; ++i)
        if (tubes[i] == tube) return int(i);
    return -1;
}

template<class Point>
bool finitePoint(const Point &p)
{
    return std::isfinite(p.x) && std::isfinite(p.y) && std::isfinite(p.z);
}

// Inactive native tubes can retain coincident controls and a noisy cached
// curve. Their accumulated arc length is nonzero, but tapering a correction
// across them would create a strand that the native animation had collapsed.
// A 0.1 mm bounding-box diagonal rejects captured interpolation noise (below
// 0.07 mm) while retaining extended loops whose first and last points coincide.
template<class Point>
bool hasExtendedSpan(const Point *points, std::size_t count)
{
    if (!points || count < 2 || count > 65536 || !finitePoint(points[0])) return false;
    double minimum[3] = {points[0].x, points[0].y, points[0].z};
    double maximum[3] = {points[0].x, points[0].y, points[0].z};
    for (std::size_t i = 1; i < count; ++i) {
        if (!finitePoint(points[i])) return false;
        const double coordinates[3] = {points[i].x, points[i].y, points[i].z};
        for (int axis = 0; axis < 3; ++axis) {
            if (coordinates[axis] < minimum[axis]) minimum[axis] = coordinates[axis];
            if (coordinates[axis] > maximum[axis]) maximum[axis] = coordinates[axis];
        }
    }
    double spanSquared = 0;
    for (int axis = 0; axis < 3; ++axis) {
        const double span = maximum[axis] - minimum[axis];
        spanSquared += span * span;
    }
    constexpr double minimumSpan = 1.0e-4;
    return std::isfinite(spanSquared) && spanSquared > minimumSpan * minimumSpan;
}

// Native AI orders these points from the body attachment to the attack tip.
// Only render storage is passed here; the final point is never written.
template<class Point>
bool shiftRenderPoints(Point *points, std::size_t count, const Point &delta)
{
    if (!finitePoint(delta) || !hasExtendedSpan(points, count)) return false;
    auto distance = [](const Point &a, const Point &b) {
        const double x = double(a.x) - b.x, y = double(a.y) - b.y, z = double(a.z) - b.z;
        return std::sqrt(x*x + y*y + z*z);
    };
    double total = 0.0;
    for (std::size_t i = 0; i < count; ++i) {
        if (i) total += distance(points[i], points[i-1]);
    }
    if (!std::isfinite(total) || total <= 0) return false;
    // Validate every result before any write so a rejected curve is unchanged.
    double length = 0.0;
    for (std::size_t i = 0; i + 1 < count; ++i) {
        if (i) length += distance(points[i], points[i-1]);
        const double weight = 1.0 - length / total;
        Point result = points[i];
        result.x = float(double(result.x) + delta.x * weight);
        result.y = float(double(result.y) + delta.y * weight);
        result.z = float(double(result.z) + delta.z * weight);
        if (!finitePoint(result)) return false;
    }
    length = 0.0;
    Point previous = points[0];
    for (std::size_t i = 0; i + 1 < count; ++i) {
        const Point original = points[i];
        if (i) length += distance(original, previous);
        previous = original;
        const double weight = 1.0 - length / total;
        points[i].x = float(double(original.x) + delta.x * weight);
        points[i].y = float(double(original.y) + delta.y * weight);
        points[i].z = float(double(original.z) + delta.z * weight);
    }
    return true;
}

template<class Vector>
bool vectorCount(const Vector &v, std::size_t maximum, std::size_t &count)
{
    using Value = typename std::remove_pointer<decltype(v.m_first)>::type;
    const auto first = reinterpret_cast<std::uintptr_t>(v.m_first);
    const auto last = reinterpret_cast<std::uintptr_t>(v.m_last);
    const auto end = reinterpret_cast<std::uintptr_t>(v.m_end);
    if (!first) { count = 0; return !last && !end; }
    if (last < first || end < last || (last-first) % sizeof(Value) ||
        (end-first) % sizeof(Value) || (end-first) / sizeof(Value) > maximum) return false;
    count = (last-first) / sizeof(Value);
    return true;
}

// The retail spline contains three MSVC vector headers followed by its cache
// fields. Keep its complete object representation and detach all owned arrays
// before invoking retail code. Allocator must be the retail heap allocator:
// rebuild_helper frees/reallocates curve_pts even when it already has capacity.
template<class Spline, template<class> class Allocator>
class ScopedSplineShadow {
    Spline &value;
    unsigned char saved[sizeof(Spline)];
    bool installed = false;

    template<class T> struct Buffer {
        T *data = nullptr;
        std::size_t count = 0;
        ~Buffer() { if (data) Allocator<T>{}.deallocate(data, count); }
        void copy(const T *source, std::size_t n) {
            count = n;
            if (!n) return;
            data = Allocator<T>{}.allocate(n);
            std::uninitialized_copy_n(source, n, data);
        }
        template<class Vector> void install(Vector &v) {
            v.m_first = data;
            v.m_last = v.m_end = data ? data + count : nullptr;
            data = nullptr;
        }
    };
    template<class Vector> static void release(Vector &v) {
        using T = typename std::remove_pointer<decltype(v.m_first)>::type;
        if (v.m_first) Allocator<T>{}.deallocate(v.m_first, 0);
    }

public:
    explicit ScopedSplineShadow(Spline &source) : value(source) {
        std::size_t controls = 0, percentages = 0, curve = 0;
        if (!vectorCount(source.control_pts, 4096, controls) || controls < 2 ||
            !vectorCount(source.control_pts_pct, 65536, percentages) ||
            !vectorCount(source.curve_pts, 65536, curve)) return;
        using Point = typename std::remove_pointer<decltype(source.control_pts.m_first)>::type;
        using Percent = typename std::remove_pointer<decltype(source.control_pts_pct.m_first)>::type;
        Buffer<Point> controlCopy, curveCopy;
        Buffer<Percent> percentCopy;
        controlCopy.copy(source.control_pts.m_first, controls);
        percentCopy.copy(source.control_pts_pct.m_first, percentages);
        curveCopy.copy(source.curve_pts.m_first, curve);
        std::memcpy(saved, &source, sizeof(source));
        controlCopy.install(source.control_pts);
        percentCopy.install(source.control_pts_pct);
        curveCopy.install(source.curve_pts);
        installed = true;
    }
    ~ScopedSplineShadow() {
        if (!installed) return;
        release(value.control_pts);
        release(value.control_pts_pct);
        release(value.curve_pts);
        std::memcpy(static_cast<void *>(&value), saved, sizeof(value));
    }
    ScopedSplineShadow(const ScopedSplineShadow &) = delete;
    ScopedSplineShadow &operator=(const ScopedSplineShadow &) = delete;
    bool valid() const { return installed; }
};

}} // namespace modmesh::attachment
