#include "../src/mod_mesh_import.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

struct NativeSection {
    std::vector<float> vertices;
    std::vector<std::uint16_t> palette;
    std::uint32_t nindices = 0;
};

struct NativeMesh {
    std::string name;
    std::vector<NativeSection> sections;
    modmesh::OrigMeshRef ref;
};

struct Reader {
    std::vector<std::uint8_t> bytes;

    explicit Reader(const char *path)
    {
        std::ifstream file(path, std::ios::binary);
        bytes.assign(std::istreambuf_iterator<char>(file), {});
        if (bytes.empty()) throw std::runtime_error("cannot read PCMESH");
    }

    void need(std::size_t off, std::size_t count) const
    {
        if (off > bytes.size() || count > bytes.size() - off)
            throw std::runtime_error("PCMESH pointer outside file");
    }

    std::uint32_t u32(std::size_t off) const
    {
        need(off, 4);
        std::uint32_t value;
        std::memcpy(&value, bytes.data() + off, 4);
        return value;
    }

    float f32(std::size_t off) const
    {
        need(off, 4);
        float value;
        std::memcpy(&value, bytes.data() + off, 4);
        return value;
    }

    std::string fixedString(std::size_t off) const
    {
        need(off, 32);
        const char *text = reinterpret_cast<const char *>(bytes.data() + off + 4);
        std::size_t count = 0;
        while (count < 28 && text[count] != '\0') ++count;
        return std::string(text, count);
    }
};

std::vector<NativeMesh> readPcmesh(const char *path)
{
    Reader r(path);
    r.need(0, 20);
    if (std::memcmp(r.bytes.data(), "PCM ", 4) != 0 || r.u32(4) != 0x601)
        throw std::runtime_error("not a PC PCMESH 0x601 file");

    const std::uint32_t count = r.u32(8);
    const std::uint32_t directory = r.u32(12);
    r.need(directory, std::size_t(count) * 12);

    std::vector<NativeMesh> meshes;
    for (std::uint32_t entry = 0; entry < count; ++entry) {
        const std::size_t de = std::size_t(directory) + std::size_t(entry) * 12;
        if (r.bytes[de + 3] != 2) continue;
        const std::uint32_t meshOff = r.u32(de + 4);
        r.need(meshOff, 0x40);

        NativeMesh mesh;
        mesh.name = r.fixedString(r.u32(meshOff));
        const std::uint32_t nsections = r.u32(meshOff + 8);
        const std::uint32_t sectionTable = r.u32(meshOff + 12);
        const std::uint32_t nbones = r.u32(meshOff + 16);
        const std::uint32_t bones = r.u32(meshOff + 20);
        mesh.ref.nbones = int(nbones);
        mesh.ref.haveSphere = true;
        for (int axis = 0; axis < 3; ++axis)
            mesh.ref.sphereCenter[axis] = r.f32(meshOff + 0x20 + axis * 4);
        mesh.ref.sphereRadius = r.f32(meshOff + 0x30);

        if (nbones != 0) {
            r.need(bones, std::size_t(nbones) * 64);
            mesh.ref.haveBones = true;
            mesh.ref.bonePos.reserve(std::size_t(nbones) * 3);
            for (std::uint32_t bone = 0; bone < nbones; ++bone) {
                for (int axis = 0; axis < 3; ++axis) {
                    const float value = r.f32(std::size_t(bones) + bone * 64 + 48 + axis * 4);
                    mesh.ref.bonePos.push_back(value);
                    if (bone == 0) {
                        mesh.ref.bonesMin[axis] = value;
                        mesh.ref.bonesMax[axis] = value;
                    } else {
                        mesh.ref.bonesMin[axis] = std::min(mesh.ref.bonesMin[axis], value);
                        mesh.ref.bonesMax[axis] = std::max(mesh.ref.bonesMax[axis], value);
                    }
                }
            }
        }

        r.need(sectionTable, std::size_t(nsections) * 8);
        mesh.sections.reserve(nsections);
        for (std::uint32_t si = 0; si < nsections; ++si) {
            const std::uint32_t sectionOff = r.u32(std::size_t(sectionTable) + si * 8 + 4);
            r.need(sectionOff, 0x60);
            const std::uint32_t sectionBones = r.u32(sectionOff + 8);
            const std::uint32_t paletteOff = r.u32(sectionOff + 12);
            const std::uint32_t nindices = r.u32(sectionOff + 0x2c);
            const std::uint32_t nvertices = r.u32(sectionOff + 0x38);
            const std::uint32_t verticesOff = r.u32(sectionOff + 0x3c);
            const std::uint32_t vertexBytes = r.u32(sectionOff + 0x40);
            const std::uint32_t stride = r.u32(sectionOff + 0x48);
            if (stride != 64 || vertexBytes != nvertices * stride)
                throw std::runtime_error(mesh.name + " contains a non-skinned section");

            NativeSection section;
            section.nindices = nindices;
            r.need(verticesOff, vertexBytes);
            section.vertices.resize(std::size_t(nvertices) * 16);
            std::memcpy(section.vertices.data(), r.bytes.data() + verticesOff, vertexBytes);
            if (sectionBones != 0) {
                r.need(paletteOff, std::size_t(sectionBones) * 2);
                section.palette.resize(sectionBones);
                std::memcpy(section.palette.data(), r.bytes.data() + paletteOff,
                            std::size_t(sectionBones) * 2);
            }
            mesh.sections.push_back(std::move(section));
        }
        meshes.push_back(std::move(mesh));
    }
    return meshes;
}

