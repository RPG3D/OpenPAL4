#include "pal4/native_combat_skill_snapshots.h"
#include "support/native_combat_slot_consume_probe.h"
#include "pal4/actor_manager_skill_action.h"
#include <cstring>
#include "pal4/native_combat_actor_skill_runtime.h"
#include "pal4/native_combat_actor_turn_services.h"
#include "pal4/native_combat_death_actions.h"
#include "pal4/combat_fend_off_sequence_task.h"
#include "pal4/native_combat_fatal_children.h"
#include "pal4/native_combat_death_sequence_services.h"
#include "pal4/native_combat_task_queue.h"
#include "support/native_combat_task_queue_probe.h"
#include "support/native_combat_status_chain_probe.h"
#include "pal4/combat_script_task_operations.h"
#include "pal4/combat_status_tasks.h"
#include "pal4/gom_task_base.h"
#include "pal4/gom_task_container.h"
#include "pal4/actor_animation_control.h"
#include "pal4/native_combat_formula_catalog.h"
#include "pal4/native_combat_damage_calculator.h"
#include "pal4/native_combat_system_manager.h"
#include "pal4/native_player_collection_values.h"
#include "pal4/native_combat_actor_owner.h"
#include "pal4/native_combat_skill_actor_registry.h"
#include "pal4/native_monster_record_catalog.h"
#include "pal4/native_game_database.h"
#include "pal4/native_skill_record_catalog.h"
#include "pal4/native_skill_system_owner.h"
#include "pal4/native_player_combat_events.h"
#include "pal4/native_script_event_router.h"
#include "pal4/random_number_generator.h"
#include <array>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include <bit>
#include <cmath>
#include <set>
#include <limits>
#include <cstdlib>
#include <source_location>

