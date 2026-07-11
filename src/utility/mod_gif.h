#pragma once

// mod_gif.h - GIF texture mods for openusm
// ------------------------------------------------------------------
// Lets you drop .gif files into the mods/ folder and have them
// override game textures, exactly like .dds mods:
//
//      mods/spidey_face.gif            -> texture "spidey_face"
//      mods/textures/web_logo.gif      -> texture "textures/web_logo"
//
// Static GIFs behave like a plain texture swap. Animated GIFs are
// played back at runtime: frame 0 is fed to nglLoadTextureTM2 as a
// synthesized DDS (A8R8G8B8) blob, and subsequent frames are uploaded
// to the live IDirect3DTexture9 every frame via modGifTick()
// (called from ngl.cpp).
//
// Header-only, C++17 (inline variables), no external dependencies:
// full GIF87a/GIF89a decoder (LZW, interlacing, local/global palettes,
// transparency, frame disposal modes 0-3) lives in here.
// ------------------------------------------------------------------

#include <cstdint>
#include <cstring>
#include <cstdio>
#include <vector>
#include <unordered_map>
#include <filesystem>
#include <fstream>

// ------------------------------------------------------------------
// Decoded animation
// ------------------------------------------------------------------

struct ModGifAnim {
    // pow2-adjusted output dimensions (what the engine texture uses)
    int width  = 0;
    int height = 0;

    // original GIF canvas dimensions (pre-resize), informational
    int srcWidth  = 0;
    int srcHeight = 0;

    // one BGRA8 buffer per frame, width*height*4 bytes, tightly packed.
    // BGRA byte order == D3DFMT_A8R8G8B8 little-endian memory layout.
    std::vector<std::vector<uint8_t>> frames;

    // per-frame delay in milliseconds (already clamped/sanitized)
    std::vector<int> delaysMs;

    // frame 0 wrapped as a DDS blob for nglLoadTextureTM2 /
    // D3DXCreateTextureFromFileInMemory
    std::vector<uint8_t> dds;

    // ---- runtime playback state --------------------------------------
    uint32_t startTick     = 0;   // GetTickCount() of first tick
    int      totalMs       = 0;   // sum of delays
    int      lastFrameSent = -1;  // last frame uploaded to the GPU

    bool animated() const { return frames.size() > 1 && totalMs > 0; }
};

// hash (same to_hash/tlFixedString hash space used by Mods) -> anim
inline std::unordered_map<uint32_t, ModGifAnim> ModGifAnims;

// ------------------------------------------------------------------
// GIF decoder internals
// ------------------------------------------------------------------

namespace modgif_detail {

struct Reader {
    const uint8_t *p, *end;
    bool ok = true;

    explicit Reader(const std::vector<uint8_t> &v) : p(v.data()), end(v.data() + v.size()) {}

