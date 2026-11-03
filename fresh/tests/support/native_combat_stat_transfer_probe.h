#pragma once
#include "pal4/native_combat_actor_skill_runtime.h"
#include "pal4/native_skill_record_catalog.h"
#include "pal4/native_skill_system_services.h"
#include "pal4/native_script_event_router.h"
#include "pal4/random_number_generator.h"
#include "pal4/native_skill_numeric.h"
#include "pal4/native_combat_actor_owner.h"
#include "pal4/native_combat_player_actor_owner.h"
#include "pal4/native_combat_skill_actor_registry.h"
#include "pal4/native_monster_record_catalog.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace pal4::test {
inline void VerifyNativeCombatStatTransfer(NativeCombatActorSkillRuntime& player,
    NativeCombatActorSkillRuntime& monster,NativeSkillRecordCatalog& catalog,
    NativeScriptEventRouter& router,INativeSkillSystemServices& system) {
    const auto check=[](bool b,const char* why){if(!b)throw std::runtime_error(why);};
    const auto word=[](std::uint32_t a)->std::uint32_t&{return *reinterpret_cast<std::uint32_t*>(a);};
    std::uint32_t id{};float gate{},hp_scale{},mp_scale{};
    for(std::size_t i=0;i<catalog.Size();++i){const auto& row=catalog.At(i);
        if(row[4]!=9106u || row[14])continue;
        const auto g=std::bit_cast<float>(row[5]),h=std::bit_cast<float>(row[6]),m=std::bit_cast<float>(row[7]);
        if(std::isfinite(g) && g>0.f && std::isfinite(h) && h>=0.f && h<1.f &&
           std::isfinite(m) && m>=0.f && m<1.f && (h>0.f || m>0.f)){
            id=row[0];gate=g;hp_scale=h;mp_scale=m;break;
        }
    }
    check(id!=0u,"original terminal9106 transfer Skill exists");
    const auto saved_rng=system.Random();
    struct RestoreRng {RandomNumberGenerator& owner;RandomNumberGenerator saved;~RestoreRng(){owner=saved;}} rng{system.Random(),saved_rng};
    const auto a=player.ActorAddress(),b=monster.ActorAddress();
    const auto immunity=word(b+536u);
    const auto ph=player.CurrentStat(7u),pm=player.CurrentStat(9u),mh=monster.CurrentStat(7u),mm=monster.CurrentStat(9u);
    unsigned immunity_events{};
    const auto token=router.Subscribe(router.Key("ae_immunity_skill"),1u,[&](std::uint32_t p){
        check(word(p)==b+716u && word(p+4u)==catalog.Address(id) && word(p+8u)==a && word(p+12u)==0u,"9106 immunity retains facet/record/context/mode");++immunity_events;
    });
    std::uint32_t seed=1u;
    for(;seed<10000u;++seed){RandomNumberGenerator r;r.Seed(seed);if(static_cast<double>(r.Next()%1000u)*static_cast<double>(0.001f)<=gate)break;}
    check(seed<10000u,"9106 deterministic passing seed");
    for(bool immune:{false,true}){
        (void)player.SetCurrentStat(7u,1);(void)player.SetCurrentStat(9u,0);
        const auto max_hp=monster.StatParameter(0u),max_mp=monster.StatParameter(2u);
        (void)monster.SetCurrentStat(7u,max_hp);(void)monster.SetCurrentStat(9u,max_mp);
        word(b+536u)=std::bit_cast<std::uint32_t>(immune?1.f:0.f);
        system.Random().Seed(seed);auto expected_rng=system.Random();(void)expected_rng.Next();(void)expected_rng.Next();
        check(player.TriggerForTarget(id,b),"9106 actual registered callback dispatch");
        const auto dh=static_cast<int>(TruncateSkillFloat(static_cast<float>(static_cast<double>(max_hp)*hp_scale)));
        const auto dm=static_cast<int>(TruncateSkillFloat(static_cast<float>(static_cast<double>(max_mp)*mp_scale)));
        check(player.CurrentStat(7u)==(immune?1:std::clamp(1+dh,0,player.StatParameter(0u))) &&
              monster.CurrentStat(7u)==(immune?max_hp:max_hp-dh) &&
              player.CurrentStat(9u)==(immune?0:std::clamp(dm,0,player.StatParameter(2u))) &&
              monster.CurrentStat(9u)==(immune?max_mp:max_mp-dm),"9106 base-stat HP/MP transfer and immunity branch");
        check(immunity_events==(immune?1u:0u),"9106 immunity dispatched exactly once without stat transfer");
        check(system.Random().Index()==expected_rng.Index(),"9106 gate and immunity consume exactly two draws");
        for(unsigned i=0;i<RandomNumberGenerator::kStateCount;++i)check(system.Random().StateAt(i)==expected_rng.StateAt(i),"9106 exact RNG words");
    }
    (void)router.Unsubscribe(token);word(b+536u)=immunity;
    (void)player.SetCurrentStat(7u,ph);(void)player.SetCurrentStat(9u,pm);
    (void)monster.SetCurrentStat(7u,mh);(void)monster.SetCurrentStat(9u,mm);
    std::cout<<"Original Skill9106 transfer: ID="<<id<<" HPscale="<<hp_scale<<" MPscale="<<mp_scale<<" actual callback, immunity, two draws and restoration pass\n";
}
inline void VerifyNativeCombatStatTransferOwners(NativePlayerManagerSession& source,
    NativeInventorySession& inventory,NativeSkillRecordCatalog& catalog,
    const NativeMonsterRecordCatalog& monsters,INativeSkillSystemServices& system) {
    NativeScriptEventRouter router;
    NativeCombatSkillActorRegistry registry(router,catalog,system);
    NativeCombatActorOwner monster;(void)monster.Construct(monsters,2502);
    NativeCombatActorSkillRuntime monster_skills(monster,registry);(void)monster_skills.RebuildStats();
    NativeCombatPlayerActorOwner player;(void)player.Construct(source,0u);
    NativeCombatActorSkillRuntime player_skills(player,registry,inventory);(void)player_skills.RebuildStats();
    VerifyNativeCombatStatTransfer(player_skills,monster_skills,catalog,router,system);
    player_skills.Close();monster_skills.Close();player.Destroy();monster.Destroy();
    if(registry.Count())throw std::runtime_error("stat-transfer actor registry did not retire");
}
}
