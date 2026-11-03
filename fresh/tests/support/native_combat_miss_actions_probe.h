#pragma once
#include "pal4/native_combat_miss_actions.h"
#include "pal4/native_combat_fatal_children.h"
#include "pal4/native_combat_actor_action_tasks.h"
#include "pal4/native_combat_actor_skill_runtime.h"
#include "pal4/native_combat_skill_actor_registry.h"
#include "pal4/native_combat_system_image.h"
#include "pal4/native_combat_task_queue.h"
#include "pal4/native_script_event_router.h"
#include "pal4/native_player_slot_copy_runtime.h"
#include "pal4/native_animated_actor_asset.h"
#include <array>
#include <functional>
#include <iostream>
#include <stdexcept>
namespace pal4::test {
inline bool HasNativeMissRecord(const NativeAnimatedActorAsset& asset){
    for(std::size_t i=0;i<asset.ActionCount();++i)
        if(_stricmp(std::string(asset.ActionAt(i)).c_str(),"z06")==0)return true;
    return false;
}
inline void VerifyNativeCombatMissActions(NativeCombatActorActionTasks& playback,
    NativeCombatActorSkillRuntime& source,NativeCombatActorSkillRuntime& target,
    NativeCombatSkillActorRegistry& registry,NativeCombatSystemImage& image,INativeSkillSystemServices& system,
    NativeScriptEventRouter& router,const std::function<void(float)>& advance,
    const NativeCombatFatalChildren::PrepareAction& prepare,const char* label,
    const std::function<bool()>& has_miss_record){
    const auto check=[](bool ok,const char* why){if(!ok)throw std::runtime_error(why);};
    const auto reject=[&](auto&& f){bool denied{};try{f();}catch(const std::logic_error&){denied=true;}check(denied,"Miss owner must reject");};
    const auto a=source.ActorAddress(),b=target.ActorAddress();
    auto& facet=*reinterpret_cast<std::uint32_t*>(b+828u);auto& health=*reinterpret_cast<std::uint32_t*>(b+648u);
    const auto prior=playback.GetAnimationName(b);const auto source_hp=source.CurrentStat(7u),target_hp=target.CurrentStat(7u);
    std::array<std::uint32_t,4> header{};const auto h=reinterpret_cast<std::uint32_t>(header.data());NativePlayerSlotCopyRuntime text;
    struct Restore{NativeCombatActorActionTasks& playback;NativePlayerSlotCopyRuntime& text;NativeScriptEventRouter& router;
        std::uint32_t actor,header,token{};std::uint32_t& facet;std::uint32_t& health;std::uint8_t& mode;
        std::uint32_t old_facet,old_health;std::uint8_t old_mode;std::string prior;
        ~Restore(){if(token)(void)router.Unsubscribe(token);try{text.AssignText(header,prior);playback.SetActorTarget(actor,header,1u);}catch(...){}
            text.ReleaseText(header);facet=old_facet;health=old_health;mode=old_mode;}
    } restore{playback,text,router,b,h,0u,facet,health,image.Mode(),facet,health,image.Mode(),prior};
    facet=1u;health=0u;image.Mode()=0u;
    unsigned notifications{},prepared{},identity_actions{};bool fail_notification{},fail_prepare{};
    NativeCombatFatalChildren children(playback,[&](std::uint32_t receiver,std::uint32_t flag){
        check(receiver==b && flag==1u,"Miss prepares actual target396(1)");
        if(fail_prepare)throw std::runtime_error("intentional Miss prepare failure");prepare(receiver,flag);++prepared;
    });
    NativeCombatMissActions actions(b,registry,image,children);
    NativeCombatTaskQueue queue(actions,system,[&](std::uint32_t){++identity_actions;throw std::runtime_error("Miss must not end actor action");});
    restore.token=router.Subscribe(router.Key("ae_attack_missed"),1u,[&](std::uint32_t payload){
        const auto* p=reinterpret_cast<const std::uint32_t*>(payload);
        check(p[0]==a+716u && p[1]==b,"actual source512 emits facet/target miss event");++notifications;
        reject([&]{(void)actions.Create(a,b);});reject([&]{source.NotifyAttackMissed(b);});
        if(fail_notification)throw std::runtime_error("intentional Miss observer failure");
    });
    auto empty=[&]{check(!queue.Count() && !actions.Count() && !actions.OwnedTexts() && !actions.OwnedAllocations() &&
        !children.Count() && !children.OwnedActionTexts() && !children.OwnedAllocations(),"Miss parent/child/name/container/queue cleanup balances");};
    auto drain=[&]{unsigned frames{};for(;frames<600u && queue.Count();++frames){advance(.016f);queue.Update(.016f);}
        check(frames<600u,"actual Miss queue must naturally drain");return frames;};
    check(!actions.Create(0u,b) && !actions.Create(a,0u),"original null-actor Miss factory gates");
    reject([&]{(void)actions.Create(a+4u,b);});reject([&]{(void)actions.Create(a,a);});empty();
    const auto normal=actions.Create(a,b);
    check(actions.Name(normal)=="MissTask" && actions.Identity(normal)==b && !actions.Processed(normal),"original normal Miss name, TARGET identity and false processed flag");
    reject([&]{actions.Close();});
    check(queue.Register(normal)==normal && notifications==1u && children.Count()==2u && !actions.Ready(normal),"real queue registers actual Miss recipe");
    queue.Update(0.f);
    const bool miss_record=has_miss_record();
    // Original SetCurrentAnimation retains its record when the requested name
    // is absent. The animation-wait child then completes on name mismatch.
    check(playback.CompareInsensitive(playback.GetAnimationName(b),miss_record?std::string("z06"):prior)==0,
          "Miss selects actual z06 when present, otherwise preserves the actual prior record");
    advance(0.f);
    check(!prepared,"Miss animation child precedes preparation");
    const auto frames=drain();check(prepared==1u && !identity_actions,"natural Miss prepares target and never sends actor480");empty();
    image.Mode()=1u;const auto quick=actions.Create(a,b);
    check(actions.Name(quick)=="quickMiss" && actions.Identity(quick)==b,"original QuickMiss constructor");
    (void)queue.Register(quick);check(actions.Ready(quick) && !children.Count() && notifications==1u,"original QuickMiss empty prepare has no miss notification/animation");
    queue.Update(0.f);empty();image.Mode()=0u;
    const auto replaced=actions.Create(a,b);(void)queue.Register(replaced);
    const auto replacement=actions.Create(a,b);(void)queue.Register(replacement);
    check(!actions.Enabled(replaced) && actions.Enabled(replacement) && !identity_actions,"same-target Miss replacement uses unprocessed semantics");
    queue.Update(0.f);advance(0.f);check(actions.Count()==1u && children.Count()==2u,"disabled parent cancellation preserves replacement children");
    (void)drain();check(prepared==2u && notifications==3u,"replacement runs only current target preparation");empty();
    fail_notification=true;const auto failed=actions.Create(a,b);bool caught{};
    try{(void)queue.Register(failed);}catch(const std::runtime_error&){caught=true;}
    fail_notification=false;
    check(caught && !queue.Count() && actions.Count()==1u && !children.Count(),"observer failure leaves cancellable failed parent, not queued success");
    reject([&]{actions.Prepare(failed);});reject([&]{actions.BeginWaiting(failed);});actions.Cancel(failed);empty();
    fail_prepare=true;const auto retry=actions.Create(a,b);(void)queue.Register(retry);caught=false;
    for(unsigned i=0u;i<600u && !caught;++i){advance(.016f);try{queue.Update(.016f);}catch(const std::runtime_error&){caught=true;}}
    check(caught && queue.Count()==1u && children.Count()==2u,"real Miss child preparation failure stays owned by queue");
    fail_prepare=false;(void)drain();check(prepared==3u,"Miss queue recovers after child failure");empty();
    image.Mode()=1u;const auto result=actions.Create(a,b);
    *reinterpret_cast<std::uint32_t*>(result+36u)=0x12345678u;
    check(actions.ResultEvent(result)==0x12345678u,"original547A90 reads actual result36, not fabricated zero");
    *reinterpret_cast<std::uint32_t*>(result+36u)=0u;actions.Cancel(result);empty();
    check(source.CurrentStat(7u)==source_hp && target.CurrentStat(7u)==target_hp && !identity_actions,"Miss changes no HP or action-ending callbacks");
    actions.Close();reject([&]{(void)actions.Create(a,b);});
    std::cout<<"Native combat Miss parent: "<<label<<" actual512 -> target z06 request ->396, record_present="<<miss_record<<", frames="<<frames
        <<", real queue normal/quick/replacement, notification/child failure recovery, identity/result and zero ownership pass\n";
}
}
