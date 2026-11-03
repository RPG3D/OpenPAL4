#include "pal4/portable/scene_collision.h"
#include <cmath>
#include <fstream>
#include <iostream>
#include <limits>
#include <source_location>
#include <sstream>
#include <stdexcept>
using namespace pal4::portable;
void Check(bool value,const std::source_location where=std::source_location::current()){if(!value)throw std::runtime_error("scene collision failed at line "+std::to_string(where.line()));}
std::vector<CollisionTriangle> Wall(float z){return {{{Point3{-100,-100,z},Point3{100,-100,z},Point3{-100,100,z}}},{{Point3{100,-100,z},Point3{100,100,z},Point3{-100,100,z}}}};}
bool Near(Point3 a,Point3 b){for(unsigned i=0;i<3;++i)if(std::abs(a[i]-b[i])>1e-4f)return false;return true;}
int main(int argc,char** argv){try{
    if(argc==3){
        std::ifstream input(argv[1],std::ios::binary);const auto mesh=ReadCollision(input);SceneCollision collision;
        // 真实缓存按原atomic分组注册，保留每组顺序。
        for(std::size_t first=0;first<mesh.wall.size();){auto end=first+1;while(end<mesh.wall.size() && mesh.wall[end].atomic==mesh.wall[first].atomic)++end;
            collision.Add(CollisionGroup::wall,std::span(mesh.wall).subspan(first,end-first));first=end;}
        auto upper=std::array<float,4>{-158,30,40,5},lower=std::array<float,4>{-158,15,40,3};Point3 delta{.5f,0,.5f};collision.Resolve(upper,lower,delta);
        std::ofstream output(argv[2]);collision.WriteReport(output);return 0;
    }
    Check(argc==1);
    const auto geometry=Wall(0);auto upper=std::array<float,4>{0,2,-2,1},lower=std::array<float,4>{0,0,-2,1};
    SceneCollision empty;Point3 delta{1,0,1.5};empty.Resolve(upper,lower,delta);Check(Near(delta,{1,0,1.5}));
    SceneCollision wall;wall.Add(CollisionGroup::wall,geometry);delta={1,0,1.5};wall.Resolve(upper,lower,delta);
    Check(Near(delta,{1,0,1})); // 法向修正，切向仍可移动。
    delta={1,0,3};wall.Resolve(upper,lower,delta);Check(Near(delta,{0,0,0})); // 中心线穿墙硬阻挡。
    delta={0,0,-1};wall.Resolve(upper,lower,delta);Check(Near(delta,{0,0,-1}));
    SceneCollision intri;intri.Add(CollisionGroup::intri,geometry);delta={0,0,4};intri.Resolve(upper,lower,delta);Check(Near(delta,{0,0,0}));
    CollisionObjectProperties properties;properties.mode=0;properties.owner=77;properties.plugin=true;properties.callback=123;
    std::vector<std::pair<bool,unsigned>> events;
    SceneCollision objects([&](unsigned callback,bool enter,unsigned previous){Check(callback==123);events.emplace_back(enter,previous);});
    const auto object=objects.Add(CollisionGroup::object,geometry,properties);
    delta={0,0,1.5};objects.Resolve(upper,lower,delta);Check(events==std::vector<std::pair<bool,unsigned>>{{true,0}} && Near(delta,{0,0,1.5}));
    objects.Resolve(upper,lower,delta);Check(events.back()==std::pair<bool,unsigned>{true,1});
    delta={0,0,-1};objects.Resolve(upper,lower,delta);Check(events.back()==std::pair<bool,unsigned>{false,1});
    const auto count=events.size();delta={0,0,1.5};objects.Resolve(upper,lower,delta,77);Check(events.size()==count);
    objects.Replace(object,Wall(50));objects.Resolve(upper,lower,delta);Check(events.size()==count);
    objects.Remove(object);std::ostringstream report;objects.WriteReport(report);Check(report.str().find("\"objects\":0")!=std::string::npos);
    SceneCollision* removing_owner=nullptr;unsigned removing_handle{},removed{};
    SceneCollision removing([&](unsigned,bool,unsigned){removing_owner->Remove(removing_handle);++removed;});removing_owner=&removing;
    removing_handle=removing.Add(CollisionGroup::object,geometry,properties);delta={0,0,1.5};removing.Resolve(upper,lower,delta);Check(removed==1);
    removing.Resolve(upper,lower,delta);Check(removed==1);
    CollisionObjectProperties disabled=properties;disabled.plugin=false;SceneCollision filtered;
    filtered.Add(CollisionGroup::object,geometry,disabled);delta={0,0,1.5};filtered.Resolve(upper,lower,delta);
    disabled=properties;disabled.patch=true;disabled.hierarchy=false;filtered.Add(CollisionGroup::object,geometry,disabled);
    filtered.Resolve(upper,lower,delta); // 未提供事件服务，但不满足原筛选条件的形状不会调用它。
    SceneCollision hard;auto solid=properties;solid.mode=1;solid.hard_block=true;hard.Add(CollisionGroup::object,geometry,solid);
    delta={0,0,3};hard.Resolve(upper,lower,delta);Check(Near(delta,{0,0,0}));
    delta={0,0,1.5};
    SceneCollision no_events;no_events.Add(CollisionGroup::object,geometry,properties);bool rejected=false;
    try{no_events.Resolve(upper,lower,delta);}catch(const std::logic_error&){rejected=true;}Check(rejected);
    // 失败清理查询状态，后续使用不相交位移仍能正常查询。
    delta={0,0,-1};no_events.Resolve(upper,lower,delta);
    lower[3]=-1;rejected=false;try{wall.Resolve(upper,lower,delta);}catch(const std::logic_error&){rejected=true;}Check(rejected);
    std::cout<<"original collision groups, sphere response, slide, owner exclusion and event removal passed\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
