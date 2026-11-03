#pragma once
#include "pal4/native_combat_manager_events.h"
#include "native_particle_probe_camera.h"
#include "pal4/native_combat_actor_model_owner.h"
#include "pal4/native_combat_actor_effects.h"
#include "pal4/native_miles_digital_driver.h"
#include "pal4/native_miles_sound_2d_runtime.h"
#include "pal4/native_actor_instance_pool.h"
#include "pal4/native_animated_actor_asset.h"
#include "pal4/native_monster_record_catalog.h"
#include <iostream>
namespace pal4::test {
inline void VerifyNativeCombatManagerEvents(NativeEffectSceneSession& effects,const std::filesystem::path& root) {
    std::cout<<std::unitbuf<<"Native ActorManager ten callback probe begin\n";
    const auto check=[](bool ok,const char* m){if(!ok)throw std::runtime_error(m);};
    const auto addr=[](const void* p){return reinterpret_cast<std::uint32_t>(p);};
    std::array<std::uint32_t,100> image{};std::array<std::uint32_t,130> combat{};
    std::array<std::uint32_t,128> game{};NativeScriptEventRouter router;
    NativeGameDatabase db;check(db.Load(root),"manager events original database");NativeMonsterRecordCatalog records(db);
    NativeActorInstancePool pool([]{return false;},[&](std::string_view name){
        auto m=std::make_unique<NativeActorInstanceOwner>();m->Construct(addr(game.data()));m->Load(root,name);return m;
    });
    NativeCombatActorModelOwner model(pool);const auto a=model.Construct(records,2579);
    model.SetSlot(0u);(void)model.RefreshResource();
    NativeParticleProbeCamera camera;camera.Begin();(void)camera.EndPixels();
    NativeMilesDigitalDriver driver(root.string());check(driver.Open(),"Combat event Miles driver");driver.SetMasterVolume(0);
    NativeMilesSound2dRuntime audio(root,driver);audio.SetVolume(0);effects.BindSound(&audio);
    effects.BindCombatEffects(addr(image.data()),addr(combat.data()),router);
    NativeCombatActorEffects attachments(model,effects,router);
    effects.BindDialogFacetReader([&](std::uint32_t facet){return attachments.ReadPositionFacet(facet);});
    effects.BindCombatCamera(addr(camera.camera));
    struct Drain {NativeEffectSceneSession& e;~Drain(){try{e.ClearCombatEffects();e.UnbindCombatEffects();e.BindDialogFacetReader({});e.BindSound(nullptr);}catch(...){}}} drain{effects};
    unsigned resolved{},variant{},kind=2u;
    NativeCombatManagerEvents handlers(addr(image.data()),addr(combat.data()),effects,router,[&](std::uint32_t actor){
        check(actor==a,"manager event resolves real managed actor");++resolved;
        const auto c=model.Model()->Address();
        // Selection variants are an explicit UI service fixture. The actual
        // Player1192 owner is not yet bound; model vectors are real B01.
        return NativeCombatEventActor{kind,c+172u,c+160u,variant};
    });
    check(handlers.SubscriptionCount()==10u && router.SubscriptionCount()==10u,"all ten actual subscriptions");
    handlers.Subscribe();check(router.SubscriptionCount()==10u,"resubscribe retires previous subscriptions");
    const auto reject=[&](auto&& f){bool bad=false;try{f();}catch(const std::logic_error&){bad=true;}check(bad,"manager callback mutation rejected");};
    unsigned messages{};
    const auto message_observer=router.Subscribe(router.Key("effect_message"),2u,[&](std::uint32_t p){
        const auto* record=reinterpret_cast<const std::uint32_t*>(p);
        check(record[1]==addr(image.data()) && (!record[4] || record[3]),"original effect message retains bound Combat owner/text");
        ++messages;
        reject([&]{effects.UnbindCombatEffects();});reject([&]{effects.ClearCombatEffects();});
        reject([&]{effects.UpdateCombatEffects(0.01);});
        std::cout<<"Native original Combat effect_message: id="<<record[0]<<" text="
            <<(record[3]?reinterpret_cast<const char*>(record[3]):"")<<'\n';
    });
    std::uint32_t command=1u;const std::array pair{a+716u,a,a};
    bool inside_prepare=true;
    unsigned starts{};const auto observer=router.Subscribe(router.Key("combat_start"),2u,[&](std::uint32_t){
        ++starts;if(inside_prepare){reject([&]{handlers.Close();});reject([&]{handlers.Subscribe();});}
    });
    *reinterpret_cast<std::uint8_t*>(addr(combat.data())+516u)=1u;
    (void)router.Dispatch("combat_prepare",addr(&command));
    std::cout<<"Native ActorManager inactive prepare complete\n";
    for(const auto* name:{"combat_over","re_start_ui_control","re_close_ui_control","ae_protect_action",
        "ae_critical_attack","ae_fend_offed","ae_fleed","me_join_stage"})(void)router.Dispatch(name,0u);
    check(starts==1u && combat[127]==2u && !resolved && !effects.CombatActiveCount(),"inactive path skips all actor/payload/effect work and advances phase");
    *reinterpret_cast<std::uint8_t*>(addr(combat.data())+516u)=0u;
    (void)router.Dispatch("combat_prepare",addr(&command));
    std::cout<<"Native ActorManager active prepare complete\n";
    inside_prepare=false;
    auto& selected=*reinterpret_cast<EffectSystemHandleWords*>(addr(image.data())+384u);
    check(EffectHandle::DialogNeedsUpdate(selected,effects.ContentHandles()) &&
        EffectHandle::GetName(selected,effects.ContentHandles())=="CA1","prepare loads original selected CA1");
    const auto prior=starts;effects.ReleaseCombatEffect(selected,true);
    check(starts==prior+1u && combat[127]==2u,"selected prepare completion advances original phase and dispatches start");
    (void)router.Dispatch("combat_over",addr(&command));
    for(unsigned i=0;i<10u;++i)effects.UpdateCombatEffects(0.04);
    std::cout<<"Native combat CA47 camera frames: ten updates complete\n";
    for(variant=0u;variant<4u;++variant) {
        (void)router.Dispatch("re_start_ui_control",addr(pair.data()));
        const auto h=*reinterpret_cast<const EffectSystemHandleWords*>(addr(image.data())+392u);
        check(EffectHandle::DialogNeedsUpdate(h,effects.ContentHandles()) &&
            EffectHandle::GetName(h,effects.ContentHandles())==std::string("CA")+std::to_string(15u+variant),"original four UI selection effects");
        std::array<float,3> pos{};
        check(effects.CombatEffectVector(h,"T1",pos,[&](std::uint32_t facet){
            check(facet==a+724u,"selection facet is actor724");
            return *reinterpret_cast<const std::array<float,3>*>(model.Model()->Address()+172u);
        }),"selection installs live actor facet");
        (void)router.Dispatch("re_close_ui_control",0u);
        check(!image[99],"selection close retires manager392 handle");
    }
    for(const auto* name:{"ae_protect_action","ae_critical_attack","ae_fend_offed","ae_fleed","me_join_stage"}) {
        const auto before=effects.CombatActiveCount();
        (void)router.Dispatch(name,addr(pair.data()));
        check(effects.CombatActiveCount()==before+1u,"real Combat event creates its original effect");
        std::cout<<"Native ActorManager event original effect: "<<name<<'\n';
    }
    combat[127]=1u;const auto before=effects.CombatActiveCount();(void)router.Dispatch("me_join_stage",0u);
    check(effects.CombatActiveCount()==before,"join effects skip actor payload before active phase");
    combat[127]=2u;kind=1u;
    for(const auto* name:{"ae_fleed","me_join_stage","ae_fend_offed"}) {
        const auto count=effects.CombatActiveCount();(void)router.Dispatch(name,addr(pair.data()));
        check(effects.CombatActiveCount()==count,"actor kind predicates retain original early return");
    }
    kind=2u;
    effects.BindCombatCamera(addr(camera.camera));
    unsigned event_pixels{};
    for(unsigned i=0;i<100u;++i){
        effects.UpdateCombatEffects(0.04);audio.Update();
        camera.Begin();effects.DrawCombatEffects(addr(camera.camera));event_pixels+=camera.EndPixels();
    }
    std::cout<<"Native original Combat event effects:100 update/draw frames with actual muted Miles service complete; pixels="
        <<event_pixels<<" remaining="<<effects.CombatActiveCount()<<" messages="<<messages<<'\n';
    check(router.Unsubscribe(message_observer),"manager message observer retired");
    check(router.Unsubscribe(observer),"manager event observer retired");handlers.Close();handlers.Close();
    check(!router.SubscriptionCount(),"all ten subscriptions retire");
    effects.ClearCombatEffects();effects.UnbindCombatEffects();effects.BindDialogFacetReader({});effects.BindSound(nullptr);audio.StopAll();model.Destroy();pool.Clear();
    std::cout<<"Native ActorManager ten callbacks: original prepare/selection/protect/fend/flee/join effects; critical EFF3 creation, inactive gates and subscription lifetime verified\n";
}
}
