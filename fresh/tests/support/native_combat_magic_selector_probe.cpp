#include "native_combat_magic_selector_probe.h"
#include "pal4/native_combat_action_controller.h"
#include "pal4/native_combat_action_window.h"
#include "pal4/native_event_handler_array.h"
#include "pal4/native_combat_actor_skill_runtime.h"
#include "pal4/native_magic_window_records.h"
#include "pal4/native_combat_target_selection.h"
#include "pal4/native_combat_roster_targets.h"
#include "pal4/native_script_event_router.h"
#include "../../native/ui/cegui_exports_native.h"
#include <algorithm>
#include <array>
#include <iostream>
#include <stdexcept>
namespace pal4::test {
bool StartNativeCombatMagicFromSelector(NativeCombatActionController& controller,NativeCombatActionWindow& action,NativeEventHandlerArray& active,
    NativeCombatActorSkillRuntime& source,const NativeMagicWindowRecords& records,NativeCombatTargetSelection& selection,
    NativeCombatRosterTargets& targets,NativeScriptEventRouter& router,std::uint32_t target,std::uint32_t magic){
    const auto check=[](bool v,const char* m){if(!v)throw std::runtime_error(m);};
    unsigned element=5;std::array<std::vector<std::uint32_t>,5> learned;
    for(unsigned n=0;n<5;++n){learned[n]=source.MasteredMagic(n);if(std::find(learned[n].begin(),learned[n].end(),magic)!=learned[n].end())element=n;}
    if(element==5)return false;
    bool invalid_element{};try{(void)source.MasteredMagic(5u);}catch(const std::invalid_argument&){invalid_element=true;}
    check(invalid_element,"learned collection rejects element outside original five slots");
    const auto module=GetModuleHandleW(L"CEGUIBase.dll");const auto actor=source.ActorAddress();
    auto& layout=action.Selector(NativeCombatSelectorKind::Magic);
    const auto word=[](std::uint32_t p)->std::uint32_t&{return *reinterpret_cast<std::uint32_t*>(p);};
    const auto press=[&](std::uint32_t key){std::array<std::uint32_t,5> e{};e[4]=key;const auto handled=active.DispatchKeyDown(active.GetLast(),reinterpret_cast<std::uint32_t>(e.data()));action.RethrowEventError();return handled;};
    struct Restore{NativeCombatActionWindow& action;std::uint32_t& automatic;std::uint32_t old;~Restore(){automatic=old;}} restore{action,word(actor+1200u),word(actor+1200u)};
    restore.automatic=0u;
    const auto open=[&]{action.Word(88u)=actor;action.SetVisible(actor,true);(void)press(4u);
        check(active.GetLast()==layout.Address()+48u && word(layout.Root()+68u)!=0u && !controller.ActiveIterators(),"actual numeric3 opens learned Magic popup and publishes input facet");};
    open();
    const auto rows=[&]{return detail::Export<unsigned (__thiscall*)(const void*)>(module,"?getRowCount@MultiColumnList@CEGUI@@QBEIXZ")(reinterpret_cast<void*>(layout.List()));};
    const auto select_element=[&]{for(unsigned n=0;n<5u && layout.Word(56u)!=element;++n)(void)press(layout.Word(56u)<element?205u:203u);
        check(layout.Word(56u)==element,"original left/right reaches actual learned element");};
    select_element();
    unsigned expected{};for(auto id:learned[element])if(auto* r=records.FindMagic(id);r && r->kind!=3u && r->kind!=6u)++expected;
    check(rows()==expected && expected,"DLL row count equals original learned IDs after type gates");
    (void)press(208u);(void)press(200u);
    check(press(1u) && active.GetLast()==action.Address()+40u && !word(layout.Root()+68u) && action.Word(88u)==actor,"actual Escape cancels popup and restores Action input without selecting");
    {
        const auto original_mp=source.CurrentStat(9u);
        struct RestoreMp{NativeCombatActorSkillRuntime& source;std::int32_t mp;~RestoreMp(){(void)source.SetCurrentStat(9u,mp);}} restore_mp{source,original_mp};
        (void)source.SetCurrentStat(9u,0);
        open();select_element();
        const auto hovered=reinterpret_cast<std::uint32_t>(detail::Export<void* (__thiscall*)(const void*)>(module,"?getHoverItem@MultiColumnList@CEGUI@@QBEPAVListboxItem@2@XZ")(reinterpret_cast<void*>(layout.List())));
        check(hovered && *reinterpret_cast<const std::uint8_t*>(hovered+421u)!=0u,"zero MP marks original learned Magic cell disabled");
        const auto old_input=active.GetLast();(void)press(28u);
        check(active.GetLast()==old_input && word(layout.Root()+68u) && action.Word(88u)==actor && !controller.PendingPayloads(),"unaffordable Enter keeps selector active without creating command payload");
        (void)press(1u);
    }
    open();select_element();check(rows()==expected,"reopen resets DLL rows before releasing owned cells");
    const auto hover=detail::Export<void* (__thiscall*)(const void*)>(module,"?getHoverItem@MultiColumnList@CEGUI@@QBEPAVListboxItem@2@XZ");
    bool found{};for(unsigned n=0;n<expected;++n){const auto item=reinterpret_cast<std::uint32_t>(hover(reinterpret_cast<void*>(layout.List())));
        if(item && word(item+312u)==magic){check(*reinterpret_cast<const std::uint8_t*>(item+421u)==0u,"selected learned Magic is affordable");found=true;break;}(void)press(208u);}
    check(found,"original DLL hover navigation locates learned Magic5000");
    unsigned commands{};const auto route=router.Subscribe(router.Key("re_select_command"),3u,[&](auto payload){
        check(word(payload)==actor+716u && word(payload+8u)==magic && word(payload+12u)==0u,"541910 sends actual learned Magic ID in original four-word command");
        bool rejected{};try{controller.Close();}catch(const std::logic_error&){rejected=true;}check(rejected,"Magic callback pins Action controller across synchronous command routing");++commands;});
    struct Unsubscribe{NativeScriptEventRouter& r;std::uint32_t t;~Unsubscribe(){(void)r.Unsubscribe(t);}} unsubscribe{router,route};
    (void)press(28u);
    check(commands==1u && !word(layout.Root()+68u) && !action.Word(88u) && !controller.ActiveIterators() && !controller.PendingPayloads(),"Enter validates attached Skill, closes selectors and retires command transport");
    check(selection.CurrentActor()!=0u,"actual Magic selection starts original manual target controller");
    (void)ActorManagerTargetSelectionController::SwitchActiveActor(targets.CombatSystemAddress(),target,targets);
    check(selection.CurrentActor()==target,"real Magic target controller selects actual M01 minion");
    (void)selection.ConfirmTarget();
    std::cout<<"Native Magic selector: actual learned collection, DLL rows="<<expected<<", numeric3/open, element keys/up/down/Escape/reopen, zero-MP Enter rejected, affordable Enter -> original541910/attached Skill/re_select_command/manual target confirm pass\n";
    return true;
}
}
