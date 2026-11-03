#pragma once
#include "pal4/native_combat_actor_skill_runtime.h"
#include "pal4/native_skill_record_catalog.h"
#include "pal4/native_skill_system_owner.h"
#include "pal4/native_player_combat_events.h"
#include "pal4/native_script_event_router.h"
#include <bit>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace pal4::test {
template<class Owner>
void VerifyOriginalCombatHaste(Owner& owner,NativeCombatActorSkillRuntime& skills,
    INativeSkillSystemServices& system,NativeScriptEventRouter& router,
    const NativeSkillRecordCatalog& catalog) {
    const auto check=[](bool ok,const char* why){if(!ok)throw std::runtime_error(why);};
    const auto word=[](std::uint32_t a){return *reinterpret_cast<const std::uint32_t*>(a);};
    const auto actor=owner.Address();
    const auto speed=skills.StatParameter(5u),hp=skills.CurrentStat(7u);
    const auto physical=skills.StatParameter(3u);
    std::vector<std::uint32_t> previous;
    const auto subscription=router.Subscribe(router.Key(skills.IsPlayerActor()?"re_attr_int_changed":"me_attr_int_changed"),1u,
        [&](std::uint32_t p){if(word(p)==actor+716u && word(p+4u)==5u)previous.push_back(word(p+8u));});
    struct Unsubscribe {NativeScriptEventRouter& router;std::uint32_t id;~Unsubscribe(){(void)router.Unsubscribe(id);}} guard{router,subscription};
    const auto event=system.PropertyEvents().Lookup(9u);
    const auto set=[&](float value){skills.SetCombatEvent(event,std::bit_cast<std::uint32_t>(value));};
    set(static_cast<float>(speed)+7.75f);
    check(skills.StatParameter(5u)==speed+7 && previous.size()==1u && previous.front()==static_cast<std::uint32_t>(speed),
          "CA_SPEED real binding truncates float through actor64/property164 and emits original previous-value event");
    set(static_cast<float>(speed)+7.75f);
    check(previous.size()==1u,"equal CA_SPEED setter does not emit another change event");
    set(static_cast<float>(speed));
    check(skills.StatParameter(5u)==speed && previous.size()==2u &&
          skills.StatParameter(3u)==physical && skills.CurrentStat(7u)==hp,
          "CA_SPEED restoration preserves neighboring parameters and current HP");
    const auto record=catalog.Address(7539u);
    check(record!=0u,"original haste7539 resource exists");
    std::cout<<"Original7539 resource: duration="<<word(record+12u)<<" function="<<word(record+16u)<<" params=";
    for(unsigned offset=20u;offset<=52u;offset+=4u)std::cout<<std::bit_cast<float>(word(record+offset))<<',';
    std::cout<<'\n';
    check(record && word(record+12u)==5u && word(record+32u)==std::bit_cast<std::uint32_t>(9.f) &&
          word(record+36u)==std::bit_cast<std::uint32_t>(.2f) && word(record+40u)==std::bit_cast<std::uint32_t>(1.f),
          "original haste7539 has five-turn duration and20 percent CA_SPEED increment");
    const auto boosted=static_cast<std::int32_t>(static_cast<float>(speed)+static_cast<float>(speed)*.2f);
    const auto added=skills.AddRecord(7539u,actor);
    std::cout<<"Haste7539 diagnostic: baseline="<<speed<<" actual="<<skills.StatParameter(5u)
             <<" function="<<word(record+16u)<<" params="<<std::bit_cast<float>(word(record+20u))
             <<','<<std::bit_cast<float>(word(record+24u))<<','<<std::bit_cast<float>(word(record+28u))
             <<','<<std::bit_cast<float>(word(record+32u))<<','<<std::bit_cast<float>(word(record+36u))<<'\n';
    check(added && skills.StatParameter(5u)==boosted,
          "original7539 reads CA_SPEED and applies20 percent through real script attributes");
    (void)skills.RebuildStats();
    check(skills.StatParameter(5u)==boosted,"haste stat rebuild retains original boost without compounding");
    skills.Advance(4u);
    check(skills.StatParameter(5u)==boosted,"original7539 remains before fifth turn");
    skills.Advance(1u);
    check(skills.StatParameter(5u)==speed && skills.CurrentStat(7u)==hp,
          "original7539 expiry restores original speed through real stat rebuild");
    std::cout<<"Native Combat CA_SPEED: actor="<<actor<<" speed="<<speed<<", indexed setter/notification, original7539 +20percent/rebuild/five-turn expiry pass\n";
}
}
