#include "../src/mod_polytube_attachment.h"

#include <array>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <set>
#include <stdexcept>

namespace {
struct Point { float x, y, z; };
template<class T> struct NativeVector { unsigned opaque; T *m_first, *m_last, *m_end; };
struct NativeSpline {
    NativeVector<Point> control_pts;
    NativeVector<float> control_pts_pct;
    NativeVector<Point> curve_pts;
    unsigned flags;
    double cache;
};
std::set<void *> live;
int allocations = 0;
int failAfter = -1;
template<class T> struct CheckedAllocator {
    T *allocate(std::size_t n) {
        if (allocations++ == failAfter) throw std::bad_alloc();
        T *p = static_cast<T *>(::operator new(n * sizeof(T)));
        if (!live.insert(p).second) std::abort();
        return p;
    }
    void deallocate(T *p, std::size_t) {
        if (live.erase(p) != 1) std::abort(); // Never free a borrowed original.
        ::operator delete(p);
    }
};
void require(bool condition, const char *message) {
    if (!condition) throw std::runtime_error(message);
}
bool near(float actual, float expected) { return std::fabs(actual - expected) < 1.0e-6f; }

void endpointChecks() {
    Point original[] = {{0,0,0}, {1,0,0}, {4,0,0}};
    Point points[3];
    std::memcpy(points, original, sizeof(points));
    require(modmesh::attachment::shiftRenderPoints(points, 3, Point{0,4,0}), "shift rejected");
    require(near(points[0].y, 4) && near(points[1].y, 3), "arc-length attachment taper wrong");
    require(std::memcmp(&points[2], &original[2], sizeof(Point)) == 0, "attack tip changed");

    Point collapsed[] = {{2,2,2}, {2,2,2}, {2,2,2}};
    Point collapsedBefore[3];
    std::memcpy(collapsedBefore, collapsed, sizeof(collapsed));
    require(!modmesh::attachment::shiftRenderPoints(collapsed, 3, Point{0,4,0}), "collapsed curve became a strand");
    require(std::memcmp(collapsed,collapsedBefore,sizeof(collapsed)) == 0, "collapsed rejection changed points");
    // Captured inactive native tubes contain submillimetre interpolation noise.
    // Repeated small zigzags can have a large total arc length while all points
    // still occupy the same tiny region; arc length alone cannot detect this.
    std::array<Point,25> noisy;
    for (std::size_t i = 0; i < noisy.size(); ++i)
        noisy[i] = {2.f + (i % 2 ? 0.00002f : -0.00002f), 2, 2};
    const auto noisyBefore = noisy;
    require(!modmesh::attachment::shiftRenderPoints(noisy.data(),noisy.size(),Point{0,4,0}), "collapsed noisy curve became a strand");
    require(std::memcmp(noisy.data(),noisyBefore.data(),sizeof(noisy)) == 0, "noisy rejection changed points");
    Point loop[] = {{0,0,0}, {.4f,.2f,0}, {0,0,0}};
    const Point loopTip = loop[2];
    require(modmesh::attachment::shiftRenderPoints(loop,3,Point{0,4,0}), "extended loop rejected because endpoints coincide");
    require(near(loop[0].y,4) && near(loop[1].y,2.2f), "extended loop taper wrong");
    require(std::memcmp(&loop[2],&loopTip,sizeof(loopTip)) == 0, "extended loop endpoint changed");
    auto nan = std::numeric_limits<float>::quiet_NaN();
    std::memcpy(points, original, sizeof(points));
    require(!modmesh::attachment::shiftRenderPoints(points,3,Point{nan,0,0}), "NaN delta accepted");
    require(std::memcmp(points,original,sizeof(points)) == 0, "NaN failure mutated points");
    points[2].x = nan;
    Point before[3];
    std::memcpy(before, points, sizeof(before));
    require(!modmesh::attachment::shiftRenderPoints(points,3,Point{1,0,0}), "NaN point accepted");
    require(std::memcmp(points,before,sizeof(points)) == 0, "partial writes before failed validation");
    const float big = std::numeric_limits<float>::max();
    Point huge[] = {{big,0,0},{big,1,0}};
    require(!modmesh::attachment::shiftRenderPoints(huge,2,Point{big,0,0}), "overflow accepted");
    require(huge[0].x == big && huge[1].x == big && huge[1].y == 1, "overflow changed points");
    require(!modmesh::attachment::shiftRenderPoints(points,1,Point{1,0,0}), "single point accepted");
    require(!modmesh::attachment::shiftRenderPoints(points,65537,Point{1,0,0}), "unbounded count accepted");
}

void ownershipChecks() {
    std::array<Point,3> controls{{{0,0,0},{1,0,0},{4,0,0}}};
    std::array<Point,3> curves = controls;
    std::array<float,3> percents{{0,.25f,1}};
    NativeSpline source{
        {17,controls.data(),controls.data()+3,controls.data()+3},
        {23,percents.data(),percents.data()+3,percents.data()+3},
        {31,curves.data(),curves.data()+3,curves.data()+3},
        0x12345678, 2.5};
    unsigned char original[sizeof(source)];
    std::memcpy(original,&source,sizeof(source));
    auto originalControls = controls;
    auto originalCurves = curves;
    using Shadow = modmesh::attachment::ScopedSplineShadow<NativeSpline,CheckedAllocator>;
    {
        Shadow shadow(source);
        require(shadow.valid() && live.size() == 3, "three independent arrays not installed");
        require(source.control_pts.m_first != controls.data() &&
                source.curve_pts.m_first != curves.data(), "borrowed vector exposed");
        require(modmesh::attachment::shiftRenderPoints(source.curve_pts.m_first,3,Point{0,4,0}), "shadow shift failed");
        // Native rebuild always frees the old curve, even with spare capacity.
        CheckedAllocator<Point>{}.deallocate(source.curve_pts.m_first,0);
        source.curve_pts.m_first = CheckedAllocator<Point>{}.allocate(7);
        source.curve_pts.m_last = source.curve_pts.m_end = source.curve_pts.m_first + 7;
        source.curve_pts.m_first[0] = {90,80,70};
        source.control_pts_pct.m_first[0] = .125f;
        source.flags = 0;
        source.cache = 99;
    }
    require(live.empty(), "shadow arrays leaked");
    require(std::memcmp(&source,original,sizeof(source)) == 0, "spline object not restored byte for byte");
    require(std::memcmp(controls.data(),originalControls.data(),sizeof(controls)) == 0 &&
            std::memcmp(curves.data(),originalCurves.data(),sizeof(curves)) == 0 && percents[0] == 0,
            "original spline arrays modified");
    for (int failure = 0; failure < 3; ++failure) {
        allocations = 0;
        failAfter = failure;
        bool threw = false;
        try { Shadow shadow(source); } catch (const std::bad_alloc &) { threw = true; }
        require(threw && live.empty(), "partial allocation failure leaked");
        require(std::memcmp(&source,original,sizeof(source)) == 0, "failed shadow changed source");
    }
    failAfter = -1;
    source.control_pts.m_last = source.control_pts.m_first + 1;
    {
        Shadow shadow(source);
        require(!shadow.valid() && live.empty(), "invalid control count allocated");
    }
}

void nativeInterfaceChecks() {
    // The captured retail interface has five definition records and zip state
    // three. Generated tubes have no AI info; exact table membership suffices.
    std::array<std::uint32_t,11> raw{};
    raw[7] = 0x0f3bdc08;
    raw[8] = 0x01000005;
    raw[9] = 0x0f3bdf10;
    raw[10] = 3;
    modmesh::attachment::NativeTentacleInterface ifc;
    std::memcpy(&ifc,raw.data(),sizeof(ifc));
    std::uint32_t tubes[5] = {0x0f4e2190,0x0f4e2628,0x0f4e2938,0x0f4e3000,0x0f4e3400};
    require(modmesh::attachment::matchingTubeIndex(ifc,tubes,tubes[4]) == 4,
            "zip state incorrectly used as tube count");
    require(modmesh::attachment::matchingTubeIndex(ifc,tubes,0x12345678) == -1,
            "foreign tube matched an owner");
    ifc.zipState = 0;
    require(modmesh::attachment::matchingTubeIndex(ifc,tubes,tubes[0]) == 0,
            "no-AI generated tube rejected while zip inactive");
    ifc.definitionCount = 129;
    require(modmesh::attachment::matchingTubeIndex(ifc,tubes,tubes[0]) == -1,
            "unbounded definition count accepted");
}
}

int main() {
    try {
        endpointChecks();
        ownershipChecks();
        nativeInterfaceChecks();
        std::cout << "PASS attachment arc-length taper, exact tip, collapsed/noisy curve rejection, extended loops, nonfinite/overflow/count guards; "
                     "private spline arrays, native reallocation, byte-exact restoration, allocation-failure cleanup; "
                     "native definition count, no-AI tube ownership, foreign tube rejection\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << "FAIL " << e.what() << '\n';
        return 1;
    }
}
