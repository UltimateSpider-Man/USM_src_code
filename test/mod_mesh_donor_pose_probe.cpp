#include "../src/mod_mesh_import.h"
#include <iostream>
#include <stdexcept>
#include <limits>
#include <filesystem>
#include <chrono>

namespace dp = modmesh::donorpose;
namespace rt = modmesh::retarget;
static void require(bool value, const char *why) { if (!value) throw std::runtime_error(why); }
static bool near(const rt::Vec3 &a, const rt::Vec3 &b, float e=2e-5f) {
    for(int i=0;i<3;++i) if(std::abs(a[i]-b[i])>e) return false; return true;
}
static modmesh::OrigMeshRef reference() {
    modmesh::OrigMeshRef r;r.nbones=7;r.customSource=true;r.targetFileName="VENOM";
    r.donorPoseMetadataValidated=true;r.boneParents={-1,0,1,2,0,4,5};
    r.boneNames={"pelvis","L_UPPERARM","L_FOREARM","L_HAND","R_UPPERARM","R_FOREARM","R_HAND"};
    r.bonePos={0,0,0,-1,0,0,-2,0,0,-3,0,0,1,0,0,2,0,0,3,0,0};return r;
}
static void math() {
    const auto ref=reference();dp::Plan p;std::string why;
    require(dp::prepareArmsDown(ref.boneNames,ref.boneParents,ref.bonePos,p,&why),why.c_str());
    require(p.upperarm[0]==1&&p.upperarm[1]==4&&p.arm[0]==0&&p.arm[3]==1&&p.arm[6]==2,"semantic arm subtree selection");
    float indices[4]={0,1,-1,-1},weights[4]={.5f,.5f,0,0};rt::Vec3 q,n,b,nb;
    const rt::Vec3 bind{{-2,0,0}},normal{{.6f,.8f,0}};
    require(dp::transform(p,indices,weights,bind,normal,false,q,n,&why),why.c_str());
    require(near(q,{{-1.5f,-.5f,0}}),"independent half-body half-left-arm analytic position");
    require(dp::transform(p,indices,weights,q,n,true,b,nb,&why),why.c_str());
    require(near(b,bind)&&near(nb,normal),"inverse of blended skin matrix recovers point and normal");
    const auto immutable=p.deformation;
    indices[0]=6;weights[0]=1;weights[1]=0;
    require(dp::transform(p,indices,weights,{{3,0,0}},{{0,0,1}},false,q,n,&why),why.c_str());
    require(near(q,{{1,-2,0}})&&p.deformation==immutable,"right arm down without changing plan");
    dp::Plan pitched;
    require(dp::prepareArmsDown(ref.boneNames,ref.boneParents,ref.bonePos,pitched,&why,-30,30),why.c_str());
    require(dp::transform(pitched,indices,weights,{{3,0,0}},{{0,0,1}},false,q,n,&why),why.c_str());
    require(near(q,{{1,-std::sqrt(3.f),1}}),"explicit positive pitch points forward in model Z");
    require(!dp::prepareArmsDown(ref.boneNames,ref.boneParents,ref.bonePos,pitched,&why,0,81),"excessive pitch rejected");
    require(!dp::prepareArmsDown(ref.boneNames,ref.boneParents,ref.bonePos,pitched,&why,0,std::numeric_limits<double>::quiet_NaN()),"non-finite pitch rejected");
    indices[0]=0;require(dp::transform(p,indices,weights,{{0,-2,1}},normal,true,q,n,&why),why.c_str());
    require(near(q,{{0,-2,1}})&&near(n,normal),"non-arm identity");
    const auto safe=p.deformation;auto parents=ref.boneParents;parents[0]=3;
    require(!dp::prepareArmsDown(ref.boneNames,parents,ref.bonePos,p,&why)&&p.deformation==safe,"cycle rejects transactionally");
    auto names=ref.boneNames;names[1]="L_UPPERARM_PIVOT";
    require(!dp::prepareArmsDown(names,ref.boneParents,ref.bonePos,p,&why),"surface upperarm-pivot helper cannot replace joint");
    indices[0]=100;require(!dp::transform(p,indices,weights,bind,normal,true,q,n,&why),"invalid influence rejects");
    indices[0]=0;weights[0]=-1;require(!dp::transform(p,indices,weights,bind,normal,true,q,n,&why),"negative weight rejects");
    weights[0]=0;require(!dp::transform(p,indices,weights,bind,normal,true,q,n,&why),"weightless vertex rejects");
    dp::Plan singular;singular.deformation.assign(2,rt::identity());singular.deformation[1][0]=-1;singular.deformation[1][5]=-1;
    indices[0]=0;indices[1]=1;weights[0]=weights[1]=.5f;
    const auto previous=q;require(!dp::transform(singular,indices,weights,bind,normal,true,q,n,&why)&&q==previous,"singular blend rejects without output mutation");
}
static void builder() {
    auto ref=reference();modmesh::Scene s;s.srcName="Explicit arms down.fbx";s.cfg.custom=true;s.cfg.fit=false;s.cfg.skin=2;
    s.cfg.donorPose="arms_down";s.cfg.anim=false;s.cfg.retarget=false;s.cfg.whiteAuto=false;s.cfg.autoTex=false;s.cfg.roundtripEps=0;
    modmesh::Model model;model.id=1;model.name="Costume";model.type="Mesh";model.geoms={2};model.materials={10,11,12};s.models[1]=model;s.meshModelOrder={1};
    modmesh::Geom g;g.id=2;g.matMapping="ByPolygon";g.matIdx={0,1,2};
    g.uv.valid=true;g.uv.comps=2;g.uv.mapping="ByPolygonVertex";g.uv.reference="Direct";
    g.nrm.valid=true;g.nrm.comps=3;g.nrm.mapping="ByPolygonVertex";g.nrm.reference="Direct";
    std::vector<float> native;std::vector<rt::Vec3> expected;
    for(int part=0;part<3;++part) {
        s.materials[10+part]="Slot"+std::to_string(part);s.matTexStem[10+part]="Image"+std::to_string(part);
        for(int corner=0;corner<3;++corner) {
            const float x=(part==0?-3.f:part==1?3.f:0.f)+(corner==1?.03f:0.f);
            const float y=(part==2?-2.f:0.f)+(corner==2?.04f:0.f),z=.01f;
            expected.push_back({{x,y,z}});
            // Independent right-angle pose around the known shoulder pivots.
            g.ctrl.insert(g.ctrl.end(),{part==0?-1.-y:part==1?1.+y:double(x),part==0?double(x)+1:part==1?1.-x:double(y),double(z)});
            const int id=part*3+corner;g.pvi.push_back(corner==2?-id-1:id);
            g.uv.data.insert(g.uv.data.end(),{double(id),.25});g.nrm.data.insert(g.nrm.data.end(),{0,0,1});
            const float row[16]={x,y,z,0,0,1,float(id),.75f,float(part),-1,-1,-1,1,0,0,0};native.insert(native.end(),row,row+16);
        }
    }
    s.geoms[2]=g;const auto unchanged=native;const uint16_t palette[3]={3,6,0};
    modmesh::OrigSectionView view;view.verts=native.data();view.nverts=9;view.strideBytes=64;view.nbones=3;view.palette=palette;
    const auto built=modmesh::buildSectionsForMesh(s,"venom000",{view},ref);
    size_t triangles=0;std::set<int> seen;std::set<std::string> materials;
    for(const auto& section:built)if(section&&!section->hide) {
        triangles+=section->indices.size()/3;materials.insert(section->sourceMaterialName);
        require(section->palette.size()<=26,"draw palette limit");
        for(size_t v=0;v<section->vertices.size();v+=16) {
            const auto& a=section->vertices;const int id=int(std::lround(a[v+6]));require(id>=0&&id<9,"authored UV identity");seen.insert(id);
            require(a[v+7]==.75f&&near({{a[v],a[v+1],a[v+2]}},expected[size_t(id)]),"full builder recovers bind geometry and UV");
            const int slot=int(a[v+8]);require(slot>=0&&size_t(slot)<section->palette.size()&&section->palette[size_t(slot)]==palette[id/3]&&std::abs(a[v+12]-1)<1e-6f,"arms receive correct native bones");
        }
    }
    require(triangles==3&&seen.size()==9&&materials.size()==3&&native==unchanged,"all faces/materials retained, native rows untouched");
    auto bad=ref;bad.donorPoseMetadataValidated=false;
    require(modmesh::buildSectionsForMesh(s,"venom000",{view},bad).empty(),"unverified metadata leaves native geometry intact");
    bad=ref;bad.boneParents[0]=3;require(modmesh::buildSectionsForMesh(s,"venom000",{view},bad).empty(),"bad hierarchy leaves native geometry intact");
    bad=ref;bad.customSource=false;require(modmesh::buildSectionsForMesh(s,"venom000",{view},bad).empty(),"native source never silently receives donor pose");
    s.cfg.donorPose="unsupported_pose";require(modmesh::buildSectionsForMesh(s,"venom000",{view},ref).empty(),"unknown explicit pose rejects");
    s.cfg.donorPose="arms_down";modmesh::GCluster cluster;cluster.boneName="L_HAND";cluster.idx={0};cluster.w={1};s.geoms[2].clusters={cluster};
    require(modmesh::buildSectionsForMesh(s,"venom000",{view},ref).empty(),"authored source skin never overwritten by explicit unrigged pose");
    s.geoms[2].clusters.clear();s.cfg.donorPose.clear();s.cfg.donorArmPitchSet=true;
    require(modmesh::buildSectionsForMesh(s,"venom000",{view},ref).empty(),"arm pitch without pose cannot be silently ignored");
}
static void sidecarAndFit() {
    const auto path=std::filesystem::temp_directory_path()/("usm-donor-pose-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+".fbx");
    const auto ini=path.string()+".ini";
    {std::ofstream f(ini);f<<"donor_pose=\"arms_down\"\ndonor_left_arm_pitch=-25\ndonor_right_arm_pitch=30\n";}
    auto cfg=modmesh::loadSidecar(path.string());std::filesystem::remove(ini);
    require(cfg.donorPose=="arms_down"&&cfg.donorArmPitchSet&&cfg.donorLeftArmPitch==-25&&cfg.donorRightArmPitch==30,"sidecar explicit pose and pitch parsing");
    {std::ofstream f(ini);f<<"donor_pose=off\ndonor_right_arm_pitch=invalid\n";}
    cfg=modmesh::loadSidecar(path.string());std::filesystem::remove(ini);
    require(cfg.donorPose.empty()&&cfg.donorArmPitchSet&&!std::isfinite(cfg.donorRightArmPitch),"invalid pitch is retained for rejection, not silently zeroed");
    modmesh::Scene s;s.srcName="Exact custom fit.fbx";s.cfg.custom=true;s.cfg.fit=true;s.cfg.skin=2;s.cfg.retarget=false;s.cfg.whiteAuto=false;s.cfg.roundtripEps=0;
    modmesh::Model m;m.id=1;m.name="Costume";m.type="Mesh";m.geoms={2};m.materials={3};s.models[1]=m;s.meshModelOrder={1};s.materials[3]="Body";
    modmesh::Geom g;g.id=2;g.ctrl={-.1,0,.01,.1,0,.01,0,1,.01};g.pvi={0,1,-3};g.matMapping="AllSame";g.matIdx={0};s.geoms[2]=g;
    const float vertices[]={-.11f,0,.01f,0,0,1,0,0,0,-1,-1,-1,1,0,0,0,
        .11f,0,.01f,0,0,1,1,0,0,-1,-1,-1,1,0,0,0,0,1.1f,.01f,0,0,1,0,1,0,-1,-1,-1,1,0,0,0};
    const uint16_t palette[]={0};modmesh::OrigSectionView v;v.verts=vertices;v.nverts=3;v.strideBytes=64;v.palette=palette;v.nbones=1;
    auto ref=reference();const auto out=modmesh::buildSectionsForMesh(s,"venom000",{v},ref);float maximum=-100;
    require(out.size()==1&&out[0]&&out[0]->indices.size()==3,"exact custom fit retains face count");
    for(size_t i=0;i<out[0]->vertices.size();i+=16)maximum=std::max(maximum,out[0]->vertices[i+1]);
    require(std::abs(maximum-1.1f)<1e-6f,"custom fit corrects height mismatch inside native export dead zone");
}
static void surfaceContinuity() {
    using Corner=modmesh::detail::Corner;
    using Surface=modmesh::donorsurface::Surface<Corner>;
    const auto ref=reference();dp::Plan plan;std::string why;
    require(dp::prepareArmsDown(ref.boneNames,ref.boneParents,ref.bonePos,plan,&why),why.c_str());
    modmesh::detail::DonorGrid native;
    for (int family=0;family<3;++family) {
        modmesh::detail::Donor d{};d.p[0]=family==1?-1.f:1.f;d.p[1]=-.5f;d.p[2]=0;
        d.n[2]=1;d.bi[0]=family==1?3.f:family==2?6.f:0.f;d.bw[0]=1;
        native.donors.push_back(d);
    }
    native.build();
    // Two material buckets share an arm grid. A nearby hip grid has no edge
    // connection; another FBX model deliberately overlaps the arm exactly.
    std::vector<Corner> buckets[4];
    auto addGrid=[&](int surface,float z,bool arm) {
        constexpr int columns=17;
        for(int x=0;x<columns-1;++x) for(int triangle=0;triangle<2;++triangle) {
            const int dx[2][3]={{0,1,0},{1,1,0}},dy[2][3]={{0,0,1},{0,1,1}};
            const int bucket=surface==0?(x<8?0:1):surface+1;
            for(int k=0;k<3;++k) {
                const int column=x+dx[triangle][k],row=dy[triangle][k];
                Corner c{};c.px=-1.f+column*.002f;c.py=-.5f+row*.003f;c.pz=z;
                c.nx=.6f;c.nz=.8f;c.u=float(column+row*columns);c.v=float(bucket)+.25f;
                // One badly transferred arm vertex models the small donor
                // ownership jump that previously produced a large flap.
                c.bi[0]=arm&&!(column==8&&row==0)?3.f:0.f;
                buckets[bucket].push_back(c);
            }
        }
    };
    addGrid(0,0,true);addGrid(1,.0005f,false);addGrid(2,0,false);
    const auto before0=buckets[0],before1=buckets[1],before2=buckets[2],before3=buckets[3];
    std::vector<Surface> surfaces={{11,&buckets[0]},{11,&buckets[1]},{11,&buckets[2]},{22,&buckets[3]}};
    auto maxBindEdge=[&](const std::vector<Corner>& corners) {
        double maximum=0;
        for(size_t i=0;i<corners.size();i+=3) {
            rt::Vec3 points[3];
            for(int k=0;k<3;++k){const auto&c=corners[i+k];rt::Vec3 n;
                require(dp::transform(plan,c.bi,c.bw,{{c.px,c.py,c.pz}},{{c.nx,c.ny,c.nz}},true,points[k],n,&why),why.c_str());}
            for(int k=0;k<3;++k){double squared=0;for(int d=0;d<3;++d){double v=points[k][d]-points[(k+1)%3][d];squared+=v*v;}maximum=std::max(maximum,std::sqrt(squared));}
        }return maximum;
    };
    const double badEdge=std::max(maxBindEdge(buckets[0]),maxBindEdge(buckets[1]));
    require(badEdge>.5,"fixture reproduces large flap on tiny connected source faces");
    modmesh::donorsurface::Stats stats;
    require(modmesh::donorsurface::smooth(surfaces,plan.arm,native,&stats,&why),why.c_str());
    require(std::max(maxBindEdge(buckets[0]),maxBindEdge(buckets[1]))<.02,"topology removes isolated flap without removing faces");
    const std::vector<Corner>* original[]={&before0,&before1,&before2,&before3};
    std::map<std::pair<float,float>,std::array<float,8>> seam;
    size_t faces=0;
    for(int bucket=0;bucket<4;++bucket) {
        require(buckets[bucket].size()==original[bucket]->size(),"all original surface faces survive");faces+=buckets[bucket].size()/3;
        for(size_t i=0;i<buckets[bucket].size();++i) {
            const auto &c=buckets[bucket][i],&old=(*original[bucket])[i];
            require(std::memcmp(&c.px,&old.px,8*sizeof(float))==0,"original position/normal/UV corner bytes survive");
            float arm=0,sum=0;std::array<float,8> values;
            for(int k=0;k<4;++k){require(std::isfinite(c.bw[k])&&c.bw[k]>=0,"finite nonnegative final weights");sum+=c.bw[k];
                if(c.bw[k]>0){require(c.bi[k]>=0&&c.bi[k]<7,"validated actual native bone only");if(plan.arm[size_t(c.bi[k])])arm+=c.bw[k];}
                values[k]=c.bi[k];values[k+4]=c.bw[k];}
            require(std::abs(sum-1)<1e-6f,"compressed weights normalized");
            if(bucket<2){require(arm>.9f,"connected arm retains correct ownership");auto it=seam.emplace(std::make_pair(c.px,c.py),values);require(it.second||it.first->second==values,"material/UV seams share the same final skin");}
            else require(arm==0,"disconnected nearby hip and coincident separate model never acquire arm ownership");
        }
    }
    require(faces==96&&stats.positions==102&&stats.maximumGroupError<1e-6,"complete faces and model-scoped source graph");
    const auto saved=buckets[0];auto corrupt=native;corrupt.donors[0].bw[0]=std::numeric_limits<float>::infinity();
    require(!modmesh::donorsurface::smooth(surfaces,plan.arm,corrupt,nullptr,&why)
        &&std::memcmp(saved.data(),buckets[0].data(),saved.size()*sizeof(Corner))==0,"malformed native donor rejects without modifying source corners");

    // Two material corners at each source position carry four substantial
    // native leg/body bones between them and a negligible arm tail. That
    // tail previously reserved a lane and discarded one meaningful bone.
    std::vector<Corner> legMaterials[2];
    modmesh::detail::DonorGrid legDonors;
    const std::vector<uint8_t> legFamilies={0,1,1,1,2,2,2,0,0,0};
    for(int material=0;material<2;++material) for(int k=0;k<3;++k) {
        Corner c{};c.px=k==1?.01f:0;c.py=k==2?.01f:0;c.nz=1;
        c.u=float(k);c.v=float(material);
        c.bi[0]=0;c.bi[1]=7;c.bi[2]=float(8+material);c.bi[3]=3;
        c.bw[0]=.5f;c.bw[1]=.3f;c.bw[2]=.1999995f;c.bw[3]=.0000005f;
        legMaterials[material].push_back(c);
        modmesh::detail::Donor donor{};donor.p[0]=float(material*3+k);donor.n[2]=1;
        std::copy(c.bi,c.bi+4,donor.bi);std::copy(c.bw,c.bw+4,donor.bw);legDonors.donors.push_back(donor);
    }
    legDonors.build();
    require(modmesh::donorsurface::smooth(std::vector<Surface>{{31,&legMaterials[0]},{31,&legMaterials[1]}},legFamilies,legDonors,nullptr,&why),why.c_str());
    for(const auto& material:legMaterials)for(const auto& c:material) {
        std::set<int> retained;float sum=0;
        for(int k=0;k<4;++k){require(c.bw[k]>.09f,"negligible arm tail cannot displace a substantial native leg influence");retained.insert(int(c.bi[k]));sum+=c.bw[k];}
        require(retained==std::set<int>({0,7,8,9})&&std::abs(sum-1)<1e-6f,"all four meaningful body influences survive normalized");
    }
}
int main() {try{modmesh::setLog(+[](const char *line){std::cout<<line<<'\n';});math();builder();sidecarAndFit();surfaceContinuity();std::cout<<"PASS donor_pose arms_down: analytic blended inverse/normals, public full-geometry/material/UV preservation, native ownership, malformed metadata and pose rejection, explicit pitch parsing, exact custom fit, connected surface continuity and disconnected model isolation\n";return 0;}catch(const std::exception&e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}}
