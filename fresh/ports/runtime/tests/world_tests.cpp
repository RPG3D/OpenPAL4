#include "pal4/portable/world.h"
#include "pal4/portable/scene_movement.h"
#include "pal4/portable/actor.h"
#include "pal4/floor_color_sample.h"
#include <iomanip>
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace {
using namespace pal4::portable;
void Check(bool x) { if (!x) throw std::runtime_error("world contract failed"); }
template<class T> void Add(std::string& b,const T& x) { b.append(reinterpret_cast<const char*>(&x),sizeof x); }
std::string Fixture() {
    std::string b="P4CL"; Add(b,1u); Add(b,2u); Add(b,1u);
    for (float y:{0.f,10.f}) {
        for (const Point3 p:{Point3{-5,y,-5},{5,y,-5},{0,y,5}}) Add(b,p);
        Add(b,0u); Add(b,2u); Add(b,9u);
    }
    for (const Point3 p:{Point3{2,-5,-5},{2,5,-5},{2,0,5}}) Add(b,p);
    Add(b,1u); Add(b,3u); Add(b,4u); return b;
}
}
int main(int argc,char** argv) {
    try {
        if(argc==4) {
            std::ifstream world(argv[1],std::ios::binary),model(argv[2],std::ios::binary);
            const auto mesh=ReadCollision(world);const auto actor=ReadActor(model);const auto sphere=ActorBindSphere(actor);Check(sphere.has_value());
            const float property=(*sphere)[3]*2,above=property*.35f,below=property*2.53f;
            SceneMovement movement(mesh.floor);Check(movement.SelectAndUpdate({7,7,-22},above,below));Check(movement.CheckActive({7,7,-22},above,below));
            const auto hit=movement.LastHit();Check(hit.has_value());const auto& triangle=mesh.floor[hit->triangle];
            std::ofstream report(argv[3]);report<<std::setprecision(9)<<"{\"radius\":"<<(*sphere)[3]<<",\"above\":"<<above<<",\"below\":"<<below
                <<",\"floor_y\":"<<hit->position[1]<<",\"triangle\":"<<hit->triangle<<",\"source_atomic\":"<<triangle.atomic
                <<",\"source_material\":"<<triangle.material<<",\"source_triangle\":"<<triangle.triangle<<'}';Check(bool(report));return 0;
        }
        if (argc==3) {
            std::ifstream c(argv[1],std::ios::binary),l(argv[2],std::ios::binary);
            const auto mesh=ReadCollision(c); const auto level=ReadLevel(l);
            const auto floor=FindFloor(mesh,level.position,20,20);
            Check(bool(floor));
            std::cout<<"floor="<<mesh.floor.size()<<" wall="<<mesh.wall.size()<<" instance_floor_y="
                <<floor->position[1]<<" source_triangle="<<mesh.floor[floor->triangle].triangle<<'\n'; return 0;
        }
        const auto bytes=Fixture(); std::istringstream s(bytes); const auto mesh=ReadCollision(s);
        Check(!mesh.floor[0].attributes_known && !mesh.floor[0].rgba);
        unsigned fallback=7,alpha=9;
        Check(pal4::UpdateFloorColorSample({0x01000000,0x02000000,0x03000000},0u,fallback,alpha,
            [](auto value){return value;},[&](auto){fallback=23;})==23 && alpha==23);
        std::string v2="P4CL"; Add(v2,2u); Add(v2,2u); Add(v2,1u);
        for(unsigned i=0;i<3;++i) {
            v2.append(bytes,16+i*48,48);
            Add(v2,i==0?1u:0u);
            Add(v2,i==0?std::array<std::uint32_t,3>{0x04030201,0x08070605,0x0c0b0a09}:std::array<std::uint32_t,3>{});
        }
        std::istringstream v2stream(v2);const auto colored=ReadCollision(v2stream);
        Check(colored.floor[0].attributes_known && colored.floor[0].rgba.value()[2]==0x0c0b0a09);
        Check(colored.floor[1].attributes_known && !colored.floor[1].rgba);
        for(std::size_t length=0;length<v2.size();++length) {
            bool rejected=false;try{std::istringstream short_input(v2.substr(0,length));(void)ReadCollision(short_input);}
            catch(const std::runtime_error&){rejected=true;}Check(rejected);
        }
        for(auto [offset,value]:{std::pair{64u,2u},std::pair{132u,1u},std::pair{4u,3u}}) {
            auto broken=v2;broken[offset]=static_cast<char>(value);bool rejected=false;
            try{std::istringstream input(broken);(void)ReadCollision(input);}catch(const std::runtime_error&){rejected=true;}Check(rejected);
        }
        // Check all color/alpha equality patterns, including the EAX return
        // that differs from current_alpha when only vertices 1 and 2 agree.
        for(const auto rgb: {std::array<unsigned,3>{1,1,2},{1,2,2},{1,2,1},{1,2,3},{0,0,2}}) {
            for(const auto alpha: {std::array<unsigned,3>{4,4,8},{4,8,8},{4,8,4},{4,8,12}}) {
                std::array<std::uint32_t,3> raw{};for(unsigned i=0;i<3;++i)raw[i]=(alpha[i]<<24)|rgb[i];
                unsigned conversions=0,selected=99,current_alpha=77;
                const auto result=pal4::UpdateFloorColorSample(raw,0xff000000,55,current_alpha,
                    [&](std::uint32_t rgba){++conversions;Check((rgba>>24)==255);return rgba;},
                    [&](std::uint32_t pixel){Check(conversions==3 && current_alpha==77);selected=pixel;});
                const auto majority=rgb[0]==rgb[1] || rgb[0]==rgb[2]?rgb[0]:rgb[1]==rgb[2]?rgb[1]:0;
                Check(selected==(0xff000000|majority));
                const auto expected=alpha[0]==alpha[1] || alpha[0]==alpha[2]?alpha[0]:alpha[1]==alpha[2]?alpha[1]:55;
                Check(current_alpha==expected);
                Check(result==(alpha[0]!=alpha[1] && alpha[1]==alpha[2]?((raw[1]&0xffffff)<<8)|alpha[0]:expected));
            }
        }
        Check(FindFloor(mesh,{0,9,0},2,20)->position[1]==10);
        Check(FindFloor(mesh,{0,9,0},0,20)->position[1]==0);
        Check(!FindFloor(mesh,{100,9,0},2,20));
        Check(!FindFloor(mesh,{0,9,0},0,2));
        Check(FindFloor(mesh,{0,1,0},20,20)->position[1]==0);
        Check(FindFloorFromAbove(mesh.floor,{0,1,0},20,20)->position[1]==10);
        Check(!FindFloorFromAbove(mesh.floor,{0,0,0},0,0));
        auto secondary=mesh.floor;for(auto& triangle:secondary)for(auto& p:triangle.points)p[1]+=10;
        SceneMovement movement(mesh.floor,secondary);
        Check(movement.ActiveChild()==0 && !movement.LastHit());
        Check(movement.SelectAndUpdate({0,1,0},30,20) && movement.ActiveChild()==1);
        Check(movement.CheckActive({0,1,0},30,20) && movement.LastHit()->position[1]==20);
        Check(movement.SelectAndUpdate({0,1,0},14,20) && movement.ActiveChild()==0 && movement.LastHit()->position[1]==10);
        Point3 adjusted{0,1,0};Check(movement.CheckAndAdjust(adjusted,14,20) && adjusted==Point3{0,10,0});
        adjusted={100,1,0};Check(!movement.CheckAndAdjust(adjusted,14,20) && adjusted==Point3{100,1,0});
        Check(movement.SelectAndUpdate({0,1,0},30,20) && movement.ActiveChild()==1);
        adjusted={0,1,0};Check(movement.CheckAndAdjust(adjusted,30,20) && adjusted==Point3{0,20,0});
        Check(movement.SelectAndUpdate({0,1,0},14,20) && movement.ActiveChild()==0);
        Check(!movement.SelectAndUpdate({100,1,0},30,20) && movement.ActiveChild()==0);
        Check(!movement.CheckActive({100,1,0},30,20) && !movement.LastHit());
        bool invalid_query=false;try{movement.SelectAndUpdate({0,1,0},-1,20);}catch(const std::runtime_error&){invalid_query=true;}Check(invalid_query);
        Check(SegmentBlocked(mesh,{0,0,0},{3,0,0}));
        Check(SegmentBlocked(mesh,{3,0,0},{0,0,0}));
        Check(!SegmentBlocked(mesh,{3,0,0},{4,0,0}));
        Check(!SegmentBlocked(mesh,{0,0,0},{0,0,0}));
        Check(mesh.floor[0].material==2 && mesh.floor[0].triangle==9 && mesh.wall[0].atomic==1);
        for (std::size_t length=0;length<bytes.size();++length) {
            bool failed=false; try { std::istringstream bad(bytes.substr(0,length)); (void)ReadCollision(bad); }
            catch (const std::runtime_error&) { failed=true; } Check(failed);
        }
        const auto m=InstanceMatrix({5,6,7},90); Check(std::abs(m[0]+m[12]-5)<1e-5f && std::abs(m[2]+m[14]-6)<1e-5f);
        std::string level="P4LV"; Add(level,1u); Add(level,3u); level+="N01"; Add(level,9u); level+="actor_101";
        Add(level,Point3{-158,7.3f,40}); Add(level,180.f);
        std::istringstream input(level); Check(ReadLevel(input).world=="N01");
        level+='x'; bool failed=false;
        try { std::istringstream bad(level); (void)ReadLevel(bad); } catch (const std::runtime_error&) { failed=true; } Check(failed);
        std::cout<<"world floor ranges, segment clipping, source IDs, transforms and resource contracts passed\n";
        return 0;
    } catch (const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
}
