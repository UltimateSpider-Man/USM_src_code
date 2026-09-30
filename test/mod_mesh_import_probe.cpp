#include "../src/mod_mesh_import.h"

#include <algorithm>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace {

void logLine(const char *message)
{
    std::fprintf(stderr, "%s\n", message != nullptr ? message : "");
}

} // namespace

int main(int argc, char **argv)
{
    if (argc != 2) {
        std::fprintf(stderr, "usage: mod_mesh_import_probe <mesh.fbx|mesh.obj>\n");
        return 2;
    }

    std::ifstream file(argv[1], std::ios::binary);
    std::vector<unsigned char> bytes((std::istreambuf_iterator<char>(file)),
                                     std::istreambuf_iterator<char>());
    if (bytes.empty()) {
        std::fprintf(stderr, "read failed: %s\n", argv[1]);
        return 1;
    }

    modmesh::setLog(logLine);
    const auto scene = modmesh::loadScene(argv[1], bytes.data(), bytes.size());
    if (!scene) {
        std::fprintf(stderr, "parse failed: %s\n", argv[1]);
        return 1;
    }

    std::size_t controlPoints = 0;
    std::size_t polygonCorners = 0;
    std::size_t clusters = 0;
    std::size_t weightedPoints = 0;
    std::set<std::string> boneNames;

    bool haveBounds = false;
    double lo[3] = {0.0, 0.0, 0.0};
    double hi[3] = {0.0, 0.0, 0.0};

    for (const auto &entry : scene->geoms) {
        const modmesh::Geom &geom = entry.second;
        controlPoints += geom.ctrl.size() / 3;
        polygonCorners += geom.pvi.size();
        clusters += geom.clusters.size();

        std::set<std::int64_t> weighted;
        for (const modmesh::GCluster &cluster : geom.clusters) {
            boneNames.insert(cluster.boneName);
            weighted.insert(cluster.idx.begin(), cluster.idx.end());
            std::printf("cluster name=%s points=%zu link=%d pos=%.9g,%.9g,%.9g\n",
                        cluster.boneName.c_str(), cluster.idx.size(),
                        cluster.haveLink ? 1 : 0, cluster.linkPos[0],
                        cluster.linkPos[1], cluster.linkPos[2]);
        }
        weightedPoints += weighted.size();

        std::map<int, std::set<std::int64_t>> slotPoints;
        std::map<int, std::size_t> slotTriangles;
        std::vector<std::int64_t> polygon;
        std::size_t polygonIndex = 0;
        for (const std::int64_t encoded : geom.pvi) {
            const bool last = encoded < 0;
            polygon.push_back(last ? ~encoded : encoded);
            if (!last)
                continue;
            int slot = 0;
            if (geom.matMapping == "ByPolygon"
                && polygonIndex < geom.matIdx.size()) {
                slot = static_cast<int>(geom.matIdx[polygonIndex]);
            } else if (!geom.matIdx.empty()) {
                slot = static_cast<int>(geom.matIdx.front());
            }
            if (slot < 0)
                slot = 0;
            slotPoints[slot].insert(polygon.begin(), polygon.end());
            if (polygon.size() >= 3)
                slotTriangles[slot] += polygon.size() - 2;
            polygon.clear();
            ++polygonIndex;
        }

        for (const auto &slot : slotPoints) {
            std::set<std::string> slotBones;
            std::size_t influencedPoints = 0;
            for (const modmesh::GCluster &cluster : geom.clusters) {
                bool used = false;
                for (std::size_t i = 0; i < cluster.idx.size(); ++i) {
                    if (i < cluster.w.size() && cluster.w[i] > 0.0
                        && slot.second.count(cluster.idx[i]) != 0) {
                        used = true;
                        ++influencedPoints;
                    }
                }
                if (used)
                    slotBones.insert(cluster.boneName);
            }
            std::printf("slot index=%d triangles=%zu control_points=%zu "
                        "bones=%zu weighted_memberships=%zu names=",
                        slot.first, slotTriangles[slot.first],
                        slot.second.size(), slotBones.size(), influencedPoints);
            bool first = true;
            for (const std::string &bone : slotBones) {
                std::printf("%s%s", first ? "" : ",", bone.c_str());
                first = false;
            }
            std::printf("\n");
        }

        for (std::size_t i = 0; i + 2 < geom.ctrl.size(); i += 3) {
            const double p[3] = {geom.ctrl[i], geom.ctrl[i + 1], geom.ctrl[i + 2]};
            if (!haveBounds) {
                std::copy(p, p + 3, lo);
                std::copy(p, p + 3, hi);
                haveBounds = true;
            } else {
                for (int axis = 0; axis < 3; ++axis) {
                    lo[axis] = std::min(lo[axis], p[axis]);
                    hi[axis] = std::max(hi[axis], p[axis]);
                }
            }
        }
    }

    std::printf("scene source=%s models=%zu mesh_models=%zu geoms=%zu "
                "materials=%zu control_points=%zu polygon_corners=%zu "
                "clusters=%zu bones=%zu weighted_points=%zu unit_scale=%.9g "
                "scene_scale=%.9g animations=%zu\n",
                scene->srcName.c_str(), scene->models.size(),
                scene->meshModelOrder.size(), scene->geoms.size(),
                scene->materials.size(), controlPoints, polygonCorners,
                clusters, boneNames.size(), weightedPoints, scene->unitScale,
                scene->sceneScale, scene->anims.size());
    if (haveBounds) {
        std::printf("control_bounds min=%.9g,%.9g,%.9g max=%.9g,%.9g,%.9g\n",
                    lo[0], lo[1], lo[2], hi[0], hi[1], hi[2]);
    }
    for (const auto &entry : scene->materials) {
        const auto texture = scene->matTexStem.find(entry.first);
        std::printf("material id=%lld name=%s texture=%s\n",
                    static_cast<long long>(entry.first), entry.second.c_str(),
                    texture != scene->matTexStem.end() ? texture->second.c_str() : "");
    }

    for (const auto id : scene->meshModelOrder) {
        const auto found = scene->models.find(id);
        if (found == scene->models.end())
            continue;
        const auto &model = found->second;
        std::printf("mesh_model id=%lld name=%s parent=%lld "
                    "T=%.9g,%.9g,%.9g R=%.9g,%.9g,%.9g "
                    "S=%.9g,%.9g,%.9g geoT=%.9g,%.9g,%.9g "
                    "geoR=%.9g,%.9g,%.9g geoS=%.9g,%.9g,%.9g materials=%zu\n",
                    static_cast<long long>(model.id), model.name.c_str(),
                    static_cast<long long>(model.parent),
                    model.T.x, model.T.y, model.T.z,
                    model.R.x, model.R.y, model.R.z,
                    model.S.x, model.S.y, model.S.z,
                    model.geoT.x, model.geoT.y, model.geoT.z,
                    model.geoR.x, model.geoR.y, model.geoR.z,
                    model.geoS.x, model.geoS.y, model.geoS.z,
                    model.materials.size());
    }

    return 0;
}
