#pragma once

// Only the external PC STREAMS_MUSIC bank uses this path. Keep its native
// hashes, codec, loop flags, parameters and payload intact; NSL owns playback.
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <limits>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace openusm::streams_music {

inline std::uint32_t read_u32(const std::uint8_t *p)
{
    return std::uint32_t(p[0]) | (std::uint32_t(p[1]) << 8)
        | (std::uint32_t(p[2]) << 16) | (std::uint32_t(p[3]) << 24);
}

inline bool validate_bank(const std::filesystem::path &path,
                          std::uint32_t &wave_count, std::string &reason)
{
    wave_count = 0;
    reason.clear();
    auto reject = [&](const char *message) { reason = message; return false; };
    std::error_code ec;
    const auto file_size = std::filesystem::file_size(path, ec);
    if (ec || file_size < 0x1000 || file_size > (std::numeric_limits<int>::max)())
        return reject("unreadable bank or unsupported file size");

    std::ifstream input(path, std::ios::binary);
    std::array<std::uint8_t, 0x100> prefix{};
    if (!input.read(reinterpret_cast<char *>(prefix.data()), prefix.size()))
        return reject("incomplete bank header");
    if (std::memcmp(prefix.data(), "WAVEBK11", 8) != 0
        || std::memcmp(prefix.data() + 0x20, "STREAMS_MUSIC\0", 14) != 0)
        return reject("expected a WAVEBK11 STREAMS_MUSIC bank");

    const std::uint64_t header_size = read_u32(prefix.data() + 0x10);
    const std::uint64_t stream_start = read_u32(prefix.data() + 0x18);
    const std::uint64_t stream_size = read_u32(prefix.data() + 0x1c);
    const auto count = read_u32(prefix.data() + 0x40);
    const std::uint64_t entries = read_u32(prefix.data() + 0x44);
    const std::uint64_t parameter_size = read_u32(prefix.data() + 0x50);
    const std::uint64_t parameters = read_u32(prefix.data() + 0x54);
    const std::uint64_t string_size = read_u32(prefix.data() + 0x60);
    const std::uint64_t strings = read_u32(prefix.data() + 0x64);
    if (header_size < 0x1000 || header_size > 16 * 1024 * 1024
        || header_size > file_size || (header_size & 0xfff) != 0
        || read_u32(prefix.data() + 0x14) != 0
        || stream_start < header_size || stream_start + stream_size > file_size
        || count == 0 || count > 65535 || entries < 0x100
        || entries + std::uint64_t(count) * 0x28 > header_size
        || parameters < entries + std::uint64_t(count) * 0x28
        || parameters + parameter_size > header_size
        || strings < parameters + parameter_size || strings + string_size > header_size)
        return reject("invalid streamed bank tables or bounds");

    std::vector<std::uint8_t> header(static_cast<std::size_t>(header_size));
    input.seekg(0);
    if (!input.read(reinterpret_cast<char *>(header.data()), header.size()))
        return reject("incomplete bank metadata");

    std::set<std::uint32_t> hashes;
    std::vector<std::pair<std::uint64_t, std::uint64_t>> spans;
    spans.reserve(count);
    for (std::uint32_t i = 0; i < count; ++i) {
        const auto *entry = header.data() + entries + std::uint64_t(i) * 0x28;
        const std::uint64_t bytes = read_u32(entry + 8);
        const auto frames = read_u32(entry + 0x0c);
        const std::uint64_t offset = read_u32(entry + 0x1c);
        const auto rate = read_u32(entry + 0x20);
        const std::uint64_t param = read_u32(entry + 0x14);
        const auto group = read_u32(entry + 0x10);
        const auto coefficients = read_u32(entry + 0x18);
        // PC raw IMA stereo: one compressed byte per stereo sample frame.
        // Xbox block ADPCM (5) must not be relabelled as PC codec 7.
        if (entry[4] != 7 || (entry[5] & 2) == 0
            || entry[6] != 3 || entry[7] != 0)
            return reject("expected streamed stereo PC IMA codec 0x07");
        if (!hashes.insert(read_u32(entry)).second || frames == 0 || bytes != frames
            || rate < 1000 || rate > 192000 || coefficients != 0xffffffffu
            || offset < stream_start || (offset & 0xfff) != 0
            || offset + bytes > stream_start + stream_size
            || param + 0x20 > parameter_size)
            return reject("invalid wave metadata or payload bounds");
        if (group != 0xffffffffu
            && (group >= string_size || std::memchr(header.data() + strings + group,
                    0, static_cast<std::size_t>(string_size - group)) == nullptr))
            return reject("invalid wave group offset");
        spans.emplace_back(offset, offset + bytes);
    }
    std::sort(spans.begin(), spans.end());
    for (std::size_t i = 1; i < spans.size(); ++i)
        if (spans[i].first < spans[i - 1].second)
            return reject("overlapping wave payloads");
    wave_count = count;
    return true;
}

inline bool resolve_external_bank(std::filesystem::path &path,
                                  std::uint32_t &wave_count, std::string &diagnostic,
                                  const std::filesystem::path &base = {})
{
    path.clear();
    wave_count = 0;
    diagnostic.clear();
    // Explicit external locations only. Ordinary SOUND/PC/STREAMS remains
    // under the original loader's control when no valid override exists.
    static constexpr const char *candidates[] = {
        "extra/STREAMS_MUSIC.WBK", "extra/sound/STREAMS_MUSIC.WBK",
        "extra/SOUND/PC/STREAMS/STREAMS_MUSIC.WBK",
        "extra/data/SOUND/PC/STREAMS/STREAMS_MUSIC.WBK",
        "mods/STREAMS_MUSIC.WBK", "mods/sound/STREAMS_MUSIC.WBK",
        "mods/SOUND/PC/STREAMS/STREAMS_MUSIC.WBK",
        "mods/data/SOUND/PC/STREAMS/STREAMS_MUSIC.WBK", "STREAMS_MUSIC.WBK"
    };
    for (const auto *candidate : candidates) {
        const auto candidate_path = base / candidate;
        std::error_code ec;
        if (!std::filesystem::is_regular_file(candidate_path, ec) || ec)
            continue;
        std::string reason;
        if (!validate_bank(candidate_path, wave_count, reason)) {
            diagnostic += std::string(candidate) + ": " + reason + "; ";
            continue;
        }
        auto resolved = std::filesystem::absolute(candidate_path, ec);
        // Native NFL uses a 256-byte path buffer. Keep its terminator in range.
        if (ec || resolved.string().size() >= 256) {
            diagnostic += std::string(candidate) + ": native path too long; ";
            wave_count = 0;
            continue;
        }
        path = std::move(resolved);
        return true;
    }
    return false;
}

} // namespace openusm::streams_music
