#pragma once
// ---------------------------------------------------------------------------
// mod_mesh_import.h - native, zero-dependency mesh importer for openusm mods.
//
// Replaces the Assimp import path. Supports:
//   * binary FBX 7.0 .. 7.7  (32-bit records < 7500, 64-bit records >= 7500,
//     zlib-deflated property arrays are inflated by the embedded decoder)
//   * ASCII FBX 7.x
//   * Wavefront OBJ
//
// The importer converts a source scene into per-section replacement data for
// an nglMesh, in the exact usperson/us_character 64-byte vertex layout:
//
//     float px py pz  nx ny nz  u v  i0 i1 i2 i3  w0 w1 w2 w3
//
// Conventions were established empirically against ULTIMATE_SPIDERMAN.PCMESH
// vs the Blender-addon FBX round trip of the same mesh:
//   - positions are identical (no axis conversion, no Z flip),
//   - game V = 1 - fbx V,
//   - game triangle winding is the REVERSE of FBX polygon winding,
//   - unused blend-index slots hold -1.0f (not 0), weights 0.
//
// Section mapping runs in three tiers:
//   Tier A  per-section objects:   model "name_<i>" -> section i of mesh
//           "name" ("ultimate_spiderman000_8" -> section 8 of
//           "ultimate_spiderman000"). Blender ".001" duplicate suffixes are
//           stripped. Unmatched sections keep their original geometry, so
//           partial replacements (just a head, etc.) still work.
//   Tier B  merged object:         model "name" with per-polygon material
//           slots -> polygons of slot s go to section s (the layout the
//           Blender addon produces before "separate by section").
//   Tier C  foreign scene:         no name match at all -> every mesh in the
//           file is baked through its node transform, split by material slot
//           and distributed over the sections in order; leftover sections are
//           hidden, extra buckets merge into the last section. Auto-fit
//           rescales/centres the import onto the original height whenever
//           it deviates by more than 25% (sidecar fit=0 disables).
//
// Skinning source, in priority order:
//   1. FBX skin clusters whose bone names parse to skeleton indices
//      ("Bone_12", "joint12", plain "12", ...) -> direct skeleton indices,
//      compact per-section palette is rebuilt.
//   2. Nearest-position weight transfer from the ORIGINAL section vertex
//      data (the from-disk floats are still bound when we run). For a
//      round-trip export the positions are identical, so this is an exact
//      recovery of the authored skin; for foreign meshes it behaves like an
//      automatic "data transfer" skin wrap. Tier A/B transfer keeps the
//      original section palette; Tier C transfers in skeleton space and
//      rebuilds a palette per section.
//   3. Rigid fallback (slot 0, weight 1).
//
// The header is engine-agnostic on purpose (no ngl.h): the engine hands the
// original sections in as plain OrigSectionView records. That keeps the file
// reusable from the standalone ModLoader project and host-testable on Linux.
//
// Optional sidecar "<file>.fbx.ini" next to the mod:
//     scale=1.0        uniform pre-scale (Tier C)
//     offset_x/y/z=0   pre-translate, applied after scale (Tier C)
//     yaw=0            rotation around +Y in degrees (Tier C)
//     fit=1            0 disables bounding-box auto-fit (Tier C)
//     skin=auto        auto | clusters | transfer | rigid
//     weld=1           0 disables vertex welding
// ---------------------------------------------------------------------------

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace modmesh {

// ===========================================================================
//  logging
// ===========================================================================
using LogFn = void (*)(const char *msg);
inline LogFn &logSink() { static LogFn fn = nullptr; return fn; }
inline void setLog(LogFn fn) { logSink() = fn; }

inline void logf(const char *fmt, ...)
{
    if (!logSink()) return;
    char buf[1024];
    va_list ap; va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    logSink()(buf);
}

// ===========================================================================
//  tiny zlib inflate (RFC 1950/1951). Enough for FBX property arrays.
// ===========================================================================
namespace inflate_impl {

struct BitReader {
    const uint8_t *p, *end;
    uint32_t bitbuf = 0;
    int      bitcnt = 0;
    bool     fail   = false;

    BitReader(const uint8_t *d, size_t n) : p(d), end(d + n) {}

    int bits(int need)
    {
        while (bitcnt < need) {
            if (p >= end) { fail = true; return 0; }
            bitbuf |= uint32_t(*p++) << bitcnt;
            bitcnt += 8;
        }
        int v = int(bitbuf & ((1u << need) - 1u));
        bitbuf >>= need;
        bitcnt  -= need;
        return v;
    }
    void align() { bitbuf = 0; bitcnt = 0; }
};

struct Huff {
    // canonical Huffman decode tables (puff-style)
    uint16_t count[16] = {};   // number of codes of each length
    uint16_t sym[288]  = {};   // symbols ordered by code

    bool build(const uint8_t *lens, int n)
    {
        for (int i = 0; i < 16; ++i) count[i] = 0;
        for (int i = 0; i < n; ++i)  count[lens[i]]++;
        if (count[0] == n) return true;        // no codes (allowed for dist)
        int left = 1;                          // over-subscription check
        for (int len = 1; len < 16; ++len) {
            left <<= 1;
            left -= count[len];
            if (left < 0) return false;
        }
        uint16_t offs[16]; offs[1] = 0;
        for (int len = 1; len < 15; ++len) offs[len + 1] = uint16_t(offs[len] + count[len]);
        for (int i = 0; i < n; ++i)
            if (lens[i]) sym[offs[lens[i]]++] = uint16_t(i);
        return true;
    }

    int decode(BitReader &br) const
    {
        int code = 0, first = 0, index = 0;
        for (int len = 1; len <= 15; ++len) {
            code |= br.bits(1);
            if (br.fail) return -1;
            int cnt = count[len];
            if (code - first < cnt) return sym[index + (code - first)];
            index += cnt;
            first = (first + cnt) << 1;
            code <<= 1;
        }
        return -1;
    }
};

inline bool inflate_raw(const uint8_t *src, size_t srcLen,
                        std::vector<uint8_t> &out, size_t expect)
{
    static const uint16_t LBASE[29] = { 3,4,5,6,7,8,9,10,11,13,15,17,19,23,27,31,
        35,43,51,59,67,83,99,115,131,163,195,227,258 };
    static const uint8_t  LEXT[29]  = { 0,0,0,0,0,0,0,0,1,1,1,1,2,2,2,2,
        3,3,3,3,4,4,4,4,5,5,5,5,0 };
    static const uint16_t DBASE[30] = { 1,2,3,4,5,7,9,13,17,25,33,49,65,97,129,193,
        257,385,513,769,1025,1537,2049,3073,4097,6145,8193,12289,16385,24577 };
    static const uint8_t  DEXT[30]  = { 0,0,0,0,1,1,2,2,3,3,4,4,5,5,6,6,
        7,7,8,8,9,9,10,10,11,11,12,12,13,13 };
    static const uint8_t  ORD[19]   = { 16,17,18,0,8,7,9,6,10,5,11,4,12,3,13,2,14,1,15 };

    BitReader br(src, srcLen);
    out.clear();
    if (expect) out.reserve(expect);

    for (;;) {
        int last  = br.bits(1);
        int btype = br.bits(2);
        if (br.fail) return false;

        if (btype == 0) {                                  // stored
            br.align();
            if (br.p + 4 > br.end) return false;
            uint16_t len  = uint16_t(br.p[0] | (br.p[1] << 8));
            uint16_t nlen = uint16_t(br.p[2] | (br.p[3] << 8));
            br.p += 4;
            if (uint16_t(~len) != nlen) return false;
            if (br.p + len > br.end) return false;
            out.insert(out.end(), br.p, br.p + len);
            br.p += len;
        } else if (btype == 1 || btype == 2) {
            Huff lit, dist;
            if (btype == 1) {                              // fixed
                uint8_t lens[288];
                for (int i = 0;   i < 144; ++i) lens[i] = 8;
                for (int i = 144; i < 256; ++i) lens[i] = 9;
                for (int i = 256; i < 280; ++i) lens[i] = 7;
                for (int i = 280; i < 288; ++i) lens[i] = 8;
                lit.build(lens, 288);
                uint8_t dl[30];
                for (int i = 0; i < 30; ++i) dl[i] = 5;
                dist.build(dl, 30);
            } else {                                       // dynamic
                int hlit  = br.bits(5) + 257;
                int hdist = br.bits(5) + 1;
                int hclen = br.bits(4) + 4;
                if (br.fail || hlit > 286 || hdist > 30) return false;
                uint8_t cl[19] = {};
                for (int i = 0; i < hclen; ++i) cl[ORD[i]] = uint8_t(br.bits(3));
                Huff clh;
                if (!clh.build(cl, 19)) return false;
                uint8_t lens[288 + 30] = {};
                int n = 0;
                while (n < hlit + hdist) {
                    int s = clh.decode(br);
                    if (s < 0) return false;
                    if (s < 16) lens[n++] = uint8_t(s);
                    else {
                        int rep, val = 0;
                        if (s == 16) { if (!n) return false; val = lens[n - 1]; rep = 3 + br.bits(2); }
                        else if (s == 17) rep = 3 + br.bits(3);
                        else              rep = 11 + br.bits(7);
                        if (br.fail || n + rep > hlit + hdist) return false;
                        while (rep--) lens[n++] = uint8_t(val);
                    }
                }
                if (!lens[256]) return false;              // need end-of-block
                if (!lit.build(lens, hlit)) return false;
                if (!dist.build(lens + hlit, hdist)) return false;
            }

            for (;;) {                                     // block payload
                int s = lit.decode(br);
                if (s < 0) return false;
                if (s < 256) out.push_back(uint8_t(s));
                else if (s == 256) break;
                else {
                    s -= 257;
                    if (s >= 29) return false;
                    int len = LBASE[s] + br.bits(LEXT[s]);
                    int ds  = dist.decode(br);
                    if (ds < 0 || ds >= 30) return false;
                    size_t d = size_t(DBASE[ds]) + size_t(br.bits(DEXT[ds]));
                    if (br.fail || d > out.size()) return false;
                    size_t from = out.size() - d;
                    for (int i = 0; i < len; ++i)          // may overlap
                        out.push_back(out[from + i]);
                }
                if (out.size() > (64u << 20)) return false; // 64 MB sanity cap
            }
        } else return false;

        if (last) break;
    }
    return true;
}

} // namespace inflate_impl

// zlib stream (2-byte header + deflate + adler32)
inline bool zlib_inflate(const uint8_t *src, size_t n,
                         std::vector<uint8_t> &out, size_t expect = 0)
{
    if (n < 2) return false;
    uint8_t cmf = src[0], flg = src[1];
    if ((cmf & 0x0F) != 8) return false;                   // deflate only
    if (((unsigned(cmf) << 8) | flg) % 31 != 0) return false;
    if (flg & 0x20) return false;                          // FDICT unsupported
    return inflate_impl::inflate_raw(src + 2, n - 2, out, expect);
}

// ===========================================================================
//  small vector / matrix math (doubles; converted to float at emit time)
// ===========================================================================
struct V3 { double x = 0, y = 0, z = 0; };

struct M4 {
    // row-major storage, column-vector convention: v' = M * v
    double m[16] = { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };

    static M4 identity() { return M4{}; }

    static M4 translate(const V3 &t)
    {
        M4 r; r.m[3] = t.x; r.m[7] = t.y; r.m[11] = t.z; return r;
    }
    static M4 scale(const V3 &s)
    {
        M4 r; r.m[0] = s.x; r.m[5] = s.y; r.m[10] = s.z; return r;
    }
    static M4 rotX(double a)
    {
        double c = std::cos(a), s = std::sin(a);
        M4 r; r.m[5] = c; r.m[6] = -s; r.m[9] = s; r.m[10] = c; return r;
    }
    static M4 rotY(double a)
    {
        double c = std::cos(a), s = std::sin(a);
        M4 r; r.m[0] = c; r.m[2] = s; r.m[8] = -s; r.m[10] = c; return r;
    }
    static M4 rotZ(double a)
    {
        double c = std::cos(a), s = std::sin(a);
        M4 r; r.m[0] = c; r.m[1] = -s; r.m[4] = s; r.m[5] = c; return r;
    }

    M4 operator*(const M4 &b) const
    {
        M4 r;
        for (int i = 0; i < 4; ++i)
            for (int j = 0; j < 4; ++j) {
                double s = 0;
                for (int k = 0; k < 4; ++k) s += m[i * 4 + k] * b.m[k * 4 + j];
                r.m[i * 4 + j] = s;
            }
        return r;
    }
    V3 point(const V3 &v) const
    {
        return { m[0]*v.x + m[1]*v.y + m[2]*v.z  + m[3],
                 m[4]*v.x + m[5]*v.y + m[6]*v.z  + m[7],
                 m[8]*v.x + m[9]*v.y + m[10]*v.z + m[11] };
    }
    V3 vec(const V3 &v) const
    {
        return { m[0]*v.x + m[1]*v.y + m[2]*v.z,
                 m[4]*v.x + m[5]*v.y + m[6]*v.z,
                 m[8]*v.x + m[9]*v.y + m[10]*v.z };
    }
    double det3() const
    {
        return m[0]*(m[5]*m[10]-m[6]*m[9])
             - m[1]*(m[4]*m[10]-m[6]*m[8])
             + m[2]*(m[4]*m[9] -m[5]*m[8]);
    }
    // inverse-transpose of the upper 3x3, for normals
    M4 normalMatrix() const
    {
        double a=m[0],b=m[1],c=m[2], d=m[4],e=m[5],f=m[6], g=m[8],h=m[9],i=m[10];
        double det = a*(e*i-f*h) - b*(d*i-f*g) + c*(d*h-e*g);
        if (std::fabs(det) < 1e-30) return M4::identity();
        double id = 1.0 / det;
        M4 r;
        // adjugate/det = inverse (row-major) ...
        r.m[0] = (e*i - f*h) * id;  r.m[1] = (c*h - b*i) * id;  r.m[2]  = (b*f - c*e) * id;
        r.m[4] = (f*g - d*i) * id;  r.m[5] = (a*i - c*g) * id;  r.m[6]  = (c*d - a*f) * id;
        r.m[8] = (d*h - e*g) * id;  r.m[9] = (b*g - a*h) * id;  r.m[10] = (a*e - b*d) * id;
        // ... then transpose in place -> inverse-transpose
        std::swap(r.m[1], r.m[4]); std::swap(r.m[2], r.m[8]); std::swap(r.m[6], r.m[9]);
        return r;
    }
};

inline M4 eulerDeg(const V3 &deg, int order /*0=XYZ..5=ZYX*/)
{
    const double k = 3.14159265358979323846 / 180.0;
    M4 X = M4::rotX(deg.x * k), Y = M4::rotY(deg.y * k), Z = M4::rotZ(deg.z * k);
    switch (order) {
    default:
    case 0: return Z * Y * X;   // XYZ: X applied first
    case 1: return Y * Z * X;   // XZY
    case 2: return X * Z * Y;   // YZX
    case 3: return Z * X * Y;   // YXZ
    case 4: return Y * X * Z;   // ZXY
    case 5: return X * Y * Z;   // ZYX
    }
}

// ===========================================================================
//  generic FBX node tree (shared by binary and ASCII readers)
// ===========================================================================
struct FbxProp {
    char                 type = 0;   // Y C I F D L S R f d l i b  ('N' = ascii number)
    int64_t              i    = 0;
    double               d    = 0;
    std::string          s;
    std::vector<double>  fa;         // float/double arrays
    std::vector<int64_t> ia;         // int arrays
};

struct FbxNode {
    std::string          name;
    std::vector<FbxProp> props;
    std::vector<FbxNode> children;

    const FbxNode *child(const char *n) const
    {
        for (auto &c : children) if (c.name == n) return &c;
        return nullptr;
    }
};

// ---------------------------------------------------------------------------
//  binary FBX reader
// ---------------------------------------------------------------------------
namespace fbxbin {

struct Cur {
    const uint8_t *p, *end;
    bool fail = false;

    size_t left() const { return size_t(end - p); }
    bool need(size_t n) { if (left() < n) { fail = true; return false; } return true; }
    uint8_t  u8()  { if (!need(1)) return 0; return *p++; }
    uint32_t u32() { if (!need(4)) return 0; uint32_t v; std::memcpy(&v, p, 4); p += 4; return v; }
    uint64_t u64() { if (!need(8)) return 0; uint64_t v; std::memcpy(&v, p, 8); p += 8; return v; }
    void bytes(void *dst, size_t n) { if (!need(n)) return; std::memcpy(dst, p, n); p += n; }
    void skip(size_t n) { if (!need(n)) return; p += n; }
};

inline bool readArray(Cur &c, char type, FbxProp &prop)
{
    uint32_t len = c.u32(), enc = c.u32(), clen = c.u32();
    if (c.fail || len > (32u << 20)) return false;
    size_t elem = (type == 'f' || type == 'i') ? 4 : (type == 'b') ? 1 : 8;

    const uint8_t *data = nullptr;
    std::vector<uint8_t> tmp;
    if (enc == 0) {
        if (!c.need(size_t(len) * elem)) return false;
        data = c.p;
        c.skip(size_t(len) * elem);
    } else if (enc == 1) {
        if (!c.need(clen)) return false;
        if (!zlib_inflate(c.p, clen, tmp, size_t(len) * elem)) return false;
        if (tmp.size() < size_t(len) * elem) return false;
        data = tmp.data();
        c.skip(clen);
    } else return false;

    prop.type = type;
    if (type == 'f') {
        prop.fa.resize(len);
        for (uint32_t k = 0; k < len; ++k) { float v; std::memcpy(&v, data + 4 * k, 4); prop.fa[k] = v; }
    } else if (type == 'd') {
        prop.fa.resize(len);
        for (uint32_t k = 0; k < len; ++k) { double v; std::memcpy(&v, data + 8 * k, 8); prop.fa[k] = v; }
    } else if (type == 'i') {
        prop.ia.resize(len);
        for (uint32_t k = 0; k < len; ++k) { int32_t v; std::memcpy(&v, data + 4 * k, 4); prop.ia[k] = v; }
    } else if (type == 'l') {
        prop.ia.resize(len);
        for (uint32_t k = 0; k < len; ++k) { int64_t v; std::memcpy(&v, data + 8 * k, 8); prop.ia[k] = v; }
    } else { // 'b'
        prop.ia.resize(len);
        for (uint32_t k = 0; k < len; ++k) prop.ia[k] = data[k] ? 1 : 0;
    }
    return true;
}

inline bool readNode(Cur &c, uint32_t version, const uint8_t *base, FbxNode &out, int depth)
{
    if (depth > 64) return false;
    uint64_t endOff, nProps, propLen;
    if (version >= 7500) { endOff = c.u64(); nProps = c.u64(); propLen = c.u64(); }
    else                 { endOff = c.u32(); nProps = c.u32(); propLen = c.u32(); }
    uint8_t nameLen = c.u8();
    if (c.fail) return false;
    if (endOff == 0) { out.name.clear(); return true; }    // NULL record

    if (!c.need(nameLen)) return false;
    out.name.assign(reinterpret_cast<const char *>(c.p), nameLen);
    c.skip(nameLen);

    const uint8_t *propEnd = c.p + propLen;
    if (propEnd > c.end) return false;
    for (uint64_t k = 0; k < nProps && !c.fail; ++k) {
        FbxProp pr;
        char t = char(c.u8());
        pr.type = t;
        switch (t) {
        case 'Y': { int16_t v = 0; c.bytes(&v, 2); pr.i = v; pr.d = v; } break;
        case 'C': { pr.i = c.u8() ? 1 : 0; pr.d = double(pr.i); } break;
        case 'I': { int32_t v = 0; c.bytes(&v, 4); pr.i = v; pr.d = v; } break;
        case 'F': { float v = 0;   c.bytes(&v, 4); pr.d = v; pr.i = int64_t(v); } break;
        case 'D': { double v = 0;  c.bytes(&v, 8); pr.d = v; pr.i = int64_t(v); } break;
        case 'L': { int64_t v = 0; c.bytes(&v, 8); pr.i = v; pr.d = double(v); } break;
        case 'S': case 'R': {
            uint32_t n = c.u32();
            if (!c.need(n)) return false;
            pr.s.assign(reinterpret_cast<const char *>(c.p), n);
            c.skip(n);
        } break;
        case 'f': case 'd': case 'l': case 'i': case 'b':
            if (!readArray(c, t, pr)) return false;
            break;
        default: return false;
        }
        out.props.push_back(std::move(pr));
    }
    if (c.fail || c.p > propEnd) return false;
    c.p = propEnd;

    const uint8_t *nodeEnd = base + endOff;
    if (nodeEnd > c.end || nodeEnd < c.p) return false;

    while (c.p < nodeEnd) {
        FbxNode child;
        if (!readNode(c, version, base, child, depth + 1)) return false;
        if (child.name.empty() && child.props.empty() && child.children.empty())
            break;                                          // NULL terminator
        out.children.push_back(std::move(child));
    }
    c.p = nodeEnd;
    return true;
}

inline bool parse(const uint8_t *data, size_t size, FbxNode &root, uint32_t &version)
{
    static const char MAGIC[] = "Kaydara FBX Binary  ";
    if (size < 27 || std::memcmp(data, MAGIC, 20) != 0) return false;
    Cur c{ data, data + size };
    c.skip(23);
    version = c.u32();
    if (c.fail || version < 7000 || version > 7999) return false;

    root.name = "<root>";
    for (;;) {
        if (c.left() < ((version >= 7500) ? 25u : 13u)) break;
        FbxNode top;
        if (!readNode(c, version, data, top, 0)) return false;
        if (top.name.empty() && top.props.empty() && top.children.empty()) break;
        root.children.push_back(std::move(top));
    }
    return !root.children.empty();
}

} // namespace fbxbin

// ---------------------------------------------------------------------------
//  ASCII FBX reader (7.x). Tolerant tokenizer -> same FbxNode tree.
// ---------------------------------------------------------------------------
namespace fbxtxt {

struct Lex {
    const char *p, *end;

    void ws()
    {
        for (;;) {
            while (p < end && (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n' || *p == ',')) ++p;
            if (p < end && *p == ';') { while (p < end && *p != '\n') ++p; continue; }
            break;
        }
    }
    bool eof() { ws(); return p >= end; }
};

inline bool ident(Lex &lx, std::string &out)
{
    lx.ws();
    const char *s = lx.p;
    while (lx.p < lx.end && (std::isalnum(uint8_t(*lx.p)) || *lx.p == '_')) ++lx.p;
    if (lx.p == s) return false;
    out.assign(s, lx.p);
    return true;
}

inline bool value(Lex &lx, FbxProp &pr)
{
    lx.ws();
    if (lx.p >= lx.end) return false;
    char c = *lx.p;
    if (c == '"') {
        ++lx.p;
        const char *s = lx.p;
        while (lx.p < lx.end && *lx.p != '"') ++lx.p;
        pr.type = 'S';
        pr.s.assign(s, lx.p);
        if (lx.p < lx.end) ++lx.p;
        return true;
    }
    if (c == '*') {                    // *N { a: 1,2,3 ... }
        ++lx.p;
        while (lx.p < lx.end && std::isdigit(uint8_t(*lx.p))) ++lx.p;
        lx.ws();
        if (lx.p < lx.end && *lx.p == '{') {
            ++lx.p;
            std::string aname;
            ident(lx, aname);          // "a"
            lx.ws();
            if (lx.p < lx.end && *lx.p == ':') ++lx.p;
            pr.type = 'd';
            for (;;) {
                lx.ws();
                if (lx.p >= lx.end || *lx.p == '}') break;
                char *pe = nullptr;
                double v = std::strtod(lx.p, &pe);
                if (pe == lx.p) { ++lx.p; continue; }
                lx.p = pe;
                pr.fa.push_back(v);
                pr.ia.push_back(int64_t(v));
            }
            if (lx.p < lx.end && *lx.p == '}') ++lx.p;
            return true;
        }
        return true;
    }
    if (c == '-' || c == '+' || c == '.' || std::isdigit(uint8_t(c))) {
        char *pe = nullptr;
        double v = std::strtod(lx.p, &pe);
        if (pe == lx.p) return false;
        lx.p = pe;
        pr.type = 'N';
        pr.d = v;
        pr.i = int64_t(v);
        return true;
    }
    if (std::isalpha(uint8_t(c))) {    // bare word (Y, T, W, ...)
        std::string w;
        ident(lx, w);
        pr.type = 'S';
        pr.s = std::move(w);
        return true;
    }
    return false;
}

inline bool node(Lex &lx, FbxNode &out, int depth)
{
    if (depth > 64) return false;
    if (!ident(lx, out.name)) return false;
    lx.ws();
    if (lx.p < lx.end && *lx.p == ':') ++lx.p;

    for (;;) {                          // values until '{' or next node
        lx.ws();
        if (lx.p >= lx.end) break;
        char c = *lx.p;
        if (c == '{' || c == '}') break;
        // lookahead: identifier followed by ':' means the next node starts
        if (std::isalpha(uint8_t(c)) || c == '_') {
            const char *save = lx.p;
            std::string w;
            ident(lx, w);
            lx.ws();
            bool nextNode = (lx.p < lx.end && *lx.p == ':');
            lx.p = save;
            if (nextNode) break;
        }
        FbxProp pr;
        if (!value(lx, pr)) break;
        out.props.push_back(std::move(pr));
    }

    lx.ws();
    if (lx.p < lx.end && *lx.p == '{') {
        ++lx.p;
        for (;;) {
            lx.ws();
            if (lx.p >= lx.end) return false;
            if (*lx.p == '}') { ++lx.p; break; }
            FbxNode ch;
            if (!node(lx, ch, depth + 1)) return false;
            out.children.push_back(std::move(ch));
        }
    }
    return true;
}

inline bool parse(const char *data, size_t size, FbxNode &root)
{
    Lex lx{ data, data + size };
    root.name = "<root>";
    bool any = false;
    while (!lx.eof()) {
        FbxNode top;
        if (!node(lx, top, 0)) break;
        root.children.push_back(std::move(top));
        any = true;
    }
    return any && root.child("Objects") != nullptr;
}

} // namespace fbxtxt

// ===========================================================================
//  scene model
// ===========================================================================
struct GLayer {                         // resolved LayerElement*
    std::string          mapping;       // ByPolygonVertex / ByVertice / ByPolygon / AllSame
    std::string          reference;     // Direct / IndexToDirect
    std::vector<double>  data;
    std::vector<int64_t> index;
    int                  comps = 3;
    bool                 valid = false;
};

struct GCluster {
    std::string          boneName;
    std::vector<int64_t> idx;
    std::vector<double>  w;
};

struct Geom {
    int64_t              id = 0;
    std::string          name;
    std::vector<double>  ctrl;          // xyz control points
    std::vector<int64_t> pvi;           // polygon vertex indices (neg-terminated)
    GLayer               nrm, uv;
    std::vector<int64_t> matIdx;        // LayerElementMaterial data
    std::string          matMapping;    // ByPolygon / AllSame
    std::vector<GCluster> clusters;     // resolved via skin connections
};

struct Model {
    int64_t id = 0;
    std::string name, type;
    int64_t parent = 0;
    V3 T{}, R{}, S{ 1, 1, 1 }, preR{}, postR{};
    V3 rOff{}, rPiv{}, sOff{}, sPiv{};
    V3 geoT{}, geoR{}, geoS{ 1, 1, 1 };
    int rotOrder = 0;
    std::vector<int64_t> geoms;
    std::vector<int64_t> materials;     // in slot order
};

struct SidecarCfg {
    double scale = 1.0, yaw = 0.0;
    V3     offset{};
    bool   fit = true, weld = true;
    int    skin = 0;                    // 0 auto, 1 clusters, 2 transfer, 3 rigid
};

struct Scene {
    std::map<int64_t, Geom>  geoms;
    std::map<int64_t, Model> models;
    std::map<int64_t, std::string> materials;   // id -> name
    std::vector<int64_t>     meshModelOrder;    // models of type Mesh, id order
    double unitScale = 1.0;     // FBX UnitScaleFactor (cm per file unit)
    double sceneScale = 1.0;    // applied at the root: unitScale / 100 (game = metres)
    bool   isObj = false;
    SidecarCfg cfg;
    std::string srcName;

    static std::string normName(const std::string &raw)
    {
        // binary: "name\x00\x01Model"; ascii: "Model::name"; strip ".001"
        std::string s = raw;
        if (auto p = s.find('\0'); p != std::string::npos) s.resize(p);
        if (auto p = s.rfind("::"); p != std::string::npos) s = s.substr(p + 2);
        auto dot = s.rfind('.');
        if (dot != std::string::npos && dot + 1 < s.size()) {
            bool digits = true;
            for (size_t k = dot + 1; k < s.size(); ++k)
                if (!std::isdigit(uint8_t(s[k]))) { digits = false; break; }
            if (digits) s.resize(dot);
        }
        for (auto &c : s) c = char(std::tolower(uint8_t(c)));
        return s;
    }

    const Model *findModelByNorm(const std::string &n) const
    {
        for (auto id : meshModelOrder) {
            auto it = models.find(id);
            if (it != models.end() && normName(it->second.name) == n)
                return &it->second;
        }
        return nullptr;
    }
};

// ===========================================================================
//  FBX tree -> Scene
// ===========================================================================
namespace detail {

inline void readP70(const FbxNode &n, Model &m)
{
    const FbxNode *p70 = n.child("Properties70");
    if (!p70) p70 = n.child("Properties60");
    if (!p70) return;
    for (auto &P : p70->children) {
        if (P.props.size() < 4) continue;
        const std::string &key = P.props[0].s;
        auto vec = [&](V3 &dst) {
            size_t n3 = P.props.size();
            if (n3 >= 7) { dst.x = P.props[n3-3].d; dst.y = P.props[n3-2].d; dst.z = P.props[n3-1].d; }
        };
        if      (key == "Lcl Translation")      vec(m.T);
        else if (key == "Lcl Rotation")         vec(m.R);
        else if (key == "Lcl Scaling")          vec(m.S);
        else if (key == "PreRotation")          vec(m.preR);
        else if (key == "PostRotation")         vec(m.postR);
        else if (key == "RotationOffset")       vec(m.rOff);
        else if (key == "RotationPivot")        vec(m.rPiv);
        else if (key == "ScalingOffset")        vec(m.sOff);
        else if (key == "ScalingPivot")         vec(m.sPiv);
        else if (key == "GeometricTranslation") vec(m.geoT);
        else if (key == "GeometricRotation")    vec(m.geoR);
        else if (key == "GeometricScaling")     vec(m.geoS);
        else if (key == "RotationOrder")        m.rotOrder = int(P.props.back().i);
    }
}

inline void readLayer(const FbxNode *le, const char *dataName, const char *idxName,
                      int comps, GLayer &out)
{
    if (!le) return;
    if (auto *mp = le->child("MappingInformationType"); mp && !mp->props.empty())
        out.mapping = mp->props[0].s;
    if (auto *rf = le->child("ReferenceInformationType"); rf && !rf->props.empty())
        out.reference = rf->props[0].s;
    if (auto *d = le->child(dataName); d && !d->props.empty() && !d->props[0].fa.empty())
        out.data = d->props[0].fa;
    if (auto *ix = le->child(idxName); ix && !ix->props.empty() && !ix->props[0].ia.empty())
        out.index = ix->props[0].ia;
    out.comps = comps;
    out.valid = !out.data.empty();
}

inline void buildScene(const FbxNode &root, Scene &sc)
{
    if (auto *gs = root.child("GlobalSettings")) {
        if (auto *p70 = gs->child("Properties70"))
            for (auto &P : p70->children)
                if (!P.props.empty() && P.props[0].s == "UnitScaleFactor")
                    sc.unitScale = P.props.back().d;
    }
    if (sc.unitScale > 1e-9)
        sc.sceneScale = sc.unitScale / 100.0;   // cm file -> metre game
    // (a Blender armature export pairs UnitScaleFactor=1 with node scale 100,
    //  so the two cancel; a Maya cm export lands a 180cm rig at 1.8 units)

    const FbxNode *objects = root.child("Objects");
    if (!objects) return;

    std::map<int64_t, int64_t> geomOfSkin;      // skin id -> geometry id
    std::map<int64_t, std::vector<int64_t>> clustersOfSkin;
    std::map<int64_t, GCluster> clusterById;
    std::map<int64_t, int64_t>  boneModelOfCluster;

    struct Conn { int64_t child, parent; };
    std::vector<Conn> conns;
    if (auto *cn = root.child("Connections"))
        for (auto &C : cn->children) {
            if (C.name != "C" && C.name != "Connect") continue;
            if (C.props.size() < 3) continue;
            conns.push_back(Conn{ C.props[1].i, C.props[2].i });
        }

    for (auto &O : objects->children) {
        if (O.props.empty()) continue;
        int64_t id = O.props[0].i;
        std::string oname  = O.props.size() > 1 ? O.props[1].s : std::string();
        std::string oclass = O.props.size() > 2 ? O.props[2].s : std::string();

        if (O.name == "Geometry" ||
            (O.name == "Model" && oclass == "Mesh" && O.child("Vertices"))) {
            Geom g;
            g.id = id;
            g.name = oname;
            if (auto *v = O.child("Vertices"); v && !v->props.empty()) g.ctrl = v->props[0].fa;
            if (auto *ix = O.child("PolygonVertexIndex"); ix && !ix->props.empty()) g.pvi = ix->props[0].ia;
            readLayer(O.child("LayerElementNormal"), "Normals", "NormalsIndex", 3, g.nrm);
            if (g.nrm.valid && g.nrm.index.empty())
                readLayer(O.child("LayerElementNormal"), "Normals", "NormalIndex", 3, g.nrm);
            readLayer(O.child("LayerElementUV"), "UV", "UVIndex", 2, g.uv);
            if (auto *lm = O.child("LayerElementMaterial")) {
                if (auto *mp = lm->child("MappingInformationType"); mp && !mp->props.empty())
                    g.matMapping = mp->props[0].s;
                if (auto *mi = lm->child("Materials"); mi && !mi->props.empty())
                    g.matIdx = mi->props[0].ia;
            }
            sc.geoms[id] = std::move(g);
        }
        if (O.name == "Model") {
            Model m;
            m.id = id;
            m.name = oname;
            m.type = oclass;
            readP70(O, m);
            if (sc.geoms.count(id))            // FBX6-style: model IS the geo
                m.geoms.push_back(id);
            sc.models[id] = std::move(m);
        }
        if (O.name == "Material")
            sc.materials[id] = oname;
        if (O.name == "Deformer") {
            if (oclass == "Skin") {
                geomOfSkin[id] = 0;
            } else if (oclass == "Cluster") {
                GCluster cl;
                if (auto *ix = O.child("Indexes"); ix && !ix->props.empty()) cl.idx = ix->props[0].ia;
                if (auto *w  = O.child("Weights"); w  && !w->props.empty())  cl.w   = w->props[0].fa;
                clusterById[id] = std::move(cl);
            }
        }
    }

    for (auto &c : conns) {
        auto pm = sc.models.find(c.parent);
        auto cm = sc.models.find(c.child);
        if (cm != sc.models.end() && clusterById.count(c.parent)) {
            boneModelOfCluster[c.parent] = c.child;
            continue;
        }
        if (cm != sc.models.end() && (pm != sc.models.end() || c.parent == 0)) {
            cm->second.parent = c.parent;
            continue;
        }
        if (sc.geoms.count(c.child) && pm != sc.models.end()) {
            pm->second.geoms.push_back(c.child);
            continue;
        }
        if (sc.materials.count(c.child) && pm != sc.models.end()) {
            pm->second.materials.push_back(c.child); // encounter order == slot order
            continue;
        }
        if (geomOfSkin.count(c.child) && sc.geoms.count(c.parent)) {
            geomOfSkin[c.child] = c.parent;
            continue;
        }
        if (clusterById.count(c.child) && geomOfSkin.count(c.parent)) {
            clustersOfSkin[c.parent].push_back(c.child);
            continue;
        }
    }

    for (auto &[skinId, geoId] : geomOfSkin) {
        auto git = sc.geoms.find(geoId);
        if (git == sc.geoms.end()) continue;
        for (int64_t clId : clustersOfSkin[skinId]) {
            auto cit = clusterById.find(clId);
            if (cit == clusterById.end()) continue;
            GCluster cl = cit->second;
            auto bit = boneModelOfCluster.find(clId);
            if (bit != boneModelOfCluster.end()) {
                auto mit = sc.models.find(bit->second);
                if (mit != sc.models.end())
                    cl.boneName = Scene::normName(mit->second.name);
            }
            if (!cl.idx.empty() && cl.idx.size() == cl.w.size())
                git->second.clusters.push_back(std::move(cl));
        }
    }

    for (auto &[id, m] : sc.models)
        if (m.type == "Mesh" && !m.geoms.empty())
            sc.meshModelOrder.push_back(id);
    std::sort(sc.meshModelOrder.begin(), sc.meshModelOrder.end());
}

inline M4 nodeLocal(const Model &m)
{
    M4 t   = M4::translate(m.T);
    M4 rof = M4::translate(m.rOff);
    M4 rp  = M4::translate(m.rPiv);
    M4 rpi = M4::translate({ -m.rPiv.x, -m.rPiv.y, -m.rPiv.z });
    M4 sof = M4::translate(m.sOff);
    M4 sp  = M4::translate(m.sPiv);
    M4 spi = M4::translate({ -m.sPiv.x, -m.sPiv.y, -m.sPiv.z });
    M4 pre = eulerDeg(m.preR, 0);
    M4 rot = eulerDeg(m.R, m.rotOrder);
    // R(post)^-1 == R(-post) reversed-order; exporters write tiny/zero values
    M4 post = eulerDeg({ -m.postR.x, -m.postR.y, -m.postR.z }, 5);
    M4 s   = M4::scale(m.S);
    return t * rof * rp * pre * rot * post * rpi * sof * sp * s * spi;
}

inline M4 nodeGlobal(const Scene &sc, const Model &m, int depth = 0)
{
    M4 local = nodeLocal(m);
    const M4 unit = M4::scale({ sc.sceneScale, sc.sceneScale, sc.sceneScale });
    if (depth > 64 || m.parent == 0) return unit * local;
    auto it = sc.models.find(m.parent);
    if (it == sc.models.end()) return unit * local;
    return nodeGlobal(sc, it->second, depth + 1) * local;
}

inline M4 geometricXf(const Model &m)
{
    return M4::translate(m.geoT) * eulerDeg(m.geoR, 0) * M4::scale(m.geoS);
}

} // namespace detail

// ===========================================================================
//  OBJ reader -> Scene (o/g become "models", usemtl become material slots)
// ===========================================================================
namespace objtxt {

inline bool parse(const char *data, size_t size, Scene &sc)
{
    std::vector<double> P, N, T;
    int64_t nextId = 1;
    Geom  *g = nullptr;
    Model *m = nullptr;
    std::map<std::string, int64_t> matIds;
    int64_t curMat = -1;

    auto beginObject = [&](const std::string &name) {
        int64_t gid = nextId++, mid = nextId++;
        Geom gg; gg.id = gid; gg.name = name;
        gg.nrm.mapping = gg.uv.mapping = "ByPolygonVertex";
        gg.nrm.reference = gg.uv.reference = "Direct";
        gg.nrm.comps = 3; gg.uv.comps = 2;
        gg.matMapping = "ByPolygon";
        sc.geoms[gid] = std::move(gg);
        Model mm; mm.id = mid; mm.name = name; mm.type = "Mesh";
        mm.geoms.push_back(gid);
        sc.models[mid] = std::move(mm);
        sc.meshModelOrder.push_back(mid);
        g = &sc.geoms[gid];
        m = &sc.models[mid];
    };

    const char *p = data, *end = data + size;
    std::string ln;
    auto line = [&]() {
        if (p >= end) return false;
        const char *s = p;
        while (p < end && *p != '\n') ++p;
        ln.assign(s, p);
        if (p < end) ++p;
        if (!ln.empty() && ln.back() == '\r') ln.pop_back();
        return true;
    };

    while (line()) {
        const char *s = ln.c_str();
        while (*s == ' ' || *s == '\t') ++s;
        if (*s == '#' || !*s) continue;

        if (s[0] == 'v' && s[1] == ' ') {
            double x = 0, y = 0, z = 0;
            std::sscanf(s + 2, "%lf %lf %lf", &x, &y, &z);
            P.push_back(x); P.push_back(y); P.push_back(z);
        } else if (s[0] == 'v' && s[1] == 'n') {
            double x = 0, y = 0, z = 0;
            std::sscanf(s + 3, "%lf %lf %lf", &x, &y, &z);
            N.push_back(x); N.push_back(y); N.push_back(z);
        } else if (s[0] == 'v' && s[1] == 't') {
            double u = 0, v = 0;
            std::sscanf(s + 3, "%lf %lf", &u, &v);
            T.push_back(u); T.push_back(v);
        } else if ((s[0] == 'o' || s[0] == 'g') && s[1] == ' ') {
            beginObject(std::string(s + 2));
        } else if (!std::strncmp(s, "usemtl", 6)) {
            std::string name = s + 6;
            while (!name.empty() && name.front() == ' ') name.erase(name.begin());
            auto it = matIds.find(name);
            if (it == matIds.end()) {
                int64_t id = 1000000 + int64_t(matIds.size());
                sc.materials[id] = name;
                it = matIds.emplace(name, id).first;
            }
            curMat = it->second;
            if (m) {
                auto &mats = m->materials;
                if (std::find(mats.begin(), mats.end(), curMat) == mats.end())
                    mats.push_back(curMat);
            }
        } else if (s[0] == 'f' && s[1] == ' ') {
            if (!g) beginObject("obj");
            std::vector<long> corners;
            const char *q = s + 2;
            while (*q) {
                while (*q == ' ') ++q;
                if (!*q) break;
                long vi = std::strtol(q, const_cast<char **>(&q), 10);
                long ti = 0, ni = 0;
                if (*q == '/') {
                    ++q;
                    if (*q != '/') ti = std::strtol(q, const_cast<char **>(&q), 10);
                    if (*q == '/') { ++q; ni = std::strtol(q, const_cast<char **>(&q), 10); }
                }
                auto fix = [](long i, size_t n) -> long {
                    if (i > 0) return i - 1;
                    if (i < 0) return long(n) + i;
                    return -1;
                };
                long pv = fix(vi, P.size() / 3);
                long tv = fix(ti, T.size() / 2);
                long nv = fix(ni, N.size() / 3);
                if (pv < 0 || size_t(pv) >= P.size() / 3) continue;

                if (g->ctrl.size() < P.size())
                    g->ctrl = P;                    // share the global pool
                corners.push_back(pv);
                if (nv >= 0 && size_t(nv) < N.size() / 3) {
                    g->nrm.data.push_back(N[3 * nv]);
                    g->nrm.data.push_back(N[3 * nv + 1]);
                    g->nrm.data.push_back(N[3 * nv + 2]);
                } else {
                    g->nrm.data.push_back(0); g->nrm.data.push_back(0); g->nrm.data.push_back(0);
                }
                if (tv >= 0 && size_t(tv) < T.size() / 2) {
                    g->uv.data.push_back(T[2 * tv]);
                    g->uv.data.push_back(T[2 * tv + 1]);
                } else {
                    g->uv.data.push_back(0); g->uv.data.push_back(0);
                }
            }
            if (corners.size() >= 3) {
                for (size_t k = 0; k + 1 < corners.size(); ++k)
                    g->pvi.push_back(corners[k]);
                g->pvi.push_back(~corners.back());
                int slot = 0;
                if (curMat >= 0 && m) {
                    auto &mats = m->materials;
                    slot = int(std::find(mats.begin(), mats.end(), curMat) - mats.begin());
                }
                g->matIdx.push_back(slot);
            }
        }
    }

    for (auto &[id, gg] : sc.geoms) {
        gg.nrm.valid = !gg.nrm.data.empty();
        gg.uv.valid  = !gg.uv.data.empty();
    }
    sc.isObj = true;
    return !sc.meshModelOrder.empty();
}

} // namespace objtxt

// ===========================================================================
//  public: original-section views + build results
// ===========================================================================
struct OrigSectionView {
    const float    *verts   = nullptr;   // 16 floats per vertex (64-byte layout)
    uint32_t        nverts  = 0;
    uint32_t        strideBytes = 0;     // from the file; only 64 is replaceable
    const uint16_t *palette = nullptr;   // BonesIdx
    int             nbones  = 0;
};

struct BuiltSection {
    std::vector<float>    vertices;      // 16 floats / vertex
    std::vector<uint32_t> indices;       // triangle list, game winding
    uint32_t              weightClass = 2; // -> nglMeshSection::field_5C
    bool                  keepOriginalPalette = true;
    std::vector<uint16_t> palette;       // used when !keepOriginalPalette
    std::string           source;
    bool                  hide = false;
};

// ===========================================================================
//  expansion: geometry (+node transform) -> corner stream
// ===========================================================================
namespace detail {

struct Corner {
    float px, py, pz, nx, ny, nz, u, v;
    float bi[4] = { 0, -1, -1, -1 };
    float bw[4] = { 1, 0, 0, 0 };
};

struct TriBucket {
    std::vector<Corner> corners;         // 3 per triangle
};

inline void layerValue(const GLayer &L, size_t ctrlIdx, size_t polyVertIdx,
                       size_t polyIdx, double *out, int comps)
{
    for (int k = 0; k < comps; ++k) out[k] = 0;
    if (!L.valid) return;
    size_t k;
    if      (L.mapping == "ByPolygonVertex")                      k = polyVertIdx;
    else if (L.mapping == "ByVertice" || L.mapping == "ByVertex") k = ctrlIdx;
    else if (L.mapping == "ByPolygon")                            k = polyIdx;
    else                                                          k = 0;   // AllSame

    if (L.reference == "IndexToDirect" || L.reference == "Index") {
        if (k >= L.index.size()) return;
        int64_t r = L.index[k];
        if (r < 0) return;
        k = size_t(r);
    }
    if ((k + 1) * size_t(L.comps) > L.data.size()) return;
    for (int c = 0; c < comps && c < L.comps; ++c) out[c] = L.data[k * L.comps + c];
}

// per-control-point skin from clusters, remapped through `boneRemap`
struct SkinTable {
    struct Inf { float idx; float w; };
    std::vector<std::array<Inf, 4>> per;

    void build(const Geom &g, const std::vector<int> &boneRemap)
    {
        size_t n = g.ctrl.size() / 3;
        per.assign(n, { { { -1, 0 }, { -1, 0 }, { -1, 0 }, { -1, 0 } } });
        for (size_t ci = 0; ci < g.clusters.size(); ++ci) {
            int mapped = ci < boneRemap.size() ? boneRemap[ci] : -1;
            if (mapped < 0) continue;
            const GCluster &cl = g.clusters[ci];
            for (size_t k = 0; k < cl.idx.size(); ++k) {
                int64_t v = cl.idx[k];
                double  w = cl.w[k];
                if (v < 0 || size_t(v) >= n || w <= 0) continue;
                auto &a = per[size_t(v)];
                int weakest = 0;
                for (int j = 1; j < 4; ++j) if (a[j].w < a[weakest].w) weakest = j;
                if (w > a[weakest].w) a[weakest] = { float(mapped), float(w) };
            }
        }
        for (auto &a : per) {
            std::sort(a.begin(), a.end(), [](const Inf &x, const Inf &y) { return x.w > y.w; });
            float s = a[0].w + a[1].w + a[2].w + a[3].w;
            if (s > 0) {
                float inv = 1.f / s;
                for (auto &e : a) { e.w *= inv; if (e.w <= 0) e.idx = -1; }
                if (a[0].idx < 0) a[0] = { 0, 1 };
            } else a[0] = { 0, 1 };
        }
    }
};

inline void expandGeom(const Geom &g, const M4 &world,
                       const SkinTable *skin,
                       std::map<int, TriBucket> &buckets)
{
    const size_t nCtrl = g.ctrl.size() / 3;
    if (!nCtrl || g.pvi.empty()) return;

    M4 nrmM = world.normalMatrix();
    const bool mirrored = world.det3() < 0;

    size_t polyIdx = 0, pvBase = 0;
    std::vector<int> poly;
    poly.reserve(8);

    auto emitPoly = [&](size_t pvStart) {
        if (poly.size() < 3) return;
        int slot = 0;
        if (g.matMapping == "ByPolygon" && polyIdx < g.matIdx.size())
            slot = int(g.matIdx[polyIdx]);
        else if (!g.matIdx.empty())
            slot = int(g.matIdx[0]);
        if (slot < 0) slot = 0;

        auto corner = [&](size_t iInPoly) -> Corner {
            size_t ctrlIdx = size_t(poly[iInPoly]);
            size_t pv      = pvStart + iInPoly;
            Corner c{};
            V3 p{ g.ctrl[3 * ctrlIdx], g.ctrl[3 * ctrlIdx + 1], g.ctrl[3 * ctrlIdx + 2] };
            p = world.point(p);
            c.px = float(p.x); c.py = float(p.y); c.pz = float(p.z);
            double nv[3];
            layerValue(g.nrm, ctrlIdx, pv, polyIdx, nv, 3);
            V3 n = nrmM.vec({ nv[0], nv[1], nv[2] });
            double len = std::sqrt(n.x * n.x + n.y * n.y + n.z * n.z);
            if (len > 1e-12) { n.x /= len; n.y /= len; n.z /= len; }
            c.nx = float(n.x); c.ny = float(n.y); c.nz = float(n.z);
            double uv[2] = { 0, 0 };
            layerValue(g.uv, ctrlIdx, pv, polyIdx, uv, 2);
            c.u = float(uv[0]);
            c.v = float(1.0 - uv[1]);                   // game V = 1 - fbx V
            if (skin && ctrlIdx < skin->per.size()) {
                auto &a = skin->per[ctrlIdx];
                for (int k = 0; k < 4; ++k) {
                    c.bi[k] = a[k].idx < 0 ? -1.f : a[k].idx;
                    c.bw[k] = a[k].w;
                }
                if (c.bi[0] < 0) { c.bi[0] = 0; c.bw[0] = 1; }
            }
            return c;
        };

        auto &B = buckets[slot];
        (void)mirrored;
        for (size_t k = 1; k + 1 < poly.size(); ++k) {      // fan triangulation
            // source order is kept; orientTriangles() enforces the game's
            // clockwise-vs-normal convention per triangle afterwards (needed
            // because strip-unrolled exports carry alternating winding)
            B.corners.push_back(corner(0));
            B.corners.push_back(corner(k));
            B.corners.push_back(corner(k + 1));
        }
    };

    for (size_t k = 0; k < g.pvi.size(); ++k) {
        int64_t ix = g.pvi[k];
        bool lastInPoly = ix < 0;
        int  ci = int(lastInPoly ? ~ix : ix);
        if (ci < 0 || size_t(ci) >= nCtrl) { poly.clear(); continue; }
        poly.push_back(ci);
        if (lastInPoly) {
            emitPoly(pvBase);
            pvBase += poly.size();
            ++polyIdx;
            poly.clear();
        }
    }
}

// ---------------------------------------------------------------------------
//  vertex welding
// ---------------------------------------------------------------------------
struct WeldKey {
    int32_t v[13];
    bool operator==(const WeldKey &o) const { return std::memcmp(v, o.v, sizeof(v)) == 0; }
};
struct WeldHash {
    size_t operator()(const WeldKey &k) const
    {
        uint32_t h = 2166136261u;
        const uint8_t *p = reinterpret_cast<const uint8_t *>(k.v);
        for (size_t i = 0; i < sizeof(k.v); ++i) { h ^= p[i]; h *= 16777619u; }
        return h;
    }
};

inline void buildBuffers(const TriBucket &B, bool weld,
                         std::vector<float> &verts, std::vector<uint32_t> &idx)
{
    verts.clear(); idx.clear();
    idx.reserve(B.corners.size());
    auto push = [&](const Corner &c) {
        const float row[16] = { c.px, c.py, c.pz, c.nx, c.ny, c.nz, c.u, c.v,
                                c.bi[0], c.bi[1], c.bi[2], c.bi[3],
                                c.bw[0], c.bw[1], c.bw[2], c.bw[3] };
        verts.insert(verts.end(), row, row + 16);
    };
    if (!weld) {
        verts.reserve(B.corners.size() * 16);
        for (auto &c : B.corners) {
            idx.push_back(uint32_t(verts.size() / 16));
            push(c);
        }
        return;
    }
    std::unordered_map<WeldKey, uint32_t, WeldHash> map;
    map.reserve(B.corners.size());
    auto q = [](float f, float s) { return int32_t(std::lround(double(f) * s)); };
    for (auto &c : B.corners) {
        WeldKey k;
        k.v[0] = q(c.px, 8192); k.v[1] = q(c.py, 8192); k.v[2] = q(c.pz, 8192);
        k.v[3] = q(c.nx, 1024); k.v[4] = q(c.ny, 1024); k.v[5] = q(c.nz, 1024);
        k.v[6] = q(c.u, 8192);  k.v[7] = q(c.v, 8192);
        k.v[8]  = int32_t(c.bi[0]); k.v[9]  = int32_t(c.bi[1]);
        k.v[10] = int32_t(c.bi[2]); k.v[11] = int32_t(c.bi[3]);
        k.v[12] = q(c.bw[0], 2048) ^ (q(c.bw[1], 2048) << 10) ^ (q(c.bw[2], 2048) << 20);
        auto it = map.find(k);
        uint32_t vi;
        if (it != map.end()) vi = it->second;
        else {
            vi = uint32_t(verts.size() / 16);
            push(c);
            map.emplace(k, vi);
        }
        idx.push_back(vi);
    }
}

// ---------------------------------------------------------------------------
//  nearest-position weight transfer
// ---------------------------------------------------------------------------
struct Donor { float p[3]; float n[3]; float bi[4]; float bw[4]; };

struct DonorGrid {
    std::vector<Donor> donors;
    std::unordered_map<uint64_t, std::vector<uint32_t>> cells;
    float cell = 1.f;
    float minB[3] = { 0, 0, 0 };

    static uint64_t key(int x, int y, int z)
    {
        return (uint64_t(uint32_t(x)) & 0x1FFFFF)
             | ((uint64_t(uint32_t(y)) & 0x1FFFFF) << 21)
             | ((uint64_t(uint32_t(z)) & 0x1FFFFF) << 42);
    }

    void build()
    {
        if (donors.empty()) return;
        float mn[3] = { 1e30f, 1e30f, 1e30f }, mx[3] = { -1e30f, -1e30f, -1e30f };
        for (auto &d : donors)
            for (int k = 0; k < 3; ++k) {
                mn[k] = std::min(mn[k], d.p[k]);
                mx[k] = std::max(mx[k], d.p[k]);
            }
        float dx = mx[0]-mn[0], dy = mx[1]-mn[1], dz = mx[2]-mn[2];
        float diag = std::sqrt(dx*dx + dy*dy + dz*dz);
        cell = std::max(diag / 48.f, 1e-4f);
        for (int k = 0; k < 3; ++k) minB[k] = mn[k];
        cells.reserve(donors.size());
        for (uint32_t i = 0; i < uint32_t(donors.size()); ++i) {
            auto &d = donors[i];
            int cx = int((d.p[0] - minB[0]) / cell);
            int cy = int((d.p[1] - minB[1]) / cell);
            int cz = int((d.p[2] - minB[2]) / cell);
            cells[key(cx, cy, cz)].push_back(i);
        }
    }

    const Donor *nearest(const float p[3], float *outD2 = nullptr) const
    {
        if (donors.empty()) return nullptr;
        int cx = int((p[0] - minB[0]) / cell);
        int cy = int((p[1] - minB[1]) / cell);
        int cz = int((p[2] - minB[2]) / cell);
        const Donor *best = nullptr;
        float bestD = 1e30f;
        for (int r = 0; r <= 4; ++r) {
            for (int dz = -r; dz <= r; ++dz)
                for (int dy = -r; dy <= r; ++dy)
                    for (int dx = -r; dx <= r; ++dx) {
                        if (std::max({ std::abs(dx), std::abs(dy), std::abs(dz) }) != r) continue;
                        auto it = cells.find(key(cx + dx, cy + dy, cz + dz));
                        if (it == cells.end()) continue;
                        for (uint32_t i : it->second) {
                            const Donor &d = donors[i];
                            float ax = d.p[0]-p[0], ay = d.p[1]-p[1], az = d.p[2]-p[2];
                            float dist = ax*ax + ay*ay + az*az;
                            if (dist < bestD) { bestD = dist; best = &d; }
                        }
                    }
            if (best && bestD <= (cell * float(r)) * (cell * float(r))) break;
        }
        if (!best) {
            for (auto &d : donors) {
                float ax = d.p[0]-p[0], ay = d.p[1]-p[1], az = d.p[2]-p[2];
                float dist = ax*ax + ay*ay + az*az;
                if (dist < bestD) { bestD = dist; best = &d; }
            }
        }
        if (outD2) *outD2 = bestD;
        return best;
    }
};

// Flat-shaded sources (the Blender addon export writes one face normal per
// polygon) block welding and look faceted on the cel shader. When a bucket
// is dominated by flat triangles and the donor transfer could NOT recover
// the authored normals (foreign geometry), rebuild smooth normals by
// accumulating outward-aligned, area-weighted face normals per welded
// position. Sources with real per-vertex normals are left untouched.
inline void smoothNormalsIfFlat(TriBucket &B, size_t exactNormalHits)
{
    const size_t n = B.corners.size();
    if (n < 3 || exactNormalHits * 2 >= n) return;

    size_t flat = 0, tris = 0;
    for (size_t t = 0; t + 2 < n; t += 3) {
        const Corner &a = B.corners[t], &b = B.corners[t+1], &c = B.corners[t+2];
        ++tris;
        if (a.nx == b.nx && a.ny == b.ny && a.nz == b.nz &&
            a.nx == c.nx && a.ny == c.ny && a.nz == c.nz)
            ++flat;
    }
    if (!tris || flat * 10 < tris * 9) return;      // < 90% flat: keep as-is

    struct K { int32_t x, y, z;
        bool operator==(const K &o) const { return x==o.x && y==o.y && z==o.z; } };
    struct KH { size_t operator()(const K &k) const {
        uint64_t h = (uint64_t(uint32_t(k.x)) * 73856093u)
                   ^ (uint64_t(uint32_t(k.y)) * 19349663u)
                   ^ (uint64_t(uint32_t(k.z)) * 83492791u);
        return size_t(h); } };
    auto key = [](const Corner &c) {
        // same quantization as the weld key, so smoothing groups == weld groups
        return K{ int32_t(std::lround(double(c.px) * 8192)),
                  int32_t(std::lround(double(c.py) * 8192)),
                  int32_t(std::lround(double(c.pz) * 8192)) };
    };

    std::unordered_map<K, std::array<double, 3>, KH> acc;
    acc.reserve(n / 4);
    for (size_t t = 0; t + 2 < n; t += 3) {
        const Corner &a = B.corners[t], &b = B.corners[t+1], &c = B.corners[t+2];
        double e1[3] = { double(b.px)-a.px, double(b.py)-a.py, double(b.pz)-a.pz };
        double e2[3] = { double(c.px)-a.px, double(c.py)-a.py, double(c.pz)-a.pz };
        double g[3]  = { e1[1]*e2[2]-e1[2]*e2[1],
                         e1[2]*e2[0]-e1[0]*e2[2],
                         e1[0]*e2[1]-e1[1]*e2[0] };          // area-weighted
        // align to the source face normal so alternating strip winding
        // cannot cancel the accumulation
        double d = g[0]*a.nx + g[1]*a.ny + g[2]*a.nz;
        if (d < 0) { g[0] = -g[0]; g[1] = -g[1]; g[2] = -g[2]; }
        for (const Corner *cc : { &a, &b, &c }) {
            auto &v = acc[key(*cc)];
            v[0] += g[0]; v[1] += g[1]; v[2] += g[2];
        }
    }
    size_t changed = 0;
    for (auto &c : B.corners) {
        auto it = acc.find(key(c));
        if (it == acc.end()) continue;
        double l = std::sqrt(it->second[0]*it->second[0]
                           + it->second[1]*it->second[1]
                           + it->second[2]*it->second[2]);
        if (l < 1e-20) continue;
        c.nx = float(it->second[0] / l);
        c.ny = float(it->second[1] / l);
        c.nz = float(it->second[2] / l);
        ++changed;
    }
    if (changed)
        logf("[modmesh]   flat-shaded source: rebuilt smooth normals for %u corners",
             unsigned(changed));
}

// Enforce the game convention cross(B-A, C-A) . normal < 0 (clockwise seen
// from the normal side) on every triangle. Empirically the retail PCMESH
// strips are 100% negative after D3D parity correction, while exports can
// carry mixed winding (raw strip unrolls alternate every triangle).
inline void orientTriangles(TriBucket &B)
{
    for (size_t t = 0; t + 2 < B.corners.size(); t += 3) {
        Corner &a = B.corners[t], &b = B.corners[t + 1], &c = B.corners[t + 2];
        const double e1[3] = { b.px - a.px, b.py - a.py, b.pz - a.pz };
        const double e2[3] = { c.px - a.px, c.py - a.py, c.pz - a.pz };
        const double g[3]  = { e1[1]*e2[2] - e1[2]*e2[1],
                               e1[2]*e2[0] - e1[0]*e2[2],
                               e1[0]*e2[1] - e1[1]*e2[0] };
        const double n[3]  = { double(a.nx) + b.nx + c.nx,
                               double(a.ny) + b.ny + c.ny,
                               double(a.nz) + b.nz + c.nz };
        const double d = g[0]*n[0] + g[1]*n[1] + g[2]*n[2];
        if (d > 1e-20) std::swap(b, c);
    }
}

inline uint32_t classifyWeights(const std::vector<float> &verts)
{
    // matches the vanilla loader: 2 = <=2 bones, 3 = 3 bones, 4 = 4 bones
    uint32_t cls = 2;
    for (size_t v = 0; v + 16 <= verts.size(); v += 16) {
        const float w2 = verts[v + 14], w3 = verts[v + 15];
        if (w3 != 0.f) return 4;
        if (w2 != 0.f && cls < 3) cls = 3;
    }
    return cls;
}

inline bool parseBoneIndexName(const std::string &n, int &out)
{
    if (n.empty()) return false;
    size_t e = n.size(), s = e;
    while (s > 0 && std::isdigit(uint8_t(n[s - 1]))) --s;
    if (s == e) return false;                          // no trailing digits
    std::string prefix = n.substr(0, s);
    while (!prefix.empty() && (prefix.back() == '_' || prefix.back() == ' ' || prefix.back() == '.'))
        prefix.pop_back();
    for (auto &c : prefix) c = char(std::tolower(uint8_t(c)));
    if (!(prefix.empty() || prefix == "bone" || prefix == "joint" || prefix == "b"))
        return false;
    out = std::atoi(n.c_str() + s);
    return out >= 0 && out < 1024;
}

} // namespace detail

// ===========================================================================
//  sidecar ini
// ===========================================================================
inline SidecarCfg loadSidecar(const std::string &meshPath)
{
    SidecarCfg cfg;
    std::ifstream f((meshPath + ".ini").c_str());
    if (!f) return cfg;
    std::string ln;
    while (std::getline(f, ln)) {
        auto eq = ln.find('=');
        if (eq == std::string::npos) continue;
        std::string k = ln.substr(0, eq), v = ln.substr(eq + 1);
        auto trim = [](std::string &s) {
            while (!s.empty() && std::isspace(uint8_t(s.front()))) s.erase(s.begin());
            while (!s.empty() && std::isspace(uint8_t(s.back())))  s.pop_back();
        };
        trim(k); trim(v);
        for (auto &c : k) c = char(std::tolower(uint8_t(c)));
        if      (k == "scale")    cfg.scale = std::atof(v.c_str());
        else if (k == "yaw")      cfg.yaw = std::atof(v.c_str());
        else if (k == "offset_x") cfg.offset.x = std::atof(v.c_str());
        else if (k == "offset_y") cfg.offset.y = std::atof(v.c_str());
        else if (k == "offset_z") cfg.offset.z = std::atof(v.c_str());
        else if (k == "fit")      cfg.fit = std::atoi(v.c_str()) != 0;
        else if (k == "weld")     cfg.weld = std::atoi(v.c_str()) != 0;
        else if (k == "skin") {
            for (auto &c : v) c = char(std::tolower(uint8_t(c)));
            if      (v == "clusters") cfg.skin = 1;
            else if (v == "transfer") cfg.skin = 2;
            else if (v == "rigid")    cfg.skin = 3;
            else                      cfg.skin = 0;
        }
    }
    logf("[modmesh] sidecar %s.ini: scale=%.3f yaw=%.1f fit=%d skin=%d weld=%d",
         meshPath.c_str(), cfg.scale, cfg.yaw, int(cfg.fit), cfg.skin, int(cfg.weld));
    return cfg;
}

// ===========================================================================
//  scene loading + cache
// ===========================================================================
inline std::shared_ptr<Scene> parseScene(const std::string &path,
                                         const void *data, size_t size)
{
    auto sc = std::make_shared<Scene>();
    sc->srcName = path;
    sc->cfg = loadSidecar(path);

    std::string ext;
    {
        auto dot = path.rfind('.');
        if (dot != std::string::npos) ext = path.substr(dot);
        for (auto &c : ext) c = char(std::tolower(uint8_t(c)));
    }

    const uint8_t *bytes = static_cast<const uint8_t *>(data);
    bool ok = false;

    if (ext == ".obj") {
        ok = objtxt::parse(reinterpret_cast<const char *>(bytes), size, *sc);
        if (ok)
            logf("[modmesh] \"%s\": OBJ, %u object(s)",
                 path.c_str(), unsigned(sc->meshModelOrder.size()));
    } else {
        FbxNode root;
        uint32_t version = 0;
        if (size > 27 && !std::memcmp(bytes, "Kaydara FBX Binary  ", 20)) {
            if (fbxbin::parse(bytes, size, root, version)) {
                detail::buildScene(root, *sc);
                ok = !sc->meshModelOrder.empty();
                logf("[modmesh] \"%s\": binary FBX %u, %u mesh model(s), %u geometrie(s)",
                     path.c_str(), version, unsigned(sc->meshModelOrder.size()),
                     unsigned(sc->geoms.size()));
            } else {
                logf("[modmesh] \"%s\": binary FBX parse FAILED", path.c_str());
            }
        } else {
            if (fbxtxt::parse(reinterpret_cast<const char *>(bytes), size, root)) {
                detail::buildScene(root, *sc);
                ok = !sc->meshModelOrder.empty();
                logf("[modmesh] \"%s\": ascii FBX, %u mesh model(s)",
                     path.c_str(), unsigned(sc->meshModelOrder.size()));
            } else {
                logf("[modmesh] \"%s\": not a supported mesh format "
                     "(binary/ascii FBX 7.x or OBJ)", path.c_str());
            }
        }
    }
    if (!ok) return nullptr;
    return sc;
}

// cached by (path, size, head/tail fingerprint) - survives mod re-enumeration
inline std::shared_ptr<Scene> loadScene(const std::string &path,
                                        const void *data, size_t size)
{
    struct Entry { size_t size; uint64_t fp; std::shared_ptr<Scene> scene; };
    static std::unordered_map<std::string, Entry> cache;

    auto fingerprint = [&]() -> uint64_t {
        const uint8_t *p = static_cast<const uint8_t *>(data);
        uint64_t h = 1469598103934665603ull;
        size_t n = std::min<size_t>(size, 64);
        for (size_t i = 0; i < n; ++i) { h ^= p[i]; h *= 1099511628211ull; }
        for (size_t i = size > 64 ? size - 64 : 0; i < size; ++i) { h ^= p[i]; h *= 1099511628211ull; }
        return h;
    };

    uint64_t fp = (data && size) ? fingerprint() : 0;
    auto it = cache.find(path);
    if (it != cache.end() && it->second.size == size && it->second.fp == fp)
        return it->second.scene;

    auto sc = parseScene(path, data, size);
    cache[path] = Entry{ size, fp, sc };
    return sc;
}

// ===========================================================================
//  buildSectionsForMesh - the three-tier mapper
// ===========================================================================
inline std::vector<std::optional<BuiltSection>>
buildSectionsForMesh(Scene &sc,
                     const std::string &meshNameIn,
                     const std::vector<OrigSectionView> &origs)
{
    using namespace detail;

    std::vector<std::optional<BuiltSection>> out(origs.size());
    if (origs.empty()) return {};

    std::string meshName = meshNameIn;
    for (auto &c : meshName) c = char(std::tolower(uint8_t(c)));

    // ---------------- shared helpers -----------------------------------
    auto donorForSection = [&](size_t si, bool skeletonSpace) {
        DonorGrid grid;
        auto push = [&](const OrigSectionView &ov) {
            if (!ov.verts || ov.strideBytes != 64) return;
            for (uint32_t v = 0; v < ov.nverts; ++v) {
                const float *row = ov.verts + size_t(v) * 16;
                Donor d;
                d.p[0] = row[0]; d.p[1] = row[1]; d.p[2] = row[2];
                d.n[0] = row[3]; d.n[1] = row[4]; d.n[2] = row[5];
                for (int k = 0; k < 4; ++k) {
                    float slot = row[8 + k];
                    float w    = row[12 + k];
                    if (skeletonSpace && slot >= 0 && ov.palette && int(slot) < ov.nbones)
                        d.bi[k] = float(ov.palette[int(slot)]);
                    else
                        d.bi[k] = slot;
                    d.bw[k] = w;
                }
                grid.donors.push_back(d);
            }
        };
        if (skeletonSpace) { for (auto &ov : origs) push(ov); }
        else if (si < origs.size()) push(origs[si]);
        grid.build();
        return grid;
    };

    // corner-level nearest-position transfer, BEFORE welding. Weights are
    // always taken from the donor; the normal only when the position is an
    // exact round-trip match (<= 1e-4 units), which restores the authored
    // smooth shading that a flat-normal export lost - and lets welding
    // collapse the corner soup back to the original vertex count.
    auto transferCorners = [&](TriBucket &B, const DonorGrid &grid) -> size_t {
        if (grid.donors.empty()) return 0;
        size_t nrmHits = 0;
        for (auto &c : B.corners) {
            const float p[3] = { c.px, c.py, c.pz };
            float d2 = 1e30f;
            const Donor *d = grid.nearest(p, &d2);
            if (!d) continue;
            for (int k = 0; k < 4; ++k) { c.bi[k] = d->bi[k]; c.bw[k] = d->bw[k]; }
            if (d2 <= 1e-8f) {
                c.nx = d->n[0]; c.ny = d->n[1]; c.nz = d->n[2];
                ++nrmHits;
            }
        }
        if (nrmHits)
            logf("[modmesh]   normals recovered from original for %u/%u corners",
                 unsigned(nrmHits), unsigned(B.corners.size()));
        return nrmHits;
    };

    // user transform (sidecar scale/yaw/offset) + auto-fit of arbitrary
    // corner sets onto the original mesh height. Shared by Tier C (foreign
    // scenes) and the cross-family tier (renamed piece exports).
    auto fitCorners = [&](std::vector<TriBucket *> &parts) {
        double mnx = 1e30, mny = 1e30, mnz = 1e30;
        double mxx = -1e30, mxy = -1e30, mxz = -1e30;
        M4 user = M4::translate(sc.cfg.offset)
                * M4::scale({ sc.cfg.scale, sc.cfg.scale, sc.cfg.scale })
                * eulerDeg({ 0, sc.cfg.yaw, 0 }, 0);
        M4 userN = user.normalMatrix();
        for (TriBucket *part : parts)
            for (auto &c : part->corners) {
                V3 p = user.point({ c.px, c.py, c.pz });
                V3 n = userN.vec({ c.nx, c.ny, c.nz });
                double l = std::sqrt(n.x*n.x + n.y*n.y + n.z*n.z);
                if (l > 1e-12) { n.x /= l; n.y /= l; n.z /= l; }
                c.px = float(p.x); c.py = float(p.y); c.pz = float(p.z);
                c.nx = float(n.x); c.ny = float(n.y); c.nz = float(n.z);
                mnx = std::min(mnx, p.x); mny = std::min(mny, p.y); mnz = std::min(mnz, p.z);
                mxx = std::max(mxx, p.x); mxy = std::max(mxy, p.y); mxz = std::max(mxz, p.z);
            }

        double omnx = 1e30, omny = 1e30, omnz = 1e30;
        double omxx = -1e30, omxy = -1e30, omxz = -1e30;
        bool haveOrig = false;
        for (auto &ov : origs) {
            if (!ov.verts || ov.strideBytes != 64) continue;
            for (uint32_t v = 0; v < ov.nverts; ++v) {
                const float *row = ov.verts + size_t(v) * 16;
                omnx = std::min<double>(omnx, row[0]); omxx = std::max<double>(omxx, row[0]);
                omny = std::min<double>(omny, row[1]); omxy = std::max<double>(omxy, row[1]);
                omnz = std::min<double>(omnz, row[2]); omxz = std::max<double>(omxz, row[2]);
                haveOrig = true;
            }
        }
        double newH  = mxy - mny;
        double origH = omxy - omny;
        if (sc.cfg.fit && haveOrig && newH > 1e-6 && origH > 1e-6) {
            double ratio = newH / origH;
            if (ratio > 1.25 || ratio < 0.8) {
                double s2 = origH / newH;
                double cx = (mnx + mxx) * 0.5, cz = (mnz + mxz) * 0.5;
                double ocx = (omnx + omxx) * 0.5, ocz = (omnz + omxz) * 0.5;
                logf("[modmesh] auto-fit: height %.2f -> %.2f (scale %.4f)",
                     newH, origH, s2);
                for (TriBucket *part : parts)
                    for (auto &c : part->corners) {
                        c.px = float((c.px - cx) * s2 + ocx);
                        c.py = float((c.py - mny) * s2 + omny);
                        c.pz = float((c.pz - cz) * s2 + ocz);
                    }
            }
        }
    };

    // rebuild a compact palette from skeleton-space indices in `verts`;
    // rewrites the vertex indices to palette slots. Returns the palette.
    auto compactPalette = [&](std::vector<float> &verts) {
        std::map<int, float> weightOf;                    // skel bone -> total w
        for (size_t v = 0; v + 16 <= verts.size(); v += 16)
            for (int k = 0; k < 4; ++k) {
                int b = int(verts[v + 8 + k]);
                float w = verts[v + 12 + k];
                if (b >= 0 && w > 0) weightOf[b] += w;
            }
        std::vector<std::pair<int, float>> ord(weightOf.begin(), weightOf.end());
        if (ord.size() > 64) {
            std::sort(ord.begin(), ord.end(),
                      [](const std::pair<int, float> &a, const std::pair<int, float> &b)
                      { return a.second > b.second; });
            logf("[modmesh] palette overflow: %u bones used, keeping the 64 heaviest",
                 unsigned(ord.size()));
            ord.resize(64);
        }
        std::sort(ord.begin(), ord.end());
        std::map<int, int> slotOf;
        std::vector<uint16_t> pal;
        pal.reserve(ord.size());
        for (auto &bw : ord) { slotOf[bw.first] = int(pal.size()); pal.push_back(uint16_t(bw.first)); }
        if (pal.empty()) pal.push_back(0);

        for (size_t v = 0; v + 16 <= verts.size(); v += 16) {
            float wsum = 0;
            for (int k = 0; k < 4; ++k) {
                int b = int(verts[v + 8 + k]);
                auto it = slotOf.find(b);
                if (verts[v + 12 + k] > 0 && it != slotOf.end()) {
                    verts[v + 8 + k] = float(it->second);
                    wsum += verts[v + 12 + k];
                } else {
                    verts[v + 8 + k]  = -1;
                    verts[v + 12 + k] = 0;
                }
            }
            if (wsum <= 0) { verts[v + 8] = 0; verts[v + 12] = 1; }
            else if (wsum < 0.999f)
                for (int k = 0; k < 4; ++k) verts[v + 12 + k] /= wsum;
            if (verts[v + 8] < 0) { verts[v + 8] = 0; verts[v + 12] = 1; }
        }
        return pal;
    };

    auto makeHidden = [&](const char *why) {
        BuiltSection b;
        b.hide = true;
        b.source = why;
        b.vertices.assign(16, 0.f);
        b.vertices[9] = b.vertices[10] = b.vertices[11] = -1.f;
        b.vertices[12] = 1.f;
        b.indices = { 0, 0, 0 };
        b.weightClass = 2;
        return b;
    };

    // skin decision for one geometry: 1 clusters / 2 transfer / 3 rigid
    auto skinModeFor = [&](const Geom &g, std::vector<int> &remapOut) -> int {
        if (sc.cfg.skin == 3) return 3;
        if (sc.cfg.skin != 2 && !g.clusters.empty()) {
            std::vector<int> remap(g.clusters.size(), -1);
            bool allNamed = true;
            for (size_t k = 0; k < g.clusters.size(); ++k) {
                int idx;
                if (parseBoneIndexName(g.clusters[k].boneName, idx)) remap[k] = idx;
                else { allNamed = false; break; }
            }
            if (allNamed || sc.cfg.skin == 1) {
                if (!allNamed) {                          // forced: cluster order
                    for (size_t k = 0; k < remap.size(); ++k) remap[k] = int(k);
                    logf("[modmesh] skin=clusters forced, using cluster order");
                }
                remapOut = std::move(remap);
                return 1;
            }
        }
        return 2;
    };

    auto oversize = [&](size_t si, const char *src, const BuiltSection &b) {
        if (b.vertices.size() / 16 > 1000000 || b.indices.size() > 3000000) {
            logf("[modmesh] section %u from \"%s\": too large (%u verts / %u idx), hidden",
                 unsigned(si), src,
                 unsigned(b.vertices.size() / 16), unsigned(b.indices.size()));
            return true;
        }
        return false;
    };

    // expand every geometry of a model into `buckets`; returns the skin mode
    auto expandModel = [&](const Model &m, std::map<int, TriBucket> &buckets) -> int {
        int mode = 2;
        M4 world = nodeGlobal(sc, m) * geometricXf(m);
        for (int64_t gid : m.geoms) {
            auto git = sc.geoms.find(gid);
            if (git == sc.geoms.end()) continue;
            const Geom &g = git->second;
            if (g.ctrl.empty() || g.pvi.empty()) continue;
            std::vector<int> remap;
            mode = skinModeFor(g, remap);
            SkinTable st;
            const SkinTable *stp = nullptr;
            if (mode == 1) { st.build(g, remap); stp = &st; }
            expandGeom(g, world, stp, buckets);
        }
        return mode;
    };

    // ================= Tier A: per-section objects ======================
    {
        bool any = false;
        std::vector<const Model *> match(origs.size(), nullptr);
        for (size_t si = 0; si < origs.size(); ++si) {
            char suffix[32];
            std::snprintf(suffix, sizeof(suffix), "_%u", unsigned(si));
            const Model *m = sc.findModelByNorm(meshName + suffix);
            if (m) { match[si] = m; any = true; }
        }
        if (any) {
            for (size_t si = 0; si < origs.size(); ++si) {
                const Model *m = match[si];
                if (!m) continue;                          // keep original
                if (origs[si].strideBytes != 64) {
                    logf("[modmesh] %s sec%u: stride %u unsupported, kept original",
                         meshName.c_str(), unsigned(si), origs[si].strideBytes);
                    continue;
                }
                std::string src = Scene::normName(m->name);
                std::map<int, TriBucket> buckets;
                int skinMode = expandModel(*m, buckets);

                TriBucket all;                             // one section = all slots
                for (auto &sb : buckets)
                    all.corners.insert(all.corners.end(),
                                       sb.second.corners.begin(), sb.second.corners.end());
                if (all.corners.empty()) {                 // empty piece -> hide
                    out[si] = makeHidden(src.c_str());
                    logf("[modmesh] %s sec%u <- \"%s\" (empty, hidden)",
                         meshName.c_str(), unsigned(si), src.c_str());
                    continue;
                }

                BuiltSection b;
                b.source = src;
                size_t nrmHits = 0;
                if (skinMode == 2) {
                    DonorGrid grid = donorForSection(si, /*skeletonSpace=*/false);
                    nrmHits = transferCorners(all, grid);  // palette-slot space
                }
                smoothNormalsIfFlat(all, nrmHits);
                orientTriangles(all);
                buildBuffers(all, sc.cfg.weld, b.vertices, b.indices);

                if (skinMode == 1) {
                    b.palette = compactPalette(b.vertices);
                    b.keepOriginalPalette = false;
                } else {
                    b.keepOriginalPalette = true;          // transfer / rigid
                }
                b.weightClass = classifyWeights(b.vertices);
                if (oversize(si, src.c_str(), b)) { out[si] = makeHidden(src.c_str()); continue; }

                logf("[modmesh] %s sec%u <- \"%s\" (%u verts, %u tris, skin=%s)",
                     meshName.c_str(), unsigned(si), src.c_str(),
                     unsigned(b.vertices.size() / 16),
                     unsigned(b.indices.size() / 3),
                     skinMode == 1 ? "clusters" : skinMode == 3 ? "rigid" : "transfer");
                out[si] = std::move(b);
            }
            return out;
        }
    }

    // ================= Tier B: merged object, split by material =========
    if (const Model *m = sc.findModelByNorm(meshName)) {
        std::map<int, TriBucket> buckets;
        int skinMode = expandModel(*m, buckets);
        std::string src = Scene::normName(m->name);

        if (!buckets.empty()) {
            for (size_t si = 0; si < origs.size(); ++si) {
                if (origs[si].strideBytes != 64) continue; // keep original
                TriBucket mine;
                auto it = buckets.find(int(si));
                if (it != buckets.end())
                    mine.corners = std::move(it->second.corners);
                if (si + 1 == origs.size())                // overflow -> last
                    for (auto &sb : buckets)
                        if (sb.first > int(si))
                            mine.corners.insert(mine.corners.end(),
                                                sb.second.corners.begin(),
                                                sb.second.corners.end());
                if (mine.corners.empty()) {
                    out[si] = makeHidden("merged/empty-slot");
                    continue;
                }

                BuiltSection b;
                b.source = src;
                size_t nrmHits = 0;
                if (skinMode == 2) {
                    DonorGrid grid = donorForSection(si, false);
                    nrmHits = transferCorners(mine, grid);
                }
                smoothNormalsIfFlat(mine, nrmHits);
                orientTriangles(mine);
                buildBuffers(mine, sc.cfg.weld, b.vertices, b.indices);
                if (skinMode == 1) {
                    b.palette = compactPalette(b.vertices);
                    b.keepOriginalPalette = false;
                }
                b.weightClass = classifyWeights(b.vertices);
                if (oversize(si, src.c_str(), b)) { out[si] = makeHidden(src.c_str()); continue; }

                logf("[modmesh] %s sec%u <- \"%s\" slot %u (%u verts, %u tris)",
                     meshName.c_str(), unsigned(si), src.c_str(), unsigned(si),
                     unsigned(b.vertices.size() / 16),
                     unsigned(b.indices.size() / 3));
                out[si] = std::move(b);
            }
            return out;
        }
    }

    // ================= Tier A': cross-family piece exports ===============
    // A renamed export ("VENOM.fbx" dropped in as ULTIMATE_SPIDERMAN.fbx)
    // still contains per-section pieces of its ORIGINAL mesh family
    // ("venom000_7"). Map the family whose LOD suffix is closest to the
    // requested one onto this mesh: piece i -> section i, pieces past the
    // last section merged into it, uncovered sections hidden. Skinning is
    // transferred in skeleton space (piece i of another character is NOT
    // the same body part as section i) with a fresh palette per section.
    {
        auto splitLod = [](const std::string &base, std::string &fam, int &lod) {
            size_t e = base.size(), st = e;
            while (st > 0 && std::isdigit(uint8_t(base[st - 1]))) --st;
            fam = base.substr(0, st);
            lod = (st == e) ? -1 : std::atoi(base.c_str() + st);
        };
        std::string reqFam; int reqLod;
        splitLod(meshName, reqFam, reqLod);

        std::map<std::string, std::map<int, const Model *>> families;
        for (int64_t mid : sc.meshModelOrder) {
            const Model &m = sc.models[mid];
            std::string nn = Scene::normName(m.name);
            size_t us = nn.rfind('_');
            if (us == std::string::npos || us + 1 >= nn.size()) continue;
            bool digits = true;
            for (size_t k = us + 1; k < nn.size(); ++k)
                if (!std::isdigit(uint8_t(nn[k]))) { digits = false; break; }
            if (!digits) continue;
            std::string base = nn.substr(0, us);
            if (base == meshName) continue;            // Tier A territory
            int piece = std::atoi(nn.c_str() + us + 1);
            if (piece < 0 || piece > 4096) continue;
            bool hasGeo = false;
            for (int64_t gid : m.geoms) {
                auto git = sc.geoms.find(gid);
                if (git != sc.geoms.end() && !git->second.ctrl.empty()
                    && !git->second.pvi.empty()) { hasGeo = true; break; }
            }
            if (!hasGeo) continue;
            families[base][piece] = &m;
        }

        const std::map<int, const Model *> *chosen = nullptr;
        std::string chosenBase;
        int bestScore = 1 << 30; size_t bestPieces = 0;
        for (auto &fb : families) {
            std::string fam; int lod;
            splitLod(fb.first, fam, lod);
            if (lod < 0 || fb.second.size() < 2)
                continue;      // not a per-section LOD family: Tier C decides
            int score = std::abs(lod - reqLod >= 0 ? lod - reqLod : reqLod - lod);
            if (reqLod < 0) score = 512;
            if (score < bestScore
                || (score == bestScore && fb.second.size() > bestPieces)) {
                bestScore = score;
                bestPieces = fb.second.size();
                chosen = &fb.second;
                chosenBase = fb.first;
            }
        }

        int lastReplaceable = -1;
        for (size_t si = 0; si < origs.size(); ++si)
            if (origs[si].strideBytes == 64) lastReplaceable = int(si);

        if (chosen != nullptr && !chosen->empty() && lastReplaceable >= 0) {
            logf("[modmesh] %s: cross-family mapping from \"%s\" (%u pieces)",
                 meshName.c_str(), chosenBase.c_str(), unsigned(chosen->size()));

            std::map<int, TriBucket> perSection;
            for (auto &pm : *chosen) {
                std::map<int, TriBucket> slots;
                expandModel(*pm.second, slots);
                int target = pm.first;
                if (target > lastReplaceable
                    || (size_t(target) < origs.size()
                        && origs[size_t(target)].strideBytes != 64))
                    target = lastReplaceable;
                TriBucket &dst = perSection[target];
                for (auto &sb : slots)
                    dst.corners.insert(dst.corners.end(),
                                       sb.second.corners.begin(),
                                       sb.second.corners.end());
            }
            std::vector<TriBucket *> parts;
            parts.reserve(perSection.size());
            for (auto &sb : perSection) parts.push_back(&sb.second);
            fitCorners(parts);

            for (size_t si = 0; si < origs.size(); ++si) {
                if (origs[si].strideBytes != 64) continue;
                auto it = perSection.find(int(si));
                if (it == perSection.end() || it->second.corners.empty()) {
                    out[si] = makeHidden("cross-family/uncovered");
                    continue;
                }
                TriBucket &mine = it->second;

                BuiltSection b;
                b.source = chosenBase;
                DonorGrid grid = donorForSection(si, /*skeletonSpace=*/true);
                size_t nrmHits = 0;
                if (!grid.donors.empty())
                    nrmHits = transferCorners(mine, grid);
                smoothNormalsIfFlat(mine, nrmHits);
                orientTriangles(mine);
                buildBuffers(mine, sc.cfg.weld, b.vertices, b.indices);
                if (!grid.donors.empty()) {
                    b.palette = compactPalette(b.vertices);
                    b.keepOriginalPalette = false;
                } else {
                    b.keepOriginalPalette = true;
                }
                b.weightClass = classifyWeights(b.vertices);
                if (oversize(si, chosenBase.c_str(), b)) {
                    out[si] = makeHidden("cross-family/oversize");
                    continue;
                }
                logf("[modmesh] %s sec%u <- \"%s\" piece(s) (%u verts, %u tris)",
                     meshName.c_str(), unsigned(si), chosenBase.c_str(),
                     unsigned(b.vertices.size() / 16),
                     unsigned(b.indices.size() / 3));
                out[si] = std::move(b);
            }
            return out;
        }
    }

    // ================= Tier C: foreign scene ============================
    {
        // A file that names pieces of some OTHER mesh family of this same
        // .PCMESH ("usm_blacksuit000" while we load usm_blacksuit001) is
        // still applied here, so all LODs pick up the replacement.
        std::map<int64_t, int> globalSlot;                 // material id -> bucket
        auto slotOfMaterial = [&](int64_t matId) {
            auto it = globalSlot.find(matId);
            if (it != globalSlot.end()) return it->second;
            int s = int(globalSlot.size());
            globalSlot[matId] = s;
            return s;
        };

        std::map<int, TriBucket> buckets;
        int skinMode = 2;
        bool anyClusterMode = false;

        for (int64_t mid : sc.meshModelOrder) {
            const Model &m = sc.models[mid];
            std::map<int, TriBucket> local;
            int mode = expandModel(m, local);
            if (mode == 1) anyClusterMode = true;
            for (auto &sb : local) {
                int gslot = 0;
                int slot = sb.first;
                if (slot >= 0 && size_t(slot) < m.materials.size())
                    gslot = slotOfMaterial(m.materials[slot]);
                else if (!m.materials.empty())
                    gslot = slotOfMaterial(m.materials[0]);
                else
                    gslot = 0;
                auto &dst = buckets[gslot];
                dst.corners.insert(dst.corners.end(),
                                   sb.second.corners.begin(), sb.second.corners.end());
            }
        }
        if (buckets.empty()) return {};
        skinMode = anyClusterMode ? 1 : (sc.cfg.skin == 3 ? 3 : 2);

        // ---- user transform + auto-fit (positions only, uniform scale) ----
        {
            std::vector<TriBucket *> parts;
            parts.reserve(buckets.size());
            for (auto &sb : buckets) parts.push_back(&sb.second);
            fitCorners(parts);
        }

        // ---- distribute buckets over sections ------------------------------
        int lastReplaceable = -1;
        for (size_t si = 0; si < origs.size(); ++si)
            if (origs[si].strideBytes == 64) lastReplaceable = int(si);
        if (lastReplaceable < 0) return {};

        int nBuckets = buckets.empty() ? 0 : (buckets.rbegin()->first + 1);

        for (size_t si = 0; si < origs.size(); ++si) {
            if (origs[si].strideBytes != 64) continue;     // keep original
            TriBucket mine;
            auto it = buckets.find(int(si));
            if (it != buckets.end())
                mine.corners = std::move(it->second.corners);
            if (int(si) == lastReplaceable)                // overflow -> last
                for (auto &sb : buckets)
                    if (sb.first > lastReplaceable)
                        mine.corners.insert(mine.corners.end(),
                                            sb.second.corners.begin(),
                                            sb.second.corners.end());
            if (mine.corners.empty()) {
                out[si] = makeHidden("foreign/no-bucket");
                continue;
            }

            BuiltSection b;
            b.source = sc.srcName;
            bool skelIndices = false;
            size_t nrmHits = 0;
            if (skinMode == 2) {
                // transfer in SKELETON space from all original sections,
                // then compact to a fresh palette for this section
                DonorGrid grid = donorForSection(si, /*skeletonSpace=*/true);
                if (!grid.donors.empty()) {
                    nrmHits = transferCorners(mine, grid);
                    skelIndices = true;
                }
            }
            smoothNormalsIfFlat(mine, nrmHits);
            orientTriangles(mine);
            buildBuffers(mine, sc.cfg.weld, b.vertices, b.indices);

            if (skinMode == 1 || skelIndices) {
                b.palette = compactPalette(b.vertices);
                b.keepOriginalPalette = false;
            } else {
                b.keepOriginalPalette = true;
            }
            b.weightClass = classifyWeights(b.vertices);
            if (oversize(si, sc.srcName.c_str(), b)) { out[si] = makeHidden("foreign/oversize"); continue; }

            logf("[modmesh] %s sec%u <- foreign bucket %u/%u (%u verts, %u tris, skin=%s)",
                 meshName.c_str(), unsigned(si), unsigned(si), unsigned(nBuckets),
                 unsigned(b.vertices.size() / 16),
                 unsigned(b.indices.size() / 3),
                 skinMode == 1 ? "clusters" : skinMode == 3 ? "rigid" : "transfer");
            out[si] = std::move(b);
        }
        return out;
    }
}

} // namespace modmesh
