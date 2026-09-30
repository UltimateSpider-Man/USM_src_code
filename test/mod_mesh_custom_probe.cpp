// Public importer regression: geometry bytes, exact triangle identities,
// material ownership and authored skin weights are independent invariants.
#include "../src/mod_mesh_import.h"
#include "../src/mod_pcmesh_source.h"
#include <chrono>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace {
using Bytes = std::vector<uint8_t>;
void require(bool value, const char* why) { if (!value) throw std::runtime_error(why); }
template<class T> void append(Bytes& out, T value) {
    const auto* p=reinterpret_cast<const uint8_t*>(&value); out.insert(out.end(),p,p+sizeof(value));
}
Bytes integer(int64_t value) { Bytes b{'L'};append(b,value);return b; }
Bytes text(const std::string& value) { Bytes b{'S'};append(b,uint32_t(value.size()));b.insert(b.end(),value.begin(),value.end());return b; }
Bytes number(double value) { Bytes b{'D'};append(b,value);return b; }
template<class T> Bytes array(char type,const std::vector<T>& values) {
    Bytes b{uint8_t(type)};append(b,uint32_t(values.size()));append(b,uint32_t(0));append(b,uint32_t(values.size()*sizeof(T)));
    const auto* p=reinterpret_cast<const uint8_t*>(values.data());b.insert(b.end(),p,p+values.size()*sizeof(T));return b;
}
struct Node {std::string name;std::vector<Bytes> props;std::vector<Node> children;};
Node str(const char* name,const std::string& value) {return {name,{text(value)},{}};}
void encode(Bytes& out,const Node& n) {
    const size_t start=out.size();out.resize(start+13);out[start+12]=uint8_t(n.name.size());
    out.insert(out.end(),n.name.begin(),n.name.end());uint32_t propsSize=0;
    for(const auto& p:n.props){out.insert(out.end(),p.begin(),p.end());propsSize+=uint32_t(p.size());}
    for(const auto& c:n.children)encode(out,c);
    if(!n.children.empty())out.resize(out.size()+13);
    const uint32_t fields[]={uint32_t(out.size()),uint32_t(n.props.size()),propsSize};std::memcpy(out.data()+start,fields,12);
}
constexpr size_t kMainVertices=99999+31*16320;
constexpr size_t kVertices=kMainVertices+6;
constexpr size_t kTriangles=kVertices/3;
Node geometry(int id,const std::string& name,bool main,size_t firstTriangle) {
    std::vector<double> pos,nrm,uv;std::vector<int32_t> indices,materials;
    const size_t corners=main?kMainVertices:3;pos.reserve(corners*3);nrm.reserve(corners*3);uv.reserve(corners*2);indices.reserve(corners);
    size_t triangle=firstTriangle;
    for(int material=0;material<(main?32:1);++material) {
        const size_t count=main?(material?16320:99999):3;
        for(size_t t=0;t<count/3;++t,++triangle) {
            const int32_t base=int32_t(indices.size());const double x=double(triangle%512)*0.003,y=double(triangle/512)*0.003;
            pos.insert(pos.end(),{x,y,0.,x+0.001,y,0.,x,y+0.001,0.});
            nrm.insert(nrm.end(),{0.,0.,1.,0.,0.,1.,0.,0.,1.});
            // Distinct integral UV identity survives transforms and winding.
            for(int corner=0;corner<3;++corner)uv.insert(uv.end(),{double(triangle*3+size_t(corner)),double(material)});
            indices.insert(indices.end(),{base,base+1,-base-3});materials.push_back(material);
        }
    }
    const auto layer=[](const char* type,const char* field,const std::vector<double>& values) {
        return Node{type,{integer(0)},{str("MappingInformationType","ByPolygonVertex"),str("ReferenceInformationType","Direct"),{field,{array('d',values)},{}}}};
    };
    return {"Geometry",{integer(id),text("Geometry::"+name),text("Mesh")},{
        {"Vertices",{array('d',pos)},{}},{"PolygonVertexIndex",{array('i',indices)},{}},
        layer("LayerElementNormal","Normals",nrm),layer("LayerElementUV","UV",uv),
        {"LayerElementMaterial",{integer(0)},{str("MappingInformationType","ByPolygon"),str("ReferenceInformationType","IndexToDirect"),{"Materials",{array('i',materials)},{}}}}}};
}
Bytes fixture() {
    Node objects{"Objects",{},{}},connections{"Connections",{}, {}};
    for(int part=0;part<3;++part) {
        const std::string name=part==0?"CustomBody":(part==1?"DetachedEyes":"SeparateBelt");
        objects.children.push_back(geometry(100+part,name,part==0,part==0?0:kMainVertices/3+size_t(part-1)));
        objects.children.push_back({"Model",{integer(200+part),text("Model::"+name),text("Mesh")},{}});
        connections.children.push_back({"C",{text("OO"),integer(100+part),integer(200+part)}, {}});
        connections.children.push_back({"C",{text("OO"),integer(200+part),integer(0)}, {}});
        for(int material=0;material<(part?1:32);++material)
            connections.children.push_back({"C",{text("OO"),integer(1000+material),integer(200+part)}, {}});
    }
    for(int material=0;material<32;++material) {
        const std::string suffix=std::to_string(material),stem="CUSTOM_TEX_"+suffix;
        objects.children.push_back({"Material",{integer(1000+material),text("Material::CustomMaterial"+suffix),text("")},
            {{"Properties70",{},{{"P",{text("DiffuseColor"),text("ColorRGB"),text(""),text("A"),
                number(double(material)/31.),number(double(31-material)/31.),number(.25)},{}}}}}});
        objects.children.push_back({"Texture",{integer(2000+material),text("Texture::"+stem),text("TextureVideoClip")},{str("RelativeFilename",stem+".dds")}});
        connections.children.push_back({"C",{text("OP"),integer(2000+material),integer(1000+material),text("DiffuseColor")},{}});
    }
    const char magic[]="Kaydara FBX Binary  \0\x1a\0";Bytes bytes(magic,magic+23);append(bytes,uint32_t(7400));
    encode(bytes,{"FBXHeaderExtension",{},{{"FBXVersion",{integer(7400)},{}}}});
    encode(bytes,{"GlobalSettings",{},{{"Properties70",{},{{"P",{text("UnitScaleFactor"),text("double"),text("Number"),text(""),number(100.)},{}}}}}});
    encode(bytes,objects);encode(bytes,connections);bytes.resize(bytes.size()+13);
    // Footer is structural; >40 MiB comes entirely from live geometry arrays.
    const uint8_t marker[]={0xfa,0xbc,0xab,0x09,0xd0,0xc8,0xd4,0x66,0xb1,0x76,0xfb,0x83,0x1c,0xf7,0x26,0x7e};
    bytes.insert(bytes.end(),std::begin(marker),std::end(marker));bytes.resize((bytes.size()+15)&~size_t(15));append(bytes,uint32_t(7400));bytes.resize(bytes.size()+120);
    const uint8_t tail[]={0xf8,0x5a,0x8c,0x6a,0xde,0xf5,0xd9,0x7e,0xec,0xe9,0x0c,0xe3,0x75,0x8f,0x29,0x0b};bytes.insert(bytes.end(),std::begin(tail),std::end(tail));
    return bytes;
}
void configure(modmesh::Scene& sc) {sc.cfg.fit=false;sc.cfg.weld=false;sc.cfg.skin=3;sc.cfg.roundtripEps=0.;sc.cfg.anim=false;}
struct Target {
    std::string name="venom000";
    std::vector<std::vector<float>> rows;
    std::vector<std::vector<uint16_t>> palettes;
    modmesh::OrigMeshRef ref;
    std::vector<modmesh::OrigSectionView> views()const {
        std::vector<modmesh::OrigSectionView> result;
        for(size_t i=0;i<rows.size();++i){modmesh::OrigSectionView v;v.verts=rows[i].data();v.nverts=uint32_t(rows[i].size()/16);v.strideBytes=64;v.palette=palettes[i].data();v.nbones=int(palettes[i].size());result.push_back(v);}return result;
    }
};
Target syntheticTarget(int bones,size_t sections=17) {
    Target t;t.ref.nbones=bones;t.ref.customSource=true;t.ref.targetFileName="VENOM";t.ref.targetMeshNames={"venom000","venom001"};
    std::vector<float> rows(48);for(int i=0;i<3;++i){rows[size_t(i)*16+size_t(i%3)]=0.01f;rows[size_t(i)*16+5]=1.f;rows[size_t(i)*16+12]=1.f;}
    t.rows.assign(sections,rows);t.palettes.assign(sections,{0});return t;
}
using Built=std::vector<std::optional<modmesh::BuiltSection>>;
void verifyGenerated(const Built& built,const Target& target) {
    std::vector<bool> seen(kVertices,false);std::set<std::string> textures,models;size_t vertices=0,triangles=0,visible=0;
    for(const auto& p:built) {
        require(bool(p),"host geometry survived full replacement");if(p->hide)continue;const auto& b=*p;++visible;
        require(!b.keepGeometry&&b.customMaterial,"custom material/geometry identity lost");require(b.templateSection<target.rows.size(),"appended section template invalid");
        require(b.vertices.size()/16<=60000&&b.palette.size()<=26,"native draw budget exceeded");
        require(b.indices.size()%3==0,"incomplete output triangle");require(b.indices.size()==b.vertices.size()/16,"unwelded corners missing or unused");
        std::vector<bool> used(b.vertices.size()/16,false);std::set<int> materialSlots;
        for(size_t n=0;n<b.indices.size();n+=3) {
            size_t tri=0;unsigned cornerMask=0;
            for(size_t c=0;c<3;++c){size_t ix=b.indices[n+c];require(ix<used.size()&&!used[ix],"truncated/duplicate local index");used[ix]=true;
                size_t identity=size_t(std::lround(b.vertices[ix*16+6]));require(identity<seen.size()&&!seen[identity],"source corner duplicated or lost");seen[identity]=true;
                if(c==0)tri=identity/3;
                require(identity/3==tri,"triangle connects unrelated source faces");cornerMask|=1u<<unsigned(identity%3);
                const size_t sourceTriangle=identity/3;
                const int material=sourceTriangle>=kMainVertices/3||sourceTriangle<33333
                    ?0:1+int((sourceTriangle-33333)/5440);
                materialSlots.insert(material);
            }
            require(cornerMask==7,"triangle corner identity changed");
        }
        require(materialSlots.size()==1,"distinct source material buckets merged");
        const std::string tex="CUSTOM_TEX_"+std::to_string(*materialSlots.begin());
        require(std::find(b.textureCandidates.begin(),b.textureCandidates.end(),tex)!=b.textureCandidates.end(),"material has another bucket's texture");
        require(b.texRelPath.count(tex)!=0,"authored texture path missing");
        const int material=*materialSlots.begin();
        require(std::fabs(b.customDiffuse[0]-float(material)/31.f)<1e-6f
            &&std::fabs(b.customDiffuse[1]-float(31-material)/31.f)<1e-6f
            &&b.customDiffuse[2]==.25f,"authored material color changed");
        textures.insert(tex);models.insert(b.sourceMeshName);vertices+=b.vertices.size()/16;triangles+=b.indices.size()/3;
    }
    require(vertices==kVertices&&triangles==kTriangles,"custom geometry missing");require(std::all_of(seen.begin(),seen.end(),[](bool b){return b;}),"unvisited source corner");
    require(textures.size()==32&&models.size()==3&&visible>target.rows.size(),"material or independent model omitted");
    std::cout<<"large "<<target.name<<" vertices="<<vertices<<" triangles="<<triangles<<" materials="<<textures.size()<<" models="<<models.size()<<" sections="<<visible<<" native_bones="<<target.ref.nbones<<" PASS\n";
}
void verifyInterleavedPaletteBatches() {
    auto target=syntheticTarget(96,1);modmesh::Scene scene;scene.srcName="Interleaved custom.fbx";
    configure(scene);scene.cfg.skin=1;
    modmesh::Model model;model.id=1;model.name="Body";model.type="Mesh";model.geoms={2};model.materials={3};
    scene.models[1]=model;scene.meshModelOrder={1};scene.materials[3]="Body material";
    modmesh::Geom geometry;geometry.id=2;geometry.matMapping="AllSame";geometry.matIdx={0};
    geometry.uv.valid=true;geometry.uv.comps=2;geometry.uv.mapping="ByPolygonVertex";geometry.uv.reference="Direct";
    for(int serial=0;serial<1920;++serial) {
        const int bone=serial%96,base=serial*3;const double z=double(serial/96)*.002;
        geometry.ctrl.insert(geometry.ctrl.end(),{double(bone),0.,z,double(bone),.1,z,double(bone),0.,z+.001});
        geometry.pvi.insert(geometry.pvi.end(),{base,base+1,-base-3});
        for(int corner=0;corner<3;++corner)geometry.uv.data.insert(geometry.uv.data.end(),{double(base+corner),0.});
        modmesh::GCluster cluster;cluster.boneName="bone_"+std::to_string(bone);
        cluster.idx={base,base+1,base+2};cluster.w={1,1,1};geometry.clusters.push_back(cluster);
        target.ref.clusterBoneIndices[cluster.boneName]=bone;
    }
    scene.geoms[2]=std::move(geometry);
    const auto built=modmesh::buildSectionsForMesh(scene,target.name,target.views(),target.ref);
    require(built.size()==4,"interleaved faces should reuse four safe palette draws");
    std::set<int> identities;size_t triangles=0;
    for(const auto& section:built) {
        require(section&&section->palette.size()<=26,"unsafe interleaved draw palette");
        triangles+=section->indices.size()/3;
        for(uint32_t index:section->indices) {
            require(index<section->vertices.size()/16,"interleaved face index out of range");
            const auto* vertex=section->vertices.data()+size_t(index)*16;
            const int identity=int(std::lround(vertex[6]));
            require(identity>=0&&identity<5760&&identities.insert(identity).second,"interleaved source corner changed or duplicated");
            require(section->palette.at(size_t(vertex[8]))==uint16_t((identity/3)%96)&&vertex[12]==1.f,"interleaved bone weight changed");
        }
    }
    require(triangles==1920&&identities.size()==5760,"interleaved source faces omitted");
    std::cout<<"interleaved 1920 triangles/96 bones/5760 corner identities retained in four draws PASS\n";
}
void verifySkin() {
    auto target=syntheticTarget(96,1);modmesh::Scene sc;sc.srcName="96Bones.FBX";configure(sc);sc.cfg.skin=1;
    modmesh::Model model;model.id=1;model.name="Model::Body";model.type="Mesh";model.geoms={2};model.materials={3};sc.models[1]=model;sc.meshModelOrder={1};sc.materials[3]="Material::Skin";
    sc.materials[3]=std::string("Skin.001\0\1Material",18);
    modmesh::Geom g;g.id=2;g.matMapping="AllSame";g.matIdx={0};g.uv.valid=true;g.uv.comps=2;g.uv.mapping="ByPolygonVertex";g.uv.reference="Direct";
    for(int bone=0;bone<96;++bone){int base=bone*3;g.ctrl.insert(g.ctrl.end(),{double(bone),0.,0.,double(bone),.1,0.,double(bone),0.,.1});g.pvi.insert(g.pvi.end(),{base,base+1,-base-3});g.uv.data.insert(g.uv.data.end(),{double(bone),0.,double(bone),0.,double(bone),0.});
        modmesh::GCluster c;c.boneName="bone_"+std::to_string(bone);c.idx={base,base+1,base+2};c.w={1.,1.,1.};g.clusters.push_back(c);target.ref.clusterBoneIndices[c.boneName]=bone;}
    sc.geoms[2]=std::move(g);const auto built=modmesh::buildSectionsForMesh(sc,target.name,target.views(),target.ref);size_t checked=0,triangles=0;
    for(const auto& p:built)if(p&&!p->hide){require(p->sourceMaterialName=="Skin.001","binary material type suffix blocks local texture discovery");require(p->palette.size()<=26,"bone palette overwrites native shader control constants");triangles+=p->indices.size()/3;
        for(size_t v=0;v<p->vertices.size();v+=16){int expected=int(std::lround(p->vertices[v+6]));float weight=0;
            for(size_t k=0;k<4;++k)if(p->vertices[v+12+k]>0){int slot=int(p->vertices[v+8+k]);require(slot>=0&&size_t(slot)<p->palette.size(),"weight references invalid palette slot");if(p->palette[size_t(slot)]==expected)weight+=p->vertices[v+12+k];}
            require(std::fabs(weight-1.f)<1e-6f,"authored bone weight changed by palette overflow");++checked;}}
    require(checked==288&&triangles==96&&built.size()==4,"bone palette was truncated instead of split into four safe draws");std::cout<<"skin 96 bones 288 authored weights preserved across "<<built.size()<<" sections PASS\n";
}
void verifySmallFaces() {
    auto target=syntheticTarget(60,1);modmesh::Scene sc;sc.srcName="Authored small faces.FBX";configure(sc);
    modmesh::Model model;model.id=1;model.name="Model::FineSurface";model.type="Mesh";model.geoms={2};model.materials={3};sc.models[1]=model;sc.meshModelOrder={1};sc.materials[3]="Material::FineSurface";
    modmesh::Geom g;g.id=2;g.matMapping="AllSame";g.matIdx={0};g.uv.valid=true;g.uv.comps=2;g.uv.mapping="ByPolygonVertex";g.uv.reference="Direct";
    // The first nonzero face is smaller than the old positional weld grid.
    // The other two intentionally occupy the same surface with distinct UVs.
    g.ctrl={0.,0.,0.,0.000001,0.,0.,0.,0.000001,0.,
        .1,0.,0.,.2,0.,0.,.1,.1,0.,.1,0.,0.,.2,0.,0.,.1,.1,0.};
    g.pvi={0,1,-3,3,4,-6,6,7,-9};
    for(int corner=0;corner<9;++corner)g.uv.data.insert(g.uv.data.end(),{double(corner),0.});
    sc.geoms[2]=std::move(g);
    for(bool weld:{false,true}) {
        sc.cfg.weld=weld;auto built=modmesh::buildSectionsForMesh(sc,target.name,target.views(),target.ref);std::set<int> seen;size_t triangles=0;
        for(const auto& p:built)if(p&&!p->hide){triangles+=p->indices.size()/3;
            for(auto ix:p->indices){require(ix<p->vertices.size()/16,"small face index outside buffer");seen.insert(int(std::lround(p->vertices[size_t(ix)*16+6])));}}
        require(triangles==3&&seen.size()==9,"small or coincident authored faces removed");
    }
    auto& near=sc.geoms.at(2);
    near.ctrl={0.,0.,0.,.1,0.,0.,0.,.1,0.,0.,0.,.000001,.1,0.,.000001,0.,.1,.000001};
    near.pvi={0,1,-3,3,4,-6};near.uv.data.assign(12,0.);
    near.nrm.valid=true;near.nrm.comps=3;near.nrm.mapping="ByPolygonVertex";near.nrm.reference="Direct";
    near.nrm.data={0.,0.,1.,0.,0.,1.,0.,0.,1.,0.,0.,1.,0.,0.,1.,0.,0.,1.};
    std::set<std::array<float,3>> expectedPositions;
    for(size_t i=0;i<near.ctrl.size();i+=3)expectedPositions.insert({float(near.ctrl[i]),float(near.ctrl[i+1]),float(near.ctrl[i+2])});
    sc.cfg.weld=true;const auto nearby=modmesh::buildSectionsForMesh(sc,target.name,target.views(),target.ref);
    std::set<std::array<float,3>> positions;size_t triangles=0;
    for(const auto& p:nearby)if(p&&!p->hide){triangles+=p->indices.size()/3;for(size_t v=0;v<p->vertices.size();v+=16)positions.insert({p->vertices[v],p->vertices[v+1],p->vertices[v+2]});}
    require(triangles==2&&positions==expectedPositions,"welding merged nearby distinct authored positions");
    std::cout<<"small nonzero and coincident authored triangles preserved with weld on/off PASS\n";
}
Bytes readFile(const char* path){std::ifstream f(path,std::ios::binary);Bytes b((std::istreambuf_iterator<char>(f)),{});require(!b.empty(),"cannot read fixture");return b;}
void verifyReal(const char* path,const char* pcmesh,size_t expectedTriangles,size_t expectedMaterials) {
    auto bytes=readFile(path);auto scene=modmesh::loadScene(path,bytes.data(),bytes.size());require(bool(scene),"real FBX rejected");configure(*scene);scene->cfg.fit=true;scene->cfg.weld=true;scene->cfg.skin=2;
    modmesh::pcmeshsource::Source native;std::string why;auto image=readFile(pcmesh);require(native.parse(std::move(image),&why),why.c_str());
    auto selected=native.replacementMeshNames("VENOM");require(!selected.empty(),"no native target LOD");
    for(const auto& mesh:native.meshes){if(std::find(selected.begin(),selected.end(),mesh.name)==selected.end())continue;
        Target t;t.name=mesh.name;t.ref.nbones=int(mesh.nbones);t.ref.customSource=true;t.ref.targetMeshNames=selected;t.ref.targetFileName="VENOM";
        t.ref.haveSphere=true;t.ref.sphereRadius=mesh.sphereRadius;
        std::copy(std::begin(mesh.sphereCenter),std::end(mesh.sphereCenter),t.ref.sphereCenter);
        t.ref.bonePos=mesh.bonePos;t.ref.haveBones=!mesh.bonePos.empty();
        for(size_t bone=0;bone<mesh.bonePos.size();bone+=3)for(size_t axis=0;axis<3;++axis){
            const float value=mesh.bonePos[bone+axis];
            if(bone==0)t.ref.bonesMin[axis]=t.ref.bonesMax[axis]=value;
            else{t.ref.bonesMin[axis]=std::min(t.ref.bonesMin[axis],value);t.ref.bonesMax[axis]=std::max(t.ref.bonesMax[axis],value);}}
        for(const auto& s:mesh.sections){require(s.stride==64,"non-skinned target unsupported by fixture");std::vector<float> row(size_t(s.vertexCount)*16);std::memcpy(row.data(),native.bytes.data()+s.vertexOffset,s.vertexBytes);t.rows.push_back(std::move(row));t.palettes.push_back(s.palette);}
        const auto built=modmesh::buildSectionsForMesh(*scene,t.name,t.views(),t.ref);size_t triangles=0,vertices=0;std::set<std::string> materials,models;
        for(const auto& p:built){require(bool(p),"real custom swap left native section");if(p->hide)continue;require(p->customMaterial&&!p->keepGeometry,"real source identity lost");require(p->templateSection<t.rows.size()&&p->palette.size()<=26&&p->vertices.size()/16<=60000,"real draw budget invalid");
            for(auto ix:p->indices)require(ix<p->vertices.size()/16,"real index outside vertex window");
            triangles+=p->indices.size()/3;vertices+=p->vertices.size()/16;materials.insert(p->sourceMaterialName);models.insert(p->sourceMeshName);}
        std::cout<<"real "<<path<<" -> "<<t.name<<" triangles="<<triangles<<" vertices="<<vertices<<" materials="<<materials.size()<<" models="<<models.size()<<" native_bones="<<t.ref.nbones<<"\n";
        require(triangles==expectedTriangles&&materials.size()==expectedMaterials,"real source faces/materials omitted");require(models.size()==scene->meshModelOrder.size(),"real source model omitted");
        std::cout<<"PASS\n";
    }
}
}
int main(int argc,char** argv) {
    try {
        const auto start=std::chrono::steady_clock::now();
        if(argc==5){verifyReal(argv[1],argv[2],size_t(std::stoull(argv[3])),size_t(std::stoull(argv[4])));return 0;}
        require(argc==1,"usage: probe [file.FBX target.PCMESH expectedTriangles expectedMaterials]");
        auto bytes=fixture();require(bytes.size()>40u*1024u*1024u,"fixture below 40 MiB");std::cout<<"binary FBX bytes="<<bytes.size()<<" pointer_bits="<<sizeof(void*)*8<<"\n";
        auto scene=modmesh::loadScene("Generated Forty MiB.FBX",bytes.data(),bytes.size());require(bool(scene),"valid large FBX rejected");bytes.clear();bytes.shrink_to_fit();configure(*scene);
        require(scene->meshModelOrder.size()==3&&scene->matTexStem.size()==32,"binary parser lost objects/materials");size_t controlPoints=0;for(const auto& g:scene->geoms)controlPoints+=g.second.ctrl.size()/3;require(controlPoints==kVertices,"binary parser truncated vertices");
        for(const char* name:{"venom000","venom001"}){auto target=syntheticTarget(60,std::string(name)=="venom000"?17:16);target.name=name;verifyGenerated(modmesh::buildSectionsForMesh(*scene,target.name,target.views(),target.ref),target);
            require(modmesh::buildSectionsForMesh(*scene,"venom_tentacle000",target.views(),target.ref).empty(),"custom body leaked into native auxiliary mesh");}
        verifySkin();verifySmallFaces();verifyInterleavedPaletteBatches();std::cout<<"PASS elapsed_seconds="<<std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count()<<"\n";return 0;
    }catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<"\n";return 1;}
}
