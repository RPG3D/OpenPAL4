#include "native_combat_rewards_probe.h"
#include "native_combat_ending_presentation_probe.h"
#include "native_combat_ending_tasks_probe.h"
#include "native_combat_ending_sequence_probe.h"
#include "pal4/native_combat_ending_window.h"
#include "pal4/native_combat_rewards.h"
#include "pal4/native_combat_report.h"
#include "pal4/native_combat_player_manager.h"
#include "pal4/native_combat_player_actor_owner.h"
#include "pal4/native_combat_actor_skill_runtime.h"
#include "pal4/native_combat_skill_actor_registry.h"
#include "pal4/native_combat_event_handlers.h"
#include "pal4/native_combat_system_image.h"
#include "pal4/native_player_event_runtime.h"
#include "pal4/native_upgrade_record_catalog.h"
#include "pal4/native_dialog_settings_snapshot.h"
#include "pal4/native_inventory_session.h"
#include "pal4/native_pal_game_iv_texture_storage.h"
#include "pal4/native_game_database.h"
#include "pal4/native_combat_formula_catalog.h"
#include "pal4/native_skill_system_services.h"
#include "pal4/native_script_event_router.h"
#include "pal4/random_number_generator.h"
#include <algorithm>
#include <array>
#include <bit>
#include <iostream>
#include <map>
#include <stdexcept>
namespace pal4::test {
void VerifyNativeCombatRewards(const NativeCombatReport& source,NativeCombatSystemImage& image,
    NativePalGameIVTextureStorage& game,NativeCombatEventHandlers& handlers,NativePlayerManagerSession& scene,
    NativeSkillRecordCatalog& catalog,INativeSkillSystemServices& system,NativePlayerEventRuntime& growth,
    const NativeUpgradeRecordCatalog& upgrades,const NativeCombatFormulaCatalog& formulas,const NativeGameDatabase& db,NativeCombatEndingWindow& ending,NativeMilesSound2dRuntime& sounds,NativeCombatConsoleVisuals& visuals,NativeCombatConsole& console) {
    const auto check=[](bool ok,const char* why){if(!ok)throw std::runtime_error(why);};
    const auto word=[](std::uint32_t a)->std::uint32_t&{return *reinterpret_cast<std::uint32_t*>(a);};
    const auto rank=source.Word(8u);check(rank<5u,"reward fixture original rank range");
    const auto before_counter=game.GameDataEntry(rank+2u),before_ready=handlers.ReadyActor();const auto before_rng=system.Random();
    struct Restore{NativePalGameIVTextureStorage& game;NativeCombatEventHandlers& handlers;INativeSkillSystemServices& system;
        std::uint32_t rank,counter,ready;RandomNumberGenerator rng;
        ~Restore(){game.SetGameDataEntry(rank+2u,counter);handlers.SetReadyActor(ready);system.Random()=rng;}
    } restore{game,handlers,system,rank,before_counter,before_ready,before_rng};
    NativeDialogSettingsSnapshot progress;progress.Reset(progress.State());
    progress.ClearTree(progress.State(),148u);progress.ClearTree(progress.State(),164u);
    NativeInventorySession inventory;(void)inventory.Start();inventory.BindDatabase(db);inventory.Reset();
    NativeCombatRewards* active{};bool fail=rank==0u;unsigned guards{},equipment_events{};
    inventory.BindItemObserver([&](std::uint32_t id){
        progress.InsertTreeValue(progress.State(),148u,id);
        if(active){bool close{},nested{};try{active->Close();}catch(const std::logic_error&){close=true;}
            try{(void)active->Process({});}catch(const std::logic_error&){nested=true;}
            check(close && nested,"actual inventory observer preserves reward reentry/Close guards");++guards;}
        if(fail)throw std::runtime_error("controlled reward inventory observer");
    });
    inventory.BindEquipmentObserver([&](std::uint32_t id){progress.RecordEquipmentProgress(std::bit_cast<std::int32_t>(id));++equipment_events;});
    NativeScriptEventRouter router;NativeCombatPlayerManager players(scene,router,system);
    NativeCombatSkillActorRegistry registry(router,catalog,system);
    std::vector<std::unique_ptr<NativeCombatActorSkillRuntime>> actors;
    players.BindActorPreparation([&](std::uint32_t a){actors.push_back(std::make_unique<NativeCombatActorSkillRuntime>(players.Actor(a),registry,inventory));});
    for(unsigned role=0;role<2u;++role){const auto a=players.AddPlayer(role,role?6u:1u);
        // Unequipped diagnostic copies use this independent inventory. The
        // actual in-progress party and its equipped instances stay untouched.
        for(unsigned n=0;n<6u;++n)word(a+896u+n*4u)=0xffffffffu;
        (void)actors.back()->SetCurrentStatViaActor(4u,1);(void)actors.back()->SetCurrentStatViaActor(5u,0);
        actors.back()->SetIndexedPlayerStat(5u,0);(void)actors.back()->SetCurrentStatViaActor(7u,1);
        actors.back()->SetFacetState(role?6u:1u,false);
    }
    std::array<std::vector<std::uint32_t>,2> prior_learned;
    for(unsigned role=0;role<2u;++role)for(auto p=word(players.ActorAt(role)+1084u);p!=word(players.ActorAt(role)+1088u);p+=4u)prior_learned[role].push_back(word(p));
    handlers.SetReadyActor(players.ActorAt(1u));
    NativeCombatRewards rewards(image,game,handlers,players,inventory,growth,formulas,db,system,
        [&](std::uint32_t a)->NativeCombatActorSkillRuntime&{for(auto& p:actors)if(p->ActorAddress()==a)return *p;throw std::logic_error("reward probe foreign Player");});
    active=&rewards;
    const auto slot=image.ExtendedHandlersSlot();image.ExtendedHandlersSlot()=0u;bool unpublished{};
    try{(void)rewards.Process(source.Clone());}catch(const std::logic_error&){unpublished=true;}
    image.ExtendedHandlersSlot()=slot;
    check(unpublished && game.GameDataEntry(rank+2u)==before_counter && system.Random().Index()==before_rng.Index(),
        "reward publication validation precedes RNG and real grants");
    if(fail){bool threw{};try{(void)rewards.Process(source.Clone());}catch(const std::runtime_error& e){threw=std::string_view(e.what())=="controlled reward inventory observer";}
        check(threw && !rewards.PendingReports() && !players.ActiveIterators() && !rewards.ProcessedReports() && inventory.ReadDword(inventory.Address()+308u)>0u,
            "inventory exception retains original partial grants but releases all report/cursor ownership");
        fail=false;inventory.Reset();progress.ClearTree(progress.State(),148u);progress.ClearTree(progress.State(),164u);equipment_events=0u;
        game.SetGameDataEntry(rank+2u,before_counter);system.Random()=before_rng;
    }
    if(rank==1u){
        const auto token=router.Subscribe(router.Key("re_attr_int_ext_changed"),3u,[&](std::uint32_t p){
            if(word(p+4u)==5u)throw std::runtime_error("controlled reward XP observer");
        });
        bool threw{};try{(void)rewards.Process(source.Clone());}catch(const std::runtime_error& e){threw=std::string_view(e.what())=="controlled reward XP observer";}
        (void)router.Unsubscribe(token);
        check(threw && !players.ActiveIterators() && !rewards.PendingReports() && !rewards.ProcessedReports() && actors[0]->CurrentStat(5u)>0,
            "actual XP observer failure releases first-roster cursor/reports and retains original partial Player changes");
        // Reinitialize only this independent fixture before retry; production
        // rewards do not roll back inventory, counters, RNG or Player mutations.
        for(auto& p:actors){(void)p->SetCurrentStatViaActor(5u,0);p->SetIndexedPlayerStat(5u,0);}
        inventory.Reset();progress.ClearTree(progress.State(),148u);progress.ClearTree(progress.State(),164u);equipment_events=0u;
        game.SetGameDataEntry(rank+2u,before_counter);system.Random()=before_rng;
    }
    const auto& property=db.Tables().at("Property");
    const auto column=[&](const NativeGameDbTable& t,std::string_view name){auto it=std::find_if(t.fields.begin(),t.fields.end(),[&](const auto& f){return f.name==name;});check(it!=t.fields.end(),"reward expected original DB column");return static_cast<std::size_t>(it-t.fields.begin());};
    const auto id_column=column(property,"ID"),prize_column=column(property,"PRIZE_LEVEL");
    constexpr std::array<std::array<unsigned,6>,4> types{{{{4,3,2,1,5,6}},{{3,2,1,6,0,0}},{{2,1,6,0,0,0}},{{1,0,0,0,0,0}}}};
    constexpr std::array<unsigned,5> counts{6,4,3,1,0};
    auto expected_rng=before_rng;std::map<std::uint32_t,std::uint32_t> bonus;
    for(unsigned n=0;n<counts[rank];++n){std::vector<std::uint32_t> pool;
        for(const auto& row:property.rows)if(std::get<std::uint32_t>(row[prize_column])==types[rank][n])pool.push_back(std::get<std::uint32_t>(row[id_column]));
        check(!pool.empty(),"original rank reward pool exists");
        const auto selected=static_cast<std::size_t>(double(expected_rng.Next()%1000u)*double(0.001f)*double(float(pool.size()-1u)));
        ++bonus[pool.at(selected)];
    }
    const auto multiplier=rank==0u?1.9f:rank==1u?1.6f:rank==2u?1.3f:1.f;
    const auto money=static_cast<std::uint32_t>(double(source.Word(0u))*double(multiplier));
    const auto xp=static_cast<std::int32_t>(double(source.Word(12u))*double(multiplier));
    auto output=rewards.Process(source.Clone());
    std::cout<<"Reward rank="<<rank<<" money="<<output->Word(0u)<<'/'<<money<<" inventory="<<inventory.ReadDword(inventory.Address()+308u)
        <<" xp="<<output->Word(12u)<<'/'<<xp<<" counter="<<game.GameDataEntry(rank+2u)<<'/'<<before_counter+1u
        <<" guards="<<guards<<" processed="<<rewards.ProcessedReports()<<" pending="<<rewards.PendingReports()<<" cursors="<<players.ActiveIterators()<<'\n';
    check(output->Word(0u)==money && output->Word(12u)==static_cast<std::uint32_t>(xp) && output->Word(8u)==rank &&
        output->Word(4u)==source.Word(4u) && output->Word(16u)==3u && inventory.ReadDword(inventory.Address()+308u)==money &&
        game.GameDataEntry(rank+2u)==before_counter+1u && guards && rewards.ProcessedReports()==1u && !rewards.PendingReports() && !players.ActiveIterators(),
        "native5715F0 applies original rank money/XP/counter and drains actual owners");
    check(output->Items()==source.Items() && output->Items(true)==std::vector<std::pair<std::uint32_t,std::uint32_t>>(bonus.begin(),bonus.end()),
        "native bonuses use original Property row order, PRIZE_LEVEL and exact legacy rolls");
    for(unsigned n=0;n<RandomNumberGenerator::kStateCount;++n)check(system.Random().StateAt(n)==expected_rng.StateAt(n),"reward RNG state words match exact draws");
    check(system.Random().Index()==expected_rng.Index(),"reward RNG draw index");
    std::map<std::uint32_t,std::uint32_t> grants=bonus;for(auto [id,count]:source.Items())grants[id]+=count;
    unsigned equipment_count{},item_kinds{};
    for(auto [id,count]:grants)if(inventory.PropertyAddress(id)){++item_kinds;check(inventory.ItemCount(id)==count,"actual item quantity includes merged drop and rank bonus");}
        else if(inventory.EquipmentDefinitionAddress(id)){equipment_count+=count;check(inventory.HasEquipment(id) && progress.HasEquipmentProgress(std::bit_cast<std::int32_t>(id)),"actual equipment instances and original discovery tree164");}
    unsigned instances{};for(unsigned kind=0;kind<7u;++kind)instances+=inventory.EquipmentCount(kind);
    check(equipment_events==equipment_count && instances==equipment_count && progress.State().Word(160u)==item_kinds &&
        progress.State().Word(176u)==(equipment_count?1u:0u),"actual inventory observers update unique original discovery sets and retain every equipment instance");
    const auto& stunts=db.Tables().at("Stunt");const auto stunt_id=column(stunts,"ID");
    constexpr std::array<std::string_view,2> learn_fields{"ePT_TIANHE_LV_LMT","ePT_LINSHA_LV_LMT"};
    std::size_t learned_count{};
    for(unsigned role=0;role<2u;++role){float gained=float(xp);if(role)gained=float(double(formulas.FloatAt(30))*gained);
        if(image.Mode())gained=float(double(formulas.FloatAt(31))*gained);
        if(role)gained=float((double(formulas.FloatAt(29))+1.0)*gained);
        const auto gain=static_cast<std::int32_t>(gained);const auto a=players.ActorAt(role);std::int32_t level=100;
        for(std::size_t n=0;n<upgrades.Count();++n){const auto& row=upgrades.Record(n);if(row[37u]==word(a+1196u) && std::bit_cast<std::int32_t>(row[39u])>gain && std::bit_cast<std::int32_t>(row[38u])<level)level=std::bit_cast<std::int32_t>(row[38u]);}
        check(level>1 && level<100,"original controlled XP crosses real level thresholds");
        const auto r=52u+role*40u;
        std::cout<<"Reward Player="<<role<<" active="<<unsigned(output->Byte(r+36u))<<" gain="<<output->Word(r+16u)<<'/'<<gain
            <<" oldlevel="<<output->Word(r+20u)<<" level="<<output->Word(r+24u)<<'/'<<level<<" xp="<<output->Word(r+28u)<<"->"<<output->Word(r+32u)
            <<" actual="<<actors[role]->CurrentStat(4u)<<','<<actors[role]->CurrentStat(5u)<<','<<actors[role]->IndexedPlayerStat(5u)<<'\n';
        check(output->Byte(r+36u) && output->Word(r+16u)==static_cast<std::uint32_t>(gain) && output->Word(r+20u)==1u && output->Word(r+24u)==static_cast<std::uint32_t>(level) &&
            output->Word(r+28u)==0u && output->Word(r+32u)==static_cast<std::uint32_t>(gain) && actors[role]->CurrentStat(4u)==level &&
            actors[role]->CurrentStat(5u)==gain && actors[role]->IndexedPlayerStat(5u)==gain,
            "actual Player readiness/mode/focus factors, before/gain/after XP, original UpgradeData level and base/effective writeback");
        std::vector<std::uint32_t> learned;const auto requirement=column(stunts,learn_fields[role]);
        for(const auto& row:stunts.rows){const auto need=std::bit_cast<std::int32_t>(std::get<std::uint32_t>(row[requirement]));const auto id=std::get<std::uint32_t>(row[stunt_id]);
            if(need>0 && need<=level && std::find(prior_learned[role].begin(),prior_learned[role].end(),id)==prior_learned[role].end())learned.push_back(id);}
        check(std::vector<std::uint32_t>(output->Learned(role).begin(),output->Learned(role).end())==learned,"actual level-up learned IDs copied into owned report");
        learned_count+=learned.size();
    }
    check(learned_count && !output->Byte(52u+80u+36u) && !output->Byte(52u+120u+36u),"populated learning and two absent Player records");
    // Each diagnostic report follows the original show -> populate -> task lifecycle.
    // Retire the previous active handler before re-showing the same cached root.
    ending.SetVisible(false);ending.SetVisible(true);
    VerifyNativeCombatEndingPresentation(ending,*output,players,inventory,db,
        [&](std::uint32_t a)->NativeCombatActorSkillRuntime&{for(auto& p:actors)if(p->ActorAddress()==a)return *p;throw std::logic_error("ending probe foreign Player");});
    VerifyNativeCombatEndingTasks(ending,players,sounds,image,upgrades,
        [&](std::uint32_t a)->NativeCombatActorSkillRuntime&{for(auto& p:actors)if(p->ActorAddress()==a)return *p;throw std::logic_error("ending task probe foreign Player");},visuals,console);
    VerifyNativeCombatEndingSequence(ending,players,sounds,image,upgrades,
        [&](std::uint32_t a)->NativeCombatActorSkillRuntime&{for(auto& p:actors)if(p->ActorAddress()==a)return *p;throw std::logic_error("ending sequence probe foreign Player");},visuals,console,system);
    auto clone=output->Clone();output.reset();rewards.Close();active=nullptr;handlers.SetReadyActor(before_ready);
    players.ClearActors([](std::uint32_t){},[&](std::uint32_t a){for(auto& p:actors)if(p->ActorAddress()==a)p->Close();});
    actors.clear();players.Close();inventory.Close();
    check(!registry.Count() && !inventory.OutstandingAllocations() && clone->Word(0u)==money && !clone->Learned(0u).empty(),
        "processed report clone survives input/output/services and all Player/inventory resources retire");
    check(ending.Report().Items()==clone->Items() && !ending.Report().Learned(0u).empty(),
        "ending report remains deep owned after reward source, Player/inventory and presenter retirement");
    std::cout<<"Native5715F0 rewards rank="<<rank<<": actual money/items/equipment-discovery, original bonus RNG, two Player XP/level/growth/learning, deep report and exception/guard/teardown pass\n";
}
}
