#pragma once

// Data-only NAL resource mappings. Paths retain their spelling; the runtime
// resolves them relative to the configuration file and validates asset bytes.
#include "mod_mesh_aliases.h"
#include <map>
#include <set>
#include <tuple>

namespace modmesh { namespace nalconfig {

enum class Kind { Skeleton, Animation, Clip };

struct Mapping {
    Kind kind = Kind::Skeleton;
    aliases::Target bank;
    aliases::Target targetClip, sourceClip;
    std::string sourceFile;
};

class Config {
public:
    static constexpr size_t maxBytes = 256u * 1024u;
    static constexpr size_t maxEntries = 4096;
    static constexpr size_t maxSourceBytes = 32767;

    bool load(const std::string &path, const aliases::Dictionary &dictionary,
              std::string *why = nullptr)
    {
        std::string bytes;
        return aliases::readFile(path, maxBytes, bytes, why)
            && parse(bytes, dictionary, why);
    }

    bool parse(std::string_view bytes, const aliases::Dictionary &dictionary,
               std::string *why = nullptr)
    {
        if (bytes.size() > maxBytes) return aliases::fail(why, "NAL config exceeds byte limit");
        if (bytes.substr(0, 3) == "\xEF\xBB\xBF") bytes.remove_prefix(3);
        Config parsed;
        std::set<std::tuple<Kind,uint32_t,uint32_t>> keys;
        std::map<uint32_t,Kind> animationKinds;
        bool haveSection = false;
        Kind section = Kind::Skeleton;
        size_t lineNumber = 0;
        auto fail = [&](const std::string &message) {
            return aliases::fail(why, "NAL config line " + std::to_string(lineNumber) + ": " + message);
        };
        while (!bytes.empty()) {
            const auto end = bytes.find('\n');
            auto line = bytes.substr(0, end);
            if (end == std::string_view::npos) bytes = {};
            else bytes.remove_prefix(end + 1);
            ++lineNumber;
            if (!line.empty() && line.back() == '\r') line.remove_suffix(1);
            // Check the complete line, including comments, so NUL/control data
            // cannot hide after a syntactically valid mapping.
            for (unsigned char c : line)
                if ((c < 32 && c != '\t') || c == 127) return fail("invalid control character");
            bool quoted = false;
            for (size_t i = 0; i < line.size(); ++i) {
                if (line[i] == '"') quoted = !quoted;
                else if (!quoted && (line[i] == '#' || line[i] == ';')) {
                    line = line.substr(0, i); break;
                }
            }
            if (quoted) return fail("unterminated quote");
            line = aliases::trimmed(line);
            if (line.empty()) continue;
            if (line.front() == '[') {
                if (line.back() != ']') return fail("malformed section header");
                const auto name = aliases::normalized(aliases::trimmed(line.substr(1, line.size() - 2)));
                if (name == "skeletons") section = Kind::Skeleton;
                else if (name == "animations") section = Kind::Animation;
                else if (name == "clips") section = Kind::Clip;
                else return fail("empty or unknown section");
                haveSection = true; continue;
            }
            if (!haveSection) return fail("mapping requires a section");
            size_t equals = 0;
            if (!singleSeparator(line, "=", equals)) return fail("expected one target = source assignment");
            auto left = aliases::trimmed(line.substr(0, equals));
            auto right = aliases::trimmed(line.substr(equals + 1));
            Mapping entry; entry.kind = section;
            std::string detail;
            auto resolve = [&](std::string_view token, aliases::Target &target) {
                if (!value(token)) { detail = "empty or malformed resource token"; return false; }
                return dictionary.resolveTarget(token, target, &detail);
            };
            if (section == Kind::Clip) {
                size_t slash = 0, sourceSeparator = 0;
                if (!singleSeparator(left, "/", slash))
                    return fail("clip key requires bank / target clip; quote names containing slashes");
                if (!singleSeparator(right, "::", sourceSeparator))
                    return fail("clip source requires path.PCANIM :: source clip");
                if (!resolve(left.substr(0, slash), entry.bank)
                    || !resolve(left.substr(slash + 1), entry.targetClip)
                    || !resolve(right.substr(sourceSeparator + 2), entry.sourceClip)) return fail(detail);
                right = right.substr(0, sourceSeparator);
            } else if (!resolve(left, entry.bank)) return fail(detail);
            if (!value(right) || right.size() > maxSourceBytes)
                return fail("empty, malformed or excessive source path");
            const std::string extension = section == Kind::Skeleton ? ".pcskel" : ".pcanim";
            if (right.size() <= extension.size()
                || aliases::normalized(right.substr(right.size() - extension.size())) != extension)
                return fail("source path requires " + extension);
            entry.sourceFile = std::string(right);
            const auto key = std::make_tuple(section, entry.bank.hash,
                section == Kind::Clip ? entry.targetClip.hash : 0u);
            if (!keys.insert(key).second) return fail("duplicate resource mapping");
            if (section != Kind::Skeleton) {
                const auto found = animationKinds.find(entry.bank.hash);
                if (found != animationKinds.end() && found->second != section)
                    return fail("whole-bank and per-clip mappings conflict for the same bank");
                animationKinds[entry.bank.hash] = section;
            }
            if (parsed.entries_.size() >= maxEntries) return fail("entry limit exceeded");
            parsed.entries_.push_back(std::move(entry));
        }
        // Empty/comment-only configurations intentionally disable mappings.
        *this = std::move(parsed);
        if (why) why->clear();
        return true;
    }

    size_t size() const { return entries_.size(); }
    const std::vector<Mapping> &mappings() const { return entries_; }

private:
    static bool singleSeparator(std::string_view text, std::string_view separator, size_t &position)
    {
        position = std::string_view::npos;
        bool quoted = false;
        for (size_t i = 0; i < text.size(); ++i) {
            if (text[i] == '"') { quoted = !quoted; continue; }
            if (!quoted && text.substr(i, separator.size()) == separator) {
                if (position != std::string_view::npos) return false;
                position = i; i += separator.size() - 1;
            }
        }
        return !quoted && position != std::string_view::npos;
    }

    static bool value(std::string_view &text)
    {
        text = aliases::trimmed(text);
        if (!text.empty() && text.front() == '"') {
            if (text.size() < 2 || text.back() != '"') return false;
            text.remove_prefix(1); text.remove_suffix(1);
        }
        if (text.empty() || text.find('"') != std::string_view::npos) return false;
        for (unsigned char c : text) if (c < 32 || c == 127) return false;
        return true;
    }

    std::vector<Mapping> entries_;
};

}} // namespace modmesh::nalconfig
