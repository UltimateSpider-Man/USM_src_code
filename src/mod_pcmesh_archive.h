#pragma once

// Read-only access to native mesh scaffolds in a standard PCMESH.zip. Members
// are never written to disk; NGL still validates and privately relocates the
// extracted PCMESH image. ZIP64, split archives and encrypted entries are not
// supported. FBX's bounded inflater is shared to avoid another dependency.
#include "mod_mesh_import.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <fstream>
#include <limits>
#include <string>
#include <unordered_map>
#include <vector>

namespace modmesh { namespace pcmeshzip {

class Archive {
    static constexpr std::size_t maxMemberBytes = 64u << 20;
    static constexpr std::size_t maxDirectoryBytes = 64u << 20;

    struct Entry {
        std::string name;
        std::uint16_t flags = 0, method = 0;
        std::uint32_t crc = 0, compressed = 0, uncompressed = 0, offset = 0;
    };

    std::string path_;
    std::uint64_t fileBytes_ = 0, directoryOffset_ = 0;
    std::vector<Entry> entries_;
    std::unordered_map<std::string, std::size_t> index_;

    static bool fail(std::string *why, const char *message)
    {
        if (why) *why = message;
        return false;
    }

    static std::uint16_t u16(const std::uint8_t *p)
    { return std::uint16_t(p[0]) | (std::uint16_t(p[1]) << 8); }

    static std::uint32_t u32(const std::uint8_t *p)
    { return std::uint32_t(u16(p)) | (std::uint32_t(u16(p + 2)) << 16); }

    static std::string lower(std::string text)
    {
        for (char &c : text)
            if (c >= 'A' && c <= 'Z') c = char(c + ('a' - 'A'));
        return text;
    }

    static std::string stem(const std::string &name)
    {
        const std::size_t slash = name.find_last_of("/\\");
        std::string result = lower(name.substr(slash == std::string::npos ? 0 : slash + 1));
        if (result.size() >= 7 && result.compare(result.size() - 7, 7, ".pcmesh") == 0)
            result.resize(result.size() - 7);
        return result;
    }

    static bool isMesh(const std::string &name)
    {
        return name.size() > 7 && lower(name.substr(name.size() - 7)) == ".pcmesh";
    }

    static bool readAt(std::ifstream &file, std::uint64_t offset,
                       void *out, std::size_t count)
    {
        if (offset > std::uint64_t((std::numeric_limits<std::streamoff>::max)())
            || count > std::size_t((std::numeric_limits<std::streamsize>::max)()))
            return false;
        file.clear();
        file.seekg(std::streamoff(offset), std::ios::beg);
        if (!file) return false;
        if (count) file.read(static_cast<char *>(out), std::streamsize(count));
        return bool(file);
    }

    static std::uint32_t crc32(const std::vector<std::uint8_t> &bytes)
    {
        static const std::array<std::uint32_t, 256> table = [] {
            std::array<std::uint32_t, 256> result{};
            for (std::uint32_t i = 0; i < result.size(); ++i) {
                std::uint32_t c = i;
                for (int bit = 0; bit < 8; ++bit)
                    c = (c >> 1) ^ ((c & 1) ? 0xedb88320u : 0u);
                result[i] = c;
            }
            return result;
        }();
        std::uint32_t value = 0xffffffffu;
        for (std::uint8_t byte : bytes)
            value = table[(value ^ byte) & 255u] ^ (value >> 8);
        return value ^ 0xffffffffu;
    }

public:
    std::size_t size() const { return entries_.size(); }

