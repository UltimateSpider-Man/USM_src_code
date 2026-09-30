#include "../src/mod_mesh_texture_source.h"
#include <chrono>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace {
namespace fs=std::filesystem;
unsigned checks=0;
void require(bool value,const std::string& message){++checks;if(!value)throw std::runtime_error(message);}
void file(const fs::path& path){fs::create_directories(path.parent_path());std::ofstream f(path,std::ios::binary);f<<"image fixture";require(bool(f),"fixture creation failed");}
void selftest() {
    const auto unique=std::chrono::high_resolution_clock::now().time_since_epoch().count();
    const fs::path root=fs::temp_directory_path()/("usm-texture-source-"+std::to_string(unique));
    fs::create_directories(root);
    // Every cleanup target is the unique directory just created by this test.
    struct Cleanup{fs::path path;~Cleanup(){std::error_code ec;fs::remove_all(path,ec);}}cleanup{fs::canonical(root)};
    const auto a=root/"model-a",b=root/"model-b";const auto fbx=a/"Hero.fbx";
    file(fbx);file(b/"Hero.fbx");file(a/"Body.DDS");file(a/"body.png");file(b/"body.dds");
    fs::path out;std::string why;
    const auto resolve=[&](const std::string& ref,const std::string& stem,const fs::path& expected){
        require(modmesh::texturesource::resolveLocalImage(fbx,ref,stem,out,&why),why);
        require(out==fs::canonical(expected),"wrong source-local image: "+out.string());
    };
    resolve("body.jpg","BODY",a/"Body.DDS");
    resolve("C:\\OldMachine\\Project\\body.jpg","BODY",a/"Body.DDS");
    resolve("/foreign/export/body.jpg","BODY",a/"Body.DDS");
    resolve("body.PNG","BODY",a/"body.png"); // Authored reference wins over DDS preference.
    resolve("","body",a/"Body.DDS");
    require(modmesh::texturesource::resolveLocalImage(b/"Hero.fbx","body.jpg","body",out,&why)&&out==fs::canonical(b/"body.dds"),"same basename leaked between sources");
    resolve((a/"body.png").string(),"body",a/"body.png");
    file(a/"Hero.FBM"/"nested"/"Mouth.TGA");
    file(a/"mouth.jpg"); // Relocated basename must not beat the authored folder.
    resolve("hero.fbm\\NESTED\\mouth.jpg","MOUTH",a/"Hero.FBM"/"nested"/"Mouth.TGA");
    resolve("Z:\\Artist\\hero.fbm\\nested\\mouth.jpg","MOUTH",a/"Hero.FBM"/"nested"/"Mouth.TGA");
    file(a/"Hero.FBM"/"Eye.DDS");resolve("eye.jpg","EYE",a/"Hero.FBM"/"Eye.DDS");
    file(a/"other-mod"/"secret.dds");const auto unchanged=out;
    require(!modmesh::texturesource::resolveLocalImage(fbx,"secret.jpg","secret",out,&why)&&out==unchanged,"unreferenced descendant scan or failure output mutation");
    file(root/"outside.dds");
    for(const auto& ref:{"../outside.dds","..\\outside.dds","body.dds:stream","body.exe"})
        require(!modmesh::texturesource::resolveLocalImage(fbx,ref,"body",out,&why)&&out==unchanged,"unsafe/unsupported reference accepted");
    require(!modmesh::texturesource::resolveLocalImage(fbx,"","../body",out,&why),"stem traversal accepted");
    require(!modmesh::texturesource::resolveLocalImage(fbx,std::string("body\0.dds",9),"body",out,&why),"embedded NUL accepted");
    file(a/"mesh_medpoly_material0000_map_Kd.dds");
    resolve("","mesh_medpoly_material0000_map_Kd",a/"mesh_medpoly_material0000_map_Kd.dds");
    require(modmesh::texturesource::resolveMaterialImage(fbx,"MaTeRiAl0000",out,&why)
            &&out==fs::canonical(a/"mesh_medpoly_material0000_map_Kd.dds"),"material-only discovery depended on FBX/object prefix");
    file(a/"material0001_map_Kd.dds");
    require(modmesh::texturesource::resolveMaterialImage(fbx,"material0001",out,&why)
            &&out==fs::canonical(a/"material0001_map_Kd.dds"),"exact material albedo name not found");
    file(a/"Hero.FBM"/"old_export_material0002_map_KD.PNG");
    require(modmesh::texturesource::resolveMaterialImage(fbx,"material0002",out,&why)
            &&out==fs::canonical(a/"Hero.FBM"/"old_export_material0002_map_KD.PNG"),"own FBM material albedo not found");
    const auto materialUnchanged=out;
    file(a/"other-mod"/"old_export_material0003_map_Kd.dds");
    file(b/"foreign_material0003_map_Kd.dds");
    file(a/"material0003_normal.dds");file(a/"material0003_roughness.dds");file(a/"material0003_height.dds");
    file(a/"notmaterial0003_map_Kd.dds");file(a/"material00030_map_Kd.dds");file(a/"old_material0003_map_Kd_normal.dds");
    require(!modmesh::texturesource::resolveMaterialImage(fbx,"material0003",out,&why)&&out==materialUnchanged,
            "material discovery accepted non-albedo, partial identity, unrelated folder, or mutated output on failure");
    file(a/"other_export_material0000_map_Kd.dds");
    require(!modmesh::texturesource::resolveMaterialImage(fbx,"material0000",out,&why)&&out==materialUnchanged,
            "material discovery guessed between two exporter prefixes");
    file(a/"material0001_map_Kd.png");
    require(!modmesh::texturesource::resolveMaterialImage(fbx,"material0001",out,&why),"material inference chose between two source images");
    file(a/"Hero.FBM"/"material0004_map_Kd.dds");file(a/"material0004_map_Kd.dds");
    require(!modmesh::texturesource::resolveMaterialImage(fbx,"material0004",out,&why),"root/FBM material ambiguity accepted");
    require(!modmesh::texturesource::resolveMaterialImage(fbx,"../material0000",out,&why),"material traversal accepted");
    require(!modmesh::texturesource::resolveMaterialImage(fbx,std::string("mat\0name",8),out,&why),"material embedded NUL accepted");
    file(a/"venom mouth tounge teath texture.dds");
    resolve("venom mouth tounge teath texture.jpg","VENOM MOUTH TOUNGE TEATH TEXTURE",a/"venom mouth tounge teath texture.dds");
    // The pure matching primitive rejects two equal-priority candidates on
    // case-sensitive filesystems. Windows cannot create this ambiguity.
    file(a/"Ambiguous.dds");file(a/"ambiguous.DDS");
    std::error_code ec;
    if(!fs::equivalent(a/"Ambiguous.dds",a/"ambiguous.DDS",ec))
        require(!modmesh::texturesource::resolveLocalImage(fbx,"ambiguous.jpg","ambiguous",out,&why),"case-insensitive duplicate accepted");
    fs::create_directory_symlink(root,a/"escape",ec);
    if(!ec)require(!modmesh::texturesource::resolveLocalImage(fbx,"escape/outside.dds","outside",out,&why),"symlink source escape accepted");
}
void actual(const fs::path& venom,const fs::path& miles) {
    fs::path out;std::string why;
    for(const char* stem:{"venom body texture","venom mouth tounge teath texture","venom body height"}){
        require(modmesh::texturesource::resolveLocalImage(venom,std::string(stem)+".jpg",stem,out,&why),why);
        require(out==fs::canonical(venom.parent_path()/(std::string(stem)+".dds")),"actual Venom reference mismatch");
    }
    for(int i=0;i<19;++i){
        const auto material="material"+std::string(i<10?"000":"00")+std::to_string(i);
        const auto stem="mesh_medpoly_"+material+"_map_Kd";
        require(modmesh::texturesource::resolveMaterialImage(miles,material,out,&why),why);
        require(out==fs::canonical(miles.parent_path()/(stem+".dds")),"actual Miles material mismatch");
    }
    std::cout<<"PASS real Venom3 authored JPG-to-DDS references and Miles19 material-only albedo discoveries\n";
}
}
int main(int argc,char**argv){try{selftest();if(argc==4&&std::string(argv[1])=="--assets")actual(argv[2],argv[3]);else if(argc!=1)throw std::runtime_error("usage: probe [--assets venom.fbx miles.fbx]");std::cout<<"PASS source-local texture checks="<<checks<<'\n';return 0;}catch(const std::exception&e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
