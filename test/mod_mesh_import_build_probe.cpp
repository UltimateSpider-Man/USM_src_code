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

std::vector<std::pair<int, float>> skeletonWeights(const float *row,
                                                   const NativeSection &section)
{
    std::vector<std::pair<int, float>> result;
    for (int lane = 0; lane < 4; ++lane) {
        const int slot = int(row[8 + lane]);
        const float weight = row[12 + lane];
        if (!(weight > 0.f)) continue;
        if (slot < 0 || std::size_t(slot) >= section.palette.size())
            return {{-9999, weight}};
        result.emplace_back(int(section.palette[std::size_t(slot)]), weight);
    }
    std::sort(result.begin(), result.end());
    return result;
}

bool sameWeights(const std::vector<std::pair<int, float>> &a,
                 const std::vector<std::pair<int, float>> &b)
{
    if (a.size() != b.size()) return false;
    for (std::size_t i = 0; i < a.size(); ++i)
        if (a[i].first != b[i].first || std::fabs(a[i].second - b[i].second) > 1e-5f)
            return false;
    return true;
}

int verifyLod0(modmesh::Scene &scene, const NativeMesh &mesh)
{
    const auto originalViews = views(mesh);
    const auto built = modmesh::buildSectionsForMesh(scene, mesh.name, originalViews, mesh.ref);
    static const std::uint32_t expectedVertices[] =
        {2882, 557, 186, 169, 122, 103, 93, 122, 50, 807};
    static const std::uint32_t expectedIndices[] =
        {14424, 2193, 684, 654, 474, 306, 321, 411, 84, 3042};

    int failures = 0;
    if (built.size() != mesh.sections.size()) {
        std::printf("FAIL lod0 output sections=%zu expected=%zu\n", built.size(), mesh.sections.size());
        return 1;
    }

    for (std::size_t si = 0; si < built.size(); ++si) {
        if (!built[si]) {
            std::printf("FAIL lod0 sec%zu not rebuilt\n", si);
            ++failures;
            continue;
        }
        const modmesh::BuiltSection &section = *built[si];
        const NativeSection &original = mesh.sections[si];
        const std::uint32_t nvertices = std::uint32_t(section.vertices.size() / 16);
        bool ok = nvertices == expectedVertices[si]
               && section.indices.size() == expectedIndices[si]
               && section.keepOriginalPalette && section.palette.empty()
               && !section.keepGeometry && !section.hide;

        std::size_t badIndices = 0, badSlots = 0, badSums = 0, weightMismatches = 0;
        for (std::uint32_t index : section.indices)
            if (index >= nvertices) ++badIndices;

        for (std::size_t v = 0; v < nvertices; ++v) {
            const float *row = section.vertices.data() + v * 16;
            float sum = 0.f;
            for (int lane = 0; lane < 4; ++lane) {
                const int slot = int(row[8 + lane]);
                const float weight = row[12 + lane];
                if (!std::isfinite(weight) || !std::isfinite(row[8 + lane])) {
                    ++badSlots;
                } else if (weight > 0.f) {
                    sum += weight;
                    if (slot < 0 || std::size_t(slot) >= original.palette.size()) ++badSlots;
                }
            }
            if (std::fabs(sum - 1.f) > 1e-4f) ++badSums;

            const auto got = skeletonWeights(row, original);
            bool matched = false;
            for (std::size_t ov = 0; ov < original.vertices.size() / 16; ++ov) {
                const float *candidate = original.vertices.data() + ov * 16;
                const float dx = row[0] - candidate[0], dy = row[1] - candidate[1],
                            dz = row[2] - candidate[2];
                if (dx * dx + dy * dy + dz * dz > 1e-8f) continue;
                if (std::fabs(row[6] - candidate[6]) > 0.002f
                    || std::fabs(row[7] - candidate[7]) > 0.002f) continue;
                if (sameWeights(got, skeletonWeights(candidate, original))) {
                    matched = true;
                    break;
                }
            }
            if (!matched) ++weightMismatches;
        }

        ok = ok && badIndices == 0 && badSlots == 0 && badSums == 0 && weightMismatches == 0;
        std::printf("%s lod0 sec%zu verts=%u/%u indices=%zu/%u palette=native(%zu) "
                    "bad_index=%zu bad_slot=%zu bad_sum=%zu weight_mismatch=%zu\n",
                    ok ? "PASS" : "FAIL", si, nvertices, expectedVertices[si],
                    section.indices.size(), expectedIndices[si], original.palette.size(),
                    badIndices, badSlots, badSums, weightMismatches);
        if (!ok) ++failures;
    }
    return failures;
}

