// Included by ngl.cpp after the engine types and the standalone importers.
// Native files supply target palettes and source materials, never animations.
static bool modReadNativePcmesh(const std::string &stem,
                                const std::filesystem::path &hint,
                                std::vector<uint8_t> &bytes)
{
    std::filesystem::path path;
    if (modResolveFile(stem + ".pcmesh", path, hint)
        && modReadWholeFile(path, bytes) && modPCMESHImageUsable(bytes.data(), bytes.size()))
        return true;

    // Reference-only extracted meshes live outside mod discovery, so reading
    // their declarations/names cannot accidentally install a raw mesh override.
    const auto wanted = transformToLower(std::filesystem::path(stem).filename().string() + ".pcmesh");
    for (const auto &root : modRootDirs()) {
        const auto directory = root.parent_path() / "mesh_sources" / "pcmesh";
        std::error_code ec;
        for (std::filesystem::directory_iterator it(directory, ec), end;
             it != end && !ec; it.increment(ec)) {
            if (it->is_regular_file(ec)
                && transformToLower(it->path().filename().string()) == wanted
                && modReadWholeFile(it->path(), bytes)
                && modPCMESHImageUsable(bytes.data(), bytes.size())) return true;
        }
    }

    if (!modResolveFile("pcmesh.zip", path, hint))
        return false;
    static std::map<std::string, std::unique_ptr<modmesh::pcmeshzip::Archive>> archives;
    auto &archive = archives[path.string()];
    std::string why;
    if (!archive) {
        auto candidate = std::make_unique<modmesh::pcmeshzip::Archive>();
        if (!candidate->open(path.string(), &why)) {
            sp_log("[modmesh] PCMESH archive rejected: %s", why.c_str());
            return false;
        }
        archive = std::move(candidate);
    }
    if (!archive->extract(stem, bytes, nullptr, &why)
        || !modPCMESHImageUsable(bytes.data(), bytes.size())) {
        sp_log("[modmesh] native mesh %s unavailable: %s", stem.c_str(), why.c_str());
        return false;
    }
    return true;
}

static std::string modNativeFamily(std::string name)
{
    while (!name.empty() && std::isdigit(static_cast<unsigned char>(name.back())))
        name.pop_back();
    return name;
}

// Private serialized images retain every shader-specific material field. The
// source mesh is never registered, and its skeleton never replaces the host's.
struct ModNativeMaterialFile {
    modmesh::pcmeshsource::Source source;
    std::filesystem::path assetRoot;
    std::vector<uint8_t> rebound;
    std::map<uint32_t, nglMaterialBase *> materials;
    std::map<std::string, nglTexture *> textures;
    std::map<nglMaterialBase *, std::pair<nglTexture *, nglTexture *>> originalBindings;
};
static std::map<nglMeshFile *, std::map<std::string, std::unique_ptr<ModNativeMaterialFile>>>
    modNativeMaterialFiles;
static std::map<char *, nglMeshFile *> modMeshBufferOwners;
static std::set<char *> modOwnedMeshBuffers;
static std::map<nglMeshFile *, std::set<nglMaterialBase *>> modFileReplacedMaterials;
using ModSectionEntry = std::remove_reference_t<decltype(std::declval<nglMesh>().Sections[0])>;
struct ModExpandedMesh {
    std::vector<ModSectionEntry> table;
    std::vector<nglMeshSection> sections;
    std::vector<nglVertexDef> definitions;
};
static std::map<nglMeshFile *, std::vector<std::unique_ptr<ModExpandedMesh>>> modExpandedMeshes;

