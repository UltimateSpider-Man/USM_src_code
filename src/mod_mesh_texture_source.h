#pragma once

// Local image lookup for an individual FBX. No process-wide basename search:
// two models may legitimately ship different images with the same filename.
#include <algorithm>
#include <filesystem>
#include <string>
#include <vector>

namespace modmesh { namespace texturesource {
namespace detail {
inline std::string lower(std::string value) {
    for (char& c:value) if(c>='A'&&c<='Z')c=char(c-'A'+'a');
    return value;
}
inline std::string portable(std::string value) {
    std::replace(value.begin(),value.end(),'\\','/');return value;
}
inline bool fail(std::string* why,const char* message) {
    if(why)*why=message;
    return false;
}
inline const std::vector<std::string>& extensions() {
    static const std::vector<std::string> value={".dds",".png",".tga",".jpg",".jpeg",".bmp"};
    return value;
}
inline bool imageExtension(const std::string& value) {
    const auto ext=lower(std::filesystem::path(value).extension().string());
    return std::find(extensions().begin(),extensions().end(),ext)!=extensions().end();
}
inline bool contained(const std::filesystem::path& root,const std::filesystem::path& path) {
    auto ri=root.begin(),pi=path.begin();
    for(;ri!=root.end();++ri,++pi)
        if(pi==path.end()||*ri!=*pi)return false;
    return true;
}
// Return 0 for absent, 1 for unique, -1 for ambiguous or inaccessible.
inline int child(const std::filesystem::path& directory,const std::string& name,
                 bool wantDirectory,std::filesystem::path& result) {
    std::error_code ec;std::filesystem::directory_iterator it(directory,ec),end;
    if(ec)return 0;
    const auto key=lower(name);std::vector<std::filesystem::path> matches;
    for(;it!=end;it.increment(ec)) {
        if(ec)return -1;
        if(lower(it->path().filename().string())!=key)continue;
        const bool kind=wantDirectory?it->is_directory(ec):it->is_regular_file(ec);
        if(ec)return -1;
        if(kind)matches.push_back(it->path());
    }
    if(ec)return -1;
    if(matches.empty())return 0;
    if(matches.size()!=1)return -1;
    result=matches.front();return 1;
}
inline int directory(const std::filesystem::path& root,const std::filesystem::path& relative,
                     std::filesystem::path& result) {
    result=root;
    for(const auto& part:relative) {
        const auto name=part.string();if(name.empty()||name==".")continue;
        if(name==".."||name.find(':')!=std::string::npos)return -1;
        std::filesystem::path next;const int found=child(result,name,true,next);
        if(found!=1)return found;
        result=std::move(next);
    }
    std::error_code ec;result=std::filesystem::canonical(result,ec);
    return !ec&&contained(root,result)?1:-1;
}
}

inline bool resolveLocalImage(const std::filesystem::path& fbxPath,
                              const std::string& referencePath,const std::string& stem,
                              std::filesystem::path& out,std::string* why=nullptr) {
    namespace fs=std::filesystem;
    using namespace detail;
    if(referencePath.size()>16384||stem.size()>4096||referencePath.find('\0')!=std::string::npos||
       stem.find('\0')!=std::string::npos)return fail(why,"invalid image reference");
    std::error_code ec;fs::path source=fs::absolute(fbxPath,ec);
    if(ec)return fail(why,"invalid FBX source path");
    fs::path root=fs::canonical(source.parent_path(),ec);
    if(ec)return fail(why,"FBX source folder does not exist");
    std::string ref=portable(referencePath);
    const bool foreignAbsolute=(!ref.empty()&&ref[0]=='/')||
        (ref.size()>2&&((ref[0]>='A'&&ref[0]<='Z')||(ref[0]>='a'&&ref[0]<='z'))&&ref[1]==':'&&ref[2]=='/');
    if(foreignAbsolute) {
        const auto rootText=portable(root.generic_string())+"/";
        if(lower(ref).compare(0,rootText.size(),lower(rootText))==0)ref.erase(0,rootText.size());
        else {
            // Exporter absolute paths are portable only within the source's
            // image namespace. Preserve a referenced .fbm subfolder suffix.
            size_t start=std::string::npos,pos=0;
            while(pos<ref.size()) {
                const size_t end=ref.find('/',pos);const auto part=ref.substr(pos,end-pos);
                if(lower(fs::path(part).extension().string())==".fbm")start=pos;
                if(end==std::string::npos)break;
                pos=end+1;
            }
            ref=start==std::string::npos?ref.substr(ref.find_last_of('/')+1):ref.substr(start);
        }
    }
    const fs::path relative(ref);
    for(const auto& part:relative)
        if(part==".."||part.string().find(':')!=std::string::npos)
            return fail(why,"image reference leaves FBX source folder");
    if(relative.is_absolute()||relative.filename()==".")return fail(why,"invalid local image reference");
    std::string requested=relative.filename().string();
    if(!requested.empty()&&!imageExtension(requested))return fail(why,"unsupported referenced image extension");
    if(stem.find_first_of("/\\:")!=std::string::npos||stem=="."||stem=="..")
        return fail(why,"invalid local image stem");
    std::vector<std::string> stems;
    if(!requested.empty())stems.push_back(fs::path(requested).stem().string());
    if(!stem.empty()) {
        const auto candidate=imageExtension(stem)?fs::path(stem).stem().string():stem;
        if(std::none_of(stems.begin(),stems.end(),[&](const auto& s){return lower(s)==lower(candidate);}))stems.push_back(candidate);
    }
    if(stems.empty())return fail(why,"missing local image identity");
    std::vector<fs::path> folders;
    const auto addFolder=[&](const fs::path& rel)->bool {
        fs::path resolved;const int status=directory(root,rel,resolved);
        if(status<0)return false;
        if(status==1&&std::find(folders.begin(),folders.end(),resolved)==folders.end())folders.push_back(std::move(resolved));
        return true;
    };
    if(!addFolder(relative.parent_path())||!addFolder({})||
       !addFolder(source.stem().string()+".fbm"))return fail(why,"ambiguous or external image folder");
    const auto lookup=[&](const fs::path& folder,const std::string& name)->int {
        fs::path found;const int status=child(folder,name,false,found);
        if(status!=1)return status;
        found=fs::canonical(found,ec);
        if(ec||!contained(root,found))return -1;
        out=std::move(found);if(why)why->clear();return 1;
    };
    for(const auto& folder:folders) {
        if(!requested.empty()) {
            const int status=lookup(folder,requested);
            if(status==1)return true;
            if(status<0)return fail(why,"ambiguous or external referenced image");
        }
        for(const auto& identity:stems)for(const auto& ext:extensions()) {
            const int status=lookup(folder,identity+ext);
            if(status==1)return true;
            if(status<0)return fail(why,"ambiguous or external local image");
        }
    }
    return fail(why,"image not found in FBX source folders");
}

// An unlinked material can still identify an exported OBJ-style albedo map.
// The object prefix is deliberately not guessed from the FBX mesh name:
// renamed/imported models often retain maps named by their earlier exporter.
inline bool resolveMaterialImage(const std::filesystem::path& fbxPath,
                                 const std::string& materialName,
                                 std::filesystem::path& out,std::string* why=nullptr) {
    namespace fs=std::filesystem;
    using namespace detail;
    if(materialName.empty()||materialName.size()>4096||materialName.find('\0')!=std::string::npos||
       materialName.find_first_of("/\\:")!=std::string::npos||materialName=="."||materialName=="..")
        return fail(why,"invalid material image identity");
    std::error_code ec;const fs::path source=fs::absolute(fbxPath,ec);
    if(ec)return fail(why,"invalid FBX source path");
    const fs::path root=fs::canonical(source.parent_path(),ec);
    if(ec)return fail(why,"FBX source folder does not exist");
    std::vector<fs::path> folders{root};fs::path embeddedFolder;
    const int folderStatus=directory(root,source.stem().string()+".fbm",embeddedFolder);
    if(folderStatus<0)return fail(why,"ambiguous or external material image folder");
    if(folderStatus==1&&embeddedFolder!=root)folders.push_back(embeddedFolder);
    const auto exact=lower(materialName)+"_map_kd",suffix="_"+exact;
    fs::path match;
    for(const auto& folder:folders) {
        fs::directory_iterator it(folder,ec),end;
        if(ec)return fail(why,"cannot inspect material image folder");
        for(;it!=end;it.increment(ec)) {
            if(ec)return fail(why,"cannot inspect material image folder");
            const auto filename=it->path().filename().string();
            if(!imageExtension(filename))continue;
            const auto name=lower(it->path().stem().string());
            if(name!=exact&&(name.size()<=suffix.size()||name.compare(name.size()-suffix.size(),suffix.size(),suffix)!=0))continue;
            if(!it->is_regular_file(ec)) {
                if(ec)return fail(why,"cannot inspect material image");
                continue;
            }
            const auto canonical=fs::canonical(it->path(),ec);
            if(ec||!contained(root,canonical))return fail(why,"material image leaves FBX source folder");
            if(!match.empty()&&match!=canonical)return fail(why,"ambiguous material albedo images");
            match=canonical;
        }
        if(ec)return fail(why,"cannot inspect material image folder");
    }
    if(match.empty())return fail(why,"material albedo image not found in FBX source folders");
    out=std::move(match);if(why)why->clear();return true;
}
}} // namespace modmesh::texturesource
