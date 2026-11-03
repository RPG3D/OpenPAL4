#include "pal4/portable/scene_pathways.h"
#include "pal4/portable/package.h"
#include <bit>
#include <cmath>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>

namespace {
using namespace pal4::portable;
void Check(bool value){if(!value)throw std::runtime_error("scene pathways contract failed");}
void Word(std::string& b,std::uint32_t value){for(unsigned n=0;n<32;n+=8)b+=static_cast<char>(value>>n);}
void Text(std::string& b,std::string_view value){Word(b,static_cast<std::uint32_t>(value.size()+1));b+=value;b+='\0';}
void Float(std::string& b,float f){Word(b,std::bit_cast<std::uint32_t>(f));}
template<class F> void Reject(F&& f){bool rejected=false;try{f();}catch(const std::exception&){rejected=true;}Check(rejected);}
std::vector<ScenePath> Parse(const std::string& b){std::istringstream in(b);return ReadScenePaths(in);}
void Near(float actual,float expected){Check(std::isfinite(actual) && std::abs(actual-expected)<1e-4f);}
void Point(const std::array<float,3>& actual,const std::array<float,3>& expected){for(unsigned a=0;a<3;++a)Near(actual[a],expected[a]);}
ScenePath Path(std::uint8_t loop=0){return {"route",7,std::bit_cast<std::uint32_t>(2.f),loop,{{0,0,0},{3,0,0},{3,0,4}}};}
std::string Record(const ScenePath& path){
    std::string b;Text(b,path.name);Word(b,path.grid_marker);
    if(path.grid_marker){Word(b,path.radius_bits);b+=static_cast<char>(path.loop_byte);Word(b,static_cast<std::uint32_t>(path.points.size()));for(const auto& p:path.points)for(float f:p)Float(b,f);}
    return b;
}
void Contracts(){
    const auto path=Path(2);std::string bytes;Word(bytes,2);bytes+=Record(path);bytes+=Record({"absent",0,0,0,{}});
    const auto read=Parse(bytes);Check(read.size()==2 && read[0].grid_marker==7 && read[0].loop_byte==2 && read[0].points==path.points && read[0].radius_bits==path.radius_bits && !read[1].grid_marker);
    for(std::size_t i=0;i<bytes.size();++i)Reject([&]{(void)Parse(bytes.substr(0,i));});
    Reject([&]{(void)Parse(bytes+"x");});
    auto invalid=bytes;invalid[0]=char(0xff);invalid[1]=char(0xff);Reject([&]{(void)Parse(invalid);});
    auto nonfinite=path;nonfinite.points[1][0]=std::numeric_limits<float>::quiet_NaN();std::string bad;Word(bad,1);bad+=Record(nonfinite);Reject([&]{(void)Parse(bad);});
    nonfinite=path;nonfinite.radius_bits=std::bit_cast<std::uint32_t>(std::numeric_limits<float>::infinity());bad.clear();Word(bad,1);bad+=Record(nonfinite);Reject([&]{(void)Parse(bad);});
    std::string huge;Word(huge,1);Text(huge,"bad");Word(huge,1);Float(huge,1);huge+='\0';Word(huge,0xffffffffu);Reject([&]{(void)Parse(huge);});
    std::string empty;Word(empty,0);Check(Parse(empty).empty());
    // Source records preserve active zero-point data; grid construction refuses it.
    const ScenePath no_points{"empty-grid",1,0,0,{}};bad.clear();Word(bad,1);bad+=Record(no_points);const auto zero=Parse(bad);Check(zero.size()==1 && zero[0].points.empty());Reject([&]{ScenePathways owner(1,zero);});
    auto records=std::vector<ScenePath>{Path(),Path(2),{"absent",0,0,0,{}}};records[1].name="loop";
    ScenePathways paths(1,records);Check(paths.Size()==3 && paths.Find("route")->index==0 && !paths.Find("missing"));
    const auto open=paths.Handle(0),loop=paths.Handle(1),absent=paths.Handle(2);
    Check(paths.HasGrid(open) && !paths.HasGrid(absent) && paths.PointCount(open)==3 && paths.PointCount(loop)==4);
    Near(paths.Length(open),7);Near(paths.Length(loop),12);Point(paths.Point(loop,3),{0,0,0});
    Point(paths.CoordinatesAt(open,-1),{0,0,0});Point(paths.CoordinatesAt(open,5),{3,0,2});Point(paths.CoordinatesAt(open,10),{3,0,4});
    Point(paths.CoordinatesAt(loop,9),{1.8f,0,2.4f});Near(paths.ClosestPosition(open,{3,2,2}),5);
    const auto projection=paths.ProjectToTube(open,{3,3,2},{11,12,13});Point(projection.point,{3,0,2});Point(projection.direction,{0,0,1});Near(projection.outside,1);
    Near(paths.ProjectToTube(open,{2,1,0},{0,0,1}).outside,-1);
    records[0].points[0]={99,99,99};Check(paths.Source(open).points[0]==std::array<float,3>{0,0,0});
    Reject([&]{(void)paths.Length({2,0});});Reject([&]{(void)paths.Handle(3);});Reject([&]{(void)paths.Point(open,3);});
    Reject([&]{(void)paths.Length(absent);});Reject([&]{(void)paths.CoordinatesAt(absent,0);});
    Reject([&]{(void)paths.ClosestPosition(open,{std::numeric_limits<float>::infinity(),0,0});});
    Reject([&]{(void)paths.CoordinatesAt(open,std::numeric_limits<float>::quiet_NaN());});
    Reject([&]{ScenePathways invalid_generation(0,records);});
    records={Path(),Path(2)};ScenePathways duplicates(2,records);Check(duplicates.Find("route")->index==0); // original ordered lookup
    ScenePath single{"one",1,std::bit_cast<std::uint32_t>(1.f),0,{{4,5,6}}};ScenePathways one(3,{&single,1});
    const auto singleton=one.Handle(0);Check(one.PointCount(singleton)==1);Near(one.Length(singleton),0);Point(one.CoordinatesAt(singleton,0),{4,5,6});
    const auto singleton_projection=one.ProjectToTube(singleton,{2,0,0},{7,8,9});Point(singleton_projection.point,{0,0,0});Point(singleton_projection.direction,{7,8,9});Near(singleton_projection.outside,1); // original no-segment behavior
}
void Source(const char* archive,const char* member,const char* report){
    std::ifstream file(archive,std::ios::binary);PackageReader package(file);const auto bytes=package.Read(member);
    const auto records=Parse(std::string(bytes.begin(),bytes.end()));ScenePathways paths(1,records);SceneCatalog catalog;catalog.paths=records;
    std::ofstream out(report);out<<"{\"source\":";WriteSceneCatalogReport(out,catalog);out<<",\"queries\":[";
    const auto point=[&](const std::array<float,3>& p){out<<'['<<std::bit_cast<std::uint32_t>(p[0])<<','<<std::bit_cast<std::uint32_t>(p[1])<<','<<std::bit_cast<std::uint32_t>(p[2])<<']';};
    for(std::size_t i=0;i<paths.Size();++i){
        if(i)out<<',';const auto h=paths.Handle(i);out<<"{\"has_grid\":"<<paths.HasGrid(h);
        if(paths.HasGrid(h)){
            const auto length=paths.Length(h);out<<",\"points\":"<<paths.PointCount(h)<<",\"length_bits\":"<<std::bit_cast<std::uint32_t>(length)<<",\"samples\":[";
            for(unsigned j=0;j<5;++j){if(j)out<<',';const auto p=paths.CoordinatesAt(h,length*static_cast<float>(j)/4.f);
                const auto distance=paths.ClosestPosition(h,p);auto query=p;query[1]+=3.f;
                const auto tube=paths.ProjectToTube(h,query,{7,8,9});
                out<<"{\"point_bits\":";point(p);out<<",\"distance_bits\":"<<std::bit_cast<std::uint32_t>(distance)<<",\"projection_bits\":";point(tube.point);
                out<<",\"direction_bits\":";point(tube.direction);out<<",\"outside_bits\":"<<std::bit_cast<std::uint32_t>(tube.outside)<<'}';}
            out<<']';
        }
        out<<'}';
    }
    out<<"],\"monster_paths\":[";
    const auto separator=std::string_view(member).find_last_of('\\');Check(separator!=std::string_view::npos);
    const auto monster_bytes=package.Read(std::string(std::string_view(member).substr(0,separator+1))+"mstInfo.mst");
    std::istringstream monster_input(std::string(monster_bytes.begin(),monster_bytes.end()));const auto monsters=ReadSceneMonsters(monster_input);
    const auto hex=[&](std::string_view value){constexpr char digits[]="0123456789abcdef";out<<'"';for(unsigned char c:value)out<<digits[c>>4]<<digits[c&15];out<<'"';};
    for(std::size_t i=0;i<monsters.size();++i){
        if(i)out<<',';const auto& m=monsters[i];const auto& children=m.behavior.Root().children;std::string name="NOPathWay";
        if(!children.empty())if(const auto* a=children.front().FindAttribute("MSTINFO_Behavior_GroupPathWayID"))
            if(const auto* value=std::get_if<std::string>(&a->value))name=*value;
        const auto found=children.empty()?std::optional<ScenePathHandle>{}:paths.Find(name);
        out<<"{\"monster_hex\":";hex(m.name);out<<",\"path_hex\":";hex(name);out<<",\"config_present\":"<<!children.empty()<<",\"resolved\":"<<bool(found);
        if(found)out<<",\"path_index\":"<<found->index<<",\"has_grid\":"<<paths.HasGrid(*found);
        out<<'}';
    }
    out<<"],\"gameplay_executed\":false}";Check(bool(out));
}
}
int main(int argc,char** argv) try {
    if(argc==5 && std::string_view(argv[1])=="source"){Source(argv[2],argv[3],argv[4]);return 0;}
    Contracts();std::cout<<"original pathway reader, owned grids, lookup, handles and spatial query contracts passed\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
