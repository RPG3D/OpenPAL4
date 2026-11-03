#pragma once
#include "pal4/native_combat_skill_domain.h"
#include "pal4/native_combat_monster_manager.h"
#include "pal4/native_combat_actor_skill_runtime.h"
#include "pal4/native_combat_actor_model_owner.h"
#include "pal4/native_actor_instance_owner.h"
#include "pal4/native_effect_scene_session.h"
#include "pal4/effect_handle.h"
#include "pal4/combat_effect_config_keys.h"
#include "pal4/native_magic_window_records.h"
#include "pal4/native_combat_task_queue.h"
#include "pal4/native_script_event_router.h"
#include <iostream>
#include <cmath>
#include <stdexcept>
#include <vector>
namespace pal4::test {
// Explicit fixture operands, never applied to the live comparison pair.
inline void VerifyNativeCombatGroupRecovery(NativeCombatMonsterManager& monsters,
    const NativeMagicWindowRecords& records,NativeCombatTaskQueue& queue,
    NativeCombatActorSkillRuntime& player,NativeScriptEventRouter& router,NativeEffectSceneSession& effects,
    const std::function<NativeCombatSkillDomain&(std::uint32_t)>& resolve,
    const std::function<NativeCombatSkillDomain&(std::uint32_t)>& magic,
    const std::function<void()>& frame){
    const auto check=[](bool ok,const char* why){if(!ok)throw std::runtime_error(why);};
    check(!queue.Count() && monsters.Count(),"group recovery requires idle actual combat actors");
    auto& source=monsters.Skills(monsters.ActorAt(0u));
    check(monsters.ModelOwner(source.ActorAddress()).CurrentModel().ModelName()=="404",
        "group recovery fixture requires actual Monster2503/model404");
    const auto record=records.StuntAddress(15002u);
    check(record && source.ReadyForAction(),"group recovery requires original15002 and ready source");
    check(*reinterpret_cast<const std::uint32_t*>(record+136u)==3u,
        "original15002 uses the allied group target path");
    auto& domain=resolve(source.ActorAddress());
    check(domain.Kind()==NativeCombatSkillKind::stunt,"group recovery uses the Stunt domain");
    const auto hp=source.CurrentStat(7u),sp=source.CurrentStat(8u);
    struct Restore {NativeCombatActorSkillRuntime& source;std::int32_t hp,sp;
        ~Restore(){(void)source.SetCurrentStat(7u,hp);(void)source.SetCurrentStat(8u,sp);}} restore{source,hp,sp};
    EffectSystemHandleWords lingering_ice{};
    bool force_retired_callback_alias{};
    const auto run_recovery=[&]{
        (void)source.SetCurrentStat(7u,11);(void)source.SetCurrentStat(8u,90);
        const auto casts=domain.Casts(),targets=monsters.StageCount();
        const auto parent=domain.Create(record,source.ActorAddress());(void)queue.Register(parent);
        if(force_retired_callback_alias){
            const auto word=[](std::uint32_t a){return *reinterpret_cast<const std::uint32_t*>(a);};
            const auto sentinel=word(parent+8u),node=word(sentinel);
            check(node!=sentinel && lingering_ice.id,"callback alias fixture requires actual Ice tail and Stunt child");
            const auto child=word(node+8u);
            // Explicit isolated fixture: reproduce a retired task address being
            // reused, while keeping the real Ice tail and its natural end event.
            (void)EffectHandle::SetConfigInt(lingering_ice,CombatEffectConfigKeys::Cast,
                static_cast<std::int32_t>(child),effects.MutationHandles());
            std::cout<<"Recovery retired-callback alias fixture: Ice="<<lingering_ice.id<<" new_stunt="<<child<<std::endl;
        }
        unsigned frames{};while(queue.Count() && frames<1800u){frame();++frames;}
        std::cout<<"Native group recovery probe: record15002/model404 targets="<<targets
            <<" HP11->"<<source.CurrentStat(7u)<<" SP90->"<<source.CurrentStat(8u)
            <<" casts="<<domain.Casts()-casts
            <<" frames="<<frames<<" queue="<<queue.Count()<<std::endl;
        check(!queue.Count() && domain.Casts()==casts+targets && source.CurrentStat(7u)==41 && source.CurrentStat(8u)==80,
            "original15002 X01 AMF must cast7607 for each ally, recover11->41 and consume10 SP");
        check(!domain.Count() && !domain.OwnedAllocations() && !domain.OwnedTexts() && !domain.Subscriptions(),
            "group recovery retires actual animation/effect/sequence ownership");
    };
    // Keep the original first-list ownership; real leave/join notifications
    // isolate the single live ally case without editing a roster image.
    {
        struct StageRestore {
            NativeCombatMonsterManager& monsters;std::vector<std::uint32_t> departed;
            void Restore(){while(!departed.empty()){monsters.JoinStageAndScene(departed.front());departed.erase(departed.begin());}}
            ~StageRestore(){Restore();}
        } stage{monsters};
        std::vector<std::uint32_t> others;
        const auto count=monsters.StageCount();
        for(std::size_t i=0;i<count;++i){const auto a=monsters.StageActorAt(i);if(a!=source.ActorAddress())others.push_back(a);}
        for(auto a:others){stage.departed.push_back(a);monsters.LeaveStageAndScene(a);}
        check(monsters.StageCount()==1u && monsters.StageActorAt(0u)==source.ActorAddress(),
            "single-ally recovery uses the actual sole stage actor");
        run_recovery();stage.Restore();
        check(monsters.StageCount()==count,"single-ally fixture restores actual stage membership");
    }
    run_recovery();
    const auto mp=player.CurrentStat(9u);
    struct RestoreMp {NativeCombatActorSkillRuntime& player;std::int32_t mp;
        ~RestoreMp(){(void)player.SetCurrentStat(9u,mp);}} restore_mp{player,mp};
    (void)source.SetCurrentStat(7u,100);(void)player.SetCurrentStat(9u,71);
    const auto cast_mp=player.CurrentStat(9u);
    auto& cast=magic(player.ActorAddress());
    const auto ice=reinterpret_cast<std::uint32_t>(records.FindMagic(5000u));
    check(ice!=0u,"group recovery fixture requires actual Ice5000");
    const auto before=cast.Casts();
    const auto ice_route=router.Subscribe(router.Key("effect_message"),1u,[&](auto payload){
        const auto h=*reinterpret_cast<const EffectSystemHandleWords*>(payload);
        if(EffectHandle::GetConfigInt(h,CombatEffectConfigKeys::Cast,effects.MutationHandles()) &&
           static_cast<std::uint32_t>(EffectHandle::GetConfigInt(h,CombatEffectConfigKeys::Target,effects.MutationHandles()))==source.ActorAddress())
            lingering_ice=h;
    });
    struct IceRoute {NativeScriptEventRouter& router;std::uint32_t token;~IceRoute(){(void)router.Unsubscribe(token);}} ice_route_owner{router,ice_route};
    (void)queue.Register(cast.Create(ice,source.ActorAddress()));
    unsigned frames{};while(queue.Count() && frames<1800u){frame();++frames;}
    std::cout<<"Native group recovery Ice warmup: HP100->"<<source.CurrentStat(7u)<<" MP="<<cast_mp
        <<"->"<<player.CurrentStat(9u)<<" frames="<<frames<<" queue="<<queue.Count()<<std::endl;
    check(!queue.Count() && cast.Casts()==before+1u && source.CurrentStat(7u)<100 && player.CurrentStat(9u)==cast_mp-15,
        "group recovery warmup runs real Ice AMF, damage and reaction tasks");
    check(!cast.Count() && !cast.OwnedAllocations() && !cast.OwnedTexts() && !cast.Subscriptions(),
        "Ice warmup retires its actual sequence before recovery");
    check(lingering_ice.id!=0u,"Ice warmup observes its actual target effect");
    (void)router.Unsubscribe(ice_route);
    force_retired_callback_alias=true;
    run_recovery();
}
inline void VerifyNativeCombatStunt(NativeCombatMonsterManager& monsters,
    NativeCombatActorSkillRuntime& target,const NativeMagicWindowRecords& records,
    NativeCombatTaskQueue& queue,NativeScriptEventRouter& router,
    const std::function<NativeCombatSkillDomain&(std::uint32_t)>& resolve,
    const std::function<void()>& frame,const NativeCombatActorSkillRuntime::ActionMotionRefresh& motion){
    const auto check=[](bool ok,const char* why){if(!ok)throw std::runtime_error(why);};
    check(!queue.Count(),"Stunt probe requires idle actual queue");
    NativeCombatActorSkillRuntime* source{};std::uint32_t record{};
    for(std::size_t i=0;i<monsters.Count() && !source;++i){
        auto& candidate=monsters.Skills(monsters.ActorAt(i));
        for(unsigned offset=896u;offset<916u;offset+=4u){
            const auto id=*reinterpret_cast<const std::uint32_t*>(candidate.ActorAddress()+offset);
            const auto found=records.StuntAddress(id);
            std::cout<<"Stunt candidate: actor="<<candidate.ActorAddress()<<" id="<<id<<" record="<<found
                <<" ready="<<candidate.ReadyForAction()<<" SP="<<candidate.CurrentStat(8u)
                <<" cost="<<(found?*reinterpret_cast<const std::int32_t*>(found+116u):-1)<<std::endl;
            if(found && candidate.ReadyForAction() && candidate.CurrentStat(8u)>=
                *reinterpret_cast<const std::int32_t*>(found+116u)){source=&candidate;record=found;break;}
        }
    }
    check(source,"actual Monster skill table contains an affordable Stunt");
    auto& domain=resolve(source->ActorAddress());
    check(domain.Kind()==NativeCombatSkillKind::stunt,"Stunt resolves its own action domain");
    const auto hp=target.CurrentStat(7u),sp=source->CurrentStat(8u);
    const auto position=[&]{const auto model=*reinterpret_cast<const std::uint32_t*>(source->ActorAddress()+808u);
        check(model!=0u,"actual Stunt source retains its model");return *reinterpret_cast<const std::array<float,3>*>(model+172u);};
    const auto home=position();
    const auto cost=*reinterpret_cast<const std::int32_t*>(record+116u);
    const auto cancelled=domain.Create(record,target.ActorAddress());domain.Cancel(cancelled);
    check(!domain.Count() && !domain.OwnedAllocations() && !domain.OwnedTexts() && !domain.Subscriptions() &&
        source->CurrentStat(8u)==sp && target.CurrentStat(7u)==hp,"unpublished Stunt cancellation releases ownership without gameplay changes");
    {
        struct PreparationFailure:std::runtime_error{PreparationFailure():std::runtime_error("Stunt preparation callback fixture") {}};
        const auto route=router.Subscribe(router.Key("ae_start_action"),1u,[&](auto event){
            if(*reinterpret_cast<const std::uint32_t*>(event)==source->ActorAddress()+716u)throw PreparationFailure();
        });
        struct Route{NativeScriptEventRouter& r;std::uint32_t id;~Route(){(void)r.Unsubscribe(id);}} route_owner{router,route};
        const auto action=domain.Create(record,target.ActorAddress());bool failed{};
        try{(void)queue.Register(action);}catch(const PreparationFailure&){failed=true;}
        check(failed && !queue.Count() && domain.Owns(action),"failed Stunt preparation retains its unpublished parent for caller cancellation");
        domain.Cancel(action);
        check(!domain.Count() && !domain.OwnedAllocations() && !domain.OwnedTexts() && !domain.Subscriptions() &&
            source->CurrentStat(8u)==sp && target.CurrentStat(7u)==hp,"failed Stunt preparation cancellation retires all ownership before costs/damage");
    }
    source->EndAction(motion);
    unsigned ended{},stunts{};
    struct Routes{NativeScriptEventRouter& r;std::uint32_t a{},b{};~Routes(){if(a)(void)r.Unsubscribe(a);if(b)(void)r.Unsubscribe(b);}} routes{router};
    const auto own=[&](std::uint32_t p){return *reinterpret_cast<const std::uint32_t*>(p)==source->ActorAddress()+716u;};
    routes.a=router.Subscribe(router.Key("ae_end_action"),3u,[&](auto p){if(own(p))++ended;});
    routes.b=router.Subscribe(router.Key("ae_stunt"),3u,[&](auto p){if(own(p))++stunts;});
    const auto casts=domain.Casts();
    (void)queue.Register(domain.Create(record,target.ActorAddress()));
    unsigned frames{};while(queue.Count() && frames<1800u){frame();++frames;}
    check(!queue.Count() && ended && stunts && domain.Casts()>casts && source->CurrentStat(8u)==sp-cost,
        "actual Stunt animation, resource cost and EndStunt EndAction drain the queue");
    check(!domain.Count() && !domain.OwnedAllocations() && !domain.OwnedTexts() && !domain.Subscriptions(),
        "completed Stunt retires parents, children, text and subscriptions");
    const auto returned=position();for(unsigned i=0;i<3u;++i)check(std::fabs(returned[i]-home[i])<.002f,"Stunt returns to its actual starting formation position");
    std::cout<<"Native Stunt original-resource probe: source="<<source->ActorAddress()<<" record="<<record
        <<" frames="<<frames<<" SP="<<sp<<"->"<<source->CurrentStat(8u)<<" targetHP="<<hp<<"->"<<target.CurrentStat(7u)
        <<" stunt="<<stunts<<" EndAction="<<ended<<" allocations/text/subscriptions=0; cold cancellation, failed preparation rollback and formation return pass\n";
}
}
