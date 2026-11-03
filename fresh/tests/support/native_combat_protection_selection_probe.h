#pragma once
#include "pal4/native_combat_protection_selection.h"
#include "pal4/native_combat_player_manager.h"
#include "pal4/native_combat_monster_manager.h"
#include "pal4/native_combat_actor_skill_runtime.h"
#include "pal4/native_combat_formula_catalog.h"
#include "pal4/native_skill_system_services.h"
#include "pal4/random_number_generator.h"
#include <array>
#include <iostream>
#include <stdexcept>
#include <vector>
namespace pal4::test {
inline void VerifyNativeCombatProtectionSelection(NativeCombatProtectionSelection& selection,
    NativeCombatPlayerManager& players,NativeCombatMonsterManager& monsters,
    std::span<NativeCombatActorSkillRuntime* const> skills,const NativeCombatFormulaCatalog& formulas,
    INativeSkillSystemServices& system){
    const auto check=[](bool b,const char* why){if(!b)throw std::runtime_error(why);};
    const auto word=[](std::uint32_t a)->std::uint32_t&{return *reinterpret_cast<std::uint32_t*>(a);};
    check(players.StageCount()==2u && monsters.StageCount()>0u,"actual protection rosters available");
    const auto target=players.StageActorAt(0),candidate=players.StageActorAt(1),source=monsters.StageActorAt(0);
    auto& random=system.Random();
    struct Restore {
        RandomNumberGenerator& random;RandomNumberGenerator saved;
        std::array<std::uint32_t*,4> fields;std::array<std::uint32_t,4> values;
        ~Restore(){random=saved;for(std::size_t i=0;i<fields.size();++i)*fields[i]=values[i];}
    } restore{random,random,{&word(candidate+636u),&word(candidate+648u),&word(target+648u),&word(source+1148u)},
        {word(candidate+636u),word(candidate+648u),word(target+648u),word(source+1148u)}};
    const auto initial=selection.Select(source,target);
    check((!initial || initial==candidate) && !players.ActiveIterators(),"actual initial protection uses staged ally and releases cursor");
    // Only state fields and RNG seed are fixtures; actors/rosters/formulas and
    // property dispatch remain actual. All changes restore on success/failure.
    word(candidate+636u)=0u;word(candidate+648u)=0u;word(target+648u)=1u;
    const auto no_random=[&](auto&& operation,const char* why){
        const auto before=random;check(!operation(),why);
        auto expected=before;check(random.Next()==expected.Next(),"protection gate must not consume RNG");
        random=before;check(!players.ActiveIterators(),"protection gate cursor balanced");
    };
    word(source+1148u)=1u;no_random([&]{return selection.Select(source,target);},"actual Monster1148 blocks protection");
    word(source+1148u)=0u;
    no_random([&]{return selection.Select(target,candidate);},"same-type attack cannot select protector");
    no_random([&]{return selection.Select(target,source);},"Monster target cannot select protector");
    no_random([&]{return selection.CanProtect(source,target);},"Monster virtual416 returns false");
    no_random([&]{return selection.CanProtect(candidate,source);},"Player eligibility rejects Monster target before Player-only reads");
    no_random([&]{return selection.CanProtect(candidate,candidate);},"self protection rejected");
    word(candidate+636u)=4u;
    no_random([&]{return selection.CanProtect(candidate,target);},"actual primary property blocks protection");
    word(candidate+636u)=0u;word(target+648u)=3u;
    no_random([&]{return selection.CanProtect(candidate,target);},"actual target secondary property blocks protection");
    word(target+648u)=1u;
    unsigned accepted{},rejected{};
    for(std::uint32_t seed=0;seed<256u;++seed){
        random.Seed(seed);auto expected=random;
        const auto eligible=selection.CanProtect(candidate,target);(void)expected.Next();
        check(random.Next()==expected.Next(),"eligible probability consumes exactly one actual RNG word");
        random.Seed(seed);expected=random;
        const auto chosen=selection.Select(source,target);(void)expected.Next();
        if(eligible){++accepted;(void)expected.Next();}else ++rejected;
        check(chosen==(eligible?candidate:0u) && random.Next()==expected.Next(),
            "stage selection preserves eligibility and extra random choice draw");
        check(!players.ActiveIterators(),"protection repeated selection cursor balanced");
    }
    check(accepted>0u && rejected>0u,"original formulas exercise both probability outcomes");
    word(candidate+648u)=3u;
    no_random([&]{return selection.Select(source,target);},"health-state3 candidate skipped before probability draw");
    word(candidate+648u)=0u;
    bool failed{};try{selection.Select(0x1000u,target);}catch(const std::invalid_argument&){failed=true;}
    check(failed && !players.ActiveIterators(),"foreign protection source rejected before dereference");
    NativeCombatProtectionSelection missing(players,monsters,{},formulas,system);
    failed=false;try{missing.Select(source,target);}catch(const std::logic_error&){failed=true;}
    check(failed && !players.ActiveIterators(),"missing property services unwind actual roster cursor");
    std::cout<<"Native combat protection selection: actual rosters/properties/formulas, both probability outcomes="
        <<accepted<<'/'<<rejected<<", RNG draw order, gates and cursor failure cleanup pass\n";
}
}
