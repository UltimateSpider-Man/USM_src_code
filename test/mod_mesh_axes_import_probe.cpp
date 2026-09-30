#include "../src/mod_mesh_import.h"
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace {
using Vec=std::array<double,3>;
void require(bool value,const char *why) {if(!value)throw std::runtime_error(why);}
void equal(const Vec &actual,const Vec &expected,const char *why,double tolerance=1e-5) {
    for(unsigned d=0;d<3;++d)require(std::abs(actual[d]-expected[d])<=tolerance,why);
}
std::string fixture(const std::string &axes,const std::string &properties="Properties70",bool normals=true) {
    std::ostringstream out;
    out<<"; FBX 7.4.0 project file\nGlobalSettings: {\n"<<properties<<": {\n"
       <<"P: \"UnitScaleFactor\", \"double\", \"Number\", \"\", 50\n"<<axes<<"\n}\n}\n";
    out<<R"fbx(
Objects: {
  Model: 100, "Model::Root", "Null" {
    Properties70: {
      P: "Lcl Translation", "Lcl Translation", "", "A", 10,20,30
      P: "Lcl Rotation", "Lcl Rotation", "", "A", 0,0,90
      P: "Lcl Scaling", "Lcl Scaling", "", "A", 2,3,4
    }
  }
  Model: 101, "Model::ImportedSurface", "Mesh" {
    Properties70: {
      P: "Lcl Translation", "Lcl Translation", "", "A", 1,2,3
      P: "GeometricTranslation", "Vector3D", "Vector", "", 4,5,6
    }
  }
  Geometry: 102, "Geometry::ImportedSurface", "Mesh" {
    Vertices: *9 { a: 0,0,0, 2,0,0, 0,1,0 }
    PolygonVertexIndex: *3 { a: 0,1,-3 }
    LayerElementUV: 0 {
      MappingInformationType: "ByPolygonVertex"
      ReferenceInformationType: "Direct"
      UV: *6 { a: 0,0.25, 1,0.25, 2,0.25 }
    }
)fbx";
    if(normals)out<<R"fbx(
    LayerElementNormal: 0 {
      MappingInformationType: "ByPolygonVertex"
      ReferenceInformationType: "Direct"
      Normals: *9 { a: 0,0,1, 0,0,1, 0,0,1 }
    }
)fbx";
    out<<R"fbx(
    LayerElementMaterial: 0 {
      MappingInformationType: "AllSame"
      ReferenceInformationType: "IndexToDirect"
      Materials: *1 { a: 0 }
    }
  }
  Material: 103, "Material::OriginalMaterial", "" { }
  Model: 104, "Model::Bone_0", "LimbNode" { }
  Deformer: 105, "Deformer::Skin", "Skin" { }
  Deformer: 106, "SubDeformer::Cluster", "Cluster" {
    Indexes: *3 { a: 0,1,2 }
    Weights: *3 { a: 1,1,1 }
    TransformLink: *16 { a: 0,2,0,0, -3,0,0,0, 0,0,4,0, 10,20,30,1 }
  }
}
Connections: {
  C: "OO",100,0
  C: "OO",101,100
  C: "OO",102,101
  C: "OO",103,101
  C: "OO",104,100
  C: "OO",104,106
  C: "OO",106,105
  C: "OO",105,102
}
)fbx";
    return out.str();
}
std::string settings(int up,int upSign,int front,int frontSign,int right,int rightSign) {
    std::ostringstream out;
    const char *names[]={"UpAxis","UpAxisSign","FrontAxis","FrontAxisSign","CoordAxis","CoordAxisSign"};
    const int values[]={up,upSign,front,frontSign,right,rightSign};
    for(int i=0;i<6;++i)out<<"P: \""<<names[i]<<"\", \"int\", \"Integer\", \"\", "<<values[i]<<'\n';
    return out.str();
}
struct Frame {
    std::string metadata;
    Vec positions[3];
    Vec normal,pivot,bindPoint;
};
void checkFrame(const Frame &frame,const std::string &properties,bool normals) {
    const auto text=fixture(frame.metadata,properties,normals);
    auto scene=modmesh::parseScene("axis_import_synthetic.fbx",text.data(),text.size());
    require(bool(scene)&&scene->meshModelOrder.size()==1,"ASCII fixture must parse one source mesh");
    require(scene->sceneScale==.5,"FBX units must be retained separately from axis conversion");
    auto &geometry=scene->geoms.at(102);
    require(geometry.clusters.size()==1&&geometry.clusters[0].haveLink,"authored cluster binding must survive parsing");
    const auto &cluster=geometry.clusters[0];
    equal({{cluster.linkPos[0],cluster.linkPos[1],cluster.linkPos[2]}},frame.pivot,
        "global bone pivot must use the same file-to-game axes");
    const auto &bind=cluster.linkMatrix;
    const Vec local{{2,-3,5}};
    Vec bindPoint{};
    for(unsigned d=0;d<3;++d)bindPoint[d]=local[0]*bind[d]+local[1]*bind[4+d]+local[2]*bind[8+d]+bind[12+d];
    equal(bindPoint,frame.bindPoint,"complete global bind basis must change output axes only");

    scene->cfg.custom=true;scene->cfg.skin=1;scene->cfg.fit=false;scene->cfg.anim=false;
    scene->cfg.retarget=false;scene->cfg.weld=false;scene->cfg.roundtripEps=0;
    modmesh::OrigMeshRef ref;ref.nbones=1;ref.customSource=true;ref.targetFileName="test_native";
    ref.clusterBoneIndices={{cluster.boneName,0}};
    const uint16_t palette[]={0};
    const float vertices[]={0,0,0,0,0,1,0,0,0,-1,-1,-1,1,0,0,0,
        1,0,0,0,0,1,1,0,0,-1,-1,-1,1,0,0,0,0,1,0,0,0,1,0,1,0,-1,-1,-1,1,0,0,0};
    modmesh::OrigSectionView view;view.verts=vertices;view.nverts=3;view.nbones=1;view.palette=palette;view.strideBytes=64;
    const auto built=modmesh::buildSectionsForMesh(*scene,"test_native",{view},ref);
    require(built.size()==1&&built[0]&&!built[0]->hide,"public builder must produce one complete draw");
    const auto &part=*built[0];
    require(part.indices.size()==3&&part.vertices.size()==48,"source triangle and all UV identities must survive");
    require(part.sourceMaterialName=="OriginalMaterial","source material must retain ownership");
    bool seen[3]={false,false,false};
    for(size_t v=0;v<part.vertices.size();v+=16) {
        const auto &row=part.vertices;const int id=int(std::lround(row[v+6]));
        require(id>=0&&id<3&&!seen[id],"each authored UV identity must occur once");seen[id]=true;
        equal({{row[v],row[v+1],row[v+2]}},frame.positions[id],"axis and unit transforms must apply once after nested and geometric transforms");
        if(normals)equal({{row[v+3],row[v+4],row[v+5]}},frame.normal,"reflections must preserve outward normal direction");
        require(row[v+7]==.75f,"FBX V conversion must remain unchanged");
        require(row[v+12]==1&&part.palette[size_t(row[v+8])]==0,"explicit authored skin must survive axis conversion");
    }
    Vec p[3];
    for(unsigned k=0;k<3;++k)for(unsigned d=0;d<3;++d)p[k][d]=part.vertices[size_t(part.indices[k])*16+d];
    Vec a{},b{};for(unsigned d=0;d<3;++d){a[d]=p[1][d]-p[0][d];b[d]=p[2][d]-p[0][d];}
    const Vec cross{{a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]}};
    double facing=0;for(unsigned d=0;d<3;++d)facing+=cross[d]*frame.normal[d];
    require(facing<0,"game clockwise winding must survive axis reflection, including absent normals");
}
void malformedMetadata() {
    const std::string good=settings(1,1,2,1,0,1);
    for(const char *bad:{"4294967296","0.5","\"invalid\""}) {
        std::string metadata=good;
        metadata+="P: \"UpAxis\", \"int\", \"Integer\", \"\", ";metadata+=bad;metadata+='\n';
        const auto text=fixture(metadata);auto scene=modmesh::parseScene("axis_invalid_synthetic.fbx",text.data(),text.size());
        require(bool(scene)&&scene->axisTransform.identity(),"invalid metadata must keep deterministic identity frame");
        // Use a nonidentity otherwise-valid frame so truncation cannot hide
        // invalid integer, fractional, or string metadata behind identity.
        metadata=settings(2,1,1,-1,0,1);
        metadata+="P: \"CoordAxis\", \"int\", \"Integer\", \"\", ";metadata+=bad;metadata+='\n';
        const auto invalid=fixture(metadata);scene=modmesh::parseScene("axis_invalid_synthetic.fbx",invalid.data(),invalid.size());
        require(bool(scene)&&scene->axisTransform.identity(),"malformed scalar metadata must reject the whole axis frame");
    }
}
void animationPolicy() {
    for(bool converted:{false,true}) {
        const auto text=fixture(converted?settings(2,1,1,-1,0,1):"");
        modmesh::FbxNode root;
        require(modmesh::fbxtxt::parse(text.data(),text.size(),root),"animation policy fixture parses");
        modmesh::Scene scene;scene.cfg.anim=true;
        modmesh::detail::buildScene(root,scene);
        require(scene.cfg.anim!=converted,"nonidentity axis conversion must disable unsupported imported local animation channels");
        require(scene.meshModelOrder.size()==1,"animation policy must retain converted mesh geometry");
    }
}
}
int main() {
    try {
        const Frame identity{"",{{{-5.5,15,33}},{{-5.5,17,33}},{{-7,15,33}}},{{0,0,1}},{{10,20,30}},{{19,24,50}}};
        const Frame zup{settings(2,1,1,-1,0,1),{{{-5.5,33,-15}},{{-5.5,33,-17}},{{-7,33,-15}}},{{0,1,0}},{{10,30,-20}},{{19,50,-24}}};
        const Frame reflected{settings(1,1,2,1,0,-1),{{{5.5,15,33}},{{5.5,17,33}},{{7,15,33}}},{{0,0,1}},{{-10,20,30}},{{-19,24,50}}};
        for(const auto &frame:{identity,zup,reflected})for(const char *properties:{"Properties70","Properties60"})
            for(bool normals:{true,false})checkFrame(frame,properties,normals);
        malformedMetadata();
        animationPolicy();
        std::cout<<"PASS FBX axis import: ASCII GlobalSettings70/60, nested and geometric transforms, units once, full bind pivots/bases, normals and reflected winding, source skin/materials/UVs, malformed metadata, imported animation policy\n";
        return 0;
    } catch(const std::exception &e) {std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}
}
