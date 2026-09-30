#include "../src/mod_mesh_donor_surface.h"
#include <iostream>
#include <stdexcept>

namespace surface = modmesh::donorsurface;
struct Corner {
    float px=0, py=0, pz=0;
    float bi[4]={-1,-1,-1,-1}, bw[4]={0,0,0,0};
};
struct Donor { float p[3]={0,0,0}, bi[4]={-1,-1,-1,-1}, bw[4]={0,0,0,0}; };
struct Grid {
    std::vector<Donor> donors;
    void build() {}
    const Donor *nearest(const float p[3]) const {
        const Donor *found=nullptr; double best=std::numeric_limits<double>::infinity();
        for (const auto &d:donors) {
            double distance=0;
            for (unsigned k=0;k<3;++k) {const double delta=p[k]-d.p[k];distance+=delta*delta;}
            if (distance<best) {best=distance;found=&d;}
        }
        return found;
    }
};
static void require(bool value,const char *message) {if (!value) throw std::runtime_error(message);}
static void checkAnchors(float scale) {
    std::vector<Corner> corners;
    // A connected arm/torso/leg strip with a known native four-bone leg skin.
    // Smoothing the junction cannot create arm control half a body away.
    for (int row=0;row<80;++row) for (int triangle=0;triangle<2;++triangle) {
        const int x[2][3]={{0,1,0},{1,1,0}}, y[2][3]={{0,0,1},{0,1,1}};
        for (int k=0;k<3;++k) {
            const int height=row+y[triangle][k]; Corner c;
            c.px=x[triangle][k]*.01f*scale; c.py=height*.01f*scale;
            if (height>=72) {c.bi[0]=4; c.bw[0]=1;}
            else for (int j=0;j<4;++j) {c.bi[j]=float(j); c.bw[j]=.25f;}
            corners.push_back(c);
        }
    }
    Grid native;
    for (int i=0;i<2;++i) {
        Donor d; d.p[1]=i?.8f*scale:0;
        if (i) {d.bi[0]=4; d.bw[0]=1;}
        else for (int j=0;j<4;++j) {d.bi[j]=float(j); d.bw[j]=.25f;}
        native.donors.push_back(d);
    }
    const auto before=corners; surface::Stats stats;std::string why;
    require(surface::smooth(std::vector<surface::Surface<Corner>>{{1,&corners}},
        {0,0,0,0,1},native,&stats,&why),why.c_str());
    require(corners.size()==before.size(),"all complete source triangles must survive");
    size_t legCorners=0, transitionalCorners=0;
    for (size_t i=0;i<corners.size();++i) {
        const auto &c=corners[i];
        require(c.px==before[i].px&&c.py==before[i].py&&c.pz==before[i].pz,
            "smoothing must not move source positions");
        float sum=0,arm=0;
        for (int k=0;k<4;++k) {
            require(std::isfinite(c.bw[k])&&c.bw[k]>=0,"finite nonnegative skin weights");
            sum+=c.bw[k]; if(c.bi[k]==4)arm+=c.bw[k];
        }
        require(std::abs(sum-1)<1e-6f,"normalized skin weights");
        if (c.py<=.4f*scale) {
            ++legCorners;
            require(arm==0,"distant leg must not acquire an arm influence");
            for (int k=0;k<4;++k)
                require(c.bi[k]==float(k)&&c.bw[k]==.25f,
                    "all four native leg influences must remain unchanged");
        }
        if (arm>0&&arm<1) ++transitionalCorners;
    }
    require(legCorners>200,"fixture covers distant lower-body surface");
    require(transitionalCorners>0,"junction is still smoothed between anchored regions");
}
int main() {
    try {
        for(float scale:{.01f,1.f,100.f}) checkAnchors(scale);
        std::cout<<"PASS anchored donor surface: distant leg weights preserved, local junction smoothed, scale invariance, source triangles and positions preserved\n";
        return 0;
    } catch(const std::exception &e) {std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}
}
