// Private custom FBX materials and images belong to the loaded mesh file.
// They are not inserted in the global hash directory: two FBXs may both
// contain material0000 without sharing an image or repainting one another.
#include "mod_mesh_texture_source.h"
#include "mod_mesh_static_layout.h"

struct ModCustomMaterial {
    nglMaterialBase *material = nullptr;
    size_t size = 0;
    tlFixedString name, diffuseName;
};
struct ModCustomMaterialFile {
    std::map<std::string, nglTexture *> textures;
    std::map<std::string, std::unique_ptr<ModCustomMaterial>> materials;
};
static std::map<nglMeshFile *, ModCustomMaterialFile> modCustomMaterialFiles;

static nglMaterialBase *modCustomBodyMaterial(nglMeshFile *owner, nglMeshSection *section)
{
    if (!owner || !section || section->m_stride != 64) return nullptr;
    size_t count = 0;
    for (auto *material = owner->FirstMaterial; material && count++ < 1024;
         material = material->NextMaterial) {
        if (!material->Name || !material->m_shader) continue;
        const auto name = transformToLower(material->Name->to_string());
        if (name.find("body") == std::string::npos || name.find("mouth") != std::string::npos)
            continue;
        if (material->m_shader->CheckVertexDefVersion(section)) return material;
    }
    return nullptr;
}

static std::string modCustomImageKey(const std::filesystem::path &path)
{
    std::error_code ec;
    auto canonical = std::filesystem::weakly_canonical(path, ec);
    return transformToLower((ec ? path.lexically_normal() : canonical).generic_string());
}

static void modReleaseCustomMaterials(nglMeshFile *owner)
{
    const auto found = modCustomMaterialFiles.find(owner);
    if (found == modCustomMaterialFiles.end()) return;
    // These clones did not call BindMaterial, so native material-release must
    // not decrement the shared template's texture references a second time.
    for (auto &entry : found->second.materials)
        if (entry.second->material) tlMemFree(entry.second->material);
    for (auto &entry : found->second.textures)
        if (entry.second) nglDestroyTexture(entry.second);
    modCustomMaterialFiles.erase(found);
}

static nglTexture *modCustomDiffuse(ModCustomMaterialFile &file,
                                   const modmesh::BuiltSection &built,
                                   const std::filesystem::path &modPath,
                                   std::string &selected)
{
    // Some exports retain material names but omit every Texture/Video link.
    // Prefer explicit FBX images, then a unique local material_map_Kd image.
    auto candidates = built.textureCandidates;
    std::filesystem::path materialImage;
    if (modmesh::texturesource::resolveMaterialImage(modPath, built.sourceMaterialName, materialImage)) {
        const auto stem = materialImage.stem().string();
        if (std::find(candidates.begin(), candidates.end(), stem) == candidates.end())
            candidates.push_back(stem);
    }
    for (const auto &candidate : candidates) {
        std::filesystem::path imagePath;
        const auto reference = built.texRelPath.find(candidate);
        const std::string authored = reference == built.texRelPath.end() ? "" : reference->second;
        std::string why, key;
        std::vector<uint8_t> bytes;
        bool forceD3DX = false;
        if (modmesh::texturesource::resolveLocalImage(modPath, authored, candidate, imagePath, &why)) {
            key = modCustomImageKey(imagePath);
            forceD3DX = true;
        } else {
            const auto embedded = built.embeddedTex.find(candidate);
            if (embedded == built.embeddedTex.end() || !embedded->second || embedded->second->empty())
                continue;
            key = modCustomImageKey(modPath) + "|embedded|" + candidate;
            bytes = *embedded->second;
        }
        if (const auto cached = file.textures.find(key); cached != file.textures.end()) {
            if (cached->second) { selected = candidate; return cached->second; }
            continue;
        }
        if (!imagePath.empty()) {
            std::error_code ec;
            const auto size = std::filesystem::file_size(imagePath, ec);
            if (ec || !size || size > 128u * 1024u * 1024u || !modReadWholeFile(imagePath, bytes)) {
                sp_log("[modmesh] custom texture unreadable: %s", imagePath.string().c_str());
                file.textures.emplace(key, nullptr);
                continue;
            }
        }
        auto *texture = modConstructTexFromBytes(candidate, bytes.data(), bytes.size(), forceD3DX);
        file.textures.emplace(key, texture);
        if (!texture) {
            sp_log("[modmesh] custom texture decode failed: %s", key.c_str());
            continue;
        }
        if (texture->DXTexture) {
            D3DSURFACE_DESC description{};
            if (SUCCEEDED(texture->DXTexture->lpVtbl->GetLevelDesc(texture->DXTexture, 0, &description))) {
                texture->m_width = description.Width;
                texture->m_height = description.Height;
                texture->m_d3d_format = description.Format;
                texture->m_numLevel = texture->DXTexture->lpVtbl->GetLevelCount(texture->DXTexture);
            }
        }
        selected = candidate;
        sp_log("[modmesh] custom diffuse %s <- %s (%u x %u)", candidate.c_str(),
               key.c_str(), texture->m_width, texture->m_height);
        return texture;
    }
    return nullptr;
}