    uint8_t u8() {
        if (p >= end) { ok = false; return 0; }
        return *p++;
    }
    uint16_t u16() {                     // little-endian
        uint16_t lo = u8(), hi = u8();
        return static_cast<uint16_t>(lo | (hi << 8));
    }
    void skip(size_t n) {
        if (static_cast<size_t>(end - p) < n) { ok = false; p = end; return; }
        p += n;
    }
    bool read(uint8_t *dst, size_t n) {
        if (static_cast<size_t>(end - p) < n) { ok = false; return false; }
        std::memcpy(dst, p, n);
        p += n;
        return true;
    }
};

// LZW decompressor for one GIF image data stream (sub-block chain).
// Appends decoded palette indices to `out`. Returns false on stream error.
inline bool lzwDecode(Reader &r, int minCodeSize, std::vector<uint8_t> &out, size_t maxPixels)
{
    if (minCodeSize < 2 || minCodeSize > 11)
        return false;

    // gather sub-blocks into one contiguous buffer
    std::vector<uint8_t> data;
    for (;;) {
        uint8_t len = r.u8();
        if (!r.ok) return false;
        if (len == 0) break;
        size_t base = data.size();
        data.resize(base + len);
        if (!r.read(data.data() + base, len)) return false;
    }

    const int clearCode = 1 << minCodeSize;
    const int endCode   = clearCode + 1;

    // dictionary: prefix chain + suffix byte
    static constexpr int MAX_CODES = 4096;
    int16_t prefix[MAX_CODES];
    uint8_t suffix[MAX_CODES];
    uint8_t stack [MAX_CODES + 1];

    int codeSize  = minCodeSize + 1;
    int nextCode  = endCode + 1;
    int prevCode  = -1;

    for (int i = 0; i < clearCode; ++i) {
        prefix[i] = -1;
        suffix[i] = static_cast<uint8_t>(i);
    }

    uint32_t bitBuf = 0;
    int      bitCnt = 0;
    size_t   pos    = 0;

    auto emit = [&](int code) -> bool {
        // walk prefix chain onto stack, then pop
        int sp = 0;
        while (code >= 0) {
            if (sp > MAX_CODES) return false;
            stack[sp++] = suffix[code];
            code = prefix[code];
        }
        while (sp > 0) {
            if (out.size() >= maxPixels) return true; // clip overlong streams
            out.push_back(stack[--sp]);
        }
        return true;
    };

    for (;;) {
        while (bitCnt < codeSize) {
            if (pos >= data.size())
                return true;              // stream ended (tolerate truncation)
            bitBuf |= static_cast<uint32_t>(data[pos++]) << bitCnt;
            bitCnt += 8;
        }

        int code = static_cast<int>(bitBuf & ((1u << codeSize) - 1));
        bitBuf >>= codeSize;
        bitCnt  -= codeSize;

        if (code == clearCode) {
            codeSize = minCodeSize + 1;
            nextCode = endCode + 1;
            prevCode = -1;
            continue;
        }
        if (code == endCode)
            return true;

        if (prevCode < 0) {
            if (code >= nextCode) return false;
            if (!emit(code)) return false;
            prevCode = code;
            continue;
        }

        int emitCode = code;
        if (code >= nextCode) {
            // KwKwK case: emit prev + firstChar(prev)
            if (code > nextCode) return false;
            emitCode = prevCode;
        }

        // find first char of emitCode
        int c = emitCode;
        while (prefix[c] >= 0) c = prefix[c];
        uint8_t firstChar = suffix[c];

        if (code >= nextCode) {
            if (!emit(prevCode)) return false;
            if (out.size() < maxPixels) out.push_back(firstChar);
        } else {
            if (!emit(code)) return false;
        }

        if (nextCode < MAX_CODES) {
            prefix[nextCode] = static_cast<int16_t>(prevCode);
            suffix[nextCode] = firstChar;
            ++nextCode;
            if (nextCode == (1 << codeSize) && codeSize < 12)
                ++codeSize;
        }

        prevCode = code;

        if (out.size() >= maxPixels && pos >= data.size())
            return true;
    }
}

inline int nextPow2(int v)
{
    int p = 1;
    while (p < v && p < 4096) p <<= 1;
    return p;
}

// bilinear resize BGRA src(w,h) -> dst(dw,dh)
inline void resizeBilinear(const uint8_t *src, int w, int h,
                           uint8_t *dst, int dw, int dh)
{
    if (w == dw && h == dh) {
        std::memcpy(dst, src, static_cast<size_t>(w) * h * 4);
        return;
    }

    for (int y = 0; y < dh; ++y) {
        float fy = (dh > 1) ? (static_cast<float>(y) * (h - 1) / (dh - 1)) : 0.f;
        int   y0 = static_cast<int>(fy);
        int   y1 = (y0 + 1 < h) ? y0 + 1 : y0;
        float ty = fy - y0;

        for (int x = 0; x < dw; ++x) {
            float fx = (dw > 1) ? (static_cast<float>(x) * (w - 1) / (dw - 1)) : 0.f;
            int   x0 = static_cast<int>(fx);
            int   x1 = (x0 + 1 < w) ? x0 + 1 : x0;
            float tx = fx - x0;

            const uint8_t *p00 = src + (static_cast<size_t>(y0) * w + x0) * 4;
            const uint8_t *p10 = src + (static_cast<size_t>(y0) * w + x1) * 4;
            const uint8_t *p01 = src + (static_cast<size_t>(y1) * w + x0) * 4;
            const uint8_t *p11 = src + (static_cast<size_t>(y1) * w + x1) * 4;

            uint8_t *o = dst + (static_cast<size_t>(y) * dw + x) * 4;
            for (int c = 0; c < 4; ++c) {
                float a = p00[c] + (p10[c] - p00[c]) * tx;
                float b = p01[c] + (p11[c] - p01[c]) * tx;
                float v = a + (b - a) * ty;
                o[c] = static_cast<uint8_t>(v + 0.5f);
            }
        }
    }
}

// Build a minimal DDS (A8R8G8B8, 1 mip, no cubemap) around BGRA pixels.
// Layout matches nglTextureInfo in ngl.cpp:
//   0x00 'DDS ', 0x04 dwSize=124, 0x08 dwFlags, 0x0C h, 0x10 w,
//   0x14 pitch, 0x18 depth, 0x1C mips, 0x4C pfSize=32, 0x50 pfFlags=0x41,
//   0x54 fourCC=0, 0x58 bitCount=32, 0x5C..0x68 masks, 0x6C caps,
//   0x70 caps2 (=0, no cubemap), pixels @ 0x80.
inline std::vector<uint8_t> buildDDS(const uint8_t *bgra, int w, int h)
{
    std::vector<uint8_t> blob(0x80 + static_cast<size_t>(w) * h * 4, 0);
    auto put32 = [&](size_t off, uint32_t v) { std::memcpy(blob.data() + off, &v, 4); };

    put32(0x00, 0x20534444u);                     // 'DDS '
    put32(0x04, 124);                             // header size
    put32(0x08, 0x1 | 0x2 | 0x4 | 0x8 | 0x1000);  // CAPS|HEIGHT|WIDTH|PITCH|PIXELFORMAT
    put32(0x0C, static_cast<uint32_t>(h));
    put32(0x10, static_cast<uint32_t>(w));
    put32(0x14, static_cast<uint32_t>(w * 4));    // pitch
    put32(0x18, 0);                               // depth (must be 0, see loader)
    put32(0x1C, 1);                               // mip count -> Tex->m_numLevel
    put32(0x4C, 32);                              // pixelformat dwSize
    put32(0x50, 0x41);                            // DDPF_RGB | DDPF_ALPHAPIXELS
    put32(0x54, 0);                               // no fourCC (uncompressed)
    put32(0x58, 32);                              // bit count
    put32(0x5C, 0x00FF0000u);                     // R mask
    put32(0x60, 0x0000FF00u);                     // G mask
    put32(0x64, 0x000000FFu);                     // B mask
    put32(0x68, 0xFF000000u);                     // A mask
    put32(0x6C, 0x1000);                          // DDSCAPS_TEXTURE
    put32(0x70, 0);                               // caps2: not a cubemap

    std::memcpy(blob.data() + 0x80, bgra, static_cast<size_t>(w) * h * 4);
    return blob;
}

} // namespace modgif_detail

