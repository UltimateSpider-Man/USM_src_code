#include "wav_pcm16.h"

#include <fstream>
#include <cstring>

namespace {

template <typename T>
bool read_le(std::ifstream& f, T& out) {
    f.read(reinterpret_cast<char*>(&out), sizeof(T));
    return (bool)f;
}

bool read_exact(std::ifstream& f, void* dst, size_t n) {
    f.read(reinterpret_cast<char*>(dst), (std::streamsize)n);
    return (bool)f;
}

uint32_t fourcc(const char a, const char b, const char c, const char d) {
    return (uint32_t)(uint8_t)a | ((uint32_t)(uint8_t)b << 8) | ((uint32_t)(uint8_t)c << 16) | ((uint32_t)(uint8_t)d << 24);
}

}

bool load_wav_pcm16(const std::string& path,
                    uint16_t& outChannels,
                    uint32_t& outSampleRate,
                    std::vector<int16_t>& outSamples)
{
    outChannels = 0;
    outSampleRate = 0;
    outSamples.clear();

    std::ifstream f(path, std::ios::binary);
    if (!f) return false;

    // RIFF header
    uint32_t riff = 0, riffSize = 0, wave = 0;
    if (!read_le(f, riff) || !read_le(f, riffSize) || !read_le(f, wave)) return false;
    if (riff != fourcc('R','I','F','F') || wave != fourcc('W','A','V','E')) return false;

    // Chunks
    bool haveFmt = false;
    bool haveData = false;

    uint16_t fmtAudioFormat = 0;
    uint16_t fmtNumChannels = 0;
    uint32_t fmtSampleRate = 0;
    uint32_t fmtByteRate = 0;
    uint16_t fmtBlockAlign = 0;
    uint16_t fmtBitsPerSample = 0;

    std::vector<uint8_t> dataBytes;

    while (f && !(haveFmt && haveData)) {
        uint32_t id = 0, size = 0;
        if (!read_le(f, id) || !read_le(f, size)) break;

        if (id == fourcc('f','m','t',' ')) {
            // PCM fmt chunk is at least 16 bytes.
            if (size < 16) return false;
            if (!read_le(f, fmtAudioFormat) ||
                !read_le(f, fmtNumChannels) ||
                !read_le(f, fmtSampleRate) ||
                !read_le(f, fmtByteRate) ||
                !read_le(f, fmtBlockAlign) ||
                !read_le(f, fmtBitsPerSample))
                return false;

            // Skip any extra fmt bytes
            if (size > 16) {
                f.seekg((std::streamoff)(size - 16), std::ios::cur);
                if (!f) return false;
            }

            haveFmt = true;
        } else if (id == fourcc('d','a','t','a')) {
            dataBytes.resize(size);
            if (size > 0 && !read_exact(f, dataBytes.data(), size)) return false;
            haveData = true;
        } else {
            // Skip unknown chunk (pad to even)
            f.seekg((std::streamoff)size, std::ios::cur);
            if (!f) return false;
        }

        // Chunks are word-aligned
        if (size & 1) {
            f.seekg(1, std::ios::cur);
            if (!f) return false;
        }
    }

    if (!haveFmt || !haveData) return false;

    // Validate PCM16
    if (fmtAudioFormat != 1) return false; // PCM
    if (fmtBitsPerSample != 16) return false;
    if (fmtNumChannels == 0 || fmtSampleRate == 0) return false;
    if (fmtBlockAlign != (uint16_t)(fmtNumChannels * 2)) {
        // Some files lie, but we will assume standard interleaved PCM16.
    }

    if (dataBytes.size() % 2 != 0) return false;

    outChannels = fmtNumChannels;
    outSampleRate = fmtSampleRate;

    outSamples.resize(dataBytes.size() / 2);
    std::memcpy(outSamples.data(), dataBytes.data(), dataBytes.size());
    return true;
}
