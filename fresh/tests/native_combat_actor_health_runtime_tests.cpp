#include "pal4/native_combat_actor_health_runtime.h"
#include "pal4/native_combat_actor_action_progress.h"
#include "pal4/native_combat_formula_catalog.h"
#include "pal4/native_combat_actor_skill_events.h"
#include "pal4/native_skill_system_owner.h"
#include "pal4/native_player_combat_events.h"
#include "pal4/random_number_generator.h"
#include "pal4/native_combat_actor_owner.h"
#include "pal4/native_monster_record_catalog.h"
#include "pal4/native_skill_record_catalog.h"
#include "pal4/native_script_event_router.h"
#include "pal4/native_combat_console.h"
#include "pal4/native_base_object_memory.h"
#include "pal4/script_skill_collection.h"
#include <array>
#include <bit>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
void Check(bool value,const char* message) { if(!value) throw std::runtime_error(message); }
std::uint32_t& Word(std::uint32_t a,std::uint32_t offset=0u) {
    return *reinterpret_cast<std::uint32_t*>(a+offset);
}
template<class F> void Reject(F&& f) {
    bool rejected=false; try { f(); } catch(const std::logic_error&) { rejected=true; }
    Check(rejected,"expected explicit rejection");
}
// Skill chain effects remain explicitly unbound. SkillSystem initialization,
// health, notifications, native stats and console subscriptions are real.
class Skills final : public pal4::IScriptSkillCollectionRuntime {
public:
    pal4::NativeCombatActorOwner& actor;
    pal4::NativeSkillRecordCatalog& catalog;
    pal4::NativeCombatActorSkillEvents& notifications;
    pal4::NativeSkillSystemOwner& system;
    pal4::NativeCombatActorHealthRuntime* health{};
    unsigned touches{},rebuilds{},removed{};
    bool fail_touch{};
    Skills(pal4::NativeCombatActorOwner& a,pal4::NativeSkillRecordCatalog& c,pal4::NativeCombatActorSkillEvents& n,
           pal4::NativeSkillSystemOwner& s) : actor(a),catalog(c),notifications(n),system(s) {}
    [[noreturn]] static void Unexpected() { throw std::logic_error("unbound test skill effect"); }
    std::uint32_t ReadWord(std::uint32_t a) override { return Word(a); }
    void WriteWord(std::uint32_t a,std::uint32_t v) override { Word(a)=v; }
    void WriteByte(std::uint32_t a,std::uint8_t v) override { *reinterpret_cast<std::uint8_t*>(a)=v; }
    std::uint32_t Allocate(std::uint32_t) override { Unexpected(); }
    void Free(std::uint32_t) override { Unexpected(); }
    void InitializeOuterBase(std::uint32_t) override { Unexpected(); }
    void InitializeInnerBase(std::uint32_t) override { Unexpected(); }
    std::uint32_t GameDatabase() override { Unexpected(); }
    std::uint32_t FindSkillRecord(std::int32_t id) override { return catalog.Address(static_cast<std::uint32_t>(id)); }
    std::uint32_t ResolveSkillRecord(std::uint32_t record) override { return catalog.ResolveRecord(record); }
    void UpdateHealthStatus(std::uint32_t a) override {
        Check(a==actor.Address(),"skill health receiver"); (void)health->Update();
    }
    std::uint32_t SkillSystemInstance() override {
        ++touches;
        if(fail_touch) Unexpected();
        return system.Address();
    }
    void ProcessSkillChain(std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t) override {
        Unexpected();
    }
    std::uint32_t InvokeVirtual(std::uint32_t a,std::uint32_t slot,
                                std::span<const std::uint32_t> args) override {
        Check(a==actor.Address(),"skill callback receiver");
        if(slot==276u) { Check(args.empty(),"rebuild arguments"); ++rebuilds; return actor.RebuildStats(*health,*this); }
        if(slot==296u || slot==304u) {
            Check(args.size()==1u && catalog.OwnsRecord(args[0]),"actual removed skill record");
            notifications.InvokeVirtual(a,slot,args);
            ++removed; return 0u;
        }
        Unexpected();
    }
};