// ------------------------------------------------------------------
// Decode a whole GIF file into a ModGifAnim (canvas-composited BGRA
// frames + delays). Returns false if the file is not a valid GIF.
// ------------------------------------------------------------------

inline bool modGifDecode(const std::vector<uint8_t> &file, ModGifAnim &outAnim)
{
    using namespace modgif_detail;

    static constexpr int    MAX_FRAMES   = 256;
    static constexpr size_t MAX_CANVAS   = 4096u * 4096u;

    if (file.size() < 13)
        return false;
    if (std::memcmp(file.data(), "GIF87a", 6) != 0 &&
        std::memcmp(file.data(), "GIF89a", 6) != 0)
        return false;

    Reader r(file);
    r.skip(6);

    const int canvasW = r.u16();
    const int canvasH = r.u16();
    const uint8_t lsdFlags = r.u8();
    const uint8_t bgIndex  = r.u8();
    r.skip(1); // aspect

    if (!r.ok || canvasW <= 0 || canvasH <= 0 ||
        static_cast<size_t>(canvasW) * canvasH > MAX_CANVAS)
        return false;

    uint8_t gct[256][3]{};
    bool hasGct = (lsdFlags & 0x80) != 0;
    int  gctSize = hasGct ? (2 << (lsdFlags & 7)) : 0;
    for (int i = 0; i < gctSize; ++i)
        if (!r.read(gct[i], 3)) return false;

    // canvas we composite into (BGRA)
    const size_t canvasBytes = static_cast<size_t>(canvasW) * canvasH * 4;
    std::vector<uint8_t> canvas(canvasBytes, 0);      // start fully transparent
    std::vector<uint8_t> prevCanvas;                  // for disposal 3

    // pending Graphic Control Extension state
    int  gceDelayMs     = 100;
    int  gceTransparent = -1;
    int  gceDisposal    = 0;
    bool gcePresent     = false;

    outAnim.srcWidth  = canvasW;
    outAnim.srcHeight = canvasH;
    outAnim.frames.clear();
    outAnim.delaysMs.clear();

    std::vector<uint8_t> indices;   // reused LZW output buffer

    for (;;) {
        uint8_t block = r.u8();
        if (!r.ok || block == 0x3B)   // trailer (or EOF -> accept what we have)
            break;

        if (block == 0x21) {          // extension
            uint8_t label = r.u8();
            if (label == 0xF9) {      // Graphic Control Extension
                uint8_t sz = r.u8();  // == 4
                uint8_t packed = r.u8();
                uint16_t delay = r.u16();
                uint8_t transp = r.u8();
                if (sz >= 4) r.skip(sz - 4);
                r.u8();               // block terminator

                gcePresent     = true;
                gceDisposal    = (packed >> 2) & 7;
                gceTransparent = (packed & 1) ? transp : -1;
                gceDelayMs     = delay * 10;
                if (gceDelayMs < 20) gceDelayMs = 100; // browsers' convention for 0/1
            } else {
                // skip any other extension's sub-blocks
                for (;;) {
                    uint8_t len = r.u8();
                    if (!r.ok || len == 0) break;
                    r.skip(len);
                }
            }
            continue;
        }

        if (block != 0x2C)            // not an image descriptor -> corrupt
            break;

        // ---- image descriptor ----------------------------------------
        int ix = r.u16(), iy = r.u16();
        int iw = r.u16(), ih = r.u16();
        uint8_t idFlags = r.u8();
        if (!r.ok || iw <= 0 || ih <= 0)
            break;

        // clamp sub-image rect to canvas
        if (ix >= canvasW || iy >= canvasH) { ix = iy = 0; }
        int copyW = (ix + iw > canvasW) ? canvasW - ix : iw;
        int copyH = (iy + ih > canvasH) ? canvasH - iy : ih;

        uint8_t lct[256][3];
        const uint8_t (*pal)[3] = gct;
        if (idFlags & 0x80) {
            int lctSize = 2 << (idFlags & 7);
            for (int i = 0; i < lctSize; ++i)
                if (!r.read(lct[i], 3)) return !outAnim.frames.empty();
            pal = lct;
        } else if (!hasGct) {
            // no palette at all: grayscale fallback
            for (int i = 0; i < 256; ++i)
                lct[i][0] = lct[i][1] = lct[i][2] = static_cast<uint8_t>(i);
            pal = lct;
        }

        const bool interlaced = (idFlags & 0x40) != 0;

        int minCodeSize = r.u8();
        indices.clear();
        indices.reserve(static_cast<size_t>(iw) * ih);
        if (!lzwDecode(r, minCodeSize, indices, static_cast<size_t>(iw) * ih))
            break;
        indices.resize(static_cast<size_t>(iw) * ih, 0);

        // snapshot for disposal 3
        if (gceDisposal == 3)
            prevCanvas = canvas;

        // ---- composite sub-image onto canvas ---------------------------
        auto blitRow = [&](int srcRow, int dstRow) {
            if (dstRow < 0 || dstRow >= canvasH) return;
            const uint8_t *src = indices.data() + static_cast<size_t>(srcRow) * iw;
            uint8_t *dst = canvas.data() + (static_cast<size_t>(dstRow) * canvasW + ix) * 4;
            for (int x = 0; x < copyW; ++x) {
                int idx = src[x];
                if (idx == gceTransparent)
                    continue;                         // keep underlying pixel
                uint8_t *o = dst + static_cast<size_t>(x) * 4;
                o[0] = pal[idx][2];   // B
                o[1] = pal[idx][1];   // G
                o[2] = pal[idx][0];   // R
                o[3] = 0xFF;          // A
            }
        };

        if (interlaced) {
            static const int start[4] = { 0, 4, 2, 1 };
            static const int step [4] = { 8, 8, 4, 2 };
            int srcRow = 0;
            for (int pass = 0; pass < 4; ++pass)
                for (int y = start[pass]; y < ih; y += step[pass], ++srcRow)
                    if (y < copyH) blitRow(srcRow, iy + y);
        } else {
            for (int y = 0; y < copyH; ++y)
                blitRow(y, iy + y);
        }

        // ---- store composed frame --------------------------------------
        if (static_cast<int>(outAnim.frames.size()) < MAX_FRAMES) {
            outAnim.frames.push_back(canvas);
            outAnim.delaysMs.push_back(gcePresent ? gceDelayMs : 100);
        }

        // ---- apply disposal for next frame ------------------------------
        if (gceDisposal == 2) {
            // restore to background: GIF89a spec says bg color, but every
            // modern renderer treats it as transparent; do the same.
            for (int y = 0; y < copyH; ++y) {
                int row = iy + y;
                if (row < 0 || row >= canvasH) continue;
                uint8_t *dst = canvas.data() + (static_cast<size_t>(row) * canvasW + ix) * 4;
                std::memset(dst, 0, static_cast<size_t>(copyW) * 4);
            }
            (void)bgIndex;
        } else if (gceDisposal == 3 && !prevCanvas.empty()) {
            canvas = prevCanvas;
        }
        // disposal 0/1: leave canvas as-is

        gcePresent     = false;
        gceTransparent = -1;
        gceDisposal    = 0;
        gceDelayMs     = 100;
    }

    if (outAnim.frames.empty())
        return false;

    // ---- pow2 resize + finalize ------------------------------------------
    const int dw = nextPow2(canvasW);
    const int dh = nextPow2(canvasH);
    outAnim.width  = dw;
    outAnim.height = dh;

    if (dw != canvasW || dh != canvasH) {
        for (auto &f : outAnim.frames) {
            std::vector<uint8_t> resized(static_cast<size_t>(dw) * dh * 4);
            resizeBilinear(f.data(), canvasW, canvasH, resized.data(), dw, dh);
            f = std::move(resized);
        }
    }

    outAnim.totalMs = 0;
    for (int d : outAnim.delaysMs)
        outAnim.totalMs += d;

    outAnim.dds = buildDDS(outAnim.frames[0].data(), dw, dh);
    return true;
}

