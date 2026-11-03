#pragma once
#include "pal4/native_combat_stats_actors.h"
#include "pal4/native_combat_game_event_handler.h"
#include "pal4/native_combat_player_manager.h"
#include "pal4/native_combat_monster_manager.h"
#include "pal4/native_combat_actor_skill_runtime.h"
#include "pal4/native_script_event_router.h"
#include "pal4/native_combat_system_image.h"
#include <array>
#include <iostream>
#include <stdexcept>
namespace pal4::test {
inline void VerifyNativeCombatStatsActors(const NativeGameDatabase& database,NativeCombatSystemImage& image,
    NativeCombatPlayerManager& players,NativeCombatMonsterManager& monsters,
    std::span<NativeCombatActorSkillRuntime* const> skills) {
    const auto check=[](bool b,const char* reason){if(!b)throw std::runtime_error(reason);};
    NativeScriptEventRouter router;
    NativeCombatGameEventHandler stats(database,router,reinterpret_cast<std::uint32_t>(&image));
    NativeCombatStatsActors actors(players,monsters,skills,stats);stats.BindActors(&actors);
    struct Unbind {NativeCombatGameEventHandler& stats;~Unbind(){stats.BindActors(nullptr);}} unbind{stats};
    auto count=[&](unsigned stat){return *reinterpret_cast<const std::uint32_t*>(stats.Address()+0xf0u+stat*4u);};
    const auto player=players.ActorAt(0),monster=monsters.ActorAt(0);
    const std::array pair{player+716u,monster};
    check(router.Dispatch("ae_joint_attack",reinterpret_cast<std::uint32_t>(pair.data()))==1u && count(1)==1u,
          "actual Player facet reaches original joint stat callback");
    const std::array reversed{monster+716u,player};
    (void)router.Dispatch("ae_physical_attack",reinterpret_cast<std::uint32_t>(reversed.data()));
    const auto role=actors.ReadWord(player+1192u);
    check(role<4u && *reinterpret_cast<const std::uint32_t*>(stats.Address()+0x248u+role*4u)==1u,
          "actual Monster-to-Player event indexes original per-role target counter");
    bool rejected{};try{(void)actors.CreateIterator(actors.GetActorManager()+8u);}catch(const std::invalid_argument&){rejected=true;}
    check(rejected,"erroneous byte offset8 rejected before native iteration");
    unsigned full{},secondary{};bool all_zero=true;
    for(std::size_t i=0;i<players.Count();++i){const auto a=players.ActorAt(i);
        full+=actors.ActorProperty52(a,7u)==actors.ActorProperty48(a,0u);
        secondary+=actors.MonsterState(a)==2u;all_zero=all_zero && actors.ActorMetric252(a)==0u;
    }
    stats.Reset();(void)router.Dispatch("combat_start",0u);
    check(count(14)==secondary && !players.ActiveIterators(),"original battle-start stats traverse actual Player first roster at32");
    stats.Reset();(void)router.Dispatch("combat_over",0u);
    check(count(12)==4u && count(13)==full && count(15)==players.Count() && count(16)==(all_zero?1u:0u) &&
          !players.ActiveIterators() && !monsters.ActiveIterators(),"original end stats query real HP, maxHP, metric832 and release first-list cursors");
    const auto average_monster=actors.AverageProperty4(actors.GetActorManager());
    const auto average_player=actors.AverageProperty4(actors.GetActorManager()+32u);
    check(count(19)==(average_monster<average_player?1u:0u) && count(20)==(average_monster>average_player?1u:0u),
          "original average-level comparison uses actual Monster and Player first rosters");
    stats.Reset();unsigned score_events{};
    const auto subscription=router.Subscribe(router.Key("score_changed"),1u,[&](std::uint32_t){
        if(++score_events==5u)throw std::runtime_error("intentional real stat iterator callback failure");
    });
    rejected=false;try{(void)router.Dispatch("combat_over",0u);}catch(const std::runtime_error&){rejected=true;}
    (void)router.Unsubscribe(subscription);
    check(rejected && score_events==5u && !players.ActiveIterators(),"score callback failure unwinds actual first-list cursor");
    std::cout<<"Native combat stats actors: actual facet/role counters, Player first roster32, HP/metric832/averages, end iteration and exception cursor cleanup pass\n";
}
}
