#include "../src/mod_pcskel_source.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>

using modmesh::pcskelsource::Source;

static void check(bool value, const char *message) {
    if (!value) throw std::runtime_error(message);
}
static void put(std::vector<uint8_t> &b, size_t o, uint32_t value) {
    for (int i=0;i<4;++i) b.at(o+i)=uint8_t(value>>(8*i));
}
static void label(std::vector<uint8_t> &b, size_t o, const char *text) {
    put(b,o,0x12345678u);
    std::memcpy(b.data()+o+4,text,std::strlen(text));
}
static std::vector<uint8_t> fixture() {
    std::vector<uint8_t> b(0x150);
    put(b,4,0x10003); label(b,8,"fixture"); label(b,0x28,"character");
    put(b,0x60,1); put(b,0x64,1); put(b,0x6c,0x40);
    put(b,0x70,0x90); put(b,0x74,0xa0); put(b,0x78,0x110);
    put(b,0x90,1); put(b,0x94,0xc5e45dcf); put(b,0x98,4);
    put(b,0xa0,1); put(b,0xa4,0x10);
    put(b,0xb0,1); put(b,0xb4,1); put(b,0xb8,1);
    put(b,0xc8,0x20); put(b,0xcc,0x50);
    label(b,0xd0,"root"); put(b,0xf4,0xffff0000u); put(b,0xf8,0x00010001u);
    put(b,0x100,0); put(b,0x110,1); put(b,0x114,0x10);
    put(b,0x120,1); put(b,0x124,2); put(b,0x13c,0x3f800000);
    put(b,0x140,0x3f800000); put(b,0x144,0x40000000); put(b,0x148,0x40400000);
    return b;
}
static std::vector<uint8_t> genericFixture() {
    std::vector<uint8_t> b(0x174);
    put(b,4,0x10200); label(b,8,"generic_fixture"); label(b,0x28,"generic");
    put(b,0x60,1); put(b,0x74,1); put(b,0x80,1); put(b,0x88,1);
    put(b,0x90,0x1c); put(b,0x94,4); put(b,0xb0,4); put(b,0xc4,4);
    put(b,0x64,2); put(b,0x6c,2);
    b[0xe0]=2; b[0xe2]=8;
    label(b,0xe4,"root"); put(b,0x108,44); put(b,0x10c,28); put(b,0x110,0xffffffffu);
    label(b,0x114,"nal_entropyposition"); put(b,0x13c,1);
    return b;
}
static std::vector<uint8_t> reorderedGenericFixture(bool unpacked=false) {
    std::vector<uint8_t> program={2,0,2,4,0x80,1,0x80,2,0x80,3,
                                      1,1,1,2,1,3,12,0,1,12,1,2,12,0,3};
    std::vector<uint8_t> hierarchy={8,0,8,4,13,0,1,7,1,13,1,2,7,2,13,0,3,7,3};
    if (unpacked) {
        program[6]=0;
        hierarchy.insert(hierarchy.begin()+12,{6,2});
    }
    const size_t table=(0xe0+program.size()+hierarchy.size()+3)&~size_t(3);
    const size_t component=table+5*0x30,pose=component+0x30;
    std::vector<uint8_t> b(pose+0x80);
    put(b,4,0x10200);label(b,8,"reordered_fixture");label(b,0x28,"generic");
    put(b,0x60,5);put(b,0x64,uint32_t(program.size()));put(b,0x6c,uint32_t(hierarchy.size()));
    put(b,0x74,5);put(b,0x80,5);put(b,0x88,1);put(b,0x90,0x80);
    put(b,0x94,4);put(b,0xb0,4);put(b,0xc4,4);
    std::copy(program.begin(),program.end(),b.begin()+0xe0);
    std::copy(hierarchy.begin(),hierarchy.end(),b.begin()+0xe0+program.size());
    const char *names[]={"body_root","effect_root","spine","hand","leg"};
    const uint32_t q[]={28,56,72,88,104},p[]={44,72,0,12,24};
    const uint32_t parent[]={0xffffffffu,0xffffffffu,0,2,0};
    for (int i=0;i<5;++i) {
        const auto row=table+size_t(i)*0x30;
        label(b,row,names[i]);put(b,row+0x20,i>=2 && !(unpacked && i==3));put(b,row+0x24,p[i]);
        put(b,row+0x28,q[i]);put(b,row+0x2c,parent[i]);
    }
    label(b,component,"nal_entropyposition");put(b,component+0x28,5);
    return b;
}
static std::vector<uint8_t> pedestrianFixture() {
    std::vector<uint8_t> b(0x2d0);
    put(b,4,0x500); label(b,8,"ped_fixture"); label(b,0x28,"ped");
    put(b,0x7c,0x3f800000);
    for (uint32_t i=0;i<21;++i) put(b,0x210+4*i,i);
    // Distinct local positions expose mistaken record/output index reuse.
    for (uint32_t i=1;i<21;++i) put(b,0x120+(i-1)*12,0x3f800000+i*0x10000);
    return b;
}
static std::vector<uint8_t> cameraFixture() {
    std::vector<uint8_t> b(0x90);
    put(b,4,0x10000); label(b,8,"camera_fixture"); label(b,0x28,"camera");
    put(b,0x64,0x3f490fdb); put(b,0x68,0x4b189680); put(b,0x7c,0x3f800000);
    return b;
}
static std::vector<uint8_t> panelFixture() {
    std::vector<uint8_t> b(0xb8);
    put(b,4,0x300); label(b,8,"panel_fixture"); label(b,0x28,"panel");
    put(b,0x64,1); put(b,0x6c,0x18); put(b,0x70,0x90); put(b,0x74,0xa0); put(b,0x78,0xa0);
    put(b,0x90,0x55667788); put(b,0x94,0x909beafd);
    put(b,0xa0,1); put(b,0xa4,8);
    return b;
}
static void fixedFamilyTests() {
    Source source; std::string why;
    auto bytes=pedestrianFixture();
    check(source.parse(bytes,&why),why.c_str());
    check(source.renderBoneCount==21 && source.bones.size()==21
        && source.fixedPoseOffset==0x60 && source.fixedPoseSize==0xc0,"pedestrian fixed layout");
    check(source.bones[2].name=="SPINE1" && source.bones[2].parent==1
        && source.bones[5].parent==2 && source.bones[13].parent==0
        && source.bones[20].name=="R_TOE" && source.bones[20].parent==19,
        "pedestrian canonical bone channels and anatomical parents");
    check(source.bones[0].hasLocalBind && source.bones[1].hasLocalPosition
        && !source.bones[1].hasLocalBind,"pedestrian known positions do not invent local quaternions");
    const auto original=source.bones;
    put(bytes,0x210,20); put(bytes,0x260,0);
    check(source.parse(bytes,&why),why.c_str());
    check(source.bones[20].name=="PELVIS" && source.bones[1].parent==20
        && source.bones[13].parent==20 && source.bones[17].parent==20
        && source.bones[0].name=="R_TOE" && source.bones[0].parent==19
        && source.bones[0].localPosition==original[20].localPosition,
        "pedestrian output permutation remaps names, parents and bind offsets");
    for (int mutation=0;mutation<8;++mutation) {
        auto bad=pedestrianFixture();
        switch(mutation) {
        case 0:bad.resize(0x2cf);break;
        case 1:bad[0x2c]='x';break;
        case 2:put(bad,0x214,0);break;
        case 3:put(bad,0x260,21);break;
        case 4:put(bad,0x120,0x7fc00000);break;
        case 5:put(bad,0x7c,0);break;
        case 6:put(bad,0x60,0x12340000);break;
        case 7:put(bad,0x270,0x7f800000);break;
        }
        check(!source.parse(bad,&why),"malformed pedestrian skeleton was accepted");
        check(source.bytes.empty() && source.bones.empty(),"failed pedestrian parse retained state");
    }
    std::cout<<"PASS pedestrian fixed channels, arbitrary output permutation + 8 malformed guards\n";
    bytes=cameraFixture();
    check(source.parse(bytes,&why),why.c_str());
    check(source.renderBoneCount==0 && source.bones.empty() && source.components.empty()
        && source.fixedPoseOffset==0x60 && source.fixedPoseSize==0x30,
        "camera trajectory does not invent render bones");
    for (int mutation=0;mutation<8;++mutation) {
        auto bad=bytes;
        switch(mutation) {
        case 0:bad.resize(0x8f);break;
        case 1:bad[0x2c]='x';break;
        case 2:put(bad,0x64,0x7fc00000);break;
        case 3:put(bad,0x68,0x7f800000);break;
        case 4:put(bad,0x7c,0);break;
        case 5:put(bad,0x60,1);break;
        case 6:put(bad,0x80,0x7fc00000);break;
        case 7:put(bad,4,0x10001);break;
        }
        check(!source.parse(bad,&why),"malformed camera skeleton was accepted");
        check(source.bytes.empty() && source.bones.empty(),"failed camera parse retained state");
    }
    std::cout<<"PASS camera fixed trajectory pose + 8 malformed guards\n";
    bytes=panelFixture();
    check(source.parse(bytes,&why),why.c_str());
    check(source.bones.empty() && source.renderBoneCount==0 && source.components.size()==1
        && source.components[0].typeHash==0x909beafd && source.components[0].defaultPoseOffset==0xa8
        && source.components[0].defaultPoseSize==0x10,"panel component pose has no skin skeleton");
    for (int mutation=0;mutation<8;++mutation) {
        auto bad=bytes;
        switch(mutation) {
        case 0:bad.resize(0xb7);break;
        case 1:bad[0x2c]='x';break;
        case 2:put(bad,0x70,0xfffffff0);break;
        case 3:put(bad,0x64,129);break;
        case 4:put(bad,0x94,0x12345678);break;
        case 5:put(bad,0x98,4);break;
        case 6:put(bad,0xa4,0x10);break;
        case 7:put(bad,0xa0,0);break;
        }
        check(!source.parse(bad,&why),"malformed panel skeleton was accepted");
        check(source.bytes.empty() && source.components.empty(),"failed panel parse retained state");
    }
    std::cout<<"PASS panel component directory + 8 malformed guards\n";
}
static void selftest() {
    Source s; std::string why;
    auto b=fixture();
    for (const auto characterVersion : {0x10002u,0x10003u}) {
    b=fixture(); put(b,4,characterVersion);
    check(s.parse(b,&why),why.c_str());
    check(s.version==characterVersion && s.bytes==b,"character metadata rewrote source version/bytes");
    check(s.renderBoneCount==1 && s.bones.size()==1,"fixture output count");
    check(s.bones[0].name=="root" && s.bones[0].parent==-1,"fixture name/parent");
    check(s.bones[0].hasLocalBind && s.bones[0].localPosition[2]==3,"fixture pose position");
    check(s.bones[0].localQuaternion[3]==1,"fixture pose rotation");
    for (int mutation=0;mutation<8;++mutation) {
        auto bad=b;
        switch(mutation) {
        case 0: bad.resize(80); break;
        case 1: put(bad,4,0x10200); break;
        case 2: put(bad,0x70,0xfffffff0); break;
        case 3: put(bad,0xa4,0xfffffffc); break;
        case 4: put(bad,0xf4,0); break; // self-parent
        case 5: put(bad,0xf0,1000); break; // quaternion index
        case 6: put(bad,0x140,0x7fc00000); break; // non-finite position
        case 7: put(bad,0x100,1); break; // order outside record table
        }
        check(!s.parse(std::move(bad),&why),"malformed fixture was accepted");
        check(s.bytes.empty() && s.bones.empty(),"failed parse retained partial state");
    }
    std::cout<<"PASS synthetic character v"<<std::hex<<characterVersion<<std::dec<<" + 8 malformed guards\n";
    }
    b=genericFixture();
    check(s.parse(b,&why),why.c_str());
    check(s.bones.size()==1 && s.bones[0].name=="root" && s.bones[0].parent==-1,
          "generic fixture bone order/parent");
    check(s.components.size()==1 && s.components[0].trackCount==1
          && s.components[0].defaultPoseOffset==0x144,"generic component tracks");
    for (int mutation=0;mutation<8;++mutation) {
        auto bad=b;
        switch(mutation) {
        case 0: bad.resize(0x150); break;
        case 1: put(bad,0x64,0xffffffffu); break;
        case 2: put(bad,0x74,4097); break;
        case 3: put(bad,0x94,3); break;
        case 4: put(bad,0x13c,2); break;
        case 5: put(bad,0x110,0); break;
        case 6: put(bad,0x110,2); break;
        case 7: put(bad,0x104,2); break;
        }
        check(!s.parse(std::move(bad),&why),"malformed generic fixture was accepted");
        check(s.bytes.empty() && s.bones.empty(),"failed generic parse retained partial state");
    }
    std::cout<<"PASS synthetic generic skeleton + 8 malformed guards\n";
    b=reorderedGenericFixture();
    check(s.parse(b,&why),why.c_str());
    check(s.bones[1].name=="spine" && s.bones[1].parent==0
          && s.bones[2].name=="hand" && s.bones[2].parent==1
          && s.bones[3].name=="leg" && s.bones[3].parent==0
          && s.bones[4].name=="effect_root" && s.bones[4].parent==-1,
          "generic name table was mistaken for output order");
    check(s.bones[1].genericRecordIndex==2 && s.bones[4].genericRecordIndex==1,
          "generic original record identity was lost");
    for (int mutation=0;mutation<8;++mutation) {
        auto bad=b;
        const size_t table=0x10c;
        switch(mutation) {
        case 0: bad[0xe0]=3;break; // unknown opcode
        case 1: bad[0xe7]=1;break; // duplicate rotation destination
        case 2: bad[0xe3]=5;break; // output outside declared extent
        case 3: bad[0xe4]=0;break; // wrong channel type
        case 4: bad[0xe0+17]=4;break; // bytecode parent disagrees with named parent
        case 5: put(bad,table+2*0x30+0x28,28);break; // overlapping channels
        case 6: bad[0xe0+25+4]=0x0c;break; // unknown hierarchy opcode
        case 7: bad[0xe0+25+10]=4;break; // hierarchy parent disagrees
        }
        check(!s.parse(std::move(bad),&why),"malformed output program was accepted");
        check(s.bytes.empty() && s.bones.empty(),"failed program parse retained state");
    }
    std::cout<<"PASS explicit reordered generic outputs + 8 bytecode guards\n";
    b=reorderedGenericFixture(true);
    check(s.parse(b,&why),why.c_str());
    check(s.bones[2].name=="hand" && s.bones[2].parent==1,
          "mixed packed/unpacked output order");
    b[0xe0+25+12]=7; // Removing the unpacked prepass is not an extra finalize.
    check(!s.parse(b,&why),"missing unpacked prepass was accepted");
    b=reorderedGenericFixture(true);
    b[0xe0+25+14]=6; // A second prepass must still be rejected.
    check(!s.parse(b,&why),"duplicate unpacked prepass was accepted");
    std::cout<<"PASS mixed generic channel phases + 2 malformed guards\n";
}
int main(int argc,char **argv) {
    try {
        selftest();
        fixedFamilyTests();
        unsigned good=0, unsupported=0, failed=0;
        auto inspect=[&](const std::filesystem::path &path) {
            std::ifstream in(path,std::ios::binary);
            check(bool(in),"cannot open PCSKEL input");
            std::vector<uint8_t> b((std::istreambuf_iterator<char>(in)),{});
            const uint32_t version=b.size()<8?0:uint32_t(b[4])|uint32_t(b[5])<<8
                |uint32_t(b[6])<<16|uint32_t(b[7])<<24;
            const bool supported=version==0x10002 || version==0x10003 || version==0x10200 || version==0x500
                || version==0x10000 || version==0x300;
            Source s;std::string why;
            if (!s.parse(std::move(b),&why)) {
                if (!supported) { ++unsupported; return; }
                std::cerr<<"FAIL "<<path.filename().string()<<": "<<why<<"\n";
                ++failed;return;
            }
            ++good;
            if (s.version==0x500) {
                check(s.bones.size()==21 && s.renderBoneCount==21,"native pedestrian 21 outputs");
                check(s.bones[0].name=="PELVIS" && s.bones[5].name=="L_CLAVICLE"
                    && s.bones[5].parent==2 && s.bones[20].name=="R_TOE",
                    "native pedestrian member identities");
            }
            if (s.version==0x10000)
                check(s.bones.empty() && s.fixedPoseSize==0x30,"native camera pose without skin bones");
            if (s.version==0x300)
                check(s.bones.empty() && s.components.size()==12,"native panel twelve components without skin bones");
            if (s.name=="venom") {
                check(s.renderBoneCount==91 && s.bones.size()==93,"native Venom render/helpers");
                check(s.bones[5].name=="NECK" && s.bones[5].parent==4,"native Venom neck intermediary");
                check(s.bones[61].name=="uprighttent_6" && s.bones[61].parent==60,"native Venom tentacle topology");
                check(s.bones[73].name=="tongue_6" && s.bones[73].parent==6,"native Venom tongue topology");
            }
            if (s.name=="usm_blacksuit" || s.name=="ultimate_spiderman") {
                check(s.renderBoneCount==66 && s.bones.size()==68,"native Spider-Man render/helpers");
                check(s.bones[4].name=="NECK" && s.bones[4].parent==3,"native Spider-Man neck");
                check(s.bones[58].name=="L_THIGH" && s.bones[58].parent==0,"native Spider-Man left leg");
                if (s.version==0x10002) {
                    check(s.components.size()==7 && s.components[3].typeHash==0xe7d9a8d3
                        && s.components[3].defaultPoseOffset==0x970
                        && s.components[3].defaultPoseSize==0x90
                        && s.components[4].defaultPoseOffset==0xa00
                        && s.components[5].defaultPoseOffset==0xa30,
                        "installed v2 finger/default pose layout was mistaken for v3");
                }
            }
            if (s.name=="venom_eddie") {
                check(s.renderBoneCount==106 && s.bones.size()==108,"native Eddie output count");
                check(s.bones[0].name=="bip01 pelvis" && s.bones[1].name=="bip01 spine"
                      && s.bones[73].name=="dummy01" && s.bones[73].parent==-1,
                      "native Eddie render order");
                check(s.bones[5].name=="bip01 neck1" && s.bones[5].parent==4,
                      "native Eddie neck hierarchy");
                check(s.bones[30].name=="bip01 r clavicle" && s.bones[30].parent==4
                      && s.bones[61].name=="bip01 r thigh" && s.bones[61].parent==0
                      && s.bones[64].name=="bip01 r toe0" && s.bones[64].parent==63,
                      "native Eddie 65-bone mesh prefix");
                for (uint32_t i=1;i<65;++i)
                    check(s.bones[i].genericRecordIndex==i+1,"native Eddie compact palette record identity");
            }
        };
        for (int i=1;i<argc;++i) {
            std::filesystem::path path(argv[i]);
            if (std::filesystem::is_directory(path)) {
                for (const auto &entry:std::filesystem::directory_iterator(path))
                    if (entry.is_regular_file() && Source::normalize(entry.path().extension().string())==".pcskel") inspect(entry.path());
            } else inspect(path);
        }
        std::cout<<"PCSKEL inputs accepted="<<good<<" unsupported-family="<<unsupported<<" invalid-supported="<<failed<<"\n";
        return failed?1:0;
    } catch (const std::exception &e) {
        std::cerr<<"FAIL "<<e.what()<<"\n";return 1;
    }
}
