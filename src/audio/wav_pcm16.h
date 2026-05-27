#pragma once

#include <cstdint>
#include <string>
#include <vector>

// Minimal RIFF/WAVE reader (PCM 16-bit only).
//
// Supported:
//   - RIFF WAVE
//   - fmt chunk: PCM (format=1), 16-bit
//   - data chunk
//
// Returns true on success and fills outSamples with interleaved signed 16-bit PCM.
bool load_wav_pcm16(const std::string& path,
                    uint16_t& outChannels,
                    uint32_t& outSampleRate,
                    std::vector<int16_t>& outSamples);
