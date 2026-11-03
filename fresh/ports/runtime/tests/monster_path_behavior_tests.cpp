#include "pal4/portable/monster_movement_behavior.h"
#include "pal4/portable/monster_base_motion.h"
#include "pal4/portable/monster_proximity.h"
#include "pal4/portable/monster_pursuit_paths.h"
#include "pal4/portable/monster_pursuit_frame.h"
#include "pal4/portable/arena_source.h"
#include "pal4/portable/monster_scene_resources.h"
#include "pal4/portable/monster_scene_frame.h"
#include "pal4/portable/monster_scene_actor.h"
#include "pal4/portable/monster_scene_actors.h"
#include "pal4/portable/monster_player_view.h"
#include "pal4/portable/monster_interaction.h"
#include "pal4/portable/monster_encounter_storage.h"
#include "pal4/portable/monster_combat_collector.h"
#include "pal4/portable/monster_combat_handoff.h"
#include "pal4/portable/original_player_data.h"
#include "pal4/portable/floor_attributes.h"
#include "pal4/portable/actor_resource_frame.h"
#include "pal4/portable/actor_resource_initialization.h"
#include "pal4/game_resource_manager.h"
#include "pal4/collision_actor.h"
#include "pal4/portable/monster_movement.h"
#include "pal4/portable/world.h"
#include "pal4/behavior_helper_configuration.h"
#include "pal4/portable/package.h"
#include "pal4/portable/object_memory.h"
#include "pal4/portable/borrowed_actor_base_members.h"
#include "../src/monster_behavior_scheduler.h"
#include "../src/monster_component_source.h"
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <bit>
#include <cmath>
#include <iostream>
#include <limits>
#include <source_location>
#include <sstream>
#include <stdexcept>
using namespace pal4;
using namespace pal4::portable;
namespace {
void Check(bool value,const std::source_location where=std::source_location::current()){if(!value)throw std::runtime_error("path behavior contract failed at "+std::to_string(where.line()));}
template<class F>void Reject(F call){bool rejected=false;try{call();}catch(const std::exception&){rejected=true;}Check(rejected);}
void EncounterStorage(){
    ObjectMemory memory;MonsterBehaviorManagerState state{};MonsterEncounterStorage storage(state,memory);
    storage.Append({11,22,33});storage.Append({44,55,66});auto begin=state.words[0x714/4];
    Check(memory.ReadDword(begin)==11 && memory.ReadDword(begin+20)==66 && state.words[0x718/4]-begin==24);
    state.words[0x718/4]=begin+12;storage.Append({77,88,99});begin=state.words[0x714/4];
    Check(state.words[0x718/4]-begin==24 && memory.ReadDword(begin+12)==77);
    storage.AddName("first");storage.AddName("second");storage.AddName("first");auto names=state.words[0x724/4];
    Check(state.words[0x728/4]-names==32);const auto first=memory.ReadDword(names+4),second=memory.ReadDword(names+20);
    Check(storage.Name(first)=="first" && storage.Name(second)=="second");
    Check(storage.ReleaseHistoryRecord(names) && !storage.ReleaseHistoryRecord(names+1));Reject([&]{(void)storage.Name(first);});
    state.words[0x728/4]=names+16;storage.AddName("third");names=state.words[0x724/4];
    Reject([&]{(void)storage.Name(second);});Check(storage.Name(memory.ReadDword(names+20))=="third");
    const auto end=state.words[0x718/4];state.words[0x718/4]=state.words[0x71c/4]+12;Reject([&]{storage.Close();});state.words[0x718/4]=end;
    storage.Close();storage.Close();Check(memory.AllocationCount()==0 && !state.words[0x714/4] && !state.words[0x724/4]);
    ObjectMemory limited({0x10000000,0x10001000,16,1});MonsterBehaviorManagerState other{};MonsterEncounterStorage failed(other,limited);
    Reject([&]{failed.AddName("long name");});Check(limited.AllocationCount()==0 && !other.words[0x724/4]);failed.Close();
}
BinaryXmlAttribute Int(std::string name,int value){return {std::move(name),1,std::int32_t(value)};}
BinaryXmlAttribute Real(std::string name,float value){return {std::move(name),2,value};}
BinaryXmlConfig Behavior(int kind){
    BinaryXmlNode parameters;parameters.name="AAB_MST_parameters";
    parameters.attributes={Real(kind==0?"AABPedestrian_MST_walk_max_speed":"AABPedestrian_MST_run_max_speed",6),Real(kind==0?"AABPedestrian_MST_walk_max_force":"AABPedestrian_MST_run_max_force",3),Int("AABPedestrian_MST_walk_pathfollow",1)};
    if(kind==3)parameters.attributes={Real("AABWander_MST_max_speed",6),Real("AABWander_MST_max_force",3),Real("AABWander_MST_max_range",40)};
    BinaryXmlNode choice;choice.name="choice";choice.children={parameters};choice.attributes={Int("MSTINFO_Behavior_ObjectType",kind),Int("MSTINFO_Behavior_ObjectProportion",100),Int("MSTINFO_Behavior_ObjectWaitMode",1),Real("MSTINFO_Behavior_ObjectWaitTime",0),Real("MSTINFO_Behavior_ObjectExpectTime",1)};
    BinaryXmlNode group;group.name="group";group.children={choice};group.attributes={Int("MSTINFO_Behavior_GroupMode",0),{"MSTINFO_Behavior_GroupPathWayID",3,std::string("route")}};
    BinaryXmlNode root;root.name="MSTINFO_Behavior";root.children={group};const auto bytes=BinaryXmlConfig::Serialize(root);Check(bool(bytes));BinaryXmlConfig result;Check(result.Parse(*bytes));return result;
}
Actor Model(){
    Actor a;const Matrix identity{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};a.frames.push_back({-1,identity});a.palettes.push_back({0,identity});a.mesh.vertices.resize(3);a.mesh.vertices[0].position[0]=1;
    a.atomic_spheres.push_back({0,{0,0,0,2}});a.influences.resize(3,{{0,0,0,0},{1,0,0,0}});
    for(const auto name:{"C01","Z01","C02","C03","Z02"})a.clips.push_back({name,2,{{0,{{0,{0,0,0,1},{0,0,0}},{2,{0,0,0,1},{2,0,0}}}}}});return a;
}
std::vector<SceneMonster> Records(bool random_second=false){
    std::vector<SceneMonster> records(2);for(unsigned i=0;i<2;++i){auto& r=records[i];r.name=i?"second":"first";r.resource="401";r.fields[1]=std::bit_cast<std::uint32_t>(100.f);r.position={i?-3.f:3.f,2,100+20.f*i};r.behavior=Behavior(i==1 && random_second?3:static_cast<int>(i));
        if(i==0){BinaryXmlNode cache;cache.attributes={Real("AAB_MST_ESSBEH_max_pursuit_speed",12)};const auto bytes=BinaryXmlConfig::Serialize(cache);Check(bytes && r.cache.Parse(*bytes));}}return records;
}
struct Fixture {
    bool fail_idle{};std::function<float()> radius_callback;std::uint32_t identifiers=10;std::size_t draws{};
    std::vector<SceneMonster> records=Records();SceneMonsterInstances actors;
    std::shared_ptr<ScenePathways> paths;MonsterMovementWorld world;MonsterMovement movement;
    std::vector<CollisionTriangle> floor_geometry;SceneMovement floor;MonsterOrientationCaches caches;MonsterOrientation orientation;
    std::array<BinaryXmlNode,2> memory;std::array<std::unique_ptr<MonsterMovementBehavior>,2> behavior;
    Fixture(bool random_second=false):records(Records(random_second)),actors(1,records,[](auto){return Model();},[this](std::string_view member)->std::optional<std::vector<std::uint8_t>>{if(fail_idle && member.find("C01")!=std::string_view::npos)throw std::runtime_error("controlled idle failure");return {};}),
        paths(std::make_shared<ScenePathways>(1,std::vector<ScenePath>{{"route",1,std::bit_cast<std::uint32_t>(5.f),0,{{0,0,0},{0,0,800}}}})),
        world(1,paths,[this]{constexpr std::uint32_t stream[]{16384,16384,24575,100};return stream[draws++%4];}),movement(world),
        floor_geometry{{{Point3{-2000,0,-2000},Point3{2000,0,-2000},Point3{0,0,2000}},1,0,0}},floor(floor_geometry),orientation(world,actors,floor,caches){
        for(unsigned i=0;i<2;++i)behavior[i]=std::make_unique<MonsterMovementBehavior>(world,actors,actors.Handle(i),memory[i],orientation,[this]{return radius_callback?radius_callback():2.f;},[this]{return identifiers++;});
    }
    void Grid(float seconds){world.UpdateMembers(seconds,[&](auto helper,float elapsed){Check(movement.Update(helper,{},elapsed).active);});}
    void Actors(float seconds){
        // Explicit controlled animation phase for generated-model contracts.
        for(unsigned i=0;i<2;++i)if(!actors.Action(actors.Handle(i)).empty())actors.AdvanceAnimation(actors.Handle(i),seconds);
        for(auto& owner:behavior)owner->AdvanceAfterGrid(seconds);
    }
    void Tick(float seconds){Grid(seconds);Actors(seconds);}
};

void ProcessorLifecycle(){
    using Scheduler=pal4::portable::detail::MonsterBehaviorScheduler;
    using Bindings=pal4::portable::detail::MonsterSchedulerBindings;
    const auto bindings=[] {return Bindings{[]{return true;},[]{return false;},[](float){return 0u;},[]{return 0u;}};};
    MonsterBehaviorDefinition definition;definition.fuzzy.emplace().mode=0;
    ObjectMemory storage;const auto unrelated=storage.Allocate(32);storage.WriteDword(unrelated,0xdeadbeef);
    BinaryXmlNode memory;memory.attributes.push_back(Int("saved",17));
    {
        Scheduler scheduler(definition,bindings(),memory,&storage);Check(&scheduler.Memory()==&memory && scheduler.MemoryToken() && scheduler.AllocationCount()==4 && storage.AllocationCount()==5);
        Check(memory.name=="behAIAtomicMgr-memory" && memory.FindAttribute("saved")->ToInt(0)==17);
        scheduler.SetEnabled(true);Check(scheduler.Advance(0)==0 && scheduler.AllocationCount()==5);
        scheduler.Close();Check(scheduler.Closed() && !scheduler.Enabled() && !scheduler.MemoryToken() && !scheduler.AllocationCount());scheduler.Close();
        Check(memory.FindAttribute("saved")->ToInt(0)==17 && storage.AllocationCount()==1 && storage.ReadDword(unrelated)==0xdeadbeef);
        Reject([&]{(void)scheduler.Advance(0);});Reject([&]{scheduler.SetEnabled(true);});Reject([&]{(void)scheduler.Memory();});
    }
    {
        MonsterBehaviorDefinition none;BinaryXmlNode restored;restored.name="restored";restored.attributes.push_back(Int("saved",21));
        Scheduler scheduler(none,bindings(),restored,&storage);Check(scheduler.AllocationCount()==2 && restored.name=="restored");scheduler.SetEnabled(true);Check(scheduler.Advance(0)==0);
    }
    Check(storage.AllocationCount()==1);(void)storage.Free(unrelated);
    for(unsigned budget:{16u,60u,64u,104u}){
        ObjectMemory limited({0x10000000,0x70000000,budget,100});BinaryXmlNode data;data.attributes.push_back(Int("saved",33));
        Reject([&]{Scheduler failed(definition,bindings(),data,&limited);});Check(limited.AllocationCount()==0 && limited.AllocatedBytes()==0 && data.name.empty() && data.FindAttribute("saved")->ToInt(0)==33);
    }
    {
        ObjectMemory limited({0x10000000,0x70000000,108,100});BinaryXmlNode data;Scheduler scheduler(definition,bindings(),data,&limited);scheduler.SetEnabled(true);
        Reject([&]{(void)scheduler.Advance(0);});scheduler.Close();Check(limited.AllocationCount()==0);
    }
    Fixture f;auto& owner=*f.behavior[0];Check(&owner.ProcessorMemory()==&f.memory[0] && owner.ProcessorMemoryToken());
    unsigned nested{};f.radius_callback=[&]{++nested;Reject([&]{owner.Close();});return 2.f;};Check(owner.Start());f.Tick(0);Check(nested && !owner.Closed() && !owner.Failed());f.radius_callback={};
    owner.Cancel();const auto saved=BinaryXmlConfig::Serialize(f.memory[0]);Check(saved.has_value());owner.Close();owner.Close();owner.Cancel();
    Check(owner.Closed() && !owner.ProcessorMemoryToken() && !owner.ProcessorAllocationCount() && BinaryXmlConfig::Serialize(f.memory[0])==saved);
    Reject([&]{(void)owner.Start();});Reject([&]{(void)owner.ProcessorMemory();});Check(!f.behavior[1]->Closed() && f.behavior[1]->ProcessorAllocationCount()>0);
}

void Pipeline(bool random_second=false){
    Fixture f(random_second);
    std::array<MonsterBehaviorManagerState,2> states{};
    CollisionSurface surface(f.floor_geometry);surface.EnableNavigation();
    std::array<std::unique_ptr<MonsterBaseMotion>,2> motion;
    for(unsigned i=0;i<2;++i){auto& state=states[i];const auto pose=f.actors.MotionTransform(f.actors.Handle(i));state.words[5]=1;state.words[8]=1;
        for(unsigned a=0;a<3;++a){state.words[43+a]=std::bit_cast<std::uint32_t>(pose.position[a]);state.words[56+a]=state.words[43+a];state.words[40+a]=std::bit_cast<std::uint32_t>(pose.rotation[a]);state.words[46+a]=std::bit_cast<std::uint32_t>(1.f);state.words[60+a]=std::bit_cast<std::uint32_t>(1.f);state.words[63+a]=std::bit_cast<std::uint32_t>(-1.f);}
        // 显式受控资源服务；此合同不声称完成真实资源初始化。
        motion[i]=std::make_unique<MonsterBaseMotion>(f.actors,f.actors.Handle(i),state,0x10000u+i*0x10000u,surface,2.f,[](auto,auto){return false;},[](auto&,float){});
    }
    const auto verify=[&]{for(unsigned i=0;i<2;++i){const auto h=f.actors.Handle(i);const auto pose=f.actors.MotionTransform(h);const auto matrix=f.actors.InstanceTransform(h);
        for(unsigned a=0;a<3;++a){Check(std::bit_cast<float>(states[i].words[43+a])==pose.position[a]);
            Check(std::bit_cast<float>(states[i].words[40+a])==pose.rotation[a]);
            Check(matrix[12+a]==pose.position[a]);
            Check(std::abs(std::bit_cast<float>(states[i].words[56+a])-pose.position[a])<.001f);}
    }};
    for(auto& behavior:f.behavior)Check(behavior->Start());f.Tick(0);verify();
    Check(f.world.Stats().members==2 && f.behavior[0]->Stats().entered==1 && f.behavior[1]->Stats().entered==1);
    const auto born=f.actors.Source(f.actors.Handle(0)).position;
    for(unsigned frame=0;frame<100 && (!f.behavior[0]->Ready() || !f.behavior[1]->Ready());++frame){f.Tick(1.f/60);verify();}
    for(unsigned i=0;i<2;++i){const auto& owner=f.behavior[i];Check(owner->Ready() && !owner->Failed() && owner->TaskCount()==0 && owner->Stats().saved==1 && owner->Stats().live_actions==0);
        Check(f.memory[i].FindAttribute("AAB_MST_memory_last_behavior_id")->ToInt(99)==(i==1 && random_second?3u:i));Check(f.actors.MotionTransform(f.actors.Handle(i)).position[1]==0);}
    Check(f.world.Stats().helpers==0 && f.world.Stats().associations==0 && f.actors.Source(f.actors.Handle(0)).position==born);
    Check(f.actors.MotionTransform(f.actors.Handle(0)).position!=born);
    // A new single round begins from the actual current actor transform/memory.
    const auto position=f.actors.MotionTransform(f.actors.Handle(0)).position;Check(f.behavior[0]->Start());f.Tick(0);
    f.world.UpdateMembers(0,[&](auto h,float){if(f.world.Owner(h).index==0)Check(BehaviorHelperConfiguration::ReadVector(f.world.Helper(h),0x18)==position);});
    f.behavior[0]->Cancel();f.behavior[0]->Cancel();Check(f.world.Stats().helpers==0 && f.behavior[0]->Stats().saved==2);
    if(random_second){
        Check(f.memory[1].FindAttribute("AABWander_MST_memory_start_x") && f.memory[1].FindAttribute("AABWander_MST_memory_start_y") && f.memory[1].FindAttribute("AABWander_MST_memory_start_z"));
        Check(f.caches.random.guard==3 && f.caches.kinds[0].guard==3 && f.caches.kinds[1].guard==0);
        Check(f.behavior[1]->Start());f.Tick(0);f.world.UpdateMembers(0,[&](auto h,float){Check(f.world.IsRandom(h));});
        f.behavior[1]->Cancel();Check(f.behavior[1]->Stats().saved==2 && f.world.Stats().helpers==0);
    }
    std::ostringstream report;f.behavior[0]->WriteReport(report);Check(report.str().find("\"failed\":false")!=std::string::npos);
}
// 原包输入探针：显式构造测试管理器图像，不代表原管理器初始化已完成。
enum class SourceProbe {proximity,paths,frame,resource,initialization,base_lifetime,members_lifetime,component_lifetime};
struct SourceMovementOwner {
    ObjectMemory storage;MonsterMovementWorld world;MonsterMovement movement;SceneMovement ground;MonsterOrientationCaches caches;MonsterOrientation orientation;
    BinaryXmlNode memory;unsigned identifier=3;bool aggregated;std::unique_ptr<MonsterMovementBehavior> behavior;
    SourceMovementOwner(SceneMonsterInstances& actors,SceneMonsterHandle h,const CollisionMesh& mesh,std::shared_ptr<ScenePathways> paths,MonsterBehaviorManagerState* state=nullptr,std::uint32_t token=0,bool component=false)
        :world(7,std::move(paths),[]{return 16384u;}),movement(world),ground(mesh.floor),orientation(world,actors,ground,caches),aggregated(state!=nullptr){
        if(state)behavior=std::make_unique<MonsterMovementBehavior>(world,actors,h,memory,orientation,[&actors,h]{return actors.LocalSphere(h)[3];},[this]{return identifier++;},*state,token,storage,component?MonsterBehaviorPreparation::Component:MonsterBehaviorPreparation::Immediate);
        else behavior=std::make_unique<MonsterMovementBehavior>(world,actors,h,memory,orientation,[&actors,h]{return actors.LocalSphere(h)[3];},[this]{return identifier++;});
    }
    ~SourceMovementOwner(){if(aggregated){behavior->Close();Check(storage.AllocationCount()==0);}}
};
struct SourceBaseOwner {
    ObjectMemory storage;MonsterBehaviorManagerState& state;BaseObjectState base{};std::uint32_t token;BorrowedActorBaseMembers members;
    SourceBaseOwner(MonsterBehaviorManagerState& image,std::uint32_t id):state(image),token(id),members(storage,[this](auto& value){Check(&value==&base);return token;}){
        (void)members.Construct(base,token);std::copy(base.words.begin(),base.words.end(),state.words.begin());
    }
    ~SourceBaseOwner(){
        std::copy_n(state.words.begin(),base.words.size(),base.words.begin());base.words[5]=0;
        members.Destroy(base);Check(storage.AllocationCount()==0);
    }
};
// 原碰撞构造拥有路径；运动组件只借用。源探针仍未构造完整怪物。
struct SourceCollisionOwner {
    ObjectMemory storage;MonsterBehaviorManagerState& state;
    explicit SourceCollisionOwner(MonsterBehaviorManagerState& image):state(image){
        CollisionActorState collision{};(void)CollisionActor::Construct(collision,0,storage.Path());
        std::copy(collision.words.begin(),collision.words.end(),state.words.begin()+0x564/4);
    }
    ~SourceCollisionOwner(){
        CollisionActorState collision;std::copy_n(state.words.begin()+0x564/4,collision.words.size(),collision.words.begin());
        (void)CollisionActor::Destroy(collision,storage.Path());
    }
};
// 多怪同时使用同一空间网格；场景服务仍是明确受控的测试替身。
void SharedWorldSource(const std::filesystem::path& game,const std::string& world_name,const std::string& scene,const std::filesystem::path& report,bool world_collision=false,bool real_player=false){
    auto source=std::make_shared<ArenaSource>(game);
    std::ifstream file(game/"scenedata.cpk",std::ios::binary);PackageReader package(file);
    const auto read=[&](const char* name){const auto bytes=package.Read(world_name+"\\"+scene+"\\"+name);return std::string(bytes.begin(),bytes.end());};
    std::istringstream monsters_input(read("mstInfo.mst")),paths_input(read("pathway.pwy"));
    MonsterSceneResources resources(7,source,ReadSceneMonsters(monsters_input),ReadScenePaths(paths_input));
    auto& actors=resources.Monsters();const auto mesh=source->ReadOriginalCollision(world_name,scene);
    const auto traversal=actors.OriginalTraversalOrder();
    Check(traversal.size()==actors.Size());
    for(std::size_t i=1;i<traversal.size();++i)
        Check(actors.Source(actors.Handle(traversal[i-1])).name<actors.Source(actors.Handle(traversal[i])).name);
    if(world_collision || real_player)Check(actors.Size()>4);
    std::optional<Scene> world_geometry;if(world_collision)world_geometry=source->ReadOriginalWorld(world_name,scene);
    CollisionSurface floor(mesh.floor);floor.EnableNavigation();SceneMovement ground(mesh.floor);
    std::uint32_t identifiers=3;MonsterMovementWorld world(7,resources.Paths(),[]{return 16384u;});
    MonsterMovement movement(world);MonsterOrientationCaches caches;MonsterOrientation orientation(world,actors,ground,caches);
    std::ifstream actor_file(game/"palactor.cpk",std::ios::binary);PackageReader actor_package(actor_file);
    PlayerRoster players;ActorController controller;std::unique_ptr<MonsterPlayerView> player_view;
    if(real_player){const auto data=ReadOriginalPlayerData(ReadOriginalDatabasePackage(game));
        players.Prepare(data.roles,data.upgrades,[&](std::string_view model){return source->ReadOriginalActorResource("gamedata\\PALActor\\"+std::string(model)+"\\",model,{},true);});
        players.EnterScene(world_name,scene);players.SetPosition(0,{100000,0,100000});
        controller.Initialize(1280,720,[&]{return source->ReadOriginalActorResource("gamedata\\PALObject\\JDumy\\","JDumy");});controller.SelectActor(0,1);
        player_view=std::make_unique<MonsterPlayerView>(players,controller);
    }
    struct Services final:IMonsterPursuitSceneServices {
        MonsterPlayerView* actual{};MonsterSceneActor* closing{};MonsterSceneActors* closing_scene{};MonsterSceneActors* encounter_group{};MonsterInteraction* interaction{};MonsterCombatCollector* collector{};MonsterCombatHandoff* handoff{};MonsterBehaviorManagerState* encounter_state{};std::string arena_name;std::uint32_t encounter_manager{};std::size_t encounter_index{};unsigned targets{},rejected_close{},rejected_scene_close{};bool fail_visit{};std::vector<unsigned> combat;
        MonsterProximityActor player{0xa00000,false,{},{},2};
        MonsterProximityActor ControlledActor() override {return actual?actual->ControlledActor():player;}
        MonsterPursuitTarget Target(std::uint32_t id) override {++targets;if(actual)return actual->Target(id);Check(id==player.address);return {player.active,player.position};}
        bool PlayerControlBusy() override {return false;}
        bool ProcessObjectInteraction(MonsterBehaviorManagerState& state,std::uint32_t manager,std::uint32_t target,float) override {if(!interaction)return false;Check(&state==encounter_state && manager==encounter_manager);return interaction->Process(target);}
        void DispatchCombatPositions(std::uint32_t manager,float) override {Check(interaction && manager==encounter_manager);if(collector)collector->Collect(encounter_index);combat.push_back(1);}
        void InitializeCombat(std::uint32_t manager,std::int32_t mode) override {Check(interaction && manager==encounter_manager && mode==-1);if(handoff){Check(encounter_group);(void)handoff->Process(encounter_group->At(encounter_index),mode,arena_name);}combat.push_back(2);encounter_state->words[0x1f0/4]=0;}
        void ClearHistoryRecord(std::uint32_t) override {throw std::logic_error("unexpected history");}
        void ProcessActionWithScript(std::uint32_t,float) override {}
        void VisitArenaObjects(std::uint32_t) override {if(closing){Reject([&]{closing->Close();});++rejected_close;}if(closing_scene){Reject([&]{closing_scene->Close();});Reject([&]{closing_scene->At(0).Close();});++rejected_scene_close;}if(fail_visit)throw std::runtime_error("controlled scene service failure");}
    } services;services.actual=player_view.get();
    std::size_t collision_queries=0,collision_hits=0;std::vector<std::unique_ptr<MonsterSceneActor>> owners;
    for(std::size_t i=0;i<actors.Size();++i){
        const auto h=actors.Handle(i);const auto token=(real_player?0x400000u:0x10000u)+static_cast<std::uint32_t>(i)*0x10000u;
        auto owner=std::make_unique<MonsterSceneActor>(world,actors,h,orientation,[&actors,h]{return actors.LocalSphere(h)[3];},[&identifiers]{return identifiers++;},actor_package,token,0x200000u+static_cast<std::uint32_t>(i)*8,0,1,floor,mesh,2.f,[&](auto a,auto b){if(!world_geometry)return false;++collision_queries;const bool hit=WorldSegmentBlocked(*world_geometry,a,b);collision_hits+=hit;return hit;},services);
        Check(owner->State().words[1]!=0 && owner->Label()==actors.Source(h).name);
        Check(actors.SelectAction(h,"Z01"));
        owner->Behavior().SetSchedulingEnabled(true);owners.push_back(std::move(owner));
    }
    std::size_t max_members=0;MonsterSceneActors group(world,movement,[&](auto){max_members=std::max(max_members,world.Stats().members);return std::vector<MonsterMovementObstacle>{};},std::move(owners));
    Reject([&]{group.At(0).Close();});
    if(world_collision){
        auto& owner=group.At(4);owner.State().words[0x1f0/4]=1;owner.Motion().SetSpeed(20);
        const auto stride=std::max<std::size_t>(1,mesh.floor.size()/256);
        for(std::size_t i=0;i<mesh.floor.size() && !collision_queries;i+=stride){
            Point3 target{};for(const auto& point:mesh.floor[i].points)for(unsigned a=0;a<3;++a)target[a]+=point[a]/3.f;
            const auto from=actors.MotionTransform(actors.Handle(4)).position;const auto dx=target[0]-from[0],dz=target[2]-from[2];
            if(dx*dx+dz*dz<40000.f)continue;const auto face=floor.Snap(target);if(!face)continue;
            if(owner.Motion().Start(from,target,face))for(unsigned step=0;step<3 && !collision_queries;++step)(void)owner.Motion().Update(1.f/60,1);
            owner.Motion().Stop();
        }
    }
    bool player_detected=false,return_started=false,returned=false;for(unsigned frame=0;frame<600;++frame){
        if(real_player && frame==200){group.At(4).State().words[0x1f0/4]=1;group.At(4).State().words[0x6f8/4]=std::bit_cast<std::uint32_t>(1.f);players.SetPosition(0,actors.MotionTransform(actors.Handle(4)).position);}
        if(real_player && frame==202){auto position=players.Slots()[0].position;position[0]+=150;players.SetPosition(0,position);}
        if(real_player && frame==205){group.At(4).State().words[0x704/4]=1;players.SetPosition(0,{100000,0,100000});}
        if(world_collision && frame==200){group.At(4).State().words[0x1f0/4]=1;group.At(4).State().words[0x6f8/4]=std::bit_cast<std::uint32_t>(1.f);services.player.position=actors.MotionTransform(actors.Handle(4)).position;services.player.position[0]+=10;services.player.collision_position=services.player.position;services.player.active=true;}
        if(world_collision && frame==202){services.player.position[0]+=150;services.player.collision_position=services.player.position;}
        if(world_collision && frame==240)services.player.active=false;
        if(frame==200)services.closing_scene=&group;
        group.Advance(1.f/60,1);services.closing_scene=nullptr;
        if(real_player && group.At(4).State().words[0x6e4/4]==MonsterPlayerView::Token(0))player_detected=true;
        if(real_player && player_detected){const auto pursuit=group.At(4).State().words[0x6e0/4];if(pursuit==2)return_started=true;else if(return_started && !pursuit)returned=true;}
        for(std::size_t i=0;i<group.Size();++i)Check(!group.At(i).Frame().Failed());
    }
    if(scene=="1" && (world_name=="M01" || world_name=="M02"))Check(max_members>=2);
    if(world_collision && !collision_queries)std::cerr<<"no world collision query; pursuit="<<group.At(4).State().words[0x6e0/4]<<" motion="<<group.At(4).Motion().MovementActive()<<'\n';
    if(world_collision)Check(collision_queries>0);
    if(real_player)Check(player_detected && services.targets>0 && return_started && returned);
    std::ofstream out(report);out<<"{\"world\":"<<std::quoted(world_name)<<",\"scene\":"<<std::quoted(scene)<<",\"frames\":600,\"controlled_services\":true";if(world_collision)out<<",\"original_world_collision\":true,\"collision_queries\":"<<collision_queries<<",\"collision_hits\":"<<collision_hits;if(real_player)out<<",\"player_detected\":true,\"target_queries\":"<<services.targets<<",\"return_completed\":true";out<<",\"max_members\":"<<max_members<<",\"monsters\":[";
    for(std::size_t i=0;i<group.Size();++i){if(i)out<<',';out<<"{\"index\":"<<i<<",\"behavior\":";group.At(i).Behavior().WriteReport(out);out<<'}';}
    out<<"]}";Check(bool(out));
    if(real_player || world_collision)Check(services.rejected_scene_close>0);
    if(real_player){
        std::vector<std::size_t> order;for(std::size_t i=group.Size();i>0;--i)order.push_back(i-1);
        Reject([&]{MonsterCombatCollector invalid(actors,group,*player_view,mesh,{0});});
        MonsterCombatCollector collector(actors,group,*player_view,mesh,order);
        constexpr std::size_t initiator=4;auto& current=group.At(initiator);
        for(std::size_t i=0;i<group.Size();++i)if(i!=initiator)group.At(i).State().words[0x1f0/4]=0;
        group.At(0).State().words[0x1f0/4]=1;group.At(0).State().words[0x6e0/4]=0;
        current.State().words[0x6e4/4]=MonsterPlayerView::Token(0);
        collector.Collect(initiator);
        const auto records=current.Encounter().Records();const auto& source_record=actors.Source(current.Handle());
        Check(records.size()==source_record.values.size());
        for(std::size_t i=0;i<records.size();++i){
            const auto key=std::string("AAB_MST_ESSBEH_combat_pos_")+std::to_string(i);const auto* value=source_record.cache.Root().FindAttribute(key);
            Check(records[i][0]==source_record.values[i] && records[i][1]==(value?value->ToInt(0):0) && records[i][2]==current.State().words[0x730/4]);
        }
        Check(current.Encounter().Names()==std::vector<std::string>{current.Label()});
        const auto& peer=actors.Source(group.At(0).Handle());const auto* peer_script=peer.cache.Root().FindAttribute("AAB_MST_ESSBEH_script_func");
        Check(!peer_script || (std::get_if<std::string>(&peer_script->value) && std::get<std::string>(peer_script->value).empty()));
        Check(std::bit_cast<float>(source_record.fields[3])>=0);
        current.State().words[0x718/4]=current.State().words[0x714/4];current.State().words[0x728/4]=current.State().words[0x724/4];
        group.At(0).State().words[0x6e0/4]=1;actors.SetMotionPosition(group.At(0).Handle(),actors.MotionTransform(current.Handle()).position);
        CollisionMesh no_walls;MonsterCombatCollector adjacent(actors,group,*player_view,no_walls,order);adjacent.Collect(initiator);
        Check(current.Encounter().Names()==std::vector<std::string>({current.Label(),group.At(0).Label()}));
        Check(current.Encounter().Records().size()==source_record.values.size()+peer.values.size());
        current.State().words[0x718/4]=current.State().words[0x714/4];current.State().words[0x728/4]=current.State().words[0x724/4];
        auto distant=actors.MotionTransform(current.Handle()).position;distant[0]+=std::bit_cast<float>(source_record.fields[3])+1000.f;actors.SetMotionPosition(group.At(0).Handle(),distant);
        adjacent.Collect(initiator);Check(current.Encounter().Names()==std::vector<std::string>{current.Label()});current.Encounter().Close();
        auto& encounter_actor=group.At(0);actors.SetMotionPosition(encounter_actor.Handle(),peer.position);
        for(std::size_t i=1;i<group.Size();++i)group.At(i).State().words[0x1f0/4]=0;
        auto close_player=actors.MotionTransform(encounter_actor.Handle()).position;close_player[0]+=.5f;players.SetPosition(0,close_player);
        encounter_actor.State().words[0x6e0/4]=0;encounter_actor.State().words[0x704/4]=0;encounter_actor.State().words[0x1f0/4]=1;
        MonsterInteractionRuntimeStub script_services;
        MonsterInteraction actual_interaction(actors,encounter_actor.Handle(),encounter_actor.State(),encounter_actor.ManagerToken(),encounter_actor.Behavior().SourceToken(),*player_view,script_services);
        MonsterCombatCollector actual_collector(actors,group,*player_view,mesh);
        const auto database=ReadOriginalDatabasePackage(game);
        FloorAttributes floor;CollisionTriangle sample{};sample.attributes_known=true;sample.rgba=std::array<std::uint32_t,3>{0x19000000,0x19000000,0x19000000};floor.Update(sample);
        const auto resolved=ResolveCombatArea(database.combat_areas,world_name,floor.CurrentAlpha(),floor.FallbackAlpha());
        if(world_name=="M01")Check(resolved=="M01");
        struct HandoffServices final:IMonsterCombatHandoffServices {
            MonsterSceneActor& actor;const std::vector<OriginalCombatArea>& areas;const FloorAttributes& floor;std::string expected;std::int32_t state{0};bool document{},fail_enter{};unsigned entered{},destroyed{},world_mode{};std::string arena;std::vector<std::string> names;std::vector<std::array<std::uint32_t,3>> monsters;
            HandoffServices(MonsterSceneActor& value,const std::vector<OriginalCombatArea>& records,const FloorAttributes& attributes,std::string result):actor(value),areas(records),floor(attributes),expected(result.empty()?"x":std::move(result)){}
            std::optional<std::int32_t> CurrentStateId() override{return state;}
            void CreateDocument(std::string_view root) override{Check(!document && root=="CombatParam");document=true;}
            std::string ResolveArenaPath(std::string_view name) override{arena=name;return ResolveCombatArea(areas,name,floor.CurrentAlpha(),floor.FallbackAlpha());}
            void ClearStateNames() override{names.clear();}
            void AddStateName(std::string_view name) override{names.emplace_back(name);}
            std::size_t StateNameCount() override{return names.size();}
            void SetCombatWorld(std::string_view node,std::string_view path,std::uint32_t mode,std::int32_t count) override{Check(document && node=="combatWorld" && path==expected && mode<=2 && count==1);world_mode=mode;}
            void SetCombatMonster(std::uint32_t id,std::uint32_t value,std::uint32_t group) override{monsters.push_back({id,value,group});}
            void EnterState(std::uint32_t id) override{Check(document && id==3 && actor.Encounter().Names().empty() && actor.Encounter().Records().empty());++entered;if(fail_enter)throw std::runtime_error("controlled combat entry failure");}
            std::int32_t DestroyDocument() noexcept override{document=false;++destroyed;return 17;}
        } handoff_services(encounter_actor,database.combat_areas,floor,resolved);
        MonsterCombatHandoff handoff(handoff_services);Check(handoff.Process(encounter_actor,-1,world_name)==0 && !handoff_services.document);handoff_services.state=1;
        encounter_actor.Encounter().Append({123,4,2});encounter_actor.Encounter().AddName(encounter_actor.Label());handoff_services.fail_enter=true;
        Reject([&]{(void)handoff.Process(encounter_actor,2,world_name);});
        Check(!handoff_services.document && handoff_services.destroyed==1 && handoff_services.world_mode==1 && handoff_services.monsters==std::vector<std::array<std::uint32_t,3>>({{123,4,1}}));
        Check(encounter_actor.Encounter().Names().empty() && encounter_actor.Encounter().Records().empty());
        handoff_services.fail_enter=false;handoff_services.entered=handoff_services.destroyed=0;handoff_services.monsters.clear();
        services.interaction=&actual_interaction;services.collector=&actual_collector;services.handoff=&handoff;services.encounter_group=&group;services.arena_name=world_name;services.encounter_state=&encounter_actor.State();services.encounter_manager=encounter_actor.ManagerToken();services.encounter_index=0;
        for(unsigned step=0;step<10 && services.combat.empty();++step)group.Advance(1.f/60,1);
        Check(services.combat==std::vector<unsigned>({1,2}) && handoff_services.names==std::vector<std::string>{encounter_actor.Label()});
        Check(handoff_services.arena==world_name && handoff_services.entered==1 && handoff_services.destroyed==1 && handoff_services.monsters.size()==peer.values.size());
        for(std::size_t i=0;i<peer.values.size();++i){const auto key=std::string("AAB_MST_ESSBEH_combat_pos_")+std::to_string(i);const auto* value=peer.cache.Root().FindAttribute(key);Check(handoff_services.monsters[i][0]==peer.values[i] && handoff_services.monsters[i][1]==(value?value->ToInt(0):0) && handoff_services.monsters[i][2]==(encounter_actor.State().words[0x730/4]==2?1u:0u));}
        const auto mode=encounter_actor.State().words[0x700/4];Check(handoff_services.world_mode==(mode==1?2u:mode==2?1u:0u));encounter_actor.Encounter().Close();
        services.interaction=nullptr;services.collector=nullptr;services.handoff=nullptr;services.encounter_group=nullptr;services.encounter_state=nullptr;services.combat.clear();
    }
    group.At(0).State().words[0x1f0/4]=1;services.fail_visit=true;
    Reject([&]{group.Advance(1.f/60,1);});services.fail_visit=false;
    Reject([&]{group.Advance(1.f/60,1);});
    Reject([&]{group.Close();});group.Close();Reject([&]{(void)group.At(0);});
    {
        const auto h=actors.Handle(0);
        const auto radius=[&actors,h]{return actors.LocalSphere(h)[3];};
        const auto ids=[&identifiers]{return identifiers++;};
        const auto collision=[&](auto a,auto b){return world_geometry && WorldSegmentBlocked(*world_geometry,a,b);};
        Reject([&]{MonsterSceneActor bad(world,actors,h,orientation,radius,ids,actor_package,0x700000,0,0,1,floor,mesh,2.f,collision,services);});
        Check(!actors.TransformBound(h));
        Reject([&]{MonsterSceneActor bad(world,actors,h,orientation,radius,ids,actor_package,0x700000,0x800000,0,1,floor,mesh,2.f,{},services);});
        Check(!actors.TransformBound(h));
        MonsterSceneActor owned(world,actors,h,orientation,radius,ids,actor_package,0x700000,0x800000,0,1,floor,mesh,2.f,collision,services);
        owned.Behavior().SetSchedulingEnabled(true);
        owned.State().words[0x1f0/4]=1;services.closing=&owned;
        std::array<MonsterPursuitFrame*,1> active{&owned.Frame()};
        MonsterSceneFrame scheduler(world,movement,[](auto){return std::vector<MonsterMovementObstacle>{};});scheduler.Advance(1.f/60,1,active);
        Check(services.rejected_close==1 && !owned.Frame().Failed());services.closing=nullptr;
        if(real_player){
            MonsterInteractionRuntimeStub controlled_scripts;
            MonsterInteraction interaction(actors,h,owned.State(),0x700000,owned.Behavior().SourceToken(),*player_view,controlled_scripts);
            const auto token=MonsterPlayerView::Token(0);
            Check(!interaction.Process(0));players.SetPosition(0,{100000,0,100000});Check(!interaction.Process(token));
            const auto* script=actors.Source(h).cache.Root().FindAttribute("AAB_MST_ESSBEH_script_func");
            Check(!script || (std::get_if<std::string>(&script->value) && std::get<std::string>(script->value).empty()));
            Check(std::bit_cast<float>(owned.State().words[0xdc/4])>0);
            auto nearby=actors.MotionTransform(h).position;nearby[0]+=0.5f;players.SetPosition(0,nearby);
            Check(interaction.Process(token));services.interaction=&interaction;services.encounter_state=&owned.State();services.encounter_manager=0x700000;
            for(unsigned step=0;step<10 && services.combat.empty();++step)scheduler.Advance(1.f/60,1,active);
            Check(services.combat==std::vector<unsigned>{1,2} && !owned.State().words[0x1f0/4]);
            services.interaction=nullptr;services.encounter_state=nullptr;
        }
        owned.Encounter().Append({101,2,3});owned.Encounter().AddName(actors.Source(h).name);
        owned.Close();owned.Close();
        Check(!actors.TransformBound(h));Reject([&]{(void)owned.Frame();});
    }
    Check(world.Stats().helpers==0 && world.Stats().members==0 && world.Stats().associations==0);world.Close();
}
void ProximitySource(const std::filesystem::path& game,const std::filesystem::path& cache,const std::string& world,const std::string& scene,const std::filesystem::path& report,SourceProbe mode){
    const bool component_probe=mode==SourceProbe::component_lifetime,members_probe=mode==SourceProbe::members_lifetime || component_probe,base_probe=mode==SourceProbe::base_lifetime,initialized_probe=mode==SourceProbe::initialization,paths_probe=mode==SourceProbe::paths,resource_probe=mode==SourceProbe::resource || initialized_probe || base_probe || members_probe,frame_probe=mode==SourceProbe::frame || resource_probe;
    std::ifstream actor_file(game/"palactor.cpk",std::ios::binary);PackageReader actor_package(actor_file);
    if(resource_probe){
        Check(actor_package.Find("101\\101_shadow.dff").has_value());
        for(const std::string name:{"101","missing_original_model"}){
            auto invalid=Records();invalid[0].resource=name;SceneMonsterInstances fixture(99,invalid,[](auto){return Model();},[](auto)->std::optional<std::vector<std::uint8_t>>{return {};});MonsterBehaviorManagerState image{};image.words[5]=1;
            const auto before=image.words;Reject([&]{ActorResourceFrame unsupported(fixture,fixture.Handle(0),image,actor_package);});Check(image.words==before);
        }
    }
    std::ifstream source(game/"scenedata.cpk",std::ios::binary);PackageReader package(source);const auto data=package.Read(world+"\\"+scene+"\\mstInfo.mst");
    std::istringstream stream(std::string(reinterpret_cast<const char*>(data.data()),data.size()));const auto records=ReadSceneMonsters(stream);
    const bool direct=cache=="--original";std::shared_ptr<ArenaSource> original;if(direct)original=std::make_shared<ArenaSource>(game);
    std::vector<ScenePath> path_records;
    if(frame_probe){const auto bytes=package.Read(world+"\\"+scene+"\\pathway.pwy");std::istringstream input(std::string(bytes.begin(),bytes.end()));path_records=ReadScenePaths(input);}
    std::unique_ptr<MonsterSceneResources> staged;
    std::unique_ptr<SceneMonsterInstances> cached_actors;
    if(direct)staged=std::make_unique<MonsterSceneResources>(7,original,records,path_records);
    else cached_actors=std::make_unique<SceneMonsterInstances>(7,records,[&](std::string_view model){std::ifstream file(cache/"arena"/world/scene/"monsters"/(std::string(model)+".p4a"),std::ios::binary);return ReadActor(file);},[](std::string_view)->std::optional<std::vector<std::uint8_t>>{return {};});
    auto& actors=direct?staged->Monsters():*cached_actors;
    CollisionMesh mesh;if(original)mesh=original->ReadOriginalCollision(world,scene);else{std::ifstream collision(cache/"arena"/world/scene/"collision.p4c",std::ios::binary);mesh=ReadCollision(collision);}CollisionSurface floor(mesh.floor);floor.EnableNavigation();
    std::shared_ptr<ScenePathways> original_paths;
    if(frame_probe)original_paths=direct?staged->Paths():std::make_shared<ScenePathways>(7,path_records);
    std::ofstream out(report);out<<std::setprecision(9)<<"{\"world\":"<<std::quoted(world)<<",\"scene\":"<<std::quoted(scene)<<",\"controlled_state\":true,\"monsters\":[";
    for(std::size_t i=0;i<actors.Size();++i){const auto h=actors.Handle(i);const auto& record=actors.Source(h);const auto pose=actors.MotionTransform(h);MonsterBehaviorManagerState state{};
        const auto token=0x10000u+static_cast<std::uint32_t>(i)*0x10000u;if(!members_probe)state.words[0x654/4]=0x900000;
        std::vector<std::uint32_t> initialized;
        std::optional<SourceBaseOwner> base_owner;
        std::unique_ptr<SourceMovementOwner> member_owner;
        if(members_probe){
            member_owner=std::make_unique<SourceMovementOwner>(actors,h,mesh,original_paths,&state,token,component_probe);
            Check(state.words[0]==0x842c98 && state.words[0x654/4]==0 && state.words[0x6cc/4]==token && state.words[0x6dc/4]==token);
            if(!component_probe)state.words[0x654/4]=0x900000; // 旧独立探针的受控身份。
            BindConstructedActorResource(actors,h,state,token,0x200000u+static_cast<std::uint32_t>(i)*8,0,1,actor_package);
            initialized.assign(state.words.begin(),state.words.begin()+92);
        }else if(base_probe){
            base_owner.emplace(state,token);
            BindConstructedActorResource(actors,h,state,token,0x200000u+static_cast<std::uint32_t>(i)*8,0,1,actor_package);
            Check(state.words[0]==0x8433d8 && state.words[0x654/4]==0x900000 && base_owner->storage.AllocationCount()==3);
            initialized.assign(state.words.begin(),state.words.begin()+92);
        }else if(initialized_probe){
            InitializeActorResource(actors,h,state,token,0x200000u+static_cast<std::uint32_t>(i)*8,0,1,actor_package);
            Check(state.words[9]==1 && state.words[39]==std::bit_cast<std::uint32_t>(1.f) && state.words[0x654/4]==0x900000);
            // 只证明主资源绑定，不将受控图像冒充完整怪物构造。
            GameResourceManager empty;(void)empty.Construct();MonsterBehaviorManagerState constructed{};
            std::copy(empty.words.begin(),empty.words.end(),constructed.words.begin());
            constructed.words[0]=0x842c98;constructed.words[92]=0x842c8c;constructed.words[0x654/4]=0x900000;
            const unsigned retained[]{0,1,14,37,38,39,76,91};
            for(unsigned field:retained)if(field)constructed.words[field]=0xa00000+field*4;
            const auto unbound=constructed.words;auto expected=state.words;
            for(unsigned field:retained)expected[field]=unbound[field];expected[92]=unbound[92];
            BindConstructedActorResource(actors,h,constructed,token,0x200000u+static_cast<std::uint32_t>(i)*8,0,1,actor_package);
            Check(constructed.words==expected);const auto bound=constructed.words;
            Reject([&]{BindConstructedActorResource(actors,h,constructed,token,0x200000,0,1,actor_package);});Check(constructed.words==bound);
            for(unsigned field:{0u,5u,7u,9u,46u,51u}){
                constructed.words=unbound;constructed.words[field]=field==0?0:2;const auto invalid=constructed.words;
                Reject([&]{BindConstructedActorResource(actors,h,constructed,token,0x200000,0,1,actor_package);});Check(constructed.words==invalid);
            }
            initialized.assign(state.words.begin(),state.words.begin()+92);const auto before=state.words;
            Reject([&]{InitializeActorResource(actors,h,state,token,0x200000,0,1,actor_package);});Check(before==state.words);
            if(!i){auto old_model=actors.Model(h);old_model.bind_vertices.reset();const std::array<SceneMonster,1> one{record};SceneMonsterInstances missing(99,one,[&](auto){return old_model;},[](auto)->std::optional<std::vector<std::uint8_t>>{return {};});MonsterBehaviorManagerState failed{};failed.words[92]=0xabcdef;const auto original=failed.words;
                Reject([&]{InitializeActorResource(missing,missing.Handle(0),failed,token,0x200000,0,1,actor_package);});Check(failed.words==original);
                failed.words=unbound;const auto constructed_before=failed.words;
                Reject([&]{BindConstructedActorResource(missing,missing.Handle(0),failed,token,0x200000,0,1,actor_package);});Check(failed.words==constructed_before);
                failed.words=original;
                Reject([&]{InitializeActorResource(actors,h,failed,1,0x200000,0,1,actor_package);});Check(failed.words==original);
            }
        }else{state.words[5]=1;for(unsigned a=0;a<3;++a){state.words[43+a]=std::bit_cast<std::uint32_t>(pose.position[a]);state.words[40+a]=std::bit_cast<std::uint32_t>(pose.rotation[a]);state.words[46+a]=std::bit_cast<std::uint32_t>(1.f);state.words[56+a]=state.words[43+a];}}
        std::unique_ptr<ActorResourceFrame> resource;
        if(resource_probe){
            resource=std::make_unique<ActorResourceFrame>(actors,h,state,actor_package);
            state.words[9]=0;Check(!resource->Update(state,0));state.words[9]=1;state.words[6]=1;Check(!resource->Update(state,0));state.words[7]=1;Check(resource->Update(state,0));state.words[6]=0;
            auto wrong=state;Reject([&]{(void)resource->Update(wrong,0);});Reject([&]{(void)resource->Update(state,-1);});Check(!resource->Failed());
        }
        std::optional<SourceCollisionOwner> collision_owner;if(initialized_probe)collision_owner.emplace(state);
        MonsterBaseMotion motion(actors,h,state,token,floor,2.f,[](auto,auto){return false;},[&](auto& image,float elapsed){if(resource)(void)resource->Update(image,elapsed);},member_owner?&member_owner->storage:base_owner?&base_owner->storage:collision_owner?&collision_owner->storage:nullptr);
        Check(state.words[0x170/4+1]==state.words[5]);
        std::uint32_t source_token=0x900000;
        if(component_probe){
            auto& behavior=*member_owner->behavior;Check(!behavior.SourceToken() && !behavior.BehaviorChildToken() && !state.words[0x654/4]);
            behavior.InitializeComponent(motion);source_token=behavior.SourceToken();
            Check(source_token && source_token!=0x900000 && state.words[0x654/4]==source_token && state.words[0x6d0/4]==behavior.BehaviorChildToken());
            Check(state.words[0x1f0/4]==record.fields[0] && (state.words[MonsterBehaviorManager::kProcessorOffset/4+6]&0xff)==1);
            Check((behavior.BehaviorChildToken()!=0)==!record.behavior.Root().children.empty() && member_owner->storage.ReadDword(source_token+0x48));
            for(unsigned a=0;a<3;++a)Check(member_owner->storage.ReadDword(source_token+0x18+a*4)==std::bit_cast<std::uint32_t>(record.position[a]));
        }
        MonsterProximityActor player{0xa00000,true,pose.position,pose.position,2};MonsterProximity probe(actors,h,state,token,source_token,mesh,[&]{return player;});
        const auto radius=std::bit_cast<float>(record.fields[1]);const auto near=probe.Check();Check(near==(radius>=0));
        const auto start=state.words[0x6e8/4];player.position[0]+=std::abs(radius)+2000;player.collision_position=player.position;Check(!probe.Check());
        const auto unchanged=actors.MotionTransform(h);player.active=false;Check(!probe.Check());
        Check(unchanged.position==pose.position && actors.Source(h).position==record.position);
        bool accepted{};unsigned speed_bits{};Point3 final_position=pose.position;
        if(paths_probe){
            Check(actors.SelectAction(h,"Z01"));state.words[0x1f0/4]=1;
            MonsterPursuitTarget chase{true,{pose.position[0]+10,pose.position[1],pose.position[2]}};
            MonsterPursuitPaths paths(actors,h,state,token,0x900000,motion,floor,[&](auto id){Check(id==player.address);return chase;},[]{throw std::logic_error("unexpected reset in source pursuit path probe");return 0u;});
            accepted=paths.Process(0,player.address);speed_bits=state.words[(0x564+36)/4];
            if(accepted){for(unsigned frame=0;frame<20;++frame)(void)motion.Update(1.f/60,1);}
            final_position=actors.MotionTransform(h).position;
            for(unsigned a=0;a<3;++a){Check(std::bit_cast<float>(state.words[43+a])==final_position[a]);}
        }
        std::ostringstream frame_report;
        if(frame_probe){
            Check(actors.SelectAction(h,"Z01"));state.words[0x1f0/4]=1;state.words[0x6f8/4]=std::bit_cast<std::uint32_t>(1.f);
            if(!component_probe)state.words[0x6d0/4]=record.behavior.Root().children.empty()?0u:0x900100u;
            std::unique_ptr<SourceMovementOwner> standalone;if(!member_owner)standalone=std::make_unique<SourceMovementOwner>(actors,h,mesh,original_paths);
            auto& actual=member_owner?*member_owner:*standalone;auto& movement_world=actual.world;auto& movement=actual.movement;auto& memory=actual.memory;auto& behavior=*actual.behavior;
            struct Services final:IMonsterPursuitSceneServices {
                MonsterProximityActor player;unsigned actions{},visits{};
                explicit Services(Point3 position):player{0xa00000,false,position,position,2}{}
                MonsterProximityActor ControlledActor() override {return player;}
                MonsterPursuitTarget Target(std::uint32_t id) override {Check(id==player.address);return {player.active,player.position};}
                bool PlayerControlBusy() override {return false;}
                bool ProcessObjectInteraction(MonsterBehaviorManagerState&,std::uint32_t,std::uint32_t,float) override {return false;} // 独立探针明确禁用交互。
                void DispatchCombatPositions(std::uint32_t,float) override {throw std::logic_error("unexpected source probe combat dispatch");}
                void InitializeCombat(std::uint32_t,std::int32_t) override {throw std::logic_error("unexpected source probe combat initialization");}
                void ClearHistoryRecord(std::uint32_t) override {throw std::logic_error("unexpected source probe history");}
                void ProcessActionWithScript(std::uint32_t,float) override {++actions;} // 受控探针，不执行场景脚本。
                void VisitArenaObjects(std::uint32_t) override {++visits;}
            } services(pose.position);
            MonsterPursuitFrame frame(actors,h,state,token,source_token,state.words[0x6d0/4],behavior,motion,floor,mesh,services);
            for(std::int32_t id=0;id<4;++id)Check(behavior.FactoryAddress(id)==MonsterBehaviorManager::kFactories[static_cast<std::size_t>(id)]);Check(!behavior.FactoryAddress(4));
            behavior.SetSchedulingEnabled(true);
            unsigned frames{};const auto tick=[&]{movement_world.UpdateMembers(1.f/60,[&](auto helper,float dt){Check(movement.Update(helper,{},dt).active);});++frames;(void)frame.AdvanceAfterGrid(1.f/60,1);Check(state.words[MonsterBehaviorManager::kProcessorOffset/4+4]==behavior.ProcessorMemoryToken() && &behavior.ProcessorMemory()==&memory);Check(state.words[(MonsterBehaviorManager::kScriptOffset+4)/4] && state.words[(MonsterBehaviorManager::kScriptOffset+8)/4]==behavior.TaskCount() && (state.words[(MonsterBehaviorManager::kScriptOffset+12)/4]&0xffu)==std::uint32_t(behavior.Ready()));};
            for(unsigned k=0;k<4;++k)tick();
            if(actors.Behavior(h).fuzzy){for(unsigned k=0;k<1200 && !behavior.Stats().entered;++k)tick();Check(behavior.Stats().entered>0);}
            services.player.position=actors.MotionTransform(h).position;services.player.position[0]+=10;services.player.collision_position=services.player.position;services.player.active=true;tick();Check(state.words[0x6e0/4]==1 && !behavior.SchedulingEnabled());
            tick();tick();state.words[0x704/4]=1;tick();Check(state.words[0x6e0/4]==2);services.player.active=false;
            for(unsigned k=0;k<600 && state.words[0x6e0/4];++k)tick();
            Check(!state.words[0x6e0/4] && behavior.SchedulingEnabled()==bool(state.words[0x6d0/4]) && !frame.Failed());behavior.Cancel();
            frame_report<<",\"pursuit_frames\":"<<frames<<",\"actions\":"<<services.actions<<",\"visits\":"<<services.visits<<",\"frame_behavior\":";behavior.WriteReport(frame_report);
        }
        if(i)out<<',';out<<"{\"index\":"<<i<<",\"radius_bits\":"<<record.fields[1]<<",\"near\":"<<(near?"true":"false")<<",\"far\":false,\"inactive\":false,\"saved_x_bits\":"<<start;
        if(paths_probe)out<<",\"path_accepted\":"<<(accepted?"true":"false")<<",\"speed_bits\":"<<speed_bits<<",\"final_position_bits\":["<<std::bit_cast<std::uint32_t>(final_position[0])<<','<<std::bit_cast<std::uint32_t>(final_position[1])<<','<<std::bit_cast<std::uint32_t>(final_position[2])<<']';out<<frame_report.str();
        if(resource){const auto stats=resource->Stats();Check(stats.updated>0 && stats.skipped==2);out<<",\"resource_calls\":"<<stats.calls<<",\"resource_updated\":"<<stats.updated<<",\"resource_skipped\":"<<stats.skipped;
            state.words[38]=1;Reject([&]{(void)resource->Update(state,0);});Check(resource->Failed());state.words[38]=0;Reject([&]{(void)resource->Update(state,0);});}
        if(initialized_probe || base_probe || members_probe){out<<",\"model\":"<<std::quoted(record.resource)<<",\"initial_words\":[";for(std::size_t k=0;k<initialized.size();++k){if(k)out<<',';out<<initialized[k];}out<<"],\"source_position\":["<<record.position[0]<<','<<record.position[1]<<','<<record.position[2]<<"],\"source_rotation\":["<<record.rotation[0]<<','<<record.rotation[1]<<','<<record.rotation[2]<<']';}
        out<<'}';
    }
    out<<"]}";Check(bool(out));
}
void ResourceFrameGates(){
    struct Runtime final:IGameResourceFrameRuntime,IRwDictSchemaUpdateRuntime {
        std::vector<unsigned> order;float elapsed{};unsigned materials=2;
        void RenderCameraScene(std::uint32_t*,float time,std::uint32_t*) override {Check(time==.25f);order.push_back(99);}
        std::uint32_t ReadWord(std::uint32_t address) override {Check(address>=0x100 && (address-0x100)/4<materials);return 10+(address-0x100)/4;}
        std::uint32_t ListBegin(std::uint32_t list) override {Check(list==1);return 0x100;}
        std::uint32_t ListEnd(std::uint32_t list) override {Check(list==1);return 0x100+materials*4;}
        void AdvanceUVAnim(std::uint32_t material,float time) override {Check(time==.5f);elapsed=time;order.push_back(material*2);if(material==10)materials=3;}
        void ApplyUVAnim(std::uint32_t material) override {order.push_back(material*2+1);}
    } runtime;
    GameResourceManager resource;resource.words[38]=1;resource.words[39]=std::bit_cast<std::uint32_t>(2.f);
    Check(!resource.UpdateAndRenderClump(.25f,runtime,runtime) && runtime.order.empty());
    resource.words[9]=1;resource.words[6]=1;Check(!resource.UpdateAndRenderClump(.25f,runtime,runtime) && runtime.order.empty());
    resource.words[7]=1;resource.words[11]=1;resource.words[12]=2;resource.words[13]=3;
    Check(resource.UpdateAndRenderClump(.25f,runtime,runtime) && runtime.order==std::vector<unsigned>{20,21,22,23,24,25,99});
    runtime.order.clear();resource.words[6]=0;resource.words[7]=0;resource.words[38]=0;resource.words[12]=0;
    Check(resource.UpdateAndRenderClump(.25f,runtime,runtime) && runtime.order.empty());
}
void GridBoundary(){
    Fixture f;for(auto& b:f.behavior)Check(b->Start());f.Tick(0);std::vector<MonsterHelperHandle> visited;
    const auto extra=f.world.CreateEndpoint(f.actors.Handle(0),99);Check(!f.world.Registered(extra));
    f.world.UpdateMembers(.125f,[&](auto h,float elapsed){
        visited.push_back(h);Check(elapsed==.125f && f.world.InGridFrame() && f.world.Contains(h));
        Reject([&]{f.world.UpdateMembers(0,[](auto,float){});});Reject([&]{f.world.Close();});Reject([&]{f.world.DestroyHelper(h);});
        Reject([&]{(void)f.world.CreateEndpoint(f.actors.Handle(0),100);});
        Reject([&]{f.world.Bind(f.world.Helper(h),f.world.GridParameter());});Reject([&]{f.world.AddElement(f.world.Helper(extra));});
        Reject([&]{f.behavior[0]->AdvanceAfterGrid(0);});Reject([&]{f.behavior[0]->Cancel();});Check(!f.behavior[0]->Failed());
        (void)f.world.QuerySphere({0,0,0},1);(void)f.world.UpdatePosition(f.world.Helper(h).words[42],{1,2,3});
    });Check(visited.size()==2 && visited[0].token<visited[1].token && !f.world.InGridFrame());
    unsigned count{};Reject([&]{f.world.UpdateMembers(0,[&](auto,float){if(++count==2)throw std::runtime_error("controlled grid failure");});});
    Check(count==2 && !f.world.InGridFrame());Reject([&]{f.world.UpdateMembers(-1,[](auto,float){});});Reject([&]{f.world.UpdateMembers(0,{});});
    f.world.DestroyHelper(extra);for(auto& b:f.behavior)b->Cancel();Check(f.world.Stats().helpers==0 && !f.world.Contains(visited[0]));f.world.Close();Check(!f.world.Contains(visited[1]));
}

void ComponentSourceViews(){
    auto record=Records().front();record.values={2501,2502};record.fields[4]=2;ObjectMemory storage;
    detail::MonsterComponentSource source(storage,record);const auto token=source.Token(),document=source.Document(token),child=source.ChildToken();
    Check(token && document && child && token!=document && document!=child && storage.AllocationCount()==5 && &source.Record()==&record);
    Check(source.FirstChild(document,0)==child && storage.ReadDword(token+0x30)==record.fields[0] && storage.ReadDword(token+0x40)==2);
    Reject([&]{source.Document(token+4);});Reject([&]{source.FirstChild(document,1);});
    storage.WriteDword(token+0x48,123);Reject([&]{source.Record();});storage.WriteDword(token+0x48,document);
    const auto values=storage.ReadDword(token+0x44);Check(storage.ReadDword(values)==2501);storage.WriteDword(values,7);Reject([&]{source.Record();});storage.WriteDword(values,2501);
    Check(source.Token()==token);source.Close();source.Close();Check(storage.AllocationCount()==0);Reject([&]{source.Token();});
    for(unsigned budget:{64u,100u,180u,240u,260u}){ObjectMemory limited({0x10000000,0x70000000,budget,100});Reject([&]{detail::MonsterComponentSource failure(limited,record);});Check(limited.AllocationCount()==0);}
    SceneMonster empty;empty.name="empty";empty.resource="401";detail::MonsterComponentSource no_child(storage,empty);Check(!no_child.ChildToken() && !no_child.FirstChild(no_child.Document(no_child.Token()),0));no_child.Close();Check(storage.AllocationCount()==0);
    record.fields[4]=3;Reject([&]{detail::MonsterComponentSource invalid(storage,record);});Check(storage.AllocationCount()==0);
}

void ComponentMovementInitialization(){
    Fixture f;f.behavior[0].reset();const auto actor=f.actors.Handle(0);ObjectMemory storage;MonsterBehaviorManagerState state{};
    MonsterMovementBehavior owner(f.world,f.actors,actor,f.memory[0],f.orientation,[]{return 2.f;},[&]{return f.identifiers++;},state,0x10000,storage,MonsterBehaviorPreparation::Component);
    Check(owner.ProcessorAllocationCount()==1 && storage.AllocationCount()==4 && !owner.SourceToken() && !owner.BehaviorChildToken());
    const auto& record=f.actors.Source(actor);state.words[5]=0x200000;for(unsigned a=0;a<3;++a)state.words[43+a]=std::bit_cast<std::uint32_t>(record.position[a]);
    CollisionSurface surface(f.floor_geometry);surface.EnableNavigation();
    {
        MonsterBaseMotion motion(f.actors,actor,state,0x10000,surface,2,[](auto,auto){return false;},[](auto&,float){},&storage);
        f.actors.SetMotionPosition(actor,{5,2,105});f.actors.SetMotionRotation(actor,{0,45,0});state.words[0x704/4]=1;state.words[0x730/4]=7;state.words[0x734/4]=8;state.words[0x738/4]=9;
        owner.InitializeComponent(motion,true);const auto source=owner.SourceToken();
        Check(source && source==state.words[0x654/4] && owner.BehaviorChildToken()==state.words[0x6d0/4] && owner.BehaviorChildToken());
        Check(f.actors.MotionTransform(actor).position==record.position && f.actors.MotionTransform(actor).rotation==record.rotation && f.actors.Action(actor).empty());
        Check(state.words[0x1f0/4]==record.fields[0] && owner.SchedulingEnabled() && owner.Ready() && !owner.Stats().entered);
        for(unsigned offset:{0x704u,0x6e0u,0x730u,0x734u,0x738u})Check(!state.words[offset/4]);
        Check(storage.ReadDword(source+0x48) && owner.ProcessorAllocationCount()>1);Check(motion.Close());owner.Close();
        Check(storage.AllocationCount()==0 && !owner.SourceToken() && !owner.BehaviorChildToken() && !state.words[0x654/4] && !state.words[0x6d0/4]);
    }
    for(unsigned budget:{300u,430u}){
        ObjectMemory limited({0x10000000,0x70000000,budget,100});MonsterBehaviorManagerState image{};
        MonsterMovementBehavior failure(f.world,f.actors,actor,f.memory[0],f.orientation,[]{return 2.f;},[&]{return f.identifiers++;},image,0x20000,limited,MonsterBehaviorPreparation::Component);
        image.words[5]=0x300000;for(unsigned a=0;a<3;++a)image.words[43+a]=std::bit_cast<std::uint32_t>(f.actors.MotionTransform(actor).position[a]);
        MonsterBaseMotion motion(f.actors,actor,image,0x20000,surface,2,[](auto,auto){return false;},[](auto&,float){},&limited);
        Reject([&]{failure.InitializeComponent(motion);});Check(failure.Failed() && !failure.SourceToken());Check(motion.Close());Reject([&]{failure.Close();});Check(failure.Closed() && limited.AllocationCount()==0);
    }
}

void ConstructedMovementBehavior(){
    Fixture f;f.behavior[0].reset();const auto actor=f.actors.Handle(0);ObjectMemory storage;MonsterBehaviorManagerState state{};
    MonsterMovementBehavior owner(f.world,f.actors,actor,f.memory[0],f.orientation,[]{return 2.f;},[&]{return f.identifiers++;},state,0x10000,storage);
    Check(state.words[0]==0x842c98 && state.words[0x6cc/4]==0x10000 && state.words[0x6a8/4]==4);
    Check(owner.ProcessorMemoryToken()==state.words[0x668/4] && &owner.ProcessorMemory()==&f.memory[0]);
    const auto position=f.actors.MotionTransform(actor).position;state.words[5]=0x200000;state.words[0x1f0/4]=1;
    for(unsigned a=0;a<3;++a)state.words[43+a]=std::bit_cast<std::uint32_t>(position[a]);
    CollisionSurface surface(f.floor_geometry);surface.EnableNavigation();
    {
        MonsterBaseMotion motion(f.actors,actor,state,0x10000,surface,2,[](auto,auto){return false;},[](auto&,float){},&storage);
        const auto running=state.words;const auto allocations=storage.AllocationCount();Reject([&]{owner.Close();});
        Check(state.words==running && storage.AllocationCount()==allocations && !owner.Closed() && !owner.Failed());
        Check(owner.Start());unsigned guarded{};
        for(unsigned frame=0;frame<20;++frame){
            f.world.UpdateMembers(.1f,[&](auto h,float elapsed){if(!guarded++){Reject([&]{owner.Close();});}Check(f.movement.Update(h,{},elapsed).active);});
            owner.AdvanceAfterGrid(.1f);(void)motion.Update(.1f,1);
        }
        Check(guarded && owner.Stats().entered>0 && !owner.Failed());owner.Cancel();Check(owner.Ready() && owner.TaskCount()==0 && f.world.Stats().helpers==0);
        Check(motion.Close());owner.Close();owner.Close();owner.Cancel();
        Check(owner.Closed() && owner.Ready() && !owner.SchedulingEnabled() && !owner.ProcessorMemoryToken() && !owner.ProcessorAllocationCount() && storage.AllocationCount()==0);
        Check(!owner.FactoryAddress(0));Reject([&]{(void)owner.Start();});Reject([&]{(void)owner.ProcessorMemory();});std::ostringstream report;owner.WriteReport(report);Check(report.str().find("\"queue\":null")!=std::string::npos);
    }
    for(unsigned budget:{16u,32u,64u,100u,150u}){
        ObjectMemory limited({0x10000000,0x70000000,budget,100});MonsterBehaviorManagerState image{};const auto remembered=BinaryXmlConfig::Serialize(f.memory[0]);
        Reject([&]{MonsterMovementBehavior failure(f.world,f.actors,actor,f.memory[0],f.orientation,[]{return 2.f;},[&]{return f.identifiers++;},image,0x20000,limited);});
        Check(limited.AllocationCount()==0 && image.words==MonsterBehaviorManagerState{}.words && BinaryXmlConfig::Serialize(f.memory[0])==remembered);
    }
}

void AnimationBindingRollback(){
    Fixture f;const auto actor=f.actors.Handle(0);Check(f.actors.SelectAction(actor,"C02"));
    std::array<std::uint32_t,31> external{};const auto cookie=f.actors.Playback(actor).BindAnimationImage(external,0x50170,1);
    MonsterBehaviorManagerState state{};state.words[5]=1;for(unsigned a=0;a<3;++a)state.words[46+a]=std::bit_cast<std::uint32_t>(1.f);
    state.words[43]=std::bit_cast<std::uint32_t>(2.f);const auto before=state.words;const auto pose=f.actors.MotionTransform(actor);
    CollisionSurface surface(f.floor_geometry);surface.EnableNavigation();
    Reject([&]{MonsterBaseMotion fail(f.actors,actor,state,0x10000,surface,1,[](auto,auto){return false;},[](auto&,float){});});
    Check(!f.actors.TransformBound(actor) && state.words==before && f.actors.MotionTransform(actor).position==pose.position && f.actors.MotionTransform(actor).rotation==pose.rotation);
    Check(f.actors.Playback(actor).OwnsAnimationImage(cookie));f.actors.Playback(actor).UnbindAnimationImage(cookie);
    {MonsterBaseMotion success(f.actors,actor,state,0x10000,surface,1,[](auto,auto){return false;},[](auto&,float){});}
    Check(!f.actors.TransformBound(actor));const auto again=f.actors.Playback(actor).BindAnimationImage(external,0x50170,1);f.actors.Playback(actor).UnbindAnimationImage(again);
    {
        MonsterBaseMotion damaged(f.actors,actor,state,0x10000,surface,1,[](auto,auto){return false;},[](auto&,float){});
        state.words[0x170/4+19]=123;Check(!damaged.Close() && !damaged.Close());
        Check(state.words[0x170/4+19]==123 && !f.actors.TransformBound(actor));
        f.actors.Playback(actor).SetPause(false);Check(state.words[0x170/4+19]==123);
    }
}

void ConstructedBaseMotion(){
    Fixture f;const auto actor=f.actors.Handle(0);Check(f.actors.SelectAction(actor,"C02"));
    ObjectMemory storage;CollisionActorState collision{};(void)CollisionActor::Construct(collision,0,storage.Path());const auto sentinel=collision.words[21];
    MonsterBehaviorManagerState state{};state.words[5]=1;state.words[0x1f0/4]=1;
    const auto position=f.actors.MotionTransform(actor).position;
    for(unsigned a=0;a<3;++a){state.words[43+a]=std::bit_cast<std::uint32_t>(position[a]);state.words[46+a]=std::bit_cast<std::uint32_t>(1.f);}
    std::copy(collision.words.begin(),collision.words.end(),state.words.begin()+0x564/4);const auto original=state.words;
    CollisionSurface surface(f.floor_geometry);surface.EnableNavigation();
    Reject([&]{MonsterBaseMotion invalid(f.actors,actor,state,0x10000,surface,1,[](auto,auto){return false;},[](auto&,float){});});
    Check(state.words==original && storage.AllocationCount()==1);
    // 已有动画绑定使构造失败时，不可销毁借用的碰撞成员。
    std::array<std::uint32_t,31> other{};const auto cookie=f.actors.Playback(actor).BindAnimationImage(other,0x50170,1);
    Reject([&]{MonsterBaseMotion invalid(f.actors,actor,state,0x10000,surface,1,[](auto,auto){return false;},[](auto&,float){},&storage);});
    Check(state.words==original && storage.AllocationCount()==1 && !f.actors.TransformBound(actor));f.actors.Playback(actor).UnbindAnimationImage(cookie);
    {
        MonsterBaseMotion motion(f.actors,actor,state,0x10000,surface,1,[](auto,auto){return false;},[](auto&,float){},&storage);
        Check(state.words[0x564/4+21]==sentinel && storage.AllocationCount()==1);
        Point3 from=position;from[1]=0;Point3 target=from;target[0]+=10;
        Check(motion.Start(from,target,surface.Select(target)));motion.SetSpeed(5);(void)motion.Update(.1f,1);
        Check(storage.AllocationCount()>1 && state.words[0x564/4+21]==sentinel);
    }
    Check(!f.actors.TransformBound(actor) && state.words[0x564/4+21]==sentinel && state.words[0x564/4+1]==0);
    std::copy_n(state.words.begin()+0x564/4,collision.words.size(),collision.words.begin());
    (void)CollisionActor::Destroy(collision,storage.Path());Check(storage.AllocationCount()==0);
}

void BaseMotion(){
    Fixture f;const auto actor=f.actors.Handle(0);Check(f.actors.SelectAction(actor,"C02"));
    MonsterBehaviorManagerState state{};state.words[5]=1;state.words[8]=1;
    const auto initial=f.actors.MotionTransform(actor).position;
    for(unsigned a=0;a<3;++a){state.words[43+a]=std::bit_cast<std::uint32_t>(initial[a]);state.words[46+a]=std::bit_cast<std::uint32_t>(1.f);state.words[60+a]=std::bit_cast<std::uint32_t>(1.f);state.words[63+a]=std::bit_cast<std::uint32_t>(-1.f);}
    state.words[55]=std::bit_cast<std::uint32_t>(2.f);state.words[59]=state.words[55];
    state.words[40]=std::bit_cast<std::uint32_t>(11.f);state.words[42]=std::bit_cast<std::uint32_t>(33.f);
    CollisionSurface surface(f.floor_geometry);surface.EnableNavigation();unsigned resources{};bool fail{},reenter{};float before_time{};MonsterBaseMotion* current{};
    const Scene empty_world;
    auto motion_owner=std::make_unique<MonsterBaseMotion>(f.actors,actor,state,0x10000u,surface,2.f,[&](auto a,auto b){return WorldSegmentBlocked(empty_world,a,b);},[&](auto& s,float){
        Check(&s==&state && f.actors.AnimationTime(actor)==before_time);++resources;s.words[0x608/4]=77;if(reenter)(void)current->Update(0,1);if(fail)throw std::runtime_error("actual resource failure");});auto& motion=*motion_owner;current=&motion;
    Check(f.actors.TransformBound(actor));
    const auto animation_word=0x170/4;Check(state.words[animation_word+1]==state.words[5] && state.words[animation_word+3]);
    f.actors.Playback(actor).SetPause(true);Check(state.words[animation_word+19]==1);f.actors.Playback(actor).SetPause(false);Check(!state.words[animation_word+19]);
    const auto before_duplicate=state.words;
    Reject([&]{MonsterBaseMotion duplicate(f.actors,actor,state,0x20000,surface,2,[](auto,auto){return false;},[](auto&,float){});});Check(state.words==before_duplicate);
    const auto tick=[&](float elapsed){before_time=f.actors.AnimationTime(actor);return motion.Update(elapsed,1);};
    const auto inactive=tick(.1f);Check(inactive.position_result==0 && !resources && f.actors.AnimationTime(actor)==0);
    state.words[0x1f0/4]=1;state.words[0x608/4]=42;motion.SetSpeed(6);Check(state.words[0x608/4]==42);
    const auto idle=tick(.1f);Check(idle.resource_updates==1 && idle.animation_updates==1 && idle.collision_updates==1 && idle.position_result==1 && resources==1);Check(f.actors.AnimationTime(actor)>.09f && state.words[0x608/4]==77);
    Point3 from=initial;from[1]=0;const Point3 to{initial[0]+12,0,initial[2]};const auto face=surface.Select(to);Check(face && motion.Start(from,to,face));
    const auto launch=tick(.1f);Check(launch.position_result==0x14000 && f.actors.MotionTransform(actor).position==initial); // Original first node consumes its own frame.
    const auto step=tick(.1f);const auto first=f.actors.MotionTransform(actor);Check(step.position_result==0x100e0 && first.position[0]>from[0] && first.position[1]==0 && std::abs(first.rotation[1]-90)<.01f);
    Check(first.rotation[0]==11 && first.rotation[2]==33 && state.words[59]==std::bit_cast<std::uint32_t>(2.f));for(unsigned a=0;a<3;++a){Check(std::abs(std::bit_cast<float>(state.words[56+a])-first.position[a])<.001f);Check(std::bit_cast<float>(state.words[66+a])>=first.position[a] && std::bit_cast<float>(state.words[69+a])<=first.position[a]);}
    state.words[0x604/4]=1;(void)tick(.1f);Check(std::abs(f.actors.MotionTransform(actor).rotation[1]-270)<.01f);
    state.words[0x604/4]=0;
    for(unsigned frame=0;frame<100 && motion.MovementActive();++frame)(void)tick(.1f);
    (void)tick(.1f);Check(!motion.MovementActive() && std::abs(f.actors.MotionTransform(actor).position[0]-to[0])<.01f);
    Check((state.words[animation_word+18]!=0)==f.actors.Playback(actor).Completed());
    motion.Stop();const auto stopped=f.actors.MotionTransform(actor).position;(void)tick(.1f);Check(f.actors.MotionTransform(actor).position==stopped);
    struct FrameServices final:IMonsterBehaviorFrameRuntime {
        MonsterBaseMotion& motion;explicit FrameServices(MonsterBaseMotion& m):motion(m){}
        std::uint32_t ProcessManagerState(MonsterBehaviorManagerState&,std::uint32_t,float) override {return 1;} // Controlled pursuit gate; not a production service.
        void CheckStateAndProcessCollision(MonsterBehaviorManagerState&,std::uint32_t,float) override {throw std::logic_error("unexpected expiry in motion contract");}
        IScriptEventSchedulerRuntime& SchedulerRuntime() override {throw std::logic_error("owned scheduler must be used");}
        std::uint32_t UpdatePosition(std::uint32_t token,float elapsed,std::uint32_t arg) override {Check(token==0x10000 && arg==1);return motion.Update(elapsed,arg).position_result;}
    } frame(motion);
    before_time=f.actors.AnimationTime(actor);Check(f.behavior[0]->AdvanceManagerAfterGrid(state,0x10000,.1f,1,frame)==1);
    Check(state.words[0x68c/4] && state.words[0x690/4]==f.behavior[0]->TaskCount() && (state.words[0x694/4]&0xffu)==std::uint32_t(f.behavior[0]->Ready()));
    f.actors.SetMotionPosition(actor,{3,4,5});f.actors.SetMotionRotation(actor,{11,450,33});
    const auto committed=f.actors.MotionTransform(actor);
    Check(std::abs(committed.rotation[1]-90)<.01f && f.actors.InstanceTransform(actor)[13]==4);
    for(unsigned a=0;a<3;++a){Check(std::bit_cast<float>(state.words[43+a])==committed.position[a]);Check(std::bit_cast<float>(state.words[40+a])==committed.rotation[a]);Check(std::abs(std::bit_cast<float>(state.words[56+a])-committed.position[a])<.001f);}
    Check(f.actors.Source(actor).position==initial);
    state.words[0x654/4]=0x50000;CollisionMesh walls;
    MonsterProximityActor player{0x80000,true,{6,4,5},{6,4,5},2};unsigned snapshots{};MonsterProximity* probe{};bool recursive{};
    MonsterProximity proximity(f.actors,actor,state,0x10000,0x50000,walls,[&]{++snapshots;if(recursive)Reject([&]{(void)probe->Check();});return player;});probe=&proximity;
    recursive=true;Check(proximity.Check());recursive=false;
    Check(state.words[0x6e4/4]==player.address && std::bit_cast<float>(state.words[0x6e8/4])==3);
    walls.wall={{{Point3{4,-20,-20},Point3{4,20,-20},Point3{4,0,20}},1,0,0}};
    Check(!proximity.Check() && !state.words[0x6e4/4]);walls.wall.clear();
    player.position[1]=8;Check(!proximity.Check());player.position[1]=4;
    player.position[0]=1000;Check(!proximity.Check());player.position[0]=6;
    player.active=false;Check(!proximity.Check());player.active=true;Check(proximity.Check());
    const auto count=snapshots;Check(!proximity.BeyondReturnThreshold());
    f.actors.SetMotionPosition(actor,{2003,4,5});Check(proximity.BeyondReturnThreshold() && !proximity.CheckReturnTargetReached());
    f.actors.SetMotionPosition(actor,{3.01f,4,5});Check(proximity.CheckReturnTargetReached());
    Check(f.actors.MotionTransform(actor).position==Point3{3,4,5} && std::bit_cast<float>(state.words[56])==3 && snapshots==count);
    state.words[0x654/4]=0x50004;Reject([&]{(void)proximity.Check();});state.words[0x654/4]=0x50000;
    player.half_height=std::numeric_limits<float>::quiet_NaN();Reject([&]{(void)proximity.Check();});player.half_height=2;Check(proximity.Check());
    unsigned resets{},targets{};MonsterPursuitPaths* current_paths{};bool path_reentry{};
    MonsterPursuitTarget target{true,{15,0,5}};
    MonsterPursuitPaths paths(f.actors,actor,state,0x10000,0x50000,motion,surface,[&](auto id){Check(id==0x80000);++targets;if(path_reentry)Reject([&]{(void)current_paths->Process(0,id);});return target;},[&]{++resets;return 123u;});current_paths=&paths;
    Reject([&]{MonsterPursuitPaths wrong(f.actors,actor,state,0x20000,0x50000,motion,surface,[](auto){return MonsterPursuitTarget{};},[]{return 0u;});});
    Check(!paths.Process(0,0) && !targets);target.active=false;Check(!paths.Process(0,0x80000));target.active=true;
    path_reentry=true;Check(paths.Process(0,0x80000));path_reentry=false;
    Check(std::bit_cast<float>(state.words[(0x564+36)/4])==12 && motion.MovementActive());
    for(unsigned i=0;i<100 && motion.MovementActive();++i)(void)tick(.1f);
    Check(std::abs(f.actors.MotionTransform(actor).position[0]-15)<.01f);
    const auto return_result=paths.EnterReturn(0);
    Check(return_result==std::bit_cast<std::uint32_t>(12.f) && state.words[0x6e0/4]==2 && !state.words[0x6e4/4]);
    for(unsigned i=0;i<100 && motion.MovementActive();++i)(void)tick(.1f);
    Check(std::abs(f.actors.MotionTransform(actor).position[0]-3)<.01f && !resets);
    Check(proximity.CheckReturnTargetReached() && f.actors.MotionTransform(actor).position==Point3{3,4,5});
    Reject([&]{(void)paths.Process(-1,0x80000);});Check(!paths.Failed());
    target.position={1e6f,0,1e6f};Check(paths.Process(0,0x80000) && !paths.Failed());motion.Stop(); // 原Snap会把面外目标夹到地面边界。
    Reject([&]{(void)paths.Process(0,1);});Check(paths.Failed());Reject([&]{(void)paths.EnterReturn(0);});
    fail=true;Reject([&]{(void)tick(.1f);});Check(motion.Failed() && state.words[0x608/4]==77);Reject([&]{(void)tick(0);});motion.Stop();
    motion_owner.reset();Check(!f.actors.TransformBound(actor));
    MonsterBehaviorManagerState damaged=state;
    {MonsterBaseMotion protected_motion(f.actors,actor,damaged,0x30000,surface,2,[](auto,auto){return false;},[](auto& s,float){s.words[0x570/4]^=1;});
    Reject([&]{(void)protected_motion.Update(.1f,1);});Check(protected_motion.Failed());protected_motion.Stop();}
    {MonsterBaseMotion protected_transform(f.actors,actor,damaged,0x30000,surface,2,[](auto,auto){return false;},[](auto& s,float){s.words[43]=std::bit_cast<std::uint32_t>(99.f);});
    const auto before=f.actors.MotionTransform(actor).position;Reject([&]{(void)protected_transform.Update(.1f,1);});Check(protected_transform.Failed() && f.actors.MotionTransform(actor).position==before);protected_transform.Stop();}
    Check(damaged.words[(0x564+84)/4]==0 && damaged.words[(0x564+88)/4]==0 && (damaged.words[(0x564+40)/4]&0xff)==0);
    f.behavior[0].reset(); // Borrowed manager image outlives the bound behavior.
    MonsterBehaviorManagerState bad=state;bad.words[46]=std::bit_cast<std::uint32_t>(2.f);Reject([&]{MonsterBaseMotion invalid(f.actors,actor,bad,0x20000,surface,2,[](auto,auto){return false;},[](auto&,float){});});
}
void PursuitFailures(){
    auto records=Records();records[0].cache={};SceneMonsterInstances actors(1,records,[](auto){return Model();},[](auto)->std::optional<std::vector<std::uint8_t>>{return {};});const auto h=actors.Handle(0);
    std::vector<CollisionTriangle> triangles{{{Point3{-10,0,90},Point3{10,0,90},Point3{0,0,120}},1,0,0},{{Point3{490,0,90},Point3{510,0,90},Point3{500,0,120}},2,0,0}};
    CollisionSurface floor(triangles);floor.EnableNavigation();MonsterBehaviorManagerState state{};state.words[5]=1;state.words[0x654/4]=0x50000;
    for(unsigned a=0;a<3;++a){state.words[43+a]=std::bit_cast<std::uint32_t>(records[0].position[a]);state.words[46+a]=std::bit_cast<std::uint32_t>(1.f);}
    MonsterBaseMotion motion(actors,h,state,0x10000,floor,2.f,[](auto,auto){return false;},[](auto&,float){});
    MonsterPursuitTarget target{true,{4,0,100}};unsigned resets{};bool fail_reset{};
    MonsterPursuitPaths paths(actors,h,state,0x10000,0x50000,motion,floor,[&](auto){return target;},[&]{++resets;if(fail_reset)throw std::runtime_error("reset service failure");state.words[0x6e0/4]=0;return 123u;});
    Check(paths.Process(0,0x80000) && std::bit_cast<float>(state.words[(0x564+36)/4])==50);motion.Stop();
    target.position={500,0,100};Check(!paths.Process(0,0x80000) && !paths.Failed());
    for(unsigned a=0;a<3;++a){state.words[0x6e8/4+a]=std::bit_cast<std::uint32_t>(target.position[a]);}
    Check(paths.EnterReturn(0)==123 && resets==1 && state.words[0x6e0/4]==0 && !state.words[0x6e4/4]);
    fail_reset=true;Reject([&]{(void)paths.EnterReturn(0);});Check(resets==2 && paths.Failed() && state.words[0x6e0/4]==2);motion.Stop();
}
void PursuitFrames(){
    Fixture f;const auto h=f.actors.Handle(0);Check(f.actors.SelectAction(h,"Z01"));MonsterBehaviorManagerState state{};
    state.words[5]=1;state.words[0x1f0/4]=1;state.words[0x654/4]=0x50000;state.words[0x6d0/4]=0x60000;state.words[0x6f8/4]=std::bit_cast<std::uint32_t>(.1f);
    const auto initial=f.actors.MotionTransform(h).position;
    for(unsigned a=0;a<3;++a){state.words[43+a]=std::bit_cast<std::uint32_t>(initial[a]);state.words[56+a]=state.words[43+a];state.words[46+a]=std::bit_cast<std::uint32_t>(1.f);}
    CollisionSurface floor(f.floor_geometry);floor.EnableNavigation();CollisionMesh mesh;mesh.floor=f.floor_geometry;
    MonsterBaseMotion motion(f.actors,h,state,0x10000,floor,2.f,[](auto,auto){return false;},[](auto&,float){});
    struct Services final:IMonsterPursuitSceneServices {
        MonsterBehaviorManagerState& state;MonsterProximityActor player{0x80000,true,{10000,2,100},{10000,2,100},2};
        MonsterPursuitFrame* frame{};bool reenter{},interaction{},fail_visit{};unsigned actions{},visits{},interactions{};std::vector<unsigned> combat;
        explicit Services(MonsterBehaviorManagerState& s):state(s){}
        MonsterProximityActor ControlledActor() override {return player;}
        MonsterPursuitTarget Target(std::uint32_t id) override {Check(id==player.address);return {player.active,player.position};}
        bool PlayerControlBusy() override {if(reenter)Reject([&]{(void)frame->AdvanceAfterGrid(0,1);});return false;}
        bool ProcessObjectInteraction(MonsterBehaviorManagerState& s,std::uint32_t id,std::uint32_t target,float) override {Check(&s==&state && id==0x10000);++interactions;return target && interaction;}
        void DispatchCombatPositions(std::uint32_t id,float) override {Check(id==0x10000);combat.push_back(1);}
        void InitializeCombat(std::uint32_t id,std::int32_t argument) override {Check(id==0x10000 && argument==-1);combat.push_back(2);state.words[0x1f0/4]=0;}
        void ClearHistoryRecord(std::uint32_t) override {throw std::logic_error("unexpected history in generated pursuit frame");}
        void ProcessActionWithScript(std::uint32_t id,float) override {Check(id==0x10000);++actions;}
        void VisitArenaObjects(std::uint32_t id) override {Check(id==0x10000);++visits;if(fail_visit)throw std::runtime_error("distance visitor failure");}
    } services(state);
    MonsterPursuitFrame frame(f.actors,h,state,0x10000,0x50000,0x60000,*f.behavior[0],motion,floor,mesh,services);services.frame=&frame;
    f.behavior[0]->SetSchedulingEnabled(true);
    std::array<MonsterPursuitFrame*,1> frames{&frame};unsigned candidates{};MonsterSceneFrame* active=nullptr;
    MonsterSceneFrame scheduler(f.world,f.movement,[&](auto){++candidates;Reject([&]{active->Advance(0,1,frames);});return std::vector<MonsterMovementObstacle>{};});active=&scheduler;
    Reject([&]{MonsterSceneFrame invalid(f.world,f.movement,{});});
    Fixture other;Reject([&]{MonsterSceneFrame invalid(other.world,f.movement,[](auto){return std::vector<MonsterMovementObstacle>{};});});
    MonsterSceneFrame other_scheduler(other.world,other.movement,[](auto){return std::vector<MonsterMovementObstacle>{};});
    Reject([&]{other_scheduler.Advance(0,1,frames);});Check(!other_scheduler.Failed());
    std::array<MonsterPursuitFrame*,2> duplicate{&frame,&frame};Reject([&]{scheduler.Advance(0,1,duplicate);});
    Reject([&]{scheduler.Advance(-1,1,frames);});Check(!scheduler.Failed());
    const auto tick=[&](float dt){scheduler.Advance(dt,1,frames);};
    for(unsigned i=0;i<4;++i)(void)tick(1.f/60);
    Check(f.world.Stats().helpers==1 && f.behavior[0]->Stats().entered==1);
    MonsterSceneFrame failing(f.world,f.movement,[](auto)->std::vector<MonsterMovementObstacle>{throw std::runtime_error("controlled obstacle failure");});
    Reject([&]{failing.Advance(1.f/60,1,frames);});Check(failing.Failed() && !f.world.InGridFrame());
    const auto pose=f.actors.MotionTransform(h).position;services.player.position={pose[0]+10,pose[1],pose[2]};services.player.collision_position=services.player.position;services.reenter=true;
    (void)tick(1.f/60);services.reenter=false;
    Check(state.words[0x6e0/4]==1 && !f.behavior[0]->SchedulingEnabled() && f.world.Stats().helpers==0 && f.behavior[0]->Stats().saved==1 && services.actions==1);
    for(unsigned i=0;i<8;++i)(void)tick(1.f/60);
    Check(services.interactions>0 && f.actors.Action(h)=="Z02");state.words[0x704/4]=1;
    (void)tick(1.f/60);Check(state.words[0x6e0/4]==2 && !state.words[0x6e4/4]);
    for(unsigned i=0;i<100 && state.words[0x6e0/4];++i)(void)tick(1.f/60);
    Check(state.words[0x6e0/4]==0 && f.behavior[0]->SchedulingEnabled() && !frame.Failed());state.words[0x704/4]=0;
    // 已恢复的事件下一帧再次分派；玩家移远，避免立即开始第二轮追逐。
    services.player.position[0]=10000;for(unsigned i=0;i<3;++i)(void)tick(1.f/60);
    Check(f.behavior[0]->Stats().constructed>=2);
    std::ostringstream report;f.behavior[0]->WriteReport(report);Check(report.str().find("\"event_deletions\":1")!=std::string::npos && report.str().find("\"fuzzy_reloads\":1")!=std::string::npos && report.str().find("\"condition_documents\":2")!=std::string::npos);
    services.player.position=f.actors.MotionTransform(h).position;services.player.position[0]+=10;services.player.collision_position=services.player.position;
    (void)tick(1.f/60);Check(state.words[0x6e0/4]==1);services.interaction=true;(void)tick(1.f/60);
    Check(services.combat==std::vector<unsigned>{1,2} && !state.words[0x1f0/4]);const auto visits=services.visits;(void)tick(1.f/60);Check(services.visits==visits);
    state.words[0x1f0/4]=1;services.interaction=false;services.fail_visit=true;Reject([&]{(void)tick(1.f/60);});Check(frame.Failed() && f.behavior[0]->Failed() && scheduler.Failed() && candidates>0);Reject([&]{scheduler.Advance(0,1,frames);});
    Reject([&]{f.behavior[0]->Cancel();});Check(f.world.Stats().helpers==0);
    f.behavior[0].reset(); // 借用的管理器在拥有者之后销毁。
}
void ManagerFrames(){
    Fixture f(true);auto& owner=f.behavior[1];owner->SetSchedulingEnabled(true);MonsterBehaviorManagerState state{};
    struct Services final:IMonsterBehaviorFrameRuntime {
        MonsterBehaviorManagerState& state;std::vector<unsigned> calls;bool blocked{},fail_expiry{},fail_motion{},disable_processor{};
        explicit Services(MonsterBehaviorManagerState& s):state(s){}
        void CheckTarget(MonsterBehaviorManagerState& s,std::uint32_t a,float e){Check(&s==&state && a==0x10000 && e>=0);}
        std::uint32_t ProcessManagerState(MonsterBehaviorManagerState& s,std::uint32_t a,float e) override {
            CheckTarget(s,a,e);calls.push_back(1);if(disable_processor)s.words[(MonsterBehaviorManager::kProcessorOffset+24)/4]&=~0xffu;return blocked?1u:0u;
        }
        void CheckStateAndProcessCollision(MonsterBehaviorManagerState& s,std::uint32_t a,float e) override {CheckTarget(s,a,e);calls.push_back(2);if(fail_expiry)throw std::runtime_error("expiry failure");}
        IScriptEventSchedulerRuntime& SchedulerRuntime() override {throw std::logic_error("frame must use the actual owned scheduler");}
        std::uint32_t UpdatePosition(std::uint32_t a,float e,std::uint32_t arg) override {Check(a==0x10000 && e>=0 && arg==7);calls.push_back(3);if(fail_motion)throw std::runtime_error("motion failure");return 0xabcdef01;}
    } services(state);
    const auto tick=[&](float e){return owner->AdvanceManagerAfterGrid(state,0x10000,e,7,services);};
    Check(tick(.5f)==0 && services.calls.empty() && owner->Stats().constructed==0); // Original active gate.
    state.words[0x1f0/4]=1;state.words[0x730/4]=1;state.words[0x734/4]=std::bit_cast<std::uint32_t>(1.f);
    Check(tick(.5f)==0xabcdef01 && services.calls==std::vector<unsigned>{3} && owner->Stats().constructed==0);services.calls.clear();
    Check(tick(.5f)==0xabcdef01 && services.calls==std::vector<unsigned>{3} && state.words[0x730/4]==1);services.calls.clear();
    // Compare the unrounded sum even when the stored float rounds to the limit.
    Check(tick(std::bit_cast<float>(0x33800000u))==0xabcdef01 && services.calls==std::vector<unsigned>{2,3} && state.words[0x730/4]==0 && owner->Stats().constructed==0);services.calls.clear();
    Check(state.words[0x738/4]==state.words[0x734/4]);services.blocked=true;
    Check(tick(0)==0xabcdef01 && services.calls==std::vector<unsigned>{1,3} && owner->Stats().constructed==0);services.calls.clear();services.blocked=false;
    Check(tick(0)==0xabcdef01 && services.calls==std::vector<unsigned>{1,3} && owner->Stats().entered==1 && f.world.Stats().helpers==1);services.calls.clear();
    const auto updates=owner->Stats().updates;state.words[0x1f0/4]=0;Check(tick(.5f)==0 && services.calls.empty() && owner->Stats().updates==updates);
    state.words[0x1f0/4]=1;services.blocked=true;Check(tick(.5f)==0xabcdef01 && owner->Stats().updates==updates);services.blocked=false;services.calls.clear();
    services.disable_processor=true;Check(tick(.5f)==0xabcdef01 && !owner->SchedulingEnabled() && owner->Stats().updates==updates);services.disable_processor=false;
    owner->SetSchedulingEnabled(true);Check(tick(.1f)==0xabcdef01 && owner->Stats().updates==updates+1);
    // Cleanup remains explicit after disabling the event processor.
    owner->SetSchedulingEnabled(false);owner->Cancel();Check(f.world.Stats().helpers==0);
    Fixture wrong(true);MonsterBehaviorManagerState wrong_state{};Services wrong_service(wrong_state);
    Reject([&]{(void)wrong.behavior[1]->AdvanceManagerAfterGrid(wrong_state,1,0,7,wrong_service);});Check(wrong_service.calls.empty());
    Fixture replaced(true);MonsterBehaviorManagerState first{},other{};Services first_service(first);
    Check(replaced.behavior[1]->AdvanceManagerAfterGrid(first,0x10000,0,7,first_service)==0);
    Reject([&]{(void)replaced.behavior[1]->AdvanceManagerAfterGrid(other,0x10000,0,7,first_service);});Check(first_service.calls.empty());
    Fixture failed(true);auto& bad=failed.behavior[1];MonsterBehaviorManagerState broken{};Services throwing(broken);broken.words[0x1f0/4]=1;broken.words[0x730/4]=1;
    throwing.fail_expiry=true;Reject([&]{(void)bad->AdvanceManagerAfterGrid(broken,0x10000,.1f,7,throwing);});Check(bad->Failed() && broken.words[0x730/4]==0 && throwing.calls==std::vector<unsigned>{2});
    Fixture motion(true);auto& moving=motion.behavior[1];MonsterBehaviorManagerState motion_state{};Services motion_service(motion_state);motion_state.words[0x1f0/4]=1;motion_service.fail_motion=true;moving->SetSchedulingEnabled(true);
    Reject([&]{(void)moving->AdvanceManagerAfterGrid(motion_state,0x10000,0,7,motion_service);});Check(moving->Failed() && motion.world.Stats().helpers==1);Reject([&]{moving->Cancel();});Check(motion.world.Stats().helpers==0);
    f.behavior[1].reset();wrong.behavior[1].reset();replaced.behavior[1].reset();failed.behavior[1].reset();motion.behavior[1].reset();
}
void ScheduledRounds(){
    Fixture f(true);
    for(auto& owner:f.behavior){Check(!owner->SchedulingEnabled() && owner->AdvanceScheduledAfterGrid(.25f)==0 && owner->Ready());owner->SetSchedulingEnabled(true);}
    const auto tick=[&](float seconds){
        f.Grid(seconds);for(unsigned i=0;i<2;++i)if(!f.actors.Action(f.actors.Handle(i)).empty())f.actors.AdvanceAnimation(f.actors.Handle(i),seconds);
        for(auto& owner:f.behavior)(void)owner->AdvanceScheduledAfterGrid(seconds);
    };
    tick(0);Check(f.world.Stats().helpers==2);
    const auto draws=f.draws;const auto updates=f.behavior[1]->Stats().updates;
    f.behavior[1]->SetSchedulingEnabled(false);Check(f.behavior[1]->AdvanceScheduledAfterGrid(.5f)==0 && f.behavior[1]->Stats().updates==updates && f.draws==draws && !f.behavior[1]->Ready());
    f.behavior[1]->SetSchedulingEnabled(true);
    for(unsigned frame=0;frame<1000;++frame){
        tick(1.f/60);for(auto& owner:f.behavior)if(owner->Stats().saved>=3 && owner->Ready() && owner->SchedulingEnabled())owner->SetSchedulingEnabled(false);
        if(!f.behavior[0]->SchedulingEnabled() && !f.behavior[1]->SchedulingEnabled())break;
    }
    for(auto& owner:f.behavior){Check(owner->Stats().saved==3 && owner->Ready() && !owner->Failed());
        std::ostringstream report;owner->WriteReport(report);Check(report.str().find("\"dispatches\":3")!=std::string::npos && report.str().find("\"condition_documents\":1")!=std::string::npos);
        owner->Cancel();}
    Check(f.world.Stats().helpers==0 && f.world.Stats().associations==0);
    // Disabled processing neither dispatches nor advances a running queue.
    Fixture failed(true);auto& owner=failed.behavior[1];owner->SetSchedulingEnabled(true);(void)owner->AdvanceScheduledAfterGrid(0);failed.fail_idle=true;
    Reject([&]{owner->Cancel();});Check(owner->Failed() && failed.world.Stats().helpers==0);Reject([&]{(void)owner->AdvanceScheduledAfterGrid(0);});
    // An actor with no fuzzy event does not fabricate a task or document.
    const std::vector<SceneMonster> empty_records=[] {auto r=Records();r[0].behavior={};return r;}();
    SceneMonsterInstances actors(1,empty_records,[](auto){return Model();},[](auto)->std::optional<std::vector<std::uint8_t>>{return {};});
    auto paths=std::make_shared<ScenePathways>(1,std::span<const ScenePath>{});MonsterMovementWorld world(1,paths,[]{return 16384u;});const std::vector<CollisionTriangle> geometry{};SceneMovement floor(geometry);MonsterOrientationCaches caches;MonsterOrientation orientation(world,actors,floor,caches);BinaryXmlNode memory;
    MonsterMovementBehavior none(world,actors,actors.Handle(0),memory,orientation,[]{return 2.f;},[]{return 1u;});none.SetSchedulingEnabled(true);
    for(unsigned i=0;i<3;++i)Check(none.AdvanceScheduledAfterGrid(.1f)==0);
    Check(none.Ready() && none.Stats().constructed==0 && world.Stats().helpers==0);none.SetSchedulingEnabled(false);none.Cancel();
}
void RandomFailures(){
    for(int mode=0;mode<4;++mode){
        Fixture f(true);auto& owner=f.behavior[1];Check(owner->Start());f.Tick(0);Check(f.world.Stats().helpers==1);
        if(mode==0){f.fail_idle=true;Reject([&]{owner->Cancel();});Check(owner->Failed());}
        if(mode==1){f.memory[1].attributes.push_back({"AAB_MST_memory_speed",2,std::int32_t{1}});Reject([&]{owner->Cancel();});Check(owner->Failed() && owner->Stats().discarded==1);}
        if(mode==2){f.world.UpdateMembers(0,[&](auto h,float){f.world.Helper(h).words[46]&=~0xffu;});owner->Cancel();Check(owner->Stats().saved==0 && owner->Stats().discarded==1 && f.memory[1].attributes.empty());}
        if(mode==3){MonsterHelperHandle h;f.world.UpdateMembers(0,[&](auto helper,float){h=helper;});f.world.DestroyHelper(h);Reject([&]{owner->AdvanceAfterGrid(0);});Reject([&]{owner->Cancel();});Check(owner->Failed());}
        Check(f.world.Stats().helpers==0 && owner->Stats().live_actions==0);
    }
}
void Failures(){
    {
        Fixture f;Check(f.behavior[0]->Start());f.Tick(0);f.fail_idle=true;
        Reject([&]{f.behavior[0]->Cancel();});Check(f.behavior[0]->Failed() && f.behavior[0]->Ready() && f.world.Stats().helpers==0 && f.behavior[0]->Stats().live_actions==0);
        Reject([&]{f.behavior[0]->Start();});Reject([&]{f.behavior[0]->AdvanceAfterGrid(0);});
        std::ostringstream out;f.behavior[0]->WriteReport(out);Check(out.str().find("controlled idle failure")!=std::string::npos);
    }
    {
        Fixture f;Check(f.behavior[0]->Start());f.radius_callback=[&]{f.behavior[0]->Cancel();return 2.f;};
        Reject([&]{f.behavior[0]->AdvanceAfterGrid(0);});Check(f.behavior[0]->Failed() && f.world.Stats().helpers==0 && f.behavior[0]->Stats().live_actions==0);
    }
    {
        Fixture f;Check(f.behavior[0]->Start());f.Tick(0);f.world.UpdateMembers(0,[&](auto h,float){f.world.Helper(h).words[46]&=~0xffu;});
        f.behavior[0]->Cancel();Check(f.behavior[0]->Stats().saved==0 && f.behavior[0]->Stats().discarded==1 && f.memory[0].attributes.empty() && f.world.Stats().helpers==0);
    }
    {
        Fixture f;auto unsupported=f.actors.Behavior(f.actors.Handle(0));unsupported.fuzzy->choices[0].class_id=2;
        Reject([&]{(void)f.behavior[0]->Start(unsupported);});Check(f.behavior[0]->Failed() && f.world.Stats().helpers==0);
    }
    {
        Fixture f;Check(f.behavior[0]->Start());f.Tick(0);f.memory[0].attributes.push_back({"AAB_MST_memory_speed",2,std::int32_t{1}});
        Reject([&]{f.behavior[0]->Cancel();});Check(f.behavior[0]->Failed() && f.world.Stats().helpers==0 && f.behavior[0]->Stats().discarded==1);
    }
    {
        Fixture f;Check(f.behavior[0]->Start());f.Tick(0);f.behavior[0].reset();Check(f.world.Stats().helpers==0);
    }
    {
        Fixture f;Check(f.behavior[0]->Start());f.Tick(0);MonsterHelperHandle lost;
        f.world.UpdateMembers(0,[&](auto h,float){lost=h;});f.world.DestroyHelper(lost);
        Reject([&]{f.behavior[0]->AdvanceAfterGrid(0);});Check(f.behavior[0]->Failed());
        Reject([&]{f.behavior[0]->Cancel();});Check(f.behavior[0]->Ready() && f.behavior[0]->Stats().live_actions==0 && f.world.Stats().helpers==0);
    }
}
}
int main(int argc,char** argv)try{if(argc==6 && std::string_view(argv[1])=="shared-world-player-source"){SharedWorldSource(argv[2],argv[3],argv[4],argv[5],true,true);return 0;}if(argc==6 && (std::string_view(argv[1])=="shared-world-source" || std::string_view(argv[1])=="shared-world-collision-source")){SharedWorldSource(argv[2],argv[3],argv[4],argv[5],std::string_view(argv[1])=="shared-world-collision-source");return 0;}if(argc==7){const std::string_view name(argv[1]);const auto mode=name=="proximity-source"?SourceProbe::proximity:name=="pursuit-source"?SourceProbe::paths:name=="pursuit-frame-source"?SourceProbe::frame:name=="resource-frame-source"?SourceProbe::resource:name=="base-lifetime-source"?SourceProbe::base_lifetime:name=="members-lifetime-source"?SourceProbe::members_lifetime:name=="component-lifetime-source"?SourceProbe::component_lifetime:SourceProbe::initialization;Check(name=="proximity-source" || name=="pursuit-source" || name=="pursuit-frame-source" || name=="resource-frame-source" || name=="resource-initialization-source" || name=="base-lifetime-source" || name=="members-lifetime-source" || name=="component-lifetime-source");ProximitySource(argv[2],argv[3],argv[4],argv[5],argv[6],mode);return 0;}EncounterStorage();ProcessorLifecycle();ResourceFrameGates();Pipeline();Pipeline(true);GridBoundary();Failures();RandomFailures();ScheduledRounds();ManagerFrames();ComponentSourceViews();ComponentMovementInitialization();ConstructedMovementBehavior();AnimationBindingRollback();ConstructedBaseMotion();BaseMotion();PursuitFailures();PursuitFrames();std::cout<<"path0/1 and random3 task factory, shared member grid phase, save/error propagation and cleanup contracts passed; no gameplay executed\n";return 0;}
catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
