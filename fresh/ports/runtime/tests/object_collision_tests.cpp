#include "pal4/portable/object_collision.h"
#include <bit>
#include <iostream>
#include <sstream>
#include <stdexcept>
using namespace pal4::portable;
void Check(bool v){if(!v)throw std::runtime_error("object collision cache contract failed");}
void Word(std::string& b,unsigned v){for(unsigned i=0;i<32;i+=8)b+=static_cast<char>(v>>i);}
std::string Fixture(){std::string b="P4CG";for(auto v:{1u,1u,0u,1u,1u})Word(b,v);for(float v:{1.f,0.f,0.f,0.f,1.f,0.f,0.f,0.f,1.f})Word(b,std::bit_cast<unsigned>(v));Word(b,0);Word(b,0);return b;}
int main(){try{
    const auto data=Fixture();std::istringstream input(data);const auto geometry=ReadObjectCollision(input);
    Check(geometry.atomics.size()==1 && geometry.atomics[0].flags==1 && geometry.atomics[0].triangles.size()==1);
    for(std::size_t size=0;size<data.size();++size){bool failed=false;try{std::istringstream cut(data.substr(0,size));ReadObjectCollision(cut);}catch(const std::runtime_error&){failed=true;}Check(failed);}
    for(auto [offset,value]:{std::pair{4u,2u},std::pair{8u,0u}}){auto bad=data;bad[offset]=static_cast<char>(value);bool failed=false;try{std::istringstream in(bad);ReadObjectCollision(in);}catch(const std::runtime_error&){failed=true;}Check(failed);}
    auto bad=data+"x";bool failed=false;try{std::istringstream in(bad);ReadObjectCollision(in);}catch(const std::runtime_error&){failed=true;}Check(failed);
    constexpr Matrix identity{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};Actor actor;actor.frames={{-1,identity}};actor.atomic_spheres={{0,{0,0,0,1}}};
    auto& binding=actor.render_bindings.emplace();binding.atomics={{0,1}};binding.geometry_materials={{0}};ValidateObjectCollision(geometry,actor);
    binding.atomics[0].flags=4;failed=false;try{ValidateObjectCollision(geometry,actor);}catch(const std::runtime_error&){failed=true;}Check(failed);binding.atomics[0].flags=1;
    auto frame=identity;frame[12]=10;auto model=identity;model[0]=model[5]=model[10]=2;model[12]=5;
    const std::array<Matrix,1> frames{frame};const auto transformed=TransformObjectCollision(geometry.atomics[0],frames,model);
    Check(transformed[0].points[0]==Point3{27,0,0} && transformed[0].atomic==0 && transformed[0].material==0);
    std::cout<<"complete nonrendering collision transport, mapping and pose transform passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