// A source costume's textures need not exist in the target character's pack.
// The packed texture directory returns a fallback for unknown names and its
// Add/Release methods are no-ops. Own these textures alongside the donor image
// and assign them directly after BindMaterial has acquired its stock bindings.
static nglTexture *modSourceTexture(ModNativeMaterialFile &file, const std::string &stem)
{
    if (stem.empty()) return nullptr;
    if (auto found = file.textures.find(stem); found != file.textures.end())
        return found->second;
    auto *override = modFindTextureOverride(stem);
    std::vector<uint8_t> bytes;
    std::filesystem::path imagePath;
    if (override && readModFile(override, bytes) && !bytes.empty())
        imagePath = override->Path;
    else {
        // color=original can intentionally omit a same-stem DDS from the
        // global texture registry. The donor material still needs its own
        // source image when an explicit mapping keeps the FBX's filename.
        for (const char *extension : {".dds", ".tm2", ".png", ".tga", ".bmp", ".jpg"}) {
            if (modResolveFile(stem + extension, imagePath, file.assetRoot)
                && modReadWholeFile(imagePath, bytes) && !bytes.empty()) break;
            imagePath.clear();
        }
    }
    if (imagePath.empty()) return nullptr;
    const bool standard = modLooksD3DXImage(bytes.data(), bytes.size())
        || transformToLower(imagePath.extension().string()) == ".tga";
    auto *texture = modConstructTexFromBytes(stem, bytes.data(), bytes.size(), standard);
    if (!texture) return nullptr;
    file.textures.emplace(stem, texture);
    sp_log("[modmesh] source texture %s loaded from %s", stem.c_str(),
           imagePath.string().c_str());
    return texture;
}

static bool modExpandMeshSections(nglMesh *mesh,
                                  const std::vector<std::optional<modmesh::BuiltSection>> &built)
{
    if (built.size() <= mesh->NSections) return true;
    const size_t count = mesh->NSections;
    if (!count || built.size() > 1024) return false;
    auto extra = std::make_unique<ModExpandedMesh>();
    extra->table.assign(mesh->Sections, mesh->Sections + count);
    extra->table.resize(built.size());
    extra->sections.resize(built.size() - count);
    extra->definitions.resize(built.size() - count);
    for (size_t i = count; i < built.size(); ++i) {
        if (!built[i] || built[i]->templateSection >= count) return false;
        auto *source = mesh->Sections[built[i]->templateSection].Section;
        if (!source || !source->VertexDef) return false;
        auto &section = extra->sections[i - count];
        section = *source;
        section.m_indexBuffer = nullptr;
        section.field_3C.m_vertexBuffer = nullptr;
        extra->definitions[i - count] = *source->VertexDef;
        section.VertexDef = &extra->definitions[i - count];
        extra->table[i] = {1, &section};
    }
    for (auto &section : extra->sections) modExtraSections.insert(&section);
    mesh->Sections = extra->table.data();
    mesh->NSections = uint32_t(built.size());
    modExpandedMeshes[mesh->File].push_back(std::move(extra));
    modMeshBufferOwners[mesh->File->FileBuf.Buf] = mesh->File;
    return true;
}

bool modReleaseMeshFileBuffer(tlFileBuf *buffer, bool packed)
{
    if (!buffer) return false;
    auto found = modMeshBufferOwners.find(buffer->Buf);
    if (found != modMeshBufferOwners.end()
        && &found->second->FileBuf != buffer) {
        // A failed second-consumer copy can leave a borrowed view here.
        // Detach that view without destroying the live owner's allocations.
        *buffer = {};
        return true;
    }
    if (found != modMeshBufferOwners.end()) {
        modForgetRetargetFile(found->second);
        // File sections are released in place: retail drops their GPU buffers,
        // but never destroys the section or our separately allocated palette.
        // Their CPU snapshots must leave the registry before the pack storage
        // can be reused by another resource.
        for (auto *mesh = found->second->FirstMesh; mesh; mesh = mesh->NextMesh) {
            for (uint32_t i = 0; i < mesh->NSections; ++i) {
                auto *section = mesh->Sections[i].Section;
                if (auto *storage = modLiveStorage(section)) {
                    // The loose native release path skips raw 32-bit IBs
                    // without a uint16_t CPU index view. Packed release may
                    // already have cleared it; the shared helper is idempotent.
                    modReleaseOwnedWideIndexBuffer(section);
                    if (!storage->rigid && storage->palette
                        && section->BonesIdx == storage->palette) {
                        tlMemFree(section->BonesIdx);
                        section->BonesIdx = nullptr;
                        section->NBones = 0;
                    }
                    modForgetSection(section);
                }
            }
        }
        auto marked = modFileReplacedMaterials.find(found->second);
        if (marked != modFileReplacedMaterials.end()) {
            for (auto *material : marked->second) modReplacedMaterials.erase(material);
            modFileReplacedMaterials.erase(marked);
        }
        auto files = modNativeMaterialFiles.find(found->second);
        if (files != modNativeMaterialFiles.end()) {
            for (auto &file : files->second) {
                if (!file.second) continue;
                for (auto &material : file.second->materials) {
                    // Release exactly the references acquired by BindMaterial;
                    // our private textures have one owner irrespective of how
                    // many sections/materials point to them.
                    auto prior = file.second->originalBindings.find(material.second);
                    if (prior != file.second->originalBindings.end()) {
                        material.second->field_1C = prior->second.first;
                        material.second->field_24 = prior->second.second;
                    }
                    material.second->m_shader->ReleaseMaterial(material.second);
                }
                for (auto &texture : file.second->textures)
                    nglDestroyTexture(texture.second);
            }
            modNativeMaterialFiles.erase(files);
        }
        auto expanded = modExpandedMeshes.find(found->second);
        if (expanded != modExpandedMeshes.end()) {
            for (auto &mesh : expanded->second)
                for (auto &section : mesh->sections) {
                    modExtraSections.erase(&section);
                    modForgetSection(&section);
                }
            modExpandedMeshes.erase(expanded);
        }
        modReleaseCustomMaterials(found->second);
        modMeshBufferOwners.erase(found);
    }
    // Xbox meshes own converted CPU streams, section tables and hash names
    // in addition to the serialized image. Packed buffers need this cleanup
    // too, even when no raw override was registered in modMeshBufferOwners.
    modReleaseXboxMeshFileResources(buffer, packed);
    if (!modOwnedMeshBuffers.erase(buffer->Buf)) return false;
    tlMemFree(buffer->Buf);
    *buffer = {};
    return true;
}

