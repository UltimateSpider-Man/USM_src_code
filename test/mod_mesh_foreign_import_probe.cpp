#include "../src/mod_mesh_foreign_import.h"
#include "../src/mod_pcmesh_source.h"
#include "../src/mod_pcskel_source.h"

#include <cstdio>
#include <fstream>
#include <iterator>
#include <stdexcept>

namespace {
void require(bool pass, const std::string &why)
{
    if (!pass) throw std::runtime_error(why);
}
std::vector<uint8_t> read(const char *path)
{
    std::ifstream input(path, std::ios::binary);
    require(bool(input), std::string("cannot read ") + path);
    return {std::istreambuf_iterator<char>(input), {}};
}

void synthetic()
{
    namespace mm = modmesh;
    const char *roles[] = {"Pelvis","Spine","Head","L UpperArm","L Forearm","L Hand",
                          "R UpperArm","R Forearm","R Hand","L Thigh","L Calf","L Foot",
                          "R Thigh","R Calf","R Foot"};
    const char *sourceNames[] = {"Hips","Spine","Head","LeftArm","LeftForeArm","LeftHand",
                                "RightArm","RightForeArm","RightHand","LeftUpLeg","LeftLeg","LeftFoot",
                                "RightUpLeg","RightLeg","RightFoot"};
    const int parent[] = {-1,0,1,1,3,4,1,6,7,0,9,10,0,12,13};
    const mm::retarget::Vec3 positions[] = {
        {{0,0,0}},{{0,.5f,0}},{{0,1,0}},{{.2f,.8f,0}},{{.6f,.8f,0}},{{.9f,.8f,0}},
        {{-.2f,.8f,0}},{{-.6f,.8f,0}},{{-.9f,.8f,0}},{{.15f,0,0}},{{.15f,-.5f,0}},{{.15f,-1,0}},
        {{-.15f,0,0}},{{-.15f,-.5f,0}},{{-.15f,-1,0}}
    };
    mm::Scene input;
    input.sceneScale = .01; input.cfg.scale = 2; input.cfg.yaw = 90;
    input.cfg.offset = {1,2,3}; input.cfg.fit = true;
    mm::Model armature; armature.id = 1; armature.name = "Armature"; armature.type = "Null";
    armature.T = {100,0,0}; input.models.emplace(1,armature);
    std::vector<std::string> nativeNames;
    std::vector<mm::retarget::Matrix> nativeBind;
    mm::Geom geometry; geometry.id = 500;
    geometry.ctrl = {0,0,0, 10,0,0, 0,10,0}; geometry.pvi = {0,1,-3};
    for (int j = 0; j < 15; ++j) {
        nativeNames.push_back(std::string("Bip01 ")+roles[j]);
        auto matrix = mm::retarget::identity();
        for (int d = 0; d < 3; ++d) matrix[12+d] = positions[j][d];
        nativeBind.push_back(matrix);
        mm::Model model; model.id = 100+j; model.name = std::string("mixamorig:")+sourceNames[j]+".001";
        model.type = "LimbNode"; model.parent = parent[j] < 0 ? 1 : 100+parent[j];
        const auto previous = parent[j] < 0 ? mm::retarget::Vec3{} : positions[parent[j]];
        model.T = {(positions[j][0]-previous[0])*100,(positions[j][1]-previous[1])*100,(positions[j][2]-previous[2])*100};
        input.models.emplace(model.id,model);
        mm::GCluster cluster; cluster.boneModelId = model.id; cluster.boneName = mm::Scene::normName(model.name);
        cluster.haveLink = cluster.haveTransform = true;
        cluster.linkMatrix = cluster.transformMatrix = {{1,0,0,0,0,1,0,0,0,0,1,0,100,0,0,1}};
        for (int d = 0; d < 3; ++d) {
            cluster.linkPos[d] = cluster.linkMatrix[12+d] = positions[j][d]*100+(d==0?100:0);
            cluster.transformMatrix[12+d] = -positions[j][d]*100;
        }
        if (j == 0) { cluster.idx = {0,2}; cluster.w = {1,1}; }
        if (j == 3) { cluster.idx = {1}; cluster.w = {.25}; }
        if (j == 4) { cluster.idx = {1}; cluster.w = {.75}; }
        geometry.clusters.push_back(cluster);
    }
    for (int j = 0; j < 2; ++j) {
        mm::Model helper; helper.id = 200+j; helper.name = j ? "helper.002" : "helper.001";
        helper.type = "LimbNode"; helper.parent = 103; input.models.emplace(helper.id,helper);
        mm::GCluster cluster; cluster.boneModelId = helper.id; cluster.boneName = "helper";
        geometry.clusters.push_back(cluster);
    }
    input.geoms.emplace(geometry.id,geometry);
    mm::Model mesh; mesh.id = 900; mesh.parent = 1; mesh.name = "CustomBody"; mesh.type = "Mesh";
    mesh.geoms = {500}; input.models.emplace(mesh.id,mesh); input.meshModelOrder = {900};
    const auto originalWeights = input.geoms.at(500).clusters[3].w;
    mm::Scene prepared; mm::OrigMeshRef ref; mm::retarget::Plan plan; std::string why;
    bool ok = mm::foreignrig::prepareScene(input,nativeNames,nativeBind,prepared,ref,plan,&why);
    require(ok,why);
    require(ref.nbones == 18 && ref.clusterBoneIndices.size() == 17, "connected model IDs did not preserve distinct suffixed helpers");
    require(input.geoms.at(500).clusters[3].w == originalWeights && input.cfg.scale == 2,
            "private preparation changed source scene");
    require(std::abs(prepared.cfg.scale-1) < 1e-6 && !prepared.cfg.fit, "bone fit was not applied exactly once");
    const mm::M4 user = mm::M4::translate(prepared.cfg.offset)
        * mm::M4::scale({prepared.cfg.scale,prepared.cfg.scale,prepared.cfg.scale})
        * mm::eulerDeg({0,prepared.cfg.yaw,0},0);
    const auto transformed = user.point(mm::detail::nodeGlobal(prepared,prepared.models.at(900)).point({0,0,0}));
    const int root = ref.clusterBoneIndices.at(prepared.geoms.at(500).clusters[0].boneName);
    for (const auto &pair : {std::pair<double,float>{transformed.x,plan.sourceBind[size_t(root)][12]},
                            {transformed.y,plan.sourceBind[size_t(root)][13]},
                            {transformed.z,plan.sourceBind[size_t(root)][14]}})
        require(std::abs(pair.first-pair.second) < 1e-5, "mesh and skeleton fit/user transforms diverged");
    const auto rejected = [&](mm::Scene bad, const char *message) {
        mm::Scene untouched; untouched.srcName = "sentinel";
        mm::OrigMeshRef untouchedRef; untouchedRef.nbones = 999;
        mm::retarget::Plan untouchedPlan; untouchedPlan.rootDriver = 999;
        const bool accepted = mm::foreignrig::prepareScene(bad,nativeNames,nativeBind,untouched,untouchedRef,untouchedPlan,&why);
        require(!accepted && !why.empty(),message);
        require(untouched.srcName == "sentinel" && untouchedRef.nbones == 999 && untouchedPlan.rootDriver == 999,
                "failed preparation published partial state");
    };
    auto bad = input; bad.geoms.at(500).clusters[0].transformMatrix[12] += 100;
    rejected(bad,"mismatched mesh bind/current transform accepted");
    bad = input; bad.cfg.donorPose = "arms_down"; rejected(bad,"authored rig accepted unrigged donor pose");
    bad = input; bad.cfg.retarget = false; rejected(bad,"retarget=off silently remapped authored palette");
    auto hole = input;
    hole.geoms.at(500).clusters[0].w[0] = 0;
    mm::Scene repaired; mm::OrigMeshRef repairRef; mm::retarget::Plan repairPlan;
    ok = mm::foreignrig::prepareScene(hole,nativeNames,nativeBind,repaired,repairRef,repairPlan,&why);
    require(ok,why);
    std::map<size_t,double> repairedWeights;
    const auto unchangedPrefix = [](const mm::GCluster &before,const mm::GCluster &after) {
        require(after.idx.size() >= before.idx.size() && after.w.size() >= before.w.size(), "authored skin entries removed");
        require(before.idx.empty() || std::memcmp(before.idx.data(),after.idx.data(),before.idx.size()*sizeof(int64_t)) == 0,
                "authored vertex indices changed");
        require(before.w.empty() || std::memcmp(before.w.data(),after.w.data(),before.w.size()*sizeof(double)) == 0,
                "authored source weights changed bytes");
    };
    for (size_t c = 0; c < hole.geoms.at(500).clusters.size(); ++c) {
        const auto &before = hole.geoms.at(500).clusters[c], &after = repaired.geoms.at(500).clusters[c];
        unchangedPrefix(before,after);
        for (size_t k = before.idx.size(); k < after.idx.size(); ++k) {
            require(after.idx[k] == 0, "repair changed an already weighted vertex");
            repairedWeights[c] += after.w[k];
        }
    }
    require(repairedWeights.size() == 3 && repairedWeights[0] == .5
            && repairedWeights[3] == .125 && repairedWeights[4] == .375,
            "isolated hole did not average normalized adjacent authored weights");
    auto wide = hole;
    auto &wideClusters = wide.geoms.at(500).clusters;
    wideClusters[0].idx = {0}; wideClusters[0].w = {0};
    for (size_t c = 3; c <= 10; ++c) {
        wideClusters[c].idx = {c <= 6 ? 1 : 2}; wideClusters[c].w = {.25};
    }
    ok = mm::foreignrig::prepareScene(wide,nativeNames,nativeBind,repaired,repairRef,repairPlan,&why);
    require(ok,why);
    unsigned lanes = 0; double total = 0;
    for (size_t c = 0; c < wideClusters.size(); ++c) {
        const auto &after = repaired.geoms.at(500).clusters[c]; unchangedPrefix(wideClusters[c],after);
        for (size_t k = wideClusters[c].idx.size(); k < after.idx.size(); ++k) {
            require(after.idx[k] == 0 && after.w[k] == .25, "four-lane repair was not normalized");
            ++lanes; total += after.w[k];
        }
    }
    require(lanes == 4 && total == 1, "isolated repair exceeded the four-influence limit");
    bad = input;
    for (auto &cluster : bad.geoms.at(500).clusters) std::fill(cluster.w.begin(),cluster.w.end(),0.0);
    rejected(bad,"wholly unweighted geometry was assigned invented source weights");
    bad = input;
    bad.geoms.at(500).ctrl.insert(bad.geoms.at(500).ctrl.end(),{20,0,0,30,0,0,20,10,0});
    bad.geoms.at(500).pvi.insert(bad.geoms.at(500).pvi.end(),{3,4,-6});
    rejected(bad,"disconnected unweighted island was assigned source weights");
    bad.geoms.at(500).pvi.insert(bad.geoms.at(500).pvi.end(),{2,3,-5});
    rejected(bad,"repair propagated across an unweighted region through newly repaired points");
    std::puts("PASS: foreign import seam, distinct bone IDs, coherent transforms, isolated source-weight repair, unchanged authored weights, invalid binds/islands/partial-state rejection");
}
}

