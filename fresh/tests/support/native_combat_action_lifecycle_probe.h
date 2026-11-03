#pragma once
#include "pal4/native_combat_actor_skill_runtime.h"
#include "pal4/native_script_event_router.h"
#include "native_combat_pair_events_probe.h"
#include <array>
#include <vector>
#include <string>
#include <iostream>
#include <stdexcept>
namespace pal4::test {
// Uses actual actor/skills/model refresh with a scoped test router. Formal
// production57B5E0 composition is verified separately by the action-end probe.
inline void VerifyNativeCombatActionLifecycle(NativeCombatActorSkillRuntime& skills,
    NativeScriptEventRouter& router,const NativeCombatActorSkillRuntime::ActionMotionRefresh& actual_motion,
    const char* label){
    VerifyNativeCombatPairEvents(skills,router,label);
    const auto check=[](bool b,const char* why){if(!b)throw std::runtime_error(why);};
    const auto actor=skills.ActorAddress();
    auto& state=*reinterpret_cast<std::uint32_t*>(actor+828u);
    struct Restore {NativeCombatActorSkillRuntime& s;std::uint32_t old;~Restore(){try{s.SetFacetState(old,false);}catch(...){}}} restore{skills,state};
    struct Routes {NativeScriptEventRouter& r;std::vector<std::uint32_t> handles;
        ~Routes(){for(auto h:handles)(void)r.Unsubscribe(h);}} routes{router};
    const auto reject=[&](auto&& f,const char* why){bool denied{};try{f();}catch(const std::logic_error&){denied=true;}check(denied,why);};
    std::vector<std::string> trace;
    bool fail_start{},fail_motion{};std::uint32_t expected_end=1u;unsigned motions{};
    const auto same_actor=[&](std::uint32_t p){return *reinterpret_cast<const std::uint32_t*>(p)==actor+716u;};
    routes.handles.push_back(router.Subscribe(router.Key("ae_state_changed"),1u,[&](std::uint32_t p){
        if(same_actor(p))trace.push_back("state"+std::to_string(state));
    }));
    routes.handles.push_back(router.Subscribe(router.Key("ae_start_action"),1u,[&](std::uint32_t p){
        if(!same_actor(p))return;check(state==2u,"start event sees actual facet2");trace.push_back("start");
        reject([&]{skills.Close();},"actor services cannot retire in start callback");
        reject([&]{skills.StartAction();},"action lifecycle start reentry rejected");
        reject([&]{skills.EndAction({});},"action lifecycle end reentry rejected");
        if(fail_start)throw std::logic_error("intentional action start observer failure");
    }));
    routes.handles.push_back(router.Subscribe(router.Key("ae_end_action"),1u,[&](std::uint32_t p){
        if(!same_actor(p))return;check(state==expected_end,"end event observes state after actual motion");trace.push_back("end");
    }));
    const auto motion=[&](std::uint32_t a,std::uint32_t flag){
        check(a==actor && flag==1u && state==1u,"actual396 refresh receives actor/flag after state1");
        trace.push_back("motion");
        if(fail_motion)throw std::logic_error("intentional action motion failure");
        actual_motion(a,flag);++motions;
    };
    skills.StartAction();skills.EndAction(motion);
    check(trace==std::vector<std::string>{"state2","start","state1","motion","end"} && motions==1u,
        "actual start/state/end and model refresh preserve original ordering");
    // Explicit boundary state fixture; setters and all notifications are real.
    skills.SetFacetState(6u,false);expected_end=6u;trace.clear();skills.EndAction({});
    check(state==6u && trace==std::vector<std::string>{"end"} && motions==1u,
        "non-action state end skips state/motion but still dispatches end");
    skills.SetFacetState(2u,false);expected_end=1u;trace.clear();
    reject([&]{skills.EndAction({});},"state2 end requires actual motion service");
    check(state==2u && trace.empty(),"missing motion rejects before state/event mutation");
    fail_motion=true;reject([&]{skills.EndAction(motion);},"motion failure propagates");fail_motion=false;
    check(state==1u && trace==std::vector<std::string>{"state1","motion"},
        "motion failure preserves preceding state and suppresses later end event");
    skills.SetFacetState(2u,false);trace.clear();skills.EndAction(motion);
    check(motions==2u && trace==std::vector<std::string>{"state1","motion","end"},"motion retry releases all guards");
    trace.clear();fail_start=true;reject([&]{skills.StartAction();},"start observer failure propagates");fail_start=false;
    check(state==2u && trace==std::vector<std::string>{"state2","start"},"start failure preserves original state/event writes");
    trace.clear();skills.EndAction(motion);check(state==1u && motions==3u,"start failure does not strand lifecycle guard");
    const auto property=*reinterpret_cast<const std::uint32_t*>(actor+800u);
    auto& link=*reinterpret_cast<std::uint32_t*>(property+8u);
    struct Link {std::uint32_t& field;std::uint32_t old;~Link(){field=old;}} link_restore{link,link};
    check(!link,"actual lifecycle test uses original base property without wrapper");link=0x1000u;trace.clear();
    reject([&]{skills.StartAction();},"unbound linked property rejects before forwarding");
    check(state==1u && trace.empty(),"linked property preflight preserves state");link=link_restore.old;
    std::cout<<"Native combat action lifecycle: "<<label
        <<" actual state/events/property28/32, model refresh, branch/failure/reentry and cleanup pass\n";
}
}
