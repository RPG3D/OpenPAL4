#pragma once
#include "pal4/native_combat_roster_targets.h"
#include "pal4/native_combat_player_manager.h"
#include "pal4/native_combat_monster_manager.h"
#include <array>
#include <iostream>
#include <stdexcept>
#include <utility>
#include <vector>
namespace pal4::test {
inline void VerifyNativeCombatReadyCandidates(NativeCombatRosterTargets& targets,
    NativeCombatPlayerManager& players,NativeCombatMonsterManager& monsters){
    const auto check=[](bool ok,const char* why){if(!ok)throw std::runtime_error(why);};
    const auto word=[](std::uint32_t a)->std::uint32_t&{return *reinterpret_cast<std::uint32_t*>(a);};
    check(players.StageCount() && monsters.StageCount(),"ready candidates require actual mixed stage");
    struct Restore{
        std::vector<std::pair<std::uint32_t,std::uint32_t>> words;
        ~Restore(){for(auto [a,v]:words)*reinterpret_cast<std::uint32_t*>(a)=v;}
    } restore;
    std::array<std::vector<std::uint32_t>,2> sides;
    for(std::size_t i=0;i<players.StageCount();++i)sides[0].push_back(players.StageActorAt(i));
    for(std::size_t i=0;i<monsters.StageCount();++i)sides[1].push_back(monsters.StageActorAt(i));
    for(const auto& side:sides)for(auto actor:side)for(auto offset:{648u,800u,828u})
        restore.words.emplace_back(actor+offset,word(actor+offset));
    const auto balanced=[&]{check(!targets.ActiveIterators() && !players.ActiveIterators() && !monsters.ActiveIterators(),
        "actual ready candidate cursors retire on every return and exception");};
    for(unsigned side=0;side<2u;++side){
        const auto source=sides[side].front();
        word(source+828u)=1u;word(source+648u)=0u;
        for(auto actor:sides[1u-side])word(actor+648u)=0u;
        check(targets.ProcessReadyCandidate(source),"actual Player/Monster uses opposite stage and property slot4");balanced();
        word(source+828u)=0u;
        check(!targets.ProcessReadyCandidate(source),"source facet zero blocks candidate processing");balanced();
        word(source+828u)=1u;word(source+648u)=3u;
        check(!targets.ProcessReadyCandidate(source),"source health3 blocks candidate processing");balanced();
        word(source+648u)=0u;
        for(auto actor:sides[1u-side])word(actor+648u)=3u;
        check(!targets.ProcessReadyCandidate(source),"all opposite actors health3 rejects action");balanced();
        word(sides[1u-side].back()+648u)=0u;
        check(targets.ProcessReadyCandidate(source),"skip health3 then find actual opposite candidate");balanced();
        const auto first=sides[1u-side].front(),property=word(first+800u);
        word(first+800u)=0u;bool failed{};
        try{(void)targets.ProcessReadyCandidate(source);}catch(const std::logic_error&){failed=true;}
        word(first+800u)=property;
        check(failed,"invalid concrete target property propagates through actual query");balanced();
        check(targets.ProcessReadyCandidate(source),"candidate query recovers after real property failure");balanced();
    }
    bool failed{};try{(void)targets.ProcessReadyCandidate(0x1000u);}catch(const std::invalid_argument&){failed=true;}
    check(failed,"foreign ready candidate source rejected before dereference");balanced();
    std::cout<<"Native ready candidates: shared52B410, actual Player/Monster opposite stage, source gates, health3 scan, property4, failure cursor cleanup and recovery pass\n";
}
}
