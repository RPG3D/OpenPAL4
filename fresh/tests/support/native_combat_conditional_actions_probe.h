#pragma once
#include "pal4/native_combat_actor_skill_runtime.h"
#include "pal4/native_skill_record_catalog.h"
#include "pal4/native_skill_system_services.h"
#include "pal4/random_number_generator.h"
#include <iostream>
#include <stdexcept>

namespace pal4::test {
inline void VerifyNativeCombatConditionalActions(NativeCombatActorSkillRuntime& player,
    NativeCombatActorSkillRuntime& monster,NativeSkillRecordCatalog& catalog,
    INativeSkillSystemServices& system) {
    const auto check=[](bool ok,const char* text){if(!ok)throw std::runtime_error(text);};
    const auto word=[](std::uint32_t a)->std::uint32_t&{return *reinterpret_cast<std::uint32_t*>(a);};
    const auto reset=catalog.Address(7750u),select=catalog.Address(7752u);
    check(word(reset+16u)==9126u && !word(reset+56u) && word(select+16u)==9125u && !word(select+56u),
          "original terminal M13 conditional Skill records exist");
    const auto pa=player.ActorAddress(),ma=monster.ActorAddress();
    const auto pi=word(pa+1196u),mi=word(ma+960u),pc=word(pa+832u),mc=word(ma+832u),replacement=word(ma+1164u);
    const auto php=player.CurrentStat(7u),mhp=monster.CurrentStat(7u);
    struct Restore {
        NativeCombatActorSkillRuntime& p;NativeCombatActorSkillRuntime& m;
        unsigned pa,ma,pi,mi,pc,mc,replacement;int php,mhp;
        ~Restore(){
            p.BindAllTargets({});m.BindAllTargets({});p.BindConditionalRetirement({});m.BindConditionalRetirement({});
            *reinterpret_cast<unsigned*>(pa+1196u)=pi;*reinterpret_cast<unsigned*>(ma+960u)=mi;
            *reinterpret_cast<unsigned*>(pa+832u)=pc;*reinterpret_cast<unsigned*>(ma+832u)=mc;
            *reinterpret_cast<unsigned*>(ma+1164u)=replacement;
            (void)p.SetCurrentStatViaActor(7u,php);(void)m.SetCurrentStatViaActor(7u,mhp);
        }
    } restore{player,monster,pa,ma,pi,mi,pc,mc,replacement,php,mhp};
    word(pa+1196u)=word(ma+960u)=2600u;
    unsigned enumerations{},retired{},enumerated_actor{};bool fail{},rebind_rejected{};
    player.BindAllTargets([&](unsigned a){check(a==pa,"9126 Player roster service receives its owner");++enumerations;enumerated_actor=a;return std::vector<unsigned>{ma,pa};});
    monster.BindAllTargets([&](unsigned a){check(a==ma,"9126 Monster roster service receives its owner");++enumerations;enumerated_actor=a;return std::vector<unsigned>{ma,pa};});
    const auto retire=[&](NativeCombatActorSkillRuntime& actor){
        check(actor.CurrentStat(7u)==0 && actor.ConditionalRetirementActive() && !actor.ExitDispatchActive(),
              "9126 resets actual HP before candidate388 without death/replacement permission");
        ++retired;
        try{actor.BindConditionalRetirement({});}catch(const std::logic_error&){rebind_rejected=true;}
        if(fail)throw std::runtime_error("intentional9126 retirement failure");
    };
    player.BindConditionalRetirement([&]{retire(player);});monster.BindConditionalRetirement([&]{retire(monster);});
    auto reference=system.Random();
    for(auto* source:{&player,&monster}){
        word(source->ActorAddress()+832u)=1u;
        const auto prior=enumerations;
        check(source->TriggerForTarget(7750u,ma) && enumerations==prior,"9126 rejected counter gate does not enumerate");
        word(source->ActorAddress()+832u)=2u;
        (void)player.SetCurrentStatViaActor(7u,1);(void)monster.SetCurrentStatViaActor(7u,1);
        const auto before=retired;
        check(source->TriggerForTarget(7750u,ma) && retired==before+2u && rebind_rejected && enumerated_actor==source->ActorAddress(),
              "9126 mixed candidate IDs and actual Player/Monster HP dispatch");
    }
    fail=true;bool caught{};word(ma+832u)=2u;
    try{(void)monster.TriggerForTarget(7750u,ma);}catch(const std::runtime_error&){caught=true;}
    check(caught && !monster.ConditionalRetirementActive() && !player.ConditionalRetirementActive(),
          "9126 exception releases candidate permit and temporary vector");
    fail=false;check(monster.TriggerForTarget(7750u,ma),"9126 retry succeeds after exception");
    //588EC0/58A810: collection owner is callback arg2, stored context arg3,
    // and the notification target arg4. These must remain distinct.
    word(pa+832u)=2u;word(ma+832u)=1u;const auto before_context=retired;
    check(player.AddRecord(7750u,ma),"9126 stores a different registered context");
    player.Process(6u,pa);
    check(retired==before_context+2u && enumerated_actor==ma,
          "9126 gates on collection owner, enumerates stored context and ignores fourth callback argument");
    player.Remove(7750u);
    for(auto* source:{&player,&monster}){
        word(source->ActorAddress()+832u)=2u;
        check(source->TriggerForTarget(7752u,ma),"9125 rejects counter gate");
        word(source->ActorAddress()+832u)=3u;word(ma+1164u)=0u;
        check(source->TriggerForTarget(7752u,ma) && word(ma+1164u)==2601u,
              "9125 writes original Monster376 replacement ID");
        (void)reference.Next();
        check(source->TriggerForTarget(7752u,pa),"9125 original Player376 no-op dispatch");
        (void)reference.Next();
    }
    word(pa+832u)=3u;word(ma+832u)=2u;
    check(player.AddRecord(7752u,ma),"9125 stores a different registered context");
    player.Process(6u,pa);(void)reference.Next();
    player.Remove(7752u);
    auto actual=system.Random();
    for(unsigned i=0;i<624u;++i)check(actual.Next()==reference.Next(),"9126 consumes no MT; admitted9125 consumes one draw");
    std::cout<<"Native Combat9125/9126: original7750/7752 mixed roster, counter gates, HP/388 ordering, replacement ID, exception/permission and MT pass\n";
}
}
