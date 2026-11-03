#include "pal4/native_combat_actor_skill_events.h"
#include "pal4/native_combat_actor_owner.h"
#include "pal4/native_script_event_router.h"
#include "pal4/native_monster_record_catalog.h"
#include "pal4/native_skill_record_catalog.h"
#include <array>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
void Check(bool v,const char* s) { if(!v) throw std::runtime_error(s); }
std::uint32_t& Word(std::uint32_t a,std::uint32_t o=0u) { return *reinterpret_cast<std::uint32_t*>(a+o); }
std::uint8_t& Byte(std::uint32_t a,std::uint32_t o) { return *reinterpret_cast<std::uint8_t*>(a+o); }
template<class F> void Reject(F&& f) {
    bool rejected{}; try { f(); } catch(const std::logic_error&) { rejected=true; }
    Check(rejected,"unsupported or invalid input must reject");
}
void Run(const pal4::NativeMonsterRecordCatalog& monsters,pal4::NativeSkillRecordCatalog& catalog,int id) {
    pal4::NativeCombatActorOwner actor;
    const auto a=actor.Construct(monsters,id);
    pal4::NativeScriptEventRouter router;
    pal4::NativeCombatActorSkillEvents events(actor,router,catalog);
    const auto skill=catalog.Address(7432u);
    Check(skill && catalog.OwnsRecord(skill),"original revive skill record");
    std::vector<std::string> trace;
    std::vector<std::uint32_t> subscriptions;
    subscriptions.push_back(router.Subscribe(router.Key("ae_triger_skill"),1u,[&](std::uint32_t p){
        const std::array expected{a+716u,skill,43u,42u,41u};
        for(unsigned i=0;i<expected.size();++i) Check(Word(p,4u*i)==expected[i],"original five-word trigger payload");
        Reject([&]{actor.Destroy();});
        // A nested dispatch may not overwrite the borrowed outer payload.
        events.InvokeVirtual(a,288u,std::array{51u,52u,53u});
        for(unsigned i=0;i<expected.size();++i) Check(Word(p,4u*i)==expected[i],"nested dispatch preserves trigger payload");
        trace.push_back("trigger");
    }));
    subscriptions.push_back(router.Subscribe(router.Key("ae_call_fun_id"),1u,[&](std::uint32_t p){
        Check(Word(p)==a+716u && Word(p,4u)==51u && Word(p,8u)==53u && Word(p,12u)==52u,"call-function argument order");
        trace.push_back("call");
    }));
    for(const auto name:{"ae_remove_mutex_skill","ae_remove_skill"})
        subscriptions.push_back(router.Subscribe(router.Key(name),1u,[&,name](std::uint32_t p){
            Check(Word(p)==a+716u && Word(p,4u)==skill && Byte(a,892u)==1u,"remove mutation precedes event");
            Reject([&]{actor.Destroy();});
            trace.push_back(name);
        }));
    events.InvokeVirtual(a,280u,std::array{skill,41u,42u,43u});
    Check(trace==std::vector<std::string>{"call","trigger"},"nested router dispatch order");
    events.InvokeVirtual(a,284u,std::array{skill,41u,42u,43u});
    events.InvokeVirtual(a,292u,std::array{51u,52u,53u});
    for(const auto slot:{308u,312u,316u,320u}) events.InvokeVirtual(a,slot,std::array{51u,52u,53u});
    Check(trace.size()==2u,"original empty callbacks and null damage link emit nothing");
    Byte(a,892u)=0u;
    events.InvokeVirtual(a,296u,std::array{skill});
    Byte(a,892u)=0u;
    events.InvokeVirtual(a,304u,std::array{skill},0x1234u);
    Check(trace==std::vector<std::string>{"call","trigger","ae_remove_mutex_skill","ae_remove_skill"},"real skill removals traverse empty native effect tree");
    // Reject malformed calls before mutation, including a foreign record and
    // a foreign receiver with a superficially correct vtable.
    const auto count=trace.size(); Byte(a,892u)=0u;
    Reject([&]{events.InvokeVirtual(a,296u,std::array{0x1234u});});
    Reject([&]{events.InvokeVirtual(a,304u,{});});
    Reject([&]{events.InvokeVirtual(a+4u,296u,std::array{skill});});
    Reject([&]{events.InvokeVirtual(a,276u,{});});
    Check(Byte(a,892u)==0u && trace.size()==count,"invalid calls do not mutate actor or emit");
    const auto head=Word(a,840u);
    Word(a,848u)=1u;
    Reject([&]{events.InvokeVirtual(a,304u,std::array{skill});});
    Check(Byte(a,892u)==1u && trace.size()==count,"unbound populated effect rejects after original dirty mutation before event");
    Word(a,848u)=0u;
    unsigned adds{};
    subscriptions.push_back(router.Subscribe(router.Key("ae_add_skill"),1u,[&](std::uint32_t p){
        Check(Word(p)==a+716u && Word(p,4u)==skill && Word(p,8u)==11u && Byte(a,892u)==1u,
              "add mutation precedes original three-word event");
        Reject([&]{actor.Destroy();});
        ++adds;
    }));
    events.InvokeVirtual(a,300u,std::array{11u,skill});
    Check(adds==1u && Word(a,840u)==head && actor.OwnedAllocations()==5u,
          "no child clump follows actual384 empty result and emits add without allocation");
    std::array<std::uint32_t,6u> child{};
    Word(a,808u)=reinterpret_cast<std::uint32_t>(child.data());
    events.InvokeVirtual(a,300u,std::array{11u,skill});
    Check(adds==2u,"child object with null clump retains original empty result");
    child[5]=0x1234u;
    Reject([&]{events.InvokeVirtual(a,300u,std::array{11u,skill});});
    Check(adds==2u && Word(a,840u)==head && actor.OwnedAllocations()==5u,
          "populated clump requires real camera/effect service before event");
    Word(a,808u)=0u; // borrowed child fixture must detach before owner cleanup.
    const auto failing=router.Subscribe(router.Key("ae_remove_mutex_skill"),3u,[](std::uint32_t){throw std::logic_error("subscriber failure");});
    Reject([&]{events.InvokeVirtual(a,296u,std::array{skill});});
    Check(Byte(a,892u)==1u,"event failure retains original mutation");
    Check(router.Unsubscribe(failing),"unsubscribe throwing callback");
    events.InvokeVirtual(a,304u,std::array{skill});
    for(const auto handle:subscriptions) Check(router.Unsubscribe(handle),"unsubscribe notifications");
    Check(router.SubscriptionCount()==0u,"no subscription leak");
    actor.Destroy(); Check(actor.OwnedAllocations()==0u,"dispatch guard recovers and allocations retire");
    Reject([&]{events.InvokeVirtual(a,296u,std::array{skill});});
}
}
int main(int argc,char** argv) {
    try {
        Check(argc==2,"original root required");
        pal4::NativeGameDatabase db; Check(db.Load(argv[1]),"load original database");
        pal4::NativeMonsterRecordCatalog monsters(db);
        pal4::NativeSkillRecordCatalog skills(db);
        for(const auto id:{2579,2580,2580}) Run(monsters,skills,id);
        std::cout<<"Original cave monsters: skill notification events, nested payloads and empty native effect removal pass\n";
        return 0;
    } catch(const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
}
