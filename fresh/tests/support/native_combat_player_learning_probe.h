#pragma once
#include "pal4/native_combat_actor_skill_runtime.h"
#include "pal4/native_combat_player_actor_owner.h"
#include "pal4/native_combat_skill_actor_registry.h"
#include "pal4/native_player_manager_session.h"
#include "pal4/player_manager_object.h"
#include "pal4/native_stunt_learning_catalog.h"
#include "pal4/native_script_event_router.h"
#include "pal4/native_skill_system_services.h"
#include "pal4/random_number_generator.h"
#include "pal4/native_game_database.h"
#include <algorithm>
#include <array>
#include <bit>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <vector>
namespace pal4::test {
inline void VerifyNativeCombatPlayerLearning(NativePlayerManagerSession& source,NativeInventorySession& inventory,
    NativeSkillRecordCatalog& records,INativeSkillSystemServices& system,const NativeGameDatabase& database) {
    const auto check=[](bool ok,const char* why){if(!ok)throw std::runtime_error(why);};
    const auto word=[](std::uint32_t a)->std::uint32_t&{return *reinterpret_cast<std::uint32_t*>(a);};
    const auto less=[](auto l,auto r){return std::bit_cast<std::int32_t>(l)<std::bit_cast<std::int32_t>(r);};
    const auto values=[&](std::uint32_t slot){
        std::vector<std::uint32_t> out;
        for(auto p=word(slot+4u);p!=word(slot+8u);p+=4u)out.push_back(word(p));
        return out;
    };
    NativeStuntLearningCatalog learning(database);
    const auto& table=database.Tables().at("Stunt");
    constexpr std::array<std::string_view,4> fields{
        "ePT_TIANHE_LV_LMT","ePT_LINSHA_LV_LMT","ePT_MENGLI_LV_LMT","ePT_ZIYING_LV_LMT"};
    const auto column=[&](std::string_view field){
        const auto it=std::find_if(table.fields.begin(),table.fields.end(),[&](const auto& f){return f.name==field;});
        check(it!=table.fields.end(),"learning expected original schema field");return static_cast<std::size_t>(it-table.fields.begin());
    };
    const auto id_column=column("ID");
    const auto saved_rng=system.Random();
    std::size_t learned_total{};
    for(unsigned role=0;role<4u;++role) {
        // Copy real loaded player data through the actual537940 constructor.
        // Independent Combat owners/registry keep this diagnostic's level99
        // learning separate from the in-progress party and outcome assertions.
        const auto source_slot=source.Address()+role*PlayerManagerObject::kElementBytes+2520u;
        const auto original_source=values(source_slot);
        NativeCombatPlayerActorOwner actor;const auto a=actor.Construct(source,role);
        NativeScriptEventRouter router;NativeCombatSkillActorRegistry registry(router,records,system);
        NativeCombatActorSkillRuntime skills(actor,registry,inventory);
        auto expected=values(a+1080u);const auto allocations=actor.OwnedAllocations(),texts=actor.OwnedTexts();
        const auto requirements=column(fields[role]);
        for(const auto level:{1,10,99,1}) {
            (void)skills.SetCurrentStatViaActor(4u,level);
            std::vector<std::uint32_t> wanted;
            for(const auto& row:table.rows) {
                const auto need=std::bit_cast<std::int32_t>(std::get<std::uint32_t>(row[requirements]));
                const auto id=std::get<std::uint32_t>(row[id_column]);
                if(need>0 && need<=level && !std::binary_search(expected.begin(),expected.end(),id,less))wanted.push_back(id);
            }
            const auto result=skills.LearnPlayerStunts(learning);
            check(result==wanted,"actual53A460 returns only new original Stunts in catalog order");
            learned_total+=result.size();expected.insert(expected.end(),wanted.begin(),wanted.end());
            std::sort(expected.begin(),expected.end(),less);expected.erase(std::unique(expected.begin(),expected.end()),expected.end());
            check(values(a+1080u)==expected && skills.LearnPlayerStunts(learning).empty(),
                "actual host-owned Stunt collection is sorted/unique and repeat/lower level retain learned IDs");
            check(actor.OwnedAllocations()==allocations && actor.OwnedTexts()==texts && !skills.OwnedBuffers(),
                "learning uses host vectors without foreign Scene heap or skill-list scratch ownership");
        }
        check(!expected.empty(),"original role has populated learned Stunts by level99");
        const auto original_header=std::array{word(a+1084u),word(a+1088u),word(a+1092u)};
        word(a+1084u)=0x1000u;bool foreign{};
        try{(void)skills.LearnPlayerStunts(learning);}catch(const std::logic_error&){foreign=true;}
        for(unsigned n=0;n<3u;++n)word(a+1084u+n*4u)=original_header[n];
        check(foreign && skills.LearnPlayerStunts(learning).empty(),"learning rejects foreign collection views and retries after restoration");
        word(a+1192u)=4u;bool invalid{};
        try{(void)skills.LearnPlayerStunts(learning);}catch(const std::logic_error&){invalid=true;}
        word(a+1192u)=role;
        check(invalid && values(a+1080u)==expected,"invalid learning role rejects before collection mutation");
        if(role==0u) {
            NativeGameDbTable controlled;controlled.name="Stunt";
            controlled.fields.push_back({"ID",1u});for(const auto name:fields)controlled.fields.push_back({std::string(name),1u});
            for(const auto id:{0xffffffffu,0x80000000u,0xffffffffu,0x7fffffffu})
                controlled.rows.push_back({id,1u,0u,0u,0u});
            const std::vector<std::uint32_t> wanted{0xffffffffu,0x80000000u,0xffffffffu,0x7fffffffu};
            std::vector<std::uint32_t> kept;
            {NativeStuntLearningCatalog synthetic(controlled);kept=skills.LearnPlayerStunts(synthetic);}
            check(kept==wanted,"two-pass original learning preserves duplicate newly matched records and returns stable IDs past catalog lifetime");
            expected.insert(expected.end(),wanted.begin(),wanted.end());std::sort(expected.begin(),expected.end(),less);
            expected.erase(std::unique(expected.begin(),expected.end()),expected.end());
            check(values(a+1080u)==expected,"actual host collection uses signed32 ordering and unique insertion for INT_MIN/negative IDs");
        }
        check(values(source_slot)==original_source,"Combat learning never changes persistent Scene source collections");
        skills.Close();bool closed{};
        try{(void)skills.LearnPlayerStunts(learning);}catch(const std::logic_error&){closed=true;}
        check(closed && !registry.Count(),"closed learning owner rejects and registry releases actual actor");
        actor.Destroy();check(!actor.OwnedAllocations() && !actor.OwnedArrays() && !actor.OwnedTexts(),"learned host vectors retire with their Combat Player owner");
    }
    check(learned_total>0 && system.Random().Index()==saved_rng.Index(),"original learning consumes no RNG");
    for(unsigned n=0;n<RandomNumberGenerator::kStateCount;++n)check(system.Random().StateAt(n)==saved_rng.StateAt(n),"learning preserves all legacy RNG state words");
    std::cout<<"Native53A460 learning: four original-source Combat Player owners, levels1/10/99/lower, original ordered new Stunt IDs="<<learned_total
        <<", repeat/retention, signed host uniqueness, scoped handles, foreign/closed/role rejection, unchanged Scene/RNG and full cleanup pass\n";
}
}