static nglMaterialBase *modSourceMaterial(const modmesh::BuiltSection &built,
                                         Mod *mod, nglMeshFile *owner)
{
    if (built.sourceMeshName.empty() || built.sourceSection < 0)
        return nullptr;
    const std::string family = modNativeFamily(built.sourceMeshName);
    modMeshBufferOwners[owner->FileBuf.Buf] = owner;
    auto &file = modNativeMaterialFiles[owner][family];
    if (!file) {
        auto candidate = std::make_unique<ModNativeMaterialFile>();
        candidate->assetRoot = mod->Path.parent_path();
        std::vector<uint8_t> bytes;
        std::string why;
        if (!modReadNativePcmesh(family, mod->Path.parent_path(), bytes)
            || !candidate->source.parse(std::move(bytes), &why)) {
            sp_log("[modmesh] cannot restore %s materials: %s", family.c_str(), why.c_str());
            return nullptr;
        }
        candidate->rebound = candidate->source.bytes;
        file = std::move(candidate);
    }
    for (const auto &mesh : file->source.meshes) {
        if (transformToLower(mesh.name) != transformToLower(built.sourceMeshName)
            || size_t(built.sourceSection) >= mesh.sections.size())
            continue;
        const auto &section = mesh.sections[built.sourceSection];
        const uint32_t offset = section.materialOffset;
        auto found = file->materials.find(offset);
        if (found != file->materials.end())
            return found->second;
        for (const auto &metadata : file->source.materials) {
            if (metadata.offset != offset || !metadata.characterBindable) continue;
            auto *entry = nglShaderBank.Search(tlHashString{metadata.shaderHash});
            if (!entry) return nullptr;
            auto *shader = static_cast<nglShader *>(entry->field_20);
            auto *material = reinterpret_cast<nglMaterialBase *>(file->rebound.data() + offset);
            if (!shader->CheckMaterialVersion(material)) return nullptr;
            const uint32_t base = reinterpret_cast<uint32_t>(file->rebound.data());
            PTR_OFFSET(base, material->Name);
            material->m_shader = shader;
            material->File = owner;
            material->NextMaterial = nullptr;
            shader->RebaseMaterial(material, base);
            shader->BindMaterial(material);
            file->originalBindings[material] = {material->field_1C, material->field_24};
            if (auto *texture = modSourceTexture(*file, metadata.diffuseName))
                material->field_1C = texture;
            if (auto *texture = modSourceTexture(*file, metadata.detailName))
                material->field_24 = texture;
            file->materials[offset] = material;
            sp_log("[modmesh] material %s <- %s section %d (original colors)",
                   metadata.name.c_str(), built.sourceMeshName.c_str(), built.sourceSection);
            return material;
        }
    }
    return nullptr;
}
