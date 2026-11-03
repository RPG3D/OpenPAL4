#include "pal4/portable/monster_scene_runtime.h"
#include "pal4/portable/monster_orientation.h"
#include "pal4/portable/package.h"
#include "pal4/portable/scene_movement.h"
#include "pal4/base_object_activity.h"

#include <iostream>
#include <filesystem>
#include <fstream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string_view>

using namespace pal4::portable;

namespace {
void Check(bool value,const char* message) {
    if(!value)throw std::runtime_error(message);
}

class UnboundSceneServices final:public IMonsterPursuitSceneServices {
    [[noreturn]] static void Unbound(){throw std::logic_error("monster scene services are not bound");}
public:
    pal4::MonsterProximityActor ControlledActor() override{Unbound();}
    pal4::MonsterPursuitTarget Target(std::uint32_t) override{Unbound();}
    bool PlayerControlBusy() override{Unbound();}
    bool ProcessObjectInteraction(pal4::MonsterBehaviorManagerState&,std::uint32_t,std::uint32_t,float) override{Unbound();}
    void DispatchCombatPositions(std::uint32_t,float) override{Unbound();}
    void InitializeCombat(std::uint32_t,std::int32_t) override{Unbound();}
    void ClearHistoryRecord(std::uint32_t) override{Unbound();}
    void ProcessActionWithScript(std::uint32_t,float) override{Unbound();}
    void VisitArenaObjects(std::uint32_t) override{Unbound();}
};
class RenderServices final:public IMonsterSceneRenderServices {
public:
    std::function<bool(SceneMonsterHandle,std::uint32_t,std::array<float,4>)> cull;
    std::function<void(SceneMonsterHandle,std::uint32_t)> draw;
    bool FrustumTestSphere(SceneMonsterHandle h,std::uint32_t camera,std::array<float,4> sphere) override{return cull(h,camera,sphere);}
    void DrawClump(SceneMonsterHandle h,std::uint32_t clump) override{draw(h,clump);}
    void Output(std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t) override{throw std::logic_error("unexpected unbound render output callback");}
    void Path(std::uint32_t,std::uint32_t,std::uint32_t) override{throw std::logic_error("unexpected unbound render path callback");}
    std::uint32_t Move(std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t) override{throw std::logic_error("unexpected unbound render move callback");}
};

void RejectsAdvanceBeforeActorOwnerIsBound() {
    auto source=std::make_shared<ArenaSource>("composition-test-data");
    const std::vector<SceneMonster> monsters;
    const std::vector<ScenePath> paths;
    MonsterSceneRuntime runtime(51,source,monsters,paths,[]{return 16384u;});
    bool rejected=false;
    try{runtime.Advance(1.f/60,1);}catch(const std::logic_error&){rejected=true;}
    Check(rejected,"runtime does not advance without an actor/frame owner");
    Check(!runtime.Failed(),"missing owner is a binding error, not a failed partial frame");
    const auto borrowed=runtime.World().CreateEndpoint({51,0},7);
    rejected=false;
    try{runtime.CloseActors();}catch(const std::logic_error&){rejected=true;}
    Check(rejected && !runtime.Closed(),"borrowed helper close failure preserves the open runtime");
    runtime.World().DestroyHelper(borrowed);
    runtime.AdoptActors({},[](MonsterHelperHandle){return std::vector<MonsterMovementObstacle>{};});
    Check(runtime.ActorsBound() && runtime.ActorCount()==0,"an actually empty scene can own an empty frame group");
    runtime.Advance(0,1);
    Check(!runtime.Failed(),"empty scene frame advances without fabricating actors");
    runtime.CloseActors();
    runtime.Close();
    Check(runtime.Closed(),"unbound runtime closes in dependency order");
}

std::vector<SceneMonster> ReadMonsters(ArenaSource& source,std::string_view world,std::string_view scene) {
    const auto bytes=source.ReadSceneMember(std::string(world)+"\\"+std::string(scene)+"\\mstInfo.mst");
    Check(bytes.has_value(),"original mstInfo.mst exists");
    std::istringstream input(std::string(bytes->begin(),bytes->end()));
    return ReadSceneMonsters(input);
}

std::vector<ScenePath> ReadPaths(ArenaSource& source,std::string_view world,std::string_view scene) {
    const auto bytes=source.ReadSceneMember(std::string(world)+"\\"+std::string(scene)+"\\pathway.pwy");
    Check(bytes.has_value(),"original pathway.pwy exists");
    std::istringstream input(std::string(bytes->begin(),bytes->end()));
    return ReadScenePaths(input);
}

void OriginalM01ActorsAreAdoptedAndReleasedBeforeResources(const std::filesystem::path& game_data) {
    auto source=std::make_shared<ArenaSource>(game_data);
    auto mesh=source->ReadOriginalCollision("M01","1");
    CollisionSurface floor(mesh.floor);floor.EnableNavigation();
    SceneMovement ground(mesh.floor);
    MonsterOrientationCaches caches;
    UnboundSceneServices scene_services;
    std::ifstream actor_file(game_data/"palactor.cpk",std::ios::binary);
    Check(bool(actor_file),"original palactor.cpk opens");
    PackageReader actor_package(actor_file);
    std::unique_ptr<MonsterOrientation> orientation;
    std::unique_ptr<MonsterSceneRuntime> runtime;

    auto monsters=ReadMonsters(*source,"M01","1");
    auto paths=ReadPaths(*source,"M01","1");
    Check(monsters.size()==8 && paths.size()==8,"M01/1 input has eight monsters and pathways");
    // Deterministic RNG is limited to this ownership contract; it is not a host service.
    runtime=std::make_unique<MonsterSceneRuntime>(52,source,monsters,paths,[]{return 16384u;});
    orientation=std::make_unique<MonsterOrientation>(runtime->World(),runtime->Monsters(),ground,caches);
    Check(runtime->Monsters().Size()==monsters.size(),"runtime owns all M01 instances");

    const auto make_actor=[&](MonsterSceneRuntime& owner,MonsterOrientation& actor_orientation,
                               std::uint32_t& identifiers,std::size_t index) {
        const auto handle=owner.Monsters().Handle(owner.Monsters().OriginalTraversalOrder().at(index));
        const auto manager=0x10000u+static_cast<std::uint32_t>(index)*0x10000u;
        auto actor=std::make_unique<MonsterSceneActor>(owner.World(),owner.Monsters(),handle,actor_orientation,
            [&owner,handle]{return owner.Monsters().LocalSphere(handle)[3];},
            [&identifiers]{return identifiers++;},actor_package,manager,
            0x200000u+static_cast<std::uint32_t>(index)*8u,0,1,floor,mesh,2.f,
            [](Point3,Point3)->bool{throw std::logic_error("original scene collision is not bound");},scene_services);
        actor->Behavior().SetSchedulingEnabled(true);
        return actor;
    };

    {
        auto foreign_runtime=std::make_unique<MonsterSceneRuntime>(52,source,monsters,paths,[]{return 16384u;});
        MonsterOrientationCaches foreign_caches;
        auto foreign_orientation=std::make_unique<MonsterOrientation>(foreign_runtime->World(),foreign_runtime->Monsters(),ground,foreign_caches);
        std::uint32_t foreign_identifiers=3;
        std::vector<std::unique_ptr<MonsterSceneActor>> foreign_actors;
        foreign_actors.push_back(make_actor(*foreign_runtime,*foreign_orientation,foreign_identifiers,0));
        Check(foreign_actors.front()->Handle()==runtime->Monsters().Handle(runtime->Monsters().OriginalTraversalOrder()[0]),
              "same-generation foreign actor has numerically equal handle");
        std::string rejection;
        try {
            runtime->AdoptActors(std::move(foreign_actors),[](MonsterHelperHandle){return std::vector<MonsterMovementObstacle>{};});
        } catch(const std::invalid_argument& error) {
            rejection=error.what();
        }
        Check(rejection=="monster actor belongs to another instance owner",
              "runtime rejects a numeric-handle collision from another instance owner");
        Check(foreign_runtime->World().Stats().helpers==0,"rejected foreign actor releases its helper");
        foreign_orientation.reset();
        foreign_runtime->Close();
    }

    std::uint32_t identifiers=3;
    std::vector<std::unique_ptr<MonsterSceneActor>> actors;
    const auto traversal=runtime->Monsters().OriginalTraversalOrder();
    for(std::size_t i=0;i<traversal.size();++i) {
        const auto handle=runtime->Monsters().Handle(traversal[i]);
        auto actor=make_actor(*runtime,*orientation,identifiers,i);
        Check(actor->Label()==runtime->Monsters().Source(handle).name,"actor owns its original MST identity");
        Check(runtime->Monsters().SelectAction(handle,"Z01"),"test selects an explicit original animation");
        actors.push_back(std::move(actor));
    }

    runtime->World().UpdateMembers(0,[&](MonsterHelperHandle helper,float elapsed){
        Check(runtime->Movement().Update(helper,{},elapsed).active,"initial owner grid update runs before behavior dispatch");
    });
    for(auto& actor:actors)actor->Behavior().AdvanceScheduledAfterGrid(0);

    unsigned obstacle_requests{};bool in_frame_close_rejected{};
    runtime->AdoptActors(std::move(actors),[&](MonsterHelperHandle)->std::vector<MonsterMovementObstacle>{
        ++obstacle_requests;
        try{runtime->CloseActors();}catch(const std::logic_error&){in_frame_close_rejected=true;}
        Check(in_frame_close_rejected && runtime->ActorsBound() && !runtime->Closed(),
              "close during scene grid traversal preserves the complete runtime owner");
        throw std::logic_error("original obstacle producer is not bound");
    });
    Check(runtime->ActorsBound() && runtime->ActorCount()==monsters.size(),
          "runtime adopts every real actor in original manager traversal order");
    Check(runtime->World().Stats().helpers!=0,"actor owners register real movement helpers");

    bool failed=false;
    try{runtime->Advance(1.f/60,1);}catch(const std::logic_error&){failed=true;}
    Check(failed && obstacle_requests!=0 && in_frame_close_rejected && runtime->Failed(),
          "unbound obstacle producer fails the shared scene frame closed");
    runtime->CloseActors();
    Check(!runtime->ActorsBound() && runtime->ActorCount()==0,"actor group closes before borrowed services");
    Check(runtime->World().Stats().helpers==0,"closing actor owners releases all helpers");
    orientation.reset();
    runtime->Close();
    Check(runtime->Closed(),"movement, world, and resources close after actor helpers");
}

void OriginalM01ConstructionFailureReleasesPartialActors(const std::filesystem::path& game_data) {
    auto source=std::make_shared<ArenaSource>(game_data);
    auto mesh=source->ReadOriginalCollision("M01","1");
    CollisionSurface floor(mesh.floor);floor.EnableNavigation();
    SceneMovement ground(mesh.floor);
    MonsterOrientationCaches caches;
    UnboundSceneServices services;
    std::ifstream actor_file(game_data/"palactor.cpk",std::ios::binary);
    Check(bool(actor_file),"original actor package opens for complete construction");
    PackageReader package(actor_file);
    const auto monsters=ReadMonsters(*source,"M01","1");
    const auto paths=ReadPaths(*source,"M01","1");
    MonsterSceneRuntime runtime(53,source,monsters,paths,[]{return 16384u;});
    const auto order=runtime.Monsters().OriginalTraversalOrder();
    unsigned token_requests{},failure_at{};
    enum class Failure { none, callback, duplicate_manager, duplicate_clump, invalid_clump } failure{};
    std::uint32_t identifiers=3;
    std::vector<SceneMonsterHandle> requested;
    const auto token_for=[](SceneMonsterHandle h) {
        return MonsterSceneActorTokens{0x10000u+h.index*0x10000u,0x200000u+h.index*8u,0,1};
    };
    // All unbound gameplay services throw. This checks ownership only and cannot
    // silently advance using empty obstacles or a fabricated combat response.
    MonsterSceneActorBindings bindings{package,ground,caches,floor,mesh,services,2.f,
        [&](SceneMonsterHandle h) {
            requested.push_back(h);++token_requests;
            if(token_requests==failure_at && failure==Failure::callback)
                throw std::runtime_error("injected token source failure");
            auto tokens=token_for(h);
            if(token_requests==failure_at){
                const auto first=token_for(runtime.Monsters().Handle(order.front()));
                if(failure==Failure::duplicate_manager)tokens.manager=first.manager;
                if(failure==Failure::duplicate_clump)tokens.clump=first.clump;
                if(failure==Failure::invalid_clump)++tokens.clump;
            }
            return tokens;
        },
        [&](SceneMonsterHandle h){return runtime.Monsters().LocalSphere(h)[3];},
        [&]{return identifiers++;},
        [](Point3,Point3)->bool{throw std::logic_error("collision not bound in construction contract");},
        [](MonsterHelperHandle)->std::vector<MonsterMovementObstacle>{
            throw std::logic_error("obstacle production not bound in construction contract");
        }};
    const auto check_unbound=[&] {
        Check(!runtime.ActorsBound() && runtime.ActorCount()==0 && !runtime.Failed(),
              "construction failure does not publish or poison a partial actor group");
        const auto stats=runtime.World().Stats();
        Check(stats.helpers==0 && stats.members==0 && stats.associations==0,
              "partial construction releases helper grid registrations");
        for(const auto index:order)
            Check(!runtime.Monsters().TransformBound(runtime.Monsters().Handle(index)),
                  "partial construction releases every instance transform binding");
    };
    auto incomplete=bindings;incomplete.obstacles={};
    bool rejected=false;
    try{runtime.CreateActors(incomplete);}catch(const std::invalid_argument&){rejected=true;}
    Check(rejected && token_requests==0,"missing service is rejected before actor side effects");
    check_unbound();
    auto invalid_height=bindings;invalid_height.collision_height=std::numeric_limits<float>::infinity();
    rejected=false;
    try{runtime.CreateActors(invalid_height);}catch(const std::invalid_argument&){rejected=true;}
    Check(rejected && token_requests==0,"nonfinite collision height is rejected before construction");
    check_unbound();

    failure_at=3;
    for(const auto injected:{Failure::callback,Failure::duplicate_manager,Failure::duplicate_clump,Failure::invalid_clump}) {
        failure=injected;token_requests=0;requested.clear();rejected=false;
        try{runtime.CreateActors(bindings);}catch(const std::exception&){rejected=true;}
        Check(rejected && token_requests==3,"construction fails after two complete actor owners");
        check_unbound();
    }
    failure=Failure::none;token_requests=0;requested.clear();
    runtime.CreateActors(bindings);
    Check(runtime.ActorsBound() && runtime.ActorCount()==8 && requested.size()==8,
          "same runtime can create the complete original M01 actor group after failures");
    for(std::size_t i=0;i<order.size();++i) {
        const auto h=runtime.Monsters().Handle(order[i]);
        Check(requested[i]==h && runtime.Actor(i).Handle()==h,"factory preserves original name traversal order");
        Check(runtime.Actor(i).ManagerToken()==token_for(h).manager,"factory preserves caller logical identity");
        Check(runtime.Actor(i).Label()==runtime.Monsters().Source(h).name,"factory owns original MST label");
        Check(runtime.Monsters().TransformBound(h),"factory binds actual instance transforms");
    }
    rejected=false;
    try{runtime.CreateActors(bindings);}catch(const std::logic_error&){rejected=true;}
    Check(rejected && token_requests==8,"complete group rejects accidental reconstruction");

    const auto set_activity=[](MonsterSceneActor& actor,std::uint32_t flag){
        pal4::BaseObjectState base;std::copy_n(actor.State().words.begin(),base.words.size(),base.words.begin());
        (void)pal4::BaseObjectActivity::SetStateFlag(base,flag);
        actor.State().words[0x1f0/4]=base.Read(0x1f0);
    };
    RenderServices render;
    const auto hidden=runtime.Actor(1).Handle();
    std::vector<SceneMonsterHandle> submitted;
    unsigned culls{};bool close_rejected{};
    render.cull=[&](SceneMonsterHandle h,std::uint32_t camera,std::array<float,4> sphere){
        Check(camera==0x8000 && sphere[3]>=0,"render forwards actual camera and world sphere");
        ++culls;return h!=hidden;
    };
    render.draw=[&](SceneMonsterHandle h,std::uint32_t clump){
        Check(clump==token_for(h).clump,"render forwards the actual bound clump");
        submitted.push_back(h);
        try{runtime.CloseActors();}catch(const std::logic_error&){close_rejected=true;}
        Check(close_rejected && runtime.ActorCount()==8,"group rejects destruction during render traversal");
    };
    for(std::size_t i=0;i<runtime.ActorCount();++i)set_activity(runtime.Actor(i),0);
    auto rendered=runtime.Render(0x8000,render);
    Check(rendered.active_objects==0 && rendered.character_updates==0 && culls==0 && submitted.empty(),
          "shared base-activity rules skip inactive monsters without platform callbacks");
    for(std::size_t i=0;i<3;++i){
        auto& actor=runtime.Actor(i);set_activity(actor,1);
        actor.State().words[9]=1;actor.State().words[6]=1; // Explicit world-entry fixture, not a production default.
        Check(runtime.Monsters().SelectAction(actor.Handle(),"Z01"),"render fixture selects original Z01");
        runtime.Monsters().AdvanceAnimation(actor.Handle(),.125f);
    }
    rendered=runtime.Render(0x8000,render);
    Check(rendered.active_objects==3 && rendered.render_successes==2 && rendered.visible_objects==2 &&
          rendered.animation_calls==2 && rendered.character_updates==3 && culls==3,
          "shared resource visibility gates animation while collision callbacks still run for every active actor");
    Check(submitted==std::vector<SceneMonsterHandle>{runtime.Actor(0).Handle(),runtime.Actor(2).Handle()},
          "render preserves original actor order while omitting a culled clump");
    for(std::size_t i=0;i<3;++i)Check(runtime.Monsters().AnimationTime(runtime.Actor(i).Handle())==.125f,
                                    "render-side animation processing does not consume a frame delta");
    runtime.Actor(2).State().words[6]=0;submitted.clear();culls=0;
    rendered=runtime.Render(0x8000,render);
    Check(rendered.render_successes==2 && rendered.visible_objects==1 && rendered.animation_calls==2 && submitted.size()==1 && culls==2,
          "original resource mode zero succeeds without a clump submission or frustum query");
    runtime.Actor(0).State().words[9]=0;submitted.clear();
    rendered=runtime.Render(0x8000,render);
    Check(rendered.render_successes==1 && rendered.animation_calls==1 && rendered.character_updates==3 && submitted.empty(),
          "disabled primary resource suppresses drawing and animation but keeps original character callback ordering");
    rejected=false;
    try{runtime.Render(0,render);}catch(const std::invalid_argument&){rejected=true;}
    Check(rejected && !runtime.Failed(),"invalid camera is rejected before any partial render frame");
    runtime.Actor(0).State().words[9]=1;
    render.draw=[](SceneMonsterHandle,std::uint32_t){throw std::runtime_error("injected GLES submission failure");};
    rejected=false;
    try{runtime.Render(0x8000,render);}catch(const std::runtime_error&){rejected=true;}
    Check(rejected && runtime.Failed(),"partial render failure stops further use of the scene owner");
    runtime.CloseActors();
    Check(!runtime.ActorsBound() && runtime.World().Stats().helpers==0,"failed render still releases the complete actor group");
    rejected=false;
    try{runtime.CreateActors(bindings);}catch(const std::logic_error&){rejected=true;}
    Check(rejected,"closed scene generation cannot rebind actors");
    runtime.Close();
    Check(runtime.Closed(),"owned orientation and actors close before the world resources");
    std::cout<<"original M01 complete construction, partial failures/retry, shared render rules, clocks and close passed\n";
}
}

int main(int argc,char** argv) {
    try {
        RejectsAdvanceBeforeActorOwnerIsBound();
        if(argc==1)return 0;
        if(argc==3 && std::string_view(argv[1])=="--original-gamedata") {
            OriginalM01ActorsAreAdoptedAndReleasedBeforeResources(argv[2]);
            OriginalM01ConstructionFailureReleasesPartialActors(argv[2]);
            return 0;
        }
        throw std::invalid_argument("expected no arguments or --original-gamedata <path>");
    } catch(const std::exception& error) {
        std::cerr<<error.what()<<'\n';
        return 1;
    }
}
