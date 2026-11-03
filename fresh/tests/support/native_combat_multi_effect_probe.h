#pragma once
#include "pal4/native_combat_actor_skill_runtime.h"
#include "pal4/native_combat_actor_owner.h"
#include "pal4/native_skill_record_catalog.h"
#include "pal4/native_skill_system_services.h"
#include "pal4/native_script_event_router.h"
#include "pal4/random_number_generator.h"
#include <bit>
#include <cmath>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>

namespace pal4::test {
inline void VerifyNativeCombatMultiEffect(NativeCombatActorSkillRuntime& player,
    NativeCombatActorSkillRuntime& monster,NativeCombatSkillActorRegistry& registry,
    const NativeMonsterRecordCatalog& monsters,NativeSkillRecordCatalog& catalog,
    NativeScriptEventRouter& router,INativeSkillSystemServices& system) {
    const auto check=[](bool value,const char* message){if(!value)throw std::runtime_error(message);};
    std::uint32_t id{};unsigned limit{};std::vector<std::array<std::uint32_t,2>> expected;
    for(std::size_t i=0;i<catalog.Size();++i) {
        const auto& record=catalog.At(i);const auto cap=std::bit_cast<float>(record[5]);
        if(record[4]!=9115u || record[14] || !std::isfinite(cap) || cap<0.f || cap>4.f)continue;
        std::vector<std::array<std::uint32_t,2>> pairs;
        for(unsigned p=0;p<4;++p) {
            const auto first=static_cast<std::int32_t>(std::bit_cast<float>(record[6u+p*2u]));
            const auto second=static_cast<std::int32_t>(std::bit_cast<float>(record[7u+p*2u]));
            if(first && second)pairs.push_back({std::bit_cast<std::uint32_t>(first),std::bit_cast<std::uint32_t>(second)});
        }
        if(pairs.empty())continue;
        id=record[0];limit=static_cast<unsigned>(cap);expected=std::move(pairs);break;
    }
    check(id!=0u,"original terminal9115 Skill with summon operands exists");
    struct Owned {NativeCombatActorOwner actor;std::unique_ptr<NativeCombatActorSkillRuntime> skills;};
    std::vector<std::unique_ptr<Owned>> extra;
    std::vector<std::uint32_t> roster{monster.ActorAddress()};
    for(unsigned i=0;i<limit;++i) {
        auto entry=std::make_unique<Owned>();const auto a=entry->actor.Construct(monsters,2579);
        entry->skills=std::make_unique<NativeCombatActorSkillRuntime>(entry->actor,registry);
        roster.push_back(a);extra.push_back(std::move(entry));
    }
    auto& state=*reinterpret_cast<std::uint32_t*>(roster.back()+648u);const auto prior_state=state;state=0u;
    const auto prior_first=*reinterpret_cast<const std::uint32_t*>(roster.front()+648u);
    *reinterpret_cast<std::uint32_t*>(roster.front()+648u)=0u;
    std::vector<std::array<std::uint32_t,2>> observed;
    bool fail{},rebind_rejected{};
    const auto subscription=router.Subscribe(router.Key("summon"),1u,[&](std::uint32_t payload){
        const auto* pair=reinterpret_cast<const std::uint32_t*>(payload);observed.push_back({pair[0],pair[1]});
        try{monster.BindAllTargets({});}catch(const std::logic_error&){rebind_rejected=true;}
        if(fail)throw std::runtime_error("intentional9115 observer failure");
    });
    struct Unsubscribe {NativeScriptEventRouter& router;unsigned id;~Unsubscribe(){(void)router.Unsubscribe(id);}} guard{router,subscription};
    const auto before_rng=system.Random();
    monster.BindAllTargets([&](auto actor){check(actor==monster.ActorAddress(),"9115 enumerates target receiver");return roster;});
    check(monster.TriggerForTarget(id,monster.ActorAddress()) && observed.empty(),"9115 blocks when complete active count exceeds original threshold");
    state=3u;
    check(monster.TriggerForTarget(id,monster.ActorAddress()) && observed==expected && rebind_rejected,
          "9115 excludes exactly state3 and dispatches original nonzero pairs in order");
    observed.clear();
    check(player.TriggerForTarget(id,monster.ActorAddress()) && observed.empty(),"Player372 is the original no-op even for a Monster target roster");
    monster.BindAllTargets([](auto){return std::vector<std::uint32_t>{};});
    check(monster.TriggerForTarget(id,monster.ActorAddress()) && observed==expected,"empty complete roster still passes original9115 count gate");
    observed.clear();fail=true;bool caught{};
    try{(void)monster.TriggerForTarget(id,monster.ActorAddress());}catch(const std::runtime_error&){caught=true;}
    check(caught,"9115 synchronous dispatch exception is propagated");
    fail=false;observed.clear();
    check(monster.TriggerForTarget(id,monster.ActorAddress()) && observed==expected,"9115 releases its temporary vector and actor scope after exception");
    auto after_rng=system.Random(),reference=before_rng;
    for(unsigned i=0;i<624;++i)check(after_rng.Next()==reference.Next(),"9115 counting and dispatch consumes no MT values");
    monster.BindAllTargets({});state=prior_state;
    *reinterpret_cast<std::uint32_t*>(roster.front()+648u)=prior_first;
    for(auto& entry:extra){entry->skills->Close();entry->actor.Destroy();}
    std::cout<<"Native Combat9115: original Skill="<<id<<" threshold="<<limit<<" complete roster, state3, Player no-op, ordered summon pairs and exception/RNG guards pass\n";
}
}