static size_t modCustomMaterialSize(const nglMaterialBase *material)
{
    if (!material || !material->File || !material->File->FileBuf.Buf) return 0;
    const auto *header = reinterpret_cast<const nglMeshFileHeader *>(material->File->FileBuf.Buf);
    if (std::memcmp(header->Tag, "PCM ", 4) || header->Version != 0x601
        || !header->DirectoryEntries || header->NDirectoryEntries > 1048576) return 0;
    for (uint32_t i = 0; i < header->NDirectoryEntries; ++i) {
        const auto &entry = header->DirectoryEntries[i];
        if (entry.field_3 != TypeDirectoryEntry::MATERIAL || entry.field_4.Material != material) continue;
        const size_t size = uint8_t(entry.field_0) | (uint32_t(uint8_t(entry.field_1)) << 8)
            | (uint32_t(uint8_t(entry.field_2)) << 16);
        return size >= 0x18 && size <= 4096 ? size : 0;
    }
    // A previously cloned section can be reused without treating a generic
    // nglMaterialBase sizeof as the true shader-specific allocation length.
    for (const auto &file : modCustomMaterialFiles)
        for (const auto &entry : file.second.materials)
            if (entry.second && entry.second->material == material) return entry.second->size;
    return 0;
}

static void modRetargetCustomMaterial(nglMeshSection *section,
                                      const modmesh::BuiltSection &built,
                                      const std::filesystem::path &modPath,
                                      int sectionIndex, nglMesh *mesh)
{
    if (!mesh || !mesh->File || !section || !section->Material || !section->Material->m_shader) return;
    modmesh::staticlayout::MaterialLayout layout;
    const std::string shader = section->Material->m_shader->GetName().to_string();
    const size_t size = modCustomMaterialSize(section->Material);
    if (!modmesh::staticlayout::materialLayout(shader, layout) || size < layout.minimumSize) {
        sp_log("[modmesh] custom material %s: unsupported/truncated %s layout (%u bytes), native material retained",
               built.sourceMaterialName.c_str(), shader.c_str(), unsigned(size));
        return;
    }
    auto &file = modCustomMaterialFiles[mesh->File];
    modMeshBufferOwners[mesh->File->FileBuf.Buf] = mesh->File;
    std::string selected;
    auto *texture = built.forceWhite ? modWhiteTexture() : modCustomDiffuse(file, built, modPath, selected);
    // A pin on existing static geometry must not turn its native material
    // white when the requested image is unavailable. White fallback belongs
    // only to untextured custom FBX material slots (or explicit white=).
    if (!texture && !built.customMaterial && !built.forceWhite) return;
    const bool haveDiffuse = texture != nullptr;
    if (!texture) texture = modWhiteTexture();
    if (!texture) return;
    if (selected.empty()) selected = "nglwhite";
    const std::string key = modCustomImageKey(modPath) + "|" + built.sourceMaterialName
        + "|" + selected + "|" + std::to_string(reinterpret_cast<uintptr_t>(section->Material));
    auto &record = file.materials[key];
    if (!record) {
        auto candidate = std::make_unique<ModCustomMaterial>();
        candidate->material = static_cast<nglMaterialBase *>(tlMemAlloc(uint32_t(size), 8, 0x1000000u));
        if (!candidate->material) { file.materials.erase(key); return; }
        std::memcpy(candidate->material, section->Material, size);
        candidate->size = size;
        candidate->name = tlFixedString{built.sourceMaterialName.empty() ? "custom_fbx" : built.sourceMaterialName.c_str()};
        candidate->diffuseName = tlFixedString{selected.c_str()};
        candidate->material->Name = &candidate->name;
        candidate->material->File = mesh->File;
        candidate->material->NextMaterial = nullptr;
        // A connected color image supplies the base color. FBX exporters often
        // retain a viewport/fallback DiffuseColor (green in Venom 2018) even
        // though the material's image input replaces it. Untextured slots use
        // that authored fallback; they never inherit the target hero's skin.
        float color[4];
        for (size_t channel = 0; channel < 4; ++channel)
            color[channel] = haveDiffuse ? 1.f : built.customDiffuse[channel];
        // Static shaders keep their native blend/UV/lighting fields. Only
        // USPerson has the float4 tint at +0x28; frontend stores flags there.
        if (!modmesh::staticlayout::retargetMaterial(candidate->material, size, layout,
                uint32_t(reinterpret_cast<uintptr_t>(&candidate->diffuseName)),
                uint32_t(reinterpret_cast<uintptr_t>(texture)), color,
                built.customMaterial)) {
            tlMemFree(candidate->material);
            file.materials.erase(key);
            return;
        }
        record = std::move(candidate);
    }
    section->Material = record->material;
    sp_log("[modmesh] custom material sec%u %s -> %s%s (shader %s, %u bytes)", unsigned(sectionIndex),
           built.sourceMaterialName.c_str(), selected.c_str(), haveDiffuse ? "" : " (untextured fallback)",
           shader.c_str(), unsigned(size));
}
