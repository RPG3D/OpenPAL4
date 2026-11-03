#pragma once
#include "pal4/native_combat_actor_skill_runtime.h"
#include "pal4/native_skill_record_catalog.h"
#include "pal4/native_skill_system_services.h"
#include "pal4/native_script_event_router.h"
#include "pal4/random_number_generator.h"
#include <array>
#include <bit>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace pal4::test {
inline void VerifyNativeCombatRandomResponse(NativeCombatActorSkillRuntime& source,
    NativeCombatActorSkillRuntime& target,NativeSkillRecordCatalog& catalog,
    NativeScriptEventRouter& router,INativeSkillSystemServices& system) {
    const auto check=[](bool ok,const char* why){if(!ok)throw std::runtime_error(why);};
    std::uint32_t id{};
    for(std::size_t i=0;i<catalog.Size();++i){
        const auto& row=catalog.At(i);
        if(row[4]==9127u && !row[14] && std::bit_cast<float>(row[5])>=1.f){id=row[0];break;}
    }
    check(id!=0u,"original terminal9127 Skill with admitted outer gate exists");
    const auto actor=target.ActorAddress();
    auto& condition=*reinterpret_cast<std::uint32_t*>(actor+540u);
    const auto saved_condition=condition;const auto hp=target.CurrentStat(7u);
    std::vector<std::array<std::uint32_t,4>> events;
    const auto subscription=router.Subscribe(router.Key("ae_immunity_skill"),1u,[&](std::uint32_t p){
        const auto* v=reinterpret_cast<const std::uint32_t*>(p);events.push_back({v[0],v[1],v[2],v[3]});
    });
    struct Restore {
        NativeCombatActorSkillRuntime& target;NativeScriptEventRouter& router;
        std::uint32_t subscription,&condition,saved;int hp;
        ~Restore(){(void)router.Unsubscribe(subscription);condition=saved;(void)target.SetCurrentStatViaActor(7u,hp);}
    } restore{target,router,subscription,condition,saved_condition,hp};
    auto reference=system.Random();
    condition=std::bit_cast<std::uint32_t>(1.f);
    check(source.TriggerForTarget(id,actor),"9127 actual mixed actor Skill dispatch");
    (void)reference.Next();(void)reference.Next();
    check(events==std::vector<std::array<std::uint32_t,4>>{{actor+716u,catalog.Address(id),source.ActorAddress(),1u}},
        "9127 true condition preserves target facet, Skill, callback context and mode");
    check(target.CurrentStat(7u)==hp,"9127 true condition preserves HP");
    condition=std::bit_cast<std::uint32_t>(0.f);events.clear();
    check(source.TriggerForTarget(id,actor),"9127 false condition enters actual dispatch");
    (void)reference.Next();(void)reference.Next();
    check(events.empty() && target.CurrentStat(7u)==0,"9127 false condition applies actor84 negative current HP");
    auto actual=system.Random();
    for(unsigned i=0;i<624;++i)check(actual.Next()==reference.Next(),"9127 consumes exactly two draws per admitted outer gate");
    std::cout<<"Native Combat9127: original Skill="<<id<<" immunity payload, actual HP reset, two MT draws and restoration pass\n";
}
}
