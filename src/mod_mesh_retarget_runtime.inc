// Native skeleton metadata supplies output order and hierarchy. The gameplay
// skeleton stays native; imported palettes and the draw-only pose use source
// order so extra source helpers do not alias unrelated target bones.
#include "mod_mesh_attachment_geometry.h"
#include "mod_mesh_foreign_import.h"
struct ModMeshRetarget {
    modmesh::retarget::Plan plan;
    modmesh::attachment::Geometry attachmentGeometry;
    bool attachmentGeometryReady = false;
    bool customSkin = false; // validated posed-scan skin; private FBX materials
    nglMeshFile *owner = nullptr;
    std::string targetMeshName;
    std::vector<math::MatClass<4, 3>> sourceInverse;
    std::vector<std::string> sourceNames;
    bool logged = false, warned = false;
};
static std::map<nglMesh *, std::shared_ptr<ModMeshRetarget>> modMeshRetargets;
static std::map<nglMaterialBase *, std::map<std::string, std::shared_ptr<ModMeshRetarget>>>
    modMaterialRetargets;

static std::string modRetargetBoneKey(std::string name)
{
    name = modmesh::Scene::normName(name);
    name.erase(std::remove_if(name.begin(), name.end(),
                             [](char c) { return c == ' ' || c == '_'; }), name.end());
    if (name.compare(0, 5, "bip01") == 0) name.erase(0, 5);
    if (name == "lforetwist" || name == "rforetwist") name += '0';
    if (name == "ltoe0" || name == "rtoe0") name.pop_back();
    return name;
}

static std::shared_ptr<modmesh::pcmeshsource::Source>
modNativeMeshSource(const std::string &family, const std::filesystem::path &hint)
{
    static std::map<std::string, std::shared_ptr<modmesh::pcmeshsource::Source>> cache;
    const std::string key = transformToLower(family) + "|" + hint.string();
    if (auto found = cache.find(key); found != cache.end()) return found->second;
    std::vector<uint8_t> bytes;
    auto source = std::make_shared<modmesh::pcmeshsource::Source>();
    std::string why;
    if (!modReadNativePcmesh(family, hint, bytes) || !source->parse(std::move(bytes), &why))
        return nullptr;
    cache.emplace(key, source);
    return source;
}

static bool modNativeBindMatches(const nglMesh *mesh, const modmesh::pcmeshsource::Mesh *native)
{
    if (!mesh || !native || !mesh->Name || !mesh->Bones || mesh->NBones <= 0
        || mesh->NBones > 1024 || std::size_t(mesh->NBones) != native->nbones
        || native->boneMatrices.size() != native->nbones * 16u
        || modmesh::Scene::normName(mesh->Name->to_string()) != modmesh::Scene::normName(native->name))
        return false;
    // Called before the native loader inverts bind matrices. Compare every
    // meaningful lane, including rotation, so a reused name/count alone never
    // substitutes another character's skeleton metadata. W lanes are padding.
    for (int bone = 0; bone < mesh->NBones; ++bone) {
        const auto *live = reinterpret_cast<const float *>(&mesh->Bones[bone]);
        for (int lane = 0; lane < 16; ++lane) {
            if (lane % 4 == 3) continue;
            const float expected = native->boneMatrices[std::size_t(bone) * 16 + lane];
            if (!std::isfinite(live[lane])
                || std::abs(live[lane] - expected) > 1e-4f * std::max(1.f, std::abs(expected)))
                return false;
        }
    }
    return true;
}

