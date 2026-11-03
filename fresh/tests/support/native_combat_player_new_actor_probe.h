#pragma once
#include "native_combat_player_entry_probe.h"
#include "pal4/native_combat_actor_retirement.h"
#include "pal4/native_actor_instance_pool.h"
#include "pal4/native_combat_player_scene_actors.h"
#include "pal4/native_combat_actor_action_tasks.h"
#include "pal4/native_combat_message_children.h"
#include <map>
namespace pal4::test {
inline void VerifyNativeCombatPlayerNewActor(NativePlayerManagerSession& source,
    NativeActorInstancePool& pool,NativeInventorySession& inventory,const NativeGameDatabase& database,
    NativeSkillRecordCatalog& records,INativeSkillSystemServices& system,NativeEffectSceneSession& effects) {
    const auto check=[](bool ok,const char* message){if(!ok)throw std::runtime_error(message);};
    const auto reject=[&](auto&& f){bool denied=false;try{f();}catch(const std::logic_error&){denied=true;}
        check(denied,"Player creation callback expected rejection");};
    NativeScriptEventRouter router;
    NativeCombatPlayerManager players(source,router,system);
    NativeCombatSkillActorRegistry registry(router,records,system);
    NativePlayerCollectionCatalog passives(database);
    struct Entry {std::unique_ptr<NativeCombatPlayerModelCache> model;std::unique_ptr<NativeCombatActorSkillRuntime> skills;};
    std::map<std::uint32_t,Entry> services;
    NativeCombatPlayerMotion motion(players,system,router,[&](std::uint32_t a)->NativeCombatPlayerMotionBinding {
        auto& e=services.at(a);if(!e.model || !e.skills)throw std::logic_error("Player new-actor services absent");
        return {*e.model,*e.skills};
    });
    std::uint32_t payload_now{};bool throw_after{};
    std::vector<std::uint32_t> selected;std::vector<std::uint8_t> results;
    std::function<void(std::uint32_t,std::uint32_t)> initialize;
    initialize=[&](std::uint32_t a,std::uint32_t slot){
        reject([&]{(void)players.AddPlayer(0u);});reject([&]{players.Close();});
        reject([&]{(void)players.HandleNewActor(payload_now,initialize);});
        auto& e=services[a];
        check(!e.model && !e.skills,"Player creation initializes each actor once");
        e.model=std::make_unique<NativeCombatPlayerModelCache>(players.Actor(a),pool,inventory,120300);
        e.skills=std::make_unique<NativeCombatActorSkillRuntime>(players.Actor(a),registry,inventory);
        InitializeNativeCombatActor(players.Actor(a),*e.model,*e.skills,passives,effects,router,motion,slot);
        check(players.Actor(a).Slot()==slot && e.model->Current().Address() && e.skills->ReadyForAction() &&
              _stricmp(e.model->Current().Asset()->CurrentAction().c_str(),"z01")==0,
              "creation callback finishes actual52CFD0 before returning");
        selected.push_back(slot);
    };
    const auto id=router.Subscribe(router.Key("re_new_added"),1u,[&](std::uint32_t payload){
        payload_now=payload;
        const auto result=players.HandleNewActor(payload,initialize);results.push_back(result);
        if(result==1u)reject([&]{(void)players.HandleNewActor(payload,initialize);});
        if(throw_after){throw_after=false;throw std::logic_error("intentional observer failure after Player initialization");}
    });
    struct Unsubscribe {NativeScriptEventRouter& r;std::uint32_t id;~Unsubscribe(){(void)r.Unsubscribe(id);}} unsub{router,id};
    for(auto slot:{2u,2u,99u,99u})(void)players.AddPlayer(0u,slot);
    check(selected==std::vector<std::uint32_t>{2u,0u,1u} && results==std::vector<std::uint8_t>{1u,1u,1u,3u} &&
          players.Count()==4u && players.StageCount()==3u && registry.Count()==3u,
          "actual Player creation handles specified, occupied, invalid and full three-slot stage");
    const auto full=players.ActorAt(3u);
    check(!*reinterpret_cast<const std::uint32_t*>(full+808u) && players.Actor(full).Slot()==99u && !services.contains(full),
          "full Player roster retains unmodeled offstage fourth actor");
    // Give the offstage actor its empty cache solely for the original576 leave
    // adapter used by the existing52DE40 clear path; no model or skills load.
    services[full].model=std::make_unique<NativeCombatPlayerModelCache>(players.Actor(full),pool,inventory,120300);
    {
        NativeCombatPlayerSceneActors scene(players,router);
        for(auto& [actor,entry]:services)scene.Register(*entry.model);
        check(scene.ValidateStageModels()==3u && !scene.SceneChild(full) &&
              !services[full].model->Count() && !services[full].model->OwnedModels(),
              "four-Player startup validates three staged models without loading the reserve");
        scene.Close();
    }
    {
        NativeCombatActorSkillRuntime reserve(players.Actor(full),registry,inventory);
        NativeCombatActorActionTasks playback(*services[full].model,system,router);
        unsigned resolutions{};float scale=1.f;
        NativeCombatMessageChildren children(reserve,playback,
            [&]() -> NativeActorInstanceOwner& {++resolutions;return services[full].model->Current();},
            effects,router,scale);
        check(!resolutions && !children.Count(),"reserve message owner must not resolve an absent model at construction");
        const auto subscriptions=router.SubscriptionCount();
        const auto message=children.Create("z16");
        children.BeginMessage(message);
        check(!resolutions && children.MessageComplete(message) && children.SubscriptionCount()==1u &&
              router.SubscriptionCount()==subscriptions+2u &&
              !*reinterpret_cast<const std::uint32_t*>(full+808u),
              "reserve level-up message uses actual actor564 null-child action and name/flag completion without loading a model");
        children.StopMessage(message);children.Destroy(message);
        check(!children.Count() && !children.SubscriptionCount() && !children.OwnedAllocations() && !children.OwnedTexts() && router.SubscriptionCount()==subscriptions,
              "reserve level-up message retires subscriptions, effects, text and task storage");
        children.Close();
    }
    const auto clear=[&]{
        players.ClearActors([&](std::uint32_t a){
            auto& e=services.at(a);
            (void)RetireNativeCombatActor(players.Actor(a),effects.ContentHandles(),[&]{(void)e.model->LeaveStage(router);});
        },[&](std::uint32_t a){
            auto& e=services.at(a);if(e.skills)e.skills->Close();e.skills.reset();e.model->Close();services.erase(a);
        });
        check(!players.Count() && !players.StageCount() && !registry.Count() && services.empty() && players.SubscriptionCount()==3u,
              "actual Player creation owners clear while manager subscriptions remain reusable");
    };
    clear();selected.clear();results.clear();
    throw_after=true;reject([&]{(void)players.AddPlayer(0u,6u);});
    check(players.Count()==1u && players.StageCount()==1u && registry.Count()==1u && selected==std::vector<std::uint32_t>{0u},
          "observer failure retains inserted and fully staged Player");
    (void)players.AddPlayer(0u,0u);
    check(selected==std::vector<std::uint32_t>{0u,1u},"creation after observer exception restores dispatch pins and occupancy");
    const auto old=players.ActorAt(0u);clear();
    std::uint32_t facet=old+716u;
    reject([&]{(void)players.HandleNewActor(reinterpret_cast<std::uint32_t>(&facet),initialize);});
    players.Close();
    std::cout<<"Native57B120 new Player: actual creation callback, requested2/duplicate2/invalid99/full3, real52CFD0/models/z01, recursion guards, observer failure, slot reuse and reusable clear pass\n";
}
}
