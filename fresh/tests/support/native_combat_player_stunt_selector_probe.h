#pragma once
#include "pal4/native_combat_action_controller.h"
#include "pal4/native_combat_action_window.h"
#include "pal4/native_combat_selector_layout.h"
#include "pal4/native_combat_actor_skill_runtime.h"
#include "pal4/native_combat_command_actions.h"
#include "pal4/native_combat_event_handlers.h"
#include "pal4/native_combat_target_selection.h"
#include "pal4/native_combat_roster_targets.h"
#include "pal4/native_combat_task_queue.h"
#include "pal4/native_combat_player_action_memory.h"
#include "pal4/native_combat_skill_domain.h"
#include "pal4/native_event_handler_array.h"
#include "pal4/native_script_event_router.h"
#include "pal4/native_magic_window_records.h"
#include "pal4/native_stunt_learning_catalog.h"
#include "../../native/ui/cegui_exports_native.h"
#include <array>
#include <algorithm>
#include <iostream>
#include <stdexcept>
namespace pal4::test {
// Explicit disposable cave-memory fixture: original level/learning algorithms
// grant actual database skills. Never enabled on the normal NewGame route.
inline void VerifyNativeCombatPlayerStuntSelector(NativeCombatActionController& controller,
    NativeCombatActionWindow& action,NativeEventHandlerArray& active,
    NativeCombatActorSkillRuntime& source,const NativeMagicWindowRecords& records,const NativeGameDatabase& db,
    NativeCombatCommandActions& commands,NativeCombatEventHandlers& handlers,NativeCombatTargetSelection& selection,
    NativeCombatRosterTargets& targets,NativeScriptEventRouter& router,NativeCombatTaskQueue& queue,
    NativeCombatPlayerActionMemory& memory,NativeCombatSkillDomain& domain,std::uint32_t target,
    const std::function<void()>& frame){
    const auto check=[](bool v,const char* why){if(!v)throw std::runtime_error(why);};
    const auto word=[](std::uint32_t p)->std::uint32_t&{return *reinterpret_cast<std::uint32_t*>(p);};
    const auto actor=source.ActorAddress(),activity=active.GetLast();
    auto& layout=action.Selector(NativeCombatSelectorKind::Stunt);
    const auto module=GetModuleHandleW(L"CEGUIBase.dll");
    const auto rows=[&]{return detail::Export<unsigned (__thiscall*)(const void*)>(module,"?getRowCount@MultiColumnList@CEGUI@@QBEIXZ")(reinterpret_cast<void*>(layout.List()));};
    const auto hover=[&]{return reinterpret_cast<std::uint32_t>(detail::Export<void* (__thiscall*)(const void*)>(module,"?getHoverItem@MultiColumnList@CEGUI@@QBEPAVListboxItem@2@XZ")(reinterpret_cast<void*>(layout.List())));};
    const auto press=[&](std::uint32_t key){std::array<std::uint32_t,5> e{};e[4]=key;const auto result=active.DispatchKeyDown(active.GetLast(),reinterpret_cast<std::uint32_t>(e.data()));action.RethrowEventError();return result;};
    const auto open=[&]{action.Word(88u)=actor;action.SetVisible(actor,true);(void)press(5u);
        check(active.GetLast()==layout.Address()+40u && word(layout.Root()+68u) && !controller.ActiveIterators(),"Stunt shortcut opens actual DLL root/input facet with no cursor leaks");};
    check(!queue.Count() && !memory.Count() && domain.Kind()==NativeCombatSkillKind::stunt,"Player Stunt selector fixture requires idle production owners");
    const auto level=source.CurrentStat(4u),sp=source.CurrentStat(8u);
    const auto automatic=word(actor+1200u);
    struct Restore{NativeCombatActorSkillRuntime& source;std::int32_t level,sp;std::uint32_t& automatic;std::uint32_t old;
        ~Restore(){automatic=old;(void)source.SetCurrentStatViaActor(4u,level);(void)source.SetCurrentStat(8u,sp);}} restore{source,level,sp,word(actor+1200u),automatic};
    restore.automatic=0u;
    const auto expected_rows=[&]{unsigned count{};for(auto id:source.LearnedPlayerStunts()){const auto r=records.StuntAddress(id);if(r && word(r+4u)!=3u && word(r+4u)!=6u)++count;}return count;};
    open();const auto initial_rows=rows();check(initial_rows==expected_rows(),"Stunt rows reflect actual learned collection before diagnostic grant");
    if(!initial_rows){(void)press(28u);check(active.GetLast()==layout.Address()+40u && !queue.Count() && !memory.Count(),"empty Stunt Enter keeps actual menu open and publishes no task");}
    (void)press(1u);check(active.GetLast()==action.Address()+40u && !word(layout.Root()+68u),"Stunt Escape restores Action input");
    NativeStuntLearningCatalog learning(db);
    (void)source.SetCurrentStatViaActor(4u,99);const auto added=source.LearnPlayerStunts(learning);
    (void)source.SetCurrentStat(8u,100);
    std::uint32_t selected{},record{};
    for(auto id:source.LearnedPlayerStunts()){
        const auto r=records.StuntAddress(id);
        if(r && word(r+4u)!=3u && word(r+4u)!=6u && word(r+116u)>0u && source.CanUsePlayerStunt(r,records)){selected=id;record=r;break;}
    }
    check(selected && record,"original learning algorithm provides an affordable nonzero-cost Player Stunt from real database");
    bool rebound{};try{commands.BindStunt(records,[&](auto)->NativeCombatSkillDomain&{return domain;});}catch(const std::logic_error&){rebound=true;}
    check(rebound,"Stunt command rejects replacement of borrowed execution services");
    const std::array<std::uint32_t,4> missing{actor+716u,target,0xffffffffu,1u};
    check(!commands.Handle(handlers.ExtendedAddress(),reinterpret_cast<std::uint32_t>(missing.data())) && !queue.Count() && !memory.Count(),"missing Stunt lookup returns before queue/replay publication");
    const auto cost=static_cast<std::int32_t>(word(record+116u));
    bool foreign{};try{(void)source.CanUsePlayerStunt(0x1234u,records);}catch(const std::invalid_argument&){foreign=true;}
    check(foreign,"Player Stunt condition rejects non-catalog pointers");
    const auto find=[&]{for(unsigned n=0;n<rows();++n){const auto cell=hover();if(cell && word(cell+312u)==selected)return cell;(void)press(208u);}return 0u;};
    (void)source.SetCurrentStat(8u,0);check(!source.CanUsePlayerStunt(record,records),"original Player predicate rejects insufficient SP");
    open();check(rows()==expected_rows(),"DLL Stunt rows match learned IDs after original type filtering");
    const auto disabled=find();check(disabled && *reinterpret_cast<const std::uint8_t*>(disabled+421u),"actual zero-SP Stunt row is disabled");
    (void)press(28u);check(active.GetLast()==layout.Address()+40u && !queue.Count() && !memory.Count() && !controller.PendingPayloads(),"disabled Stunt Enter does not dispatch commands");
    (void)press(200u);(void)press(208u);(void)press(1u);
    (void)source.SetCurrentStat(8u,100);const auto before_sp=source.CurrentStat(8u);
    open();const auto enabled=find();check(enabled && !*reinterpret_cast<const std::uint8_t*>(enabled+421u),"reopening refreshes affordable Stunt flags");
    unsigned selected_events{},ended{};
    const auto command_route=router.Subscribe(router.Key("re_select_command"),3u,[&](auto event){
        check(word(event)==actor+716u && word(event+8u)==selected && word(event+12u)==1u,"5419C0 publishes actual Stunt ID and command kind1");
        bool pinned{};try{controller.Close();}catch(const std::logic_error&){pinned=true;}check(pinned,"Stunt callback keeps controller alive during synchronous command dispatch");++selected_events;
    });
    const auto end_route=router.Subscribe(router.Key("ae_end_action"),3u,[&](auto event){if(word(event)==actor+716u)++ended;});
    struct Routes{NativeScriptEventRouter& r;std::uint32_t first,second;~Routes(){(void)r.Unsubscribe(first);(void)r.Unsubscribe(second);}} routes{router,command_route,end_route};
    const auto casts=domain.Casts();
    (void)press(28u);
    check(selected_events==1u && !word(layout.Root()+68u) && !action.Word(88u) && selection.CurrentActor() && !controller.PendingPayloads() && !controller.ActiveIterators(),"Stunt Enter resolves attached Skill, closes popup and starts actual target selection");
    (void)ActorManagerTargetSelectionController::SwitchActiveActor(targets.CombatSystemAddress(),target,targets);
    check(selection.CurrentActor()==target,"actual Player Stunt target controller reaches production enemy");(void)selection.ConfirmTarget();
    const auto replay=memory.Get(actor);check(replay && domain.Owns(replay) && !queue.Contains(replay) && queue.Count(),"Stunt57C4B0 creates distinct queued action and cold replay template");
    unsigned frames{};while(queue.Count() && frames<1200u){frame();++frames;}
    check(!queue.Count() && ended && domain.Casts()>casts && source.CurrentStat(8u)==before_sp-cost,"actual Player Stunt executes original animation/AMF/cost and EndAction");
    memory.Clear();check(!domain.Count() && !domain.OwnedAllocations() && !domain.OwnedTexts() && !domain.Subscriptions() && !controller.ActiveIterators(),"Player Stunt action/replay/UI ownership drains");
    action.SetVisible(actor,activity==action.Address()+40u);check(active.GetLast()==activity,"Player Stunt fixture restores activity stack");
    std::cout<<"Native Player Stunt selector: diagnostic_fixtures=1 original level99/learning grant="<<added.size()<<" initial_rows="<<initial_rows<<" learned="<<source.LearnedPlayerStunts().size()<<" Stunt="<<selected<<" frames="<<frames<<" SP="<<before_sp<<"->"<<source.CurrentStat(8u)<<" EndAction="<<ended<<" empty/disabled/reopen/selection/target/command/cold_replay ownership pass"<<std::endl;
}
}
