#pragma once
#include "pal4/native_combat_fend_off_actions.h"
#include "pal4/native_combat_death_actions.h"
#include "pal4/native_combat_task_queue.h"
#include "pal4/native_combat_system_image.h"
#include "pal4/native_combat_actor_skill_runtime.h"
#include "pal4/native_combat_reaction_children.h"
#include <iostream>

namespace pal4::test {
// Caller supplies actual target model/services and a reversible pre-Entry
// health648=0 fixture. Death execution is tested by the existing death probes;
// here successors are inspected and retired before their execution.
inline void VerifyNativeCombatFendOffActions(NativeCombatActorSkillRuntime& source,
    NativeCombatActorSkillRuntime& target,NativeCombatSkillActorRegistry& registry,
    NativeCombatSystemImage& image,INativeSkillSystemServices& system,
    NativeCombatReactionChildren& children,const std::function<void(float)>& advance,
    const NativeCombatFatalChildren::PrepareAction& prepare,const char* label){
    const auto check=[](bool ok,const char* why){if(!ok)throw std::runtime_error(why);};
    const auto reject=[&](auto&& f){bool denied{};try{f();}catch(const std::logic_error&){denied=true;}
        check(denied,"FendOff parent gate must reject");};
    const auto a=source.ActorAddress(),b=target.ActorAddress();
    auto& health=*reinterpret_cast<std::uint32_t*>(b+648u);
    struct Restore{std::uint8_t& mode;std::uint32_t& health;std::uint8_t m;std::uint32_t h;
        ~Restore(){mode=m;health=h;}} restore{image.Mode(),health,image.Mode(),health};
    health=0u;image.Mode()=0u;
    NativeCombatDeathActions deaths([&]{return image.Mode()!=0u;});
    bool fail_initial{};
    NativeCombatFendOffActions actions(b,registry,image,children,deaths,[&](std::uint32_t actor,std::uint32_t flag){
        if(fail_initial)throw std::runtime_error("intentional initial396 failure");prepare(actor,flag);
    });
    NativeCombatTaskQueue queue(actions,system);queue.BindActions(deaths);
    const auto empty=[&]{check(!queue.Count() && !actions.Count() && !actions.OwnedAllocations() &&
        !actions.OwnedTexts() && !children.Count() && !children.OwnedAllocations() && !children.OwnedTexts() &&
        !children.Subscriptions() && !deaths.Count() && !deaths.OwnedAllocations(),"FendOff queue/parent/children ownership balance");};
    const auto drain=[&]{unsigned frames{};for(;frames<600u && queue.Count();++frames){advance(.016f);queue.Update(.016f);}
        check(frames<600u,"FendOff real queue naturally drains");return frames;};
    check(!actions.Create(0u,b) && !actions.Create(a,0u),"original null FendOff factory gates");
    reject([&]{(void)actions.Create(a+4u,b);});reject([&]{(void)actions.Create(a,a);});
    const auto normal=actions.Create(a,b);
    check(actions.Identity(normal)==b && !actions.Processed(normal) && actions.Name(normal)=="FendOff",
          "original FendOff target identity, false processed and name");
    reject([&]{(void)actions.ResultEvent(normal);});reject([&]{actions.Close();});
    check(queue.Register(normal)==normal && children.Count()==3u,"real queue prepares original three-child FendOff");
    const auto frames=drain();empty();
    image.Mode()=1u;const auto quick=actions.Create(a,b);
    check(actions.Name(quick)=="QuickFendOff","original QuickFendOff name");
    (void)queue.Register(quick);check(actions.Ready(quick) && !children.Count(),"QuickFendOff has empty prepare");
    queue.Update(0.f);empty();
    image.Mode()=0u;const auto first=actions.Create(a,b),second=actions.Create(a,b);
    (void)queue.Register(first);(void)queue.Register(second);
    check(!actions.Enabled(first) && actions.Enabled(second),"same-target FendOff replacement disables prior task");
    (void)drain();empty();
    fail_initial=true;const auto failed=actions.Create(a,b);bool caught{};
    try{(void)queue.Register(failed);}catch(const std::runtime_error&){caught=true;}
    fail_initial=false;
    check(caught && !queue.Count() && actions.Count()==1u && children.Count()==1u,
          "failed immediate396 retains only linked impact in cancellable parent");
    reject([&]{actions.Prepare(failed);});reject([&]{actions.BeginWaiting(failed);});actions.Cancel(failed);empty();
    // The queued parent has finished before state-three is switched on. Query
    // through real virtual168/property268; facet828 alone is not that query.
    for(bool quick_kind:{false,true}){
        health=0u;image.Mode()=quick_kind?1u:0u;
        const auto parent=actions.Create(a,b);actions.Prepare(parent);actions.BeginWaiting(parent);
        for(unsigned n=0;n<600u && !actions.Ready(parent);++n){advance(.016f);actions.Update(parent,.016f);}
        check(actions.Ready(parent),"FendOff children complete before successor");
        health=3u;check(target.IsStateThree(),"real target168 reads health648 state-three fixture");
        image.Mode()=quick_kind?0u:1u;
        const auto next=actions.ResultEvent(parent);
        check(deaths.Owns(next) && deaths.Name(next)==(quick_kind?"QuickDead":"Dead") &&
            *reinterpret_cast<std::uint32_t*>(next+44u)==b && *reinterpret_cast<std::uint32_t*>(next+48u)==a,
            "actual FendOff parent result adopts forced death with exchanged actors");
        actions.Cancel(parent);deaths.Destroy(next);empty();
    }
    health=0u;actions.Close();actions.Close();reject([&]{(void)actions.Create(a,b);});
    std::cout<<"Native combat FendOff parent: "<<label<<" original factory/568E30, frames="<<frames
        <<", real queue normal/quick/replacement, partial failure cancellation, real168 forced death successors and zero ownership pass\n";
}
}
