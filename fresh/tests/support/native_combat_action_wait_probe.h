#pragma once
#include "pal4/native_combat_actor_action_tasks.h"
#include "pal4/native_combat_action_messages.h"
#include "pal4/native_script_event_router.h"
#include "pal4/native_player_slot_copy_runtime.h"
#include "pal4/combat_action_task_operations.h"
#include <array>
#include <functional>
#include <cstring>
#include <iostream>
#include <stdexcept>
namespace pal4::test {
// The message-task header is an explicit fixture. Playback, animation records,
// model updates and completion flags are actual Player/Monster services.
inline void VerifyNativeCombatActionWait(NativeCombatActorActionTasks& tasks,
    std::uint32_t actor,const std::function<void(float)>& advance,const char* label,NativeScriptEventRouter& router){
    const auto check=[](bool b,const char* why){if(!b)throw std::runtime_error(why);};
    const auto reject=[&](auto&& f){bool denied{};try{f();}catch(const std::logic_error&){denied=true;}check(denied,"action wait operation must reject");};
    std::array<std::uint32_t,24> task{};const auto address=reinterpret_cast<std::uint32_t>(task.data());
    const auto header=address+64u;NativePlayerSlotCopyRuntime text;
    const auto prior=tasks.GetAnimationName(actor);
    struct Restore{NativeCombatActorActionTasks& tasks;NativePlayerSlotCopyRuntime& text;std::uint32_t actor,header;std::string name;
        ~Restore(){try{text.AssignText(header,name);tasks.SetActorTarget(actor,header,1u);}catch(...){}text.ReleaseText(header);}} restore{tasks,text,actor,header,prior};
    text.AssignText(header,"z02");
    check(CombatActionTaskOperations::ProcessAction(address,tasks)==1u && tasks.GetAnimationName(actor)==prior,"null task actor does not play an action");
    task[15]=actor;check(tasks.SkillSystem()!=0u,"actual SkillSystem prerequisite");
    check(CombatActionTaskOperations::ProcessAction(address,tasks)==1u,"shared ProcessAction forwards actual564 mode0");advance(0.f);
    check(tasks.CompareInsensitive(tasks.GetAnimationName(actor),"Z02")==0 && tasks.GetAnimationFlag(actor)==0 &&
        !CombatActionTaskOperations::IsAnimationComplete(address,tasks),"real z02 starts incomplete and compares names without case");
    unsigned frames{};for(;frames<600u && !CombatActionTaskOperations::IsAnimationComplete(address,tasks);++frames)advance(0.016f);
    check(frames>0u && frames<600u && tasks.GetAnimationFlag(actor)>0 && CombatActionTaskOperations::IsAnimationComplete(address,tasks),
        "actual SDK clip naturally reaches original completion flag");
    text.AssignText(header,"z01");check(CombatActionTaskOperations::IsAnimationComplete(address,tasks),"interrupted/name-mismatched animation completes original wait");
    text.AssignText(header,"z02");
    auto& child=*reinterpret_cast<std::uint32_t*>(actor+808u);const auto original_child=child;
    auto& flag=*reinterpret_cast<std::uint32_t*>(original_child+440u);
    const auto old_flag=flag;flag=0x80000000u;
    const auto signed_flag=tasks.GetAnimationFlag(actor);const auto complete=CombatActionTaskOperations::IsAnimationComplete(address,tasks);flag=old_flag;
    check(signed_flag<0 && !complete,"completion retains original signed-positive flag test");
    reject([&]{(void)tasks.GetAnimationName(actor+4u);});reject([&]{(void)tasks.GetAnimationFlag(actor+4u);});
    reject([&]{tasks.StartActorAction(actor+4u,0u,0u);});
    struct ChildRestore{std::uint32_t& field;std::uint32_t old;~ChildRestore(){field=old;}} child_restore{child,child};
    child=0u;
    const auto absent_name=tasks.GetAnimationName(actor);
    const auto absent_flag=tasks.GetAnimationFlag(actor);
    tasks.StartActorAction(actor,0u,0u);child=original_child;
    check(absent_name.empty() && absent_flag==1,"detached actor defaults and no-text-read playback gate");
    {
        NativeCombatActionMessages messages(router);
        const auto before=router.SubscriptionCount();unsigned commands{},attack_commands{};
        // Record actual AMF callbacks only. This observer is not an attack/damage consumer.
        messages.Register(address,[&](std::uint32_t owner,std::uint32_t event){
            check(owner==address && event,"real animation callback owner/event");
            const auto* name=reinterpret_cast<const char*>(event+128u);
            const auto n=strnlen_s(name,64u);check(n>0u && n<64u,"real AMF message_name view");
            ++commands;if(n>=3u && _strnicmp(name,"atk",3u)==0)++attack_commands;
        },[&](auto,auto,auto,auto){throw std::logic_error("unexpected effect message during animation-only probe");});
        text.AssignText(header,"z03");(void)CombatActionTaskOperations::ProcessAction(address,tasks);advance(0.f);
        unsigned attack_frames{};
        for(;attack_frames<600u && !CombatActionTaskOperations::IsAnimationComplete(address,tasks);++attack_frames)advance(0.016f);
        check(attack_frames>0u && attack_frames<600u && commands>0u && attack_commands>0u,
              "real z03 AMF reaches subscribed task and includes actual atk command");
        messages.Detach(address);
        check(router.SubscriptionCount()==before && !task[20] && !task[21] && !task[22] && !task[23],"real task subscription cleanup");
        const auto observed=commands;
        (void)CombatActionTaskOperations::ProcessAction(address,tasks);advance(0.f);
        for(unsigned i=0u;i<600u && !CombatActionTaskOperations::IsAnimationComplete(address,tasks);++i)advance(0.016f);
        check(commands==observed,"retired task does not receive actual later animation messages");
        std::cout<<"Native combat action messages: "<<label<<" actual z03 AMF commands="<<commands<<" atk="<<attack_commands
            <<" frames="<<attack_frames<<" and retired subscriptions pass\n";
    }
    text.ReleaseText(header);
    check(!text.OwnedTextCount(),"action task fixture releases host string");
    std::cout<<"Native combat action wait: "<<label<<" actual ProcessAction/z02 records, natural SDK completion frames="<<frames
        <<", mismatch/signed/null/foreign gates and host text cleanup pass\n";
}
}
