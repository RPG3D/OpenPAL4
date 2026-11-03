#pragma once
#include "pal4/native_combat_command_actions.h"
#include "pal4/native_combat_player_action_memory.h"
#include "pal4/native_combat_task_queue.h"
#include "pal4/native_combat_attack_actions.h"
#include "pal4/native_combat_physical_attack_domain.h"
#include "pal4/native_combat_event_handlers.h"
#include "pal4/native_combat_actor_skill_runtime.h"
#include "pal4/native_combat_damage_calculator.h"
#include "pal4/native_combat_system_image.h"
#include "pal4/native_script_event_router.h"
#include "pal4/native_skill_system_services.h"
#include "pal4/random_number_generator.h"
#include <array>
#include <bit>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <vector>
#include <cmath>
namespace pal4::test {
// Real command -> CheckProtect -> approach -> physical/AMF/damage in the
// production owners. Only probability/HP operands are reversible test fixtures;
// there is no fabricated predecessor, completion flag or victory result.
inline void VerifyNativeCombatPhysicalIntegration(NativeCombatCommandActions& command,
    NativeCombatPlayerActionMemory& memory,NativeCombatTaskQueue& queue,NativeCombatAttackActions& approach,
    const std::vector<std::unique_ptr<NativeCombatPhysicalAttackDomain>>& domains,
    NativeCombatEventHandlers& handlers,NativeCombatActorSkillRuntime& source,NativeCombatActorSkillRuntime& target,
    NativeCombatSystemImage& image,NativeScriptEventRouter& router,NativeCombatDamageCalculator& damage,
    INativeSkillSystemServices& skills,const std::function<void(float)>& advance,
    const NativeCombatActorSkillRuntime::ActionMotionRefresh& source_motion,
    const NativeCombatActorSkillRuntime::ActionMotionRefresh& target_motion,bool force_counter=false,
    const std::function<void()>& start_action={}){
    const auto check=[](bool ok,const char* why){if(!ok)throw std::runtime_error(why);};
    const auto word=[](std::uint32_t a)->std::uint32_t&{return *reinterpret_cast<std::uint32_t*>(a);};
    const auto a=source.ActorAddress(),b=target.ActorAddress();
    check(!queue.Count() && !memory.Count() && !approach.Count(),"physical integration requires idle production owners");
    struct Restore {
        NativeCombatTaskQueue& queue;NativeCombatPlayerActionMemory& memory;NativeCombatSystemImage& image;
        NativeScriptEventRouter& router;std::uint32_t state,token{};std::vector<std::pair<std::uint32_t,std::uint32_t>> fields;
        RandomNumberGenerator& rng;RandomNumberGenerator saved;
        std::uint32_t end_token{},hint_token{},react_token{};
        ~Restore(){
            if(token)(void)router.Unsubscribe(token);
            if(end_token)(void)router.Unsubscribe(end_token);
            if(hint_token)(void)router.Unsubscribe(hint_token);
            if(react_token)(void)router.Unsubscribe(react_token);
            try{queue.ResetForStart();memory.Clear();}catch(...){}
            for(auto [address,value]:fields)*reinterpret_cast<std::uint32_t*>(address)=value;
            image.State()=state;
            rng=saved;
        }
    } restore{queue,memory,image,router,image.State(),0u,{},skills.Random(),skills.Random()};
    for(auto actor:{a,b}){
        for(auto offset:{340u,344u,352u,356u,360u,364u,368u,376u,380u,384u,416u,440u,588u,600u,604u,828u})restore.fields.emplace_back(actor+offset,word(actor+offset));
        for(unsigned offset=544u;offset<584u;offset+=4u)restore.fields.emplace_back(actor+offset,word(actor+offset));
        for(unsigned offset=632u;offset<668u;offset+=4u)restore.fields.emplace_back(actor+offset,word(actor+offset));
        word(actor+344u)=100000u;word(actor+572u)=50000u;word(actor+828u)=1u;
        word(actor+636u)=0u;word(actor+648u)=0u;word(actor+640u)=0u;word(actor+652u)=0u;
        word(actor+364u)=0u;word(actor+368u)=0u;word(actor+560u)=0u;
    }
    word(a+376u)=std::bit_cast<std::uint32_t>(-2.f);word(a+384u)=std::bit_cast<std::uint32_t>(2.f);
    word(b+380u)=std::bit_cast<std::uint32_t>(-2.f);
    // This original cave Player/Monster pair can calculate zero normal damage.
    // Reversible attack/defense/factor operands exercise HP/Wounded integration
    // through the real calculator, without changing gameplay damage rules.
    word(a+356u)=200u;word(b+360u)=20u;word(b+340u)=0u;
    word(a+588u)=std::bit_cast<std::uint32_t>(1.f);word(a+604u)=std::bit_cast<std::uint32_t>(1.f);
    word(b+440u)=0u;word(b+416u)=0u;
    if(force_counter)word(b+600u)=std::bit_cast<std::uint32_t>(2.f);
    const auto before_calculation=skills.Random();const auto expected=damage.Calculate(a,b);skills.Random()=before_calculation;
    std::cout<<"Native production physical operands: damage="<<expected.main<<" target_allows_damage="<<target.AllowsDamage()<<'\n';
    check(expected.main<-1.f && expected.main>-50000.f && target.AllowsDamage(),"controlled actual damage operands support a nonlethal HP change");
    (void)source.SetCurrentStatViaActor(3u,0);
    unsigned hits{};
    restore.token=router.Subscribe(router.Key("ae_physical_attack"),3u,[&](std::uint32_t p){
        if(word(p)==a+716u && word(p+4u)==b)++hits;
    });
    unsigned ends{},hint_clears{},counters{};
    restore.end_token=router.Subscribe(router.Key("ae_end_action"),3u,[&](std::uint32_t p){if(word(p)==a+716u)++ends;});
    restore.hint_token=router.Subscribe(router.Key("select_join_atk"),3u,[&](std::uint32_t p){
        if(!word(p) && !word(p+4u))++hint_clears;
    });
    restore.react_token=router.Subscribe(router.Key("ae_react_attack"),3u,[&](std::uint32_t p){
        if(word(p)==b+716u && word(p+4u)==a)++counters;
    });
    const auto position=[&]{return *reinterpret_cast<const std::array<float,3>*>(word(a+808u)+172u);};
    const auto home=position();
    const std::array request{a+716u,b,0u,4u};
    if(start_action)start_action();
    else (void)command.Handle(handlers.ExtendedAddress(),reinterpret_cast<std::uint32_t>(request.data()));
    check(queue.Count()==1u && memory.Count()==1u && approach.Count()==2u,
          "real Player command owns one queued CheckProtect and one independent replay template");
    bool physical_seen{},end_seen{},moved{};unsigned frames{};
    for(;frames<2400u && queue.Count();++frames){
        bool physical{};for(const auto& domain:domains)physical|=domain->Count()!=0u;
        physical_seen|=physical;
        end_seen|=physical_seen && !physical && approach.Count()>1u;
        moved|=position()!=home;
        advance(1.f/60.f);
    }
    std::cout<<"Native production physical snapshot: frames="<<frames<<" handoff="<<physical_seen
             <<" hits="<<hits<<" target_HP="<<target.CurrentStat(7u)<<" EndAttack="<<end_seen<<" end_events="<<ends<<" hint_clears="<<hint_clears<<" counter_fixture="<<force_counter<<" counters="<<counters<<'\n';
    check(!queue.Count() && frames<2400u && physical_seen && end_seen && hits && ends && hint_clears && moved && target.CurrentStat(7u)<50000,
          "production command/AMF/damage/Wounded/EndAttack naturally drains and ends actual source action");
    for(unsigned i=0;i<3u;++i)check(std::fabs(position()[i]-home[i])<0.002f,"EndAttack returns source to actual per-slot formation");
    check(word(a+828u)==1u,"actual EndAction resets source facet before fixture cleanup");
    if(force_counter)check(counters==1u,"actual EndAttack counter probability creates one reversed attack chain");
    check(!queue.PendingSuccessorCount() && approach.Count()==1u && memory.Count()==1u,
          "physical handoff transfers ownership while preserving only Player replay template");
    queue.ResetForStart();memory.Clear();
    source_motion(a,1u);
    target_motion(b,1u);
    for(const auto& domain:domains)check(!domain->Count() && !domain->OwnedAllocations() &&
        !domain->OwnedTexts() && !domain->Subscriptions(),"production physical domains release parents/messages/subscriptions");
    check(!queue.Count() && !memory.Count() && !approach.Count() && !approach.OwnedAllocations(),
          "production chain teardown releases queued and retained actions separately");
    std::cout<<"Native production physical integration: real Player command to Monster, shared queue/AMF/damage/Wounded, "
             <<"EndAttack/formation return/EndAction and full scoped retirement pass\n";
}
}