// The selected file/hash remains the FBX target. Some character packs expose
// that resource under a different hero name while retaining another native
// body's mesh and controller. Resolve only its render metadata from a verified
// file-primary bind signature, then allow that body's own meshes/linked LODs.
static std::string modTargetMeshMetadata(Mod *mod, nglMesh *mesh, modmesh::OrigMeshRef &ref)
{
    const auto logical = ref.targetFileName;
    if (!mod || !mesh || !mesh->Name) return logical;
    const auto hint = mod->Path.parent_path();
    if (const auto native = modNativeMeshSource(logical, hint)) {
        ref.targetMeshNames = native->replacementMeshNames(logical);
        for (const auto &part : native->meshes) ref.targetFileMeshNames.push_back(part.name);
    }
    const auto currentName = modmesh::Scene::normName(mesh->Name->to_string());
    const auto contains = [&](const std::vector<std::string> &names) {
        return std::any_of(names.begin(), names.end(), [&](const std::string &name) {
            return modmesh::Scene::normName(name) == currentName;
        });
    };
    auto *primary = mesh->File ? mesh->File->FirstMesh : nullptr;
    if (!primary || !primary->Name) return logical;
    std::string actual = modmesh::Scene::normName(primary->Name->to_string());
    // Preserve digits belonging to the family: foo2000 is the LOD of foo2.
    if (actual.size() > 3 && std::all_of(actual.end() - 3, actual.end(),
            [](unsigned char c) { return c >= '0' && c <= '9'; })) actual.resize(actual.size() - 3);
    if (actual.empty() || actual == modmesh::Scene::normName(logical)) return logical;
    const auto native = modNativeMeshSource(actual, hint);
    if (!native) return logical;
    const auto *nativePrimary = native->findMesh(primary->Name->to_string());
    if (!modNativeBindMatches(primary, nativePrimary)) return logical;
    // This fallback is for character packs. An unrelated first rigid/FX mesh
    // cannot turn a character filename into a broad replacement permission.
    for (const auto &section : nativePrimary->sections) {
        const auto *material = native->findMaterial(section.materialOffset);
        if (!material || !material->characterBindable) return logical;
    }
    auto names = native->replacementMeshNames(actual);
    if (!contains(names) || !modNativeBindMatches(mesh, native->findMesh(currentName))) return logical;
    ref.targetMeshNames = std::move(names);
    sp_log("[modmesh] target metadata %s -> %s for %s: verified native bind (%d bones)",
           logical.c_str(), actual.c_str(), currentName.c_str(), mesh->NBones);
    return actual;
}

static std::shared_ptr<modmesh::pcskelsource::Source>
modNativeSkeleton(const std::string &name, const std::filesystem::path &hint)
{
    static std::map<std::string, std::shared_ptr<modmesh::pcskelsource::Source>> cache;
    std::filesystem::path path;
    if (!modResolveFile(name, path, hint)) {
        // Metadata kept outside mods is not registered as a wholesale native
        // skeleton override by enumerate_mods()/modScanNalOverrides().
        const auto wanted = transformToLower(std::filesystem::path(name).filename().string());
        for (const auto &root : modRootDirs()) {
            const auto directory = root.parent_path() / "mesh_sources" / "pcskel";
            std::error_code ec;
            for (std::filesystem::directory_iterator it(directory, ec), end;
                 it != end && !ec; it.increment(ec)) {
                if (it->is_regular_file(ec)
                    && transformToLower(it->path().filename().string()) == wanted) {
                    path = it->path(); break;
                }
            }
            if (!path.empty()) break;
        }
    }
    if (path.empty()) return nullptr;
    const auto key = path.string();
    if (auto found = cache.find(key); found != cache.end()) return found->second;
    std::vector<uint8_t> bytes;
    auto source = std::make_shared<modmesh::pcskelsource::Source>();
    std::string why;
    if (!modReadWholeFile(path, bytes) || !source->parse(std::move(bytes), &why)) {
        sp_log("[modmesh] skeleton metadata unavailable: %s: %s", key.c_str(), why.c_str());
        return nullptr;
    }
    cache.emplace(key, source);
    return source;
}

static modmesh::retarget::Matrix modRetargetMatrix(const float *native)
{
    modmesh::retarget::Matrix result;
    std::copy(native, native + 16, result.begin());
    // MatClass<4,3> has four padded rows; the fourth lanes carry no affine data.
    result[3] = result[7] = result[11] = 0.f;
    result[15] = 1.f;
    return result;
}

