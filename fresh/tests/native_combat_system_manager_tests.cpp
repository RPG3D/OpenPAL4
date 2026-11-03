#include "pal4/native_combat_system_manager.h"
#include "pal4/native_script_event_router.h"
#include <array>
#include <cmath>
#include <functional>
#include <iostream>
#include <limits>
#include <map>
#include <stdexcept>
#include <vector>

namespace {
void Check(bool b,const char* m) { if (!b) throw std::runtime_error(m); }
template<class F> void Reject(F f) {
    bool rejected{}; try { f(); } catch (const std::logic_error&) { rejected=true; }
    Check(rejected,"expected lifecycle/input rejection");
}
// Explicit protocol fixture, not the live game roster or action implementation.
class Services final : public pal4::INativeCombatSystemManagerServices {
public:
    std::map<std::uint32_t,float> rates{{101,1.0f},{102,2.0f}}, properties{{101,0.0f},{102,0.0f}};
    std::map<std::uint32_t,std::size_t> positions;
    std::vector<std::uint32_t> prepared,started;
    std::uint32_t state{1u},group_argument{},released{},draws{},touches{};
    bool inactive{},ready{true},throw_ready{};
    std::function<void()> on_set;
    std::uint32_t ReadWord(std::uint32_t a) override { Check(a==900u,"prepare group pointer"); return group_argument; }
    void WriteWord(std::uint32_t,std::uint32_t) override { throw std::logic_error("unexpected foreign write"); }
    std::uint8_t ReadByte(std::uint32_t) override { throw std::logic_error("unexpected foreign byte"); }
    std::uint32_t DialogDataFromParam(std::uint32_t e) override { return InternalObject(e); }
    std::uint32_t InternalObject(std::uint32_t e) override { return e>=100u?1000u+e:e==1?101u:e==2?102u:0u; }
    std::uint32_t CombatSystemAddress() override { return 0x7000u; }
    bool CombatInactive() override { return inactive; }
    float ObjectRate(std::uint32_t o) override { return rates.at(o); }
    float ObjectProperty(std::uint32_t o) override { return properties.at(o); }
    void SetObjectProperty(std::uint32_t o,float v) override { if(on_set)on_set(); properties.at(o)=v; }
    std::uint32_t CombatState() override { return state; }
    void SetCombatState(std::uint32_t s) override { state=s; }
    void TouchSkillSystem() override { ++touches; }
    void TouchGlobalSystem() override {}
    bool ObjectReady(std::uint32_t) override { return ready; }
    std::uint32_t BeginActors(std::uint32_t g) override { Check(g<2u,"two groups");positions[g+1u]=0;return g+1u; }
    bool IteratorFinished(std::uint32_t i) override { return positions.at(i)!=0u; }
    std::uint32_t IteratorCurrent(std::uint32_t i) override { return 100u+i; }
    void IteratorAdvance(std::uint32_t i) override { ++positions.at(i); }
    std::uint8_t ActorReady(std::uint32_t) override { if(throw_ready)throw std::runtime_error("readiness error");return ready?1u:0u; }
    void ReleaseActorIterator(std::uint32_t) override { ++released; }
    void PauseManager(std::uint32_t) override { throw std::logic_error("manager owns pause"); }
    void StartObject(std::uint32_t o) override { started.push_back(o); }
    void InvokeManagerSlot4(std::uint32_t) override { throw std::logic_error("manager owns slot4"); }
    std::uint32_t ReleaseEvents(const std::vector<std::uint32_t>&) override { throw std::logic_error("host owns event vector"); }
    void PrepareObject(std::uint32_t o) override { prepared.push_back(o); }
    float GameDbFloat(std::uint32_t i) override { Check(i==24u || i==25u,"original game float indexes"); return i==24u?0.2f:0.8f; }
    std::uint32_t RandomNumber() override { ++draws;return 500u; }
    std::uint32_t ObjectGroup(std::uint32_t o) override { return o==101u?1u:2u; }
};
}
int main() { try {
    pal4::NativeScriptEventRouter router;
    Services services;
    const auto unrelated=router.Subscribe(router.Key("unrelated"),1u,[](std::uint32_t){});
    {
        pal4::NativeCombatSystemManager manager(router,services);
        Check(manager.Address() && !manager.Active() && manager.Objects().empty() && manager.SubscriptionCount()==13u && router.SubscriptionCount()==14u,"owned manager constructor");
        auto event=[&](const char* name,std::uint32_t e=0u){Check(router.Dispatch(name,e)==1u,"named SystemManager callback");};
        for(unsigned i=0;i<3u;++i)manager.Subscribe();
        Check(router.SubscriptionCount()==14u,"replacement subscription no leaks");
        event("re_join_stage",1u);event("me_join_stage",2u);event("ae_become_alive",1u);
        Check(manager.Objects().size()==2u && manager.Objects()[0]==101u && manager.Objects()[1]==102u && services.prepared.size()==3u,"join/alive duplicate suppression after preparation");
        event("ae_become_alive",0u);Check(manager.Objects().size()==2u,"null internal object is skipped");
        for(const char* name: {"ae_become_dead","me_leave_stage","re_leave_stage"}) {
            event(name,1u);Check(manager.Objects().size()==1u && manager.Objects()[0]==102u,"remove callback shifts host vector");
            event("re_join_stage",1u);
        }
        services.properties[101u]=63.0f;services.properties[102u]=62.0f;
        (void)manager.Update(0.001f);
        Check(manager.Active() && manager.Objects().size()==2u && services.started.empty(),
              "preparation readiness pauses original slot0 and retains both scheduling facets");
        event("combat_start");
        services.state=2u;
        services.properties[101u]=63.0f;services.properties[102u]=62.0f;
        (void)manager.Update(0.001f);
        Check(services.properties[101u]==64.0f && services.properties[102u]==64.0f && services.started.back()==102u,"real frame uses milliseconds clamp and fastest-ready selection");
        event("combat_over");Check(manager.Active(),"slot0 pauses action sequence");
        const auto started=services.started.size();(void)manager.Update(10.0f);
        Check(services.started.size()==started,"active gate blocks update");
        event("combat_start");Check(!manager.Active(),"slot4 resumes action sequence");
        services.properties[101u]=0;services.properties[102u]=0;
        services.group_argument=1u;event("combat_prepare",900u);
        Check(services.draws==3u && services.properties[101u]==64.0f && services.properties[102u]<64.0f,"weighted prepare and player group override");
        services.group_argument=2u;event("combat_prepare",900u);
        Check(services.properties[102u]==64.0f && services.properties[101u]<64.0f,"monster group override");
        services.ready=false;event("ae_state_changed",1u);
        Check(manager.Objects().size()==1u && manager.Objects()[0]==102u,"state callback removes not-ready object");
        services.ready=true;event("ae_start_action",2u);Check(services.state==3u,"start action changes combat state3");
        event("ae_end_action");Check(services.state==2u && !manager.Active(),"end action scans both groups and changes state2");
        event("combat_over");event("ae_end_action");Check(!manager.Active(),"ready check resumes active manager");
        event("close_system");Check(manager.Objects().empty(),"close_system clears logical vector");
        services.state=1u;
        for(unsigned e=100u;e<180u;++e)event("me_join_stage",e);
        Check(manager.Objects().size()==80u && manager.Objects().front()==1100u && manager.Objects().back()==1179u,"host vector grows without stale shared views");
        for(unsigned e=100u;e<179u;++e)event("me_leave_stage",e);
        Check(manager.Objects().size()==1u && manager.Objects().front()==1179u,"shared shifting/removal survives storage growth");
        manager.Reset();
        services.state=1u;event("re_join_stage",1u);event("me_join_stage",2u);
        Check(manager.AllObjectsReady(),"both groups ready");
        services.throw_ready=true;const auto old_released=services.released;
        bool threw{};try{(void)manager.AllObjectsReady();}catch(const std::runtime_error&){threw=true;}
        Check(threw && services.released==old_released+1u,"iterator reclaimed on service exception");services.throw_ready=false;
        manager.Reset();Check(manager.Objects().empty(),"explicit reset owns storage");
        event("re_join_stage",1u);services.state=2u;
        services.on_set=[&]{Reject([&]{manager.Close();});Reject([&]{manager.Reset();});Reject([&]{manager.Subscribe();});Reject([&]{(void)manager.Update(0);});};
        (void)manager.Update(0.01f);services.on_set={};
        services.on_set=[] {throw std::runtime_error("setter failure");};
        threw=false;try{(void)manager.Update(0.01f);}catch(const std::runtime_error&){threw=true;}Check(threw,"frame exception propagated");
        services.on_set={};manager.Reset();
        services.state=1u;event("re_join_stage",1u);services.state=2u;services.inactive=true;
        services.properties[101u]=0;(void)manager.Update(0.0f);
        Check(services.properties[101u]==64.0f && services.started.back()==101u,"out-of-combat fast readiness loop");services.inactive=false;
        services.state=3u;services.ready=false;(void)manager.Update(0.0f);
        Check(manager.Active() && manager.Objects().size()==1u && manager.Objects()[0]==101u,
              "not-ready state3 pauses without discarding registered actor facets");services.ready=true;
        event("ae_end_action",1u);
        Check(!manager.Active() && services.state==2u && manager.Objects().size()==1u,
              "ready end-action resumes scheduling with the same actor facet");
        Reject([&]{(void)manager.Update(-1.0f);});Reject([&]{(void)manager.Update(std::numeric_limits<float>::quiet_NaN());});
        manager.Close();manager.Close();Check(!manager.Address() && manager.SubscriptionCount()==0u && router.SubscriptionCount()==1u && router.Contains(unrelated),"close preserves unrelated subscription");
        Reject([&]{manager.Reset();});
    }
    {pal4::NativeCombatSystemManager scoped(router,services);}
    Check(router.SubscriptionCount()==1u,"destructor releases13 handlers");
    std::cout<<"PASS native SystemManager:13 named bindings, modern roster, original ATB/prepare/state callbacks, scopes and cleanup; actor/action services are protocol fixtures\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;} }
