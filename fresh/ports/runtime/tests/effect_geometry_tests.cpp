#include "pal4/portable/effect_geometry.h"
#include <cmath>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
using namespace pal4::portable;
namespace {
void Check(bool v){if(!v)throw std::runtime_error("effect cylinder geometry contract failed");}
void Near(float a,float b){Check(std::abs(a-b)<.0002f);}
template<class F> void Reject(F f){bool failed=false;try{f();}catch(const std::exception&){failed=true;}Check(failed);}
}
int main(int argc,char** argv){try{
    CylinderShape shape;shape.height=20;shape.top_radius=10;shape.bottom_radius=2;
    Texture texture;texture.width=texture.height=2;texture.rgba.resize(16,255);
    auto mesh=BuildCylinder(shape,texture);Check(mesh.atomics==8 && mesh.source_vertices==32 && mesh.source_triangles==16 && mesh.scene.vertices.size()==48 && mesh.scene.draws.size()==8);
    Near(mesh.scene.minimum[1],0);Near(mesh.scene.maximum[1],20);Near(mesh.scene.minimum[0],-10);Near(mesh.scene.maximum[2],10);
    const auto& first=mesh.scene.vertices.front();Near(first.position[0],std::sqrt(50.f));Near(first.position[2],-std::sqrt(50.f));Near(first.uv0[0],1.125f);Near(first.uv0[1],.25f);
    for(const auto& d:mesh.scene.draws){Check(d.count==6 && d.diffuse==0 && d.lightmap==-1);Near(d.color[3],254.f/255);const auto& v=mesh.scene.vertices;for(unsigned c=0;c<3;++c){Near(v[d.first+1].position[c],v[d.first+3].position[c]);Near(v[d.first+2].position[c],v[d.first+5].position[c]);}}
    for(const auto& v:mesh.scene.vertices)for(float c:v.color)Near(c,1);
    shape.alternate_uv=true;auto alternate=BuildCylinder(shape,texture);Near(alternate.scene.vertices[0].uv0[0],.25f);Near(alternate.scene.vertices[0].uv0[1],1.125f);
    shape.centered=true;auto centered=BuildCylinder(shape);Near(centered.scene.minimum[1],-10);Near(centered.scene.maximum[1],10);Check(centered.scene.draws[0].diffuse==-1);
    shape.bordered=true;shape.sides=3;shape.border_width=1;for(bool flip:{false,true}){shape.alternate_uv=flip;auto bordered=BuildCylinder(shape,texture);Check(bordered.atomics==12 && bordered.source_triangles==24 && bordered.scene.vertices.size()==72);}
    shape.sides=2;Reject([&]{BuildCylinder(shape);});shape.sides=4097;Reject([&]{BuildCylinder(shape);});shape.sides=8;shape.height=std::numeric_limits<float>::infinity();Reject([&]{BuildCylinder(shape);});shape.height=20;texture.width=0;Reject([&]{BuildCylinder(shape,texture);});
    if(argc==2){std::ofstream out(argv[1]);out<<"{\"atomics\":"<<mesh.atomics<<",\"source_vertices\":"<<mesh.source_vertices<<",\"triangles\":"<<mesh.source_triangles<<",\"expanded_vertices\":"<<mesh.scene.vertices.size()<<",\"first_position\":["<<first.position[0]<<','<<first.position[1]<<','<<first.position[2]<<"],\"first_uv\":["<<first.uv0[0]<<','<<first.uv0[1]<<"],\"sdk_numeric_parity_proven\":false,\"chapter_gameplay_validated\":false}";Check(bool(out));}
    std::cout<<"original cylinder plain/bordered/UV/center, material alpha, winding, bounds and budgets passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
