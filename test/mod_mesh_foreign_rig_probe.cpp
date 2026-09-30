#include "../src/mod_mesh_foreign_rig.h"

#include <cstdio>
#include <stdexcept>

namespace {
namespace fr = modmesh::foreignrig;
namespace rt = modmesh::retarget;

void require(bool condition, const std::string &message)
{
    if (!condition) throw std::runtime_error(message);
}

void equal(const rt::Vec3 &a, const rt::Vec3 &b, const char *message)
{
    for (int d = 0; d < 3; ++d) require(std::abs(a[d] - b[d]) < 2e-5f, message);
}

rt::Vec3 difference(const rt::Vec3 &a, const rt::Vec3 &b)
{
    return {{a[0]-b[0], a[1]-b[1], a[2]-b[2]}};
}

double length(const rt::Vec3 &v)
{
    return std::sqrt(double(v[0])*v[0] + double(v[1])*v[1] + double(v[2])*v[2]);
}

struct Fixture {
    std::vector<fr::Joint> source;
    std::vector<std::string> nativeNames;
    std::vector<rt::Matrix> nativeBind;
    std::map<std::string, std::string> explicitMap;

    Fixture()
    {
        const char *native[] = {"Pelvis", "Spine", "Head", "L UpperArm", "L Forearm", "L Hand",
                               "R UpperArm", "R Forearm", "R Hand", "L Thigh", "L Calf", "L Foot",
                               "R Thigh", "R Calf", "R Foot"};
        const char *mixamo[] = {"Hips", "Spine", "Head", "LeftArm", "LeftForeArm", "LeftHand",
                               "RightArm", "RightForeArm", "RightHand", "LeftUpLeg", "LeftLeg", "LeftFoot",
                               "RightUpLeg", "RightLeg", "RightFoot"};
        const int parent[] = {-1,0,1,1,3,4,1,6,7,0,9,10,0,12,13};
        const rt::Vec3 target[] = {
            {{0,0,0}}, {{0,.5f,0}}, {{0,1,0}},
            {{.25f,.8f,0}}, {{.6f,.8f,0}}, {{.9f,.8f,0}},
            {{-.25f,.8f,0}}, {{-.6f,.8f,0}}, {{-.9f,.8f,0}},
            {{.15f,-.05f,0}}, {{.15f,-.55f,0}}, {{.15f,-1,.1f}},
            {{-.15f,-.05f,0}}, {{-.15f,-.55f,0}}, {{-.15f,-1,.1f}}
        };
        const rt::Vec3 posed[] = {
            {{0,0,0}}, {{0,.42f,.06f}}, {{0,.9f,.12f}},
            {{.22f,.63f,.04f}}, {{.5f,.42f,.08f}}, {{.64f,.15f,.12f}},
            {{-.2f,.63f,.04f}}, {{-.4f,.38f,-.04f}}, {{-.58f,.11f,-.14f}},
            {{.15f,-.04f,0}}, {{.2f,-.5f,.18f}}, {{.26f,-.95f,.13f}},
            {{-.15f,-.04f,0}}, {{-.27f,-.46f,-.08f}}, {{-.32f,-.91f,.18f}}
        };
        source.push_back({"Armature", -1, {{-.1f,0,0}}});
        for (int j = 0; j < 15; ++j) {
            nativeNames.push_back(std::string("Bip01 ") + native[j]);
            auto bind = rt::identity();
            for (int d = 0; d < 3; ++d) bind[12+d] = target[j][d];
            nativeBind.push_back(bind);
            source.push_back({std::string("mixamorig:") + mixamo[j], parent[j]+1, posed[j]});
        }
        // A later-index twist between elbow and shoulder proves preparation
        // obeys parent topology and preserves a palette larger than 26 bones.
        source.push_back({"unmapped_upper_arm_twist", 4, {{.36f,.525f,.06f}}});
        source[5].parent = 16;
        for (int j = 0; j < 11; ++j)
            source.push_back({"custom_helper_" + std::to_string(j), 4,
                              {{.23f + j*.01f, .6f - j*.01f, .05f}}});
    }

