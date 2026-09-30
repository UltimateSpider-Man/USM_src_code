// Included after the texture/file helpers. Resolve only meshes on demand;
// .obj/.fbx/.glb/.gltf aliases can be discovered from mesh entries under mods/extra.
struct ModMeshAliasState {
    modmesh::aliases::Dictionary dictionary;
    std::map<uint32_t, Mod *> named;
    std::map<uint32_t, std::unique_ptr<Mod>> configured;
    std::vector<std::unique_ptr<Mod>> discovered;
};

static bool modEnsureMeshData(Mod *mod)
{
    if (mod == nullptr || mod->Path.empty() || !mod->Data.empty())
        return mod != nullptr && !mod->Path.empty() && !mod->Data.empty();
    if (!modReadWholeFile(mod->Path, mod->Data) || mod->Data.empty()) {
        sp_log("[modmesh] failed to read mesh for mesh swap: %s",
               mod->Path.string().c_str());
        return false;
    }
    return true;
}

static ModMeshAliasState &modMeshAliasState()
{
    static ModMeshAliasState state = [] {
        ModMeshAliasState result;
        std::filesystem::path dictionaryPath;
        std::string why;
        if (!modResolveFile("string_hash_dictionary.txt", dictionaryPath)) {
            for (const auto &root : modRootDirs()) {
                const auto candidate = root.parent_path() / "mesh_sources" / "string_hash_dictionary.txt";
                std::error_code ec;
                if (std::filesystem::is_regular_file(candidate, ec)) { dictionaryPath = candidate; break; }
            }
        }
        if (dictionaryPath.empty()) {
            std::error_code ec;
            auto search = std::filesystem::weakly_canonical(std::filesystem::current_path(ec), ec);
            if (!ec) {
                while (true) {
                    const auto buildMesh = search / "build-mesh-names" / "string_hash_dictionary.txt";
                    const auto meshSource = search / "mesh_sources" / "string_hash_dictionary.txt";
                    if (std::filesystem::is_regular_file(buildMesh, ec)) { dictionaryPath = buildMesh; break; }
                    if (ec) { ec.clear(); }
                    if (std::filesystem::is_regular_file(meshSource, ec)) { dictionaryPath = meshSource; break; }
                    if (ec) { ec.clear(); }
                    const auto parent = search.parent_path();
                    if (search == parent)
                        break;
                    search = parent;
                }
            }
        }
        if (!dictionaryPath.empty()) {
            if (result.dictionary.load(dictionaryPath.string(), &why)) {
                sp_log("[modmesh] dictionary: %u names from %s",
                       unsigned(result.dictionary.size()), dictionaryPath.string().c_str());
            } else {
                sp_log("[modmesh] dictionary rejected: %s", why.c_str());
            }
        }
        const bool hasDictionary = result.dictionary.size() > 0u;
        auto prettyPath = [](const std::filesystem::path &path) {
            std::error_code ec;
            const auto canonical = std::filesystem::weakly_canonical(path, ec);
            return ec ? path.string() : canonical.string();
        };
        for (auto &[registeredHash, mod] : Mods) {
            if (mod.Type != TLRESOURCE_TYPE_MESH
                || !modmesh::aliases::meshExtension(mod.Path.extension().string())) continue;
            if (!hasDictionary) {
                modmesh::aliases::Target target;
                if (!result.dictionary.resolveTarget(mod.Path.stem().string(), target, &why)) continue;
                const uint32_t hash = target.hash;
                if (result.configured.count(hash) || result.named.count(hash))
                    continue;
                result.named.emplace(hash, &mod);
                sp_log("[modmesh] fallback mesh alias: %s (0x%08X) <- %s",
                       mod.Path.stem().string().c_str(),
                       hash,
                        prettyPath(mod.Path).c_str());
            } else {
                modmesh::aliases::Target target;
                if (result.dictionary.resolveTarget(mod.Path.stem().string(), target, &why)) {
                    if (result.configured.count(target.hash) || result.named.count(target.hash))
                        continue;
                    result.named.emplace(target.hash, &mod);
                    const auto *name = result.dictionary.lookup(target.hash);
                    sp_log("[modmesh] dictionary mesh alias: %s (0x%08X) <- %s",
                           name ? name->c_str() : target.name.c_str(),
                           target.hash,
                            prettyPath(mod.Path).c_str());
                }
            }
        }
        std::set<std::filesystem::path> seenConfig;
        for (const auto &root : modRootDirs()) {
            std::error_code ec;
            if (!std::filesystem::is_directory(root, ec)) continue;
            std::vector<std::filesystem::path> configs;
            for (std::filesystem::recursive_directory_iterator it(root,
                     std::filesystem::directory_options::skip_permission_denied, ec), end;
                 it != end; it.increment(ec)) {
                if (ec) { ec.clear(); continue; }
                if (it->is_regular_file(ec)
                    && transformToLower(it->path().filename().string()) == "mesh_swaps.ini")
                    configs.push_back(it->path());
            }
            std::sort(configs.begin(), configs.end());
            for (const auto &path : configs) {
                auto canonical = std::filesystem::weakly_canonical(path, ec);
                if (ec || !seenConfig.insert(canonical).second) continue;
                modmesh::aliases::Config config;
                if (!config.load(path.string(), result.dictionary, &why)) {
                    sp_log("[modmesh] swap config rejected: %s: %s",
                           path.string().c_str(), why.c_str());
                    continue;
                }
                for (const auto &mapping : config.mappings()) {
                    if (result.configured.count(mapping.target.hash)) {
                        sp_log("[modmesh] duplicate configured target skipped: %s",
                               mapping.target.name.c_str());
                        continue;
                    }
                    std::filesystem::path source;
                    auto mod = std::make_unique<Mod>();
                    if (!modResolveFile(mapping.sourceFile, source, path.parent_path())
                        || !modReadWholeFile(source, mod->Data) || mod->Data.empty()) {
                        sp_log("[modmesh] configured mesh unavailable: %s",
                               mapping.sourceFile.c_str());
                        continue;
                    }
                    mod->Path = source;
                    mod->Type = TLRESOURCE_TYPE_MESH;
                    result.configured.emplace(mapping.target.hash, std::move(mod));
                    sp_log("[modmesh] configured swap: %s (0x%08X) <- %s",
                           mapping.target.name.c_str(), mapping.target.hash,
                           source.string().c_str());
                }
            }
        }
        // If we have no dictionary, still allow direct name-hash mesh discovery.
        std::set<std::filesystem::path> seenFbx;
        for (const auto &root : modRootDirs()) {
            std::error_code ec;
            if (!std::filesystem::is_directory(root, ec)) continue;
            for (std::filesystem::recursive_directory_iterator it(root,
                     std::filesystem::directory_options::skip_permission_denied, ec), end;
                 it != end; it.increment(ec)) {
                if (ec) { ec.clear(); continue; }
                if (!it->is_regular_file(ec))
                    continue;
                if (!modmesh::aliases::meshExtension(it->path().extension().string()))
                    continue;

                modmesh::aliases::Target target;
                uint32_t hash = 0u;
                bool fromDictionary = false;
                // The dictionary is a hash-name aid, not a mesh whitelist.
                // Resolve even with an empty dictionary so 0xHASH filenames
                // use the numeric engine hash rather than hashing the text.
                if (!result.dictionary.resolveTarget(it->path().stem().string(), target, &why)) continue;
                hash = target.hash;
                fromDictionary = target.fromDictionary;

                auto canonical = std::filesystem::weakly_canonical(it->path(), ec);
                if (ec || !seenFbx.insert(canonical).second) { ec.clear(); continue; }
                if (result.configured.count(hash) || result.named.count(hash))
                    continue;
                auto mod = std::make_unique<Mod>();
                mod->Path = it->path();
                mod->Type = TLRESOURCE_TYPE_MESH;
                const auto *name = fromDictionary ? result.dictionary.lookup(hash) : nullptr;
                result.named.emplace(hash, mod.get());
                result.discovered.push_back(std::move(mod));
                sp_log("[modmesh] %s mesh alias: %s (0x%08X) <- %s",
                       fromDictionary ? "dictionary" : "fallback",
                       name ? name->c_str() : it->path().stem().string().c_str(),
                       hash,
                       canonical.string().c_str());
            }
        }
        return result;
    }();
    return state;
}

