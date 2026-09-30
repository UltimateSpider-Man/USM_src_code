#pragma once

// Retarget an authored humanoid skin without replacing its source weights.
// Input pivots must already share the imported geometry's final game frame.
// This helper builds render metadata only; it does not infer weights, alter
// the gameplay skeleton, or claim that arbitrary rigs are humanoids.
#include "mod_miles_skin.h"
#include <map>

namespace modmesh { namespace foreignrig {
namespace rt = retarget;

struct Joint {
    std::string name;
    int parent = -1;
    rt::Vec3 position{};
};

inline std::string jointName(std::string name)
{
    if (auto p = name.find('\0'); p != std::string::npos) name.resize(p);
    if (auto p = name.rfind("::"); p != std::string::npos) name.erase(0, p + 2);
    for (char &c : name) if (c >= 'A' && c <= 'Z') c = char(c - 'A' + 'a');
    return name;
}

inline std::string boneKey(std::string name)
{
    name = jointName(std::move(name));
    if (auto p = name.rfind(':'); p != std::string::npos) name.erase(0, p + 1);
    // Finite semantic aliases, never trailing-number or substring guesses.
    static const std::map<std::string, std::string> aliases{
        {"hips", "pelvis"},
        {"leftshoulder", "lclavicle"}, {"rightshoulder", "rclavicle"},
        {"leftarm", "lupperarm"}, {"rightarm", "rupperarm"},
        {"leftforearm", "lforearm"}, {"rightforearm", "rforearm"},
        {"lefthand", "lhand"}, {"righthand", "rhand"},
        {"leftupleg", "lthigh"}, {"rightupleg", "rthigh"},
        {"leftleg", "lcalf"}, {"rightleg", "rcalf"},
        {"leftfoot", "lfoot"}, {"rightfoot", "rfoot"},
        {"lefttoebase", "ltoe"}, {"righttoebase", "rtoe"},
        {"ltoe0", "ltoe"}, {"rtoe0", "rtoe"}
    };
    const auto normalized = [](std::string value) {
        value.erase(std::remove_if(value.begin(), value.end(), [](char c) {
            return c == ' ' || c == '_' || c == '-' || c == '.';
        }), value.end());
        if (value.compare(0, 5, "bip01") == 0) value.erase(0, 5);
        return value;
    };
    static const std::set<std::string> roles{
        "pelvis", "spine", "spine1", "spine2", "neck", "head",
        "lclavicle", "lupperarm", "lforearm", "lhand", "lthigh", "lcalf", "lfoot", "ltoe",
        "rclavicle", "rupperarm", "rforearm", "rhand", "rthigh", "rcalf", "rfoot", "rtoe"
    };
    // Blender's duplicate suffix is harmless for a recognized semantic role.
    // Preserve it on arbitrary helpers: foo.001 and foo.002 are distinct bones.
    if (const auto dot = name.rfind('.'); dot != std::string::npos && dot+1 < name.size()) {
        const bool digits = std::all_of(name.begin()+std::ptrdiff_t(dot+1), name.end(),
                                        [](char c) { return c >= '0' && c <= '9'; });
        const auto base = normalized(name.substr(0, dot));
        if (digits && (aliases.count(base) || roles.count(base))) name.resize(dot);
    }
    name = normalized(std::move(name));
    const auto alias = aliases.find(name);
    return alias == aliases.end() ? name : alias->second;
}

inline bool prepare(const std::vector<Joint> &source,
                    const std::vector<std::string> &targetNames,
                    const std::vector<rt::Matrix> &targetBind,
                    const std::map<std::string, std::string> &explicitBoneMap,
                    rt::Plan &out, std::string *why = nullptr)
{
    auto fail = [&](const std::string &message) { return rt::fail(why, message.c_str()); };
    if (source.empty() || source.size() > 1024 || targetBind.empty()
        || targetBind.size() > 1024 || targetNames.size() != targetBind.size())
        return fail("invalid foreign humanoid source/target bone counts");

    std::map<std::string, int> sourceExact;
    std::map<std::string, std::vector<int>> sourceByKey, targetByKey;
    std::vector<int> parents, mapping(source.size(), -1);
    for (size_t j = 0; j < source.size(); ++j) {
        const auto &joint = source[j];
        if (joint.name.empty() || !sourceExact.emplace(joint.name, int(j)).second)
            return fail("empty or duplicate foreign source joint name");
        const auto key = boneKey(joint.name);
        if (!key.empty()) sourceByKey[key].push_back(int(j));
        if (joint.parent < -1 || joint.parent >= int(source.size()) || joint.parent == int(j))
            return fail("invalid foreign source parent");
        parents.push_back(joint.parent);
        for (float value : joint.position)
            if (!std::isfinite(value)) return fail("nonfinite foreign source bind pivot");
    }
    for (size_t j = 0; j < targetNames.size(); ++j) {
        rt::Matrix check;
        if (!rt::inverse(targetBind[j], check)) return fail("invalid native humanoid bind matrix");
        const auto key = boneKey(targetNames[j]);
        if (!key.empty()) targetByKey[key].push_back(int(j));
    }

    auto uniqueTarget = [&](const std::string &name, bool exactFirst) {
        if (exactFirst) {
            int exact = -1;
            for (size_t j = 0; j < targetNames.size(); ++j) if (targetNames[j] == name) {
                if (exact >= 0) return -2;
                exact = int(j);
            }
            if (exact >= 0) return exact;
        }
        const auto found = targetByKey.find(boneKey(name));
        if (found == targetByKey.end()) return -1;
        return found->second.size() == 1 ? found->second.front() : -2;
    };
    std::map<int, int> overrides;
    for (const auto &entry : explicitBoneMap) {
        int sourceIndex = -1;
        const auto exact = sourceExact.find(entry.first);
        if (exact != sourceExact.end()) sourceIndex = exact->second;
        else {
            const auto named = sourceByKey.find(boneKey(entry.first));
            if (named == sourceByKey.end() || named->second.size() != 1)
                return fail("missing or ambiguous explicit source joint: " + entry.first);
            sourceIndex = named->second.front();
        }
        const int target = uniqueTarget(entry.second, true);
        if (target < 0) return fail("missing or ambiguous explicit native joint: " + entry.second);
        if (!overrides.emplace(sourceIndex, target).second)
            return fail("duplicate explicit mapping of one source joint");
    }
    std::set<int> usedTargets;
    std::map<std::string, int> sourceRole;
    for (size_t j = 0; j < source.size(); ++j) {
        const auto specified = overrides.find(int(j));
        const int target = specified == overrides.end()
            ? uniqueTarget(source[j].name, false) : specified->second;
        if (target == -2) return fail("ambiguous native joint for source: " + source[j].name);
        if (target < 0) continue;
        if (!usedTargets.insert(target).second)
            return fail("multiple source joints map to one native bone: " + targetNames[size_t(target)]);
        mapping[j] = target;
        const auto role = boneKey(targetNames[size_t(target)]);
        if (!sourceRole.emplace(role, int(j)).second)
            return fail("ambiguous mapped native humanoid role: " + role);
    }

    // Each required role names the previous required role in its chain.
    // Optional spine/neck/clavicle joints and unmapped twists may intervene.
    static const std::pair<const char *, const char *> core[] = {
        {"pelvis", ""}, {"spine", "pelvis"}, {"head", "spine"},
        {"lupperarm", "spine"}, {"lforearm", "lupperarm"}, {"lhand", "lforearm"},
        {"rupperarm", "spine"}, {"rforearm", "rupperarm"}, {"rhand", "rforearm"},
        {"lthigh", "pelvis"}, {"lcalf", "lthigh"}, {"lfoot", "lcalf"},
        {"rthigh", "pelvis"}, {"rcalf", "rthigh"}, {"rfoot", "rcalf"}
    };
    std::set<int> coreIndices;
    for (const auto &role : core) {
        const auto found = sourceRole.find(role.first);
        if (found == sourceRole.end()) return fail(std::string("missing required humanoid mapping: ") + role.first);
        coreIndices.insert(found->second);
    }
    std::vector<unsigned char> state(source.size(), 0);
    std::vector<size_t> order;
    std::function<bool(size_t)> visit = [&](size_t j) {
        if (state[j] == 1) return false;
        if (state[j] == 2) return true;
        state[j] = 1;
        if (parents[j] >= 0 && !visit(size_t(parents[j]))) return false;
        state[j] = 2;
        order.push_back(j);
        return true;
    };
    for (size_t j = 0; j < source.size(); ++j)
        if (!visit(j)) return fail("foreign source parent cycle");
    for (const auto &role : core) {
        if (!*role.second) continue;
        int ancestor = parents[size_t(sourceRole.at(role.first))];
        while (ancestor >= 0 && !coreIndices.count(ancestor)) ancestor = parents[size_t(ancestor)];
        if (ancestor != sourceRole.at(role.second))
            return fail(std::string("incompatible humanoid limb hierarchy at: ") + role.first);
    }

    // FBX scene/armature wrappers can precede hips. Keep them in the palette,
    // but make hips the render root and attach its old wrapper chain below it.
    const int pelvis = sourceRole.at("pelvis");
    int wrapper = parents[size_t(pelvis)];
    if (wrapper >= 0) {
        while (parents[size_t(wrapper)] >= 0) wrapper = parents[size_t(wrapper)];
        parents[size_t(pelvis)] = -1;
        parents[size_t(wrapper)] = pelvis;
    }
    state.assign(source.size(), 0);
    order.clear();
    for (size_t j = 0; j < source.size(); ++j)
        if (!visit(j)) return fail("foreign render parent cycle");
    for (size_t j : order) {
        int ancestor = int(j);
        while (ancestor >= 0 && ancestor != pelvis) ancestor = parents[size_t(ancestor)];
        if (ancestor != pelvis) return fail("foreign source joint is outside the mapped hips hierarchy");
    }

    // Explicit anatomical guide chains avoid choosing a coincident twist or
    // an arbitrary finger as the direction of an entire forearm/hand.
    static const std::map<std::string, std::vector<std::string>> guides{
        {"pelvis", {"spine", "spine1", "spine2", "neck", "head"}},
        {"spine", {"spine1", "spine2", "neck", "head"}},
        {"spine1", {"spine2", "neck", "head"}}, {"spine2", {"neck", "head"}},
        {"neck", {"head"}}, {"lclavicle", {"lupperarm"}}, {"rclavicle", {"rupperarm"}},
        {"lupperarm", {"lforearm"}}, {"rupperarm", {"rforearm"}},
        {"lforearm", {"lhand"}}, {"rforearm", {"rhand"}},
        {"lthigh", {"lcalf"}}, {"rthigh", {"rcalf"}},
        {"lcalf", {"lfoot"}}, {"rcalf", {"rfoot"}},
        {"lfoot", {"ltoe"}}, {"rfoot", {"rtoe"}}
    };
    std::vector<rt::Matrix> binds(source.size()), corrections(source.size(), rt::identity());
    for (size_t j : order) {
        const int parent = parents[j], target = mapping[j];
        if (parent >= 0) corrections[j] = corrections[size_t(parent)];
        if (target < 0) {
            if (parent < 0) return fail("unmapped foreign render root");
            binds[j] = binds[size_t(parent)];
        } else {
            int guide = -1;
            const auto candidates = guides.find(boneKey(targetNames[size_t(target)]));
            if (candidates != guides.end()) for (const auto &role : candidates->second) {
                const auto found = sourceRole.find(role);
                if (found == sourceRole.end()) continue;
                int ancestor = parents[size_t(found->second)];
                while (ancestor >= 0 && ancestor != int(j)) ancestor = parents[size_t(ancestor)];
                if (ancestor != int(j)) return fail("incompatible optional humanoid guide hierarchy");
                guide = found->second;
                break;
            }
            if (guide >= 0) {
                rt::Vec3 nativeDirection{}, sourceDirection{};
                for (int d = 0; d < 3; ++d) {
                    nativeDirection[d] = targetBind[size_t(mapping[size_t(guide)])][12+d]
                                       - targetBind[size_t(target)][12+d];
                    sourceDirection[d] = source[size_t(guide)].position[d] - source[j].position[d];
                }
                if (!miles::directionRotation(nativeDirection, sourceDirection, corrections[j]))
                    return fail("coincident or invalid foreign/native humanoid guide");
            }
            binds[j] = rt::multiply(targetBind[size_t(target)], corrections[j]);
        }
        for (int d = 0; d < 3; ++d) binds[j][12+d] = source[j].position[d];
    }
    rt::Plan plan;
    if (!rt::prepare(binds, parents, mapping, targetBind, plan, why)) return false;
    plan.absoluteBoneAxes = true;
    out = std::move(plan);
    if (why) why->clear();
    return true;
}

}} // namespace modmesh::foreignrig
