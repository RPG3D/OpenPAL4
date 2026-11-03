#pragma once
#include "pal4/native_combat_damage_calculator.h"
#include "pal4/native_combat_actor_skill_runtime.h"
#include "pal4/native_combat_formula_catalog.h"
#include "pal4/native_skill_system_services.h"
#include "pal4/random_number_generator.h"
#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <iostream>
#include <vector>
namespace pal4::test {
inline void VerifyNativeCombatDamage(NativeCombatDamageCalculator& calculator,
    NativeCombatActorSkillRuntime& attacker,NativeCombatActorSkillRuntime& defender,
    const NativeCombatFormulaCatalog& formulas,INativeSkillSystemServices& system,const char* label){
    const auto check=[](bool ok,const char* why){if(!ok)throw std::runtime_error(why);};
    const auto reject=[&](auto&& f){bool failed{};try{f();}catch(const std::logic_error&){failed=true;}check(failed,"native damage must reject");};
    const auto a=attacker.ActorAddress(),d=defender.ActorAddress();
    const auto word=[](std::uint32_t p)->std::uint32_t&{return *reinterpret_cast<std::uint32_t*>(p);};
    auto& random=system.Random();
    struct Restore {RandomNumberGenerator& random;RandomNumberGenerator saved;
        std::vector<std::pair<std::uint32_t,std::uint32_t>> fields;
        ~Restore(){random=saved;for(const auto& [p,v]:fields)*reinterpret_cast<std::uint32_t*>(p)=v;}
    } restore{random,random};
    const auto hp_a=attacker.CurrentStat(7u),hp_d=defender.CurrentStat(7u);
    const auto initial=calculator.Calculate(a,d);
    check(std::isfinite(initial.main) && std::isfinite(initial.secondary),"actual loaded pair computes finite normal damage");
    const auto extended=calculator.CalculateExtended(d,a);
    check(std::isfinite(extended.main) && std::isfinite(extended.secondary),"actual reverse pair computes finite extended damage");
    // Explicit reversible operand fixtures on actual owned actors. The original
    // property receivers, formula catalog and global RNG remain real.
    auto set=[&](std::uint32_t address,std::uint32_t value){
        if(std::none_of(restore.fields.begin(),restore.fields.end(),[&](const auto& e){return e.first==address;}))
            restore.fields.emplace_back(address,word(address));
        word(address)=value;
    };
    auto floating=[&](std::uint32_t address,float value){set(address,std::bit_cast<std::uint32_t>(value));};
    set(a+356u,200u);set(a+372u,80u);set(d+360u,20u);set(d+340u,0u);set(d+660u,0u);
    for(unsigned i=0;i<6u;++i)set(a+392u+i*4u,0u);
    for(unsigned i=0;i<5u;++i){set(a+464u+i*4u,0u);set(d+464u+i*4u,0u);}
    floating(a+584u,1.25f);floating(a+588u,1.5f);floating(a+604u,1.1f);
    floating(d+416u,.25f);floating(d+440u,.2f);
    check(attacker.StatParameter(3u)==200 && defender.StatParameter(4u)==20 && attacker.AttackBonus(0u)==0 &&
          attacker.ElementStat(0u)==0 && attacker.AttackFactor(1u)==1.5 && defender.ReflectedDamageFactor(0u)==.25 &&
          defender.DamageReduction(0u)==static_cast<double>(.2f) && attacker.RaceAttackFactor(0u)==static_cast<double>(1.1f),
          "exact actor28/40/48/88/100/112/124 getter routing");
    check(attacker.ElementStat(5u)==5 && attacker.AttackBonus(6u)==6 && attacker.AttackFactor(5u)==5.0 &&
          attacker.ReflectedDamageFactor(6u)==6.0 && attacker.DamageReduction(6u)==6.0 && attacker.RaceAttackFactor(7u)==7.0 &&
          attacker.AttackFactor(0xffffffffu)==-1.0,"original out-of-range getter returns retain signedness");
    const auto approximately_equal=[&](double x,double y){return std::abs(x-y)<=std::max(0.001,std::abs(y)*0.00001);};
    double standard{},critical{};
    for(bool ext:{false,true}){
        random.Seed(0x1234u);auto expected_rng=random;
        const auto roll=expected_rng.Next()%1000u;
        if(ext)expected_rng=random; // Extended has no primary random draw.
        for(unsigned i=0;i<5u;++i)(void)expected_rng.Next();
        const auto result=ext?calculator.CalculateExtended(a,d):calculator.Calculate(a,d);
        auto observed_rng=random,comparison_rng=expected_rng;
        check(observed_rng.Next()==comparison_rng.Next() && random.Index()==expected_rng.Index(),"exact original six/five random draws");
        const double base=std::max(0.,static_cast<double>(formulas.FloatAt(0))*200.-20.);
        const auto lo=std::min(formulas.FloatAt(1),formulas.FloatAt(2));
        const auto hi=std::max(formulas.FloatAt(1),formulas.FloatAt(2));
        const double factor=ext?formulas.FloatAt(3):lo+roll*.001*(hi-lo);
        const double raw=base*factor*1.5*static_cast<double>(1.1f);
        check(approximately_equal(result.main,-raw*(1.-static_cast<double>(.2f))) && approximately_equal(result.secondary,-raw*.25),
              "controlled actual operands match independent reduced formula");
        (ext?critical:standard)=result.main;
        set(d+660u,1u);random.Seed(0x1234u);
        const auto half=ext?calculator.CalculateExtended(a,d):calculator.Calculate(a,d);
        check(approximately_equal(half.main,result.main*.5) && half.secondary==result.secondary,"defending halves main only");
        random.Seed(0x1234u);const auto skip=ext?calculator.CalculateExtended(a,d,true):calculator.Calculate(a,d,true);
        check(skip.main==result.main && skip.secondary==result.secondary,"skip-half retains full deltas");set(d+660u,0u);
        set(a+396u,100u);set(a+464u,40u);set(d+468u,20u); // Element0 -> Next(0)=1.
        random.Seed(0x1234u);const auto element=ext?calculator.CalculateExtended(a,d):calculator.Calculate(a,d);
        check(std::isfinite(element.main) && element.main<result.main && element.secondary==result.secondary,
              "actual element/bonus contribution affects main only");
        set(a+396u,0u);set(a+464u,0u);set(d+468u,0u);
    }
    auto before=random;
    reject([&]{(void)calculator.Calculate(a,d+4u);});reject([&]{(void)calculator.Calculate(0u,d);});
    auto after=random;check(before.Next()==after.Next() && before.Index()==after.Index(),"invalid pairs rejected before RNG consumption");
    const auto property=word(d+800u);set(d+800u,0u);
    // Absent base property must not silently return an invented factor.
    reject([&]{(void)calculator.Calculate(a,d);});set(d+800u,property);
    check(attacker.CurrentStat(7u)==hp_a && defender.CurrentStat(7u)==hp_d,"calculation does not apply HP changes");
    std::cout<<"Native combat damage: "<<label<<" actual actor properties/formulas/RNG normal="<<initial.main
        <<" reverse_extended="<<extended.main<<" controlled="<<standard<<"/"<<critical
        <<", half/skip/element/signed bounds/foreign gates pass; HP unchanged\n";
}
}
