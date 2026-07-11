#pragma once

// mod_png.h - PNG texture mods for openusm
// ------------------------------------------------------------------
// Lets you drop .png files into the mods/ folder and have them
// override game textures, exactly like .dds and .gif mods:
//
//      mods/spidey_face.png            -> texture "spidey_face"
//      mods/textures/web_logo.png      -> texture "textures/web_logo"
//
// The PNG is decoded once at enumerate_mods() time into a BGRA8
// image, resized to power-of-2 dimensions if needed, and wrapped as
// an uncompressed A8R8G8B8 DDS blob that nglLoadTextureTM2 consumes
// through the exact same path .dds mods use.
//
// nglLoadTextureTM2 additionally converts RAW PNG bytes on sight
// (modPngWrapRaw): if a .png ends up routed through the generic mod
// data path - or a repacked resource carries PNG payload - the
// loader synthesizes the DDS on first use and caches it.
//
// Header-only, C++17, no external dependencies: a full DEFLATE
// (RFC 1951) + zlib (RFC 1950) inflater and a PNG (RFC 2083) decoder
// live in here. Supported: bit depths 1/2/4/8/16, color types
// 0 (gray) / 2 (RGB) / 3 (palette) / 4 (gray+alpha) / 6 (RGBA),
// tRNS transparency for types 0/2/3, all five scanline filters,
// interlace methods 0 (none) and 1 (Adam7).
// ------------------------------------------------------------------

#include "mod_gif.h"   // modgif_detail::{nextPow2, resizeBilinear, buildDDS}

#include <cstdint>
#include <cstring>
#include <cstdio>
#include <vector>
#include <unordered_map>
#include <filesystem>
#include <fstream>

// ------------------------------------------------------------------
// Decoded texture
// ------------------------------------------------------------------

struct ModPngTexture {
    // pow2-adjusted output dimensions (what the engine texture uses)
    int width  = 0;
    int height = 0;

    // original PNG dimensions (pre-resize), informational
    int srcWidth  = 0;
    int srcHeight = 0;

    // the image wrapped as a DDS blob for nglLoadTextureTM2 /
    // D3DXCreateTextureFromFileInMemory
    std::vector<uint8_t> dds;
};

// hash (same to_hash/tlFixedString hash space used by Mods) -> texture
inline std::unordered_map<uint32_t, ModPngTexture> ModPngTextures;

// ------------------------------------------------------------------
// PNG decoder internals
// ------------------------------------------------------------------

namespace modpng_detail {

// ---------------- DEFLATE (RFC 1951) ------------------------------

struct BitReader {
    const uint8_t *p, *end;
    uint32_t buf = 0;
    int      cnt = 0;
    bool     ok  = true;

    BitReader(const uint8_t *data, size_t size) : p(data), end(data + size) {}

    int bits(int n) {                    // LSB-first, n <= 16
        while (cnt < n) {
            if (p >= end) { ok = false; return 0; }
            buf |= static_cast<uint32_t>(*p++) << cnt;
            cnt += 8;
        }
        const int v = static_cast<int>(buf & ((1u << n) - 1u));
        buf >>= n;
        cnt -= n;
        return v;
    }
    int bit() { return bits(1); }
    void alignByte() { buf = 0; cnt = 0; }
};

// Canonical Huffman decoder (puff-style: counts per length + sorted symbols).
struct Huff {
    uint16_t count[16] = {};
    uint16_t symbol[288] = {};

    // lens[i] = code length of symbol i (0 = unused); n <= 288
    bool build(const uint8_t *lens, int n) {
        std::memset(count, 0, sizeof(count));
        for (int i = 0; i < n; ++i)
            ++count[lens[i]];
        if (count[0] == n)
            return false;                          // no codes at all

        // over-subscription check
        int left = 1;
        for (int len = 1; len < 16; ++len) {
            left <<= 1;
            left -= count[len];
            if (left < 0)
                return false;
        }

        uint16_t offs[16];
        offs[1] = 0;
        for (int len = 1; len < 15; ++len)
            offs[len + 1] = static_cast<uint16_t>(offs[len] + count[len]);

        for (int i = 0; i < n; ++i)
            if (lens[i] != 0)
                symbol[offs[lens[i]]++] = static_cast<uint16_t>(i);

        return true;
    }

