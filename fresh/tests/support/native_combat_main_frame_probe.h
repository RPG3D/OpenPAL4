#pragma once
#include "pal4/native_combat_main_frame.h"
#include "pal4/native_combat_main_window.h"
#include "pal4/native_cegui_ui_session.h"
#include "pal4/native_script_event_router.h"
#include "pal4/native_event_handler_array.h"
#include "pal4/native_game_timer_queue.h"
#include "pal4/script_game_event_subscriptions.h"
#include "pal4/cegui_window_pointer_events.h"
#include "../../native/ui/cegui_exports_native.h"
#include <iostream>
#include <vector>
#include <stdexcept>
namespace pal4::test {
inline void VerifyNativeCombatMainFrame(NativeCeguiUiSession& ui,const std::filesystem::path& root){
    const auto check=[](bool ok,const char* reason){if(!ok)throw std::runtime_error(reason);};
    const auto reject=[&](auto&& f){bool failed{};try{f();}catch(const std::logic_error&){failed=true;}check(failed,"Combat main frame rejection");};
    NativeCombatMainWindow window(ui,root);
    NativeScriptEventRouter router;
    NativeCombatMainFrame* frame{};
    bool fail_activation{},fail_event{};unsigned keys{},clicks{};
    std::vector<std::pair<std::uint32_t,std::uint32_t>> events;
    NativeEventHandlerArray active([&](std::uint32_t o,std::uint32_t slot){
        check(frame!=nullptr,"Combat active owner published before visibility");
        if(fail_activation && slot==12u)throw std::runtime_error("intentional Combat input activation failure");
        return frame->InvokeVirtual(o,slot);
    },[&](std::uint32_t o,std::uint32_t slot,std::uint32_t event){return frame->InvokePredicate(o,slot,event);});
    NativeGameTimerQueue timers([&](std::uint32_t,std::uint32_t){throw std::runtime_error("cancelled Combat timer fired");});
    (void)timers.Construct();
    NativeCombatMainFrameServices services;
    services.game_event=[&](std::uint32_t entry,std::uint32_t payload){
        reject([&]{frame->Close();});reject([&]{frame->SetVisible(false);});reject([&]{window.Close();});
        if(fail_event)throw std::runtime_error("intentional Combat event consumer failure");events.emplace_back(entry,payload);
    };
    services.key_down=[&](std::uint32_t event){check(event==0x1234u,"Combat actual activity payload");++keys;reject([&]{frame->Close();});return true;};
    services.root_click=[&](std::uint32_t event){check(event!=0u,"Combat frame actual root click");++clicks;reject([&]{frame->Close();});return true;};
    reject([&]{NativeCombatMainFrame missing(window,ui,router,active,timers,0x2000u,{});});
    NativeCombatMainFrame owner(window,ui,router,active,timers,0x2000u,services);frame=&owner;
    reject([&]{NativeCombatMainFrame duplicate(window,ui,router,active,timers,0x2000u,services);});
    const auto module=GetModuleHandleW(L"CEGUIBase.dll");
    const auto parent=detail::Export<void* (__thiscall*)(const void*)>(module,"?getParent@Window@CEGUI@@QBEPAV12@XZ");
    owner.SetVisible(true);check(owner.SubscriptionCount()==19u && router.SubscriptionCount()==19u &&
        active.GetLast()==owner.InputAddress() && parent(reinterpret_cast<void*>(window.Root())),"real nineteen subscriptions, activity and desktop attachment");
    for(const auto& b:ScriptGameEventSubscriptions::Bindings()){
        const auto n=events.size();check(router.Dispatch(b.name,0x1234u)==1u && events.size()==n+1u &&
            events.back()==std::pair{b.callback,0x1234u},"all original callback IDs and payloads route once");
    }
    check(active.DispatchKeyDown(owner.InputAddress(),0x1234u)==1u && keys==1u && !active.Contains(0x1234u),"original input virtual and noncapture predicates");
    active.Update();
    {
        const auto fire=detail::Export<void (__thiscall*)(void*,const void*,void*,const void*)>(module,"?fireEvent@EventSet@CEGUI@@UAEXABVString@2@AAVEventArgs@2@0@Z");
        CeguiMouseEvent event{};event.vtable=reinterpret_cast<std::uint32_t>(detail::Export<const void*>(module,"??_7MouseEventArgs@CEGUI@@6B@"));event.window=window.Root();
        detail::NativeString empty(module,"");fire(reinterpret_cast<void*>(window.Root()),
            detail::Export<const void*>(module,"?EventMouseClick@Window@CEGUI@@2VString@2@B"),&event,empty.Data());
        window.RethrowClickError();check(clicks==1u,"frame click uses original root connection");
    }
    fail_event=true;bool failed{};try{(void)router.Dispatch("combat_start",0u);}catch(const std::runtime_error&){failed=true;}
    check(failed && owner.SubscriptionCount()==19u,"domain exception retains live registrations and releases guard");fail_event=false;
    owner.SetVisible(true);check(owner.SubscriptionCount()==19u && router.SubscriptionCount()==19u && active.Size()==2u,"repeat show rebinds nineteen and preserves original activity insert semantics");
    timers.AddCombatRefreshEvent("ResetCombatIcon",0.4f,window.Address());
    timers.AddRepeating("ResetCombatIcon",10.f,window.Address(),0x54ca80u);
    owner.SetVisible(false);check(!owner.SubscriptionCount() && !router.SubscriptionCount() && !active.Size() &&
        !parent(reinterpret_cast<void*>(window.Root())),"hide detaches, unregisters, removes activity and named timer");
    timers.Update(2.f);check(!timers.EventCount() && timers.Count()==1u,"hide cancels one-shot refresh while preserving same-name repeating timer");
    (void)timers.Remove("ResetCombatIcon");timers.Update(0.01f);check(!timers.Count(),"retire independent repeating timer fixture");
    for(const auto& b:ScriptGameEventSubscriptions::Bindings())check(!router.Dispatch(b.name,0x1234u),"hidden frame has no stale game callbacks");
    const auto word=[](std::uint32_t a){return *reinterpret_cast<const std::uint32_t*>(a);};
    check(!word(window.Address()+308u) && word(window.Address()+312u)==0xc0200000u,"original visibility tail resets");
    fail_activation=true;failed=false;try{owner.SetVisible(true);}catch(const std::runtime_error&){failed=true;}
    check(failed && !owner.SubscriptionCount() && !active.Size() && !parent(reinterpret_cast<void*>(window.Root())),"activation failure rolls back published callback/activity/root");fail_activation=false;
    owner.SetVisible(true);owner.Close();check(!owner.InputAddress() && !owner.SubscriptionCount() && !router.SubscriptionCount() && !active.Size(),"frame close after retry retires dependencies");
    reject([&]{owner.SetVisible(true);});
    {
        NativeCombatMainFrame replacement(window,ui,router,active,timers,0x2000u,services);frame=&replacement;
        replacement.SetVisible(true);check(replacement.SubscriptionCount()==19u,"frame can reuse borrowed original root");replacement.Close();
    }
    window.Close();active.ReleaseStorage();timers.Destroy();
    check(!active.OutstandingAllocations() && !timers.OutstandingAllocations(),"frame underlying activity/timer allocations retire");
    check(ui.SetCombatLayoutVisible(NativeCombatLayout::Main,false),"restore resource staging after frame probe");
    std::cout<<"Native Combat main frame: original19 subscriptions, actual parent/input/timer/click lifecycle, repeat show, guards, failure rollback and reuse pass; domain/input consumers are protocol fixtures\n";
}
}