// ------------------------------------------------------------------
// Registration & lookup (used by enumerate_mods / ngl.cpp)
// ------------------------------------------------------------------

// Decode a .gif from the mods folder and register it under `hash`
// (the same to_hash(relative-path-without-extension) key that .dds
// mods use). Returns true if the GIF was decoded successfully.
inline bool modGifRegisterFile(const std::filesystem::path &path, uint32_t hash)
{
    std::ifstream file(path, std::ios::binary);
    if (!file)
        return false;

    file.seekg(0, std::ios::end);
    std::streamsize sz = file.tellg();
    file.seekg(0, std::ios::beg);
    if (sz < 13)
        return false;

    std::vector<uint8_t> data(static_cast<size_t>(sz));
    file.read(reinterpret_cast<char *>(data.data()), sz);

    ModGifAnim anim;
    if (!modGifDecode(data, anim)) {
        std::printf("mod: gif FAILED to decode: %s\n", path.string().c_str());
        return false;
    }

    std::printf("mod: gif %s -> 0x%08X (%dx%d -> %dx%d, %u frame%s, %d ms loop)\n",
                path.string().c_str(), hash,
                anim.srcWidth, anim.srcHeight, anim.width, anim.height,
                static_cast<unsigned>(anim.frames.size()),
                anim.frames.size() == 1 ? "" : "s",
                anim.totalMs);

    ModGifAnims[hash] = std::move(anim);
    return true;
}

