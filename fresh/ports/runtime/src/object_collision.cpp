#include "pal4/portable/object_collision.h"
#include <bit>
#include <cmath>
#include <istream>
#include <stdexcept>
namespace pal4::portable {
namespace {
void Require(bool value,const char* message){if(!value)throw std::runtime_error(message);}
std::uint32_t Word(std::istream& in){unsigned char b[4]{};Require(bool(in.read(reinterpret_cast<char*>(b),4)),"truncated object collision cache");return b[0]|(unsigned(b[1])<<8)|(unsigned(b[2])<<16)|(unsigned(b[3])<<24);}
void Finite(float value){Require(std::isfinite(value),"non-finite object collision cache");}
void Affine(const Matrix& m){for(auto v:m)Finite(v);Require(m[3]==0 && m[7]==0 && m[11]==0 && m[15]==1,"object collision transform must be affine");}
Point3 Transform(Point3 p,const Matrix& m){Point3 out{};for(unsigned i=0;i<3;++i){out[i]=m[i]*p[0]+m[4+i]*p[1]+m[8+i]*p[2]+m[12+i];Finite(out[i]);}return out;}
}
ObjectCollisionGeometry ReadObjectCollision(std::istream& input){
    Require(Word(input)==0x47433450 && Word(input)==1,"unsupported object collision cache");
    const auto count=Word(input);Require(count>0 && count<=8192,"invalid object collision Atomic count");ObjectCollisionGeometry result;result.atomics.resize(count);std::uint64_t total{};
    for(unsigned i=0;i<count;++i){auto& atomic=result.atomics[i];atomic.frame=Word(input);atomic.flags=Word(input);const auto triangles=Word(input);total+=triangles;
        Require(atomic.frame<8192 && total<=1000000,"object collision budget exceeded");atomic.triangles.resize(triangles);
        for(auto& triangle:atomic.triangles){for(auto& point:triangle.points)for(auto& value:point){value=std::bit_cast<float>(Word(input));Finite(value);}
            triangle.atomic=i;triangle.material=Word(input);triangle.triangle=Word(input);Require(triangle.material<65536 && triangle.triangle<1000000,"invalid object collision source index");}
    }
    Require(input.peek()==std::char_traits<char>::eof(),"trailing object collision cache bytes");return result;
}
void ValidateObjectCollision(const ObjectCollisionGeometry& geometry,const Actor& actor){
    Require(actor.render_bindings.has_value(),"object collision requires original Atomic bindings");
    Require(geometry.atomics.size()==actor.atomic_spheres.size() && geometry.atomics.size()==actor.render_bindings->atomics.size(),"object collision Atomic mapping mismatch");
    for(std::size_t i=0;i<geometry.atomics.size();++i){const auto& atomic=geometry.atomics[i];const auto& binding=actor.render_bindings->atomics[i];
        Require(atomic.frame<actor.frames.size() && atomic.frame==actor.atomic_spheres[i].frame && atomic.flags==binding.flags,"object collision source frame/flags mismatch");
        Require(binding.geometry<actor.render_bindings->geometry_materials.size(),"object collision geometry mapping is missing");
        for(const auto& triangle:atomic.triangles)Require(triangle.atomic==i && triangle.material<actor.render_bindings->geometry_materials[binding.geometry].size(),"object collision source material mapping mismatch");
    }
}
std::vector<CollisionTriangle> TransformObjectCollision(const ObjectCollisionAtomic& atomic,std::span<const Matrix> frames,const Matrix& model){
    Require(atomic.frame<frames.size(),"object collision pose frame is missing");Affine(frames[atomic.frame]);Affine(model);
    auto result=atomic.triangles;for(auto& triangle:result)for(auto& p:triangle.points)p=Transform(Transform(p,frames[atomic.frame]),model);return result;
}
}
