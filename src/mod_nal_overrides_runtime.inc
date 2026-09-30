// Included by nal_system.cpp after nalResolveImageIdentity. All source bytes
// remain pristine; native constructors receive independent engine allocations.
namespace {
namespace ncfg = modmesh::nalconfig;

struct NalConfiguredMapping {
    ncfg::Mapping mapping;
    std::filesystem::path path;
};

struct NalConfiguredState {
    std::map<uint32_t, NalConfiguredMapping> skeletons, banks;
    std::map<uint32_t, std::vector<NalConfiguredMapping>> clips;
    std::set<std::string> paths;
    std::map<std::string, std::shared_ptr<modanim::Source>> animations;
    std::map<std::string, std::shared_ptr<modmesh::pcskelsource::Source>> skeletonImages;
    std::map<uint32_t, std::vector<uint8_t>> composed;
    std::map<void *, void *> skeletonOriginals;
    std::set<std::string> reported;
};

static std::string nalConfigPathKey(const std::filesystem::path &path)
{
    std::error_code ec;
    auto canonical = std::filesystem::weakly_canonical(path, ec);
    return transformToLower((ec ? path.lexically_normal() : canonical).generic_string());
}

static bool nalConfigResolve(const std::string &name, const std::filesystem::path &hint,
                             std::filesystem::path &out)
{
    std::vector<std::filesystem::path> candidates{hint / name};
    for (const auto &root : modRootDirs()) {
        candidates.push_back(root / name);
        candidates.push_back(root.parent_path() / "mesh_sources" / "pcskel" / name);
        candidates.push_back(root.parent_path() / "animation_sources" / "pcanim" / name);
    }
    candidates.emplace_back(name);
    for (const auto &candidate : candidates) {
        std::error_code ec;
        if (std::filesystem::is_regular_file(candidate, ec)) {
            out = std::filesystem::weakly_canonical(candidate, ec);
            if (ec) out = candidate;
            return true;
        }
    }
    // An explicit path must not silently resolve to another file with the
    // same basename. Bare names may search recursively, but must be unique.
    if (std::filesystem::path(name).has_parent_path()) return false;
    std::set<std::string> matches;
    for (const auto &root : modRootDirs()) {
        std::error_code ec;
        for (std::filesystem::recursive_directory_iterator it(root,
                 std::filesystem::directory_options::skip_permission_denied, ec), end;
             it != end; it.increment(ec)) {
            if (ec) { ec.clear(); continue; }
            if (it->is_regular_file(ec) && transformToLower(it->path().filename().string())
                    == transformToLower(name)) {
                matches.insert(nalConfigPathKey(it->path()));
                out = it->path();
            }
        }
    }
    return matches.size() == 1;
}

static NalConfiguredState &nalConfiguredState()
{
    static NalConfiguredState state = [] {
        NalConfiguredState result;
        modmesh::aliases::Dictionary dictionary;
        std::string why;
        for (const auto &root : modRootDirs()) {
            const auto path = root.parent_path() / "mesh_sources" / "string_hash_dictionary.txt";
            if (dictionary.load(path.string(), &why)) break;
        }
        std::set<std::string> visited;
        for (const auto &root : modRootDirs()) {
            std::vector<std::filesystem::path> configs;
            std::error_code ec;
            for (std::filesystem::recursive_directory_iterator it(root,
                     std::filesystem::directory_options::skip_permission_denied, ec), end;
                 it != end; it.increment(ec)) {
                if (ec) { ec.clear(); continue; }
                if (it->is_regular_file(ec)
                    && transformToLower(it->path().filename().string()) == "nal_swaps.ini")
                    configs.push_back(it->path());
            }
            std::sort(configs.begin(), configs.end());
            for (const auto &path : configs) {
                if (!visited.insert(nalConfigPathKey(path)).second) continue;
                ncfg::Config config;
                if (!config.load(path.string(), dictionary, &why)) {
                    sp_log("[modnal] config rejected: %s: %s", path.string().c_str(), why.c_str());
                    continue;
                }
                for (const auto &mapping : config.mappings()) {
                    NalConfiguredMapping entry{mapping, path.parent_path() / mapping.sourceFile};
                    nalConfigResolve(mapping.sourceFile, path.parent_path(), entry.path);
                    // Reserve source paths even when a conflicting mapping is
                    // rejected, so they cannot leak into legacy global scans.
                    result.paths.insert(nalConfigPathKey(entry.path));
                    const auto hash = mapping.bank.hash;
                    bool accepted = false;
                    if (mapping.kind == ncfg::Kind::Skeleton)
                        accepted = result.skeletons.emplace(hash, entry).second;
                    else if (mapping.kind == ncfg::Kind::Animation) {
                        if (!result.clips.count(hash))
                            accepted = result.banks.emplace(hash, entry).second;
                    } else if (!result.banks.count(hash)) {
                        auto &clips = result.clips[hash];
                        const bool duplicate = std::any_of(clips.begin(), clips.end(),
                            [&](const auto &old) { return old.mapping.targetClip.hash == mapping.targetClip.hash; });
                        if (!duplicate) { clips.push_back(entry); accepted = true; }
                    }
                    if (!accepted)
                        sp_log("[modnal] duplicate/conflicting mapping ignored: %s", mapping.bank.name.c_str());
                }
            }
        }
        sp_log("[modnal] INI mappings: %u skeletons, %u animation banks, %u banks with clip swaps",
            unsigned(result.skeletons.size()), unsigned(result.banks.size()), unsigned(result.clips.size()));
        return result;
    }();
    return state;
}

static void nalConfigReport(const std::string &key, const std::string &message)
{
    if (nalConfiguredState().reported.insert(key + message).second)
        sp_log("[modnal] %s", message.c_str());
}

static bool nalConfigRead(const std::filesystem::path &path, std::vector<uint8_t> &bytes)
{
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) return false;
    const auto size = file.tellg();
    if (size <= 0 || size > 128 * 1024 * 1024) return false;
    bytes.resize(static_cast<size_t>(size));
    file.seekg(0);
    return bool(file.read(reinterpret_cast<char *>(bytes.data()), size));
}