static bool modPrepareForeignRig(Mod *mod, nglMesh *mesh,
                                  std::shared_ptr<modmesh::Scene> &scene,
                                  modmesh::OrigMeshRef &ref,
                                  const std::string &metadataFamily,
                                  bool &recognized)
{
    recognized = scene && mesh && mesh->NBones > 0 && ref.customSource
        && scene->cfg.skin != 2 && scene->cfg.skin != 3
        && modmesh::foreignrig::hasAuthoredSkin(*scene);
    if (!recognized) return false;
    ref.rejectImportedSkin = true;
    const auto reject = [&](const std::string &why) {
        sp_log("[modmesh] authored foreign rig rejected: %s; native mesh retained. "
               "Map humanoid joints with bone.<source>=<native> or explicitly choose skin=transfer/rigid",
               why.c_str());
        return false;
    };
    if (!mod || !mesh || !mesh->File || !mesh->Name || !mesh->Bones
        || mesh->NBones <= 0 || mesh->NBones > 1024)
        return reject("missing native skinned target");
    if (!scene->cfg.retarget) return reject("retarget=off cannot animate authored foreign bone palettes");
    const auto hint = mod->Path.parent_path();
    const auto family = metadataFamily.empty() ? ref.targetFileName : metadataFamily;
    const auto native = modNativeMeshSource(family, hint);
    const auto *nativeMesh = native ? native->findMesh(mesh->Name->to_string()) : nullptr;
    if (!modNativeBindMatches(mesh, nativeMesh))
        return reject("native PCMESH bind matrices could not be verified");
    auto names = modmesh::nativeBoneNames(mesh->Name->to_string(), mesh->NBones, ref.bonePos);
    if (names.size() != size_t(mesh->NBones)) {
        const auto skeleton = modNativeSkeleton(family + ".PCSKEL", hint);
        if (!skeleton || skeleton->bones.size() < size_t(mesh->NBones))
            return reject("native PCSKEL output names/order are unavailable");
        names.clear();
        for (int bone = 0; bone < mesh->NBones; ++bone) names.push_back(skeleton->bones[size_t(bone)].name);
    }
    std::vector<modmesh::retarget::Matrix> targetBind;
    for (int bone = 0; bone < mesh->NBones; ++bone)
        targetBind.push_back(modRetargetMatrix(reinterpret_cast<const float *>(&mesh->Bones[bone])));
    auto rig = std::make_shared<ModMeshRetarget>();
    auto prepared = std::make_shared<modmesh::Scene>();
    std::string why;
    if (!modmesh::foreignrig::prepareScene(*scene, names, targetBind, *prepared, ref, rig->plan, &why))
        return reject(why);
    rig->customSkin = true;
    rig->owner = mesh->File;
    rig->targetMeshName = modmesh::Scene::normName(mesh->Name->to_string());
    rig->sourceNames = ref.boneNames;
    rig->sourceInverse.resize(rig->plan.sourceInverse.size());
    size_t mapped = 0;
    for (size_t bone = 0; bone < rig->sourceInverse.size(); ++bone) {
        std::memcpy(&rig->sourceInverse[bone], rig->plan.sourceInverse[bone].data(), 64);
        if (rig->plan.targetBone[bone] >= 0) {
            ++mapped;
            // Draw support alignment selects feet semantically. An explicit
            // source spelling can be arbitrary, so expose its mapped native
            // label here while ref/cluster palettes retain source identities.
            rig->sourceNames[bone] = names[size_t(rig->plan.targetBone[bone])];
        }
    }
    scene = std::move(prepared);
    modMeshRetargets[mesh] = rig;
    modMeshBufferOwners[mesh->File->FileBuf.Buf] = mesh->File;
    sp_log("[modmesh] authored foreign humanoid: %u source bones, %u native mappings, %u attached helpers; "
           "original cluster weights, native animations, source limb lengths",
           unsigned(rig->sourceNames.size()), unsigned(mapped), unsigned(rig->sourceNames.size()-mapped));
    return true;
}

