#pragma once
#include "pal4/native_combat_actor_reaction_services.h"
#include "pal4/native_combat_actor_skill_runtime.h"
#include "pal4/native_combat_fatal_children.h"
#include "pal4/native_script_event_router.h"
#include "pal4/gom_task_base.h"
#include "pal4/gom_task_container.h"
#include <array>
#include <functional>
#include <iostream>
#include <stdexcept>
namespace pal4::test {
inline void VerifyNativeCombatFendOffImpact(NativeCombatActorSkillRuntime& source,
    NativeCombatActorSkillRuntime& target,NativeCombatSkillActorRegistry& registry,
    NativeScriptEventRouter& router,const char* label){
    const auto check=[](bool ok,const char* why){if(!ok)throw std::runtime_error(why);};
    const auto reject=[&](auto&& f){bool denied{};try{f();}catch(const std::logic_error&){denied=true;}check(denied,"FendOff ownership/service gate must reject");};
    const auto a=source.ActorAddress(),b=target.ActorAddress();
    const auto source_hp=source.CurrentStat(7u),target_hp=target.CurrentStat(7u);
    auto& property=*reinterpret_cast<std::uint32_t*>(a+800u);
    NativeCombatActorReactionServices status(registry);
    NativeCombatFatalChildren children(status);
    GomTaskBase base(children.BaseRuntime());GomTaskContainer container(children.ContainerRuntime(),base);
    GomTaskContainerState state{};const auto address=reinterpret_cast<std::uint32_t>(&state);
    (void)container.Initialize(state,0u);
    struct Cleanup{GomTaskContainer& container;GomTaskContainerState& state;NativeScriptEventRouter& router;
        std::uint32_t& property;std::uint32_t prior_property;std::array<std::uint32_t,2> tokens{};bool active{true};
        ~Cleanup(){property=prior_property;for(auto token:tokens)if(token)(void)router.Unsubscribe(token);
            if(active)(void)container.Destroy(state);}
    } cleanup{container,state,router,property,property};
    unsigned calls{},nested_calls{};std::function<void(std::uint32_t)> hook;
    cleanup.tokens[0]=router.Subscribe(router.Key("ae_fend_offed"),1u,[&](std::uint32_t payload){
        const auto* p=reinterpret_cast<const std::uint32_t*>(payload);
        if(p[0]!=a+716u)return;
        check(p[1]==b,"actual508 event has source facet and target identity");++calls;
        reject([&]{(void)children.Create(0x845d10u);});
        reject([&]{source.NotifyFendOffed(b);});
        reject([&]{source.Close();});reject([&]{target.Close();});
        if(hook)hook(payload);
    });
    cleanup.tokens[1]=router.Subscribe(router.Key("ae_fend_offed"),1u,[&](std::uint32_t payload){
        const auto* p=reinterpret_cast<const std::uint32_t*>(payload);
        if(p[0]==b+716u){check(p[1]==a,"nested other-actor508 keeps reverse target");++nested_calls;}
    });
    const auto schedule=[&](std::uint32_t first,std::uint32_t second){
        const auto child=children.Create(0x845d10u);auto& task=children.ContainerRuntime().TaskAt(child);
        check(task.vtable==0x845d18u && !task.context,"actual68-byte FendOff constructor");
        *reinterpret_cast<std::uint32_t*>(child+60u)=first;*reinterpret_cast<std::uint32_t*>(child+64u)=second;
        (void)container.AddTaskAndInitialize(state,address,task,child,"FendOffTask",{1u,0.f,0.f});
        (void)container.BeginWaiting(state);
        reject([&]{children.Destroy(child);});return child;
    };
    const auto drain=[&]{unsigned n{};for(;n<8u && state.count;++n)(void)container.Update(state,.016f);
        check(n<8u && state.ready && !children.Count() && !children.OwnedActionTexts(),"actual FendOff GOM task naturally retires");};
    reject([&]{(void)children.Create(0x845d14u);});reject([&]{(void)children.Create(0x845de0u);});
    reject([&]{status.CallActor(a,544u,b);});
    source.NotifyFendOffed(0u);check(!calls,"actual508 null target has no notification");
    hook=[&](std::uint32_t payload){
        const auto* p=reinterpret_cast<const std::uint32_t*>(payload);const auto record=std::array{p[0],p[1]};
        target.NotifyFendOffed(a);check(record==std::array{p[0],p[1]},"nested real events retain outer stack record");
    };
    (void)schedule(a,b);drain();hook={};check(calls==1u && nested_calls==1u,"actual impact invokes source508 once");
    for(auto pair:{std::array{0u,b},std::array{a,0u},std::array{0u,0u}}){(void)schedule(pair[0],pair[1]);drain();}
    check(calls==1u,"original missing-participant impact children are no-ops");
    (void)schedule(a+4u,b);reject([&]{(void)container.Update(state,0.f);});
    (void)container.CleanupAndReady(state);
    (void)schedule(a,b+4u);reject([&]{(void)container.Update(state,0.f);});
    (void)container.CleanupAndReady(state);check(calls==1u,"foreign participants fail before notifications");
    (void)schedule(a,b);hook=[](auto){throw std::runtime_error("intentional FendOff observer failure");};
    bool caught{};try{(void)container.Update(state,0.f);}catch(const std::runtime_error&){caught=true;}
    check(caught && state.count==1u && children.Count()==1u,"failed actual508 remains queued in original phase");
    hook={};drain();check(calls==3u,"GOM retries508 after observer failure without stuck callback guards");
    // Invalidate the real property only after the event. Forward60 must still
    // run and reject; restoring the receiver allows the original phase retry.
    (void)schedule(a,b);hook=[&](auto){property=0u;};
    reject([&]{(void)container.Update(state,0.f);});property=cleanup.prior_property;hook={};
    check(calls==4u && state.count==1u,"actual property60 follows event and failure preserves ownership");
    drain();check(calls==5u,"actual property60 service recovers after receiver restoration");
    (void)schedule(a,b);property=0u;reject([&]{(void)container.Update(state,0.f);});
    property=cleanup.prior_property;(void)container.CleanupAndReady(state);
    check(calls==5u,"missing actual property fails before event dispatch");
    const auto cancelled=schedule(a,b);(void)cancelled;(void)container.CleanupAndReady(state);
    check(calls==5u && !children.Count(),"cancellation before BeginRun sends no notification");
    const auto unlinked=children.Create(0x845d10u);children.Destroy(unlinked);
    (void)container.Destroy(state);cleanup.active=false;
    check(!children.OwnedAllocations() && source.CurrentStat(7u)==source_hp && target.CurrentStat(7u)==target_hp,
          "actual impact owns no remaining memory and changes neither HP");
    std::cout<<"Native combat FendOff impact: "<<label<<" actual68-byte ->508 event/property60, nested actors, null/foreign gates, observer/property retry, cancellation and zero ownership pass\n";
}
}
