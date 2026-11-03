#pragma once
#include "pal4/native_combat_actor_health_runtime.h"
#include "pal4/native_combat_actor_action_progress.h"
#include "pal4/native_combat_formula_catalog.h"
#include "pal4/native_combat_player_actor_owner.h"
#include "pal4/native_skill_system_services.h"
#include "pal4/native_script_event_router.h"
#include "pal4/script_skill_collection.h"
#include <bit>
#include <array>
#include <cmath>
#include <functional>
#include <iostream>
#include <string>
#include <vector>
namespace pal4::test {
// Real copied Player property/health/facet services. Skill chains are empty;
// nonempty chains deliberately fail through the protocol fixture below.
inline void VerifyNativeCombatPlayerHealth(NativePlayerManagerSession& source,
    INativeSkillSystemServices& system,const NativeCombatFormulaCatalog& formulas) {
    const auto check=[](bool v,const char* m){if(!v)throw std::runtime_error(m);};
    const auto reject=[&](auto&& f){bool failed=false;try{f();}catch(const std::logic_error&){failed=true;}check(failed,"Player health guard");};
    const auto word=[](std::uint32_t a)->std::uint32_t&{return *reinterpret_cast<std::uint32_t*>(a);};
    class EmptySkills final : public IScriptSkillCollectionRuntime {
        INativeSkillSystemServices& system;
    public:
        explicit EmptySkills(INativeSkillSystemServices& s):system(s){}
        [[noreturn]] static void Unbound(){throw std::logic_error("Player skill protocol fixture unbound");}
        std::uint32_t ReadWord(std::uint32_t a) override{return *reinterpret_cast<std::uint32_t*>(a);}
        void WriteWord(std::uint32_t a,std::uint32_t v) override{*reinterpret_cast<std::uint32_t*>(a)=v;}
        void WriteByte(std::uint32_t a,std::uint8_t v) override{*reinterpret_cast<std::uint8_t*>(a)=v;}
        std::uint32_t Allocate(std::uint32_t) override{Unbound();}
        void Free(std::uint32_t) override{Unbound();}
        void InitializeOuterBase(std::uint32_t) override{Unbound();}
        void InitializeInnerBase(std::uint32_t) override{Unbound();}
        std::uint32_t GameDatabase() override{Unbound();}
        std::uint32_t ResolveSkillRecord(std::uint32_t) override{Unbound();}
        void UpdateHealthStatus(std::uint32_t) override{Unbound();}
        std::uint32_t SkillSystemInstance() override{return system.Address();}
        void ProcessSkillChain(std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t) override{Unbound();}
        std::uint32_t InvokeVirtual(std::uint32_t,std::uint32_t,std::span<const std::uint32_t>) override{Unbound();}
    } skills(system);
    for(unsigned index=0;index<4u;++index) {
        NativeCombatPlayerActorOwner owner;
        const auto a=owner.Construct(source,index);
        {
            constexpr std::array<std::string_view,4> portraits{"101","103","106","105"};
            const auto original=word(a+1192u);
            check(owner.SequenceImage()==portraits[original<4u?original:0u],"real Player virtual452 mapping");
            for(unsigned role=0;role<5u;++role){word(a+1192u)=role;check(owner.SequenceImage()==portraits[role<4u?role:0u],"Player portrait map and original upper-bound fallback");}
            word(a+1192u)=original;
        }
        // Preserve copied role data, use deterministic HP only for boundaries.
        word(a+344u)=100u;word(a+572u)=100u;word(a+648u)=0u;word(a+652u)=0u;
        check(word(a+680u)==word(a+684u) && word(a+696u)==word(a+700u),"Player health fixture needs empty skill lists");
        NativeScriptEventRouter router;
        NativeCombatActorHealthRuntime health(owner,router,skills);
        NativeCombatActorActionProgress progress(owner,health,formulas,router,system);
        std::vector<std::string> trace;
        std::vector<std::uint32_t> tokens;
        std::function<void()> hook;
        for(const auto name:{"re_state_changed","re_attr_int_ext_changed","ae_state_changed","ae_become_alive","re_moved_at_action_sequence","re_attr_float_ext_changed","re_race_attack_param_changed"}) {
            tokens.push_back(router.Subscribe(router.Key(name),1u,[&,name](std::uint32_t p){
                check(word(p)==a+716u,"Player event actual facet");
                trace.emplace_back(name);
                if(std::string_view(name)=="re_state_changed")check(word(p+4u)==4u || word(p+4u)==7u,"Player state field index");
                reject([&]{owner.Destroy();});reject([&]{owner.SetSlot(3u);});
                if(hook)hook();
            }));
        }
        unsigned wrong_events{};
        for(const auto name:{"me_state_changed","me_attr_int_ext_changed","me_moved_at_action_sequence"})
            tokens.push_back(router.Subscribe(router.Key(name),1u,[&](std::uint32_t){++wrong_events;}));
        check(health.QueryIndexed(a,48u,0u)==100 && health.QueryIndexed(a,52u,7u)==100,"Player forwarded max/current HP");
        check(health.Update()==0 && trace.empty(),"Player unchanged health");
        check(health.SetCurrent(7u,20)==20 && word(a+648u)==2u,"Player low HP state");
        check(trace==std::vector<std::string>{"re_state_changed","re_attr_int_ext_changed"},"Player state before HP event");
        trace.clear();check(health.SetCurrent(7u,-1)==0 && word(a+648u)==3u,"Player HP clamps to zero");
        health.SetFacetState(1u,false);check(!health.IsReady(),"dead Player blocks scheduler readiness");
        trace.clear();check(health.SetCurrent(7u,120)==100 && word(a+648u)==0u,"Player maximum clamp/revival");
        check(trace==std::vector<std::string>{"re_state_changed","ae_state_changed","ae_become_alive","re_attr_int_ext_changed"},"Player revival event order");
        check(health.IsReady(),"live Player facet state1 ready");
        health.SetFacetState(0u,false);check(!health.IsReady(),"idle Player not scheduler ready");
        for(auto state:{1u,2u,5u}){health.SetFacetState(state,false);check(health.IsReady(),"Player ready state set");}
        trace.clear();check(health.Set(a,196u,2)==2 && word(a+660u)==2u,"Player action state distinct from facet");
        check(trace==std::vector<std::string>{"re_state_changed"},"Player action state notification");
        trace.clear();check(health.Set(a,196u,7)==2 && trace.size()==1u,"invalid action state keeps value but emits original event");
        owner.SetSlot(0u);
        trace.clear();progress.Prepare();check(progress.Progress()==0.0f && trace.empty(),"Player prepare clears silently");
        progress.SetProgress(0.25f);check(progress.Progress()==0.25f && trace==std::vector<std::string>{"re_moved_at_action_sequence"},"Player progress event family");
        owner.SetSlot(3u);trace.clear();progress.SetProgress(0.875f);
        check(progress.Progress()==0.25f && trace.empty(),"Player progress rejects slot3 boundary");
        owner.SetSlot(0u);
        check(std::isfinite(progress.Rate()),"Player actual formula rate finite");
        trace.clear();health.SetScalar(0u,std::bit_cast<std::uint32_t>(0.375f));
        health.SetRaceAttack(0u,std::bit_cast<std::uint32_t>(0.625f));
        check(word(a+584u)==std::bit_cast<std::uint32_t>(0.375f) && health.RaceAttack(0u)==0.625f,"Player float writes");
        check(trace==std::vector<std::string>{"re_attr_float_ext_changed","re_race_attack_param_changed"},"Player float event family");
        hook=[] {throw std::runtime_error("intentional Player callback failure");};
        bool failed=false;try{progress.SetProgress(0.5f);}catch(const std::runtime_error&){failed=true;}
        check(failed && progress.Progress()==0.5f,"Player callback failure retains committed progress");
        hook={};progress.SetProgress(0.75f);check(progress.Progress()==0.75f,"Player callback guard restored");
        const auto property=word(a+800u),vtable=word(property);
        word(property)=0u;reject([&]{(void)health.IsReady();});word(property)=vtable;
        const auto hp=word(a+572u);reject([&]{health.BecomeDead(0u);});check(word(a+572u)==hp,"unbound Player death rejects before mutation");
        check(!wrong_events,"Player never dispatches Monster event family");
        for(auto token:tokens)(void)router.Unsubscribe(token);
        owner.Destroy();check(!owner.OwnedAllocations(),"Player health ownership balanced");
        reject([&]{(void)health.IsReady();});
    }
    std::cout<<"Native Player health: actual4 copies, HP/state/revival/readiness, re events and action progress pass; empty skill protocol fixture\n";
}
}