int main(int argc, char **argv)
{
    if (argc != 1 && argc != 4) {
        std::fprintf(stderr, "usage: mod_mesh_foreign_import_probe <authored.fbx> <target.PCMESH> <target.PCSKEL>\n");
        return 2;
    }
    try {
        synthetic();
        if (argc == 1) return 0;
        const auto fbx = read(argv[1]);
        const auto scene = modmesh::loadScene(argv[1], fbx.data(), fbx.size());
        require(bool(scene), "FBX parse failed");
        modmesh::pcmeshsource::Source native;
        modmesh::pcskelsource::Source skeleton;
        std::string why;
        bool ok = native.parse(read(argv[2]), &why);
        require(ok, why);
        ok = skeleton.parse(read(argv[3]), &why);
        require(ok, why);
        const auto targets = native.replacementMeshNames(argv[2]);
        const auto *target = targets.empty() ? nullptr : native.findMesh(targets.front());
        require(target && skeleton.bones.size() >= target->nbones, "no native skinned target");
        std::vector<std::string> names;
        std::vector<modmesh::retarget::Matrix> bind;
        for (uint32_t bone = 0; bone < target->nbones; ++bone) {
            names.push_back(skeleton.bones[bone].name);
            modmesh::retarget::Matrix m;
            std::copy_n(target->boneMatrices.data()+size_t(bone)*16, 16, m.begin());
            m[3] = m[7] = m[11] = 0; m[15] = 1;
            bind.push_back(m);
        }
        const auto originalBind = bind;
        modmesh::Scene prepared;
        modmesh::OrigMeshRef ref;
        ref.customSource = true;
        ref.targetMeshNames = {target->name};
        modmesh::retarget::Plan plan;
        ok = modmesh::foreignrig::prepareScene(*scene, names, bind, prepared, ref, plan, &why);
        require(ok, why);
        require(bind == originalBind, "native bind matrices changed");
        require(!prepared.cfg.fit && !prepared.cfg.anim && prepared.anims.empty(), "geometry/animation frame not fixed");
        require(!ref.rejectImportedSkin && ref.nbones == int(plan.sourceBind.size()), "source palette not installed");
        size_t triangles = 0;
        std::vector<double> expected(size_t(ref.nbones), 0), actual(size_t(ref.nbones), 0);
        size_t reduced = 0, repairedPoints = 0;
        for (const auto &entry : prepared.geoms) {
            const auto &geometry = entry.second;
            const auto &original = scene->geoms.at(entry.first);
            require(geometry.ctrl == original.ctrl && geometry.pvi == original.pvi
                    && geometry.clusters.size() == original.clusters.size(), "authored mesh data changed");
            std::vector<std::vector<std::pair<int,double>>> per(geometry.ctrl.size()/3);
            std::set<int64_t> appendedVertices;
            for (size_t c = 0; c < geometry.clusters.size(); ++c) {
                const auto &cluster = geometry.clusters[c];
                const auto &authored = original.clusters[c];
                require(cluster.idx.size() >= authored.idx.size() && cluster.w.size() >= authored.w.size(), "authored skin was removed");
                require(authored.idx.empty() || std::memcmp(authored.idx.data(),cluster.idx.data(),authored.idx.size()*sizeof(int64_t)) == 0,
                        "authored source vertex indices changed bytes");
                require(authored.w.empty() || std::memcmp(authored.w.data(),cluster.w.data(),authored.w.size()*sizeof(double)) == 0,
                        "authored source weights changed bytes");
                for (size_t k = authored.idx.size(); k < cluster.idx.size(); ++k) appendedVertices.insert(cluster.idx[k]);
                const int bone = ref.clusterBoneIndices.at(cluster.boneName);
                for (size_t k = 0; k < cluster.idx.size(); ++k)
                    if (cluster.w[k] > 0) per[size_t(cluster.idx[k])].push_back({bone,cluster.w[k]});
            }
            repairedPoints += appendedVertices.size();
            for (auto &point : per) {
                std::sort(point.begin(), point.end(), [](const auto &a, const auto &b) { return a.second > b.second; });
                if (point.size() > 4) { point.resize(4); ++reduced; }
                double sum = 0; for (const auto &w : point) sum += w.second;
                if (sum > 0) for (auto &w : point) w.second /= sum;
            }
            std::vector<size_t> polygon;
            for (int64_t encoded : geometry.pvi) {
                polygon.push_back(size_t(encoded < 0 ? ~encoded : encoded));
                if (encoded >= 0) continue;
                for (size_t k = 1; k+1 < polygon.size(); ++k) {
                    ++triangles;
                    for (size_t vertex : {polygon[0],polygon[k],polygon[k+1]})
                        for (const auto &w : per[vertex]) expected[size_t(w.first)] += w.second;
                }
                polygon.clear();
            }
        }
        std::vector<modmesh::OrigSectionView> views;
        for (const auto &section : target->sections) {
            modmesh::OrigSectionView view;
            view.verts = reinterpret_cast<const float *>(native.bytes.data()+section.vertexOffset);
            view.nverts = section.vertexCount; view.strideBytes = section.stride;
            view.palette = section.palette.data(); view.nbones = int(section.palette.size());
            require(section.stride == 64, "fixture native section is not skinned");
            views.push_back(view);
        }
        const auto built = modmesh::buildSectionsForMesh(prepared, target->name, views, ref);
        size_t emitted = 0, sections = 0;
        for (const auto &optional : built) {
            if (!optional || optional->hide || optional->keepGeometry) continue;
            const auto &section = *optional;
            ++sections; emitted += section.indices.size()/3;
            require(section.palette.size() <= 26, "draw palette exceeds native shader limit");
            for (uint32_t vertex : section.indices) {
                require(size_t(vertex)*16+16 <= section.vertices.size(), "triangle references missing vertex");
                const float *v = section.vertices.data()+size_t(vertex)*16;
                double sum = 0;
                for (int lane = 0; lane < 4; ++lane) if (v[12+lane] > 0) {
                    const int paletteIndex = int(v[8+lane]);
                    require(paletteIndex >= 0 && size_t(paletteIndex) < section.palette.size(), "invalid emitted palette slot");
                    const int bone = section.palette[size_t(paletteIndex)];
                    require(bone >= 0 && size_t(bone) < actual.size(), "invalid emitted source joint");
                    actual[size_t(bone)] += v[12+lane]; sum += v[12+lane];
                }
                require(std::abs(sum-1) < 1e-5, "emitted source weights are not normalized");
            }
        }
        require(emitted == triangles, "authored triangles were lost");
        for (size_t bone = 0; bone < expected.size(); ++bone)
            require(std::abs(expected[bone]-actual[bone]) < std::max(.02,expected[bone]*1e-5),
                    "authored source influence totals changed");
        std::vector<modmesh::retarget::Matrix> posed;
        ok = modmesh::retarget::evaluate(plan, bind.data(), bind.size(), posed, &why);
        require(ok && posed.size() == size_t(ref.nbones), why);
        std::printf("PASS: foreign FBX -> %s: %d source bones, %zu sections, %zu/%zu triangles, authored weights retained; %zu isolated gaps repaired, %zu points reduced to four influences\n",
                    target->name.c_str(),ref.nbones,sections,emitted,triangles,repairedPoints,reduced);
        return 0;
    } catch (const std::exception &error) {
        std::fprintf(stderr, "FAIL: %s\n", error.what());
        return 1;
    }
}
