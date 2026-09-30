#pragma once
// Runtime adaptation port of the user-supplied usm_toolkit add-on:
// usm_noyau/usm_pipeline.py: orienter, aligner, ecarter_aberrants,
// normales_par_position, Epingle.contributions. See THIRD_PARTY_MESH_PORT.md.
// No Blender, Python, fixed Spider-Man offsets, or 66-bone assumptions.
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <map>
#include <string>
#include <vector>

namespace modmesh { namespace toolkit {
struct Pin {
    std::string target, mode;
    int center=-1, left=-1, right=-1;
    double strength=.85;
};
struct Settings {
    int fitAxes=-1; // -1: automatic for custom donor-skinned characters only
    std::string upAxis="none";
    bool flipUp=false, weldNormals=false, dropOutliers=false;
    double outlierFactor=8.;
    std::vector<Pin> pins;
    std::string error;
};
inline std::string lower(std::string s)
{for(char&c:s)if(c>='A'&&c<='Z')c=char(c-'A'+'a');return s;}
inline std::string trim(std::string s)
{auto a=s.find_first_not_of(" \t\r\n"),b=s.find_last_not_of(" \t\r\n");return a==std::string::npos?"":s.substr(a,b-a+1);}
inline bool scalar(const std::string&s,double&value)
{char*end=nullptr;value=std::strtod(s.c_str(),&end);return end!=s.c_str()&&!*end&&std::isfinite(value);}
inline bool boolean(const std::string&s,bool&v)
{auto t=lower(trim(s));if(t=="1"||t=="on"||t=="true"||t=="yes"){v=true;return true;}if(t=="0"||t=="off"||t=="false"||t=="no"){v=false;return true;}return false;}
// Returns true for recognized keys, including invalid values (error is sticky).
inline bool option(Settings&cfg,const std::string&key,const std::string&value)
{
    const auto v=lower(trim(value));bool good=true;
    if(key=="fit_axes") {bool b=false;good=boolean(v,b);cfg.fitAxes=b?1:0;}
    else if(key=="up_axis") {good=v=="none"||v=="auto"||v=="x"||v=="y"||v=="z";if(good)cfg.upAxis=v;}
    else if(key=="flip_up")good=boolean(v,cfg.flipUp);
    else if(key=="weld_normals")good=boolean(v,cfg.weldNormals);
    else if(key=="drop_outliers")good=boolean(v,cfg.dropOutliers);
    else if(key=="outlier_factor")good=scalar(v,cfg.outlierFactor)&&cfg.outlierFactor>1.;
    else if(key.rfind("pin.",0)==0) {
        Pin p;p.target=lower(trim(key.substr(4)));std::vector<std::string> fields;size_t start=0;
        do{size_t end=v.find(',',start);fields.push_back(trim(v.substr(start,end==std::string::npos?end:end-start)));if(end==std::string::npos)break;start=end+1;}while(true);
        good=!p.target.empty()&&cfg.pins.size()<128&&!fields.empty();
        if(good){p.mode=fields[0];size_t expected=p.mode=="skirt"?5:p.mode=="mix"?3:p.mode=="rigid"?2:0;
            good=expected&&fields.size()==expected;
            auto bone=[&](size_t i,int&b){double x=0;bool ok=scalar(fields[i],x)&&x>=0&&x<=1023&&x==std::floor(x);if(ok)b=int(x);return ok;};
            if(good)good=bone(1,p.center);
            if(good&&p.mode=="skirt")good=bone(2,p.left)&&bone(3,p.right)&&p.left!=p.right&&p.left!=p.center&&p.right!=p.center&&scalar(fields[4],p.strength);
            if(good&&p.mode=="mix")good=scalar(fields[2],p.strength);
            good=good&&p.strength>=0&&p.strength<=1.;
        }
        if(good)cfg.pins.push_back(std::move(p));
    }else return false;
    if(!good&&cfg.error.empty())cfg.error="invalid toolkit option: "+key+"="+value;
    return true;
}
struct Bounds {
    std::array<double,3> lo{{1e30,1e30,1e30}},hi{{-1e30,-1e30,-1e30}};bool valid=false;
    void add(double x,double y,double z){double v[]={x,y,z};for(double c:v)if(!std::isfinite(c))return;for(int k=0;k<3;++k){lo[k]=std::min(lo[k],v[k]);hi[k]=std::max(hi[k],v[k]);}valid=true;}
    double diagonal()const{if(!valid)return 0;double sum=0;for(int i=0;i<3;++i)sum+=(hi[i]-lo[i])*(hi[i]-lo[i]);return std::sqrt(sum);}
};
struct Fit {
    std::array<double,3> scale{{1,1,1}},shift{{0,0,0}};bool valid=false;
};
inline Fit fit(const Bounds&source,const Bounds&target)
{
    Fit out;if(!source.valid||!target.valid)return out;
    double fallback=source.hi[1]-source.lo[1]>1e-9?(target.hi[1]-target.lo[1])/(source.hi[1]-source.lo[1]):1.;
    if(!(fallback>1e-9))fallback=1.;
    for(int k=0;k<3;++k){double extent=source.hi[k]-source.lo[k],goal=target.hi[k]-target.lo[k];
        // A flat plane has no thickness to fit. Do not collapse normals/geometry.
        out.scale[k]=(extent>1e-9&&goal>1e-9)?goal/extent:fallback;
        if(!std::isfinite(out.scale[k])||out.scale[k]<1e-9||out.scale[k]>1e9)return Fit{};
        out.shift[k]=(target.lo[k]+target.hi[k])/2.-(source.lo[k]+source.hi[k])/2.*out.scale[k];
    }out.valid=true;return out;
}
template<class Corner>inline void apply(const Fit&f,Corner&c)
{
    c.px=float(c.px*f.scale[0]+f.shift[0]);c.py=float(c.py*f.scale[1]+f.shift[1]);c.pz=float(c.pz*f.scale[2]+f.shift[2]);
    double n[]={c.nx/f.scale[0],c.ny/f.scale[1],c.nz/f.scale[2]};double length=std::sqrt(n[0]*n[0]+n[1]*n[1]+n[2]*n[2]);
    if(length>1e-12){c.nx=float(n[0]/length);c.ny=float(n[1]/length);c.nz=float(n[2]/length);}
}
template<class Bucket>inline int orient(const std::vector<Bucket*>&parts,const Settings&cfg,bool&flipped)
{
    int axis=cfg.upAxis=="x"?0:cfg.upAxis=="z"?2:1;flipped=cfg.flipUp;
    if(cfg.upAxis=="auto") {
        // Unique positions avoid bias from material/UV splits and tessellation corners.
        std::vector<std::array<double,3>> points;for(auto*b:parts)for(const auto&c:b->corners)points.push_back({{c.px,c.py,c.pz}});
        std::sort(points.begin(),points.end());points.erase(std::unique(points.begin(),points.end()),points.end());
        Bounds bounds;for(const auto&p:points)bounds.add(p[0],p[1],p[2]);
        if(!points.empty()&&bounds.valid){double best=2.;for(int k=0;k<3;++k){double extent=bounds.hi[k]-bounds.lo[k],central=1.;if(extent>1e-12){size_t count=0;for(const auto&p:points){double t=(p[k]-bounds.lo[k])/extent;if(t>.4&&t<.6)++count;}central=double(count)/points.size();}if(central<best){best=central;axis=k;}}
            std::vector<double> values;for(const auto&p:points)values.push_back(p[axis]);std::sort(values.begin(),values.end());double median=values[values.size()/2];if(values.size()%2==0)median=(median+values[values.size()/2-1])/2.;if(median<=(bounds.lo[axis]+bounds.hi[axis])/2.)flipped=!flipped;
        }
    }
    // Proper rotations (determinant +1), same as Blender toolkit orienter().
    auto rotate=[&](float&x,float&y,float&z){float a=x,b=y,d=z;if(axis==2){y=d;z=-b;}else if(axis==0){x=-b;y=a;}if(flipped){y=-y;z=-z;}};
    for(auto*b:parts)for(auto&c:b->corners){rotate(c.px,c.py,c.pz);rotate(c.nx,c.ny,c.nz);}return axis;
}
inline std::vector<int64_t> outliers(const std::map<int64_t,Bounds>&bounds,double factor)
{
    std::vector<int64_t> result;if(bounds.size()<3)return result;std::vector<double>d;
    for(const auto&e:bounds){double n=e.second.diagonal();if(n>0&&std::isfinite(n))d.push_back(n);}if(d.empty())return result;
    std::sort(d.begin(),d.end());double median=d[d.size()/2];if(d.size()%2==0)median=(median+d[d.size()/2-1])/2.;
    for(const auto&e:bounds)if(e.second.diagonal()>factor*median)result.push_back(e.first);return result;
}
inline bool validatePins(const Settings&cfg,int nbones,const std::vector<float>&positions,std::string&why)
{
    if(!cfg.error.empty()){why=cfg.error;return false;}
    for(const auto&p:cfg.pins){if(p.center<0||p.center>=nbones){why="pin center bone outside target skeleton";return false;}
        if(p.mode=="skirt"){if(p.left<0||p.right<0||p.left>=nbones||p.right>=nbones||positions.size()!=size_t(nbones)*3){why="skirt pin needs valid target bind positions and lateral bones";return false;}for(float v:positions)if(!std::isfinite(v)||std::abs(v)>=1e8){why="skirt pin bind positions invalid";return false;}}
    }return true;
}
template<class Corner>inline bool pin(const Pin&p,const std::vector<float>&positions,Corner&c)
{
    std::map<int,double> weights;
    if(p.mode=="rigid")weights[p.center]=1.;
    else if(p.mode=="mix") {for(int k=0;k<4;++k)if(c.bi[k]>=0&&c.bw[k]>0)weights[int(c.bi[k])]+=(1.-p.strength)*c.bw[k];weights[p.center]+=p.strength;}
    else {
        const float*center=positions.data()+size_t(p.center)*3;double low=center[1];for(size_t i=1;i<positions.size();i+=3)low=std::min(low,double(positions[i]));
        const double flank=std::max(std::abs(double(positions[size_t(p.left)*3])-positions[size_t(p.right)*3]),1e-3);
        const double depth=std::max(.6*(center[1]-low),1e-3);
        const double t=std::clamp((center[1]-c.py)/depth,0.,1.)*p.strength;
        if(t<=1e-6)return false; // upper coat keeps the transferred spine weights
        const double wl=std::clamp((center[0]-c.px)/flank,0.,1.),wr=std::clamp((c.px-center[0])/flank,0.,1.);
        weights[p.center]=1.-t*(wl+wr);weights[p.left]=t*wl;weights[p.right]=t*wr;
    }
    std::vector<std::pair<int,double>> sorted;for(const auto&e:weights)if(e.second>1e-6)sorted.push_back(e);
    std::stable_sort(sorted.begin(),sorted.end(),[](const auto&a,const auto&b){return a.second>b.second;});if(sorted.size()>4)sorted.resize(4);
    double sum=0;for(const auto&e:sorted)sum+=e.second;if(sum<=0)return false;
    for(int k=0;k<4;++k){c.bi[k]=k<int(sorted.size())?float(sorted[k].first):-1.f;c.bw[k]=k<int(sorted.size())?float(sorted[k].second/sum):0.f;}return true;
}
// Area-weighted seam normals, grouped by source object rather than material.
// Positions are measured after fitting. Hard edges are retained unless explicitly enabled.
template<class Bucket>inline void weldNormals(const std::vector<std::pair<int64_t,Bucket*>>&parts)
{
    using Key=std::array<int64_t,4>;std::map<Key,std::array<double,3>> sums;
    auto key=[](int64_t object,const auto&c){return Key{{object,int64_t(std::llround(double(c.px)*10000)),int64_t(std::llround(double(c.py)*10000)),int64_t(std::llround(double(c.pz)*10000))}};};
    for(const auto&part:parts){auto&v=part.second->corners;for(size_t i=0;i+2<v.size();i+=3){const auto&a=v[i],&b=v[i+1],&c=v[i+2];double ux=b.px-a.px,uy=b.py-a.py,uz=b.pz-a.pz,vx=c.px-a.px,vy=c.py-a.py,vz=c.pz-a.pz;std::array<double,3>n{{uy*vz-uz*vy,uz*vx-ux*vz,ux*vy-uy*vx}};double dot=n[0]*(a.nx+b.nx+c.nx)+n[1]*(a.ny+b.ny+c.ny)+n[2]*(a.nz+b.nz+c.nz);if(dot<0)for(double&d:n)d=-d;for(size_t j=0;j<3;++j){auto&s=sums[key(part.first,v[i+j])];for(int d=0;d<3;++d)s[d]+=n[d];}}}
    for(const auto&part:parts)for(auto&c:part.second->corners){auto&s=sums[key(part.first,c)];double l=std::sqrt(s[0]*s[0]+s[1]*s[1]+s[2]*s[2]);if(l>1e-12){c.nx=float(s[0]/l);c.ny=float(s[1]/l);c.nz=float(s[2]/l);}}
}
} } // namespace modmesh::toolkit
