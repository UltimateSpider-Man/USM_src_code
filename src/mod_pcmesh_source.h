#pragma once

// Pristine PC PCMESH metadata used by FBX swaps. All references remain file
// offsets: parsing never rebases, registers, or modifies an engine resource.
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include "mod_mesh_static_layout.h"

namespace modmesh { namespace pcmeshsource {

inline std::string normalized(std::string value)
{
    for (char &c : value) c = char(std::tolower(static_cast<unsigned char>(c)));
    return value;
}

struct Material {
    uint32_t offset = 0, size = 0;
    uint32_t nameOffset = 0, shaderOffset = 0, shaderHash = 0;
    std::string name, shaderName, diffuseName, detailName;
    // Only known character layouts have every rebased pointer validated.
    bool characterBindable = false;
    bool textureBindable = false;
    modmesh::staticlayout::MaterialLayout layout;
};

struct Section {
    uint32_t offset = 0, materialOffset = 0;
    uint32_t vertexOffset = 0, vertexCount = 0, vertexBytes = 0, stride = 0;
    uint32_t indexOffset = 0, indexCount = 0, primitiveType = 0;
    std::string materialName;
    std::string vertexDefinitionName;
    std::vector<uint16_t> palette;
};

struct Mesh {
    struct LOD {
        uint32_t nameOffset = 0;
        // Zero means a valid named reference outside this PCMESH image.
        uint32_t meshOffset = 0;
        std::string name;
        float range = 0.f;
    };
    uint32_t offset = 0, nbones = 0;
    std::string name;
    std::vector<float> bonePos, boneMatrices;
    float sphereCenter[3] = {0.f, 0.f, 0.f};
    float sphereRadius = 0.f;
    std::vector<Section> sections;
    std::vector<LOD> lods;
};

class Source {
public:
    std::vector<uint8_t> bytes;
    std::vector<Mesh> meshes;
    std::vector<Material> materials;

    const Mesh *findMesh(const std::string &name) const
    {
        const auto key = normalized(name);
        for (const auto &mesh : meshes)
            if (normalized(mesh.name) == key) return &mesh;
        return nullptr;
    }

    const Material *findMaterial(uint32_t offset) const
    {
        for (const auto &material : materials)
            if (material.offset == offset) return &material;
        return nullptr;
    }

