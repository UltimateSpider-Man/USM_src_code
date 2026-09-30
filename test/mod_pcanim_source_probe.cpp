#include "../src/mod_pcanim_source.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <stdexcept>

namespace {
unsigned checks=0;
void require(bool ok,const char*message){++checks;if(!ok)throw std::runtime_error(message);}
std::vector<uint8_t> read(const std::filesystem::path&p){std::ifstream f(p,std::ios::binary);if(!f)throw std::runtime_error("cannot open "+p.string());return {std::istreambuf_iterator<char>(f),{}};}
void name(std::vector<uint8_t>&b,size_t p,uint32_t hash,const char*text){modanim::put32(b,p,hash);std::memset(b.data()+p+4,0,28);std::memcpy(b.data()+p+4,text,std::strlen(text));}
void scalar(std::vector<uint8_t>&b,size_t p,float value){std::memcpy(b.data()+p,&value,4);}
std::vector<uint8_t> fixture(bool reversed=false,size_t clipSize=0xb0){
    const size_t first=0x90;std::vector<uint8_t>b(first+clipSize*2,0);
    modanim::put32(b,0,0x10101);modanim::put32(b,8,64);modanim::put32(b,12,2);name(b,0x10,99,"allanims");
    modanim::put32(b,0x30,1);modanim::put32(b,0x34,uint32_t(first));
    name(b,0x48,reversed?202:101,reversed?"rig_b":"rig_a");name(b,0x68,reversed?101:202,reversed?"rig_a":"rig_b");
    for(size_t i=0;i<2;++i){const size_t p=first+i*clipSize;name(b,p+8,uint32_t(301+i),i?"walk":"idle");
        modanim::put32(b,p+4,i?0:uint32_t(clipSize));modanim::put32(b,p+0x28,reversed?1:0);modanim::put32(b,p+0x2c,0x10003);scalar(b,p+0x38,float(i+1));
        modanim::put32(b,p+0x40,0x60);modanim::put32(b,p+0x44,0x80);modanim::put32(b,p+0x48,0x90);modanim::put32(b,p+0x50,30*uint32_t(i+1));
        modanim::put32(b,p+0x60,3);modanim::put32(b,p+0x80,1);modanim::put32(b,p+0x84,8);modanim::put32(b,p+0x88,0x12345678+uint32_t(i));
        modanim::put32(b,p+0x90,1);modanim::put32(b,p+0x94,8);modanim::put32(b,p+0x98,0xabcdef01+uint32_t(i));
        for(size_t k=0xa0;k<clipSize;++k)b[p+k]=uint8_t(k+i);
    }return b;
}
void checkClipBytes(const modanim::Source& result,const modanim::Clip& actual,
                    const modanim::Source& source,const modanim::Clip& expected,
                    bool identityChanged){
    require(actual.end-actual.start==expected.end-expected.start,"clip byte length changed");
    for(size_t i=0;i<expected.end-expected.start;++i){
        if(i>=4&&i<8)continue; // The next link belongs to the assembled bank.
        if(identityChanged&&((i>=8&&i<40)||(i>=0x28&&i<0x2c)))continue;
        require(result.bytes[actual.start+i]==source.bytes[expected.start+i],"clip payload/events/version changed");
    }
}
void characterVersions(){
    auto v2bytes=fixture();modanim::put32(v2bytes,0x90+0x2c,0x10002);
    modanim::Source mixed,v3,parsed;std::string why;std::vector<uint8_t>out;
    require(mixed.parse(v2bytes,&why),"mixed character v2/v3 bank rejected");
    require(v3.parse(fixture(false,0xe0),&why),"v3 source fixture rejected");
    require(mixed.clips[0].version==0x10002&&mixed.clips[1].version==0x10003,"mixed versions not retained");
    require(mixed.clips[0].animComponentCount==1&&mixed.clips[0].poseComponentCount==1,"v2 component metadata wrong");
    require(modanim::compatibleBank(v3,mixed,&why)&&modanim::compatibleBank(mixed,v3,&why),"same-rig character versions incompatible");
    require(modanim::replaceClips(mixed,{{301,&v3,302}},out,&why)&&parsed.parse(out,&why),"v3 onto v2 replacement failed");
    require(parsed.clips[0].version==0x10003&&parsed.clips[0].hash==301&&parsed.clips[0].name=="idle","source v3 marker/target identity changed");
    checkClipBytes(parsed,parsed.clips[0],v3,v3.clips[1],true);
    checkClipBytes(parsed,parsed.clips[1],mixed,mixed.clips[1],false);
    require(modanim::replaceClips(v3,{{302,&mixed,301}},out,&why)&&parsed.parse(out,&why),"v2 onto v3 replacement failed");
    require(parsed.clips[1].version==0x10002&&parsed.clips[1].hash==302&&parsed.clips[1].name=="walk","source v2 marker/target identity changed");
    checkClipBytes(parsed,parsed.clips[1],mixed,mixed.clips[0],true);
    const auto unchanged=out;
    for(size_t countOffset:{size_t(0x80),size_t(0x90)}){
        auto mismatch=v2bytes;modanim::put32(mismatch,0x90+countOffset,0);modanim::Source wrong;
        require(wrong.parse(mismatch,&why),"bounded different-count fixture rejected");
        require(!modanim::compatibleBank(wrong,v3,&why),"cross-version different-count bank accepted");
        require(!modanim::replaceClips(v3,{{301,&wrong,301}},out,&why)&&out==unchanged,"cross-version different-count swap not atomic");
    }
    for(const auto& change:std::vector<std::pair<size_t,uint32_t>>{
            {0x2c,0x10001},{0x2c,0x10004},{0x40,0x10},{0x44,0xfffffff0},
            {0x80,4097},{0x84,4},{0x84,0x1000},{0x90,4097},{0x94,0x1000}}){
        auto bad=v2bytes;modanim::put32(bad,0x90+change.first,change.second);
        require(!parsed.parse(std::move(bad),&why),"unknown version or malformed v2 layout accepted");
    }
}
void selftest(){
    modanim::Source target,source;std::string why;auto original=fixture();require(target.parse(original,&why),"valid bank rejected");require(target.clips.size()==2&&target.clips[1].duration==2,"clip metadata wrong");require(target.findClip(301)&&!target.findClip(999),"clip lookup wrong");
    require(source.parse(fixture(true,0xe0),&why),"reordered source rejected");require(!modanim::compatibleBank(source,target,&why),"reordered bank accepted");
    std::vector<uint8_t>out{9,8,7};require(modanim::replaceClips(target,{{301,&source,302}},out,&why),"same-skeleton clip replacement rejected");modanim::Source result;require(result.parse(out,&why),"assembled bank invalid");require(modanim::compatibleBank(result,target,&why),"assembled bank loses contract");
    require(result.clips[0].hash==301&&result.clips[0].name=="idle"&&result.clips[0].skeletonIndex==0&&result.clips[0].duration==2,"replacement identity/index/duration wrong");
    require(result.clips[1].hash==302&&result.clips[1].start==0x170,"replacement chain alignment wrong");
    for(size_t i=0;i<0xe0;++i){if((i>=4&&i<40)||(i>=0x28&&i<0x2c))continue;require(out[result.clips[0].start+i]==source.bytes[source.clips[1].start+i],"replacement payload/events changed");}
    for(size_t i=0;i<0xb0;++i)require(out[result.clips[1].start+i]==target.bytes[target.clips[1].start+i],"untouched clip payload changed");
    require(std::equal(original.begin(),original.begin()+target.firstClipOffset,out.begin()),"original bank header changed");
    const auto unchanged=out;require(!modanim::replaceClips(target,{{301,&source,999}},out,&why)&&out==unchanged,"unknown source not atomic");
    require(!modanim::replaceClips(target,{{999,&source,302}},out,&why)&&out==unchanged,"unknown target not atomic");
    require(!modanim::replaceClips(target,{{301,&source,301},{301,&source,302}},out,&why)&&out==unchanged,"duplicate swap not atomic");
    auto wrong=source;wrong.clips[0].skeletonHash=202;require(!modanim::replaceClips(target,{{301,&wrong,301}},out,&why)&&out==unchanged,"cross-rig swap accepted");
    wrong=source;wrong.clips[0].version=0x500;require(!modanim::replaceClips(target,{{301,&wrong,301}},out,&why)&&out==unchanged,"cross-version swap accepted");
    require(modanim::replaceClips(target,{},out,&why)&&out==original,"empty swap changes bytes");
    const auto reject=[&](std::vector<uint8_t>b,const char*msg){modanim::Source parsed;require(!parsed.parse(std::move(b),&why),msg);};
    auto b=original;b.resize(0x60);reject(b,"truncation accepted");
    const auto corrupt=[&](size_t p,uint32_t v,const char*msg){auto x=original;modanim::put32(x,p,v);reject(std::move(x),msg);};
    corrupt(0,0,"bad outer version");corrupt(4,8,"rebased bank accepted");corrupt(12,0,"zero skeletons");corrupt(8,32,"wrong table bytes");corrupt(0x68,101,"duplicate skeleton");corrupt(0x34,0x80,"bad first clip");
    corrupt(0x90+4,0xfffffff0,"outside next link");corrupt(0x90+4,0x44,"unaligned next link");corrupt(0x90+4,0x10,"overlapping next clip");corrupt(0x90+0x28,2,"bad skeleton index");corrupt(0x140+8,301,"duplicate clip hash");
    corrupt(0x90+0x2c,0x999,"unknown clip version");corrupt(0x90+0x38,0x7fc00000,"NaN duration");corrupt(0x90+0x40,0x10,"flags inside name accepted");corrupt(0x90+0x44,0xfffffff0,"outside directory");corrupt(0x90+0x80,4097,"huge component count");corrupt(0x90+0x84,4,"component overlaps directory");corrupt(0x90+0x84,0x1000,"component leaves clip");
    b=original;std::fill(b.begin()+0x90+12,b.begin()+0x90+40,'a');reject(b,"unterminated name");
    b=original;modanim::put32(b,0x90+0x94,9);require(result.parse(b,&why),"byte-aligned entropy payload rejected");
    b=original;
    for(size_t p0:{size_t(0x90),size_t(0x140)}){modanim::put32(b,p0+0x2c,0x10200);modanim::put32(b,p0+0x44,1);modanim::put32(b,p0+0x48,1);modanim::put32(b,p0+0x50,4);modanim::put32(b,p0+0x54,4);modanim::put32(b,p0+0x58,4);modanim::put32(b,p0+0x64,1);modanim::put32(b,p0+0x70,4);modanim::put32(b,p0+0x74,4);}
    require(result.parse(b,&why),"valid generic layout rejected");require(modanim::replaceClips(result,{{301,&result,302}},out,&why),"generic clip replacement rejected");
    auto badGeneric=b;modanim::put32(badGeneric,0x90+0x58,3);reject(badGeneric,"non-power-of-two generic alignment accepted");badGeneric=b;modanim::put32(badGeneric,0x90+0x54,0x1000);reject(badGeneric,"outside generic static pose accepted");badGeneric=b;modanim::put32(badGeneric,0x90+0x64,0xffffffff);reject(badGeneric,"unbounded generic blocks accepted");
    b=original;for(size_t p0:{size_t(0x90),size_t(0x140)}){modanim::put32(b,p0+0x2c,0x500);modanim::put32(b,p0+0x60,30);modanim::put32(b,p0+0x64,0);modanim::put32(b,p0+0x6c,0);}
    require(result.parse(b,&why),"valid pedestrian layout rejected");require(modanim::replaceClips(result,{{301,&result,302}},out,&why),"pedestrian clip replacement rejected");modanim::put32(b,0x90+0x6c,0x1000);reject(b,"outside pedestrian stream accepted");
    require(target.parse(original,&why),"valid parser state lost");const auto valid=target.bytes;require(!target.parse({1,2,3},&why)&&target.bytes==valid,"parse failure not atomic");
    characterVersions();
}
void installedPair(const std::filesystem::path& installed,const std::filesystem::path& supplied){
    modanim::Source original,source;std::string why;
    require(original.parse(read(installed),&why),"installed mixed Spider-Man bank rejected");
    require(source.parse(read(supplied),&why),"supplied Spider-Man bank rejected");
    require(original.bytes.size()==403600&&source.bytes.size()==403200,"unexpected installed/supplied fixture size");
    require(original.clips.size()==195&&source.clips.size()==195,"Spider-Man fixture clip count changed");
    require(original.skeletonHashes==std::vector<uint32_t>{0x1189ab87}&&source.skeletonHashes==original.skeletonHashes,"Spider-Man skeleton identity changed");
    std::set<std::string>v2names;size_t v3count=0;
    for(const auto&c:original.clips){if(c.version==0x10002)v2names.insert(c.name);else if(c.version==0x10003)++v3count;}
    require(v2names==std::set<std::string>{"usmidl","usmkck_1","usmpch_1","usmpch_2","usmpch_3","usmpch_4"}&&v3count==189,"installed mixed-version identities changed");
    for(const auto&c:source.clips)require(c.version==0x10003,"supplied fixture is not the original v3 bank");
    require(modanim::compatibleBank(source,original,&why)&&modanim::compatibleBank(original,source,&why),"actual whole-bank character family rejected");
    std::vector<modanim::ClipSwap>swaps;
    for(const auto&c:original.clips)swaps.push_back({c.hash,&source,c.hash});
    std::vector<uint8_t>out;modanim::Source changed;
    require(modanim::replaceClips(original,swaps,out,&why)&&changed.parse(out,&why),"actual whole-bank assembly failed");
    require(out.size()==source.bytes.size()&&changed.clips.size()==195,"actual whole-bank length/order changed");
    require(std::equal(original.bytes.begin(),original.bytes.begin()+original.firstClipOffset,out.begin()),"actual original bank header changed");
    for(size_t i=0;i<original.clips.size();++i){
        const auto&c=changed.clips[i];const auto*s=source.findClip(original.clips[i].hash);
        require(s&&c.hash==original.clips[i].hash&&c.name==original.clips[i].name&&c.version==0x10003,"actual whole-bank identity/version changed");
        checkClipBytes(changed,c,source,*s,true);
    }
    constexpr uint32_t punch=0x94e4aae0;
    const auto*oldPunch=original.findClip(punch);const auto*newPunch=source.findClip(punch);
    require(oldPunch&&newPunch&&oldPunch->end-oldPunch->start==2992&&newPunch->end-newPunch->start==2896,"actual unequal-length punch fixture changed");
    require(modanim::replaceClips(original,{{punch,&source,punch}},out,&why)&&changed.parse(out,&why),"actual v3 punch onto v2 failed");
    require(out.size()==original.bytes.size()-96,"actual changed-length bank size wrong");
    for(size_t i=0;i<original.clips.size();++i){
        const auto&old=original.clips[i];const auto&now=changed.clips[i];
        require(now.hash==old.hash&&now.name==old.name&&now.skeletonIndex==old.skeletonIndex,"actual per-clip identity/order changed");
        checkClipBytes(changed,now,old.hash==punch?source:original,old.hash==punch?*newPunch:old,old.hash==punch);
    }
    require(changed.findClip(punch)->version==0x10003&&changed.findClip(0x194a24ce)->version==0x10002,"actual mixed markers not preserved");
    require(modanim::replaceClips(source,{{punch,&original,punch}},out,&why)&&changed.parse(out,&why),"actual v2 punch onto v3 failed");
    require(changed.findClip(punch)->version==0x10002,"actual v2 source marker rewritten");
    checkClipBytes(changed,*changed.findClip(punch),original,*oldPunch,true);
    require(modanim::replaceClips(original,{{punch,&original,punch}},out,&why)&&out==original.bytes,"actual mixed self replacement not byte-identical");
    std::cout<<"PASS installed Spider-Man mixed195 clips (6v2/189v3): whole bank, unequal-length v3-to-v2, reverse v2-to-v3, source markers and untouched clips preserved\n";
}
std::string quote(const std::string&s){std::string r="\"";for(unsigned char c:s){if(c=='"'||c=='\\')r+='\\';if(c>=32&&c<127)r+=char(c);else r+='?';}return r+'"';}
void inspect(const std::filesystem::path&p){modanim::Source s;std::string why;bool ok=s.parse(read(p),&why);std::cout<<"{\"path\":"<<quote(p.string())<<",\"valid\":"<<(ok?"true":"false")<<",\"reason\":"<<quote(why)<<",\"skeletons\":[";
    for(size_t i=0;i<s.skeletonHashes.size();++i){if(i)std::cout<<',';std::cout<<"{\"hash\":"<<s.skeletonHashes[i]<<",\"name\":"<<quote(s.skeletonNames[i])<<'}';}
    std::cout<<"],\"clips\":[";for(size_t i=0;i<s.clips.size();++i){const auto&c=s.clips[i];if(i)std::cout<<',';std::cout<<"{\"hash\":"<<c.hash<<",\"name\":"<<quote(c.name)<<",\"version\":"<<c.version<<",\"skeletonHash\":"<<c.skeletonHash<<",\"skeletonIndex\":"<<c.skeletonIndex<<",\"start\":"<<c.start<<",\"end\":"<<c.end<<",\"duration\":"<<c.duration<<'}';}std::cout<<"]}\n";
}
void actual(const std::filesystem::path&root,size_t expectedRejected){size_t passed=0,rejected=0,clips=0,alternate=0;std::set<uint32_t>families;for(const auto&e:std::filesystem::recursive_directory_iterator(root)){if(!e.is_regular_file())continue;auto ext=e.path().extension().string();std::transform(ext.begin(),ext.end(),ext.begin(),[](unsigned char c){return char(std::tolower(c));});if(ext!=".pcanim")continue;
    modanim::Source s;std::string why;if(!s.parse(read(e.path()),&why)){++rejected;std::cout<<"REJECT "<<e.path().string()<<": "<<why<<'\n';continue;}++passed;clips+=s.clips.size();require(modanim::compatibleBank(s,s,&why),"actual identity bank incompatible");std::vector<uint8_t>out;require(modanim::replaceClips(s,{{s.clips[0].hash,&s,s.clips[0].hash}},out,&why)&&out==s.bytes,"actual self replacement changed bytes");
    std::set<uint32_t>tested;
    for(const auto&dst:s.clips){if(tested.count(dst.version))continue;for(const auto&src:s.clips){if(src.hash==dst.hash||src.version!=dst.version||src.skeletonHash!=dst.skeletonHash)continue;
        require(modanim::replaceClips(s,{{dst.hash,&s,src.hash}},out,&why),"actual alternate clip failed");modanim::Source changed;require(changed.parse(out,&why),"actual alternate parse failed");const auto*c=changed.findClip(dst.hash);require(c&&c->duration==src.duration&&c->skeletonIndex==dst.skeletonIndex&&c->name==dst.name,"actual alternate identity/duration changed");
        for(size_t k=0;k<src.end-src.start;++k){if((k>=4&&k<40)||(k>=0x28&&k<0x2c))continue;require(out[c->start+k]==s.bytes[src.start+k],"actual alternate payload/events changed");}
        for(const auto&old:s.clips){if(old.hash==dst.hash)continue;const auto*n=changed.findClip(old.hash);require(n&&n->end-n->start==old.end-old.start,"actual untouched clip size changed");for(size_t k=0;k<old.end-old.start;++k){if(k>=4&&k<8)continue;require(out[n->start+k]==s.bytes[old.start+k],"actual untouched clip payload/events changed");}}
        ++alternate;families.insert(dst.version);tested.insert(dst.version);break;}}
    }
    require(passed>0,"no PCANIM fixtures found");require(rejected==expectedRejected,"unexpected native input rejection count");std::cout<<"PASS native directory accepted="<<passed<<" rejected="<<rejected<<" clips="<<clips<<" alternate-swaps="<<alternate<<" families="<<families.size()<<"; accepted images self-replace byte-identically, alternate payloads and untouched clips preserved\n";
}
}
int main(int argc,char**argv){try{if(argc==3&&std::string(argv[1])=="--inspect"){inspect(argv[2]);return 0;}selftest();if((argc==3||argc==4)&&std::string(argv[1])=="--directory")actual(argv[2],argc==4?std::stoul(argv[3]):0);else if(argc==4&&std::string(argv[1])=="--installed-pair")installedPair(argv[2],argv[3]);else if(argc!=1)throw std::runtime_error("usage: probe [--inspect file | --directory directory [expected-rejections] | --installed-pair original supplied]");std::cout<<"PASS PCANIM checks="<<checks<<'\n';return 0;}catch(const std::exception&e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
