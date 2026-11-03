#pragma once
#include "pal4/native_combat_player_model_cache.h"
#include "pal4/native_actor_instance_pool.h"
#include "pal4/native_combat_player_actor_owner.h"
#include "pal4/native_combat_player_manager.h"
#include "pal4/game_resource_manager.h"
#include "pal4/native_animated_actor_asset.h"
#include "pal4/native_animation_event_queue_runtime.h"
#include <rphanim.h>
#include <array>
#include <cstring>
#include <iostream>
#include <stdexcept>
namespace pal4::test {
inline void VerifyNativeCombatPlayerModelCache(NativePlayerManagerSession& source,
    NativeInventorySession& inventory,NativePkFileManagerRuntime& packages,
    const std::filesystem::path& root,std::uint32_t game,INativeSkillSystemServices& skills,
    NativeEffectSceneSession& effects,NativeScriptEventRouter& router) {
    const auto check=[](bool ok,const char* m){if(!ok)throw std::runtime_error(m);};
    const auto word=[](std::uint32_t a){std::uint32_t v;std::memcpy(&v,reinterpret_cast<const void*>(a),4u);return v;};
    const auto reject=[&](auto&& f){bool denied=false;try{f();}catch(const std::logic_error&){denied=true;}check(denied,"Player model cache expected rejection");};
    inventory.Reset();bool inactive=true;unsigned created{};
    NativeActorInstancePool pool([&]{return inactive;},[&](std::string_view name){
        auto model=std::make_unique<NativeActorInstanceOwner>();(void)model->Construct(game);
        model->Load(root,name);model->BindHandResources(root,packages,effects,skills,[&]{return inactive;});
        ++created;return model;
    });
    NativeCombatPlayerActorOwner actor;const auto a=actor.Construct(source,0u);
    NativeCombatPlayerModelCache cache(actor,pool,inventory,120300);
    check(cache.Activate()==0u && !cache.Count() && !cache.OwnedModels() && !word(a+808u),
          "inactive Combat does not fabricate cached Player models");
    reject([&]{NativeCombatPlayerModelCache duplicate(actor,pool,inventory,120300);});
    reject([&]{actor.Destroy();});
    inactive=false;
    std::array<std::uint32_t,4> models{};unsigned index{};
    for(auto name:{"101","103","106","105"})models[index++]=cache.FindOrLoad(name);
    check(created==4u && cache.Count()==4u && cache.OwnedModels()==4u &&
          word(a+1304u)-word(a+1296u)==16u && word(a+1300u)==word(a+1304u),
          "actual Player cache grows1/2/4 and owns all four real models");
    check(cache.FindOrLoad("103")==models[1] && created==4u,"cache exact-name hit avoids pool/factory");
    (void)cache.Activate();
    check(cache.Current().Address()==models[0] && word(a+808u)==models[0] &&
          word(models[0]+492u)==a && word(models[0]+516u)==a && word(models[0]+644u),
          "shared5390F0 publishes actual model, binds real Player and loads equipment");
    const auto hand=word(models[0]+644u);
    const auto time=cache.Current().Asset()->Hierarchy()->currentAnim->currentTime;
    (void)cache.Current().Update(0.02f,router);
    check(cache.Current().Asset()->Hierarchy()->currentAnim->currentTime>time,
          "actual cached Player advances original HAnim");
    reject([&]{(void)cache.Activate();});
    // A live actor callback may read its model, but must not retire or replace
    // that model while the queue/skill/turn caller still borrows its resources.
    class Activity final : public IAnimationEventQueueActivityRuntime {
    public:
        std::function<void()> hook;
        void DeleteAnimation(std::uint32_t) override {}
        void AdvanceAnimation(std::uint32_t,float) override {hook();}
        void CallAnimationEvent(std::uint32_t,std::uint32_t) override {hook();}
        std::uint32_t AnimationFinished(std::uint32_t) override {return 0u;}
    } activity;
    std::array<std::uint32_t,14> task{};
    actor.BindAnimationActivities(activity);
    struct RetireActivity {
        NativeCombatPlayerActorOwner& actor;
        ~RetireActivity(){try{actor.ClearAnimations();actor.UnbindAnimationActivities();}catch(...){}}
    } retire_activity{actor};
    (void)actor.AddAnimation(reinterpret_cast<std::uint32_t>(task.data()),false);
    activity.hook=[&]{
        check(cache.Current().Address()==models[0],"actor callback retains readable real Player model");
        cache.PlayAction("Z01",1u,skills,router);
        reject([&]{cache.Close();});
        reject([&]{cache.DeactivateCurrent();});
        reject([&]{(void)cache.FindOrLoad("101");});
        reject([&]{(void)cache.Activate();});
        reject([&]{(void)cache.EnterStage(router);});
        reject([&]{(void)cache.LeaveStage(router);});
        reject([&]{cache.RestoreRenderCallbacks();});
        check(cache.OwnedModels()==4u && word(a+808u)==models[0] &&
              word(models[0]+644u)==hand,"callback rejection preserves body/cache/hand ownership");
    };
    actor.UpdateAnimations(0.01f);actor.DispatchAnimations(0u);
    auto guarded=activity.hook;
    activity.hook=[&]{guarded();throw std::runtime_error("intentional modeled Player callback failure");};
    bool callback_failed{};
    try{actor.UpdateAnimations(0.01f);}catch(const std::runtime_error&){callback_failed=true;}
    check(callback_failed,"modeled Player callback failure propagates");
    activity.hook=guarded;actor.UpdateAnimations(0.01f);
    actor.ClearAnimations();actor.UnbindAnimationActivities();
    std::cout<<"Native Player cache lifetime: actual body/hand, queue update/dispatch and exception recovery reject model mutation while keeping reads available; task protocol fixture\n";
    cache.DeactivateCurrent();check(!word(a+808u) && cache.Count()==4u,"deactivation retains owned local cache");
    (void)cache.Activate();check(cache.Current().Address()==models[0] && word(models[0]+644u)==hand,
          "reactivation reuses real body and owned hand");
    const auto saved=word(a+1300u),foreign=saved+4u;
    std::memcpy(reinterpret_cast<void*>(a+1300u),&foreign,4u);
    reject([&]{cache.Close();});std::memcpy(reinterpret_cast<void*>(a+1300u),&saved,4u);
    check(word(a+808u)==models[0] && cache.OwnedModels()==4u,"foreign cache bounds reject before model transfer");
    cache.Close();cache.Close();
    check(!cache.Count() && !cache.OwnedModels() && !word(a+808u) &&
          !word(a+1296u) && !word(a+1300u) && !word(a+1304u) && pool.CachedCount()==4u,
          "local cache retirement returns all models and clears published ownership");
    reject([&]{(void)cache.FindOrLoad("101");});actor.ValidateDetached();actor.Destroy();
    NativeCombatPlayerActorOwner second;const auto b=second.Construct(source,0u);
    NativeCombatPlayerModelCache next(second,pool,inventory,120300);(void)next.Activate();
    check(next.Current().Address()==models[0] && word(models[0]+492u)==b && created==4u,
          "second actual Player borrows pooled model without stale parent association");
    next.Close();second.Destroy();pool.Clear();
    {
        NativeCombatPlayerManager players(source,router,skills);
        const auto player=players.AddPlayer(0u);auto& owner=players.Actor(player);
        NativeCombatPlayerModelCache stage(owner,pool,inventory,120300);
        (void)stage.FindOrLoad("103");(void)stage.FindOrLoad("106");
        std::vector<std::string> events;
        std::array<std::uint32_t,4> observers{};
        const std::array<const char*,4> names{"re_join_stage","re_join_scene","re_leave_stage","re_leave_scene"};
        bool fail_join=false;
        for(unsigned i=0;i<4u;++i)observers[i]=router.Subscribe(router.Key(names[i]),3u,[&,i](std::uint32_t payload){
            check(word(payload)==player+716u,"actual Player stage event facet identity");
            events.emplace_back(names[i]);
            if(i==1u){
                check(players.StageCount()==1u && stage.Current().Address()==word(player+808u),
                      "real stage list precedes scene event and exposes matching owned model");
                reject([&]{stage.Close();});
                if(fail_join)throw std::logic_error("intentional Player scene observer failure");
            }
        });
        struct Unsubscribe {NativeScriptEventRouter& r;std::array<std::uint32_t,4>& ids;
            ~Unsubscribe(){for(auto id:ids)(void)r.Unsubscribe(id);}} unsubscribe{router,observers};
        const std::array<ResourceVec3,3> expected{{{-58,0,172},{8,0,156},{70,0,170}}};
        for(unsigned slot=0;slot<3u;++slot){
            owner.SetSlot(slot);events.clear();(void)stage.EnterStage(router);
            const auto position=word(player+808u)+172u;
            check(!std::memcmp(reinterpret_cast<const void*>(position),&expected[slot],sizeof(ResourceVec3)),
                  "Player formation uses original three IDA positions");
            check(events==std::vector<std::string>{"re_join_stage","re_join_scene"} && players.StageCount()==1u,
                  "native Player entry sends original stage then scene order");
            events.clear();(void)stage.LeaveStage(router);
            check(events==std::vector<std::string>{"re_leave_stage","re_leave_scene","re_leave_scene","re_leave_scene"} &&
                  !players.StageCount() && !word(player+808u) && stage.Count()==3u,
                  "actual Player leave removes stage then visits all local cached models");
        }
        fail_join=true;reject([&]{(void)stage.EnterStage(router);});
        check(players.StageCount()==1u && word(player+808u),"throwing scene listener retains original preceding stage/current writes");
        fail_join=false;(void)stage.LeaveStage(router);stage.Close();players.Close();pool.Clear();
        check(!players.StageCount(),"Player stage exception recovery retires real manager and cache");
        std::cout<<"Native Player stage: real manager registration, IDA formation3, facet order, cached leave and observer failure recovery pass\n";
    }
    check(!pool.CachedCount() && !effects.CombatXmlEffectCount() && !effects.CombatActiveCount(),
          "Player cache and global pool explicit teardown drains model/effect owners");
    std::cout<<"Native Player model cache: actual4 models growth1/2/4, activate/equip/update/reuse, pool transfer/rebind and teardown pass\n";
}
}