    // Selection starts only after the caller matches an actual mesh-file
    // resource. Preserve numeric characters in its name: foo2 matches foo2000,
    // while foo does not claim foo01 or foo2. Serialized LOD links may use an
    // entirely different family, but only links resolved inside this file join.
    std::vector<std::string> replacementMeshNames(std::string requestedFilename) const
    {
        if (meshes.empty()) return {};
        const size_t slash = requestedFilename.find_last_of("/\\");
        if (slash != std::string::npos) requestedFilename.erase(0, slash + 1);
        requestedFilename = normalized(std::move(requestedFilename));
        for (const char *extension : {".pcmesh", ".xbmesh", ".fbx", ".obj", ".glb", ".gltf"}) {
            const size_t length = std::strlen(extension);
            if (requestedFilename.size() > length
                && requestedFilename.compare(requestedFilename.size() - length, length, extension) == 0) {
                requestedFilename.resize(requestedFilename.size() - length);
                break;
            }
        }
        const auto belongs = [](const std::string &mesh, const std::string &family) {
            if (mesh == family) return true;
            if (mesh.size() != family.size() + 3 || mesh.compare(0, family.size(), family)) return false;
            for (size_t i = family.size(); i < mesh.size(); ++i)
                if (mesh[i] < '0' || mesh[i] > '9') return false;
            return true;
        };
        const auto triangleCount = [](const Mesh &mesh) {
            uint64_t triangles = 0;
            for (const auto &section : mesh.sections) {
                const uint64_t count = section.indexCount ? section.indexCount : section.vertexCount;
                if (section.primitiveType == 4) triangles += count / 3;
                else if (section.primitiveType == 5 || section.primitiveType == 6)
                    triangles += count > 2 ? count - 2 : 0;
            }
            return triangles;
        };
        std::vector<size_t> selected;
        std::vector<bool> visited(meshes.size(), false);
        const auto selectFamily = [&](const std::string &family) {
            for (size_t i = 0; i < meshes.size(); ++i) {
                if (!visited[i] && belongs(normalized(meshes[i].name), family)) {
                    visited[i] = true;
                    selected.push_back(i);
                }
            }
        };
        selectFamily(requestedFilename);
        // Hero/boss gauge files name their private panel '<file>_back'. The
        // larger tick meshes are shared gauge pieces, not the file's primary.
        if (selected.empty()) selectFamily(requestedFilename + "_back");
        if (selected.empty()) {
            size_t primary = 0;
            uint64_t largest = triangleCount(meshes.front());
            for (size_t i = 1; i < meshes.size(); ++i) {
                const uint64_t size = triangleCount(meshes[i]);
                if (size > largest) { primary = i; largest = size; }
            }
            std::string family = normalized(meshes[primary].name);
            if (family.size() > 3) {
                bool suffix = true;
                for (size_t i = family.size() - 3; i < family.size(); ++i)
                    suffix = suffix && family[i] >= '0' && family[i] <= '9';
                if (suffix) family.resize(family.size() - 3);
            }
            selectFamily(family);
        }
        for (size_t next = 0; next < selected.size(); ++next) {
            for (const auto &lod : meshes[selected[next]].lods) {
                if (!lod.meshOffset) continue;
                for (size_t i = 0; i < meshes.size(); ++i) {
                    if (!visited[i] && meshes[i].offset == lod.meshOffset) {
                        visited[i] = true;
                        selected.push_back(i);
                        break;
                    }
                }
            }
        }
        std::vector<std::string> names;
        names.reserve(selected.size());
        for (size_t index : selected) names.push_back(meshes[index].name);
        return names;
    }

    bool parse(std::vector<uint8_t> image, std::string *why = nullptr)
    {
        Source parsed;
        parsed.bytes = std::move(image);
        try {
            parsed.read();
            *this = std::move(parsed);
            if (why) why->clear();
            return true;
        } catch (const std::exception &error) {
            bytes.clear(); meshes.clear(); materials.clear();
            if (why) *why = error.what();
            return false;
        }
    }

private:
    void range(size_t offset, size_t count) const
    {
        if (offset > bytes.size() || count > bytes.size() - offset)
            throw std::runtime_error("PCMESH reference outside file");
    }

    void array(uint32_t offset, uint32_t count, size_t stride) const
    {
        if (count && !offset) throw std::runtime_error("PCMESH array has null offset");
        if (stride && size_t(count) > bytes.size() / stride)
            throw std::runtime_error("PCMESH array count outside file");
        range(offset, size_t(count) * stride);
    }

    uint32_t u32(size_t offset) const
    {
        range(offset, 4);
        uint32_t result;
        std::memcpy(&result, bytes.data() + offset, 4);
        return result;
    }

    float f32(size_t offset) const
    {
        range(offset, 4);
        float result;
        std::memcpy(&result, bytes.data() + offset, 4);
        if (!std::isfinite(result)) throw std::runtime_error("PCMESH nonfinite geometry");
        return result;
    }

    std::string fixedString(uint32_t offset, bool optional = false) const
    {
        if (!offset) {
            if (optional) return {};
            throw std::runtime_error("PCMESH required string has null offset");
        }
        range(offset, 32);
        const char *text = reinterpret_cast<const char *>(bytes.data() + offset + 4);
        const char *end = static_cast<const char *>(std::memchr(text, 0, 28));
        if (!end) throw std::runtime_error("PCMESH fixed string is unterminated");
        return std::string(text, end);
    }

