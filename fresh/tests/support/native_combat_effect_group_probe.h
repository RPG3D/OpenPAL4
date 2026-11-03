#include "pal4/native_combat_system_image.h"
#pragma once
#include "pal4/native_effect_scene_session.h"
#include "pal4/native_effect_creation.h"
#include "pal4/native_script_event_router.h"
#include "pal4/native_gob_object_image.h"
#include "pal4/native_combat_effect_notifications.h"
#include "native_particle_probe_camera.h"
#include "native_combat_actor_effects_probe.h"
#include "native_combat_manager_events_probe.h"
#include "native_combat_death_probe.h"
#include "native_combat_xml_probe.h"
#include "native_combat_hand_effects_probe.h"
#include "native_actor_animation_binding_probe.h"
#include "native_combat_hand_resources_probe.h"
#include <array>
#include <vector>
#include <string>
#include <iostream>
#include <stdexcept>
namespace pal4::test {
inline void VerifyNativeCombatEffectGroup(NativeEffectSceneSession& effects,const std::filesystem::path& root,
    NativePkFileManagerRuntime& packages) {
    const auto check=[](bool ok,const char* message){if(!ok) throw std::runtime_error(message);};
    const auto address=[](const void* p){return reinterpret_cast<std::uint32_t>(p);};
    const auto word=[](std::uint32_t p)->std::uint32_t {return *reinterpret_cast<const std::uint32_t*>(p);};
    const auto reject=[&](auto&& f) {bool rejected=false;try{f();}catch(const std::logic_error&){rejected=true;}check(rejected,"expected Combat effect group rejection");};
    // Separate SDK world in the actual CombatSystem+116 image. It must survive
    // active effects, returned pooled models and both auxiliary group owners.
    struct World {RpWorld* p{};~World(){if(p)RpWorldDestroy(p);}} world;
    RwBBox bounds{{1000,1000,1000},{-1000,-1000,-1000}};
    world.p=RpWorldCreate(&bounds);check(world.p,"Combat model effect world");
    std::array<std::uint32_t,100> manager{};
    NativeCombatSystemImage combat_image;
    std::span<std::uint32_t,130> combat(reinterpret_cast<std::uint32_t*>(&combat_image),130u);
    combat[116u/4u+4u]=address(world.p);
    combat[116u/4u+12u]=1u;
    std::array<std::uint32_t,80> ui{};
    manager[96]=0xffffffffu;manager[98]=0xffffffffu;
    NativeScriptEventRouter router;
    effects.BindUiEffects(address(ui.data()));
    effects.BindCombatEffects(address(manager.data()),address(combat.data()),router);
    struct Retire {
        NativeEffectSceneSession& effects; NativeScriptEventRouter& router;
        ~Retire() {
            router.Clear();
            try {effects.Clear();effects.ClearUiEffects();effects.ClearCombatEffects();
                 effects.UnbindCombatEffects();effects.UnbindUiEffects();} catch(...) {}
        }
    } retire{effects,router};
    check(manager[0]==0x8452c8u && manager[96]==0xffffffffu && manager[98]==0xffffffffu,
          "Combat effect base preserves manager tail and original vtable identity");
    check(manager[75]==0x53dcf0u && !manager[71] && !manager[73],
          "Combat game object installs original callback and uses host catalog without legacy tree");
    NativeParticleProbeCamera camera;camera.Begin(0.0f);(void)camera.EndPixels();
    std::array<std::uint32_t,512> game{};
    auto* missing_raster=RwCameraCreate();check(missing_raster!=nullptr,"Combat rollback camera");
    reject([&]{effects.InitializeCombatGameObject(address(game.data()),address(missing_raster));});
    check(!manager[65] && !manager[69] && !manager[12] && !effects.CombatScreenEffectHandlerCount(),"Combat missing-raster initialization rolls back base and handler state");
    RwCameraDestroy(missing_raster);
    for(unsigned cycle=0;cycle<2u;++cycle) {
        check(effects.InitializeCombatGameObject(address(game.data()),address(camera.camera)),
              "Combat game object initializes actual camera and fifteen host CG handlers");
        check(*reinterpret_cast<const std::uint8_t*>(address(manager.data())+260u)==1u &&
              manager[69]!=0u && manager[12]==address(camera.camera) && effects.CombatScreenEffectHandlerCount()==15u,
              "Combat game object retains actual capture record and borrowed camera");
        effects.DeinitializeCombatGameObject();
        check(!manager[65] && !manager[66] && !manager[67] && !manager[68] && !manager[69] && !manager[12] && !effects.CombatScreenEffectHandlerCount(),
              "Combat deinitialize releases capture/catalog and disables base");
    }
    check(effects.InitializeCombatGameObject(address(game.data()),address(camera.camera)),
          "Combat game object reinitializes for real effect playback");
    check(effects.InitializeCombatGameObject(address(game.data()),address(camera.camera)) && manager[69]!=0u && effects.CombatScreenEffectHandlerCount()==15u,
          "consecutive Combat initialization resets previous host capture/catalog");
    std::cout<<"Native Combat game object: missing-raster rollback, four real camera/host catalog initializations including consecutive reset, two deinitializations, callback53DCF0 and no legacy handler tree verified\n";
    reject([&]{effects.BindCombatEffects(address(manager.data()),address(combat.data()),router);});
    std::vector<std::string> order;unsigned ends{},starts{};
    const auto start=router.Subscribe(router.Key("combat_start"),1u,[&](unsigned p) {
        check(word(p)==0u && combat[127]==2u,"combat_start follows actual phase508 write");
        order.emplace_back("start");++starts;
        reject([&]{effects.UnbindCombatEffects();});reject([&]{effects.ClearCombatEffects();});
    });
    bool fail=false;
    const auto end=router.Subscribe(router.Key("effect_end"),1u,[&](unsigned p) {
        check(word(p+4u)==address(manager.data()) && word(p+12u)==0u && word(p+16u)==0u,
              "Combat end has actual handle and empty DNameNode text");
        const auto snapshot=std::array{word(p),word(p+4u),word(p+8u),word(p+12u),word(p+16u),word(p+20u)};
        reject([&]{effects.UnbindCombatEffects();});reject([&]{effects.UpdateCombatEffects(0.01);});
        reject([&]{(void)effects.LoadCombatXmlEffect("",0u);});
        EffectSystemHandleWords empty_xml{0xffffffffu,0u};
        reject([&]{effects.RemoveCombatXmlEffect(empty_xml);});
        reject([&]{effects.DrainCombatXmlEffect(empty_xml);});
        check(snapshot==std::array{word(p),word(p+4u),word(p+8u),word(p+12u),word(p+16u),word(p+20u)},"callback guard preserves borrowed payload");
        order.emplace_back("end");++ends;
        if(fail) throw std::logic_error("test Combat end observer failure");
    });
    const auto scene=effects.GobEffects().Create("foot_2",1u,false);
    const auto ui_handle=effects.UiEffectCreation().Create("foot_2",{0,1,1},1,0u);
    const auto h=effects.CombatEffectCreation().Create("foot_2",{0,1,1},1,0u);
    std::array<float,3> facet_vector{10,20,30},facet_result{};
    const NativeDialogProperties::Reader facet_reader=[](std::uint32_t p) {
        return *reinterpret_cast<const std::array<float,3>*>(p);
    };
    EffectSystemHandleWords facet_handle{h.id,h.system};
    std::array<float,3> configured_vector{};
    const auto configured=effects.CombatEffectVector(facet_handle,"T1",configured_vector,{});
    effects.SetCombatEffectFacet(facet_handle,"T1",address(facet_vector.data()));
    check(effects.CombatEffectVector(facet_handle,"T1",facet_result,facet_reader) && facet_result==facet_vector,
          "Combat handle facet uses actual loaded Dialog and modern property adapter");
    check(h.system==address(manager.data()) && effects.CombatActiveCount()==1u &&
          effects.ActiveCount()==1u && effects.UiActiveCount()==1u && order.empty(),
          "actual original foot_2 lives in independent Scene UI Combat groups; slot20 emits no end event");
    reject([&]{effects.InitializeCombatGameObject(address(game.data()),address(camera.camera));});
    reject([&]{effects.DeinitializeCombatGameObject();});
    auto foreign=EffectSystemHandleWords{ui_handle.id,ui_handle.system};
    reject([&]{effects.ReleaseCombatEffect(foreign,true);});
    reject([&]{effects.UnbindCombatEffects();});reject([&]{effects.UnbindUiEffects();});
    camera.Begin(0.0f);effects.BindCombatCamera(address(camera.camera));
    for(unsigned i=0;i<3u;++i) effects.UpdateCombatEffects(0.04);
    effects.DrawCombatEffects(0u);const auto pixels=camera.EndPixels();
    check(pixels>0u,"Combat effect draws actual colored pixels");
    manager[96]=h.id;manager[97]=h.system;
    EffectSystemHandleWords owned{h.id,h.system};effects.ReleaseCombatEffect(owned,true);
    check(!owned.system && !effects.CombatActiveCount() && order==std::vector<std::string>{"start","end"} &&
          effects.ActiveCount()==1u && effects.UiActiveCount()==1u,"selected effect release advances phase then end while preserving other groups");
    const auto h2=effects.CombatEffectCreation().Create("foot_2",{0,1,1},1,0u);
    check(effects.CombatEffectVector({h2.id,h2.system},"T1",facet_result,{})==configured &&
          (!configured || facet_result==configured_vector),
          "recycled Combat Dialog clears role facet and restores configured fallback");
    manager[96]=h2.id;manager[97]=h2.system;
    fail=true;EffectSystemHandleWords retry{h2.id,h2.system};reject([&]{effects.ReleaseCombatEffect(retry,true);});
    check(effects.CombatActiveCount()==1u && combat[127]==2u,"throwing end observer retains preceding phase/write and active record");
    fail=false;effects.ReleaseCombatEffect(retry,true);check(!effects.CombatActiveCount(),"effect release retries after event failure");
    // An unselected real handle emits effect_end without advancing the phase.
    const auto h3=effects.CombatEffectCreation().Create("foot_2",{0,1,1},1,0u);
    const auto before=starts;EffectSystemHandleWords unselected{h3.id,h3.system};
    effects.ReleaseCombatEffect(unselected,true);
    check(starts==before,"unselected effect end does not advance combat phase");
    const auto natural=effects.CombatEffectCreation().Create("foot_2",{0,1,1},1,0u);
    manager[96]=natural.id;manager[97]=natural.system;
    const auto natural_ends=ends;
    for(unsigned i=0;i<1000u && effects.CombatActiveCount();++i)
        effects.UpdateCombatEffects(0.04);
    check(!effects.CombatActiveCount() && starts==before+1u && ends==natural_ends+1u,
          "finite Combat effect completes through real frame playback and slot24 events");
    VerifyNativeCombatXml(effects,root);
    VerifyNativeActorAnimationBinding(root);
    VerifyNativeCombatHandEffects(effects,router,root);
    VerifyNativeCombatHandResources(effects,router,packages,root,address(game.data()),address(camera.camera));
    VerifyNativeCombatActorEffects(effects,router,root,world.p);
    VerifyNativeCombatActorEntry(effects,router,root,address(combat.data()));
    effects.GobEffects().Release(scene.first,scene.second);
    EffectSystemHandleWords u{ui_handle.id,ui_handle.system};effects.ReleaseUiEffect(u,true);
    effects.Clear();effects.ClearUiEffects();effects.ClearCombatEffects();
    VerifyNativeCombatDeath(effects,router,root,combat_image);
    effects.UnbindCombatEffects();
    for(unsigned i=0;i<65u;++i) check(!manager[i],"Combat group teardown clears only owned base");
    check(manager[96]==natural.id && manager[97]==natural.system,"Combat teardown preserves actor-manager fields");
    effects.UnbindUiEffects();
    check(router.Unsubscribe(start) && router.Unsubscribe(end),"Combat effect test subscriptions retired");
    reject([&]{(void)effects.CombatEffectCreation();});
    reject([&]{(void)effects.LoadCombatXmlEffect("",0u);});
    reject([&]{(void)effects.CombatXmlEffectCount();});
    std::cout<<"Native Combat effect group: real foot_2 playback/GPU pixels="<<pixels
             <<" phase508, end events="<<ends<<", isolation and retirement verified\n";
    VerifyNativeCombatManagerEvents(effects,root);
}
}
