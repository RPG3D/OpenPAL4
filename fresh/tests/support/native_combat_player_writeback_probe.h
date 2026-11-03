#pragma once
#include "pal4/native_combat_player_writeback.h"
#include "pal4/native_combat_player_actor_owner.h"
#include "pal4/native_player_manager_session.h"
#include "pal4/native_player_slot_process_runtime.h"
#include "pal4/native_player_script_refresh_runtime.h"
#include <array>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
namespace pal4::test {
inline void VerifyNativeCombatPlayerWriteback(NativePlayerManagerSession& players,
    NativeBaseObjectMemory& memory,NativePlayerSlotProcessRuntime& processor,
    NativePlayerScriptRefreshRuntime& refresh){
    const auto check=[](bool ok,const char* msg){if(!ok)throw std::runtime_error(msg);};
    const auto word=[](std::uint32_t a)->std::uint32_t&{return *reinterpret_cast<std::uint32_t*>(a);};
    const auto text=[&](std::uint32_t r){return word(r+4u)?std::string(reinterpret_cast<const char*>(word(r+4u)),word(r+8u)):std::string{};};
    NativeCombatPlayerWriteback writeback(players,memory,processor,refresh);
    NativeCombatPlayerActorOwner baseline,actor;
    (void)baseline.Construct(players,0u);
    const auto a=actor.Construct(players,0u),s=players.Address(),d=s+1620u;
    const auto party=word(d+668u),hp=word(d+572u),mp=word(d+580u);
    const auto allocations=memory.OutstandingAllocations();
    bool rejected{};
    try{(void)writeback.Copy(actor,4u);}catch(const std::invalid_argument&){rejected=true;}
    check(rejected && word(d+572u)==hp,"outcome invalid index rejects before copying");
    word(a+572u)=0u;word(a+580u)=0u;word(a+668u)=123u;word(a+332u)=123u;
    for(auto i:{158u,159u,160u,161u,163u,164u,165u,166u})word(a+i*4u)=1u;
    std::array<std::string,9> names;
    for(unsigned i=0;i<9u;++i)names[i]=text(a+(i<4u?1112u+i*16u:1208u+(i-4u)*16u));
    check(writeback.Copy(actor,0u)==party,"outcome restores saved party position");
    check(word(d+572u)==1u && word(d+580u)==0u,"outcome revives zero HP and preserves zero MP");
    check(word(d+668u)==party && word(d+332u)==party,"outcome restores both party-position mirrors");
    for(auto i:{158u,159u,160u,161u,163u,164u,165u,166u})check(!word(d+i*4u),"outcome resets eight real status properties");
    for(unsigned i=0;i<9u;++i){
        const auto sr=a+(i<4u?1112u+i*16u:1208u+(i-4u)*16u);
        const auto dr=s+(i<4u?2552u+i*16u:2648u+(i-4u)*16u);
        players.CopyRuntime().ValidateText(dr);
        check(text(dr)==names[i] && (!word(sr+8u) || word(sr+4u)!=word(dr+4u)),"outcome persistent strings are independent");
    }
    actor.Destroy();
    for(unsigned i=0;i<9u;++i){
        const auto dr=s+(i<4u?2552u+i*16u:2648u+(i-4u)*16u);
        players.CopyRuntime().ValidateText(dr);check(text(dr)==names[i],"outcome strings survive Combat owner destruction");
    }
    for(unsigned repeat=0;repeat<2u;++repeat){
        (void)writeback.Copy(baseline,0u);
        check(word(d+572u)==hp && word(d+580u)==mp && word(d+668u)==party,"baseline outcome restores real loaded stats");
    }
    baseline.Destroy();
    check(memory.OutstandingAllocations()==allocations,"empty initial skill/learned arrays preserve persistent allocation balance");
    std::cout<<"Native5388D0 Player outcome: real loaded Scene player, HP0->1, MP0, eight status resets, party mirror restore, nine independent strings survive Combat destruction, repeated baseline writeback pass\n";
}
}