    int decode(BitReader &br) const {
        int code = 0, first = 0, index = 0;
        for (int len = 1; len < 16; ++len) {
            code |= br.bit();
            if (!br.ok)
                return -1;
            const int cnt = count[len];
            if (code - first < cnt)
                return symbol[index + (code - first)];
            index += cnt;
            first = (first + cnt) << 1;
            code <<= 1;
        }
        return -1;
    }
};

inline bool inflateBlockLoop(BitReader &br, std::vector<uint8_t> &out, size_t maxOut)
{
    static const uint16_t lenBase[29] = {
        3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31,
        35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258};
    static const uint8_t lenExtra[29] = {
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2,
        3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0};
    static const uint16_t distBase[30] = {
        1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193,
        257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145,
        8193, 12289, 16385, 24577};
    static const uint8_t distExtra[30] = {
        0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6,
        7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13};

    for (;;) {
        const int final = br.bit();
        const int type  = br.bits(2);
        if (!br.ok)
            return false;

        if (type == 0) {
            // stored block
            br.alignByte();
            if (br.end - br.p < 4)
                return false;
            const uint16_t len  = static_cast<uint16_t>(br.p[0] | (br.p[1] << 8));
            const uint16_t nlen = static_cast<uint16_t>(br.p[2] | (br.p[3] << 8));
            br.p += 4;
            if (static_cast<uint16_t>(~len) != nlen)
                return false;
            if (static_cast<size_t>(br.end - br.p) < len ||
                out.size() + len > maxOut)
                return false;
            out.insert(out.end(), br.p, br.p + len);
            br.p += len;
        } else if (type == 1 || type == 2) {
            Huff lit, dist;

            if (type == 1) {
                // fixed tables
                uint8_t lens[288];
                int i = 0;
                for (; i < 144; ++i) lens[i] = 8;
                for (; i < 256; ++i) lens[i] = 9;
                for (; i < 280; ++i) lens[i] = 7;
                for (; i < 288; ++i) lens[i] = 8;
                if (!lit.build(lens, 288))
                    return false;
                uint8_t dl[30];
                std::memset(dl, 5, sizeof(dl));
                if (!dist.build(dl, 30))
                    return false;
            } else {
                // dynamic tables
                const int hlit  = br.bits(5) + 257;
                const int hdist = br.bits(5) + 1;
                const int hclen = br.bits(4) + 4;
                if (!br.ok || hlit > 286 || hdist > 30)
                    return false;

                static const uint8_t order[19] = {
                    16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};
                uint8_t clens[19] = {};
                for (int i = 0; i < hclen; ++i)
                    clens[order[i]] = static_cast<uint8_t>(br.bits(3));
                if (!br.ok)
                    return false;

                Huff cl;
                if (!cl.build(clens, 19))
                    return false;

                uint8_t lens[288 + 30] = {};
                int n = 0;
                while (n < hlit + hdist) {
                    const int sym = cl.decode(br);
                    if (sym < 0)
                        return false;
                    if (sym < 16) {
                        lens[n++] = static_cast<uint8_t>(sym);
                    } else if (sym == 16) {
                        if (n == 0)
                            return false;
                        const uint8_t prev = lens[n - 1];
                        int rep = 3 + br.bits(2);
                        if (!br.ok || n + rep > hlit + hdist)
                            return false;
                        while (rep--) lens[n++] = prev;
                    } else if (sym == 17) {
                        int rep = 3 + br.bits(3);
                        if (!br.ok || n + rep > hlit + hdist)
                            return false;
                        while (rep--) lens[n++] = 0;
                    } else {
                        int rep = 11 + br.bits(7);
                        if (!br.ok || n + rep > hlit + hdist)
                            return false;
                        while (rep--) lens[n++] = 0;
                    }
                }

                if (lens[256] == 0)                       // must have end-of-block
                    return false;
                if (!lit.build(lens, hlit))
                    return false;
                if (!dist.build(lens + hlit, hdist))
                    return false;
            }

            // symbol loop
            for (;;) {
                const int sym = lit.decode(br);
                if (sym < 0)
                    return false;
                if (sym < 256) {
                    if (out.size() >= maxOut)
                        return false;
                    out.push_back(static_cast<uint8_t>(sym));
                } else if (sym == 256) {
                    break;                                // end of block
                } else {
                    const int li = sym - 257;
                    if (li >= 29)
                        return false;
                    const int len = lenBase[li] + br.bits(lenExtra[li]);

                    const int ds = dist.decode(br);
                    if (ds < 0 || ds >= 30)
                        return false;
                    const size_t d = static_cast<size_t>(distBase[ds]) + br.bits(distExtra[ds]);
                    if (!br.ok || d == 0 || d > out.size() ||
                        out.size() + len > maxOut)
                        return false;

                    size_t src = out.size() - d;          // may overlap itself
                    for (int i = 0; i < len; ++i)
                        out.push_back(out[src + i]);
                }
            }
        } else {
            return false;                                 // reserved type
        }

        if (final)
            return true;
    }
}

// zlib stream (RFC 1950): 2-byte header, DEFLATE payload, adler32 (ignored).
inline bool zlibInflate(const uint8_t *src, size_t srcLen,
                        std::vector<uint8_t> &out, size_t maxOut)
{
    if (srcLen < 6)
        return false;
    const uint8_t cmf = src[0], flg = src[1];
    if ((cmf & 0x0F) != 8)                                // must be DEFLATE
        return false;
    if (((cmf << 8) | flg) % 31 != 0)                     // header checksum
        return false;
    if (flg & 0x20)                                       // FDICT unsupported
        return false;

    BitReader br(src + 2, srcLen - 2);
    return inflateBlockLoop(br, out, maxOut);
}

// ---------------- PNG (RFC 2083) -----------------------------------

inline uint32_t be32(const uint8_t *p)
{
    return (static_cast<uint32_t>(p[0]) << 24) | (static_cast<uint32_t>(p[1]) << 16) |
           (static_cast<uint32_t>(p[2]) << 8) | p[3];
}

inline int paeth(int a, int b, int c)
{
    const int p  = a + b - c;
    const int pa = p > a ? p - a : a - p;
    const int pb = p > b ? p - b : b - p;
    const int pc = p > c ? p - c : c - p;
    if (pa <= pb && pa <= pc) return a;
    if (pb <= pc) return b;
    return c;
}

inline const uint8_t kPngSig[8] = {0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A};

// Unfilter `rows` scanlines of `rowBytes` bytes each (filter byte
// prefixed), `bpp` = filtering byte distance. In place on `data`.
inline bool unfilter(uint8_t *data, int rows, size_t rowBytes, int bpp)
{
    const uint8_t *prev = nullptr;
    for (int y = 0; y < rows; ++y) {
        uint8_t *row = data + static_cast<size_t>(y) * (rowBytes + 1);
        const uint8_t filter = row[0];
        uint8_t *cur = row + 1;

        switch (filter) {
        case 0:
            break;
        case 1:                                           // Sub
            for (size_t x = bpp; x < rowBytes; ++x)
                cur[x] = static_cast<uint8_t>(cur[x] + cur[x - bpp]);
            break;
        case 2:                                           // Up
            if (prev != nullptr)
                for (size_t x = 0; x < rowBytes; ++x)
                    cur[x] = static_cast<uint8_t>(cur[x] + prev[x]);
            break;
        case 3:                                           // Average
            for (size_t x = 0; x < rowBytes; ++x) {
                const int a = x >= static_cast<size_t>(bpp) ? cur[x - bpp] : 0;
                const int b = prev != nullptr ? prev[x] : 0;
                cur[x] = static_cast<uint8_t>(cur[x] + ((a + b) >> 1));
            }
            break;
        case 4:                                           // Paeth
            for (size_t x = 0; x < rowBytes; ++x) {
                const int a = x >= static_cast<size_t>(bpp) ? cur[x - bpp] : 0;
                const int b = prev != nullptr ? prev[x] : 0;
                const int c = (prev != nullptr && x >= static_cast<size_t>(bpp))
                                  ? prev[x - bpp] : 0;
                cur[x] = static_cast<uint8_t>(cur[x] + paeth(a, b, c));
            }
            break;
        default:
            return false;
        }

        prev = cur;
    }
    return true;
}

struct PngState {
    int      width = 0, height = 0;
    int      bitDepth = 0, colorType = 0, interlace = 0;
    int      channels = 0;

