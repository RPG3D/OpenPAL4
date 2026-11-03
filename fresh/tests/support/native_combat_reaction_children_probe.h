#pragma once
#include "pal4/native_combat_fatal_children.h"
#include "pal4/native_combat_actor_action_tasks.h"
#include "pal4/native_player_slot_copy_runtime.h"
#include "pal4/gom_task_base.h"
#include "pal4/gom_task_container.h"
#include <array>
#include <functional>
#include <iostream>
#include <stdexcept>
namespace pal4::test {
inline void VerifyNativeCombatReactionChildren(NativeCombatActorActionTasks& playback,
    std::uint32_t actor,const std::function<void(float)>& advance,
    const NativeCombatFatalChildren::PrepareAction& prepare,const char* label){
    const auto check=[](bool ok,const char* why){if(!ok)throw std::runtime_error(why);};
    const auto reject=[&](auto&& f){bool failed{};try{f();}catch(const std::logic_error&){failed=true;}check(failed,"reaction child ownership must reject");};
    const auto prior=playback.GetAnimationName(actor);
    // These resource-probe actors have not entered production Combat. Supply
    // an explicit reversible ready/alive fixture so original396 can prepare.
    auto& facet=*reinterpret_cast<std::uint32_t*>(actor+828u);
    auto& health=*reinterpret_cast<std::uint32_t*>(actor+648u);
    struct StateRestore{std::uint32_t& facet;std::uint32_t& health;std::uint32_t old_facet,old_health;
        ~StateRestore(){facet=old_facet;health=old_health;}} state_restore{facet,health,facet,health};
    facet=1u;health=0u;
    std::array<std::uint32_t,4> header{};const auto text_header=reinterpret_cast<std::uint32_t>(header.data());
    NativePlayerSlotCopyRuntime text;
    struct Restore{NativeCombatActorActionTasks& playback;NativePlayerSlotCopyRuntime& text;
        std::uint32_t actor,header;std::string prior;
        ~Restore(){try{text.AssignText(header,prior);playback.SetActorTarget(actor,header,1u);}catch(...){}text.ReleaseText(header);}
    } restore{playback,text,actor,text_header,prior};
    reject([&]{NativeCombatFatalChildren missing(playback,{});});
    unsigned prepared{},attempts{};bool fail{};
    NativeCombatFatalChildren* active{};
    NativeCombatFatalChildren children(playback,[&](std::uint32_t receiver,std::uint32_t flag){
        check(receiver==actor && flag==1u,"actual prepare child passes actor396(1)");++attempts;
        reject([&]{(void)active->Create(0x845d14u);});
        if(fail)throw std::runtime_error("intentional reaction preparation failure");
        prepare(receiver,flag);++prepared;
    });active=&children;
    reject([&]{(void)children.Create(0x845de0u);});reject([&]{(void)children.Create(0x845decu);});
    GomTaskBase base(children.BaseRuntime());GomTaskContainer containers(children.ContainerRuntime(),base);
    GomTaskContainerState state{};const auto address=reinterpret_cast<std::uint32_t>(&state);
    (void)containers.Initialize(state,0u);
    struct Cleanup{GomTaskContainer& containers;GomTaskContainerState& state;bool active{true};
        ~Cleanup(){if(active)(void)containers.Destroy(state);}} cleanup{containers,state};
    auto schedule=[&]{
        const auto animation=children.Create(0x845d14u),ready=children.Create(0x845a80u);
        check(animation && ready,"actual80/64 reaction child constructors");
        children.AssignAction(animation,"z04");
        *reinterpret_cast<std::uint32_t*>(animation+60u)=actor;*reinterpret_cast<std::uint32_t*>(ready+60u)=actor;
        (void)containers.AddTaskAndInitialize(state,address,children.ContainerRuntime().TaskAt(animation),animation,"ChangeActionTask",{1u,0.f,0.f});
        (void)containers.AddTaskAndInitialize(state,address,children.ContainerRuntime().TaskAt(ready),ready,"ChangeToPrepareActionTask",{1u,0.f,0.f});
        (void)containers.BeginWaiting(state);
        reject([&]{children.AssignAction(animation,"z01");});reject([&]{children.Destroy(animation);});
        reject([&]{(void)children.ContainerRuntime().TaskAt(animation+4u);});
        (void)containers.Update(state,0.f);advance(0.f);
        check(playback.CompareInsensitive(playback.GetAnimationName(actor),"z04")==0 && !playback.GetAnimationFlag(actor),
              "real GOM80 starts actual wounded animation, initially incomplete");
    };
    schedule();check(!prepared,"prepare child waits behind actual animation");
    unsigned frames{};
    for(;frames<600u && state.count;++frames){advance(.016f);(void)containers.Update(state,.016f);}
    check(frames>1u && frames<600u && prepared==1u && state.ready && !children.Count() && !children.OwnedActionTexts(),
          "natural actual clip completion releases wait then original396 preparation");
    check(playback.CompareInsensitive(playback.GetAnimationName(actor),"z04")!=0,"actual preparation leaves wounded action");
    fail=true;schedule();bool caught{};
    for(unsigned frame=0u;frame<600u && !caught;++frame){advance(.016f);
        try{(void)containers.Update(state,.016f);}catch(const std::runtime_error&){caught=true;}}
    check(caught && prepared==1u && attempts==2u && state.count==2u,"prepare failure leaves original GOM stage retryable");
    fail=false;for(unsigned frame=0u;frame<8u && state.count;++frame)(void)containers.Update(state,0.f);
    check(!state.count && prepared==2u && attempts==3u && !children.Count() && !children.OwnedActionTexts(),"failed callback guard recovers and retires both children");
    const auto cancelled=children.Create(0x845d14u);children.AssignAction(cancelled,"z04");children.Destroy(cancelled);
    check(!children.Count() && !children.OwnedActionTexts(),"unlinked cancellation releases actual host action text");
    (void)containers.Destroy(state);cleanup.active=false;
    check(!children.OwnedAllocations(),"reaction task names, sentinel, list nodes and objects all retired");
    std::cout<<"Native combat reaction children: "<<label<<" actual80-byte z04 wait ->64-byte396 preparation, frames="<<frames
        <<", GOM order, failure retry, reentry/foreign/linked guards and zero ownership pass\n";
}
}
