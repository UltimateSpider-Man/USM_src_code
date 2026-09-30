#pragma once

// Validated, per-control-point reconstructed skin for an UNRIGGED FBX.
// This is render metadata, never a replacement PCSKEL/PCANIM or gameplay pose.
#include "mod_mesh_retarget.h"
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <set>

namespace modmesh { namespace miles {
namespace rt = retarget;
struct Influence {
    std::array<uint8_t,4> bone{};
    std::array<float,4> weight{};
};
struct Joint {
    std::string name;
    int parent = -1, guide = -1;
    rt::Vec3 position{};
};
inline uint64_t fingerprint(const uint8_t *bytes, size_t size) {
    uint64_t value = UINT64_C(14695981039346656037);
    for (size_t i=0;i<size;++i) value=(value^bytes[i])*UINT64_C(1099511628211);
    return value;
}
inline std::string boneKey(std::string value) {
    if (auto p=value.find('\0');p!=std::string::npos)value.resize(p);
    if (auto p=value.rfind("::");p!=std::string::npos)value.erase(0,p+2);
    for (auto &c:value) if(c>='A'&&c<='Z')c=char(c-'A'+'a');
    value.erase(std::remove_if(value.begin(),value.end(),[](char c){
        return c==' '||c=='_'||c=='-';}),value.end());
    if(value.compare(0,5,"bip01")==0)value.erase(0,5);
    if(value=="ltoe0"||value=="rtoe0")value.pop_back();
    return value;
}
struct Skin {
    uint64_t sourceFingerprint=0;
    float yaw=0, scale=1;
    rt::Vec3 offset{};
    std::vector<Joint> joints;
    std::vector<Influence> vertices;

