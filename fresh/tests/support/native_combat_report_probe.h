#pragma once
#include "pal4/native_combat_report.h"
#include "pal4/native_combat_system_image.h"
#include "pal4/native_combat_game_event_handler.h"
#include "pal4/native_combat_monster_manager.h"
#include "pal4/native_combat_actor_skill_runtime.h"
#include "pal4/native_skill_system_services.h"
#include "pal4/random_number_generator.h"
#include "pal4/native_inventory_session.h"
#include "pal4/native_game_database.h"
#include <array>
#include <bit>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <functional>
namespace pal4::test {
inline void VerifyNativeCombatReport(NativeCombatSystemImage& image,NativeCombatGameEventHandler& stats,
    NativeCombatMonsterManager& monsters,INativeSkillSystemServices& skills,NativeInventorySession& inventory,const NativeGameDatabase& database,
    const std::function<void(const NativeCombatReport&)>& check_rewards={}){
    const auto check=[](bool ok,const char* why){if(!ok)throw std::runtime_error(why);};
    const auto word=[](std::uint32_t a)->std::uint32_t&{return *reinterpret_cast<std::uint32_t*>(a);};
    const auto mode=image.Mode();const auto stats_slot=image.GameEventHandlerSlot();const auto saved_rng=skills.Random();
    std::vector<std::pair<std::uint32_t,std::uint32_t>> fields;
    struct Restore{NativeCombatSystemImage& image;INativeSkillSystemServices& skills;RandomNumberGenerator rng;
        std::uint8_t mode;std::uint32_t slot;std::vector<std::pair<std::uint32_t,std::uint32_t>>& fields;
        ~Restore(){for(auto [a,v]:fields)*reinterpret_cast<std::uint32_t*>(a)=v;image.Mode()=mode;image.GameEventHandlerSlot()=slot;skills.Random()=rng;}
    } restore{image,skills,saved_rng,mode,stats_slot,fields};
    check(monsters.Count()==3u && !monsters.StageCount(),"report probe consumes actual retired three-Monster owners");
    std::uint32_t item{};
    const auto table=database.Tables().find("Property");check(table!=database.Tables().end(),"report fixture uses original Property table");
    for(const auto& row:table->second.rows)if(!row.empty())if(const auto* id=std::get_if<std::uint32_t>(&row[0]);id && *id && inventory.PropertyAddress(*id)){item=*id;break;}
    check(item!=0u,"report fixture borrows a real original Property ID");
    std::uint32_t xp{};
    for(unsigned i=0;i<3u;++i){const auto a=monsters.ActorAt(i);
        for(auto o:{564u,828u,916u,920u,928u,932u,936u,940u,944u,948u,952u,956u})fields.emplace_back(a+o,word(a+o));
        if(check_rewards)word(a+564u)=20000u;
        word(a+828u)=i==2u?2u:(i==0u?4u:3u);word(a+916u)=word(a+920u)=3u+i;
        for(auto o:{928u,932u,936u,940u,944u,948u,952u,956u})word(a+o)=0u;
        word(a+928u)=item;word(a+944u)=std::bit_cast<std::uint32_t>(1.f);
        if(i==0u){word(a+932u)=item;word(a+948u)=std::bit_cast<std::uint32_t>(1.f);}
        if(i==1u)word(a+944u)=std::bit_cast<std::uint32_t>(std::numeric_limits<float>::quiet_NaN());
        if(i!=2u)xp+=std::bit_cast<std::uint32_t>(monsters.Skills(a).CurrentStat(5u));
    }
    image.Mode()=1u;skills.Random().Seed(0x571240u);const auto initial_rng=skills.Random();
    NativeCombatReportPreparer builder(image,stats,monsters,skills);
    image.GameEventHandlerSlot()=0u;bool foreign{};try{(void)builder.Prepare();}catch(const std::logic_error&){foreign=true;}
    image.GameEventHandlerSlot()=stats_slot;check(foreign && skills.Random().Index()==initial_rng.Index(),"report validates published stats before consuming RNG");
    auto report=builder.Prepare();
    check(report->Word(0u)==7u && report->Word(4u)==stats.Total() && report->Word(8u)==4u && report->Word(12u)==xp && report->Word(16u)==3u,
        "real report includes only states3/4, sums equal-bound money/actual actor52(5), original score and Quick rank4");
    const std::vector<std::pair<std::uint32_t,std::uint32_t>> expected{{item,3u}};
    check(report->Items()==expected && report->Items(true).empty() && report->Word(32u)==1u && !report->Word(48u),
        "real drop map merges original ID, includes x87 unordered NaN probability, skips inactive actor and empty slots");
    auto expected_rng=initial_rng;for(unsigned i=0;i<5u;++i)(void)expected_rng.Next();
    check(skills.Random().Index()==expected_rng.Index(),"report consumes exact five original money/drop rolls");
    for(unsigned i=0;i<RandomNumberGenerator::kStateCount;++i)check(skills.Random().StateAt(i)==expected_rng.StateAt(i),"report retains actual legacy RNG state");
    for(unsigned i=0;i<4u;++i)check(report->Learned(i).empty() && !report->Byte(52u+i*40u+36u),"report preserves four original inactive player records");
    if(check_rewards){
        check_rewards(*report);
        const auto score=word(stats.Address()+136u);
        fields.emplace_back(stats.Address()+136u,score);
        std::uint32_t equipment{};
        for(const auto& row:database.Tables().at("Equip").rows)if(!row.empty())if(const auto* id=std::get_if<std::uint32_t>(&row[0]);id &&
            inventory.EquipmentDefinitionAddress(*id) && !inventory.PropertyAddress(*id)){equipment=*id;break;}
        check(equipment!=0u,"reward fixture includes original equipment drop ID");
        word(monsters.ActorAt(0u)+936u)=word(monsters.ActorAt(0u)+940u)=equipment;
        word(monsters.ActorAt(0u)+952u)=word(monsters.ActorAt(0u)+956u)=std::bit_cast<std::uint32_t>(1.f);
        image.Mode()=0u;
        for(unsigned rank=0;rank<4u;++rank){
            const auto target=rank?word(stats.Address()+604u+(rank-1u)*4u):word(stats.Address()+604u)+1u;
            word(stats.Address()+136u)=word(stats.Address()+136u)+target-stats.Total();
            auto ranked=builder.Prepare();check(ranked->Word(8u)==rank,"controlled score selects actual original rank threshold");
            check_rewards(*ranked);
        }
        word(stats.Address()+136u)=score;image.Mode()=1u;
        word(monsters.ActorAt(0u)+936u)=word(monsters.ActorAt(0u)+940u)=0u;
        word(monsters.ActorAt(0u)+952u)=word(monsters.ActorAt(0u)+956u)=0u;
    }
    auto clone=report->Clone();
    check(clone->Address()!=report->Address() && clone->Word(24u)!=report->Word(24u) && clone->Items()==expected,"native report copy has independent image and drop owner");
    report.reset();
    word(monsters.ActorAt(0u)+828u)=2u;image.Mode()=0u;skills.Random().Seed(0x571240u);
    auto next=builder.Prepare();
    check(next->Word(0u)==4u && next->Items()==std::vector<std::pair<std::uint32_t,std::uint32_t>>{{item,1u}} &&
        next->Word(8u)==stats.Rank(std::bit_cast<std::int32_t>(stats.Total())) && clone->Word(0u)==7u && clone->Items()==expected,
        "copy survives source destruction/new actor-filtered report and normal rank uses actual stats thresholds");
    check(!monsters.ActiveIterators(),"native report preparation releases first-roster cursors");
    builder.Close();bool closed{};try{(void)builder.Prepare();}catch(const std::logic_error&){closed=true;}
    check(closed && clone->Items()==expected,"completed report owns its data beyond builder retirement");
    std::cout<<"Native571240 report: actual416 stats/first Monster owners/RNG/actor52(5), state3/4 filtering, money7, original item merged3 including NaN, exact5 rolls, rank4/normal, four records and deep-copy/retirement pass\n";
}
}
