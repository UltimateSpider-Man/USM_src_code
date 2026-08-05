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
// an nglMesh. Sections come in two families and both are supported:
//
//   SKINNED (characters) - the retail usperson/us_character 64-byte vertex:
//
//     float px py pz  nx ny nz  u v  i0 i1 i2 i3  w0 w1 w2 w3
//
//   STATIC RIGID (everything else: weapons, pickups, hero-gauge/HUD meshes,
//   props, world geometry) - an arbitrary interleaved D3D vertex, stride
//   16..64, no blend data, usually with a D3DCOLOR channel. The engine hands
//   the layout in as byte offsets on OrigSectionView (posOff/nrmOff/uvOff/
//   colOff, skinned=false); BuiltSection::vertices stays the canonical
//   16-float row and the engine applier packs it back into that layout
//   (BuiltSection::rigid, targetStride, tPosOff..tColOff). Static USM
//   geometry carries its BAKED LIGHTING in the colour channel, so every
//   imported vertex inherits the colour of the nearest original one
//   (BuiltSection::colors) instead of rendering flat white. Skinning,
//   palettes and the morph dead zone do not apply to these sections; the
//   fit uses the bounding-box diagonal and centre rather than height and
//   feet, since a prop is not standing on the ground.
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
//           partial replacements (just a head, etc.) still work. The whole
//           export is normalized as one rigid body onto the bounds of the
//           ORIGINAL sections it replaces: auto-fit past +/-25% height
//           deviation, and always translated onto them (feet to feet,
//           centred X/Z) so hip-rooted armature exports line up with the
//           retail feet-at-0 frame before weights are transferred. Exact
//           round trips shift/scale by ~0 and stay bit-exact.
//   Tier B  merged object:         model "name" with per-polygon material
//           slots -> polygons of slot s go to section s (the layout the
//           Blender addon produces before "separate by section"). Normalized
//           like Tier A, against all sections.
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
//      recovery of the authored skin; for foreign meshes an inverse-distance
//      blend of the 4 nearest donors smooths the transfer across bone
//      boundaries. Tier A/B transfer keeps the original section palette;
//      Tier A'/C transfer in skeleton space and rebuild a palette per
//      section, after ALWAYS aligning the import onto the original bounds
//      (feet to feet, centred X/Z) so offset bodies sample the right bones.
//   3. Rigid fallback (slot 0, weight 1).
//
// Texture identity travels with the import: per-piece FBX material
// references map to the sections those pieces land on, FBX-embedded images
// (Video nodes with Content) are carried as byte payloads, and the paths
// written in the file are kept so the engine side can read them next to the
// mod (BuiltSection::embeddedTex / texRelPath).
//
// ANIMATED FBX: every FBX 7.x animation take in the file (AnimationStack ->
// AnimationLayer -> AnimationCurveNode -> AnimationCurve, both binary and
// ASCII) is baked into Scene::anims as per-bone local-TRS keyframe clips:
//   - key times in SECONDS (KTime / 46186158000), clip-relative (start at 0),
//   - translations in game units (sceneScale applied, same as geometry),
//   - rotations as quaternions with the node's RotationOrder and Pre/Post
//     rotation baked in (composed exactly like nodeLocal), hemisphere-
//     corrected for interpolation,
//   - the three per-axis curves of one property merged on the union of
//     their key times (linear resampling; tangent data is ignored).
// buildSectionsForMesh() then resolves every channel onto the TARGET mesh's
// skeleton (AnimChannel::skelIndex) with the same bind-pose cluster table the
// skinning path trusts, falling back to Bone_N name digits. Consumers sample
// with sampleChannel()/channelLocalMatrix() - see the "animation sampling"
// section. KeyAttr* tangent arrays are always parsed-and-discarded, and
// sidecar anim=0 skips the KeyTime/KeyValueFloat payloads entirely, so a
// mesh-only import of a big animation dump costs no extra memory.
//
// The header is engine-agnostic on purpose (no ngl.h): the engine hands the
// original sections in as plain OrigSectionView records. That keeps the file
// reusable from the standalone ModLoader project and host-testable on Linux.
//
// Optional sidecar "<file>.fbx.ini" next to the mod:
//     scale=1.0        uniform pre-scale
//     offset_x/y/z=0   pre-translate, applied after scale
//     yaw=0            rotation around +Y in degrees
//     fit=1            0 disables auto-fit AND the bounds alignment (all tiers)
//     skin=auto        auto | clusters | transfer | rigid | rigidparts
//                      rigid      - the whole import on ONE bone: always
//                                   visible, coherent, does NOT deform
//                      rigidparts - ONE bone per vertex (no blending) with a
//                                   majority filter over the triangle
//                                   neighbourhood: still animates, joints go
//                                   faceted, isolated bad influences that
//                                   would spike are absorbed
//     texture=auto     auto | keep | mod. keep never retargets the section
//                      material; auto refuses palette/luminance DDS files
//                      (pack extractions decode to grayscale) whenever a
//                      resident texture with the same stem exists; mod lets
//                      mod-shipped bytes win unconditionally (old behaviour)
//     tex<N>=STEM      PIN: bind STEM to section N's material, whatever the
//                      policy says. Bypasses texture=keep, the round-trip
//                      restriction and every heuristic, works on sections that
//                      were never replaced, and CLONES the material first so
//                      only section N changes. This is how an untextured
//                      (pure white) piece gets its sheet back:
//                          tex7=VENOM_MOUTH
//                          tex11=VENOM_EDDIE
//     white=1,7        draw those sections PURE WHITE: the engine binds its
//                      own 1x1 white texture to a private clone of the
//                      section material and runs no other texture logic on
//                      them. For geometry that is white ON PURPOSE - the
//                      black suit's eye lenses and chest spider - where every
//                      blank-repair heuristic below would otherwise hide the
//                      piece or paint the body sheet over it. Wins over
//                      hide=, keep=, blank= and tex<N>=. tex<N>=WHITE is the
//                      same instruction spelled as a pin.
//     tex_default=STEM last-resort stem for every section that reaches the
//                      engine with no texture reference at all
//     autotex=1        0/off disables the automatic blank-section coverage:
//                      guessed/empty candidate lists are extended with the
//                      sheets the file itself brings, and every untouched
//                      section gets a blank-fix carrier so a material with
//                      NO diffuse texture (draws pure white) is repainted
//                      with one of those sheets. Never displaces a texture
//                      a section already draws with.
//     blank=keep       keep | import | hide. A NAMED per-section piece with
//                      no texture reference that fails the round-trip test
//                      is morph-held reveal geometry (the venom_eddie
//                      cocoon/teeth/eddie-head): importing it dead-zones the
//                      morphs and freezes it OPEN as a permanent white shell
//                      over the shoulders. keep (default) leaves the vanilla
//                      section in place so retail morphs keep folding it
//                      away; import forces the old replace behaviour; hide
//                      swaps in a degenerate triangle. Explicit keep=/hide=/
//                      tex<N>= lines for the section win over this policy.
//     weld=1           0 disables vertex welding
//     anim=1           0 skips animation curves entirely (parse + bake)
//     layout=...       STATIC sections only: override the vertex format the
//                      engine sniffed, as byte offsets inside one vertex
//                      (-1 = channel absent), e.g.
//                          layout=stride:32,pos:0,nrm:12,uv:24,col:-1
//                      omitted keys keep the sniffed value
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
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
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

// ---------------------------------------------------------------------------
//  quaternions (for baked animation rotations)
// ---------------------------------------------------------------------------
struct Quat { double x = 0, y = 0, z = 0, w = 1; };

// rotation part of a row-major column-vector M4 -> unit quaternion
// (Shepperd's method: pick the largest diagonal combination for stability)
inline Quat m4ToQuat(const M4 &mm)
{
    const double *m = mm.m;
    const double t = m[0] + m[5] + m[10];
    Quat q;
    if (t > 0.0) {
        double s = std::sqrt(t + 1.0) * 2.0;
        q.w = 0.25 * s;
        q.x = (m[9] - m[6]) / s;
        q.y = (m[2] - m[8]) / s;
        q.z = (m[4] - m[1]) / s;
    } else if (m[0] > m[5] && m[0] > m[10]) {
        double s = std::sqrt(1.0 + m[0] - m[5] - m[10]) * 2.0;
        q.w = (m[9] - m[6]) / s;
        q.x = 0.25 * s;
        q.y = (m[1] + m[4]) / s;
        q.z = (m[2] + m[8]) / s;
    } else if (m[5] > m[10]) {
        double s = std::sqrt(1.0 + m[5] - m[0] - m[10]) * 2.0;
        q.w = (m[2] - m[8]) / s;
        q.x = (m[1] + m[4]) / s;
        q.y = 0.25 * s;
        q.z = (m[6] + m[9]) / s;
    } else {
        double s = std::sqrt(1.0 + m[10] - m[0] - m[5]) * 2.0;
        q.w = (m[4] - m[1]) / s;
        q.x = (m[2] + m[8]) / s;
        q.y = (m[6] + m[9]) / s;
        q.z = 0.25 * s;
    }
    const double n = std::sqrt(q.x*q.x + q.y*q.y + q.z*q.z + q.w*q.w);
    if (n > 1e-12) { q.x /= n; q.y /= n; q.z /= n; q.w /= n; }
    else           { q = Quat{}; }
    return q;
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

inline bool readArray(Cur &c, char type, FbxProp &prop, bool discard = false)
{
    uint32_t len = c.u32(), enc = c.u32(), clen = c.u32();
    if (c.fail || len > (32u << 20)) return false;
    size_t elem = (type == 'f' || type == 'i') ? 4 : (type == 'b') ? 1 : 8;

    // Discard mode: consume the record without storing (or even inflating) it.
    // Used for animation payloads the caller does not want - the file position
    // still has to advance past them exactly.
    if (discard) {
        prop.type = type;
        if (enc == 0)      c.skip(size_t(len) * elem);
        else if (enc == 1) c.skip(clen);
        else return false;
        return !c.fail;
    }

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

inline bool readNode(Cur &c, uint32_t version, const uint8_t *base, FbxNode &out, int depth,
                     bool keepAnimArrays = true)
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

    // Mesh-only parse (sidecar anim=0): animation OBJECTS are skipped
    // wholesale - the record is validated and jumped over without building
    // any of its (very numerous) children. An animation dump export carries
    // hundreds of thousands of AnimationCurve objects; representing them as
    // FbxNode trees is what dominates peak memory, not the key payloads.
    if (!keepAnimArrays
        && (out.name == "AnimationCurve" || out.name == "AnimationCurveNode"
            || out.name == "AnimationStack" || out.name == "AnimationLayer")) {
        const uint8_t *nodeEnd = base + endOff;
        if (nodeEnd > c.end || nodeEnd < c.p) return false;
        c.p = nodeEnd;            // name kept, props/children dropped
        return true;
    }

    // Animation payload policy. KeyAttr* tangent data is never consumed by
    // the baker (linear resampling only), so its arrays are ALWAYS skipped;
    // the key times/values themselves are skipped only when the caller asked
    // for a mesh-only parse (sidecar anim=0). Skipped compressed arrays are
    // not even inflated, which is most of the byte volume of an animation
    // dump export.
    bool discardArrays = false;
    if (out.name == "KeyAttrDataFloat" || out.name == "KeyAttrFlags"
        || out.name == "KeyAttrRefCount")
        discardArrays = true;
    else if (!keepAnimArrays
             && (out.name == "KeyTime" || out.name == "KeyValueFloat"))
        discardArrays = true;

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
            if (!readArray(c, t, pr, discardArrays)) return false;
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
        if (!readNode(c, version, base, child, depth + 1, keepAnimArrays)) return false;
        if (child.name.empty() && child.props.empty() && child.children.empty())
            break;                                          // NULL terminator
        // Children of an AnimationCurve the baker never reads: keeping them
        // across ~10^5 curve objects is pure overhead, so they are parsed
        // (the cursor must advance) but not retained.
        if (out.name == "AnimationCurve"
            && (child.name == "KeyAttrFlags" || child.name == "KeyAttrDataFloat"
                || child.name == "KeyAttrRefCount" || child.name == "KeyVer"
                || child.name == "Default"))
            continue;
        out.children.push_back(std::move(child));
    }
    c.p = nodeEnd;
    // exact-fit the containers: with hundreds of thousands of tiny nodes the
    // vector growth slack alone is tens of MB
    out.props.shrink_to_fit();
    out.children.shrink_to_fit();
    return true;
}

inline bool parse(const uint8_t *data, size_t size, FbxNode &root, uint32_t &version,
                  bool keepAnimArrays = true)
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
        if (!readNode(c, version, data, top, 0, keepAnimArrays)) return false;
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
    // TransformLink row 3 = the bone's GLOBAL bind-pose position, in file
    // units. Ground truth for matching this cluster onto the target mesh's
    // bone array: exporter bone numbering can lie (a source rig with extra
    // jaw/tongue bones shifts every later index), bind positions cannot.
    bool   haveLink = false;
    double linkPos[3] { 0, 0, 0 };
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

// ---------------------------------------------------------------------------
//  baked animation clips
//
// One AnimChannel per animated node (bone), carrying its LOCAL transform in
// parent space over time:
//   - pos: Lcl Translation keys, already in game units (sceneScale applied,
//     the same unit bake nodeGlobal performs on geometry - valid because a
//     pure root scale distributes onto local translations),
//   - rot: quaternion keys with the node's RotationOrder and the constant
//     PreRotation/PostRotation folded in, i.e. exactly the rotation block
//     `pre * rot * post^-1` of nodeLocal, hemisphere-corrected key to key,
//   - scl: Lcl Scaling keys, unitless.
// Key times are SECONDS relative to the clip start. Tracks a take never
// touches are simply empty (sample as bind pose / identity).
//
// skelIndex starts unresolved (-1); buildSectionsForMesh() fills it against
// the TARGET mesh with the same bind-pose cluster table used for skinning
// (fallback: Bone_N name digits), so after one build the clips are ready to
// drive nglMesh::Bones-indexed palettes on the engine side.
// ---------------------------------------------------------------------------
struct AnimKey3 {
    float t = 0.f;                       // seconds from clip start
    float v[3] { 0.f, 0.f, 0.f };
};
struct AnimKeyQ {
    float t = 0.f;                       // seconds from clip start
    float q[4] { 0.f, 0.f, 0.f, 1.f };   // x y z w
};
struct AnimChannel {
    std::string           boneName;      // Scene::normName of the animated node
    int                   skelIndex = -1;// target skeleton bone, -1 = unmapped
    std::vector<AnimKey3> pos;
    std::vector<AnimKeyQ> rot;
    std::vector<AnimKey3> scl;
};
struct AnimClip {
    std::string              name;       // take name, normName'd
    double                   duration = 0.0;   // seconds
    std::vector<AnimChannel> channels;
};

struct SidecarCfg {
    double scale = 1.0, yaw = 0.0;
    V3     offset{};
    bool   fit = true, weld = true;
    // anim=0 skips FBX animation entirely: curve payloads are not even
    // inflated by the binary reader and no clips are baked
    bool   anim = true;
    // bind-pose cluster matching (bone_match=off falls back to Bone_N digits)
    bool   boneMatch = true;
    int    skin = 0;                    // 0 auto, 1 clusters, 2 transfer, 3 rigid
    int    tex  = 0;                    // 0 auto, 1 keep (never retarget),
                                        // 2 mod (mod files always win)
    // PER-SECTION TEXTURE PIN: tex<N>=STEM binds STEM to section N's material
    // unconditionally - it bypasses texture=keep, the round-trip guard and
    // every "is this really an override?" heuristic, and it works on sections
    // that were NOT replaced at all (vanilla geometry whose material simply
    // has no diffuse texture in this build). This is the deterministic lever
    // for the venom_eddie reveal: the exporter tags the forearm/hand/claw run
    // with VENOM_MOUTH and leaves the cocoon/eddie-head pieces with no
    // material at all, so those sections reach the engine with an empty
    // candidate list and draw pure white.
    //     tex7=VENOM_MOUTH
    //     tex11=VENOM_EDDIE
    std::map<int, std::string> texPin;
    // tex_default=STEM - last-resort stem for ANY section that would reach
    // the engine with no candidate at all. Kills the "blank white" class of
    // symptom in one line, before the per-section pins are dialled in.
    std::string texDefault;
    // autotex=off - disables the automatic blank-section texture coverage:
    // the scene-stem fallback candidates appended to guessed/empty lists and
    // the blank-fix carriers that let the engine repaint sections whose
    // material carries no diffuse texture at all. On by default: an
    // untextured section draws pure white, and binding one of the sheets the
    // file itself brings is always closer to the author's intent than white.
    bool autoTex = true;
    // per-section overrides (applied over every tier's result):
    //   keep=9,11-14  never replace those sections: geometry, index buffer
    //                 AND retail morph playback stay fully vanilla
    //   hide=3,7      force-hide those sections (degenerate triangle)
    std::set<int> keepSections;
    std::set<int> hideSections;
    // white=1,7  force those sections to draw PURE WHITE: the engine binds its
    // own 1x1 white texture (nglWhiteTex) to a private clone of the section
    // material and stops there - no candidate search, no numbered-variant
    // salvage, no sibling-donor borrow, no autotex carrier, and the blank
    // policy (blank=hide/import) is overridden.
    //
    // This is NOT the same thing as leaving a section blank and hoping. Some
    // vanilla pieces are white ON PURPOSE - the black suit's eye lenses and
    // the chest spider are untextured white geometry, and every "a blank
    // material is a bug" heuristic in this file will happily hide them
    // (blank=hide), repaint them with the body sheet (the sibling-donor
    // fallback) or dress them in whatever stem travelled with the FBX. white=
    // is how you say "this one really is white, leave it alone".
    //
    // Wins over hide=, keep=, blank= and tex<N>= for the same section.
    std::set<int> whiteSections;
    // white= also takes KEYWORDS, so the pieces can be addressed without
    // knowing their section numbers:
    //   white=auto   (default) a section whose material has NO diffuse
    //                texture AND whose material name reads like a piece that
    //                is meant to be white (EYE, LENS, LOGO, EMBLEM, ...)
    //                draws white instead of being "repaired" by the salvage
    //                and sibling-donor fallbacks. Those fallbacks exist for
    //                the venom_eddie reveal shells; on an eye lens they paint
    //                the body sheet over the lens, which is never right.
    //   white=blank  the same, for EVERY blank material regardless of name.
    //                The blunt instrument that needs no section numbers and
    //                no material names: nothing untextured gets repainted,
    //                white is left as white, exactly like retail.
    //   white=off    neither; an explicit white=N list still applies.
    // Keywords and numbers mix: white=blank,7
    bool whiteAuto  = true;
    bool whiteBlank = false;
    // white_names=EYE,LOGO - replaces the built-in keyword list used by
    // white=auto when non-empty (uppercase, matched as substrings)
    std::vector<std::string> whiteNames;
    // blank=keep|import|hide - what to do with a NAMED per-section piece
    // (Tier A/B, where piece i IS section i by name) that carries NO texture
    // reference in the file AND does not pass the round-trip detector.
    // Those pieces are the exporter's shape for morph-held reveal geometry
    // (the venom_eddie cocoon / teeth / eddie's head): the file stores them
    // at the fully OPEN pose and retail MORPHS fold them away every frame.
    // Replacing them dead-zones the morphs and freezes the shell OPEN over
    // the shoulders - and with no texture it draws PURE WHITE on top of the
    // character.
    //   keep   (default) leave the vanilla section untouched: geometry,
    //          palette and morph playback stay live, so the piece keeps
    //          folding away exactly like retail. Texture retarget still
    //          runs in blank-fix mode (repaint only if the material has no
    //          diffuse texture).
    //   import old behaviour: swap the geometry in (the piece WILL be
    //          permanently visible; combine with tex<N>= to dress it).
    //   hide   replace with a degenerate triangle (invisible, including in
    //          the reveal cutscene).
    // An explicit keep=/hide=/tex<N>= sidecar line for the section wins over
    // this policy either way.
    int blankGeo = 0;                   // 0 keep, 1 import, 2 hide
    // round-trip detector tolerance, in game units. A built section whose
    // geometry is spatially IDENTICAL to the original section within this
    // distance is not swapped in: the vanilla buffers - and with them retail
    // MORPH playback (the venom_eddie reveal cocoon, mouth visemes, facial
    // animation), which addresses vertices by vanilla index and is dead-zoned
    // on replaced sections - stay live. Texture retargeting still runs.
    // sidecar roundtrip=0 (or off) disables the detector and forces the
    // import; roundtrip=<eps> tunes the tolerance.
    double roundtripEps = 1e-3;
    // Static-section vertex layout override, for when the engine-side sniffer
    // reads a format wrong (a beta build's section layout, an unusual channel
    // order). Byte offsets inside one vertex, -1 = channel absent:
    //     layout=stride:32,pos:0,nrm:12,uv:24,col:-1
    // Any omitted key keeps the sniffed value. Applies to RIGID sections only:
    // the skinned 64-byte row is fixed by the shader.
    bool layoutSet = false;
    int  layStride = -1, layPos = -1, layNrm = -2, layUv = -2, layCol = -2;
};

struct Scene {
    std::map<int64_t, Geom>  geoms;
    std::map<int64_t, Model> models;
    std::map<int64_t, std::string> materials;   // id -> name
    std::map<int64_t, std::string> matTexStem;  // material id -> texture stem
    // per-stem payloads for the engine-side texture builder:
    // path exactly as written in the file ("VENOM.fbm\\USM_BLACKSUIT.DDS"),
    // and image bytes for FBX-embedded textures (Video nodes with Content)
    std::map<std::string, std::string> texRelOfStem;
    std::map<std::string, std::shared_ptr<std::vector<uint8_t>>> embeddedTex;
    std::vector<int64_t>     meshModelOrder;    // models of type Mesh, id order
    std::vector<AnimClip>    anims;             // baked takes (see above)
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

    struct Conn { int64_t child, parent; std::string prop; };
    std::map<int64_t, std::string> textureStems;   // texture id -> stem
    std::map<int64_t, std::string> textureRel;     // texture id -> path as written
    std::map<int64_t, std::string> videoStems;     // video id -> stem
    std::map<int64_t, std::string> videoRel;       // video id -> path as written
    std::map<int64_t, std::shared_ptr<std::vector<uint8_t>>> videoBytes; // embedded Content
    std::vector<Conn> conns;
    if (auto *cn = root.child("Connections"))
        for (auto &C : cn->children) {
            if (C.name != "C" && C.name != "Connect") continue;
            if (C.props.size() < 3) continue;
            Conn cc{ C.props[1].i, C.props[2].i, {} };
            // OP connections carry the destination property name ("Lcl
            // Translation" on a curve node -> model link, "d|X" on a curve ->
            // curve node link); OO connections have no 4th value
            if (C.props.size() >= 4 && C.props[3].type == 'S')
                cc.prop = C.props[3].s;
            conns.push_back(std::move(cc));
        }

