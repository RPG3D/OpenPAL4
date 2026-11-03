#pragma once
#include "pal4/native_combat_hand_resources.h"
#include "pal4/native_actor_instance_owner.h"
#include "pal4/actor_animation_control.h"
#include "pal4/native_animated_actor_asset.h"
#include "pal4/native_player_slot_copy_runtime.h"
#include "pal4/native_skill_system_owner.h"
#include "pal4/native_player_combat_events.h"
#include "pal4/random_number_generator.h"
#include "pal4/native_actor_user_data.h"
#include "pal4/native_rw_geometry_processor.h"
#include "pal4/native_base_object_memory.h"
#include <rphanim.h>
#include "pal4/native_script_event_router.h"
#include "pal4/effect_handle.h"
#include "native_particle_probe_camera.h"
#include <array>
#include <cstring>
#include <iostream>
#include <stdexcept>
namespace pal4::test {
inline void VerifyNativeCombatHandResources(NativeEffectSceneSession& effects,NativeScriptEventRouter& router,
    NativePkFileManagerRuntime& packages,const std::filesystem::path& root,
    std::uint32_t combat_game,std::uint32_t combat_camera) {
    const auto check=[](bool ok,const char* m){if(!ok)throw std::runtime_error(m);};
    const auto address=[](const void* p){return reinterpret_cast<std::uint32_t>(p);};
    const auto reject=[&](auto&& f){bool denied=false;try{f();}catch(const std::logic_error&){denied=true;}check(denied,"hand resource expected rejection");};
    NativeAnimatedActorAsset parent;check(parent.Load(root,"103"),"hand resource original parent103");
    NativePlayerCombatEvents events;RandomNumberGenerator random;NativeSkillSystemOwner skills(random,events);
    (void)skills.Construct("H:/PAL4/objs");
    std::array<std::uint32_t,128> game{};NativeActorInstanceOwner owner;
    const auto actor=owner.Construct(address(game.data()));const auto component=actor+624u;
    auto view=std::span<std::uint32_t,116>(reinterpret_cast<std::uint32_t*>(component),116u);
    std::array<std::uint32_t,116> right_before{};std::memcpy(right_before.data(),reinterpret_cast<const void*>(actor+1088u),464u);
    NativePlayerSlotCopyRuntime texts;bool inactive=true;
    const auto package_count=packages.LoadedPackageCount();
    NativeCombatHandResources hand(view,*parent.Main(),root,packages,texts,effects,skills,[&]{return inactive;});
    ActorLoadRequest request;request.animation_context=address(parent.Main());request.animation_name="hook_LHWeapon";
    request.animation_id=1u;request.actor_name="WTA";request.texture_name="WTA1";request.alternate_resource=true;
    request.primary_text="ZL-20";request.dialog_text="ZL-20";request.secondary_text="OM03";
    std::array<std::uint32_t,116> initial{};std::copy(view.begin(),view.end(),initial.begin());
    check(hand.Load(request)==1u && !std::memcmp(initial.data(),view.data(),464u),"inactive hand load preserves real component");
    inactive=false;
    check(hand.Load(request)==0u && hand.Clump() && view[6]==1u && view[9]==1u && view[115]==1u,
          "shared hand load publishes actual active resource");
    auto* target=NativeActorUserData::FindAtomic(parent.Main(),"hook_LHWeapon");
    check(target && RwFrameGetParent(RpClumpGetFrame(hand.Clump()))==RpAtomicGetFrame(target),"owned weapon bound to actual103 hook");
    const auto handle=[&](unsigned o){return *reinterpret_cast<const EffectSystemHandleWords*>(component+o);};
    check(effects.CombatXmlEffectCount()==2u && handle(404u).system,"loader creates original XML and ordinary effects");
    auto* first=hand.Clump();const auto old_dialog=handle(404u).id;
    // Requests may borrow all currently owned strings; shared reload/reset must
    // not invalidate those views before the runtime has copied them.
    request.actor_name=reinterpret_cast<const char*>(view[1]);request.texture_name=reinterpret_cast<const char*>(view[4]);
    request.primary_text=reinterpret_cast<const char*>(view[104]);
    request.dialog_text=reinterpret_cast<const char*>(view[108]);request.secondary_text=reinterpret_cast<const char*>(view[112]);
    check(hand.Load(request)==0u && hand.Clump()==first && handle(404u).id!=old_dialog,
          "same-model reuse preserves clump but refreshes effects using aliased owned text");
    NativeParticleProbeCamera camera;effects.UpdateCombatEffects(0.04);camera.Begin();effects.DrawCombatEffects(0u);(void)camera.EndPixels();
    request.actor_name="OM07";request.texture_name="";request.alternate_resource=false;
    request.primary_text="";request.dialog_text="";request.secondary_text="";
    bool throw_release=true;unsigned observer{};
    observer=router.Subscribe(router.Key("effect_end"),1u,[&](std::uint32_t payload){
        if(reinterpret_cast<const std::uint32_t*>(payload)[0]!=handle(404u).id)return;
        reject([&]{hand.Close();});
        if(throw_release)throw std::logic_error("intentional owned hand release failure");
    });
    struct Unsubscribe {NativeScriptEventRouter& r;unsigned& id;~Unsubscribe(){if(id)(void)r.Unsubscribe(id);}} unsubscribe{router,observer};
    reject([&]{(void)hand.Load(request);});
    check(hand.Clump()==first && !effects.CombatXmlEffectCount() && handle(404u).system,
          "failed replacement retains old clump and pending ordinary handle");
    throw_release=false;
    check(hand.Load(request)==0u && hand.Clump() && std::string_view(reinterpret_cast<const char*>(view[2]))=="PALObject\\OM07\\",
          "replacement retries then loads actual PALObject through Script3");
    check(!effects.CombatXmlEffectCount() && !handle(404u).system && packages.LoadedPackageCount()==package_count,
          "object replacement drains previous effects and restores package stack");
    check(router.Unsubscribe(observer),"owned hand observer retirement");observer=0u;
    request.actor_name="WTA";request.texture_name="WTA1";request.alternate_resource=true;
    check(hand.Load(request)==0u && std::string_view(reinterpret_cast<const char*>(view[2]))=="PALWeapon\\WTA\\",
          "original weapon loads again after object replacement");
    const auto saved=view[5];view[5]=0u;reject([&]{hand.Close();});view[5]=saved;
    hand.SetActive(0u);hand.SetActive(1u);hand.Close();hand.Close();
    check(!hand.Clump() && !view[5] && !view[1] && !view[37] && !view[38] && !view[115] &&
          !texts.OwnedTextCount() && !effects.CombatXmlEffectCount(),"hand close releases resources, texts, dictionaries and effects");
    check(!std::memcmp(right_before.data(),reinterpret_cast<const void*>(actor+1088u),464u),"resource loader leaves other hand untouched");
    owner.Destroy();
    // Exercise the production owner's public loading and cleanup path against
    // its own body rather than borrowing a separate parent fixture.
    NativeActorInstanceOwner integrated;(void)integrated.Construct(address(game.data()));integrated.Load(root,"103");
    integrated.BindHandResources(root,packages,effects,skills,[&]{return inactive;});
    ActorAttachmentLoadArgs args;args.model_name="WTA";args.texture_name="WTA1";args.alternate_resource=true;
    args.animation_id=1u;args.primary_text="ZL-20";args.dialog_text="ZL-20";args.secondary_text="OM03";
    check(integrated.LoadHand(false,args)==0u && integrated.LoadHand(true,args)==0u &&
          effects.CombatXmlEffectCount()==4u,"actual ActorInstance owns both loaded hands and effects");
    const auto left_clump=*reinterpret_cast<const std::uint32_t*>(integrated.Address()+624u+20u);
    const auto right_clump=*reinterpret_cast<const std::uint32_t*>(integrated.Address()+1088u+20u);
    check(left_clump && right_clump && left_clump!=right_clump,"actual ActorInstance distinct hand resources");
    struct Queue final : IRwGeometryQueueAllocator {
        std::array<std::uint32_t,4> words{};NativeBaseObjectMemory memory;
        std::uint32_t Address(){return reinterpret_cast<std::uint32_t>(words.data());}
        Queue(){NativeRwGeometryProcessor::BindQueueAllocator(Address(),this);}
        ~Queue(){NativeRwGeometryProcessor::UnbindQueueAllocator(Address(),this);}
        std::uint32_t AllocateQueueBytes(std::uint32_t n)override{return memory.Allocate(n);}
        void FreeQueueBytes(std::uint32_t a)override{(void)memory.Free(a);}
    } queue;
    struct World {RpWorld* p{};~World(){if(p)RpWorldDestroy(p);}} world;
    RwBBox bounds{{10000,10000,10000},{-10000,-10000,-10000}};
    world.p=RpWorldCreate(&bounds);check(world.p,"equipped model world");
    std::array<std::uint32_t,1024> actor_image{};
    (void)integrated.InitializeFrame();(void)integrated.BindParent(address(actor_image.data()));
    integrated.Activate(address(world.p),queue.Address());
    check(*reinterpret_cast<std::uint8_t*>(integrated.Address()+1084u)==1u &&
          *reinterpret_cast<std::uint8_t*>(integrated.Address()+1548u)==1u,"equipped activation keeps both hands active");
    const auto time=integrated.Asset()->Hierarchy()->currentAnim->currentTime;
    (void)integrated.Update(0.02f,router);
    check(integrated.Asset()->Hierarchy()->currentAnim->currentTime>time,"equipped body advances original HAnim");
    integrated.Deactivate();actor_image[808u/4u]=integrated.Address();
    check(!effects.CombatXmlEffectCount(),"equipped action begins with inactive hands");
    integrated.PlayAction(address(actor_image.data()),"Z01",1u,skills);
    check(effects.CombatXmlEffectCount()==4u,"shared action reactivates both equipped hand owners");
    // Both hands share the original transparency queue and shared alpha path.
    (void)ActorAnimationControl::SetAlpha(integrated.Address(),0.5f,queue.Address(),integrated.AnimationRuntime());
    camera.BeginAt({0.0f,60.0f,-300.0f});
    integrated.RenderSlot(address(camera.camera),52u);
    check(queue.words[2]>queue.words[1],"equipped render submits actual transparent atomics");
    (void)NativeRwGeometryProcessor::RenderQueued(queue.Address());
    (void)NativeRwGeometryProcessor::ClearQueued(queue.Address());
    (void)camera.EndPixels();
    check(*reinterpret_cast<std::uint32_t*>(integrated.Address()+624u+28u)==1u &&
          *reinterpret_cast<std::uint32_t*>(integrated.Address()+1088u+28u)==1u,"both actual weapon render paths executed");
    integrated.ReleaseToPool();
    check(!effects.CombatXmlEffectCount() && !*reinterpret_cast<std::uint8_t*>(integrated.Address()+1084u) &&
          !*reinterpret_cast<std::uint8_t*>(integrated.Address()+1548u),"equipped pool return deactivates real hand effects");
    camera.BeginAt({0.0f,60.0f,-300.0f});
    (void)RpClumpRender(reinterpret_cast<RpClump*>(left_clump));
    (void)RpClumpRender(reinterpret_cast<RpClump*>(right_clump));
    check(queue.words[2]==queue.words[1],"returned weapons restore SDK callbacks and stop borrowing transparent queue");
    (void)camera.EndPixels();
    (void)integrated.BindParent(address(actor_image.data()));
    integrated.Activate(address(world.p),queue.Address());
    check(effects.CombatXmlEffectCount()==4u,"equipped cached model reactivation recreates both hand effects");
    // Foreign right-hand ownership must reject before camera/left-hand writes.
    const auto right_field=integrated.Address()+1088u+20u;
    *reinterpret_cast<std::uint32_t*>(right_field)=0u;
    reject([&]{integrated.Deactivate();});
    check(effects.CombatXmlEffectCount()==4u,"preflight rejects foreign second hand before first changes");
    *reinterpret_cast<std::uint32_t*>(right_field)=right_clump;
    integrated.ReleaseToPool();
    integrated.ClearHandResources();integrated.ClearHandResources();
    check(!effects.CombatXmlEffectCount(),"actual ActorInstance releases hand effects before body");
    // Original 63AE20 leaves the identifier untouched when its owner is null.
    // Such unowned IDs must not prevent re-binding or destruction.
    *reinterpret_cast<std::uint32_t*>(integrated.Address()+624u+388u)=17u;
    *reinterpret_cast<std::uint32_t*>(integrated.Address()+1088u+396u)=23u;
    // 63AE20 also leaves the owner untouched for a negative XML identifier.
    // That opaque tag can belong to a retired Combat group; never dereference it.
    *reinterpret_cast<std::uint32_t*>(integrated.Address()+624u+396u)=0xffffffffu;
    *reinterpret_cast<std::uint32_t*>(integrated.Address()+624u+400u)=0x13579bdfu;
    *reinterpret_cast<std::uint32_t*>(integrated.Address()+1088u+388u)=0xffffffffu;
    *reinterpret_cast<std::uint32_t*>(integrated.Address()+1088u+392u)=0x2468ace0u;
    integrated.BindHandResources(root,packages,effects,skills,[&]{return inactive;});
    check(integrated.LoadHand(false,args)==0u,"actual ActorInstance hand services rebind after clear");
    integrated.Destroy();
    check(!integrated.OwnedAllocations() && !integrated.OwnedTexts(),"actual ActorInstance Destroy retires populated hands");
    NativeActorInstanceOwner retired;
    (void)retired.Construct(combat_game);retired.Load(root,"103");
    retired.BindHandResources(root,packages,effects,skills,[&]{return inactive;});
    check(retired.LoadHand(false,args)==0u,"retired-group fixture loads a real equipped hand");
    const auto old=*reinterpret_cast<const EffectSystemHandleWords*>(retired.Address()+624u+388u);
    check(old.system && static_cast<std::int32_t>(old.id)>=0,"retired-group fixture has a real positive XML handle");
    effects.EndCombatGameObject();
    auto& registered=effects.ContentHandles();
    check(!registered.ContainsOwner(registered.RegisterSystem(),old.system),"actual battle end unregisters the old hand effect owner");
    retired.Destroy();
    check(!retired.OwnedAllocations() && !retired.OwnedTexts(),"model destruction after effect-group end retires resource views");
    check(effects.InitializeCombatGameObject(combat_game,combat_camera),"original group reinitializes after old model destruction");
    skills.Destroy();effects.ClearCombatEffects();
    check(!owner.OwnedAllocations() && !owner.OwnedTexts() && !skills.OwnedAllocations() &&
          packages.LoadedPackageCount()==package_count,"explicit resource fixture retirement balance");
    std::cout<<"Native529D00 hand resources: original103/WTA/WTA1 -> OM07 -> WTA, inactive gate, aliased same-model reuse, XML/ordinary effects, real hook/texture/config, callback failure/retry, foreign receiver, opposite-hand and package/explicit teardown balance pass\n";
}
}
