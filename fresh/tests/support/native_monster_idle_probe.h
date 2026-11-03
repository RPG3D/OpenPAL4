#pragma once
#include "pal4/native_monster_behavior_session.h"
#include "pal4/native_world_path_planner_owner.h"
#include "pal4/native_monster_object_owner.h"
#include "pal4/native_actor_animation_state_binding.h"
#include "pal4/native_behavior_helper_storage.h"
#include "pal4/native_script_object_runtime.h"
#include "pal4/native_script_event_scheduler_runtime.h"
#include "pal4/native_monster_scene_update.h"
#include "pal4/native_monster_pursuit_frame.h"
#include <cstdlib>

namespace pal4::test {
inline void VerifyMonsterIdle(const std::filesystem::path& root,NativeBaseObjectMemory& memory,
    NativeMonsterManagerOwner& manager,NativeSceneMovementAsset& floor,
    NativePlayerTransformRuntime& transforms,std::uint32_t actor,
    NativeArenaCollisionTree& collision,IRwDictSchemaRuntime& schema,
    const GameMathTables& math,NativeMonsterProximityRuntime& proximity,std::uint32_t* control,std::uint32_t world) {
    const auto require=[](bool ok,const char* message){if(!ok)throw std::runtime_error(message);};
    NativeWorldPathPlannerOwner paths;
    require(paths.Construct()!=0u,"construct monster path planner");
    require(paths.LoadScene(root,"M06","3A"),"load original monster scene path resources");
    NativeActorBehaviorOrientationCaches caches{};
    std::uint32_t identifier{};
    NativeMonsterBehaviorSession session(manager,memory,paths,floor,transforms,caches,identifier,
        []{return static_cast<std::uint32_t>(std::rand());});
    auto& behavior=manager.OwnedComponent(actor).Runtime();
    auto& docs=behavior.EventDocuments();
    auto helpers=session.Helpers(actor,docs);
    // Real resource C03 admission/retirement, before any synthetic config.
    const auto raw=memory.Allocate(NativeBehaviorHelperStorage::ImageSize(1));
    helpers->Construct(1,raw);
    helpers->Accept(1,actor+0x6b8u,raw);
    require(manager.GridElementCount()==1u && session.LiveHelpers()==1u,
        "original C03 parameters accept real monster helper into owned grid");
    (void)manager.UpdateGridElements(std::bit_cast<std::uint32_t>(0.01f));
    helpers->Update(1,actor+0x6b8u,raw);
    helpers->Release(1,actor+0x6b8u,raw); // Deleting grid callback frees raw.
    require(!manager.GridElementCount() && !session.LiveHelpers(),"original C03 helper releases through grid owner");
    const auto retained_grid_allocations=manager.OutstandingAllocations();
    const auto last=docs.FindNumericAttribute(memory.ReadDword(actor+0x668u),"AAB_MST_memory_last_behavior_id");
    require(last && docs.AttributeToInt(last,-1)==1,"monster C03 save retains class1 rather than class2");
    const auto document=memory.ReadDword(actor+0x668u);
    const auto before_action=std::string(manager.AnimationBinding(actor).CurrentRecordName());
    const auto before_time=manager.AnimationBinding(actor).Time();
    docs.WriteFloatAttribute(document,"AAB_MST_memory_actiontime",-1.f);
    const auto rejected_helper=memory.Allocate(NativeBehaviorHelperStorage::ImageSize(1));
    helpers->Construct(1,rejected_helper);
    bool rejected=false;
    try {helpers->Accept(1,actor+0x6b8u,rejected_helper);}
    catch(const std::runtime_error& error) {rejected=std::string_view(error.what())=="actor behavior animation time rejected";}
    require(rejected && !session.LiveHelpers() && !manager.GridElementCount() && session.Stats().rolled_back==1u &&
        manager.AnimationBinding(actor).CurrentRecordName()==before_action && manager.AnimationBinding(actor).Time()==before_time,
        "failed monster admission after grid registration rolls back animation and retains helper for factory discard");
    helpers->Discard(1,rejected_helper);(void)memory.Free(rejected_helper);
    docs.WriteFloatAttribute(document,"AAB_MST_memory_actiontime",0.f);

    BinaryXmlNode config{"fixture"};
    for(int kind:{0,1,3}) {
        BinaryXmlNode params{"AAB_MST_parameters",{
            {"AABPedestrian_MST_walk_max_speed",2u,30.f},
            {"AABPedestrian_MST_walk_max_force",2u,10.f},
            {"AABPedestrian_MST_run_max_speed",2u,40.f},
            {"AABPedestrian_MST_run_max_force",2u,10.f},
            {"AABWander_MST_max_speed",2u,20.f},
            {"AABWander_MST_max_force",2u,10.f},
            {"AABWander_MST_max_range",2u,100.f}}};
        config.children.push_back({"behavior",{{"MSTINFO_Behavior_ObjectType",1u,std::int32_t(kind)}},{params}});
    }
    const auto previous=memory.ReadDword(actor+0x6d0u);
    struct Restore{NativeBaseObjectMemory& memory;std::uint32_t actor,node;~Restore(){memory.WriteDword(actor+0x6d0u,node);}} restore{memory,actor,previous};
    memory.WriteDword(actor+0x6d0u,reinterpret_cast<std::uint32_t>(&config));
    NativeBehaviorTasks factory(memory,behavior.Tasks(),actor,behavior.BehaviorTreeNil(),helpers);
    for(int kind=0;kind<4;++kind) {
        docs.WriteIntAttribute(memory.ReadDword(actor+0x668u),"AAB_MST_memory_last_behavior_id",kind);
        docs.WriteFloatAttribute(memory.ReadDword(actor+0x668u),"AAB_MST_memory_speed",10.f);
        const auto task=factory.Create(actor+0x698u,kind,actor+0x688u,"monster-idle-integration",{3,0.f,60.f});
        factory.Bind(task,actor+0x6acu,0u);
        (void)behavior.Tasks().BeginWaiting(actor+0x688u);
        for(unsigned tick=0;tick<3 && !session.LiveHelpers();++tick)
            (void)behavior.Tasks().UpdateTaskContainer(actor+0x688u,0.f);
        require(session.LiveHelpers()==1u && manager.GridElementCount()==1u,
            "all four monster task factories accept concrete helper services");
        for(unsigned tick=0;tick<4;++tick) {
            (void)manager.UpdateGridElements(std::bit_cast<std::uint32_t>(0.01f));
            (void)behavior.Tasks().UpdateTaskContainer(actor+0x688u,0.01f);
            require(manager.AnimationBinding(actor).AdvanceFrame(0.01f),"idle helper shares actual animation update");
        }
        (void)behavior.Tasks().ClearTaskContainer(actor+0x688u);
        require(!session.LiveHelpers() && !manager.GridElementCount() && !behavior.Tasks().RegisteredTasks(),
            "task cancellation retires helper and spatial association once");
        require(manager.OutstandingAllocations()==retained_grid_allocations,
            "all-four helper cycles reuse the manager's retained pointer-vector capacity");
        const auto saved=docs.FindNumericAttribute(memory.ReadDword(actor+0x668u),"AAB_MST_memory_last_behavior_id");
        require(saved && docs.AttributeToInt(saved,-1)==kind,"all monster saves retain actual behavior kind");
    }
    require(session.Stats().accepted==5u && session.Stats().released==5u && session.Stats().grid_frames>=17u,
        "native monster helpers actually run grid and task callbacks");
    memory.WriteDword(actor+0x6d0u,previous);
    (void)manager.ResetOwnedPursuit(actor);
    class NoEncounter final : public IMonsterEncounterRuntime {
    public:
        void DispatchCombatPositions(std::uint32_t,float) override {throw std::logic_error("idle fixture has no controlled encounter target");}
        void InitializeCombat(std::uint32_t,std::int32_t) override {throw std::logic_error("idle fixture must not enter Combat");}
    } encounter;
    MonsterInteractionRuntimeStub interaction; // Unused with no controlled actor.
    std::array<std::uint32_t,14> empty_skills{};
    const auto controlled=control[1u],busy=control[0x2e0u/4u];
    struct ControlRestore {std::uint32_t* control;std::uint32_t actor,busy;
        ~ControlRestore(){control[1u]=actor;control[0x2e0u/4u]=busy;}} control_restore{control,controlled,busy};
    control[1u]=0u;control[0x2e0u/4u]=1u;
    NativeMonsterSceneUpdate update({manager,memory,paths,collision,floor,transforms,schema,math,proximity,
        reinterpret_cast<std::uint32_t>(control),interaction,encounter,
        [&]{return reinterpret_cast<std::uint32_t>(empty_skills.data());},caches,identifier,
        []{return static_cast<std::uint32_t>(std::rand());}});
    for(unsigned frame=0;frame<1800;++frame)update.Update(0.01f,world);
    require(update.Stats().accepted>0u && update.Stats().grid_frames>0u && update.Stats().target_updates>0u,
        "full scene composition naturally schedules original idle events across three real monsters");
    update.Stop();
    require(!manager.GridElementCount(),"full scene Stop cancels all outstanding helpers before scene release");
    std::cout<<"Native monster full scene1800: accepted="<<update.Stats().accepted
        <<" grid="<<update.Stats().grid_frames<<" targets="<<update.Stats().target_updates<<'\n';
    std::cout<<"PASS: real monster C03 admission plus all-four native idle tasks/grid/orientation/cancellation\n";
}
}
