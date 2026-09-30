#include "../src/mod_mesh_retarget.h"
#include "../src/mod_pcmesh_source.h"
#include "../src/mod_pcskel_source.h"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <limits>

namespace rt = modmesh::retarget;

static void require(bool pass, const char *what)
{
    if (!pass) { std::fprintf(stderr, "FAIL: %s\n", what); std::exit(1); }
}

static rt::Matrix transform(float angle, float x=0, float y=0, float z=0)
{
    auto m=rt::identity();
    m[0]=m[5]=std::cos(angle); m[1]=std::sin(angle); m[4]=-m[1];
    m[12]=x; m[13]=y; m[14]=z;
    return m;
}

static bool close(const rt::Matrix &a, const rt::Matrix &b, float epsilon=2e-5f)
{
    for (std::size_t i=0; i<a.size(); ++i)
        if (std::abs(a[i]-b[i])>epsilon) return false;
    return true;
}

static float distance(const rt::Matrix &a, const rt::Matrix &b)
{
    float d=0;
    for (int i=12; i<15; ++i) d+=(a[i]-b[i])*(a[i]-b[i]);
    return std::sqrt(d);
}

static std::vector<rt::Matrix> chain(const rt::Matrix &root, float upper, float lower,
                                    float bend1=0, float bend2=0, float delta=0)
{
    std::vector<rt::Matrix> result{root};
    result.push_back(rt::multiply(transform(bend1,upper,delta),result[0]));
    result.push_back(rt::multiply(transform(bend2,lower),result[1]));
    return result;
}

static std::vector<uint8_t> readFile(const char *path)
{
    std::ifstream file(path,std::ios::binary);
    require(bool(file),"open native retarget fixture");
    return std::vector<uint8_t>(std::istreambuf_iterator<char>(file),{});
}

static void supportHeightTest()
{
    auto tilt=rt::identity();
    tilt[5]=tilt[10]=std::cos(0.4f);tilt[6]=std::sin(0.4f);tilt[9]=-tilt[6];
    const auto world=rt::multiply(tilt,transform(0.7f,12.f,-8.f,3.f));
    const rt::Vec3 up{{world[4],world[5],world[6]}};
    std::vector<rt::Matrix> source{transform(0),transform(0,-0.2f,-1.f,0.1f),transform(0,0.2f,-0.9f,0.1f)};
    std::vector<rt::Matrix> target{transform(0),transform(0,-0.4f,-1.8f,0.15f),transform(0,0.4f,-1.6f,0.15f)};
    for(auto &m:source)m=rt::multiply(m,world);
    for(auto &m:target)m=rt::multiply(m,world);
    const std::vector<std::pair<std::size_t,std::size_t>> feet{{1,1},{2,2}};
    const auto original=source,untouchedTarget=target;
    float shift=0;
    require(rt::alignSupportHeight(source,target.data(),target.size(),feet,
            {{up[0]*3.5f,up[1]*3.5f,up[2]*3.5f}},&shift),"support alignment accepts non-unit arbitrary actor up");
    require(std::abs(shift+0.8f)<2e-5f,"neutral shorter legs align to native lowest foot");
    for(std::size_t j=0;j<source.size();++j) {
        auto expected=original[j];
        for(int c=0;c<3;++c)expected[12+c]-=0.8f*up[c];
        require(close(source[j],expected),"support correction only translates along actor up");
        for(int c=0;c<12;++c)require(source[j][c]==original[j][c],"support correction preserves rotations and scale");
    }
    require(target==untouchedTarget,"support alignment never changes native pose");
    require(std::abs(distance(source[0],source[1])-distance(original[0],original[1]))<2e-5f,"support correction preserves source limb lengths");
    auto same=target;
    require(rt::alignSupportHeight(same,target.data(),target.size(),feet,up,&shift),"equal-rig support alignment");
    require(shift==0 && same==target,"equal source and target poses remain unchanged");

    // Switch the support leg, then jump with both poses. The correction must
    // follow current animation instead of pinning the actor to a ground plane.
    source=original;
    for(int c=0;c<3;++c) {source[1][12+c]+=0.4f*up[c];target[1][12+c]+=1.f*up[c];}
    auto jumpingSource=source,jumpingTarget=target;
    for(auto &m:jumpingSource)for(int c=0;c<3;++c)m[12+c]+=3.f*up[c];
    for(auto &m:jumpingTarget)for(int c=0;c<3;++c)m[12+c]+=3.f*up[c];
    require(rt::alignSupportHeight(source,target.data(),target.size(),feet,up,&shift),"animated support leg switch");
    require(std::abs(shift+0.7f)<2e-5f,"support correction follows animated foot heights");
    require(rt::alignSupportHeight(jumpingSource,jumpingTarget.data(),jumpingTarget.size(),feet,up),"jump support alignment");
    for(std::size_t j=0;j<source.size();++j) {
        auto expected=source[j];for(int c=0;c<3;++c)expected[12+c]+=3.f*up[c];
        require(close(jumpingSource[j],expected),"native jump/root displacement is preserved");
    }
    const auto sentinel=source;
    require(!rt::alignSupportHeight(source,target.data(),target.size(),{{1,99}},up),"reject invalid support target index");
    require(!rt::alignSupportHeight(source,target.data(),target.size(),feet,{{0,0,0}}),"reject missing actor up axis");
    require(source==sentinel,"failed support correction is atomic");
}

