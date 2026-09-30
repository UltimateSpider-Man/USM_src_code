// Included after mod_mesh_retarget_runtime.inc. This profile applies only to
// the exact unrigged Miles asset and leaves the native gameplay rig untouched.
#include "mod_miles_skin.h"

static bool modPrepareMilesSkin(Mod *mod, nglMesh *mesh,
                                std::shared_ptr<modmesh::Scene> &scene,
                                modmesh::OrigMeshRef &ref,
                                const std::string &metadataFamily,
                                bool &recognized)
{
    recognized = mod && transformToLower(mod->Path.stem().string()) == "spiderman_miles_morales";
    if (!recognized) return false;
    // An explicit donor-pose sidecar selects the general unrigged importer.
    // A filename must not override that choice by requiring a separate skin
    // profile. The importer still validates native bind metadata and rejects
    // authored source weights before applying the requested donor pose.
    if (scene && scene->cfg.custom && scene->cfg.skin == 2
        && scene->cfg.donorPose == "arms_down") {
        recognized = false;
        return false;
    }
    const auto reject = [&](const std::string &why) {
        sp_log("[modmesh] Miles skin rejected: %s; native mesh retained", why.c_str());
        return false;
    };
    if (!scene || !mesh || !mesh->File || !mesh->Name || !mesh->Bones
        || mesh->NBones <= 0 || mesh->NBones > 1024 || mod->Data.empty())
        return reject("missing native mesh or source scene");
    if (scene->meshModelOrder.size() != 1 || scene->geoms.size() != 1 || !scene->anims.empty())
        return reject("profile requires the original single-mesh, unrigged FBX");
    const auto model = scene->models.find(scene->meshModelOrder.front());
    if (model == scene->models.end() || model->second.geoms.size() != 1)
        return reject("unexpected Miles geometry bindings");
    const auto geometry = scene->geoms.find(model->second.geoms.front());
    if (geometry == scene->geoms.end() || !geometry->second.clusters.empty())
        return reject("authored skins must not be overwritten by the unrigged profile");

    const uint64_t fingerprint = modmesh::miles::fingerprint(mod->Data.data(), mod->Data.size());
    auto profilePath = mod->Path;
    profilePath += ".skin";
    const std::string key = profilePath.string() + "|" + std::to_string(fingerprint);
    static std::map<std::string, std::shared_ptr<modmesh::miles::Skin>> profiles;
    std::shared_ptr<modmesh::miles::Skin> skin;
    if (const auto found = profiles.find(key); found != profiles.end()) skin = found->second;
    if (!skin) {
        std::error_code ec;
        const auto size = std::filesystem::file_size(profilePath, ec);
        if (ec || size < 60 || size > 8u * 1024u * 1024u)
            return reject("place spiderman_miles_morales.fbx.skin next to the original FBX");
        std::vector<uint8_t> bytes;
        std::string why;
        auto candidate = std::make_shared<modmesh::miles::Skin>();
        if (!modReadWholeFile(profilePath, bytes) || !candidate->parse(bytes, fingerprint, &why))
            return reject(why.empty() ? "unreadable Miles skin profile" : why);
        profiles.emplace(key, candidate);
        skin = std::move(candidate);
    }
    if (geometry->second.ctrl.size() != skin->vertices.size() * 3)
        return reject("Miles skin/control-point count differs");

    // The built-in name table verifies every native bind position, not just
    // the filename or palette count. An external PCSKEL is used only alongside
    // a full native PCMESH bind signature that matches the loaded resource.
    auto names = modmesh::nativeBoneNames(mesh->Name->to_string(), mesh->NBones, ref.bonePos);
    if (names.size() != size_t(mesh->NBones)) {
        const auto hint = mod->Path.parent_path();
        const auto family = metadataFamily.empty() ? ref.targetFileName : metadataFamily;
        const auto native = modNativeMeshSource(family, hint);
        const auto skeleton = modNativeSkeleton(family + ".PCSKEL", hint);
        const auto *nativeMesh = native ? native->findMesh(mesh->Name->to_string()) : nullptr;
        if (!modNativeBindMatches(mesh, nativeMesh) || !skeleton
            || skeleton->bones.size() < size_t(mesh->NBones))
            return reject("native bone names/order could not be validated");
        names.clear();
        for (int i = 0; i < mesh->NBones; ++i) names.push_back(skeleton->bones[i].name);
    }
    std::vector<modmesh::retarget::Matrix> targetBind;
    for (int i = 0; i < mesh->NBones; ++i)
        targetBind.push_back(modRetargetMatrix(reinterpret_cast<const float *>(&mesh->Bones[i])));
    auto rig = std::make_shared<ModMeshRetarget>();
    std::string why;
    if (!modmesh::miles::prepare(*skin, names, targetBind, rig->plan, &why)) return reject(why);
    rig->customSkin = true;
    rig->owner = mesh->File;
    rig->targetMeshName = modmesh::Scene::normName(mesh->Name->to_string());
    rig->sourceInverse.resize(rig->plan.sourceInverse.size());
    for (size_t i = 0; i < rig->sourceInverse.size(); ++i)
        std::memcpy(&rig->sourceInverse[i], rig->plan.sourceInverse[i].data(), 64);

    // Never insert this target-specific skin into loadScene's shared cache.
    // Native sibling/LOD imports and later hero switches retain clean scenes.
    auto prepared = std::make_shared<modmesh::Scene>(*scene);
    if (!modmesh::miles::install(*skin, *prepared, ref, &why)) return reject(why);
    for (const auto &joint : skin->joints) rig->sourceNames.push_back(joint.name);
    scene = std::move(prepared);
    modMeshRetargets[mesh] = rig;
    modMeshBufferOwners[mesh->File->FileBuf.Buf] = mesh->File;
    sp_log("[modmesh] Miles reconstructed skin: %u control points, %u source bones -> %d native bones; yaw %.1f; native animations, source limb lengths",
           unsigned(skin->vertices.size()), unsigned(skin->joints.size()), mesh->NBones, skin->yaw);
    return true;
}
