#pragma once
#include "pal4/native_combat_action_children.h"
#include "pal4/native_combat_actor_skill_runtime.h"
#include "pal4/native_combat_scene_actors.h"
#include "pal4/native_script_event_router.h"
#include "pal4/game_resource_manager.h"
#include "pal4/gom_task_base.h"
#include "pal4/gom_task_container.h"
#include <array>
#include <iostream>
#include <stdexcept>
#include <vector>
namespace pal4::test {
inline void VerifyNativeCombatProtectionChildren(NativeCombatActorSkillRuntime& skills,
    INativeCombatSceneActors& models,NativeScriptEventRouter& router,const char* label){
    const auto check=[](bool b,const char* why){if(!b)throw std::runtime_error(why);};
    const auto reject=[&](auto&& f){bool denied{};try{f();}catch(const std::logic_error&){denied=true;}check(denied,"protection child operation must reject");};
    const auto actor=skills.ActorAddress(),child=models.SceneChild(actor);
    check(child!=0u,"protection child probe requires actual model");
    const auto position=[&]{return *reinterpret_cast<const std::array<float,3>*>(child+172u);};
    struct Restore{INativeCombatSceneActors& models;std::uint32_t actor;std::array<float,3> p;
        ~Restore(){try{models.PositionSceneModel(actor,{p[0],p[1],p[2]});}catch(...){}}} restore{models,actor,position()};
    NativeCombatActionChildren* running{};bool fail_move{},fail_protect{};
    unsigned moves{},responses{};std::vector<unsigned> order;
    NativeCombatActionChildren owner([&](std::uint32_t a,const std::array<float,3>& p){
        check(a==actor,"translation receives actual actor");
        reject([&]{(void)running->Create(0x84591cu);});reject([&]{running->Close();});
        if(fail_move)throw std::logic_error("intentional position service failure");
        models.PositionSceneModel(a,{p[0],p[1],p[2]});++moves;order.push_back(1u);
    },[&](std::uint32_t a,std::uint32_t target,std::uint32_t other){
        check(a==actor && target==actor && other==actor,"response uses original receiver/target/other order");
        reject([&]{running->Close();});
        if(fail_protect)throw std::logic_error("intentional protection service failure");
        (void)skills.ProtectAction(target,other);++responses;order.push_back(2u);
    });running=&owner;
    struct Route{NativeScriptEventRouter& router;std::uint32_t handle;~Route(){(void)router.Unsubscribe(handle);}} route{router,0u};
    unsigned records{};
    route.handle=router.Subscribe(router.Key("ae_protect_action"),1u,[&](std::uint32_t payload){
        const auto* w=reinterpret_cast<const std::uint32_t*>(payload);
        check(w[0]==actor+716u && w[1]==actor && w[2]==actor,"scheduled response delivers actual three-word actor record");++records;
        reject([&]{owner.Close();});reject([&]{skills.Close();});
    });
    auto& runtime=owner.ContainerRuntime();GomTaskBase base(owner.BaseRuntime());GomTaskContainer queue(runtime,base);
    GomTaskContainerState state{};const auto address=reinterpret_cast<std::uint32_t>(&state);
    queue.Initialize(state,0u);
    struct Cleanup{GomTaskContainer& queue;GomTaskContainerState& state;~Cleanup(){if(state.sentinel)try{(void)queue.Destroy(state);}catch(...){}}} cleanup{queue,state};
    reject([&]{NativeCombatActionChildren invalid({},[](auto,auto,auto){});});
    reject([&]{(void)owner.Create(0u);});reject([&]{owner.Close();});
    const auto unconfigured=owner.Create(0x845920u);
    reject([&]{runtime.BeginRun(unconfigured);});owner.Destroy(unconfigured);
    for(unsigned cycle=0;cycle<3u;++cycle){
        const auto translation=owner.Create(0x84591cu),response=owner.Create(0x845920u);
        check(runtime.TaskAt(translation).vtable==0x8459c4u && runtime.ReadWord(translation+72u)==0u &&
            runtime.TaskAt(response).vtable==0x8459e0u,"actual protection child constructors and layouts");
        (void)queue.AddTaskAndInitialize(state,address,runtime.TaskAt(translation),translation,"position",{1u,0.f,0.f});
        (void)queue.AddTaskAndInitialize(state,address,runtime.TaskAt(response),response,"response",{1u,0.f,0.f});
        const std::array<float,3> goal{restore.p[0]+3.f+cycle,restore.p[1],restore.p[2]-4.f};
        owner.ConfigurePosition(translation,actor,goal);owner.ConfigureResponse(response,actor,actor,actor);
        reject([&]{owner.ConfigureResponse(translation,actor,actor,actor);});reject([&]{owner.Destroy(translation);});
        (void)queue.BeginWaiting(state);
        reject([&]{owner.ConfigurePosition(translation,actor,goal);});
        fail_move=cycle==1u;fail_protect=cycle==2u;
        if(fail_move){reject([&]{(void)queue.Update(state,0.1f);});check(runtime.TaskAt(translation).phase==2u && !state.ready,"failed movement does not falsely finish");fail_move=false;}
        (void)queue.Update(state,0.1f);
        check(position()==goal && runtime.TaskAt(translation).phase==3u && runtime.TaskAt(response).phase==2u,
            "real model moves before dependent protection response");
        if(fail_protect){
            bool failed{};for(unsigned tick=0;tick<8u && !failed;++tick)try{(void)queue.Update(state,0.1f);}catch(const std::logic_error&){failed=true;}
            check(failed && runtime.TaskAt(response).phase==2u && !state.ready,"failed response remains retryable");fail_protect=false;
        }
        for(unsigned tick=0;tick<12u && !state.ready;++tick)(void)queue.Update(state,0.1f);
        check(state.ready && !state.count && owner.Count()==0u && owner.OwnedAllocations()==1u,
            "shared GOM completes and releases tasks/names/list nodes");
        reject([&]{owner.Destroy(response);});
    }
    check(moves==3u && responses==3u && records==3u && order==std::vector<unsigned>{1u,2u,1u,2u,1u,2u},
        "actual model/event dispatch is ordered and occurs once after retry");
    const auto null_position=owner.Create(0x84591cu);
    owner.ConfigurePosition(null_position,0u,{0.f,0.f,0.f});runtime.BeginRun(null_position);owner.Destroy(null_position);
    check(moves==3u,"null position actor preserves original no-op gate");
    (void)queue.Destroy(state);state.sentinel=0u;check(!owner.Count() && !owner.OwnedAllocations(),"balanced protection children and GOM storage");
    owner.Close();reject([&]{(void)owner.Create(0x84591cu);});
    std::cout<<"Native combat protection children: "<<label<<" actual model translation and protect record, shared GOM ordering/retry, guards and zero allocations pass\n";
}
}
