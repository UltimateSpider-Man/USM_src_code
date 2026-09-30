#pragma once

#include <array>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <utility>

namespace openusm::frontend_throb {

inline constexpr std::uint32_t hash = 0x39E51482u;
inline constexpr std::uint32_t samples = 12160u;
inline constexpr std::uint32_t sample_rate = 8000u;
inline constexpr std::uint32_t encoded_bytes = 6080u;

inline std::uint32_t read_u32(const std::uint8_t *p)
{
    return std::uint32_t(p[0]) | (std::uint32_t(p[1]) << 8)
        | (std::uint32_t(p[2]) << 16) | (std::uint32_t(p[3]) << 24);
}

// This is deliberately a validator for the supplied, single-wave PC bank,
// not a general Xbox/PC codec converter. Never relabel Xbox ADPCM as IMA.
inline bool validate_bank(const std::filesystem::path &path, std::string &reason)
{
    reason.clear();
    const auto reject = [&](const char *why) { reason = why; return false; };
    std::error_code ec;
    const auto size = std::filesystem::file_size(path, ec);
    if (ec || size != 0x3000u)
        return reject("expected the 12288-byte FE_MM_THROB bank");

    std::ifstream input(path, std::ios::binary);
    std::array<std::uint8_t, 0x1000> header{};
    if (!input.read(reinterpret_cast<char *>(header.data()), header.size()))
        return reject("unreadable bank header");
    const auto *h = header.data();
    if (std::memcmp(h, "WAVEBK11", 8) != 0 ||
        std::memcmp(h + 0x20, "FE_MM_THROB\0", 12) != 0)
        return reject("wrong bank name or format");
    if (read_u32(h + 0x10) != header.size() ||
        read_u32(h + 0x14) != encoded_bytes ||
        read_u32(h + 0x18) != size || read_u32(h + 0x1c) != 0 ||
        read_u32(h + 0x40) != 1 || read_u32(h + 0x44) != 0x100)
        return reject("invalid single-wave resident-bank layout");

    const std::uint64_t param_size = read_u32(h + 0x50);
    const std::uint64_t params = read_u32(h + 0x54);
    const std::uint64_t string_size = read_u32(h + 0x60);
    const std::uint64_t strings = read_u32(h + 0x64);
    if (params < 0x128 || params + param_size > header.size() ||
        strings < params + param_size || strings + string_size > header.size())
        return reject("bank metadata outside the header");

    const auto *wave = h + 0x100;
    if (read_u32(wave) != hash || read_u32(wave + 4) != 0x00000807u ||
        read_u32(wave + 8) != encoded_bytes || read_u32(wave + 12) != samples ||
        read_u32(wave + 0x18) != 0xffffffffu || read_u32(wave + 0x1c) != 0 ||
        read_u32(wave + 0x20) != sample_rate || read_u32(wave + 0x24) != 0)
        return reject("unexpected FE_MM_THROB codec, flags, size or rate");
    const std::uint64_t parameter = read_u32(wave + 0x14);
    const std::uint64_t group = read_u32(wave + 0x10);
    if (parameter + 0x20 > param_size || group + 10 > string_size ||
        std::memcmp(h + strings + group, "INTERFACE\0", 10) != 0)
        return reject("invalid wave parameter or INTERFACE group");
    // Preserve the original UI source parameters (type, volume, pitch,
    // distances), not just the compressed payload.
    static constexpr std::array<std::uint8_t, 32> expected_parameters = {{
        0x07,0x00,0x00,0x0e, 0x00,0x00,0x00,0x00,
        0x00,0x00,0x80,0x3f, 0x00,0x00,0x80,0x3f,
        0x00,0x00,0x00,0x00, 0x00,0x00,0x20,0x41,
        0x00,0x00,0xf0,0x41, 0x00,0x00,0x80,0x3f
    }};
    if (std::memcmp(h + params + parameter, expected_parameters.data(), 32) != 0)
        return reject("unexpected UI sound parameters");
    return true;
}

inline bool resolve_bank(const std::filesystem::path &base,
                         std::filesystem::path &path, std::string &reason)
{
    static constexpr const char *candidates[] = {
        "extra/FE_MM_THROB.WBK", "extra/sound/FE_MM_THROB.WBK",
        "mods/FE_MM_THROB.WBK", "FE_MM_THROB.WBK"
    };
    path.clear();
    reason.clear();
    for (const auto *candidate : candidates) {
        const auto file = base / candidate;
        std::error_code ec;
        if (!std::filesystem::is_regular_file(file, ec) || ec)
            continue;
        std::string error;
        if (!validate_bank(file, error)) {
            reason += std::string(candidate) + ": " + error + "; ";
            continue;
        }
        auto absolute = std::filesystem::absolute(file, ec);
        if (ec || absolute.string().size() >= 256) {
            reason += std::string(candidate) + ": path exceeds the native NFL limit; ";
            continue;
        }
        path = std::move(absolute);
        return true;
    }
    if (reason.empty())
        reason = "copy game_files/extra/FE_MM_THROB.WBK into the game's extra folder";
    return false;
}

} // namespace openusm::frontend_throb
