#include "pal4/portable/scene_events.h"
#include "pal4/portable/script_stack.h"
#include "pal4/portable/game_services.h"
#include <iostream>
#include <bit>
#include <sstream>
#include <stdexcept>
using namespace pal4::portable;
void Check(bool value){if(!value)throw std::runtime_error("scene event contract failed");}
struct Scripts:ISceneEventScripts,IScriptContextServices {
    ScriptGlobals globals{};ScriptStack stack{globals};unsigned loads{},pushes{};
    std::ostringstream trace;GameServices services{trace};
    Scripts(){stack.Compile("Q01","void func1001(){giWait(0.1);global_mvar++;} void Hidden(){giEventVolumeVisible(\"ev1\",false);}");}
    ScriptResponse Invoke(const ScriptRequest& request) override{return services.Invoke(request);}
    bool Contains(std::string_view world,std::string_view declaration) override{return stack.ContainsActive(world,declaration);}
    void LoadWorld(std::string_view world) override{Check(world=="Q01");++loads;}
    std::int32_t Push(std::string_view world,std::string_view declaration) override {
        while(stack.State()==ScriptState::finished)stack.PopFinished();
        const auto result=stack.Push(world,declaration,*this);if(result)++pushes;return result?0:-6;
    }
};
ArenaResources Fixture(){ArenaResources a;a.description.world="Q01";a.description.scene="N01";a.description.event_collision_cache=true;
    for(unsigned type:{1,5,4,0}){SceneEvent event;event.effect_type=type;event.name="ev"+std::to_string(type);event.script="func1001";event.clump={1};a.catalog.events.push_back(event);
        CollisionTriangle triangle;triangle.points={Point3{-10,0,-10},Point3{0,0,10},Point3{10,0,-10}};if(type==0)for(auto& p:triangle.points)p[0]+=100;
        ObjectCollisionAtomic atomic;atomic.triangles={triangle};ObjectCollisionGeometry geometry;geometry.atomics={atomic};a.event_collisions.emplace_back(geometry);}
    a.catalog.events.push_back(a.catalog.events[0]);a.catalog.events.back().name="overlap";a.event_collisions.push_back(a.event_collisions[0]);
    return a;
}
void MachineInteraction(){
    auto arena=Fixture();SceneCollision collision;Scripts scripts;
    auto& machine=arena.catalog.events[1];
    std::array<std::uint32_t,20> property{};
    property[0]=std::bit_cast<std::uint32_t>(39.f);
    property[1]=std::bit_cast<std::uint32_t>(2.f);
    machine.properties={property};
    // 首条非机关即使有更近属性点，也不能参与确认键选择。
    arena.catalog.events[0].properties={std::array<std::uint32_t,20>{}};
    SceneEvents events(arena,collision,scripts);events.ActivateAll();
    Check(events.States()[1].movement_shapes==0);
    Check(events.FindInteraction({0,0,0},2,78)==1); // 原距离39；平方1521会错误拒绝。
    Check(!events.FindInteraction({0,0,0},1,78));
    Check(!events.FindInteraction({0,0,0},2,39)); // 水平严格范围。
    Check(!events.TriggerInteraction(0));
    Check(events.TriggerInteraction(1) && scripts.pushes==1);
    Check(!events.TriggerInteraction(1) && scripts.pushes==1);
    Check(scripts.stack.Tick(0)==ScriptState::suspended);
    Check(scripts.stack.Tick(.1)==ScriptState::finished && scripts.globals[0]==1);
    Check(events.TriggerInteraction(1) && scripts.pushes==2);
    Check(events.SetVisible("ev5",false));
    Check(!events.FindInteraction({0,0,0},2,78) && !events.TriggerInteraction(1));
    Check(events.SetVisible("ev5",true));
    machine.script.clear();Check(!events.TriggerInteraction(1) && scripts.pushes==2);
    machine.properties.clear();Check(!events.FindInteraction({0,0,0},2,78));
}
int main(){try{MachineInteraction();auto arena=Fixture();Scripts scripts;SceneEvents* active=nullptr;
    SceneCollision collision([&](auto resource,bool entering,auto previous){Check(active!=nullptr);active->OnCollision(resource,entering,previous);});
    const auto resolve=[&](float x){std::array<float,4> upper{x,3,0,1},lower{x,1,0,1};Point3 movement{.1f,0,0};collision.Resolve(upper,lower,movement);};
    {SceneEvents events(arena,collision,scripts);active=&events;
        scripts.services.event_visible=[&](auto name,bool visible){events.SetVisible(name,visible);};
        events.ActivateAll();Check(events.States()[0].active && events.States()[0].movement_shapes==1);
        Check(events.States()[1].active && events.States()[1].movement_shapes==0);Check(events.States()[2].camera_requested && events.States()[2].movement_shapes==0);
        Check(events.States()[3].secondary_requested && events.States()[3].movement_shapes==1);
        resolve(0);Check(scripts.pushes==1 && scripts.stack.Depth()==1);
        {std::ostringstream first;events.WriteReport(first);Check(first.str().find("\"entries\":1")!=std::string::npos);}
        resolve(0);Check(scripts.pushes==1);
        resolve(50);resolve(0);Check(scripts.pushes==1); // 原活动脚本缓存阻止重入。
        Check(scripts.stack.Tick(0)==ScriptState::suspended);Check(scripts.stack.Tick(.1)==ScriptState::finished && scripts.globals[0]==1);
        resolve(50);resolve(0);Check(scripts.pushes==2);
        Check(events.SetVisible("ev1",false) && events.States()[0].movement_shapes==0);resolve(0);Check(scripts.pushes==2);
        Check(events.SetVisible("ev1",true) && events.States()[0].movement_shapes==1);Check(!events.SetVisible("missing",true));
        Check(scripts.stack.Tick(0)==ScriptState::suspended);Check(scripts.stack.Tick(.1)==ScriptState::finished);
        Check(scripts.Push("Q01","void Hidden()")==0 && scripts.stack.Tick(0)==ScriptState::finished);
        Check(!events.States()[0].active && events.States()[0].movement_shapes==0);
        std::ostringstream report;events.WriteReport(report);Check(report.str().find("\"pushes\":2")!=std::string::npos);
    }
    active=nullptr;resolve(0);std::ostringstream empty;collision.WriteReport(empty);Check(empty.str().find("\"shapes\":0")!=std::string::npos);
    bool rejected=false;try{collision.InterruptEventTraversal();}catch(const std::logic_error&){rejected=true;}Check(rejected);
    arena.description.event_collision_cache=false;rejected=false;try{SceneEvents events(arena,collision,scripts);}catch(const std::logic_error&){rejected=true;}Check(rejected);
    std::cout<<"scene event contracts passed\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
