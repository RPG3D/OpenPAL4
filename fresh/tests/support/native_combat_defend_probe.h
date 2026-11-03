#pragma once
#include "native_combat_action_controller_probe.h"
#include "pal4/native_combat_defend_actions.h"
#include "pal4/native_combat_player_action_memory.h"
#include "pal4/native_combat_task_queue.h"
#include "pal4/native_combat_action_children.h"
#include "pal4/combat_defend_sequence_task.h"
namespace pal4::test {
inline void VerifyNativeCombatDefend(NativeCombatDefendActions& defend,NativeCombatPlayerActionMemory& memory,
    NativeCombatTaskQueue& queue,NativeCombatActionChildren& children,NativeCombatActionController& controller,
    NativeCombatActionWindow& window,NativeCombatActionFrame& frame,NativeCombatMainSession& main,NativeCombatMainWindow& main_window,
    NativeCombatActorSkillRuntime& source,INativeCombatActorTurnServices& turn,NativeCombatEventHandlers& handlers,
    NativeEventHandlerArray& active,NativeScriptEventRouter& router,NativeCombatSystemImage& image,
    const std::function<void()>& dismiss_tutorial,const std::function<void(float)>& advance){
    const auto check=[](bool ok,const char* why){if(!ok)throw std::runtime_error(why);};
    const auto word=[](std::uint32_t p)->std::uint32_t&{return *reinterpret_cast<std::uint32_t*>(p);};
    const auto a=source.ActorAddress(),last=active.GetLast(),activity_count=active.Size(),subscriptions=router.SubscriptionCount();
    check(!queue.Count() && !memory.Count() && !defend.Count() && !children.Count(),"Defend requires idle real owners");
    struct Restore {
        NativeCombatTaskQueue& queue;NativeCombatPlayerActionMemory& memory;NativeCombatActionFrame& frame;NativeCombatMainSession& main;
        NativeCombatEventHandlers& handlers;NativeCombatSystemImage& image;NativeScriptEventRouter& router;
        std::uint32_t state,ready_actor,control_actor;bool started;std::uint8_t mode;
        std::vector<std::pair<std::uint32_t,std::uint32_t>> fields;std::vector<std::uint32_t> routes;
        ~Restore(){for(auto token:routes)(void)router.Unsubscribe(token);try{queue.ResetForStart();memory.Clear();frame.SetVisible(false);main.SetVisible(false);}catch(...){}
            for(auto [p,v]:fields)*reinterpret_cast<std::uint32_t*>(p)=v;
            image.State()=state;image.Mode()=mode;handlers.SetReadyActor(ready_actor);handlers.SetControlActor(control_actor);handlers.SetUiControlStarted(started);}
        void Save(std::uint32_t p){fields.emplace_back(p,*reinterpret_cast<std::uint32_t*>(p));}
    } restore{queue,memory,frame,main,handlers,image,router,image.State(),handlers.ReadyActor(),handlers.ControlActor(),handlers.UiControlStarted(),image.Mode()};
    for(unsigned o=632u;o<676u;o+=4u)restore.Save(a+o);
    for(auto o:{720u,724u,828u,832u})restore.Save(a+o);
    for(auto o:{60u,88u,0x1e4u})restore.Save(window.Address()+o);
    for(auto o:{268u,272u})restore.Save(main_window.Address()+o);
    main.SetVisible(true);frame.SetVisible(true,main_window.Root());
    unsigned commands{},defends{},ends{};bool fail_status{};
    restore.routes.push_back(router.Subscribe(router.Key("re_select_command"),3u,[&](std::uint32_t p){
        check(word(p)==a+716u && word(p+12u)==5u,"actual UI emits Defend5");++commands;
    }));
    restore.routes.push_back(router.Subscribe(router.Key("ae_defende"),3u,[&](std::uint32_t p){
        check(word(p)==a+716u && source.ActionState()==0u,"Defend event precedes original actor196 state1 setter");
        bool rejected{};try{defend.Close();}catch(const std::logic_error&){rejected=true;}check(rejected,"Defend task owner pinned during status event");
        rejected=false;try{source.Close();}catch(const std::logic_error&){rejected=true;}check(rejected,"Defend actual actor registry pinned during event");
        ++defends;if(fail_status)throw std::runtime_error("intentional Defend status observer failure");
    }));
    restore.routes.push_back(router.Subscribe(router.Key("ae_end_action"),3u,[&](std::uint32_t p){if(word(p)==a+716u)++ends;}));
    const auto module=GetModuleHandleW(L"CEGUIBase.dll");
    unsigned total_frames{};
    for(unsigned mode=0;mode<4u;++mode){
        image.Mode()=0u;image.State()=3u;word(a+828u)=1u;word(a+636u)=word(a+648u)=0u;source.SetActionState(0u);
        source.SetActionProgress(.75f);source.CanTakeAction(turn);dismiss_tutorial();
        check(window.Word(88u)==a && active.GetLast()==window.Address()+40u,"real Player ready activates Defend input");
        if(mode!=1u){std::array<std::uint32_t,5> event{};event[4]=6u;
            (void)active.DispatchKeyDown(active.GetLast(),reinterpret_cast<std::uint32_t>(event.data())); // Original numeric5 -> button1.
        }else{
            CeguiMouseEvent event{};event.vtable=reinterpret_cast<std::uint32_t>(detail::Export<const void*>(module,"??_7MouseEventArgs@CEGUI@@6B@"));event.window=window.Button(1u);
            detail::NativeString empty(module,"");detail::Export<void (__thiscall*)(void*,const void*,void*,const void*)>(module,"?fireEvent@EventSet@CEGUI@@UAEXABVString@2@AAVEventArgs@2@0@Z")(
                reinterpret_cast<void*>(event.window),detail::Export<const void*>(module,"?EventClicked@PushButton@CEGUI@@2VString@2@B"),&event,empty.Data());window.RethrowEventError();
        }
        const auto saved=memory.Get(a);
        check(queue.Count()==1u && memory.Count()==1u && defend.Count()==2u && saved && !queue.Contains(saved) &&
            word(saved)==DefendSequenceTask::kVtable && word(saved+44u)==a && !window.Word(88u) && source.ActionState()==0u,
            "Defend owns distinct queued and cold replay tasks; effect waits for GOM rather than UI click");
        fail_status=mode==2u;bool failed{};unsigned frames{};
        try{for(;frames<120u && queue.Count();++frames)advance(1.f/60.f);}catch(const std::runtime_error& e){
            if(!fail_status || std::string_view(e.what()).find("intentional Defend")==std::string_view::npos)throw;failed=true;
        }
        fail_status=false;total_frames+=frames;
        if(mode==2u){
            check(failed && source.ActionState()==0u && queue.Count()==1u,"failed status event preserves failed queued owner without fabricating state1");
            queue.ResetForStart();
        }else check(!queue.Count() && frames<120u && source.ActionState()==1u && ends==(mode==3u?3u:mode+1u),
            "real GOM Defend sets action state1, calls actual EndAction and drains naturally");
        check(defend.Count()==1u && memory.Get(a)==saved && !children.Count(),"queue retirement preserves only independent replay template");
        memory.Clear();check(!defend.Count() && !defend.OwnedAllocations() && !defend.OwnedTexts() && !children.Count(),"Defend parent/child/name/template owners fully retire");
    }
    check(commands==4u && defends==4u && ends==3u && !controller.PendingPayloads() && !controller.ActiveIterators(),"Defend keyboard/DLL/failure routes preserve exact events and transport cleanup");
    for(auto token:restore.routes)(void)router.Unsubscribe(token);restore.routes.clear();frame.SetVisible(false);main.SetVisible(false);
    check(active.GetLast()==last && active.Size()==activity_count && router.SubscriptionCount()==subscriptions,"Defend restores original global activities and subscriptions");
    std::cout<<"Native Defend: numeric5 and actual DLL button1 -> command5 -> real GOM/status/property84/action-state660=1/EndAction; natural drains3, observer failure cleanup/retry1, distinct replay ownership, frames="<<total_frames<<" zero allocations pass\n";
}
}
