#pragma once
#include "pal4/native_combat_actor_skill_runtime.h"
#include "pal4/native_script_event_router.h"
#include <array>
#include <bit>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
namespace pal4::test {
inline void VerifyNativeCombatAttackNotifications(NativeCombatActorSkillRuntime& source,
    NativeCombatActorSkillRuntime& other,NativeScriptEventRouter& router,const char* label){
    const auto check=[](bool ok,const char* why){if(!ok)throw std::runtime_error(why);};
    const auto reject=[&](auto&& f){bool denied{};try{f();}catch(const std::logic_error&){denied=true;}check(denied,"attack interaction must reject");};
    const auto a=source.ActorAddress(),b=other.ActorAddress();
    const bool player=std::string_view(label)=="Player";
    const auto word=[](std::uint32_t address)->std::uint32_t&{return *reinterpret_cast<std::uint32_t*>(address);};
    struct Restore{NativeScriptEventRouter& router;std::vector<std::pair<std::uint32_t,std::uint32_t>> fields;
        std::vector<std::uint32_t> tokens;
        ~Restore(){for(auto t:tokens)(void)router.Unsubscribe(t);for(auto [p,v]:fields)*reinterpret_cast<std::uint32_t*>(p)=v;}
    } restore{router};
    auto save=[&](std::uint32_t address){restore.fields.emplace_back(address,word(address));};
    for(auto address:{a+348u,a+576u,a+636u,a+800u,b+576u,b+800u})save(address);
    if(!player)save(a+924u);
    const auto family=player?"re_state_changed":"me_state_changed";
    std::vector<std::string> trace;
    std::function<void(std::uint32_t)> hook;
    unsigned physical{},critical{};
    for(auto name:{"ae_physical_attack","ae_critical_attack"}){
        restore.tokens.push_back(router.Subscribe(router.Key(name),1u,[&,name](std::uint32_t payload){
            if(word(payload)!=a+716u)return;
            check(word(payload+4u)==b,"actual attack notification target");
            trace.emplace_back(name);if(std::string_view(name)=="ae_physical_attack")++physical;else ++critical;
            reject([&]{source.Close();});reject([&]{other.Close();});
            if(hook)hook(payload);
        }));
    }
    restore.tokens.push_back(router.Subscribe(router.Key(family),1u,[&](std::uint32_t p){
        if(word(p)!=a+716u)return;
        check(word(p+4u)==1u && word(p+8u)==4u,"reset actual primary state event");trace.emplace_back(family);
    }));
    source.NotifyPhysicalAttack(0u);source.NotifyCriticalAttack(0u);check(trace.empty(),"null attack notification has no event");
    hook=[&](std::uint32_t){reject([&]{source.NotifyPhysicalAttack(b);});};
    source.NotifyPhysicalAttack(b);hook={};source.NotifyCriticalAttack(b);
    check(physical==1u && critical==1u && trace==std::vector<std::string>{"ae_physical_attack","ae_critical_attack"},
          "actual shared500/504 event names, payloads and reentry guard");
    const auto subscribed=router.SubscriptionCount();
    bool nested{};unsigned nested_count{};
    const auto nested_token=router.Subscribe(router.Key("ae_critical_attack"),1u,[&](auto p){
        if(word(p)==b+716u){check(word(p+4u)==a,"nested other actor attack target");++nested_count;}
    });
    restore.tokens.push_back(nested_token);
    hook=[&](std::uint32_t p){
        const auto snapshot=std::array{word(p),word(p+4u)};
        if(!nested){nested=true;other.NotifyCriticalAttack(a);nested=false;}
        check(snapshot==std::array{word(p),word(p+4u)},"nested attack retains outer stack record");
    };
    source.NotifyPhysicalAttack(b);hook={};check(nested_count==1u && router.SubscriptionCount()==subscribed+1u,"nested actual actors keep subscription ownership");
    hook=[](auto){throw std::runtime_error("intentional attack notification failure");};
    bool failed{};try{source.NotifyPhysicalAttack(b);}catch(const std::runtime_error&){failed=true;}
    hook={};check(failed,"attack observer exception propagates");source.NotifyCriticalAttack(b);
    // Confirm the property forward still executes after the event: invalidate
    // its borrowed receiver in the observer and require an explicit failure.
    const auto original_property=word(a+800u);
    hook=[&](auto){word(a+800u)=0u;};
    bool forward_failed{};try{source.NotifyPhysicalAttack(b);}catch(const std::logic_error&){forward_failed=true;}
    word(a+800u)=original_property;hook={};
    check(forward_failed,"property forwarding occurs after actual attack event");source.NotifyCriticalAttack(b);
    trace.clear();word(a+636u)=4u;source.NotifyReceivedAttack(a);
    check(word(a+636u)==4u && trace.empty(),"same owner/type skips reset before property36");
    source.NotifyReceivedAttack(b);
    check(word(a+636u)==0u && trace==std::vector<std::string>{family},"different owner/type resets actual state4 before property36");
    trace.clear();source.NotifyReceivedAttack(b);check(trace.empty(),"already-reset received attack emits no state change");
    word(a+348u)=7u;word(a+576u)=10u;word(b+576u)=20u;if(!player)word(a+924u)=5u;
    source.TransferAttackMetric8(b,1.5f);
    check(source.CurrentStat(8u)==20 && other.CurrentStat(8u)==(player?20:27),"cross-type metric uses original432/428, truncation and additive actor84");
    source.TransferAttackMetric8(a,10.f);check(source.CurrentStat(8u)==20,"same-owner metric changes neither receiver");
    source.TransferAttackMetric8(b,-.5f);
    check(source.CurrentStat(8u)==17 && other.CurrentStat(8u)==(player?20:25),"negative scaled metric truncates toward zero and adds");
    source.TransferAttackMetric8(b,100.f);check(source.CurrentStat(8u)==100 && other.CurrentStat(8u)==(player?20:100),"SP additions use actual upper bound");
    const auto events=physical+critical;
    reject([&]{source.NotifyPhysicalAttack(b+4u);});reject([&]{source.NotifyReceivedAttack(0u);});
    reject([&]{source.TransferAttackMetric8(0u,1.f);});
    check(events==physical+critical,"foreign/null participants fail before event dispatch");
    const auto property=word(a+800u);word(a+800u)=0u;reject([&]{source.NotifyPhysicalAttack(b);});word(a+800u)=property;
    check(events==physical+critical,"missing actual property fails before notifications");
    std::cout<<"Native combat attack notifications: "<<label<<" actual500/504/484, nested records, property reset, SP8 additive scaling, null/foreign/reentry/exception and ownership gates pass\n";
}
}