// The supplied native FBX exports carry transformed PCMESH bind bases. Keep
// all twelve affine lanes: forearm and twist bones can share a position, and
// generic exporters can label a cluster with a name-table ordinal instead of
// its actual output-pose bone. A unique full bind identifies the skin palette.
static int modSourceClusterBindIndex(const modmesh::GCluster &cluster,
                                     double sceneScale,
                                     const modmesh::pcmeshsource::Mesh &mesh)
{
    if (!cluster.haveLink || !std::isfinite(sceneScale) || sceneScale <= 0.0
        || mesh.boneMatrices.size() != std::size_t(mesh.nbones) * 16u) return -1;
    const int rows[] = {1, 2, 0};
    const double signs[] = {-1.0, 1.0, -1.0};
    double link[12];
    for (int r = 0; r < 3; ++r)
        for (int c = 0; c < 3; ++c)
            link[r * 3 + c] = cluster.linkMatrix[rows[r] * 4 + c] * signs[r] * sceneScale;
    for (int c = 0; c < 3; ++c) link[9 + c] = cluster.linkPos[c] * sceneScale;
    for (double value : link) if (!std::isfinite(value)) return -1;
    int match = -1;
    for (uint32_t bone = 0; bone < mesh.nbones; ++bone) {
        double error = 0.0;
        for (int r = 0; r < 4; ++r)
            for (int c = 0; c < 3; ++c)
                error = std::max(error, std::abs(link[r * 3 + c]
                    - mesh.boneMatrices[std::size_t(bone) * 16 + r * 4 + c]));
        if (error > 5e-5) continue;
        if (match >= 0) return -1; // Ambiguous binds never select a name guess.
        match = int(bone);
    }
    return match;
}

