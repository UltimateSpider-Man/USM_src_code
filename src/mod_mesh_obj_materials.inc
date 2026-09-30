// Included inside modmesh after the shared glTF local-file safety helpers.
// Optional OBJ MTL diffuse metadata. Geometry remains usable when a library
// is missing or has an unsupported map option; the reason is logged.
namespace objtxt {
inline std::string unquotedPath(std::string s)
{
    s = toolkit::trim(std::move(s));
    if (s.size() >= 2 && s.front() == '"' && s.back() == '"') s = s.substr(1, s.size()-2);
    std::replace(s.begin(), s.end(), '\\', '/');
    return s;
}
inline void loadMaterials(const std::string &objPath, Scene &scene)
{
    for (const auto &library : scene.objMaterialLibs) {
        try {
            const auto mtlPath = gltf::localPath(objPath, unquotedPath(library));
            std::ifstream input(mtlPath, std::ios::binary | std::ios::ate);
            gltf::check(bool(input), "cannot open OBJ material library");
            const auto size = input.tellg();
            gltf::check(size >= 0 && uint64_t(size) <= 4u*1024u*1024u, "OBJ material library exceeds 4 MiB");
            input.seekg(0); std::string line; int64_t material = -1;
            while (std::getline(input, line)) {
                // Strip only unquoted comments, so a quoted filename can contain #.
                bool quoted = false;
                for (size_t i=0; i<line.size(); ++i) {
                    if (line[i]=='"') quoted=!quoted;
                    else if (line[i]=='#' && !quoted) { line.resize(i); break; }
                }
                std::istringstream words(line); std::string key, value;
                if (!(words >> key)) continue;
                std::getline(words, value); value=toolkit::trim(std::move(value));
                if (key == "newmtl") {
                    material = -1;
                    for (const auto &entry : scene.materials) if (entry.second == value) { material=entry.first; break; }
                    if (material >= 0 && !scene.materialDiffuse.count(material))
                        scene.materialDiffuse[material] = {{1,1,1,1}};
                    continue;
                }
                if (material < 0) continue;
                auto &color=scene.materialDiffuse[material];
                if (key=="Kd") {
                    std::istringstream values(value); double r=1,g=1,b=1;
                    gltf::check(bool(values>>r>>g>>b) && std::isfinite(r) && std::isfinite(g) && std::isfinite(b), "invalid MTL Kd color");
                    color[0]=float(std::clamp(r,0.,1.));color[1]=float(std::clamp(g,0.,1.));color[2]=float(std::clamp(b,0.,1.));
                } else if (key=="d" || key=="Tr") {
                    double alpha=1;gltf::check(toolkit::scalar(value,alpha),"invalid MTL opacity");
                    color[3]=float(std::clamp(key=="Tr"?1.-alpha:alpha,0.,1.));
                } else if (key=="map_Kd") {
                    if (value.empty() || value.front()=='-') {
                        logf("[modmesh] %s: map_Kd options require baking in the exporter; diffuse path skipped",mtlPath.string().c_str());
                        continue;
                    }
                    const auto imagePath=gltf::localPath(mtlPath.string(),unquotedPath(value));
                    std::error_code ec;
                    gltf::check(std::filesystem::is_regular_file(imagePath,ec),"MTL diffuse image is missing");
                    const auto stem="OBJ_DIFFUSE_"+std::to_string(material);
                    scene.matTexStem[material]=stem;
                    scene.texRelOfStem[stem]=imagePath.string();
                }
            }
        } catch (const std::exception &error) {
            logf("[modmesh] %s: optional MTL '%s': %s",objPath.c_str(),library.c_str(),error.what());
        }
    }
}
} // namespace objtxt
