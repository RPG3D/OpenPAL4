#include "pal4/portable/effect_geometry.h"
#include "pal4/geo_cylinder.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <stdexcept>
namespace pal4::portable {
namespace {
void Require(bool v,const char* message){if(!v)throw std::logic_error(message);}
struct CylinderBuilder final:pal4::IGeoCylinderRuntime {
    struct Geometry {std::array<Vertex,4> vertices{};std::array<std::array<unsigned,3>,2> triangles{};unsigned writes{},indices{},material{};bool finished{},released{};};
    struct Atomic {unsigned geometry{},frame{};bool pipeline{};};
    std::vector<Geometry> geometries;std::vector<Atomic> atomics;std::vector<unsigned> attached,parents;
    std::vector<unsigned> material_texture;std::vector<bool> material_released;
    std::optional<Texture> texture;unsigned root{},rgba{};bool clump{},identity{};
    Geometry& Geo(unsigned id){Require(id && id<=geometries.size(),"invalid cylinder geometry handle");return geometries[id-1];}
    Atomic& At(unsigned id){Require(id>=0x20001 && id-0x20001<atomics.size(),"invalid cylinder atomic handle");return atomics[id-0x20001];}
    unsigned Material(unsigned id){Require(id>=0x10001 && id-0x10001<material_texture.size(),"invalid cylinder material handle");return id-0x10001;}
    unsigned Frame(unsigned id){Require(id>=0x30001 && id-0x30001<parents.size(),"invalid cylinder frame handle");return id-0x30001;}
    unsigned TextureWidth(unsigned id) override{Require(id==1 && texture.has_value(),"invalid cylinder texture handle");return texture->width;}
    unsigned TextureHeight(unsigned id) override{Require(id==1 && texture.has_value(),"invalid cylinder texture handle");return texture->height;}
    unsigned CreateGeometry(unsigned vertices,unsigned triangles,unsigned flags) override{Require(vertices==4 && triangles==2 && flags==77,"unsupported cylinder geometry layout");geometries.emplace_back();return static_cast<unsigned>(geometries.size());}
    void WriteVertex(unsigned g,unsigned index,pal4::VisualModelVector p,pal4::CylinderUv uv,unsigned color) override {
        auto& geometry=Geo(g);Require(index<4 && !(geometry.writes&(1u<<index)),"invalid cylinder vertex write");geometry.writes|=1u<<index;
        auto& v=geometry.vertices[index];v={{p.x,p.y,p.z},{uv.u,uv.v},{0,0},{0,0,0,0}};
        for(unsigned c=0;c<4;++c)v.color[c]=float((color>>(8*c))&255)/255.f;
    }
    unsigned CreateMaterial() override{material_texture.push_back(0);material_released.push_back(false);return 0x10000+static_cast<unsigned>(material_texture.size());}
    void SetMaterialTexture(unsigned m,unsigned t) override{Require(t==1 && texture.has_value(),"invalid cylinder material texture");material_texture[Material(m)]=t;}
    void SetTriangle(unsigned g,unsigned index,unsigned a,unsigned b,unsigned c,unsigned m) override {
        auto& geometry=Geo(g);Require(index<2 && a<4 && b<4 && c<4 && !(geometry.indices&(1u<<index)),"invalid cylinder triangle");(void)Material(m);Require(!geometry.material || geometry.material==m,"mixed cylinder materials");geometry.material=m;geometry.indices|=1u<<index;geometry.triangles[index]={a,b,c};
    }
    void DestroyMaterial(unsigned m) override{const auto i=Material(m);Require(!material_released[i],"cylinder material released twice");material_released[i]=true;}
    void FinishGeometry(unsigned g) override{auto& geometry=Geo(g);Require(geometry.writes==15 && geometry.indices==3 && geometry.material,"incomplete cylinder geometry");geometry.finished=true;}
    unsigned CreateAtomic() override{atomics.emplace_back();return 0x20000+static_cast<unsigned>(atomics.size());}
    unsigned CreateFrame() override{parents.push_back(0);return 0x30000+static_cast<unsigned>(parents.size());}
    void SetAtomicFrame(unsigned a,unsigned f) override{(void)Frame(f);At(a).frame=f;}
    void SetAtomicGeometry(unsigned a,unsigned g) override{Require(Geo(g).finished,"unfinished cylinder geometry binding");At(a).geometry=g;}
    void DestroyGeometry(unsigned g) override{auto& geometry=Geo(g);Require(!geometry.released,"cylinder construction reference released twice");geometry.released=true;}
    void CaptureSharedAtomicFlags(unsigned a) override{Require(At(a).geometry && At(a).frame,"cylinder render binding is incomplete");At(a).pipeline=true;}
    unsigned CreateClump() override{Require(!clump,"duplicate cylinder clump");clump=true;return 0x40000;}
    void SetClumpFrame(unsigned c,unsigned f) override{Require(c==0x40000 && clump,"invalid cylinder clump");(void)Frame(f);root=f;}
    void SetFrameIdentity(unsigned f) override{Require(f==root,"invalid cylinder root frame");identity=true;}
    void AddAtomic(unsigned c,unsigned a) override{Require(c==0x40000 && At(a).pipeline,"invalid cylinder render part");attached.push_back(a);}
    void AddChildFrame(unsigned parent,unsigned child) override{Require(parent==root,"invalid cylinder frame parent");parents[Frame(child)]=parent;}
    void SetMaterialFlagsForAllAtomics(unsigned c,unsigned color) override{Require(c==0x40000,"invalid cylinder color owner");rgba=color;}
    CylinderMesh Finish(){
        Require(clump && root && identity && attached.size()==atomics.size(),"incomplete cylinder assembly");CylinderMesh result;auto& s=result.scene;s.saturation={1,1,1};
        if(texture)s.textures.push_back(std::move(*texture));
        for(auto it=attached.rbegin();it!=attached.rend();++it){const auto& a=At(*it);const auto& g=Geo(a.geometry);Require(parents[Frame(a.frame)]==root && g.released,"cylinder part ownership is incomplete");
            Draw d{static_cast<unsigned>(s.vertices.size()),6,material_texture[Material(g.material)]?0:-1,-1,{0,0,0,0}};
            for(unsigned c=0;c<4;++c)d.color[c]=float((rgba>>(8*c))&255)/255.f;
            s.draws.push_back(d);for(const auto& tri:g.triangles)for(auto index:tri)s.vertices.push_back(g.vertices[index]);
        }
        Require(!s.vertices.empty(),"empty cylinder mesh");
        for(unsigned c=0;c<3;++c)s.minimum[c]=s.maximum[c]=s.vertices[0].position[c];
        for(const auto& v:s.vertices)for(unsigned c=0;c<3;++c){Require(std::isfinite(v.position[c]),"nonfinite cylinder vertex");s.minimum[c]=std::min(s.minimum[c],v.position[c]);s.maximum[c]=std::max(s.maximum[c],v.position[c]);}
        result.atomics=static_cast<unsigned>(atomics.size());result.source_vertices=result.atomics*4;result.source_triangles=result.atomics*2;return result;
    }
};
}
CylinderMesh BuildCylinder(const CylinderShape& shape,std::optional<Texture> texture){
    Require(shape.sides>=3 && shape.sides<=4096,"cylinder side count outside geometry budget");
    for(float v:{shape.height,shape.top_radius,shape.bottom_radius,shape.border_width})Require(std::isfinite(v),"invalid cylinder shape");
    if(texture)Require(texture->width && texture->height && texture->width<=16384 && texture->height<=16384 && texture->rgba.size()==std::size_t(texture->width)*texture->height*4,"invalid cylinder texture");
    CylinderBuilder builder;builder.texture=std::move(texture);pal4::VisualModelState model;
    const pal4::CylinderCreateOptions options{builder.texture?1u:0u,shape.sides,shape.height,shape.bottom_radius,shape.top_radius,shape.bordered,shape.border_width,shape.arc_degrees,shape.alternate_uv,shape.centered,0};
    Require(pal4::GeoCylinder::Create(model,options,builder)!=0,"original cylinder creation failed");return builder.Finish();
}
}
