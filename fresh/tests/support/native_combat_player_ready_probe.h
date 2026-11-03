#pragma once
#include "pal4/native_combat_player_ready_events.h"
#include "pal4/native_combat_action_frame.h"
#include "pal4/native_combat_action_window.h"
#include "pal4/native_combat_main_session.h"
#include "pal4/native_combat_main_window.h"
#include "pal4/native_combat_event_handlers.h"
#include "pal4/native_combat_player_manager.h"
#include "pal4/native_combat_monster_manager.h"
#include "pal4/native_combat_roster_targets.h"
#include "pal4/native_combat_actor_skill_runtime.h"
#include "pal4/native_combat_actor_turn_services.h"
#include "pal4/native_script_event_router.h"
#include "pal4/native_combat_system_image.h"
#include "pal4/native_event_handler_array.h"
#include "pal4/native_cegui_ui_session.h"
#include "../../native/ui/cegui_exports_native.h"
#include <bit>
#include <functional>
#include <iostream>
#include <vector>
namespace pal4::test {
inline void VerifyNativeCombatPlayerReady(NativeCombatPlayerReadyEvents& ready,NativeCombatActionFrame& frame,
    NativeCombatActionWindow& window,NativeCombatMainSession& main,NativeCombatMainWindow& main_window,
    NativeCombatEventHandlers& handlers,NativeCombatPlayerManager& players,NativeCombatMonsterManager& monsters,
    NativeCombatRosterTargets& targets,NativeCombatActorSkillRuntime& source,INativeCombatActorTurnServices& turn,
    NativeScriptEventRouter& router,NativeCombatSystemImage& image,NativeEventHandlerArray& active,
    NativeCeguiUiSession& ui,const std::function<void()>& dismiss_tutorial,const std::function<void(float)>& advance){
    const auto check=[](bool b,const char* why){if(!b)throw std::runtime_error(why);};
    const auto word=[](std::uint32_t p)->std::uint32_t&{return *reinterpret_cast<std::uint32_t*>(p);};
    const auto a=source.ActorAddress(),previous=active.GetLast(),activities=active.Size(),subscriptions=router.SubscriptionCount();
    struct Restore {
        NativeCombatActionFrame& frame;NativeCombatMainSession& main;NativeScriptEventRouter& router;
        NativeCombatEventHandlers& handlers;NativeCombatSystemImage& image;
        std::uint32_t ready_actor,control_actor,state;bool started;std::uint8_t mode;
        std::vector<std::pair<std::uint32_t,std::uint32_t>> fields;std::uint32_t observer{};
        ~Restore(){if(observer)(void)router.Unsubscribe(observer);try{frame.SetVisible(false);main.SetVisible(false);}catch(...){}
            for(auto [p,v]:fields)*reinterpret_cast<std::uint32_t*>(p)=v;
            handlers.SetReadyActor(ready_actor);handlers.SetControlActor(control_actor);handlers.SetUiControlStarted(started);image.State()=state;image.Mode()=mode;}
        void Save(std::uint32_t p){fields.emplace_back(p,*reinterpret_cast<std::uint32_t*>(p));}
    } restore{frame,main,router,handlers,image,handlers.ReadyActor(),handlers.ControlActor(),image.State(),handlers.UiControlStarted(),image.Mode()};
    for(unsigned o=340u;o<668u;o+=4u)restore.Save(a+o);
    for(auto o:{672u,720u,724u,828u,832u})restore.Save(a+o);
    for(auto o:{88u,60u,0x1e4u})restore.Save(window.Address()+o);
    for(auto o:{268u,272u})restore.Save(main_window.Address()+o);
    image.Mode()=0u;image.State()=3u;word(a+828u)=1u;word(a+636u)=word(a+648u)=0u;word(a+832u)=0u;
    main.SetVisible(true);frame.SetVisible(true,main_window.Root());
    check(frame.SubscriptionCount()==3u && router.SubscriptionCount()==subscriptions+22u && !window.Word(88u),"Action actual frame registers original three events alongside Main19 and starts with hidden panel");
    bool rejected{};try{window.Close();}catch(const std::logic_error&){rejected=true;}check(rejected,"live Action frame prevents window retirement");
    unsigned starts{};bool fail_observer{};
    restore.observer=router.Subscribe(router.Key("re_start_ui_control"),3u,[&](std::uint32_t payload){
        if(!payload)return; // The separate malformed-envelope case belongs to the real frame consumer.
        check(word(payload)==a+716u && image.State()==4u && handlers.ReadyActor()==a && handlers.ControlActor()==a,
            "actual Player ready publishes state4 and both actor fields before UI start dispatch");
        bool guarded{};try{ready.Close();}catch(const std::logic_error&){guarded=true;}check(guarded,"Player ready remains pinned across UI event callbacks");
        guarded=false;try{ready.Handle(handlers.ExtendedAddress(),payload);}catch(const std::logic_error&){guarded=true;}check(guarded,"Player ready reentry rejected");
        guarded=false;try{source.Close();}catch(const std::logic_error&){guarded=true;}check(guarded,"actual source registry remains pinned during UI event");
        ++starts;if(fail_observer)throw std::runtime_error("Player ready UI observer failure fixture");
    });
    const auto tutorials=frame.TutorialRequests();handlers.SetUiControlStarted(false);source.SetActionProgress(.75f);
    source.CanTakeAction(turn);dismiss_tutorial();
    const auto visible=detail::Export<bool (__thiscall*)(const void*,bool)>(GetModuleHandleW(L"CEGUIBase.dll"),"?isVisible@Window@CEGUI@@QBE_N_N@Z");
    check(starts==1u && frame.TutorialRequests()==tutorials+1u && handlers.UiControlStarted() && window.Word(88u)==a &&
        active.GetLast()==window.Address()+40u && visible(reinterpret_cast<void*>(window.Word(0x1e0u)),false),
        "actual CanTakeAction/re_can_take_action/57C290 -> motion -> original ShowDialog/tutorial9 -> actual panel and global input");
    check(source.ActionProgress()==.75f,"Player input-wait branch preserves progress until an action is chosen");
    (void)router.Dispatch("re_close_ui_control",0u);check(!window.Word(88u) && active.GetLast()==main.InputAddress(),"real close-ui event hides panel and restores Main activity");
    const auto facet=a+716u;
    handlers.SetUiControlStarted(false);fail_observer=true;bool failed{};
    try{ready.Handle(handlers.ExtendedAddress(),reinterpret_cast<std::uint32_t>(&facet));}catch(const std::runtime_error&){failed=true;}
    fail_observer=false;dismiss_tutorial();(void)router.Dispatch("re_close_ui_control",0u);
    check(failed && !handlers.UiControlStarted() && !players.ActiveIterators() && !monsters.ActiveIterators() && !targets.ActiveIterators(),
        "real synchronous UI event failure prevents final started flag and releases all roster pins");
    ready.Handle(handlers.ExtendedAddress(),reinterpret_cast<std::uint32_t>(&facet));dismiss_tutorial();
    check(handlers.UiControlStarted() && window.Word(88u)==a,"same Player ready owner recovers after event exception");
    (void)router.Dispatch("combat_start",0u);check(!window.Word(88u) && active.GetLast()==main.InputAddress(),"actual null combat_start clears Action context and input");
    const auto before=starts;word(a+828u)=0u;source.SetActionProgress(.75f);
    ready.Handle(handlers.ExtendedAddress(),reinterpret_cast<std::uint32_t>(&facet));
    check(starts==before && source.ActionProgress()==0.f,"original not-ready branch resets actual progress without opening Player UI");word(a+828u)=1u;
    // Original inactive/primary5/6 branches run actual Player AI. A reversible
    // missing-config type selects the original no-action path, without inventing
    // a normal attack or requiring other still-unbound AI action domains.
    const auto ai_type=word(a+672u);word(a+672u)=0x7fffffffu;unsigned fallbacks{};
    for(unsigned branch=0;branch<3u;++branch){
        image.Mode()=branch==0u?1u:0u;word(a+636u)=branch==0u?0u:branch==1u?5u:6u;image.State()=3u;
        source.SetActionProgress(.75f);const auto before_fallback=starts;
        ready.Handle(handlers.ExtendedAddress(),reinterpret_cast<std::uint32_t>(&facet));
        check(starts==before_fallback && source.ActionProgress()==0.f && image.State()==3u && !window.Word(88u),
            "original inactive/primary5/6 Player AI no-config branch resets progress without UI start");++fallbacks;
    }
    word(a+672u)=ai_type;word(a+636u)=0u;image.Mode()=0u;
    rejected=false;try{ready.Handle(0u,reinterpret_cast<std::uint32_t>(&facet));}catch(const std::invalid_argument&){rejected=true;}check(rejected,"foreign extended owner rejected by Player callback");
    const auto monster_facet=monsters.StageActorAt(0u)+716u;rejected=false;
    try{ready.Handle(handlers.ExtendedAddress(),reinterpret_cast<std::uint32_t>(&monster_facet));}catch(const std::invalid_argument&){rejected=true;}check(rejected,"actual Monster cannot enter Player ready callback");
    rejected=false;try{(void)router.Dispatch("re_start_ui_control",0u);}catch(const std::invalid_argument&){rejected=true;}check(rejected,"Action frame rejects malformed event and clears partial UI state");
    (void)router.Unsubscribe(restore.observer);restore.observer=0u;
    advance(.5f);advance(.01f);advance(2.1f);advance(.01f);
    frame.SetVisible(false);check(!frame.SubscriptionCount(),"original Action frame hide removes all three subscriptions");
    frame.SetVisible(true,main_window.Root());check(frame.SubscriptionCount()==3u,"Action frame reentry recreates exactly original subscriptions");
    failed=false;try{frame.SetVisible(true,1u);}catch(const std::runtime_error&){failed=true;}
    check(failed && !frame.SubscriptionCount() && !window.Word(88u),"invalid native frame parent rolls back subscriptions and dialog state");
    frame.SetVisible(true,main_window.Root());check(frame.SubscriptionCount()==3u,"Action frame recovers after parent failure");frame.SetVisible(false);main.SetVisible(false);
    check(active.Size()==activities && active.GetLast()==previous && router.SubscriptionCount()==subscriptions && !targets.ActiveIterators() &&
        !players.ActiveIterators() && !monsters.ActiveIterators(),"Player UI flow restores original input/subscriptions and zero cursors");
    std::cout<<"Native Player ready: AI_fallbacks="<<fallbacks<<" actual CanTakeAction/re_can_take_action/57C290, state4/actor fields/motion, Action3 subscriptions, original dialog/tutorial9, DLL panel/global input, close/combat_start, observer exception recovery, not-ready reset, guards and frame hide/reentry pass; this fixture does not verify action selection or complete battles\n";
}
}
