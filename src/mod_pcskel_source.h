#pragma once

// Pristine character (0x10002/0x10003), generic (0x10200), pedestrian (0x500),
// camera (0x10000), and panel (0x300) PCSKEL images.
// This reader never constructs
// an engine skeleton or uses serialized pointer values as process addresses.
// Body channels use fixed component layouts; ArbitraryPO carries explicit
// names, output indices and parents. PCMESH bind matrices remain the source of
// global render bind transforms: PCSKEL does not contain a uniform matrix array.
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace modmesh { namespace pcskelsource {

struct Bone {
    uint32_t index = 0;
    int parent = -1;
    std::string name;
    // Zero for names implicit in a standard character component.
    uint32_t hash = 0;
    uint32_t component = 0;
    // -1 when a standard component owns an IK/FK channel layout instead of an
    // explicit per-bone channel. These indices are component-local.
    int rotationChannel = -1, positionChannel = -1;
    bool rotationFromPose = false, positionFromPose = false;
    // Generic records describe native component-expanded pose byte offsets.
    // They are metadata, not offsets safe to dereference in the file image.
    uint32_t genericFlags = 0, positionByteOffset = 0, rotationByteOffset = 0;
    uint32_t genericRecordIndex = 0xffffffffu;
    bool hasLocalBind = false;
    // Pedestrian FK/IK channels expose fixed local positions without a uniform
    // per-joint quaternion array. Do not mistake their default identity value
    // below for an authored rotation when hasLocalBind is false.
    bool hasLocalPosition = false;
    std::array<float, 3> localPosition{{0, 0, 0}};
    // Raw native XYZW quaternion, for explicitly described ArbitraryPO bones.
    std::array<float, 4> localQuaternion{{0, 0, 0, 1}};
};

struct Component {
    uint32_t nameId = 0, typeHash = 0, flags = 0;
    std::string name;
    uint32_t firstTrack = 0, trackCount = 0;
    uint32_t perSkelOffset = 0, perSkelSize = 0;
    uint32_t defaultPoseOffset = 0, defaultPoseSize = 0;
};

struct Source {
    std::vector<uint8_t> bytes;
    std::string name, animationType;
    uint32_t version = 0;
    // The header count may be zero. In that case renderBoneCount is the output
    // extent derived from components; it is never the PCMESH palette length.
    uint32_t renderBoneCount = 0, declaredBoneCount = 0;
    // Ped/camera use a fixed native pose block instead of component directories.
    uint32_t fixedPoseOffset = 0, fixedPoseSize = 0;
    std::vector<Component> components;
    // Indexed by the engine's output pose index, not component order. Includes
    // camera/shake helper roots beyond renderBoneCount when the image has them.
    // Retail Peter/Nick Fury have unused indices: empty name denotes an unused
    // slot and must not be matched or treated as an invented bone.
    std::vector<Bone> bones;

    const Bone *findBone(const std::string &query) const {
        const auto key = normalize(query);
        for (const auto &bone : bones)
            if (!bone.name.empty() && normalize(bone.name) == key) return &bone;
        return nullptr;
    }
    static std::string normalize(std::string text) {
        for (auto &c : text)
            if (c >= 'A' && c <= 'Z') c = char(c - 'A' + 'a');
        return text;
    }
    bool parse(std::vector<uint8_t> input, std::string *why = nullptr) {
        try {
            *this = Source{};
            bytes = std::move(input);
            parseImage();
            if (why) why->clear();
            return true;
        } catch (const std::exception &e) {
            if (why) *why = e.what();
            *this = Source{};
            return false;
        }
    }

private:
    void require(bool condition, const char *message) const {
        if (!condition) throw std::runtime_error(message);
    }
    void span(size_t offset, size_t count) const {
        require(offset <= bytes.size() && count <= bytes.size() - offset,
                "PCSKEL range is outside the image");
    }
    uint32_t u32(size_t offset) const {
        span(offset, 4);
        return uint32_t(bytes[offset]) | uint32_t(bytes[offset+1]) << 8
             | uint32_t(bytes[offset+2]) << 16 | uint32_t(bytes[offset+3]) << 24;
    }
    uint16_t u16(size_t offset) const {
        span(offset, 2);
        return uint16_t(bytes[offset] | uint16_t(bytes[offset+1]) << 8);
    }
    float f32(size_t offset, float maximum = 1000000.0f) const {
        const auto bits = u32(offset);
        float value;
        std::memcpy(&value, &bits, 4);
        require(std::isfinite(value) && std::abs(value) <= maximum,
                "PCSKEL contains an invalid pose value");
        return value;
    }
    std::string fixedString(size_t offset) const {
        span(offset, 32);
        std::string result;
        for (size_t i = 4; i < 32; ++i) {
            const auto c = bytes[offset+i];
            if (!c) {
                require(!result.empty(), "PCSKEL has an empty name");
                return result;
            }
            require(c >= 32 && c < 127, "PCSKEL name is not ASCII");
            result += char(c);
        }
        throw std::runtime_error("PCSKEL name is not terminated");
    }
    static bool absent(uint32_t value) { return value == 0xffffffffu || value == 0xffffu; }
    int parentIndex(uint32_t value) const {
        if (absent(value)) return -1;
        require(value < 4096u, "PCSKEL parent index is out of range");
        return int(value);
    }
    Bone *addBone(uint32_t output, const std::string &boneName, int parent, uint32_t component) {
        if (absent(output)) return nullptr;
        require(output < 4096u, "PCSKEL output bone index is out of range");
        require(parent < 0 || uint32_t(parent) != output, "PCSKEL bone is its own parent");
        if (output >= bones.size()) bones.resize(output + 1);
        auto &bone = bones[output];
        require(bone.name.empty(), "PCSKEL components define the same output bone twice");
        bone.index = output;
        bone.name = boneName;
        bone.parent = parent;
        bone.component = component;
        return &bone;
    }
    std::vector<uint32_t> directory(uint32_t base, uint32_t size, uint32_t expectedCount) const {
        span(base, size);
        const auto count = u32(base);
        require(count == expectedCount && count <= 128u,
                "PCSKEL component data directory count does not match flags");
        require(size >= 4u * (count + 1u), "PCSKEL component directory is truncated");
        std::vector<uint32_t> result;
        for (uint32_t i = 0; i < count; ++i) {
            const auto offset = u32(size_t(base) + 4u*(i+1u));
            require(offset >= 4u*(count+1u) && offset < size && (offset & 3u) == 0,
                    "PCSKEL component data offset is invalid");
            require(std::find(result.begin(), result.end(), offset) == result.end(),
                    "PCSKEL component data offsets overlap");
            result.push_back(offset);
        }
        return result;
    }
    static uint32_t blockSize(const std::vector<uint32_t> &offsets, uint32_t start, uint32_t end) {
        for (auto offset : offsets) if (offset > start && offset < end) end = offset;
        return end - start;
    }
    void componentSpan(const Component &c, uint32_t minimum) const {
        require(c.perSkelOffset != 0 && c.perSkelSize >= minimum,
                "PCSKEL character component is truncated");
        span(c.perSkelOffset, minimum);
    }
    void torso(const Component &c, uint32_t ci, bool extra) {
        componentSpan(c, 0x74);
        const size_t s = c.perSkelOffset;
        const char *names[] = {"PELVIS", "SPINE", "SPINE1", "SPINE2", "NECK", "HEAD"};
        uint32_t indices[6];
        for (int i = 0; i < 6; ++i) indices[i] = u32(s + 0x58 + 4*i);
        const auto helper = u32(s + 0x70);
        for (int i = 0; i < 6; ++i) {
            int parent = i == 0 ? -1 : parentIndex(indices[i-1]);
            if (i == 4 && extra && !absent(helper)) parent = parentIndex(helper);
            addBone(indices[i], names[i], parent, ci);
        }
        if (extra) addBone(helper, "Bone_" + std::to_string(helper), parentIndex(indices[3]), ci);
    }
    void legs(const Component &c, uint32_t ci) {
        componentSpan(c, 0xb4);
        const size_t s = c.perSkelOffset;
        uint32_t idx[8];
        for (int i = 0; i < 8; ++i) idx[i] = u32(s + 0x90 + 4*i);
        const auto root = parentIndex(u32(s + 0xb0));
        for (int side = 0; side < 2; ++side) {
            const std::string prefix = side ? "R_" : "L_";
            addBone(idx[4+2*side], prefix+"THIGH", root, ci);
            addBone(idx[5+2*side], prefix+"CALF", parentIndex(idx[4+2*side]), ci);
            addBone(idx[2+side], prefix+"FOOT", parentIndex(idx[5+2*side]), ci);
            addBone(idx[side], prefix+"TOE", parentIndex(idx[2+side]), ci);
        }
    }
    void arms(const Component &c, uint32_t ci, bool ik) {
        const uint32_t start = ik ? 0xc0 : 0x90;
        componentSpan(c, start + 0x34);
        const size_t s = c.perSkelOffset;
        uint32_t idx[12];
        for (int i = 0; i < 12; ++i) idx[i] = u32(s + start + 4*i);
        const auto root = parentIndex(u32(s + start + 0x30));
        for (int side = 0; side < 2; ++side) {
            const std::string prefix = side ? "R_" : "L_";
            const auto clav = idx[ik ? side : side*4];
            const auto upper = idx[ik ? 4+side : side*4+1];
            const auto fore = idx[ik ? 6+side : side*4+2];
            const auto hand = idx[ik ? 2+side : side*4+3];
            addBone(clav, prefix+"CLAVICLE", root, ci);
            addBone(upper, prefix+"UPPERARM", parentIndex(clav), ci);
            addBone(fore, prefix+"FOREARM", parentIndex(upper), ci);
            addBone(hand, prefix+"HAND", parentIndex(fore), ci);
            addBone(idx[8+2*side], prefix+"FORE_TWIST_0", parentIndex(fore), ci);
            addBone(idx[9+2*side], prefix+"FORE_TWIST_1", parentIndex(idx[8+2*side]), ci);
        }
    }
    void fingers(const Component &c, uint32_t ci) {
        componentSpan(c, 0x1e8);
        const size_t s = c.perSkelOffset;
        // Serialized lane order is L thumb, R thumb, L fingers 1..4, R 1..4.
        for (int lane = 0; lane < 10; ++lane) {
            const int side = lane == 1 || lane >= 6 ? 1 : 0;
            const int finger = lane < 2 ? 0 : lane < 6 ? lane-1 : lane-5;
            int parent = parentIndex(u32(s + 0x1e0 + 4*side));
            for (int joint = 0; joint < 3; ++joint) {
                const auto index = u32(s + 0x168 + 4*(10*joint+lane));
                std::string name = side ? "R_FINGER_" : "L_FINGER_";
                name += std::to_string(finger);
                if (joint) name += std::to_string(joint);
                addBone(index, name, parent, ci);
                parent = parentIndex(index);
            }
        }
    }
    void arbitrary(const Component &c, uint32_t ci) {
        componentSpan(c, 0x20);
        const size_t s = c.perSkelOffset;
        const auto count = u32(s);
        require(count <= 4096u, "PCSKEL ArbitraryPO bone count is excessive");
        const auto table = u32(s+0x18), order = u32(s+0x1c);
        auto relative = [&](uint32_t offset, size_t size) {
            require(offset >= 0x20 && offset <= c.perSkelSize && size <= c.perSkelSize-offset,
                    "PCSKEL ArbitraryPO relative range is invalid");
            return s + offset;
        };
        const auto records = relative(table, size_t(count)*0x30);
        const auto traversal = relative(order, size_t(count)*4);
        std::vector<bool> seen(count);
        for (uint32_t i = 0; i < count; ++i) {
            const auto ordinal = u32(traversal+4*i);
            require(ordinal < count && !seen[ordinal], "PCSKEL ArbitraryPO order is not a permutation");
            seen[ordinal] = true;
        }
        const uint32_t fixedQuatCount = u16(s+0x0c), fixedPosCount = u16(s+0x0e);
        const uint32_t poseQuatCount = u32(s+4), posePosCount = u32(s+8);
        const size_t qfixed = fixedQuatCount ? relative(u32(s+0x10), size_t(fixedQuatCount)*16) : 0;
        const size_t pfixed = fixedPosCount ? relative(u32(s+0x14), size_t(fixedPosCount)*12) : 0;
        require(c.defaultPoseSize >= 16u && c.defaultPoseOffset,
                "PCSKEL ArbitraryPO default pose is missing");
        const size_t p = c.defaultPoseOffset;
        require(poseQuatCount <= 4096u && posePosCount <= 4096u,
                "PCSKEL ArbitraryPO channel count is excessive");
        // The second word is the total channel count, not the position count.
        // The remaining eight header bytes are uninitialized native padding.
        require(u32(p) == poseQuatCount && u32(p+4) == poseQuatCount + posePosCount,
                "PCSKEL ArbitraryPO pose counts do not match");
        require(size_t(poseQuatCount)*16 + size_t(posePosCount)*12 + 16 <= c.defaultPoseSize,
                "PCSKEL ArbitraryPO default channels are truncated");
        for (uint32_t i = 0; i < count; ++i) {
            const auto record = records + 0x30*i;
            const auto qi = u16(record+0x20), pi = u16(record+0x22);
            const bool qp = u16(record+0x28) != 0, pp = u16(record+0x2a) != 0;
            require(u16(record+0x28) <= 1 && u16(record+0x2a) <= 1,
                    "PCSKEL ArbitraryPO channel flag is invalid");
            require(qi < (qp ? poseQuatCount : fixedQuatCount)
                 && pi < (pp ? posePosCount : fixedPosCount),
                    "PCSKEL ArbitraryPO channel index is out of range");
            auto *bone = addBone(u16(record+0x24), fixedString(record),
                                 parentIndex(u16(record+0x26)), ci);
            if (!bone) continue;
            bone->hash = u32(record);
            bone->rotationChannel = qi;
            bone->positionChannel = pi;
            bone->rotationFromPose = qp;
            bone->positionFromPose = pp;
            const size_t q = qp ? p+16+16*qi : qfixed+16*qi;
            const size_t pos = pp ? p+16+16*poseQuatCount+12*pi : pfixed+12*pi;
            for (int k = 0; k < 4; ++k) bone->localQuaternion[k] = f32(q+4*k);
            for (int k = 0; k < 3; ++k) bone->localPosition[k] = f32(pos+4*k);
            bone->hasLocalBind = true;
            bone->hasLocalPosition = true;
        }
    }
    void generic() {
        span(0, 0xe0);
        require(normalize(animationType) == "generic", "PCSKEL generic type does not match version");
        declaredBoneCount = renderBoneCount = u32(0x60);
        const auto instructions = u32(0x64), hierarchy = u32(0x6c);
        const auto count = u32(0x74), trackInfoCount = u32(0x7c), tracks = u32(0x80);
        const auto componentCount = u32(0x88), poseSize = u32(0x90);
        require(count > 0 && count <= 4096u && declaredBoneCount <= count
             && trackInfoCount <= 65536u && tracks <= 65536u
             && componentCount > 0 && componentCount <= 128u,
                "PCSKEL generic header count is invalid");
        size_t cursor = 0xe0;
        auto take = [&](size_t size) {
            span(cursor, size);
            const size_t start = cursor;
            cursor += size;
            return start;
        };
        auto align = [&](uint32_t alignment) {
            require(alignment && alignment <= 16u && !(alignment & (alignment-1u)),
                    "PCSKEL generic alignment is invalid");
            cursor = (cursor + alignment-1u) & ~size_t(alignment-1u);
            span(cursor,0);
        };
        const auto instructionOffset=take(instructions);
        const auto hierarchyOffset=take(hierarchy); align(4);
        const auto table = take(size_t(count)*0x30); align(4);
        take(size_t(trackInfoCount)*0x28); align(4);
        const auto componentTable = take(size_t(componentCount)*0x30);
        // Xbox v10 has a different serialized descriptor layout despite sharing
        // version 0x10200. This reader deliberately accepts the pristine PC
        // layout only; no process pointer or platform fixup is applied.
        require(u32(0x94) <= 4u, "Unsupported PCSKEL generic platform alignment");
        align(u32(0x94));
        const auto pose = take(poseSize); align(4);
        take(u32(0x9c)); align(4);
        const auto staticCount = u32(0xa4);
        require(staticCount <= 128u, "PCSKEL generic static component count is excessive");
        take(size_t(staticCount)*0x30); align(u32(0xb0));
        take(u32(0xac)); align(4);
        take(u32(0xb8)); align(u32(0xc4)); take(u32(0xc0));
        for (uint32_t i=0; i<componentCount; ++i) {
            const auto r = componentTable + size_t(i)*0x30;
            Component c;
            c.nameId=i; c.typeHash=u32(r); c.name=fixedString(r);
            c.firstTrack=u32(r+0x24); c.trackCount=u32(r+0x28);
            const auto offset=u32(r+0x2c);
            require(c.typeHash && c.firstTrack <= tracks && c.trackCount <= tracks-c.firstTrack
                 && offset <= poseSize, "PCSKEL generic component track range is invalid");
            c.defaultPoseOffset=uint32_t(pose)+offset;
            c.defaultPoseSize=poseSize-offset;
            components.push_back(std::move(c));
        }
        for (auto &component : components) {
            uint32_t end=uint32_t(pose)+poseSize;
            for (const auto &other : components)
                if (other.defaultPoseOffset > component.defaultPoseOffset
                    && other.defaultPoseOffset < end) end=other.defaultPoseOffset;
            component.defaultPoseSize=end-component.defaultPoseOffset;
        }
        for (uint32_t i=0; i<count; ++i) {
            const auto r = table + size_t(i)*0x30;
            const auto flags=u32(r+0x20);
            require(flags <= 1u, "PCSKEL generic bone flags are invalid");
            auto *bone = addBone(i,fixedString(r),parentIndex(u32(r+0x2c)),0xffffffffu);
            bone->hash=u32(r);
            bone->genericFlags=flags;
            bone->positionByteOffset=u32(r+0x24);
            bone->rotationByteOffset=u32(r+0x28);
            bone->genericRecordIndex=i;
        }
        genericOutputOrder(instructionOffset,instructions,hierarchyOffset,hierarchy);
        validateHierarchy();
    }
    void genericOutputOrder(size_t program, size_t programSize,
                            size_t hierarchy, size_t hierarchySize) {
        // Generic name records are ordered by component storage, not by the
        // render palette. The pose program walks expanded rotation channels
        // in byte-offset order and explicitly names each output destination.
        // In particular, an auxiliary root can precede body channels in the
        // name table while rendering after them. Preserve its real output.
        const size_t count=declaredBoneCount;
        std::vector<uint32_t> channels(bones.size());
        for (uint32_t i=0;i<channels.size();++i) channels[i]=i;
        std::sort(channels.begin(),channels.end(),[&](uint32_t a,uint32_t b) {
            return bones[a].rotationByteOffset < bones[b].rotationByteOffset;
        });
        for (size_t i=1;i<channels.size();++i)
            require(bones[channels[i-1]].rotationByteOffset != bones[channels[i]].rotationByteOffset,
                    "PCSKEL generic rotation channels overlap");
        std::vector<int> recordToOutput(bones.size(),-1), outputToRecord(count,-1);
        std::vector<int> poseParent(count,-1), hierarchyParent(count,-1);
        std::vector<uint8_t> position(count,0), rotation(count,0), hierarchyOutput(count,0);
        std::vector<uint8_t> hierarchyUnpackedRotation(count,0);
        size_t channel=0;
        auto operand=[&](size_t &cursor,size_t end) {
            require(cursor<end,"PCSKEL generic instruction is truncated");
            const uint32_t value=bytes[cursor++];
            require(value<count,"PCSKEL generic instruction output is invalid");
            return value;
        };
        for (size_t cursor=program,end=program+programSize;cursor<end;) {
            const uint8_t op=bytes[cursor++];
            require(op==0 || op==0x80 || op==1 || op==2 || op==0x0c,
                    "Unsupported PCSKEL generic pose opcode");
            const auto output=operand(cursor,end);
            if (op==0x0c) {
                const auto child=operand(cursor,end);
                require(child!=output && poseParent[child]<0,
                        "PCSKEL generic pose parent is duplicated");
                poseParent[child]=int(output);
            } else if (op==1) {
                require(!position[output],"PCSKEL generic position output is duplicated");
                position[output]=1;
            } else {
                require(channel<count && !rotation[output],
                        "PCSKEL generic rotation output is duplicated");
                const auto record=channels[channel++];
                const auto &bone=bones[record];
                require((op==0x80)==(bone.genericFlags==1),
                        "PCSKEL generic rotation channel type does not match program");
                if (op==2) {
                    require(!position[output] && bone.positionByteOffset>=16u
                            && bone.positionByteOffset-16u==bone.rotationByteOffset,
                            "PCSKEL generic full-pose channel is invalid");
                    position[output]=1;
                }
                rotation[output]=uint8_t(op+1);
                recordToOutput[record]=int(output);
                outputToRecord[output]=int(record);
            }
        }
        require(channel==count,"PCSKEL generic program omits rotation channels");
        for (size_t cursor=hierarchy,end=hierarchy+hierarchySize;cursor<end;) {
            const uint8_t op=bytes[cursor++];
            require(op==6 || op==7 || op==8 || op==0x0d,
                    "Unsupported PCSKEL generic hierarchy opcode");
            const auto output=operand(cursor,end);
            if (op==0x0d) {
                const auto child=operand(cursor,end);
                require(child!=output && hierarchyParent[child]<0,
                        "PCSKEL generic hierarchy parent is duplicated");
                hierarchyParent[child]=int(output);
            } else if (op==6) {
                // Unpacked rotation channels have a separate prepass. Their
                // output is finalized by opcode7 later, after parent setup.
                require(rotation[output]==1 && !hierarchyUnpackedRotation[output],
                        "PCSKEL generic unpacked rotation prepass is invalid");
                hierarchyUnpackedRotation[output]=1;
            } else {
                require(!hierarchyOutput[output],"PCSKEL generic hierarchy output is duplicated");
                hierarchyOutput[output]=1;
                const auto record=outputToRecord[output];
                require(record>=0 && ((op==8)==(rotation[output]==3))
                        && (rotation[output]!=1 || hierarchyUnpackedRotation[output]),
                        "PCSKEL generic hierarchy channel type is invalid");
            }
        }
        for (size_t output=0;output<count;++output) {
            require(rotation[output] && position[output] && hierarchyOutput[output],
                    "PCSKEL generic program output is incomplete");
            const auto &bone=bones[outputToRecord[output]];
            const int expected=bone.parent<0 ? -1 : recordToOutput.at(size_t(bone.parent));
            require((bone.parent<0 || expected>=0) && poseParent[output]==expected
                    && hierarchyParent[output]==expected,
                    "PCSKEL generic output hierarchy disagrees with named records");
        }
        // Camera/trajectory helpers not emitted by this render program remain
        // metadata after the declared output extent; they are not skin bones.
        uint32_t extra=uint32_t(count);
        for (size_t record=0;record<bones.size();++record)
            if (recordToOutput[record]<0) recordToOutput[record]=int(extra++);
        std::vector<Bone> ordered(bones.size());
        for (size_t record=0;record<bones.size();++record) {
            auto bone=std::move(bones[record]);
            if (bone.parent>=0) bone.parent=recordToOutput.at(size_t(bone.parent));
            bone.index=uint32_t(recordToOutput[record]);
            ordered[bone.index]=std::move(bone);
        }
        bones=std::move(ordered);
    }
    void pedestrian() {
        require(normalize(animationType) == "ped", "PCSKEL pedestrian type does not match version");
        span(0, 0x2d0);
        require(u32(0x60) == 0, "PCSKEL pedestrian image is already processed");
        // Retail vtable 0x892010 reports exactly 21 outputs (0x4B9DF0).
        // BuildBoneMatrices at 0x5F52C0 reads their permutation at +0x210.
        // Names independently match ped_fem/ped_male ENT member hashes and
        // bone indices; the file's output permutation also applies to parents.
        static const char *names[] = {
            "PELVIS", "SPINE", "SPINE1", "NECK", "HEAD",
            "L_CLAVICLE", "L_UPPERARM", "L_FOREARM", "L_HAND",
            "R_CLAVICLE", "R_UPPERARM", "R_FOREARM", "R_HAND",
            "L_THIGH", "L_CALF", "L_FOOT", "L_TOE",
            "R_THIGH", "R_CALF", "R_FOOT", "R_TOE"
        };
        static const int parents[] = {-1,0,1,2,3, 2,5,6,7, 2,9,10,11, 0,13,14,15, 0,17,18,19};
        std::array<uint32_t,21> output{};
        std::array<bool,21> seen{};
        for (size_t logical = 0; logical < output.size(); ++logical) {
            output[logical] = u32(0x210 + logical * 4);
            require(output[logical] < output.size() && !seen[output[logical]],
                    "PCSKEL pedestrian output table is not a permutation");
            seen[output[logical]] = true;
        }
        // +0x60 is an unused serialized runtime pointer. +0x64..0x11F is
        // the fixed default pose; +0x120..0x20F contains twenty local offsets.
        // +0x264/+0x26C are runtime/padding words, not floats or file pointers.
        for (size_t offset = 0x64; offset < 0x210; offset += 4) (void)f32(offset);
        for (size_t offset = 0x270; offset < 0x2d0; offset += 4) (void)f32(offset);
        fixedPoseOffset = 0x60;
        fixedPoseSize = 0xc0; // Native pose allocation/copy at 0x5FCCF0.
        declaredBoneCount = renderBoneCount = uint32_t(output.size());
        for (size_t logical = 0; logical < output.size(); ++logical) {
            auto *bone = addBone(output[logical], names[logical],
                parents[logical] < 0 ? -1 : int(output[size_t(parents[logical])]), 0);
            const size_t position = logical ? 0x120 + (logical - 1) * 12 : 0x64;
            for (int axis = 0; axis < 3; ++axis) bone->localPosition[axis] = f32(position + axis * 4);
            bone->hasLocalPosition = true;
            bone->rotationFromPose = true;
            bone->positionFromPose = logical == 0;
            if (!logical) {
                double norm = 0;
                for (int axis = 0; axis < 4; ++axis) {
                    const float value = bone->localQuaternion[axis] = f32(0x70 + axis * 4);
                    norm += double(value) * value;
                }
                require(std::abs(norm - 1.0) < 0.02, "PCSKEL pedestrian root quaternion is invalid");
                bone->hasLocalBind = true;
            }
        }
        validateHierarchy();
    }
    void camera() {
        require(normalize(animationType) == "camera", "PCSKEL camera type does not match version");
        span(0, 0x90);
        require(u32(0x60) == 0, "PCSKEL camera image is already processed");
        (void)f32(0x64);
        // The supplied camera's far distance is 10,000,000, unlike bone offsets.
        (void)f32(0x68, std::numeric_limits<float>::max());
        double norm = 0;
        for (size_t offset = 0x70; offset < 0x80; offset += 4) {
            const float value = f32(offset);
            norm += double(value) * value;
        }
        require(std::abs(norm - 1.0) < 0.02, "PCSKEL camera quaternion is invalid");
        for (size_t offset = 0x80; offset < 0x8c; offset += 4) (void)f32(offset);
        fixedPoseOffset = 0x60;
        fixedPoseSize = 0x30; // Retail 0x5FCD40 copies this complete pose block.
        // Camera vtable 0x892064: GetNumBones returns zero at 0x5FB880;
        // BuildBoneMatrices is empty. Its transform is an animation trajectory,
        // not a fictitious character skin bone.
        declaredBoneCount = renderBoneCount = 0;
    }
    void panel() {
        require(normalize(animationType) == "panel", "PCSKEL panel type does not match version");
        const uint32_t count = u32(0x64), table = u32(0x70);
        const uint32_t perSkel = u32(0x74), pose = u32(0x78), poseSize = u32(0x6c);
        require(u32(0x60) == 0 && count > 0 && count <= 128,
                "PCSKEL panel header count is invalid");
        require(table >= 0x84 && table % 4 == 0 && pose % 4 == 0
            && pose >= table + count * 12 && perSkel == pose,
                "PCSKEL panel data regions overlap");
        span(table, size_t(count) * 12);
        const auto offsets = directory(pose, poseSize, count);
        for (uint32_t i = 0; i < count; ++i) {
            Component c;
            const size_t record = size_t(table) + i * 12;
            c.nameId = u32(record); c.typeHash = u32(record + 4); c.flags = u32(record + 8);
            // The native panel components use default-pose channels only;
            // no per-skeleton skin data or bone output table is serialized.
            require(c.nameId && c.flags == 0, "Unsupported PCSKEL panel component flags");
            uint32_t expected = 0;
            switch (c.typeHash) {
            case 0x825bfb8bu: expected = 0x30; break;
            case 0xd3075df6u: expected = 0x24; break;
            case 0x965d992bu: expected = 0x14; break;
            case 0x25bbf0c1u: case 0x8b9af7b9u: expected = 0x1c; break;
            case 0xcdf516afu: expected = 0x14; break;
            case 0x909beafdu: expected = 0x10; break;
            case 0x9c65857du: expected = 0x20; break;
            default: throw std::runtime_error("Unsupported PCSKEL panel component type");
            }
            c.defaultPoseOffset = pose + offsets[i];
            c.defaultPoseSize = blockSize(offsets, offsets[i], poseSize);
            require(c.defaultPoseSize >= expected, "PCSKEL panel component is truncated");
            for (const auto &previous : components)
                require(previous.nameId != c.nameId, "PCSKEL panel component name is duplicated");
            components.push_back(std::move(c));
        }
        declaredBoneCount = renderBoneCount = 0;
    }
    void parseImage() {
        require(bytes.size() >= 0x84 && bytes.size() <= 16u*1024u*1024u,
                "PCSKEL image size is invalid");
        version = u32(4);
        require(version == 0x10002u || version == 0x10003u || version == 0x10200u || version == 0x500u
            || version == 0x10000u || version == 0x300u, "Unsupported PCSKEL version");
        name = fixedString(8);
        animationType = fixedString(0x28);
        if (version == 0x10200u) { generic(); return; }
        if (version == 0x500u) { pedestrian(); return; }
        if (version == 0x10000u) { camera(); return; }
        if (version == 0x300u) { panel(); return; }
        // The installed GAME.PCPACK also contains character v2. Its component
        // directories and bone records use the same bounded metadata layout,
        // but default pose sizes can differ (notably fingers). Preserve both
        // the original version and each serialized span; metadata acceptance
        // does not establish native v2/v3 skeleton replacement compatibility.
        require(normalize(animationType) == "character", "PCSKEL is not a character skeleton");
        declaredBoneCount = renderBoneCount = u32(0x60);
        const auto count = u32(0x64), table = u32(0x70);
        const auto perSkel = u32(0x74), pose = u32(0x78), poseSize = u32(0x6c);
        require(renderBoneCount <= 4096u && count > 0 && count <= 128u,
                "PCSKEL header count is invalid");
        require(table >= 0x84 && perSkel >= table + 12u*count && pose > perSkel,
                "PCSKEL data regions overlap");
        span(table, size_t(count)*12);
        span(pose, poseSize);
        uint32_t skelCount = 0, poseCount = 0;
        for (uint32_t i = 0; i < count; ++i) {
            const auto flags = u32(size_t(table)+12*i+8);
            require(flags <= 7, "PCSKEL component flags are invalid");
            skelCount += (flags & 4) != 0;
            poseCount += (flags & 1) == 0;
        }
        const auto skelDir = directory(perSkel, pose-perSkel, skelCount);
        const auto poseDir = directory(pose, poseSize, poseCount);
        uint32_t si = 0, pi = 0;
        for (uint32_t i = 0; i < count; ++i) {
            const size_t record = size_t(table)+12*i;
            Component c;
            c.nameId=u32(record); c.typeHash=u32(record+4); c.flags=u32(record+8);
            if (c.flags & 4) {
                const auto off = skelDir[si++];
                c.perSkelOffset=perSkel+off;
                c.perSkelSize=blockSize(skelDir, off, pose-perSkel);
            }
            if (!(c.flags & 1)) {
                const auto off = poseDir[pi++];
                c.defaultPoseOffset=pose+off;
                c.defaultPoseSize=blockSize(poseDir, off, poseSize);
            }
            components.push_back(c);
        }
        for (uint32_t i = 0; i < count; ++i) {
            const auto &c = components[i];
            switch (c.typeHash) {
            case 0x70ea5df2u: torso(c,i,false); break;
            case 0x7e916d6au: torso(c,i,true); break;
            case 0xa556994fu: legs(c,i); break;
            case 0xe01f4f4du: arms(c,i,false); break;
            case 0xf0ad5c8eu: arms(c,i,true); break;
            case 0xe7d9a8d3u: fingers(c,i); break;
            case 0xc5e45dcfu: arbitrary(c,i); break;
            case 0xb916e121u: case 0x464a04d8u: case 0xec4755bdu: break;
            default: throw std::runtime_error("Unsupported PCSKEL character component type");
            }
        }
        validateHierarchy();
    }
    void validateHierarchy() {
        // Several retail character images leave the optional header count zero.
        // Their complete output indices still come from the components.
        if (!renderBoneCount) renderBoneCount = uint32_t(bones.size());
        require(!bones.empty() && bones.size() >= renderBoneCount,
                "PCSKEL render bone definitions are missing");
        for (uint32_t i = 0; i < bones.size(); ++i) {
            bones[i].index = i;
            if (bones[i].name.empty()) continue;
            auto parent = bones[i].parent;
            for (size_t depth = 0; parent >= 0; ++depth) {
                require(uint32_t(parent) < bones.size(), "PCSKEL parent bone is missing");
                require(!bones[parent].name.empty(), "PCSKEL parent points to an unused output slot");
                require(depth < bones.size() && uint32_t(parent) != i, "PCSKEL hierarchy contains a cycle");
                parent = bones[parent].parent;
            }
        }
    }
};

}} // namespace modmesh::pcskelsource
