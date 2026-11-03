#include "pal4/portable/dff_collision.h"
#include <bit>
#include <iostream>
#include <stdexcept>
using namespace pal4::portable;
namespace {
using Bytes=std::vector<std::uint8_t>;
void Word(Bytes& b,std::uint32_t v){for(unsigned i=0;i<4;++i)b.push_back(static_cast<std::uint8_t>(v>>(i*8)));}
void Float(Bytes& b,float v){Word(b,std::bit_cast<std::uint32_t>(v));}
void Add(Bytes& a,const Bytes& b){a.insert(a.end(),b.begin(),b.end());}
Bytes Chunk(unsigned id,const Bytes& data){Bytes b;Word(b,id);Word(b,static_cast<unsigned>(data.size()));Word(b,0x1803ffff);Add(b,data);return b;}
Bytes Fixture(){
    Bytes material;Word(material,0);Word(material,0xffffffff);Word(material,0);Word(material,0);Float(material,1);Float(material,0);Float(material,1);
    auto mat=Chunk(1,material);Add(mat,Chunk(3,{}));Bytes table;Word(table,1);Word(table,0xffffffff);auto materials=Chunk(1,table);Add(materials,Chunk(7,mat));
    Bytes geometry;for(auto v:{2u,1u,3u,1u})Word(geometry,v);
    // SDK stream ordering: high half of first word is v0, low half is v1;
    // high half of second word is v2, low half is material.
    Word(geometry,1);Word(geometry,2u<<16);
    for(float v:{0.f,0.f,0.f,1.f})Float(geometry,v);Word(geometry,1);Word(geometry,0);
    for(float v:{0.f,0.f,0.f,1.f,0.f,0.f,0.f,1.f,0.f})Float(geometry,v);
    auto geom=Chunk(1,geometry);Add(geom,Chunk(8,materials));Add(geom,Chunk(3,{}));
    Bytes frame;Word(frame,1);for(float v:{1.f,0.f,0.f,0.f,1.f,0.f,0.f,0.f,1.f,0.f,0.f,0.f})Float(frame,v);Word(frame,0xffffffff);Word(frame,0);
    auto frames=Chunk(1,frame);Add(frames,Chunk(3,{}));Bytes count;Word(count,1);auto geometries=Chunk(1,count);Add(geometries,Chunk(0xf,geom));
    Bytes atomic;for(auto v:{0u,0u,4u,0u})Word(atomic,v);auto atom=Chunk(1,atomic);Add(atom,Chunk(3,{}));
    Bytes header;Word(header,1);Word(header,0);Word(header,0);auto clump=Chunk(1,header);Add(clump,Chunk(0xe,frames));Add(clump,Chunk(0x1a,geometries));Add(clump,Chunk(0x14,atom));Add(clump,Chunk(3,{}));return Chunk(0x10,clump);
}
void Check(bool ok,const char* text){if(!ok)throw std::runtime_error(text);}
}
int main(){try{
    const auto bytes=Fixture();const auto scene=ReadDffEnvironment(bytes,{});Check(scene.vertices.size()==3,"synthetic DFF triangle missing");
    const auto& a=scene.vertices[0];const auto& b=scene.vertices[1];const auto& c=scene.vertices[2];
    Check(a.position[0]==0&&a.position[1]==0&&b.position[0]==1&&b.position[1]==0&&c.position[0]==0&&c.position[1]==1,"DFF rendering lost canonical SDK vertex winding");
    const auto grouped=ReadDffEnvironment(bytes,{},true);Check(grouped.atomic_centers.size()==1,"atomic sphere center missing");
    const auto collision=ReadDffCollision(bytes);Check(collision.size()==1&&collision[0].points[0][0]==1&&collision[0].points[1][0]==0,"visual winding fix changed established collision indexing");
    std::cout<<"SDK DFF stream winding, atomic metadata and collision isolation passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
