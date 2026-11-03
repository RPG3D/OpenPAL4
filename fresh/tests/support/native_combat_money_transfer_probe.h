#pragma once
#include "pal4/native_combat_actor_skill_runtime.h"
#include "pal4/native_combat_actor_owner.h"
#include "pal4/native_combat_player_actor_owner.h"
#include "pal4/native_combat_skill_actor_registry.h"
#include "pal4/native_monster_record_catalog.h"
#include "pal4/native_combat_inventory_events.h"
#include "pal4/native_inventory_session.h"
#include "pal4/native_script_event_router.h"
#include "pal4/native_skill_record_catalog.h"
#include "pal4/native_skill_system_services.h"
#include "pal4/random_number_generator.h"
#include <bit>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace pal4::test {
inline void VerifyNativeCombatMoneyTransfer(NativeCombatActorSkillRuntime& player,
    NativeCombatActorSkillRuntime& monster,NativeInventorySession& inventory,
    NativeSkillRecordCatalog& catalog,NativeScriptEventRouter& router,
    INativeSkillSystemServices& system) {
    const auto check=[](bool ok,const char* why){if(!ok)throw std::runtime_error(why);};
    std::uint32_t id{};int amount{};
    for(std::size_t i=0;i<catalog.Size();++i) {
        const auto& row=catalog.At(i);
        const auto value=std::bit_cast<float>(row[5]);
        if(row[4]==9123u && !row[14] && std::isfinite(value) && value>=1.f && value<10000.f) {
            id=row[0];amount=static_cast<int>(value);break;
        }
    }
    check(id!=0u,"original terminal money-transfer Skill9123 exists");
    const auto money=[&] {return *reinterpret_cast<const int*>(inventory.Address()+0x134u);};
    const auto initial=money();
    const auto set_money=[&](int value){(void)inventory.UpdateCounter(value-money());};
    std::vector<std::pair<std::string,int>> events;
    const auto changed=router.Subscribe(router.Key("add_money"),1u,[&](unsigned payload){
        events.emplace_back("add_money",*reinterpret_cast<const int*>(payload));
        (void)NativeCombatInventoryEvents::Handle(0x57b0a0u,payload,inventory);
    });
    const auto picked=router.Subscribe(router.Key("add_picked_money"),1u,[&](unsigned payload){
        events.emplace_back("add_picked_money",*reinterpret_cast<const int*>(payload));
    });
    const auto before_rng=system.Random();
    set_money(amount+7);
    check(monster.TriggerForTarget(id,player.ActorAddress()),"Monster transfer enters actual Skill dispatch");
    check(money()==7 && events==std::vector<std::pair<std::string,int>>{
        {"add_money",-amount},{"add_picked_money",-amount}},
        "Monster9123 debits Player via both original events, no Monster credit event");
    events.clear();
    check(player.TriggerForTarget(id,monster.ActorAddress()),"Player transfer enters actual mixed dispatch");
    check(money()==amount+7 && events==std::vector<std::pair<std::string,int>>{
        {"add_money",amount},{"add_picked_money",amount}},
        "Player9123 credits Player after original Monster debit RET");
    events.clear();set_money(0);
    check(player.TriggerForTarget(id,player.ActorAddress()),"same-Player transfer enters actual dispatch");
    check(money()==amount && events==std::vector<std::pair<std::string,int>>{
        {"add_money",-amount},{"add_picked_money",-amount},
        {"add_money",amount},{"add_picked_money",amount}},
        "same Player clamps debit before credit, retaining event order and identity");
    events.clear();
    check(monster.TriggerForTarget(id,monster.ActorAddress()) && events.empty() && money()==amount,
        "Monster9123 endpoints are original RET without inventory events");
    // The transfer callback does not consume RNG; compare its complete owner image.
    check(system.Random().Index()==before_rng.Index(),"money transfer leaves RNG index unchanged");
    for(unsigned i=0;i<RandomNumberGenerator::kStateCount;++i)
        check(system.Random().StateAt(i)==before_rng.StateAt(i),"money transfer leaves RNG word unchanged");
    set_money(initial);
    (void)router.Unsubscribe(changed);(void)router.Unsubscribe(picked);
    std::cout<<"Original Skill9123 money transfer: ID="<<id<<" amount="<<amount
             <<" mixed/same actor, debit-before-credit clamp, exact event order, RNG and inventory restoration pass\n";
}

inline void VerifyNativeCombatMoneyTransferOwners(NativePlayerManagerSession& source,
    NativeInventorySession& inventory,NativeSkillRecordCatalog& catalog,
    const NativeMonsterRecordCatalog& monsters,INativeSkillSystemServices& system) {
    NativeScriptEventRouter router;
    NativeCombatSkillActorRegistry registry(router,catalog,system);
    NativeCombatActorOwner monster;
    (void)monster.Construct(monsters,2579);
    NativeCombatActorSkillRuntime monster_skills(monster,registry);
    (void)monster_skills.RebuildStats();
    NativeCombatPlayerActorOwner player;
    (void)player.Construct(source,0u);
    NativeCombatActorSkillRuntime player_skills(player,registry,inventory);
    (void)player_skills.RebuildStats();
    VerifyNativeCombatMoneyTransfer(player_skills,monster_skills,inventory,catalog,router,system);
    player_skills.Close();monster_skills.Close();
    player.Destroy();monster.Destroy();
    if(registry.Count())throw std::runtime_error("money-transfer actor registry did not retire");
}
}