    uint8_t  palette[256 * 3] = {};
    int      paletteSize = 0;

    uint8_t  paletteAlpha[256];
    bool     hasPaletteAlpha = false;

    bool     hasColorKey = false;
    uint16_t keyR = 0, keyG = 0, keyB = 0;                // at full bit depth
};

// Read one sample (bit-packed for depths < 8, high byte for 16) from an
// unfiltered scanline; `idx` = sample index within the row.
inline int sampleAt(const uint8_t *row, int idx, int depth)
{
    switch (depth) {
    case 1:  return (row[idx >> 3] >> (7 - (idx & 7))) & 1;
    case 2:  return (row[idx >> 2] >> (6 - ((idx & 3) << 1))) & 3;
    case 4:  return (row[idx >> 1] >> (idx & 1 ? 0 : 4)) & 15;
    case 8:  return row[idx];
    default: return row[idx * 2];                          // 16: high byte
    }
}

// full-precision sample (for tRNS color-key comparison at depth 16)
inline int sampleFull(const uint8_t *row, int idx, int depth)
{
    if (depth == 16)
        return (row[idx * 2] << 8) | row[idx * 2 + 1];
    return sampleAt(row, idx, depth);
}

// Convert one unfiltered scanline of `pixels` pixels into BGRA at
// dst (stride 4, advancing `stepPx` output pixels each write).
inline void emitRow(const PngState &st, const uint8_t *row, int pixels,
                    uint8_t *dst, int stepPx)
{
    const int d = st.bitDepth;
    const int maxV = (d >= 8) ? 255 : ((1 << d) - 1);

    for (int x = 0; x < pixels; ++x, dst += 4 * stepPx) {
        int r = 0, g = 0, b = 0, a = 255;

        switch (st.colorType) {
        case 0: {                                          // gray
            int v = sampleAt(row, x, d);
            if (st.hasColorKey && sampleFull(row, x, d) == st.keyR)
                a = 0;
            if (d < 8)
                v = v * 255 / maxV;
            r = g = b = v;
            break;
        }
        case 2: {                                          // RGB
            r = sampleAt(row, x * 3 + 0, d);
            g = sampleAt(row, x * 3 + 1, d);
            b = sampleAt(row, x * 3 + 2, d);
            if (st.hasColorKey &&
                sampleFull(row, x * 3 + 0, d) == st.keyR &&
                sampleFull(row, x * 3 + 1, d) == st.keyG &&
                sampleFull(row, x * 3 + 2, d) == st.keyB)
                a = 0;
            break;
        }
        case 3: {                                          // palette
            const int idx = sampleAt(row, x, d);
            if (idx < st.paletteSize) {
                r = st.palette[idx * 3 + 0];
                g = st.palette[idx * 3 + 1];
                b = st.palette[idx * 3 + 2];
            }
            if (st.hasPaletteAlpha)
                a = st.paletteAlpha[idx & 0xFF];
            break;
        }
        case 4: {                                          // gray + alpha
            r = g = b = sampleAt(row, x * 2 + 0, d);
            a = sampleAt(row, x * 2 + 1, d);
            break;
        }
        default: {                                         // 6: RGBA
            r = sampleAt(row, x * 4 + 0, d);
            g = sampleAt(row, x * 4 + 1, d);
            b = sampleAt(row, x * 4 + 2, d);
            a = sampleAt(row, x * 4 + 3, d);
            break;
        }
        }

        dst[0] = static_cast<uint8_t>(b);                  // BGRA byte order ==
        dst[1] = static_cast<uint8_t>(g);                  // D3DFMT_A8R8G8B8
        dst[2] = static_cast<uint8_t>(r);                  // little-endian
        dst[3] = static_cast<uint8_t>(a);
    }
}

} // namespace modpng_detail