static void nativeFixture(const char *sourceMeshPath, const char *sourceSkeletonPath,
                          const char *targetMeshPath, const char *targetSkeletonPath)
{
    modmesh::pcmeshsource::Source sourceFile,targetFile;
    modmesh::pcskelsource::Source sourceSkeleton,targetSkeleton;
    std::string why;
    require(sourceFile.parse(readFile(sourceMeshPath),&why),"parse native source PCMESH");
    require(targetFile.parse(readFile(targetMeshPath),&why),"parse native target PCMESH");
    require(sourceSkeleton.parse(readFile(sourceSkeletonPath),&why),"parse source PCSKEL");
    require(targetSkeleton.parse(readFile(targetSkeletonPath),&why),"parse target PCSKEL");
    const auto primary=[](const modmesh::pcmeshsource::Source &file) -> const modmesh::pcmeshsource::Mesh & {
        for(const auto &mesh:file.meshes)
            if(mesh.nbones && mesh.name.size()>=3 && mesh.name.substr(mesh.name.size()-3)=="000") return mesh;
        require(false,"native primary mesh exists");
        return file.meshes.front();
    };
    const auto &sm=primary(sourceFile), &tm=primary(targetFile);
    require(sourceSkeleton.bones.size()>=sm.nbones && targetSkeleton.bones.size()>=tm.nbones,
            "PCSKEL describes both native mesh palettes");
    const auto matrices=[](const modmesh::pcmeshsource::Mesh &mesh) {
        std::vector<rt::Matrix> result(mesh.nbones);
        for(std::size_t j=0;j<result.size();++j) {
            for(int k=0;k<16;++k) result[j][k]=mesh.boneMatrices[j*16+k];
            // Native Mat4x3 has unused W lanes, not serialized homogeneous data.
            result[j][3]=result[j][7]=result[j][11]=0;
            result[j][15]=1;
        }
        return result;
    };
    const auto source=matrices(sm),target=matrices(tm);
    std::vector<int> parents(sm.nbones),mapping(sm.nbones,-1);
    unsigned mapped=0;
    for(std::size_t j=0;j<source.size();++j) {
        parents[j]=sourceSkeleton.bones[j].parent;
        const auto *found=targetSkeleton.findBone(sourceSkeleton.bones[j].name);
        if(found && found->index<target.size()) { mapping[j]=int(found->index); ++mapped; }
    }
    require(mapped>=50,"native source-target name mapping covers body skin");
    rt::Plan plan;
    require(rt::prepare(source,parents,mapping,target,plan,&why),"prepare actual native skeletons");
    const auto world=transform(0.63f,15.f,4.f,-12.f);
    std::vector<rt::Matrix> current;
    for(const auto &m:target) current.push_back(rt::multiply(m,world));
    std::vector<rt::Matrix> result;
    require(rt::evaluate(plan,current.data(),current.size(),result,&why),"native bind evaluation");
    for(std::size_t j=0;j<source.size();++j) {
        require(close(result[j],rt::multiply(source[j],world),1e-4f),"actual source bind proportions preserved");
        require(close(rt::multiply(plan.sourceInverse[j],result[j]),world,1e-4f),"actual source skin bind cancels once");
    }
    std::vector<std::pair<std::size_t,std::size_t>> feet;
    for(const char *name:{"L_TOE","R_TOE"}) {
        const auto *s=sourceSkeleton.findBone(name),*t=targetSkeleton.findBone(name);
        require(s && t,"actual source and target toe mappings");feet.emplace_back(s->index,t->index);
    }
    float supportShift=0;
    require(rt::alignSupportHeight(result,current.data(),current.size(),feet,
            {{world[4],world[5],world[6]}},&supportShift),"actual neutral support-height correction");
    double sourceHeight=1e30,targetHeight=1e30;
    for(const auto &pair:feet) {
        double a=0,b=0;for(int c=0;c<3;++c){a+=result[pair.first][12+c]*world[4+c];b+=current[pair.second][12+c]*world[4+c];}
        sourceHeight=std::min(sourceHeight,a);targetHeight=std::min(targetHeight,b);
    }
    require(std::abs(sourceHeight-targetHeight)<1e-4,"actual BlackSuit toe height matches native Venom");
    const auto *upper=targetSkeleton.findBone("L_UPPERARM");
    const auto *fore=targetSkeleton.findBone("L_FOREARM");
    require(upper && fore,"native animated arm channels available");
    std::vector<unsigned char> done(target.size());
    std::function<void(std::size_t)> animate=[&](std::size_t j) {
        if(done[j]) return;
        const int parent=targetSkeleton.bones[j].parent;
        require(parent<int(target.size()),"native body parent stays inside skin palette");
        if(parent>=0) animate(std::size_t(parent));
        auto local=parent<0?target[j]:rt::multiply(target[j],plan.targetInverse[std::size_t(parent)]);
        if(j==upper->index || j==fore->index)
            local=rt::multiply(transform(j==upper->index?0.8f:-1.1f),local);
        current[j]=rt::multiply(local,parent<0?world:current[std::size_t(parent)]);
        done[j]=1;
    };
    for(std::size_t j=0;j<target.size();++j) animate(j);
    require(rt::evaluate(plan,current.data(),current.size(),result,&why),"native animated evaluation");
    const auto *sourceUpper=sourceSkeleton.findBone("L_UPPERARM");
    const auto *sourceFore=sourceSkeleton.findBone("L_FOREARM");
    const auto *sourceHand=sourceSkeleton.findBone("L_HAND");
    require(sourceUpper && sourceFore && sourceHand,"source arm channels available");
    for(const auto pair:{std::make_pair(sourceUpper->index,sourceFore->index),
                         std::make_pair(sourceFore->index,sourceHand->index)}) {
        require(std::abs(distance(result[pair.first],result[pair.second])-
                         distance(source[pair.first],source[pair.second]))<1e-4f,
                "actual BlackSuit arm length survives native Venom articulation");
    }
    std::printf("PASS native %s (%u bones) -> %s (%u bones): %u names, bind identity, animated source arm lengths, support shift %.6f\n",
                sm.name.c_str(),sm.nbones,tm.name.c_str(),tm.nbones,mapped,supportShift);
}