static bool modPrepareMeshRetarget(Mod *mod, nglMesh *mesh, const modmesh::Scene &scene,
                                   modmesh::OrigMeshRef &ref,
                                   const std::string &targetMetadataFamily = {})
{
    if (!mod || !mesh || !mesh->File || mesh->NBones <= 0 || !mesh->Bones) return false;
    const auto hint = mod->Path.parent_path();
    const auto targetSkeleton = modNativeSkeleton((targetMetadataFamily.empty()
        ? ref.targetFileName : targetMetadataFamily) + ".PCSKEL", hint);
    if (!targetSkeleton || targetSkeleton->bones.size() < size_t(mesh->NBones)) return false;
    ref.boneNames.clear();
    for (int i = 0; i < mesh->NBones; ++i) ref.boneNames.push_back(targetSkeleton->bones[i].name);
    if (!scene.cfg.donorPose.empty()) {
        const auto native = modNativeMeshSource(targetMetadataFamily.empty()
            ? ref.targetFileName : targetMetadataFamily, hint);
        const auto *nativeMesh = native && mesh->Name ? native->findMesh(mesh->Name->to_string()) : nullptr;
        if (modNativeBindMatches(mesh, nativeMesh)) {
            ref.boneParents.clear();
            for (int i = 0; i < mesh->NBones; ++i) ref.boneParents.push_back(targetSkeleton->bones[i].parent);
            ref.donorPoseMetadataValidated = true;
        } else sp_log("[modmesh] donor_pose metadata rejected: native PCMESH bind differs from loaded mesh");
        // An explicit pose helper always retains target palette order. The
        // custom builder separately rejects authored skins and bad settings.
        return false;
    }
    if (!scene.cfg.retarget) return false;
    const std::string primary = modmesh::primaryMeshName(scene);
    const auto source = modNativeMeshSource(modNativeFamily(primary), hint);
    const auto *sourceMesh = source ? source->findMesh(primary) : nullptr;
    if (!sourceMesh || !sourceMesh->nbones || sourceMesh->nbones > 1024) return false;
    const auto sourceSkeleton = modNativeSkeleton(scene.cfg.sourceSkeleton.empty()
        ? modNativeFamily(primary) + ".PCSKEL" : scene.cfg.sourceSkeleton, hint);
    if (!sourceSkeleton || sourceSkeleton->bones.size() < sourceMesh->nbones) return false;
    const bool genericSource = sourceSkeleton->version == 0x10200u;
    // Generic name tables need a verified output/bind association. A rejected
    // association must retain native geometry instead of entering the legacy
    // name-based import path with the same misleading exporter labels.
    ref.rejectImportedSkin = genericSource;
    for (const auto &section : sourceMesh->sections) {
        const auto *material = source->findMaterial(section.materialOffset);
        if (!material || !material->characterBindable) return false;
    }
    auto rig = std::make_shared<ModMeshRetarget>();
    rig->owner = mesh->File;
    rig->targetMeshName = mesh->Name ? modmesh::Scene::normName(mesh->Name->to_string()) : "";
    std::map<std::string, int> sourceIndices, targetIndices;
    for (int i = 0; i < mesh->NBones; ++i)
        if (!ref.boneNames[i].empty()) targetIndices.emplace(modRetargetBoneKey(ref.boneNames[i]), i);
    std::vector<modmesh::retarget::Matrix> sourceBind, targetBind;
    std::vector<int> parents, targetMap;
    for (uint32_t i = 0; i < sourceMesh->nbones; ++i) {
        const auto &bone = sourceSkeleton->bones[i];
        rig->sourceNames.push_back(bone.name);
        if (!bone.name.empty()) sourceIndices.emplace(modRetargetBoneKey(bone.name), int(i));
        parents.push_back(bone.parent);
        const auto target = targetIndices.find(modRetargetBoneKey(bone.name));
        targetMap.push_back(target == targetIndices.end() ? -1 : target->second);
        sourceBind.push_back(modRetargetMatrix(sourceMesh->boneMatrices.data() + size_t(i) * 16));
    }
    // An unknown weighted joint must not fall into target-order donor weights
    // while the rest of this draw uses source-order palettes.
    size_t weighted = 0;
    std::map<std::string, int> verifiedClusters;
    for (int64_t id : scene.meshModelOrder) {
        const auto &model = scene.models.at(id);
        const auto name = modmesh::Scene::normName(model.name);
        if (name != primary && name.compare(0, primary.size() + 1, primary + "_") != 0) continue;
        for (int64_t geometryId : model.geoms) {
            const auto geometry = scene.geoms.find(geometryId);
            if (geometry == scene.geoms.end()) continue;
            for (const auto &cluster : geometry->second.clusters) {
                if (genericSource) {
                    const int bone = modSourceClusterBindIndex(cluster, scene.sceneScale, *sourceMesh);
                    const auto inserted = verifiedClusters.emplace(cluster.boneName, bone);
                    if (!inserted.second && inserted.first->second != bone) {
                        sp_log("[modmesh] retarget rejected: inconsistent source cluster bind %s", cluster.boneName.c_str());
                        return false;
                    }
                    if (cluster.idx.empty()) continue;
                    if (bone < 0) {
                        sp_log("[modmesh] retarget rejected: missing or ambiguous source cluster bind %s", cluster.boneName.c_str());
                        return false;
                    }
                    ++weighted;
                    continue;
                }
                if (cluster.idx.empty()) continue;
                const auto found = sourceIndices.find(modRetargetBoneKey(cluster.boneName));
                if (found == sourceIndices.end()) {
                    sp_log("[modmesh] retarget rejected: unknown weighted source joint %s", cluster.boneName.c_str());
                    return false;
                }
                if (cluster.haveLink) for (int axis = 0; axis < 3; ++axis)
                    if (std::abs(sourceBind[found->second][12 + axis]
                                 - float(cluster.linkPos[axis] * scene.sceneScale)) > 0.02f) {
                        sp_log("[modmesh] retarget rejected: source bind mismatch at %s", cluster.boneName.c_str());
                        return false;
                    }
                ++weighted;
            }
        }
    }
    if (!weighted) return false;
    for (int i = 0; i < mesh->NBones; ++i)
        targetBind.push_back(modRetargetMatrix(reinterpret_cast<const float *>(&mesh->Bones[i])));
    std::string why;
    if (!modmesh::retarget::prepare(sourceBind, parents, targetMap, targetBind, rig->plan, &why)) {
        sp_log("[modmesh] retarget rejected: %s", why.c_str());
        return false;
    }
    // Optional attachment correspondence must use the exact validated native
    // body, including custom hero aliases whose file identity differs from it.
    const auto targetSource = modNativeMeshSource(targetMetadataFamily.empty()
        ? ref.targetFileName : targetMetadataFamily, hint);
    const auto *targetMesh = targetSource && mesh->Name
        ? targetSource->findMesh(mesh->Name->to_string()) : nullptr;
    if (targetMesh && modNativeBindMatches(mesh, targetMesh)) {
        rig->attachmentGeometryReady = modmesh::attachment::prepare(*source, *sourceMesh,
            *targetSource, *targetMesh, rig->plan, rig->attachmentGeometry, &why);
        if (!rig->attachmentGeometryReady)
            sp_log("[modmesh] attachment geometry unavailable for %s: %s",
                   rig->targetMeshName.c_str(), why.c_str());
    }
    rig->sourceInverse.resize(sourceBind.size());
    for (size_t i = 0; i < sourceBind.size(); ++i)
        std::memcpy(&rig->sourceInverse[i], rig->plan.sourceInverse[i].data(), 64);
    ref.nbones = int(sourceBind.size());
    ref.boneNames = rig->sourceNames;
    ref.clusterBoneIndices = std::move(verifiedClusters);
    ref.rejectImportedSkin = false;
    ref.bonePos = sourceMesh->bonePos;
    ref.haveBones = true;
    for (int axis = 0; axis < 3; ++axis) {
        ref.bonesMin[axis] = ref.bonesMax[axis] = ref.bonePos[axis];
        for (size_t i = 1; i < sourceBind.size(); ++i) {
            ref.bonesMin[axis] = std::min(ref.bonesMin[axis], ref.bonePos[i * 3 + axis]);
            ref.bonesMax[axis] = std::max(ref.bonesMax[axis], ref.bonePos[i * 3 + axis]);
        }
    }
    modMeshRetargets[mesh] = rig;
    modMeshBufferOwners[mesh->File->FileBuf.Buf] = mesh->File;
    if (genericSource)
        sp_log("[modmesh] verified generic source skin: %u weighted clusters, %u palette bones; FBX labels resolved by full bind matrices",
               unsigned(weighted), sourceMesh->nbones);
    sp_log("[modmesh] PCSKEL retarget %s: source %s (%u bones) -> %s (%d skin bones); source proportions",
           rig->targetMeshName.c_str(), sourceSkeleton->name.c_str(), sourceMesh->nbones,
           targetSkeleton->name.c_str(), mesh->NBones);
    return true;
}

static std::shared_ptr<ModMeshRetarget> modRetargetForMesh(nglMesh *mesh)
{
    const auto found = modMeshRetargets.find(mesh);
    return found == modMeshRetargets.end() ? nullptr : found->second;
}

static void modBindRetargetMaterial(nglMesh *mesh, nglMaterialBase *material)
{
    if (auto rig = modRetargetForMesh(mesh)) modMaterialRetargets[material][rig->targetMeshName] = rig;
}

static void modForgetRetargetFile(nglMeshFile *file)
{
    for (auto it = modMaterialRetargets.begin(); it != modMaterialRetargets.end();) {
        for (auto rig = it->second.begin(); rig != it->second.end();)
            if (rig->second->owner == file) rig = it->second.erase(rig); else ++rig;
        if (it->second.empty()) it = modMaterialRetargets.erase(it); else ++it;
    }
    for (auto it = modMeshRetargets.begin(); it != modMeshRetargets.end();)
        if (it->second->owner == file) it = modMeshRetargets.erase(it); else ++it;
}