static Mod *modSelectMeshOverrideHash(uint32_t hash)
{
    auto &state = modMeshAliasState();
    if (auto it = state.configured.find(hash); it != state.configured.end())
        return it->second.get();
    if (auto it = state.named.find(hash); it != state.named.end()) return it->second;
    Mod *mod = getMod(hash, TLRESOURCE_TYPE_MESH);
    return mod && modmesh::aliases::meshExtension(mod->Path.extension().string()) ? mod : nullptr;
}

static Mod *modSelectMeshOverride(const nglMeshFile *file, const char *meshName = nullptr)
{
    if (!file) return nullptr;
    // Embedded object names can differ from their containing mesh-file name.
    // Exact object overrides win, but never propagate to sibling meshes.
    if (meshName && *meshName)
        if (auto *mod = modSelectMeshOverrideHash(modmesh::aliases::gameHash(meshName))) return mod;
    return modSelectMeshOverrideHash(file->FileName.m_hash);
}

// Use the engine's numeric object identity even when to_string() has no
// dictionary spelling. Re-hashing a diagnostic "0x..." name is not equivalent.
static Mod *modSelectMeshOverride(const nglMeshFile *file, uint32_t meshHash)
{
    if (!file) return nullptr;
    if (auto *mod = modSelectMeshOverrideHash(meshHash)) return mod;
    return modSelectMeshOverride(file);
}

static std::string modMeshTargetName(const nglMeshFile *file)
{
    if (!file) return {};
    auto &dictionary = modMeshAliasState().dictionary;
    if (const auto *name = dictionary.lookup(file->FileName.m_hash)) return *name;
    return file->FileName.to_string();
}
