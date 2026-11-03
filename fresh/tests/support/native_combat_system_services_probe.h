#pragma once
#include "pal4/native_combat_system_services.h"
#include "pal4/native_combat_system_image.h"
#include "pal4/native_combat_player_manager.h"
#include "pal4/native_combat_monster_manager.h"
#include "pal4/native_combat_actor_skill_runtime.h"
#include "pal4/native_combat_actor_turn_services.h"
#include "pal4/native_combat_skill_actor_registry.h"
#include "pal4/native_actor_instance_pool.h"
#include "pal4/native_script_event_router.h"
#include <array>
#include <iostream>
namespace pal4::test {
inline void VerifyNativeCombatSystemServices(const std::filesystem::path& root,std::uint32_t game,
    NativePlayerManagerSession& source,NativeInventorySession& inventory,
    NativeMonsterRecordCatalog& records,NativeSkillRecordCatalog& catalog,
    INativeSkillSystemServices& system,const NativeCombatFormulaCatalog& formulas){
    const auto check=[](bool ok,const char* m){if(!ok)throw std::runtime_error(m);};
    const auto reject=[&](auto&& f){bool failed{};try{f();}catch(const std::logic_error&){failed=true;}check(failed,"System services expected rejection");};
    NativeCombatSystemImage image;image.State()=2u;
    NativeScriptEventRouter router;
    NativeCombatPlayerManager players(source,router,system);
    const auto a=players.AddPlayer(0u,1u),b=players.AddPlayer(1u,1u);
    NativeCombatSkillActorRegistry registry(router,catalog,system);
    NativeCombatActorSkillRuntime first(players.Actor(a),registry,inventory),second(players.Actor(b),registry,inventory);
    NativeActorInstancePool pool([&]{return image.Mode()!=0u;},[&](std::string_view name){
        auto owner=std::make_unique<NativeActorInstanceOwner>();owner->Construct(game);owner->Load(root,name);return owner;
    });
    NativeScriptEventRouter foreign_router;
    reject([&]{NativeCombatMonsterManager invalid(pool,records,catalog,foreign_router,system,registry);});
    NativeCombatMonsterManager monsters(pool,records,catalog,router,system,registry);
    const auto c=monsters.CreateMonster(2579,0u);auto& third=monsters.Skills(c);
    for(auto* s:{&first,&second,&third}){
        (void)s->RebuildStats();s->BindActionProgress(formulas);
        (void)s->SetCurrentStat(7u,s->StatParameter(0u));s->SetFacetState(1u,false);
    }
    check(registry.Count()==3u,"both actual managers share one skill routing domain");
    unsigned effects{};
    const auto effect_observer=router.Subscribe(router.Key("me_attr_float_ext_changed"),1u,[&](std::uint32_t e){
        check(*reinterpret_cast<const std::uint32_t*>(e)==c+716u,"shared manager effect uses actual Monster facet");++effects;
        reject([&]{monsters.Close();});reject([&]{(void)monsters.CreateMonster(2579,1u);});
        check(monsters.Count()==1u && registry.Count()==3u,"cross-roster callbacks preserve manager and routing ownership");
    });
    for(unsigned i=0;i<5u;++i)*reinterpret_cast<std::uint32_t*>(c+584u+i*4u)=0x3f7fffffu;
    check(first.AddRecord(7045u,a),"original Player scalar skill insertion in manager routing domain");
    first.Process(*reinterpret_cast<const std::uint32_t*>(catalog.Address(7045u)+8u),c);
    check(effects>0u,"Player skill routes effects into manager-owned Monster");
    first.Remove(7045u);(void)router.Unsubscribe(effect_observer);
    class Turns final : public INativeCombatPlayerTurnServices {
    public:
        NativeCombatSystemImage& image;
        unsigned mode_reads{};
        explicit Turns(NativeCombatSystemImage& i):image(i){}
        bool CombatInactive() override{++mode_reads;return image.Mode()!=0u;}
        std::uint32_t CreateDeathAction(std::uint32_t) override{throw std::logic_error("living scheduler probe has no death service");}
        void RegisterActorAction(std::uint32_t) override{throw std::logic_error("living scheduler probe has no death queue");}
        void ApplyPendingSceneRecord(std::uint32_t) override{throw std::logic_error("scheduler probe has no pending scene record");}
        std::uint32_t GeometryQueue() override{throw std::logic_error("childless Player scheduler probe has no geometry");}
        IActorAnimationControlRuntime& AnimationRuntime() override{throw std::logic_error("childless Player scheduler probe has no animation");}
    } turns(image);
    Turns second_turns(image);
    const std::array<NativeCombatActorSkillRuntime*,2> bindings{&first,&second};
    unsigned global_touches{},started{};bool fail_global{},fail_turn{};
    std::vector<std::uint32_t> resolved_turns;
    NativeCombatSystemServices services(image,players,monsters,bindings,system,formulas,
        [&](std::uint32_t actor)->INativeCombatPlayerTurnServices&{
            check(actor==a || actor==b || actor==c,"resolver receives actual roster actor");
            if(fail_turn)throw std::logic_error("intentional turn resolution failure");
            resolved_turns.push_back(actor);return actor==b?second_turns:turns;
        },[&]{++global_touches;if(fail_global)throw std::logic_error("intentional global service failure");});
    const std::array<NativeCombatActorSkillRuntime*,2> duplicate{&first,&first};
    reject([&]{NativeCombatSystemServices invalid(image,players,monsters,duplicate,system,formulas,turns,[]{});});
    NativeCombatSystemManager manager(router,services,image.SystemManagerSlot());
    reject([&]{services.BindPlayerSkills(first);});
    reject([&]{services.BindPlayerSkills(third);});
    check(image.SystemManagerSlot()==manager.Address(),"manager publishes actual member396");
    const auto registered=router.SubscriptionCount();
    reject([&]{NativeCombatSystemManager duplicate_manager(router,services,image.SystemManagerSlot());});
    check(router.SubscriptionCount()==registered,"duplicate manager rejects before subscriptions");
    const auto baseline=router.SubscriptionCount();
    check(manager.SubscriptionCount()==13u && !manager.AllObjectsReady(),"real SystemManager subscribes13 and empty stage is not ready");
    const auto emit=[&](const char* name,std::uint32_t actor){const std::array payload{actor+716u};(void)router.Dispatch(name,reinterpret_cast<std::uint32_t>(payload.data()));};
    emit("re_join_stage",a);emit("re_join_stage",b);emit("me_join_stage",c);
    check(players.StageCount()==2u && monsters.StageCount()==1u && manager.Objects().size()==3u && manager.AllObjectsReady(),"both actual native stage rosters participate");
    check(!services.ActiveIterators() && !players.ActiveIterators() && !monsters.ActiveIterators(),"both readiness iterators release on success");
    const std::array<std::uint32_t,1> group{1u};
    check(services.ObjectGroup(a+716u)==1u && services.ObjectGroup(c+716u)==2u,"actual actor804 identifies both groups");
    (void)router.Dispatch("combat_prepare",reinterpret_cast<std::uint32_t>(group.data()));
    check(first.ActionProgress()==64.0f && second.ActionProgress()==64.0f &&
        third.ActionProgress()>=0.0f && third.ActionProgress()<=64.0f && !manager.Active(),
        "original formulas24/25 and real RNG distribute progress then advantage readies Player group");
    const auto cursor=services.BeginActors(0u);
    reject([&]{services.BindPlayerSkills(second);});
    check(services.IteratorCurrent(cursor)==a,"Player stage iteration order");
    reject([&]{(void)players.AddPlayer(2u);});services.IteratorAdvance(cursor);
    check(services.IteratorCurrent(cursor)==b,"Player stage iteration advances");services.ReleaseActorIterator(cursor);
    reject([&]{(void)services.IteratorCurrent(cursor);});reject([&]{(void)services.BeginActors(2u);});
    const auto observer=router.Subscribe(router.Key("re_can_take_action"),1u,[&](std::uint32_t event){
        check(services.DialogDataFromParam(event)==a,"actual selected Player receives turn");++started;
        reject([&]{manager.Close();});reject([&]{players.Close();});
    });
    first.SetActionProgress(64.0f);second.SetActionProgress(0.0f);third.SetActionProgress(0.0f);
    const auto prior_turns=*reinterpret_cast<std::uint32_t*>(a+832u);
    (void)manager.Update(0.0f);
    std::cout<<"Native System turn probe: started="<<started<<" turns_before="<<prior_turns
             <<" turns_after="<<*reinterpret_cast<std::uint32_t*>(a+832u)<<" progress="<<first.ActionProgress()<<'\n';
    check(started==1u && *reinterpret_cast<std::uint32_t*>(a+832u)==prior_turns+1u,"native SystemManager dispatches actual Player538040 turn");
    emit("ae_start_action",a);check(image.State()==3u,"action event updates actual CombatSystem508");
    emit("ae_end_action",a);check(image.State()==2u,"both actual ready stage rosters return state2");
    first.SetFacetState(4u,true);check(manager.Objects().size()==2u,"nonready actual state event removes Player action facet");
    first.SetFacetState(1u,false);emit("ae_become_alive",a);
    check(manager.Objects().size()==3u && manager.Objects().back()==a+716u && first.ActionProgress()==0.0f,"revival restores roster tail and resets actual progress");
    fail_global=true;reject([&]{emit("ae_end_action",a);});fail_global=false;
    emit("ae_end_action",a);emit("re_join_stage",b);
    check(manager.Objects().size()==3u,"duplicate join prepares without duplicate scheduling entry");
    // Explicit callback failure reaches the native owner guard and can recover.
    reject([&]{services.WriteWord(0u,0u);});
    image.Mode()=1u;check(services.CombatInactive(),"mode reads actual516");image.Mode()=0u;
    check(reinterpret_cast<const std::uint8_t*>(&image)[517u]==0u,"scheduling does not commit cold initialization");
    manager.Reset();
    {
        auto& damage=*reinterpret_cast<std::uint32_t*>(c+800u);
        struct Restore {std::uint32_t& field;std::uint32_t value;~Restore(){field=value;}} restore{damage,damage};
        damage=0u;reject([&]{(void)manager.AllObjectsReady();});
        check(!services.ActiveIterators() && !players.ActiveIterators() && !monsters.ActiveIterators(),"both stage iterators release when Monster readiness fails");
    }
    (void)router.Unsubscribe(observer);
    *reinterpret_cast<std::uint32_t*>(b+832u)=50u;
    const auto b_turns=*reinterpret_cast<const std::uint32_t*>(b+832u);
    fail_turn=true;reject([&]{services.StartObject(b+716u);});fail_turn=false;
    check(*reinterpret_cast<const std::uint32_t*>(b+832u)==b_turns,"resolver failure precedes turn mutation");
    services.StartObject(b+716u);
    check(resolved_turns==std::vector<std::uint32_t>{a,b} &&
          *reinterpret_cast<const std::uint32_t*>(b+832u)==b_turns+1u &&
          second_turns.mode_reads==1u && turns.mode_reads==0u,
          "two actual Player turns resolve independently");
    manager.Close();check(!image.SystemManagerSlot(),"manager clears actual member396");
    std::cout<<"Native System turn resolver: two actual Players, failure before mutation and member396 ownership pass\n";
    check(router.SubscriptionCount()==baseline-13u && !services.ActiveIterators(),"SystemManager subscriptions and cursors retire");
    {
        NativeScriptEventRouter creation_router;
        NativeCombatPlayerManager creation_players(source,creation_router,system);
        NativeCombatSkillActorRegistry creation_registry(creation_router,catalog,system);
        NativeCombatMonsterManager creation_monsters(pool,records,catalog,creation_router,system,creation_registry);
        NativeCombatSystemServices creation_services(image,creation_players,creation_monsters,{},system,formulas,turns,[]{});
        const auto created=creation_players.AddPlayer(0u,1u);
        NativeCombatActorSkillRuntime created_skills(creation_players.Actor(created),creation_registry,inventory);
        creation_services.BindPlayerSkills(created_skills);
        check(creation_services.ObjectGroup(created+716u)==1u,"empty scheduler binds actual newly created Player skills");
        reject([&]{creation_services.BindPlayerSkills(created_skills);});
        created_skills.Close();creation_players.Close();creation_monsters.Close();
        std::cout<<"Native System creation binding: empty scheduler, actual new Player and duplicate/foreign/iterator guards pass\n";
    }
    monsters.Close();
    check(registry.Count()==2u && registry.Contains(a) && registry.Contains(b),"borrowed Monster manager Close preserves Player registrations");
    reject([&]{first.Process(1u,c);});
    check(first.AddRecord(7605u,a),"Player registry remains usable after Monster manager Close");first.Remove(7605u);
    first.Close();second.Close();players.Close();pool.Clear();
    check(!registry.Count() && global_touches>0u,"both native manager resources retire");
    std::cout<<"Native System services: actual Player2 Monster1 stage rosters,13 subscriptions, readiness iterators, Player turn, state508/mode516, removal/revival and cleanup pass; childless Player turn fixture\n";
    std::cout<<"Native shared manager routing: Player2 Monster1, original7045 cross-roster effects, domain guard and independent Monster retirement pass\n";
}
}