static std::shared_ptr<modanim::Source> nalConfigAnimation(const std::filesystem::path &path)
{
    auto &cache = nalConfiguredState().animations;
    const auto key = nalConfigPathKey(path);
    if (const auto found = cache.find(key); found != cache.end()) return found->second;
    std::vector<uint8_t> bytes;
    auto source = std::make_shared<modanim::Source>();
    std::string why = "file unavailable or exceeds 128 MiB";
    if (!nalConfigRead(path, bytes) || !source->parse(std::move(bytes), &why)) {
        nalConfigReport(key, "PCANIM rejected: " + path.string() + ": " + why);
        source.reset();
    }
    cache.emplace(key, source);
    return source;
}
} // namespace

bool modNalConfiguredSource(const std::filesystem::path &path)
{
    return nalConfiguredState().paths.count(nalConfigPathKey(path)) != 0;
}

bool modNalHasAnimationSwap(uint32_t hash)
{
    auto &state = nalConfiguredState();
    return state.banks.count(hash) || state.clips.count(hash);
}

uint8_t *modNalConfiguredAnimation(uint32_t hash, int *sizeOut,
    const uint8_t *original, int originalSize, bool *handled)
{
    auto &state = nalConfiguredState();
    *handled = modNalHasAnimationSwap(hash);
    if (!*handled) return nullptr;
    if (sizeOut) *sizeOut = 0;
    std::string why;
    modanim::Source target;
    if (!original || originalSize <= 0 || originalSize > 128 * 1024 * 1024
        || !target.parse(std::vector<uint8_t>(original, original + originalSize), &why)) {
        nalConfigReport(std::to_string(hash), "animation mapping requires a valid original bank: "
            + std::to_string(hash) + ": " + why);
        return nullptr;
    }
    auto &output = state.composed[hash];
    if (const auto bank = state.banks.find(hash); bank != state.banks.end()) {
        const auto source = nalConfigAnimation(bank->second.path);
        if (!source) return nullptr;
        if (!modanim::compatibleBank(*source, target, &why)) {
            nalConfigReport(std::to_string(hash), "bank mapping rejected: " + bank->second.mapping.bank.name + ": " + why);
            return nullptr;
        }
        output = source->bytes;
    } else {
        std::vector<modanim::ClipSwap> swaps;
        for (const auto &entry : state.clips.at(hash)) {
            const auto source = nalConfigAnimation(entry.path);
            if (!source) return nullptr;
            swaps.push_back({entry.mapping.targetClip.hash, source.get(), entry.mapping.sourceClip.hash});
        }
        if (!modanim::replaceClips(target, swaps, output, &why)) {
            nalConfigReport(std::to_string(hash), "clip mapping rejected: " + std::to_string(hash) + ": " + why);
            return nullptr;
        }
    }
    if (output.empty() || output.size() > size_t(std::numeric_limits<int>::max())) return nullptr;
    if (sizeOut) *sizeOut = int(output.size());
    nalConfigReport(std::to_string(hash), "selected INI PCANIM bank " + std::to_string(hash)
        + " (" + std::to_string(output.size()) + " bytes)");
    return output.data();
}