void logLine(const char *message)
{
    if (message != nullptr) std::fprintf(stderr, "%s\n", message);
}

} // namespace

int main(int argc, char **argv)
{
    if (argc < 3 || argc > 4) {
        std::fprintf(stderr, "usage: mod_mesh_import_build_probe <USM_BLACKSUIT.fbx> "
                             "<USM_BLACKSUIT.PCMESH> [VENOM.PCMESH]\n");
        return 2;
    }

    try {
        std::ifstream fbxFile(argv[1], std::ios::binary);
        std::vector<std::uint8_t> fbx(std::istreambuf_iterator<char>(fbxFile), {});
        if (fbx.empty()) throw std::runtime_error("cannot read FBX");

        modmesh::setLog(logLine);
        const auto scene = modmesh::loadScene(argv[1], fbx.data(), fbx.size());
        if (!scene) throw std::runtime_error("FBX parse failed");
        std::printf("CONFIG weld=%d skin=%d roundtrip=%.6g source=%s\n",
                    int(scene->cfg.weld), scene->cfg.skin, scene->cfg.roundtripEps,
                    scene->srcName.c_str());
        int failures = 0;
        if (!scene->cfg.weld || scene->cfg.skin != 2 || scene->cfg.roundtripEps != 0.0) {
            std::printf("FAIL live sidecar is not weld=1, skin=transfer, roundtrip=0\n");
            ++failures;
        }

        const auto blackSuitMeshes = readPcmesh(argv[2]);
        failures += verifyLod0(*scene, named(blackSuitMeshes, "usm_blacksuit000"));
        for (const char *lod : {"usm_blacksuit001", "usm_blacksuit002"}) {
            const NativeMesh &mesh = named(blackSuitMeshes, lod);
            const auto built = modmesh::buildSectionsForMesh(*scene, mesh.name, views(mesh), mesh.ref);
            const bool ok = built.empty();
            std::printf("%s %s output_sections=%zu (native LOD retained)\n",
                        ok ? "PASS" : "FAIL", lod, built.size());
            failures += !ok;
        }

        if (argc == 4) {
            const auto venomMeshes = readPcmesh(argv[3]);
            for (const NativeMesh &mesh : venomMeshes) {
                const std::string norm = modmesh::Scene::normName(mesh.name);
                if (norm.rfind("venom", 0) != 0) continue;
                const auto built = modmesh::buildSectionsForMesh(*scene, mesh.name, views(mesh), mesh.ref);
                const bool ok = built.empty();
                std::printf("%s %s output_sections=%zu (unrelated host retained)\n",
                            ok ? "PASS" : "FAIL", mesh.name.c_str(), built.size());
                failures += !ok;
            }
        } else {
            const NativeMesh &dummy = named(blackSuitMeshes, "usm_blacksuit000");
            for (const char *name : {"venom000", "venom001"}) {
                const auto built = modmesh::buildSectionsForMesh(*scene, name, views(dummy), dummy.ref);
                const bool ok = built.empty();
                std::printf("%s %s output_sections=%zu (scope guard)\n",
                            ok ? "PASS" : "FAIL", name, built.size());
                failures += !ok;
            }
        }

        std::printf("RESULT failures=%d\n", failures);
        return failures == 0 ? 0 : 1;
    } catch (const std::exception &e) {
        std::fprintf(stderr, "probe error: %s\n", e.what());
        return 1;
    }
}