    bool parse(const std::vector<uint8_t> &bytes,uint64_t actualFingerprint,
               std::string *why=nullptr) {
        auto fail=[&](const char *s){return rt::fail(why,s);};
        constexpr size_t header=52, jointStride=52, vertexStride=20;
        if(bytes.size()<header+8||bytes.size()>8u*1024u*1024u||
           std::memcmp(bytes.data(),"USMSKN01",8))return fail("invalid Miles skin header/size");
        auto u32=[&](size_t p){return uint32_t(bytes[p])|(uint32_t(bytes[p+1])<<8)|
            (uint32_t(bytes[p+2])<<16)|(uint32_t(bytes[p+3])<<24);};
        auto u64=[&](size_t p){return uint64_t(u32(p))|(uint64_t(u32(p+4))<<32);};
        auto f32=[&](size_t p){uint32_t bits=u32(p);float f;std::memcpy(&f,&bits,4);return f;};
        const auto count=u32(12), bones=u32(16);
        if(u32(8)!=1||u32(20)!=0||!count||count>300000||!bones||bones>26)
            return fail("unsupported Miles skin version/counts");
        const size_t expected=header+size_t(bones)*jointStride+size_t(count)*vertexStride+8;
        if(bytes.size()!=expected)return fail("truncated or trailing Miles skin data");
        if(u64(24)!=actualFingerprint)return fail("Miles skin does not match this exact FBX");
        if(u64(expected-8)!=fingerprint(bytes.data(),expected-8))return fail("Miles skin checksum mismatch");
        Skin parsed;parsed.sourceFingerprint=actualFingerprint;
        parsed.yaw=f32(32);parsed.scale=f32(36);
        for(unsigned d=0;d<3;++d)parsed.offset[d]=f32(40+d*4);
        if(!std::isfinite(parsed.yaw)||std::abs(parsed.yaw)>360||
           !std::isfinite(parsed.scale)||parsed.scale<=0||parsed.scale>10)
            return fail("invalid Miles model transform");
        for(float v:parsed.offset)if(!std::isfinite(v)||std::abs(v)>100)
            return fail("invalid Miles model offset");
        std::set<std::string> names;
        for(uint32_t j=0;j<bones;++j) {
            const size_t p=header+j*jointStride;
            const char *name=reinterpret_cast<const char *>(bytes.data()+p);
            const char *end=static_cast<const char *>(std::memchr(name,0,32));
            if(!end||end==name)return fail("invalid Miles joint name");
            Joint joint;joint.name.assign(name,end);
            for(unsigned char c:joint.name)if(c<32||c>126)return fail("invalid Miles joint name bytes");
            if(!names.insert(boneKey(joint.name)).second)return fail("duplicate Miles joint name");
            const uint32_t parent=u32(p+32),guide=u32(p+36);
            if((j==0&&parent!=UINT32_MAX)||(j>0&&parent>=j))return fail("invalid Miles parent order");
            if(guide!=UINT32_MAX&&(guide>=bones||guide==j))return fail("invalid Miles orientation guide");
            joint.parent=parent==UINT32_MAX?-1:int(parent);
            joint.guide=guide==UINT32_MAX?-1:int(guide);
            for(unsigned d=0;d<3;++d) {
                joint.position[d]=f32(p+40+d*4);
                if(!std::isfinite(joint.position[d])||std::abs(joint.position[d])>20)
                    return fail("invalid Miles bind pivot");
            }
            parsed.joints.push_back(std::move(joint));
        }
        if(boneKey(parsed.joints[0].name)!="pelvis")return fail("Miles root must map to pelvis");
        for(float v:parsed.joints[0].position)if(std::abs(v)>1e-6f)
            return fail("Miles root pivot must be model origin");
        parsed.vertices.resize(count);
        for(uint32_t v=0;v<count;++v) {
            const size_t p=header+size_t(bones)*jointStride+size_t(v)*vertexStride;
            auto &out=parsed.vertices[v];double total=0;
            for(unsigned lane=0;lane<4;++lane) {
                out.bone[lane]=bytes[p+lane];out.weight[lane]=f32(p+4+lane*4);
                if(out.bone[lane]>=bones||!std::isfinite(out.weight[lane])||
                   out.weight[lane]<0||out.weight[lane]>1)return fail("invalid Miles skin influence");
                for(unsigned prior=0;prior<lane;++prior)
                    if(out.weight[lane]>0&&out.weight[prior]>0&&out.bone[prior]==out.bone[lane])
                        return fail("duplicate Miles skin influence");
                total+=out.weight[lane];
            }
            if(std::abs(total-1)>2e-6)return fail("Miles skin weights are not normalized");
        }
        *this=std::move(parsed);if(why)why->clear();return true;
    }
};

// Shortest proper row-vector rotation. Antiparallel inputs choose a stable
// perpendicular axis instead of generating a zero/NaN quaternion.
inline bool directionRotation(rt::Vec3 from,rt::Vec3 to,rt::Matrix &out) {
    double a[3],b[3],la=0,lb=0;
    for(int d=0;d<3;++d){a[d]=from[d];b[d]=to[d];la+=a[d]*a[d];lb+=b[d]*b[d];}
    if(!std::isfinite(la)||!std::isfinite(lb)||la<1e-12||lb<1e-12)return false;
    for(int d=0;d<3;++d){a[d]/=std::sqrt(la);b[d]/=std::sqrt(lb);}
    double c=std::clamp(a[0]*b[0]+a[1]*b[1]+a[2]*b[2],-1.0,1.0);
    double axis[3]={a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]};
    double s=std::sqrt(axis[0]*axis[0]+axis[1]*axis[1]+axis[2]*axis[2]);
    out=rt::identity();
    if(s<1e-10) {
        if(c>0)return true;
        int k=0;for(int d=1;d<3;++d)if(std::abs(a[d])<std::abs(a[k]))k=d;
        double e[3]={0,0,0};e[k]=1;
        axis[0]=a[1]*e[2]-a[2]*e[1];axis[1]=a[2]*e[0]-a[0]*e[2];axis[2]=a[0]*e[1]-a[1]*e[0];
        const double n=std::sqrt(axis[0]*axis[0]+axis[1]*axis[1]+axis[2]*axis[2]);
        for(double &v:axis)v/=n;s=0;
    } else for(double &v:axis)v/=s;
    for(int row=0;row<3;++row) {
        double e[3]={0,0,0};e[row]=1;
        const double cross[3]={axis[1]*e[2]-axis[2]*e[1],axis[2]*e[0]-axis[0]*e[2],axis[0]*e[1]-axis[1]*e[0]};
        for(int col=0;col<3;++col)out[row*4+col]=float(c*e[col]+s*cross[col]+(1-c)*axis[row]*axis[col]);
    }
    return rt::affine(out);
}

inline bool prepare(const Skin &skin,const std::vector<std::string> &targetNames,
                    const std::vector<rt::Matrix> &targetBind,rt::Plan &out,
                    std::string *why=nullptr) {
    if(skin.joints.empty()||targetNames.size()!=targetBind.size())
        return rt::fail(why,"missing validated native Miles target metadata");
    std::vector<int> map,parents;
    for(const auto &joint:skin.joints) {
        int found=-1;
        for(size_t j=0;j<targetNames.size();++j)if(boneKey(targetNames[j])==boneKey(joint.name)) {
            if(found>=0)return rt::fail(why,"ambiguous native Miles target joint");found=int(j);
        }
        if(found<0)return rt::fail(why,"required native Miles target joint is missing");
        map.push_back(found);parents.push_back(joint.parent);
    }
    std::vector<rt::Matrix> source(skin.joints.size()),correction(skin.joints.size(),rt::identity());
    for(size_t j=0;j<skin.joints.size();++j) {
        const auto &joint=skin.joints[j];
        if(joint.guide>=0) {
            const size_t g=size_t(joint.guide);rt::Vec3 a{},b{};
            for(int d=0;d<3;++d){a[d]=targetBind[size_t(map[g])][12+d]-targetBind[size_t(map[j])][12+d];b[d]=skin.joints[g].position[d]-joint.position[d];}
            if(!directionRotation(a,b,correction[j]))return rt::fail(why,"coincident Miles joint guide");
        } else if(joint.parent>=0)correction[j]=correction[size_t(joint.parent)];
        source[j]=rt::multiply(targetBind[size_t(map[j])],correction[j]);
        for(int d=0;d<3;++d)source[j][12+d]=joint.position[d];
    }
    rt::Plan plan;
    if(!rt::prepare(source,parents,map,targetBind,plan,why))return false;
    // The input is a posed scan, not a T-pose. Use native absolute bone axes
    // with reconstructed limb lengths; applying native rest deltas to this
    // lowered-arm pose would lower its arms a second time.
    plan.absoluteBoneAxes=true;
    out=std::move(plan);if(why)why->clear();return true;
}
// The caller supplies a PRIVATE Scene copy. Shared loadScene() cache entries
// and every native section view remain unchanged. Used by the runtime and
// the standalone asset regression probe through the same importer seam.
template<class Scene, class Ref>
bool install(const Skin &skin, Scene &scene, Ref &ref, std::string *why=nullptr) {
    if (scene.meshModelOrder.size()!=1 || scene.geoms.size()!=1 || !scene.anims.empty())
        return rt::fail(why,"unexpected posed-scan scene structure");
    const auto model=scene.models.find(scene.meshModelOrder.front());
    if(model==scene.models.end() || model->second.geoms.size()!=1)
        return rt::fail(why,"missing posed-scan mesh model");
    const auto geometry=scene.geoms.find(model->second.geoms.front());
    if(geometry==scene.geoms.end() || !geometry->second.clusters.empty() ||
       geometry->second.ctrl.size()!=skin.vertices.size()*3 || skin.joints.empty())
        return rt::fail(why,"posed-scan skin/geometry mismatch");
    scene.cfg.custom=true; scene.cfg.skin=1; scene.cfg.fit=false;
    scene.cfg.anim=false; scene.cfg.retarget=true;
    scene.cfg.scale=skin.scale; scene.cfg.yaw=skin.yaw;
    scene.cfg.offset={skin.offset[0],skin.offset[1],skin.offset[2]};
    scene.cfg.donorPose.clear(); scene.cfg.donorArmPitchSet=false;
    scene.cfg.roundtripEps=0;
    scene.cfg.keepSections.clear(); scene.cfg.hideSections.clear();
    if(scene.cfg.tex==1)scene.cfg.tex=0;
    auto &clusters=geometry->second.clusters;
    clusters.resize(skin.joints.size());
    ref.clusterBoneIndices.clear();ref.boneNames.clear();ref.boneParents.clear();ref.bonePos.clear();
    for(size_t bone=0;bone<skin.joints.size();++bone) {
        const auto &joint=skin.joints[bone];
        clusters[bone].boneName="miles_profile_"+joint.name;
        ref.clusterBoneIndices[clusters[bone].boneName]=int(bone);
        ref.boneNames.push_back(joint.name);ref.boneParents.push_back(joint.parent);
        ref.bonePos.insert(ref.bonePos.end(),joint.position.begin(),joint.position.end());
    }
    for(size_t v=0;v<skin.vertices.size();++v)for(size_t lane=0;lane<4;++lane) {
        const auto &row=skin.vertices[v];
        if(row.weight[lane]>0) {
            clusters[row.bone[lane]].idx.push_back(int64_t(v));
            clusters[row.bone[lane]].w.push_back(row.weight[lane]);
        }
    }
    ref.nbones=int(skin.joints.size());ref.customSource=true;ref.rejectImportedSkin=false;
    ref.haveBones=true;ref.donorPoseMetadataValidated=false;
    for(int d=0;d<3;++d) {
        ref.bonesMin[d]=ref.bonesMax[d]=ref.bonePos[d];
        for(size_t j=1;j<skin.joints.size();++j) {
            ref.bonesMin[d]=std::min(ref.bonesMin[d],ref.bonePos[j*3+d]);
            ref.bonesMax[d]=std::max(ref.bonesMax[d],ref.bonePos[j*3+d]);
        }
    }
    if(why)why->clear();return true;
}
}} // namespace modmesh::miles