void *modNalConfiguredSkeleton(void *original, bool *handled)
{
    *handled = false;
    if (!original) return nullptr;
    const auto *raw = static_cast<const uint8_t *>(original);
    const uint32_t hash = nalReadU32(raw, 8);
    auto &state = nalConfiguredState();
    const auto mapping = state.skeletons.find(hash);
    if (mapping == state.skeletons.end()) return nullptr;
    *handled = true;
    const auto key = nalConfigPathKey(mapping->second.path);
    auto &cached = state.skeletonImages[key];
    std::string why;
    if (!cached) {
        std::vector<uint8_t> bytes;
        auto source = std::make_shared<modmesh::pcskelsource::Source>();
        if (!nalConfigRead(mapping->second.path, bytes) || !source->parse(std::move(bytes), &why)) {
            nalConfigReport(key, "PCSKEL rejected: " + mapping->second.path.string() + ": " + why);
            return nullptr;
        }
        cached = std::move(source);
    }
    const auto identity = nalResolveImageIdentity(original, TLRESOURCE_TYPE_SKELETON, hash, 0);
    modmesh::pcskelsource::Source target;
    if (identity.size <= 0 || identity.size > 64 * 1024 * 1024
        || !target.parse(std::vector<uint8_t>(raw, raw + identity.size), &why)) {
        nalConfigReport(key, "skeleton mapping requires a validated original: " + mapping->second.mapping.bank.name
            + ": " + (why.empty() ? "missing or invalid original resource size" : why));
        return nullptr;
    }
    // Mesh skin indices and compressed animation channels use the native
    // layout. Changing the declared skeleton's identity must not change them.
    bool compatible = target.version == cached->version
        && target.animationType == cached->animationType
        && target.bones.size() == cached->bones.size()
        && target.components.size() == cached->components.size()
        && target.renderBoneCount == cached->renderBoneCount
        && target.fixedPoseSize == cached->fixedPoseSize;
    for (size_t i = 0; compatible && i < target.bones.size(); ++i) {
        const auto &a = target.bones[i]; const auto &b = cached->bones[i];
        compatible = a.name == b.name && a.parent == b.parent && a.component == b.component
            && a.rotationChannel == b.rotationChannel && a.positionChannel == b.positionChannel
            && a.rotationFromPose == b.rotationFromPose && a.positionFromPose == b.positionFromPose
            && a.genericFlags == b.genericFlags && a.genericRecordIndex == b.genericRecordIndex
            && a.positionByteOffset == b.positionByteOffset && a.rotationByteOffset == b.rotationByteOffset;
    }
    for (size_t i = 0; compatible && i < target.components.size(); ++i) {
        const auto &a = target.components[i]; const auto &b = cached->components[i];
        compatible = a.nameId == b.nameId && a.typeHash == b.typeHash && a.flags == b.flags
            && a.firstTrack == b.firstTrack && a.trackCount == b.trackCount
            && a.perSkelOffset == b.perSkelOffset && a.defaultPoseOffset == b.defaultPoseOffset
            && a.perSkelSize == b.perSkelSize && a.defaultPoseSize == b.defaultPoseSize;
    }
    if (!compatible) {
        nalConfigReport(key, "skeleton mapping rejected: incompatible pose/channel layout for " + mapping->second.mapping.bank.name);
        return nullptr;
    }
    void *copy = tlMemAlloc(uint32_t(cached->bytes.size()), 16u, 0x2000000u);
    if (!copy) return nullptr;
    std::memcpy(copy, cached->bytes.data(), cached->bytes.size());
    std::memcpy(static_cast<uint8_t *>(copy) + 8, raw + 8, 32); // Preserve target resource identity.
    state.skeletonOriginals.emplace(copy, original);
    nalConfigReport(key, "selected INI PCSKEL " + mapping->second.mapping.bank.name
        + " <- " + mapping->second.path.string());
    return copy;
}
