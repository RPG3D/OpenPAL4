#pragma once
#include "pal4/native_combat_actor_skill_runtime.h"
#include "pal4/native_combat_actor_turn_services.h"
#include "pal4/native_combat_actor_death_services.h"
#include "pal4/native_combat_death_actions.h"
#include "pal4/native_combat_task_queue.h"
#include "pal4/native_combat_player_actor_owner.h"
#include "pal4/native_combat_skill_actor_registry.h"
#include "pal4/native_player_collection_values.h"
#include "pal4/native_inventory_session.h"
#include "pal4/native_skill_system_services.h"
#include "pal4/native_script_event_router.h"
#include "pal4/native_game_database.h"
#include <cstring>
#include <functional>
#include <iostream>
namespace pal4::test {
inline void VerifyNativeCombatPlayerTurns(NativePlayerManagerSession& source,NativeInventorySession& inventory,
    NativeSkillRecordCatalog& catalog,const NativePlayerCollectionCatalog& collections,
    INativeSkillSystemServices& system,const NativeGameDatabase& database){
    const auto check=[](bool v,const char* m){if(!v)throw std::runtime_error(m);};
    const auto reject=[&](auto&& f){bool denied=false;try{f();}catch(const std::logic_error&){denied=true;}check(denied,"Player turn guard");};
    const auto word=[](std::uint32_t a)->std::uint32_t&{return *reinterpret_cast<std::uint32_t*>(a);};
    NativeScriptEventRouter router;NativeCombatSkillActorRegistry registry(router,catalog,system);
    for(unsigned role=0;role<4u;++role){
        NativeCombatPlayerActorOwner actor;const auto a=actor.Construct(source,role);
        const auto allocations=actor.OwnedAllocations();
        NativeCombatActorSkillRuntime skills(actor,registry,inventory);(void)skills.RebuildStats();
        class Battle final : public INativeCombatPlayerTurnServices {
        public:
            NativeCombatDeathActions* deaths{};NativeCombatTaskQueue* queue{};
            bool inactive{},fail_mode{};unsigned mode_queries{},created{},registered{};
            bool CombatInactive() override{++mode_queries;if(fail_mode)throw std::logic_error("intentional Player mode query failure");return inactive;}
            std::uint32_t CreateDeathAction(std::uint32_t a) override{++created;return deaths->Create(a,a);}
            void RegisterActorAction(std::uint32_t a) override{++registered;(void)queue->Register(a);}
            void ApplyPendingSceneRecord(std::uint32_t) override{throw std::logic_error("Player never applies Monster pending scene records");}
            std::uint32_t GeometryQueue() override{throw std::logic_error("childless Player turn must not query world geometry");}
            IActorAnimationControlRuntime& AnimationRuntime() override{throw std::logic_error("childless Player turn must not query model animation");}
        } battle;
        NativeCombatActorDeathServices death_services(skills,system.PropertyEvents(),collections,&battle);
        NativeCombatDeathActions deaths([&]{return battle.CombatInactive();},death_services);
        NativeCombatTaskQueue queue(deaths,system);battle.deaths=&deaths;battle.queue=&queue;
        unsigned turns{},fled{},dead{},states{};std::function<void()> hook;
        std::vector<std::uint32_t> tokens;
        for(const auto name:{"re_can_take_action","ae_fleed","ae_become_dead","ae_state_changed"})
            tokens.push_back(router.Subscribe(router.Key(name),1u,[&,name](std::uint32_t p){
                check(word(p)==a+716u,"Player turn/death actual facet");
                if(std::string_view(name)=="re_can_take_action")++turns;
                if(std::string_view(name)=="ae_fleed")++fled;
                if(std::string_view(name)=="ae_become_dead"){check(word(p+4u)==a,"Player actual death cause");++dead;}
                if(std::string_view(name)=="ae_state_changed")++states;
                reject([&]{actor.Destroy();});reject([&]{skills.Close();});
                if(hook)hook();
            }));
        (void)skills.SetCurrentStat(7u,skills.StatParameter(0u));skills.SetFacetState(1u,false);
        word(a+832u)=49u;battle.inactive=true;
        const auto text=word(a+1164u);
        skills.CanTakeAction(battle);
        check(turns==1u && word(a+832u)==50u && !battle.mode_queries && word(a+1164u)==text,"Player turn49 processes once without mode query or Monster tail write");
        skills.CanTakeAction(battle);
        check(fled==1u && turns==1u && word(a+828u)==6u && word(a+832u)==50u && battle.mode_queries==1u,"Player turn50 inactive takes original Fled shortcut");
        battle.inactive=false;skills.SetFacetState(1u,false);skills.CanTakeAction(battle);
        check(turns==2u && word(a+832u)==51u && battle.mode_queries==2u,"Player turn50 active executes normal shared chain");
        battle.fail_mode=true;reject([&]{skills.CanTakeAction(battle);});battle.fail_mode=false;
        check(word(a+832u)==51u && turns==2u,"mode-query failure precedes turn mutation");
        word(a+832u)=0u;
        hook=[] {throw std::runtime_error("intentional Player turn observer failure");};
        bool failed=false;try{skills.CanTakeAction(battle);}catch(const std::runtime_error&){failed=true;}
        check(failed && word(a+832u)==1u,"turn notification failure retains original counter timing");hook={};skills.CanTakeAction(battle);
        check(word(a+832u)==2u,"Player turn callback guards recover");
        // Borrow original learned IDs in scoped headers; forty entries force
        // the native result buffer past its original32-ID reservation.
        std::vector<std::uint32_t> passives;
        {
        struct BorrowedSlot {
            std::uint32_t address;std::array<std::uint32_t,4> saved{};
            std::vector<std::uint32_t> ids;
            BorrowedSlot(std::uint32_t a,std::uint32_t id):address(a){
                std::memcpy(saved.data(),reinterpret_cast<void*>(a),16u);
                if(!id)return;
                ids.assign(40u,id);
                const auto begin=reinterpret_cast<std::uint32_t>(ids.data());
                const std::array<std::uint32_t,4> header{0u,begin,begin+160u,begin+160u};
                std::memcpy(reinterpret_cast<void*>(a),header.data(),16u);
            }
            ~BorrowedSlot(){std::memcpy(reinterpret_cast<void*>(address),saved.data(),16u);}
        };
        const auto candidate=[&](const char* name){
            const auto& table=database.Tables().at(name);
            std::size_t key=table.fields.size(),link=key;
            for(std::size_t i=0;i<table.fields.size();++i){
                if(table.fields[i].name=="ID")key=i;
                if(table.fields[i].name=="ATTACHED_SKILL")link=i;
            }
            check(key<table.fields.size() && link<table.fields.size(),"original passive table schema");
            for(const auto& row:table.rows){
                const auto id=std::get<std::uint32_t>(row[link]);
                if(!id || collections.SkillDuration(id)!=0xffffffffu)continue;
                const auto record=catalog.Address(id);if(!record)continue;
                const auto fn=word(record+16u);
                if(word(record+56u) || (fn!=9105u && fn!=9109u && fn!=9112u && fn!=9113u))continue;
                std::cout<<"Native Player passive fixture: "<<name<<" id="<<std::get<std::uint32_t>(row[key])<<" skill="<<id<<'\n';
                return std::get<std::uint32_t>(row[key]);
            }
            return 0u;
        };
        const auto magic=candidate("Magic"),stunt=candidate("Stunt");
        check(magic || stunt,"original database contains supported learned permanent passive");
        BorrowedSlot magic_slot(a+920u,magic),stunt_slot(a+1080u,stunt);
        check(skills.AddRecord(7605u,a),"original Player permanent Skill before cleanup");
        skills.CleanupObjectLists(collections);
        for(auto p=word(a+696u);p!=word(a+700u);p+=8u)if(word(p))passives.push_back(word(word(p)));
        for(auto id:passives){const auto duration=collections.SkillDuration(id);check(duration && *duration==0xffffffffu,"Player restored only permanent original passive Skills");}
        check(!passives.empty(),"nonempty original learned passive restoration");
        const auto hp=skills.CurrentStat(7u);skills.CleanupObjectLists(collections);
        std::vector<std::uint32_t> repeated;
        for(auto p=word(a+696u);p!=word(a+700u);p+=8u)if(word(p))repeated.push_back(word(word(p)));
        check(passives==repeated && skills.CurrentStat(7u)==hp,"Player passive restoration order and HP stable across retry");
        } // Restore the owned collection headers before death/destruction.
        // Real inactive QuickDead parent/children/queue exercise Player544,
        // its actual584 collection cleanup and538BC0 stat refresh.
        battle.inactive=true;word(a+832u)=0u;(void)skills.SetCurrentStat(7u,0);
        const auto previous_turns=turns;
        skills.CanTakeAction(battle);
        check(battle.created==1u && battle.registered==1u && queue.Count()==1u && deaths.Count()==1u && turns==previous_turns,
              "dead Player turn queues actual QuickDead and suppresses ready notification");
        for(unsigned tick=0;tick<30u && queue.Count();++tick)queue.Update(0.1f);
        check(!queue.Count() && !deaths.Count() && !deaths.OwnedChildren() && !deaths.OwnedAllocations() &&
              dead==1u && word(a+828u)==4u && word(a+648u)==3u && !skills.CurrentStat(7u),
              "Player real QuickDead completes event43/544/584/rebuild/final state with balanced task allocations");
        for(auto token:tokens)(void)router.Unsubscribe(token);
        skills.Close();check(actor.OwnedAllocations()==allocations && !registry.Count(),"Player turn/death list ownership balanced");
        actor.Destroy();
        std::cout<<"Native Player turn probe: role="<<role<<" turns="<<turns<<" fled="<<fled<<" dead="<<dead<<" restored_passives="<<passives.size()<<'\n';
    }
    std::cout<<"Native Player turns: actual4 shared turns, threshold50 Fled, passive cleanup and real queued QuickDead pass; childless mode fixture\n";
}
}
