#pragma once
#include "pal4/native_combat_message_children.h"
#include "pal4/native_combat_actor_action_tasks.h"
#include "pal4/native_combat_actor_skill_runtime.h"
#include "pal4/native_script_event_router.h"
#include "pal4/native_effect_scene_session.h"
#include "pal4/native_effect_creation.h"
#include "pal4/native_player_slot_copy_runtime.h"
#include "pal4/gom_task_base.h"
#include "pal4/gom_task_container.h"
#include "pal4/effect_handle.h"
#include <array>
#include <cstring>
#include <functional>
#include <iostream>
#include <stdexcept>
namespace pal4::test {
inline void VerifyNativeCombatMessageChildren(NativeCombatActorActionTasks& playback,
    NativeCombatActorSkillRuntime& actor,NativeCombatMessageChildren::Model model,
    NativeEffectSceneSession& effects,NativeScriptEventRouter& router,
    const std::function<void(float)>& advance,float& scale,const char* label){
    const auto check=[](bool ok,const char* why){if(!ok)throw std::runtime_error(why);};
    const auto reject=[&](auto&& f){bool denied{};try{f();}catch(const std::logic_error&){denied=true;}check(denied,"message child gate must reject");};
    const auto a=actor.ActorAddress();const auto hp=actor.CurrentStat(7u);const auto prior=playback.GetAnimationName(a);
    NativePlayerSlotCopyRuntime text;std::array<std::uint32_t,4> header{};const auto h=reinterpret_cast<std::uint32_t>(header.data());
    struct Restore{NativeCombatActorActionTasks& p;NativePlayerSlotCopyRuntime& text;std::uint32_t a,h;std::string prior;
        ~Restore(){try{text.AssignText(h,prior);p.SetActorTarget(a,h,1u);}catch(...){}text.ReleaseText(h);}
    } restore{playback,text,a,h,prior};
    const auto subscriptions=router.SubscriptionCount();
    NativeCombatMessageChildren children(actor,playback,std::move(model),effects,router,scale);
    GomTaskBase base(children.BaseRuntime());GomTaskContainer container(children.ContainerRuntime(),base);
    GomTaskContainerState state{};const auto address=reinterpret_cast<std::uint32_t>(&state);(void)container.Initialize(state,0u);
    struct Cleanup{GomTaskContainer& c;GomTaskContainerState& s;bool active{true};~Cleanup(){if(active)(void)c.Destroy(s);}} cleanup{container,state};
    auto schedule=[&]{const auto child=children.Create("z03");auto& t=children.ContainerRuntime().TaskAt(child);
        check(t.vtable==0x8456c4u && *reinterpret_cast<std::uint32_t*>(child+60u)==a,"actual216-byte constructor and actor");
        for(unsigned i=0;i<5u;++i)check(!children.ContainerRuntime().ReadWord(child+96u+24u*i+4u),"actual five slots initially unowned");
        (void)container.AddTaskAndInitialize(state,address,t,child,"PlayActionAndMessageTask",{1u,0.f,0.f});
        (void)container.BeginWaiting(state);reject([&]{children.Destroy(child);});reject([&]{children.Close();});
        (void)container.Update(state,0.f);
        check(children.SubscriptionCount()==1u && router.SubscriptionCount()==subscriptions+2u,"actual task owns two original mode2 subscriptions");
        return child;
    };
    auto drain=[&](bool effect_frames){unsigned n{};for(;n<600u && state.count;++n){
        advance(.016f);if(effect_frames)(void)effects.UpdateCombatEffects(.016);(void)container.Update(state,.016f);}
        check(n<600u && !children.Count() && !children.SubscriptionCount() && !children.OwnedTexts() && router.SubscriptionCount()==subscriptions,
              "actual message task animation/effects naturally complete and release ownership");return n;
    };
    (void)schedule();
    std::array<std::uint32_t,2> event{a+716u,0u};
    reject([&]{(void)router.Dispatch("ae_animation_message",reinterpret_cast<std::uint32_t>(event.data()));});
    std::array<unsigned char,0x14c> malformed{};std::memset(malformed.data()+128u,'x',64u);
    event[1]=reinterpret_cast<std::uint32_t>(malformed.data());
    reject([&]{(void)router.Dispatch("ae_animation_message",reinterpret_cast<std::uint32_t>(event.data()));});
    check(children.Count()==1u && children.SubscriptionCount()==1u,"invalid event keeps actual task/subscriptions recoverable");
    const auto frames=drain(false);
    check(children.Commands()>0u,"actual AMF commands reach actual216-byte task and shared processor");
    // Explicit slot fixtures carry five real Dialog handles; no fake ready flags
    // or elapsed completion. Hold effect updates until real animation completion
    // to verify each active slot participates in the original completion gate.
    const auto held=schedule();std::array<EffectSystemHandleWords,5> handles{};
    for(unsigned i=0;i<5u;++i){const auto made=effects.CombatEffectCreation().Create("foot_2",{0,1,1},1,0u);
        handles[i]={made.id,made.system};check(EffectHandle::DialogNeedsUpdate(handles[i],effects.ContentHandles()),"actual slot Dialog exists");
        *reinterpret_cast<EffectSystemHandleWords*>(held+96u+24u*i)=handles[i];}
    std::array<std::uint32_t,6> message{handles[0].id,handles[0].system,0u,0u,0u,0u};
    (void)router.Dispatch("effect_message",reinterpret_cast<std::uint32_t>(message.data()));
    check(EffectHandle::DialogNeedsUpdate(handles[0],effects.ContentHandles()) &&
        children.ContainerRuntime().ReadWord(held+96u)==handles[0].id,"base effect message resets a value copy, not the actual live slot");
    {
        auto& slot=*reinterpret_cast<EffectSystemHandleWords*>(held+96u);
        struct RestoreSlot{EffectSystemHandleWords& slot;EffectSystemHandleWords prior;~RestoreSlot(){slot=prior;}} restore_slot{slot,slot};
        slot.system+=4u;reject([&]{(void)children.ContainerRuntime().IsDurationComplete(held);});
    }
    unsigned waited{};for(;waited<600u && !playback.GetAnimationFlag(a);++waited)advance(.016f);
    check(waited<600u && !children.ContainerRuntime().IsDurationComplete(held),"five actual live effects hold completed animation task");
    for(unsigned i=0;i<4u;++i){auto& slot=*reinterpret_cast<EffectSystemHandleWords*>(held+96u+24u*i);
        effects.ReleaseCombatEffect(slot);check(!children.ContainerRuntime().IsDurationComplete(held),"remaining actual effect slot still holds completion");}
    const auto effect_frames=drain(true);
    const auto cancelled=schedule();
    auto made=effects.CombatEffectCreation().Create("foot_2",{0,1,1},1,0u);
    *reinterpret_cast<EffectSystemHandleWords*>(cancelled+96u)=EffectSystemHandleWords{made.id,made.system};
    (void)container.CleanupAndReady(state);
    check(!children.Count() && !children.SubscriptionCount() && router.SubscriptionCount()==subscriptions &&
        !EffectHandle::DialogNeedsUpdate({made.id,made.system},effects.ContentHandles()),"active cancellation releases actual effect and subscriptions");
    const auto unlinked=children.Create("z03");children.Destroy(unlinked);
    reject([&]{(void)children.CreatePhysical("z03",a,1u,2u,{});});
    unsigned attacks{},attempts{};bool fail_attack{};std::uint32_t physical{};
    const auto processor=[&](std::uint32_t embedded){
        check(embedded==physical+216u && children.ContainerRuntime().ReadWord(embedded)==0x8456ecu &&
              children.ContainerRuntime().ReadWord(embedded+60u)==a && children.ContainerRuntime().ReadWord(embedded+64u)==a &&
              *reinterpret_cast<const std::uint8_t*>(embedded+72u)==1u && *reinterpret_cast<const std::uint8_t*>(embedded+73u)==2u,
              "physical AMF callback receives actual embedded76-byte operands");
        ++attempts;reject([&]{children.Close();});reject([&]{children.Destroy(physical);});
        if(fail_attack)throw std::runtime_error("intentional physical attack service failure");++attacks;
    };
    physical=children.CreatePhysical("z03",a,1u,2u,processor);
    check(children.ContainerRuntime().TaskAt(physical).vtable==0x8459fcu,
          "physical child original292-byte final vtable");
    // Embedded GOM name allocation proves physical destruction retires both
    // bases through the same allocator, including cancellation.
    const auto embedded_name=children.ContainerRuntime().AllocateMemory(8u);
    children.ContainerRuntime().WriteWord(physical+216u+offsetof(GomTaskBaseState,name),embedded_name);
    (void)container.AddTaskAndInitialize(state,address,children.ContainerRuntime().TaskAt(physical),physical,"PhysicalAttackChild",{1u,0,0});
    (void)container.BeginWaiting(state);(void)container.Update(state,0.f);
    std::array<unsigned char,0x14c> attack_message{};std::memcpy(attack_message.data()+128u,"AtK_probe",10u);
    event[1]=reinterpret_cast<std::uint32_t>(attack_message.data());fail_attack=true;bool attack_failed{};
    try{(void)router.Dispatch("ae_animation_message",reinterpret_cast<std::uint32_t>(event.data()));}
    catch(const std::runtime_error&){attack_failed=true;}
    check(attack_failed && attempts==1u && !attacks && children.Count()==1u,"failed physical service retains active task");
    fail_attack=false;(void)router.Dispatch("ae_animation_message",reinterpret_cast<std::uint32_t>(event.data()));
    check(attempts==2u && attacks==1u,"physical service retry releases callback guard");
    const auto physical_frames=drain(false);
    check(attacks>1u && children.OwnedAllocations()==1u,"actual z03 atk invokes borrowed processor and retires both GOM bases");
    const auto cancelled_physical=children.CreatePhysical("z03",a,1u,2u,processor);
    physical=cancelled_physical;
    (void)container.AddTaskAndInitialize(state,address,children.ContainerRuntime().TaskAt(physical),physical,"PhysicalAttackCancel",{1u,0,0});
    (void)container.BeginWaiting(state);(void)container.Update(state,0.f);
    (void)container.CleanupAndReady(state);
    check(children.OwnedAllocations()==1u && !children.Count() && !children.SubscriptionCount(),"physical cancellation retires callback/subscriptions/both bases");
    (void)container.Destroy(state);cleanup.active=false;
    check(!children.OwnedAllocations() && !children.OwnedTexts() && actor.CurrentStat(7u)==hp,"message child task/name/slot/list allocation balance and unchanged HP");
    children.Close();reject([&]{(void)children.Create("z03");});
    std::cout<<"Native combat message child: "<<label<<" actual216-byte z03 commands="<<children.Commands()<<" frames="<<frames
        <<" five real Dialog slots gate/drain="<<effect_frames<<", actual GOM, subscription retirement, cancellation and zero ownership pass\n";
    std::cout<<"Native physical attack child: "<<label<<" actual292/embedded76, z03 atk callbacks="<<attacks
        <<" frames="<<physical_frames<<", service failure/retry, cancellation and both-base ownership pass; damage processor is explicit fixture\n";
}
}
