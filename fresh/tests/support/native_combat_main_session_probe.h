#pragma once
#include "pal4/native_combat_main_session.h"
#include "pal4/native_combat_main_window.h"
#include "pal4/native_combat_main_action_ready.h"
#include "pal4/native_combat_console.h"
#include "pal4/native_combat_system_image.h"
#include "pal4/native_combat_task_queue.h"
#include "pal4/native_combat_actor_skill_runtime.h"
#include "pal4/native_script_event_router.h"
#include "pal4/native_event_handler_array.h"
#include "pal4/native_game_timer_queue.h"
#include "pal4/cegui_window_pointer_events.h"
#include "../../native/ui/cegui_exports_native.h"
#include <array>
#include <functional>
#include <iostream>
#include <stdexcept>
namespace pal4::test {
// Uses the production session/consumers, actual global activity array, timers
// and DLL click. run_attacks is the actual command/AMF/end/counter regression.
inline void VerifyNativeCombatMainSession(NativeCombatMainSession& session,NativeCombatMainWindow& window,
    NativeScriptEventRouter& router,NativeEventHandlerArray& active,NativeGameTimerQueue& timers,
    NativeCombatMainActionReady& ready,NativeCombatConsole& console,NativeCombatTaskQueue& queue,
    NativeCombatActorSkillRuntime& source,std::uint32_t target,NativeCombatSystemImage& image,
    const NativeCombatActorSkillRuntime::ActionMotionRefresh& motion,const std::function<void()>& run_attacks){
    const auto check=[](bool ok,const char* why){if(!ok)throw std::runtime_error(why);};
    const auto word=[](std::uint32_t a)->std::uint32_t&{return *reinterpret_cast<std::uint32_t*>(a);};
    const auto subscriptions=router.SubscriptionCount(),activities=active.Size(),previous=active.GetLast();
    const auto state=image.State();
    const std::array saved{word(window.Address()+268u),word(window.Address()+272u),image.translator.words[2],image.translator.words[3]};
    struct Restore{
        NativeCombatMainSession& session;NativeScriptEventRouter& router;NativeCombatTaskQueue& queue;
        NativeCombatSystemImage& image;NativeCombatMainWindow& window;std::uint32_t state,guard{};std::array<std::uint32_t,4> saved;
        ~Restore(){if(guard)(void)router.Unsubscribe(guard);try{queue.ResetForStart();session.SetVisible(false);}catch(...){}
            image.State()=state;auto* p=reinterpret_cast<std::uint32_t*>(window.Address());p[67]=saved[0];p[68]=saved[1];
            image.translator.words[2]=saved[2];image.translator.words[3]=saved[3];}
    } restore{session,router,queue,image,window,state,0u,saved};
    session.SetVisible(true);
    const auto input=session.InputAddress();
    check(session.SubscriptionCount()==19u && router.SubscriptionCount()==subscriptions+19u &&
        active.GetLast()==input && active.Size()==activities+1u,"production Main owns all19 subscriptions and actual global input");
    (void)router.Dispatch("combat_start",0u);
    std::array<std::uint32_t,5> key{};
    for(auto code:{1u,28u}){key[4]=code;check(active.DispatchKeyDown(input,reinterpret_cast<std::uint32_t>(key.data()))==1u,
        "actual global Main activity handles no-target Esc and Enter");}
    unsigned guards{};
    const auto actor=source.ActorAddress();
    restore.guard=router.Subscribe(router.Key("ae_start_action"),1u,[&](std::uint32_t p){
        if(word(p)!=actor+716u)return;bool rejected{};
        try{session.Close();}catch(const std::logic_error&){rejected=true;}check(rejected,"DLL Main click pins complete frame during actual StartAction");++guards;
    });
    const std::array pair{actor+716u,target};(void)router.Dispatch("joint_attack_respond",reinterpret_cast<std::uint32_t>(pair.data()));
    const auto module=GetModuleHandleW(L"CEGUIBase.dll");
    const auto fire=detail::Export<void (__thiscall*)(void*,const void*,void*,const void*)>(module,"?fireEvent@EventSet@CEGUI@@UAEXABVString@2@AAVEventArgs@2@0@Z");
    CeguiMouseEvent click{};click.vtable=reinterpret_cast<std::uint32_t>(detail::Export<const void*>(module,"??_7MouseEventArgs@CEGUI@@6B@"));click.window=window.Root();
    detail::NativeString empty(module,"");fire(reinterpret_cast<void*>(window.Root()),
        detail::Export<const void*>(module,"?EventMouseClick@Window@CEGUI@@2VString@2@B"),&click,empty.Data());window.RethrowClickError();
    check(guards && queue.Count()==1u && !word(window.Address()+268u) && !word(window.Address()+272u),
        "actual Main DLL click traverses real joint hint/select event/CheckProtect route once");
    (void)router.Unsubscribe(restore.guard);restore.guard=0u;
    queue.ResetForStart();source.EndAction(motion);
    const std::array<std::uint32_t,2> none{};(void)router.Dispatch("select_join_atk",reinterpret_cast<std::uint32_t>(none.data()));
    //The actual Monster57C370/Main portrait/timer chain is now exercised by
    //run_attacks through the production AI probe. Player ready/UI remains open.
    const std::array foreign{0x1000u,target};bool rejected{};
    try{(void)router.Dispatch("joint_attack_respond",reinterpret_cast<std::uint32_t>(foreign.data()));}catch(const std::invalid_argument&){rejected=true;}
    check(rejected && session.SubscriptionCount()==19u && active.GetLast()==input,"consumer failure preserves real frame registrations and releases dispatch pin");
    run_attacks();
    check(session.SubscriptionCount()==19u && active.GetLast()==input && !queue.Count(),"actual attack/end/counter events retain coherent Main lifecycle");
    session.SetVisible(false);
    check(!session.SubscriptionCount() && router.SubscriptionCount()==subscriptions && active.Size()==activities && active.GetLast()==previous,
        "Main hide removes all19 routes and restores actual previous global activity");
    session.SetVisible(true);session.SetVisible(false);
    check(!session.SubscriptionCount() && router.SubscriptionCount()==subscriptions && active.GetLast()==previous,"same production Main can reenter without stale callbacks");
    std::cout<<"Native production Main session: actual19 consumers, global key activity, DLL joint click, attack/end/counter, failure pin, hide/reentry and input restoration pass\n";
}
}
