#pragma once
#include "pal4/native_combat_ai_decision.h"
#include "pal4/native_combat_player_manager.h"
#include "pal4/native_combat_monster_manager.h"
#include "pal4/native_combat_actor_skill_runtime.h"
#include "pal4/native_script_event_router.h"
#include "pal4/native_skill_system_services.h"
#include "pal4/random_number_generator.h"
#include <bit>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>
namespace pal4::test {
inline void VerifyNativeCombatAiDecision(NativeCombatAiDecisionRuntime& decisions,
    NativeCombatPlayerManager& players,NativeCombatMonsterManager& monsters,
    std::span<NativeCombatActorSkillRuntime* const> player_skills,
    NativeScriptEventRouter& router,INativeSkillSystemServices& system){
    const auto check=[](bool ok,const char* why){if(!ok)throw std::runtime_error(why);};
    const auto word=[](std::uint32_t a)->std::uint32_t&{return *reinterpret_cast<std::uint32_t*>(a);};
    check(!decisions.Configs().empty() && !decisions.Properties().empty(),"actual AI database catalogs loaded");
    std::vector<NativeCombatActorSkillRuntime*> actors(player_skills.begin(),player_skills.end());
    for(std::size_t i=0;i<monsters.Count();++i)actors.push_back(&monsters.Skills(monsters.ActorAt(i)));
    struct Restore{
        NativeScriptEventRouter& router;std::uint32_t handle{};
        std::vector<std::pair<std::uint32_t,std::uint32_t>> fields;
        ~Restore(){if(handle)(void)router.Unsubscribe(handle);for(auto [a,v]:fields)*reinterpret_cast<std::uint32_t*>(a)=v;}
    } restore{router};
    for(auto* actor:actors)for(auto offset:{672u,832u,572u})restore.fields.emplace_back(actor->ActorAddress()+offset,word(actor->ActorAddress()+offset));
    struct LogFailure final:std::runtime_error{LogFailure():std::runtime_error("AI log failure fixture") {}};
    auto before=system.Random();unsigned logs{},hits{},turns{},failures{};bool fail_log{};
    restore.handle=router.Subscribe(router.Key("caae_message"),3u,[&](std::uint32_t p){
        const auto source=std::string_view(reinterpret_cast<const char*>(word(p)));
        const auto text=std::string_view(reinterpret_cast<const char*>(word(p+4u)));
        check(source.starts_with("cbActorAi::get") && text.starts_with("return EA"),"actual AI log event carries original source/text");++logs;
        bool rejected{};try{decisions.Close();}catch(const std::logic_error&){rejected=true;}
        check(rejected,"AI log dispatch pins runtime Close");
        rejected=false;try{(void)decisions.Choose(actors.front()->ActorAddress());}catch(const std::logic_error&){rejected=true;}
        check(rejected,"AI decision rejects reentry from public log event");
        if(fail_log)throw LogFailure();
    });
    const auto balanced=[&]{check(!decisions.ActiveIterators() && !players.ActiveIterators() && !monsters.ActiveIterators(),"AI actual cursors retire on normal/exception paths");};
    const auto verify=[&](const NativeCombatAiDecision& d){
        if(!d.property)return;
        std::uint32_t first{};
        for(const auto& row:decisions.Properties())if(row[1]==d.ai_type && row[2]==static_cast<std::uint32_t>(d.team) && row[3]==static_cast<std::uint32_t>(d.enemy)){
            first=reinterpret_cast<std::uint32_t>(row.data());break;
        }
        check(d.property==first,"shared ChooseRecord returns first actual AI type/team/enemy property row");++hits;
    };
    for(auto* actor:actors){
        const auto a=actor->ActorAddress();
        check(actor->AiType()==word(a+672u) && actor->TurnCount()==std::bit_cast<std::int32_t>(word(a+832u)),"actual actor virtual12/252 values");
        check(actor->AiTaskAddress()==word(a+(actor->IsPlayerActor()?1288u:1156u)),"actual owned AI task facet used");
        auto d=decisions.Choose(a);verify(d);balanced();
        if(!d.config)continue;
        const auto* config=reinterpret_cast<const std::uint32_t*>(d.config);
        const auto turn=static_cast<std::uint32_t>(static_cast<std::int32_t>(std::bit_cast<float>(config[12])));
        word(a+832u)=turn;d=decisions.Choose(a);
        check(d.team==AiTeamState::TurnMatch,"actual AiConfig turn-match branch wins before HP classification");++turns;verify(d);balanced();
        for(unsigned count=1;count<=4u && !failures;++count){
            word(a+832u)=count;const auto previous=logs;fail_log=true;bool failed{};
            try{d=decisions.Choose(a);verify(d);}catch(const LogFailure&){failed=true;}
            fail_log=false;balanced();
            if(logs>previous){check(failed,"actual AI log exception propagates");++failures;verify(decisions.Choose(a));balanced();}
        }
    }
    check(hits && turns && logs && failures,"real database and actors cover row selection, turn gate, log failure and recovery");
    // Effective AI type is distinct from its initial source configuration.
    const auto a=actors.front()->ActorAddress();word(a+672u)=0xfffffffeu;
    const auto missing=decisions.Choose(a);
    check(!missing.config && !missing.property && missing.ai_type==0xfffffffeu,"missing actual AI type preserves null lookup result");balanced();
    bool rejected{};try{(void)decisions.Choose(0x1000u);}catch(const std::invalid_argument&){rejected=true;}
    check(rejected,"foreign AI actor rejected before reading memory");
    auto after=system.Random();check(before.Next()==after.Next(),"AI row classification consumes no action RNG");
    std::cout<<"Native AI decision: configs="<<decisions.Configs().size()<<" properties="<<decisions.Properties().size()
        <<" actors="<<actors.size()<<" rows="<<hits<<" turn_matches="<<turns<<" logs="<<logs
        <<" actual facets/stats/rosters, null lookup, callback guards, failure cleanup/recovery and RNG preservation pass\n";
}
}
