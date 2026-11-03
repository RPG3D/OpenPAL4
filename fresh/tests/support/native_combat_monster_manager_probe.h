#pragma once
#include "pal4/native_combat_monster_manager.h"
#include "pal4/native_combat_actor_model_owner.h"
#include "pal4/native_combat_actor_owner.h"
#include "pal4/native_combat_actor_skill_runtime.h"
#include "pal4/native_combat_actor_turn_runtime.h"
#include "pal4/native_rw_geometry_processor.h"
#include "pal4/native_actor_instance_pool.h"
#include "pal4/native_animated_actor_asset.h"
#include "pal4/native_monster_record_catalog.h"
#include "pal4/native_skill_record_catalog.h"
#include "pal4/native_player_collection_values.h"
#include "pal4/native_skill_system_owner.h"
#include "pal4/native_player_combat_events.h"
#include "pal4/native_script_event_router.h"
#include "pal4/random_number_generator.h"
#include <array>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
namespace pal4::test {
inline void VerifyNativeCombatMonsterManager(const std::filesystem::path& root) {
    auto check=[](bool ok,const char* message) { if(!ok) throw std::runtime_error(message); };
    auto word=[](std::uint32_t a)->std::uint32_t& { return *reinterpret_cast<std::uint32_t*>(a); };
    auto reject=[&](auto&& operation) {
        bool rejected=false; try { operation(); } catch(const std::logic_error&) { rejected=true; }
        check(rejected,"expected native Monster manager rejection");
    };
    NativeGameDatabase database; check(database.Load(root),"original manager database");
    NativeMonsterRecordCatalog records(database); NativeSkillRecordCatalog catalog(database);
    NativeScriptEventRouter router; NativePlayerCombatEvents events;
    RandomNumberGenerator random; NativeSkillSystemOwner system(random,events);
    (void)system.Construct("H:/PAL4/objs");
    std::array<std::uint32_t,128> game{};
    NativeActorInstancePool pool([] { return false; },[&](std::string_view model) {
        auto owner=std::make_unique<NativeActorInstanceOwner>();
        owner->Construct(reinterpret_cast<std::uint32_t>(game.data())); owner->Load(root,model); return owner;
    });
    NativeCombatMonsterManager manager(pool,records,catalog,router,system);
    check(router.SubscriptionCount()==3u,"original three mode2 manager subscriptions");
    unsigned added=0u; bool fail_new=false;
    const auto new_actor=router.Subscribe(router.Key("me_new_added"),1u,[&](std::uint32_t payload) {
        const auto a=word(payload)-716u;
        check(manager.ActorAt(manager.Count()-1u)==a && !manager.StageCount(),
              "new actor is in first list, not stage list, before me_new_added");
        const auto index=manager.Count()-1u;
        check(manager.ModelOwner(a).Actor().Name()==std::to_string(index)+"_"+std::string(manager.ModelOwner(a).Actor().BaseName()),
              "original index_ActorBaseName is stored by native host text owner before event");
        reject([&] { manager.Close(); }); reject([&] { manager.Skills(a).Close(); });
        ++added; if(fail_new) throw std::runtime_error("manager new-actor observer failure");
    });
    check(!manager.CreateMonster(-123456,0u) && !manager.Count() && !added,"missing original DB record creates no actor/event");
    const auto a=manager.CreateMonster(2579,0u),b=manager.CreateMonster(2580,1u);
    for(const auto address:{a,b}){
        const auto p=word(address+988u),n=word(address+992u);
        check(manager.ModelOwner(address).Actor().SequenceImage()==(p?std::string_view(reinterpret_cast<const char*>(p),n):std::string_view{}),"actual Monster virtual452 reads its independent portrait record");
        auto& skills=manager.Skills(address);const auto original=word(address+828u);
        skills.SetFacetState(0u,false);check(skills.IsStateZero(),"real state0 sequence count query");
        skills.SetFacetState(3u,false);check(!skills.IsStateZero(),"real nonzero sequence count query");
        skills.SetFacetState(original,false);
    }
    fail_new=true; bool failed=false;
    try { (void)manager.CreateMonster(2580,2u); } catch(const std::runtime_error&) { failed=true; }
    const auto c=manager.ActorAt(2u); fail_new=false;
    check(failed && added==3u && manager.Count()==3u,"new-actor observer failure retains already inserted named actor");
    check(router.Unsubscribe(new_actor),"new actor observer retired");
    NativePlayerCollectionCatalog passive_catalog(database);
    for(const auto actor:{a,b,c}) {
        manager.CleanupObjectLists(actor,passive_catalog);
        (void)manager.Skills(actor).RebuildStats(); manager.JoinStageAndScene(actor);
    }
    check(manager.StageCount()==3u && manager.StageActorAt(0u)==a && manager.StageActorAt(1u)==b && manager.StageActorAt(2u)==c,
          "actual531720 events append actual stage roster in join order");
    manager.JoinStageAndScene(b);
    check(manager.StageCount()==4u && manager.StageActorAt(3u)==b,"repeated join appends duplicate as original533950");
    manager.LeaveStageAndScene(b);
    check(manager.StageCount()==3u && manager.StageActorAt(1u)==c && manager.StageActorAt(2u)==b,
          "original53B580 removes first matching stage entry and preserves later duplicate/order");
    const auto iterator=manager.CreateStageIterator();
    std::vector<std::uint32_t> visited;
    while(!manager.StageIteratorAtEnd(iterator)) { visited.push_back(manager.StageIteratorActor(iterator)); manager.NextStageIterator(iterator); }
    manager.NextStageIterator(iterator);
    check(visited==std::vector<std::uint32_t>{a,c,b},"live stage iterator traverses actual second list and end increment is no-op");
    reject([&] { manager.Close(); }); reject([&] { (void)manager.CreateMonster(2579,4u); });
    reject([&] { manager.LeaveStageAndScene(a); });
    check(manager.StageCount()==3u,"stage mutation rejects while iterator is active");
    manager.DeleteStageIterator(iterator);
    reject([&] { (void)manager.StageIteratorAtEnd(iterator); });
    reject([&] { manager.DeleteStageIterator(iterator); });
    const auto next=manager.CreateStageIterator(); check(next!=iterator,"retired iterator token is never reused"); manager.DeleteStageIterator(next);
    auto& cmodel=manager.ModelOwner(c);
    std::string alternative;
    for(std::size_t i=0u;i<cmodel.Model()->Asset()->ActionCount();++i)
        if(cmodel.Model()->Asset()->ActionAt(i)!="Z01") { alternative=cmodel.Model()->Asset()->ActionAt(i); break; }
    check(!alternative.empty(),"original model has alternate action for filter probe");
    cmodel.PlayAction(alternative,1u,system);
    manager.ModelOwner(b).SetSlot(5u); cmodel.SetSlot(4u);
    for(const auto actor:{a,b,c}) *reinterpret_cast<std::uint8_t*>(actor+1160u)=1u;
    manager.UpdateMonsterMotion(a);
    check(!*reinterpret_cast<std::uint8_t*>(a+1160u) && !*reinterpret_cast<std::uint8_t*>(b+1160u) &&
          *reinterpret_cast<std::uint8_t*>(c+1160u)==1u && !manager.ActiveIterators(),
          "original5317E0 roster propagation handles self recursion and peers, skips alternate animation, frees cursor");
    check(std::fabs(*reinterpret_cast<float*>(manager.ModelOwner(b).Model()->Address()+172u)-126.0f)<0.001f &&
          std::fabs(*reinterpret_cast<float*>(cmodel.Model()->Address()+172u)-161.0f)<0.001f &&
          cmodel.Model()->Asset()->CurrentAction()==alternative,
          "matching peer really repositions in SDK; alternate peer keeps original position and animation");
    cmodel.PlayAction("Z01",1u,system); *reinterpret_cast<std::uint8_t*>(a+1160u)=1u;
    manager.UpdateMonsterMotion(a);
    check(!*reinterpret_cast<std::uint8_t*>(c+1160u) && !manager.ActiveIterators() &&
          std::fabs(*reinterpret_cast<float*>(cmodel.Model()->Address()+172u)+34.0f)<0.001f,
          "restored Z01 joins actual peer motion synchronization");
    // A real peer failure must free the outer cursor and preserve preceding writes.
    const auto bchild=manager.ModelOwner(b).Model()->Address();
    const auto parent=word(bchild+492u); word(bchild+492u)=0u;
    *reinterpret_cast<std::uint8_t*>(a+1160u)=1u;
    failed=false; try { manager.UpdateMonsterMotion(a); } catch(const std::logic_error&) { failed=true; }
    check(failed && !manager.ActiveIterators() && !*reinterpret_cast<std::uint8_t*>(a+1160u),
          "peer model failure unwinds native roster iterator and preserves original flag clear");
    word(bchild+492u)=parent; manager.UpdateMonsterMotion(a);
    reject([&] { (void)manager.MonsterAnimationName(0x1234u); });
    {
        std::array<unsigned,4> world_queue{};
        NativeCombatActorTurnRuntime turn(manager.ModelOwner(a),[&]{return reinterpret_cast<unsigned>(world_queue.data());});
        const auto clump=manager.ModelOwner(a).Model()->Asset()->Main();
        struct Restore {RpClump* clump;~Restore(){NativeRwGeometryProcessor::RestoreClump(clump);}} restore{clump};
        unsigned turns{};
        const auto observer=router.Subscribe(router.Key("me_can_take_action"),1u,[&](unsigned payload){
            check(word(payload)==a+716u,"manager turn routes actual Monster facet");
            reject([&]{manager.Close();});reject([&]{(void)manager.CreateMonster(2580,3u);});
            reject([&]{manager.CanTakeAction(a,turn);});++turns;
        });
        manager.CanTakeAction(a,turn);
        check(turns==1u && manager.Count()==3u && !manager.ActiveIterators(),
              "manager-scoped real turn retains entire roster during synchronous callbacks");
        check(router.Unsubscribe(observer),"manager turn observer retires");
        reject([&]{manager.CanTakeAction(0x1234u,turn);});
    }
    for(const auto actor:{a,b,c}) manager.LeaveStageAndScene(actor);
    check(!manager.StageCount() && manager.Count()==3u,"leave stage does not destroy first-list owners");
    (void)router.Dispatch("combat_over",0u); check(manager.Count()==3u,"original combat_over handler is RET-only");
    manager.Close(); manager.Close();
    check(!manager.Count() && !manager.StageCount() && !manager.ActiveIterators() && !router.SubscriptionCount(),
          "manager explicit retirement releases actor/model/skills/names and subscriptions");
    {
        // Generated Skill17020 tests the group predicate using a real original
        // RET callback; it does not substitute for original skill-effect coverage.
        NativeGameDbTable table;
        for(const auto* name:{"ID","TYPE","TRIGER","EXIST_TIME","FUNCTION","ATTACHED_SKILL","MUTEX_ID","STATUS_ID"})
            table.fields.push_back({name,1u});
        for(unsigned i=1u;i<=9u;++i) table.fields.push_back({"SKILL_PARAM_"+std::to_string(i),3u});
        for(const auto* name:{"EF","BIND","EF1","BIND1"}) table.fields.push_back({name,2u});
        std::vector<NativeGameDbValue> row;
        for(const auto& field:table.fields) {
            if(field.type==2u) row.emplace_back(std::string{});
            else if(field.type==3u) row.emplace_back(0.0f);
            else row.emplace_back(field.name=="ID"?17020u:field.name=="TRIGER"?1u:
                field.name=="EXIST_TIME"?0xffffffffu:field.name=="FUNCTION"?9101u:0u);
        }
        table.rows.push_back(std::move(row));
        NativeSkillRecordCatalog group_catalog(table);
        NativeCombatMonsterManager group_manager(pool,records,group_catalog,router,system);
        std::vector<char> trace;
        const auto created=router.Subscribe(router.Key("me_new_added"),1u,[&](std::uint32_t payload) {
            const auto actor=word(payload)-716u;
            check(group_manager.ActorAt(group_manager.Count()-1u)==actor,"group actor inserted before creation event");
            trace.push_back('N');
        });
        const auto function=router.Subscribe(router.Key("ae_call_fun_id"),1u,[&](std::uint32_t payload) {
            const auto actor=word(payload)-716u;
            check(group_manager.ActorAt(group_manager.Count()-1u)==actor && word(payload+4u)==group_catalog.Address(17020u) &&
                  word(payload+8u)==actor && word(payload+12u)==9101u,
                  "group1 invokes actual Monster268 Skill17020 with self/context and original FUNCTION payload");
            trace.push_back('F');
        });
        (void)group_manager.CreateMonster(2579,0u,0u);
        (void)group_manager.CreateMonster(2580,1u,2u);
        const auto boosted=group_manager.CreateMonster(2580,2u,1u);
        check(trace==std::vector<char>{'N','N','N','F'} && word(boosted+696u)!=word(boosted+700u),
              "only group==1 applies17020, after me_new_added, into actual owned skill list");
        NativeCombatMonsterManager other(pool,records,group_catalog,router,system);
        const auto foreign_cursor=other.CreateStageIterator();
        reject([&] { (void)group_manager.StageIteratorAtEnd(foreign_cursor); });
        other.DeleteStageIterator(foreign_cursor); other.Close();
        check(router.Unsubscribe(created) && router.Unsubscribe(function),"group observers retire");
        group_manager.Close(); check(!router.SubscriptionCount(),"all group manager subscriptions retire");
    }
    {
        NativeCombatMonsterManager first_boss(pool,records,catalog,router,system);
        const auto prior=first_boss.CreateMonster(2583,1u);
        check(prior,"create original first M07 boss 2583");
        NativePlayerCollectionCatalog passive_catalog(database);
        first_boss.CleanupObjectLists(prior,passive_catalog);
        (void)first_boss.Skills(prior).RebuildStats();
        first_boss.JoinStageAndScene(prior);
        const std::string prior_model(first_boss.ModelOwner(prior).Model()->ModelName());
        first_boss.LeaveStageAndScene(prior);
        first_boss.Close();
        NativeCombatMonsterManager boss_manager(pool,records,catalog,router,system);
        const auto boss=boss_manager.CreateMonster(2584,1u);
        check(boss && boss_manager.Count()==1u,"create original second M07 boss 2584");
        boss_manager.CleanupObjectLists(boss,passive_catalog);
        (void)boss_manager.Skills(boss).RebuildStats();
        boss_manager.JoinStageAndScene(boss);
        check(boss_manager.StageCount()==1u,"second M07 boss joins stage");
        const std::string next_model(boss_manager.ModelOwner(boss).Model()->ModelName());
        boss_manager.LeaveStageAndScene(boss);
        boss_manager.Close();
        check(!router.SubscriptionCount(),"second M07 boss manager retires subscriptions");
        std::cout << "Original Monster2583/2584 sequential stage lifecycle verified: "
                  << prior_model << " -> " << next_model << '\n';
    }
    pool.Clear(); system.Destroy(); check(!system.OwnedAllocations(),"manager dependencies retire without live SkillSystem allocations");
    std::cout << "Original Monster creation/name/me_new_added, event-driven stage roster, native cursors and cross-Monster SDK motion verified\n";
}
}