    void readMaterial(uint32_t offset, uint32_t size)
    {
        if (size < 0x18) throw std::runtime_error("PCMESH material header is truncated");
        Material material;
        material.offset = offset;
        material.size = size;
        material.nameOffset = u32(offset);
        material.shaderOffset = u32(size_t(offset) + 4);
        material.name = fixedString(material.nameOffset);
        material.shaderName = fixedString(material.shaderOffset);
        material.shaderHash = u32(material.shaderOffset);
        const auto shader = normalized(material.shaderName);
        if (modmesh::staticlayout::materialLayout(shader, material.layout)) {
            if (size < material.layout.minimumSize)
                throw std::runtime_error("PCMESH material layout is truncated");
            material.diffuseName = fixedString(u32(size_t(offset) + material.layout.textureName), true);
            if (u32(size_t(offset) + material.layout.texture))
                throw std::runtime_error("PCMESH material texture is already bound");
            material.textureBindable = true;
        }
        // Retail USPersonMorphable registers the same USPerson shader vtable:
        // RebaseMaterial 0x410C60 and BindMaterial 0x410C90 use these same fields.
        if (shader == "usperson" || shader == "uspersonsolid" || shader == "uspersonmorphable") {
            const uint32_t required = shader == "uspersonsolid" ? 0x58 : 0x50;
            if (size < required) throw std::runtime_error("PCMESH character material is truncated");
            // These shader families rebase two tlFixedString texture names.
            material.diffuseName = fixedString(u32(size_t(offset) + 0x18), true);
            material.detailName = fixedString(u32(size_t(offset) + 0x20), true);
            if (u32(size_t(offset) + 0x1c) || u32(size_t(offset) + 0x24))
                throw std::runtime_error("PCMESH character material is already bound");
            for (size_t component = 0; component < 4; ++component)
                (void)f32(size_t(offset) + 0x28 + component * 4);
            material.characterBindable = true;
        }
        materials.push_back(std::move(material));
    }

    void readMesh(uint32_t offset, uint32_t size)
    {
        if (size < 0x40) throw std::runtime_error("PCMESH mesh header is truncated");
        Mesh mesh;
        mesh.offset = offset;
        mesh.name = fixedString(u32(offset));
        mesh.nbones = u32(size_t(offset) + 0x10);
        if (mesh.nbones > 65536) throw std::runtime_error("PCMESH bone count exceeds palette range");
        const uint32_t bones = u32(size_t(offset) + 0x14);
        array(bones, mesh.nbones, 64);
        mesh.bonePos.reserve(size_t(mesh.nbones) * 3);
        mesh.boneMatrices.reserve(size_t(mesh.nbones) * 16);
        for (uint32_t bone = 0; bone < mesh.nbones; ++bone) {
            const size_t base = size_t(bones) + size_t(bone) * 64;
            for (size_t element = 0; element < 16; ++element)
                mesh.boneMatrices.push_back(f32(base + element * 4));
            for (size_t axis = 0; axis < 3; ++axis)
                mesh.bonePos.push_back(f32(base + 48 + axis * 4));
        }
        for (size_t axis = 0; axis < 3; ++axis)
            mesh.sphereCenter[axis] = f32(size_t(offset) + 0x20 + axis * 4);
        mesh.sphereRadius = f32(size_t(offset) + 0x30);
        const uint32_t lodCount = u32(size_t(offset) + 0x18);
        const uint32_t lodTable = u32(size_t(offset) + 0x1c);
        if (lodCount > 1024) throw std::runtime_error("PCMESH LOD count exceeds limit");
        array(lodTable, lodCount, 8);
        mesh.lods.reserve(lodCount);
        for (uint32_t index = 0; index < lodCount; ++index) {
            const size_t entry = size_t(lodTable) + size_t(index) * 8;
            Mesh::LOD lod;
            lod.nameOffset = u32(entry);
            lod.name = fixedString(lod.nameOffset);
            lod.range = f32(entry + 4);
            if (lod.range < 0) throw std::runtime_error("PCMESH negative LOD range");
            mesh.lods.push_back(std::move(lod));
        }
        const uint32_t count = u32(size_t(offset) + 8);
        const uint32_t table = u32(size_t(offset) + 12);
        array(table, count, 8);
        mesh.sections.reserve(count);
        for (uint32_t index = 0; index < count; ++index) {
            Section section;
            section.offset = u32(size_t(table) + size_t(index) * 8 + 4);
            const size_t base = section.offset;
            if (!base) throw std::runtime_error("PCMESH section has null offset");
            range(base, 0x60);
            section.materialName = fixedString(u32(base));
            const uint32_t definition = u32(base + 0x54);
            if (definition) {
                range(definition, 8);
                section.vertexDefinitionName = fixedString(u32(definition));
            }
            for (const auto &material : materials)
                if (normalized(material.name) == normalized(section.materialName)) {
                    section.materialOffset = material.offset;
                    break;
                }
            // A material can live in a separate material file; offset 0 means
            // this source cannot independently restore that section's material.
            section.primitiveType = u32(base + 0x28);
            section.indexCount = u32(base + 0x2c);
            section.indexOffset = u32(base + 0x30);
            array(section.indexOffset, section.indexCount, 2);
            section.vertexCount = u32(base + 0x38);
            section.vertexOffset = u32(base + 0x3c);
            section.vertexBytes = u32(base + 0x40);
            section.stride = u32(base + 0x48);
            if (section.vertexCount && !section.stride)
                throw std::runtime_error("PCMESH vertex stride is zero");
            array(section.vertexOffset, section.vertexCount, section.stride);
            if (size_t(section.vertexCount) * section.stride > section.vertexBytes)
                throw std::runtime_error("PCMESH vertex buffer is truncated");
            range(section.vertexOffset, section.vertexBytes);
            const uint32_t paletteCount = u32(base + 8);
            const uint32_t paletteOffset = u32(base + 12);
            array(paletteOffset, paletteCount, 2);
            section.palette.resize(paletteCount);
            if (paletteCount)
                std::memcpy(section.palette.data(), bytes.data() + paletteOffset,
                            size_t(paletteCount) * 2);
            for (const auto bone : section.palette)
                if (bone >= mesh.nbones)
                    throw std::runtime_error("PCMESH palette index outside skeleton");
            mesh.sections.push_back(std::move(section));
        }
        meshes.push_back(std::move(mesh));
    }

