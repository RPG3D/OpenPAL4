#pragma once
#include "pal4/native_combat_actor_turn_runtime.h"
#include "pal4/native_combat_actor_action_tasks.h"
#include "pal4/native_combat_actor_death_services.h"
#include "pal4/native_combat_actor_skill_runtime.h"
#include "pal4/native_combat_skill_actor_registry.h"
#include "pal4/native_combat_death_actions.h"
#include "pal4/native_combat_task_queue.h"
#include "pal4/native_combat_death_domain.h"
#include "pal4/native_combat_player_actor_owner.h"
#include "pal4/native_combat_player_model_cache.h"
#include "pal4/native_combat_system_image.h"
#include "pal4/native_player_collection_values.h"
#include "pal4/native_skill_system_services.h"
#include "pal4/native_script_event_router.h"
#include "pal4/native_actor_instance_owner.h"
#include "pal4/native_animated_actor_asset.h"
#include <array>
#include <functional>
#include <iostream>
#include <stdexcept>
namespace pal4::test {
inline void VerifyNativeCombatPlayerModeledDeath(NativeCombatPlayerActorOwner& actor,
    NativeCombatPlayerModelCache& model,NativeCombatSystemImage& combat,
    const NativeGameDatabase& database,NativeInventorySession& inventory,
    NativeSkillRecordCatalog& catalog,INativeSkillSystemServices& system,
    NativeScriptEventRouter& router,std::uint32_t geometry,
    const std::function<void(float)>& advance) {
    const auto check=[](bool ok,const char* m){if(!ok)throw std::runtime_error(m);};
    const auto reject=[&](auto&& f){bool denied=false;try{f();}catch(const std::logic_error&){denied=true;}check(denied,"modeled Player death guard");};
    const auto word=[](std::uint32_t a)->std::uint32_t&{return *reinterpret_cast<std::uint32_t*>(a);};
    const auto a=actor.Address();const auto tail=word(a+1164u);
    NativeCombatSkillActorRegistry registry(router,catalog,system);
    NativeCombatActorSkillRuntime skills(actor,registry,inventory);(void)skills.RebuildStats();
    NativePlayerCollectionCatalog passives(database);
    NativeCombatActorTurnRuntime turn(model,combat,[&]{return geometry;});
    const auto saved_mode=combat.Mode();
    struct Mode {NativeCombatSystemImage& c;std::uint8_t saved;~Mode(){c.Mode()=saved;}} restore{combat,saved_mode};
    combat.Mode()=1u;check(turn.CombatInactive(),"Player turn borrows real inactive byte516");
    combat.Mode()=0u;check(!turn.CombatInactive(),"Player turn borrows real active byte516");
    reject([&]{turn.ApplyPendingSceneRecord(a);});
    reject([&]{(void)turn.CreateDeathAction(a);});
    NativeCombatDeathDomain domain(model,skills,turn,combat,system,passives,router);
    auto& actions=domain.Actions();
    NativeCombatTaskQueue queue(actions,system);turn.BindDeathQueue(actions,queue);
    unsigned ready{},dead{};std::array<std::uint32_t,2> subscriptions{};
    subscriptions[0]=router.Subscribe(router.Key("re_can_take_action"),1u,[&](std::uint32_t p){
        if(word(p)==a+716u)++ready;
    });
    subscriptions[1]=router.Subscribe(router.Key("ae_become_dead"),1u,[&](std::uint32_t p){
        if(word(p)!=a+716u)return;
        check(word(p+4u)==a,"modeled Player death self-cause from real turn");++dead;
        check(word(a+828u)==4u && model.Current().Address()==word(a+808u),"death notification retains current owned model");
        reject([&]{model.DeactivateCurrent();});reject([&]{skills.Close();});
    });
    struct Drain {NativeCombatTaskQueue& q;NativeScriptEventRouter& r;std::array<std::uint32_t,2>& ids;
        ~Drain(){for(auto id:ids)if(id)(void)r.Unsubscribe(id);try{q.Clear();}catch(...){}}
    } drain{queue,router,subscriptions};
    skills.SetFacetState(1u,false);(void)skills.SetCurrentStat(7u,skills.StatParameter(0u));
    word(a+832u)=50u;combat.Mode()=1u;skills.CanTakeAction(turn);
    check(!ready && word(a+828u)==6u && !queue.Count() && word(a+832u)==50u,
        "actual Player threshold50 uses real mode516 for Fled without queueing death");
    combat.Mode()=0u;skills.SetFacetState(1u,false);
    word(a+832u)=0u;skills.CanTakeAction(turn);
    check(ready==1u && word(a+832u)==1u && !queue.Count(),"real modeled living Player turn processes original chain");
    const auto cancelled=actions.Create(a,a);check(actions.Name(cancelled)=="Dead","active real mode selects Dead");
    (void)queue.Register(cancelled);check(actions.OwnedChildren()==3u,"Player active Dead has original three children");
    queue.Clear();check(!actions.Count() && !actions.OwnedChildren() && !dead,"cancelled modeled Player death retires children");
    skills.SetFacetState(1u,false);(void)skills.SetCurrentStat(7u,0);
    skills.CanTakeAction(turn);
    check(queue.Count()==1u && actions.OwnedChildren()==3u && ready==1u && word(a+828u)==3u && word(a+804u)==1u,
        "actual Player538040 queues three-child Dead and suppresses ready event");
    unsigned collapse{};
    for(unsigned tick=0;tick<40u && queue.Count();++tick){
        queue.Update(0.1f);advance(0.1f);
        if(_stricmp(model.Current().Asset()->CurrentAction().c_str(),"z12")==0)++collapse;
    }
    check(collapse && dead==1u && !queue.Count() && word(a+828u)==4u && word(a+648u)==3u && !skills.CurrentStat(7u),
        "actual Player z12 collapse and544/584/538BC0 final death chain complete");
    check(!actions.Count() && !actions.OwnedChildren() && !actions.OwnedAllocations() && !actions.OwnedTexts() &&
          word(a+1164u)==tail,"modeled Player death balances tasks and preserves Player-only text tail");
    skills.BecomeAlive();(void)skills.SetCurrentStat(7u,skills.StatParameter(0u));
    model.PlayAction("Z01",1u,system,router);skills.CanTakeAction(turn);
    check(ready==2u && word(a+828u)==1u && skills.CurrentStat(7u)>0 && !queue.Count(),
        "modeled Player revives and takes another real living turn");
    skills.Close();check(!registry.Count(),"modeled Player death releases shared skill registration");
    std::cout<<"Native Player modeled Dead: actor="<<a<<" three children, original z12, actual mode516/world queue, turn/death/revival and ownership pass\n";
}
}