    rt::Plan prepare() const
    {
        rt::Plan plan;
        std::string why;
        const bool ok = fr::prepare(source, nativeNames, nativeBind, explicitMap, plan, &why);
        require(ok, why);
        return plan;
    }
};

void mappingsAndBinds()
{
    require(fr::boneKey("mixamorig:LeftArm.001") == "lupperarm", "recognized Blender-suffixed Mixamo role lost");
    require(fr::boneKey("helper.001") != fr::boneKey("helper.002"), "distinct arbitrary helper suffixes collapsed");
    const Fixture f;
    const auto original = f.nativeBind;
    const auto plan = f.prepare();
    require(plan.absoluteBoneAxes && plan.sourceBind.size() == 28, "source palette was lost or limited to 26");
    require(plan.parent[1] == -1 && plan.parent[0] == 1, "armature wrapper did not follow mapped pelvis");
    for (int j = 0; j < 15; ++j) require(plan.targetBone[size_t(j+1)] == j, "Mixamo/native alias failed");
    for (size_t j = 16; j < 28; ++j) require(plan.targetBone[j] == -1, "helper was guessed as a native bone");
    for (size_t j = 0; j < f.source.size(); ++j)
        equal(rt::position(plan.sourceBind[j]), f.source[j].position, "source bind pivot changed");
    require(f.nativeBind == original, "native matrices were modified");

    Fixture renamed = f;
    for (int j = 0; j < 15; ++j) {
        renamed.source[size_t(j+1)].name = "author_joint_" + std::to_string(100-j);
        renamed.explicitMap.emplace(renamed.source[size_t(j+1)].name, renamed.nativeNames[size_t(j)]);
    }
    const auto explicitPlan = renamed.prepare();
    require(explicitPlan.targetBone == plan.targetBone, "explicit names were interpreted as joint indices");
    for (size_t j = 0; j < plan.sourceBind.size(); ++j)
        require(explicitPlan.sourceBind[j] == plan.sourceBind[j], "explicit mapping changed the source binds");
}

void rejectedInputs()
{
    auto rejected = [](const Fixture &f, const char *reason) {
        rt::Plan untouched;
        untouched.rootDriver = 777;
        std::string why;
        require(!fr::prepare(f.source, f.nativeNames, f.nativeBind, f.explicitMap, untouched, &why), reason);
        require(!why.empty() && untouched.rootDriver == 777, "failure published an incomplete plan");
    };
    Fixture missing;
    missing.source[6].name = "unmapped_left_hand";
    rejected(missing, "missing core hand was accepted");
    missing.explicitMap.emplace("unmapped_left_hand", "Bip01 L Hand");
    missing.prepare();

    Fixture ambiguous;
    ambiguous.nativeNames.push_back("L_UpperArm");
    ambiguous.nativeBind.push_back(ambiguous.nativeBind[3]);
    rejected(ambiguous, "ambiguous native alias was accepted");

    Fixture duplicate;
    duplicate.source.push_back({"other_namespace:LeftArm", 2, {{.3f,.7f,0}}});
    rejected(duplicate, "two source arms mapped to one native arm");

    Fixture wrongLimb;
    wrongLimb.source[5].parent = 7;
    rejected(wrongLimb, "left forearm accepted a right-arm parent");

    Fixture cycle;
    cycle.source[4].parent = 5;
    rejected(cycle, "parent cycle was accepted");

    Fixture disconnected;
    disconnected.source.push_back({"disconnected_helper", -1, {{0,0,0}}});
    rejected(disconnected, "disconnected helper was accepted");

    Fixture zeroLimb;
    zeroLimb.source[5].position = zeroLimb.source[4].position;
    rejected(zeroLimb, "coincident limb guide was accepted");

    Fixture typo;
    typo.explicitMap.emplace("spelling_mistake", "Bip01 L Hand");
    rejected(typo, "explicit map typo was silently ignored");
}

void evaluatePoses()
{
    const Fixture f;
    const auto plan = f.prepare();
    auto authoredPose = f.nativeBind;
    for (size_t j = 0; j < plan.targetBone.size(); ++j)
        if (plan.targetBone[j] >= 0) authoredPose[size_t(plan.targetBone[j])] = plan.sourceBind[j];
    std::vector<rt::Matrix> evaluated;
    std::string why;
    bool ok = rt::evaluate(plan, authoredPose.data(), authoredPose.size(), evaluated, &why);
    require(ok, why);
    // Every original source influence remains an identity deformation when
    // native driver axes reproduce the source rest pose, helpers included.
    for (size_t j = 0; j < plan.sourceBind.size(); ++j) {
        const auto skin = rt::multiply(plan.sourceInverse[j], evaluated[j]);
        const auto identity = rt::identity();
        for (size_t lane = 0; lane < 16; ++lane)
            require(std::abs(skin[lane] - identity[lane]) < 2e-5f, "authored rest geometry deformed");
    }

    ok = rt::evaluate(plan, f.nativeBind.data(), f.nativeBind.size(), evaluated, &why);
    require(ok, why);
    const auto restResult = evaluated;
    const std::pair<int,int> limbs[] = {{4,5},{5,6},{7,8},{8,9},{10,11},{11,12},{13,14},{14,15}};
    for (const auto &limb : limbs) {
        const auto a = size_t(limb.first), b = size_t(limb.second);
        const auto sourceSegment = difference(f.source[b].position, f.source[a].position);
        const auto emittedSegment = difference(rt::position(evaluated[b]), rt::position(evaluated[a]));
        require(std::abs(length(sourceSegment)-length(emittedSegment)) < 2e-5,
                "native rest pose changed authored limb length");
        const auto targetSegment = difference(rt::position(f.nativeBind[size_t(plan.targetBone[b])]),
                                              rt::position(f.nativeBind[size_t(plan.targetBone[a])]));
        rt::Vec3 direction{}, expected{};
        for (int d = 0; d < 3; ++d) {
            direction[d] = float(emittedSegment[d] / length(emittedSegment));
            expected[d] = float(targetSegment[d] / length(targetSegment));
        }
        equal(direction, expected, "lowered source limbs were lowered twice instead of aligned to native axes");
    }

    auto shoulderRotation = rt::identity();
    const float angle = .7f, c = std::cos(angle), s = std::sin(angle);
    shoulderRotation[0] = c; shoulderRotation[1] = s;
    shoulderRotation[4] = -s; shoulderRotation[5] = c;
    auto animated = f.nativeBind;
    for (int target : {3,4,5}) animated[size_t(target)] = rt::multiply(animated[size_t(target)], shoulderRotation);
    ok = rt::evaluate(plan, animated.data(), animated.size(), evaluated, &why);
    require(ok, why);
    for (size_t j : {size_t(5),size_t(6),size_t(16),size_t(17),size_t(27)}) {
        const auto before = difference(rt::position(restResult[j]), rt::position(restResult[4]));
        const auto after = difference(rt::position(evaluated[j]), rt::position(evaluated[4]));
        equal(after, rt::vector(before, shoulderRotation), "source helper or limb detached during native arm rotation");
    }
    for (const auto &matrix : evaluated) require(rt::affine(matrix), "animated source pose became nonfinite");
}

} // namespace

int main(int argc, char **)
{
    try {
        mappingsAndBinds();
        rejectedInputs();
        if (argc == 1) evaluatePoses();
        std::puts(argc == 1
            ? "PASS: 28-bone foreign rig, Mixamo/explicit mapping, rest geometry, source proportions, animated helpers, rejection cases"
            : "PASS: 28-bone foreign rig preparation, Mixamo/explicit mapping, bind pivots, rejection cases (evaluation skipped)");
        return 0;
    } catch (const std::exception &error) {
        std::fprintf(stderr, "FAIL: %s\n", error.what());
        return 1;
    }
}
