// Included after mod_mesh_retarget_draw.inc. Attachment points are calculated
// from live native poses; no controller, bone, spline or gameplay array changes.
#include "conglom.h"
#include "entity_base.h"
#include "skeleton_interface.h"
#include "oldmath_po.h"
#include "vector3d.h"

static std::shared_ptr<ModMeshRetarget> modAttachmentRetarget(nglMesh *mesh)
{
    if (!mesh || !mesh->Sections || mesh->NSections > 1024) return nullptr;
    const auto direct = modMeshRetargets.find(mesh);
    const std::string name = mesh->Name
        ? modmesh::pcmeshsource::normalized(mesh->Name->to_string()) : std::string();
    std::shared_ptr<ModMeshRetarget> fallback;
    // A plan may have been prepared even if every geometry upload failed.
    // Only a successfully replaced material activates attachment adjustment.
    for (uint32_t i = 0; i < mesh->NSections; ++i) {
        const auto *section = mesh->Sections[i].Section;
        if (!section) continue;
        const auto material = modMaterialRetargets.find(section->Material);
        if (material == modMaterialRetargets.end()) continue;
        if (direct != modMeshRetargets.end())
            for (const auto &entry : material->second)
                if (entry.second == direct->second) return entry.second;
        const auto exact = material->second.find(name);
        if (exact != material->second.end() && exact->second) return exact->second;
        for (const auto &entry : material->second) {
            if (!entry.second) continue;
            if (!fallback) fallback = entry.second;
            if (mesh->NBones >= 0
                && entry.second->plan.targetBind.size() == std::size_t(mesh->NBones))
                return entry.second;
        }
    }
    return fallback;
}

bool modMeshAttachmentDelta(conglomerate *owner, entity_base *base, vector3d &delta)
{
    if (!owner || !base || !base->my_abs_po || !owner->my_abs_po
        || !owner->skeleton_ifc || !owner->skeleton_ifc->abs_po) return false;
    auto rig = modAttachmentRetarget(owner->get_mesh());
    if (!rig) return false;
    namespace rt = modmesh::retarget;
    const auto &plan = rig->plan;
    auto *skeleton = owner->skeleton_ifc;
    if (plan.targetBind.empty() || plan.targetBind.size() > 1024
        || skeleton->po_count < 0 || std::size_t(skeleton->po_count) < plan.targetBind.size())
        return false;

    bool belongsToOwner = base->my_conglom_root == owner;
    std::size_t ownershipSteps = 0;
    for (auto *parent = base; !belongsToOwner && parent && ownershipSteps++ <= 1024;
         parent = parent->m_parent) {
        if (parent == owner) belongsToOwner = true;
        else if (parent->m_parent == parent) return false;
    }
    if (!belongsToOwner) return false;

    std::size_t sourceAncestor = plan.sourceBind.size(), targetAncestor = 0;
    // A native tongue joint can be outside the body mesh palette (73 vs 60).
    // Its parent HEAD is in the palette and provides an exact attachment frame.
    std::size_t parentSteps = 0;
    for (auto *ancestor = base; ancestor && ancestor != owner;) {
        const auto index = ancestor->get_bone_idx();
        if (std::size_t(index) < plan.targetBind.size()) {
            // A body index must identify the owner's actual pose storage.
            // Do not turn a bad ancestor pointer into a surface fallback.
            if (ancestor->my_abs_po != &skeleton->abs_po[index]) return false;
            for (std::size_t j = 0; j < plan.targetBone.size(); ++j)
                if (plan.targetBone[j] == int(index)) {
                    sourceAncestor = j;
                    targetAncestor = std::size_t(index);
                    break;
                }
            if (sourceAncestor < plan.sourceBind.size()) break;
        }
        // Entity trees are bounded by the native skeleton index range. Guard
        // malformed parent cycles without changing any entity relationships.
        auto *next = ancestor->m_parent;
        if (next == ancestor) return false;
        ancestor = next;
        if (++parentSteps > 1024) return false;
    }
    // Native getters may refresh dirty world transforms. Resolve both before
    // copying the skeleton, so the body pose and attachment point represent
    // the same frame even when this effect renders before the body.
    const auto world = modRetargetDrawMatrix(owner->get_abs_po().m);
    const auto baseWorld = modRetargetDrawMatrix(base->get_abs_po().m);
    if (!rt::affine(world) || !rt::affine(baseWorld)) return false;
    std::vector<rt::Matrix> targetWorld(plan.targetBind.size()), sourceWorld;
    for (std::size_t j = 0; j < targetWorld.size(); ++j)
        targetWorld[j] = modRetargetDrawMatrix(skeleton->abs_po[j].m);
    if (!rt::evaluate(plan, targetWorld.data(), targetWorld.size(), sourceWorld)) return false;
    if (!modAlignRetargetSupport(*rig, sourceWorld, targetWorld, world)) return false;
    const auto point = rt::position(baseWorld);
    if (sourceAncestor >= plan.sourceBind.size()) {
        // Native attack controls may be independent actor children, outside
        // the skin palette. Their animation moves them between the back and
        // hands; associate them with the closest animated native skin each
        // draw instead of pinning them to a guessed or first-frame spine bone.
        modmesh::attachment::Anchor anchor;
        if (!rig->attachmentGeometryReady ||
            !modmesh::attachment::chooseAnchor(rig->attachmentGeometry,
                targetWorld.data(), targetWorld.size(), point, anchor)) return false;
        sourceAncestor = anchor.source;
        targetAncestor = anchor.target;
    }
    rt::Vec3 moved;
    if (rig->attachmentGeometryReady) {
        if (!modmesh::attachment::transfer(rig->attachmentGeometry, point,
                targetWorld[targetAncestor], sourceWorld[sourceAncestor],
                sourceAncestor, targetAncestor, moved)) return false;
    } else {
        // Metadata without usable skin envelopes retains the earlier mapped
        // ancestor behavior. Independent controls require verified geometry.
        rt::Matrix targetInverse;
        if (!rt::inverse(targetWorld[targetAncestor], targetInverse)) return false;
        moved = rt::point(rt::point(point, targetInverse), sourceWorld[sourceAncestor]);
    }
    vector3d result;
    for (int axis = 0; axis < 3; ++axis) {
        result[axis] = moved[axis] - point[axis];
        if (!std::isfinite(result[axis])) return false;
    }
    delta = result;
    return true;
}