    bool open(const std::string &path, std::string *why = nullptr)
    {
        path_.clear();
        entries_.clear();
        index_.clear();
        fileBytes_ = directoryOffset_ = 0;
        if (why) why->clear();
        try {
            std::ifstream file(path, std::ios::binary | std::ios::ate);
            if (!file) return fail(why, "cannot open ZIP file");
            const std::streamoff end = file.tellg();
            if (end < 22) return fail(why, "ZIP file is truncated");
            const std::uint64_t length = std::uint64_t(end);
            const std::size_t tailSize = std::size_t((std::min)(length, std::uint64_t(65535u + 22u)));
            std::vector<std::uint8_t> tail(tailSize);
            if (!readAt(file, length - tailSize, tail.data(), tail.size()))
                return fail(why, "cannot read ZIP end record");

            std::size_t eocd = tail.size();
            for (std::size_t p = tail.size() - 22;; --p) {
                if (u32(tail.data() + p) == 0x06054b50u
                    && p + 22u + u16(tail.data() + p + 20) == tail.size()) {
                    eocd = p;
                    break;
                }
                if (p == 0) break;
            }
            if (eocd == tail.size()) return fail(why, "ZIP end record is missing");
            const std::uint8_t *eo = tail.data() + eocd;
            const std::uint16_t count = u16(eo + 10);
            const std::uint32_t directoryBytes = u32(eo + 12);
            const std::uint32_t directoryOffset = u32(eo + 16);
            if (u16(eo + 4) != 0 || u16(eo + 6) != 0 || u16(eo + 8) != count)
                return fail(why, "split ZIP archives are unsupported");
            if (count == 0xffffu || directoryBytes == 0xffffffffu || directoryOffset == 0xffffffffu)
                return fail(why, "ZIP64 archives are unsupported");
            const std::uint64_t endRecordOffset = length - tailSize + eocd;
            if (directoryBytes > maxDirectoryBytes
                || std::uint64_t(directoryOffset) + directoryBytes > endRecordOffset
                || std::uint64_t(count) * 46u > directoryBytes)
                return fail(why, "ZIP central directory is outside the file");

            std::vector<std::uint8_t> directory(directoryBytes);
            if (!readAt(file, directoryOffset, directory.data(), directory.size()))
                return fail(why, "cannot read ZIP central directory");
            std::vector<Entry> parsed;
            std::unordered_map<std::string, std::size_t> names;
            std::size_t p = 0;
            for (std::size_t n = 0; n < count; ++n) {
                if (p > directory.size() || directory.size() - p < 46u
                    || u32(directory.data() + p) != 0x02014b50u)
                    return fail(why, "invalid ZIP central directory entry");
                const std::uint8_t *record = directory.data() + p;
                const std::size_t nameBytes = u16(record + 28);
                const std::size_t recordBytes = 46u + nameBytes + u16(record + 30) + u16(record + 32);
                if (recordBytes > directory.size() - p || nameBytes == 0)
                    return fail(why, "truncated ZIP member name or metadata");
                Entry entry;
                entry.name.assign(reinterpret_cast<const char *>(record + 46), nameBytes);
                entry.flags = u16(record + 8);
                entry.method = u16(record + 10);
                entry.crc = u32(record + 16);
                entry.compressed = u32(record + 20);
                entry.uncompressed = u32(record + 24);
                entry.offset = u32(record + 42);
                if (u16(record + 34) != 0 || entry.compressed == 0xffffffffu
                    || entry.uncompressed == 0xffffffffu || entry.offset == 0xffffffffu)
                    return fail(why, "split or ZIP64 member is unsupported");
                if (entry.name.find('\0') != std::string::npos)
                    return fail(why, "ZIP member name contains a null byte");
                if (std::uint64_t(entry.offset) + 30u > directoryOffset)
                    return fail(why, "ZIP local header is outside the data area");
                if (isMesh(entry.name)) {
                    const std::string key = stem(entry.name);
                    const auto existing = names.emplace(key, parsed.size());
                    // A bare game mesh name must never select an arbitrary one
                    // of two differently located or differently cased members.
                    if (!existing.second)
                        existing.first->second = (std::numeric_limits<std::size_t>::max)();
                    parsed.push_back(std::move(entry));
                }
                p += recordBytes;
            }
            if (p != directory.size()) return fail(why, "ZIP central directory size mismatch");
            entries_.swap(parsed);
            index_.swap(names);
            fileBytes_ = length;
            directoryOffset_ = directoryOffset;
            path_ = path;
            return true;
        } catch (const std::exception &) {
            return fail(why, "cannot allocate or read ZIP metadata");
        }
    }

