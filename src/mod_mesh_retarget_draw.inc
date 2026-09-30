// Included by ngl.cpp after ModMeshRetarget and modMaterialRetargets.
// Only the shader sees these temporary source-sized matrices. Animation and
// gameplay continue reading the actor's original skeleton and pose arrays.
static modmesh::retarget::Matrix modRetargetDrawMatrix(const matrix4x4 &value)
{
    static_assert(sizeof(matrix4x4) == sizeof(modmesh::retarget::Matrix),
                  "retargeting requires the native 64-byte matrix stride");
    modmesh::retarget::Matrix result;
    std::memcpy(result.data(), &value, sizeof(result));
    // Mat4x3's W lanes may be uninitialized in native pose storage.
    result[3] = result[7] = result[11] = 0.f;
    result[15] = 1.f;
    return result;
}

static void modRetargetDrawStore(matrix4x4 &out, const modmesh::retarget::Matrix &value)
{
    std::memcpy(&out, value.data(), sizeof(value));
}

// Shared by body and attachment rendering. Neither caller depends on whether
// the other has rendered yet in the current frame.
static bool modAlignRetargetSupport(const ModMeshRetarget &rig,
                                    std::vector<modmesh::retarget::Matrix> &sourceWorld,
                                    const std::vector<modmesh::retarget::Matrix> &targetWorld,
                                    const modmesh::retarget::Matrix &world,
                                    float *shift = nullptr, std::string *why = nullptr)
{
    std::vector<std::pair<std::size_t, std::size_t>> feet;
    // Prefer toes independently for each leg; use ankles if no toe maps.
    for (const char side : {'l', 'r'}) {
        bool found = false;
        for (const char *joint : {"toe", "foot"}) {
            const std::string wanted = std::string(1, side) + joint;
            for (std::size_t j = 0; j < rig.sourceNames.size()
                 && j < rig.plan.targetBone.size(); ++j) {
                if (rig.plan.targetBone[j] >= 0
                    && modRetargetBoneKey(rig.sourceNames[j]) == wanted) {
                    feet.emplace_back(j, std::size_t(rig.plan.targetBone[j]));
                    found = true;
                    break;
                }
            }
            if (found) break;
        }
    }
    if (feet.empty()) {
        if (shift) *shift = 0.f;
        if (why) why->clear();
        return true;
    }
    return modmesh::retarget::alignSupportHeight(sourceWorld, targetWorld.data(),
        targetWorld.size(), feet, {{world[4], world[5], world[6]}}, shift, why);
}

