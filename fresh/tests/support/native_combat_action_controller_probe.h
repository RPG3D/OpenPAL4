#pragma once
#include "pal4/native_combat_action_controller.h"
#include "pal4/native_combat_command_selection.h"
#include "pal4/native_combat_action_window.h"
#include "pal4/native_combat_selector_layout.h"
#include "pal4/native_combat_action_frame.h"
#include "pal4/native_combat_main_session.h"
#include "pal4/native_combat_main_window.h"
#include "pal4/native_combat_actor_skill_runtime.h"
#include "pal4/native_combat_actor_turn_services.h"
#include "pal4/native_combat_event_handlers.h"
#include "pal4/native_combat_target_selection.h"
#include "pal4/native_combat_roster_targets.h"
#include "pal4/native_event_handler_array.h"
#include "pal4/native_script_event_router.h"
#include "pal4/native_combat_system_image.h"
#include "pal4/cegui_window_pointer_events.h"
#include "../../native/ui/cegui_exports_native.h"
#include <array>
#include <cstring>
#include <functional>
#include <iostream>
#include <vector>
#include <string_view>
namespace pal4::test {
inline void VerifyNativeCombatActionController(NativeCombatActionController& controller,NativeCombatCommandSelection& commands,
    NativeCombatActionWindow& window,NativeCombatActionFrame& frame,NativeCombatMainSession& main,NativeCombatMainWindow& main_window,
    NativeCombatActorSkillRuntime& source,INativeCombatActorTurnServices& turn,NativeCombatEventHandlers& handlers,
    NativeCombatTargetSelection& selection,NativeCombatRosterTargets& targets,NativeEventHandlerArray& active,
    NativeScriptEventRouter& router,NativeCombatSystemImage& image,std::uint32_t target,
    const std::function<void()>& dismiss_tutorial,const std::function<void(const std::function<void()>&)>& run_physical){
    const auto check=[](bool b,const char* why){if(!b)throw std::runtime_error(why);};
    const auto word=[](std::uint32_t p)->std::uint32_t&{return *reinterpret_cast<std::uint32_t*>(p);};
    const auto a=source.ActorAddress(),before=active.GetLast(),count=active.Size(),subscriptions=router.SubscriptionCount();
    struct Restore {
        NativeCombatActionFrame& frame;NativeCombatMainSession& main;NativeCombatEventHandlers& handlers;
        NativeCombatSystemImage& image;NativeScriptEventRouter& router;std::uint32_t state,ready_actor,control_actor;bool started;std::uint8_t mode;
        std::array<std::uint32_t,21> selection;std::vector<std::pair<std::uint32_t,std::uint32_t>> fields;std::vector<std::uint32_t> routes;
        ~Restore(){for(auto t:routes)(void)router.Unsubscribe(t);try{frame.SetVisible(false);main.SetVisible(false);}catch(...){}
            for(auto [p,v]:fields)*reinterpret_cast<std::uint32_t*>(p)=v;image.State()=state;image.Mode()=mode;image.target_selection.words=selection;
            handlers.SetReadyActor(ready_actor);handlers.SetControlActor(control_actor);handlers.SetUiControlStarted(started);}
        void Save(std::uint32_t p){fields.emplace_back(p,*reinterpret_cast<std::uint32_t*>(p));}
    } restore{frame,main,handlers,image,router,image.State(),handlers.ReadyActor(),handlers.ControlActor(),handlers.UiControlStarted(),image.Mode(),image.target_selection.words};
    for(auto offset:{636u,648u,660u,672u,720u,724u,828u,832u,1200u})restore.Save(a+offset);
    for(auto offset:{60u,88u,0x1e4u})restore.Save(window.Address()+offset);
    for(auto offset:{268u,272u})restore.Save(main_window.Address()+offset);
    main.SetVisible(true);frame.SetVisible(true,main_window.Root());
    unsigned selected{},selecting{},completed{};
    restore.routes.push_back(router.Subscribe(router.Key("re_select_command"),3u,[&](std::uint32_t payload){
        check(word(payload)==a+716u && word(payload+12u)==4u,"actual Action command packet contains original actor facet and Attack4");
        bool rejected{};try{controller.Close();}catch(const std::logic_error&){rejected=true;}check(rejected,"Action controller is pinned across synchronous command event");++selected;
    }));
    restore.routes.push_back(router.Subscribe(router.Key("start_select_target"),3u,[&](std::uint32_t){
        bool rejected{};try{commands.Close();}catch(const std::logic_error&){rejected=true;}check(rejected,"command selection remains pinned across nested start-target event");++selecting;
    }));
    const auto press=[&](unsigned key){std::array<std::uint32_t,5> e{};e[4]=key;return active.DispatchKeyDown(active.GetLast(),reinterpret_cast<std::uint32_t>(e.data()));};
    const auto module=GetModuleHandleW(L"CEGUIBase.dll");
    for(unsigned mode=0;mode<3u;++mode){
        run_physical([&]{
            image.State()=3u;image.Mode()=0u;word(a+1200u)=mode==2u?1u:0u;source.SetActionProgress(.75f);source.CanTakeAction(turn);dismiss_tutorial();
            check(window.Word(88u)==a && active.GetLast()==window.Address()+40u,"actual Player ready enables Action controller through global activity");
            if(mode==0u){
                const auto current=window.Word(0x1e4u),next=word(current+8u);
                const auto disabled=detail::Export<bool (__thiscall*)(const void*,bool)>(module,"?isDisabled@Window@CEGUI@@QBE_N_N@Z");
                const auto expected=next && !disabled(reinterpret_cast<void*>(word(next)),false)?next:current;
                (void)press(205u);check(window.Word(0x1e4u)==expected,"actual right key follows original directional graph and disabled check");
                (void)press(5u);
                const auto stunt=window.Selector(NativeCombatSelectorKind::Stunt).Address();
                check(active.GetLast()==stunt+40u && !controller.PendingPayloads() && !controller.ActiveIterators() && window.Word(88u)==a,
                    "real Stunt DLL click opens its populated selector with released cursors");
                (void)press(1u);
                check(active.GetLast()==window.Address()+40u && !controller.PendingPayloads() && !controller.ActiveIterators() && window.Word(88u)==a,
                    "Stunt Escape cancels selection and restores the Action activity without dispatching an attack");
            }
            const auto old=selected;
            if(mode==0u)(void)press(2u); // Original1 shortcut, through DLL mouse-at-center.
            else if(mode==1u)(void)press(28u); // Original Enter on first enabled Attack button.
            else{
                CeguiMouseEvent event{};event.vtable=reinterpret_cast<std::uint32_t>(detail::Export<const void*>(module,"??_7MouseEventArgs@CEGUI@@6B@"));event.window=window.Button(0u);
                detail::NativeString empty(module,"");detail::Export<void (__thiscall*)(void*,const void*,void*,const void*)>(module,"?fireEvent@EventSet@CEGUI@@UAEXABVString@2@AAVEventArgs@2@0@Z")(
                    reinterpret_cast<void*>(event.window),detail::Export<const void*>(module,"?EventClicked@PushButton@CEGUI@@2VString@2@B"),&event,empty.Data());window.RethrowEventError();
            }
            check(selected==old+1u && !window.Word(88u) && active.GetLast()==main.InputAddress(),"keyboard/DLL Attack click sends one real command and closes Action UI");
            if(mode<2u){
                check(image.State()==5u && selection.CurrentActor(),"manual Attack enters actual state5 target controller");
                for(unsigned n=0;n<8u && selection.CurrentActor()!=target;++n)(void)press(205u);
                check(selection.CurrentActor()==target,"actual Main keyboard navigation reaches the production Monster target");
                (void)press(28u);
            }
            check(!controller.ActiveIterators() && !controller.PendingPayloads() && !targets.ActiveIterators(),"nested key/DLL/command callbacks retire transport ownership before queued attack advances");
        });++completed;
    }
    check(selected==3u && selecting==2u && completed==3u,"numeric key and Enter manual selection plus DLL automatic selection execute distinct original routes");
    for(auto h:restore.routes)(void)router.Unsubscribe(h);restore.routes.clear();frame.SetVisible(false);main.SetVisible(false);
    check(active.Size()==count && active.GetLast()==before && router.SubscriptionCount()==subscriptions && !controller.PendingPayloads() && !controller.ActiveIterators(),"three actual input-to-attack flows restore frame/input and callback ownership");
    std::cout<<"Native Action controller: actual global right/numeric1/Enter, DLL click, original Attack4/re_select_command/57C440, manual state5 target/confirmation twice and automatic opposite target once -> real physical/end chains3; Stunt open/Escape, nested pins and zero payload/cursors pass\n";
}
}