// ------------------------------------------------------------------
// Signature check / stream measuring
// ------------------------------------------------------------------

inline bool modPngLooksLikePng(const uint8_t *p)
{
    return p != nullptr && std::memcmp(p, modpng_detail::kPngSig, 8) == 0;
}

// Walk the chunk chain of an unsized PNG blob and return its total byte
// length (through IEND), or 0 if it doesn't stay inside `cap` bytes.
inline size_t modPngMeasure(const uint8_t *data, size_t cap = 64u << 20)
{
    if (!modPngLooksLikePng(data))
        return 0;

    size_t off = 8;
    while (off + 12 <= cap) {
        const uint32_t len = modpng_detail::be32(data + off);
        if (len > cap - off - 12)
            return 0;
        const bool iend = std::memcmp(data + off + 4, "IEND", 4) == 0;
        off += 12 + len;
        if (iend)
            return off;
    }
    return 0;
}

// ------------------------------------------------------------------
// Decode a whole PNG file into a tightly packed BGRA8 buffer.
// Returns false if the data is not a PNG this decoder supports.
// ------------------------------------------------------------------

inline bool modPngDecode(const uint8_t *data, size_t size,
                         std::vector<uint8_t> &outBgra, int &outW, int &outH)
{
    using namespace modpng_detail;

    if (size < 8 + 12 || !modPngLooksLikePng(data))
        return false;

    PngState st;
    std::vector<uint8_t> idat;
    bool sawIHDR = false, sawIEND = false;

    // ---- chunk walk --------------------------------------------------------
    size_t off = 8;
    while (off + 12 <= size && !sawIEND) {
        const uint32_t len = be32(data + off);
        if (len > size - off - 12)
            return false;
        const uint8_t *type = data + off + 4;
        const uint8_t *body = data + off + 8;

        if (std::memcmp(type, "IHDR", 4) == 0) {
            if (len != 13)
                return false;
            st.width     = static_cast<int>(be32(body + 0));
            st.height    = static_cast<int>(be32(body + 4));
            st.bitDepth  = body[8];
            st.colorType = body[9];
            st.interlace = body[12];
            if (body[10] != 0 || body[11] != 0)            // compression/filter method
                return false;
            sawIHDR = true;
        } else if (std::memcmp(type, "PLTE", 4) == 0) {
            if (len % 3 != 0 || len > 256 * 3)
                return false;
            std::memcpy(st.palette, body, len);
            st.paletteSize = static_cast<int>(len / 3);
        } else if (std::memcmp(type, "tRNS", 4) == 0) {
            if (st.colorType == 3) {
                std::memset(st.paletteAlpha, 0xFF, sizeof(st.paletteAlpha));
                std::memcpy(st.paletteAlpha, body, len > 256 ? 256 : len);
                st.hasPaletteAlpha = true;
            } else if (st.colorType == 0 && len >= 2) {
                st.keyR = static_cast<uint16_t>((body[0] << 8) | body[1]);
                st.hasColorKey = true;
            } else if (st.colorType == 2 && len >= 6) {
                st.keyR = static_cast<uint16_t>((body[0] << 8) | body[1]);
                st.keyG = static_cast<uint16_t>((body[2] << 8) | body[3]);
                st.keyB = static_cast<uint16_t>((body[4] << 8) | body[5]);
                st.hasColorKey = true;
            }
        } else if (std::memcmp(type, "IDAT", 4) == 0) {
            idat.insert(idat.end(), body, body + len);
        } else if (std::memcmp(type, "IEND", 4) == 0) {
            sawIEND = true;
        }
        // every other chunk (gAMA, sRGB, tEXt, ...) is ignored

        off += 12 + len;
    }

    if (!sawIHDR || idat.empty())
        return false;

    // ---- validate IHDR combos ----------------------------------------------
    constexpr int kMaxDim = 8192;
    if (st.width <= 0 || st.height <= 0 || st.width > kMaxDim || st.height > kMaxDim)
        return false;

    switch (st.colorType) {
    case 0: st.channels = 1; break;
    case 2: st.channels = 3; break;
    case 3: st.channels = 1; break;
    case 4: st.channels = 2; break;
    case 6: st.channels = 4; break;
    default: return false;
    }

    const int d = st.bitDepth;
    const bool depthOk =
        (st.colorType == 0 && (d == 1 || d == 2 || d == 4 || d == 8 || d == 16)) ||
        (st.colorType == 3 && (d == 1 || d == 2 || d == 4 || d == 8)) ||
        ((st.colorType == 2 || st.colorType == 4 || st.colorType == 6) &&
         (d == 8 || d == 16));
    if (!depthOk || (st.interlace != 0 && st.interlace != 1))
        return false;
    if (st.colorType == 3 && st.paletteSize == 0)
        return false;

    const auto rowBytesFor = [&](int pixels) -> size_t {
        return (static_cast<size_t>(pixels) * st.channels * d + 7) / 8;
    };
    const int filterBpp = (d >= 8) ? (d / 8) * st.channels : 1;

    // ---- inflate the filtered image ----------------------------------------
    // expected size: per pass, rows * (1 + rowBytes)
    size_t expect = 0;
    if (st.interlace == 0) {
        expect = static_cast<size_t>(st.height) * (rowBytesFor(st.width) + 1);
    } else {
        static const int x0[7] = {0, 4, 0, 2, 0, 1, 0};
        static const int y0[7] = {0, 0, 4, 0, 2, 0, 1};
        static const int dx[7] = {8, 8, 4, 4, 2, 2, 1};
        static const int dy[7] = {8, 8, 8, 4, 4, 2, 2};
        for (int p = 0; p < 7; ++p) {
            const int pw = (st.width - x0[p] + dx[p] - 1) / dx[p];
            const int ph = (st.height - y0[p] + dy[p] - 1) / dy[p];
            if (pw > 0 && ph > 0)
                expect += static_cast<size_t>(ph) * (rowBytesFor(pw) + 1);
        }
    }

    std::vector<uint8_t> raw;
    raw.reserve(expect);
    if (!zlibInflate(idat.data(), idat.size(), raw, expect) || raw.size() != expect)
        return false;

    // ---- unfilter + expand to BGRA ------------------------------------------
    outBgra.assign(static_cast<size_t>(st.width) * st.height * 4, 0);
    outW = st.width;
    outH = st.height;

    if (st.interlace == 0) {
        const size_t rb = rowBytesFor(st.width);
        if (!unfilter(raw.data(), st.height, rb, filterBpp))
            return false;
        for (int y = 0; y < st.height; ++y) {
            const uint8_t *row = raw.data() + static_cast<size_t>(y) * (rb + 1) + 1;
            emitRow(st, row, st.width,
                    outBgra.data() + static_cast<size_t>(y) * st.width * 4, 1);
        }
    } else {
        static const int x0[7] = {0, 4, 0, 2, 0, 1, 0};
        static const int y0[7] = {0, 0, 4, 0, 2, 0, 1};
        static const int dx[7] = {8, 8, 4, 4, 2, 2, 1};
        static const int dy[7] = {8, 8, 8, 4, 4, 2, 2};

        size_t passOff = 0;
        std::vector<uint8_t> line;                        // one emitted pass row
        for (int p = 0; p < 7; ++p) {
            const int pw = (st.width - x0[p] + dx[p] - 1) / dx[p];
            const int ph = (st.height - y0[p] + dy[p] - 1) / dy[p];
            if (pw <= 0 || ph <= 0)
                continue;

            const size_t rb = rowBytesFor(pw);
            uint8_t *pass = raw.data() + passOff;
            if (!unfilter(pass, ph, rb, filterBpp))
                return false;

            line.assign(static_cast<size_t>(pw) * 4, 0);
            for (int y = 0; y < ph; ++y) {
                const uint8_t *row = pass + static_cast<size_t>(y) * (rb + 1) + 1;
                emitRow(st, row, pw, line.data(), 1);

                uint8_t *dst = outBgra.data() +
                    (static_cast<size_t>(y0[p] + y * dy[p]) * st.width + x0[p]) * 4;
                for (int x = 0; x < pw; ++x)
                    std::memcpy(dst + static_cast<size_t>(x) * dx[p] * 4,
                                line.data() + static_cast<size_t>(x) * 4, 4);
            }

            passOff += static_cast<size_t>(ph) * (rb + 1);
        }
    }

    return true;
}

