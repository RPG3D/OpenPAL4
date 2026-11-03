#pragma once
#include "pal4/native_combat_damage_calculator.h"
#include "pal4/native_combat_actor_skill_runtime.h"
#include "pal4/native_combat_formula_catalog.h"
#include "pal4/native_skill_system_services.h"
#include "pal4/random_number_generator.h"
#include <array>
#include <bit>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <vector>
namespace pal4::test {
inline void VerifyNativeCombatAttackGates(NativeCombatDamageCalculator& calculator,
    NativeCombatActorSkillRuntime& source,NativeCombatActorSkillRuntime& target,
    const NativeCombatFormulaCatalog& formulas,INativeSkillSystemServices& system,const char* label){
    const auto check=[](bool ok,const char* why){if(!ok)throw std::runtime_error(why);};
    const auto reject=[&](auto&& f){bool denied{};try{f();}catch(const std::logic_error&){denied=true;}check(denied,"attack gates must reject");};
    const auto a=source.ActorAddress(),d=target.ActorAddress();
    const auto word=[](std::uint32_t p)->std::uint32_t&{return *reinterpret_cast<std::uint32_t*>(p);};
    auto& random=system.Random();
    struct Restore{RandomNumberGenerator& random;RandomNumberGenerator saved;
        std::vector<std::pair<std::uint32_t,std::uint32_t>> fields;
        ~Restore(){random=saved;for(auto [p,v]:fields)*reinterpret_cast<std::uint32_t*>(p)=v;}
    } restore{random,random};
    auto save=[&](std::uint32_t p){for(auto [address,v]:restore.fields)if(address==p)return;restore.fields.emplace_back(p,word(p));};
    const auto flag=a+(std::string_view(label)=="Player"?1204u:1152u);
    save(flag);save(a+800u);save(d+800u);
    const auto property=word(a+800u);save(property+8u);
    for(auto p:{a,d})for(auto offset:{368u,376u,560u,364u,384u,380u})save(p+offset);
    check(source.AllowsAttack() && source.AllowsDamage() && target.AllowsAttack() && target.AllowsDamage(),
          "actual base property8/12 permissions use original null-link semantics");
    word(flag)=0u;check(!source.BlocksReflectedDamage(),"actual424 zero permits reflected HP delta");
    word(flag)=1u;check(source.BlocksReflectedDamage(),"actual424 nonzero blocks reflected HP delta");
    word(flag)=0x80000000u;check(source.BlocksReflectedDamage(),"actual424 is nonzero test, not signed-positive");
    word(property+8u)=1u;reject([&]{(void)source.AllowsAttack();});reject([&]{(void)source.AllowsDamage();});
    word(property+8u)=0u;
    for(auto p:{a,d}){word(p+368u)=0u;word(p+560u)=0u;word(p+364u)=0u;}
    auto number=[&](std::uint32_t p,float value){word(p)=std::bit_cast<std::uint32_t>(value);};
    auto once=[&](auto&& operation,std::uint32_t seed){
        random.Seed(seed);auto expected=random;(void)expected.Next();
        const auto value=operation();auto observed=random,comparison=expected;
        check(observed.Next()==comparison.Next() && random.Index()==expected.Index(),"one original probability RNG draw per query");return value;
    };
    for(float bonus:{-2.f,2.f}){
        number(a+376u,bonus);number(a+384u,bonus);
        number(d+380u,bonus);number(a+380u,-bonus);
        const bool high=bonus>0.f;
        check(once([&]{return calculator.CheckHit(a,d);},7u)==high,"actual Hit threshold clamps at both ends");
        check(once([&]{return calculator.CheckDodge(a,d);},7u)==high,"actual Dodge threshold clamps at both ends");
        check(once([&]{return calculator.CheckFloatProperty(a,d);},7u)==high,"float-property check reads target operand");
        check(once([&]{return calculator.CheckDodge(a,d,high?-2.f:2.f);},7u)!=high,"extra applied after Dodge clamp");
    }
    number(a+376u,0.f);number(a+384u,0.f);number(d+380u,.25f);
    unsigned hits{},dodges{},floats{};
    for(unsigned seed=0u;seed<64u;++seed){
        RandomNumberGenerator expected;expected.Seed(seed);
        const auto sample=expected.Next()%1000u;
        const auto roll=static_cast<float>(sample*.001);
        const auto hit=once([&]{return calculator.CheckHit(a,d);},seed);
        const auto dodge=once([&]{return calculator.CheckDodge(a,d);},seed);
        const auto property_result=once([&]{return calculator.CheckFloatProperty(a,d);},seed);
        check(hit==(formulas.FloatAt(22)>roll) && dodge==(formulas.FloatAt(19)>roll),
              "zero-delta actual operands use original configuration factors22/19");
        check(property_result==(sample*.001<.25+static_cast<double>(.01f)),"float-property exact64-seed threshold");
        hits+=hit;dodges+=dodge;floats+=property_result;
    }
    const auto hp_a=source.CurrentStat(7u),hp_d=target.CurrentStat(7u);
    const auto before=random;
    reject([&]{(void)calculator.CheckHit(a,d+4u);});reject([&]{(void)calculator.CheckDodge(0u,d);});
    reject([&]{(void)calculator.CheckFloatProperty(a,d+4u);});
    word(a+800u)=0u;reject([&]{(void)calculator.CheckHit(a,d);});reject([&]{(void)source.BlocksReflectedDamage();});word(a+800u)=property;
    auto expected=before,observed=random;check(expected.Next()==observed.Next(),"invalid/missing owners reject before RNG use");
    check(source.CurrentStat(7u)==hp_a && target.CurrentStat(7u)==hp_d,"gate queries do not apply damage");
    std::cout<<"Native combat attack gates: "<<label<<" actual property8/12, actor424 and64 seeds hit/dodge/float="
        <<hits<<'/'<<dodges<<'/'<<floats<<", one-draw order, clamp/extra, foreign/link gates and unchanged HP pass\n";
}
}
