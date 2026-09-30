#include "../src/mod_mesh_attachment_geometry.h"
#include <cstdio>
#include <cstdlib>
#include <limits>

namespace rt = modmesh::retarget;
namespace at = modmesh::attachment;
static unsigned checks = 0;

static void require(bool pass, const char *label)
{
    ++checks;
    if (!pass) { std::fprintf(stderr, "FAIL: %s\n", label); std::exit(1); }
}

static bool close(const rt::Vec3 &a, const rt::Vec3 &b, float epsilon = 3e-5f)
{
    for (int axis = 0; axis < 3; ++axis) if (std::abs(a[axis] - b[axis]) > epsilon) return false;
    return true;
}

static rt::Matrix transform(float angle, float x = 0, float y = 0, float z = 0)
{
    auto result = rt::identity();
    result[0] = result[5] = std::cos(angle);
    result[1] = std::sin(angle); result[4] = -result[1];
    result[12] = x; result[13] = y; result[14] = z;
    return result;
}

struct Fixture {
    modmesh::pcmeshsource::Source file;
    modmesh::pcmeshsource::Mesh mesh;
    explicit Fixture(std::size_t bones)
    {
        mesh.nbones = unsigned(bones);
        mesh.sections.resize(1);
        mesh.sections[0].stride = 64;
        for (std::size_t bone = 0; bone < bones; ++bone)
            mesh.sections[0].palette.push_back(uint16_t(bone));
    }
    void add(const rt::Vec3 &position, std::size_t bone, float weight = 1,
             std::size_t otherBone = 0)
    {
        float row[16]{};
        std::copy(position.begin(), position.end(), row);
        row[8] = float(bone); row[12] = weight;
        row[9] = float(otherBone); row[13] = 1 - weight;
        // Retail unused influences can carry an invalid sentinel index.
        row[10] = row[11] = -1;
        const auto old = file.bytes.size(); file.bytes.resize(old + sizeof(row));
        std::memcpy(file.bytes.data() + old, row, sizeof(row));
        ++mesh.sections[0].vertexCount;
        mesh.sections[0].vertexBytes = unsigned(file.bytes.size());
    }
    void box(const rt::Vec3 &joint, const rt::Vec3 &negative, const rt::Vec3 &positive,
             std::size_t bone = 0)
    {
        for (unsigned corner = 0; corner < 8; ++corner) {
            rt::Vec3 point = joint;
            for (int axis = 0; axis < 3; ++axis)
                point[axis] += corner & (1u << axis) ? positive[axis] : -negative[axis];
            add(point, bone);
        }
    }
    void setFloat(std::size_t vertex, std::size_t lane, float value)
    { std::memcpy(file.bytes.data() + vertex * 64 + lane * 4, &value, 4); }
};

static rt::Plan plan(const std::vector<rt::Matrix> &source, const std::vector<rt::Matrix> &target,
                     const std::vector<int> &map)
{
    rt::Plan result;
    require(rt::prepare(source, std::vector<int>(source.size(), -1), map, target, result),
            "prepare valid test retarget plan");
    return result;
}

static at::Geometry prepare(const Fixture &source, const Fixture &target, const rt::Plan &plan)
{
    at::Geometry result;
    std::string why;
    require(at::prepare(source.file, source.mesh, target.file, target.mesh, plan, result, &why),
            why.empty() ? "prepare geometry" : why.c_str());
    return result;
}

