#pragma once
#include "pal4/native_combat_monster_manager.h"
#include "pal4/native_combat_world_resources.h"
#include "pal4/native_combat_world_scene_events.h"
#include "pal4/native_actor_animation_events.h"
#include <rphanim.h>
#include <rtanim.h>
#include <cmath>
#include "pal4/native_world_asset.h"
#include "pal4/native_animated_actor_asset.h"
#include "pal4/native_rw_geometry_processor.h"
#include "pal4/actor_animation_control.h"
#include "pal4/native_combat_actor_model_owner.h"
#include "pal4/native_combat_actor_owner.h"
#include "pal4/native_animation_event_queue_runtime.h"
#include "pal4/native_actor_instance_pool.h"
#include "pal4/native_monster_record_catalog.h"
#include "pal4/native_skill_record_catalog.h"
#include "pal4/native_skill_system_owner.h"
#include "pal4/native_player_combat_events.h"
#include "pal4/native_script_event_router.h"
#include "pal4/native_combat_world_frame_services.h"
#include "pal4/random_number_generator.h"
#include <array>
#include <bit>
#include <iostream>
#include <functional>
#include <limits>
#include <string>
namespace pal4::test {
template<class Capture>
void VerifyNativeCombatMonsterRendering(const std::filesystem::path& root,NativeCombatWorldResources& world,
    RwCamera* camera,Capture&& capture,const std::vector<std::uint32_t>& baseline) {
    const auto check=[](bool ok,const char* msg){if(!ok)throw std::runtime_error(msg);};
    const auto word=[](unsigned a)->unsigned&{return *reinterpret_cast<unsigned*>(a);};
    NativeGameDatabase db;check(db.Load(root),"Monster render original DB");
    NativeMonsterRecordCatalog records(db);NativeSkillRecordCatalog skills(db);
    NativeScriptEventRouter router;NativePlayerCombatEvents events;RandomNumberGenerator random;
    NativeSkillSystemOwner system(random,events);(void)system.Construct("H:/PAL4/objs");
    std::array<unsigned,128> game{};
    NativeActorInstancePool pool([]{return false;},[&](std::string_view name){
        auto model=std::make_unique<NativeActorInstanceOwner>();model->Construct(reinterpret_cast<unsigned>(game.data()));
        model->Load(root,name);return model;
    });
    NativeCombatMonsterManager manager(pool,records,skills,router,system);
    const auto before_scene_subscriptions=router.SubscriptionCount();
    NativeCombatWorldSceneEvents scene_events(world,router,manager);
    check(router.SubscriptionCount()==before_scene_subscriptions+4u,"world owns four original scene subscriptions");
    const auto a=manager.CreateMonster(2579,0u),b=manager.CreateMonster(2580,1u);
    // The third original actor intentionally has no child: real loops skip it.
    const auto absent=manager.CreateMonster(2580,2u);
    for(auto actor:{a,b})manager.JoinStageAndScene(actor);
    const auto child=manager.ModelOwner(a).Model()->Address();
    check(word(child+24u)==1u && word(child+36u)==1u,"native ActorInstance preserves original LoadClump mode1 and active gate");
    struct Restore {
        NativeCombatMonsterManager& manager;NativeCombatWorldResources& world;
        ~Restore(){world.ClearGeometryQueues();for(std::size_t i=0;i<manager.Count();++i)
            if(auto* model=manager.ModelOwner(manager.ActorAt(i)).Model())NativeRwGeometryProcessor::RestoreClump(model->Asset()->Main());}
    } restore{manager,world};
    class Frame final : public INativeCombatWorldFrameServices {
    public:
        NativeCombatMonsterManager& manager;unsigned queue,draws{},components{},queued{};
        std::function<void()> during_draw;
        Frame(NativeCombatMonsterManager& m,unsigned q):manager(m),queue(q){}
        unsigned Address() const override{return reinterpret_cast<unsigned>(this);}
        static unsigned Word(unsigned a){return *reinterpret_cast<unsigned*>(a);}
        void ProcessActorUpdates(unsigned camera) override{
            manager.RenderModels(camera,52u);++draws;queued=(Word(queue+8u)-Word(queue+4u))/8u;
            if(during_draw)during_draw();
        }
        unsigned UpdateActorComponents(unsigned camera) override{
            if(Word(queue+8u)!=Word(queue+4u))throw std::runtime_error("Monster shadow phase preceded transparency drain");
            manager.RenderModels(camera,36u);++components;return 0x55u;
        }
    } frame(manager,world.GeometryQueue());
    const auto camera_address=reinterpret_cast<unsigned>(camera);
    const auto rejects=[&](auto&& f,const char* message){bool rejected=false;try{f();}catch(const std::logic_error&){rejected=true;}check(rejected,message);};
    const auto world_address=reinterpret_cast<unsigned>(world.Asset().World());
    rejects([&]{world.Reset();},"bound scene events protect world lifetime");
    rejects([&]{manager.Close();},"bound scene events protect Monster lifetime");
    rejects([&]{NativeCombatWorldSceneEvents duplicate(world,router,manager);},"duplicate world scene binding rejects");
    const auto dispatch=[&](std::string_view name,unsigned actor){
        const std::array record{actor?actor+716u:0u};
        (void)router.Dispatch(name,reinterpret_cast<unsigned>(record.data()));
    };
    for(auto name:{"me_join_scene","me_leave_scene","re_join_scene","re_leave_scene"})dispatch(name,0u);
    dispatch("me_join_scene",absent);dispatch("me_leave_scene",absent);
    rejects([&]{dispatch("re_join_scene",a);},"unbound Player owners cannot use Monster models");
    rejects([&]{dispatch("me_join_scene",a+4u);},"foreign Monster scene facet rejects before dereferencing actor image");
    const auto saved_facet=word(child+492u); // parent linkage survives leave.
    manager.LeaveStageAndScene(a);
    check(manager.StageCount()==1u && word(child+172u)==0u && word(child+176u)==0x7f7fffffu &&
        word(child+180u)==0u && word(child+492u)==saved_facet,"real leave event moves to original FLT_MAX sentinel without pool return");
    // Restore through real resource/join events before drawing.
    manager.JoinStageAndScene(a);
    check(manager.StageCount()==2u && manager.SceneChild(a)==child,"join refresh restores model position and original stage list");
    rejects([&]{manager.ModelOwner(a).Activate(0u,world.GeometryQueue());},"activation rejects absent world");
    rejects([&]{manager.ModelOwner(a).Activate(world_address,0u);},"activation rejects absent queue");
    // The actual52ADF0 path replaces the prior fixture-only queue setup.
    // Original B01/B02 lack shadow resources, so6664A0 genuinely returns early.
    dispatch("me_join_scene",a);dispatch("me_join_scene",b);
    check(*reinterpret_cast<unsigned char*>(child+1084u)==1u &&
          *reinterpret_cast<unsigned char*>(child+1548u)==1u,"scene activation sets both empty-hand active flags");
    manager.ModelOwner(a).Activate(world_address,world.GeometryQueue());
    rejects([&]{manager.RenderModels(camera_address,52u);},"Monster render requires begun world camera");
    rejects([&]{manager.RenderModels(camera_address,99u);},"Monster render rejects unsupported phase");
    frame.during_draw=[&]{
        rejects([&]{dispatch("me_join_scene",a);},"scene activation cannot mutate callbacks during draw");
        rejects([&]{scene_events.Close();},"scene Close cannot clear queues during draw");
    };
    const auto opaque=capture(nullptr,&frame,camera_address);
    frame.during_draw={};
    unsigned differences{};for(std::size_t i=0;i<opaque.size();++i)if(opaque[i]!=baseline[i])++differences;
    check(differences>10u && word(child+28u)==1u && frame.draws==1u && frame.components==1u,
          "actual manager/body render with original frustum gate changes M01 GPU pixels");
    const auto* first_asset=manager.ModelOwner(a).Model()->Asset();
    auto* first_time=&first_asset->Hierarchy()->currentAnim->currentTime;
    const auto second_child=manager.ModelOwner(b).Model()->Address();
    auto* second_time=&manager.ModelOwner(b).Model()->Asset()->Hierarchy()->currentAnim->currentTime;
    const auto initial_time=*first_time;
    manager.UpdateStageModels(0.02f);
    check(*first_time>initial_time && *second_time>0.0f,"actual stage models advance through original child56 and HAnim");
    check(std::isfinite(*reinterpret_cast<float*>(child+524u)) &&
          *reinterpret_cast<float*>(child+524u)>*reinterpret_cast<float*>(child+536u) &&
          word(child+584u)==word(child+600u),"actual child bounds initialized and per-frame radius copied");
    rejects([&]{manager.UpdateStageModels(-1.0f);},"negative elapsed time rejects");
    rejects([&]{manager.UpdateStageModels(std::numeric_limits<float>::quiet_NaN());},"NaN elapsed time rejects");
    word(child+444u)=1u;const auto paused=*first_time;manager.UpdateStageModels(0.01f);
    check(*first_time==paused,"original animation pause flag suppresses time advancement");word(child+444u)=0u;
    word(child+416u)=std::bit_cast<unsigned>(0.5f);manager.UpdateStageModels(0.02f);
    check(std::fabs(*first_time-paused-0.01f)<0.001f,"original model animation time scale is respected");
    word(child+416u)=std::bit_cast<unsigned>(1.0f);
    dispatch("me_leave_stage",b);const auto offstage=*second_time;manager.UpdateStageModels(0.01f);
    check(*second_time==offstage,"first-list model outside stage list does not advance");dispatch("me_join_stage",b);
    dispatch("me_join_stage",absent);manager.UpdateStageModels(0.0f);dispatch("me_leave_stage",absent);
    // Protocol fixture for the second53B3E0 loop; actual CEGUI drawing remains
    // separate. The null-child actor is also offstage, but its728 must advance.
    class Activity final : public IAnimationEventQueueActivityRuntime {
    public:
        std::function<void()> advance;
        unsigned calls{};
        void DeleteAnimation(unsigned) override {throw std::logic_error("borrowed fixture cannot delete");}
        void AdvanceAnimation(unsigned,float) override {advance();++calls;}
        void CallAnimationEvent(unsigned,unsigned) override {throw std::logic_error("unexpected UI render in update");}
        unsigned AnimationFinished(unsigned) override {return 0u;}
    } activity;
    auto& animation_actor=manager.ModelOwner(absent).Actor();
    std::array<unsigned,18> floating{};floating[13]=std::bit_cast<unsigned>(-1.0f);
    animation_actor.BindAnimationActivities(activity);
    const auto absent_model=word(absent+808u);
    rejects([&]{manager.ModelOwner(absent).Destroy();},"bound UI services protect model ownership before detaching");
    check(word(absent+808u)==absent_model,"rejected model destruction preserves child");
    animation_actor.AddAnimation(reinterpret_cast<unsigned>(floating.data()),false);
    const auto before_full_update=*first_time;
    activity.advance=[&]{
        check(*first_time>before_full_update,"stage model update precedes first-list UI update");
        rejects([&]{manager.Close();},"UI callback cannot close manager");
        rejects([&]{manager.UpdateActors(0.0f);},"UI callback cannot recursively update manager");
        rejects([&]{dispatch("me_join_stage",absent);},"UI callback cannot mutate stage list");
    };
    manager.UpdateActors(0.01f);
    check(activity.calls==1u && std::fabs(std::bit_cast<float>(floating[12])-0.01f)<0.0001f,
        "full Monster update advances offstage null-child UI queue once");
    animation_actor.ClearAnimations();animation_actor.UnbindAnimationActivities();
    // Observe actual original AMF events; do not inject synthetic messages.
    std::string event_action;
    for(std::size_t i=0;i<first_asset->ActionCount();++i){
        NativeActorAnimationEvents amf;
        if(amf.Load(root,"B01",first_asset->ActionResourceAt(i)) && !amf.Events().empty()){
            event_action=first_asset->ActionAt(i);break;
        }
    }
    check(!event_action.empty(),"original B01 AMF contains a real event-bearing action");
    unsigned animation_messages{};
    const auto message_subscription=router.Subscribe(router.Key("ae_animation_message"),4u,[&](unsigned payload){
        check(word(payload)==a+716u && word(payload+4u)!=0u,"real Combat animation message carries actor facet and original AMF record");
        ++animation_messages;
        rejects([&]{manager.Close();},"animation callback cannot retire actor owners");
        rejects([&]{manager.UpdateStageModels(0.01f);},"animation callback cannot recursively advance stage");
        rejects([&]{dispatch("me_leave_stage",a);},"animation callback cannot mutate the stage list");
    });
    manager.ModelOwner(a).PlayAction(event_action,0u,system);
    const auto duration=first_asset->Hierarchy()->currentAnim->pCurrentAnim->duration;
    // Pause the second model so this observer receives only the selected actor.
    word(second_child+444u)=1u;
    manager.UpdateStageModels(duration+0.1f);
    check(animation_messages>0u && word(child+440u)==1u,"real AMF dispatch and original completion flag are reached");
    const auto completed_messages=animation_messages;manager.UpdateStageModels(0.01f);
    check(animation_messages==completed_messages,"original event fired flag prevents duplicate emission");
    (void)router.Unsubscribe(message_subscription);word(second_child+444u)=0u;
    manager.ModelOwner(a).PlayAction("Z01",1u,system);
    std::cout<<"Native Monster update: real stage56, HAnim time/pause/scale/completion, AMF messages="<<animation_messages
             <<", bounds, null/offstage models and callback guards pass\n";
    for(auto actor:{a,b}){
        auto& model=manager.ModelOwner(actor);
        (void)ActorAnimationControl::SetAlpha(model.Model()->Address(),0.5f,world.GeometryQueue(),model.AnimationRuntime());
    }
    (void)capture(nullptr,&frame,camera_address);
    check(frame.queued>0u && frame.components==2u,"real Monster52 phase populates actual world queue before Monster36 phase");
    // Original visibility gate suppresses a body without advancing animation.
    word(child+36u)=0u;
    const auto second=manager.ModelOwner(b).Model()->Address();word(second+36u)=0u;
    (void)capture(nullptr,&frame,camera_address);
    check(!frame.queued,"disabled original model gate produces no transparent records");
    word(child+36u)=1u;word(second+36u)=1u;
    const auto sphere=std::array{word(child+224u),word(child+228u),word(child+232u),word(child+236u)};
    word(child+224u)=std::bit_cast<unsigned>(1000000.0f);
    (void)capture(nullptr,&frame,camera_address);
    check(word(child+28u)==0u,"real SDK frustum rejection clears original visible flag");
    for(unsigned i=0;i<4u;++i)word(child+224u+4u*i)=sphere[i];
    // The debug skeleton branch must not silently succeed when requested.
    word(child+464u)=1u;word(child+468u)=1u;
    word(child+428u)=1u;(void)capture(nullptr,&frame,camera_address);word(child+428u)=0u;
    rejects([&]{(void)capture(nullptr,&frame,camera_address);},"missing skeleton drawing rejects through actual render phase");
    word(child+464u)=0u;word(child+468u)=0u;world.ClearGeometryQueues();
    // Original667590 requires all three resources; populated camera rendering
    // remains an explicit missing service, while the genuine empty gate passes.
    const auto shadow=word(child+48u);
    // B01 has no shadow resource on this installation. Borrow its actual SDK
    // body solely as a non-null test resource to exercise the missing-camera
    // boundary; the rejected adapter never dereferences it as a shadow.
    word(child+48u)=word(child+20u);word(child+52u)=1u;
    rejects([&]{dispatch("me_join_scene",a);},
        "activation of populated shadow resources requires real camera initialization");
    word(child+44u)=1u;
    rejects([&]{(void)capture(nullptr,&frame,camera_address);},"populated shadow camera must require real runtime");
    word(child+44u)=0u;word(child+52u)=0u;word(child+48u)=shadow;world.ClearGeometryQueues();
    dispatch("me_join_scene",a);
    (void)capture(nullptr,&frame,camera_address);
    check(word(child+28u)==1u,"native manager render retries after explicit missing-service branches");
    std::cout<<"Native Monster render: original B01/B02, mode1, GPU differences="<<differences
             <<", actual52ADF0 activation and52/36 loops, queue drain, frustum, null child, missing-service guards and retry pass\n";
    const auto before_close=router.SubscriptionCount();
    scene_events.Close();scene_events.Close();
    check(router.SubscriptionCount()+4u==before_close,
        "scene Close removes exactly its four subscriptions");
    bool restored=true;
    for(auto actor:{a,b})RpClumpForAllAtomics(manager.ModelOwner(actor).Model()->Asset()->Main(),
        [](RpAtomic* atomic,void* result)->RpAtomic*{
            *static_cast<bool*>(result)&=reinterpret_cast<unsigned>(RpAtomicGetRenderCallBack(atomic))!=
                NativeRwGeometryProcessor::RenderCallbackAddress();return atomic;
        },&restored);
    check(restored,"scene Close restores actual SDK callbacks before releasing world binding");
    std::cout<<"Native world scene events: four mode3 subscriptions, actual Monster join/leave, FLT_MAX, lifetime guards, draw guards, callback restoration and retry pass\n";
    manager.Close();pool.Clear();system.Destroy();
}
}