// ------------------------------------------------------------------
// Registration & lookup (used by enumerate_mods / ngl.cpp)
// ------------------------------------------------------------------

// Decode PNG bytes and register the pow2-resized DDS wrap under `hash`.
inline bool modPngRegisterMemory(const uint8_t *bytes, size_t size,
                                 uint32_t hash, const char *label)
{
    std::vector<uint8_t> bgra;
    int w = 0, h = 0;
    if (!modPngDecode(bytes, size, bgra, w, h)) {
        std::printf("mod: png FAILED to decode: %s\n", label ? label : "<memory>");
        return false;
    }

    ModPngTexture tex;
    tex.srcWidth  = w;
    tex.srcHeight = h;
    tex.width     = modgif_detail::nextPow2(w);
    tex.height    = modgif_detail::nextPow2(h);

    if (tex.width != w || tex.height != h) {
        std::vector<uint8_t> resized(static_cast<size_t>(tex.width) * tex.height * 4);
        modgif_detail::resizeBilinear(bgra.data(), w, h,
                                      resized.data(), tex.width, tex.height);
        bgra = std::move(resized);
    }

    tex.dds = modgif_detail::buildDDS(bgra.data(), tex.width, tex.height);

    std::printf("mod: png %s -> 0x%08X (%dx%d -> %dx%d)\n",
                label ? label : "<memory>", hash,
                tex.srcWidth, tex.srcHeight, tex.width, tex.height);

    ModPngTextures[hash] = std::move(tex);
    return true;
}

