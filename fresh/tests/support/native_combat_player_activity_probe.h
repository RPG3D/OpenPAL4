#pragma once
#include "pal4/native_combat_player_actor_owner.h"
#include "pal4/native_combat_player_manager.h"
#include "pal4/native_combat_player_scene_actors.h"
#include "pal4/native_animation_event_queue_runtime.h"
#include "pal4/native_script_event_router.h"
#include <array>
#include <bit>
#include <functional>
#include <iostream>
#include <limits>
#include <set>
#include <vector>
#include <stdexcept>
namespace pal4::test {
// Actual copied Player actors/manager/queues; activity virtual calls below are
// a protocol fixture, not reconstructed game actions or battle acceptance.
inline void VerifyNativeCombatPlayerActivities(NativePlayerManagerSession& source,INativeSkillSystemServices& skills){
    const auto check=[](bool ok,const char* m){if(!ok)throw std::runtime_error(m);};
    const auto reject=[&](auto&& f){bool denied{};try{f();}catch(const std::logic_error&){denied=true;}check(denied,"Player activity expected rejection");};
    const auto addr=[](const void* p){return reinterpret_cast<std::uint32_t>(p);};
    NativeScriptEventRouter router;
    NativeCombatPlayerManager manager(source,router,skills);
    const auto first=manager.AddPlayer(0u),second=manager.AddPlayer(1u);
    auto& a=manager.Actor(first);auto& b=manager.Actor(second);
    NativeCombatPlayerSceneActors actors(manager,router);
    std::array<std::uint32_t,14> one{},two{},delayed{};
    const auto id1=addr(one.data()),id2=addr(two.data()),id3=addr(delayed.data());
    class Activity final : public IAnimationEventQueueActivityRuntime {
    public:
        std::vector<std::uint32_t> advanced,deleted,events;
        std::set<std::uint32_t> finished;
        std::function<void()> hook;
        void DeleteAnimation(std::uint32_t id)override{deleted.push_back(id);}
        void AdvanceAnimation(std::uint32_t id,float)override{advanced.push_back(id);if(hook)hook();}
        void CallAnimationEvent(std::uint32_t id,std::uint32_t arg)override{events.push_back(id);events.push_back(arg);if(hook)hook();}
        std::uint32_t AnimationFinished(std::uint32_t id)override{return finished.contains(id)?1u:0u;}
    } activity;
    struct Retire {NativeCombatPlayerActorOwner& a;NativeCombatPlayerActorOwner& b;Activity& s;
        ~Retire(){s.hook={};try{a.ClearAnimations();a.UnbindAnimationActivities();b.ClearAnimations();b.UnbindAnimationActivities();}catch(...){}}} retire{a,b,activity};
    actors.UpdateActors(0.0f);reject([&]{(void)a.AddAnimation(id1,true);});
    reject([&]{a.UpdateAnimations(std::numeric_limits<float>::quiet_NaN());});
    const auto allocations_a=a.OwnedAllocations(),allocations_b=b.OwnedAllocations();
    a.BindAnimationActivities(activity);b.BindAnimationActivities(activity);
    reject([&]{manager.Close();});check(manager.Count()==2u,"bound activity prevents partial manager teardown");
    (void)a.AddAnimation(id1,true);(void)a.AddAnimation(id3,false,true);(void)b.AddAnimation(id2,false);
    reject([&]{a.UnbindAnimationActivities();});
    check(std::bit_cast<float>(delayed[13])==0.2f,"Player original stagger interval");
    actors.UpdateActors(0.05f);
    check(activity.advanced==std::vector<std::uint32_t>{id1,id2} && std::bit_cast<float>(one[12])==0.05f &&
          std::bit_cast<float>(two[12])==0.05f && manager.StageCount()==0u,
          "first-list queues update in order including all offstage Players");
    activity.advanced.clear();actors.UpdateActors(0.2f);
    check(activity.advanced==std::vector<std::uint32_t>{id1,id2} && std::bit_cast<float>(delayed[13])<0.0f,
          "positive delay consumes entire frame without immediate task advance");
    activity.hook=[&]{reject([&]{actors.UpdateActors(0.0f);});reject([&]{manager.Close();});
        reject([&]{(void)manager.AddPlayer(2u);});reject([&]{a.UpdateAnimations(0.0f);});
        reject([&]{a.ClearAnimations();});reject([&]{a.UnbindAnimationActivities();});
        reject([&]{a.SetSlot(0u);});throw std::runtime_error("intentional Player activity failure");};
    bool failed{};try{actors.UpdateActors(0.0f);}catch(const std::runtime_error&){failed=true;}
    check(failed && !manager.ActiveIterators() && a.AnimationCount()==2u,"throw restores queue/manager/adapter guards");
    activity.hook={};a.DispatchAnimations(37u);
    check(activity.events==std::vector<std::uint32_t>{id1,37u,id3,37u},"Player task slot8 argument/order");
    activity.events.clear();manager.DispatchAnimations(53u);
    check(activity.events==std::vector<std::uint32_t>{id1,53u,id3,53u,id2,53u} && !manager.StageCount(),
          "aggregate529B70 visits both first-list Player queues even when offstage");
    activity.hook=[&]{reject([&]{manager.DispatchAnimations(53u);});reject([&]{manager.Close();});
        reject([&]{(void)manager.AddPlayer(2u);});throw std::runtime_error("intentional Player render callback failure");};
    failed=false;try{manager.DispatchAnimations(53u);}catch(const std::runtime_error&){failed=true;}
    check(failed && !manager.ActiveIterators(),"render callback exception releases Player first cursor and dispatch pin");
    activity.hook={};activity.events.clear();manager.DispatchAnimations(53u);
    check(activity.events.size()==6u,"aggregate render retries after callback exception");
    activity.finished.insert(id1);activity.advanced.clear();actors.UpdateActors(0.01f);
    check(activity.advanced==std::vector<std::uint32_t>{id1,id3,id2} && activity.deleted==std::vector<std::uint32_t>{id1} &&
          a.AnimationCount()==1u,"owned finished task deleted, delayed task advances next frame");
    a.ClearAnimations();b.ClearAnimations();
    check(activity.deleted.size()==1u && a.OwnedAllocations()==allocations_a && b.OwnedAllocations()==allocations_b,
          "unowned activities retained and queue nodes fully retired");
    (void)b.AddAnimation(id2,true);b.ClearAnimations();
    check(activity.deleted==std::vector<std::uint32_t>{id1,id2},"explicit clear deletes owned task");
    a.UnbindAnimationActivities();b.UnbindAnimationActivities();
    check(!a.AnimationActivitiesBound() && !b.AnimationActivitiesBound(),"Player activity services unbound");
    actors.Close();
    std::cout<<"Native Player activities: actual2 offstage queues, delay/owned/dispatch, exception/iterator guards and cleanup pass; task protocol fixture\n";
}
}
