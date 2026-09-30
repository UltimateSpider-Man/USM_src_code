#pragma once

// Data-only OBJ/FBX/glTF/GLB aliases. A dictionary explains hashes; it never selects or
// activates a resource. Callers apply a mapping only to an actual mesh load.
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace modmesh { namespace aliases {

inline char lower(char value)
{
    return value >= 'A' && value <= 'Z' ? char(value + ('a' - 'A')) : value;
}

inline std::string normalized(std::string_view value)
{
    std::string result(value);
    for (char &c : result) c = lower(c);
    return result;
}

inline uint32_t gameHash(std::string_view value)
{
    uint32_t hash = 0;
    for (char c : value) {
        // The retail hash lowercases ASCII and sign-extends other char bytes.
        const auto byte = static_cast<uint8_t>(lower(c));
        hash = hash * 33u + uint32_t(int32_t(static_cast<int8_t>(byte)));
    }
    return hash;
}

inline std::string_view trimmed(std::string_view value)
{
    while (!value.empty() && (value.front() == ' ' || value.front() == '\t'
                              || value.front() == '\r')) value.remove_prefix(1);
    while (!value.empty() && (value.back() == ' ' || value.back() == '\t'
                              || value.back() == '\r')) value.remove_suffix(1);
    return value;
}

inline bool fail(std::string *why, const std::string &message)
{
    if (why) *why = message;
    return false;
}

inline bool parseHex(std::string_view text, uint32_t &hash, bool prefixedOnly = true)
{
    if (text.size() >= 2 && text[0] == '0' && lower(text[1]) == 'x') {
        text.remove_prefix(2);
    } else if (prefixedOnly || text.size() != 8) {
        return false;
    }
    if (text.empty() || text.size() > 8) return false;
    hash = 0;
    for (char c : text) {
        c = lower(c);
        unsigned digit;
        if (c >= '0' && c <= '9') digit = unsigned(c - '0');
        else if (c >= 'a' && c <= 'f') digit = unsigned(c - 'a') + 10;
        else return false;
        hash = (hash << 4) | digit;
    }
    return true;
}

inline bool readFile(const std::string &path, size_t limit, std::string &bytes,
                     std::string *why)
{
    std::ifstream input(path, std::ios::binary | std::ios::ate);
    if (!input) return fail(why, "cannot open " + path);
    const auto length = input.tellg();
    if (length < 0 || uint64_t(length) > limit)
        return fail(why, "file exceeds size limit: " + path);
    std::string result(size_t(length), '\0');
    input.seekg(0);
    if (!result.empty() && !input.read(&result[0], std::streamsize(result.size())))
        return fail(why, "cannot read " + path);
    bytes = std::move(result);
    return true;
}

struct Target {
    uint32_t hash = 0;
    std::string name;
    bool fromDictionary = false;
};

class Dictionary {
public:
    static constexpr size_t maxBytes = 32u * 1024u * 1024u;
    static constexpr size_t maxEntries = 500000;
    static constexpr size_t maxNameBytes = 4096;

    bool load(const std::string &path, std::string *why = nullptr)
    {
        std::string bytes;
        return readFile(path, maxBytes, bytes, why) && parse(bytes, why);
    }

    bool parse(std::string_view bytes, std::string *why = nullptr)
    {
        if (bytes.size() > maxBytes) return fail(why, "dictionary exceeds size limit");
        if (bytes.substr(0, 3) == "\xEF\xBB\xBF") bytes.remove_prefix(3);
        std::unordered_map<uint32_t, std::string> parsed;
        size_t lineNumber = 0;
        while (!bytes.empty()) {
            const size_t end = bytes.find('\n');
            auto line = bytes.substr(0, end);
            if (end == std::string_view::npos) bytes = {};
            else bytes.remove_prefix(end + 1);
            ++lineNumber;
            if (!line.empty() && line.back() == '\r') line.remove_suffix(1);
            if (line.size() > maxNameBytes + 32)
                return fail(why, "dictionary line exceeds size limit");
            if (line.empty() || line == "hashcode\tstring"
                || line.find_first_not_of('-') == std::string_view::npos) continue;
            const size_t tab = line.find('\t');
            uint32_t hash = 0;
            if (tab == std::string_view::npos || !parseHex(line.substr(0, tab), hash))
                return fail(why, "invalid dictionary hash at line " + std::to_string(lineNumber));
            const auto name = line.substr(tab + 1);
            if (name.empty() || name.size() > maxNameBytes
                || name.find_first_of("\0\r\n", 0, 3) != std::string_view::npos)
                return fail(why, "dictionary name/hash mismatch at line " + std::to_string(lineNumber));
            const uint32_t canonicalHash = gameHash(name);
            if (canonicalHash != hash) {
                // Some extracted entries spell the four little-endian file
                // bytes as hex. Accept that representation only when the name
                // verifies it, and index by the engine's numeric hash.
                const uint32_t reversed = (hash >> 24) | ((hash >> 8) & 0x0000FF00u)
                    | ((hash << 8) & 0x00FF0000u) | (hash << 24);
                if (canonicalHash != reversed)
                    return fail(why, "dictionary name/hash mismatch at line " + std::to_string(lineNumber));
                hash = canonicalHash;
            }
            const auto existing = parsed.find(hash);
            if (existing != parsed.end()) {
                if (normalized(existing->second) != normalized(name))
                    return fail(why, "ambiguous dictionary hash at line " + std::to_string(lineNumber));
            } else {
                if (parsed.size() >= maxEntries)
                    return fail(why, "dictionary exceeds entry limit");
                parsed.emplace(hash, std::string(name));
            }
        }
        if (parsed.empty()) return fail(why, "dictionary has no entries");
        names_ = std::move(parsed);
        if (why) why->clear();
        return true;
    }

    size_t size() const { return names_.size(); }
    const std::unordered_map<uint32_t, std::string> &entries() const { return names_; }

    const std::string *lookup(uint32_t hash) const
    {
        const auto found = names_.find(hash);
        return found == names_.end() ? nullptr : &found->second;
    }

    // Plain names retain compatibility with custom resources absent from the
    // dictionary. Hex keys identify exactly one hash, never a name prefix.
    bool resolveTarget(std::string_view key, Target &target, std::string *why = nullptr) const
    {
        // Preserve dictionary spellings whose hash includes trailing space or
        // tab. Both occur in the shipped dictionary; quoted config keys and
        // numeric aliases must still address them exactly.
        if (const auto *exact = lookup(gameHash(key)); exact
            && normalized(*exact) == normalized(key)) {
            target = Target{gameHash(key), *exact, true};
            if (why) why->clear();
            return true;
        }
        key = trimmed(key);
        if (key.empty() || key.size() > maxNameBytes
            || key.find_first_of("\0\r\n\t", 0, 4) != std::string_view::npos)
            return fail(why, "empty or invalid mesh target");
        const uint32_t namedHash = gameHash(key);
        const auto *named = lookup(namedHash);
        uint32_t hash = namedHash;
        const bool exactName = named && normalized(*named) == normalized(key);
        if (!exactName) {
            const bool prefixed = key.size() >= 2 && key[0] == '0' && lower(key[1]) == 'x';
            if (prefixed && !parseHex(key, hash)) return fail(why, "invalid hexadecimal mesh target");
            if (!prefixed) {
                uint32_t unprefixed = 0;
                if (parseHex(key, unprefixed, false) && lookup(unprefixed)) hash = unprefixed;
            }
        }
        Target resolved;
        resolved.hash = hash;
        if (const auto *known = lookup(hash)) {
            if (hash == namedHash && normalized(*known) != normalized(key))
                return fail(why, "mesh target collides with another dictionary name");
            resolved.name = *known;
            resolved.fromDictionary = true;
        } else {
            resolved.name = std::string(key);
        }
        target = std::move(resolved);
        if (why) why->clear();
        return true;
    }

private:
    std::unordered_map<uint32_t, std::string> names_;
};

struct Mapping {
    Target target;
    std::string sourceFile;
};

inline bool meshExtension(std::string_view extension)
{
    const auto ext = normalized(extension);
    return ext == ".fbx" || ext == ".obj" || ext == ".glb" || ext == ".gltf";
}

inline bool sourceBasename(std::string_view value)
{
    const size_t dot = value.find_last_of('.');
    if (value.size() < 5 || value.size() > 255 || dot == std::string_view::npos
        || !meshExtension(value.substr(dot))
        || value.find_first_of("/\\:<>\"|?*") != std::string_view::npos) return false;
    const auto stem = value.substr(0, dot);
    if (stem.empty() || stem == "." || stem == "..") return false;
    for (unsigned char c : value) if (c < 32 || c == 127) return false;
    return true;
}

class Config {
public:
    static constexpr size_t maxBytes = 256u * 1024u;
    static constexpr size_t maxMappings = 4096;

    bool load(const std::string &path, const Dictionary &dictionary,
              std::string *why = nullptr)
    {
        std::string bytes;
        return readFile(path, maxBytes, bytes, why) && parse(bytes, dictionary, why);
    }

    bool parse(std::string_view bytes, const Dictionary &dictionary,
               std::string *why = nullptr)
    {
        if (bytes.size() > maxBytes) return fail(why, "mesh swap config exceeds size limit");
        if (bytes.substr(0, 3) == "\xEF\xBB\xBF") bytes.remove_prefix(3);
        Config parsed;
        size_t lineNumber = 0;
        while (!bytes.empty()) {
            const size_t end = bytes.find('\n');
            auto line = bytes.substr(0, end);
            if (end == std::string_view::npos) bytes = {};
            else bytes.remove_prefix(end + 1);
            ++lineNumber;
            if (line.size() > Dictionary::maxNameBytes + 512)
                return fail(why, "mesh swap line exceeds size limit");
            bool quoted = false;
            size_t equals = std::string_view::npos;
            for (size_t i = 0; i < line.size(); ++i) {
                if (line[i] == '"') quoted = !quoted;
                else if (!quoted && (line[i] == '#' || line[i] == ';')) {
                    line = line.substr(0, i);
                    break;
                } else if (!quoted && line[i] == '=' && equals == std::string_view::npos) equals = i;
            }
            if (quoted) return fail(why, "unterminated quote at line " + std::to_string(lineNumber));
            line = trimmed(line);
            if (line.empty()) continue;
            if (line.front() == '[') {
                if (normalized(line) != "[mesh_swaps]")
                    return fail(why, "unsupported mesh swap section at line " + std::to_string(lineNumber));
                continue;
            }
            // Recompute after trimming so whitespace never shifts the split.
            quoted = false;
            equals = std::string_view::npos;
            for (size_t i = 0; i < line.size(); ++i) {
                if (line[i] == '"') quoted = !quoted;
                else if (!quoted && line[i] == '=') { equals = i; break; }
            }
            if (equals == std::string_view::npos)
                return fail(why, "expected target = source.fbx at line " + std::to_string(lineNumber));
            auto key = trimmed(line.substr(0, equals));
            auto source = trimmed(line.substr(equals + 1));
            const auto unquote = [](std::string_view &value) {
                if (!value.empty() && value.front() == '"') {
                    if (value.size() < 2 || value.back() != '"') return false;
                    value = value.substr(1, value.size() - 2);
                }
                return value.find('"') == std::string_view::npos;
            };
            if (!unquote(key) || !unquote(source) || !sourceBasename(source))
                return fail(why, "source must be an mesh basename at line " + std::to_string(lineNumber));
            Mapping mapping;
            std::string detail;
            if (!dictionary.resolveTarget(key, mapping.target, &detail))
                return fail(why, detail + " at line " + std::to_string(lineNumber));
            if (parsed.byHash_.count(mapping.target.hash))
                return fail(why, "duplicate mesh target at line " + std::to_string(lineNumber));
            if (parsed.mappings_.size() >= maxMappings)
                return fail(why, "mesh swap config exceeds mapping limit");
            mapping.sourceFile = std::string(source);
            parsed.byHash_.emplace(mapping.target.hash, parsed.mappings_.size());
            parsed.mappings_.push_back(std::move(mapping));
        }
        *this = std::move(parsed);
        if (why) why->clear();
        return true;
    }

    const std::vector<Mapping> &mappings() const { return mappings_; }
    size_t size() const { return mappings_.size(); }
    const Mapping *find(uint32_t actualMeshHash) const
    {
        const auto found = byHash_.find(actualMeshHash);
        return found == byHash_.end() ? nullptr : &mappings_[found->second];
    }

private:
    std::vector<Mapping> mappings_;
    std::unordered_map<uint32_t, size_t> byHash_;
};

}} // namespace modmesh::aliases