int main()
{
    // Different joint bind axes (90 versus 30 degrees), source size, actor
    // rotation and translations. Expected values are source model coordinates
    // carried by the actor, independent of either bone's local coordinate axes.
    const auto sourceBind = transform(0.5235988f, 1, 3, -2);
    const auto targetBind = transform(1.5707963f, -4, 8, 1);
    Fixture source(1), target(1);
    source.box(rt::position(sourceBind), {{1,2,3}}, {{2,3,4}});
    target.box(rt::position(targetBind), {{2,4,6}}, {{4,6,8}});
    const auto mapping = plan({sourceBind}, {targetBind}, {0});
    const auto geometry = prepare(source, target, mapping);
    const auto world = transform(-0.71f, 37, -12, 4);
    const auto targetCurrent = rt::multiply(targetBind, world);
    const auto sourceCurrent = rt::multiply(sourceBind, world);
    const auto nativeWorld = rt::point({{-2,6,5}}, world); // target joint + (2,-2,4)
    rt::Vec3 moved{{99,99,99}};
    require(at::transfer(geometry, nativeWorld, targetCurrent, sourceCurrent, 0, 0, moved),
            "transfer different-sized geometry with rotated bind bases");
    require(close(moved, rt::point({{2,2,0}}, world)),
            "signed model-axis scale and source joint offset survive bind/world rotation");
    const auto outside = rt::point({{100,-100,100}}, world);
    require(at::transfer(geometry, outside, targetCurrent, sourceCurrent, 0, 0, moved),
            "transfer outside native envelope");
    require(close(moved, rt::point({{3,1,2}}, world)),
            "different shape clamps render anchor inside source envelope");
    require(geometry.sourceSamples.size() == 8 && geometry.targetSamples.size() == 8,
            "retain source and target skin samples for correspondence diagnostics");

    // Same rig must be exactly invariant even for points well outside its skin
    // envelope; clamping those native offsets would change authored attachments.
    const auto equalMapping = plan({sourceBind}, {sourceBind}, {0});
    const auto equal = prepare(source, source, equalMapping);
    const auto animated = transform(1.32f, -12, 29, 8);
    const rt::Vec3 farPoint{{100,-70,60}};
    require(at::transfer(equal, farPoint, animated, animated, 0, 0, moved), "same-rig transfer");
    require(close(moved, farPoint), "same rig preserves arbitrary outside offset");

    // Signed radii are asymmetric: positive and negative sides cannot use a
    // single radius or an overall body-height scale.
    require(at::transfer(geometry, rt::point({{-6,4,-5}}, world), targetCurrent,
                         sourceCurrent, 0, 0, moved), "negative signed side transfer");
    require(close(moved, rt::point({{0,1,-5}}, world)), "negative side uses its own extent");

    // The nearest pivot is bone 1, but the nearest actual skinned surface is
    // bone 0. Following a moving skin sample also observes current animation.
    Fixture surfaces(2);
    surfaces.add({{10,0,0}}, 0);
    surfaces.add({{20,0,0}}, 1);
    auto surfacePlan = plan({transform(0), transform(0,9)}, {transform(0), transform(0,9)}, {0,1});
    const auto surfaceGeometry = prepare(surfaces, surfaces, surfacePlan);
    std::vector<rt::Matrix> current{transform(0), transform(0,9)};
    const auto nativeCopy = current;
    at::Anchor anchor{99,99};
    require(at::chooseAnchor(surfaceGeometry, current.data(), current.size(), {{10.1f,0,0}}, anchor),
            "choose from native skin samples");
    require(anchor.source == 0 && anchor.target == 0, "surface distance wins over nearest pivot");
    current[0] = transform(0,40);
    require(at::chooseAnchor(surfaceGeometry, current.data(), current.size(), {{20,0,0}}, anchor),
            "choose after attachment switches body region");
    require(anchor.target == 1, "animated closest surface switches without stale first-frame cache");
    require(current[1] == nativeCopy[1], "anchor selection preserves input pose");

    Fixture weighted(2);
    weighted.add({{0,0,0}}, 0, .75f, 1);
    weighted.add({{20,0,0}}, 1);
    const auto weightedPlan = plan({transform(0),transform(0)}, {transform(0),transform(0)}, {0,1});
    const auto weightedGeometry = prepare(weighted, weighted, weightedPlan);
    current = {transform(0,10),transform(0)};
    const auto unchanged = current;
    require(at::chooseAnchor(weightedGeometry,current.data(),current.size(),{{7.5f,0,0}},anchor),
            "evaluate all influences of native skin sample");
    require(anchor.target == 0 && current == unchanged, "blended surface chooses dominant bone without pose mutation");
    weighted.add({{100,0,0}}, 1, .99f, 0);
    const auto bounded = prepare(weighted, weighted, weightedPlan);
    require(bounded.targetBounds[0].positive[0] == 0, "small outlying skin weight does not inflate unrelated envelope");

    // All public failure cases leave an existing prepared result/output intact.
    at::Geometry sentinel = geometry;
    auto malformed = target;
    malformed.mesh.sections[0].stride = 48;
    require(!at::prepare(source.file,source.mesh,malformed.file,malformed.mesh,mapping,sentinel), "reject unsupported vertex layout");
    require(sentinel.targetSamples.size() == geometry.targetSamples.size(), "failed preparation is atomic");
    malformed = target; malformed.mesh.sections[0].vertexOffset = 0xfffffff0u;
    require(!at::prepare(source.file,source.mesh,malformed.file,malformed.mesh,mapping,sentinel), "reject truncated vertex range");
    malformed = target; malformed.mesh.sections[0].palette[0] = 5;
    require(!at::prepare(source.file,source.mesh,malformed.file,malformed.mesh,mapping,sentinel), "reject out-of-range palette bone");
    malformed = target; malformed.setFloat(0,8,.5f);
    require(!at::prepare(source.file,source.mesh,malformed.file,malformed.mesh,mapping,sentinel), "reject nonintegral positive-weight index");
    malformed = target; malformed.setFloat(0,12,-1);
    require(!at::prepare(source.file,source.mesh,malformed.file,malformed.mesh,mapping,sentinel), "reject negative skin weight");
    malformed = target; malformed.setFloat(0,12,.2f);
    require(!at::prepare(source.file,source.mesh,malformed.file,malformed.mesh,mapping,sentinel), "reject unnormalized skin weights");
    malformed = target; malformed.setFloat(0,0,std::numeric_limits<float>::quiet_NaN());
    require(!at::prepare(source.file,source.mesh,malformed.file,malformed.mesh,mapping,sentinel), "reject nonfinite vertex");
    auto badPlan = mapping; badPlan.targetBone[0] = 3;
    require(!at::prepare(source.file,source.mesh,target.file,target.mesh,badPlan,sentinel), "reject invalid correspondence index");
    moved = {{91,92,93}};
    require(!at::transfer(geometry,nativeWorld,targetCurrent,sourceCurrent,1,0,moved), "reject invalid source index");
    require(close(moved,{{91,92,93}}), "failed transfer leaves caller output intact");
    anchor = {98,99};
    require(!at::chooseAnchor(surfaceGeometry,current.data(),1,{{0,0,0}},anchor), "reject truncated native pose array");
    require(anchor.source == 98 && anchor.target == 99, "failed anchor selection leaves output intact");
    std::printf("PASS attachment geometry: %u checks (orientation, signed size, clamp, identity, animated surface, invalid data)\n", checks);
    return 0;
}