// Decode a .png from the mods folder and register it under `hash`
// (the same to_hash(relative-path-without-extension) key that .dds
// mods use). Returns true if the PNG was decoded successfully.
inline bool modPngRegisterFile(const std::filesystem::path &path, uint32_t hash)
{
    std::ifstream file(path, std::ios::binary);
    if (!file)
        return false;

    file.seekg(0, std::ios::end);
    std::streamsize sz = file.tellg();
    file.seekg(0, std::ios::beg);
    if (sz < 8 + 12)
        return false;

    std::vector<uint8_t> data(static_cast<size_t>(sz));
    file.read(reinterpret_cast<char *>(data.data()), sz);

    return modPngRegisterMemory(data.data(), data.size(), hash,
                                path.string().c_str());
}

// DDS blob for the texture loader; `size` optional (for D3DX paths).
inline uint8_t *modPngGetDDSByHash(uint32_t hash, unsigned int *size = nullptr)
{
    auto it = ModPngTextures.find(hash);
    if (it == ModPngTextures.end() || it->second.dds.empty())
        return nullptr;
    if (size != nullptr)
        *size = static_cast<unsigned int>(it->second.dds.size());
    return it->second.dds.data();
}

// Raw PNG bytes reaching the texture loader directly (a .png routed
// through the generic mod-data path, or a repacked resource carrying
// PNG payload): measure the unsized blob, decode it once, cache the
// DDS wrap under the texture's hash and hand it back. nullptr if the
// bytes aren't a decodable PNG.
inline uint8_t *modPngWrapRaw(uint32_t hash, const uint8_t *bytes)
{
    if (auto *cached = modPngGetDDSByHash(hash))
        return cached;

    const size_t size = modPngMeasure(bytes);
    if (size == 0)
        return nullptr;

    if (!modPngRegisterMemory(bytes, size, hash, nullptr))
        return nullptr;

    return modPngGetDDSByHash(hash);
}
