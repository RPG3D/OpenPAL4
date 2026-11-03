#pragma once
#include "pal4/native_combat_actor_exit_events.h"
#include "pal4/native_combat_actor_entry.h"
#include "pal4/native_combat_actor_retirement.h"
#include "pal4/native_combat_player_manager.h"
#include "pal4/native_combat_player_model_cache.h"
#include "pal4/native_combat_player_actor_owner.h"
#include "pal4/native_combat_player_motion.h"
#include "pal4/native_combat_actor_skill_runtime.h"
#include "pal4/native_combat_actor_turn_runtime.h"
#include "pal4/native_combat_world_resources.h"
#include "pal4/native_combat_system_manager.h"
#include "pal4/native_combat_system_services.h"
#include "pal4/native_combat_formula_catalog.h"
#include "pal4/native_combat_skill_actor_registry.h"
#include "pal4/native_inventory_session.h"
#include "pal4/native_actor_instance_owner.h"
#include "pal4/native_actor_instance_pool.h"
#include "pal4/native_animated_actor_asset.h"
#include "pal4/native_pk_file_manager_runtime.h"
#include "pal4/native_combat_roster_targets.h"
#include "pal4/native_combat_monster_manager.h"
#include "pal4/native_combat_event_handlers.h"
#include "pal4/native_combat_system_image.h"
#include "pal4/native_skill_record_catalog.h"
#include "pal4/native_skill_system_services.h"
#include "pal4/native_monster_record_catalog.h"
#include "pal4/native_player_collection_values.h"
#include "pal4/native_effect_scene_session.h"
#include "pal4/native_script_event_router.h"
#include "pal4/random_number_generator.h"
#include <rwcore.h>
#include <rpworld.h>
#include <algorithm>
#include <array>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace pal4::test {
inline void VerifyNativeCombatPlayerReplacement(const std::filesystem::path& root,
    NativePlayerManagerSession& source,const NativeGameDatabase& database,
    INativeSkillSystemServices& system,NativeEffectSceneSession& effects,NativePkFileManagerRuntime& packages,
    NativePalGameIVTextureStorage& game_storage){
    const auto check=[](bool ok,const char* text){if(!ok)throw std::runtime_error(text);};
    const auto reject=[&](auto&& f){bool denied{};try{f();}catch(const std::logic_error&){denied=true;}check(denied,"Player replacement scoped guard");};
    const auto word=[](std::uint32_t a){return *reinterpret_cast<const std::uint32_t*>(a);};
    NativeCombatSystemImage image;NativeScriptEventRouter router;
    NativeCombatWorldResources world(packages,game_storage,image.world);world.Load(root,"CombatWorld","M01");
    std::array<std::uint32_t,100> effect_manager{};effect_manager[96]=effect_manager[98]=0xffffffffu;
    effects.BindCombatEffects(reinterpret_cast<std::uint32_t>(effect_manager.data()),reinterpret_cast<std::uint32_t>(&image),router);
    struct Effects{NativeEffectSceneSession& e;~Effects(){try{e.ClearCombatEffects();e.UnbindCombatEffects();}catch(...){}}} effects_guard{effects};
    NativeInventorySession inventory;inventory.BindDatabase(database);(void)inventory.Start();
    NativeSkillRecordCatalog records(database);NativeMonsterRecordCatalog monster_records(database);NativePlayerCollectionCatalog passive(database);
    NativeCombatSkillActorRegistry registry(router,records,system);NativeCombatPlayerManager players(source,router,system);
    std::array<std::uint32_t,128> game{};
    NativeActorInstancePool pool([]{return false;},[&](std::string_view name){
        auto child=std::make_unique<NativeActorInstanceOwner>();child->Construct(reinterpret_cast<std::uint32_t>(game.data()));
        child->Load(root,name);child->BindHandResources(root,packages,effects,system,[]{return false;});return child;
    });
    NativeCombatMonsterManager monsters(pool,monster_records,records,router,system,registry);
    std::array<std::uint32_t,4> actors{};
    std::vector<std::unique_ptr<NativeCombatPlayerModelCache>> caches;
    std::vector<std::unique_ptr<NativeCombatActorSkillRuntime>> skills;
    std::vector<std::unique_ptr<NativeCombatActorTurnRuntime>> turns;
    for(unsigned n=0;n<4u;++n){
        actors[n]=players.AddPlayer(std::array{0u,1u,3u,2u}[n]);
        caches.push_back(std::make_unique<NativeCombatPlayerModelCache>(players.Actor(actors[n]),pool,inventory,120300));
        skills.push_back(std::make_unique<NativeCombatActorSkillRuntime>(players.Actor(actors[n]),registry,inventory));
        turns.push_back(std::make_unique<NativeCombatActorTurnRuntime>(*caches[n],image,[&]{return world.GeometryQueue();}));
    }
    const auto index=[&](std::uint32_t a){for(unsigned n=0;n<4u;++n)if(actors[n]==a)return n;throw std::invalid_argument("Player replacement fixture actor");};
    NativeCombatFormulaCatalog formulas(database);
    std::vector<NativeCombatActorSkillRuntime*> scheduling_skills;
    for(auto& skill:skills){skill->BindActionProgress(formulas);scheduling_skills.push_back(skill.get());}
    const auto enemy=monsters.CreateMonster(2579u,6u);monsters.Skills(enemy).BindActionProgress(formulas);
    NativeCombatSystemServices scheduling(image,players,monsters,scheduling_skills,system,formulas,
        [&](std::uint32_t a)->INativeCombatPlayerTurnServices&{return *turns[index(a)];},[]{});
    NativeCombatSystemManager scheduler(router,scheduling,image.SystemManagerSlot());
    const auto subscription_count=router.SubscriptionCount();players.Subscribe();
    check(router.SubscriptionCount()==subscription_count,"stage subscription rebind preserves original three handlers");
    const auto pinned=players.CreateFirstIterator();reject([&]{players.Subscribe();});players.DeleteFirstIterator(pinned);
    NativeCombatPlayerMotion motion(players,system,router,[&](std::uint32_t a)->NativeCombatPlayerMotionBinding{const auto n=index(a);return {*caches[n],*skills[n]};});
    NativeCombatRosterTargets targets(reinterpret_cast<std::uint32_t>(&image),players,monsters,effects,router);
    for(unsigned n=0;n<4u;++n)targets.RegisterPlayer(*caches[n],*skills[n]);
    monsters.InitializeMonster(enemy,0u,passive,effects);
    for(unsigned n=0;n<3u;++n)InitializeNativeCombatActor(players.Actor(actors[n]),*caches[n],*skills[n],passive,effects,router,motion,n);
    players.Actor(actors[3]).SetSlot(3u);skills[3]->SetFacetState(0u,false);
    check(players.StageCount()==3u && !word(actors[3]+808u) && !word(actors[3]+1296u),"fourth Player is a genuine unmodeled reserve");
    reject([&]{players.Subscribe();});
    image.State()=3u;
    std::unique_ptr<NativeCombatActorExitEvents> exits;
    NativeCombatEventHandlers handlers(router,image.ActorHandlersSlot(),image.ExtendedHandlersSlot(),
        [&](std::uint32_t owner,std::uint32_t entry,std::uint32_t payload){if(entry==0x57b220u){check(exits!=nullptr,"Player death consumer exists");exits->Handle(owner,payload);}});
    exits=std::make_unique<NativeCombatActorExitEvents>(image,handlers,registry,players,monsters,targets,effects,passive,router,
        [](std::uint32_t){throw std::logic_error("Player death does not record Monster progress");},NativeCombatPlayerExitServices{
            [&](std::uint32_t a)->NativeCombatPlayerModelCache&{return *caches[index(a)];},&motion});
    std::vector<std::uint32_t> tokens;std::vector<std::string> trace;unsigned outcomes{};bool fail_leave{};
    tokens.push_back(router.Subscribe(router.Key("combat_over"),1u,[&](std::uint32_t){++outcomes;}));
    tokens.push_back(router.Subscribe(router.Key("re_leave_stage"),1u,[&](std::uint32_t p){
        check(word(p)==actors[0]+716u && skills[2]->DeathDispatchActive() && !skills[0]->ExitDispatchActive() && skills[0]->ReplacementRetirementActive(),
              "last Player544 retires a different dead slot0 Player under exact permission");
        reject([&]{players.AddPlayer(0u);});reject([&]{exits->Close();});reject([&]{caches[0]->Close();});
        reject([&]{(void)caches[0]->LeaveStageDuringSkill(router,*skills[0]);});
        trace.push_back("leave_stage");if(fail_leave)throw std::runtime_error("intentional Player replacement leave failure");
    }));
    tokens.push_back(router.Subscribe(router.Key("re_leave_scene"),1u,[&](std::uint32_t p){if(word(p)==actors[0]+716u)trace.push_back("leave_scene");}));
    tokens.push_back(router.Subscribe(router.Key("re_join_stage"),1u,[&](std::uint32_t p){if(word(p)==actors[3]+716u){
        check(word(actors[3]+808u)!=0u && skills[2]->DeathDispatchActive(),"reserve lazily loads a real model before stage join");
        reject([&]{players.ClearActors([](auto){},[](auto){});});trace.push_back("join_stage");
    }}));
    tokens.push_back(router.Subscribe(router.Key("re_join_scene"),1u,[&](std::uint32_t p){if(word(p)==actors[3]+716u)trace.push_back("join_scene");}));
    const auto close=[&]{
        for(auto t:tokens)(void)router.Unsubscribe(t);tokens.clear();exits->Close();handlers.Close();targets.Close();scheduler.Close();
        players.ClearActors([&](std::uint32_t a){const auto n=index(a);(void)RetireNativeCombatActor(players.Actor(a),effects.ContentHandles(),[&]{(void)caches[n]->LeaveStage(router);});},
            [&](std::uint32_t a){const auto n=index(a);caches[n]->Close();skills[n]->Close();});
        monsters.ClearActors(effects);monsters.Close();players.Close();pool.Clear();effects.ClearCombatEffects();effects.UnbindCombatEffects();
    };
    try{
        const auto before=system.Random();
        for(unsigned n=0;n<2u;++n){(void)skills[n]->SetCurrentStat(7u,0);skills[n]->BecomeDead(actors[n],passive,turns[n].get());}
        check(players.StageCount()==3u && players.Actor(actors[0]).Slot()==0u && !word(actors[3]+808u),"earlier deaths keep stage while another Player is ready");
        (void)skills[2]->SetCurrentStat(7u,0);
        skills[2]->SetFacetState(3u,false);monsters.Skills(enemy).SetActionProgress(64.0f);
        (void)scheduler.Update(0.0f);check(scheduler.Active(),"actual pending last Player death pauses scheduling");
        const auto cursor=players.CreateStageIterator();reject([&]{skills[2]->BecomeDead(actors[2],passive,turns[2].get());});players.DeleteStageIterator(cursor);
        check(!skills[0]->ReplacementRetirementActive() && !skills[2]->DeathDispatchActive() && !players.ActiveIterators() && !targets.ActiveIterators(),
              "non-exhausted external stage cursor rejects and unwinds actual544");
        fail_leave=true;bool failed{};
        try{skills[2]->BecomeDead(actors[2],passive,turns[2].get());}catch(const std::runtime_error& e){failed=std::string_view(e.what())=="intentional Player replacement leave failure";}
        check(failed && !players.ActiveIterators() && !targets.ActiveIterators() && !skills[0]->ReplacementRetirementActive() && players.Actor(actors[0]).Slot()==0u,
              "leave failure releases exact source permission and both cursor families");
        fail_leave=false;trace.clear();skills[2]->BecomeDead(actors[2],passive,turns[2].get());
        check(players.Count()==4u && registry.Count()==5u && players.StageCount()==3u && players.Actor(actors[0]).Slot()==6u && !word(actors[0]+808u),
              "replacement retires payload and stage without destroying first owners");
        check(players.Actor(actors[3]).Slot()==0u && word(actors[3]+828u)==1u && skills[3]->CurrentStat(7u)>0 &&
              caches[3]->Current().Asset() && word(actors[3]+808u)==caches[3]->Current().Address() && caches[3]->Count()==1u,
              "unmodeled fourth Player now owns real cache, model, living stats and slot0");
        check(trace==std::vector<std::string>{"leave_stage","leave_scene","join_stage","join_scene"} && !outcomes &&
              !players.ActiveIterators() && !targets.ActiveIterators() && !skills[2]->DeathDispatchActive(),
              "Player replacement preserves original leave/join order and prevents premature defeat");
        check(scheduler.AllObjectsReady() && !scheduler.Active() && scheduler.Objects().size()==2u,
              "reserve stage entry must resume the actual paused SystemManager without a synthetic refresh");
        auto actual=system.Random(),reference=before;
        for(unsigned n=0;n<624u;++n)check(actual.Next()==reference.Next(),"Player death and reserve entry consume no MT");
        close();check(!registry.Count() && !pool.CachedCount(),"Player replacement resource ownership balances");
        std::cout<<"Native Player replacement: actual4 roles/3 slots, different544 source and retired slot0, lazy reserve model, exhausted cursor mutation, leave failure/retry, exact events, actual SystemManager pause/resume, no MT and cleanup pass (not full renderer/battle verification)\n";
    }catch(...){try{close();}catch(...){}throw;}
}
}