    bool extract(const std::string &meshStem, std::vector<std::uint8_t> &bytes,
                 std::string *member = nullptr, std::string *why = nullptr) const
    {
        bytes.clear();
        if (member) member->clear();
        if (why) why->clear();
        if (path_.empty()) return fail(why, "ZIP archive is not open");
        const auto found = index_.find(stem(meshStem));
        if (found == index_.end()) return fail(why, "PCMESH member was not found");
        if (found->second >= entries_.size()) return fail(why, "PCMESH member name is ambiguous");
        const Entry &entry = entries_[found->second];
        if (entry.flags & (1u | 0x40u | 0x2000u)) return fail(why, "encrypted ZIP members are unsupported");
        if (entry.method != 0 && entry.method != 8) return fail(why, "unsupported ZIP compression method");
        if (entry.uncompressed > maxMemberBytes || entry.compressed > maxMemberBytes)
            return fail(why, "PCMESH member exceeds the 64 MiB limit");
        try {
            std::ifstream file(path_, std::ios::binary | std::ios::ate);
            if (!file || file.tellg() != std::streamoff(fileBytes_))
                return fail(why, "ZIP file changed or cannot be reopened");
            std::array<std::uint8_t, 30> local{};
            if (!readAt(file, entry.offset, local.data(), local.size())
                || u32(local.data()) != 0x04034b50u)
                return fail(why, "invalid ZIP local header");
            if (u16(local.data() + 6) != entry.flags || u16(local.data() + 8) != entry.method)
                return fail(why, "ZIP local and central metadata disagree");
            if (!(entry.flags & 8u)
                && (u32(local.data() + 14) != entry.crc
                    || u32(local.data() + 18) != entry.compressed
                    || u32(local.data() + 22) != entry.uncompressed))
                return fail(why, "ZIP local size or CRC does not match the directory");
            const std::size_t nameBytes = u16(local.data() + 26);
            const std::uint64_t dataOffset = std::uint64_t(entry.offset) + 30u
                + nameBytes + u16(local.data() + 28);
            if (dataOffset > directoryOffset_
                || entry.compressed > directoryOffset_ - dataOffset)
                return fail(why, "ZIP compressed member is outside the data area");
            std::string localName(nameBytes, '\0');
            if (!readAt(file, std::uint64_t(entry.offset) + 30u, localName.data(), nameBytes)
                || localName != entry.name)
                return fail(why, "ZIP local member name does not match the directory");
            std::vector<std::uint8_t> compressed(entry.compressed), decoded;
            if (!readAt(file, dataOffset, compressed.data(), compressed.size()))
                return fail(why, "cannot read ZIP compressed member");
            if (entry.method == 0) {
                if (entry.compressed != entry.uncompressed)
                    return fail(why, "stored ZIP member size mismatch");
                decoded.swap(compressed);
            } else {
                std::size_t consumed = 0;
                if (compressed.empty()
                    || !inflate_impl::inflate_raw(compressed.data(), compressed.size(), decoded,
                                               entry.uncompressed, entry.uncompressed, &consumed)
                    || consumed != compressed.size() || decoded.size() != entry.uncompressed)
                    return fail(why, "invalid DEFLATE stream or expanded size mismatch");
            }
            if (crc32(decoded) != entry.crc) return fail(why, "ZIP member CRC mismatch");
            bytes.swap(decoded);
            if (member) *member = entry.name;
            return true;
        } catch (const std::exception &) {
            return fail(why, "cannot allocate or decode ZIP member");
        }
    }
};

}} // namespace modmesh::pcmeshzip