int main(int argc, char **argv)
{
    const auto source=chain(transform(0.3f),1.f,0.6f,0.2f,-0.1f);
    const auto target=chain(transform(-0.4f),2.f,1.4f,0.5f,-0.2f);
    const std::vector<int> parent{-1,0,1}, map{0,1,2};
    rt::Plan plan;
    std::string why;
    require(rt::prepare(source,parent,map,target,plan,&why),"prepare differently proportioned rigs");
    const auto world=transform(0.7f,12.f,7.f,-4.f);
    std::vector<rt::Matrix> bindInWorld;
    for(const auto &m:target) bindInWorld.push_back(rt::multiply(m,world));
    std::vector<rt::Matrix> result;
    require(rt::evaluate(plan,bindInWorld.data(),bindInWorld.size(),result,&why),"bind pose evaluation");
    for(std::size_t i=0;i<source.size();++i) {
        require(close(result[i],rt::multiply(source[i],world)),"source bind shape survives target bind and world placement");
        require(close(rt::multiply(plan.sourceInverse[i],result[i]),world),"source inverse bind cancels exactly once");
    }

    // Rotating the longer target arm bends a source-length arm without moving
    // the target pose or changing the source limb's cross-sectional dimensions.
    auto animated=chain(transform(0.6f,10.f,4.f),2.f,1.4f,1.1f,-0.8f);
    const auto untouched=animated;
    require(rt::evaluate(plan,animated.data(),animated.size(),result),"animated unequal-rig evaluation");
    require(animated==untouched,"target pose is read-only");
    require(std::abs(distance(result[1],result[0])-1.f)<2e-5f,"source upper arm length preserved");
    require(std::abs(distance(result[2],result[1])-0.6f)<2e-5f,"source forearm length preserved");

    // This includes a native animated local translation, rather than testing
    // only rotations that could pass with a rigid/rest-only implementation.
    animated=chain(transform(-0.9f,11.f,-3.f,2.f),2.f,1.4f,1.2f,0.7f,0.25f);
    rt::Plan same;
    require(rt::prepare(target,parent,map,target,same),"prepare identical rigs");
    require(rt::evaluate(same,animated.data(),animated.size(),result),"same-rig animated evaluation");
    for(std::size_t i=0;i<target.size();++i)
        require(close(result[i],animated[i]),"identical rig preserves full native animated pose");

    // Native pose-array order is independent of both FBX and PCSKEL ordering.
    auto reordered=std::vector<rt::Matrix>{animated[2],animated[0],animated[1]};
    auto reorderedBind=std::vector<rt::Matrix>{target[2],target[0],target[1]};
    rt::Plan reorderedPlan;
    require(rt::prepare(target,parent,{1,2,0},reorderedBind,reorderedPlan),"arbitrary target palette ordering");
    require(rt::evaluate(reorderedPlan,reordered.data(),reordered.size(),result),"reordered target evaluation");
    for(std::size_t i=0;i<target.size();++i) require(close(result[i],animated[i]),"mapping uses native target skin indices");

    rt::Plan helper;
    const auto helperSource=chain(rt::identity(),0.3f,0.7f);
    const std::vector<rt::Matrix> helperTarget{rt::identity(),transform(0,2.f)};
    require(rt::prepare(helperSource,parent,{0,-1,1},helperTarget,helper),"unmapped source helper");
    std::vector<rt::Matrix> helperPose{world,rt::multiply(transform(1.f,2.f),world)};
    require(rt::evaluate(helper,helperPose.data(),helperPose.size(),result),"helper follows source hierarchy");
    require(close(result[1],rt::multiply(helperSource[1],world)),"unmapped helper retains local source rest transform");
    require(std::abs(distance(result[2],result[0])-1.f)<2e-5f,"mapped descendant uses nearest mapped source ancestor");

    rt::Plan unsorted;
    require(rt::prepare({source[2],source[0],source[1]},{2,-1,1},{2,0,1},target,unsorted),"non-topological source input");
    require(rt::evaluate(unsorted,bindInWorld.data(),bindInWorld.size(),result),"topological evaluation");
    require(close(result[0],rt::multiply(source[2],world)),"source bone order remains stable in output");

    const auto sentinel=result;
    auto badPose=animated;
    badPose[1][0]=std::numeric_limits<float>::quiet_NaN();
    require(!rt::evaluate(plan,badPose.data(),badPose.size(),result),"reject nonfinite target pose");
    require(result==sentinel,"failed pose evaluation never publishes partial output");
    require(!rt::evaluate(plan,animated.data(),1,result),"reject short target pose array");
    rt::Plan invalid;
    require(!rt::prepare(source,{1,0,-1},map,target,invalid),"reject parent cycle");
    require(!rt::prepare(source,{-1,9,1},map,target,invalid),"reject parent outside source skeleton");
    require(!rt::prepare(source,parent,{0,3,2},target,invalid),"reject target palette overflow");
    require(!rt::prepare(source,parent,{-1,-1,-1},target,invalid),"reject unmapped skeleton");
    auto singular=source; singular[0][0]=singular[0][1]=singular[0][2]=0;
    require(!rt::prepare(singular,parent,map,target,invalid),"reject singular source bind");
    supportHeightTest();
    std::puts("PASS: bind identity, source limb lengths, native animated deltas, palette ordering, source helpers, hierarchy and failure atomicity");
    require(argc==1 || argc==5,"usage: probe [source.PCMESH source.PCSKEL target.PCMESH target.PCSKEL]");
    if(argc==5) nativeFixture(argv[1],argv[2],argv[3],argv[4]);
}