bool modSetupRetargetedVShaderBones(int registerBase, nglMeshNode *node,
                                   nglMeshSection *section)
{
    if (!section || !section->Material) return false;
    const auto materials = modMaterialRetargets.find(section->Material);
    if (materials == modMaterialRetargets.end()) return false;

    // A registered material belongs to imported vertices with SOURCE palette
    // indices. Once recognized, never delegate with the smaller native array,
    // including error paths and unnamed actor work-mesh clones.
    if (!node || !node->field_88 || materials->second.empty()) return true;
    const auto *mesh = node->field_88;
    const auto *params = node->field_90;
    std::string meshName;
    if (mesh->Name)
        meshName = modmesh::pcmeshsource::normalized(mesh->Name->to_string());
    std::shared_ptr<ModMeshRetarget> rig;
    const auto exact = materials->second.find(meshName);
    if (exact != materials->second.end()) rig = exact->second;
    if (!rig) {
        for (const auto &candidate : materials->second) {
            if (!candidate.second) continue;
            if (!rig) rig = candidate.second;
            if (mesh->NBones >= 0
                && candidate.second->plan.targetBind.size() == std::size_t(mesh->NBones)) {
                rig = candidate.second;
                break;
            }
        }
    }
    if (!rig || rig->plan.sourceBind.empty()) return true;

    namespace rt = modmesh::retarget;
    const auto &plan = rig->plan;
    const std::size_t sourceCount = plan.sourceBind.size();
    // prepare() enforces this cap. Keep a local check before allocating or
    // exposing a palette to the retail constant uploader as well.
    if (sourceCount > 1024 || plan.sourceInverse.size() != sourceCount) return true;

    std::string why;
    const matrix4x4 nativeWorld = params ? node->sub_419930() : node->field_0;
    rt::Matrix world = modRetargetDrawMatrix(nativeWorld), worldInverse;
    const bool validWorld = rt::inverse(world, worldInverse);
    if (!validWorld) {
        world = worldInverse = rt::identity();
        why = "invalid mesh world transform";
    }

    std::vector<rt::Matrix> targetWorld, sourceWorld;
    bool evaluated = false;
    if (validWorld && params && params->field_8 && params->NBones >= 0
        && std::size_t(params->NBones) >= plan.targetBind.size()
        && (params->Flags & (4u | 8u | 0x10u))) {
        targetWorld.resize(plan.targetBind.size());
        for (std::size_t j = 0; j < targetWorld.size(); ++j) {
            auto value = modRetargetDrawMatrix(params->field_8[j]);
            if (!(params->Flags & 4u)) {
                // Flags 8 supplies animated mesh-local bones. Flags 0x10
                // supplies already-skinned local matrices (inverse bind was
                // applied by the caller), so recover the animated bone first.
                if (!(params->Flags & 8u)) value = rt::multiply(plan.targetBind[j], value);
                value = rt::multiply(value, world);
            }
            targetWorld[j] = value;
        }
        evaluated = rt::evaluate(plan, targetWorld.data(), targetWorld.size(), sourceWorld, &why);
    } else if (why.empty()) {
        why = "native animation pose is unavailable";
    }
    float supportShift = 0.f;
    if (evaluated) {
        std::string supportError;
        if (!modAlignRetargetSupport(*rig, sourceWorld, targetWorld, world,
                                    &supportShift, &supportError) && !rig->warned) {
            sp_log("[modmesh] retarget support height %s unchanged: %s",
                   rig->targetMeshName.c_str(), supportError.c_str());
            rig->warned = true;
        }
    }
    if (!evaluated) {
        sourceWorld.resize(sourceCount);
        for (std::size_t j = 0; j < sourceCount; ++j)
            sourceWorld[j] = rt::multiply(plan.sourceBind[j], world);
        if (!rig->warned) {
            sp_log("[modmesh] retarget draw fallback %s: %s; retaining source bind pose",
                   rig->targetMeshName.c_str(), why.c_str());
            rig->warned = true;
        }
    } else if (!rig->logged) {
        sp_log("[modmesh] retarget draw %s: source-bones=%u target-bones=%u native-pose=%d flags=%08X support-shift=%.6f",
               rig->targetMeshName.c_str(), unsigned(sourceCount),
               unsigned(plan.targetBind.size()), params->NBones, unsigned(params->Flags), supportShift);
        rig->logged = true;
    }

    std::vector<math::MatClass<4, 3>> sourcePose(sourceCount), inverseFallback;
    for (std::size_t j = 0; j < sourceCount; ++j)
        modRetargetDrawStore(sourcePose[j], sourceWorld[j]);
    auto *inverseBind = rig->sourceInverse.data();
    if (rig->sourceInverse.size() != sourceCount) {
        inverseFallback.resize(sourceCount);
        for (std::size_t j = 0; j < sourceCount; ++j)
            modRetargetDrawStore(inverseFallback[j], plan.sourceInverse[j]);
        inverseBind = inverseFallback.data();
    }

    nglMesh shadowMesh = *mesh;
    shadowMesh.NBones = int(sourceCount);
    shadowMesh.Bones = inverseBind;
    nglMeshParams shadowParams{};
    if (params) shadowParams = *params;
    shadowParams.Flags = (shadowParams.Flags & ~(4u | 8u | 0x10u)) | 4u;
    shadowParams.NBones = int(sourceCount);
    shadowParams.field_8 = sourcePose.data();
    nglMeshNode shadowNode = *node;
    shadowNode.field_88 = &shadowMesh;
    shadowNode.field_90 = &shadowParams;
    matrix4x4 inverseWorldCache;
    modRetargetDrawStore(inverseWorldCache, worldInverse);
    shadowNode.field_80 = &inverseWorldCache;

    // Import validates section palettes before publication. Defensively keep
    // even a malformed clone inside the retail uploader's 64-bone scratch area.
    nglMeshSection shadowSection = *section;
    uint16_t palette[64]{};
    shadowSection.NBones = std::max(0, std::min(section->NBones, 64));
    for (int j = 0; j < shadowSection.NBones; ++j)
        if (section->BonesIdx && section->BonesIdx[j] < sourceCount)
            palette[j] = section->BonesIdx[j];
    shadowSection.BonesIdx = palette;
    CDECL_CALL(0x00772810, registerBase, &shadowNode, &shadowSection);
    return true;
}
