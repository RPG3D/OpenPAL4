#include "pal4/portable/actor.h"
#include "pal4/atomic_position_updater.h"
#include "pal4/atomic_bounding_box_builder.h"
#include <algorithm>
#include <limits>
#include <bit>
#include <cmath>
#include <stdexcept>

namespace pal4::portable {
namespace {
void Require(bool value){if(!value)throw std::runtime_error("invalid actor bind bounds");}
Matrix Multiply(const Matrix& a,const Matrix& b) {
    Matrix result{};
    for(unsigned c=0;c<4;++c)for(unsigned r=0;r<4;++r)for(unsigned k=0;k<4;++k)result[c*4+r]+=a[k*4+r]*b[c*4+k];
    return result;
}
Matrix Inverse(const Matrix& m) {
    const auto a=m[0],b=m[4],c=m[8],d=m[1],e=m[5],f=m[9],g=m[2],h=m[6],i=m[10];
    const float determinant=a*(e*i-f*h)-b*(d*i-f*g)+c*(d*h-e*g);
    Require(std::isfinite(determinant) && determinant!=0);
    Matrix out{(e*i-f*h)/determinant,(f*g-d*i)/determinant,(d*h-e*g)/determinant,0,
        (c*h-b*i)/determinant,(a*i-c*g)/determinant,(b*g-a*h)/determinant,0,
        (b*f-c*e)/determinant,(c*d-a*f)/determinant,(a*e-b*d)/determinant,0,0,0,0,1};
    for(unsigned r=0;r<3;++r)out[12+r]=-(out[r]*m[12]+out[4+r]*m[13]+out[8+r]*m[14]);
    return out;
}
pal4::AtomicMatrix Encode(const Matrix& matrix){return std::bit_cast<pal4::AtomicMatrix>(matrix);}
Matrix Decode(const pal4::AtomicMatrix& matrix){return std::bit_cast<Matrix>(matrix);}
class BoundsRuntime final:public pal4::IAtomicPositionRuntime,public pal4::IAtomicBoundingBoxRuntime {
public:
    std::vector<Matrix> world;
    const ActorAtomicSphere* atomic{};
    std::array<float,4> result{};
    const ActorGeometryVertices* vertices{};ActorGeometryVertices transformed;
    ActorLocalBox box{{-std::numeric_limits<float>::max(),-std::numeric_limits<float>::max(),-std::numeric_limits<float>::max()},
                     {std::numeric_limits<float>::max(),std::numeric_limits<float>::max(),std::numeric_limits<float>::max()}};
    bool allocated{};
    explicit BoundsRuntime(const Actor& actor) {
        for(std::size_t i=0;i<actor.frames.size();++i) {
            const auto& frame=actor.frames[i];Require(frame.parent>=-1 && frame.parent<static_cast<std::int32_t>(i));
            for(float v:frame.local)Require(std::isfinite(v));
            world.push_back(frame.parent<0?frame.local:Multiply(world[static_cast<std::size_t>(frame.parent)],frame.local));
        }
    }
    const Matrix& Transform(std::uint32_t handle) {
        Require(handle>=0x10000000 && (handle-0x10000000)%64==0 && (handle-0x10000000)/64<world.size());
        return world[(handle-0x10000000)/64];
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        if(address==0x1018){Require(vertices);return 0x6000;}
        if(address==0x6014){Require(vertices && vertices->size()<=3000000);return static_cast<std::uint32_t>(vertices->size());}
        if(address==0x605c)return 0x7000;
        if(address==0x7014)return 0x8000;
        if(address==0x104c)return 0; // first morph sphere is already resolved
        if(address==0x1004)return 0x10000000+atomic->frame*64;
        if(address==0x103c)return 0x2000;
        if(address==0x2004)return 0x10000000; // clump root frame
        if(address>=0x101c && address<=0x1028 && address%4==0)return std::bit_cast<std::uint32_t>(atomic->sphere[(address-0x101c)/4]);
        if(address>=0x3000 && address<=0x300c && address%4==0)return std::bit_cast<std::uint32_t>(result[(address-0x3000)/4]);
        if(address>=0x10000000 && address%4==0 && (address-0x10000000)/64<world.size())return Encode(Transform(0x10000000+(address-0x10000000)/64*64))[(address-0x10000000)%64/4];
        throw std::logic_error("unbound actor sphere layout read");
    }
    void WriteWord(std::uint32_t address,std::uint32_t value) override {
        Require(address>=0x3000 && address<=0x300c && address%4==0);result[(address-0x3000)/4]=std::bit_cast<float>(value);
    }
    void ResyncInterpolatedSphere(std::uint32_t) override {throw std::logic_error("animated atomic bounds are not bound");}
    std::uint32_t Allocate(std::uint32_t bytes,std::uint32_t flags) override {
        Require(vertices && !vertices->empty() && !allocated && !flags && bytes==vertices->size()*12);transformed.resize(vertices->size());allocated=true;return 0x9000;
    }
    void Free(std::uint32_t address) override {Require(address==0x9000 && allocated);allocated=false;transformed.clear();}
    bool UseSecondaryDirectTransform() override {throw std::logic_error("bounds require explicit transform mode");}
    void TransformPoints(std::uint32_t destination,std::uint32_t source,std::int32_t count,const pal4::AtomicMatrix& transform) override {
        Require(destination==0x9000 && source==0x8000 && allocated && vertices && count>0 && static_cast<std::size_t>(count)==vertices->size());const auto matrix=Decode(transform);
        for(std::size_t i=0;i<vertices->size();++i){const auto& v=(*vertices)[i];for(float value:v){Require(std::isfinite(value));}
            for(unsigned a=0;a<3;++a){transformed[i][a]=matrix[a]*v[0]+matrix[4+a]*v[1]+matrix[8+a]*v[2]+matrix[12+a];Require(std::isfinite(transformed[i][a]));}
        }
    }
    void AddPoint(std::uint32_t address,std::uint32_t point) override {
        Require(address==0xa000 && allocated && point>=0x9000 && (point-0x9000)%12==0 && (point-0x9000)/12<transformed.size());const auto& v=transformed[(point-0x9000)/12];
        for(unsigned a=0;a<3;++a){box.maximum[a]=std::max(box.maximum[a],v[a]);box.minimum[a]=std::min(box.minimum[a],v[a]);}
    }
    bool UseDirectTransform() override {throw std::logic_error("bounds require explicit transform mode");}
    std::uint32_t GetObjectTransform(std::uint32_t frame) override {(void)Transform(frame);return frame;}
    void SetupTransformMatrix(pal4::AtomicMatrix& destination,std::uint32_t source) override {destination=Encode(Inverse(Transform(source)));}
    void MultiplyMatrices(pal4::AtomicMatrix& destination,std::uint32_t source,const pal4::AtomicMatrix& parent) override {
        destination=Encode(Multiply(Decode(parent),Transform(source)));
    }
    void TransformSphere(pal4::AtomicSphere& sphere,const pal4::AtomicMatrix& transform) override {
        const auto m=Decode(transform);const auto old=sphere;
        for(unsigned r=0;r<3;++r)sphere[r]=m[r]*old[0]+m[4+r]*old[1]+m[8+r]*old[2]+m[12+r];
        // Original callback transforms the center only, retaining the radius.
    }
    void CalculatePosition(std::uint32_t,std::uint32_t,std::uint32_t) override {throw std::logic_error("unbound actor sphere position callback");}
    bool IsInitialSphere() override {throw std::logic_error("bounds require explicit initial flag");}
    void ClearInitialSphere() override {throw std::logic_error("bounds require explicit initial flag");}
    float Length(const pal4::AtomicVector3& v) override {return std::sqrt(v[0]*v[0]+v[1]*v[1]+v[2]*v[2]);}
};
}
std::optional<ActorLocalBox> ActorBindBox(const Actor& actor,bool direct){
    if(!actor.bind_vertices)return {};
    Require(actor.render_bindings && !actor.atomic_spheres.empty() && actor.render_bindings->atomics.size()==actor.atomic_spheres.size() && actor.bind_vertices->size()==actor.render_bindings->geometry_materials.size());
    BoundsRuntime runtime(actor);
    for(std::size_t i=0;i<actor.atomic_spheres.size();++i){const auto& atomic=actor.atomic_spheres[i];const auto geometry=actor.render_bindings->atomics[i].geometry;
        Require(atomic.frame<actor.frames.size() && geometry<actor.bind_vertices->size());runtime.atomic=&atomic;runtime.vertices=&(*actor.bind_vertices)[geometry];
        (void)pal4::AtomicBoundingBoxBuilder::ProcessBoundingBox(0x1000,direct?1u:0u,0xa000,runtime);
    }
    for(unsigned a=0;a<3;++a){Require(std::isfinite(runtime.box.maximum[a]) && std::isfinite(runtime.box.minimum[a]) && runtime.box.maximum[a]>=runtime.box.minimum[a]);}
    return runtime.box;
}
std::optional<std::array<float,4>> ActorBindSphere(const Actor& actor,bool direct) {
    if(actor.atomic_spheres.empty())return {};
    Require(!actor.frames.empty());BoundsRuntime runtime(actor);std::uint32_t initial=1;
    for(const auto& atomic:actor.atomic_spheres) {
        Require(atomic.frame<actor.frames.size());for(float v:atomic.sphere)Require(std::isfinite(v));Require(atomic.sphere[3]>=0);
        runtime.atomic=&atomic;(void)pal4::AtomicPositionUpdater::ProcessBoundingSphere(0x1000,direct?1u:0u,initial,0x3000,runtime);
    }
    for(float v:runtime.result)Require(std::isfinite(v));
    Require(runtime.result[3]>=0);
    return runtime.result;
}
} // namespace pal4::portable
