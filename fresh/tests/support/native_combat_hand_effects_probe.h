#pragma once
#include "pal4/native_combat_hand_effects.h"
#include "pal4/native_actor_instance_owner.h"
#include "pal4/native_gob_clump_asset.h"
#include "pal4/native_player_slot_copy_runtime.h"
#include "pal4/native_skill_system_owner.h"
#include "pal4/native_player_combat_events.h"
#include "pal4/random_number_generator.h"
#include "pal4/native_script_event_router.h"
#include "pal4/native_effect_handle_content_runtime.h"
#include "native_particle_probe_camera.h"
#include <array>
#include <cstring>
#include <iostream>
#include <stdexcept>
namespace pal4::test {
inline void VerifyNativeCombatHandEffects(NativeEffectSceneSession& effects,NativeScriptEventRouter& router,
    const std::filesystem::path& root) {
    const auto check=[](bool ok,const char* m){if(!ok)throw std::runtime_error(m);};
    const auto reject=[&](auto&& f){bool blocked=false;try{f();}catch(const std::logic_error&){blocked=true;}check(blocked,"expected hand effect rejection");};
    const auto address=[](const void* p){return reinterpret_cast<std::uint32_t>(p);};
    NativePlayerCombatEvents events;RandomNumberGenerator random;NativeSkillSystemOwner skills(random,events);
    (void)skills.Construct("H:/PAL4/objs");
    NativeGobClumpAsset weapon;check(weapon.Load(root,"gamedata\\PALWeapon\\WTA\\","WTA"),"hand real WTA");
    std::array<std::uint32_t,128> game{};NativeActorInstanceOwner owner;
    const auto actor=owner.Construct(address(game.data()));
    const auto component=actor+624u;
    std::array<std::uint32_t,116> right_before{};
    std::memcpy(right_before.data(),reinterpret_cast<const void*>(actor+1088u),464u);
    auto view=std::span<std::uint32_t,116>(reinterpret_cast<std::uint32_t*>(component),116u);
    NativePlayerSlotCopyRuntime texts;
    view[5]=address(weapon.Clump());
    texts.AssignText(component+412u,"ZL-20");texts.AssignText(component+428u,"ZL-20");texts.AssignText(component+444u,"OM03");
    struct Hook {
        RwFrame* parent=RwFrameCreate();RpClump* weapon{};
        ~Hook(){if(weapon && RwFrameGetParent(RpClumpGetFrame(weapon)))RwFrameRemoveChild(RpClumpGetFrame(weapon));if(parent)RwFrameDestroyHierarchy(parent);}
    } hook;
    hook.weapon=weapon.Clump();check(hook.parent && RwFrameAddChild(hook.parent,RpClumpGetFrame(weapon.Clump())),"real hand hook parent");
    struct Cleanup {
        NativeEffectSceneSession& effects;NativePlayerSlotCopyRuntime& texts;std::uint32_t component;
        NativeScriptEventRouter& router;std::uint32_t observer{};
        void Retire(){
            if(!component)return;
            if(observer)(void)router.Unsubscribe(observer);
            observer=0u;effects.ClearCombatEffects();
            for(auto o:{412u,428u,444u})texts.ReleaseText(component+o);
            *reinterpret_cast<std::uint32_t*>(component+20u)=0u;
            component=0u;
        }
        ~Cleanup(){try{Retire();}catch(...){}}
    } cleanup{effects,texts,component,router};
    {
    NativeCombatHandEffects hand(view,*weapon.Clump(),texts,effects,skills);
    const auto handle=[&](unsigned offset){return *reinterpret_cast<const EffectSystemHandleWords*>(component+offset);};
    NativeParticleProbeCamera camera;
    for(unsigned cycle=0;cycle<2u;++cycle) {
        hand.SetActive(1u);
        check(view[115]==1u && effects.CombatXmlEffectCount()==2u &&
              effects.CombatXmlEffectReady(handle(388u)) && effects.CombatXmlEffectReady(handle(396u)) &&
              EffectHandle::DialogNeedsUpdate(handle(404u),effects.ContentHandles()),"real hand creates two XML contexts and one Dialog");
        const auto frame=EffectHandle::GetFrame(handle(404u),effects.ContentHandles());
        check(frame && RwFrameGetParent(reinterpret_cast<RwFrame*>(frame))==hook.parent,"Dialog frame shares actual weapon hook parent");
        const auto prior=handle(404u);hand.SetActive(1u);
        check(handle(404u).id==prior.id,"unchanged hand active state preserves effect instance");
        effects.UpdateCombatEffects(0.04);camera.Begin();effects.DrawCombatEffects(0u);(void)camera.EndPixels();
        hand.Refresh();
        check(handle(404u).id!=prior.id && effects.CombatXmlEffectCount()==2u,"explicit refresh replaces old handles");
        hand.SetActive(2u);check((view[115]&255u)==2u,"hand activation preserves nonboolean state byte");
        hand.SetActive(0u);
        check(!handle(388u).system && !handle(396u).system && !handle(404u).system &&
              !effects.CombatXmlEffectCount() && !(view[115]&255u),"deactivate removes all three actual hand effects");
        effects.ClearCombatEffects();
    }
    hand.SetActive(1u);
    const auto before=std::array{handle(388u),handle(396u),handle(404u)};
    view[100]=0x1234u;
    reject([&]{hand.Refresh();});
    check(handle(388u).id==before[0].id && handle(404u).id==before[2].id && effects.CombatXmlEffectCount()==2u,
          "foreign owner preflight preserves sibling effects");view[100]=before[1].system;
    const auto name_pointer=view[108];view[108]=0x1234u;
    reject([&]{hand.Refresh();});view[108]=name_pointer;
    view[5]=0u;reject([&]{hand.Refresh();});view[5]=address(weapon.Clump());
    check(effects.CombatXmlEffectCount()==2u && handle(404u).id==before[2].id,
          "text and clump preflight reject before effect mutation");
    bool throw_end=true;unsigned callbacks{};
    cleanup.observer=router.Subscribe(router.Key("effect_end"),1u,[&](std::uint32_t p){
        const auto* event=reinterpret_cast<const std::uint32_t*>(p);
        if(event[0]!=handle(404u).id)return;
        ++callbacks;reject([&]{hand.Refresh();});reject([&]{hand.Close();});
        if(throw_end)throw std::logic_error("intentional hand effect observer failure");
    });
    reject([&]{hand.SetActive(0u);});
    check(!handle(388u).system && !handle(396u).system && handle(404u).system && (view[115]&255u)==1u,
          "throwing observer preserves prior XML removal and unretired Dialog for retry");
    throw_end=false;hand.SetActive(0u);check(callbacks==2u && !handle(404u).system,"hand release retries after observer failure");
    check(router.Unsubscribe(cleanup.observer),"hand observer retired");cleanup.observer=0u;
    check(RwFrameRemoveChild(RpClumpGetFrame(weapon.Clump())),"remove borrowed weapon hook parent");
    hand.Refresh();
    check(!(view[115]&255u) && effects.CombatXmlEffectCount()==2u && !handle(404u).system,
          "unparented refresh skips ordinary Dialog and retains real XML while inactive");
    hand.Close();hand.Close();reject([&]{hand.Refresh();});
    check(!std::memcmp(right_before.data(),reinterpret_cast<const void*>(actor+1088u),464u),"left effects leave actual right-hand image unchanged");
    check(!effects.CombatXmlEffectCount(),"hand XML resource balance");
    }
    cleanup.Retire();owner.Destroy();
    check(!texts.OwnedTextCount() && !owner.OwnedAllocations() && !owner.OwnedTexts(),
          "explicit native hand fixture teardown balances text and ActorInstance allocations");
    skills.Destroy();check(!skills.OwnedAllocations(),"hand SkillSystem retires after adapter");
    std::cout<<"Native529F90 hand effects: real ActorInstance left component/WTA/ZL-20/OM03, SkillSystem, shared refresh/active/deactivate, hook parent, update/draw, foreign/text preflight, callback guards and release retry pass\n";
}
}