    // ---- animation object harvest (FBX 7.x objectized takes) --------------
    struct ACurve   { std::vector<int64_t> t; std::vector<double> v; };
    struct ACNode   { double dflt[3] { 0, 0, 0 }; bool haveD[3] { false, false, false };
                      int64_t curve[3] { 0, 0, 0 };
                      int64_t model = 0; int prop = -1;      // 0=T 1=R 2=S
                      int64_t layer = 0; };
    std::map<int64_t, ACurve>      curveById;
    std::map<int64_t, ACNode>      cnodeById;
    std::map<int64_t, std::string> stackName;      // AnimationStack id -> name
    std::map<int64_t, int64_t>     stackStart, stackStop;   // KTime
    std::set<int64_t>              layerIds;
    std::map<int64_t, int64_t>     stackOfLayer;   // layer id -> stack id

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
        if (O.name == "Texture" || O.name == "Video") {
            std::string fn;
            if (auto *rf = O.child("RelativeFilename"); rf && !rf->props.empty())
                fn = rf->props[0].s;
            if (fn.empty())
                if (auto *ff = O.child("FileName"); ff && !ff->props.empty())
                    fn = ff->props[0].s;
            if (fn.empty())
                if (auto *ff = O.child("Filename"); ff && !ff->props.empty())
                    fn = ff->props[0].s;
            std::string raw = fn;                 // path exactly as written
            if (fn.empty()) fn = oname;
            // stem: basename without extension, uppercased
            if (auto sl = fn.find_last_of("/\\"); sl != std::string::npos)
                fn = fn.substr(sl + 1);
            if (auto dot = fn.rfind('.'); dot != std::string::npos)
                fn.resize(dot);
            if (auto nul = fn.find('\0'); nul != std::string::npos)
                fn.resize(nul);
            for (auto &c : fn) c = char(std::toupper(uint8_t(c)));
            if (O.name == "Texture") {
                if (!fn.empty()) textureStems[id] = fn;
                if (!raw.empty()) textureRel[id] = raw;
            } else {                               // Video: may embed the image
                if (!fn.empty()) videoStems[id] = fn;
                if (!raw.empty()) videoRel[id] = raw;
                if (auto *ct = O.child("Content");
                    ct && !ct->props.empty() && ct->props[0].type == 'R'
                    && !ct->props[0].s.empty())
                {
                    const std::string &bytes = ct->props[0].s;
                    videoBytes[id] = std::make_shared<std::vector<uint8_t>>(
                        bytes.begin(), bytes.end());
                }
            }
        }
        if (O.name == "Deformer") {
            if (oclass == "Skin") {
                geomOfSkin[id] = 0;
            } else if (oclass == "Cluster") {
                GCluster cl;
                if (auto *ix = O.child("Indexes"); ix && !ix->props.empty()) cl.idx = ix->props[0].ia;
                if (auto *w  = O.child("Weights"); w  && !w->props.empty())  cl.w   = w->props[0].fa;
                if (auto *tl = O.child("TransformLink");
                    tl && !tl->props.empty() && tl->props[0].fa.size() >= 16)
                {
                    const auto &m = tl->props[0].fa;
                    cl.linkPos[0] = m[12];
                    cl.linkPos[1] = m[13];
                    cl.linkPos[2] = m[14];
                    cl.haveLink = true;
                }
                clusterById[id] = std::move(cl);
            }
        }
        if (sc.cfg.anim) {
            if (O.name == "AnimationStack") {
                stackName[id] = oname;
                if (auto *p70 = O.child("Properties70"))
                    for (auto &P : p70->children) {
                        if (P.props.empty()) continue;
                        const std::string &k = P.props[0].s;
                        if      (k == "LocalStart")     stackStart[id] = P.props.back().i;
                        else if (k == "LocalStop")      stackStop[id]  = P.props.back().i;
                        else if (k == "ReferenceStop" && !stackStop.count(id))
                            stackStop[id] = P.props.back().i;
                    }
            } else if (O.name == "AnimationLayer") {
                layerIds.insert(id);
            } else if (O.name == "AnimationCurveNode") {
                ACNode cn;
                if (auto *p70 = O.child("Properties70"))
                    for (auto &P : p70->children) {
                        if (P.props.empty()) continue;
                        const std::string &k = P.props[0].s;
                        int ax = (k == "d|X") ? 0 : (k == "d|Y") ? 1
                               : (k == "d|Z") ? 2 : -1;
                        if (ax >= 0) { cn.dflt[ax] = P.props.back().d; cn.haveD[ax] = true; }
                    }
                cnodeById[id] = cn;
            } else if (O.name == "AnimationCurve") {
                ACurve cu;
                if (auto *kt = O.child("KeyTime"); kt && !kt->props.empty())
                    cu.t = kt->props[0].ia;
                if (auto *kv = O.child("KeyValueFloat"); kv && !kv->props.empty())
                    cu.v = kv->props[0].fa;
                if (!cu.t.empty() && cu.t.size() == cu.v.size())
                    curveById[id] = std::move(cu);
            }
        }
    }

    // Video -> Texture: a texture can take its file name from the connected
    // clip, and the clip can carry the actual image bytes (embedded export)
    for (auto &c : conns) {
        auto vs = videoStems.find(c.child);
        if (vs == videoStems.end() || !textureStems.count(c.parent))
            continue;
        std::string &tstem = textureStems[c.parent];
        if (tstem.empty()) tstem = vs->second;
        if (!textureRel.count(c.parent) && videoRel.count(c.child))
            textureRel[c.parent] = videoRel[c.child];
        if (auto vb = videoBytes.find(c.child);
            vb != videoBytes.end() && !tstem.empty()
            && !sc.embeddedTex.count(tstem))
            sc.embeddedTex[tstem] = vb->second;
    }
    // stand-alone embedded clips (no texture node in between)
    for (auto &vb : videoBytes)
        if (auto vs = videoStems.find(vb.first);
            vs != videoStems.end() && !vs->second.empty()
            && !sc.embeddedTex.count(vs->second))
            sc.embeddedTex[vs->second] = vb.second;

    for (auto &c : conns)
        if (auto ts = textureStems.find(c.child);
            ts != textureStems.end() && sc.materials.count(c.parent)
            && !sc.matTexStem.count(c.parent)) {
            sc.matTexStem[c.parent] = ts->second;
            if (auto tr = textureRel.find(c.child);
                tr != textureRel.end() && !sc.texRelOfStem.count(ts->second))
                sc.texRelOfStem[ts->second] = tr->second;
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

    // ---- animation wiring + bake ------------------------------------------
    if (sc.cfg.anim && !cnodeById.empty()) {
        for (auto &c : conns) {
            if (curveById.count(c.child)) {
                if (auto cn = cnodeById.find(c.parent); cn != cnodeById.end()) {
                    int ax = (c.prop == "d|X") ? 0 : (c.prop == "d|Y") ? 1
                           : (c.prop == "d|Z") ? 2 : -1;
                    if (ax >= 0 && cn->second.curve[ax] == 0)
                        cn->second.curve[ax] = c.child;
                    continue;
                }
            }
            if (auto cn = cnodeById.find(c.child); cn != cnodeById.end()) {
                if (sc.models.count(c.parent)) {
                    int pr = (c.prop == "Lcl Translation") ? 0
                           : (c.prop == "Lcl Rotation")    ? 1
                           : (c.prop == "Lcl Scaling")     ? 2 : -1;
                    if (pr >= 0) { cn->second.model = c.parent; cn->second.prop = pr; }
                    continue;
                }
                if (layerIds.count(c.parent)) { cn->second.layer = c.parent; continue; }
            }
            if (layerIds.count(c.child) && stackName.count(c.parent))
                stackOfLayer[c.child] = c.parent;
        }

        // stack -> model -> curve node per property. When two layers of one
        // stack animate the same (model, property), the FIRST curve node by
        // id wins - real layer blending is out of scope.
        std::map<int64_t, std::map<int64_t, std::array<const ACNode *, 3>>> perStack;
        size_t orphans = 0;
        for (auto &[cid, cn] : cnodeById) {
            if (cn.prop < 0 || cn.model == 0) continue;
            if (!cn.curve[0] && !cn.curve[1] && !cn.curve[2]) continue;
            int64_t stack = 0;
            if (cn.layer)
                if (auto it = stackOfLayer.find(cn.layer); it != stackOfLayer.end())
                    stack = it->second;
            if (stack == 0 && stackName.size() == 1)
                stack = stackName.begin()->first;   // single-take orphans
            if (stack == 0 && !stackName.empty()) { ++orphans; continue; }
            auto &slot = perStack[stack][cn.model];
            if (!slot[cn.prop]) slot[cn.prop] = &cn;
        }
        if (orphans)
            logf("[modmesh] animation: %u curve node(s) not reachable from "
                 "any take - skipped", unsigned(orphans));

        constexpr double KTIME = 46186158000.0;      // KTime ticks per second
        // Hard memory guard for the 32-bit host process: a baked key is
        // 16-20 bytes, so 16M keys is ~300MB. Past that the remaining takes
        // are dropped with a log instead of exhausting the address space.
        constexpr size_t kMaxBakedKeys = size_t(16) << 20;
        size_t bakedKeys = 0;
        bool   capped    = false;

        for (auto &[stackId, modelsMap] : perStack) {
            if (capped) break;
            AnimClip clip;
            clip.name = (stackId && stackName.count(stackId))
                      ? Scene::normName(stackName[stackId])
                      : std::string("take_001");

            // clip-relative time base: LocalStart when the stack declares
            // one, clamped to the earliest key so times never go negative
            int64_t ktStart = INT64_MAX;
            if (auto it = stackStart.find(stackId); it != stackStart.end())
                ktStart = it->second;
            for (auto &[mid, slot] : modelsMap)
                for (const ACNode *cn : slot) {
                    if (!cn) continue;
                    for (int ax = 0; ax < 3; ++ax)
                        if (cn->curve[ax])
                            if (auto cu = curveById.find(cn->curve[ax]);
                                cu != curveById.end() && !cu->second.t.empty())
                                ktStart = std::min(ktStart, cu->second.t.front());
                }
            if (ktStart == INT64_MAX) continue;      // no keys at all

            double maxT = 0.0;
            for (auto &[mid, slot] : modelsMap) {
                auto mit = sc.models.find(mid);
                if (mit == sc.models.end()) continue;
                const Model &bm = mit->second;

                AnimChannel ch;
                ch.boneName = Scene::normName(bm.name);
                const M4 pre   = eulerDeg(bm.preR, 0);
                const M4 postI = eulerDeg({ -bm.postR.x, -bm.postR.y, -bm.postR.z }, 5);

                for (int prop = 0; prop < 3; ++prop) {
                    const ACNode *cn = slot[prop];
                    if (!cn) continue;

                    const ACurve *ax[3] { nullptr, nullptr, nullptr };
                    std::vector<int64_t> times;
                    for (int a = 0; a < 3; ++a)
                        if (cn->curve[a])
                            if (auto cu = curveById.find(cn->curve[a]);
                                cu != curveById.end()) {
                                ax[a] = &cu->second;
                                times.insert(times.end(), cu->second.t.begin(),
                                             cu->second.t.end());
                            }
                    if (times.empty()) continue;
                    std::sort(times.begin(), times.end());
                    times.erase(std::unique(times.begin(), times.end()), times.end());

                    const V3 &stat = (prop == 0) ? bm.T : (prop == 1) ? bm.R : bm.S;
                    const double statAx[3] { stat.x, stat.y, stat.z };
                    auto evalAx = [&](int a, int64_t t) -> double {
                        if (!ax[a])
                            return cn->haveD[a] ? cn->dflt[a] : statAx[a];
                        const auto &T = ax[a]->t;
                        const auto &V = ax[a]->v;
                        auto it = std::lower_bound(T.begin(), T.end(), t);
                        if (it == T.end())   return V.back();
                        if (it == T.begin()) return V.front();
                        size_t i1 = size_t(it - T.begin());
                        if (T[i1] == t) return V[i1];
                        size_t i0 = i1 - 1;
                        const double f = double(t - T[i0]) / double(T[i1] - T[i0]);
                        return V[i0] + (V[i1] - V[i0]) * f;
                    };

                    if (bakedKeys + times.size() > kMaxBakedKeys) { capped = true; break; }
                    bakedKeys += times.size();

                    if (prop == 0) ch.pos.reserve(times.size());
                    else if (prop == 2) ch.scl.reserve(times.size());
                    else ch.rot.reserve(times.size());

                    for (int64_t t : times) {
                        const double x = evalAx(0, t), y = evalAx(1, t), z = evalAx(2, t);
                        const float ts = float(double(t - ktStart) / KTIME);
                        if (ts > maxT) maxT = ts;
                        if (prop == 0) {
                            ch.pos.push_back({ ts, { float(x * sc.sceneScale),
                                                     float(y * sc.sceneScale),
                                                     float(z * sc.sceneScale) } });
                        } else if (prop == 2) {
                            ch.scl.push_back({ ts, { float(x), float(y), float(z) } });
                        } else {
                            const M4 R = pre * eulerDeg({ x, y, z }, bm.rotOrder) * postI;
                            Quat q = m4ToQuat(R);
                            if (!ch.rot.empty()) {   // hemisphere continuity
                                const auto &p = ch.rot.back().q;
                                if (p[0]*q.x + p[1]*q.y + p[2]*q.z + p[3]*q.w < 0.0)
                                    { q.x = -q.x; q.y = -q.y; q.z = -q.z; q.w = -q.w; }
                            }
                            ch.rot.push_back({ ts, { float(q.x), float(q.y),
                                                     float(q.z), float(q.w) } });
                        }
                    }
                }
                if (capped) break;
                if (!ch.pos.empty() || !ch.rot.empty() || !ch.scl.empty())
                    clip.channels.push_back(std::move(ch));
            }

            clip.duration = maxT;
            if (auto st = stackStop.find(stackId); st != stackStop.end())
                clip.duration = std::max(clip.duration,
                                         double(st->second - ktStart) / KTIME);
            if (!clip.channels.empty())
                sc.anims.push_back(std::move(clip));
        }

        if (capped)
            logf("[modmesh] animation: baked-key budget (%u) exhausted - "
                 "remaining takes DROPPED (split the file or sidecar anim=0)",
                 unsigned(kMaxBakedKeys));
        if (!sc.anims.empty()) {
            size_t nch = 0;
            for (auto &cl : sc.anims) nch += cl.channels.size();
            logf("[modmesh] animation: %u take(s), %u channel(s), %u baked key(s)",
                 unsigned(sc.anims.size()), unsigned(nch), unsigned(bakedKeys));
        }
    }
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
//  animation sampling - engine-side helpers over Scene::anims
//
//  All samplers clamp outside the key range and interpolate linearly
//  (rotations: normalized lerp on hemisphere-corrected keys, which for the
//  small per-frame steps of a baked take is indistinguishable from slerp).
//  A missing track returns false and leaves the identity in `out`, so the
//  caller can keep that component of the bind pose.
// ===========================================================================
inline bool sampleTrack3(const std::vector<AnimKey3> &k, double t, float out[3])
{
    if (k.empty()) return false;
    if (t <= k.front().t) { for (int i = 0; i < 3; ++i) out[i] = k.front().v[i]; return true; }
    if (t >= k.back().t)  { for (int i = 0; i < 3; ++i) out[i] = k.back().v[i];  return true; }
    auto it = std::lower_bound(k.begin(), k.end(), t,
        [](const AnimKey3 &a, double x) { return a.t < x; });
    const AnimKey3 &b = *it, &a = *(it - 1);
    const float f = (b.t > a.t) ? float((t - a.t) / (b.t - a.t)) : 0.f;
    for (int i = 0; i < 3; ++i) out[i] = a.v[i] + (b.v[i] - a.v[i]) * f;
    return true;
}

inline bool sampleTrackQ(const std::vector<AnimKeyQ> &k, double t, float out[4])
{
    if (k.empty()) return false;
    auto copy = [&](const AnimKeyQ &s) { for (int i = 0; i < 4; ++i) out[i] = s.q[i]; };
    if (t <= k.front().t) { copy(k.front()); return true; }
    if (t >= k.back().t)  { copy(k.back());  return true; }
    auto it = std::lower_bound(k.begin(), k.end(), t,
        [](const AnimKeyQ &a, double x) { return a.t < x; });
    const AnimKeyQ &b = *it, &a = *(it - 1);
    const float f = (b.t > a.t) ? float((t - a.t) / (b.t - a.t)) : 0.f;
    float n = 0.f;
    for (int i = 0; i < 4; ++i) { out[i] = a.q[i] + (b.q[i] - a.q[i]) * f; n += out[i] * out[i]; }
    n = std::sqrt(n);
    if (n > 1e-8f) for (int i = 0; i < 4; ++i) out[i] /= n;
    else           { out[0] = out[1] = out[2] = 0.f; out[3] = 1.f; }
    return true;
}

// TRS at time t. Missing tracks come back as identity (pos 0, rot identity,
// scl 1); the returned mask says which tracks were actually animated
// (bit 0 = pos, bit 1 = rot, bit 2 = scl).
inline unsigned sampleChannel(const AnimChannel &c, double t,
                              float pos[3], float rot[4], float scl[3])
{
    pos[0] = pos[1] = pos[2] = 0.f;
    rot[0] = rot[1] = rot[2] = 0.f; rot[3] = 1.f;
    scl[0] = scl[1] = scl[2] = 1.f;
    unsigned mask = 0;
    if (sampleTrack3(c.pos, t, pos)) mask |= 1u;
    if (sampleTrackQ(c.rot, t, rot)) mask |= 2u;
    if (sampleTrack3(c.scl, t, scl)) mask |= 4u;
    return mask;
}

// Local bone matrix T*R*S at time t. Row-major, column-vector convention
// (v' = M * v), identical to modmesh::M4 - transpose before handing it to a
// row-vector engine matrix type.
inline unsigned channelLocalMatrix(const AnimChannel &c, double t, float m[16])
{
    float p[3], q[4], s[3];
    const unsigned mask = sampleChannel(c, t, p, q, s);
    const float x = q[0], y = q[1], z = q[2], w = q[3];
    const float r00 = 1 - 2*(y*y + z*z), r01 = 2*(x*y - z*w),   r02 = 2*(x*z + y*w);
    const float r10 = 2*(x*y + z*w),     r11 = 1 - 2*(x*x + z*z), r12 = 2*(y*z - x*w);
    const float r20 = 2*(x*z - y*w),     r21 = 2*(y*z + x*w),   r22 = 1 - 2*(x*x + y*y);
    m[0]  = r00 * s[0]; m[1]  = r01 * s[1]; m[2]  = r02 * s[2]; m[3]  = p[0];
    m[4]  = r10 * s[0]; m[5]  = r11 * s[1]; m[6]  = r12 * s[2]; m[7]  = p[1];
    m[8]  = r20 * s[0]; m[9]  = r21 * s[1]; m[10] = r22 * s[2]; m[11] = p[2];
    m[12] = 0.f; m[13] = 0.f; m[14] = 0.f; m[15] = 1.f;
    return mask;
}

inline const AnimClip *findClip(const Scene &sc, const std::string &name)
{
    const std::string n = Scene::normName(name);
    for (const auto &c : sc.anims)
        if (c.name == n) return &c;
    return nullptr;
}

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
// One ORIGINAL section as the engine sees it, BEFORE the vanilla load tail.
//
// Two families exist in the game and both are replaceable:
//
//   SKINNED  the retail 64-byte usperson/us_character row - px py pz | nx ny
//            nz | u v | i0..i3 | w0..w3 - carried by characters. The only
//            layout that can receive blend indices/weights.
//   RIGID    every STATIC mesh: weapons and pickups (skins_flamethrower),
//            hero-gauge/HUD meshes (HG_HERO_*, HG_BOSS_*), props, world
//            geometry. An arbitrary interleaved D3D vertex - stride 16..64,
//            no blend data at all, frequently with a D3DCOLOR channel
//            holding the baked lighting. Described by the byte offsets
//            below; -1 means the channel does not exist.
//
// The defaults describe the skinned row, so an engine that only fills
// strideBytes = 64 keeps exactly the pre-static behaviour.
struct OrigSectionView {
    const float    *verts   = nullptr;   // first vertex of the CPU stream
    uint32_t        nverts  = 0;
    uint32_t        strideBytes = 0;     // from the file
    const uint16_t *palette = nullptr;   // BonesIdx
    int             nbones  = 0;

    // ---- vertex layout -------------------------------------------------
    bool skinned = true;                 // blend idx at 32, weights at 48
    int  posOff  = 0;                    // float3, required
    int  nrmOff  = 12;                   // float3
    int  uvOff   = 24;                   // float2
    int  colOff  = -1;                   // D3DCOLOR dword

    // the retail skinned row: the only layout that can carry weights
    bool skinnedRow() const
    {
        return skinned && strideBytes == 64 && verts != nullptr && nverts != 0;
    }
    // any layout the importer can read donors from and write a replacement in
    bool replaceable() const
    {
        if (verts == nullptr || nverts == 0) return false;
        if (skinnedRow()) return true;
        return !skinned && strideBytes >= 12 && strideBytes <= 256
            && (strideBytes % 4) == 0 && posOff >= 0
            && posOff + 12 <= int(strideBytes);
    }
    bool rigidRow() const { return replaceable() && !skinnedRow(); }

    const uint8_t *row(uint32_t i) const
    {
        return reinterpret_cast<const uint8_t *>(verts)
             + size_t(i) * size_t(strideBytes);
    }
    void getPos(uint32_t i, float out[3]) const
    { std::memcpy(out, row(i) + posOff, 12); }
    bool getNrm(uint32_t i, float out[3]) const
    {
        if (nrmOff < 0) return false;
        std::memcpy(out, row(i) + nrmOff, 12);
        return true;
    }
    bool getUV(uint32_t i, float out[2]) const
    {
        if (uvOff < 0) return false;
        std::memcpy(out, row(i) + uvOff, 8);
        return true;
    }
    uint32_t getCol(uint32_t i) const
    {
        uint32_t c = 0xFFFFFFFFu;
        if (colOff >= 0) std::memcpy(&c, row(i) + colOff, 4);
        return c;
    }
};

// approximate reference frame of the original mesh, used to fit imports when
// none of the section vertex views is usable (prerelease/beta builds whose
// section layouts are not the retail 64-byte one). Bind-pose bone positions
// give the body frame; the header bounding sphere is the coarse fallback.
struct OrigMeshRef {
    bool  haveBones = false;
    float bonesMin[3] { 0.f, 0.f, 0.f };
    float bonesMax[3] { 0.f, 0.f, 0.f };
    bool  haveSphere = false;
    float sphereCenter[3] { 0.f, 0.f, 0.f };
    float sphereRadius = 0.f;
    // nglMesh::NBones of the TARGET mesh. Everything that ends up in a rebuilt
    // BonesIdx must be < this: the engine indexes Mesh->Bones with the palette
    // straight from the vertex blend index, so an entry past the end reads
    // whatever follows the bone array and the section deforms into spikes.
    // 0 = unknown, no range check.
    int   nbones = 0;
    // Bind-pose position of every target bone (xyz per bone, nbones entries),
    // read from nglMesh::Bones row 3 BEFORE the loader inverts them. Enables
    // matching FBX clusters onto this array by position instead of trusting
    // the Bone_N digits.
    std::vector<float> bonePos;
};

struct BuiltSection {
    std::vector<float>    vertices;      // 16 floats / vertex
    std::vector<uint32_t> indices;       // triangle list, game winding
    uint32_t              weightClass = 2; // -> nglMeshSection::field_5C
    bool                  keepOriginalPalette = true;
    // ---- static rigid target -------------------------------------------
    // vertices[] is ALWAYS the canonical 16-float row, whatever the target
    // is. When the replaced section is a static one the engine must pack
    // those rows down into the section's own vertex format instead of
    // uploading them as-is: stride and channel offsets are copied here from
    // the OrigSectionView so the applier needs nothing else. Blend lanes are
    // still filled (slot 0 / weight 1) but never written to the buffer.
    bool                  rigid = false;
    uint32_t              targetStride = 64;
    int                   tPosOff = 0;
    int                   tNrmOff = 12;
    int                   tUvOff  = 24;
    int                   tColOff = -1;
    // one D3DCOLOR per vertex, only when the target has a colour channel.
    // Static USM geometry stores its BAKED LIGHTING there, so an import that
    // wrote plain white would render flat and unlit: the values are sampled
    // from the nearest original vertex instead.
    std::vector<uint32_t> colors;
    std::vector<uint16_t> palette;       // used when !keepOriginalPalette
    std::string           source;
    bool                  hide = false;
    // exact-round-trip marker: the piece reproduces the ORIGINAL section, so
    // the engine must NOT swap buffers - vanilla geometry and retail MORPH
    // playback (the eddie reveal cocoon, mouth visemes, facial animation)
    // stay live - but still runs the texture retarget with the candidates
    // below. vertices/indices are cleared when this is set.
    bool                  keepGeometry = false;
    // texture stems (uppercase, no path/ext) to try on the section material,
    // in priority order: FBX-referenced files first, then the source family
    std::vector<std::string> textureCandidates;
    // per-stem payloads the engine-side builder can use when the stem does
    // not resolve to a resident/mod texture:
    //   embeddedTex - image file bytes carried inside the FBX (Video/Content)
    //   texRelPath  - path exactly as written in the FBX, resolved against
    //                 the mod file's own folder ("VENOM.fbm\\FOO.DDS")
    std::map<std::string, std::shared_ptr<std::vector<uint8_t>>> embeddedTex;
    std::map<std::string, std::string> texRelPath;
    // sidecar texture policy, carried to the engine-side retarget:
    // 0 auto (indexed/luminance DDS never displaces a resident texture),
    // 1 keep (never touch the material), 2 mod (mod-shipped always wins)
    int texMode = 0;
    // sidecar tex<N>=STEM: textureCandidates holds exactly the pinned stem and
    // the engine must bind it whatever the policy says - texture=keep, the
    // round-trip "user files only" restriction and the "the stem already IS
    // the section's texture" bail-out are all skipped. The engine also CLONES
    // the material before repainting it, so a pin lands on this section only
    // and never bleeds onto the sections that share the same nglMaterialBase.
    bool texExclusive = false;
    // the candidate list is a GUESS (family-name fallback and/or the
    // scene-wide stem sweep), not an authored material reference from the
    // file. buildSectionsForMesh appends the scene's own sheets to such
    // lists so a wrong guess degrades to "some sheet from this very file",
    // never to a white section.
    bool autoStems = false;
    // engine contract for the automatic blank-fix carriers: only touch the
    // material when its diffuse texture is MISSING (draws pure white). A
    // record with this set must never displace a texture the section
    // already draws with. Cleared by a sidecar tex<N>= pin.
    bool blankOnly = false;
    // sidecar white=N: bind the engine's own 1x1 white texture to a private
    // clone of this section's material and run NOTHING else - no candidate
    // search, no salvage, no donor borrow. The one deterministic way to say
    // "this piece is white on purpose" (black-suit eye lenses, chest spider)
    // to a pipeline whose every other heuristic treats white as a defect.
    bool forceWhite = false;
    // sidecar white=blank / white=auto, carried to the engine-side retarget.
    // Both only ever fire on a material with NO diffuse texture bound - the
    // one state where "draw white" is a faithful reproduction of retail and
    // the blank-repair fallbacks are a guess. whiteByName additionally
    // requires the section's material name to match whiteNames.
    bool whiteBlank  = false;
    bool whiteByName = false;
    // keyword list for whiteByName; shared, not copied per section
    std::shared_ptr<const std::vector<std::string>> whiteNames;
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

// per-control-point skin from clusters, remapped through `boneRemap`.
// `clusterShift`, when given, carries the bind-pose RETARGET displacement of
// each cluster: the vector that moves geometry authored around the source
// rig's bone onto the target mesh's equivalent bone. Accumulated per control
// point with the cluster weights, so a vertex blended across a retargeted and
// a non-retargeted bone moves proportionally instead of snapping.
struct SkinTable {
    struct Inf { float idx; float w; };
    std::vector<std::array<Inf, 4>> per;
    std::vector<std::array<float, 3>> shift;   // empty = no retarget

    void build(const Geom &g, const std::vector<int> &boneRemap,
               const std::vector<std::array<float, 3>> *clusterShift = nullptr)
    {
        size_t n = g.ctrl.size() / 3;
        per.assign(n, { { { -1, 0 }, { -1, 0 }, { -1, 0 }, { -1, 0 } } });

        std::vector<std::array<double, 3>> sAcc;
        std::vector<double>                wAcc;
        const bool wantShift = clusterShift != nullptr;
        if (wantShift) {
            sAcc.assign(n, { 0.0, 0.0, 0.0 });
            wAcc.assign(n, 0.0);
        }

        for (size_t ci = 0; ci < g.clusters.size(); ++ci) {
            int mapped = ci < boneRemap.size() ? boneRemap[ci] : -1;
            if (mapped < 0) continue;
            const GCluster &cl = g.clusters[ci];
            const bool hasShift = wantShift && ci < clusterShift->size();
            for (size_t k = 0; k < cl.idx.size(); ++k) {
                int64_t v = cl.idx[k];
                double  w = cl.w[k];
                if (v < 0 || size_t(v) >= n || w <= 0) continue;
                if (hasShift) {
                    const auto &s = (*clusterShift)[ci];
                    sAcc[size_t(v)][0] += w * double(s[0]);
                    sAcc[size_t(v)][1] += w * double(s[1]);
                    sAcc[size_t(v)][2] += w * double(s[2]);
                    wAcc[size_t(v)]    += w;
                }
                auto &a = per[size_t(v)];
                int weakest = 0;
                for (int j = 1; j < 4; ++j) if (a[j].w < a[weakest].w) weakest = j;
                if (w > a[weakest].w) a[weakest] = { float(mapped), float(w) };
            }
        }

        if (wantShift) {
            shift.assign(n, { 0.f, 0.f, 0.f });
            for (size_t v = 0; v < n; ++v)
                if (wAcc[v] > 1e-9)
                    for (int c = 0; c < 3; ++c)
                        shift[v][c] = float(sAcc[v][c] / wAcc[v]);
        } else {
            shift.clear();
        }
        for (auto &a : per) {
            std::sort(a.begin(), a.end(), [](const Inf &x, const Inf &y) { return x.w > y.w; });
            float s = a[0].w + a[1].w + a[2].w + a[3].w;
            if (s > 0) {
                float inv = 1.f / s;
                for (auto &e : a) { e.w *= inv; if (e.w <= 0) e.idx = -1; }
                if (a[0].idx < 0) a[0] = { 0, 1 };
            }
            // s == 0: every influence of this vertex sat on a DROPPED cluster
            // (a bone the target mesh does not have). Leave it weightless -
            // {-1, 0} - instead of pinning it to bone 0: the weightless-corner
            // donor patch downstream rebinds it from the ORIGINAL skin of the
            // section it lands on, which is the right answer for e.g. mouth
            // geometry whose jaw/tongue bones do not exist on this mesh.
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
            // bind-pose retarget: shift is already in game space, so it goes
            // on AFTER the node transform
            if (skin && ctrlIdx < skin->shift.size()) {
                const auto &s = skin->shift[ctrlIdx];
                p.x += s[0]; p.y += s[1]; p.z += s[2];
            }
            c.px = float(p.x); c.py = float(p.y); c.pz = float(p.z);
            double nv[3];
            layerValue(g.nrm, ctrlIdx, pv, polyIdx, nv, 3);
            V3 n = nrmM.vec({ nv[0], nv[1], nv[2] });
            // a mirrored node transform (det < 0) flips the orientation the
            // inverse-transpose hands back: without the sign fix every normal
            // points INTO the body, the coherent orienter then trusts them and
            // winds the whole shell inside-out - backface-culled from outside,
            // visible from within, toon ramp sampled at the wrong end
            if (mirrored) { n.x = -n.x; n.y = -n.y; n.z = -n.z; }
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
                // A corner whose every influence sat on a dropped cluster is
                // left WEIGHTLESS on purpose: patchWeightlessCorners re-skins
                // it from the original section. Pinning it to bone 0 here used
                // to silently defeat that (and drag the piece to the pelvis).
                if (c.bi[0] < 0) { c.bi[0] = -1.f; c.bw[0] = 0.f; }
            }
            return c;
        };

        auto &B = buckets[slot];
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
// A corner is only merged with another when position, normal, UV AND the full
// skin binding agree. The weights need one slot each: they used to be folded
// as w0 ^ (w1 << 10) ^ (w2 << 20) with w quantized to 0..2048 - a 12-bit value
// in 10-bit strides, so the fields overlapped and XOR made distinct weight
// triples collide (and w3 was not in the key at all). Two corners that share a
// position, normal and UV but sit at different points of a weight ramp then
// welded into one; the survivor's binding won, and the loser's vertex followed
// the wrong bone. On a limb - where positions repeat across a smooth ramp -
// that is exactly one triangle shooting off the arm.
struct WeldKey {
    int32_t v[16];
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
        k.v[12] = q(c.bw[0], 4096); k.v[13] = q(c.bw[1], 4096);
        k.v[14] = q(c.bw[2], 4096); k.v[15] = q(c.bw[3], 4096);
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
// bi/bw are only meaningful for donors harvested from a SKINNED section;
// `col` only for donors harvested from a RIGID one that has a colour channel.
struct Donor { float p[3]; float n[3]; float bi[4]; float bw[4];
               uint32_t col = 0xFFFFFFFFu; };

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
        // the donor cloud comes from UN-welded corners: every welded
        // position appears once per incident triangle. Deduplicate by
        // position (same 1/8192 quantization as the weld) or the k nearest
        // of any query are k copies of one point and the blend degenerates
        // to single-nearest.
        {
            struct K { int32_t x, y, z;
                bool operator==(const K &o) const { return x==o.x && y==o.y && z==o.z; } };
            struct KH { size_t operator()(const K &k) const {
                return (size_t(uint32_t(k.x)) * 73856093u)
                     ^ (size_t(uint32_t(k.y)) * 19349663u)
                     ^ (size_t(uint32_t(k.z)) * 83492791u); } };
            std::unordered_map<K, uint32_t, KH> seen;
            seen.reserve(donors.size());
            std::vector<Donor> uniq;
            uniq.reserve(donors.size() / 3);
            for (auto &d : donors) {
                K k{ int32_t(std::lround(double(d.p[0]) * 8192)),
                     int32_t(std::lround(double(d.p[1]) * 8192)),
                     int32_t(std::lround(double(d.p[2]) * 8192)) };
                if (seen.emplace(k, uint32_t(uniq.size())).second)
                    uniq.push_back(d);
            }
            donors = std::move(uniq);
        }
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

    // up to K nearest donors (K <= 8), sorted by distance. Same ring search
    // as nearest(); falls back to a full scan when the rings find nothing.
    int nearestK(const float p[3], int K, const Donor *out[], float outD2[]) const
    {
        if (donors.empty() || K <= 0) return 0;
        if (K > 8) K = 8;
        int cx = int((p[0] - minB[0]) / cell);
        int cy = int((p[1] - minB[1]) / cell);
        int cz = int((p[2] - minB[2]) / cell);
        int n = 0;
        auto offer = [&](const Donor &d) {
            float ax = d.p[0]-p[0], ay = d.p[1]-p[1], az = d.p[2]-p[2];
            float dist = ax*ax + ay*ay + az*az;
            if (n == K && dist >= outD2[n - 1]) return;
            int i = (n < K) ? n : K - 1;
            while (i > 0 && outD2[i - 1] > dist) {
                if (i < K) { out[i] = out[i - 1]; outD2[i] = outD2[i - 1]; }
                --i;
            }
            out[i] = &d; outD2[i] = dist;
            if (n < K) ++n;
        };
        for (int r = 0; r <= 4; ++r) {
            for (int dz = -r; dz <= r; ++dz)
                for (int dy = -r; dy <= r; ++dy)
                    for (int dx = -r; dx <= r; ++dx) {
                        if (std::max({ std::abs(dx), std::abs(dy), std::abs(dz) }) != r) continue;
                        auto it = cells.find(key(cx + dx, cy + dy, cz + dz));
                        if (it == cells.end()) continue;
                        for (uint32_t i : it->second) offer(donors[i]);
                    }
            if (n == K && outD2[n - 1] <= (cell * float(r)) * (cell * float(r))) break;
        }
        if (n == 0)
            for (auto &d : donors) offer(d);
        return n;
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

inline void pushUniqueTex(std::vector<std::string> &v, const std::string &t)
{
    if (t.empty()) return;
    // PBR side maps are useless on the fixed-function material slots
    static const char *side[] = { "_NORMAL", "_NORMALMAP", "_NRM", "_ORM",
        "_ROUGHNESS", "_METALLIC", "_METALNESS", "_AO", "_AMBIENTOCCLUSION",
        "_OCCLUSION", "_EMISSIVE", "_EMISSION", "_SPECULAR", "_GLOSS",
        "_HEIGHT", "_DISPLACEMENT", "_BUMP", "_OPACITY", "_ALPHA" };
    for (const char *b : side) {
        const size_t l = std::strlen(b);
        if (t.size() >= l && t.compare(t.size() - l, l, b) == 0) return;
    }
    for (auto &e : v) if (e == t) return;
    v.push_back(t);
}

// color maps first: the engine slot we retarget is the diffuse one
inline void sortColorTexFirst(std::vector<std::string> &v)
{
    std::stable_sort(v.begin(), v.end(),
        [](const std::string &a, const std::string &b) {
            auto rank = [](const std::string &s) {
                return (s.find("BASECOLOR") != std::string::npos
                     || s.find("ALBEDO")    != std::string::npos
                     || s.find("DIFFUSE")   != std::string::npos) ? 0 : 1;
            };
            return rank(a) < rank(b);
        });
}

// Every color-capable stem the file itself brings - material references,
// FBX-embedded images (Video/Content) and referenced texture file names -
// ordered for use as a last-resort candidate list: sheets named after the
// mesh family first (VENOM_EDDIE_01 before VENOM_MOUTH for venom_eddie000),
// numbered variants ascending, and environment/reflection sheets dropped
// entirely (US_CHAR_SPHRMAP_INK_3 must never become somebody's diffuse).
inline std::vector<std::string> sceneColorStems(const Scene &sc,
                                                const std::string &meshName)
{
    auto isEnvSheet = [](const std::string &s) {
        std::string u = s;
        for (auto &c : u) c = char(std::toupper(uint8_t(c)));
        static const char *bad[] = { "SPHRMAP", "SPHMAP", "SPHEREMAP",
            "ENVMAP", "CUBEMAP", "REFLECT", "LIGHTMAP", "SHADOW" };
        for (const char *b : bad)
            if (u.find(b) != std::string::npos) return true;
        return false;
    };
    std::vector<std::string> all;
    for (const auto &m : sc.matTexStem)
        if (!m.second.empty() && !isEnvSheet(m.second))
            pushUniqueTex(all, m.second);
    for (const auto &e : sc.embeddedTex)
        if (!e.first.empty() && !isEnvSheet(e.first))
            pushUniqueTex(all, e.first);
    for (const auto &r : sc.texRelOfStem)
        if (!r.first.empty() && !isEnvSheet(r.first))
            pushUniqueTex(all, r.first);

    // family prefix: "venom_eddie000" -> "VENOM_EDDIE"
    std::string famU = meshName;
    size_t st = famU.size();
    while (st > 0 && std::isdigit(uint8_t(famU[st - 1]))) --st;
    famU.resize(st);
    while (!famU.empty() && famU.back() == '_') famU.pop_back();
    for (auto &c : famU) c = char(std::toupper(uint8_t(c)));

    auto matchesFam = [&](const std::string &s) {
        if (famU.size() < 4 || s.size() < famU.size()) return false;
        for (size_t i = 0; i < famU.size(); ++i)
            if (std::toupper(uint8_t(s[i])) != int(uint8_t(famU[i])))
                return false;
        return true;
    };
    std::vector<std::string> fam, rest;
    for (const std::string &s : all)
        (matchesFam(s) ? fam : rest).push_back(s);
    std::sort(fam.begin(), fam.end());          // _01 before _03
    std::sort(rest.begin(), rest.end());
    fam.insert(fam.end(), rest.begin(), rest.end());
    sortColorTexFirst(fam);        // BASECOLOR/ALBEDO/DIFFUSE bubble up
    return fam;
}

// copy the scene's per-stem payloads (embedded bytes / written file paths)
// for the stems the section actually asks for
inline void attachTexPayloads(BuiltSection &b, const Scene &sc)
{
    for (const std::string &t : b.textureCandidates) {
        if (auto e = sc.embeddedTex.find(t);
            e != sc.embeddedTex.end() && e->second && !e->second->empty())
            b.embeddedTex.emplace(t, e->second);
        if (auto r = sc.texRelOfStem.find(t);
            r != sc.texRelOfStem.end() && !r->second.empty())
            b.texRelPath.emplace(t, r->second);
    }
}

// Enforce the game convention cross(B-A, C-A) . normal < 0 (clockwise seen
// from the normal side) on every triangle. Empirically the retail PCMESH
// strips are 100% negative after D3D parity correction, while exports can
// carry mixed winding (raw strip unrolls alternate every triangle).
// >=90% of triangles carrying one identical normal on all three corners
// marks a flat-shaded export; when the donor transfer could not recover the
// authored normals those flat normals are UNTRUSTWORTHY - the Blender addon
// writes geometric face normals of the strip-unrolled triangle order, whose
// sign alternates with strip parity.
inline bool normalsMostlyFlat(const TriBucket &B)
{
    size_t flat = 0, tris = 0;
    for (size_t t = 0; t + 2 < B.corners.size(); t += 3) {
        const Corner &a = B.corners[t], &b = B.corners[t+1], &c = B.corners[t+2];
        ++tris;
        if (a.nx == b.nx && a.ny == b.ny && a.nz == b.nz &&
            a.nx == c.nx && a.ny == c.ny && a.nz == c.nz)
            ++flat;
    }
    return tris != 0 && flat * 10 >= tris * 9;
}

inline void orientTriangles(TriBucket &B, bool untrustedNormals = false)
{
    // Coherent, connectivity-based orientation. The old per-triangle test
    // (flip when cross . normal > 0) left sporadic wrong-facing triangles
    // wherever transferred or smoothed normals disagreed with the raw
    // geometric normal (thin features, silhouettes) - in game those are
    // backface-culled and read as shredded tri-strip holes. Instead: drop
    // degenerate triangles, flood-fill one consistent winding across shared
    // edges, then flip whole components by majority vote of the game
    // convention cross(B-A, C-A) . normal < 0.
    // corrupted exports can carry NaN/inf positions; the weld keys below
    // lround() them (UB) and one bad vertex smears a giant spike across the
    // section. Drop any triangle touching a non-finite position up front.
    {
        size_t bad = 0;
        std::vector<Corner> kept;
        kept.reserve(B.corners.size());
        for (size_t t = 0; t + 2 < B.corners.size(); t += 3) {
            bool ok = true;
            for (int k = 0; k < 3 && ok; ++k) {
                const Corner &c = B.corners[t + k];
                ok = std::isfinite(c.px) && std::isfinite(c.py) && std::isfinite(c.pz);
            }
            if (!ok) { ++bad; continue; }
            kept.push_back(B.corners[t]);
            kept.push_back(B.corners[t + 1]);
            kept.push_back(B.corners[t + 2]);
        }
        if (bad) {
            logf("[modmesh]   dropped %u triangle(s) with non-finite positions",
                 unsigned(bad));
            B.corners = std::move(kept);
        }
    }

    const size_t nc = B.corners.size();
    const size_t nt = nc / 3;
    if (!nt) return;

    struct K { int32_t x, y, z;
        bool operator==(const K &o) const { return x==o.x && y==o.y && z==o.z; } };
    struct KH { size_t operator()(const K &k) const {
        return (size_t(uint32_t(k.x)) * 73856093u)
             ^ (size_t(uint32_t(k.y)) * 19349663u)
             ^ (size_t(uint32_t(k.z)) * 83492791u); } };
    auto keyOf = [](const Corner &c) {
        return K{ int32_t(std::lround(double(c.px) * 8192)),
                  int32_t(std::lround(double(c.py) * 8192)),
                  int32_t(std::lround(double(c.pz) * 8192)) };
    };

    std::unordered_map<K, int32_t, KH> vid;
    vid.reserve(nc);
    std::vector<int32_t> tv;             // 3 welded vertex ids per kept tri
    std::vector<size_t>  tsrc;           // kept tri -> original tri
    std::unordered_set<uint64_t> seenTri;
    tv.reserve(nc); tsrc.reserve(nt); seenTri.reserve(nt);
    for (size_t t = 0; t < nt; ++t) {
        int32_t v[3];
        for (int k = 0; k < 3; ++k) {
            K key = keyOf(B.corners[t*3 + k]);
            auto it = vid.find(key);
            if (it == vid.end()) it = vid.emplace(key, int32_t(vid.size())).first;
            v[k] = it->second;
        }
        if (v[0] == v[1] || v[1] == v[2] || v[0] == v[2])
            continue;                    // degenerate after welding: drop
        // strip unrolling leaves coincident duplicate triangles (often with
        // reversed winding); keep only the first occurrence, or the coherent
        // pass would legitimately preserve the pair's mutual opposition and
        // one of the two would always be backface-culled
        {
            int32_t so[3] = { v[0], v[1], v[2] };
            if (so[0] > so[1]) std::swap(so[0], so[1]);
            if (so[1] > so[2]) std::swap(so[1], so[2]);
            if (so[0] > so[1]) std::swap(so[0], so[1]);
            uint64_t tk = (uint64_t(uint32_t(so[0])) * 1000003u
                        ^ uint64_t(uint32_t(so[1]))) * 1000003u
                        ^ uint64_t(uint32_t(so[2]));
            if (!seenTri.insert(tk).second) continue;
        }
        tv.push_back(v[0]); tv.push_back(v[1]); tv.push_back(v[2]);
        tsrc.push_back(t);
    }
    const size_t n = tsrc.size();
    if (!n) { B.corners.clear(); return; }

    struct ERef { int32_t tri = -1; bool fwd = false; };
    struct EK { int32_t a, b;
        bool operator==(const EK &o) const { return a==o.a && b==o.b; } };
    struct EKH { size_t operator()(const EK &e) const {
        return size_t(uint32_t(e.a)) * 2654435761u
             ^ (size_t(uint32_t(e.b)) << 1); } };
    std::unordered_map<EK, std::array<ERef, 2>, EKH> edges;
    edges.reserve(n * 3);
    for (size_t t = 0; t < n; ++t)
        for (int k = 0; k < 3; ++k) {
            int32_t a = tv[t*3 + k], b = tv[t*3 + (k+1)%3];
            EK ek{ a < b ? a : b, a < b ? b : a };
            auto &slot = edges[ek];
            ERef r{ int32_t(t), a < b };
            if (slot[0].tri < 0) slot[0] = r;
            else if (slot[1].tri < 0) slot[1] = r;   // non-manifold extras ignored
        }

    std::vector<int8_t> state(n, int8_t(-1));   // -1 unvisited, 0 keep, 1 flip
    std::vector<int32_t> stack, comp;
    double bucketCx = 0, bucketCy = 0, bucketCz = 0;
    if (untrustedNormals) {
        size_t cn = 0;
        for (size_t t = 0; t < n; ++t) {
            const Corner *c0 = &B.corners[tsrc[t] * 3];
            for (int k = 0; k < 3; ++k) {
                bucketCx += c0[k].px; bucketCy += c0[k].py; bucketCz += c0[k].pz;
                ++cn;
            }
        }
        if (cn) { bucketCx /= double(cn); bucketCy /= double(cn); bucketCz /= double(cn); }
    }
    for (size_t seed = 0; seed < n; ++seed) {
        if (state[seed] >= 0) continue;
        state[seed] = 0;
        stack.assign(1, int32_t(seed));
        comp.clear();
        while (!stack.empty()) {
            int32_t t = stack.back(); stack.pop_back();
            comp.push_back(t);
            for (int k = 0; k < 3; ++k) {
                int32_t a = tv[t*3 + k], b = tv[t*3 + (k+1)%3];
                EK ek{ a < b ? a : b, a < b ? b : a };
                auto it = edges.find(ek);
                if (it == edges.end()) continue;
                const bool fwd = a < b;
                for (const ERef &r : it->second) {
                    if (r.tri < 0 || r.tri == t) continue;
                    // a consistent orientation traverses a shared edge in
                    // OPPOSITE directions; same direction => relative flip
                    int8_t want = int8_t(state[t] ^ (r.fwd == fwd ? 1 : 0));
                    if (state[r.tri] < 0) {
                        state[r.tri] = want;
                        stack.push_back(r.tri);
                    }
                }
            }
        }
        double score = 0;
        if (untrustedNormals) {
            // decide the component's global flip geometrically: the game
            // normal is -cross, and it should point AWAY from the bucket
            // centroid on the (mostly convex) character shell patches. The
            // BUCKET centroid, not the component's own: tiny sliver
            // components lie flat across their own centroid and would flip
            // a coin, while the section body surrounds the bucket centre.
            const double cx = bucketCx, cy = bucketCy, cz = bucketCz;
            for (int32_t t : comp) {
                const Corner *c0 = &B.corners[tsrc[size_t(t)] * 3];
                const Corner *a = c0, *bC = c0 + 1, *cC = c0 + 2;
                if (state[t]) { const Corner *tmp = bC; bC = cC; cC = tmp; }
                const double e1[3] = { bC->px - a->px, bC->py - a->py, bC->pz - a->pz };
                const double e2[3] = { cC->px - a->px, cC->py - a->py, cC->pz - a->pz };
                const double g[3]  = { e1[1]*e2[2] - e1[2]*e2[1],
                                       e1[2]*e2[0] - e1[0]*e2[2],
                                       e1[0]*e2[1] - e1[1]*e2[0] };
                const double tx = (a->px + bC->px + cC->px) / 3.0 - cx;
                const double ty = (a->py + bC->py + cC->py) / 3.0 - cy;
                const double tz = (a->pz + bC->pz + cC->pz) / 3.0 - cz;
                // want -g outward  <=>  g . (triC - compC) < 0
                score += (g[0]*tx + g[1]*ty + g[2]*tz < 0) ? 1.0 : -1.0;
            }
        } else {
            for (int32_t t : comp) {
                const Corner *c0 = &B.corners[tsrc[size_t(t)] * 3];
                const Corner *a = c0, *bC = c0 + 1, *cC = c0 + 2;
                if (state[t]) { const Corner *tmp = bC; bC = cC; cC = tmp; }
                const double e1[3] = { bC->px - a->px, bC->py - a->py, bC->pz - a->pz };
                const double e2[3] = { cC->px - a->px, cC->py - a->py, cC->pz - a->pz };
                const double g[3]  = { e1[1]*e2[2] - e1[2]*e2[1],
                                       e1[2]*e2[0] - e1[0]*e2[2],
                                       e1[0]*e2[1] - e1[1]*e2[0] };
                const double nn[3] = { double(a->nx) + bC->nx + cC->nx,
                                       double(a->ny) + bC->ny + cC->ny,
                                       double(a->nz) + bC->nz + cC->nz };
                score += (g[0]*nn[0] + g[1]*nn[1] + g[2]*nn[2] < 0) ? 1.0 : -1.0;
            }
        }
        if (score < 0)
            for (int32_t t : comp) state[t] = int8_t(state[t] ^ 1);
    }

    std::vector<Corner> outC;
    outC.reserve(n * 3);
    for (size_t t = 0; t < n; ++t) {
        const Corner *c0 = &B.corners[tsrc[t] * 3];
        outC.push_back(c0[0]);
        if (state[t]) { outC.push_back(c0[2]); outC.push_back(c0[1]); }
        else          { outC.push_back(c0[1]); outC.push_back(c0[2]); }
    }
    B.corners = std::move(outC);

    if (untrustedNormals) {
        // the winding is now authoritative: rebuild smooth normals from it
        // (area-weighted -cross per welded position) and forget the
        // alternating flat normals entirely
        std::unordered_map<K, std::array<double, 3>, KH> acc;
        acc.reserve(B.corners.size() / 3);
        for (size_t t = 0; t + 2 < B.corners.size(); t += 3) {
            const Corner &a = B.corners[t], &b = B.corners[t+1], &c = B.corners[t+2];
            const double e1[3] = { double(b.px)-a.px, double(b.py)-a.py, double(b.pz)-a.pz };
            const double e2[3] = { double(c.px)-a.px, double(c.py)-a.py, double(c.pz)-a.pz };
            const double g[3]  = { e1[1]*e2[2]-e1[2]*e2[1],
                                   e1[2]*e2[0]-e1[0]*e2[2],
                                   e1[0]*e2[1]-e1[1]*e2[0] };
            for (const Corner *cc : { &a, &b, &c }) {
                auto &v = acc[keyOf(*cc)];
                v[0] -= g[0]; v[1] -= g[1]; v[2] -= g[2];
            }
        }
        for (auto &c : B.corners) {
            auto it = acc.find(keyOf(c));
            if (it == acc.end()) continue;
            const double l = std::sqrt(it->second[0]*it->second[0]
                                     + it->second[1]*it->second[1]
                                     + it->second[2]*it->second[2]);
            if (l < 1e-20) continue;
            c.nx = float(it->second[0] / l);
            c.ny = float(it->second[1] / l);
            c.nz = float(it->second[2] / l);
        }
    } else if (!B.corners.empty()) {
        // Final facing check for the TRUSTED branch. Source normals that are
        // coherently INVERTED (a whole shell pointing inward - e.g. an export
        // whose mirrored transform was baked into the normals) sail through
        // the vote above, because winding and normals flipped together. The
        // body then renders inside-out: backface-culled from outside, visible
        // with the camera inside it, toon ramp sampled at the wrong end. So
        // re-check geometrically - the game normal (-cross) of a character
        // shell must point AWAY from the bucket centroid - and flip the whole
        // bucket when the clear majority faces inward. Genuinely mixed or
        // open geometry scores near zero and is left alone.
        double cx = 0, cy = 0, cz = 0;
        for (const Corner &c : B.corners) { cx += c.px; cy += c.py; cz += c.pz; }
        cx /= double(B.corners.size());
        cy /= double(B.corners.size());
        cz /= double(B.corners.size());
        double score = 0;
        for (size_t t = 0; t + 2 < B.corners.size(); t += 3) {
            const Corner &a = B.corners[t], &b = B.corners[t+1], &c = B.corners[t+2];
            const double e1[3] = { double(b.px)-a.px, double(b.py)-a.py, double(b.pz)-a.pz };
            const double e2[3] = { double(c.px)-a.px, double(c.py)-a.py, double(c.pz)-a.pz };
            const double g[3]  = { e1[1]*e2[2]-e1[2]*e2[1],
                                   e1[2]*e2[0]-e1[0]*e2[2],
                                   e1[0]*e2[1]-e1[1]*e2[0] };
            const double tx = (double(a.px) + b.px + c.px) / 3.0 - cx;
            const double ty = (double(a.py) + b.py + c.py) / 3.0 - cy;
            const double tz = (double(a.pz) + b.pz + c.pz) / 3.0 - cz;
            score += (g[0]*tx + g[1]*ty + g[2]*tz < 0) ? 1.0 : -1.0;
        }
        if (score < -0.25 * double(B.corners.size() / 3)) {
            for (size_t t = 0; t + 2 < B.corners.size(); t += 3)
                std::swap(B.corners[t+1], B.corners[t+2]);
            for (Corner &c : B.corners) { c.nx = -c.nx; c.ny = -c.ny; c.nz = -c.nz; }
            logf("[modmesh]   shell faced inward after trusted orientation - "
                 "flipped %u triangles", unsigned(B.corners.size() / 3));
        }
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

// sidecar section lists: "9, 11-14,16" -> {9,11,12,13,14,16}
// Material-name fragments that mean "this piece is white on purpose". Only
// ever consulted for a material with NO diffuse texture bound, which is the
// one state where drawing white reproduces retail and the blank-repair
// fallbacks (numbered-variant salvage, sibling-donor borrow) are guessing.
// On a character that means the eye lenses and the chest emblem: borrowing
// the body sheet for those is never the right answer.
inline const std::vector<std::string> &defaultWhiteNames()
{
    static const std::vector<std::string> v = {
        "EYE", "LENS", "LOGO", "EMBLEM", "SYMBOL", "INSIGNIA", "CREST",
        "SPIDER", "GLASS"
    };
    return v;
}

inline void parseSectionList(const std::string &v, std::set<int> &out)
{
    size_t pos = 0;
    while (pos <= v.size()) {
        const size_t comma = v.find(',', pos);
        std::string tok = v.substr(pos, comma == std::string::npos
                                        ? std::string::npos : comma - pos);
        pos = comma == std::string::npos ? v.size() + 1 : comma + 1;
        size_t b = 0, e = tok.size();
        while (b < e && std::isspace(uint8_t(tok[b]))) ++b;
        while (e > b && std::isspace(uint8_t(tok[e - 1]))) --e;
        if (b >= e) continue;
        tok = tok.substr(b, e - b);
        if (!std::isdigit(uint8_t(tok[0]))) continue;      // atoi("x") == 0
        int lo, hi;
        const size_t dash = tok.find('-', 1);
        if (dash != std::string::npos) {
            if (dash + 1 >= tok.size()
                || !std::isdigit(uint8_t(tok[dash + 1]))) continue;
            lo = std::atoi(tok.substr(0, dash).c_str());
            hi = std::atoi(tok.substr(dash + 1).c_str());
        } else {
            lo = hi = std::atoi(tok.c_str());
        }
        if (lo < 0 || hi < lo || hi >= 4096) continue;
        for (int s = lo; s <= hi; ++s) out.insert(s);
    }
}

// ---------------------------------------------------------------------------
// collapsed UV layers
//
// An exporter can hand us a UV layer that carries NO information: one entry
// per vertex, all of them the same pair. The venom_eddie export does exactly
// that on its first ten pieces (sections 0-9 hold a single UV each, 10-17 hold
// real ones), so those pieces draw as ONE FLAT TEXEL - the body sampling a
// light purple pixel of VENOM_EDDIE_03, the whole forearm/hand/claw run
// sampling a pale grey pixel of VENOM_MOUTH, which is the pale wedge that
// shows up over the hand. It also breaks the round-trip test below, since a
// collapsed UV never matches the vanilla one: a piece that is geometrically
// identical gets imported anyway and drags its flat texel in with it.
//
// countDistinctUV counts distinct pairs up to `cap`, quantized so exporter
// float dust does not read as variety.
inline size_t countDistinctUV(const std::vector<float> &verts, size_t cap = 4)
{
    std::set<std::pair<int64_t, int64_t>> seen;
    for (size_t v = 0; v + 16 <= verts.size(); v += 16) {
        const float *r = verts.data() + v;
        if (!std::isfinite(r[6]) || !std::isfinite(r[7])) continue;
        seen.emplace(int64_t(std::llround(double(r[6]) * 4096.0)),
                     int64_t(std::llround(double(r[7]) * 4096.0)));
        if (seen.size() >= cap) break;
    }
    return seen.size();
}

// Materializes ANY original section - skinned 64-byte row or arbitrary static
// vertex - into the canonical 16-float row, so every helper below stays
// layout-blind. Missing channels come out as zeros; the blend lanes get the
// rigid bind (slot 0, weight 1) a static vertex effectively has. Callers use
// it only for RIGID sections: a skinned section already IS this layout.
inline std::vector<float> canonicalRows(const OrigSectionView &ov)
{
    std::vector<float> out;
    if (!ov.replaceable()) return out;
    out.assign(size_t(ov.nverts) * 16, 0.f);
    for (uint32_t i = 0; i < ov.nverts; ++i) {
        float *r = out.data() + size_t(i) * 16;
        ov.getPos(i, r);
        ov.getNrm(i, r + 3);
        ov.getUV(i, r + 6);
        r[8]  = 0.f; r[9] = r[10] = r[11] = -1.f;
        r[12] = 1.f;
    }
    return out;
}

// Copies the UV of the nearest ORIGINAL vertex onto every imported vertex.
// Used when the import's own UV layer is collapsed: the vanilla UVs are then
// the only ones that mean anything, and an unedited piece gets its exact pair
// back (the nearest original vertex IS that vertex), so the round-trip test
// that follows still sees a match. Returns how many vertices were patched;
// maxDist reports how far the search had to reach.
inline size_t restoreUVsFromOriginal(std::vector<float> &verts,
                                     const OrigSectionView &ov,
                                     double &maxDist)
{
    maxDist = 0.0;
    if (!ov.replaceable()) return 0;
    // a static section carries its UVs somewhere else in the vertex (or not
    // at all): flatten once, then the body below is layout-blind
    std::vector<float> canon;
    if (!ov.skinnedRow()) {
        if (ov.uvOff < 0) return 0;                // nothing to restore from
        canon = canonicalRows(ov);
    }
    const float *base = ov.skinnedRow() ? ov.verts : canon.data();

    float mn[3] { base[0], base[1], base[2] };
    float mx[3] { mn[0], mn[1], mn[2] };
    for (uint32_t i = 1; i < ov.nverts; ++i) {
        const float *r = base + size_t(i) * 16;
        for (int c = 0; c < 3; ++c) {
            if (r[c] < mn[c]) mn[c] = r[c];
            if (r[c] > mx[c]) mx[c] = r[c];
        }
    }
    double diag = std::sqrt(double(mx[0] - mn[0]) * double(mx[0] - mn[0])
                          + double(mx[1] - mn[1]) * double(mx[1] - mn[1])
                          + double(mx[2] - mn[2]) * double(mx[2] - mn[2]));
    if (!(diag > 0.0)) diag = 1.0;
    const double cell = std::max(diag / 24.0, 1e-4);

    auto key = [](int64_t x, int64_t y, int64_t z) -> uint64_t {
        uint64_t h = uint64_t(x) * 0x9E3779B185EBCA87ull;
        h ^= uint64_t(y) * 0xC2B2AE3D27D4EB4Full;
        h ^= uint64_t(z) * 0x165667B19E3779F9ull;
        return h;
    };
    auto q = [&](float v) { return int64_t(std::floor(double(v) / cell)); };

    std::unordered_multimap<uint64_t, uint32_t> grid;
    grid.reserve(size_t(ov.nverts) * 2);
    for (uint32_t i = 0; i < ov.nverts; ++i) {
        const float *r = base + size_t(i) * 16;
        grid.emplace(key(q(r[0]), q(r[1]), q(r[2])), i);
    }

    size_t patched = 0;
    for (size_t v = 0; v + 16 <= verts.size(); v += 16) {
        float *w = verts.data() + v;
        const int64_t cx = q(w[0]), cy = q(w[1]), cz = q(w[2]);
        int    best   = -1;
        double bestD2 = 1e30;
        for (int ring = 0; ring <= 24 && best < 0; ++ring) {
            for (int dx = -ring; dx <= ring; ++dx)
            for (int dy = -ring; dy <= ring; ++dy)
            for (int dz = -ring; dz <= ring; ++dz) {
                if (ring > 0 && std::abs(dx) != ring       // shell only
                    && std::abs(dy) != ring && std::abs(dz) != ring)
                    continue;
                auto range = grid.equal_range(key(cx + dx, cy + dy, cz + dz));
                for (auto it = range.first; it != range.second; ++it) {
                    const float *r = base + size_t(it->second) * 16;
                    const double ex = double(w[0]) - double(r[0]),
                                 ey = double(w[1]) - double(r[1]),
                                 ez = double(w[2]) - double(r[2]);
                    const double d2 = ex * ex + ey * ey + ez * ez;
                    if (d2 < bestD2) { bestD2 = d2; best = int(it->second); }
                }
            }
        }
        if (best < 0) continue;                    // empty grid: leave as is
        const float *r = base + size_t(best) * 16;
        w[6] = r[6];
        w[7] = r[7];
        ++patched;
        if (bestD2 > maxDist * maxDist) maxDist = std::sqrt(bestD2);
    }
    return patched;
}

// ---------------------------------------------------------------------------
// round-trip identity: does a freshly built section carry EXACTLY the
// original section's geometry?
//
// A per-section FBX exported from the game's own mesh and re-imported
// unedited reproduces the original vertices bit-for-bit minus float dust
// (the Blender -90deg/x100 export transform is not exactly representable).
// Replacing such a section gains nothing and costs the retail MORPH
// pipeline: morph playback (the venom_eddie reveal cocoon, mouth visemes,
// spidey/MJ facial animation) writes per-vertex deltas addressed by VANILLA
// vertex indices, which the replacement's dead zone deliberately swallows.
// On venom_eddie the reveal geometry - stored in the file at its fully OPEN
// pose and folded away by morphs at runtime - then freezes open: a permanent
// untextured white cocoon over the shoulders with eddie's head inside.
// Detecting the identity and keeping the vanilla buffers keeps those morphs
// alive, and the section renders exactly like retail.
//
// The test is symmetric nearest-neighbour distance on POSITIONS over a
// uniform grid (vertex counts may differ: welding on either side of the
// round trip merges different corner sets) plus a UV check on the import
// side: a corner is accepted when ANY original vertex within `eps` also
// matches its UV, so seam vertices sharing a position with different UVs do
// not false-negative. Weights are NOT compared - a weight-only Blender edit
// needs sidecar roundtrip=0 to force the import.
inline bool sectionMatchesOriginal(const std::vector<float> &verts,
                                   const OrigSectionView &ov,
                                   double eps, double &maxDelta)
{
    maxDelta = 0.0;
    if (eps <= 0.0) return false;
    const size_t nNew = verts.size() / 16;
    if (nNew == 0 || !ov.replaceable())
        return false;
    std::vector<float> canon;
    if (!ov.skinnedRow()) canon = canonicalRows(ov);
    const float *origBase = ov.skinnedRow() ? ov.verts : canon.data();
    // a static section without a UV channel can only be compared on positions
    const bool origHasUV = ov.skinnedRow() || ov.uvOff >= 0;
    // a piece that lost or gained most of its vertices is an edit no matter
    // where the survivors sit
    if (nNew > size_t(ov.nverts) * 4 || size_t(ov.nverts) > nNew * 4)
        return false;

    const double cell = std::max(eps, 1e-6);
    auto cellKey = [](int64_t qx, int64_t qy, int64_t qz) -> uint64_t {
        uint64_t h = uint64_t(qx) * 0x9E3779B185EBCA87ull;
        h ^= uint64_t(qy) * 0xC2B2AE3D27D4EB4Full;
        h ^= uint64_t(qz) * 0x165667B19E3779F9ull;
        return h;
    };
    auto quant = [&](float v) {
        return int64_t(std::floor(double(v) / cell));
    };

    struct Ref { float x, y, z, u, v; };
    auto build = [&](const float *base, size_t n,
                     std::unordered_multimap<uint64_t, uint32_t> &grid,
                     std::vector<Ref> &pts) {
        pts.resize(n);
        grid.reserve(n * 2);
        for (size_t i = 0; i < n; ++i) {
            const float *r = base + i * 16;
            pts[i] = { r[0], r[1], r[2], r[6], r[7] };
            grid.emplace(cellKey(quant(r[0]), quant(r[1]), quant(r[2])),
                         uint32_t(i));
        }
    };

    std::unordered_multimap<uint64_t, uint32_t> gOld, gNew;
    std::vector<Ref> pOld, pNew;
    build(origBase,     ov.nverts, gOld, pOld);
    build(verts.data(), nNew,      gNew, pNew);

    const double eps2  = eps * eps;
    const double uvEps = std::max(2e-3, eps);   // quarter-texel at 512px

    double worst2 = 0.0;
    auto scan = [&](const std::vector<Ref> &from,
                    const std::vector<Ref> &to,
                    const std::unordered_multimap<uint64_t, uint32_t> &grid,
                    bool checkUV) -> bool {
        for (const Ref &p : from) {
            const int64_t qx = quant(p.x), qy = quant(p.y), qz = quant(p.z);
            double best = 1e30;
            bool uvOK = !checkUV;
            for (int dx = -1; dx <= 1; ++dx)
            for (int dy = -1; dy <= 1; ++dy)
            for (int dz = -1; dz <= 1; ++dz) {
                auto range = grid.equal_range(cellKey(qx + dx, qy + dy,
                                                      qz + dz));
                for (auto it = range.first; it != range.second; ++it) {
                    const Ref &q = to[it->second];
                    const double ex = double(p.x) - q.x,
                                 ey = double(p.y) - q.y,
                                 ez = double(p.z) - q.z;
                    const double d2 = ex * ex + ey * ey + ez * ez;
                    if (d2 < best) best = d2;
                    if (checkUV && d2 <= eps2
                        && std::fabs(double(p.u) - q.u) <= uvEps
                        && std::fabs(double(p.v) - q.v) <= uvEps)
                        uvOK = true;
                }
            }
            if (best > worst2) worst2 = best;
            if (best > eps2 || !uvOK) {
                maxDelta = std::sqrt(worst2);
                return false;
            }
        }
        return true;
    };

    // import -> original (with UV), then original -> import (position only:
    // an original vertex with no counterpart means geometry was removed)
    const bool same = scan(pNew, pOld, gOld, /*checkUV=*/origHasUV)
                   && scan(pOld, pNew, gNew, /*checkUV=*/false);
    maxDelta = std::sqrt(worst2);
    return same;
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
        // a quoted value is a natural thing to write for a key that takes a
        // file name ( tex15 = "VENOM_MOUTH.png" ); no key wants the quotes
        if (v.size() >= 2
            && ((v.front() == '"'  && v.back() == '"')
             || (v.front() == '\'' && v.back() == '\''))) {
            v.erase(v.size() - 1);
            v.erase(v.begin());
            trim(v);
        }
        for (auto &c : k) c = char(std::tolower(uint8_t(c)));
        if      (k == "scale")    cfg.scale = std::atof(v.c_str());
        else if (k == "yaw")      cfg.yaw = std::atof(v.c_str());
        else if (k == "offset_x") cfg.offset.x = std::atof(v.c_str());
        else if (k == "offset_y") cfg.offset.y = std::atof(v.c_str());
        else if (k == "offset_z") cfg.offset.z = std::atof(v.c_str());
        else if (k == "fit")      cfg.fit = std::atoi(v.c_str()) != 0;
        else if (k == "weld")     cfg.weld = std::atoi(v.c_str()) != 0;
        else if (k == "anim" || k == "animations") {
            for (auto &c : v) c = char(std::tolower(uint8_t(c)));
            cfg.anim = !(v == "off" || v == "0" || v == "no");
        }
        else if (k == "bone_match" || k == "bonematch") {
            for (auto &c : v) c = char(std::tolower(uint8_t(c)));
            cfg.boneMatch = !(v == "off" || v == "0" || v == "no" || v == "names");
        }
        else if (k == "texture") {
            for (auto &c : v) c = char(std::tolower(uint8_t(c)));
            if      (v == "keep" || v == "original" || v == "vanilla") cfg.tex = 1;
            else if (v == "mod"  || v == "force")                      cfg.tex = 2;
            else                                                       cfg.tex = 0;
        }
        // autotex=off - disable the automatic blank-section texture coverage
        else if (k == "autotex" || k == "auto_tex" || k == "auto_texture") {
            for (auto &c : v) c = char(std::tolower(uint8_t(c)));
            cfg.autoTex = !(v == "off" || v == "0" || v == "no");
        }
        // tex_default=STEM - fallback stem for sections with no candidate
        else if (k == "tex_default" || k == "texdefault"
              || k == "tex_fallback" || k == "texfallback") {
            for (auto &c : v) c = char(std::toupper(uint8_t(c)));
            auto slash = v.find_last_of("/\\");
            if (slash != std::string::npos) v.erase(0, slash + 1);
            auto dot = v.rfind('.');
            if (dot != std::string::npos && dot > 0) v.erase(dot);
            cfg.texDefault = v;
        }
        // tex<N>=STEM - per-section pin. Checked AFTER "texture"/"tex_default"
        // so those keys keep their meaning; the digit is what distinguishes it.
        else if (k.size() > 3 && k.compare(0, 3, "tex") == 0
                 && std::isdigit(uint8_t(k[3]))) {
            const int si = std::atoi(k.c_str() + 3);
            for (auto &c : v) c = char(std::toupper(uint8_t(c)));
            // strip a path/extension if the user pasted a file name
            auto slash = v.find_last_of("/\\");
            if (slash != std::string::npos) v.erase(0, slash + 1);
            auto dot = v.rfind('.');
            if (dot != std::string::npos && dot > 0) v.erase(dot);
            // tex<N>=WHITE is the pin spelling of white=N. Neither the engine
            // texture directory nor any mod folder is searched for a sheet
            // called WHITE: the engine's own 1x1 white texture is bound.
            if (si >= 0 && (v == "WHITE" || v == "NGLWHITE"))
                cfg.whiteSections.insert(si);
            else if (si >= 0 && !v.empty()) cfg.texPin[si] = v;
        }
        else if (k == "skin") {
            for (auto &c : v) c = char(std::tolower(uint8_t(c)));
            if      (v == "clusters") cfg.skin = 1;
            else if (v == "transfer") cfg.skin = 2;
            else if (v == "rigid")    cfg.skin = 3;
            else if (v == "rigidparts" || v == "rigid_parts"
                  || v == "parts"      || v == "articulated") cfg.skin = 4;
            else                      cfg.skin = 0;
        }
        else if (k == "keep" || k == "hide") {
            detail::parseSectionList(v, k == "keep" ? cfg.keepSections
                                                    : cfg.hideSections);
        }
        // white=1,7 - draw those sections pure white and shield them from
        // every blank-repair heuristic. For the pieces that are meant to be
        // white (black-suit eye lenses, chest spider) this is the answer, not
        // a tex<N>= pin at some sheet that happens to have a white corner.
        else if (k == "white" || k == "whiteout" || k == "white_sections") {
            // keywords and section numbers mix freely: "white=blank,7"
            std::string numbers;
            size_t pos = 0;
            while (pos <= v.size()) {
                const size_t comma = v.find(',', pos);
                std::string tok = v.substr(pos, comma == std::string::npos
                                                ? std::string::npos
                                                : comma - pos);
                pos = comma == std::string::npos ? v.size() + 1 : comma + 1;
                trim(tok);
                if (tok.empty()) continue;
                std::string low = tok;
                for (auto &c : low) c = char(std::tolower(uint8_t(c)));
                if (low == "auto" || low == "on" || low == "names") {
                    cfg.whiteAuto = true;
                } else if (low == "blank" || low == "untextured"
                        || low == "all") {
                    cfg.whiteBlank = true;
                    cfg.whiteAuto  = true;
                } else if (low == "off" || low == "no" || low == "none"
                        || low == "0") {
                    cfg.whiteAuto  = false;
                    cfg.whiteBlank = false;
                } else {
                    if (!numbers.empty()) numbers += ',';
                    numbers += tok;
                }
            }
            if (!numbers.empty())
                detail::parseSectionList(numbers, cfg.whiteSections);
        }
        // white_names=EYE,LOGO - override the built-in white=auto keywords
        else if (k == "white_names" || k == "whitenames") {
            size_t pos = 0;
            while (pos <= v.size()) {
                const size_t comma = v.find(',', pos);
                std::string tok = v.substr(pos, comma == std::string::npos
                                                ? std::string::npos
                                                : comma - pos);
                pos = comma == std::string::npos ? v.size() + 1 : comma + 1;
                trim(tok);
                for (auto &c : tok) c = char(std::toupper(uint8_t(c)));
                if (!tok.empty()) cfg.whiteNames.push_back(tok);
            }
        }
        // blank=keep|import|hide - untextured named pieces that fail the
        // round-trip test (the morph-held reveal shells). keep is the default:
        // the vanilla section stays, morphs keep folding the shape away.
        else if (k == "blank" || k == "blankgeo" || k == "blank_geo"
              || k == "blank_pieces") {
            for (auto &c : v) c = char(std::tolower(uint8_t(c)));
            if      (v == "import" || v == "replace" || v == "force")
                cfg.blankGeo = 1;
            else if (v == "hide"   || v == "off")
                cfg.blankGeo = 2;
            else
                cfg.blankGeo = 0;       // keep / vanilla / on / anything else
        }
        else if (k == "layout") {
            for (auto &c : v) c = char(std::tolower(uint8_t(c)));
            size_t pos = 0;
            while (pos < v.size()) {
                size_t comma = v.find(',', pos);
                std::string tok = v.substr(pos, comma == std::string::npos
                                                ? std::string::npos
                                                : comma - pos);
                pos = (comma == std::string::npos) ? v.size() : comma + 1;
                auto colon = tok.find(':');
                if (colon == std::string::npos) continue;
                std::string ck = tok.substr(0, colon);
                const int cv = std::atoi(tok.c_str() + colon + 1);
                trim(ck);
                if      (ck == "stride") cfg.layStride = cv;
                else if (ck == "pos")    cfg.layPos = cv;
                else if (ck == "nrm" || ck == "normal") cfg.layNrm = cv;
                else if (ck == "uv"  || ck == "uv0")    cfg.layUv  = cv;
                else if (ck == "col" || ck == "color" || ck == "colour")
                    cfg.layCol = cv;
                else continue;
                cfg.layoutSet = true;
            }
        }
        else if (k == "roundtrip") {
            for (auto &c : v) c = char(std::tolower(uint8_t(c)));
            if (v == "off" || v == "no")
                cfg.roundtripEps = 0.0;
            else {
                const double e = std::atof(v.c_str());
                cfg.roundtripEps = e > 0.0 ? e : 0.0;
            }
        }
    }
    logf("[modmesh] sidecar %s.ini: scale=%.3f yaw=%.1f fit=%d skin=%d weld=%d "
         "tex=%d anim=%d roundtrip=%.4g keep=%u hide=%u white=%u%s pin=%u "
         "default=\"%s\" autotex=%d blank=%s",
         meshPath.c_str(), cfg.scale, cfg.yaw, int(cfg.fit), cfg.skin, int(cfg.weld),
         cfg.tex, int(cfg.anim), cfg.roundtripEps,
         unsigned(cfg.keepSections.size()), unsigned(cfg.hideSections.size()),
         unsigned(cfg.whiteSections.size()),
         cfg.whiteBlank ? "+blank" : cfg.whiteAuto ? "+auto" : "",
         unsigned(cfg.texPin.size()), cfg.texDefault.c_str(),
         int(cfg.autoTex),
         cfg.blankGeo == 1 ? "import" : cfg.blankGeo == 2 ? "hide" : "keep");
    for (const auto &p : cfg.texPin)
        logf("[modmesh] sidecar tex%d=%s", p.first, p.second.c_str());
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
            if (fbxbin::parse(bytes, size, root, version, sc->cfg.anim)) {
                detail::buildScene(root, *sc);
                // a file with takes but no mesh is a pure ANIMATION mod
                ok = !sc->meshModelOrder.empty() || !sc->anims.empty();
                logf("[modmesh] \"%s\": binary FBX %u, %u mesh model(s), %u geometrie(s), "
                     "%u animation take(s)",
                     path.c_str(), version, unsigned(sc->meshModelOrder.size()),
                     unsigned(sc->geoms.size()), unsigned(sc->anims.size()));
            } else {
                logf("[modmesh] \"%s\": binary FBX parse FAILED", path.c_str());
            }
        } else {
            if (fbxtxt::parse(reinterpret_cast<const char *>(bytes), size, root)) {
                detail::buildScene(root, *sc);
                ok = !sc->meshModelOrder.empty() || !sc->anims.empty();
                logf("[modmesh] \"%s\": ascii FBX, %u mesh model(s), "
                     "%u animation take(s)",
                     path.c_str(), unsigned(sc->meshModelOrder.size()),
                     unsigned(sc->anims.size()));
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
buildSectionsForMeshRaw(Scene &sc,
                     const std::string &meshNameIn,
                     const std::vector<OrigSectionView> &origs,
                     const OrigMeshRef &origRef = {})
{
    using namespace detail;

    std::vector<std::optional<BuiltSection>> out(origs.size());
    if (origs.empty()) return {};

    std::string meshName = meshNameIn;
    for (auto &c : meshName) c = char(std::tolower(uint8_t(c)));

    // Does any section still carry the skinned 64-byte row? Skinning-specific
    // diagnostics ("export the FBX with skin clusters") are noise on a mesh
    // that has no skeleton to begin with.
    bool anySkinnedOrig = false;
    {
        bool anyReplaceable = false, anyRigid = false;
        for (const auto &ov : origs) {
            if (!ov.replaceable()) continue;
            anyReplaceable = true;
            if (ov.skinnedRow()) anySkinnedOrig = true; else anyRigid = true;
        }
        if (!anyReplaceable) {
            logf("[modmesh] %s: no replaceable section (no usable vertex "
                 "stream; stride %u) - mod skipped",
                 meshName.c_str(), origs.empty() ? 0u : origs[0].strideBytes);
            return {};
        }
        if (anyRigid)
            logf("[modmesh] %s: %s mesh - %u section(s) are STATIC RIGID "
                 "(no skinning, the import is written back in the section's "
                 "own vertex format)",
                 meshName.c_str(), anySkinnedOrig ? "mixed" : "static",
                 unsigned(std::count_if(origs.begin(), origs.end(),
                          [](const OrigSectionView &o) { return o.rigidRow(); })));
    }

    // ---------------- shared helpers -----------------------------------
    auto donorForSection = [&](size_t si, bool skeletonSpace) {
        DonorGrid grid;
        auto push = [&](const OrigSectionView &ov) {
            if (!ov.skinnedRow()) return;    // static sections carry no weights
            for (uint32_t v = 0; v < ov.nverts; ++v) {
                const float *row = ov.verts + size_t(v) * 16;

                // A real skinned vertex always carries at least one positive
                // weight. Rows that do not are padding - most importantly the
                // morph dead zone of a section that already holds a
                // replacement - and as donors they poison the transfer: every
                // corner near them falls into the degenerate branch and comes
                // out rigid on slot 0, which collapses the whole body onto the
                // root bone.
                bool anyW = false;
                for (int k = 0; k < 4; ++k)
                    if (row[12 + k] > 0.f) { anyW = true; break; }
                if (!anyW) continue;

                Donor d;
                d.p[0] = row[0]; d.p[1] = row[1]; d.p[2] = row[2];
                d.n[0] = row[3]; d.n[1] = row[4]; d.n[2] = row[5];
                bool anyInf = false;
                for (int k = 0; k < 4; ++k) {
                    const float slot = row[8 + k];
                    const float w    = row[12 + k];
                    if (!skeletonSpace) {
                        d.bi[k] = slot;              // palette-slot space
                        d.bw[k] = w;
                        if (slot >= 0 && w > 0.f) anyInf = true;
                        continue;
                    }
                    // Skeleton space: an influence that cannot be mapped
                    // through this section's palette is DROPPED. It used to
                    // fall back to the raw slot, which mixed slot indices into
                    // a skeleton-index donor cloud - vertices sampling a
                    // palette-less section got pulled onto whatever bone
                    // happened to sit at index 0/1/2, i.e. the spikes.
                    if (slot >= 0 && w > 0.f && ov.palette
                        && int(slot) < ov.nbones)
                    {
                        d.bi[k] = float(ov.palette[int(slot)]);
                        d.bw[k] = w;
                        anyInf  = true;
                    } else {
                        d.bi[k] = -1.f;
                        d.bw[k] = 0.f;
                    }
                }
                if (!anyInf) continue;               // nothing usable
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

        const Donor *nb[4];
        float d2[4];
        for (auto &c : B.corners) {
            const float p[3] = { c.px, c.py, c.pz };
            const int n = grid.nearestK(p, 4, nb, d2);
            if (!n) continue;
            if (d2[0] <= 1e-8f) {
                // bit-exact round trip: copy the authored skin AND normal
                for (int k = 0; k < 4; ++k) { c.bi[k] = nb[0]->bi[k]; c.bw[k] = nb[0]->bw[k]; }
                c.nx = nb[0]->n[0]; c.ny = nb[0]->n[1]; c.nz = nb[0]->n[2];
                ++nrmHits;
                continue;
            }
            // foreign vertex: inverse-distance blend of the neighbours'
            // weights in bone space, keep the 4 heaviest. Removes the hard
            // seams a single-nearest copy leaves across bone boundaries.
            struct BW { float b, w; };
            BW acc[16]; int na = 0;
            for (int i = 0; i < n; ++i) {
                const float wi = 1.f / (d2[i] + 1e-10f);
                for (int k = 0; k < 4; ++k) {
                    const float b = nb[i]->bi[k], w = nb[i]->bw[k];
                    if (b < 0 || w <= 0) continue;
                    int j = 0;
                    for (; j < na; ++j) if (acc[j].b == b) { acc[j].w += wi * w; break; }
                    if (j == na && na < 16) acc[na++] = { b, wi * w };
                }
            }
            std::sort(acc, acc + na, [](const BW &a, const BW &b) { return a.w > b.w; });
            float s = 0;
            const int keep = na < 4 ? na : 4;
            for (int k = 0; k < keep; ++k) s += acc[k].w;
            if (s <= 0) {                          // degenerate: nearest as-is
                for (int k = 0; k < 4; ++k) { c.bi[k] = nb[0]->bi[k]; c.bw[k] = nb[0]->bw[k]; }
                continue;
            }
            for (int k = 0; k < 4; ++k) {
                if (k < keep) { c.bi[k] = acc[k].b; c.bw[k] = acc[k].w / s; }
                else          { c.bi[k] = -1;       c.bw[k] = 0; }
            }

        }
        if (nrmHits)
            logf("[modmesh]   normals recovered from original for %u/%u corners",
                 unsigned(nrmHits), unsigned(B.corners.size()));
        return nrmHits;
    };

    // Re-skin ONLY the corners left weightless because every one of their
    // influences sat on a dropped cluster (a bone this mesh does not have -
    // e.g. mouth geometry rigged to jaw/tongue bones applied to a mesh whose
    // array has neither). The replacement weights come from the ORIGINAL
    // section skin via the skeleton-space donor grid, which is exactly how
    // the original mesh drives that same geometry. Corners with any valid
    // weight are left untouched: the authored cluster skin stays intact.
    auto patchWeightlessCorners = [&](TriBucket &B, size_t si) -> size_t {
        std::vector<size_t> needy;
        for (size_t i = 0; i < B.corners.size(); ++i) {
            const Corner &c = B.corners[i];
            if (c.bw[0] + c.bw[1] + c.bw[2] + c.bw[3] <= 1e-4f)
                needy.push_back(i);
        }
        if (needy.empty()) return 0;
        DonorGrid grid = donorForSection(si, /*skeletonSpace=*/true);
        if (grid.donors.empty()) {
            logf("[modmesh]   %u corners lost every influence and no donors "
                 "exist - they will be bound rigidly by the sanitizer",
                 unsigned(needy.size()));
            return 0;
        }
        TriBucket tmp;
        tmp.corners.reserve(needy.size());
        for (size_t i : needy) tmp.corners.push_back(B.corners[i]);
        transferCorners(tmp, grid);
        for (size_t k = 0; k < needy.size(); ++k) {
            Corner &dst = B.corners[needy[k]];
            const Corner &src = tmp.corners[k];
            for (int j = 0; j < 4; ++j) { dst.bi[j] = src.bi[j]; dst.bw[j] = src.bw[j]; }
            dst.nx = src.nx; dst.ny = src.ny; dst.nz = src.nz;
        }
        logf("[modmesh]   %u/%u corners rode only dropped bones - re-skinned "
             "from the original section",
             unsigned(needy.size()), unsigned(B.corners.size()));
        return needy.size();
    };

    // user transform (sidecar scale/yaw/offset) + auto-fit of arbitrary
    // corner sets onto the original mesh height. Shared by every tier. When
    // alignToDonor is set the import is additionally TRANSLATED onto the
    // original bounds even when the height already matches (ratio inside
    // 0.8..1.25): nearest-position weight transfer picks the wrong bones on
    // offset bodies. Tier A/B pass it too since v2: a Blender re-export whose
    // armature roots at the hips writes the whole body 1.5-2 units below the
    // retail feet-at-0 frame with a height ratio of ~1.0, which the auto-fit
    // dead zone used to let through untouched. Genuine round trips still come
    // out bit-exact because every correction is gated on a non-zero delta
    // (|shift| > 1e-4, ratio outside 0.8..1.25).
    //
    // replacedSecs, when given, limits the ORIGINAL reference bounds to the
    // sections the import actually replaces. A per-section partial export
    // (just a head) is measured against the original head, not the whole
    // body, so it neither trips the auto-fit (0.5 vs 3.4 units used to scale
    // the piece by ~7x) nor gets dragged feet-to-feet across the body.
    auto fitCorners = [&](std::vector<TriBucket *> &parts, bool alignToDonor = false,
                          const std::vector<size_t> *replacedSecs = nullptr) {
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
        auto secInReference = [&](size_t oi) {
            if (!replacedSecs) return true;
            for (size_t s : *replacedSecs) if (s == oi) return true;
            return false;
        };
        // A STATIC reference is fitted differently (see below): props are not
        // standing on the ground and can be flatter than they are wide, so
        // neither "feet to feet" nor a height ratio means anything on them.
        bool rigidRef = false, skinnedRef = false;
        for (size_t oi = 0; oi < origs.size(); ++oi) {
            const auto &ov = origs[oi];
            if (!ov.replaceable() || !secInReference(oi)) continue;
            if (ov.skinnedRow()) skinnedRef = true; else rigidRef = true;
            for (uint32_t v = 0; v < ov.nverts; ++v) {
                float p[3];
                ov.getPos(v, p);
                if (!std::isfinite(p[0]) || !std::isfinite(p[1])
                    || !std::isfinite(p[2]))
                    continue;
                omnx = std::min<double>(omnx, p[0]); omxx = std::max<double>(omxx, p[0]);
                omny = std::min<double>(omny, p[1]); omxy = std::max<double>(omxy, p[1]);
                omnz = std::min<double>(omnz, p[2]); omxz = std::max<double>(omxz, p[2]);
                haveOrig = true;
            }
        }
        const bool staticFit = rigidRef && !skinnedRef;
        double newH  = mxy - mny;
        // no section exposed a usable vertex stream at all (a prerelease/beta
        // build whose layout even the sniffer could not read, or a mesh whose
        // CPU-side data is gone): fall back to the mesh's own reference frame
        // so the import is still scaled and placed onto the character instead
        // of rendering at raw exporter size
        if (!haveOrig && origRef.haveBones) {
            const double h = double(origRef.bonesMax[1]) - double(origRef.bonesMin[1]);
            if (h > 1e-4) {
                // joints stop short of the skull top / soles: pad ~6% per side
                omny = origRef.bonesMin[1] - h * 0.06;
                omxy = origRef.bonesMax[1] + h * 0.06;
                omnx = origRef.bonesMin[0]; omxx = origRef.bonesMax[0];
                omnz = origRef.bonesMin[2]; omxz = origRef.bonesMax[2];
                haveOrig = true;
                logf("[modmesh] fit reference: bind-pose skeleton "
                     "(y %.2f..%.2f)", omny, omxy);
            }
        }
        if (!haveOrig && origRef.haveSphere && origRef.sphereRadius > 1e-4f) {
            // for a standing character the sphere diameter tracks the height
            const double r = origRef.sphereRadius;
            omny = origRef.sphereCenter[1] - r * 0.95;
            omxy = origRef.sphereCenter[1] + r * 0.95;
            omnx = origRef.sphereCenter[0] - r; omxx = origRef.sphereCenter[0] + r;
            omnz = origRef.sphereCenter[2] - r; omxz = origRef.sphereCenter[2] + r;
            haveOrig = true;
            logf("[modmesh] fit reference: bounding sphere "
                 "(c %.2f %.2f %.2f, r %.2f)",
                 origRef.sphereCenter[0], origRef.sphereCenter[1],
                 origRef.sphereCenter[2], r);
        }
        double origH = omxy - omny;
        bool scaled = false;

        // -------- static target: diagonal ratio + centre-to-centre ---------
        // A weapon, a pickup or a HUD mesh has no "up" the fit can key on:
        // it is placed by the engine at a bone/attach point, and matching the
        // ORIGINAL bounding box - uniform scale on the diagonal, centres
        // coincident on all three axes - is the only correction that keeps a
        // foreign model where the game expects to draw it. Same dead zone as
        // the character path, so a genuine round trip still moves by ~0.
        if (staticFit) {
            if (sc.cfg.fit && haveOrig) {
                const double nd = std::sqrt((mxx-mnx)*(mxx-mnx) + (mxy-mny)*(mxy-mny)
                                          + (mxz-mnz)*(mxz-mnz));
                const double od = std::sqrt((omxx-omnx)*(omxx-omnx) + (omxy-omny)*(omxy-omny)
                                          + (omxz-omnz)*(omxz-omnz));
                const double cx = (mnx + mxx) * 0.5,  cy = (mny + mxy) * 0.5,  cz = (mnz + mxz) * 0.5;
                const double ocx = (omnx + omxx) * 0.5, ocy = (omny + omxy) * 0.5, ocz = (omnz + omxz) * 0.5;
                double s2 = 1.0;
                if (nd > 1e-6 && od > 1e-6) {
                    const double ratio = nd / od;
                    if (ratio > 1.25 || ratio < 0.8) {
                        s2 = od / nd;
                        logf("[modmesh] static auto-fit: size %.3f -> %.3f "
                             "(scale %.4f)", nd, od, s2);
                    }
                }
                const double dx = ocx - cx, dy = ocy - cy, dz = ocz - cz;
                if (s2 != 1.0 || std::abs(dx) > 1e-4 || std::abs(dy) > 1e-4
                    || std::abs(dz) > 1e-4)
                {
                    if (s2 == 1.0)
                        logf("[modmesh] static align: shift (%.3f, %.3f, %.3f) "
                             "onto the original bounds", dx, dy, dz);
                    for (TriBucket *part : parts)
                        for (auto &c : part->corners) {
                            c.px = float((c.px - cx) * s2 + ocx);
                            c.py = float((c.py - cy) * s2 + ocy);
                            c.pz = float((c.pz - cz) * s2 + ocz);
                        }
                }
                // Uniform scale on the diagonal keeps the model's proportions,
                // so it can still overhang a thin axis of the original box.
                // When the two boxes disagree per-axis by more than ~2x the
                // import is almost certainly ROTATED with respect to the
                // game's convention for this prop - which the sidecar fixes
                // (yaw=90) and no scale can.
                if (haveOrig && nd > 1e-6 && od > 1e-6) {
                    const double ne[3] { (mxx-mnx)*s2, (mxy-mny)*s2, (mxz-mnz)*s2 };
                    const double oe[3] { omxx-omnx, omxy-omny, omxz-omnz };
                    int worst = -1;
                    double worstR = 1.0;
                    for (int k = 0; k < 3; ++k) {
                        if (ne[k] < 1e-6 || oe[k] < 1e-6) continue;
                        const double r = ne[k] > oe[k] ? ne[k] / oe[k] : oe[k] / ne[k];
                        if (r > worstR) { worstR = r; worst = k; }
                    }
                    if (worst >= 0 && worstR > 2.0)
                        logf("[modmesh] static fit: the import's %c extent is "
                             "%.1fx the original's (import %.3f x %.3f x %.3f "
                             "vs original %.3f x %.3f x %.3f) - the model is "
                             "probably rotated for this slot; try sidecar "
                             "yaw=90/180/270",
                             "XYZ"[worst], worstR, ne[0], ne[1], ne[2],
                             oe[0], oe[1], oe[2]);
                }
            }
            return;
        }

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
                scaled = true;
            }
        }
        // the "dead zone" translate: a cross-family/foreign import whose
        // height is already within 0.8..1.25 of the original used to be
        // left wherever the exporter put it, and the nearest-position
        // weight transfer then sampled the wrong body parts. Feet go to
        // feet, X/Z centres coincide. Pure translation: an import that is
        // already aligned moves by ~0 and stays visually identical.
        if (alignToDonor && !scaled && sc.cfg.fit && haveOrig
            && newH > 1e-6 && origH > 1e-6) {
            const double cx = (mnx + mxx) * 0.5, cz = (mnz + mxz) * 0.5;
            const double ocx = (omnx + omxx) * 0.5, ocz = (omnz + omxz) * 0.5;
            const double dx = ocx - cx, dy = omny - mny, dz = ocz - cz;
            if (std::abs(dx) > 1e-4 || std::abs(dy) > 1e-4 || std::abs(dz) > 1e-4) {
                logf("[modmesh] align: shift (%.3f, %.3f, %.3f) onto the original bounds",
                     dx, dy, dz);
                for (TriBucket *part : parts)
                    for (auto &c : part->corners) {
                        c.px += float(dx);
                        c.py += float(dy);
                        c.pz += float(dz);
                    }
            }
        }
    };

    // Upper bound for any bone index that may reach BonesIdx. The engine feeds
    // the palette entry straight into Mesh->Bones, so out-of-range entries read
    // past the bone array: that is what turns an FBX rigged on a taller
    // armature than the target character into a spiky mess.
    const int maxBone = origRef.nbones > 0 ? origRef.nbones : 0;
    auto boneValid = [&](int b) { return b >= 0 && (maxBone == 0 || b < maxBone); };

    // rebuild a compact palette from skeleton-space indices in `verts`;
    // rewrites the vertex indices to palette slots. Returns the palette.
    auto compactPalette = [&](std::vector<float> &verts) {
        std::map<int, float> weightOf;                    // skel bone -> total w
        size_t outOfRange = 0;
        for (size_t v = 0; v + 16 <= verts.size(); v += 16)
            for (int k = 0; k < 4; ++k) {
                int b = int(verts[v + 8 + k]);
                float w = verts[v + 12 + k];
                if (w <= 0 || b < 0) continue;
                if (!boneValid(b)) { ++outOfRange; continue; }
                weightOf[b] += w;
            }
        if (outOfRange)
            logf("[modmesh] %u influences reference a bone >= %d (the target "
                 "mesh's bone count) and were dropped",
                 unsigned(outOfRange), maxBone);
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

        size_t orphaned = 0;
        for (size_t v = 0; v + 16 <= verts.size(); v += 16) {
            // remember the heaviest influence BEFORE the rewrite: if every
            // influence of this vertex was dropped (bone out of range, or cut
            // by the 64-entry cap) the old code bound it rigidly to slot 0 -
            // i.e. to the root bone - and a whole limb worth of vertices ended
            // up collapsed at the hips. Snapping to the palette entry closest
            // in bone index is a far better guess: PCSKEL orders bones by
            // hierarchy, so the neighbouring index is usually the parent or a
            // sibling of the bone we lost.
            int   bestB = -1;
            float bestW = 0.f;
            for (int k = 0; k < 4; ++k) {
                const int   b = int(verts[v + 8 + k]);
                const float w = verts[v + 12 + k];
                if (b >= 0 && w > bestW) { bestW = w; bestB = b; }
            }

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
            if (wsum <= 0) {
                int slot = 0;
                if (bestB >= 0) {
                    int bestDist = 1 << 30;
                    for (size_t s = 0; s < pal.size(); ++s) {
                        const int d = int(pal[s]) - bestB;
                        const int a = d < 0 ? -d : d;
                        if (a < bestDist) { bestDist = a; slot = int(s); }
                    }
                }
                verts[v + 8] = float(slot); verts[v + 12] = 1;
                ++orphaned;
            }
            else if (wsum < 0.999f)
                for (int k = 0; k < 4; ++k) verts[v + 12 + k] /= wsum;
            if (verts[v + 8] < 0) { verts[v + 8] = 0; verts[v + 12] = 1; }
        }
        if (orphaned)
            logf("[modmesh] %u/%u vertices lost every influence and were bound "
                 "rigidly to the nearest surviving palette entry",
                 unsigned(orphaned), unsigned(verts.size() / 16));
        return pal;
    };

    // Heaviest bone of the WHOLE original mesh, in skeleton-index space. This
    // is the anchor for rigid binds: on a character it lands on the pelvis /
    // lower spine, so the import stays one coherent body that follows the
    // actor, instead of scattering per section (slot 0 of section 3's palette
    // is a different bone from slot 0 of section 7's).
    const int domBone = [&]() -> int {
        std::map<int, double> acc;
        for (const auto &ov : origs) {
            if (!ov.skinnedRow() || ov.palette == nullptr) continue;
            for (uint32_t v = 0; v < ov.nverts; ++v) {
                const float *row = ov.verts + size_t(v) * 16;
                for (int k = 0; k < 4; ++k) {
                    const int   s = int(row[8 + k]);
                    const float w = row[12 + k];
                    if (s < 0 || w <= 0.f || s >= ov.nbones) continue;
                    acc[int(ov.palette[s])] += double(w);
                }
            }
        }
        int best = 0; double bw = -1;
        for (const auto &e : acc) if (e.second > bw) { bw = e.second; best = e.first; }
        return best;
    }();

    // Bind every corner rigidly to one skeleton bone. Guarantees a visible,
    // non-exploding result whatever the source rig looks like: one index, in
    // range by construction, weight 1.
    auto applyRigidBind = [&](TriBucket &B, int bone) {
        if (!boneValid(bone)) bone = 0;
        for (auto &c : B.corners) {
            c.bi[0] = float(bone); c.bw[0] = 1.f;
            for (int k = 1; k < 4; ++k) { c.bi[k] = -1.f; c.bw[k] = 0.f; }
        }
    };

    // Articulated rigid bind ("rigid parts"): ONE bone per vertex, weight 1.
    //
    // Unlike skin=rigid this still animates - an arm follows the arm bone, a
    // tentacle its own chain - it just never blends, so joints go faceted
    // instead of smooth. The point is the outlier filter that follows: spikes
    // are not caused by blending as such, they are caused by a single vertex
    // grabbing a bone that is nowhere near it (on Venom: a shoulder vertex
    // picking up uprighttent_4, 30 tentacle bones being packed into the same
    // volume). Two passes of majority voting over the triangle neighbourhood
    // absorb exactly those isolated wrong assignments, while a real limb
    // boundary - a large coherent region - survives untouched.
    //
    // Input corners must already be in SKELETON index space (clusters, or a
    // skeletonSpace donor transfer). Returns the number of distinct bones used.
    auto applyRigidParts = [&](TriBucket &B) -> size_t {
        const size_t n = B.corners.size();
        if (n < 3) return 0;

        struct K { int32_t x, y, z;
            bool operator==(const K &o) const { return x==o.x && y==o.y && z==o.z; } };
        struct KH { size_t operator()(const K &k) const {
            return (size_t(uint32_t(k.x)) * 73856093u)
                 ^ (size_t(uint32_t(k.y)) * 19349663u)
                 ^ (size_t(uint32_t(k.z)) * 83492791u); } };

        // weld corners onto vertex ids (same quantization as buildBuffers)
        std::unordered_map<K, uint32_t, KH> vid;
        vid.reserve(n);
        std::vector<uint32_t> cornerVert(n, 0);
        for (size_t i = 0; i < n; ++i) {
            const Corner &c = B.corners[i];
            K k{ int32_t(std::lround(double(c.px) * 8192)),
                 int32_t(std::lround(double(c.py) * 8192)),
                 int32_t(std::lround(double(c.pz) * 8192)) };
            auto it = vid.emplace(k, uint32_t(vid.size()));
            cornerVert[i] = it.first->second;
        }
        const size_t nv = vid.size();

        // per-vertex candidate: the heaviest influence over all its corners
        std::vector<std::map<int, float>> acc(nv);
        for (size_t i = 0; i < n; ++i) {
            const Corner &c = B.corners[i];
            for (int k = 0; k < 4; ++k)
                if (c.bi[k] >= 0 && c.bw[k] > 0.f)
                    acc[cornerVert[i]][int(c.bi[k])] += c.bw[k];
        }
        std::vector<int> bone(nv, -1);
        for (size_t v = 0; v < nv; ++v) {
            float best = -1.f;
            for (const auto &e : acc[v]) if (e.second > best) { best = e.second; bone[v] = e.first; }
        }

        // triangle adjacency
        std::vector<std::vector<uint32_t>> adj(nv);
        for (size_t t = 0; t + 2 < n; t += 3) {
            const uint32_t a = cornerVert[t], b = cornerVert[t+1], c = cornerVert[t+2];
            adj[a].push_back(b); adj[a].push_back(c);
            adj[b].push_back(a); adj[b].push_back(c);
            adj[c].push_back(a); adj[c].push_back(b);
        }

        // two majority passes; own vote counts double so stable regions hold
        size_t changed = 0;
        std::vector<int> next(bone);
        for (int pass = 0; pass < 2; ++pass) {
            for (size_t v = 0; v < nv; ++v) {
                if (bone[v] < 0) continue;
                std::map<int, int> tally;
                tally[bone[v]] += 2;
                for (uint32_t nb : adj[v])
                    if (bone[nb] >= 0) tally[bone[nb]] += 1;
                int win = bone[v], winC = -1;
                for (const auto &e : tally)
                    if (e.second > winC) { winC = e.second; win = e.first; }
                if (win != bone[v]) ++changed;
                next[v] = win;
            }
            bone.swap(next);
        }

        std::unordered_set<int> used;
        for (size_t i = 0; i < n; ++i) {
            int b = bone[cornerVert[i]];
            if (!boneValid(b)) b = domBone;
            Corner &c = B.corners[i];
            c.bi[0] = float(b); c.bw[0] = 1.f;
            for (int k = 1; k < 4; ++k) { c.bi[k] = -1.f; c.bw[k] = 0.f; }
            used.insert(b);
        }
        logf("[modmesh]   rigid-parts: %u vertices over %u bones "
             "(%u reassigned by the outlier filter)",
             unsigned(nv), unsigned(used.size()), unsigned(changed));
        return used.size();
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

    // -----------------------------------------------------------------------
    // Bind-pose cluster remap. Exporter bone NUMBERS are only trustworthy when
    // the FBX was rigged on this very mesh's bone array; a sibling rig (the
    // venom_eddie export applied to venom000) inserts jaw/tongue bones and
    // shifts every later index - legs land on tongue slots, the head explodes.
    // Bind POSITIONS are ground truth: match each cluster's TransformLink
    // against the target array. Result per cluster name:
    //   >= 0  target bone index (kept or remapped)
    //   -1    no target bone within tolerance (bone absent from this mesh) -
    //         the cluster is dropped and its vertices fall to the donor patch.
    // Only trusted when the match is unambiguous; on failure the name digits
    // stay authoritative (with the maxBone clamp).
    std::map<std::string, int> bindRemap;
    std::map<std::string, std::array<float, 3>> bindShift;
    bool haveBindRemap = false;
    if (sc.cfg.boneMatch && origRef.nbones > 0
        && origRef.bonePos.size() >= size_t(origRef.nbones) * 3)
    {
        // one link position per distinct cluster name (identical across geoms)
        std::map<std::string, std::array<double, 3>> linkOf;
        for (int64_t mid : sc.meshModelOrder) {
            auto mit = sc.models.find(mid);
            if (mit == sc.models.end()) continue;
            for (int64_t gid : mit->second.geoms) {
                auto git = sc.geoms.find(gid);
                if (git == sc.geoms.end()) continue;
                for (const GCluster &cl : git->second.clusters)
                    if (cl.haveLink && !cl.boneName.empty())
                        linkOf[cl.boneName] =
                            { cl.linkPos[0], cl.linkPos[1], cl.linkPos[2] };
            }
        }
        if (linkOf.size() >= 4) {
            // File units -> game units. sceneScale (unitScale/100) is the
            // importer's own convention for geometry; verify it actually
            // brings the two clouds onto each other, and fall back to an
            // RMS-ratio auto-scale for exporters with unset unit metadata.
            auto rmsOf = [](auto get, size_t n) {
                double cx = 0, cy = 0, cz = 0;
                for (size_t i = 0; i < n; ++i) {
                    auto p = get(i);
                    cx += p[0]; cy += p[1]; cz += p[2];
                }
                cx /= double(n); cy /= double(n); cz /= double(n);
                double s = 0;
                for (size_t i = 0; i < n; ++i) {
                    auto p = get(i);
                    const double dx = p[0]-cx, dy = p[1]-cy, dz = p[2]-cz;
                    s += dx*dx + dy*dy + dz*dz;
                }
                return std::sqrt(s / double(n));
            };
            std::vector<std::array<double,3>> links;
            links.reserve(linkOf.size());
            for (auto &e : linkOf) links.push_back(e.second);
            const double rmsL = rmsOf([&](size_t i){ return links[i]; }, links.size());
            const double rmsB = rmsOf([&](size_t i){
                    const float *p = origRef.bonePos.data() + i * 3;
                    return std::array<double,3>{ p[0], p[1], p[2] };
                }, size_t(origRef.nbones));
            double scaleCand[2] = { sc.sceneScale,
                                    rmsL > 1e-9 ? rmsB / rmsL : 1.0 };

            auto matchWith = [&](double s, std::map<std::string,int> &out,
                                 std::map<std::string,std::array<float,3>> &sh,
                                 size_t *retargeted = nullptr) {
                // tolerance: tight enough to separate real bones (finger
                // joints sit ~0.07u apart), loose enough for float drift
                const double tol = 0.02, tol2 = tol * tol;
                // Retarget window. Beyond an exact match, a source bone whose
                // nearest target is still within a body-part's reach is the
                // SAME joint on a differently-proportioned rig - Eddie's head
                // sits ~0.2u lower than Venom's. Bind there and translate the
                // geometry by the difference, instead of dropping it. Past
                // this the two joints are unrelated and dropping is right.
                // NOTE: do NOT call this `far` - windef.h defines `far` (and
                // `near`) as empty macros for the old segment qualifiers, so
                // `const double far = ...` expands to `const double = ...`.
                const double retargetMax = 0.60, retargetMax2 = retargetMax * retargetMax;
                size_t exact = 0, retgt = 0;
                for (auto &e : linkOf) {
                    const double px = e.second[0] * s,
                                 py = e.second[1] * s,
                                 pz = e.second[2] * s;
                    int    best = -1, tie = -1;
                    double bestD = 1e30;
                    int digit = -1; detail::parseBoneIndexName(e.first, digit);
                    for (int b = 0; b < origRef.nbones; ++b) {
                        const float *q = origRef.bonePos.data() + b * 3;
                        const double dx = px - q[0], dy = py - q[1], dz = pz - q[2];
                        const double d2 = dx*dx + dy*dy + dz*dz;
                        if (d2 < bestD - 1e-12) { bestD = d2; best = b; tie = -1; }
                        else if (d2 <= bestD + 1e-12 && b != best) tie = b;
                        // co-located twins (roll/twist helpers): prefer the
                        // one the exporter's own numbering points at
                        if (tie >= 0 && digit >= 0) {
                            if (tie == digit) { best = tie; tie = -1; }
                            else if (best == digit) tie = -1;
                        }
                    }
                    if (bestD <= tol2) {
                        out[e.first] = best;
                        sh[e.first]  = { 0.f, 0.f, 0.f };
                        ++exact;
                    } else if (bestD <= retargetMax2) {
                        const float *q = origRef.bonePos.data() + best * 3;
                        out[e.first] = best;
                        sh[e.first]  = { float(q[0] - px), float(q[1] - py),
                                         float(q[2] - pz) };
                        ++retgt;
                    } else {
                        out[e.first] = -1;
                        sh[e.first]  = { 0.f, 0.f, 0.f };
                    }
                }
                if (retargeted) *retargeted = retgt;
                return exact;
            };

            std::map<std::string,int> m0, m1;
            std::map<std::string,std::array<float,3>> s0, s1;
            size_t rt0 = 0, rt1 = 0;
            const size_t hits0 = matchWith(scaleCand[0], m0, s0, &rt0);
            const size_t hits1 = matchWith(scaleCand[1], m1, s1, &rt1);
            const bool useAuto = hits1 > hits0;
            std::map<std::string,int> &m = useAuto ? m1 : m0;
            const size_t hits = useAuto ? hits1 : hits0;
            const size_t retgt = useAuto ? rt1 : rt0;

            // Trust the table only if it explains most clusters; a foreign
            // humanoid on a different rig should stay on the name path.
            if (hits * 3 >= linkOf.size() * 2) {
                bindRemap = std::move(m);
                bindShift = std::move(useAuto ? s1 : s0);
                haveBindRemap = true;
                size_t renamed = 0, dropped = 0;
                double maxShift = 0;
                for (auto &e : bindRemap) {
                    int digit = -1; detail::parseBoneIndexName(e.first, digit);
                    if (e.second < 0) ++dropped;
                    else if (digit >= 0 && e.second != digit) ++renamed;
                }
                for (auto &e : bindShift) {
                    const double L = std::sqrt(double(e.second[0])*e.second[0]
                                             + double(e.second[1])*e.second[1]
                                             + double(e.second[2])*e.second[2]);
                    if (L > maxShift) maxShift = L;
                }
                logf("[modmesh] bind-pose bone match (%s scale %.4f): "
                     "%u clusters -> %u exact, %u remapped to a different "
                     "index, %u RETARGETED onto a shifted joint (max %.3f "
                     "units), %u with no bone in the target mesh (dropped)",
                     useAuto ? "auto" : "scene", scaleCand[useAuto ? 1 : 0],
                     unsigned(linkOf.size()), unsigned(hits),
                     unsigned(renamed), unsigned(retgt), maxShift,
                     unsigned(dropped));
            } else {
                logf("[modmesh] bind-pose bone match rejected (%u/%u clusters "
                     "within tolerance) - trusting Bone_N digits",
                     unsigned(hits), unsigned(linkOf.size()));
            }
        }
    }

    // Resolve baked animation channels onto THIS mesh's skeleton, with the
    // exact same trust order as the skinning path: the bind-pose cluster
    // table when it validated, Bone_N name digits otherwise (clamped to the
    // target bone array). A channel that animates a node with no bone on
    // this mesh (armature root, the mesh node itself, extra jaw/tongue
    // joints) stays at -1 and is simply not played.
    if (!sc.anims.empty()) {
        size_t total = 0, viaBind = 0, viaDigits = 0;
        for (auto &clip : sc.anims)
            for (auto &ch : clip.channels) {
                ++total;
                ch.skelIndex = -1;
                if (haveBindRemap) {
                    auto it = bindRemap.find(ch.boneName);
                    if (it != bindRemap.end()) {
                        if (it->second >= 0) { ch.skelIndex = it->second; ++viaBind; }
                        continue;   // table knows this bone; -1 means DROPPED
                    }
                }
                int digit = -1;
                if (detail::parseBoneIndexName(ch.boneName, digit) && digit >= 0
                    && (origRef.nbones <= 0 || digit < origRef.nbones)) {
                    ch.skelIndex = digit;
                    ++viaDigits;
                }
            }
        logf("[modmesh] %s: %u animation channel(s) -> %u bound via bind-pose "
             "match, %u via Bone_N digits, %u unmapped",
             meshName.c_str(), unsigned(total), unsigned(viaBind),
             unsigned(viaDigits), unsigned(total - viaBind - viaDigits));
    }

    // skin decision for one geometry: 1 clusters / 2 transfer / 3 rigid
    auto skinModeFor = [&](const Geom &g, std::vector<int> &remapOut,
                           std::vector<std::array<float,3>> *shiftOut = nullptr) -> int {
        if (sc.cfg.skin == 3) return 3;
        if (sc.cfg.skin != 2 && !g.clusters.empty()) {
            std::vector<int> remap(g.clusters.size(), -1);
            bool allNamed = true;
            // The bind-pose table, when established, is authoritative: it maps
            // by where the bone actually IS, so shifted numbering (a source
            // rig with extra jaw/tongue bones) resolves to the right target,
            // a joint that merely sits elsewhere is RETARGETED (bound to its
            // counterpart plus a translation), and only a bone with no
            // counterpart at all comes back -1 (dropped cluster; its vertices
            // are patched from the original skin afterwards).
            if (haveBindRemap) {
                bool covered = true;
                std::vector<std::array<float,3>> sh(
                    g.clusters.size(), std::array<float,3>{ 0.f, 0.f, 0.f });
                for (size_t k = 0; k < g.clusters.size(); ++k) {
                    auto it = bindRemap.find(g.clusters[k].boneName);
                    if (it == bindRemap.end()) { covered = false; break; }
                    remap[k] = it->second;
                    if (auto s = bindShift.find(g.clusters[k].boneName);
                        s != bindShift.end())
                        sh[k] = s->second;
                }
                if (covered) {
                    int kept = 0;
                    for (int b : remap) if (b >= 0) ++kept;
                    if (kept > 0) {
                        if (shiftOut) *shiftOut = std::move(sh);
                        remapOut = std::move(remap);
                        return 1;
                    }
                    return 2;              // every bone absent: donor transfer
                }
                std::fill(remap.begin(), remap.end(), -1);
            }
            for (size_t k = 0; k < g.clusters.size(); ++k) {
                int idx;
                if (parseBoneIndexName(g.clusters[k].boneName, idx)) remap[k] = idx;
                else { allNamed = false; break; }
            }
            // Clusters that index past the TARGET mesh's bone array cannot be
            // used: BonesIdx feeds nglMesh::Bones directly, so such an entry
            // reads past the end of the bind-pose matrices - that is what turns
            // a correct 60-bone Venom skin into spikes when the exporter also
            // wrote the 5 non-deforming helper bones (prop hands, dummy01,
            // shake_root, camera_root) as Bone_60..Bone_64.
            //
            // Only the offending clusters are dropped, NOT the whole set: those
            // helpers carry no skin weight worth keeping, while the remaining
            // 0..59 are an exact, authored skin that the proximity transfer
            // could never reproduce (Venom's 30 tentacle/tongue bones overlap
            // in space, so nearest-position sampling picks the wrong chain).
            if (allNamed && maxBone > 0) {
                int dropped = 0, worst = -1;
                for (int &b : remap)
                    if (b >= maxBone) {
                        if (b > worst) worst = b;
                        b = -1;
                        ++dropped;
                    }
                if (dropped) {
                    int kept = 0;
                    for (int b : remap) if (b >= 0) ++kept;
                    logf("[modmesh] %d/%u skin clusters name a bone >= %d (up to "
                         "Bone_%d) and were dropped; %d clusters kept",
                         dropped, unsigned(remap.size()), maxBone, worst, kept);
                    if (kept == 0) allNamed = false;   // nothing usable left
                }
            }
            if (allNamed || sc.cfg.skin == 1) {
                if (!allNamed) {                          // forced: cluster order
                    for (size_t k = 0; k < remap.size(); ++k) remap[k] = int(k);
                    logf("[modmesh] skin=clusters forced, using cluster order");
                }
                // even when forced, never let an out-of-range index through:
                // compactPalette drops it, the vertex is re-bound by proximity
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
            std::vector<std::array<float,3>> shift;
            mode = skinModeFor(g, remap, &shift);
            SkinTable st;
            const SkinTable *stp = nullptr;
            if (mode == 1) {
                st.build(g, remap, shift.empty() ? nullptr : &shift);
                stp = &st;
            }
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
            // PASS 1: expand every matched piece first, so the sidecar
            // transform and the normalization treat the WHOLE export as one
            // rigid body. The reference frame is the union of the ORIGINAL
            // sections being replaced: a full 18-piece export is measured
            // against the whole mesh, a head-only export against the head.
            // The import is auto-fit past +/-25% height deviation and, since
            // v2, ALWAYS translated onto the reference bounds (feet to feet,
            // centred X/Z) - hip-rooted armature exports land ~1.8 units
            // below the retail feet-at-0 frame at a height ratio of ~1.0,
            // which the dead zone alone let through and the per-section
            // nearest-position weight transfer then shredded. Exact round
            // trips still pass through bit-exact: every correction is gated
            // on a non-zero delta.
            struct TierAPiece { TriBucket all; int skinMode = 2; std::string src;
                                const Model *model = nullptr; };
            std::map<size_t, TierAPiece> pieces;
            for (size_t si = 0; si < origs.size(); ++si) {
                const Model *m = match[si];
                if (!m) continue;                          // keep original
                if (sc.cfg.keepSections.count(int(si))) {
                    logf("[modmesh] %s sec%u: sidecar keep - original section "
                         "(geometry, palette, morphs) untouched",
                         meshName.c_str(), unsigned(si));
                    continue;                              // keep original
                }
                if (!origs[si].replaceable()) {
                    logf("[modmesh] %s sec%u: no usable vertex stream "
                         "(stride %u, pos offset %d), kept original",
                         meshName.c_str(), unsigned(si),
                         origs[si].strideBytes, origs[si].posOff);
                    continue;
                }
                TierAPiece p;
                p.src   = Scene::normName(m->name);
                p.model = m;
                std::map<int, TriBucket> buckets;
                p.skinMode = expandModel(*m, buckets);
                for (auto &sb : buckets)
                    p.all.corners.insert(p.all.corners.end(),
                                         sb.second.corners.begin(), sb.second.corners.end());
                if (p.all.corners.empty()) {               // empty piece -> hide
                    out[si] = makeHidden(p.src.c_str());
                    logf("[modmesh] %s sec%u <- \"%s\" (empty, hidden)",
                         meshName.c_str(), unsigned(si), p.src.c_str());
                    continue;
                }
                pieces[si] = std::move(p);
            }
            {
                std::vector<TriBucket *> parts;
                std::vector<size_t>      replaced;
                parts.reserve(pieces.size());
                replaced.reserve(pieces.size());
                for (auto &pp : pieces) {
                    parts.push_back(&pp.second.all);
                    replaced.push_back(pp.first);
                }
                fitCorners(parts, /*alignToDonor=*/true, &replaced);
            }

            // PASS 2: per-section skinning, orientation and buffers
            for (auto &pp : pieces) {
                const size_t si = pp.first;
                TriBucket &all = pp.second.all;
                const int skinMode = pp.second.skinMode;
                const std::string &src = pp.second.src;
                const Model *m = pp.second.model;

                BuiltSection b;
                b.source = src;
                for (int64_t matId : m->materials)
                    if (auto ts = sc.matTexStem.find(matId); ts != sc.matTexStem.end())
                        detail::pushUniqueTex(b.textureCandidates, ts->second);
                // no texture reference anywhere on this piece: the exporter's
                // signature for morph-held reveal geometry. Remembered for the
                // blank-geometry policy after the round-trip check below.
                const bool untexturedPiece = b.textureCandidates.empty();
                if (b.textureCandidates.empty()) {
                    // The exporter gave this piece NO material - the shape the
                    // venom_eddie reveal (cocoon, teeth, eddie's head) comes
                    // out in. With an empty list the section never enters the
                    // engine-side retarget and draws whatever the material
                    // holds, which for those pieces is nothing: pure white.
                    // Offer the mesh family as a last resort ("venom_eddie000"
                    // -> VENOM_EDDIE, VENOM_EDDIE000); a sidecar tex<N>= pin
                    // still overrides it.
                    size_t st2 = meshName.size();
                    while (st2 > 0 && std::isdigit((unsigned char) meshName[st2 - 1])) --st2;
                    std::string famU = meshName.substr(0, st2), baseU = meshName;
                    while (!famU.empty() && famU.back() == '_') famU.pop_back();
                    for (auto &ch : famU)  ch = char(std::toupper((unsigned char) ch));
                    for (auto &ch : baseU) ch = char(std::toupper((unsigned char) ch));
                    detail::pushUniqueTex(b.textureCandidates, famU);
                    detail::pushUniqueTex(b.textureCandidates, baseU);
                    b.autoStems = true;   // guesses: scene stems appended later
                    logf("[modmesh] %s sec%u <- \"%s\": no material in the file "
                         "- falling back to the mesh family [%s, %s]. Pin the "
                         "real stem with tex%u=<STEM> in the .ini if this is "
                         "the wrong sheet.",
                         meshName.c_str(), unsigned(si), src.c_str(),
                         famU.c_str(), baseU.c_str(), unsigned(si));
                }
                detail::sortColorTexFirst(b.textureCandidates);
                detail::attachTexPayloads(b, sc);
                size_t nrmHits = 0;
                const bool rigidParts = sc.cfg.skin == 4;
                if (skinMode == 3) {
                    applyRigidBind(all, domBone);
                } else if (skinMode == 2) {
                    // rigid-parts needs SKELETON indices so the palette can be
                    // rebuilt; the normal Tier A transfer stays in slot space.
                    DonorGrid grid = donorForSection(si, /*skeletonSpace=*/rigidParts);
                    if (grid.donors.empty() && anySkinnedOrig)
                        logf("[modmesh] %s sec%u: no usable skin donors (the "
                             "original section carries no weighted vertex "
                             "rows) - the piece will be bound RIGIDLY to one "
                             "bone. Export the FBX with skin clusters, or "
                             "check that the section is not already holding a "
                             "replacement.",
                             meshName.c_str(), unsigned(si));
                    nrmHits = transferCorners(all, grid);
                }
                if (rigidParts && skinMode != 3) applyRigidParts(all);
                if (skinMode == 1) patchWeightlessCorners(all, si);
                orientTriangles(all,
                    detail::normalsMostlyFlat(all) && nrmHits * 2 < all.corners.size());
                buildBuffers(all, sc.cfg.weld, b.vertices, b.indices);

                // a collapsed UV layer (one pair for the whole piece) carries
                // no information: take the vanilla UVs back before anything
                // else looks at them, or the piece draws as one flat texel
                if (detail::countDistinctUV(b.vertices) <= 1) {
                    double uvDist = 0.0;
                    const size_t np =
                        detail::restoreUVsFromOriginal(b.vertices, origs[si],
                                                       uvDist);
                    if (np > 0)
                        logf("[modmesh] %s sec%u <- \"%s\": the export carries "
                             "ONE UV for the whole piece (collapsed UV layer) "
                             "- %u vertices took the original UVs back (max "
                             "search %.4f); it would otherwise draw as a "
                             "single flat texel",
                             meshName.c_str(), unsigned(si), src.c_str(),
                             unsigned(np), uvDist);
                }

                // exact round trip of the original piece? keep the VANILLA
                // buffers: retail morph playback (the eddie reveal cocoon,
                // visemes, facial animation) is dead-zoned on replaced
                // sections and would freeze this geometry at its stored
                // pose. The texture candidates still travel for the
                // retarget.
                double rtDelta = 0.0;
                if (sc.cfg.roundtripEps > 0.0
                    && detail::sectionMatchesOriginal(b.vertices, origs[si],
                                                      sc.cfg.roundtripEps,
                                                      rtDelta)) {
                    b.keepGeometry = true;
                    b.vertices.clear();
                    b.indices.clear();
                    b.palette.clear();
                    b.keepOriginalPalette = true;
                    logf("[modmesh] %s sec%u <- \"%s\": exact round trip of "
                         "the original (max delta %.5f) - vanilla geometry "
                         "kept, morphs stay live (sidecar roundtrip=0 forces "
                         "the import)",
                         meshName.c_str(), unsigned(si), src.c_str(), rtDelta);
                    out[si] = std::move(b);
                    continue;
                }

                // Untextured piece that did NOT round-trip. This is the
                // frozen-cocoon trap: the piece is the exporter's shape for
                // morph-held reveal geometry (stored fully OPEN in the file,
                // folded away by retail morphs every frame). Swapping the
                // geometry in dead-zones those morphs, so the shell freezes
                // OPEN over the shoulders - and with no texture of its own it
                // draws PURE WHITE on top of the character. Default policy:
                // keep the VANILLA section (geometry, palette and morph
                // playback stay live, the piece keeps folding away exactly
                // like retail) and let the texture retarget run in blank-fix
                // mode. Sidecar blank=import forces the old behaviour,
                // blank=hide swaps in a degenerate triangle; a tex<N>= pin
                // or keep=/hide= line for this section wins over all of it.
                if (untexturedPiece && sc.cfg.blankGeo != 1
                    && origs[si].replaceable()
                    && !sc.cfg.texPin.count(int(si))) {
                    if (sc.cfg.blankGeo == 2) {
                        out[si] = makeHidden(src.c_str());
                        logf("[modmesh] %s sec%u <- \"%s\": untextured piece, "
                             "not a round trip - HIDDEN (sidecar blank=hide)",
                             meshName.c_str(), unsigned(si), src.c_str());
                        continue;
                    }
                    b.keepGeometry = true;
                    b.vertices.clear();
                    b.indices.clear();
                    b.palette.clear();
                    b.keepOriginalPalette = true;
                    b.blankOnly = true;     // repaint only a blank material
                    logf("[modmesh] %s sec%u <- \"%s\": untextured piece, not "
                         "a round trip - vanilla geometry kept so the retail "
                         "morphs keep folding it away (it would otherwise "
                         "freeze open as a white shell). Sidecar blank=import "
                         "forces the replacement, blank=hide hides it.",
                         meshName.c_str(), unsigned(si), src.c_str());
                    out[si] = std::move(b);
                    continue;
                }

                if (skinMode == 1 || skinMode == 3 || rigidParts) {
                    b.palette = compactPalette(b.vertices);
                    b.keepOriginalPalette = false;
                } else {
                    b.keepOriginalPalette = true;          // transfer
                }
                b.weightClass = classifyWeights(b.vertices);
                if (oversize(si, src.c_str(), b)) { out[si] = makeHidden(src.c_str()); continue; }

                logf("[modmesh] %s sec%u <- \"%s\" (%u verts, %u tris, skin=%s)",
                     meshName.c_str(), unsigned(si), src.c_str(),
                     unsigned(b.vertices.size() / 16),
                     unsigned(b.indices.size() / 3),
                     !origs[si].skinnedRow() ? "static"
                       : skinMode == 1 ? "clusters"
                       : skinMode == 3 ? "rigid" : "transfer");
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
            {   // normalize: sidecar transform + auto-fit against the
                // original mesh height (no-op within +/-25%) + dead-zone
                // translation onto the original bounds. A merged export
                // represents the whole mesh, so the reference is all
                // sections; genuine round trips shift by ~0 and stay
                // bit-exact.
                std::vector<TriBucket *> parts;
                parts.reserve(buckets.size());
                for (auto &sb : buckets) parts.push_back(&sb.second);
                fitCorners(parts, /*alignToDonor=*/true);
            }
            for (size_t si = 0; si < origs.size(); ++si) {
                if (!origs[si].replaceable()) continue;     // keep original
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
                for (int64_t matId : m->materials)
                    if (auto ts = sc.matTexStem.find(matId); ts != sc.matTexStem.end())
                        detail::pushUniqueTex(b.textureCandidates, ts->second);
                b.autoStems = b.textureCandidates.empty();
                // THIS slot's material carries no texture reference: the
                // exporter's signature for morph-held reveal geometry.
                // Remembered for the blank-geometry policy further down.
                const bool untexturedPiece =
                    si < m->materials.size()
                        ? sc.matTexStem.find(m->materials[si])
                              == sc.matTexStem.end()
                        : b.textureCandidates.empty();
                {   // family-name fallback for exports without texture refs:
                    // "usm_blacksuit000" -> USM_BLACKSUIT, USM_BLACKSUIT000
                    size_t e2 = src.size(), st2 = e2;
                    while (st2 > 0 && std::isdigit((unsigned char) src[st2 - 1])) --st2;
                    std::string famU = src.substr(0, st2), baseU = src;
                    for (auto &ch : famU)  ch = char(std::toupper((unsigned char) ch));
                    for (auto &ch : baseU) ch = char(std::toupper((unsigned char) ch));
                    detail::pushUniqueTex(b.textureCandidates, famU);
                    detail::pushUniqueTex(b.textureCandidates, baseU);
                }
                detail::sortColorTexFirst(b.textureCandidates);
                detail::attachTexPayloads(b, sc);
                size_t nrmHits = 0;
                if (skinMode == 2) {
                    DonorGrid grid = donorForSection(si, false);
                    nrmHits = transferCorners(mine, grid);
                }
                if (skinMode == 1) patchWeightlessCorners(mine, si);
                orientTriangles(mine,
                    detail::normalsMostlyFlat(mine) && nrmHits * 2 < mine.corners.size());
                buildBuffers(mine, sc.cfg.weld, b.vertices, b.indices);

                // collapsed UV layer: take the vanilla UVs back (Tier A note)
                if (detail::countDistinctUV(b.vertices) <= 1) {
                    double uvDist = 0.0;
                    const size_t np =
                        detail::restoreUVsFromOriginal(b.vertices, origs[si],
                                                       uvDist);
                    if (np > 0)
                        logf("[modmesh] %s sec%u <- \"%s\" slot %u: collapsed "
                             "UV layer - %u vertices took the original UVs "
                             "back (max search %.4f)",
                             meshName.c_str(), unsigned(si), src.c_str(),
                             unsigned(si), unsigned(np), uvDist);
                }

                // exact round trip of this slot? keep the vanilla buffers so
                // retail morph playback stays live (see the Tier A note).
                double rtDelta = 0.0;
                if (sc.cfg.roundtripEps > 0.0
                    && detail::sectionMatchesOriginal(b.vertices, origs[si],
                                                      sc.cfg.roundtripEps,
                                                      rtDelta)) {
                    b.keepGeometry = true;
                    b.vertices.clear();
                    b.indices.clear();
                    b.palette.clear();
                    b.keepOriginalPalette = true;
                    logf("[modmesh] %s sec%u <- \"%s\" slot %u: exact round "
                         "trip of the original (max delta %.5f) - vanilla "
                         "geometry kept, morphs stay live (sidecar "
                         "roundtrip=0 forces the import)",
                         meshName.c_str(), unsigned(si), src.c_str(),
                         unsigned(si), rtDelta);
                    out[si] = std::move(b);
                    continue;
                }

                // Untextured slot that did NOT round-trip: morph-held reveal
                // geometry (see the Tier A note). Keep the vanilla section so
                // the retail morphs keep folding it away instead of freezing
                // it open as a white shell; blank=import/hide overrides.
                if (untexturedPiece && sc.cfg.blankGeo != 1
                    && origs[si].replaceable()
                    && !sc.cfg.texPin.count(int(si))) {
                    if (sc.cfg.blankGeo == 2) {
                        out[si] = makeHidden(src.c_str());
                        logf("[modmesh] %s sec%u <- \"%s\" slot %u: untextured "
                             "slot, not a round trip - HIDDEN (sidecar "
                             "blank=hide)",
                             meshName.c_str(), unsigned(si), src.c_str(),
                             unsigned(si));
                        continue;
                    }
                    b.keepGeometry = true;
                    b.vertices.clear();
                    b.indices.clear();
                    b.palette.clear();
                    b.keepOriginalPalette = true;
                    b.blankOnly = true;     // repaint only a blank material
                    logf("[modmesh] %s sec%u <- \"%s\" slot %u: untextured "
                         "slot, not a round trip - vanilla geometry kept so "
                         "the retail morphs keep folding it away (it would "
                         "otherwise freeze open as a white shell). Sidecar "
                         "blank=import forces the replacement, blank=hide "
                         "hides it.",
                         meshName.c_str(), unsigned(si), src.c_str(),
                         unsigned(si));
                    out[si] = std::move(b);
                    continue;
                }

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
            if (origs[si].replaceable()) lastReplaceable = int(si);

        if (chosen != nullptr && !chosen->empty() && lastReplaceable >= 0) {
            logf("[modmesh] %s: cross-family mapping from \"%s\" (%u pieces)",
                 meshName.c_str(), chosenBase.c_str(), unsigned(chosen->size()));

            std::map<int, TriBucket> perSection;
            std::map<int, std::vector<const Model *>> piecesOfSection;
            bool warnedNoDonors = false;
            bool anyClusterMode = false;
            bool anyRigidMode   = false;
            for (auto &pm : *chosen) {
                std::map<int, TriBucket> slots;
                // The mode was DISCARDED here before. A cross-family export
                // that carries real skin clusters (the usual case: the FBX was
                // exported from this game's own rig, it just kept the name of
                // the character it was authored for) had its authored weights
                // overwritten below by a nearest-position transfer. Worse,
                // skin=rigid in the sidecar was ignored outright, so there was
                // no way to ask for the safe, always-visible result.
                const int pmode = expandModel(*pm.second, slots);
                if (pmode == 1) anyClusterMode = true;
                if (pmode == 3) anyRigidMode   = true;
                int target = pm.first;
                if (target > lastReplaceable
                    || (size_t(target) < origs.size()
                        && !origs[size_t(target)].replaceable()))
                    target = lastReplaceable;
                piecesOfSection[target].push_back(pm.second);
                TriBucket &dst = perSection[target];
                for (auto &sb : slots)
                    dst.corners.insert(dst.corners.end(),
                                       sb.second.corners.begin(),
                                       sb.second.corners.end());
            }
            std::vector<TriBucket *> parts;
            parts.reserve(perSection.size());
            for (auto &sb : perSection) parts.push_back(&sb.second);
            // alignToDonor exists to make NEAREST-POSITION sampling land on the
            // right bones. With cluster weights the skin is already bound per
            // control point, and dragging the body feet-to-feet would only push
            // it off the bind pose the weights were authored against.
            fitCorners(parts, /*alignToDonor=*/!anyClusterMode && !anyRigidMode);
            if (anyClusterMode)
                logf("[modmesh] %s: cross-family pieces carry skin clusters - "
                     "keeping the authored weights (no proximity transfer)",
                     meshName.c_str());
            else if (anyRigidMode)
                logf("[modmesh] %s: skin=rigid - the whole import is bound to "
                     "bone %d (safe, always visible, does not deform)",
                     meshName.c_str(), domBone);

            for (size_t si = 0; si < origs.size(); ++si) {
                if (!origs[si].replaceable()) continue;
                auto it = perSection.find(int(si));
                if (it == perSection.end() || it->second.corners.empty()) {
                    out[si] = makeHidden("cross-family/uncovered");
                    continue;
                }
                TriBucket &mine = it->second;

                BuiltSection b;
                b.source = chosenBase;
                // texture identity per SECTION: the pieces that actually
                // landed here come first (eyes keep the eye texture, body
                // the body one), the family union and the family-name
                // fallback only cover pieces without their own reference
                if (auto po = piecesOfSection.find(int(si));
                    po != piecesOfSection.end())
                    for (const Model *pm2 : po->second)
                        for (int64_t matId : pm2->materials)
                            if (auto ts = sc.matTexStem.find(matId); ts != sc.matTexStem.end())
                                detail::pushUniqueTex(b.textureCandidates, ts->second);
                for (const auto &pm2 : *chosen)
                    for (int64_t matId : pm2.second->materials)
                        if (auto ts = sc.matTexStem.find(matId); ts != sc.matTexStem.end())
                            detail::pushUniqueTex(b.textureCandidates, ts->second);
                b.autoStems = b.textureCandidates.empty();
                {
                    std::string famU; int lodTmp = 0;
                    splitLod(chosenBase, famU, lodTmp);
                    std::string baseU = chosenBase;
                    for (auto &ch : famU)  ch = char(std::toupper(uint8_t(ch)));
                    for (auto &ch : baseU) ch = char(std::toupper(uint8_t(ch)));
                    detail::pushUniqueTex(b.textureCandidates, famU);
                    detail::pushUniqueTex(b.textureCandidates, baseU);
                    detail::sortColorTexFirst(b.textureCandidates);
                }
                detail::attachTexPayloads(b, sc);
                size_t nrmHits = 0;
                const bool rigidParts = sc.cfg.skin == 4;
                bool   skelIndices = anyClusterMode || anyRigidMode || rigidParts;
                if (anyRigidMode && !anyClusterMode) {
                    applyRigidBind(mine, domBone);
                } else if (!anyClusterMode) {
                    DonorGrid grid = donorForSection(si, /*skeletonSpace=*/true);
                    if (!grid.donors.empty()) {
                        nrmHits = transferCorners(mine, grid);
                        skelIndices = true;
                    } else if (!warnedNoDonors && anySkinnedOrig) {
                        warnedNoDonors = true;
                        logf("[modmesh] no usable skin donors (original vertex "
                             "data unavailable or not the 64-byte layout) - "
                             "rigid fallback; export the FBX with skin clusters "
                             "(Armature modifier) for animated results");
                    }
                }
                if (rigidParts && !anyRigidMode) applyRigidParts(mine);
                if (anyClusterMode) patchWeightlessCorners(mine, si);
                orientTriangles(mine,
                    detail::normalsMostlyFlat(mine) && nrmHits * 2 < mine.corners.size());
                buildBuffers(mine, sc.cfg.weld, b.vertices, b.indices);
                if (skelIndices) {
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
        std::vector<int64_t>   slotMat;                    // bucket -> material id
        auto slotOfMaterial = [&](int64_t matId) {
            auto it = globalSlot.find(matId);
            if (it != globalSlot.end()) return it->second;
            int s = int(globalSlot.size());
            globalSlot[matId] = s;
            slotMat.push_back(matId);
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
            fitCorners(parts, /*alignToDonor=*/true);
        }

        // ---- distribute buckets over sections ------------------------------
        int lastReplaceable = -1;
        for (size_t si = 0; si < origs.size(); ++si)
            if (origs[si].replaceable()) lastReplaceable = int(si);
        if (lastReplaceable < 0) return {};

        int nBuckets = buckets.empty() ? 0 : (buckets.rbegin()->first + 1);

        for (size_t si = 0; si < origs.size(); ++si) {
            if (!origs[si].replaceable()) continue;        // keep original
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
            if (size_t(si) < slotMat.size())
                if (auto ts = sc.matTexStem.find(slotMat[size_t(si)]);
                    ts != sc.matTexStem.end())
                    detail::pushUniqueTex(b.textureCandidates, ts->second);
            b.autoStems = b.textureCandidates.empty();
            detail::sortColorTexFirst(b.textureCandidates);
            detail::attachTexPayloads(b, sc);
            bool skelIndices = false;
            size_t nrmHits = 0;
            const bool rigidParts = sc.cfg.skin == 4;
            if (skinMode == 3) {
                applyRigidBind(mine, domBone);
                skelIndices = true;
            } else if (skinMode == 2) {
                // transfer in SKELETON space from all original sections,
                // then compact to a fresh palette for this section
                DonorGrid grid = donorForSection(si, /*skeletonSpace=*/true);
                if (!grid.donors.empty()) {
                    nrmHits = transferCorners(mine, grid);
                    skelIndices = true;
                }
            }
            if (rigidParts && skinMode != 3) { applyRigidParts(mine); skelIndices = true; }
            if (skinMode == 1) patchWeightlessCorners(mine, si);
            orientTriangles(mine,
                detail::normalsMostlyFlat(mine) && nrmHits * 2 < mine.corners.size());
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
                 !origs[si].skinnedRow() ? "static"
                   : skinMode == 1 ? "clusters"
                   : skinMode == 3 ? "rigid" : "transfer");
            out[si] = std::move(b);
        }
        return out;
    }
}

// ===========================================================================
//  final safety net - runs on EVERY built section, whatever tier produced it
// ===========================================================================
// The failure ladder of a bad skin, from worst to best, is:
//   1. spikes / shrapnel   - a blend index selects a matrix outside the mesh's
//                            bone array (or a non-finite float reaches the VB);
//   2. coherent-but-wrong  - the body renders whole, in a wrong pose;
//   3. correct.
// The tiers aim for 3; this pass makes 1 IMPOSSIBLE by demoting anything
// invalid to 2: non-finite floats are zeroed, out-of-range blend slots and
// palette entries are dropped/clamped, and a vertex left with no influence is
// bound rigidly to slot 0. Whatever else goes wrong upstream, the worst the
// player can ever see is a solid body - never the exploded mess.
inline void sanitizeBuiltSection(BuiltSection &b, const OrigSectionView &ov,
                                 const OrigMeshRef &ref)
{
    const int maxBone = ref.nbones;
    // palette entries: must index inside the target mesh's bone array
    size_t palFixed = 0;
    if (!b.keepOriginalPalette && maxBone > 0)
        for (auto &e : b.palette)
            if (int(e) >= maxBone) { e = 0; ++palFixed; }

    // vertex blend slots: must index inside the palette that will be bound
    const int nslots = b.keepOriginalPalette
                     ? (ov.nbones > 0 ? ov.nbones : 1)
                     : int(b.palette.empty() ? 1 : b.palette.size());

    size_t nanFixed = 0, slotFixed = 0, rebound = 0;
    for (size_t v = 0; v + 16 <= b.vertices.size(); v += 16) {
        float *r = b.vertices.data() + v;
        for (int c = 0; c < 8; ++c)                    // pos, nrm, uv
            if (!std::isfinite(r[c])) { r[c] = 0.f; ++nanFixed; }
        float wsum = 0.f;
        for (int k = 0; k < 4; ++k) {
            float &s = r[8 + k];
            float &w = r[12 + k];
            if (!std::isfinite(s) || !std::isfinite(w) || w <= 0.f) {
                s = -1.f; w = 0.f;
                continue;
            }
            const int slot = int(s);
            if (slot < 0 || slot >= nslots) {
                s = -1.f; w = 0.f;
                ++slotFixed;
                continue;
            }
            wsum += w;
        }
        if (wsum <= 0.f) {                             // nothing left: rigid
            r[8]  = 0.f; r[9]  = r[10] = r[11] = -1.f;
            r[12] = 1.f; r[13] = r[14] = r[15] = 0.f;
            ++rebound;
        } else {
            // front-pack the survivors, heaviest first: the 2/3/4-bone shader
            // picked by weightClass reads the lanes positionally, and every
            // producer in this file keeps unused lanes TRAILING (-1 / 0).
            struct SW { float s, w; } packed[4];
            int np = 0;
            for (int k = 0; k < 4; ++k)
                if (r[12 + k] > 0.f) packed[np++] = { r[8 + k], r[12 + k] };
            std::sort(packed, packed + np,
                      [](const SW &a, const SW &b) { return a.w > b.w; });
            const float inv = (wsum < 0.999f || wsum > 1.001f) ? 1.f / wsum : 1.f;
            for (int k = 0; k < 4; ++k) {
                if (k < np) { r[8 + k] = packed[k].s; r[12 + k] = packed[k].w * inv; }
                else        { r[8 + k] = -1.f;        r[12 + k] = 0.f; }
            }
        }
    }
    if (nanFixed || slotFixed || palFixed)
        logf("[modmesh] sanitize \"%s\": %u non-finite floats zeroed, "
             "%u out-of-range blend slots dropped, %u palette entries "
             "clamped, %u vertices re-bound rigidly",
             b.source.c_str(), unsigned(nanFixed), unsigned(slotFixed),
             unsigned(palFixed), unsigned(rebound));

    // Position outliers. A FINITE vertex far outside the character's own
    // volume is an importer artefact (stray helper node, unmapped piece,
    // botched transform), never real geometry - and a single one is enough to
    // blow up the section sphere the engine culls with, picks LODs by and
    // frames the chase camera from. Snap it onto the reference volume's
    // boundary instead of dropping it: triangles stay connected (a flattened
    // sliver against the boundary, not a hole) and the worst case stays one
    // compact, visible body.
    {
        float cx = 0.f, cy = 0.f, cz = 0.f, allow = 0.f;
        if (ref.haveSphere && ref.sphereRadius > 1e-4f) {
            cx = ref.sphereCenter[0]; cy = ref.sphereCenter[1];
            cz = ref.sphereCenter[2];
            // the shipped sphere already covers every animated pose, so a
            // bind-pose vertex outside 1.5x of it cannot be legitimate
            allow = ref.sphereRadius * 1.5f + 0.25f;
        } else if (ref.haveBones) {
            cx = (ref.bonesMin[0] + ref.bonesMax[0]) * 0.5f;
            cy = (ref.bonesMin[1] + ref.bonesMax[1]) * 0.5f;
            cz = (ref.bonesMin[2] + ref.bonesMax[2]) * 0.5f;
            const float ex = ref.bonesMax[0] - ref.bonesMin[0];
            const float ey = ref.bonesMax[1] - ref.bonesMin[1];
            const float ez = ref.bonesMax[2] - ref.bonesMin[2];
            allow = std::sqrt(ex * ex + ey * ey + ez * ez) * 0.75f + 0.25f;
        }
        if (allow > 0.f) {
            size_t clamped = 0;
            for (size_t v = 0; v + 16 <= b.vertices.size(); v += 16) {
                float *r = b.vertices.data() + v;
                const float dx = r[0] - cx, dy = r[1] - cy, dz = r[2] - cz;
                const float d2 = dx * dx + dy * dy + dz * dz;
                if (d2 <= allow * allow) continue;
                const float s = allow / std::sqrt(d2);
                r[0] = cx + dx * s; r[1] = cy + dy * s; r[2] = cz + dz * s;
                ++clamped;
            }
            if (clamped)
                logf("[modmesh] sanitize \"%s\": %u vertices outside the "
                     "reference volume (r %.2f) snapped onto it",
                     b.source.c_str(), unsigned(clamped), allow);
        }
    }
    b.weightClass = detail::classifyWeights(b.vertices);
}

// ===========================================================================
//  static rigid sections: target layout + baked vertex colours
// ===========================================================================
// Runs on every built section whose TARGET is a static one, after the tiers
// and instead of the skin sanitizer. Three jobs:
//
//   1. copy the target's vertex format onto the BuiltSection, so the engine
//      applier can pack the canonical rows down without knowing the section;
//   2. neutralize the blend lanes (slot 0 / weight 1). They are never written
//      to a static vertex buffer, but a section that is later mirrored or
//      re-read must not carry -1 slots and zero weights around;
//   3. inherit the BAKED LIGHTING. USM stores per-vertex lighting for static
//      geometry in the D3DCOLOR channel: a replacement that wrote white would
//      render flat and detached from the scene, so every new vertex takes the
//      colour of the nearest ORIGINAL vertex. An exact round trip therefore
//      reproduces the vanilla shading exactly.
inline void finishRigidSection(BuiltSection &b, const OrigSectionView &ov)
{
    b.rigid        = true;
    b.targetStride = ov.strideBytes;
    b.tPosOff      = ov.posOff;
    b.tNrmOff      = ov.nrmOff;
    b.tUvOff       = ov.uvOff;
    b.tColOff      = ov.colOff;
    b.keepOriginalPalette = true;
    b.palette.clear();

    size_t nanFixed = 0;
    for (size_t v = 0; v + 16 <= b.vertices.size(); v += 16) {
        float *r = b.vertices.data() + v;
        for (int c = 0; c < 8; ++c)
            if (!std::isfinite(r[c])) { r[c] = 0.f; ++nanFixed; }
        r[8]  = 0.f; r[9]  = r[10] = r[11] = -1.f;
        r[12] = 1.f; r[13] = r[14] = r[15] = 0.f;
    }

    const size_t nv = b.vertices.size() / 16;
    b.colors.clear();
    if (ov.colOff >= 0 && nv > 0) {
        detail::DonorGrid grid;
        grid.donors.reserve(ov.nverts);
        for (uint32_t i = 0; i < ov.nverts; ++i) {
            detail::Donor d {};
            ov.getPos(i, d.p);
            if (!std::isfinite(d.p[0]) || !std::isfinite(d.p[1])
                || !std::isfinite(d.p[2]))
                continue;
            ov.getNrm(i, d.n);
            d.col = ov.getCol(i);
            grid.donors.push_back(d);
        }
        grid.build();
        b.colors.assign(nv, 0xFFFFFFFFu);
        if (!grid.donors.empty()) {
            double worst = 0.0;
            for (size_t i = 0; i < nv; ++i) {
                const float *r = b.vertices.data() + i * 16;
                float d2 = 0.f;
                if (const detail::Donor *n = grid.nearest(r, &d2)) {
                    b.colors[i] = n->col;
                    worst = std::max(worst, double(d2));
                }
            }
            logf("[modmesh]   static section: %u vertices took the baked "
                 "vertex colour of the nearest original (max search %.4f)",
                 unsigned(nv), std::sqrt(worst));
        }
    }
    if (nanFixed)
        logf("[modmesh] sanitize \"%s\": %u non-finite floats zeroed "
             "(static section)", b.source.c_str(), unsigned(nanFixed));
}

// Position outliers on a static target. The character sanitizer measures
// against the skeleton/bounding sphere; a prop has neither, so the reference
// is the ORIGINAL section's own box, generously padded. One stray vertex is
// still enough to wreck the section sphere the engine culls and picks LODs
// with, so it is snapped onto the boundary rather than dropped - triangles
// stay connected.
inline void clampRigidOutliers(BuiltSection &b, const OrigSectionView &ov)
{
    if (b.vertices.empty() || ov.nverts == 0) return;
    float mn[3], mx[3];
    ov.getPos(0, mn);
    for (int c = 0; c < 3; ++c) mx[c] = mn[c];
    for (uint32_t i = 1; i < ov.nverts; ++i) {
        float p[3];
        ov.getPos(i, p);
        for (int c = 0; c < 3; ++c) {
            if (!std::isfinite(p[c])) continue;
            mn[c] = std::min(mn[c], p[c]);
            mx[c] = std::max(mx[c], p[c]);
        }
    }
    const float cx = (mn[0] + mx[0]) * 0.5f, cy = (mn[1] + mx[1]) * 0.5f,
                cz = (mn[2] + mx[2]) * 0.5f;
    const float ex = mx[0] - mn[0], ey = mx[1] - mn[1], ez = mx[2] - mn[2];
    const float allow = std::sqrt(ex * ex + ey * ey + ez * ez) * 1.5f + 0.25f;
    if (!(allow > 0.f) || !std::isfinite(allow)) return;
    size_t clamped = 0;
    for (size_t v = 0; v + 16 <= b.vertices.size(); v += 16) {
        float *r = b.vertices.data() + v;
        const float dx = r[0] - cx, dy = r[1] - cy, dz = r[2] - cz;
        const float d2 = dx * dx + dy * dy + dz * dz;
        if (d2 <= allow * allow) continue;
        const float s = allow / std::sqrt(d2);
        r[0] = cx + dx * s; r[1] = cy + dy * s; r[2] = cz + dz * s;
        ++clamped;
    }
    if (clamped)
        logf("[modmesh] sanitize \"%s\": %u vertices outside the original "
             "section box (r %.2f) snapped onto it",
             b.source.c_str(), unsigned(clamped), allow);
}

inline std::vector<std::optional<BuiltSection>>
buildSectionsForMesh(Scene &sc,
                     const std::string &meshNameIn,
                     const std::vector<OrigSectionView> &origsIn,
                     const OrigMeshRef &origRef = {})
{
    // sidecar layout= override, applied to the STATIC sections only, before
    // anything reads a vertex through them
    std::vector<OrigSectionView> origs = origsIn;
    if (sc.cfg.layoutSet) {
        size_t n = 0;
        for (auto &ov : origs) {
            if (ov.skinned && ov.strideBytes == 64) continue;
            if (sc.cfg.layStride > 0) ov.strideBytes = uint32_t(sc.cfg.layStride);
            if (sc.cfg.layPos >= 0)   ov.posOff = sc.cfg.layPos;
            if (sc.cfg.layNrm != -2)  ov.nrmOff = sc.cfg.layNrm;
            if (sc.cfg.layUv  != -2)  ov.uvOff  = sc.cfg.layUv;
            if (sc.cfg.layCol != -2)  ov.colOff = sc.cfg.layCol;
            ov.skinned = false;
            ++n;
        }
        logf("[modmesh] sidecar layout override on %u static section(s): "
             "stride %u pos %d nrm %d uv %d col %d",
             unsigned(n), origs.empty() ? 0u : origs[0].strideBytes,
             origs.empty() ? 0 : origs[0].posOff,
             origs.empty() ? 0 : origs[0].nrmOff,
             origs.empty() ? 0 : origs[0].uvOff,
             origs.empty() ? 0 : origs[0].colOff);
    }

    auto out = buildSectionsForMeshRaw(sc, meshNameIn, origs, origRef);

    // The raw mapper returns an EMPTY vector whenever it cannot map geometry:
    // a scene with no usable mesh, a foreign scene that produced no buckets, a
    // target with no replaceable section. That verdict is about GEOMETRY, but
    // it used to take the whole sidecar down with it - and the sidecar's
    // texture instructions do not depend on the importer mapping anything.
    // white= and tex<N>= are documented to work on sections that were never
    // replaced, so a sidecar that only repaints (an .fbx that exists purely to
    // carry a white= list) silently did nothing at all. Give those
    // instructions a vector to land in; geometry stays vanilla.
    const bool texOnly = out.empty() && !origs.empty()
                      && (!sc.cfg.whiteSections.empty()
                          || sc.cfg.whiteBlank
                          || !sc.cfg.texPin.empty()
                          || !sc.cfg.texDefault.empty());
    if (texOnly) {
        out.assign(origs.size(), std::nullopt);
        logf("[modmesh] %s: no geometry mapped - running the sidecar's TEXTURE "
             "instructions only (white=%u pin=%u default=\"%s\"); every "
             "section keeps its vanilla geometry",
             meshNameIn.c_str(), unsigned(sc.cfg.whiteSections.size()),
             unsigned(sc.cfg.texPin.size()), sc.cfg.texDefault.c_str());
    }

    // sidecar per-section overrides, applied over every tier's result.
    //   hide=N,..  replaces the section with a degenerate triangle
    //   keep=N,..  drops any replacement: geometry, index buffer AND retail
    //              morph playback stay fully vanilla (keep wins over hide)
    // Both are geometry edits, so neither runs in the texture-only pass.
    if (!texOnly)
    for (int si : sc.cfg.hideSections)
        if (si >= 0 && size_t(si) < out.size()) {
            BuiltSection h;
            h.hide = true;
            h.source = "sidecar hide";
            h.vertices.assign(16, 0.f);
            h.vertices[9] = h.vertices[10] = h.vertices[11] = -1.f;
            h.vertices[12] = 1.f;
            h.indices = { 0, 0, 0 };
            h.weightClass = 2;
            out[size_t(si)] = std::move(h);
            logf("[modmesh] sec%d: sidecar hide", si);
        }
    for (int si : sc.cfg.keepSections)
        if (si >= 0 && size_t(si) < out.size() && out[si]) {
            out[size_t(si)].reset();
            logf("[modmesh] sec%d: sidecar keep - original section (geometry, "
                 "palette, morphs) untouched", si);
        }

    // -----------------------------------------------------------------------
    //  automatic blank-section coverage (sidecar autotex=off disables)
    // -----------------------------------------------------------------------
    // Untextured sections draw PURE WHITE - the venom_eddie "blank pieces":
    // this build ships several vanilla materials with no diffuse texture, and
    // the exporter gives the reveal pieces (cocoon, teeth, eddie's head) no
    // material at all. Coverage is two-fold, and both halves only ever bind
    // sheets the file itself brings:
    //   * every untouched vanilla section gets a keepGeometry carrier marked
    //     blankOnly: the engine runs the texture retarget on it but may only
    //     touch the material when its diffuse texture is MISSING;
    //   * every guessed/empty candidate list is extended (further down, after
    //     tex_default) with the scene's own color stems, so a family guess
    //     that resolves to nothing ("VENOM_EDDIE") degrades to a real sheet
    //     (VENOM_EDDIE_01) instead of a white section.
    const std::vector<std::string> autoStems =
        (sc.cfg.autoTex && !texOnly) ? detail::sceneColorStems(sc, meshNameIn)
                                     : std::vector<std::string>{};
    if (!autoStems.empty()) {
        unsigned carriers = 0;
        for (size_t si = 0; si < out.size(); ++si) {
            if (out[si] || sc.cfg.keepSections.count(int(si)))
                continue;
            BuiltSection t;
            t.keepGeometry = true;      // never swaps buffers
            t.blankOnly    = true;      // never displaces a bound texture
            t.autoStems    = true;
            t.source       = "auto blank-fix";
            if (!sc.cfg.texDefault.empty())
                detail::pushUniqueTex(t.textureCandidates, sc.cfg.texDefault);
            for (const std::string &s : autoStems)
                detail::pushUniqueTex(t.textureCandidates, s);
            detail::attachTexPayloads(t, sc);
            out[si] = std::move(t);
            ++carriers;
        }
        if (carriers != 0) {
            std::string stems;
            for (const std::string &s : autoStems) {
                if (!stems.empty()) stems += ", ";
                stems += s;
            }
            logf("[modmesh] autotex: %u untouched section(s) get a blank-fix "
                 "carrier - repainted ONLY if their material has no diffuse "
                 "texture. Stems, in order: [%s] (sidecar autotex=off "
                 "disables)", carriers, stems.c_str());
        }
    }

    for (size_t si = 0; si < out.size() && si < origs.size(); ++si)
        if (out[si]) {
            out[si]->texMode = sc.cfg.tex;
            if (out[si]->keepGeometry) continue;
            if (origs[si].rigidRow()) {
                // static target: no palette, no blend lanes, no morph window -
                // but the vertex format and the baked colours must travel
                clampRigidOutliers(*out[si], origs[si]);
                finishRigidSection(*out[si], origs[si]);
            } else {
                sanitizeBuiltSection(*out[si], origs[si], origRef);
            }
        }

    // -----------------------------------------------------------------------
    //  texture pins and the last-resort stem
    // -----------------------------------------------------------------------
    // A section that reaches the engine with an EMPTY candidate list never
    // even enters the retarget, so whatever its material holds is what it
    // draws - and when the material carries no diffuse texture that is pure
    // white. tex_default gives those pieces something to resolve; tex<N>
    // overrides them one by one.
    if (!sc.cfg.texDefault.empty())
        for (size_t si = 0; si < out.size(); ++si)
            if (out[si] && !out[si]->hide && out[si]->textureCandidates.empty()) {
                out[si]->textureCandidates.push_back(sc.cfg.texDefault);
                logf("[modmesh] sec%u: no texture reference in the file - "
                     "sidecar tex_default=%s", unsigned(si),
                     sc.cfg.texDefault.c_str());
            }

    // guessed/empty candidate lists degrade to the scene's own sheets, never
    // to white. Authored lists (a real material reference from the file) are
    // left alone: appending foreign stems there could hijack the mods/-folder
    // recolor workflow. Runs after tex_default so an explicit default keeps
    // its priority.
    if (!autoStems.empty())
        for (size_t si = 0; si < out.size(); ++si)
            if (out[si] && !out[si]->hide
                && (out[si]->autoStems || out[si]->textureCandidates.empty())) {
                const size_t before = out[si]->textureCandidates.size();
                for (const std::string &s : autoStems)
                    detail::pushUniqueTex(out[si]->textureCandidates, s);
                if (out[si]->textureCandidates.size() != before) {
                    detail::attachTexPayloads(*out[si], sc);
                    logf("[modmesh] sec%u: texture list was a guess - scene "
                         "stems appended as fallback", unsigned(si));
                }
            }

    for (const auto &pin : sc.cfg.texPin) {
        const int si = pin.first;
        if (si < 0 || size_t(si) >= out.size()) {
            logf("[modmesh] sidecar tex%d=%s: no such section (mesh has %u)",
                 si, pin.second.c_str(), unsigned(out.size()));
            continue;
        }
        if (!out[size_t(si)]) {
            // the section was NOT replaced (vanilla geometry, or an exact
            // round trip the detector kept): carry a geometry-free record so
            // the engine still runs the retarget on it. keepGeometry makes the
            // applier skip every buffer swap.
            BuiltSection t;
            t.keepGeometry = true;
            t.source       = "sidecar texpin";
            out[size_t(si)] = std::move(t);
        }
        out[size_t(si)]->textureCandidates.assign(1, pin.second);
        out[size_t(si)]->texExclusive = true;
        out[size_t(si)]->blankOnly    = true;  // a pin always repaints, even
                                                // over a blank-fix carrier
        logf("[modmesh] sec%d: texture PINNED to \"%s\" (sidecar)",
             si, pin.second.c_str());
    }

    // -----------------------------------------------------------------------
    //  white=N,.. - pieces that are white ON PURPOSE
    // -----------------------------------------------------------------------
    // Runs LAST so it wins over every policy and every other list: the black
    // suit's eye lenses and chest spider are untextured white geometry in
    // retail, and everything above is built on the assumption that an
    // untextured section is a defect to be repaired. hide= / blank=hide would
    // delete them, the autotex carrier and the sibling-donor fallback would
    // paint the body sheet over them. A white= section carries a geometry-free
    // record marked forceWhite: the engine clones its material, binds the 1x1
    // white texture and returns before any of that can run.
    for (int si : sc.cfg.whiteSections) {
        if (si < 0 || size_t(si) >= out.size()) {
            logf("[modmesh] sidecar white=%d: no such section (mesh has %u)",
                 si, unsigned(out.size()));
            continue;
        }
        if (!out[size_t(si)]) {
            BuiltSection t;
            t.keepGeometry = true;          // vanilla geometry and morphs stay
            t.source       = "sidecar white";
            out[size_t(si)] = std::move(t);
        }
        BuiltSection &b = *out[size_t(si)];
        if (b.hide) {
            // undo a hide (explicit hide= or the blank=hide policy): give the
            // vanilla piece back instead of the degenerate triangle, then
            // paint it white
            b.hide = true;
            b.keepGeometry = true;
            b.vertices.clear();
            b.indices.clear();
            b.source = "sidecar white (was hidden)";
        }
        b.forceWhite   = true;
        b.blankOnly    = true;             // white always lands
        b.texExclusive = true;              // engine: clone the material first
        b.autoStems    = true;             // no stem sweep on a white piece
        b.textureCandidates.clear();        // nothing to resolve by name
        b.embeddedTex.clear();
        b.texRelPath.clear();
        logf("[modmesh] sec%d: forced WHITE (sidecar) - 1x1 white texture on a "
             "private material clone, blank repair disabled", si);
    }

    // -----------------------------------------------------------------------
    //  white=auto / white=blank - white WITHOUT knowing section numbers
    // -----------------------------------------------------------------------
    // The explicit list above needs indices read out of the section map. These
    // two policies do not: they are evaluated engine-side against the state
    // only the engine can see - whether the section's material actually has a
    // diffuse texture bound, and what that material is called. Nothing here
    // touches a section that draws a texture; the entire scope is materials
    // that would otherwise reach the blank-repair fallbacks.
    if (sc.cfg.whiteAuto || sc.cfg.whiteBlank) {
        auto names = std::make_shared<const std::vector<std::string>>(
            sc.cfg.whiteNames.empty() ? detail::defaultWhiteNames()
                                      : sc.cfg.whiteNames);
        // every section needs a record for the engine to get a look at it.
        // autotex already leaves carriers on untouched sections; when it is
        // off, the white policies still need one.
        unsigned carriers = 0;
        for (size_t si = 0; si < out.size(); ++si) {
            if (out[si] || sc.cfg.keepSections.count(int(si)))
                continue;
            BuiltSection t;
            t.keepGeometry = true;      // never swaps buffers
            t.blankOnly    = true;      // never displaces a bound texture
            t.source       = "white policy carrier";
            out[si] = std::move(t);
            ++carriers;
        }
        for (auto &o : out)
            if (o && !o->hide && !o->forceWhite) {
                o->whiteBlank  = sc.cfg.whiteBlank;
                o->whiteByName = sc.cfg.whiteAuto;
                o->whiteNames  = names;
            }
        std::string kw;
        for (const std::string &n : *names) {
            if (!kw.empty()) kw += ", ";
            kw += n;
        }
        logf("[modmesh] white policy: %s (%u extra carrier(s)); a section whose "
             "material has NO diffuse texture draws WHITE instead of being "
             "repainted by the salvage/donor fallbacks%s%s",
             sc.cfg.whiteBlank ? "blank - EVERY untextured material"
                               : "auto - untextured materials matching a name",
             carriers,
             sc.cfg.whiteBlank ? "" : ". Names: ",
             sc.cfg.whiteBlank ? "" : kw.c_str());
    }

    // -----------------------------------------------------------------------
    //  section map: what each section ended up as, and with which stems
    // -----------------------------------------------------------------------
    // This is the table you read to write the pins: it names the sections that
    // were replaced, the ones that were hidden or kept vanilla, and the
    // texture stems each one will try. Anything showing "tex=[]" is a
    // candidate for the white/untextured symptom.
    {
        logf("[modmesh] --- section map for \"%s\" (%u sections) ---",
             meshNameIn.c_str(), unsigned(out.size()));
        for (size_t si = 0; si < out.size(); ++si) {
            if (!out[si]) {
                logf("[modmesh]   sec%02u: VANILLA (not replaced)", unsigned(si));
                continue;
            }
            const BuiltSection &b = *out[si];
            std::string tex;
            for (const std::string &t : b.textureCandidates) {
                if (!tex.empty()) tex += ", ";
                tex += t;
            }
            const char *what = b.forceWhite   ? "WHITE"
                             : b.hide         ? "HIDDEN"
                             : b.keepGeometry ? "KEEP-GEOM"
                                              : "replaced";
            logf("[modmesh]   sec%02u: %-9s verts=%u tris=%u src=\"%s\" "
                 "tex=[%s]%s", unsigned(si), what,
                 unsigned(b.vertices.size() / 16),
                 unsigned(b.indices.size() / 3), b.source.c_str(),
                 tex.c_str(), b.texExclusive ? " PINNED" : "");
        }
        std::set<std::string> stems;
        for (const auto &m : sc.matTexStem)  if (!m.second.empty()) stems.insert(m.second);
        for (const auto &e : sc.embeddedTex) if (!e.first.empty())  stems.insert(e.first);
        std::string all;
        for (const std::string &s : stems) {
            if (!all.empty()) all += ", ";
            all += s;
        }
        logf("[modmesh] texture stems present in the file: [%s]", all.c_str());
        logf("[modmesh] --- end section map ---");
    }
    return out;
}

} // namespace modmesh