#include "pal4/portable/monster_path_restore.h"
#include "pal4/portable/monster_random_state.h"
#include "pal4/portable/scene_pathways.h"
#include "pal4/portable/monster_movement_world.h"
#include "pal4/portable/monster_movement.h"
#include "pal4/portable/monster_orientation.h"
#include "pal4/portable/monster_path_save.h"
#include "pal4/portable/monster_movement_behavior.h"
#include "pal4/portable/binary_xml_memory.h"
#include "pal4/portable/package.h"
#include "pal4/portable/arena_source.h"
#include "pal4/portable/world.h"
#include <bit>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <limits>
#include <functional>
#include <sstream>
#include <source_location>
#include <stdexcept>
using namespace pal4;
using namespace pal4::portable;
namespace {
void Check(bool v,const std::source_location where=std::source_location::current()){if(!v)throw std::runtime_error("monster path restore contract failed at line "+std::to_string(where.line()));}
template<class F>void Reject(F f,const std::source_location where=std::source_location::current()){bool rejected=false;try{f();}catch(const std::exception&){rejected=true;}Check(rejected,where);}
void Near(float a,float b){Check(std::isfinite(a) && std::abs(a-b)<1e-4f);}
float Value(const BehaviorPathEndpointState& s,std::size_t w){return std::bit_cast<float>(s.words[w]);}
BinaryXmlAttribute Int(std::string name,int value){return {std::move(name),1,std::int32_t(value)};}
BinaryXmlAttribute Float(std::string name,float value){return {std::move(name),2,value};}
BinaryXmlAttribute Text(std::string name,std::string value){return {std::move(name),3,std::move(value)};}
BinaryXmlConfig Doc(const BinaryXmlNode& group){BinaryXmlNode root;root.name="config";root.children={group};const auto bytes=BinaryXmlConfig::Serialize(root);Check(bool(bytes));BinaryXmlConfig d;Check(d.Parse(*bytes));return d;}
BinaryXmlConfig Behavior(){
    BinaryXmlNode group;group.name="group";group.attributes={Text("MSTINFO_Behavior_GroupPathWayID","route")};
    for(int kind=0;kind<2;++kind){BinaryXmlNode choice;choice.name="choice";choice.attributes={Int("MSTINFO_Behavior_ObjectType",kind)};
        BinaryXmlNode parameters;parameters.name="AAB_MST_parameters";parameters.attributes={Float(kind==0?"AABPedestrian_MST_walk_max_speed":"AABPedestrian_MST_run_max_speed",kind==0?6.f:12.f),Float(kind==0?"AABPedestrian_MST_walk_max_force":"AABPedestrian_MST_run_max_force",kind==0?3.f:5.f),Int("AABPedestrian_MST_walk_pathfollow",1)};
        choice.children.push_back(parameters);group.children.push_back(choice);}
    return Doc(group);
}
Actor Model(bool c02=true,bool c03=true,bool fallback=true){
    Actor a;Matrix identity{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};a.frames.push_back({-1,identity});a.palettes.push_back({0,identity});a.mesh.vertices.resize(3);a.mesh.vertices[0].position[0]=1;
    a.influences.resize(3,{{0,0,0,0},{1,0,0,0}});
    for(const auto& [name,enabled]:{std::pair{"C02",c02},std::pair{"C03",c03},std::pair{"Z02",fallback}})
        if(enabled)a.clips.push_back({name,2,{{0,{{0,{0,0,0,1},{0,0,0}},{2,{0,0,0,1},{2,0,0}}}}}});
    return a;
}
ActorMemberReader EmptyMessages(){return [](std::string_view)->std::optional<std::vector<std::uint8_t>>{return {};};}
// Explicit contract world, not a production collision/neighbor fallback.
class World final:public IMonsterPathRestoreWorld {
    ScenePathways& paths_;std::optional<ScenePathHandle> current_;
public:
    std::vector<std::uint32_t> draws{16384,16384,24575,100};std::size_t draw_count{},binds{},updates{},adds{},removes{},releases{};
    bool fail_bind{},fail_update{},fail_add{},bound{},registered{};BehaviorVector3 last_position{};
    std::function<void()> in_bind;
    explicit World(ScenePathways& p):paths_(p){}
    std::uint32_t ReadWord(std::uint32_t address) override {
        if(address==0x700){Check(bound);return 0x84b6c0;}
        Check(current_.has_value());
        if(address==0x80008)return paths_.PointCount(*current_);
        if(address==0x8000c)return 0x100000;
        Check(address>=0x100000 && (address-0x100000)%4==0);const auto w=(address-0x100000)/4;
        return std::bit_cast<std::uint32_t>(paths_.Point(*current_,w/3)[w%3]);
    }
    void Bind(BehaviorPathEndpointState& helper,std::uint32_t wrapper) override {
        Check(wrapper==0x900);++binds;
        helper.words[42]=0;bound=false;
        if(fail_bind)throw std::runtime_error("contract bind failure");
        if(in_bind)in_bind();helper.words[42]=0x700;bound=true;
    }
    std::uint32_t UpdatePosition(std::uint32_t association,const BehaviorVector3& position) override {
        Check(association==0x700 && bound);++updates;if(fail_update)throw std::runtime_error("contract update failure");last_position=position;return 77;
    }
    std::uint32_t Rand() override {return draws.at(draw_count++);}
    std::uint32_t FindPath(std::string_view name) override {current_=paths_.Find(name);return current_ && paths_.HasGrid(*current_)?0x80000:0;}
    std::uint32_t GridParameter() override {return 0x900;}
    void AddElement(BehaviorPathEndpointState& helper) override {
        Check(bound && !registered);++adds;if(fail_add)throw std::runtime_error("contract insertion failure");registered=true;
        last_position=BehaviorHelperConfiguration::ReadVector(helper,0x18);
    }
    void RemoveElement(BehaviorPathEndpointState&) noexcept override {++removes;registered=false;}
    void ReleaseAssociation(BehaviorPathEndpointState& helper) noexcept override {++releases;bound=false;helper.words[42]=0;}
};
std::vector<ScenePath> Paths(){return {{"route",1,std::bit_cast<std::uint32_t>(2.f),0,{{1,2,3},{4,5,6},{7,8,9}}}};}
void CoreContracts(){
    const auto records=Paths();ScenePathways paths(1,records);
    BehaviorPathEndpointState helper{};BehaviorHelperConfiguration::ConstructEndpoint(helper,37);Check(helper.words[22]==37 && helper.words[0]==0x842898);Near(Value(helper,23),1);Near(Value(helper,24),.5f);
    World world(paths);BehaviorHelperConfiguration core(world);Check(world.FindPath("route")==0x80000);
    const auto result=core.Configure(helper,0x900,0x80000);Check(result==77 && world.draw_count==4 && world.binds==1 && world.updates==1);
    for(unsigned i=0;i<3;++i){Near(Value(helper,50+i),records[0].points.front()[i]);Near(Value(helper,53+i),records[0].points.back()[i]);}
    Check(helper.words[48]==1);Near(Value(helper,5),1);Near(Value(helper,25),0);Near(Value(helper,26),.1f);Near(Value(helper,27),1);
    const auto cached=helper;world.draw_count=0;world.draws.back()=32767;Check(core.Configure(helper,0x900,0)==77 && helper.words[47]==0 && helper.words[48]==0xffffffffu);
    for(unsigned i=50;i<56;++i)Check(helper.words[i]==cached.words[i]); // null path preserves endpoints
    world.draw_count=0;world.draws={32767,32767,32767,16384,16384,24575,100};Check(core.Configure(helper,0x900,0)==77 && world.draw_count==7);
    for(auto type:{0x842978u,0x842a30u,0x842ae8u}){
        BehaviorPathEndpointState other{};other.words.fill(0x12345678);other.words[0]=type;other.words[42]=0;World base_world(paths);BehaviorHelperConfiguration base_core(base_world);
        Check(base_core.Configure(other,0x900,0xabcdef)==77 && base_world.draw_count==0 && base_world.binds==1 && base_world.updates==1);
        Check(other.words[47]==(type==0x842978u?0xabcdefu:0x12345678u));for(unsigned i=48;i<56;++i)Check(other.words[i]==0x12345678);
        Near(Value(other,23),1);Near(Value(other,24),.5f);Near(Value(other,25),0);
    }
    auto invalid=helper;invalid.words[0]=99;const auto before=invalid.words;Reject([&]{(void)core.Configure(invalid,0x900,0);});Check(invalid.words==before);
    BehaviorPathEndpointState reentrant{};BehaviorHelperConfiguration::ConstructEndpoint(reentrant,3);World nested(paths);BehaviorHelperConfiguration nested_core(nested);
    nested.in_bind=[&]{(void)nested_core.Configure(reentrant,0x900,0);};Reject([&]{(void)nested_core.Configure(reentrant,0x900,0);});nested.in_bind={};Check(nested_core.Configure(reentrant,0x900,0)==77);
}
void RestoreContracts(){
    const auto records=Paths();ScenePathways paths(1,records);auto behavior=Behavior();
    const MonsterTransformRows rows{{{0,0,-1},{0,1,0},{1,0,0},{11,12,13}}};
    for(int kind=0;kind<2;++kind){
        auto model=Model();ActorPlayback playback(model,"401",EmptyMessages());BinaryXmlNode memory;memory.attributes={Float("AAB_MST_memory_speed",2.5f),Int("AABPedestrian_MST_memory_pathDirection",-1),Int("AABPedestrian_MST_memory_wander",1),Int("AAB_MST_memory_last_behavior_id",kind),Float("AAB_MST_memory_actiontime",.25f)};
        MonsterPathRestoreInput input{behavior,memory,rows,4};BehaviorPathEndpointState helper{};BehaviorHelperConfiguration::ConstructEndpoint(helper,3);World world(paths);
        Check(RestoreMonsterPath(kind,helper,input,playback,world));Check(world.registered && world.bound && world.adds==1 && world.draw_count==4);
        Near(Value(helper,27),kind==0?6.f:12.f);Near(Value(helper,26),kind==0?3.f:5.f);Near(Value(helper,24),6);Near(Value(helper,25),2.5f);
        Check(helper.words[48]==0xffffffffu && (helper.words[49]&0xffff)==0x101 && (helper.words[46]&0xff)==1);
        for(unsigned axis=0;axis<3;++axis){Near(Value(helper,1+axis),rows[0][axis]);Near(Value(helper,4+axis),rows[1][axis]);Near(Value(helper,7+axis),rows[2][axis]);Near(Value(helper,10+axis),rows[3][axis]);}
        Check(playback.Action()==(kind==0?"C02":"C03"));Near(playback.Time(),.25f);playback.Advance(2);Check(playback.Time()<2); // restart set by original Accept
        world.RemoveElement(helper);world.ReleaseAssociation(helper);
    }
    for(bool matching:{false,true}){
        auto model=Model(false,false);ActorPlayback playback(model,"401",EmptyMessages());BinaryXmlNode memory;memory.attributes={Int("AAB_MST_memory_last_behavior_id",matching?1:0),Float("AAB_MST_memory_actiontime",.5f)};
        MonsterPathRestoreInput input{behavior,memory,rows,4};World world(paths);BehaviorPathEndpointState helper{};BehaviorHelperConfiguration::ConstructEndpoint(helper,3);
        Check(RestoreMonsterPath(1,helper,input,playback,world));Check(playback.Action()=="Z02");Near(playback.Time(),matching?.5f:0.f);
    }
    for(int failure=0;failure<5;++failure){
        auto model=Model();ActorPlayback playback(model,"401",EmptyMessages());BinaryXmlNode memory;
        if(failure==4)memory.attributes={Int("AAB_MST_memory_last_behavior_id",0),Float("AAB_MST_memory_actiontime",std::numeric_limits<float>::infinity())};
        MonsterPathRestoreInput input{behavior,memory,rows,4};World world(paths);world.fail_bind=failure==0;world.fail_update=failure==1;world.fail_add=failure==2;if(failure==3)world.draws={32768};
        BehaviorPathEndpointState helper{};BehaviorHelperConfiguration::ConstructEndpoint(helper,3);const auto before=helper.words;
        Reject([&]{(void)RestoreMonsterPath(0,helper,input,playback,world);});Check(helper.words==before && !world.bound && !world.registered && world.releases==1);
        Check(world.removes==(failure==4?1u:0u));
    }
    auto model=Model();ActorPlayback playback(model,"401",EmptyMessages());BinaryXmlConfig absent;BinaryXmlNode memory;MonsterPathRestoreInput input{absent,memory,rows,4};
    World world(paths);BehaviorPathEndpointState helper{};BehaviorHelperConfiguration::ConstructEndpoint(helper,3);
    Check(!RestoreMonsterPath(0,helper,input,playback,world));Check(world.bound && !world.registered && playback.Action().empty() && world.draw_count==4);world.ReleaseAssociation(helper);
}
MonsterMovementWorld::Random RandomStream(){
    return [draw=std::size_t{}]()mutable {constexpr std::uint32_t samples[]{16384,16384,24575,100};return samples[draw++%4];};
}
void MovementWorldContracts(){
    auto paths=std::make_shared<ScenePathways>(9,Paths());MonsterMovementWorld world(9,paths,RandomStream());
    const auto baseline=world.Stats();Check(baseline.helpers==0 && baseline.spatial_nodes==0 && baseline.members==0);
    const auto grid=world.ReadWord(world.GridParameter()+4);Check(world.ReadWord(grid+24)==20 && world.ReadWord(grid+28)==1 && world.ReadWord(grid+32)==20);
    const auto path=world.FindPath("route");Check(path && world.PathHandle(path).index==0 && world.ReadWord(path+8)==3);
    const std::array<float,3> query{4,3,7},direction{0,0,1};
    const auto path_handle=world.PathHandle(path);
    Near(world.PathClosest(path,query),paths->ClosestPosition(path_handle,query));
    for(float distance:{-2.f,0.f,3.f,100.f}){
        const auto actual=world.PathCoordinates(path,distance),expected=paths->CoordinatesAt(path_handle,distance);
        for(unsigned axis=0;axis<3;++axis)Near(actual[axis],expected[axis]);
    }
    const auto projected=world.PathProject(path,query,direction),expected_projection=paths->ProjectToTube(path_handle,query,direction);
    Near(projected.outside,expected_projection.outside);
    for(unsigned axis=0;axis<3;++axis){Near(projected.point[axis],expected_projection.point[axis]);Near(projected.direction[axis],expected_projection.direction[axis]);}
    const auto before_queries=world.Stats();
    Reject([&]{(void)world.PathClosest(path+0x54,query);});
    Reject([&]{(void)world.PathCoordinates(0,1);});
    Reject([&]{(void)world.PathCoordinates(path,std::numeric_limits<float>::infinity());});
    Reject([&]{(void)world.PathProject(path,{0,std::numeric_limits<float>::quiet_NaN(),0},direction);});
    Check(world.Stats().mapped_bytes==before_queries.mapped_bytes);
    Near(world.PathClosest(path,query),paths->ClosestPosition(path_handle,query)); // Exception releases the non-reentrant guard.
    const auto positions=world.ReadWord(path+12);Near(std::bit_cast<float>(world.ReadWord(positions)),1);
    const auto a=world.CreateEndpoint({9,10},1),b=world.CreateEndpoint({9,11},2),c=world.CreateEndpoint({9,12},3);
    BehaviorHelperConfiguration configuration(world);
    const auto place=[&](MonsterHelperHandle h,const BehaviorVector3& point){
        auto& image=world.Helper(h);(void)configuration.Configure(image,world.GridParameter(),path);
        BehaviorHelperConfiguration::SetVector(image,0x1c,point);world.AddElement(image);
        // AddElement does not invent an extra position callback after Accept's transform write.
        const auto at_origin=world.QuerySphere({0,0,0},.001f);Check(!at_origin.empty());
        (void)world.UpdatePosition(image.words[42],point);
    };
    place(a,{1,1,1});place(b,{2,1,1});place(c,{3,1,1});
    auto near=world.QuerySphere({1,1,1},2);Check(near.size()==2 && near[0].helper==b && near[1].helper==a);
    Check(near[0].owner.index==11 && near[0].member && near[0].distance_squared==1);
    const auto configured=world.Stats();Check(configured.helpers==3 && configured.members==3 && configured.associations==3 && configured.spatial_nodes==3);
    auto& rebind=world.Helper(b);const auto old_association=rebind.words[42];(void)configuration.Configure(rebind,world.GridParameter(),path);
    Check(world.Stats().associations==3 && world.Stats().spatial_nodes==3);Reject([&]{(void)world.ReadWord(old_association);});
    world.DestroyHelper(b);Reject([&]{(void)world.Helper(b);});Check(world.Stats().helpers==2 && world.Stats().members==2 && world.Stats().spatial_nodes==2);
    auto& compensate=world.Helper(a);world.RemoveElement(compensate);Check(world.Stats().members==1 && world.Stats().helpers==2 && world.Stats().spatial_nodes==2);
    world.ReleaseAssociation(compensate);Check(world.Stats().spatial_nodes==1);Check(world.Helper(a).words[42]==0);world.DestroyHelper(a);
    Reject([&]{(void)world.Helper({8,c.token});});Reject([&]{(void)world.ReadWord(world.GridParameter()+1);});
    Reject([&]{(void)world.UpdatePosition(0x123,{1,2,3});});Reject([&]{(void)world.QuerySphere({0,0,0},-1);});
    Reject([&]{(void)world.QuerySphere({0,0,0},std::numeric_limits<float>::infinity());});
    world.Close();const auto closed=world.Stats();Check(closed.closed && closed.blocks==0 && closed.mapped_bytes==0 && closed.helpers==0);world.Close();
    Reject([&]{(void)world.Helper(c);});Reject([&]{(void)world.QuerySphere({0,0,0},1);});
    Reject([&]{(void)world.PathClosest(path,query);});Reject([&]{(void)world.PathCoordinates(path,0);});
    Reject([&]{(void)world.PathProject(path,query,direction);});
    auto no_paths=std::make_shared<ScenePathways>(10,std::span<const ScenePath>{});MonsterMovementWorld edges(10,no_paths,RandomStream());BehaviorHelperConfiguration edge_config(edges);
    const auto outside=edges.CreateEndpoint({10,0},1),inside=edges.CreateEndpoint({10,1},2);
    for(const auto& pair:{std::pair{outside,BehaviorVector3{-40.01f,0,0}},std::pair{inside,BehaviorVector3{-39.95f,0,0}}}){
        auto& image=edges.Helper(pair.first);(void)edge_config.Configure(image,edges.GridParameter(),0);edges.AddElement(image);(void)edges.UpdatePosition(image.words[42],pair.second);
    }
    // Preserve original truncation quirk: shallow negative overlap does not visit overflow.
    near=edges.QuerySphere({-39.95f,0,0},.1f);Check(near.size()==1 && near[0].helper==inside);
    near=edges.QuerySphere({-40,0,0},5);Check(near.size()==2 && near[0].helper==outside && near[1].helper==inside);
    Check(edges.QuerySphere({-39.95f,0,0},0).empty());
    auto stale=edges.Helper(inside).words[42];edges.DestroyHelper(inside);Reject([&]{(void)edges.UpdatePosition(stale,{0,0,0});});edges.Close();
    Reject([&]{MonsterMovementWorld invalid(11,no_paths,RandomStream());});Reject([&]{MonsterMovementWorld invalid(10,no_paths,{});});
    MonsterMovementWorld limited(10,no_paths,RandomStream(),{1,64u*1024*1024});const auto only=limited.CreateEndpoint({10,0},1);const auto before_limit=limited.Stats();
    Reject([&]{(void)limited.CreateEndpoint({10,1},2);});Check(limited.Stats().mapped_bytes==before_limit.mapped_bytes);limited.DestroyHelper(only);limited.Close();
    MonsterMovementWorld measure(10,no_paths,RandomStream());const auto base=measure.Stats().mapped_bytes;measure.Close();
    MonsterMovementWorld constrained(10,no_paths,RandomStream(),{2,base+sizeof(BehaviorPathEndpointState)+36});
    const auto handle=constrained.CreateEndpoint({10,0},1);auto& helper=constrained.Helper(handle);const auto before=helper.words;
    auto model=Model();ActorPlayback playback(model,"401",EmptyMessages());const auto behavior=Behavior();BinaryXmlNode memory;
    const MonsterTransformRows rows{{{1,0,0},{0,1,0},{0,0,1},{3,4,5}}};const MonsterPathRestoreInput input{behavior,memory,rows,2};
    Reject([&]{(void)RestoreMonsterPath(0,helper,input,playback,constrained);});
    Check(helper.words==before && constrained.Stats().helpers==1 && constrained.Stats().associations==0 && constrained.Stats().members==0 && constrained.Stats().mapped_bytes==base+sizeof(BehaviorPathEndpointState));
    constrained.DestroyHelper(handle);constrained.Close();
    MonsterMovementWorld* self{};
    MonsterMovementWorld reentrant(10,no_paths,[&]{(void)self->QuerySphere({0,0,0},1);return 0u;});self=&reentrant;
    const auto h=reentrant.CreateEndpoint({10,0},1);auto& image=reentrant.Helper(h);const auto old=image.words;
    Reject([&]{(void)RestoreMonsterPath(0,image,input,playback,reentrant);});Check(image.words==old && reentrant.Stats().associations==0);reentrant.Close();
}
void RandomWorldContracts(){
    auto paths=std::make_shared<ScenePathways>(17,std::span<const ScenePath>{});
    auto stream=RandomStream();std::size_t draws{};
    MonsterMovementWorld world(17,paths,[&]{++draws;return stream();});
    const auto path=world.CreateEndpoint({17,0},1);Check(draws==0);
    const auto random=world.CreateRandom({17,1},2);Check(draws>=3);
    auto& image=world.Helper(random);
    Check(image.words[0]==0x842ae8 && image.words[47]==0 && image.words[48]==0x40000000 && image.words[49]==0);
    Check(image.words[51]==0 && image.words[53]==0 && image.words[54]==0 && image.words[55]==0);
    const float x=std::bit_cast<float>(image.words[50]),z=std::bit_cast<float>(image.words[52]);Near(x*x+z*z,1);
    BehaviorHelperConfiguration configuration(world);
    const auto random_tail=image.words;
    for(const auto h:{path,random}){
        auto& helper=world.Helper(h);(void)configuration.Configure(helper,world.GridParameter(),0);world.AddElement(helper);
        (void)world.UpdatePosition(helper.words[42],{1,0,1});
    }
    for(std::size_t i=47;i<56;++i)Check(image.words[i]==random_tail[i]);
    const auto neighbors=world.QuerySphere({1,0,1},1);Check(neighbors.size()==2 && neighbors[0].helper==random && neighbors[1].helper==path);
    std::vector<MonsterHelperHandle> order;
    world.UpdateMembers(.125f,[&](auto h,float seconds){
        Check(seconds==.125f);order.push_back(h);Reject([&]{(void)world.CreateRandom({17,2},3);});
    });Check(order==std::vector<MonsterHelperHandle>{path,random});
    // Compensation keeps the random image and its derived state alive.
    world.RemoveElement(image);world.ReleaseAssociation(image);
    for(std::size_t i=47;i<56;++i)Check(image.words[i]==random_tail[i]);
    Check(world.Contains(random) && !world.Registered(random));world.DestroyHelper(random);Check(!world.Contains(random));
    const auto remaining=world.CreateRandom({17,2},3);auto& listed=world.Helper(remaining);
    (void)configuration.Configure(listed,world.GridParameter(),0);world.AddElement(listed);
    world.Close();Check(world.Stats().blocks==0);Reject([&]{(void)world.CreateRandom({17,3},4);});
    // Constructor exceptions must not publish tokens, associations or images.
    bool fail=true;MonsterMovementWorld throwing(17,paths,[&]()->std::uint32_t{if(fail)throw std::runtime_error("rng failure");return 16384;});
    const auto before=throwing.Stats();Reject([&]{(void)throwing.CreateRandom({17,0},1);});
    Check(throwing.Stats().mapped_bytes==before.mapped_bytes && throwing.Stats().helpers==0);
    fail=false;const auto retry=throwing.CreateRandom({17,0},1);throwing.DestroyHelper(retry);throwing.Close();
    MonsterMovementWorld invalid(17,paths,[]{return 32768u;});Reject([&]{(void)invalid.CreateRandom({17,0},1);});Check(invalid.Stats().helpers==0);invalid.Close();
    MonsterMovementWorld* self{};MonsterMovementWorld reentrant(17,paths,[&]{(void)self->CreateRandom({17,0},1);return 0u;});self=&reentrant;
    Reject([&]{(void)reentrant.CreateRandom({17,0},1);});Check(reentrant.Stats().helpers==0);reentrant.Close();
}
BinaryXmlConfig RandomBehavior(){
    BinaryXmlNode parameters;parameters.name="AAB_MST_parameters";
    parameters.attributes={Float("AABWander_MST_max_speed",7.f),Float("AABWander_MST_max_force",4.f),Float("AABWander_MST_max_range",9.f)};
    BinaryXmlNode choice;choice.name="choice";choice.attributes={Int("MSTINFO_Behavior_ObjectType",3)};choice.children.push_back(parameters);
    BinaryXmlNode group;group.name="group";group.attributes={Text("MSTINFO_Behavior_GroupPathWayID","route")};group.children.push_back(choice);return Doc(group);
}
void RandomStateContracts(){
    auto paths=std::make_shared<ScenePathways>(1,Paths());const auto behavior=RandomBehavior();
    const MonsterTransformRows rows{{{0,0,-1},{0,1,0},{1,0,0},{11,12,13}}};
    for(int saved=0;saved<3;++saved)for(int idle=0;idle<3;++idle){
        MonsterMovementWorld world(1,paths,RandomStream());auto model=Model(idle!=1,false,true);
        if(idle<2){auto clip=model.clips.front();clip.name=idle==0?"C01":"Z01";model.clips.push_back(clip);}
        ActorPlayback playback(model,"401",EmptyMessages());BinaryXmlNode memory;memory.name="actual-memory";
        memory.attributes={Text("keep","untouched"),Int("AAB_MST_memory_last_behavior_id",saved?3:0),Float("AAB_MST_memory_speed",2.25f),Float("AAB_MST_memory_actiontime",.25f),Float("AABWander_MST_memory_start_x",4.f),Float("AABWander_MST_memory_start_y",5.f),Float("AABWander_MST_memory_start_z",saved==2?std::numeric_limits<float>::max():6.f)};
        memory.children.push_back({"untouched",{}, {}});
        const MonsterPathRestoreInput input{behavior,memory,rows,4};const auto h=world.CreateRandom({1,0},7);auto& helper=world.Helper(h);
        // Invalid saved XYZ preserves constructor target; new behavior uses actor position.
        Check(RestoreMonsterRandom(helper,input,playback,world));Check(world.Registered(h));
        Near(Value(helper,27),7);Near(Value(helper,26),4);Near(Value(helper,24),6);Near(Value(helper,25),2.25f);Near(Value(helper,47),9);
        for(unsigned a=0;a<3;++a){Near(Value(helper,53+a),saved==0?rows[3][a]:saved==1?4.f+a:0.f);Near(Value(helper,10+a),rows[3][a]);}
        Check(playback.Action()==(idle==1?"Z02":"C02"));Near(playback.Time(),saved?.25f:0.f);
        Reject([&]{(void)RestoreMonsterPath(0,helper,input,playback,world);});
        playback.Seek(1.25f);BehaviorHelperConfiguration::SetScalar(helper,0x68,3.5f);
        const std::array<float,3> start{21,22,23};for(unsigned a=0;a<3;++a)helper.words[53+a]=std::bit_cast<std::uint32_t>(start[a]);
        if(idle==1)helper.words[46]&=~0xffu; // Save accepts registered paused helpers.
        const auto association=helper.words[42];const auto selected=SaveMonsterRandom(h,memory,playback,world);
        Check(selected==(idle<2?1u:0u) && !world.Contains(h));Reject([&]{(void)world.ReadWord(association);});
        Check(memory.attributes.size()==7 && memory.children.size()==1 && std::get<std::string>(memory.FindAttribute("keep")->value)=="untouched");
        Check(memory.FindAttribute("AAB_MST_memory_last_behavior_id")->ToInt(99)==3);Near(memory.FindAttribute("AAB_MST_memory_speed")->ToFloat(0),3.5f);Near(memory.FindAttribute("AAB_MST_memory_actiontime")->ToFloat(0),1.25f);
        for(unsigned a=0;a<3;++a){const std::string key=std::string("AABWander_MST_memory_start_")+"xyz"[a];Near(memory.FindAttribute(key)->ToFloat(0),start[a]);}
        if(idle<2){playback.Advance(.5f);Check(playback.Action()==(idle==0?"C01":"Z01"));}
        const auto next=world.CreateRandom({1,0},8);Check(RestoreMonsterRandom(world.Helper(next),input,playback,world));
        Near(playback.Time(),1.25f);for(unsigned a=0;a<3;++a)Near(Value(world.Helper(next),53+a),start[a]);
        world.Close();Check(world.Stats().blocks==0);
    }
    // Exceptions before registration and after animation selection compensate ownership.
    struct Draw final:IGridRandomRuntime {std::int32_t Rand() override {return 16384;}} draw;
    for(int fail=0;fail<5;++fail){
        World world(*paths);world.fail_bind=fail==0;world.fail_update=fail==1;world.fail_add=fail==2;
        auto model=Model();ActorPlayback playback(model,"401",EmptyMessages());BinaryXmlNode memory;
        if(fail>=3)memory.attributes={Int("AAB_MST_memory_last_behavior_id",3),Float(fail==3?"AAB_MST_memory_speed":"AAB_MST_memory_actiontime",std::numeric_limits<float>::infinity())};
        BehaviorPathEndpointState helper{};BehaviorHelperConfiguration::ConstructRandom(helper,1,draw);const auto before=helper.words;
        const MonsterPathRestoreInput input{behavior,memory,rows,4};Reject([&]{(void)RestoreMonsterRandom(helper,input,playback,world);});
        Check(helper.words==before && !world.bound && !world.registered && world.releases==1 && world.removes==(fail==4?1u:0u));
    }
    MonsterMovementWorld world(1,paths,RandomStream());auto model=Model();ActorPlayback playback(model,"401",EmptyMessages());BinaryXmlNode memory;
    const MonsterPathRestoreInput input{behavior,memory,rows,4};const auto endpoint=world.CreateEndpoint({1,0},1);
    Reject([&]{(void)RestoreMonsterRandom(world.Helper(endpoint),input,playback,world);});Reject([&]{(void)SaveMonsterRandom(endpoint,memory,playback,world);});world.DestroyHelper(endpoint);
    const auto unlisted=world.CreateRandom({1,0},2);Reject([&]{(void)SaveMonsterRandom(unlisted,memory,playback,world);});Check(memory.attributes.empty());world.DestroyHelper(unlisted);
    const auto bad=world.CreateRandom({1,0},3);Check(RestoreMonsterRandom(world.Helper(bad),input,playback,world));
    memory.attributes.push_back({"AAB_MST_memory_speed",2,std::int32_t{3}});Reject([&]{(void)SaveMonsterRandom(bad,memory,playback,world);});Check(world.Registered(bad));world.DestroyHelper(bad);
    memory={};auto clip=model.clips.front();clip.name="C01";model.clips.push_back(clip);bool fail=false;
    ActorPlayback rejecting(model,"401",[&](std::string_view member)->std::optional<std::vector<std::uint8_t>>{if(fail && member.find("C01")!=std::string_view::npos)throw std::runtime_error("idle failure");return {};});
    const auto last=world.CreateRandom({1,0},4);Check(RestoreMonsterRandom(world.Helper(last),input,rejecting,world));fail=true;
    Reject([&]{(void)SaveMonsterRandom(last,memory,rejecting,world);});Check(!world.Contains(last) && memory.attributes.size()==6);world.Close();
}
void MemoryWriterContracts(){
    for(std::uint32_t type=0;type<5;++type){
        BinaryXmlAttribute::Value initial;
        switch(type){case 0:initial=std::int8_t{-1};break;case 1:initial=std::int32_t{-9};break;case 2:initial=-9.f;break;case 3:initial=std::string(48,'a');break;default:initial=std::vector<std::uint8_t>{8,9};}
        BinaryXmlNode memory;memory.name="actual-memory";memory.attributes.push_back({"value",type,initial});
        WriteBinaryXmlInt(memory,"value",257);Check(memory.attributes.size()==1 && memory.attributes[0].type==type);
        const auto& v=memory.attributes[0];if(type==0)Check(std::get<std::int8_t>(v.value)==1);else if(type==3)Check(std::get<std::string>(v.value)=="257");else Check(v.ToInt(0)==257);
        WriteBinaryXmlFloat(memory,"value",3.75f);Check(memory.attributes[0].type==type);
        if(type==0 || type==1)Check(v.ToInt(0)==3);else if(type==2)Near(std::get<float>(v.value),3.75f);else if(type==3)Check(std::get<std::string>(v.value)=="3.750000");
        else {Check(std::get<std::vector<std::uint8_t>>(v.value)==std::vector<std::uint8_t>({0,0,0x70,0x40}));Near(v.ToFloat(-1),3.75f);}
        const auto bytes=BinaryXmlConfig::Serialize(memory);Check(bool(bytes));BinaryXmlConfig parsed;Check(parsed.Parse(*bytes) && parsed.Root().attributes[0].type==type);
    }
    BinaryXmlNode memory;memory.attributes={Int("duplicate",1),Int("duplicate",99),Text("keep","untouched")};
    WriteBinaryXmlInt(memory,"duplicate",42);Check(memory.attributes[0].ToInt(0)==42 && memory.attributes[1].ToInt(0)==99);
    WriteBinaryXmlFloat(memory,"new-float",1.25f);WriteBinaryXmlInt(memory,"new-int",-7);Check(memory.FindAttribute("new-float")->type==2 && memory.FindAttribute("new-int")->type==1);
    WriteBinaryXmlFloat(memory,"duplicate",4294967296.f);Check(memory.attributes[0].ToInt(1)==0); // Original signed64 ftol low32.
    const auto before=memory.attributes.size();Reject([&]{WriteBinaryXmlInt(memory,std::string_view("a\0b",3),1);});Check(memory.attributes.size()==before);
    memory.attributes.push_back({"bad",2,std::int32_t{7}});Reject([&]{WriteBinaryXmlInt(memory,"bad",2);});Check(std::get<std::int32_t>(memory.attributes.back().value)==7);
    BinaryXmlAttribute raw{"raw",4,std::vector<std::uint8_t>{}};Check(raw.ToFloat(1)==1);
    for(unsigned size=1;size<=5;++size){std::vector<std::uint8_t> bytes(size,0);bytes[0]=1;raw.value=bytes;
        const auto expected=size<3?0x3f800001u:size==3?0x3f000001u:1u;Check(std::bit_cast<std::uint32_t>(raw.ToFloat(1))==expected);}
}
void SaveContracts(){
    for(int kind=0;kind<2;++kind)for(int idle=0;idle<3;++idle){
        auto paths=std::make_shared<ScenePathways>(1,Paths());MonsterMovementWorld world(1,paths,RandomStream());
        auto model=Model();if(idle<2){auto clip=model.clips.front();clip.name=idle==0?"C01":"Z01";model.clips.push_back(clip);}
        ActorPlayback playback(model,"401",EmptyMessages());const auto behavior=Behavior();BinaryXmlNode memory;memory.name="actual-memory";memory.children.push_back({"untouched",{}, {}});memory.attributes.push_back(Text("unrelated","keep"));
        const MonsterTransformRows rows{{{1,0,0},{0,1,0},{0,0,1},{3,4,5}}};
        const auto helper=world.CreateEndpoint({1,0},3);MonsterPathRestoreInput input{behavior,memory,rows,2};Check(RestoreMonsterPath(kind,world.Helper(helper),input,playback,world));
        if(idle==1)memory.attributes.push_back({"AAB_MST_memory_speed",4,std::vector<std::uint8_t>{0,0,0,0}});
        playback.Seek(1.25f);auto& image=world.Helper(helper);BehaviorHelperConfiguration::SetScalar(image,0x68,3.25f);image.words[48]=0xffffffffu;image.words[49]=(image.words[49]&~0xffu)|0x7fu;
        if(idle==1)image.words[46]&=~0xffu; // Still listed; original Finish saves paused helpers too.
        const auto association=image.words[42];const auto result=SaveMonsterPath(kind,helper,memory,playback,world);
        Check(result==(kind==1 || idle<2?1u:0u));Reject([&]{(void)world.Helper(helper);});Reject([&]{(void)world.ReadWord(association);});
        Check(world.Stats().helpers==0 && world.Stats().members==0 && world.Stats().spatial_nodes==0 && world.Stats().associations==0);
        Check(memory.attributes.size()==6 && memory.children.size()==1 && std::get<std::string>(memory.FindAttribute("unrelated")->value)=="keep");
        Check(memory.FindAttribute("AAB_MST_memory_last_behavior_id")->ToInt(99)==std::uint32_t(kind));Near(memory.FindAttribute("AAB_MST_memory_speed")->ToFloat(0),3.25f);
        Near(memory.FindAttribute("AAB_MST_memory_actiontime")->ToFloat(0),1.25f);Check(memory.FindAttribute("AABPedestrian_MST_memory_pathDirection")->ToInt(0)==0xffffffffu && memory.FindAttribute("AABPedestrian_MST_memory_wander")->ToInt(0)==127);
        if(idle<2){playback.Advance(.5f);Check(playback.Action()==(idle==0?"C01":"Z01"));}
        // Same actual memory node feeds the original Accept on a new helper.
        const auto next=world.CreateEndpoint({1,0},4);Check(RestoreMonsterPath(kind,world.Helper(next),input,playback,world));
        Near(playback.Time(),1.25f);Near(Value(world.Helper(next),25),3.25f);
        // Release saves the unsigned byte; original Accept restores a >0 flag.
        Check(world.Helper(next).words[48]==0xffffffffu && (world.Helper(next).words[49]&0xff)==1);
        Reject([&]{(void)SaveMonsterPath(3,next,memory,playback,world);});
        (void)SaveMonsterPath(kind,next,memory,playback,world);if(idle<2)playback.Advance(.5f);
        const auto cross=world.CreateEndpoint({1,0},5);Check(RestoreMonsterPath(1-kind,world.Helper(cross),input,playback,world));
        Near(playback.Time(),0);Near(Value(world.Helper(cross),25),3.25f); // Different behavior ID does not restore old clip time.
        world.DestroyHelper(cross);world.Close();Check(world.Stats().blocks==0);
    }
    auto paths=std::make_shared<ScenePathways>(1,Paths());MonsterMovementWorld world(1,paths,RandomStream());auto model=Model();ActorPlayback playback(model,"401",EmptyMessages());BinaryXmlNode memory;
    const auto unlisted=world.CreateEndpoint({1,0},1);Reject([&]{(void)SaveMonsterPath(0,unlisted,memory,playback,world);});Check(memory.attributes.empty());world.DestroyHelper(unlisted);
    const auto behavior=Behavior();const MonsterTransformRows rows{{{1,0,0},{0,1,0},{0,0,1},{3,4,5}}};
    const auto helper=world.CreateEndpoint({1,0},2);MonsterPathRestoreInput input{behavior,memory,rows,2};Check(RestoreMonsterPath(0,world.Helper(helper),input,playback,world));
    memory.attributes.push_back({"AAB_MST_memory_speed",2,std::int32_t{3}});
    Reject([&]{(void)SaveMonsterPath(0,helper,memory,playback,world);});Check(world.Registered(helper) && memory.FindAttribute("AAB_MST_memory_last_behavior_id"));world.DestroyHelper(helper);
    auto clip=model.clips.front();clip.name="C01";model.clips.push_back(clip);bool fail=false;
    ActorPlayback rejecting(model,"401",[&](std::string_view member)->std::optional<std::vector<std::uint8_t>>{if(fail && member.find("C01")!=std::string_view::npos)throw std::runtime_error("controlled idle resource failure");return {};});
    memory={};const auto last=world.CreateEndpoint({1,0},3);Check(RestoreMonsterPath(0,world.Helper(last),input,rejecting,world));fail=true;
    Reject([&]{(void)SaveMonsterPath(0,last,memory,rejecting,world);});Check(world.Stats().helpers==0 && memory.attributes.size()==5);world.Close();
}
struct SourceData {
    std::filesystem::path game,cache;std::string world,scene;
    std::unique_ptr<ArenaSource> original;
    std::ifstream scene_file,actor_file;PackageReader scenes,actors;
    std::vector<SceneMonster> monsters;std::shared_ptr<ScenePathways> paths;std::map<std::string,Actor> models;
    SourceData(const char* g,const char* c,const char* w,const char* s)
        :game(g),cache(c),world(w),scene(s),scene_file(game/"scenedata.cpk",std::ios::binary),actor_file(game/"palactor.cpk",std::ios::binary),scenes(scene_file),actors(actor_file){
        if(std::string_view(c)=="--original")original=std::make_unique<ArenaSource>(game);
        auto p=SceneStream("pathway.pwy"),m=SceneStream("mstInfo.mst");monsters=ReadSceneMonsters(m);paths=std::make_shared<ScenePathways>(1,ReadScenePaths(p));
    }
    std::istringstream SceneStream(std::string_view name){const auto b=scenes.Read(world+"\\"+scene+"\\"+std::string(name));return std::istringstream(std::string(b.begin(),b.end()));}
    const Actor& ModelFor(std::string_view resource){
        const std::string key(resource);if(!models.contains(key)){if(original)models.emplace(key,original->ReadOriginalActorResource("gamedata\\PALActor\\"+key+"\\",key));else{std::ifstream input(cache/"arena"/world/scene/"monsters"/(key+".p4a"),std::ios::binary);models.emplace(key,ReadActor(input));}}return models.at(key);
    }
    CollisionMesh Collision(){if(original)return original->ReadOriginalCollision(world,scene);std::ifstream input(cache/"arena"/world/scene/"collision.p4c",std::ios::binary);return ReadCollision(input);}
    std::unique_ptr<ActorPlayback> Playback(const SceneMonster& record){return std::make_unique<ActorPlayback>(ModelFor(record.resource),record.resource,[this](std::string_view member)->std::optional<std::vector<std::uint8_t>>{if(!actors.Find(member))return {};return actors.Read(member);});}
    MonsterTransformRows Transform(const SceneMonster& record){const auto matrix=InstanceMatrix(record.position,record.rotation);MonsterTransformRows rows{};for(unsigned r=0;r<4;++r)for(unsigned a=0;a<3;++a)rows[r][a]=matrix[r*4+a];return rows;}
};
std::vector<std::int32_t> PathKinds(const SceneMonster& record,bool include_random=false){
    std::vector<std::int32_t> result;if(record.behavior.Root().children.empty())return result;
    for(const auto& child:record.behavior.Root().children.front().children){const auto* a=child.FindAttribute("MSTINFO_Behavior_ObjectType");if(!a)continue;const auto kind=std::bit_cast<std::int32_t>(a->ToInt(0xffffffffu));if(kind==0 || kind==1 || (include_random && kind==3))result.push_back(kind);}
    return result;
}
void WriteWords(std::ostream& out,const BehaviorPathEndpointState& helper){out<<'[';for(unsigned i=0;i<helper.words.size();++i){if(i)out<<',';out<<helper.words[i];}out<<']';}
std::size_t CheckPose(ActorPlayback& playback){std::vector<Vertex> pose;playback.Sample(pose);Check(!pose.empty());for(const auto& v:pose)for(float f:v.position)Check(std::isfinite(f));return pose.size();}
void Source(const char* game,const char* cache,const char* world_name,const char* scene,const char* report){
    SourceData data(game,cache,world_name,scene);std::ofstream out(report);out<<"{\"contract_world_only\":true,\"controlled_memory\":true,\"gameplay_executed\":false,\"restores\":[";bool first=true;
    for(const auto& record:data.monsters)for(const auto kind:PathKinds(record)){
        auto playback=data.Playback(record);const auto sphere=ActorBindSphere(data.ModelFor(record.resource));Check(bool(sphere));BinaryXmlNode memory;
        MonsterPathRestoreInput input{record.behavior,memory,data.Transform(record),(*sphere)[3]};World fixture(*data.paths);BehaviorPathEndpointState helper{};BehaviorHelperConfiguration::ConstructEndpoint(helper,3);
        Check(RestoreMonsterPath(kind,helper,input,*playback,fixture));const auto vertices=CheckPose(*playback);
        if(!first)out<<',';first=false;out<<"{\"monster\":"<<std::quoted(record.name)<<",\"kind\":"<<kind<<",\"action\":"<<std::quoted(std::string(playback->Action()))<<",\"random_draws\":"<<fixture.draw_count<<",\"helper_words\":";WriteWords(out,helper);out<<",\"vertices\":"<<vertices<<'}';
        fixture.RemoveElement(helper);fixture.ReleaseAssociation(helper);
    }
    out<<"]}";Check(bool(out));
}
void TaskSource(const char* game,const char* cache,const char* world_name,const char* scene,const char* report,bool scheduled=false){
    SourceData data(game,cache,world_name,scene);
    SceneMonsterInstances instances(1,data.monsters,[&](std::string_view id){return data.ModelFor(id);},[&](std::string_view member)->std::optional<std::vector<std::uint8_t>>{if(!data.actors.Find(member))return {};return data.actors.Read(member);});
    MonsterMovementWorld world(1,data.paths,RandomStream());MonsterMovement movement(world);
    const auto collision=data.Collision();Check(!collision.floor.empty());
    SceneMovement floor(collision.floor);MonsterOrientationCaches caches;MonsterOrientation orientation(world,instances,floor,caches);
    std::vector<BinaryXmlNode> memory(instances.Size());std::uint32_t next_id=3;std::vector<std::size_t> selected,unsupported;std::vector<std::unique_ptr<MonsterMovementBehavior>> sessions;
    for(std::size_t i=0;i<instances.Size();++i){const auto actor=instances.Handle(i);const auto& definition=instances.Behavior(actor);if(!definition.fuzzy)continue;
        auto owner=std::make_unique<MonsterMovementBehavior>(world,instances,actor,memory[i],orientation,[&,actor]{return instances.LocalSphere(actor)[3];},[&]{return next_id++;});
        const bool supported=std::all_of(definition.fuzzy->choices.begin(),definition.fuzzy->choices.end(),[](const auto& c){return c.class_id==0 || c.class_id==1 || c.class_id==3;});
        if(!supported){Reject([&]{(void)owner->Start();});Check(owner->Failed() && owner->TaskCount()==0);unsupported.push_back(i);continue;}
        if(scheduled)owner->SetSchedulingEnabled(true);else Check(owner->Start());selected.push_back(i);sessions.push_back(std::move(owner));
    }
    Check(!sessions.empty());std::ofstream out(report);out<<"{\"gameplay_executed\":false,\"whole_scene_accepted\":false,\"movement_fuzzy_subset\":true,\"controlled_actor_order\":true,\"controlled_animation_phase\":true,\"controlled_manager_radius\":true,\"controlled_memory\":true,\"controlled_random\":true,\"static_obstacles_supplied\":false,\"unsupported_fuzzy\":[";
    for(std::size_t j=0;j<unsupported.size();++j){if(j)out<<',';const auto i=unsupported[j];out<<"{\"name\":"<<std::quoted(data.monsters[i].name)<<",\"rejected\":true,\"definition\":";WriteMonsterBehaviorDefinition(out,instances.Behavior(instances.Handle(i)));out<<'}';}
    out<<"],\"samples\":[";bool first=true;unsigned frames{};
    for(;frames<(scheduled?18000u:6000u);++frames){
        world.UpdateMembers(1.f/60,[&](auto h,float elapsed){Check(movement.Update(h,{},elapsed).active);});
        for(const auto i:selected){const auto h=instances.Handle(i);if(!instances.Action(h).empty())instances.AdvanceAnimation(h,1.f/60);}
        for(auto& session:sessions){
            if(scheduled){(void)session->AdvanceScheduledAfterGrid(1.f/60);if(session->Stats().saved>=3 && session->Ready() && session->SchedulingEnabled())session->SetSchedulingEnabled(false);}
            else session->AdvanceAfterGrid(1.f/60);
        }
        const bool ready=std::all_of(sessions.begin(),sessions.end(),[scheduled](const auto& s){return s->Ready() && (!scheduled || s->Stats().saved==3);});
        if(frames%30==0 || ready){if(!first)out<<',';first=false;out<<"{\"frame\":"<<frames<<",\"members\":"<<world.Stats().members<<",\"actors\":[";
            for(std::size_t j=0;j<selected.size();++j){if(j)out<<',';const auto h=instances.Handle(selected[j]);const auto transform=instances.MotionTransform(h);const auto matrix=instances.InstanceTransform(h);Check(matrix[12]==transform.position[0] && matrix[13]==transform.position[1] && matrix[14]==transform.position[2]);
                Check(instances.Source(h).position==data.monsters[selected[j]].position);out<<"{\"index\":"<<selected[j]<<",\"action\":"<<std::quoted(std::string(instances.Action(h)))<<",\"time_bits\":"<<std::bit_cast<std::uint32_t>(instances.AnimationTime(h))<<",\"transform_bits\":[";bool axis_first=true;
                for(const auto& v:{transform.position,transform.rotation})for(const auto value:v){Check(std::isfinite(value));if(!axis_first)out<<',';axis_first=false;out<<std::bit_cast<std::uint32_t>(value);}out<<"],\"queue\":";sessions[j]->WriteReport(out);out<<'}';}
            out<<"]}";
        }
        if(ready){++frames;break;}
    }
    Check(frames<(scheduled?18000u:6000u) && world.Stats().helpers==0 && world.Stats().members==0 && world.Stats().associations==0);
    out<<"],\"frames\":"<<frames<<",\"completed\":[";
    for(std::size_t j=0;j<selected.size();++j){const auto i=selected[j];const auto h=instances.Handle(i);const auto stats=sessions[j]->Stats();Check(sessions[j]->Ready() && !sessions[j]->Failed() && stats.live_actions==0 && stats.saved>0);
        instances.AdvanceAnimation(h,.5f);std::vector<Vertex> vertices;instances.Sample(h,vertices);Check(!vertices.empty());for(const auto& v:vertices)for(float f:v.position)Check(std::isfinite(f));
        const auto bytes=BinaryXmlConfig::Serialize(memory[i]);Check(bool(bytes));if(j)out<<',';out<<"{\"name\":"<<std::quoted(data.monsters[i].name)<<",\"idle\":"<<std::quoted(std::string(instances.Action(h)))<<",\"definition\":";WriteMonsterBehaviorDefinition(out,instances.Behavior(h));out<<",\"queue\":";sessions[j]->WriteReport(out);out<<",\"memory_hex\":\"";
        for(const auto byte:*bytes){constexpr char hex[]="0123456789abcdef";out<<hex[byte>>4]<<hex[byte&15];}out<<"\"}";
    }
    sessions.clear();world.Close();Check(world.Stats().blocks==0);out<<"],\"closed_blocks\":0}";Check(bool(out));
}
void RandomCycleSource(const char* game,const char* cache,const char* world_name,const char* scene,const char* report){
    SourceData data(game,cache,world_name,scene);MonsterMovementWorld world(1,data.paths,RandomStream());
    std::ofstream out(report);Check(bool(out));out<<"{\"gameplay_executed\":false,\"whole_scene_accepted\":false,\"controlled_memory_random_radius_seek\":true,\"movement_executed\":false,\"cycles\":[";std::size_t count{};
    for(std::size_t i=0;i<data.monsters.size();++i){const auto& record=data.monsters[i];if(record.behavior.Root().children.empty())continue;
        bool random=false;for(const auto& child:record.behavior.Root().children.front().children){const auto* a=child.FindAttribute("MSTINFO_Behavior_ObjectType");if(a && a->ToInt(99)==3)random=true;}if(!random)continue;
        auto playback=data.Playback(record);const auto sphere=ActorBindSphere(data.ModelFor(record.resource));Check(bool(sphere));BinaryXmlNode memory;memory.name="controlled-actual-memory";
        const auto rows=data.Transform(record);const MonsterPathRestoreInput input{record.behavior,memory,rows,(*sphere)[3]};
        const auto h=world.CreateRandom({1,static_cast<std::uint32_t>(i)},static_cast<std::uint32_t>(3+count*2));Check(RestoreMonsterRandom(world.Helper(h),input,*playback,world));
        for(unsigned a=0;a<3;++a)Near(Value(world.Helper(h),53+a),rows[3][a]);
        const auto before=world.Helper(h);const std::string moving(playback->Action());const float time=std::min(.125f,playback->Duration()*.5f);playback->Seek(time);
        const auto selected=SaveMonsterRandom(h,memory,*playback,world);Check(!world.Contains(h) && world.Stats().members==0 && world.Stats().associations==0);
        playback->Advance(.5f);const std::string idle(playback->Action());const auto bytes=BinaryXmlConfig::Serialize(memory);Check(bool(bytes));BinaryXmlConfig parsed;Check(parsed.Parse(*bytes));
        Check(parsed.Root().FindAttribute("AAB_MST_memory_last_behavior_id")->ToInt(99)==3);Near(parsed.Root().FindAttribute("AAB_MST_memory_actiontime")->ToFloat(-1),time);
        const auto next=world.CreateRandom({1,static_cast<std::uint32_t>(i)},static_cast<std::uint32_t>(4+count*2));Check(RestoreMonsterRandom(world.Helper(next),input,*playback,world));
        const auto after=world.Helper(next);for(unsigned a=0;a<3;++a)Check(before.words[53+a]==after.words[53+a]);Check(playback->Action()==moving);Near(playback->Time(),time);Near(Value(after,25),Value(before,25));
        if(count++)out<<',';out<<"{\"record\":"<<i<<",\"moving\":\""<<moving<<"\",\"idle\":\""<<idle<<"\",\"selected\":"<<selected<<",\"time_bits\":"<<std::bit_cast<std::uint32_t>(time)<<",\"before\":";WriteWords(out,before);out<<",\"after\":";WriteWords(out,after);
        out<<",\"memory_bytes\":[";for(std::size_t b=0;b<bytes->size();++b){if(b)out<<',';out<<unsigned((*bytes)[b]);}out<<"],\"pose_vertices\":"<<CheckPose(*playback)<<'}';world.DestroyHelper(next);
    }
    Check(count>0);world.Close();Check(world.Stats().blocks==0);out<<"],\"count\":"<<count<<",\"closed_blocks\":0}";Check(bool(out));
}
void CycleSource(const char* game,const char* cache,const char* world_name,const char* scene,const char* report){
    SourceData data(game,cache,world_name,scene);MonsterMovementWorld world(1,data.paths,RandomStream());MonsterMovement movement(world);
    std::ofstream out(report);out<<"{\"gameplay_executed\":false,\"controlled_memory\":true,\"controlled_random\":true,\"controlled_animation_seek\":true,\"cycles\":[";std::size_t count{};
    for(std::size_t i=0;i<data.monsters.size();++i){const auto& record=data.monsters[i];for(const auto kind:PathKinds(record)){
        auto playback=data.Playback(record);const auto sphere=ActorBindSphere(data.ModelFor(record.resource));Check(bool(sphere));BinaryXmlNode memory;memory.name="controlled-actual-memory";
        const MonsterPathRestoreInput input{record.behavior,memory,data.Transform(record),(*sphere)[3]};
        const auto h=world.CreateEndpoint({1,static_cast<std::uint32_t>(i)},static_cast<std::uint32_t>(3+count*2));Check(RestoreMonsterPath(kind,world.Helper(h),input,*playback,world));
        for(unsigned frame=0;frame<3;++frame)Check(movement.Update(h,{},1.f/60).active);
        const auto time=std::min(.125f,playback->Duration()*.5f);playback->Seek(time);const std::string moving(playback->Action());
        const auto speed=Value(world.Helper(h),25);const auto direction=world.Helper(h).words[48];const auto wander=world.Helper(h).words[49]&0xff;
        const auto selected=SaveMonsterPath(kind,h,memory,*playback,world);Reject([&]{(void)world.Helper(h);});
        Check(world.Stats().helpers==0 && world.Stats().members==0 && world.Stats().associations==0 && world.Stats().spatial_nodes==0);
        playback->Advance(.5f);const std::string idle(playback->Action());const auto bytes=BinaryXmlConfig::Serialize(memory);Check(bool(bytes));BinaryXmlConfig parsed;Check(parsed.Parse(*bytes));
        Near(parsed.Root().FindAttribute("AAB_MST_memory_speed")->ToFloat(-1),speed);Near(parsed.Root().FindAttribute("AAB_MST_memory_actiontime")->ToFloat(-1),time);
        const auto next=world.CreateEndpoint({1,static_cast<std::uint32_t>(i)},static_cast<std::uint32_t>(4+count*2));
        Check(RestoreMonsterPath(kind,world.Helper(next),input,*playback,world));Near(playback->Time(),time);Near(Value(world.Helper(next),25),speed);
        Check(playback->Action()==moving && world.Helper(next).words[48]==direction && (world.Helper(next).words[49]&0xff)==(wander>0?1u:0u));const auto vertices=CheckPose(*playback);
        if(count)out<<',';out<<"{\"monster\":"<<std::quoted(record.name)<<",\"kind\":"<<kind<<",\"move\":"<<std::quoted(moving)<<",\"idle\":"<<std::quoted(idle)<<",\"selected\":"<<selected<<",\"time_bits\":"<<std::bit_cast<std::uint32_t>(time)<<",\"vertices\":"<<vertices<<",\"restored_words\":";WriteWords(out,world.Helper(next));out<<",\"memory_hex\":\"";
        for(const auto byte:*bytes){constexpr char hex[]="0123456789abcdef";out<<hex[byte>>4]<<hex[byte&15];}out<<"\"}";
        world.DestroyHelper(next);++count;
    }}
    Check(count>0);world.Close();Check(world.Stats().blocks==0);out<<"],\"closed_blocks\":0}";Check(bool(out));
}
void WorldSource(const char* game,const char* cache,const char* world_name,const char* scene,const char* report){
    SourceData data(game,cache,world_name,scene);std::size_t draws{};
    MonsterMovementWorld spatial(1,data.paths,[&]{constexpr std::uint32_t samples[]{16384,16384,24575,100};return samples[draws++%4];});
    struct Restored {MonsterHelperHandle handle;std::size_t record;std::unique_ptr<ActorPlayback> playback;};std::vector<Restored> restored;
    std::ofstream out(report);out<<"{\"contract_world_only\":false,\"production_spatial_world\":true,\"controlled_memory\":true,\"controlled_random\":true,\"controlled_position_notifications\":true,\"gameplay_executed\":false,\"restores\":[";bool first=true;
    for(std::size_t i=0;i<data.monsters.size();++i){const auto& record=data.monsters[i];for(const auto kind:PathKinds(record)){
        auto playback=data.Playback(record);const auto sphere=ActorBindSphere(data.ModelFor(record.resource));Check(bool(sphere));BinaryXmlNode memory;
        MonsterPathRestoreInput input{record.behavior,memory,data.Transform(record),(*sphere)[3]};const auto before_draws=draws;
        const auto handle=spatial.CreateEndpoint({1,static_cast<std::uint32_t>(i)},static_cast<std::uint32_t>(restored.size()+3));auto& helper=spatial.Helper(handle);
        Check(RestoreMonsterPath(kind,helper,input,*playback,spatial));const auto vertices=CheckPose(*playback);
        if(!first)out<<',';first=false;out<<"{\"monster\":"<<std::quoted(record.name)<<",\"record_index\":"<<i<<",\"helper\":"<<handle.token<<",\"kind\":"<<kind<<",\"action\":"<<std::quoted(std::string(playback->Action()))<<",\"random_draws\":"<<draws-before_draws<<",\"helper_words\":";WriteWords(out,helper);
        out<<",\"vertices\":"<<vertices<<'}';restored.push_back({handle,i,std::move(playback)});
    }}
    const auto grid=spatial.ReadWord(spatial.GridParameter()+4);const std::array<std::uint32_t,3> dimensions{spatial.ReadWord(grid+24),spatial.ReadWord(grid+28),spatial.ReadWord(grid+32)};
    Check(dimensions==std::array<std::uint32_t,3>{20,1,20});
    const auto count=restored.size();const auto stats=spatial.Stats();Check(stats.helpers==count && stats.members==count && stats.spatial_nodes==count && stats.associations==count);
    const auto initial=spatial.QuerySphere({0,0,0},.001f);Check(initial.size()==count); // No invented notification after Accept's transform write.
    out<<"],\"grid_dimensions\":["<<dimensions[0]<<','<<dimensions[1]<<','<<dimensions[2]<<"],\"initial_spatial_origins\":"<<initial.size()<<",\"queries\":[";
    for(const auto& entry:restored){auto& helper=spatial.Helper(entry.handle);(void)spatial.UpdatePosition(helper.words[42],data.monsters[entry.record].position);}
    for(std::size_t i=0;i<restored.size();++i){
        const auto& entry=restored[i];const auto center=data.monsters[entry.record].position;const auto neighbors=spatial.QuerySphere(center,128.f);
        std::size_t expected{};for(const auto& candidate:restored){const auto& p=data.monsters[candidate.record].position;double distance{};for(unsigned a=0;a<3;++a){const double delta=double(p[a])-center[a];distance+=delta*delta;}if(distance<128.f*128.f)++expected;}
        Check(neighbors.size()==expected);
        if(i)out<<',';out<<"{\"helper\":"<<entry.handle.token<<",\"neighbors\":[";
        for(std::size_t j=0;j<neighbors.size();++j){if(j)out<<',';const auto& neighbor=neighbors[j];Check(neighbor.member);out<<"{\"helper\":"<<neighbor.helper.token<<",\"owner_index\":"<<neighbor.owner.index<<",\"distance_bits\":"<<std::bit_cast<std::uint32_t>(neighbor.distance_squared)<<'}';}out<<"]}";
    }
    out<<"],\"path_queries\":[";bool first_path=true;
    for(std::size_t i=0;i<data.paths->Size();++i){
        const auto h=data.paths->Handle(i);if(!data.paths->HasGrid(h))continue;
        const auto token=spatial.FindPath(data.paths->Source(h).name);
        // The original name lookup selects the first duplicate, not necessarily i.
        const auto selected=spatial.PathHandle(token);
        if(!first_path)out<<',';first_path=false;
        out<<"{\"index\":"<<i<<",\"selected_index\":"<<selected.index<<",\"samples\":[";
        for(std::size_t j=0;j<data.paths->PointCount(selected);++j){
            auto point=data.paths->Point(selected,j);point[0]+=3.f;point[2]-=2.f;
            const auto distance=spatial.PathClosest(token,point);Near(distance,data.paths->ClosestPosition(selected,point));
            const auto coordinate=spatial.PathCoordinates(token,distance),expected=data.paths->CoordinatesAt(selected,distance);
            const auto projection=spatial.PathProject(token,point,{0,0,1}),expected_projection=data.paths->ProjectToTube(selected,point,{0,0,1});
            Near(projection.outside,expected_projection.outside);
            for(unsigned axis=0;axis<3;++axis){Near(coordinate[axis],expected[axis]);Near(projection.point[axis],expected_projection.point[axis]);Near(projection.direction[axis],expected_projection.direction[axis]);}
            if(j)out<<',';out<<"["<<std::bit_cast<std::uint32_t>(distance)<<','<<std::bit_cast<std::uint32_t>(projection.outside);
            for(const auto& vector:{coordinate,projection.point,projection.direction})for(const auto value:vector)out<<','<<std::bit_cast<std::uint32_t>(value);
            out<<']';
        }
        out<<"]}";
    }
    out<<"],\"retirement\":[";
    for(std::size_t i=count;i>0;--i){const auto h=restored[i-1].handle;const auto association=spatial.Helper(h).words[42];spatial.DestroyHelper(h);Reject([&]{(void)spatial.Helper(h);});Reject([&]{(void)spatial.ReadWord(association);});
        const auto left=spatial.Stats();Check(left.helpers==i-1 && left.members==i-1 && left.spatial_nodes==i-1 && left.associations==i-1);if(i<count)out<<',';out<<left.helpers;
    }
    spatial.Close();Check(spatial.Stats().blocks==0 && spatial.Stats().mapped_bytes==0);out<<"],\"closed_blocks\":0}";Check(bool(out));
}
void FrameSource(const char* game,const char* cache,const char* world_name,const char* scene,const char* report,bool grounded=false,bool include_random=false){
    SourceData data(game,cache,world_name,scene);std::size_t draws{};
    MonsterMovementWorld world(1,data.paths,[&]{constexpr std::uint32_t stream[]{16384,24575,8192,16384,8192,24575};return stream[draws++%6];});
    std::unique_ptr<SceneMonsterInstances> instances;CollisionMesh collision;std::unique_ptr<SceneMovement> floor;
    MonsterOrientationCaches caches;std::unique_ptr<MonsterOrientation> orientation;
    if(grounded){
        instances=std::make_unique<SceneMonsterInstances>(1,data.monsters,[&](std::string_view id){return data.ModelFor(id);},[&](std::string_view member)->std::optional<std::vector<std::uint8_t>>{if(!data.actors.Find(member))return {};return data.actors.Read(member);});
        collision=data.Collision();Check(!collision.floor.empty());
        floor=std::make_unique<SceneMovement>(collision.floor);orientation=std::make_unique<MonsterOrientation>(world,*instances,*floor,caches);
    }
    struct Entry {MonsterHelperHandle helper;std::size_t record;std::int32_t kind;float radius;std::unique_ptr<ActorPlayback> playback;};std::vector<Entry> entries;
    for(std::size_t i=0;i<data.monsters.size();++i){const auto& record=data.monsters[i];for(const auto kind:PathKinds(record,include_random)){
        auto playback=data.Playback(record);const auto sphere=ActorBindSphere(data.ModelFor(record.resource));Check(bool(sphere));BinaryXmlNode memory;
        MonsterPathRestoreInput input{record.behavior,memory,data.Transform(record),(*sphere)[3]};
        const SceneMonsterHandle actor{1,static_cast<std::uint32_t>(i)};const auto id=static_cast<std::uint32_t>(entries.size()+3);
        const auto h=kind==3?world.CreateRandom(actor,id):world.CreateEndpoint(actor,id);
        Check(kind==3?RestoreMonsterRandom(world.Helper(h),input,*playback,world):RestoreMonsterPath(kind,world.Helper(h),input,*playback,world));entries.push_back({h,i,kind,(*sphere)[3],std::move(playback)});
    }}
    Check(!entries.empty());MonsterMovement movement(world);const auto before=world.Stats();
    std::ofstream out(report);out<<"{\"gameplay_executed\":false,\"controlled_random\":true,\"controlled_memory\":true,\"static_obstacles_supplied\":false,\"actor_transform_written\":"<<(grounded?"true":"false")<<",\"controlled_manager_radius\":true,\"controlled_actor_order\":true,\"grid_before_actor_phase\":true,\"frames\":[";
    std::size_t moved{},floor_hits{},floor_misses{};
    std::vector<MonsterMovementResult> frame_results(entries.size());std::vector<BehaviorVector3> previous_positions(entries.size());
    for(unsigned frame=0;frame<600;++frame){if(frame)out<<',';out<<'[';
        // Original437CF0 updates the whole grid before actor behavior updates.
        // Actor iteration remains explicitly controlled by this source probe.
        for(std::size_t i=0;i<entries.size();++i)previous_positions[i]=BehaviorHelperConfiguration::ReadVector(world.Helper(entries[i].helper),0x18);
        world.UpdateMembers(1.f/60,[&](auto handle,float elapsed){
            const auto found=std::find_if(entries.begin(),entries.end(),[&](const auto& entry){return entry.helper==handle;});Check(found!=entries.end());
            const auto i=static_cast<std::size_t>(found-entries.begin());frame_results[i]=movement.Update(handle,{},elapsed);Check(frame_results[i].active);
        });
        for(std::size_t i=0;i<entries.size();++i){const auto& entry=entries[i];auto& helper=world.Helper(entry.helper);
            const auto& position=previous_positions[i];const auto& result=frame_results[i];
            if(grounded){const auto corrected=orientation->Update(entry.kind,entry.helper,entry.radius);Check(corrected.active);if(corrected.floor_hit)++floor_hits;else ++floor_misses;
                const auto h=instances->Handle(entry.record);Check(instances->Source(h).position==data.monsters[entry.record].position);
                Check(corrected.transform.position==BehaviorHelperConfiguration::ReadVector(helper,0x18));const auto matrix=instances->InstanceTransform(h);
                Check(matrix[12]==corrected.transform.position[0] && matrix[13]==corrected.transform.position[1] && matrix[14]==corrected.transform.position[2]);
            }
            const auto next=BehaviorHelperConfiguration::ReadVector(helper,0x18);moved+=next!=position;
            for(float value:next)Check(std::isfinite(value));for(float value:result.requested)Check(std::isfinite(value));
            // Floor correction does not issue a second spatial notification.
            if(!grounded){const auto members=world.QuerySphere(next,.001f);Check(std::any_of(members.begin(),members.end(),[&](const auto& member){return member.helper==entry.helper && member.distance_squared==0;}));}
            if(i)out<<',';out<<'['<<entry.record<<','<<helper.words[48]<<','<<std::bit_cast<std::uint32_t>(Value(helper,25));
            for(const auto& vector:{next,result.requested,BehaviorHelperConfiguration::ReadVector(helper,0x10)})for(const auto value:vector)out<<','<<std::bit_cast<std::uint32_t>(value);
            if(grounded){const auto transform=instances->MotionTransform(instances->Handle(entry.record));for(const auto& vector:{transform.position,transform.rotation})for(const auto value:vector){Check(std::isfinite(value));out<<','<<std::bit_cast<std::uint32_t>(value);}}out<<']';
        }
        out<<']';
    }
    Check(moved>0 && world.Stats().mapped_bytes==before.mapped_bytes);
    const auto& stats=movement.Stats();out<<"],\"helpers\":"<<entries.size()<<",\"moved_steps\":"<<moved<<",\"stats\":["<<stats.frames<<','<<stats.random_draws<<','<<stats.sphere_queries<<','<<stats.neighbor_visits<<','<<stats.candidate_tests<<','<<stats.candidate_hits<<','<<stats.proximity<<','<<stats.predicted<<','<<stats.drift_updates<<','<<stats.path_corrections<<','<<stats.direction_flips<<"],\"final_words\":[";
    for(std::size_t i=0;i<entries.size();++i){if(i)out<<',';WriteWords(out,world.Helper(entries[i].helper));}
    world.Close();Check(world.Stats().blocks==0);out<<"],\"floor_hits\":"<<floor_hits<<",\"floor_misses\":"<<floor_misses<<",\"closed_blocks\":0}";Check(bool(out));
}

}
int main(int argc,char** argv)try{
    if(argc==7 && std::string_view(argv[1])=="scheduled-source"){TaskSource(argv[2],argv[3],argv[4],argv[5],argv[6],true);return 0;}
    if(argc==7 && std::string_view(argv[1])=="task-source"){TaskSource(argv[2],argv[3],argv[4],argv[5],argv[6]);return 0;}
    if(argc==7 && std::string_view(argv[1])=="random-cycle-source"){RandomCycleSource(argv[2],argv[3],argv[4],argv[5],argv[6]);return 0;}
    if(argc==7 && std::string_view(argv[1])=="cycle-source"){CycleSource(argv[2],argv[3],argv[4],argv[5],argv[6]);return 0;}
    if(argc==7 && std::string_view(argv[1])=="ground-source"){FrameSource(argv[2],argv[3],argv[4],argv[5],argv[6],true);return 0;}
    if(argc==7 && std::string_view(argv[1])=="mixed-ground-source"){FrameSource(argv[2],argv[3],argv[4],argv[5],argv[6],true,true);return 0;}
    if(argc==7 && std::string_view(argv[1])=="mixed-frame-source"){FrameSource(argv[2],argv[3],argv[4],argv[5],argv[6],false,true);return 0;}
    if(argc==7 && std::string_view(argv[1])=="frame-source"){FrameSource(argv[2],argv[3],argv[4],argv[5],argv[6]);return 0;}
    if(argc==7 && std::string_view(argv[1])=="source"){Source(argv[2],argv[3],argv[4],argv[5],argv[6]);return 0;}
    if(argc==7 && std::string_view(argv[1])=="world-source"){WorldSource(argv[2],argv[3],argv[4],argv[5],argv[6]);return 0;}
    CoreContracts();RestoreContracts();MovementWorldContracts();RandomWorldContracts();RandomStateContracts();MemoryWriterContracts();SaveContracts();std::cout<<"shared helper setup and original C02/C03 restore contracts passed; no gameplay executed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
