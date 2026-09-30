// Included inside namespace modmesh, after Scene and FBX/OBJ helpers.
// Native glTF 2.0/GLB reader. Reference: Khronos glTF 2.0.1 specification.
// The shared section builder owns skin transfer, palettes, winding and buffers.
namespace gltf {
using Json = rapidjson::Value;
constexpr size_t maxFileBytes = 256u * 1024u * 1024u;
constexpr size_t maxDecodedBytes = 192u * 1024u * 1024u;
constexpr size_t maxElements = 2000000;

inline void check(bool condition, const char *message)
{ if (!condition) throw std::runtime_error(message); }
inline const Json *member(const Json &v, const char *name)
{
    check(v.IsObject(), "expected JSON object");
    auto it = v.FindMember(name);
    return it == v.MemberEnd() ? nullptr : &it->value;
}
inline const Json &required(const Json &v, const char *name)
{
    auto p = member(v, name); check(p != nullptr, "missing required glTF property"); return *p;
}
inline size_t integer(const Json &v)
{
    check(v.IsUint64() && v.GetUint64() <= maxFileBytes, "invalid/oversized unsigned integer");
    return size_t(v.GetUint64());
}
inline size_t number(const Json &v, const char *key, size_t fallback = 0)
{ const auto p = member(v, key); return p ? integer(*p) : fallback; }
inline std::string text(const Json &v)
{ check(v.IsString(), "expected JSON string"); return {v.GetString(), v.GetStringLength()}; }
inline std::string string(const Json &v, const char *key, const std::string &fallback = {})
{ const auto p = member(v, key); return p ? text(*p) : fallback; }
inline const Json &array(const Json &v)
{ check(v.IsArray(), "expected JSON array"); return v; }
inline const Json &at(const Json &a, size_t i)
{ check(a.IsArray() && i < a.Size(), "glTF array index out of range"); return a[rapidjson::SizeType(i)]; }
inline double real(const Json &v)
{
    check(v.IsNumber() && std::isfinite(v.GetDouble()), "nonfinite/non-numeric JSON value");
    return v.GetDouble();
}
inline uint32_t u32(const uint8_t *p)
{ return uint32_t(p[0]) | uint32_t(p[1]) << 8 | uint32_t(p[2]) << 16 | uint32_t(p[3]) << 24; }
inline bool range(size_t size, size_t offset, size_t count)
{ return offset <= size && count <= size - offset; }
inline std::vector<uint8_t> base64(const std::string &uri)
{
    auto comma = uri.find(',');
    check(comma != std::string::npos && comma >= 7 && uri.substr(comma-7,7) == ";base64",
          "only base64 data URIs are supported");
    const size_t n = uri.size()-comma-1;
    check(n % 4 == 0 && n/4 <= maxFileBytes/3, "invalid/oversized base64 data");
    std::vector<uint8_t> out; out.reserve(n/4*3);
    auto digit = [](char c) -> int {
        if (c >= 'A' && c <= 'Z') return c-'A';
        if (c >= 'a' && c <= 'z') return c-'a'+26;
        if (c >= '0' && c <= '9') return c-'0'+52;
        if (c == '+') return 62; if (c == '/') return 63;
        return -1;
    };
    for (size_t i=comma+1; i<uri.size(); i+=4) {
        const int a=digit(uri[i]), b=digit(uri[i+1]);
        const int c=uri[i+2]=='=' ? 0 : digit(uri[i+2]);
        const int d=uri[i+3]=='=' ? 0 : digit(uri[i+3]);
        const bool pc=uri[i+2]=='=', pd=uri[i+3]=='=';
        check(a>=0 && b>=0 && c>=0 && d>=0 && (!pc || pd)
              && (!(pc||pd) || i+4==uri.size()), "invalid base64 alphabet/padding");
        check((!pc || (b&15)==0) && (!pd || pc || (c&3)==0), "invalid base64 padding bits");
        out.push_back(uint8_t((a<<2)|(b>>4)));
        if (!pc) out.push_back(uint8_t((b<<4)|(c>>2)));
        if (!pd) out.push_back(uint8_t((c<<6)|d));
    }
    return out;
}
inline std::filesystem::path localPath(const std::string &model, const std::string &uri)
{
    std::string decoded;
    auto hex=[](char c)->int { if(c>='0'&&c<='9')return c-'0'; if(c>='A'&&c<='F')return c-'A'+10; if(c>='a'&&c<='f')return c-'a'+10;return -1; };
    for(size_t i=0;i<uri.size();++i) {
        unsigned char c=uint8_t(uri[i]);
        if(c=='%') { check(i+2<uri.size(), "truncated URI escape"); int a=hex(uri[i+1]),b=hex(uri[i+2]);check(a>=0&&b>=0,"invalid URI escape");c=uint8_t(a*16+b);i+=2; }
        check(c>=32 && c!=127 && c!=':' && c!='\\' && c!='?' && c!='#', "unsafe/external glTF URI");
        decoded.push_back(char(c));
    }
    std::filesystem::path rel=std::filesystem::u8path(decoded);
    check(!rel.empty() && !rel.is_absolute() && !rel.has_root_name(), "absolute/empty glTF URI rejected");
    for (const auto &part : rel) check(part != "..", "glTF URI escapes model directory");
    std::error_code ec;
    auto parent=std::filesystem::path(model).parent_path(); if(parent.empty())parent=".";
    const auto root=std::filesystem::weakly_canonical(parent,ec); check(!ec,"cannot canonicalize model directory");
    const auto path=std::filesystem::weakly_canonical(root/rel,ec);check(!ec,"cannot canonicalize glTF URI");
    auto a=root.begin(),b=path.begin();
    for(;a!=root.end();++a,++b)check(b!=path.end()&&*a==*b,"glTF URI symlink escapes model directory");
    return path;
}
inline std::vector<uint8_t> readUri(const std::string &model, const std::string &uri)
{
    if(uri.rfind("data:",0)==0)return base64(uri);
    const auto path=localPath(model,uri);
    std::ifstream in(path,std::ios::binary|std::ios::ate);check(bool(in),"cannot open glTF dependency");
    auto n=in.tellg();check(n>=0&&uint64_t(n)<=maxFileBytes,"oversized glTF dependency");
    std::vector<uint8_t> bytes(static_cast<size_t>(n));in.seekg(0);
    check(bytes.empty()||bool(in.read(reinterpret_cast<char*>(bytes.data()),std::streamsize(bytes.size()))),"cannot read glTF dependency");
    return bytes;
}
inline M4 inverse(const M4 &m)
{
    const double det=m.det3();check(std::isfinite(det)&&std::abs(det)>1e-15,"singular glTF transform/bind matrix");
    const M4 n=m.normalMatrix();M4 out;
    for(int r=0;r<3;++r)for(int c=0;c<3;++c)out.m[r*4+c]=n.m[c*4+r];
    const V3 t=out.vec({m.m[3],m.m[7],m.m[11]});out.m[3]=-t.x;out.m[7]=-t.y;out.m[11]=-t.z;
    return out;
}
inline M4 matrix(const Json &v)
{
    check(v.IsArray()&&v.Size()==16,"invalid glTF matrix");M4 m;
    for(int r=0;r<4;++r)for(int c=0;c<4;++c)m.m[r*4+c]=real(v[c*4+r]);
    check(std::abs(m.m[12])+std::abs(m.m[13])+std::abs(m.m[14])+std::abs(m.m[15]-1)<1e-8,"non-affine glTF matrix");
    return m;
}
inline M4 nodeMatrix(const Json &node)
{
    if(const auto m=member(node,"matrix")) {
        check(!member(node,"translation")&&!member(node,"rotation")&&!member(node,"scale"),"node matrix and TRS are mutually exclusive");return matrix(*m);
    }
    V3 t{},s{1,1,1};M4 r;
    auto vec=[](const Json &v)->V3 {check(v.IsArray()&&v.Size()==3,"invalid TRS vector");return {real(v[0]),real(v[1]),real(v[2])};};
    if(const auto p=member(node,"translation"))t=vec(*p);
    if(const auto p=member(node,"scale"))s=vec(*p);
    if(const auto p=member(node,"rotation")) {
        check(p->IsArray()&&p->Size()==4,"invalid node quaternion");
        double x=real((*p)[0]),y=real((*p)[1]),z=real((*p)[2]),w=real((*p)[3]);
        double length=std::sqrt(x*x+y*y+z*z+w*w);check(std::isfinite(length)&&length>1e-12,"zero/invalid quaternion");
        x/=length;y/=length;z/=length;w/=length;
        r.m[0]=1-2*(y*y+z*z);r.m[1]=2*(x*y-z*w);r.m[2]=2*(x*z+y*w);
        r.m[4]=2*(x*y+z*w);r.m[5]=1-2*(x*x+z*z);r.m[6]=2*(y*z-x*w);
        r.m[8]=2*(x*z-y*w);r.m[9]=2*(y*z+x*w);r.m[10]=1-2*(x*x+y*y);
    }
    return M4::translate(t)*r*M4::scale(s);
}
inline std::array<double,16> rowMatrix(const M4 &m)
{std::array<double,16>a{};for(int r=0;r<4;++r)for(int c=0;c<4;++c)a[r*4+c]=m.m[c*4+r];return a;}

struct Accessor { size_t count=0, components=0, type=0; bool normalized=false;std::vector<double> values; };
struct Reader {
    const Json &root; std::string path; Scene &scene;
    std::vector<std::vector<uint8_t>> buffers;
    std::map<size_t,Accessor> decoded;
    size_t budget=0, vertices=0, triangles=0, primitiveCount=0;
    std::vector<M4> worlds;
    std::map<size_t,std::string> images;
    Reader(const Json &r,const std::string &p,Scene &s):root(r),path(p),scene(s){}
    void charge(size_t count,size_t width) {
        check(count<=maxDecodedBytes/width && budget<=maxDecodedBytes-count*width,"glTF decoded-memory budget exceeded");budget+=count*width;
    }
    const Json &list(const char *key)const {return array(required(root,key));}
    struct View {const uint8_t *p;size_t size,stride;};
    View view(size_t index) {
        const auto &v=at(list("bufferViews"),index);
        if(const auto ex=member(v,"extensions"))check(!member(*ex,"EXT_meshopt_compression"),"meshopt-compressed bufferViews are not supported");
        size_t b=integer(required(v,"buffer")),off=number(v,"byteOffset"),n=integer(required(v,"byteLength"));
        check(b<buffers.size()&&range(buffers[b].size(),off,n),"bufferView is outside its buffer");
        size_t stride=number(v,"byteStride");check(!stride||(stride>=4&&stride<=252&&stride%4==0),"invalid bufferView stride");
        return {buffers[b].data()+off,n,stride};
    }
    static size_t width(size_t type) {
        switch(type){case 5120:case 5121:return 1;case 5122:case 5123:return 2;case 5125:case 5126:return 4;default:throw std::runtime_error("unsupported accessor componentType");}
    }
    static double value(const uint8_t*p,size_t t,bool normalized) {
        double v=0;
        switch(t) {
            case 5120:v=p[0]<128?p[0]:int(p[0])-256;return normalized?std::max(-1.,v/127.):v;
            case 5121:v=p[0];return normalized?v/255.:v;
            case 5122:{uint16_t u=uint16_t(p[0])|uint16_t(p[1])<<8;v=u<32768?int(u):int(u)-65536;return normalized?std::max(-1.,v/32767.):v;}
            case 5123:v=uint16_t(p[0])|uint16_t(p[1])<<8;return normalized?v/65535.:v;
            case 5125:return u32(p);
            case 5126:{uint32_t u=u32(p);float f;std::memcpy(&f,&u,4);check(std::isfinite(f),"nonfinite accessor float");return f;}
        }throw std::runtime_error("unsupported accessor type");
    }
    const Accessor &accessor(size_t index) {
        if(auto it=decoded.find(index);it!=decoded.end())return it->second;
        const auto &a=at(list("accessors"),index);Accessor out;
        out.count=integer(required(a,"count"));check(out.count>0&&out.count<=maxElements,"accessor count exceeds limit");
        const auto shape=string(a,"type");
        if(shape=="SCALAR")out.components=1;else if(shape=="VEC2")out.components=2;else if(shape=="VEC3")out.components=3;else if(shape=="VEC4")out.components=4;else if(shape=="MAT4")out.components=16;
        else throw std::runtime_error("unsupported accessor shape (expected SCALAR/VEC2/3/4/MAT4)");
        out.type=integer(required(a,"componentType"));size_t w=width(out.type),element=out.components*w;
        if(auto n=member(a,"normalized")){check(n->IsBool(),"invalid normalized flag");out.normalized=n->GetBool();}
        check(!out.normalized||(out.type>=5120&&out.type<=5123),"invalid normalized accessor type");
        check(out.components!=16||out.type==5126,"matrix accessor must use floats");
        charge(out.count*out.components,sizeof(double));out.values.assign(out.count*out.components,0.);
        if(auto bv=member(a,"bufferView")) {
            const auto v=view(integer(*bv));const size_t off=number(a,"byteOffset"),stride=v.stride?v.stride:element;
            check(stride>=element&&stride%w==0&&off%w==0,"misaligned accessor/stride");
            check(off<=v.size&&element<=v.size-off&&out.count-1<=(v.size-off-element)/stride,"accessor exceeds bufferView");
            for(size_t i=0;i<out.count;++i)for(size_t c=0;c<out.components;++c)out.values[i*out.components+c]=value(v.p+off+i*stride+c*w,out.type,out.normalized);
        } else check(number(a,"byteOffset")==0,"accessor without bufferView has byteOffset");
        if(auto sparse=member(a,"sparse")) {
            const size_t count=integer(required(*sparse,"count"));check(count>0&&count<=out.count,"invalid sparse count");
            const auto &indices=required(*sparse,"indices"),&values=required(*sparse,"values");
            const size_t it=integer(required(indices,"componentType"));check(it==5121||it==5123||it==5125,"invalid sparse index type");
            const auto iv=view(integer(required(indices,"bufferView"))),vv=view(integer(required(values,"bufferView")));
            const size_t io=number(indices,"byteOffset"),vo=number(values,"byteOffset"),iw=width(it);
            check(!iv.stride&&!vv.stride&&io%iw==0&&vo%w==0&&range(iv.size,io,count*iw)&&range(vv.size,vo,count*element),"sparse data out of range/misaligned");
            size_t prev=0;
            for(size_t i=0;i<count;++i){size_t dst=size_t(value(iv.p+io+i*iw,it,false));check(dst<out.count&&(!i||dst>prev),"sparse indices not strictly increasing/in range");prev=dst;for(size_t c=0;c<out.components;++c)out.values[dst*out.components+c]=value(vv.p+vo+i*element+c*w,out.type,out.normalized);}
        }
        return decoded.emplace(index,std::move(out)).first->second;
    }
    std::string image(size_t index) {
        if(auto it=images.find(index);it!=images.end())return it->second;
        const auto &im=at(list("images"),index);std::vector<uint8_t> bytes;
        if(auto uri=member(im,"uri"))bytes=readUri(path,text(*uri));
        else {const auto v=view(integer(required(im,"bufferView")));check(!v.stride,"image bufferView has stride");bytes.assign(v.p,v.p+v.size);}
        check(bytes.size()<=64u*1024u*1024u,"image exceeds 64 MiB limit");
        const bool png=bytes.size()>=8&&!std::memcmp(bytes.data(),"\x89PNG\r\n\x1a\n",8);
        const bool jpg=bytes.size()>=3&&bytes[0]==255&&bytes[1]==216&&bytes[2]==255;
        check(png||jpg,"glTF diffuse image is not PNG or JPEG (KTX2/WebP not supported)");
        charge(bytes.size(),1);const auto stem="GLTF_IMAGE_"+std::to_string(index);
        scene.embeddedTex[stem]=std::make_shared<std::vector<uint8_t>>(std::move(bytes));images[index]=stem;return stem;
    }
    struct Material {int64_t id=0;size_t uvSet=0;double ox=0,oy=0,sx=1,sy=1,rotation=0;};
    Material material(const Json &primitive) {
        Material out;const auto mi=member(primitive,"material");
        if(!mi){out.id=1;scene.materials[1]="GLTF_DEFAULT";scene.materialDiffuse[1]={{1,1,1,1}};return out;}
        const size_t index=integer(*mi);const auto &mat=at(list("materials"),index);out.id=int64_t(index)+2;
        scene.materials[out.id]=string(mat,"name","GLTF_MATERIAL_"+std::to_string(index));
        std::array<float,4> diffuse{{1,1,1,1}};
        if(const auto pbr=member(mat,"pbrMetallicRoughness")) {
            if(const auto color=member(*pbr,"baseColorFactor")) {
                check(color->IsArray()&&color->Size()==4,"invalid baseColorFactor");
                for(int i=0;i<4;++i){double c=real((*color)[i]);check(c>=0&&c<=1,"baseColorFactor outside [0,1]");diffuse[i]=float(c);}
            }
            if(const auto tex=member(*pbr,"baseColorTexture")) {
                out.uvSet=number(*tex,"texCoord");
                const auto &texture=at(list("textures"),integer(required(*tex,"index")));
                scene.matTexStem[out.id]=image(integer(required(texture,"source")));
                if(const auto ex=member(*tex,"extensions"))if(const auto xf=member(*ex,"KHR_texture_transform")) {
                    out.uvSet=number(*xf,"texCoord",out.uvSet);
                    auto v2=[](const Json &a,double &x,double &y){check(a.IsArray()&&a.Size()==2,"invalid texture transform vector");x=real(a[0]);y=real(a[1]);};
                    if(const auto p=member(*xf,"offset"))v2(*p,out.ox,out.oy);
                    if(const auto p=member(*xf,"scale"))v2(*p,out.sx,out.sy);
                    if(const auto p=member(*xf,"rotation"))out.rotation=real(*p);
                }
            }
        }
        scene.materialDiffuse[out.id]=diffuse;return out;
    }
    void run(const uint8_t *bin,size_t binSize) {
        check(string(required(root,"asset"),"version")=="2.0","only glTF 2.0 is supported");
        if(auto min=member(required(root,"asset"),"minVersion"))check(text(*min)=="2.0","unsupported minimum glTF version");
        if(auto ext=member(root,"extensionsRequired"))for(const auto &v:array(*ext).GetArray()) {
            const auto name=text(v);check(name=="KHR_texture_transform"||name=="KHR_materials_unlit", "required glTF extension is unsupported (export without Draco/meshopt/KTX2)");
        }
        if(auto bufs=member(root,"buffers"))for(size_t i=0;i<array(*bufs).Size();++i) {
            const auto &b=at(*bufs,i);const size_t length=integer(required(b,"byteLength"));check(length>0,"empty glTF buffer");
            std::vector<uint8_t> bytes;
            if(auto uri=member(b,"uri"))bytes=readUri(path,text(*uri));
            else {check(i==0&&bin&&length<=binSize&&binSize-length<=3,"missing/invalid GLB binary buffer");bytes.assign(bin,bin+length);}
            check(bytes.size()>=length,"glTF buffer shorter than byteLength");bytes.resize(length);charge(length,1);buffers.push_back(std::move(bytes));
        }
        const auto &nodes=list("nodes");check(nodes.Size()>0&&nodes.Size()<=32768,"invalid/excessive glTF node count");
        std::vector<int> parents(nodes.Size(),-1);std::vector<M4> local;local.reserve(nodes.Size());
        for(size_t i=0;i<nodes.Size();++i) {
            const auto &n=at(nodes,i);local.push_back(nodeMatrix(n));
            if(auto children=member(n,"children"))for(const auto &child:array(*children).GetArray()) {
                const size_t c=integer(child);check(c<nodes.Size()&&c!=i&&parents[c]==-1,"node has duplicate parents/self link");parents[c]=int(i);
            }
        }
        worlds.resize(nodes.Size());std::vector<uint8_t> visited(nodes.Size(),0);
        std::vector<size_t> levels(nodes.Size(),0);
        std::function<void(size_t,size_t)> world=[&](size_t i,size_t depth) {
            check(depth<=64,"glTF hierarchy exceeds 64 levels");if(visited[i]==2)return;check(visited[i]!=1,"glTF node cycle");visited[i]=1;
            if(parents[i]>=0){world(size_t(parents[i]),depth+1);levels[i]=levels[size_t(parents[i])]+1;check(levels[i]<=64,"glTF hierarchy exceeds 64 levels");worlds[i]=worlds[size_t(parents[i])]*local[i];}else worlds[i]=local[i];
            for(double v:worlds[i].m)check(std::isfinite(v)&&std::abs(v)<1e15,"nonfinite/excessive node transform");visited[i]=2;
        };
        for(size_t i=0;i<nodes.Size();++i)world(i,0);
        std::vector<bool> active(nodes.Size(),false);
        std::function<void(size_t)> activate=[&](size_t i){check(i<nodes.Size(),"scene root outside nodes");if(active[i])return;active[i]=true;if(auto c=member(at(nodes,i),"children"))for(const auto&child:c->GetArray())activate(integer(child));};
        if(auto scenes=member(root,"scenes")) {
            const auto &selected=at(array(*scenes),number(root,"scene"));
            if(auto roots=member(selected,"nodes"))for(const auto &n:array(*roots).GetArray()){size_t i=integer(n);check(i<nodes.Size()&&parents[i]<0,"scene root has a parent");activate(i);}
        }else for(size_t i=0;i<nodes.Size();++i)if(parents[i]<0)activate(i);
        for(size_t i=0;i<nodes.Size();++i) {
            const auto &n=at(nodes,i);Model m;m.id=int64_t(i)+1;m.parent=parents[i]<0?0:int64_t(parents[i])+1;
            m.name=string(n,"name","gltf_node_"+std::to_string(i));m.type="Null";m.matrixOverride=true;m.localMatrix=local[i];scene.models.emplace(m.id,std::move(m));
        }
        int64_t id=int64_t(nodes.Size())+1;
        for(size_t ni=0;ni<nodes.Size();++ni) {
            const auto &node=at(nodes,ni);const auto meshId=member(node,"mesh");if(!active[ni]||!meshId)continue;
            const auto &mesh=at(list("meshes"),integer(*meshId));const auto &primitives=array(required(mesh,"primitives"));
            std::vector<size_t> joints;std::vector<M4> skinMatrices;
            const auto skinId=member(node,"skin");
            if(skinId) {
                const auto &skin=at(list("skins"),integer(*skinId));const auto &js=array(required(skin,"joints"));check(js.Size()>0&&js.Size()<=1024,"invalid/excessive glTF skin joints");
                const Accessor *ib=nullptr;if(auto p=member(skin,"inverseBindMatrices")){ib=&accessor(integer(*p));check(ib->components==16&&ib->type==5126&&ib->count>=js.Size(),"invalid inverse bind accessor");}
                std::set<size_t> unique;
                for(size_t j=0;j<js.Size();++j) {
                    const size_t joint=integer(at(js,j));check(joint<nodes.Size()&&unique.insert(joint).second,"invalid/duplicate skin joint");joints.push_back(joint);scene.models.at(int64_t(joint)+1).type="LimbNode";
                    M4 inverseBind;
                    if(ib)for(int r=0;r<4;++r)for(int c=0;c<4;++c)inverseBind.m[r*4+c]=ib->values[j*16+c*4+r];
                    check(std::abs(inverseBind.m[12])+std::abs(inverseBind.m[13])+std::abs(inverseBind.m[14])+std::abs(inverseBind.m[15]-1)<1e-6,"inverse bind matrix is not affine");
                    skinMatrices.push_back(worlds[joint]*inverseBind);
                    (void)inverse(skinMatrices.back()); // Reject singular bind matrices before normal/skin evaluation.
                }
            }
            // One Scene model per glTF mesh node, with all of its primitives
            // as material-tagged geometries. Keeping this identity allows
            // object pins and seam normals to cross material boundaries.
            Model m; m.id=id++; m.name=string(node,"name",string(mesh,"name","gltf_mesh_"+std::to_string(ni)));
            m.type="Mesh";m.parent=skinId?0:int64_t(ni)+1;
            for(const auto &primitive:primitives.GetArray()) {
                check(++primitiveCount<=4096,"too many glTF mesh primitives");
                const size_t mode=number(primitive,"mode",4);check(mode==4||mode==5||mode==6,"only triangle, strip and fan primitives are supported");
                if(auto ext=member(primitive,"extensions"))check(!member(*ext,"KHR_draco_mesh_compression"),"Draco-compressed primitive is unsupported");
                const auto &attributes=required(primitive,"attributes");const auto &pos=accessor(integer(required(attributes,"POSITION")));
                check(pos.components==3&&pos.type==5126,"POSITION must be float VEC3");
                vertices+=pos.count;check(vertices<=maxElements,"scene exceeds vertex limit");charge(pos.values.size(),sizeof(double));
                Geom g;g.id=id;g.name=string(node,"name",string(mesh,"name","gltf_mesh_"+std::to_string(ni)));
                g.ctrl=pos.values;for(double p:g.ctrl)check(std::abs(p)<=1e10,"glTF position outside supported range");
                const Accessor *nrm=nullptr;
                if(auto p=member(attributes,"NORMAL")){nrm=&accessor(integer(*p));check(nrm->components==3&&nrm->count==pos.count&&nrm->type==5126,"invalid NORMAL accessor");charge(nrm->values.size(),sizeof(double));g.nrm.data=nrm->values;}
                // Bake the initial morph state, not animation. Native gameplay animation remains active.
                if(auto targets=member(primitive,"targets")) {
                    const Json *weights=member(node,"weights");if(!weights)weights=member(mesh,"weights");
                    array(*targets);if(weights)check(weights->IsArray()&&weights->Size()==targets->Size(),"morph weights/targets differ");
                    for(size_t ti=0;weights&&ti<targets->Size();++ti){double weight=real(at(*weights,ti));if(weight==0)continue;const auto&t=at(*targets,ti);
                        for(const char *channel:{"POSITION","NORMAL"})if(auto a=member(t,channel)) {
                            const auto &delta=accessor(integer(*a));check(delta.count==pos.count&&delta.components==3&&delta.type==5126,"invalid morph delta accessor");
                            auto &dst=std::string(channel)=="POSITION"?g.ctrl:g.nrm.data;
                            if(dst.empty())continue;for(size_t k=0;k<dst.size();++k)dst[k]+=weight*delta.values[k];
                        }
                    }
                }
                auto mat=material(primitive);
                std::string uvKey="TEXCOORD_"+std::to_string(mat.uvSet);
                if(auto p=member(attributes,uvKey.c_str())) {
                    const auto &uv=accessor(integer(*p));check(uv.components==2&&uv.count==pos.count&&(uv.type==5126||((uv.type==5121||uv.type==5123)&&uv.normalized)),"invalid texture coordinates");
                    charge(uv.values.size(),sizeof(double));g.uv.valid=true;g.uv.mapping="ByVertice";g.uv.reference="Direct";g.uv.comps=2;g.uv.data.resize(uv.values.size());
                    const double c=std::cos(mat.rotation),s=std::sin(mat.rotation);
                    for(size_t k=0;k<pos.count;++k){double u=uv.values[2*k]*mat.sx,v=uv.values[2*k+1]*mat.sy;g.uv.data[2*k]=mat.ox+c*u-s*v;g.uv.data[2*k+1]=1.-(mat.oy+s*u+c*v);}
                }else check(!scene.matTexStem.count(mat.id),"textured primitive lacks selected TEXCOORD set");
                std::vector<uint32_t> indices;
                if(auto p=member(primitive,"indices")) {
                    const auto &idx=accessor(integer(*p));check(idx.components==1&&!idx.normalized&&(idx.type==5121||idx.type==5123||idx.type==5125),"invalid triangle index accessor");
                    charge(idx.count,sizeof(uint32_t));indices.reserve(idx.count);for(double v:idx.values){check(v<pos.count,"triangle index outside positions");indices.push_back(uint32_t(v));}
                }else{charge(pos.count,sizeof(uint32_t));indices.resize(pos.count);for(size_t k=0;k<pos.count;++k)indices[k]=uint32_t(k);}
                check(indices.size()>=3&&(mode!=4||indices.size()%3==0),"incomplete triangle primitive");
                const size_t faces=mode==4?indices.size()/3:indices.size()-2;
                triangles+=faces;check(triangles<=maxElements,"scene exceeds triangle limit");charge(faces*3,sizeof(int64_t));g.pvi.reserve(faces*3);
                auto emit=[&](uint32_t a,uint32_t b,uint32_t c){if(a==b||b==c||a==c)return;g.pvi.push_back(a);g.pvi.push_back(b);g.pvi.push_back(~int64_t(c));};
                if(mode==4)for(size_t k=0;k<indices.size();k+=3)emit(indices[k],indices[k+1],indices[k+2]);
                else for(size_t k=2;k<indices.size();++k){if(mode==6)emit(indices[0],indices[k-1],indices[k]);else if(k%2)emit(indices[k-1],indices[k-2],indices[k]);else emit(indices[k-2],indices[k-1],indices[k]);}
                if(g.pvi.empty())continue;
                // Missing normals: area-weighted smooth normals before the game-winding conversion.
                if(g.nrm.data.empty()) {
                    charge(g.ctrl.size(),sizeof(double));g.nrm.data.assign(g.ctrl.size(),0.);
                    for(size_t k=0;k<g.pvi.size();k+=3){size_t a=size_t(g.pvi[k]),b=size_t(g.pvi[k+1]),c=size_t(~g.pvi[k+2]);V3 u{g.ctrl[3*b]-g.ctrl[3*a],g.ctrl[3*b+1]-g.ctrl[3*a+1],g.ctrl[3*b+2]-g.ctrl[3*a+2]},v{g.ctrl[3*c]-g.ctrl[3*a],g.ctrl[3*c+1]-g.ctrl[3*a+1],g.ctrl[3*c+2]-g.ctrl[3*a+2]};double n[]={u.y*v.z-u.z*v.y,u.z*v.x-u.x*v.z,u.x*v.y-u.y*v.x};for(size_t q:{a,b,c})for(int d=0;d<3;++d)g.nrm.data[3*q+d]+=n[d];}
                }
                g.nrm.valid=true;g.nrm.mapping="ByVertice";g.nrm.reference="Direct";g.nrm.comps=3;
                if(skinId) {
                    std::vector<std::pair<const Accessor*,const Accessor*>> sets;
                    for(size_t set=0;set<8;++set){auto a=member(attributes,("JOINTS_"+std::to_string(set)).c_str()),b=member(attributes,("WEIGHTS_"+std::to_string(set)).c_str());check(bool(a)==bool(b),"skin JOINTS/WEIGHTS pair missing");if(!a)break;const auto&j=accessor(integer(*a));const auto&w=accessor(integer(*b));check(j.components==4&&w.components==4&&j.count==pos.count&&w.count==pos.count&&!j.normalized&&(j.type==5121||j.type==5123)&&(w.type==5126||((w.type==5121||w.type==5123)&&w.normalized)),"invalid skin attributes");sets.push_back({&j,&w});}
                    check(!sets.empty(),"skinned primitive has no JOINTS_0/WEIGHTS_0");
                    for(auto a=attributes.MemberBegin();a!=attributes.MemberEnd();++a) {
                        const std::string key(a->name.GetString(),a->name.GetStringLength());
                        if(key.rfind("JOINTS_",0)==0||key.rfind("WEIGHTS_",0)==0) {
                            bool supported=false;for(size_t set=0;set<sets.size();++set)if(key=="JOINTS_"+std::to_string(set)||key=="WEIGHTS_"+std::to_string(set))supported=true;
                            check(supported,"skin attribute sets must be contiguous and at most eight");
                        }
                    }
                    g.clusters.resize(joints.size());
                    for(size_t j=0;j<joints.size();++j){auto&cl=g.clusters[j];cl.boneModelId=int64_t(joints[j])+1;cl.boneName=Scene::normName(scene.models.at(cl.boneModelId).name);cl.haveLink=cl.haveTransform=true;cl.linkMatrix=rowMatrix(worlds[joints[j]]);cl.transformMatrix=rowMatrix(inverse(worlds[joints[j]]));for(int d=0;d<3;++d)cl.linkPos[d]=worlds[joints[j]].m[d*4+3];}
                    // Evaluate bind/default pose according to glTF skinning (mesh-node transform is ignored).
                    // Then expose that pose to the existing FBX-style retargeter with identity mesh bind.
                    for(size_t v=0;v<pos.count;++v) {
                        std::map<size_t,double> influences;
                        for(const auto &set:sets)for(int k=0;k<4;++k){size_t j=size_t(set.first->values[v*4+k]);double w=set.second->values[v*4+k];check(j<joints.size()&&w>=0&&w<=1.000001,"invalid skin joint/weight");if(w>0)influences[j]+=w;}
                        double sum=0;for(const auto&w:influences)sum+=w.second;check(sum>1e-12&&std::isfinite(sum),"unweighted glTF skinned vertex");
                        const V3 p{g.ctrl[3*v],g.ctrl[3*v+1],g.ctrl[3*v+2]},n{g.nrm.data[3*v],g.nrm.data[3*v+1],g.nrm.data[3*v+2]};V3 bp{},bn{};
                        for(const auto &inf:influences){double w=inf.second/sum;const auto sp=skinMatrices[inf.first].point(p),sn=skinMatrices[inf.first].normalMatrix().vec(n);bp.x+=w*sp.x;bp.y+=w*sp.y;bp.z+=w*sp.z;bn.x+=w*sn.x;bn.y+=w*sn.y;bn.z+=w*sn.z;auto&cl=g.clusters[inf.first];charge(1,sizeof(int64_t)+sizeof(double));cl.idx.push_back(int64_t(v));cl.w.push_back(w);}
                        g.ctrl[3*v]=bp.x;g.ctrl[3*v+1]=bp.y;g.ctrl[3*v+2]=bp.z;g.nrm.data[3*v]=bn.x;g.nrm.data[3*v+1]=bn.y;g.nrm.data[3*v+2]=bn.z;
                    }
                }
                for(double v:g.ctrl)check(std::isfinite(v)&&std::abs(v)<=1e10,"invalid morphed/skinned position");
                for(size_t v=0;v<pos.count;++v){double*n=g.nrm.data.data()+v*3;double l=std::sqrt(n[0]*n[0]+n[1]*n[1]+n[2]*n[2]);check(std::isfinite(l),"invalid normal");if(l>1e-12)for(int d=0;d<3;++d)n[d]/=l;else{n[0]=0;n[1]=1;n[2]=0;}}
                g.matMapping="AllSame";g.matIdx={int64_t(m.materials.size())};
                m.geoms.push_back(id);m.materials.push_back(mat.id);
                scene.geoms.emplace(id,std::move(g));++id;
            }
            if(!m.geoms.empty()) {scene.meshModelOrder.push_back(m.id);scene.models.emplace(m.id,std::move(m));}
        }
        check(!scene.meshModelOrder.empty(),"selected glTF scene has no supported geometry");
        scene.isGltf=true;scene.sceneScale=1.;scene.unitScale=100.;
        if(member(root,"animations"))logf("[modmesh] glTF animation clips are not played; initial pose and native game animations are used");
        logf("[modmesh] %s: glTF 2.0, %u primitives, %u vertices, %u triangles, %u diffuse images",path.c_str(),unsigned(primitiveCount),unsigned(vertices),unsigned(triangles),unsigned(images.size()));
    }
};
inline bool parse(const std::string &path,const uint8_t *bytes,size_t size,Scene &scene)
{
    try {
        check(bytes&&size&&size<=maxFileBytes,"empty/oversized glTF file");
        const uint8_t *json=bytes,*bin=nullptr;size_t jsonSize=size,binSize=0;
        if(size>=4&&u32(bytes)==0x46546c67u) {
            check(size>=20&&u32(bytes+4)==2&&u32(bytes+8)==size,"invalid GLB header/version/length");
            size_t pos=12,chunk=0;bool gotJson=false,gotBin=false;
            while(pos<size){check(range(size,pos,8),"truncated GLB chunk header");size_t len=u32(bytes+pos);uint32_t type=u32(bytes+pos+4);pos+=8;check(len%4==0&&range(size,pos,len),"GLB chunk outside file/not aligned");
                if(type==0x4e4f534au){check(chunk==0&&!gotJson,"GLB JSON must be first and unique");json=bytes+pos;jsonSize=len;gotJson=true;}
                else if(type==0x004e4942u){check(gotJson&&!gotBin&&chunk==1,"GLB BIN must be second and unique");bin=bytes+pos;binSize=len;gotBin=true;}
                else check(chunk>0,"first GLB chunk is not JSON");pos+=len;++chunk;
            }check(gotJson,"GLB has no JSON chunk");
        }else if(path.size()>=4) {
            std::string ext=path.substr(path.size()-4);for(auto&c:ext)c=char(std::tolower(uint8_t(c)));check(ext!=".glb","GLB magic is missing");
        }
        check(jsonSize<=32u*1024u*1024u,"glTF JSON exceeds 32 MiB limit");
        // Bound nesting before RapidJSON's recursive DOM parser sees untrusted input.
        size_t depth=0;bool quoted=false,escaped=false;
        for(size_t i=0;i<jsonSize;++i){char c=char(json[i]);check(c!='\0',"NUL byte in glTF JSON");if(quoted){if(escaped)escaped=false;else if(c=='\\')escaped=true;else if(c=='"')quoted=false;}else if(c=='"')quoted=true;else if(c=='{'||c=='['){check(++depth<=64,"JSON nesting limit exceeded");}else if(c=='}'||c==']'){check(depth>0,"unbalanced JSON");--depth;}}
        rapidjson::Document doc;doc.Parse<rapidjson::kParseValidateEncodingFlag>(reinterpret_cast<const char*>(json),jsonSize);
        check(!doc.HasParseError()&&doc.IsObject(),"invalid glTF JSON");
        Reader reader(doc,path,scene);reader.run(bin,binSize);return true;
    }catch(const std::exception &e){logf("[modmesh] %s: glTF rejected: %s",path.c_str(),e.what());return false;}
}
} // namespace gltf
