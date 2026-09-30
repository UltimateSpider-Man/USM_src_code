#pragma once

// Shared by runtime and host probes: collect the authored FBX skeleton and
// apply exactly the same uniform/user transform to its pivots and geometry.
#include "mod_mesh_import.h"
#include "mod_mesh_foreign_rig.h"

namespace modmesh { namespace foreignrig {

inline bool hasAuthoredSkin(const Scene &scene)
{
    for (const auto &entry : scene.geoms)
        for (const auto &cluster : entry.second.clusters)
            for (double weight : cluster.w) if (weight > 0.0) return true;
    return false;
}

inline bool prepareScene(const Scene &input,
                         const std::vector<std::string> &targetNames,
                         const std::vector<rt::Matrix> &targetBind,
                         Scene &prepared, OrigMeshRef &reference,
                         rt::Plan &out, std::string *why = nullptr)
{
    auto fail = [&](const std::string &message) { return rt::fail(why, message.c_str()); };
    if (!input.cfg.retarget) return fail("authored foreign skin needs retarget=on or explicit skin=transfer/rigid");
    if (input.cfg.skin == 2 || input.cfg.skin == 3)
        return fail("explicit donor/rigid skin does not use authored foreign rigging");
    if (!input.cfg.donorPose.empty())
        return fail("donor_pose is for unrigged meshes; disable it to retain authored foreign skin");
    if (!std::isfinite(input.cfg.scale) || input.cfg.scale <= 0
        || !std::isfinite(input.cfg.yaw) || !std::isfinite(input.cfg.offset.x)
        || !std::isfinite(input.cfg.offset.y) || !std::isfinite(input.cfg.offset.z))
        return fail("foreign rig needs a finite positive uniform model scale");
    std::map<int64_t, size_t> jointIndex;
    std::vector<int64_t> modelIds;
    std::map<int64_t, V3> clusterPivots;
    std::map<int64_t, std::string> clusterModels;
    struct Repair {
        size_t vertex;
        std::vector<std::pair<size_t,double>> influences; // existing source cluster indices
    };
    std::map<int64_t, std::vector<Repair>> repairsOfGeometry;
    size_t weightedClusters = 0;
    for (int64_t meshId : input.meshModelOrder) {
        const auto mesh = input.models.find(meshId);
        if (mesh == input.models.end()) return fail("foreign mesh model is missing");
        for (int64_t geometryId : mesh->second.geoms) {
            const auto found = input.geoms.find(geometryId);
            if (found == input.geoms.end()) return fail("foreign geometry is missing");
            const auto &geometry = found->second;
            if (geometry.ctrl.empty() || geometry.pvi.empty()) continue;
            const M4 modelWorld = detail::nodeGlobal(input, mesh->second);
            std::vector<double> pointWeights(geometry.ctrl.size()/3, 0.0);
            std::set<int64_t> localClusters;
            for (const auto &cluster : geometry.clusters) {
                if (!cluster.boneModelId || cluster.boneName.empty()
                    || !input.models.count(cluster.boneModelId))
                    return fail("foreign skin cluster has no connected bone model");
                if (!localClusters.insert(cluster.boneModelId).second)
                    return fail("duplicate foreign skin cluster in one geometry");
                clusterModels.emplace(cluster.boneModelId, "foreign_model_" + std::to_string(cluster.boneModelId));
                if (cluster.idx.size() != cluster.w.size()) return fail("foreign skin index/weight counts differ");
                bool weighted = false;
                for (size_t k = 0; k < cluster.w.size(); ++k) {
                    const double weight = cluster.w[k];
                    if (!std::isfinite(weight) || weight < 0 || cluster.idx[k] < 0
                        || size_t(cluster.idx[k]) >= pointWeights.size())
                        return fail("invalid authored foreign skin influence");
                    pointWeights[size_t(cluster.idx[k])] += weight;
                    weighted = weighted || weight > 0;
                }
                if (weighted) {
                    ++weightedClusters;
                    if (!cluster.haveLink) return fail("weighted foreign cluster has no bind-pose TransformLink");
                    if (!cluster.haveTransform) return fail("weighted foreign cluster has no mesh bind Transform");
                    // Serialized FBX Transform is mesh-local -> bone-local,
                    // despite the SDK exposing its reconstructed global form.
                    // In row convention, meshWorld = Transform * TransformLink.
                    // Only TransformLink receives global axis conversion.
                    // Blender's official export_fbx_bin.py writes exactly this
                    // bone-relative matrix (fbx_data_armature_elements).
                    for (int row = 0; row < 4; ++row) for (int column = 0; column < 3; ++column) {
                        double bind = 0;
                        for (int k = 0; k < 4; ++k)
                            bind += cluster.transformMatrix[size_t(row*4+k)] * cluster.linkMatrix[size_t(k*4+column)];
                        bind *= input.sceneScale;
                        const double node = modelWorld.m[column*4+row];
                        if (!std::isfinite(bind) || !std::isfinite(node)
                            || std::abs(bind-node) > 1e-4*std::max({1.0,std::abs(bind),std::abs(node)}))
                            return fail("foreign mesh node differs from its skin bind Transform; export the mesh in bind pose");
                    }
                }
                if (cluster.haveLink) {
                    const V3 pivot{cluster.linkPos[0]*input.sceneScale,
                                   cluster.linkPos[1]*input.sceneScale,
                                   cluster.linkPos[2]*input.sceneScale};
                    if (!std::isfinite(pivot.x) || !std::isfinite(pivot.y) || !std::isfinite(pivot.z))
                        return fail("nonfinite foreign cluster bind position");
                    const auto inserted = clusterPivots.emplace(cluster.boneModelId, pivot);
                    if (!inserted.second) {
                        const V3 &other = inserted.first->second;
                        if (std::abs(pivot.x-other.x) > 1e-5 || std::abs(pivot.y-other.y) > 1e-5
                            || std::abs(pivot.z-other.z) > 1e-5)
                            return fail("one foreign joint has inconsistent mesh bind poses");
                    }
                }
                // Source palette follows first cluster encounter, never a
                // suffix interpreted as a native skeleton index.
                int64_t node = cluster.boneModelId;
                std::set<int64_t> ancestors;
                while (node) {
                    if (!ancestors.insert(node).second) return fail("foreign FBX bone parent cycle");
                    const auto model = input.models.find(node);
                    if (model == input.models.end()) return fail("foreign bone ancestor model is missing");
                    if (jointIndex.emplace(node, modelIds.size()).second) modelIds.push_back(node);
                    if (modelIds.size() > 1024) return fail("foreign source exceeds 1024 render bones");
                    node = model->second.parent;
                }
            }
            std::set<size_t> unweighted;
            for (int64_t encoded : geometry.pvi) {
                const int64_t vertex = encoded < 0 ? ~encoded : encoded;
                if (vertex < 0 || size_t(vertex) >= pointWeights.size()
                    || !std::isfinite(pointWeights[size_t(vertex)]))
                    return fail("foreign geometry has invalid vertex indices or total skin weights");
                if (pointWeights[size_t(vertex)] == 0.0) unweighted.insert(size_t(vertex));
            }
            if (!unweighted.empty()) {
                // Only ORIGINAL authored weights are seeds. A repaired point
                // never becomes a seed for another point, so an unweighted
                // island cannot acquire an invented rig by repeated spreading.
                std::map<size_t,std::set<size_t>> neighbors;
                for (size_t vertex : unweighted) neighbors.emplace(vertex,std::set<size_t>{});
                std::vector<size_t> polygon;
                for (int64_t encoded : geometry.pvi) {
                    polygon.push_back(size_t(encoded < 0 ? ~encoded : encoded));
                    if (encoded >= 0) continue;
                    for (size_t k = 1; k+1 < polygon.size(); ++k) {
                        const size_t triangle[] = {polygon[0],polygon[k],polygon[k+1]};
                        for (size_t a : triangle) if (unweighted.count(a))
                            for (size_t b : triangle) if (b != a && pointWeights[b] > 0.0)
                                neighbors[a].insert(b);
                    }
                    polygon.clear();
                }
                std::map<size_t,std::vector<std::pair<size_t,double>>> authored;
                for (const auto &entry : neighbors) {
                    if (entry.second.empty())
                        return fail("foreign geometry " + Scene::normName(geometry.name)
                            + " has an unweighted vertex without an authored triangle-edge neighbor; "
                              "supply source weights or explicitly choose skin=transfer");
                    for (size_t vertex : entry.second) authored.emplace(vertex,std::vector<std::pair<size_t,double>>{});
                }
                for (size_t cluster = 0; cluster < geometry.clusters.size(); ++cluster) {
                    const auto &weights = geometry.clusters[cluster];
                    for (size_t k = 0; k < weights.idx.size(); ++k) {
                        const size_t vertex = size_t(weights.idx[k]);
                        const auto seed = authored.find(vertex);
                        if (seed != authored.end() && weights.w[k] > 0.0)
                            seed->second.push_back({cluster,weights.w[k]/pointWeights[vertex]});
                    }
                }
                std::vector<Repair> repairs;
                for (const auto &entry : neighbors) {
                    std::map<size_t,double> average;
                    for (size_t neighbor : entry.second)
                        for (const auto &weight : authored.at(neighbor))
                            average[weight.first] += weight.second/double(entry.second.size());
                    Repair repaired{entry.first,{average.begin(),average.end()}};
                    std::sort(repaired.influences.begin(),repaired.influences.end(),[](const auto &a,const auto &b) {
                        return a.second == b.second ? a.first < b.first : a.second > b.second;
                    });
                    if (repaired.influences.size() > 4) repaired.influences.resize(4);
                    double sum = 0;
                    for (const auto &weight : repaired.influences) sum += weight.second;
                    if (!std::isfinite(sum) || sum <= 0.0) return fail("invalid adjacent authored skin weights");
                    for (auto &weight : repaired.influences) weight.second /= sum;
                    repairs.push_back(std::move(repaired));
                }
                repairsOfGeometry[geometryId] = std::move(repairs);
            }
        }
    }
    if (!weightedClusters || modelIds.empty()) return fail("foreign FBX contains no authored weighted skeleton");

    // Keep unweighted end joints too: a hand/foot can supply a valid limb
    // guide while its mesh vertices are weighted to the preceding joint.
    for (const auto &entry : input.models) {
        if (entry.second.type != "LimbNode" || jointIndex.count(entry.first)) continue;
        std::vector<int64_t> path;
        std::set<int64_t> visited;
        int64_t node = entry.first;
        while (node && !jointIndex.count(node)) {
            if (!visited.insert(node).second) return fail("foreign FBX end-joint parent cycle");
            const auto found = input.models.find(node);
            if (found == input.models.end()) return fail("foreign FBX end-joint ancestor is missing");
            path.push_back(node);
            node = found->second.parent;
        }
        if (!node) continue; // unrelated skeleton/scene objects are not this skin
        for (int64_t id : path) {
            if (jointIndex.emplace(id, modelIds.size()).second) modelIds.push_back(id);
            if (modelIds.size() > 1024) return fail("foreign source exceeds 1024 render bones");
        }
    }

    const M4 user = M4::translate(input.cfg.offset)
                  * M4::scale({input.cfg.scale, input.cfg.scale, input.cfg.scale})
                  * eulerDeg({0, input.cfg.yaw, 0}, 0);
    std::vector<Joint> joints;
    for (int64_t id : modelIds) {
        const auto &model = input.models.at(id);
        const auto bind = clusterPivots.find(id);
        V3 pivot = bind == clusterPivots.end()
            ? detail::nodeGlobal(input, model).point({0,0,0}) : bind->second;
        pivot = user.point(pivot);
        Joint joint;
        joint.name = jointName(model.name);
        joint.parent = model.parent ? int(jointIndex.at(model.parent)) : -1;
        joint.position = {{float(pivot.x),float(pivot.y),float(pivot.z)}};
        joints.push_back(std::move(joint));
    }
    rt::Plan plan;
    if (!prepare(joints, targetNames, targetBind, input.cfg.boneMap, plan, why)) return false;
    double fitScale = 1.0;
    V3 shift{};
    if (input.cfg.fit) {
        double sourceLow = 1e30, sourceHigh = -1e30, targetLow = 1e30, targetHigh = -1e30;
        for (size_t j = 0; j < joints.size(); ++j) if (plan.targetBone[j] >= 0) {
            sourceLow = std::min(sourceLow, double(joints[j].position[1]));
            sourceHigh = std::max(sourceHigh, double(joints[j].position[1]));
            targetLow = std::min(targetLow, double(targetBind[size_t(plan.targetBone[j])][13]));
            targetHigh = std::max(targetHigh, double(targetBind[size_t(plan.targetBone[j])][13]));
        }
        if (sourceHigh-sourceLow <= 1e-6 || targetHigh-targetLow <= 1e-6)
            return fail("foreign/native skeleton has no usable standing height");
        const double ratio = (targetHigh-targetLow)/(sourceHigh-sourceLow);
        if (!std::isfinite(ratio) || ratio < 1e-6 || ratio > 1e6)
            return fail("foreign rig fit scale is outside supported range");
        if (ratio < .8 || ratio > 1.25) fitScale = ratio;
        size_t pelvis = 0;
        for (; pelvis < joints.size(); ++pelvis)
            if (plan.targetBone[pelvis] == plan.rootDriver && plan.parent[pelvis] < 0) break;
        if (pelvis == joints.size()) return fail("foreign rig has no mapped render root");
        const auto &target = targetBind[size_t(plan.targetBone[pelvis])];
        shift = {target[12]-fitScale*joints[pelvis].position[0],
                 target[13]-fitScale*joints[pelvis].position[1],
                 target[14]-fitScale*joints[pelvis].position[2]};
        for (auto &joint : joints) {
            joint.position[0] = float(fitScale*joint.position[0]+shift.x);
            joint.position[1] = float(fitScale*joint.position[1]+shift.y);
            joint.position[2] = float(fitScale*joint.position[2]+shift.z);
        }
        if (!prepare(joints, targetNames, targetBind, input.cfg.boneMap, plan, why)) return false;
    }

    Scene scene = input;
    for (const auto &entry : repairsOfGeometry) {
        auto &geometry = scene.geoms.at(entry.first);
        for (const auto &repair : entry.second)
            for (const auto &weight : repair.influences) {
                auto &cluster = geometry.clusters.at(weight.first);
                cluster.idx.push_back(int64_t(repair.vertex));
                cluster.w.push_back(weight.second);
            }
        logf("[modmesh] foreign geometry %s: repaired %u isolated unweighted vertices from adjacent authored source weights",
             Scene::normName(geometry.name).c_str(),unsigned(entry.second.size()));
    }
    for (auto &geometry : scene.geoms)
        for (auto &cluster : geometry.second.clusters) {
            const auto found = clusterModels.find(cluster.boneModelId);
            if (found != clusterModels.end()) cluster.boneName = found->second;
        }
    scene.cfg.scale *= fitScale;
    scene.cfg.offset = {input.cfg.offset.x*fitScale+shift.x,
                        input.cfg.offset.y*fitScale+shift.y,
                        input.cfg.offset.z*fitScale+shift.z};
    scene.cfg.fit = false;
    scene.cfg.skin = input.cfg.skin == 4 ? 4 : 1;
    scene.cfg.custom = true;
    scene.cfg.anim = false;
    scene.anims.clear();
    OrigMeshRef ref = reference;
    ref.clusterBoneIndices.clear();
    for (const auto &entry : clusterModels) ref.clusterBoneIndices[entry.second] = int(jointIndex.at(entry.first));
    ref.boneNames.clear(); ref.boneParents = plan.parent; ref.bonePos.clear();
    for (const auto &joint : joints) {
        ref.boneNames.push_back(joint.name);
        ref.bonePos.insert(ref.bonePos.end(), joint.position.begin(), joint.position.end());
    }
    ref.nbones = int(joints.size()); ref.haveBones = true;
    ref.customSource = true; ref.rejectImportedSkin = false; ref.donorPoseMetadataValidated = false;
    for (int d = 0; d < 3; ++d) {
        ref.bonesMin[d] = ref.bonesMax[d] = ref.bonePos[size_t(d)];
        for (size_t j = 1; j < joints.size(); ++j) {
            ref.bonesMin[d] = std::min(ref.bonesMin[d], ref.bonePos[j*3+size_t(d)]);
            ref.bonesMax[d] = std::max(ref.bonesMax[d], ref.bonePos[j*3+size_t(d)]);
        }
    }
    prepared = std::move(scene);
    reference = std::move(ref);
    out = std::move(plan);
    if (why) why->clear();
    return true;
}

}} // namespace modmesh::foreignrig
