#pragma once
#include "pal4/native_combat_actor_skill_runtime.h"
#include "pal4/native_script_event_router.h"
#include <array>
#include <iostream>
#include <stdexcept>
#include <vector>
namespace pal4::test {
// Actual actor/registry owners with test-scoped event subscribers. These probes
// verify transport and lifetime, not execution of protection or attack tasks.
inline void VerifyNativeCombatPairEvents(NativeCombatActorSkillRuntime& skills,
    NativeScriptEventRouter& router,const char* label){
    const auto check=[](bool b,const char* why){if(!b)throw std::runtime_error(why);};
    const auto reject=[&](auto&& f,const char* why){bool denied{};try{f();}catch(const std::logic_error&){denied=true;}check(denied,why);};
    const auto actor=skills.ActorAddress();
    const auto state=*reinterpret_cast<const std::uint32_t*>(actor+828u);
    std::vector<std::array<std::uint32_t,3>> protection;
    std::vector<std::array<std::uint32_t,2>> joint;
    std::vector<std::array<std::uint32_t,2>> react;
    bool fail{};
    struct Routes{NativeScriptEventRouter& router;std::vector<std::uint32_t> handles;
        ~Routes(){for(auto h:handles)(void)router.Unsubscribe(h);}} routes{router};
    routes.handles.push_back(router.Subscribe(router.Key("ae_protect_action"),1u,[&](std::uint32_t p){
        const auto* words=reinterpret_cast<const std::uint32_t*>(p);
        protection.push_back({words[0],words[1],words[2]});
        reject([&]{skills.Close();},"protection callback cannot retire actor skills");
        reject([&]{(void)skills.ProtectAction(actor,actor);},"protection event reentry rejected");
        reject([&]{(void)skills.JointAttack(actor);},"pair event cross reentry rejected");
        if(fail)throw std::logic_error("intentional pair observer failure");
    }));
    routes.handles.push_back(router.Subscribe(router.Key("ae_joint_attack"),1u,[&](std::uint32_t p){
        const auto* words=reinterpret_cast<const std::uint32_t*>(p);
        joint.push_back({words[0],words[1]});
        reject([&]{skills.Close();},"joint callback cannot retire actor skills");
        reject([&]{(void)skills.JointAttack(actor);},"joint event reentry rejected");
    }));
    routes.handles.push_back(router.Subscribe(router.Key("ae_react_attack"),1u,[&](std::uint32_t p){
        const auto* words=reinterpret_cast<const std::uint32_t*>(p);react.push_back({words[0],words[1]});
        reject([&]{skills.Close();},"react callback cannot retire actor skills");
        reject([&]{(void)skills.ReactAttack(actor);},"react event reentry rejected");
        reject([&]{(void)skills.JointAttack(actor);},"react cross event reentry rejected");
        if(fail)throw std::logic_error("intentional react observer failure");
    }));
    (void)skills.ProtectAction(actor,actor);
    (void)skills.ProtectAction(0u,actor);
    (void)skills.ProtectAction(actor,0u);
    (void)skills.ProtectAction(0u,0u);
    check(protection==std::vector<std::array<std::uint32_t,3>>{
        {actor+716u,actor,actor},{0u,actor,actor},{actor+716u,0u,actor},{0u,0u,actor}},
        "actual protection preserves original three-word ordering and null facet");
    (void)skills.JointAttack(actor);(void)skills.JointAttack(0u);
    check(joint==std::vector<std::array<std::uint32_t,2>>{{actor+716u,actor},{actor+716u,0u}},
        "actual joint event preserves original two-word ordering and null other");
    const auto old=protection.size();
    reject([&]{(void)skills.ProtectAction(0x1000u,actor);},"foreign protected actor rejected");
    reject([&]{(void)skills.ProtectAction(actor,0x1000u);},"foreign other actor rejected");
    reject([&]{(void)skills.JointAttack(0x1000u);},"foreign joint actor rejected");
    check(protection.size()==old && joint.size()==2u,"invalid participants emit no event");
    fail=true;reject([&]{(void)skills.ProtectAction(actor,actor);},"observer failure propagates");fail=false;
    (void)skills.ProtectAction(actor,actor);(void)skills.JointAttack(actor);
    check(protection.size()==old+2u && joint.size()==3u &&
        *reinterpret_cast<const std::uint32_t*>(actor+828u)==state,
        "observer failure releases pin; pair notifications preserve actor state");
    (void)skills.ReactAttack(actor);(void)skills.ReactAttack(0u);
    check(react==std::vector<std::array<std::uint32_t,2>>{{actor+716u,actor},{actor+716u,0u}},"actual counter notification keeps original facet and null target");
    reject([&]{(void)skills.ReactAttack(0x1000u);},"foreign counter target rejected");
    fail=true;reject([&]{(void)skills.ReactAttack(actor);},"react observer failure propagates");fail=false;
    (void)skills.ReactAttack(actor);check(react.size()==4u,"react failure releases actual event pin");
    std::cout<<"Native combat pair events: "<<label
        <<" actual protect/joint/react records, nulls, ownership, exception/reentry/retirement guards pass\n";
}
}
