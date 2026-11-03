#include "pal4/portable/scene_monster_instances.h"
#include "pal4/portable/package.h"
#include "pal4/portable/arena_source.h"
#include "pal4/portable/monster_scene_resources.h"
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
using namespace pal4::portable;
namespace {
void Check(bool value){if(!value)throw std::runtime_error("monster instance contract failed");}
template<class F>void Reject(F call){bool rejected=false;try{call();}catch(const std::exception&){rejected=true;}Check(rejected);}
Actor Model(){
    Actor a;const Matrix identity{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
    a.frames.push_back({-1,identity});a.palettes.push_back({0,identity});
    a.mesh.vertices.resize(3);a.mesh.vertices[0].position[0]=1;
    a.influences.resize(3,{{0,0,0,0},{1,0,0,0}});a.atomic_spheres.push_back({0,{0,0,0,2}});
    a.clips.push_back({"Z01",2,{{0,{{0,{0,0,0,1},{0,0,0}},{2,{0,0,0,1},{2,0,0}}}}}});
    return a;
}
std::vector<SceneMonster> Records(){
    SceneMonster first;first.name="zeta";first.resource="401";first.position={1,2,3};first.fields[0]=7;
    first.values={2501,2502};first.fields[4]=2;auto second=first;second.name="second";second.position={4,5,6};
    return {first,second};
}
}
int main(int argc,char** argv){try{
    if(argc==6 && std::string_view(argv[1])=="stage"){
        auto source=std::make_shared<ArenaSource>(argv[2]);const std::string world(argv[3]),scene(argv[4]);
        const auto base=world+"\\"+scene+"\\";
        auto monster_bytes=source->ReadSceneMember(base+"mstInfo.mst");auto path_bytes=source->ReadSceneMember(base+"pathway.pwy");
        Check(monster_bytes && path_bytes);
        std::istringstream monster_input(std::string(monster_bytes->begin(),monster_bytes->end()));
        std::istringstream path_input(std::string(path_bytes->begin(),path_bytes->end()));
        const auto records=ReadSceneMonsters(monster_input);const auto paths=ReadScenePaths(path_input);
        std::weak_ptr<ArenaSource> lifetime=source;
        {
            MonsterSceneResources stage(17,source,records,paths);source.reset();Check(!lifetime.expired());
            auto& instances=stage.Monsters();Check(!records.empty() && instances.Size()==records.size() && stage.Paths()->Size()==paths.size());
            for(std::size_t i=0;i<instances.Size();++i){const auto handle=instances.Handle(i);Check(instances.SelectAction(handle,"Z01"));instances.AdvanceAnimation(handle,.125f);std::vector<Vertex> pose;instances.Sample(handle,pose);Check(!pose.empty());}
            std::ofstream output(argv[5]);instances.WriteReport(output);Check(bool(output));
        }
        Check(lifetime.expired());return 0;
    }
    if(argc==6 && std::string_view(argv[1])=="original"){
        ArenaSource source(argv[2]);const std::string world(argv[3]),scene(argv[4]);
        const auto bytes=source.ReadSceneMember(world+"\\"+scene+"\\mstInfo.mst");Check(bytes.has_value());
        std::istringstream input(std::string(bytes->begin(),bytes->end()));const auto records=ReadSceneMonsters(input);
        unsigned loads{};SceneMonsterInstances instances(17,records,[&](std::string_view model){
            ++loads;const auto name=std::string(model);return source.ReadOriginalActorResource("gamedata\\PALActor\\"+name+"\\",name,{},true);
        },[&](std::string_view member)->std::optional<std::vector<std::uint8_t>>{
            const std::string path(member);const auto separator=path.find_last_of("/\\");Check(separator!=path.npos);
            return source.ReadObjectMember("gamedata\\PALActor\\"+path.substr(0,separator)+"\\",path.substr(separator+1));
        });
        Check(instances.Size()==records.size() && loads==instances.ModelCount() && !records.empty());
        std::ostringstream before;source.WriteReport(before);Check(before.str().find(".anm")==std::string::npos);
        ArenaSource reference_source(argv[2]);
        for(std::size_t i=0;i<instances.Size();++i){const auto handle=instances.Handle(i);Check(instances.Find(records[i].name)==handle);
            Check(instances.SelectAction(handle,"Z01"));instances.AdvanceAnimation(handle,.125f);std::vector<Vertex> pose;instances.Sample(handle,pose);Check(!pose.empty());
            const auto& name=records[i].resource;auto eager=reference_source.ReadOriginalActorResource("gamedata\\PALActor\\"+name+"\\",name);
            ActorPlayback reference(eager,name,[](auto)->std::optional<std::vector<std::uint8_t>>{return {};});
            Check(reference.Select("Z01"));reference.Advance(.125f);std::vector<Vertex> expected;reference.Sample(expected);Check(pose.size()==expected.size());
            for(std::size_t vertex=0;vertex<pose.size();++vertex)for(unsigned axis=0;axis<3;++axis)
                Check(std::isfinite(pose[vertex].position[axis]) && pose[vertex].position[axis]==expected[vertex].position[axis]);}
        std::ostringstream after;source.WriteReport(after);Check(after.str().find(".anm")!=std::string::npos);
        std::ofstream out(argv[5]);instances.WriteReport(out);Check(bool(out));return 0;
    }
    if(argc==7 && std::string_view(argv[1])=="source"){
        const std::filesystem::path game(argv[2]),cache(argv[3]);const std::string world(argv[4]),scene(argv[5]);
        std::ifstream source(game/"scenedata.cpk",std::ios::binary);PackageReader scenes(source);
        const auto bytes=scenes.Read(world+"\\"+scene+"\\mstInfo.mst");
        std::istringstream input(std::string(reinterpret_cast<const char*>(bytes.data()),bytes.size()));
        const auto records=ReadSceneMonsters(input);
        std::ifstream actor_source(game/"palactor.cpk",std::ios::binary);PackageReader actors(actor_source);
        unsigned loads{};
        SceneMonsterInstances instances(17,records,[&](std::string_view model){
            ++loads;std::ifstream file(cache/"arena"/world/scene/"monsters"/(std::string(model)+".p4a"),std::ios::binary);return ReadActor(file);
        },[&](std::string_view member)->std::optional<std::vector<std::uint8_t>>{
            if(!actors.Find(member))return {};return actors.Read(member);
        });
        Check(instances.Size()==records.size() && loads==instances.ModelCount());
        for(std::size_t i=0;i<instances.Size();++i){
            const auto handle=instances.Handle(i);Check(instances.Find(records[i].name)==handle);
            Check(instances.Source(handle).position==records[i].position && instances.Source(handle).fields==records[i].fields);
            Check(instances.LocalBox(handle).has_value()==instances.Model(handle).bind_vertices.has_value());
            Check(instances.SelectAction(handle,"Z01"));instances.AdvanceAnimation(handle,.125f);
            std::vector<Vertex> pose;instances.Sample(handle,pose);Check(!pose.empty());
            for(const auto& vertex:pose)for(auto value:vertex.position)Check(std::isfinite(value));
        }
        std::ofstream out(argv[6]);instances.WriteReport(out);return 0;
    }
    Reject([&]{MonsterSceneResources invalid(1,{},Records(),{});});
    auto records=Records();unsigned loads{};
    const auto load=[&](std::string_view id){Check(id=="401");++loads;return Model();};
    const ActorMemberReader read=[](std::string_view)->std::optional<std::vector<std::uint8_t>>{return {};};
    SceneMonsterInstances instances(1,records,load,read);
    Check(instances.Size()==2 && instances.ModelCount()==1 && loads==1);
    const auto first=instances.Handle(0),second=instances.Handle(1);
    Check(instances.OriginalTraversalOrder()==std::vector<std::size_t>({1,0}));
    {
        auto byte_records=records;
        byte_records[0].name=std::string(1,static_cast<char>(0x80));
        byte_records[1].name="a";
        auto upper=byte_records[1];upper.name="A";byte_records.push_back(upper);
        auto longer=byte_records[1];longer.name="aa";byte_records.push_back(longer);
        SceneMonsterInstances byte_order(2,byte_records,load,read);
        Check(byte_order.OriginalTraversalOrder()==std::vector<std::size_t>({2,1,3,0}));
    }
    Check(&instances.Model(first)==&instances.Model(second) && instances.LocalSphere(first)[3]==2);
    Check(instances.Action(first).empty() && instances.AnimationTime(first)==0);
    records[0].position={99,99,99};records[0].values.clear();
    Check(instances.Source(first).position==std::array<float,3>{1,2,3} && instances.Source(first).values.size()==2);
    const auto transform=instances.InstanceTransform(first);Check(transform[12]==1 && transform[13]==2 && transform[14]==3);
    instances.SetMotionPosition(first,{8,9,10});instances.SetMotionRotation(first,{11,12,13});
    Check(instances.InstanceTransform(first)[13]==9 && instances.MotionTransform(first).rotation==std::array<float,3>{11,12,13});
    Check(instances.Source(first).position==std::array<float,3>{1,2,3} && instances.InstanceTransform(second)[13]==5);
    Reject([&]{instances.SetMotionPosition(first,{0,std::numeric_limits<float>::quiet_NaN(),0});});
    Check(instances.MotionTransform(first).position==std::array<float,3>{8,9,10});
    unsigned commits{};std::uint64_t binding{};bool throws{},unrelated{},reenter{};
    const auto initial=instances.MotionTransform(first);
    binding=instances.BindTransform(first,initial,[&](auto& value,auto change){
        ++commits;
        if(reenter){Reject([&]{instances.SetMotionPosition(first,{1,1,1});});Reject([&]{instances.UnbindTransform(first,binding);});}
        if(throws)throw std::runtime_error("resource commit failure");
        if(unrelated)value.position[0]+=1;
        if(change==MonsterTransformChange::Rotation)value.rotation[1]=90;
    });
    Check(instances.TransformBound(first) && instances.OwnsTransformBinding(first,binding) && !instances.TransformBound(second));
    Reject([&]{instances.BindTransform(first,initial,[](auto&,auto){});});
    Reject([&]{instances.UnbindTransform(first,binding+1);});
    Reject([&]{instances.UnbindTransform({2,0},binding);});
    reenter=true;instances.SetMotionRotation(first,{11,450,13});reenter=false;
    Check(commits==1 && instances.MotionTransform(first).rotation[1]==90);
    const auto saved=instances.MotionTransform(first);throws=true;
    Reject([&]{instances.SetMotionPosition(first,{20,30,40});});throws=false;
    Check(instances.MotionTransform(first).position==saved.position);
    unrelated=true;Reject([&]{instances.SetMotionRotation(first,{1,2,3});});unrelated=false;
    Check(instances.MotionTransform(first).rotation==saved.rotation && instances.MotionTransform(first).position==saved.position);
    const auto other=instances.BindTransform(second,instances.MotionTransform(second),[](auto&,auto){});
    Check(other!=binding);Reject([&]{instances.UnbindTransform(second,binding);});instances.UnbindTransform(second,other);
    instances.UnbindTransform(first,binding);Check(!instances.TransformBound(first));
    instances.SetMotionPosition(first,{8,9,10});Check(instances.MotionTransform(first).position==initial.position);
    Check(instances.SelectAction(first,"Z01") && instances.SelectAction(second,"Z01"));
    instances.AdvanceAnimation(first,.5f);Check(instances.AnimationTime(first)==.5f && instances.AnimationTime(second)==0);
    std::vector<Vertex> a,b;instances.Sample(first,a);instances.Sample(second,b);
    Check(std::abs(a[0].position[0]-1.5f)<.0001f && b[0].position[0]==1);
    Check(instances.Source(first).fields[0]==7 && !instances.Find("unknown"));
    SceneMonsterInstances replacement(2,Records(),load,read);
    Reject([&]{replacement.Sample(first,a);});Reject([&]{instances.Handle(2);});
    auto invalid=Records();invalid[1].name=invalid[0].name;const auto before=loads;
    Reject([&]{SceneMonsterInstances duplicate(3,invalid,load,read);});Check(loads==before);
    invalid=Records();invalid[0].resource="../401";Reject([&]{SceneMonsterInstances bad(3,invalid,load,read);});
    Reject([&]{SceneMonsterInstances bad(0,Records(),load,read);});
    Reject([&]{SceneMonsterInstances bad(3,Records(),[](auto){return Actor{};},read);});
    std::cout<<"shared immutable models, copied records, independent playback and scene handle contracts passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