inline ModGifAnim *modGifFindAnim(uint32_t hash)
{
    auto it = ModGifAnims.find(hash);
    return it != ModGifAnims.end() ? &it->second : nullptr;
}

// DDS blob for the texture loader; `size` optional (for D3DX paths).
inline uint8_t *modGifGetDDSByHash(uint32_t hash, unsigned int *size = nullptr)
{
    auto *anim = modGifFindAnim(hash);
    if (anim == nullptr || anim->dds.empty())
        return nullptr;
    if (size != nullptr)
        *size = static_cast<unsigned int>(anim->dds.size());
    return anim->dds.data();
}

// Given the current tick count, return the frame index that should be
// on screen now, or -1 if it's the same one already uploaded.
inline int modGifFrameForNow(ModGifAnim &anim, uint32_t nowMs)
{
    if (!anim.animated())
        return -1;

    if (anim.startTick == 0) {
        anim.startTick = nowMs ? nowMs : 1;
        return anim.lastFrameSent == 0 ? -1 : 0;
    }

    int t = static_cast<int>((nowMs - anim.startTick) % static_cast<uint32_t>(anim.totalMs));
    int frame = 0;
    for (size_t i = 0; i < anim.delaysMs.size(); ++i) {
        t -= anim.delaysMs[i];
        if (t < 0) { frame = static_cast<int>(i); break; }
    }

    if (frame == anim.lastFrameSent)
        return -1;

    anim.lastFrameSent = frame;
    return frame;
}