void Run(const pal4::NativeMonsterRecordCatalog& monsters,pal4::NativeSkillRecordCatalog& catalog,int id,
         pal4::NativeSkillSystemOwner& system,const pal4::NativeCombatFormulaCatalog& formulas) {
    pal4::NativeCombatActorOwner actor;
    const auto a=actor.Construct(monsters,id);
    pal4::NativeScriptEventRouter router;
    pal4::NativeCombatActorSkillEvents notifications(actor,router,catalog);
    Skills skills(actor,catalog,notifications,system);
    pal4::NativeCombatActorHealthRuntime health(actor,router,skills);
    skills.health=&health;
    // Original-property image with deterministic HP maxima for threshold probes.
    Word(a,8u)=Word(a,344u)=100u;
    Word(a,572u)=100u; Word(a,648u)=0u; Word(a,652u)=0u;
    std::vector<std::string> trace;
    std::vector<std::uint32_t> subscriptions;
    subscriptions.push_back(router.Subscribe(router.Key("me_state_changed"),1u,[&](std::uint32_t p){
        Check(Word(p)==a+716u,"state payload facet");
        trace.push_back("state:"+std::to_string(Word(p,4u))+":"+std::to_string(Word(p,8u)));
        Reject([&]{actor.Destroy();}); // Protected across synchronous callbacks.
    }));
    subscriptions.push_back(router.Subscribe(router.Key("me_attr_int_ext_changed"),1u,[&](std::uint32_t p){
        Check(Word(p)==a+716u,"attribute payload facet");
        trace.push_back("attr:"+std::to_string(Word(p,4u))+":"+std::to_string(Word(p,8u))+":"+std::to_string(Word(p,12u)));
    }));
    for(const auto name: {"ae_state_changed","ae_become_alive","ae_revive"})
        subscriptions.push_back(router.Subscribe(router.Key(name),1u,[&,name](std::uint32_t p){
            Check(Word(p)==a+716u,"actor event facet"); trace.push_back(name);
        }));
    pal4::NativeBaseObjectMemory console_memory;
    pal4::NativeCombatConsole console(console_memory,router);
    (void)console.Construct(); console.Subscribe();
    unsigned console_events{};
    console.BindDispatch([&](std::uint32_t c,std::uint32_t entry,std::uint32_t p){
        Check(c==console.Address() && entry==0x5404b0u && Word(p)==a+716u,"actual console attribute subscription");
        ++console_events;
    });
    Check(health.Update()==0 && trace.empty(),"unchanged health emits no event");
    std::vector<std::array<std::uint32_t,3>> element_events;
    subscriptions.push_back(router.Subscribe(router.Key("me_fundamental_elements_changed"),1u,[&](std::uint32_t p){
        element_events.push_back({Word(p),Word(p,4u),Word(p,8u)});
        Reject([&]{actor.Destroy();});
    }));
    const auto old_element=Word(a,464u),new_element=old_element+1u;
    Check(health.SetElementViaActor(0u,std::bit_cast<std::int32_t>(new_element))==std::bit_cast<std::int32_t>(new_element) &&
        Word(a,464u)==new_element && element_events==std::vector<std::array<std::uint32_t,3>>{{a+716u,0u,old_element}},
        "actual actor32/property132 writes element and dispatches original previous-value payload under pin");
    (void)health.SetElementViaActor(0u,std::bit_cast<std::int32_t>(new_element));
    Check(health.SetElementViaActor(5u,1)==5 && element_events.size()==1u,
        "equal and out-of-range element writes retain original no-event gate");
    (void)health.SetElementViaActor(0u,std::bit_cast<std::int32_t>(old_element));
    Check(element_events.size()==2u && element_events.back()==std::array{a+716u,0u,new_element},
        "element restoration uses same original property event route");
    Check(health.SetCurrent(7u,20)==20 && Word(a,648u)==2u,"HP setter updates actual health state");
    Check(trace==std::vector<std::string>{"state:4:0","attr:7:100:20"},"wounded state/attribute order");
    trace.clear();
    Check(health.SetCurrent(7u,0)==0 && Word(a,648u)==3u,"zero HP state");
    Check(trace==std::vector<std::string>{"state:4:2","attr:7:20:0"},"zero HP events");
    trace.clear();
    Check(health.SetCurrent(7u,100)==100 && Word(a,648u)==0u && Word(a,828u)==1u,"restored actor becomes alive");
    Check(trace==std::vector<std::string>{"state:4:3","ae_state_changed","ae_become_alive","attr:7:0:100"},
          "recovery follows original event sequence");
    Check(skills.touches==1u && console_events==3u,"actual event routing and singleton touch boundary");
    trace.clear();
    Check(health.Set(a,0xb4u,1)==1 && Word(a,652u)==1u,"revive permission property");
    trace.clear();
    Check(health.SetCurrent(7u,0)==100 && Word(a,652u)==0u && Word(a,648u)==0u,"revive restores max HP");
    Check(trace==std::vector<std::string>{"state:5:1","attr:7:0:100","ae_revive","attr:7:100:0"},
          "revive nested requested/current HP and event order");
    Check(skills.rebuilds==1u && actor.OwnedAllocations()==5u,"revive7432 cleanup triggers actual native rebuild");
    trace.clear();
    Check(actor.RebuildStats(health,skills)==0 && actor.OwnedAllocations()==5u,"native stats uses real health adapter");
    const auto saved_action_state=Word(a,660u),saved_ready_state=Word(a,828u);
    Word(a,660u)=0u;
    for (const auto value:{1,2,0}) {
        const auto previous=Word(a,660u);
        trace.clear();
        Check(health.Set(a,196u,value)==value && health.Query(a,192u)==value,
              "action-state property getter/setter");
        Check(trace==std::vector<std::string>{"state:7:"+std::to_string(previous)},
              "action-state event preserves previous value");
        Check(Word(a,828u)==saved_ready_state,"action-state does not overwrite facet readiness");
        trace.clear();
        Check(health.Set(a,196u,value)==value && trace.empty(),"same action-state emits nothing");
    }
    for (const auto invalid:{3,-1}) {
        trace.clear();
        Check(health.Set(a,196u,invalid)==0 && Word(a,660u)==0u,
              "out-of-range action-state retains original value");
        Check(trace==std::vector<std::string>{"state:7:0"},
              "original invalid action-state still emits change attempt");
    }
    const auto throwing=router.Subscribe(router.Key("me_state_changed"),1u,[&](std::uint32_t p){
        if(Word(p,4u)==7u) throw std::logic_error("action-state observer");
    });
    Reject([&]{(void)health.Set(a,196u,2);});
    Check(Word(a,660u)==2u,"action-state writes before throwing observer");
    Check(router.Unsubscribe(throwing),"remove throwing action-state observer");
    Word(a,660u)=saved_action_state;
    trace.clear();
    // Exercise the shared7432 filter with both real list formats. Their storage
    // is borrowed for this call only; production list allocation is still open.
    const auto revive_record=catalog.Address(7432u);
    Check(revive_record!=0u,"original7432 record exists");
    std::array<std::uint32_t,3u> timed{revive_record,a,0u};
    std::array<std::uint32_t,2u> persistent{revive_record,a};
    struct BorrowLists {
        std::uint32_t actor;
        ~BorrowLists() { for(const auto offset:{680u,684u,688u,696u,700u,704u}) Word(actor,offset)=0u; }
    };
    unsigned native_removals{};
    const auto removal=router.Subscribe(router.Key("ae_remove_skill"),1u,[&](std::uint32_t p){
        Check(Word(p)==a+716u && Word(p,4u)==revive_record,"7432 filter emits real Monster removal event");
        ++native_removals;
    });
    {
        BorrowLists borrowed{a};
        Word(a,680u)=reinterpret_cast<std::uint32_t>(timed.data());
        Word(a,684u)=Word(a,688u)=Word(a,680u)+12u;
        Word(a,696u)=reinterpret_cast<std::uint32_t>(persistent.data());
        Word(a,700u)=Word(a,704u)=Word(a,696u)+8u;
        const auto rebuilt=skills.rebuilds;
        (void)pal4::ScriptSkillCollection::FilterAndCleanData(a,7432u,skills);
        Check(!timed[0] && !persistent[0] && native_removals==2u && skills.removed==2u && skills.rebuilds==rebuilt+1u,
              "both7432 lists clear then dispatch native notifications before actual stat rebuild");
    }
    Check(router.Unsubscribe(removal),"remove7432 event observer");
    trace.clear();
    const auto damage=Word(a,800u);
    Word(a,800u)=0x1234u;
    Reject([&]{(void)health.Update();});
    Check(trace.empty(),"foreign property rejects before events"); Word(a,800u)=damage;
    // Synchronous nested dispatch must preserve the outer four-word record.
    const auto nested=router.Subscribe(router.Key("me_attr_int_ext_changed"),3u,[&](std::uint32_t p){
        const std::array before{Word(p),Word(p,4u),Word(p,8u),Word(p,12u)};
        if(Word(p,4u)==8u) (void)health.SetCurrent(9u,1);
        Check(before==std::array{Word(p),Word(p,4u),Word(p,8u),Word(p,12u)},"outer event record survives nesting");
    });
    (void)health.SetCurrent(8u,5);
    Check(router.Unsubscribe(nested),"remove nested handler");
    const auto failing=router.Subscribe(router.Key("me_state_changed"),3u,[](std::uint32_t){throw std::logic_error("event failure");});
    Word(a,572u)=0u;
    Reject([&]{(void)health.Update();});
    Check(Word(a,648u)==3u,"event failure preserves original mutation timing");
    Check(router.Unsubscribe(failing),"remove failed handler");
    Word(a,572u)=100u; skills.fail_touch=true;
    Reject([&]{(void)health.Update();});
    // State648 changes before BecomeAlive asks for the service; retry explicitly
    // invokes the original548 entry after repairing the missing service.
    skills.fail_touch=false;
    (void)health.BecomeAlive();
    {
        pal4::NativeCombatActorActionProgress progress(actor,health,formulas,router,system);
        const auto old_primary=Word(a,668u),old_secondary=Word(a,828u);
        const auto old_stat=Word(a,364u);
        const auto stat=health.QueryIndexed(a,48u,5u);
        const auto raw=static_cast<double>(formulas.FloatAt(10))-static_cast<double>(formulas.FloatAt(11))*static_cast<float>(stat);
        const auto expected=static_cast<float>(64.0/(raw<0.0?static_cast<double>(0.01f):raw));
        Check(std::fabs(progress.Rate()-expected)<=std::fabs(expected)*0.00001f,"real original formula and actor stat5 rate");
        std::cout<<"Original monster"<<id<<" action rate="<<progress.Rate()<<" stat5="<<stat<<'\n';
        unsigned movements{};bool nested_progress{};
        const auto movement=router.Subscribe(router.Key("me_moved_at_action_sequence"),3u,[&](std::uint32_t p){
            Check(Word(p)==a+716u,"actual action-progress event facet");
            const auto before=Word(p);++movements;
            Reject([&]{actor.Destroy();});
            if(nested_progress){nested_progress=false;progress.SetProgress(7.0f);}
            Check(Word(p)==before,"outer action-progress payload survives nesting");
        });
        Word(a,668u)=0u;Word(a,828u)=1u;
        Check(progress.Ready(),"facet readiness reads actor828, not actor112/668");
        for(unsigned state=0;state<6u;++state){
            Word(a,668u)=state;const auto before=movements;
            progress.SetProgress(static_cast<float>(state)+0.25f);
            Check(progress.Progress()==static_cast<float>(state)+0.25f && movements==before+1u,"primary states0..5 accept progress and emit event");
        }
        for(const auto state:{6u,0xffffffffu}){
            Word(a,668u)=state;const auto before=movements,previous=Word(a,720u);
            progress.SetProgress(60.0f);
            Check(Word(a,720u)==previous && movements==before,"out-of-range primary state rejects movement without event");
        }
        Word(a,668u)=0u;nested_progress=true;progress.SetProgress(9.0f);
        Check(progress.Progress()==7.0f,"nested actual progress setter");
        const auto before=movements;progress.Prepare();
        Check(progress.Progress()==0.0f && movements==before,"preparation resets progress without movement event");
        Word(a,828u)=2u;Check(!progress.Ready(),"secondary state2 is not facet state-one");
        Word(a,364u)=0x7fffffffu;
        if(formulas.FloatAt(11)>0.0f)Check(std::fabs(progress.Rate()-6400.0f)<0.01f,"negative denominator uses original0.01 clamp");
        Word(a,364u)=old_stat;
        const auto failure=router.Subscribe(router.Key("me_moved_at_action_sequence"),1u,[](std::uint32_t){throw std::logic_error("progress observer failure");});
        Reject([&]{progress.SetProgress(11.0f);});
        Check(progress.Progress()==11.0f,"observer failure retains original write-before-event timing");
        Check(router.Unsubscribe(failure),"retire throwing progress observer");
        progress.Prepare();Check(router.Unsubscribe(movement),"retire progress observer");
        Word(a,668u)=old_primary;Word(a,828u)=old_secondary;
    }
    console.Destroy();
    Check(console_memory.OutstandingAllocations()==0u,"console allocations retired");
    for(const auto subscription:subscriptions) Check(router.Unsubscribe(subscription),"unsubscribe health observer");
    Check(router.SubscriptionCount()==0u,"all subscriptions retired");
    actor.Destroy();
    Check(actor.OwnedAllocations()==0u,"native actor allocations retired");
    Reject([&]{(void)health.Update();});
}
}
int main(int argc,char** argv) {
    try {
        Check(argc==2,"original root required");
        pal4::NativeGameDatabase db;
        Check(db.Load(argv[1]),"load original database");
        pal4::NativeMonsterRecordCatalog monsters(db);
        pal4::NativeSkillRecordCatalog skills(db);
        pal4::NativeCombatFormulaCatalog formulas(db);
        const auto& formula_rows=db.Tables().at("CombatFormulaConfig").rows;
        for(const auto& row:formula_rows){
            const auto key=std::get<std::uint32_t>(row[0]);
            if(key>=1u && key<=32u)Check(formulas.FloatAt(static_cast<std::int32_t>(key-1u))==std::get<float>(row[1]),"all32 keyed original formula values");
        }
        Check(formulas.FloatAt(-1)==0.0f && formulas.FloatAt(32)==0.0f,"formula signed bounds");
        std::cout<<"Original formula10="<<formulas.FloatAt(10)<<" formula11="<<formulas.FloatAt(11)<<" prepare24="<<formulas.FloatAt(24)<<" prepare25="<<formulas.FloatAt(25)<<" warnings="<<formulas.Warnings().size()<<'\n';
        pal4::RandomNumberGenerator random;
        pal4::NativePlayerCombatEvents property_events;
        pal4::NativeSkillSystemOwner system(random,property_events);
        (void)system.Construct("H:/PAL4/objs");
        for(const auto id:{2579,2580,2580}) Run(monsters,skills,id,system,formulas);
        system.Destroy(); Check(system.OwnedAllocations()==0u,"actual SkillSystem cleanup");
        std::cout<<"Original cave monsters: native health/events/alive/revive and real SkillSystem initialization pass; skill chain effects unbound\n";
        return 0;
    } catch(const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
}
