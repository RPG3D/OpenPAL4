#pragma once
#include "pal4/native_combat_damage_calculator.h"
#include "pal4/native_combat_actor_skill_runtime.h"
#include "pal4/native_script_event_router.h"
#include "pal4/native_skill_system_services.h"
#include "pal4/native_skill_numeric.h"
#include "pal4/random_number_generator.h"
#include <array>
#include <bit>
#include <functional>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>
namespace pal4::test {
// Actual calculation, actor84 property dispatch and notifications; temporary
// maxima/current/state fields are explicit fixtures restored without events.
inline void VerifyNativeCombatCurrentDelta(NativeCombatDamageCalculator& calculator,
    NativeCombatActorSkillRuntime& target,std::uint32_t source,INativeSkillSystemServices& system,
    NativeScriptEventRouter& router,const char* label){
    const auto check=[](bool ok,const char* why){if(!ok)throw std::runtime_error(why);};
    const auto actor=target.ActorAddress();
    const auto word=[](std::uint32_t p)->std::uint32_t&{return *reinterpret_cast<std::uint32_t*>(p);};
    const std::string family=label==std::string_view("Player")?"re":"me";
    const auto attr=family+"_attr_int_ext_changed",state=family+"_state_changed";
    struct Restore {
        NativeScriptEventRouter& router;RandomNumberGenerator& rng;RandomNumberGenerator saved;
        std::vector<std::pair<std::uint32_t,std::uint32_t>> fields;std::vector<std::uint32_t> tokens;
        ~Restore(){for(auto token:tokens)(void)router.Unsubscribe(token);rng=saved;
            for(auto [p,v]:fields)*reinterpret_cast<std::uint32_t*>(p)=v;}
    } restore{router,system.Random(),system.Random()};
    auto set=[&](std::uint32_t offset,std::uint32_t value){const auto p=actor+offset;
        bool found{};for(auto [a,v]:restore.fields)if(a==p)found=true;
        if(!found)restore.fields.emplace_back(p,word(p));word(p)=value;};
    for(auto offset:{224u,344u,352u,800u,828u})set(offset,word(actor+offset));
    for(unsigned offset=544u;offset<584u;offset+=4u)set(offset,word(actor+offset));
    for(unsigned offset=632u;offset<668u;offset+=4u)set(offset,word(actor+offset));
    const auto computed=calculator.CalculateExtended(source,actor);
    const auto delta=std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(TruncateSkillFloat(computed.main)));
    check(delta<0 && delta>-50000,"actual source produces bounded nonzero damage for receiving actor");
    set(344u,100000u);set(572u,50000u);set(648u,0u);set(652u,0u);set(828u,1u);
    std::vector<std::string> trace;
    std::vector<std::array<std::uint32_t,4>> changes;
    std::function<void(std::uint32_t)> hook;
    unsigned wrong{};
    for(const auto& name:{attr,state,std::string("ae_state_changed"),std::string("ae_become_alive")})
        restore.tokens.push_back(router.Subscribe(router.Key(name),1u,[&,name](std::uint32_t payload){
            if(word(payload)!=actor+716u)return;
            trace.push_back(name);
            if(name==attr) {
                changes.push_back({word(payload),word(payload+4u),word(payload+8u),word(payload+12u)});
                if(hook)hook(payload);
            }
        }));
    const auto other=(family=="re"?"me":"re");
    restore.tokens.push_back(router.Subscribe(router.Key(std::string(other)+"_attr_int_ext_changed"),1u,[&](auto p){if(word(p)==actor+716u)++wrong;}));
    check(target.AddCurrentStat(7u,delta)==50000+delta && target.CurrentStat(7u)==50000+delta,
          "actual calculated signed delta changes actual HP through actor84");
    check(trace==std::vector<std::string>{attr} && changes.back()[2]==50000u &&
          changes.back()[3]==static_cast<std::uint32_t>(50000+delta),"HP event actual previous/requested values");
    set(344u,100u);set(572u,100u);set(648u,0u);trace.clear();changes.clear();
    check(target.AddCurrentStat(7u,-75)==25 && word(actor+648u)==1u,"HP delta selects original wounded threshold");
    check(trace==std::vector<std::string>{state,attr},"state notification precedes HP notification");
    trace.clear();check(target.AddCurrentStat(7u,-10)==15 && word(actor+648u)==2u,"low HP threshold");
    trace.clear();check(target.AddCurrentStat(7u,0)==15 && trace.empty(),"unchanged HP delta emits no change");
    check(target.AddCurrentStat(7u,-99)==0 && word(actor+648u)==3u &&
          changes.back()[3]==std::bit_cast<std::uint32_t>(-84),"zero clamp retains requested negative payload");
    trace.clear();check(target.AddCurrentStat(7u,200)==100 && word(actor+648u)==0u,"healing clamps and revives health state");
    check(trace==std::vector<std::string>{state,"ae_state_changed","ae_become_alive",attr} && changes.back()[3]==200u,
          "original revival order and unclamped requested event value");
    set(352u,20u);set(580u,5u);trace.clear();
    check(target.AddCurrentStat(9u,-100)==0 && target.AddCurrentStat(9u,100)==20 &&
          trace==std::vector<std::string>{attr,attr},"MP lower/upper clamps use same property path");
    set(576u,0x7fffffffu);trace.clear();
    check(target.AddCurrentStat(8u,1)==0 && changes.back()[3]==0x80000000u,"32-bit addition wraps before SP validation");
    set(560u,10u);check(target.AddCurrentStat(4u,-3)==7 && word(actor+224u)==7u,"index4 mirror and Player-specific forward");
    trace.clear();check(target.AddCurrentStat(10u,123)==10 && target.AddCurrentStat(0xffffffffu,1)==-1 && trace.empty(),
                       "out-of-range indices preserve original return without writes/events");
    bool nested{};
    hook=[&](std::uint32_t payload){
        if(nested)return;
        const auto snapshot=std::array{word(payload),word(payload+4u),word(payload+8u),word(payload+12u)};
        nested=true;(void)target.AddCurrentStat(9u,-1);nested=false;
        check(snapshot==std::array{word(payload),word(payload+4u),word(payload+8u),word(payload+12u)},"nested update retains outer payload storage");
    };
    check(target.AddCurrentStat(7u,-1)==99 && target.CurrentStat(9u)==19,"nested actual MP delta during HP notification");
    hook=[](auto){throw std::runtime_error("intentional delta observer failure");};
    bool threw{};try{(void)target.AddCurrentStat(7u,-1);}catch(const std::runtime_error&){threw=true;}
    check(threw && target.CurrentStat(7u)==98,"observer exception preserves committed HP");hook={};
    check(target.AddCurrentStat(7u,1)==99 && !wrong,"exception restores mutation guards and event family");
    const auto property=word(actor+800u);word(actor+800u)=0u;
    bool refused{};try{(void)target.AddCurrentStat(7u,-1);}catch(const std::logic_error&){refused=true;}
    word(actor+800u)=property;check(refused && target.CurrentStat(7u)==99,"missing property fails before HP mutation");
    std::cout<<"Native combat current delta: "<<label<<" actual computed_damage="<<computed.main<<" HP=50000->"<<50000+delta
        <<", state/payload order, clamp/revival, wrap/index4, nesting/exception and property gates pass\n";
}
}