    void read()
    {
        range(0, 20);
        if (std::memcmp(bytes.data(), "PCM ", 4) || u32(4) != 0x601)
            throw std::runtime_error("expected PC PCMESH version 0x601");
        if (u32(16)) throw std::runtime_error("PCMESH source must be pristine/unbound");
        const uint32_t count = u32(8), directory = u32(12);
        array(directory, count, 12);
        // Resolve materials first, independent of the directory entry order.
        for (unsigned pass = 1; pass <= 2; ++pass) {
            for (uint32_t index = 0; index < count; ++index) {
                const size_t entry = size_t(directory) + size_t(index) * 12;
                const uint32_t size = uint32_t(bytes[entry])
                    | (uint32_t(bytes[entry + 1]) << 8)
                    | (uint32_t(bytes[entry + 2]) << 16);
                const uint32_t offset = u32(entry + 4);
                if (!offset) throw std::runtime_error("PCMESH directory has null object offset");
                range(offset, size);
                if (bytes[entry + 3] != pass) continue;
                if (pass == 1) readMaterial(offset, size);
                else readMesh(offset, size);
            }
        }
        if (meshes.empty()) throw std::runtime_error("PCMESH source has no meshes");
        // LODs serialize tlFixedString references, not nglMesh pointers. Some
        // stock assets intentionally refer to another file; retain those names
        // as external links instead of broadening the selected file's scope.
        for (auto &mesh : meshes) {
            for (auto &lod : mesh.lods) {
                for (const auto &candidate : meshes) {
                    if (normalized(candidate.name) == normalized(lod.name)) {
                        if (lod.meshOffset && lod.meshOffset != candidate.offset)
                            throw std::runtime_error("PCMESH LOD name is ambiguous");
                        lod.meshOffset = candidate.offset;
                    }
                }
            }
        }
    }
};

}} // namespace modmesh::pcmeshsource