namespace {
void Check(bool v,const char* s) { if(!v) throw std::runtime_error(s); }
std::uint32_t& Word(std::uint32_t a,std::uint32_t o=0u) { return *reinterpret_cast<std::uint32_t*>(a+o); }
template<class F> void Reject(F&& f) {
    bool rejected{}; try { f(); } catch(const std::logic_error&) { rejected=true; }
    Check(rejected,"expected rejection");
}
pal4::NativeGameDbTable Skills() {
    pal4::NativeGameDbTable table;
    for(const auto name:{"ID","TYPE","TRIGER","EXIST_TIME","FUNCTION","ATTACHED_SKILL","MUTEX_ID","STATUS_ID"})
        table.fields.push_back({name,1u});
    for(unsigned i=1u;i<=9u;++i) table.fields.push_back({"SKILL_PARAM_"+std::to_string(i),3u});
    for(const auto name:{"EF","BIND","EF1","BIND1"}) table.fields.push_back({name,2u});
    for(unsigned id=1u;id<=11u;++id) {
        std::vector<pal4::NativeGameDbValue> row;
        for(const auto& field:table.fields) {
            if(field.type==2u) row.emplace_back(std::string{});
            else if(field.type==3u) row.emplace_back(0.0f);
            else row.emplace_back(field.name=="ID"?id:field.name=="FUNCTION"?(id>=10u?9001u:9101u):
                field.name=="TRIGER"?(id==10u?3u:1u):
                field.name=="EXIST_TIME"?(id%2u?0xffffffffu:10u):
                field.name=="ATTACHED_SKILL"?(id==1u?3u:0u):0u);
        }
        table.rows.push_back(std::move(row));
    }
    return table;
}
void AddAction(pal4::NativeGameDbTable& table,unsigned id,unsigned function,float value,
               float selector=0.0f,unsigned next=0u,unsigned trigger=1u,unsigned duration=0xffffffffu) {
    std::vector<pal4::NativeGameDbValue> row;
    for(const auto& field:table.fields) {
        if(field.type==2u) row.emplace_back(std::string{});
        else if(field.type==3u) row.emplace_back(field.name=="SKILL_PARAM_1"?value:field.name=="SKILL_PARAM_2"?selector:0.0f);
        else row.emplace_back(field.name=="ID"?id:field.name=="FUNCTION"?function:
            field.name=="TRIGER"?trigger:field.name=="EXIST_TIME"?duration:field.name=="ATTACHED_SKILL"?next:0u);
    }
    table.rows.push_back(std::move(row));
}
pal4::NativeGameDbTable PassiveLinks(std::initializer_list<std::pair<unsigned,unsigned>> entries) {
    pal4::NativeGameDbTable table;
    table.fields={{"ID",1u},{"ATTACHED_SKILL",1u}};
    for(const auto [id,skill]:entries) table.rows.push_back({id,skill});
    return table;
}
// Isolated coordinator context; actor/property/progress/router/manager owners
// are real. Player iteration and action execution remain explicitly unbound.
class TurnServices : public pal4::INativeCombatActorTurnServices {
public:
    [[noreturn]] static void Unbound(){throw std::logic_error("actual Combat turn branch services not bound");}
    std::uint32_t CreateDeathAction(std::uint32_t) override {Unbound();}
    void RegisterActorAction(std::uint32_t) override {Unbound();}
    void ApplyPendingSceneRecord(std::uint32_t) override {Unbound();}
    std::uint32_t GeometryQueue() override {Unbound();}
    pal4::IActorAnimationControlRuntime& AnimationRuntime() override {Unbound();}
};
class ProgressManagerServices final : public pal4::INativeCombatSystemManagerServices {
public:
    pal4::NativeCombatActorSkillRuntime& skills;
    pal4::NativeSkillSystemOwner& system;
    std::uint32_t actor;
    unsigned action_attempts{};
    TurnServices turns;
    ProgressManagerServices(pal4::NativeCombatActorSkillRuntime& s,pal4::NativeSkillSystemOwner& sys,std::uint32_t a)
        :skills(s),system(sys),actor(a) {}
    [[noreturn]] static void Unbound(std::source_location where=std::source_location::current()) {
        throw std::logic_error(std::string("unbound progress fixture: ")+where.function_name());
    }
    std::uint32_t ReadWord(std::uint32_t) override {Unbound();}
    std::uint8_t ReadByte(std::uint32_t) override {Unbound();}
    void WriteWord(std::uint32_t,std::uint32_t) override {Unbound();}
    std::uint32_t InternalObject(std::uint32_t e) override {Check(Word(e)==actor+716u,"actual roster event");return actor+716u;}
    std::uint32_t DialogDataFromParam(std::uint32_t e) override {(void)InternalObject(e);return actor;}
    std::uint32_t CombatSystemAddress() override {Unbound();}
    bool CombatInactive() override {return false;}
    std::uint32_t CombatState() override {return 2u;}
    void SetCombatState(std::uint32_t) override {Unbound();}
    void TouchSkillSystem() override {Check(system.Address()!=0u,"live initialized SkillSystem");}
    void TouchGlobalSystem() override {}
    void Object(std::uint32_t o){Check(o==actor+716u,"real action-taker facade");}
    void PrepareObject(std::uint32_t o) override {Object(o);skills.PrepareActionProgress();}
    float ObjectRate(std::uint32_t o) override {Object(o);return skills.ActionRate();}
    float ObjectProperty(std::uint32_t o) override {Object(o);return skills.ActionProgress();}
    void SetObjectProperty(std::uint32_t o,float v) override {Object(o);skills.SetActionProgress(v);}
    bool ObjectReady(std::uint32_t o) override {Object(o);return skills.ActionProgressReady();}
    std::uint8_t ActorReady(std::uint32_t a) override {Check(a==actor,"ready actor identity");return skills.ReadyForAction()?1u:0u;}
    std::uint32_t BeginActors(std::uint32_t) override {Unbound();}
    bool IteratorFinished(std::uint32_t) override {Unbound();}
    std::uint32_t IteratorCurrent(std::uint32_t) override {Unbound();}
    void IteratorAdvance(std::uint32_t) override {Unbound();}
    void ReleaseActorIterator(std::uint32_t) override {Unbound();}
    void PauseManager(std::uint32_t) override {Unbound();}
    void StartObject(std::uint32_t o) override {Object(o);++action_attempts;skills.CanTakeAction(turns);}
    void InvokeManagerSlot4(std::uint32_t) override {Unbound();}
    std::uint32_t ReleaseEvents(const std::vector<std::uint32_t>&) override {Unbound();}
    float GameDbFloat(std::uint32_t) override {Unbound();}
    std::uint32_t RandomNumber() override {Unbound();}
    std::uint32_t ObjectGroup(std::uint32_t) override {Unbound();}
};
void DeathActionOwnership() {
    bool inactive{},fail{};unsigned mode_queries{};
    pal4::NativeCombatDeathActions* current{};
    pal4::NativeCombatDeathActions actions([&]{
        ++mode_queries;
        Reject([&]{current->Clear();});
        if(fail) throw std::logic_error("mode query failure");
        return inactive;
    });
    current=&actions;
    Check(!actions.Create(0u,123u) && !actions.Create(123u,0u) && !mode_queries,
          "original null actor gates precede Combat mode lookup");
    std::vector<unsigned> parents;
    for(unsigned i=0;i<40u;++i) {
        inactive=(i%2u)!=0u;
        const auto a=actions.Create(1000u+i,2000u+i);
        parents.push_back(a);
        Check(actions.Owns(a) && actions.Name(a)==(inactive?"QuickDead":"Dead"),
              "shared original death factory selects name and native host string owner");
        Check(Word(a)==(inactive?0x845bbcu:0x845ba0u) && Word(a,44u)==1000u+i && Word(a,48u)==2000u+i &&
              Word(a,36u)==0u && *reinterpret_cast<unsigned char*>(a+40u)==1u,
              "original death parent image matches vtable/source/target/result/active fields");
        const auto sentinel=Word(a,8u);
        Check(sentinel && Word(sentinel)==sentinel && Word(sentinel,4u)==sentinel && !Word(a,12u) &&
              *reinterpret_cast<unsigned char*>(a+16u)==1u,"actual GOM container initialized ready with owned empty sentinel");
    }
    Check(actions.Count()==40u && actions.OwnedTexts()==40u && actions.OwnedAllocations()==80u,
          "each death parent owns actual object/sentinel and one host string");
    Word(parents.front(),36u)=0x12345678u;
    Check(actions.ResultEvent(parents.front())==0x12345678u,
          "actual death virtual4 reads result36 rather than virtual12 ReturnZero");
    Word(parents.front(),36u)=0u;
    fail=true;Reject([&]{(void)actions.Create(123u,456u);});fail=false;
    Check(actions.Count()==40u && actions.OwnedAllocations()==80u,"mode failure preserves all prior action ownership");
    Word(parents.back(),12u)=1u;Reject([&]{actions.Clear();});
    Check(actions.Count()==40u && actions.OwnedAllocations()==80u,"populated child state rejects clear before any parent retirement");
    Word(parents.back(),12u)=0u;
    actions.Destroy(parents.front());
    Check(!actions.Owns(parents.front()) && actions.Count()==39u,"one parent returns name/object/sentinel ownership");
    Reject([&]{actions.Destroy(parents.front());});
    actions.Clear();actions.Clear();
    Check(!actions.Count() && !actions.OwnedTexts() && !actions.OwnedAllocations(),"death parent cleanup balances actual native allocations");
    pal4::NativeCombatDeathActions nexts([]{return false;});
    for(bool transfer:{false,true}){
        const auto dead=actions.Create(1234u,5678u),next=nexts.Create(1234u,9012u);
        actions.AttachSuccessor(dead,next,nexts);
        Check(Word(dead,36u)==next,"native death owns original result36 successor");
        Reject([&]{actions.AttachSuccessor(dead,next,nexts);});
        Word(dead,36u)=0u;Reject([&]{actions.Cancel(dead);});Word(dead,36u)=next;
        Check(actions.Owns(dead) && nexts.Owns(next),"foreign successor replacement rejects before either owner retires");
        if(transfer){Check(actions.ResultEvent(dead)==next && !Word(dead,36u),"death result transfers original successor once");}
        actions.Cancel(dead);
        Check(nexts.Owns(next)==transfer,"pending successor cancels with death; transferred successor survives predecessor retirement");
        if(transfer)nexts.Cancel(next);
    }
    Check(!actions.Count() && !actions.OwnedAllocations() && !nexts.Count() && !nexts.OwnedAllocations(),"death successor cancellation/transfer balances both native owners");
    std::cout<<"Native Dead/QuickDead parents: original factory + actual GOM containers + host names, guards, successor transfer/cancel and cleanup pass\n";
}
void FendOffSuccessorOwnership() {
    unsigned mode_queries{},state_queries{};bool inactive{},state{};
    pal4::NativeCombatDeathActions actions([&]{++mode_queries;return inactive;});
    std::array<unsigned,13> parent{};
    const auto address=static_cast<unsigned>(reinterpret_cast<std::uintptr_t>(parent.data()));
    parent[11]=0x1000u;parent[12]=0x2000u;
    const auto query=[&](unsigned actor){
        Check(actor==parent[12],"successor queries former target");++state_queries;
        Reject([&]{actions.Clear();});
        return state;
    };
    for(unsigned cycle=0;cycle<40u;++cycle){
        const bool quick=(cycle%2u)!=0u;
        parent[0]=quick?pal4::QuickFendOffSequenceTask::kVtable:pal4::FendOffSequenceTask::kVtable;
        inactive=!quick; // Opposite mode must not override parent-selected type.
        state=false;
        Check(!actions.CreateFendOffSuccessor(address,query) && !actions.Count() && !actions.OwnedAllocations(),
              "state-three false creates no successor or allocation");
        state=true;
        const auto a=actions.CreateFendOffSuccessor(address,query);
        Check(actions.Owns(a) && actions.Name(a)==(quick?"QuickDead":"Dead") &&
              Word(a)==(quick?0x845bbcu:0x845ba0u),"parent selects successor despite opposite current mode");
        Check(Word(a,44u)==parent[12] && Word(a,48u)==parent[11] && actions.Identity(a)==parent[12] &&
              actions.Enabled(a) && !actions.Processed(a) && !actions.ResultEvent(a) && actions.Ready(a),
              "original successor exchanges actors and initializes queue fields");
        Check(actions.OwnedAllocations()==2u && actions.OwnedTexts()==1u,"successor is adopted into existing death owner");
        actions.Destroy(a);
    }
    Check(!mode_queries && state_queries==80u && !actions.OwnedAllocations() && !actions.OwnedTexts(),
          "forced successor never queries mode and releases all ownership");
    bool callback_failed{};
    try{(void)actions.CreateFendOffSuccessor(address,[](unsigned)->bool{throw std::runtime_error("state query failure");});}
    catch(const std::runtime_error&){callback_failed=true;}
    Check(callback_failed,"state callback exception propagates");
    Check(!actions.Count() && !actions.OwnedAllocations(),"state callback failure leaves death owner empty");
    state=true;const auto a=actions.CreateFendOffSuccessor(address,query);actions.Destroy(a);
    parent[0]=0u;const auto before=state_queries;
    Reject([&]{(void)actions.CreateFendOffSuccessor(address,query);});
    Reject([&]{(void)actions.CreateFendOffSuccessor(0u,query);});
    Reject([&]{(void)actions.CreateFendOffSuccessor(address,{});});
    Check(state_queries==before && !actions.Count(),"invalid parent/query rejected before dispatch");
    std::cout<<"Native FendOff successors: shared568F70/5690E0, forced types, exchanged actors, callback guards and zero ownership pass\n";
}
void SkillDamageActions(const pal4::NativeMonsterRecordCatalog& monsters,const pal4::NativeGameDatabase& db){
    auto table=Skills();
    for(unsigned id=31u;id<=35u;++id){
        AddAction(table,id,id==34u?9117u:9116u,id==35u?-1.f:1.f);
        auto& row=table.rows.back();
        for(std::size_t i=0;i<table.fields.size();++i){
            if(table.fields[i].name=="SKILL_PARAM_2")row[i]=id==32u?1.f:0.f;
            if(table.fields[i].name=="SKILL_PARAM_3")row[i]=.5f;
            if(table.fields[i].name=="SKILL_PARAM_4")row[i]=id==33u?0.f:.1f;
        }
    }
    pal4::NativeSkillRecordCatalog catalog(table);pal4::NativeCombatFormulaCatalog formulas(db);
    pal4::NativeScriptEventRouter router;pal4::RandomNumberGenerator random;
    pal4::NativePlayerCombatEvents events;pal4::NativeSkillSystemOwner system(random,events);
    (void)system.Construct("H:/PAL4/objs");pal4::NativeCombatSkillActorRegistry registry(router,catalog,system);
    pal4::NativeCombatActorOwner a_owner,d_owner,c_owner;
    const auto a=a_owner.Construct(monsters,2579),d=d_owner.Construct(monsters,2579),c=c_owner.Construct(monsters,2579);
    pal4::NativeCombatActorSkillRuntime source(a_owner,registry),target(d_owner,registry),candidate(c_owner,registry);
    for(auto* s:{&source,&target,&candidate}){
        (void)s->RebuildStats();const auto address=s->ActorAddress();Word(address,344u)=100000u;
        (void)s->SetCurrentStat(7u,50000);s->SetFacetState(2u,false);
    }
    Word(a,372u)=1000u;Word(a,356u)=1000u;Word(d,360u)=0u;Word(a,584u)=std::bit_cast<unsigned>(1.f);Word(a,588u)=std::bit_cast<unsigned>(1.f);
    pal4::NativeCombatDamageCalculator damage(registry,formulas);
    unsigned enumerations{},immunity{};bool throw_enumeration{},foreign{},reentry{};
    auto related=[&](std::uint32_t actor){
        Check(actor==d,"actual related target receiver");++enumerations;
        if(throw_enumeration)throw std::runtime_error("related target failure");
        return std::vector<std::uint32_t>{foreign?c+4u:c};
    };
    source.BindDamageActions([&](auto callback,auto record,auto actor,auto receiver){damage.ApplyAction(callback,record,actor,receiver,related);});
    const auto observer=router.Subscribe(router.Key("ae_immunity_normal"),1u,[&](std::uint32_t event){
        Check(Word(event)==d+716u,"zero damage uses actual facet notification");++immunity;
        Reject([&]{source.BindDamageActions({});});Reject([&]{target.Close();});Reject([&]{d_owner.Destroy();});
        Reject([&]{damage.ApplyAction(0x5806d0u,catalog.Address(33u),a,d,related);});reentry=true;
    });
    random.Seed(123u);auto expected=random;(void)expected.Next();const auto before=target.CurrentStat(7u);
    Check(source.TriggerForTarget(35u,d) && target.CurrentStat(7u)==before && enumerations==0u,"failed probability gate has no damage/related scan");
    auto actual=random;Check(actual.Next()==expected.Next(),"failed probability gate consumes one shared RNG draw");
    Check(source.TriggerForTarget(33u,d) && immunity==1u && reentry && target.CurrentStat(7u)==before,"zero skill uses original immunity branch and guards");
    const auto before_skill=target.CurrentStat(7u);Check(source.TriggerForTarget(31u,d) && target.CurrentStat(7u)<before_skill,"actual9116 dispatch applies shared formula through actor84");
    const auto before_magic=target.CurrentStat(7u);Check(source.TriggerForTarget(34u,d) && target.CurrentStat(7u)<before_magic,"actual9117 dispatch applies shared magic formula");
    const auto candidate_before=candidate.CurrentStat(7u);
    Check(source.TriggerForTarget(32u,d) && enumerations==1u && candidate.CurrentStat(7u)<candidate_before,"skill secondary gate applies to a distinct registered candidate");
    throw_enumeration=true;bool propagated=false;try{(void)source.TriggerForTarget(32u,d);}catch(const std::runtime_error&){propagated=true;}
    Check(propagated,"related target failure propagates");throw_enumeration=false;
    foreign=true;Reject([&]{(void)source.TriggerForTarget(32u,d);});foreign=false;
    Check(source.TriggerForTarget(32u,d),"failed vector acquisition releases scope and allows retry");
    const auto rng_before=random;Reject([&]{damage.ApplyAction(0x5806d0u,catalog.Address(31u)+4u,a,d,related);});
    Reject([&]{damage.ApplyAction(0x5806d0u,catalog.Address(31u),a,d+4u,related);});
    Reject([&]{damage.ApplyAction(0x5808e1u,catalog.Address(31u),a,d,related);});
    auto unchanged=random,comparison=rng_before;Check(unchanged.Next()==comparison.Next(),"foreign record/actor/callback rejected before RNG");
    unsigned source_hp_notifications{};
    const auto hp_observer=router.Subscribe(router.Key("re_attr_int_ext_changed"),1u,[&](std::uint32_t event){
        if(Word(event)==a+716u && Word(event,4u)==7u)++source_hp_notifications;
    });
    (void)target.SetCurrentStat(7u,1);
    Word(d,416u)=std::bit_cast<unsigned>(.5f);
    const auto lethal_source_hp=source.CurrentStat(7u);
    Check(target.StatParameter(7u)>0 && target.CurrentStat(7u)==1,
          "lethal magic fixture separates base HP from current HP");
    Check(source.TriggerForTarget(34u,d) && target.CurrentStat(7u)==0 &&
          source.CurrentStat(7u)==lethal_source_hp && !source_hp_notifications,
          "lethal9117 skips source delta and notification even with positive base HP");
    Check(router.Unsubscribe(hp_observer),"lethal magic HP observer retires");
    (void)router.Unsubscribe(observer);source.Close();target.Close();candidate.Close();
    a_owner.Destroy();d_owner.Destroy();c_owner.Destroy();system.Destroy();
    Check(!registry.Count(),"skill damage fixture retires all actual actors");
    std::cout<<"Native skill damage: actual9116/9117 dispatch, real HP/formula/RNG, immunity/reentry/foreign/vector failure/retry and cleanup pass\n";
}
void ActionProgressBinding(const pal4::NativeMonsterRecordCatalog& monsters,const pal4::NativeGameDatabase& db) {
    pal4::NativeCombatFormulaCatalog formulas(db);
    pal4::NativeSkillRecordCatalog catalog(db);
    pal4::NativeCombatActorOwner actor; const auto a=actor.Construct(monsters,2579);
    pal4::NativeScriptEventRouter router; pal4::RandomNumberGenerator random;
    pal4::NativePlayerCombatEvents events; pal4::NativeSkillSystemOwner system(random,events);
    (void)system.Construct("H:/PAL4/objs");
    pal4::NativeCombatActorSkillRuntime skills(actor,router,catalog,system);
    Reject([&]{(void)skills.ActionRate();});
    (void)skills.RebuildStats();skills.BindActionProgress(formulas);
    // Original detached constructors start with zero current HP. Restore it
    // through the same real property/event path used by stage entry.
    (void)skills.SetCurrentStat(7u,std::bit_cast<std::int32_t>(Word(a,8u)));
    Check(skills.ActionRate()>0.0f,"real actor binding exposes original formula rate");
    const auto observer=router.Subscribe(router.Key("me_moved_at_action_sequence"),3u,[&](std::uint32_t p){
        Check(Word(p)==a+716u,"bound actor progress payload");
        Reject([&]{skills.Close();});Reject([&]{skills.BindActionProgress(formulas);});Reject([&]{actor.Destroy();});
    });
    Word(a,668u)=0u;skills.SetActionProgress(25.0f);
    Check(skills.ActionProgress()==25.0f,"bound progress writes actual actor");
    skills.PrepareActionProgress();Check(skills.ActionProgress()==0.0f,"bound progress reset");
    {
        ProgressManagerServices services(skills,system,a);
        pal4::NativeCombatSystemManager manager(router,services);
        const std::uint32_t record=a+716u;
        (void)router.Dispatch("me_join_stage",reinterpret_cast<std::uint32_t>(&record));
        Check(manager.Objects().size()==1u && manager.Objects()[0]==a+716u,"real actor event populates native action manager");
        (void)manager.Update(0.01f);
        Check(std::fabs(skills.ActionProgress()-skills.ActionRate()*10.0f)<0.00001f,"native manager frame advances real original Monster progress");
        unsigned turn_events{};
        const auto action_observer=router.Subscribe(router.Key("me_can_take_action"),1u,[&](std::uint32_t p){
            Check(Word(p)==a+716u,"actual full turn event facet");
            Check(Word(a,832u)==1u,"actual turn increments actor832");
            Reject([&]{skills.Close();});Reject([&]{actor.Destroy();});
            ++turn_events;
        });
        (void)manager.Update(100.0f);
        Check(skills.ActionProgress()==64.0f && services.action_attempts==1u && turn_events==1u,
              "native manager reaches64 and runs original full living childless Monster turn");
        Check(router.Unsubscribe(action_observer),"turn observer cleanup");
        manager.Close();
    }
    Check(router.Unsubscribe(observer),"bound progress observer cleanup");
    skills.BindActionProgress(formulas);skills.Close();
    Reject([&]{(void)skills.ActionRate();});actor.Destroy();system.Destroy();
    Check(actor.OwnedAllocations()==0u && system.OwnedAllocations()==0u,"bound progress owner cleanup");
    std::cout<<"Native manager + original Monster: roster/frame/clamp/full living childless turn event pass; death/model/Player production services pending\n";
}
void TurnBoundary(const pal4::NativeMonsterRecordCatalog& monsters) {
    auto table=Skills();
    AddAction(table,5001u,9112u,0.4f,0.0f,0u,6u,2u);
    pal4::NativeSkillRecordCatalog catalog(table);
    pal4::NativeCombatActorOwner actor; const auto a=actor.Construct(monsters,2579);
    pal4::NativeScriptEventRouter router; pal4::RandomNumberGenerator random;
    pal4::NativePlayerCombatEvents events; pal4::NativeSkillSystemOwner system(random,events);
    (void)system.Construct("H:/PAL4/objs");
    pal4::NativeCombatActorSkillRuntime skills(actor,router,catalog,system);
    (void)skills.RebuildStats();
    (void)skills.SetCurrentStat(7u,std::bit_cast<std::int32_t>(Word(a,8u)));
    Check(skills.ReadyForAction(),"actual living actor ready predicate");
    Check(skills.AddRecord(5001u,a),"timed trigger6 skill inserted through actual lists");
    TurnServices missing;
    unsigned turns{};
    const auto observer=router.Subscribe(router.Key("me_can_take_action"),1u,[&](std::uint32_t p){
        Check(Word(p)==a+716u,"turn event original one-word facet");
        Check(Word(a,708u)==0u && Word(a,712u)==0u && Word(a,1164u)==0u,
              "turn clears action fields and pending scene record before notification");
        Reject([&]{skills.Close();});Reject([&]{actor.Destroy();});
        ++turns;
    });
    Word(a,708u)=123u;Word(a,712u)=456u;Word(a,1164u)=999u;
    skills.CanTakeAction(missing);
    Check(turns==1u && Word(a,832u)==1u && Word(a,584u)==std::bit_cast<unsigned>(0.4f),
          "turn runs selector6 real scalar callback before event");
    Check(Word(Word(a,680u),8u)==1u,"one turn advances real timed skill by1");
    skills.CanTakeAction(missing);
    Check(turns==2u && Word(a,832u)==2u && Word(Word(a,680u))==0u,
          "second turn expires duration2 skill through actual removal/rebuild");

    class Branches final : public TurnServices,public pal4::IActorAnimationControlRuntime {
    public:
        pal4::NativeCombatDeathActions actions{[]{return false;}};
        unsigned actor{},death_creates{},death_registers{},replacements{},colors{},atomics{};
        bool fail_death{},fail_replace{};
        std::vector<unsigned> color_values;
        std::uint32_t CreateDeathAction(std::uint32_t a) override {
            Check(a==actor && Word(a,828u)==3u,"death factory follows actual facet state3 event");
            if(fail_death) Unbound();
            ++death_creates;return actions.Create(a,a);
        }
        void RegisterActorAction(std::uint32_t action) override {
            Check(actions.Owns(action) && actions.Name(action)=="Dead" && Word(action,44u)==actor && Word(action,48u)==actor,
                  "real death parent forwarded to registration protocol");++death_registers;
        }
        void ApplyPendingSceneRecord(std::uint32_t a) override {
            Check(a==actor && Word(a,1164u)==42u,"pending replacement sees current record key");
            if(fail_replace) Unbound();
            ++replacements;
        }
        std::uint32_t GeometryQueue() override {return 0x9876u;}
        pal4::IActorAnimationControlRuntime& AnimationRuntime() override {return *this;}
        std::uint32_t ReadWord(std::uint32_t address) override {return Word(address);}
        std::int64_t FloatToInteger(float value) override {return static_cast<std::int64_t>(value);}
        void InvokeActorSlot24(std::uint32_t,unsigned color) override {++colors;color_values.push_back(color);}
        std::uint32_t ProcessClumpAtomics(std::uint32_t clump,std::uint32_t callback) override {
            Check(clump>=100u && clump<=102u && callback==0x9876u,"alpha forwards world queue and clump");
            ++atomics;return clump;
        }
        void CleanupCamera(std::uint32_t) override {Unbound();}
        void RenderCamera(std::uint32_t,std::uint32_t) override {Unbound();}
    } branches;
    branches.actor=a;
    Word(a,648u)=3u;branches.fail_death=true;
    Reject([&]{skills.CanTakeAction(branches);});
    Check(Word(a,828u)==3u && turns==2u,"missing death owner fails after state3 and suppresses action notification");
    branches.fail_death=false;skills.CanTakeAction(branches);
    Check(branches.death_creates==1u && branches.death_registers==1u && turns==2u,
          "dead actor registers protocol death action then exits without can-take-action");
    (void)skills.SetCurrentStat(7u,std::bit_cast<std::int32_t>(Word(a,8u)));
    Word(a,660u)=1u;
    const auto pending=router.Subscribe(router.Key("me_state_changed"),1u,[&](std::uint32_t p){
        if(Word(p,4u)==7u) Word(a,1164u)=42u;
    });
    branches.fail_replace=true;Reject([&]{skills.CanTakeAction(branches);});
    Check(Word(a,1164u)==42u && turns==2u,"replacement failure preserves pending key and suppresses notification");
    Word(a,660u)=1u;branches.fail_replace=false;skills.CanTakeAction(branches);
    Check(branches.replacements==1u && Word(a,1164u)==0u && turns==3u,
          "callback-scheduled replacement runs before action notification");
    Check(router.Unsubscribe(pending),"pending observer cleanup");
    // Animation service protocol only: shared alpha algorithm runs on a borrowed
    // child image, not a RenderWare model or a real world render queue.
    std::array<unsigned,279u> child{};child[5]=100u;child[161]=101u;child[277]=102u;
    Word(a,808u)=reinterpret_cast<unsigned>(child.data());
    struct ClearChild {unsigned actor;~ClearChild(){Word(actor,808u)=0u;}} clear{a};
    Word(a,640u)=4u;skills.CanTakeAction(branches);
    Check(branches.colors==3u && branches.atomics==3u &&
          branches.color_values==std::vector<unsigned>(3u,0x0cffffffu),
          "state4 alpha0.05 reaches body and both weapon colors before notification");
    Word(a,640u)=0u;skills.CanTakeAction(branches);
    Check(branches.colors==6u && branches.atomics==6u && branches.color_values.back()==0xffffffffu,
          "ordinary state restores full alpha");
    Word(a,808u)=0u;
    Check(router.Unsubscribe(observer),"turn observer cleanup");
    skills.Close();Reject([&]{skills.CanTakeAction(branches);});
    branches.actions.Clear();
    actor.Destroy();system.Destroy();
    Check(actor.OwnedAllocations()==0u,"turn releases actor skill buffers and property ownership");
    std::cout<<"Native Monster turn: real trigger6/timed expiration/events and failure guards pass; death/scene/alpha branch protocols verified\n";
}
void CombatCleanup(const pal4::NativeMonsterRecordCatalog& monsters,const pal4::NativeGameDatabase& db) {
    const auto table=Skills();
    pal4::NativeSkillRecordCatalog catalog(table);
    pal4::NativePlayerCollectionCatalog links(PassiveLinks({{100u,3u},{101u,5u},{102u,2u},{103u,999u},{105u,2u}}),
        PassiveLinks({{100u,7u},{104u,9u},{102u,999u}}),table);
    Check(links.MonsterPassiveSkills({100u,101u,100u,104u,102u})==std::vector<unsigned>{7u,5u,7u,9u},
        "five-key passive scan preserves order/duplicates and Stunt precedence including missing attached Skill");
    Check(links.MonsterPassiveSkills({103u,102u,555u,0u,105u}).empty(),"missing and timed Skill do not become passive");
    pal4::NativeCombatActorOwner actor; const auto a=actor.Construct(monsters,2579);
    pal4::NativeScriptEventRouter router; pal4::RandomNumberGenerator random;
    pal4::NativePlayerCombatEvents events; pal4::NativeSkillSystemOwner system(random,events);
    (void)system.Construct("H:/PAL4/objs");
    pal4::NativeCombatActorSkillRuntime skills(actor,router,catalog,system);
    Check(skills.AddRecord(2u,a) && skills.AddRecord(9u,a),"cleanup starts with both real list formats");
    const auto timed_buffer=Word(a,680u);
    for(unsigned i=0;i<5u;++i) Word(a,896u+i*4u)=555u;
    std::vector<unsigned> removed,added;
    const auto rem=router.Subscribe(router.Key("ae_remove_skill"),1u,[&](unsigned p) {
        Check(Word(p)==a+716u,"cleanup remove uses actual actor facet");
        removed.push_back(Word(Word(p,4u)));
        Reject([&]{skills.CleanupObjectLists(links);});
        // Original531600 collects keys AFTER old skill removal notifications.
        Word(a,896u)=100u; Word(a,900u)=101u; Word(a,904u)=100u;
        Reject([&]{skills.Close();}); Reject([&]{actor.Destroy();});
    });
    const auto add=router.Subscribe(router.Key("ae_add_skill"),1u,[&](unsigned p) {
        Check(Word(p)==a+716u && Word(p,8u)==a,"passive restoration uses actual self context");
        added.push_back(Word(Word(p,4u)));
        Reject([&]{skills.Close();});
    });
    random.Seed(456u);pal4::RandomNumberGenerator expected;expected.Seed(456u);
    skills.CleanupObjectLists(links);
    Check(removed==std::vector<unsigned>{2u,9u} && added==std::vector<unsigned>{7u,5u,7u},
        "actual531600 removes timed/permanent then restores passive list including duplicates");
    Check(Word(a,680u)==timed_buffer && Word(a,684u)==timed_buffer && Word(a,696u)!=0u &&
        Word(a,700u)-Word(a,696u)==24u && Word(Word(a,696u))==0u &&
        Word(Word(a,696u)+8u)==catalog.Address(5u) &&
        Word(Word(a,696u)+16u)==catalog.Address(7u),"cleanup retains actual buffers, duplicate tombstone and final permanent records");
    Check(random.Next()==expected.Next(),"passive cleanup with original RET callbacks consumes no RNG");
    Check(router.Unsubscribe(rem),"retire mutation observer");
    bool fail=true;
    const auto throwing=router.Subscribe(router.Key("ae_remove_skill"),2u,[&](unsigned) {if(fail) throw std::logic_error("cleanup observer failure");});
    Reject([&]{skills.CleanupObjectLists(links);});
    Check(Word(a,700u)-Word(a,696u)==16u,"cleanup failure retains original prior removal and does not restore early");
    fail=false; skills.CleanupObjectLists(links);
    Check(Word(a,700u)-Word(a,696u)==24u,"cleanup retries after callback failure");
    Check(router.Unsubscribe(throwing) && router.Unsubscribe(add),"cleanup observers retired");
    skills.Close();actor.Destroy();system.Destroy();
    Reject([&]{skills.CleanupObjectLists(links);});
    pal4::NativePlayerCollectionCatalog original(db);
    pal4::NativeSkillRecordCatalog original_skills(db);
    (void)system.Construct("H:/PAL4/objs");
    for(const auto id:{2579,2580}) {
        pal4::NativeCombatActorOwner original_actor; const auto address=original_actor.Construct(monsters,id);
        std::array<unsigned,5> keys{}; for(unsigned i=0;i<5u;++i) keys[i]=Word(address,896u+i*4u);
        const auto passive=original.MonsterPassiveSkills(keys);
        pal4::NativeCombatActorSkillRuntime original_runtime(original_actor,router,original_skills,system);
        (void)original_runtime.RebuildStats();
        Check(original_runtime.AddRecord(7605u,address),"original permanent Skill inserted before531600");
        original_runtime.CleanupObjectLists(original);
        Check(passive.empty() && Word(address,700u)==Word(address,696u) &&
              Word(address,684u)==Word(address,680u),"original cave531600 removes actual Skill and restores no passive");
        std::cout<<"Original Monster="<<id<<" passive keys=";
        for(const auto key:keys) std::cout<<key<<',';
        std::cout<<" permanent="<<passive.size()<<'\n';
        original_runtime.Close(); original_actor.Destroy();
    }
    system.Destroy();
}
void DeathLifecycle(const pal4::NativeMonsterRecordCatalog& monsters) {
    auto table=Skills();
    pal4::NativeSkillRecordCatalog catalog(table);
    pal4::NativePlayerCollectionCatalog links(PassiveLinks({{100u,5u}}),PassiveLinks({}),table);
    pal4::NativeCombatActorOwner actor,cause_owner;
    const auto a=actor.Construct(monsters,2579),cause=cause_owner.Construct(monsters,2580);
    pal4::NativeScriptEventRouter router;pal4::RandomNumberGenerator random;
    pal4::NativePlayerCombatEvents events;pal4::NativeSkillSystemOwner system(random,events);
    (void)system.Construct("H:/PAL4/objs");
    pal4::NativeCombatActorSkillRuntime skills(actor,router,catalog,system);
    (void)skills.RebuildStats();
    const auto hp_max=Word(a,344u);
    Check(hp_max>0 && Word(a,648u)==3u,"death test starts with original dead HP state");
    Check(skills.AddRecord(2u,a) && skills.AddRecord(9u,a),"death has timed and permanent skills");
    for(unsigned i=0;i<5u;++i)Word(a,896u+i*4u)=i==0?100u:0u;
    for(unsigned index:{0u,1u,2u,3u,5u,6u,7u,8u})Word(a,632u+4u*index)=1u;
    Word(a,828u)=3u;Word(a,344u)=1u;
    std::vector<unsigned> state_changes,sequence,removed,added;
    const auto changed=router.Subscribe(router.Key("me_state_changed"),1u,[&](unsigned p){
        if(Word(p,4u)==4u) {
            Check(Word(p)==a+716u && Word(p,8u)==0u && removed==std::vector<unsigned>({2u,9u}),
                  "rebuild restores base status bytes then recomputes dead health after old skills retire");
            sequence.push_back(6u);return;
        }
        if(Word(p)!=a+716u || Word(p,8u)!=1u)
            throw std::runtime_error("death reset payload index="+std::to_string(Word(p,4u))+" old="+std::to_string(Word(p,8u)));
        state_changes.push_back(Word(p,4u));sequence.push_back(1u);
        Reject([&]{skills.BecomeDead(cause,links);});Reject([&]{skills.Close();});Reject([&]{actor.Destroy();});
    });
    const auto rem=router.Subscribe(router.Key("ae_remove_skill"),1u,[&](unsigned p){
        Check(state_changes==std::vector<unsigned>({0u,1u,2u,3u,5u,6u,7u,8u}),"all eight resets precede skill cleanup");
        removed.push_back(Word(Word(p,4u)));sequence.push_back(2u);
        Reject([&]{skills.CleanupObjectLists(links);});
    });
    const auto add=router.Subscribe(router.Key("ae_add_skill"),1u,[&](unsigned p){
        Check(Word(p,8u)==a,"death restores actual passive self context");
        added.push_back(Word(Word(p,4u)));sequence.push_back(3u);
    });
    const auto state=router.Subscribe(router.Key("ae_state_changed"),1u,[&](unsigned p){
        Check(Word(p)==a+716u && Word(a,828u)==4u && Word(a,648u)==3u,"facet4 preserves health3");
        Check(Word(a,344u)==hp_max && added==std::vector<unsigned>{5u},"stats rebuild and passive restoration precede death state");
        sequence.push_back(4u);
    });
    const auto dead=router.Subscribe(router.Key("ae_become_dead"),1u,[&](unsigned p){
        Check(Word(p)==a+716u && Word(p,4u)==cause && sequence.back()==4u,"death event forwards exact cause after state event");
        sequence.push_back(5u);Reject([&]{skills.BecomeDead(cause,links);});
    });
    class EndServices final : public pal4::ICombatScriptTaskRuntime,public pal4::ICombatStatusTaskRuntime {
    public:
        pal4::NativeCombatActorSkillRuntime& skills;const pal4::NativePlayerCollectionCatalog& links;unsigned actor,cause;
        EndServices(pal4::NativeCombatActorSkillRuntime& s,const pal4::NativePlayerCollectionCatalog& l,unsigned a,unsigned c)
            :skills(s),links(l),actor(a),cause(c){}
        unsigned ReadWord(unsigned p) override{return Word(p);}
        std::uint8_t ReadByte(unsigned p) override{return *reinterpret_cast<std::uint8_t*>(p);}
        void WriteWord(unsigned p,unsigned v) override{Word(p)=v;}
        void CallActor(unsigned a,unsigned slot,unsigned c) override{
            Check(a==actor && slot==544u && c==cause,"actual native end callback receiver");skills.BecomeDead(c,links);
        }
        void FireEvent(unsigned,unsigned,unsigned) override{throw std::logic_error("unused death event object");}
        void SetProtectTarget(unsigned,unsigned,unsigned) override{throw std::logic_error("unused protect");}
        void ConstructBase(unsigned) override{throw std::logic_error("unused base");}
        void SetStateOne(unsigned) override{throw std::logic_error("unused state1");}
    } services(skills,links,a,cause);
    pal4::NativeCombatFatalChildren children(services,services);
    auto& runtime=children.ContainerRuntime();pal4::GomTaskBase base(children.BaseRuntime());
    pal4::GomTaskContainer queue(runtime,base);pal4::GomTaskContainerState tasks{};
    queue.Initialize(tasks,0u);
    const auto end=children.Create(0x845decu);
    runtime.WriteWord(end+60u,a);runtime.WriteWord(end+64u,cause);
    (void)queue.AddTaskAndInitialize(tasks,reinterpret_cast<unsigned>(&tasks),runtime.TaskAt(end),end,"end",{1u,0,0});
    (void)queue.BeginWaiting(tasks);
    for(unsigned tick=0;tick<8u && !tasks.ready;++tick)(void)queue.Update(tasks,0.1f);
    Check(tasks.ready && children.Count()==0u,"actual scheduled end reaches native Monster death and retires");
    Check(removed==std::vector<unsigned>({2u,9u}) && added==std::vector<unsigned>{5u},"death clears old lists and restores passive");
    Check(sequence==std::vector<unsigned>({1u,1u,1u,1u,1u,1u,1u,1u,2u,2u,6u,3u,6u,4u,5u}),"original544 complete event ordering including passive and final health recompute");
    (void)queue.Destroy(tasks);Check(children.OwnedAllocations()==0u,"death child container balanced");
    for(auto handle:{changed,rem,add,state,dead})Check(router.Unsubscribe(handle),"death observer cleanup");
    // Partial event failure must not send a premature death notification or
    // leave catalog/dispatch guards bound to the failed invocation.
    Word(a,632u)=1u;Word(a,828u)=3u;
    bool fail=true;unsigned notifications{};
    const auto failure=router.Subscribe(router.Key("me_state_changed"),1u,[&](unsigned){
        if(fail)throw std::logic_error("death reset observer failure");
    });
    const auto final=router.Subscribe(router.Key("ae_become_dead"),1u,[&](unsigned){++notifications;});
    Reject([&]{skills.BecomeDead(cause,links);});
    Check(Word(a,632u)==0u && Word(a,828u)==3u && !notifications,"death callback failure preserves partial original effects");
    fail=false;skills.BecomeDead(cause,links);
    Check(Word(a,828u)==4u && notifications==1u,"death can retry after event failure");
    Check(router.Unsubscribe(failure) && router.Unsubscribe(final),"failure observers cleanup");
    skills.Close();actor.Destroy();cause_owner.Destroy();system.Destroy();
    Reject([&]{skills.BecomeDead(cause,links);});
}
void QuickDeath(const pal4::NativeMonsterRecordCatalog& monsters,const pal4::NativeGameDatabase& db) {
    pal4::NativeSkillRecordCatalog catalog(db);pal4::NativePlayerCollectionCatalog passives(db);
    pal4::NativeScriptEventRouter router;pal4::RandomNumberGenerator random;
    pal4::NativePlayerCombatEvents events;pal4::NativeSkillSystemOwner system(random,events);
    (void)system.Construct("H:/PAL4/objs");
    pal4::NativeCombatActorOwner actor,cause_owner;
    const auto a=actor.Construct(monsters,2579),cause=cause_owner.Construct(monsters,2580);
    pal4::NativeCombatActorSkillRuntime skills(actor,router,catalog,system);
    (void)skills.RebuildStats();
    class Services final : public pal4::INativeCombatDeathSequenceServices {
    public:
        pal4::NativeCombatActorSkillRuntime& skills;pal4::NativePlayerCombatEvents& events;
        const pal4::NativePlayerCollectionCatalog& passives;unsigned actor,cause,calls{},deaths{};bool fail{};
        Services(pal4::NativeCombatActorSkillRuntime& s,pal4::NativePlayerCombatEvents& e,
                 const pal4::NativePlayerCollectionCatalog& p,unsigned a,unsigned c)
            :skills(s),events(e),passives(p),actor(a),cause(c){}
        unsigned ReadWord(unsigned p) override{return Word(p);}
        std::uint8_t ReadByte(unsigned p) override{return *reinterpret_cast<std::uint8_t*>(p);}
        void WriteWord(unsigned p,unsigned v) override{Word(p)=v;}
        void SetActorState(unsigned a,unsigned state,unsigned notify) override{
            Check(a==actor && state==3u && notify==1u,"QuickDead starts with actual facet3 notification");
            skills.SetFacetState(state,notify!=0u);
        }
        unsigned EventDataByIndex(unsigned index) override{Check(index==43u,"QuickDead exact event43");return events.Lookup(index);}
        void FireEvent(unsigned event,unsigned a,unsigned bits) override{
            Check(a==actor && event==events.Lookup(43u) && bits==0x40400000u,"QuickDead actual owned descriptor and float3");
            if(fail)throw std::logic_error("event43 fixture fault");
            skills.SetCombatEvent(event,bits);++calls;
        }
        void CallActor(unsigned a,unsigned slot,unsigned c) override{
            Check(a==actor && slot==544u && c==cause,"QuickDead actual death receiver and cause");
            skills.BecomeDead(c,passives);++deaths;
        }
        void SetProtectTarget(unsigned,unsigned,unsigned) override{throw std::logic_error("unexpected protect");}
        void ConstructBase(unsigned) override{throw std::logic_error("unexpected external base");}
        void SetStateOne(unsigned) override{throw std::logic_error("unexpected state1");}
    } services(skills,events,passives,a,cause);
    bool inactive=true;pal4::NativeCombatDeathActions actions([&]{return inactive;},services);
    unsigned dead_events{},set_events{},state3{};pal4::NativeCombatTaskQueue* queued{};
    const auto changed=router.Subscribe(router.Key("me_state_changed"),1u,[&](unsigned p){
        Check(Word(p)==a+716u,"QuickDead actual health facet");
        if(Word(p,4u)==4u && Word(p,8u)==1u){Check(Word(a,648u)==3u,"event43 SET3 rather than increment");++set_events;}
    });
    const auto state=router.Subscribe(router.Key("ae_state_changed"),1u,[&](unsigned){
        if(Word(a,828u)==3u)++state3;
    });
    const auto dead=router.Subscribe(router.Key("ae_become_dead"),1u,[&](unsigned p){
        Check(Word(p)==a+716u && Word(p,4u)==cause && Word(a,828u)==4u && Word(a,648u)==3u,"QuickDead reaches final real state and cause");
        ++dead_events;
        Reject([&]{actions.Clear();});Reject([&]{skills.Close();});
        if(queued){Reject([&]{queued->Clear();});Reject([&]{queued->Update(0.1f);});}
    });
    const auto action=actions.Create(a,cause);
    Check(actions.Name(action)=="QuickDead" && actions.OwnedAllocations()==2u,"real inactive-mode parent and sentinel");
    Word(a,648u)=1u;actions.Start(action);
    Check(!actions.Ready(action) && actions.OwnedChildren()==2u && state3==1u && !services.calls,
          "original QuickDead start prepares children but does not execute event immediately");
    const auto head=Word(action,8u),first_node=Word(head),event_child=Word(first_node,8u),end_child=Word(Word(first_node),8u);
    Check(Word(event_child)==0x845df4u && Word(end_child)==0x845e48u &&
          Word(event_child,40u)==2u && Word(end_child,40u)==2u && Word(end_child,48u)==event_child,
          "original Start recipe creates linked waiting event/end children");
    Check(std::string(reinterpret_cast<const char*>(Word(event_child,8u)))=="DataOperateTask" &&
          std::string(reinterpret_cast<const char*>(Word(end_child,8u)))=="DeadTask","child names use existing original catalog");
    Reject([&]{actions.Start(action);});Reject([&]{actions.Destroy(action);});
    services.fail=true;Reject([&]{actions.Update(action,0.1f);});
    Check(Word(event_child,40u)==2u && !services.deaths && Word(a,648u)==1u,"failed event43 blocks actual end child");
    services.fail=false;
    for(unsigned tick=0;tick<20u && !actions.Ready(action);++tick)actions.Update(action,0.1f);
    Check(actions.Ready(action) && actions.OwnedChildren()==0u && services.calls==1u && services.deaths==1u &&
          set_events==1u && dead_events==1u,"full native QuickDead recipe reaches real event43 and death544 exactly once");
    Reject([&]{actions.Start(action);});actions.Destroy(action);
    Check(!actions.Count() && !actions.OwnedAllocations() && !actions.OwnedTexts(),"parent child name node sentinel cleanup balanced");
    const auto cancel=actions.Create(a,cause);actions.Start(cancel);actions.Cancel(cancel);
    Check(!actions.Count() && !actions.OwnedChildren() && !actions.OwnedAllocations() && dead_events==1u,"cancellation retires waiting children without executing death");
    inactive=false;const auto active=actions.Create(a,cause);const auto before=Word(a,828u);
    Check(actions.Name(active)=="Dead","active mode retains actual Dead factory selection");
    Reject([&]{actions.Start(active);});
    Check(Word(a,828u)==before && !actions.OwnedChildren(),"unsupported active collapse path rejects before mutation");
    actions.Destroy(active);
    inactive=true;
    pal4::NativeCombatTaskQueue queue(actions,system);queued=&queue;
    Check(queue.Register(0u)==0u && queue.Count()==0u,"null native queue event preserves original gate");
    Reject([&]{(void)queue.Register(0x1234u);});
    const auto old=actions.Create(a,cause),replacement=actions.Create(a,cause);
    Word(a,648u)=1u;
    Check(queue.Register(old)==old && queue.Count()==1u && actions.OwnedChildren()==2u,"native queue registration immediately starts original recipe");
    Reject([&]{(void)queue.Register(old);});
    Check(queue.Register(replacement)==replacement && queue.Count()==2u &&
          *reinterpret_cast<std::uint8_t*>(old+40u)==0u && actions.OwnedChildren()==4u,
          "same-actor queue registration disables old action before starting replacement");
    for(unsigned tick=0;tick<20u && queue.Count();++tick)queue.Update(0.1f);
    Check(!queue.Count() && !actions.Count() && !actions.OwnedChildren() && !actions.OwnedAllocations() &&
          services.calls==2u && services.deaths==2u && dead_events==2u && set_events==2u,
          "shared queue algorithm cancels disabled old action and automatically retires completed replacement");
    const auto reset=actions.Create(a,cause);(void)queue.Register(reset);queue.ResetFlags();queue.Update(0.1f);
    Check(!queue.Count() && !actions.Count() && services.deaths==2u,"native queue ResetFlags cancels without running children");
    (void)queue.Register(actions.Create(a,cause));queue.Clear();
    Check(!queue.Count() && !actions.Count() && !actions.OwnedAllocations() && !actions.OwnedTexts(),"native std::list queue cleanup frees all actual action storage");
    queued=nullptr;
    Reject([&]{skills.SetCombatEvent(0x1234u,0x40400000u);});
    Check(router.Unsubscribe(changed)&&router.Unsubscribe(state)&&router.Unsubscribe(dead),"QuickDead observer cleanup");
    skills.Close();actor.Destroy();cause_owner.Destroy();system.Destroy();
    std::cout<<"Native QuickDead: actual parent Start, event43 SET3, scheduled Monster544, shared task queue/std::list replacement, retry/cancel and balanced ownership pass\n";
}
void SnapshotOutcomes(const pal4::NativeMonsterRecordCatalog& monsters,pal4::NativeSkillRecordCatalog& original) {
    auto table=Skills();AddAction(table,91000u,9100u,1.0f,1.0f);
    auto& row=table.rows.back();
    const std::array params{1.f,1.f,0.5f,2.f,-50.f,0.f,43.f,3.f,2.f};
    for(unsigned i=0;i<params.size();++i)row[8u+i]=params[i];
    pal4::NativeSkillRecordCatalog catalog(table);
    pal4::NativeCombatActorOwner owner;const auto address=owner.Construct(monsters,2506);
    std::array<unsigned char,676u> before{};std::memcpy(before.data(),reinterpret_cast<void*>(address),before.size());
    pal4::ActorSkillActionSnapshot target,actor;
    std::memcpy(actor.primary.data(),before.data()+4u,actor.primary.size());
    std::memcpy(actor.conditions.data(),before.data()+340u,sizeof(actor.conditions));
    actor.conditions[1]=1000u;actor.conditions[58]=200u;actor.conditions[77]=0u;
    target=actor;target.conditions[58]=400u;
    pal4::RandomNumberGenerator random,expected;pal4::NativePlayerCombatEvents events;
    pal4::NativeSkillSystemOwner system(random,events);(void)system.Construct("H:/PAL4/objs");
    random.Seed(123u);expected.Seed(123u);(void)expected.Next();(void)expected.Next();
    pal4::ProcessNativeCombatSkillSnapshots(catalog.Address(91000u),target,actor,catalog,system);
    Check(actor.conditions[58]==150u && actor.conditions[77]==3u && target.conditions[58]==400u,
          "9100 two event records modify only original swapped snapshot receiver");
    Check(random.Next()==expected.Next(),"9100 preview consumes both real RNG rolls");
    Check(std::memcmp(before.data(),reinterpret_cast<void*>(address),before.size())==0,
          "9100 preview cannot mutate actual Monster properties");
    // Mode1 must read the copied BASIC/condition image, not an opaque host
    // snapshot object's layout or a live actor after the preview began.
    auto percent_actor=actor,percent_target=target;percent_actor.conditions[58]=200u;
    Word(catalog.Address(91000u),36u)=std::bit_cast<std::uint32_t>(0.5f);
    Word(catalog.Address(91000u),40u)=std::bit_cast<std::uint32_t>(1.f);
    pal4::ProcessNativeCombatSkillSnapshots(catalog.Address(91000u),percent_target,percent_actor,catalog,system);
    Check(percent_actor.conditions[58]==300u,"9100 mode1 reads copied HP before scaled increment");
    Word(catalog.Address(91000u),36u)=std::bit_cast<std::uint32_t>(-50.f);
    Word(catalog.Address(91000u),40u)=0u;
    // Check original-resource 9100 records separately from this synthetic boundary.
    unsigned checked{};
    for(std::size_t i=0;i<original.Size();++i) {
        const auto& record=original.At(i);if(record[4]!=9100u || record[14]!=0u)continue;
        auto a=actor,t=target;
        std::cout<<"Original snapshot9100 Skill="<<record[0]<<" events="<<std::bit_cast<float>(record[8])<<','<<std::bit_cast<float>(record[11])<<'\n';
        pal4::ProcessNativeCombatSkillSnapshots(original.Address(record[0]),t,a,original,system);++checked;
    }
    Check(checked>0u,"original Skill database includes actual9100 snapshot records");
    // Actual actor dispatch must use the same callback, but its Monster348
    // group service is distinct from the snapshots' empty base implementation.
    pal4::NativeScriptEventRouter router;pal4::NativeCombatSkillActorRegistry registry(router,catalog,system);
    pal4::NativeCombatActorOwner candidate_owner;const auto candidate_address=candidate_owner.Construct(monsters,2504);
    pal4::NativeCombatActorSkillRuntime real(owner,registry),candidate(candidate_owner,registry);
    (void)real.RebuildStats();(void)candidate.RebuildStats();
    Word(address,344u)=1000u;Word(candidate_address,344u)=1000u;
    (void)real.SetCurrentStat(7u,200);(void)candidate.SetCurrentStat(7u,400);
    const auto hp=real.CurrentStat(7u),candidate_hp=candidate.CurrentStat(7u);
    unsigned scans{};real.BindRelatedTargets([&](std::uint32_t a){
        Check(a==address,"9100 Monster group uses receiver");++scans;
        Reject([&]{real.BindRelatedTargets({});});
        return std::vector<std::uint32_t>{candidate_address};
    });
    random.Seed(123u);expected.Seed(123u);(void)expected.Next();(void)expected.Next();
    Check(real.TriggerForTarget(91000u,address),"actual9100 reaches shared outcome callback");
    std::cout<<"9100 actual HP="<<hp<<"->"<<real.CurrentStat(7u)<<" candidate="<<candidate_hp<<"->"<<candidate.CurrentStat(7u)<<" scans="<<scans<<" candidate type="<<Word(candidate_address,640u)<<'\n';
    Check(real.CurrentStat(7u)==hp-50 && candidate.CurrentStat(7u)==candidate_hp-25 && scans==1u,
          "actual9100 applies both records then eligible scaled records to borrowed related targets");
    Check(random.Next()==expected.Next(),"actual9100 preserves two shared RNG draws");
    candidate.Close();real.Close();candidate_owner.Destroy();
    // A rejected first probability consumes exactly one roll and changes no snapshot.
    auto failed_actor=actor,failed_target=target;
    Word(catalog.Address(91000u),20u)=std::bit_cast<std::uint32_t>(-1.f);
    random.Seed(123u);expected.Seed(123u);(void)expected.Next();
    pal4::ProcessNativeCombatSkillSnapshots(catalog.Address(91000u),failed_target,failed_actor,catalog,system);
    Check(failed_actor.primary==actor.primary && failed_actor.conditions==actor.conditions && random.Next()==expected.Next(),
          "9100 failed preview changes nothing and consumes only first roll");
    owner.Destroy();system.Destroy();
    std::cout<<"Native snapshot9100: shared event outcome, independent properties, swapped target, exact RNG and original records="<<checked<<" pass\n";
}
void SnapshotRecovery(const pal4::NativeMonsterRecordCatalog& monsters,pal4::NativeSkillRecordCatalog& original) {
    const auto record=original.Address(17007u);
    Check(record && Word(record,16u)==9118u,"original fire poison uses actual9118 leaf record");
    pal4::NativeCombatActorOwner owner;const auto address=owner.Construct(monsters,2503);
    std::array<unsigned char,676u> live{};std::memcpy(live.data(),reinterpret_cast<void*>(address),live.size());
    pal4::ActorSkillActionSnapshot initial;
    std::memcpy(initial.primary.data(),live.data()+4u,initial.primary.size());
    std::memcpy(initial.conditions.data(),live.data()+340u,sizeof(initial.conditions));
    initial.conditions[1]=100u;initial.conditions[58]=100u;
    initial.conditions[63]=std::bit_cast<unsigned>(1.f);
    initial.conditions[75]=0u;initial.conditions[77]=0u;
    auto target=initial,actor=initial;target.conditions[58]=40u;
    pal4::RandomNumberGenerator random,expected;pal4::NativePlayerCombatEvents events;
    pal4::NativeSkillSystemOwner system(random,events);(void)system.Construct("H:/PAL4/objs");
    random.Seed(123u);expected.Seed(123u);(void)expected.Next();
    pal4::ProcessNativeCombatSkillSnapshots(record,target,actor,original,system);
    Check(actor.conditions[58]==85u && target.conditions[58]==40u && random.Next()==expected.Next(),
          "original17007 snapshot applies15percent to swapped copied receiver with one real RNG draw");
    Check(std::memcmp(live.data(),reinterpret_cast<void*>(address),live.size())==0,
          "9118 preview leaves every actual Monster property byte unchanged");
    auto inactive=initial,inactive_other=target;inactive.conditions[75]=4u;
    random.Seed(123u);expected.Seed(123u);
    pal4::ProcessNativeCombatSkillSnapshots(record,inactive_other,inactive,original,system);
    Check(inactive.conditions[58]==100u && random.Next()==expected.Next(),"9118 snapshot type4 exits before RNG");
    auto dead=initial,dead_other=target;dead.conditions[77]=3u;
    random.Seed(123u);expected.Seed(123u);(void)expected.Next();
    pal4::ProcessNativeCombatSkillSnapshots(record,dead_other,dead,original,system);
    Check(dead.conditions[58]==100u && random.Next()==expected.Next(),"9118 snapshot state3 exits after one probability draw");
    owner.Destroy();system.Destroy();
    std::cout<<"Native snapshot9118: original17007 HP100->85, independent swapped copy, actual Monster unchanged, type4/no RNG and state3/one RNG pass\n";
}
void Actions(const pal4::NativeMonsterRecordCatalog& monsters,int id) {
    auto table=Skills();
    AddAction(table,1001u,9105u,0.25f,1.9f,1002u);
    AddAction(table,1002u,9109u,0.75f,0.0f,1003u);
    AddAction(table,1003u,9112u,0.33f,0.0f,1004u);
    AddAction(table,1004u,9113u,0.66f);
    AddAction(table,1005u,9109u,0.9f,0.0f,0u,2u,20u);
    AddAction(table,2000u,9001u,1.0f);
    for(unsigned i=0;i<5u;++i) AddAction(table,17006u+i,9109u,0.37f);
    pal4::NativeSkillRecordCatalog catalog(table);
    pal4::NativeCombatActorOwner actor; const auto a=actor.Construct(monsters,id);
    pal4::NativeScriptEventRouter router; pal4::RandomNumberGenerator random;
    pal4::NativePlayerCombatEvents events; pal4::NativeSkillSystemOwner system(random,events);
    (void)system.Construct("H:/PAL4/objs");
    pal4::NativeCombatActorSkillRuntime skills(actor,router,catalog,system);
    Check(Word(a,636u)==0u,"constructed actor has no followup chance");
    unsigned calls{},adds{},changes{},specials{},followups{},last_special{},last_followup{};
    const auto call=router.Subscribe(router.Key("ae_call_fun_id"),1u,[&](std::uint32_t){++calls;});
    const auto add=router.Subscribe(router.Key("ae_add_skill"),1u,[&](std::uint32_t){++adds;});
    const auto change=router.Subscribe(router.Key("me_attr_float_ext_changed"),1u,[&](std::uint32_t p){
        Check(Word(p)==a+716u && Word(p,4u)<=4u,"actual scalar change payload");
        const std::array before{Word(p),Word(p,4u),Word(p,8u)};
        Reject([&]{actor.Destroy();}); Reject([&]{skills.Close();});
        skills.Process(99u);
        Check(before==std::array{Word(p),Word(p,4u),Word(p,8u)},"nested scalar dispatch retains outer payload");
        ++changes;
    });
    const auto special=router.Subscribe(router.Key("ae_immunity_state_a"),1u,[&](std::uint32_t p){
        Check(Word(p)==a+716u && catalog.OwnsRecord(Word(p,4u)) && Word(p,8u)==a,"special original argument order");
        last_special=Word(p,12u); ++specials;
    });
    const auto followup=router.Subscribe(router.Key("ae_immunity_state_b"),1u,[&](std::uint32_t p){
        Check(Word(p)==a+716u && Word(p,4u)==catalog.Address(1001u) && Word(p,8u)==a,"followup original argument order");
        last_followup=Word(p,12u); ++followups;
    });
    Check(skills.AddRecord(1005u,0u),"timed context present for deep snapshot copy");
    random.Seed(123u); pal4::RandomNumberGenerator expected; expected.Seed(123u);
    Check(!skills.ApplyAction(123456u) && random.Next()==expected.Next(),"missing action does not consume RNG");
    Check(skills.ApplyAction(1001u),"actual Monster268 normal action path");
    Check(calls==4u && adds==2u && changes==4u,"snapshot preview emits no Monster events; actual four scalar callbacks run once");
    for(const auto [index,value]:std::array{std::pair{0u,0.33f},std::pair{1u,0.66f},std::pair{2u,0.25f},std::pair{4u,0.75f}})
        Check(Word(a,584u+4u*index)==std::bit_cast<unsigned>(value),"Monster scalar writes actual extended property");
    const auto unchanged=changes;
    skills.Process(1u);
    Check(changes==unchanged && calls==8u,"identical scalar writes suppress change events");
    const auto permanent_end=Word(a,700u),timed_end=Word(a,684u),calls_before=calls,adds_before=adds;
    std::array<unsigned,5u> scalar_before{};
    for(unsigned i=0;i<5u;++i) scalar_before[i]=Word(a,584u+4u*i);
    Word(a,636u)=7u;
    random.Seed(123u); expected.Seed(123u); (void)expected.Next();
    Check(!skills.ApplyAction(1001u) && followups==1u && last_followup==7u && random.Next()==expected.Next(),
          "original followup reads snapshot chance and consumes one real shared RNG draw");
    for(unsigned i=0;i<5u;++i) Check(scalar_before[i]==Word(a,584u+4u*i),"preview scalar writes stay in independent host snapshot");
    Check(changes==unchanged && calls==calls_before && adds==adds_before &&
          Word(a,700u)==permanent_end && Word(a,684u)==timed_end,"followup preserves both original lists and emits no actual chain/add");
    Word(a,636u)=1u; Word(a,512u)=std::bit_cast<unsigned>(1.0f);
    Check(!skills.ApplyAction(1001u) && followups==2u && last_followup==1u,"indexed secondary threshold uses live actor condition array");
    Word(a,636u)=0u;
    for(unsigned i=0;i<5u;++i) {
        Word(a,484u+4u*i)=std::bit_cast<unsigned>(1.0f);
        random.Seed(123u); expected.Seed(123u); (void)expected.Next();
        Check(!skills.ApplyAction(17006u+i) && specials==i+1u && last_special==i && random.Next()==expected.Next(),
              "five special indices use actual primary probabilities and original immunity event");
    }
    Word(a,484u)=0u; Word(a,640u)=4u;
    Check(!skills.ApplyAction(17006u) && specials==6u,"original state4 fallback follows failed primary roll");
    Word(a,640u)=0u;
    Check(skills.ApplyAction(17006u) && Word(a,600u)==std::bit_cast<unsigned>(0.37f),"failed special gates execute snapshot then actual collection path");
    const auto before_failure=Word(a,700u),add_before_failure=adds;
    Reject([&]{skills.ApplyAction(2000u);});
    Check(Word(a,700u)==before_failure && adds==add_before_failure,"unsupported preview fails before actual insertion and add event");
    for(const auto subscription:{call,add,change,special,followup}) Check(router.Unsubscribe(subscription),"unsubscribe action events");
    skills.Close(); Check(actor.OwnedAllocations()==5u,"snapshots and both original lists retire without actor allocation leak");
    actor.Destroy(); system.Destroy();
}
void Run(const pal4::NativeMonsterRecordCatalog& monsters,pal4::NativeSkillRecordCatalog& catalog,int id) {
    pal4::NativeCombatActorOwner actor;
    const auto a=actor.Construct(monsters,id);
    pal4::NativeScriptEventRouter router;
    pal4::RandomNumberGenerator random;
    pal4::NativePlayerCombatEvents property_events;
    pal4::NativeSkillSystemOwner system(random,property_events);
    (void)system.Construct("H:/PAL4/objs");
    pal4::NativeCombatActorSkillRuntime skills(actor,router,catalog,system);
    Reject([&]{pal4::NativeCombatActorSkillRuntime duplicate(actor,router,catalog,system);});
    Reject([&]{actor.Destroy();});
    (void)skills.RebuildStats();
    Check(actor.OwnedAllocations()==5u && skills.OwnedBuffers()==0u,"empty stats rebuild uses actual owner without list allocation");
    Check(!skills.AddRecord(123456u,a),"missing record leaves lists untouched");
    unsigned added{},removed{},calls{};
    const auto add=router.Subscribe(router.Key("ae_add_skill"),1u,[&](std::uint32_t p){
        Check(Word(p)==a+716u && Word(p,8u)==a,"actual Monster add payload");
        Reject([&]{actor.Destroy();}); Reject([&]{skills.Close();});
        ++added;
    });
    const auto call=router.Subscribe(router.Key("ae_call_fun_id"),1u,[&](std::uint32_t p){
        Check(Word(p)==a+716u,"actual Monster chain actor payload"); ++calls;
    });
    const auto remove=router.Subscribe(router.Key("ae_remove_skill"),1u,[&](std::uint32_t p){
        Check(Word(p)==a+716u,"actual Monster remove payload"); ++removed;
    });
    random.Seed(123u); pal4::RandomNumberGenerator expected; expected.Seed(123u);
    for(unsigned skill=1u;skill<=9u;++skill) Check(skills.AddRecord(skill,a),"collect both native list formats");
    Check(skills.OwnedBuffers()==2u && actor.OwnedAllocations()==7u && added==9u && calls>9u,
          "real growth retains only two buffers and chains through actual notifications");
    Check(Word(a,700u)-Word(a,696u)==40u && Word(a,684u)-Word(a,680u)==48u,
          "permanent8 and timed12 lists have actual record counts");
    Check(random.Next()==expected.Next(),"empty callback chains consume no shared RNG");
    Reject([&]{skills.AddRecord(1u,0x1234u);});
    const auto begin=Word(a,696u),end=Word(a,700u),cap=Word(a,704u);
    Word(a,696u)=0x1234u;
    Reject([&]{skills.Process(1u);});
    Word(a,696u)=begin;
    Word(a,700u)=cap+8u;
    Reject([&]{skills.Process(1u);});
    Word(a,700u)=end;
    const auto saved=Word(begin); Word(begin)=0x1234u;
    Reject([&]{skills.Process(1u);}); Word(begin)=saved;
    skills.Remove(1u);
    const auto after_filter=removed;
    skills.Advance(10u);
    Check(removed==after_filter+4u,"actual timer expiration emits four removal events");
    skills.Advance(0u);
    Check(Word(a,684u)==Word(a,680u) && Word(a,700u)-Word(a,696u)==32u,
          "shared dynamic compaction removes expired and filtered records");
    Check(skills.AddRecord(10u,a),"unbound effect collects under another trigger");
    Reject([&]{skills.Process(3u);});
    Check(Word(a,684u)-Word(a,680u)==12u,"failed chain retains already inserted record");
    const auto before_failed_add=Word(a,700u),add_events=added;
    Reject([&]{skills.AddRecord(11u,a);});
    Check(Word(a,700u)==before_failed_add+8u && added==add_events &&
          Word(Word(a,700u)-8u)==catalog.Address(11u),
          "AddRecord chain failure keeps insertion and does not fabricate later add event");
    const auto failing=router.Subscribe(router.Key("ae_remove_skill"),3u,[](std::uint32_t){throw std::logic_error("subscriber failure");});
    Reject([&]{skills.Close();});
    Check(Word(a,684u)==Word(a,680u) && skills.OwnedBuffers()==2u,
          "failed cleanup retains original pop-before-event mutation and owned storage");
    Check(router.Unsubscribe(failing),"remove failing subscriber");
    const auto retire_system=router.Subscribe(router.Key("ae_call_fun_id"),4u,[&](std::uint32_t){system.Destroy();});
    Reject([&]{skills.Process(1u);});
    Check(!system.Address(),"system retired inside synchronous actor event");
    Check(router.Unsubscribe(retire_system),"retire system-destruction callback");
    const auto before=Word(a,700u);
    Reject([&]{skills.AddRecord(3u,a);});
    Check(Word(a,700u)==before,"retired system rejects before collection mutation");
    skills.Close(); skills.Close();
    Check(skills.OwnedBuffers()==0u && actor.OwnedAllocations()==5u,"retry cleanup releases actor-owned buffers after system retirement");
    for(const auto o:{680u,684u,688u,696u,700u,704u}) Check(Word(a,o)==0u,"retired list headers zeroed");
    Check(router.Unsubscribe(add)&&router.Unsubscribe(call)&&router.Unsubscribe(remove),"retire subscriptions");
    Reject([&]{skills.RebuildStats();});
    actor.Destroy(); Check(actor.OwnedAllocations()==0u,"owner finally retires all actor allocations");
}
void OriginalRecords(const pal4::NativeMonsterRecordCatalog& monsters,pal4::NativeSkillRecordCatalog& catalog) {
    pal4::NativeCombatActorOwner actor; const auto a=actor.Construct(monsters,2579);
    pal4::NativeScriptEventRouter router;
    pal4::RandomNumberGenerator random; pal4::NativePlayerCombatEvents events;
    pal4::NativeSkillSystemOwner system(random,events); (void)system.Construct("H:/PAL4/objs");
    {
        pal4::NativeCombatActorSkillRuntime runtime(actor,router,catalog,system);
        Check(runtime.AddRecord(7432u,0u),"original7432 collects with original null-context chain early return");
        (void)runtime.RebuildStats();
        Check(runtime.OwnedBuffers()==1u,"actual original record storage owned");
    }
    Check(actor.OwnedAllocations()==5u,"RAII cleans original skill list");
    // A borrowed list must not be adopted or freed by a new runtime.
    std::array<std::uint32_t,2u> borrowed{};
    Word(a,696u)=reinterpret_cast<std::uint32_t>(borrowed.data());
    Reject([&]{pal4::NativeCombatActorSkillRuntime runtime(actor,router,catalog,system);});
    Word(a,696u)=0u;
    actor.Destroy(); system.Destroy();
}
void RecoveryActions(const pal4::NativeMonsterRecordCatalog& monsters) {
    auto table=Skills();
    for(unsigned id=81u;id<=84u;++id){
        AddAction(table,id,9118u,id==83u?-1.f:1.f);
        auto& row=table.rows.back();
        for(std::size_t n=0;n<table.fields.size();++n){const auto& field=table.fields[n].name;
            if(field=="SKILL_PARAM_4")row[n]=2.f;
            if(field=="SKILL_PARAM_5")row[n]=(id==82u || id==84u)?.25f:50.f;
            if(field=="SKILL_PARAM_6")row[n]=id==82u?4.f:id==84u?1.f:0.f;
            if(field=="SKILL_PARAM_7")row[n]=6.f;
            if(field=="SKILL_PARAM_8")row[n]=(id==82u || id==84u)?.5f:20.f;
            if(field=="SKILL_PARAM_9")row[n]=id==82u?4.f:id==84u?1.f:0.f;
        }
    }
    pal4::NativeSkillRecordCatalog catalog(table);pal4::NativeScriptEventRouter router;
    pal4::RandomNumberGenerator random;pal4::NativePlayerCombatEvents events;pal4::NativeSkillSystemOwner system(random,events);
    (void)system.Construct("H:/PAL4/objs");pal4::NativeCombatSkillActorRegistry registry(router,catalog,system);
    pal4::NativeCombatActorOwner first,second;const auto a=first.Construct(monsters,2579),b=second.Construct(monsters,2580);
    pal4::NativeCombatActorSkillRuntime source(first,registry),target(second,registry);
    (void)source.RebuildStats();(void)target.RebuildStats();
    Word(b,344u)=1000u;Word(b,352u)=200u;
    Word(b,592u)=std::bit_cast<unsigned>(1.5f);Word(b,596u)=std::bit_cast<unsigned>(.5f);
    (void)target.SetCurrentStat(7u,100);(void)target.SetCurrentStat(9u,10);
    Check(source.TriggerForTarget(81u,b) && target.CurrentStat(7u)==175 && target.CurrentStat(9u)==20,
          "actual9118 fixed HP/MP recovery uses target multipliers and native event setters");
    (void)target.SetCurrentStat(7u,500);(void)target.SetCurrentStat(9u,10);
    const auto percent=source.TriggerForTarget(82u,b);
    Check(percent && target.CurrentStat(7u)==875 && target.CurrentStat(9u)==60,
          "actual9118 percentage recovery reads original BASIC max fields");
    Check(source.TriggerForTarget(83u,b) && target.CurrentStat(7u)==875 && target.CurrentStat(9u)==60,
          "failed recovery probability leaves actual HP/MP unchanged");
    (void)target.SetCurrentStat(7u,995);(void)target.SetCurrentStat(9u,195);
    Check(source.TriggerForTarget(81u,b) && target.CurrentStat(7u)==1000 && target.CurrentStat(9u)==200,
          "native actor recovery clamps through real current-stat setters");
    (void)target.SetCurrentStat(7u,500);(void)target.SetCurrentStat(9u,10);
    Check(source.TriggerForTarget(84u,b) && target.CurrentStat(7u)==687 && target.CurrentStat(9u)==12,
          "mode1 scales current HP/MP including original float truncation, distinct from mode4 maximum");
    Reject([&]{(void)source.TriggerForTarget(81u,b+4u);});
    Check(!source.OwnedBuffers() && !target.OwnedBuffers(),"immediate recovery does not collect persistent Skill records");
    source.Close();target.Close();first.Destroy();second.Destroy();system.Destroy();
    std::cout<<"Native Combat recovery9118: fixed/percentage HP+MP, actual multipliers, probability rejection, clamping, foreign target and cleanup pass\n";
}

void CrossActorRouting(const pal4::NativeMonsterRecordCatalog& monsters) {
    auto table=Skills();
    AddAction(table,1001u,9109u,0.25f);
    AddAction(table,1002u,9112u,0.75f,0.0f,0u,2u,20u);
    pal4::NativeSkillRecordCatalog catalog(table);
    pal4::NativeScriptEventRouter router; pal4::RandomNumberGenerator random;
    pal4::NativePlayerCombatEvents events; pal4::NativeSkillSystemOwner system(random,events);
    (void)system.Construct("H:/PAL4/objs");
    pal4::NativeCombatSkillActorRegistry registry(router,catalog,system);
    pal4::NativeCombatActorOwner first,second,third,unregistered;
    const auto a=first.Construct(monsters,2579),b=second.Construct(monsters,2580),c=third.Construct(monsters,2580);
    (void)unregistered.Construct(monsters,2579);
    pal4::NativeCombatActorSkillRuntime source(first,registry),target(second,registry),other(third,registry);
    Check(registry.Count()==3u && a!=b && b!=c && registry.Contains(b),"three distinct original Monster routing identities");
    {
        // Record identity fixture: notification transports the borrowed pointer;
        // execution/cost/skill lookup are separately verified by their consumers.
        std::array<unsigned,38> magic{};magic[0]=5702u;
        const auto record=reinterpret_cast<unsigned>(magic.data());
        unsigned casts{};bool fail_cast{};
        const auto cast=router.Subscribe(router.Key("ae_cast_magic"),1u,[&](unsigned payload){
            Check(Word(payload)==a+716u && Word(payload,4u)==b && Word(payload,8u)==record,
                  "original Magic event transports target before borrowed record");
            Reject([&]{source.NotifyCastMagic(b,record);});Reject([&]{source.Close();});Reject([&]{target.Close();});
            ++casts;if(fail_cast)throw std::runtime_error("intentional Magic observer failure");
        });
        source.NotifyCastMagic(b,0u);source.NotifyCastMagic(0u,record);Check(!casts,"null Magic operands suppress notification");
        source.NotifyCastMagic(b,record);Check(casts==1u,"actual source516 notifies with original pair order");
        fail_cast=true;bool failed{};try{source.NotifyCastMagic(b,record);}catch(const std::runtime_error&){failed=true;}
        fail_cast=false;Check(failed,"Magic observer error propagates");source.NotifyCastMagic(b,record);
        Check(casts==3u,"Magic dispatch guard releases after observer error");
        source.NotifyReceivedMagic(b);source.NotifyReceivedMagic(a);
        Reject([&]{source.NotifyReceivedMagic(0u);});Reject([&]{source.NotifyCastMagic(b+4u,record);});
        Check(router.Unsubscribe(cast),"Magic observer retires");
        unsigned uses{};bool fail_property{};
        const auto use=router.Subscribe(router.Key("ae_use_property"),1u,[&](unsigned payload){
            Check(Word(payload)==a+716u && Word(payload,4u)==b && Word(payload,8u)==record,
                  "original Property event transports target before borrowed record");
            Reject([&]{source.NotifyUseProperty(b,record);});Reject([&]{source.Close();});Reject([&]{target.Close();});
            ++uses;if(fail_property)throw std::runtime_error("intentional Property observer failure");
        });
        source.NotifyUseProperty(b,0u);source.NotifyUseProperty(0u,record);Check(!uses,"null Property operands suppress notification");
        source.NotifyUseProperty(b,record);Check(uses==1u,"actual source524 notifies Property pair");
        fail_property=true;bool property_failed{};try{source.NotifyUseProperty(b,record);}catch(const std::runtime_error&){property_failed=true;}
        fail_property=false;Check(property_failed,"Property observer error propagates");source.NotifyUseProperty(b,record);
        Check(uses==3u,"Property dispatch guard releases after observer failure");
        const auto saved_property_state=Word(b,828u);Word(b,828u)=4u;target.NotifyReceivedProperty(a);
        Check(Word(b,828u)==4u,"same-type received Property preserves target primary state");
        target.NotifyReceivedProperty(b);Reject([&]{target.NotifyReceivedProperty(0u);});
        Word(b,828u)=saved_property_state;
        Reject([&]{source.NotifyUseProperty(b+4u,record);});
        Check(router.Unsubscribe(use),"Property observer retires");
        std::cout<<"Native Magic/Property notifications: original target/record event, property68/76, received property40/48, null/reentry/failure recovery pass; no cast/property execution claimed\n";
    }

    (void)source.RebuildStats(); (void)target.RebuildStats(); (void)other.RebuildStats();
    const auto b_before=Word(b,600u),c_before=Word(c,600u);
    std::vector<std::array<unsigned,4u>> calls;
    const auto call=router.Subscribe(router.Key("ae_call_fun_id"),1u,[&](unsigned p) {
        calls.push_back({Word(p),Word(p,4u),Word(p,8u),Word(p,12u)});
        Reject([&]{target.Close();}); Reject([&]{other.Close();});
        Reject([&]{second.Destroy();});
        Reject([&]{pal4::NativeCombatActorSkillRuntime extra(unregistered,registry);});
    });
    unsigned changes{};
    const auto changed=router.Subscribe(router.Key("me_attr_float_ext_changed"),1u,[&](unsigned){++changes;});
    Check(source.ApplyAction(1001u,b),"cross-actor actual Monster268 action");
    Check(Word(a,600u)==std::bit_cast<unsigned>(0.25f) && Word(b,600u)==b_before && Word(c,600u)==c_before,
          "Monster action applies actual scalar to collection actor, not context or third actor");
    Check(calls.size()==1u && calls.back()==std::array{b+716u,catalog.Address(1001u),a,9109u},
          "initial chain callfun routes to context B with target A and FUNCTION9109");
    Check(Word(Word(a,696u),4u)==b && target.OwnedBuffers()==0u,"only source stores permanent context B");
    source.Process(1u,c);
    Check(Word(c,600u)==std::bit_cast<unsigned>(0.25f) && Word(b,600u)==b_before,
          "batch explicit target C receives scalar from stored context B");
    Check(calls.back()==std::array{b+716u,catalog.Address(1001u),c,9109u},"batch original receiver/skill/target/FUNCTION payload order");
    Check(source.TriggerForTarget(1001u,b),"actual5897F0 direct target path");
    Check(Word(b,600u)==std::bit_cast<unsigned>(0.25f) && calls.back()==std::array{a+716u,catalog.Address(1001u),b,9109u},
          "direct target path emits on source A and changes B");
    Check(changes==3u && source.OwnedBuffers()==1u && target.OwnedBuffers()==0u,"preview emits no real changes; direct path inserts no skill list");
    Check(source.AddRecord(1002u,c),"foreign registered timed context accepted");
    source.Process(2u,b);
    Check(Word(b,584u)==std::bit_cast<unsigned>(0.75f) &&
          calls.back()==std::array{c+716u,catalog.Address(1002u),b,9112u},"timed list routes context and target independently");
    const auto end=Word(a,700u);
    Reject([&]{source.AddRecord(1001u,0x12345678u);});
    Reject([&]{source.ApplyAction(1001u,0x12345678u);});
    Reject([&]{source.Process(1u,0u);});
    Reject([&]{source.TriggerForTarget(1001u,unregistered.Address());});
    Check(Word(a,700u)==end,"unregistered address rejected before mutation");
    // A subscriber failure retains original insertion, including the context token.
    bool fail=true;
    const auto failing=router.Subscribe(router.Key("ae_add_skill"),1u,[&](unsigned){if(fail) throw std::logic_error("test cross-actor subscriber");});
    Reject([&]{other.AddRecord(1001u,a);}); fail=false;
    Check(other.OwnedBuffers()==1u && Word(Word(c,696u),4u)==a,"cross-actor failed insertion retains owned list/context");
    other.Process(1u,b);
    router.Unsubscribe(failing);
    // Closing B leaves the actor allocation alive. Re-registering that exact
    // address must not rebind A's old context to a different runtime generation.
    router.Unsubscribe(call);
    target.Close();
    Check(registry.Count()==2u && !registry.Contains(b),"close deregisters target without destroying actor");
    Reject([&]{source.Process(1u,c);});
    unsigned leave_events{};
    const auto leave_stage=router.Subscribe(router.Key("me_leave_stage"),1u,[&](unsigned p){
        Check(Word(p)==a+716u,"retiring receiver emits its actual facet");
        Reject([&]{source.Process(1u,c);}); // Stale contexts never become evaluable.
        Reject([&]{source.Close();}); // Leave notification still pins ownership.
        ++leave_events;
    });
    const auto leave_scene=router.Subscribe(router.Key("me_leave_scene"),1u,[&](unsigned p){
        Check(Word(p)==a+716u && (leave_events%2u)==1u,"stage precedes scene retirement notification");++leave_events;
    });
    const auto stale_buffer=Word(a,696u),stale_end=Word(a,700u);
    source.LeaveStageAndScene();
    Check(leave_events==2u && Word(a,696u)==stale_buffer && Word(a,700u)==stale_end,
          "leave-only retirement ignores foreign context lifetime without changing Skill lists");
    pal4::NativeCombatActorSkillRuntime replacement(second,registry);
    Check(registry.Count()==3u && registry.Contains(b),"same native address obtains new generation");
    Reject([&]{source.Process(1u,c);});
    Reject([&]{source.AddRecord(1001u,b);});
    Reject([&]{source.ApplyAction(1001u,b);});
    source.LeaveStageAndScene();
    Check(leave_events==4u,"leave-only retirement also permits a different generation at the old context address");
    router.Unsubscribe(leave_stage);router.Unsubscribe(leave_scene);
    source.Close(); // Cleanup does not dereference stale contexts.
    Check(source.OwnedBuffers()==0u && first.OwnedAllocations()==5u,"stale-context source cleanup releases both buffers");
    other.Close(); replacement.Close();
    Check(registry.Count()==0u,"every original Monster retired from routing domain");
    router.Unsubscribe(changed);
    first.Destroy(); second.Destroy(); third.Destroy(); unregistered.Destroy(); system.Destroy();
    Check(first.OwnedAllocations()==0u && second.OwnedAllocations()==0u && third.OwnedAllocations()==0u,
          "cross-actor lifecycle returns all native allocations");
}
void IndexedEvents(const pal4::NativeMonsterRecordCatalog& monsters) {
    auto table=Skills();
    for(unsigned i=0u;i<7u;++i) AddAction(table,3000u+i,9114u,50.75f+static_cast<float>(i),i%2u?-0.25f:0.125f);
    AddAction(table,3010u,9114u,50.0f,0.5f,3011u);
    AddAction(table,3011u,9114u,56.0f,-0.5f);
    AddAction(table,3012u,9114u,53.0f,0.25f,0u,2u,20u);
    AddAction(table,3013u,9114u,49.99f,99.0f);
    AddAction(table,3014u,9114u,std::numeric_limits<float>::quiet_NaN(),99.0f);
    AddAction(table,3015u,9114u,57.0f,99.0f);
    AddAction(table,3016u,9114u,50.0f,0.0f);
    pal4::NativeSkillRecordCatalog catalog(table);
    pal4::NativeScriptEventRouter router; pal4::RandomNumberGenerator random;
    pal4::NativePlayerCombatEvents events; pal4::NativeSkillSystemOwner system(random,events);
    (void)system.Construct("H:/PAL4/objs");
    pal4::NativeCombatSkillActorRegistry registry(router,catalog,system);
    pal4::NativeCombatActorOwner first,second;
    const auto a=first.Construct(monsters,2579),b=second.Construct(monsters,2580);
    pal4::NativeCombatActorSkillRuntime source(first,registry),target(second,registry);
    (void)source.RebuildStats(); (void)target.RebuildStats();
    std::vector<std::array<unsigned,3u>> changes;
    const auto changed=router.Subscribe(router.Key("me_race_attack_param_changed"),1u,[&](unsigned p) {
        const auto facet=Word(p),index=Word(p,4u);
        Check((facet==a+716u || facet==b+716u) && index<7u,"actual race event receiver and index");
        changes.push_back({facet,index,Word(p,8u)});
        Reject([&]{source.Close();}); Reject([&]{target.Close();});
        Reject([&]{first.Destroy();}); Reject([&]{second.Destroy();});
    });
    pal4::RandomNumberGenerator expected; random.Seed(17u); expected.Seed(17u);
    const auto source_before=Word(a,604u);
    for(unsigned i=0u;i<7u;++i) {
        const auto old=Word(b,604u+i*4u);
        Check(source.TriggerForTarget(3000u+i,b),"9114 original direct-target route");
        Check(Word(b,604u+i*4u)==std::bit_cast<unsigned>(std::bit_cast<float>(old)+(i%2u?-0.25f:0.125f)),
              "9114 seven race coefficients retain fractional signed addition");
        Check(changes.back()==std::array{b+716u,i,old},"9114 writes before actual old-value event");
        Check(changes.size()==i+1u,"one actual change per direct indexed action");
    }
    Check(Word(a,604u)==source_before && source.OwnedBuffers()==0u && target.OwnedBuffers()==0u,
          "direct indexed path changes only target and inserts no list");
    Check(random.Next()==expected.Next(),"indexed effect consumes no shared RNG draws");
    const auto count=changes.size();
    for(const auto id:{3013u,3014u,3015u,3016u}) Check(source.TriggerForTarget(id,b),"invalid/no-change indexed callback completes chain");
    Check(changes.size()==count,"invalid selectors and equal write emit no race change");
    const auto a0=Word(a,604u),a6=Word(a,628u),b0=Word(b,604u),b6=Word(b,628u);
    Check(source.ApplyAction(3010u,b),"Monster action previews and applies indexed attached chain");
    Check(Word(a,604u)==std::bit_cast<unsigned>(std::bit_cast<float>(a0)+0.5f) &&
          Word(a,628u)==std::bit_cast<unsigned>(std::bit_cast<float>(a6)-0.5f),"actual indexed chain effects collection A");
    Check(Word(b,604u)==b0 && Word(b,628u)==b6 && changes.size()==count+2u,
          "snapshot indexed writes stay independent and emit no actual events");
    const auto before_followup=Word(a,604u),before_changes=changes.size();
    Word(a,636u)=7u;
    Check(!source.ApplyAction(3000u,b),"indexed preview can exit on actual followup path");
    Check(Word(a,604u)==before_followup && changes.size()==before_changes,"discarded indexed preview leaves native coefficients and notifications untouched");
    Word(a,636u)=0u;
    Check(source.AddRecord(3012u,b),"timed indexed context added");
    const auto old3=Word(b,616u);
    source.Process(2u,b);
    Check(Word(b,616u)==std::bit_cast<unsigned>(std::bit_cast<float>(old3)+0.25f),"batch timed indexed callback reaches foreign Monster");
    source.Advance(20u);
    // Independent public actor property chain, including original invalid index behavior.
    const auto stable=Word(b,604u);
    bool fail=true;
    const auto failing=router.Subscribe(router.Key("me_race_attack_param_changed"),1u,[&](unsigned){if(fail) throw std::logic_error("indexed subscriber failure");});
    Reject([&]{source.TriggerForTarget(3000u,b);}); fail=false;
    Check(Word(b,604u)==std::bit_cast<unsigned>(std::bit_cast<float>(stable)+0.125f),"event exception retains preceding original property write");
    router.Unsubscribe(failing);
    Check(source.TriggerForTarget(3000u,b),"indexed target dispatch remains usable after callback exception");
    source.Close(); target.Close();
    Check(registry.Count()==0u && first.OwnedAllocations()==5u && second.OwnedAllocations()==5u,"indexed cleanup releases both list formats and registrations");
    router.Unsubscribe(changed);
    first.Destroy(); second.Destroy(); system.Destroy();
}
void OriginalIndexedEvents(const pal4::NativeMonsterRecordCatalog& monsters,pal4::NativeSkillRecordCatalog& catalog) {
    unsigned count{};
    for(std::size_t i=0;i<catalog.Size();++i) {
        const auto& root=catalog.At(i);
        if(root[4]!=9114u || root[2]!=1u || root[3]!=0xffffffffu) continue;
        Check(root[14]==0u,"original indexed passive has no attached chain");
        const auto index=static_cast<unsigned>(std::bit_cast<float>(root[5]));
        Check(index>=50u && index<=56u,"original indexed passive selector range");
        pal4::NativeScriptEventRouter router; pal4::RandomNumberGenerator random;
        pal4::NativePlayerCombatEvents events; pal4::NativeSkillSystemOwner system(random,events);
        (void)system.Construct("H:/PAL4/objs");
        pal4::NativeCombatSkillActorRegistry registry(router,catalog,system);
        pal4::NativeCombatActorOwner first,second;
        const auto a=first.Construct(monsters,2579),b=second.Construct(monsters,2580);
        pal4::NativeCombatActorSkillRuntime source(first,registry),target(second,registry);
        (void)source.RebuildStats(); (void)target.RebuildStats();
        const auto offset=604u+4u*(index-50u),before=Word(a,offset),other=Word(b,offset);
        Check(source.ApplyAction(root[0],b),"original9114 actual Monster268 succeeds");
        const auto expected=std::bit_cast<unsigned>(std::bit_cast<float>(before)+std::bit_cast<float>(root[6]));
        Check(Word(a,offset)==expected && Word(b,offset)==other,"original9114 exact race field and cross-actor direction");
        (void)source.RebuildStats();
        Check(Word(a,offset)==expected,"original9114 stats refresh reapplies passive without compounding");
        source.Close(); target.Close(); first.Destroy(); second.Destroy(); system.Destroy();
        ++count;
        std::cout<<"Original Monster indexed Skill="<<root[0]<<" event="<<index<<'\n';
    }
    Check(count>0u,"original indexed passive coverage");
}
void OriginalRandomActorEvents(const pal4::NativeMonsterRecordCatalog& monsters,const pal4::NativeGameDatabase& db) {
    pal4::NativeSkillRecordCatalog catalog(db);
    pal4::NativeCombatActorOwner actor;const auto a=actor.Construct(monsters,2587);
    bool found=false;
    for(const auto id:{7411u,17016u}){
        const auto record=catalog.Address(id);Check(record!=0u,"Money God passive exists in real Skill table");
        if(Word(record,16u)!=9133u)continue;
        found=true;std::cout<<"Original9133 Skill="<<id<<" threshold="<<std::bit_cast<float>(Word(record,20u))<<" candidates=";
        for(unsigned i=0;i<8u;++i){const auto candidate=static_cast<unsigned>(std::bit_cast<float>(Word(record,24u+4u*i)));const auto c=catalog.Address(candidate);std::cout<<candidate<<":"<<(c?Word(c,16u):0u)<<",";}std::cout<<'\n';
    }
    Check(found,"original database contains both selected9133 regression records");
    pal4::NativeScriptEventRouter router;pal4::RandomNumberGenerator random;
    pal4::NativePlayerCombatEvents events;pal4::NativeSkillSystemOwner system(random,events);
    (void)system.Construct("H:/PAL4/objs");
    pal4::NativeCombatActorSkillRuntime skills(actor,router,catalog,system);
    std::set<unsigned> effect_handles;unsigned next_effect{};
    pal4::NativeCombatSkillEffectServices effects;
    effects.create=[&](std::string_view resource,std::string_view){
        Check(!resource.empty(),"9133 candidate visual request names an original resource");
        effect_handles.insert(++next_effect);return pal4::ActorEffectHandle{next_effect,0x123u};
    };
    effects.active=[&](pal4::ActorEffectHandle h){return h.owner==0x123u && effect_handles.contains(h.id);};
    effects.release=[&](pal4::ActorEffectHandle h){Check(h.owner==0x123u && effect_handles.erase(h.id)==1u,"9133 candidate effect release owner");};
    skills.BindEffectServices(std::move(effects));
    (void)skills.RebuildStats();random.Seed(20261031u);
    for(const auto id:{7411u,17016u}){
        random.Seed(20261031u);
        Check(skills.TriggerForTarget(id,a),"actual original9133 dispatch succeeds");
        Check(random.Index()==2u,"real9133 consumes gate and selection MT draws exactly");
    }
    skills.Close();actor.Destroy();system.Destroy();
    // External visual lifetime belongs to the effect system, modeled here by
    // fixture handles only; this regression verifies dispatch and RNG.
    effect_handles.clear();
    std::cout<<"Original9133: actual actor dispatch and MT count pass\n";
}
void OriginalActions(const pal4::NativeMonsterRecordCatalog& monsters,pal4::NativeSkillRecordCatalog& catalog) {
    std::set<unsigned> checked_functions;
    std::set<unsigned> available_functions;
    for(std::size_t i=0;i<catalog.Size();++i) {
        const auto function=catalog.At(i)[4];
        if(function==9105u || function==9109u || function==9112u || function==9113u)
            available_functions.insert(function);
    }
    std::cout<<"Original scalar FUNCTIONs present:";
    for(const auto function:available_functions) std::cout<<' '<<function;
    std::cout<<'\n';
    for(std::size_t i=0;i<catalog.Size();++i) {
        const auto& root=catalog.At(i);
        if(checked_functions.contains(root[4]) ||
           (root[4]!=9105u && root[4]!=9109u && root[4]!=9112u && root[4]!=9113u)) continue;
        std::vector<unsigned> chain;
        std::set<unsigned> seen;
        auto cursor=catalog.Address(root[0]);
        bool supported=true;
        while(cursor) {
            if(!seen.insert(cursor).second) { supported=false; break; }
            const auto function=Word(cursor,16u);
            if(function!=9105u && function!=9109u && function!=9112u && function!=9113u &&
               function!=9101u && function!=9102u && function!=9103u && function!=9104u && function!=9120u && function!=9121u) {
                supported=false; break;
            }
            chain.push_back(cursor);
            const auto next=Word(cursor,56u);
            cursor=next?catalog.Address(next):0u;
        }
        if(!supported) continue;
        pal4::NativeCombatActorOwner actor; const auto a=actor.Construct(monsters,2579);
        pal4::NativeScriptEventRouter router; pal4::RandomNumberGenerator random;
        pal4::NativePlayerCombatEvents events; pal4::NativeSkillSystemOwner system(random,events);
        (void)system.Construct("H:/PAL4/objs");
        pal4::NativeCombatActorSkillRuntime skills(actor,router,catalog,system);
        (void)skills.RebuildStats();
        std::array<unsigned,5u> expected{};
        for(unsigned slot=0u;slot<expected.size();++slot) expected[slot]=Word(a,584u+4u*slot);
        for(const auto record:chain) {
            const auto function=Word(record,16u),value=Word(record,20u);
            if(function==9109u) expected[4]=value;
            if(function==9112u) expected[0]=value;
            if(function==9113u) expected[1]=value;
            if(function==9105u) {
                const auto selector=std::bit_cast<float>(Word(record,24u));
                if(std::isfinite(selector) && selector>=1.0f && selector<4.0f) {
                    const auto integer=static_cast<int>(selector);
                    if(integer==1 || integer==3) expected[integer==1?2u:3u]=value;
                }
            }
        }
        Check(skills.ApplyAction(root[0]),"actual original scalar Skill passes Monster268 normal path");
        if(root[2]!=1u) skills.Process(root[2]);
        for(unsigned slot=0u;slot<expected.size();++slot)
            Check(Word(a,584u+4u*slot)==expected[slot],"original scalar Skill and attached chain reach exact Monster fields");
        skills.Close(); actor.Destroy(); system.Destroy();
        checked_functions.insert(root[4]);
        std::cout<<"Original Monster action Skill="<<root[0]<<" FUNCTION="<<root[4]<<" chain="<<chain.size()<<'\n';
    }
    Check(available_functions.size()>=2u && checked_functions==available_functions,
          "every scalar callback type present in original Skill data has a verified original action");
}
}
int main(int argc,char** argv) {
    try {
        Check(argc==2,"original root required");
        pal4::NativeGameDatabase db; Check(db.Load(argv[1]),"load original database");
        pal4::NativeMonsterRecordCatalog monsters(db); pal4::NativeSkillRecordCatalog original(db);
        if(std::getenv("PAL4_COMBAT_PROPERTY_CATALOG_PROBE")){
            const auto& table=db.Tables().at("Property");
            std::cout<<"Property columns:";for(const auto& f:table.fields)std::cout<<' '<<f.name;std::cout<<'\n';
            for(const auto& row:table.rows){
                std::uint32_t id{},category{},ai{},target{},skill{};
                for(std::size_t n=0;n<table.fields.size();++n)if(const auto* v=std::get_if<std::uint32_t>(&row[n])){
                    const auto& f=table.fields[n].name;
                    if(f=="ID")id=*v;if(f=="TYPE")category=*v;if(f=="AI_CMD_TYPE")ai=*v;if(f=="TARGET")target=*v;if(f=="PA_ATTACHED_SKILL")skill=*v;
                }
                if(category)continue;
                const auto s=original.Address(skill);
                std::cout<<"Property original: ID="<<id<<" TYPE="<<category<<" AI="<<ai<<" TARGET="<<target<<" Skill="<<skill
                    <<" root="<<(s?Word(s,16u):0u)<<" next="<<(s?Word(s,56u):0u);
                if(s)std::cout<<" args="<<std::bit_cast<float>(Word(s,20u))<<','<<std::bit_cast<float>(Word(s,32u))<<','<<std::bit_cast<float>(Word(s,44u));
                std::cout<<'\n';
            }
            const auto extra=original.Address(17062u);if(extra){std::cout<<"Recovery chain17062:";for(unsigned n=0;n<132u;n+=4u)std::cout<<' '<<Word(extra,n);std::cout<<'\n';}
        }
        pal4::NativeSkillRecordCatalog catalog(Skills());
        {
            pal4::NativeScriptEventRouter router;pal4::RandomNumberGenerator random;
            pal4::NativePlayerCombatEvents events;pal4::NativeSkillSystemOwner system(random,events);
            (void)system.Construct("H:/PAL4/objs");
            pal4::NativeCombatActorOwner actor;(void)actor.Construct(monsters,2506);
            pal4::NativeCombatActorSkillRuntime skills(actor,router,original,system);
            (void)skills.RebuildStats();
            pal4::test::VerifyNativeCombatStatusChain(actor,skills,system,router,original);
            pal4::test::VerifyOriginalTimedCombatStatus(actor,skills,original);
            pal4::test::VerifyOriginalCombatHaste(actor,skills,system,router,original);
            pal4::test::VerifyNativeMonsterSlotConsume(skills,original,system);
            skills.SetCombatEvent(events.Lookup(40u),std::bit_cast<unsigned>(4.f));
            skills.Close();actor.Destroy();
            Check(!actor.OwnedAllocations(),"destroying still-wrapped actor releases entire owned chain");
            system.Destroy();
        }
        for(const auto id:{2579,2580,2580}) Run(monsters,catalog,id);
        for(const auto id:{2579,2580,2580}) Actions(monsters,id);
        SnapshotOutcomes(monsters,original);
        SnapshotRecovery(monsters,original);
        CombatCleanup(monsters,db);
        DeathLifecycle(monsters);
        QuickDeath(monsters,db);
        ActionProgressBinding(monsters,db);
        SkillDamageActions(monsters,db);
        DeathActionOwnership();
        FendOffSuccessorOwnership();
        pal4::test::VerifyNativeCombatTaskQueueDomains();
        TurnBoundary(monsters);
        RecoveryActions(monsters);
        CrossActorRouting(monsters);
        IndexedEvents(monsters);
        OriginalIndexedEvents(monsters,original);
        OriginalRecords(monsters,original);
        OriginalRandomActorEvents(monsters,db);
        OriginalActions(monsters,original);
        std::cout<<"Native Monster skill lists: actual allocation/growth, dispatch, stats, timers, failure recovery and cleanup pass\n";
        return 0;
    } catch(const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
}
