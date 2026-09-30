// Public import/skin regression. No arguments runs portable malformed-profile
// and rig invariants. game-dir [profile-path] additionally validates the exact
// supplied Miles asset and writes pose metrics/point clouds under build-static-audit/miles.
#include "../src/mod_mesh_import.h"
#include "../src/mod_miles_skin.h"
#include "../src/mod_pcmesh_archive.h"
#include "../src/mod_pcmesh_source.h"
#include "../src/mod_pcskel_source.h"
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <unordered_map>

namespace {
namespace rt=modmesh::retarget;
namespace ms=modmesh::miles;
using Bytes=std::vector<uint8_t>;
using Point=rt::Vec3;
void check(bool value,const std::string &why) {if(!value)throw std::runtime_error(why);}
Bytes read(const std::string &path) {
    std::ifstream in(path,std::ios::binary);check(bool(in),"cannot read "+path);
    return {std::istreambuf_iterator<char>(in),{}};
}
void u32(Bytes &b,size_t p,uint32_t value) {for(int i=0;i<4;++i)b[p+i]=uint8_t(value>>(i*8));}
void u64(Bytes &b,size_t p,uint64_t value) {u32(b,p,uint32_t(value));u32(b,p+4,uint32_t(value>>32));}
void f32(Bytes &b,size_t p,float value) {uint32_t bits;std::memcpy(&bits,&value,4);u32(b,p,bits);}
void checksum(Bytes &b) {u64(b,b.size()-8,ms::fingerprint(b.data(),b.size()-8));}
Bytes fixture() {
    Bytes b(52+4*52+3*20+8);std::memcpy(b.data(),"USMSKN01",8);
    u32(b,8,1);u32(b,12,3);u32(b,16,4);u64(b,24,12345);f32(b,36,1);
    const char *names[]={"PELVIS","SPINE","L_UPPERARM","L_FOREARM"};
    const Point points[]={{{0,0,0}},{{0,1,0}},{{-1,1,0}},{{-1.2f,.2f,.1f}}};
    for(size_t i=0;i<4;++i) {
        const size_t p=52+i*52;std::memcpy(b.data()+p,names[i],std::strlen(names[i]));
        u32(b,p+32,i?uint32_t(i-1):UINT32_MAX);u32(b,p+36,i<3?uint32_t(i+1):UINT32_MAX);
        for(size_t d=0;d<3;++d)f32(b,p+40+d*4,points[i][d]);
    }
    for(size_t i=0;i<3;++i) {const size_t p=52+4*52+i*20;b[p]=uint8_t(i);b[p+1]=uint8_t(i+1);f32(b,p+4,.75f);f32(b,p+8,.25f);}
    checksum(b);return b;
}
double distance(const Point &a,const Point &b) {
    double value=0;for(size_t d=0;d<3;++d)value+=(double(a[d])-b[d])*(double(a[d])-b[d]);return std::sqrt(value);
}
std::vector<rt::Matrix> sourcePose(const rt::Plan &plan) {
    auto target=plan.targetBind;
    for(size_t i=0;i<plan.sourceBind.size();++i)target.at(size_t(plan.targetBone[i]))=plan.sourceBind[i];
    std::vector<rt::Matrix> pose;std::string why;
    check(rt::evaluate(plan,target.data(),target.size(),pose,&why),"source reconstruction: "+why);
    for(size_t i=0;i<pose.size();++i)for(size_t d=0;d<16;++d)
        check(std::abs(pose[i][d]-plan.sourceBind[i][d])<3e-5,"source frame reconstruction differs from bind");
    return pose;
}
void synthetic() {
    std::string why;const auto valid=fixture();ms::Skin skin;
    check(skin.parse(valid,12345,&why),why);const auto before=skin.vertices;
    auto reject=[&](Bytes bad,const char *message,bool fixChecksum=true,uint64_t hash=12345) {
        if(fixChecksum)checksum(bad);
        check(!skin.parse(bad,hash,&why),std::string("accepted ")+message);
        check(skin.vertices.size()==before.size()&&skin.vertices[1].weight==before[1].weight,"failed parse changed valid skin");
    };
    reject(valid,"wrong FBX",false,12346);
    auto bad=valid;bad[40]^=1;reject(bad,"checksum",false);
    bad=valid;bad.pop_back();reject(bad,"truncated profile",false);
    bad=valid;bad.push_back(0);reject(bad,"trailing data",false);
    bad=valid;u32(bad,52+52+32,2);reject(bad,"parent cycle");
    bad=valid;f32(bad,52+40,.01f);reject(bad,"nonzero root");
    bad=valid;f32(bad,52+4*52+4,std::numeric_limits<float>::quiet_NaN());reject(bad,"NaN weight");
    bad=valid;f32(bad,52+4*52+4,.9f);reject(bad,"unnormalized skin");
    bad=valid;bad[52+4*52+1]=0;reject(bad,"duplicate influences");
    bad=valid;bad[52+4*52]=4;reject(bad,"palette overflow");
    std::vector<std::string> names;std::vector<rt::Matrix> bind;
    for(size_t i=0;i<skin.joints.size();++i) {
        names.push_back(skin.joints[i].name);auto m=rt::identity();
        const Point p=i==3?Point{{-2,1,0}}:skin.joints[i].position;
        for(size_t d=0;d<3;++d)m[12+d]=p[d];bind.push_back(m);
    }
    rt::Plan plan;check(ms::prepare(skin,names,bind,plan,&why),why);
    check(plan.absoluteBoneAxes,"posed scan plan lost absolute axes");sourcePose(plan);
    const auto untouched=bind;std::vector<rt::Matrix> pose;
    check(rt::evaluate(plan,bind.data(),bind.size(),pose,&why),why);check(bind==untouched,"evaluate wrote native pose");
    rt::Matrix rotation;
    check(ms::directionRotation({{0,1,0}},{{0,-1,0}},rotation),"antiparallel rotation rejected");
    check(distance(rt::vector({{0,1,0}},rotation),{{0,-1,0}})<1e-6,"antiparallel rotation incorrect");
    names[3]="missing";check(!ms::prepare(skin,names,bind,plan,&why),"missing native joint accepted");
    std::cout<<"PASS Miles profile parser rejection and source/native frame invariants\n";
}

struct Key {
    std::array<int64_t,3> value{};
    bool operator==(const Key &other)const{return value==other.value;}
};
struct Hash {size_t operator()(const Key &k)const{return size_t(uint64_t(k.value[0])*73856093u^uint64_t(k.value[1])*19349663u^uint64_t(k.value[2])*83492791u);}};
Key cell(const Point &p) {Key k;for(size_t d=0;d<3;++d)k.value[d]=int64_t(std::floor(double(p[d])*100000));return k;}
struct SourcePoints {
    std::vector<Point> point;
    std::unordered_map<Key,std::vector<size_t>,Hash> cells;
    SourcePoints(const modmesh::Scene &scene,const ms::Skin &skin) {
        const auto &model=scene.models.at(scene.meshModelOrder.front());
        const auto &geometry=scene.geoms.at(model.geoms.front());
        const auto world=modmesh::detail::nodeGlobal(scene,model)*modmesh::detail::geometricXf(model);
        const double angle=skin.yaw*3.14159265358979323846/180,c=std::cos(angle),s=std::sin(angle);
        for(size_t v=0;v<geometry.ctrl.size();v+=3) {
            const auto p=world.point({geometry.ctrl[v],geometry.ctrl[v+1],geometry.ctrl[v+2]});
            // Import emits float corners before applying the sidecar transform.
            const double x=float(p.x),y=float(p.y),z=float(p.z);
            const Point value{{float(skin.scale*(c*x+s*z)+skin.offset[0]),float(skin.scale*y+skin.offset[1]),float(skin.scale*(-s*x+c*z)+skin.offset[2])}};
            cells[cell(value)].push_back(point.size());point.push_back(value);
        }
    }
    bool match(const Point &p,const std::vector<double> &weights,const ms::Skin &skin,double &error)const {
        const auto key=cell(p);
        for(int x=-1;x<=1;++x)for(int y=-1;y<=1;++y)for(int z=-1;z<=1;++z) {
            auto k=key;k.value[0]+=x;k.value[1]+=y;k.value[2]+=z;
            const auto found=cells.find(k);if(found==cells.end())continue;
            for(size_t v:found->second) {
                const double delta=distance(p,point[v]);if(delta>2e-6)continue;
                auto actual=weights;
                for(size_t lane=0;lane<4;++lane)actual[skin.vertices[v].bone[lane]]-=skin.vertices[v].weight[lane];
                double mismatch=0;for(double value:actual)mismatch+=std::abs(value);
                if(mismatch<3e-5){error=std::max(error,delta);return true;}
            }
        }
        return false;
    }
};
struct Bounds {
    Point lo{{1e20f,1e20f,1e20f}},hi{{-1e20f,-1e20f,-1e20f}};
    void add(const Point &p) {for(size_t d=0;d<3;++d){check(std::isfinite(p[d]),"nonfinite posed vertex");lo[d]=std::min(lo[d],p[d]);hi[d]=std::max(hi[d],p[d]);}}
    double diagonal()const{return distance(lo,hi);}
};
struct Metrics {
    struct Edge {double before,after;Point a,b;std::string material;};
    std::string name;
    Bounds bounds;
    std::vector<double> edgeRatios;
    std::vector<Edge> stretched;
    double maxError=0,maxEdge=0;
    explicit Metrics(const std::string &value):name(value){}
    void edges(const std::vector<Point> &original,const std::vector<Point> &posed,const std::vector<uint32_t> &indices,const std::string &material) {
        for(size_t i=0;i<indices.size();i+=3)for(size_t e=0;e<3;++e) {
            const size_t a=indices[i+e],b=indices[i+(e+1)%3];check(a<original.size()&&b<original.size(),"triangle index overflow");
            const double before=distance(original[a],original[b]),after=distance(posed[a],posed[b]);
            maxEdge=std::max(maxEdge,after);
            if(before>1e-5)edgeRatios.push_back(after/before);
            if(before>1e-5&&after/before>10&&after>.005)stretched.push_back({before,after,original[a],original[b],material});
        }
    }
    void report(std::ostream &out,const std::string &mesh) {
        check(!edgeRatios.empty(),"no measurable triangle edges");std::sort(edgeRatios.begin(),edgeRatios.end());
        out<<mesh<<','<<name<<','<<bounds.lo[0]<<','<<bounds.lo[1]<<','<<bounds.lo[2]<<','<<bounds.hi[0]<<','<<bounds.hi[1]<<','<<bounds.hi[2]
           <<','<<edgeRatios[size_t(.95*(edgeRatios.size()-1))]<<','<<edgeRatios[size_t(.99*(edgeRatios.size()-1))]<<','<<edgeRatios.back()<<','<<maxEdge<<','<<maxError<<'\n';
        // Distribution is diagnostic: a disconnected tiny surface can have a
        // large ratio without a spike. Reject catastrophic global expansion;
        // report p95/p99/max so visual quality is not inferred from finiteness.
        check(bounds.diagonal()<6,"posed mesh expanded beyond six game units");
        check(maxEdge<2,"posed mesh contains a triangle edge longer than two game units");
    }
    void worst(std::ostream &out) {
        std::sort(stretched.begin(),stretched.end(),[](const Edge &a,const Edge &b){return a.after>b.after;});
        for(size_t i=0;i<std::min<size_t>(100,stretched.size());++i) {
            const auto &edge=stretched[i];out<<name<<','<<edge.material<<','<<edge.before<<','<<edge.after<<','<<edge.after/edge.before;
            for(float value:edge.a)out<<','<<value;for(float value:edge.b)out<<','<<value;out<<'\n';
        }
    }
};
rt::Matrix rotate(unsigned axis,double degrees) {
    auto result=rt::identity();const double a=degrees*3.14159265358979323846/180,c=std::cos(a),s=std::sin(a);
    const unsigned j=(axis+1)%3,k=(axis+2)%3;
    result[j*4+j]=result[k*4+k]=float(c);result[j*4+k]=float(s);result[k*4+j]=float(-s);return result;
}
std::vector<rt::Matrix> bentPose(const std::vector<rt::Matrix> &bind,const std::vector<std::string> &names,const std::vector<int> &parents) {
    std::vector<rt::Matrix> result(bind.size());std::vector<unsigned char> state(bind.size());
    std::function<void(size_t)> visit=[&](size_t j) {
        if(state[j]==2)return;check(state[j]==0,"native parent cycle");state[j]=1;
        const int parent=parents[j];auto local=bind[j];
        if(parent>=0) {check(size_t(parent)<bind.size(),"native parent outside bind");visit(size_t(parent));rt::Matrix inverse;check(rt::inverse(bind[size_t(parent)],inverse),"invalid native parent bind");local=rt::multiply(bind[j],inverse);}
        const auto name=ms::boneKey(names[j]);unsigned axis=2;double angle=0;
        if(name=="lupperarm")angle=30;if(name=="rupperarm")angle=-30;
        if(name=="lforearm")angle=65;if(name=="rforearm")angle=-65;
        // Native leg local X follows the limb; Z exercises flexion rather
        // than a longitudinal twist, so knees and ankles actually move.
        if(name=="lthigh")angle=25;if(name=="rthigh")angle=-25;
        if(name=="lcalf")angle=-45;if(name=="rcalf")angle=30;
        local=rt::multiply(rotate(axis,angle),local);
        result[j]=parent<0?local:rt::multiply(local,result[size_t(parent)]);state[j]=2;
    };
    for(size_t j=0;j<bind.size();++j)visit(j);return result;
}
void asset(const std::string &root,const std::string &profile) {
    const std::string fbx=root+"/extra/FBX/spiderman_miles_morales.fbx";std::string why;
    const auto fbxBytes=read(fbx);ms::Skin skin;
    check(skin.parse(read(profile),ms::fingerprint(fbxBytes.data(),fbxBytes.size()),&why),"profile: "+why);
    const auto cached=modmesh::loadScene(fbx,fbxBytes.data(),fbxBytes.size());check(bool(cached),"FBX parse failed");
    check(cached->geoms.size()==1&&cached->meshModelOrder.size()==1,"unexpected original scene");
    const auto originalCtrl=cached->geoms.begin()->second.ctrl;
    const auto originalIndices=cached->geoms.begin()->second.pvi;
    check(cached->geoms.begin()->second.clusters.empty(),"original asset is already rigged");
    const auto originalCfg=cached->cfg;const SourcePoints source(*cached,skin);
    check(source.point.size()==skin.vertices.size(),"profile control-point count differs");
    // Characterization of the previously verified separated distal leg
    // surfaces on this exact source FBX. Index+side fingerprint deliberately
    // records anatomical identity, not segment weights; knee/ankle blending
    // may change freely. World-X sign is unsuitable for this leaning scan.
    uint64_t distalIdentity=UINT64_C(14695981039346656037);size_t distalCounts[2]{};
    for(size_t v=0;v<source.point.size();++v)if(source.point[v][1]<-.4f) {
        double sideMass[2]{};
        for(size_t lane=0;lane<4;++lane)if(skin.vertices[v].weight[lane]>0) {
            const auto name=ms::boneKey(skin.joints[skin.vertices[v].bone[lane]].name);
            const bool left=name=="lthigh"||name=="lcalf"||name=="lfoot"||name=="ltoe";
            const bool right=name=="rthigh"||name=="rcalf"||name=="rfoot"||name=="rtoe";
            check(left||right,"distal source leg gained a body/arm influence");
            sideMass[right?1:0]+=skin.vertices[v].weight[lane];
        }
        check(std::min(sideMass[0],sideMass[1])==0,"distal source leg gained an opposite-leg influence");
        const unsigned side=sideMass[1]>sideMass[0]?1:0;++distalCounts[side];
        for(unsigned byte=0;byte<4;++byte)distalIdentity=(distalIdentity^uint8_t(v>>(byte*8)))*UINT64_C(1099511628211);
        distalIdentity=(distalIdentity^side)*UINT64_C(1099511628211);
    }
    check(distalCounts[0]==10483&&distalCounts[1]==9848&&distalIdentity==UINT64_C(8578510170172428112),
          "verified distal anatomical leg identities changed");
    std::cout<<"PASS distal leg identity: "<<distalCounts[0]<<" left / "<<distalCounts[1]<<" right source points; zero opposite-leg influence\n";
    // Independently measured triangle edges that exposed the original abrupt
    // calf/foot, upperarm/forearm and proximal left/right thigh switches.
    // Millimetre-scale source edges must not become 5cm/14cm animation seams.
    constexpr size_t seamCount=4;
    const size_t seamPoints[seamCount][2]={{22757,22995},{62328,62157},{33639,32963},{44816,44481}};
    const char *seamNames[]={"left_ankle","right_elbow","crotch","upper_thigh"};
    const Point seamPositions[seamCount][2]={
        {{{-.0863976479f,-1.01890373f,-.204435661f}},{{-.0896945819f,-1.02141988f,-.203939691f}}},
        {{{.177329451f,.234604165f,.0474893712f}},{{.177085459f,.237511829f,.0456962027f}}},
        {{{-.0253925044f,-.211572886f,.0338806063f}},{{-.028924359f,-.200432569f,.040292874f}}},
        {{{.0335288793f,-.147862166f,-.0935011208f}},{{.0363150947f,-.145296127f,-.0926533341f}}}};
    double maximumSeamJump=0,maximumSeamStretch=0;
    for(size_t edge=0;edge<seamCount;++edge) {
        std::vector<double> delta(skin.joints.size());
        for(size_t end=0;end<2;++end) {
            const size_t v=seamPoints[edge][end];check(v<source.point.size(),"missing measured seam control point");
            check(distance(source.point[v],seamPositions[edge][end])<2e-5,"measured seam point changed source frame");
            for(size_t lane=0;lane<4;++lane)delta[skin.vertices[v].bone[lane]]+=(end?-1:1)*skin.vertices[v].weight[lane];
        }
        double jump=0;for(double value:delta)jump+=std::abs(value);maximumSeamJump=std::max(maximumSeamJump,jump);
        std::cout<<"SEAM "<<seamNames[edge]<<" influence L1 jump "<<jump<<'\n';
    }
    modmesh::pcmeshzip::Archive zip;check(zip.open(root+"/extra/FBX/pcmesh.zip",&why),why);
    Bytes bytes;std::string member;check(zip.extract("ULTIMATE_SPIDERMAN",bytes,&member,&why),why);
    modmesh::pcmeshsource::Source native;check(native.parse(std::move(bytes),&why),why);
    const auto nativeFingerprint=ms::fingerprint(native.bytes.data(),native.bytes.size());
    modmesh::pcskelsource::Source skeleton;check(skeleton.parse(read(root+"/mesh_sources/pcskel/ULTIMATE_SPIDERMAN.PCSKEL"),&why),why);
    const auto selected=native.replacementMeshNames("ULTIMATE_SPIDERMAN");check(selected.size()==3,"expected three native LOD meshes");
    std::filesystem::create_directories("build-static-audit/miles");
    std::ofstream csv("build-static-audit/miles/pose-metrics.csv");check(bool(csv),"cannot create metrics");csv<<std::setprecision(9);
    std::ofstream edgeCsv("build-static-audit/miles/stretched-edges.csv");edgeCsv<<std::setprecision(9);
    edgeCsv<<"pose,material,source_length,posed_length,ratio,ax,ay,az,bx,by,bz\n";
    std::ofstream seamCsv("build-static-audit/miles/measured-seams.csv");seamCsv<<std::setprecision(9);
    seamCsv<<"mesh,seam,pose,source_length,posed_length,stretch\n";
    csv<<"mesh,pose,min_x,min_y,min_z,max_x,max_y,max_z,edge_p95,edge_p99,edge_max,max_edge_length,max_reconstruction_error\n";
    size_t tested=0;
    for(const auto &mesh:native.meshes) {
        if(std::find(selected.begin(),selected.end(),mesh.name)==selected.end())continue;
        check(mesh.boneMatrices.size()==mesh.nbones*16&&skeleton.bones.size()>=mesh.nbones,"native bone metadata incomplete");
        std::vector<std::string> names;std::vector<int> parents;std::vector<rt::Matrix> bind(mesh.nbones);
        for(size_t j=0;j<mesh.nbones;++j) {names.push_back(skeleton.bones[j].name);parents.push_back(skeleton.bones[j].parent);std::copy_n(mesh.boneMatrices.begin()+j*16,16,bind[j].begin());}
        rt::Plan plan;check(ms::prepare(skin,names,bind,plan,&why),mesh.name+": prepare "+why);
        const auto originalBind=bind;std::vector<rt::Matrix> nativePose,bent;const auto nativeBent=bentPose(bind,names,parents);
        check(rt::evaluate(plan,bind.data(),bind.size(),nativePose,&why),why);
        check(rt::evaluate(plan,nativeBent.data(),nativeBent.size(),bent,&why),why);
        const std::vector<std::vector<rt::Matrix>> poses={sourcePose(plan),nativePose,bent};
        for(size_t edge=0;edge<seamCount;++edge)for(size_t pose=0;pose<poses.size();++pose) {
            Point ends[2]{};
            for(size_t end=0;end<2;++end) {
                const size_t v=seamPoints[edge][end];
                for(size_t lane=0;lane<4;++lane) {
                    const size_t bone=skin.vertices[v].bone[lane];const float weight=skin.vertices[v].weight[lane];
                    const auto value=rt::point(rt::point(source.point[v],plan.sourceInverse[bone]),poses[pose][bone]);
                    for(size_t d=0;d<3;++d)ends[end][d]+=weight*value[d];
                }
            }
            const double originalLength=distance(source.point[seamPoints[edge][0]],source.point[seamPoints[edge][1]]);
            const double posedLength=distance(ends[0],ends[1]);maximumSeamStretch=std::max(maximumSeamStretch,posedLength/originalLength);
            seamCsv<<mesh.name<<','<<seamNames[edge]<<','<<pose<<','<<originalLength<<','<<posedLength<<','<<posedLength/originalLength<<'\n';
        }
        check(bind==originalBind,"retarget changed native bind matrices");
        modmesh::OrigMeshRef ref;ref.nbones=int(mesh.nbones);ref.customSource=true;ref.targetFileName="ULTIMATE_SPIDERMAN";ref.targetMeshNames=selected;
        for(const auto &entry:native.meshes)ref.targetFileMeshNames.push_back(entry.name);
        ref.bonePos=mesh.bonePos;ref.boneNames=names;ref.boneParents=parents;ref.haveBones=true;
        ref.haveSphere=true;ref.sphereRadius=mesh.sphereRadius;std::copy_n(mesh.sphereCenter,3,ref.sphereCenter);
        auto scene=*cached;check(ms::install(skin,scene,ref,&why),"private install: "+why);
        check(scene.geoms.begin()->second.clusters.size()==skin.joints.size(),"skin clusters missing");
        check(scene.cfg.skin==1&&!scene.cfg.fit&&scene.cfg.retarget&&ref.nbones==int(skin.joints.size()),"profile mode not installed");
        std::vector<modmesh::OrigSectionView> views;
        for(const auto &section:mesh.sections) {
            check(section.stride==64,"native skin stride changed");modmesh::OrigSectionView view;
            view.verts=reinterpret_cast<const float *>(native.bytes.data()+section.vertexOffset);view.nverts=section.vertexCount;view.strideBytes=64;
            view.palette=section.palette.data();view.nbones=int(section.palette.size());views.push_back(view);
        }
        const auto built=modmesh::buildSectionsForMesh(scene,mesh.name,views,ref);check(!built.empty(),"public Miles import rejected");
        std::vector<Metrics> metrics={Metrics("source_reconstructed"),Metrics("native_bind"),Metrics("bent_limbs")};
        std::ofstream cloud;if(tested==0){cloud.open("build-static-audit/miles/poses.csv");cloud<<"pose,x,y,z,dominant_bone,material\n";cloud<<std::setprecision(9);}
        size_t triangles=0,vertices=0,paletteMax=0;std::set<std::string> materials;double sourceError=0;
        for(const auto &section:built) {
            check(bool(section),"retained original section in Miles replacement");if(section->hide)continue;
            check(!section->rigid&&!section->keepGeometry&&!section->keepOriginalPalette,"Miles section did not emit independent skin");
            check(section->vertices.size()%16==0&&section->indices.size()%3==0,"invalid geometry rows");
            check(!section->palette.empty()&&section->palette.size()<=26,"skin shader palette exceeded");
            for(auto bone:section->palette)check(bone<skin.joints.size(),"palette references native instead of source rig");
            paletteMax=std::max(paletteMax,section->palette.size());triangles+=section->indices.size()/3;materials.insert(section->sourceMaterialName);
            std::vector<Point> original;std::vector<std::vector<Point>> transformed(poses.size());
            for(size_t v=0;v<section->vertices.size();v+=16) {
                const auto *row=section->vertices.data()+v;for(size_t lane=0;lane<16;++lane)check(std::isfinite(row[lane]),"nonfinite imported vertex channel");
                const Point point{{row[0],row[1],row[2]}};std::vector<double> weights(skin.joints.size());double sum=0;
                for(size_t lane=0;lane<4;++lane) {
                    const double weight=row[12+lane],slot=row[8+lane];check(weight>=0&&weight<=1,"invalid imported weight");sum+=weight;
                    if(weight>0){check(slot>=0&&slot==std::floor(slot)&&slot<section->palette.size(),"invalid local palette index");weights[section->palette[size_t(slot)]]+=weight;}
                }
                check(std::abs(sum-1)<2e-6,"import changed normalized skin weight sum");
                check(source.match(point,weights,skin,sourceError),"emitted position/skin differs from original FBX/profile");original.push_back(point);
                const size_t dominant=size_t(std::max_element(weights.begin(),weights.end())-weights.begin());
                for(size_t pose=0;pose<poses.size();++pose) {
                    Point result{};
                    for(size_t bone=0;bone<weights.size();++bone)if(weights[bone]>0) {
                        const auto p=rt::point(rt::point(point,plan.sourceInverse[bone]),poses[pose][bone]);
                        for(size_t d=0;d<3;++d)result[d]+=float(weights[bone]*p[d]);
                    }
                    metrics[pose].bounds.add(result);transformed[pose].push_back(result);
                    metrics[pose].maxError=std::max(metrics[pose].maxError,distance(result,point));
                    if(cloud&&v%128==0)cloud<<metrics[pose].name<<','<<result[0]<<','<<result[1]<<','<<result[2]<<','<<skin.joints[dominant].name<<','<<section->sourceMaterialName<<'\n';
                }
            }
            vertices+=original.size();for(size_t pose=0;pose<poses.size();++pose)metrics[pose].edges(original,transformed[pose],section->indices,section->sourceMaterialName);
        }
        check(triangles==149987,"Miles triangles were lost");check(materials.size()==19,"Miles materials were lost");
        check(metrics[0].maxError<3e-5,"source inverse/pose failed to reconstruct source geometry");
        for(auto &metric:metrics){metric.report(csv,mesh.name);metric.report(std::cout,mesh.name);}
        if(tested==0)for(auto &metric:metrics)metric.worst(edgeCsv);
        check(cached->geoms.begin()->second.ctrl==originalCtrl&&cached->geoms.begin()->second.pvi==originalIndices&&cached->geoms.begin()->second.clusters.empty(),"shared scene cache was modified");
        check(cached->cfg.skin==originalCfg.skin&&cached->cfg.fit==originalCfg.fit&&cached->cfg.scale==originalCfg.scale&&cached->cfg.offset.x==originalCfg.offset.x&&cached->cfg.yaw==originalCfg.yaw,"shared sidecar config was modified");
        check(ms::fingerprint(native.bytes.data(),native.bytes.size())==nativeFingerprint,"native PCMESH bytes modified");
        std::cout<<"PASS "<<mesh.name<<": "<<triangles<<" triangles, "<<materials.size()<<" materials, "<<vertices<<" emitted vertices, palette <= "<<paletteMax<<"; source match error "<<sourceError<<'\n';++tested;
    }
    check(tested==3,"not all Miles LODs were tested");
    // These bounds permit smooth blended transitions; they do not require the
    // generator's particular choice to assign either endpoint a single bone.
    check(maximumSeamJump<.2,"measured ankle/elbow/crotch/thigh edges retain an abrupt skin-weight switch");
    check(maximumSeamStretch<3,"measured ankle/elbow/crotch/thigh edges stretch over three times under flexion");
    std::cout<<"PASS measured ankle/elbow/crotch/thigh continuity: max influence jump "<<maximumSeamJump<<", max pose stretch "<<maximumSeamStretch<<'\n';
}
}
int main(int argc,char **argv) {
    try {
        check(argc<=3,"usage: mod_miles_skin_probe [game-dir [profile-path]]");synthetic();
        if(argc>1)asset(argv[1],argc>2?argv[2]:"build-deformation/spiderman_miles_morales.fbx.skin");
        return 0;
    } catch(const std::exception &error) {std::cerr<<"FAIL "<<error.what()<<'\n';return 1;}
}