std::vector<modmesh::OrigSectionView> views(const NativeMesh &mesh)
{
    std::vector<modmesh::OrigSectionView> result;
    result.reserve(mesh.sections.size());
    for (const NativeSection &section : mesh.sections) {
        modmesh::OrigSectionView view;
        view.verts = section.vertices.data();
        view.nverts = std::uint32_t(section.vertices.size() / 16);
        view.strideBytes = 64;
        view.palette = section.palette.empty() ? nullptr : section.palette.data();
        view.nbones = int(section.palette.size());
        result.push_back(view);
    }
    return result;
}

const NativeMesh &named(const std::vector<NativeMesh> &meshes, const char *name)
{
    const auto found = std::find_if(meshes.begin(), meshes.end(), [&](const NativeMesh &mesh) {
        return modmesh::Scene::normName(mesh.name) == modmesh::Scene::normName(name);
    });
    if (found == meshes.end()) throw std::runtime_error(std::string("mesh not found: ") + name);
    return *found;
}

void require(bool value, const std::string &message)
{
    if (!value) throw std::runtime_error(message);
}

void verifySwap(const NativeMesh &mesh, modmesh::Scene &scene)
{
    const auto built = modmesh::buildSectionsForMesh(scene, mesh.name, views(mesh), mesh.ref);
    require(!built.empty(), "renamed FBX was rejected: " + mesh.name);
    size_t visible = 0, triangles = 0, maxPalette = 0;
    for (size_t si = 0; si < built.size(); ++si) {
        require(bool(built[si]), "uncovered host section retained");
        const modmesh::BuiltSection &b = *built[si];
        require(b.templateSection < mesh.sections.size(), "invalid section template");
        require(!b.permanentTint && !b.venomEddieBlankTint, "unrequested color grading");
        if (b.hide) continue;
        ++visible;
        require(!b.keepGeometry, "source mesh lost to native geometry fallback");
        require(b.sourceMeshName == "usm_blacksuit000", "unrelated FBX mesh leaked into replacement");
        require(b.sourceSection == int(si), "native source material slot lost");
        require(!b.keepOriginalPalette && !b.palette.empty(), "cross-rig native palette not remapped");
        maxPalette = std::max(maxPalette, b.palette.size());
        require(si < 10, "extra source character imported");
        triangles += b.indices.size()/3;
        for (uint16_t bone : b.palette)
            require(bone < mesh.ref.nbones, "out-of-range target bone");
        for (uint32_t index : b.indices)
            require(index < b.vertices.size()/16, "out-of-range triangle index");
        for (size_t v = 0; v < b.vertices.size(); v += 16) {
            float sum = 0;
            for (int k = 0; k < 16; ++k)
                require(std::isfinite(b.vertices[v+k]), "non-finite vertex");
            for (int k = 0; k < 4; ++k) {
                const float weight = b.vertices[v+12+k];
                if (weight > 0)
                    require(b.vertices[v+8+k] >= 0
                            && b.vertices[v+8+k] < b.palette.size(), "invalid blend slot");
                sum += weight;
            }
            require(std::fabs(sum-1.f) < 1e-4f, "unnormalized skin weights");
        }
    }
    require(visible == 10, "full 10-slot black suit was not imported");
    require(triangles >= 7500 && triangles <= 7536, "full black suit triangle count lost");
    require(built[9]->indices.size() / 3 >= 1000, "white eyes/chest spider geometry lost");
    require(maxPalette <= 20, "retail vertex shader palette budget exceeded");
    std::printf("PASS %s: %zu source sections, %zu triangles, max palette %zu, original material slots retained\n",
                mesh.name.c_str(), visible, triangles, maxPalette);
}

} // namespace

