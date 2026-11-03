#include "native_combat_property_selector_probe.h"
#include "pal4/native_combat_action_controller.h"
#include "pal4/native_combat_action_window.h"
#include "pal4/native_event_handler_array.h"
#include "pal4/native_combat_actor_skill_runtime.h"
#include "pal4/native_inventory_session.h"
#include "pal4/native_inventory_property_reader.h"
#include "pal4/native_combat_target_selection.h"
#include "pal4/native_combat_roster_targets.h"
#include "pal4/native_script_event_router.h"
#include "pal4/native_cegui_text_conversion.h"
#include "pal4/native_cegui_window_pointer_events.h"
#include "../../native/ui/cegui_exports_native.h"
#include <array>
#include <iostream>
#include <stdexcept>
namespace pal4::test {
void VerifyNativeCombatPropertyPreferred(NativeCombatActionWindow& action,NativeEventHandlerArray& active,
    NativeCombatActorSkillRuntime& source,NativeInventorySession& inventory,std::uint32_t property){
    const auto check=[](bool ok,const char* why){if(!ok)throw std::runtime_error(why);};
    const auto count=inventory.ItemCount(property);check(count>0,"history probe requires actual preferred stock");
    const auto press=[&](std::uint32_t key){std::array<std::uint32_t,5> event{};event[4]=key;
        const auto handled=active.DispatchKeyDown(active.GetLast(),reinterpret_cast<std::uint32_t>(event.data()));
        action.RethrowEventError();return handled;};
    auto& layout=action.Selector(NativeCombatSelectorKind::Property);
    action.Word(88u)=source.ActorAddress();action.SetVisible(source.ActorAddress(),true);(void)press(3u);
    check(active.GetLast()==layout.Address()+48u,"history probe opens actual Property activity");
    const auto hover=detail::Export<void* (__thiscall*)(const void*)>(GetModuleHandleW(L"CEGUIBase.dll"),
        "?getHoverItem@MultiColumnList@CEGUI@@QBEPAVListboxItem@2@XZ");
    const auto item=reinterpret_cast<std::uint32_t>(hover(reinterpret_cast<void*>(layout.List())));
    const auto id=item?*reinterpret_cast<const std::uint32_t*>(item+312u):0u;
    std::cout<<"Native Property owner replacement: first_DLL_ID="<<id<<" expected_preferred="<<property<<std::endl;
    check(id==property,"actual Property preferred first row survives retired battle owner");
    check(press(1u) && active.GetLast()==action.Address()+40u && inventory.ItemCount(property)==count,
          "history probe cancels without item consumption");
}
void StartNativeCombatPropertyFromSelector(NativeCombatActionController& controller,NativeCombatActionWindow& action,NativeEventHandlerArray& active,
    NativeCombatActorSkillRuntime& source,NativeInventorySession& inventory,NativeCombatTargetSelection& selection,NativeCombatRosterTargets& targets,
    NativeScriptEventRouter& router,std::uint32_t target,std::uint32_t property){
    const auto check=[](bool ok,const char* why){if(!ok)throw std::runtime_error(why);};
    const auto word=[](std::uint32_t p)->std::uint32_t&{return *reinterpret_cast<std::uint32_t*>(p);};
    const auto actor=source.ActorAddress(),record=inventory.PropertyAddress(property),count=inventory.ItemCount(property);
    check(record && count,"Property selector fixture requires actual owned stock");
    struct ExtraStock {NativeInventorySession& inventory;std::vector<std::uint32_t> added;
        ~ExtraStock(){for(auto id:added)(void)inventory.RemoveItem(id,1);}
        void Ensure(std::uint32_t id){if(!inventory.ItemCount(id)){(void)inventory.AddItem(id,1);added.push_back(id);}}
    } extra{inventory};
    for(const auto group:{1u,2u}){std::uint32_t id{};
        for(std::size_t n=0;n<inventory.PropertyRecordCount();++n){const auto r=inventory.PropertyRecordAt(n);if(word(r+4u)==group && word(r+8u)<6u){id=word(r);break;}}
        check(id!=0u,"Property category fixture requires actual original definition");extra.Ensure(id);
    }
    check(inventory.PropertyAddress(3012u) && word(inventory.PropertyAddress(3012u)+8u)>=6u,"original3012 is excluded by combat AI gate");extra.Ensure(3012u);
    auto& layout=action.Selector(NativeCombatSelectorKind::Property);const auto module=GetModuleHandleW(L"CEGUIBase.dll");
    const auto press=[&](std::uint32_t key){std::array<std::uint32_t,5> event{};event[4]=key;
        const auto handled=active.DispatchKeyDown(active.GetLast(),reinterpret_cast<std::uint32_t>(event.data()));action.RethrowEventError();return handled;};
    struct Restore{std::uint32_t& automatic;std::uint32_t old;~Restore(){automatic=old;}} restore{word(actor+1200u),word(actor+1200u)};restore.automatic=0u;
    const auto open=[&]{action.Word(88u)=actor;action.SetVisible(actor,true);(void)press(3u);
        check(active.GetLast()==layout.Address()+48u && word(layout.Root()+68u) && !controller.ActiveIterators(),"actual numeric2 opens Property popup and publishes original facet");};
    open();
    const auto rows=detail::Export<unsigned (__thiscall*)(const void*)>(module,"?getRowCount@MultiColumnList@CEGUI@@QBEIXZ");
    NativeInventoryPropertyReader reader(inventory);unsigned expected{};
    for(int n=0,size=reader.Count(0);n<size;++n){const auto item=reader.AtReverse(0,static_cast<unsigned>(n));if(item && word(item->definition+8u)<6u)++expected;}
    std::cout<<"Property selector row operands: group="<<layout.Word(56u)<<" count0="<<reader.Count(0)<<" expected="<<expected
        <<" rows="<<rows(reinterpret_cast<void*>(layout.List()))<<" record4="<<word(record+4u)<<" record8="<<word(record+8u)<<" target12="<<word(record+12u)<<std::endl;
    check(layout.Word(56u)==0u && expected && rows(reinterpret_cast<void*>(layout.List()))==expected,"Property DLL cure rows equal actual usable inventory entries");
    (void)press(208u);(void)press(200u);
    const auto category=[&](std::uint32_t group){unsigned expected_rows{};for(int n=0,size=reader.Count(static_cast<int>(group));n<size;++n){const auto item=reader.AtReverse(static_cast<int>(group),static_cast<unsigned>(n));if(item && word(item->definition+8u)<6u)++expected_rows;}
        check(layout.Word(56u)==group && rows(reinterpret_cast<void*>(layout.List()))==expected_rows && expected_rows,"original Property button/group mapping and DLL category rows");};
    (void)press(205u);category(2u);(void)press(205u);category(1u);(void)press(203u);category(2u);(void)press(203u);category(0u);
    NativeCeguiWindowPointerEvents mouse(module);(void)mouse.HandleMouseAtCenter(layout.Control("BtnAuxiliary"));action.RethrowEventError();category(2u);
    (void)mouse.HandleMouseAtCenter(layout.Control("BtnCure"));action.RethrowEventError();category(0u);
    check(press(1u) && active.GetLast()==action.Address()+40u && !word(layout.Root()+68u) && inventory.ItemCount(property)==count && !controller.PendingPayloads(),
          "Property Escape restores Action input without consuming stock or routing command");
    open();
    // Original cure row keys and hover description, using actual DLL cells.
    const auto hover=detail::Export<void* (__thiscall*)(const void*)>(module,"?getHoverItem@MultiColumnList@CEGUI@@QBEPAVListboxItem@2@XZ");
    bool found{};for(unsigned n=0;n<expected;++n){const auto item=reinterpret_cast<std::uint32_t>(hover(reinterpret_cast<void*>(layout.List())));
        if(item && word(item+312u)==property){found=true;break;}(void)press(208u);}
    check(found,"Property real DLL hover navigation locates owned combat-usable recovery item");
    const auto text=word(record+52u),size=word(record+56u);const auto expected_text=ConvertGameTextToUtf8(text?std::string_view(reinterpret_cast<const char*>(text),size):std::string_view{});
    check(expected_text.has_value(),"Property original description converts");
    // Hover callbacks are emitted by original list movement; refresh through the
    // actual event owner before comparing text (one-row lists need no movement).
    const auto get=detail::Export<const void* (__thiscall*)(const void*)>(module,"?getText@Window@CEGUI@@QBEABVString@2@XZ");
    const auto cstr=detail::Export<const char* (__thiscall*)(const void*)>(module,"?c_str@String@CEGUI@@QBEPBDXZ");
    check(std::string(cstr(get(reinterpret_cast<void*>(layout.Description()))))==*expected_text,"Property DLL description equals actual selected record52");
    unsigned commands{};const auto route=router.Subscribe(router.Key("re_select_command"),3u,[&](auto p){
        check(word(p)==actor+716u && word(p+8u)==property && word(p+12u)==2u,"541A70 routes actual Property ID/kind2 command packet");
        bool rejected{};try{controller.Close();}catch(const std::logic_error&){rejected=true;}check(rejected,"Property callback pins Action controller during command routing");++commands;});
    struct Unsubscribe{NativeScriptEventRouter& r;std::uint32_t t;~Unsubscribe(){(void)r.Unsubscribe(t);}} unsubscribe{router,route};
    (void)press(28u);
    check(commands==1u && !word(layout.Root()+68u) && !action.Word(88u) && !controller.PendingPayloads() && !controller.ActiveIterators(),
          "Property Enter closes selectors and retires synchronous command transport");
    check(selection.CurrentActor()!=0u && inventory.ItemCount(property)==count,"Property manual target selection precedes inventory consumption");
    (void)ActorManagerTargetSelectionController::SwitchActiveActor(targets.CombatSystemAddress(),target,targets);
    check(selection.CurrentActor()==target,"Property target selection reaches actual same-party Player");(void)selection.ConfirmTarget();
    std::cout<<"Native Property selector: actual ID="<<property<<" inventory/DLL cure rows="<<expected<<" numeric2/up/down/three-category keys/DLL buttons/Escape/reopen/3012 excluded/description/Enter -> original541A70/kind2/manual Player target confirm pass\n";
}
}
