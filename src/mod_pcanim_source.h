#pragma once

// Pristine retail PC animation banks. Native loading replaces serialized
// vtables/Skeleton/instance-count words; those words are never file offsets.
// The container links and the validated character component directories are
// relative. Generic and pedestrian payloads are decoded by their native NAL
// families; this parser does not claim to validate compressed bitstreams.
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace modanim {
inline bool fail(std::string *why, const char *message) {
    if (why) *why = message;
    return false;
}
inline uint32_t u32(const std::vector<uint8_t> &b, size_t p) {
    uint32_t v; std::memcpy(&v, b.data()+p, 4); return v;
}
inline void put32(std::vector<uint8_t> &b, size_t p, uint32_t v) {
    std::memcpy(b.data()+p, &v, 4);
}
inline bool range(size_t p, size_t n, size_t size) { return p <= size && n <= size-p; }
inline bool alignment(uint32_t n) { return n && n <= 16 && !(n & (n-1)); }
inline bool characterVersion(uint32_t version) { return version==0x10002 || version==0x10003; }

struct Clip {
    uint32_t hash = 0, version = 0, skeletonHash = 0, skeletonIndex = 0;
    uint32_t start = 0, end = 0;
    uint32_t animComponentCount = 0, poseComponentCount = 0;
    std::string name;
    float duration = 0;
};

class Source {
    static bool fixedName(const std::vector<uint8_t>& b, size_t p, std::string& out) {
        if (!range(p, 32, b.size()) || !u32(b,p)) return false;
        size_t n=0;
        while (n<28 && b[p+4+n]) {
            if (b[p+4+n]<0x20 || b[p+4+n]>0x7e) return false;
            ++n;
        }
        if (!n || n==28) return false;
        out.assign(reinterpret_cast<const char*>(b.data()+p+4),n);
        return true;
    }
    static bool directory(const std::vector<uint8_t>& b, const Clip& c,
                          uint32_t relative, std::string* why) {
        const size_t size=c.end-c.start;
        if (relative<0x58 || (relative&3) || !range(relative,4,size))
            return fail(why,"invalid character component directory");
        const uint32_t count=u32(b,c.start+relative);
        if (count>4096 || !range(relative,size_t(count+1)*4,size))
            return fail(why,"invalid character component count");
        for (uint32_t j=0;j<count;++j) {
            const uint32_t offset=u32(b,c.start+relative+4+j*4);
            // Entropy/bitstream component payloads need not be word aligned.
            if (offset<size_t(count+1)*4 || offset>=size-relative)
                return fail(why,"character component payload leaves its clip");
        }
        return true;
    }
    static bool payload(const std::vector<uint8_t>& b, Clip& c, std::string* why) {
        const size_t size=c.end-c.start, p=c.start;
        if (characterVersion(c.version)) {
            if (size<0x58) return fail(why,"truncated character clip");
            const uint32_t flags=u32(b,p+0x40), anim=u32(b,p+0x44), pose=u32(b,p+0x48);
            // 0x58 and 0x60 are both present in pristine retail clips. A flags
            // pointer into the fixed clip name is a damaged input, not a form.
            if (flags<0x58 || (flags&3) || !range(flags,4,size) || flags>anim)
                return fail(why,"invalid character component flags offset");
            if (!directory(b,c,anim,why) || !directory(b,c,pose,why)) return false;
            c.animComponentCount=u32(b,p+anim);
            c.poseComponentCount=u32(b,p+pose);
            return true;
        }
        if (c.version==0x10200) {
            if (size<0x84) return fail(why,"truncated generic clip");
            const uint32_t frames=u32(b,p+0x44), channels=u32(b,p+0x48);
            const uint32_t staticBytes=u32(b,p+0x54), blocks=u32(b,p+0x64);
            if (!frames || frames>0x1000000 || !channels || channels>65536 ||
                !alignment(u32(b,p+0x50)) || !alignment(u32(b,p+0x58)) ||
                !alignment(u32(b,p+0x74)) || !blocks || blocks>65536 ||
                staticBytes>size-0x80 || size_t(blocks)*4>size-0x80-staticBytes ||
                u32(b,p+0x70)>=size)
                return fail(why,"invalid generic clip layout");
            // Process at 0x7939C0 obtains the channel-mask length from the
            // referenced skeleton. Full generic block decoding needs that
            // skeleton; copying the complete aligned clip preserves it.
            return true;
        }
        if (c.version==0x500) {
            if (size<0x70 || !u32(b,p+0x60) || u32(b,p+0x60)>0x1000000 ||
                !range(0x70,u32(b,p+0x6c),size))
                return fail(why,"invalid pedestrian clip layout");
            return true;
        }
        return fail(why,"unsupported native animation clip version");
    }
public:
    std::vector<uint8_t> bytes;
    std::vector<Clip> clips;
    std::vector<uint32_t> skeletonHashes;
    std::vector<std::string> skeletonNames;
    uint32_t firstClipOffset = 0;

    const Clip* findClip(uint32_t hash) const {
        for (const auto &clip:clips) if (clip.hash==hash) return &clip;
        return nullptr;
    }
    bool parse(std::vector<uint8_t> input, std::string* why=nullptr) {
        if (input.size()<0x70 || input.size()>512u*1024u*1024u)
            return fail(why,"invalid animation bank size");
        if (u32(input,0)!=0x10101 || (u32(input,4)&8))
            return fail(why,"unsupported or already rebased animation bank");
        const uint32_t count=u32(input,12);
        if (!count || count>128 || u32(input,8)!=count*32 ||
            !range(0x48,size_t(count)*32,input.size()))
            return fail(why,"invalid animation skeleton table");
        Source parsed; std::string fileName;
        if (!fixedName(input,0x10,fileName)) return fail(why,"invalid animation bank name");
        std::set<uint32_t> skeletons, names;
        for (uint32_t j=0;j<count;++j) {
            std::string name;const size_t p=0x48+j*32;
            if (!fixedName(input,p,name) || !skeletons.insert(u32(input,p)).second)
                return fail(why,"invalid or duplicate animation skeleton identity");
            parsed.skeletonHashes.push_back(u32(input,p));
            parsed.skeletonNames.push_back(std::move(name));
        }
        parsed.firstClipOffset=u32(input,0x34);
        if (parsed.firstClipOffset!=((0x48+count*32+15)&~15u))
            return fail(why,"unexpected first animation offset");
        uint32_t p=parsed.firstClipOffset;
        for (;;) {
            if ((p&15) || !range(p,0x40,input.size()) || parsed.clips.size()>=65536)
                return fail(why,"invalid animation clip chain");
            Clip c;c.start=p;c.hash=u32(input,p+8);c.version=u32(input,p+0x2c);
            c.skeletonIndex=u32(input,p+0x28);
            if (!fixedName(input,p+8,c.name) || !names.insert(c.hash).second || c.skeletonIndex>=count)
                return fail(why,"invalid or duplicate clip identity/skeleton");
            c.skeletonHash=parsed.skeletonHashes[c.skeletonIndex];
            std::memcpy(&c.duration,input.data()+p+0x38,4);
            if (!std::isfinite(c.duration) || c.duration<=0)
                return fail(why,"invalid animation duration");
            const uint32_t next=u32(input,p+4);
            if (next && ((next&15) || next<0x40 || !range(p,next+size_t(0x40),input.size())))
                return fail(why,"invalid relative animation link");
            c.end=next?p+next:uint32_t(input.size());
            if (!payload(input,c,why)) return false;
            parsed.clips.push_back(std::move(c));
            if (!next) break;
            p+=next;
        }
        parsed.bytes=std::move(input);
        *this=std::move(parsed);
        if (why) why->clear();
        return true;
    }
};

inline bool compatibleClip(const Clip& source,const Clip& target,std::string* why=nullptr) {
    if (source.skeletonHash!=target.skeletonHash)
        return fail(why,"replacement clip skeleton differs");
    if (source.version==target.version) return true;
    // Installed retail banks can mix character v2 and v3 on one skeleton.
    // Preserve each source payload/version; only these two native character
    // forms may cross versions, with matching component directory counts.
    if (!characterVersion(source.version) || !characterVersion(target.version))
        return fail(why,"replacement clip version differs");
    if (source.animComponentCount!=target.animComponentCount ||
        source.poseComponentCount!=target.poseComponentCount)
        return fail(why,"cross-version character component counts differ");
    return true;
}

inline bool compatibleBank(const Source& source,const Source& target,std::string* why=nullptr) {
    if (source.bytes.empty() || target.bytes.empty() || source.clips.empty() || target.clips.empty())
        return fail(why,"missing parsed animation bank");
    if (source.skeletonHashes!=target.skeletonHashes)
        return fail(why,"ordered animation skeleton contract differs");
    for (const auto& original:target.clips) {
        const auto* replacement=source.findClip(original.hash);
        if (!replacement) return fail(why,"replacement bank omits a required clip");
        if (!compatibleClip(*replacement,original,why)) return false;
    }
    if (why) why->clear();
    return true;
}

struct ClipSwap { uint32_t targetHash=0; const Source* source=nullptr; uint32_t sourceHash=0; };

inline bool replaceClips(const Source& target,const std::vector<ClipSwap>& swaps,
                         std::vector<uint8_t>& out,std::string* why=nullptr) {
    if (target.bytes.empty() || target.clips.empty()) return fail(why,"missing target animation bank");
    if (!range(0,target.firstClipOffset,target.bytes.size()))
        return fail(why,"invalid target animation header range");
    for (const auto& clip:target.clips)
        if (clip.end<clip.start || !range(clip.start,clip.end-clip.start,target.bytes.size()) ||
            !range(clip.start,0x40,target.bytes.size()))
            return fail(why,"invalid target animation clip range");
    std::map<uint32_t,std::pair<const Source*,const Clip*>> selected;
    for (const auto& swap:swaps) {
        const auto* original=target.findClip(swap.targetHash);
        const auto* replacement=swap.source?swap.source->findClip(swap.sourceHash):nullptr;
        if (!original || !replacement || !selected.emplace(swap.targetHash,std::make_pair(swap.source,replacement)).second)
            return fail(why,"unknown or duplicate animation clip swap");
        if (!compatibleClip(*replacement,*original,why)) return false;
    }
    if (swaps.empty()) { out=target.bytes;if(why)why->clear();return true; }
    std::vector<uint8_t> result(target.bytes.begin(),target.bytes.begin()+target.firstClipOffset);
    uint32_t previous=0;
    for (const auto& original:target.clips) {
        const auto found=selected.find(original.hash);
        const Source* file=found==selected.end()?&target:found->second.first;
        const Clip* clip=found==selected.end()?&original:found->second.second;
        if (clip->end<clip->start || !range(clip->start,clip->end-clip->start,file->bytes.size()))
            return fail(why,"invalid parsed animation clip range");
        const size_t aligned=(result.size()+15)&~size_t(15),length=clip->end-clip->start;
        if (aligned>512u*1024u*1024u || length>512u*1024u*1024u-aligned)
            return fail(why,"replacement animation bank is too large");
        result.resize(aligned,0);const uint32_t start=uint32_t(aligned);
        result.insert(result.end(),file->bytes.begin()+clip->start,file->bytes.begin()+clip->end);
        if (previous) put32(result,previous+4,start-previous);
        put32(result,start+4,0);
        std::copy_n(target.bytes.data()+original.start+8,32,result.data()+start+8);
        put32(result,start+0x28,original.skeletonIndex);
        previous=start;
    }
    Source verified;
    if (!verified.parse(result,why) || !compatibleBank(verified,target,why)) return false;
    out=std::move(result);
    if (why)why->clear();
    return true;
}
} // namespace modanim