int main(int argc, char **argv)
{
    if (argc != 3) {
        std::fprintf(stderr, "usage: mod_mesh_swap_probe <USM_BLACKSUIT.fbx> <VENOM.PCMESH>\n");
        return 2;
    }
    try {
        std::ifstream file(argv[1], std::ios::binary);
        std::vector<uint8_t> bytes(std::istreambuf_iterator<char>(file), {});
        require(!bytes.empty(), "cannot read FBX");
        const auto source = modmesh::loadScene("VENOM.fbx", bytes.data(), bytes.size());
        require(bool(source), "FBX parser failed");
        require(modmesh::primaryMeshName(*source) == "usm_blacksuit000", "source identity was replaced by filename");
        require(!source->cfg.anim && source->anims.empty(), "FBX overrides native gameplay animations by default");
        auto meshes = readPcmesh(argv[2]);
        for (auto &mesh : meshes) {
            mesh.ref.boneNames = modmesh::nativeBoneNames(mesh.name, mesh.ref.nbones, mesh.ref.bonePos);
            require(mesh.ref.boneNames.size() == 60, "native Venom skeleton names unavailable");
            require(mesh.ref.boneNames[11] == "l_hand" && mesh.ref.boneNames[33] == "r_hand"
                    && mesh.ref.boneNames[52] == "l_thigh", "incorrect native limb mapping");
            verifySwap(mesh, *source);
        }
        NativeMesh tiny = named(meshes, "venom000");
        tiny.sections.resize(2);
        verifySwap(tiny, *source);
        const NativeMesh &native = named(meshes, "venom000");
        for (const char *aux : {"venom_tentacles", "venom_spider000", "venom_eddie000", "fx_venomclaw"})
            require(modmesh::buildSectionsForMesh(*source, aux, views(native), native.ref).empty(),
                    std::string("native attack/auxiliary mesh replaced: ") + aux);
        const auto originalFilename = modmesh::loadScene("USM_BLACKSUIT.fbx", bytes.data(), bytes.size());
        require(bool(originalFilename), "source filename FBX parser failed");
        NativeMesh configured = native;
        configured.ref.targetFileName = "VENOM";
        verifySwap(configured, *originalFilename);
        for (const char *aux : {"venom_tentacles", "venom_spider000", "fx_venomclaw"})
            require(modmesh::buildSectionsForMesh(*originalFilename, aux, views(configured),
                                                 configured.ref).empty(),
                    std::string("configured swap escaped its target family: ") + aux);
        std::vector<float> changed = native.ref.bonePos;
        changed[0] += 10.f;
        require(modmesh::nativeBoneNames("venom000", 60, changed).empty(), "unknown skeleton layout trusted");
        std::printf("PASS renamed and explicitly mapped source filenames, overflow materials, auxiliary mesh scope, native animation policy, skeleton validation\n");
        return 0;
    } catch (const std::exception &error) {
        std::fprintf(stderr, "FAIL %s\n", error.what());
        return 1;
    }
}
