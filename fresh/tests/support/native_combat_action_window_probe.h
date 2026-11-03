#pragma once
#include "pal4/native_combat_action_window.h"
#include "pal4/native_cegui_ui_session.h"
#include "pal4/native_event_handler_array.h"
#include "pal4/combat_action_console_window.h"
#include "pal4/cegui_window_pointer_events.h"
#include "../../native/ui/cegui_exports_native.h"
#include <array>
#include <iostream>
#include <stdexcept>
namespace pal4::test {
inline void VerifyNativeCombatActionWindow(NativeCeguiUiSession& ui,const std::filesystem::path& root){
    const auto check=[](bool b,const char* why){if(!b)throw std::runtime_error(why);};
    const auto reject=[&](auto&& f){bool caught{};try{f();}catch(const std::logic_error&){caught=true;}check(caught,"Combat Action owner/dispatch guard");};
    NativeCombatActionWindow* current{};bool fail_activation{};unsigned activated{};
    NativeEventHandlerArray active([&](std::uint32_t p,std::uint32_t slot){
        check(current && p==current->Address()+40u && (slot==12u || slot==16u),"Action activity uses actual input image and lifecycle slot");
        if(fail_activation && slot==12u)throw std::runtime_error("Action activity failure fixture");++activated;return 1u;
    },[](std::uint32_t,std::uint32_t,std::uint32_t)->std::uint8_t{throw std::logic_error("Action controller input remains separate");});
    const auto module=GetModuleHandleW(L"CEGUIBase.dll");
    const auto visible=detail::Export<bool (__thiscall*)(const void*,bool)>(module,"?isVisible@Window@CEGUI@@QBE_N_N@Z");
    const auto disabled=detail::Export<bool (__thiscall*)(const void*,bool)>(module,"?isDisabled@Window@CEGUI@@QBE_N_N@Z");
    const auto id=detail::Export<unsigned (__thiscall*)(const void*)>(module,"?getID@Window@CEGUI@@QBEIXZ");
    const auto fire=detail::Export<void (__thiscall*)(void*,const void*,void*,const void*)>(module,"?fireEvent@EventSet@CEGUI@@UAEXABVString@2@AAVEventArgs@2@0@Z");
    NativeCombatActionWindow owner(ui,root,active);current=&owner;
    check(owner.Word(0u)==CombatActionConsoleWindow::kVtable && owner.Word(40u)==CombatActionConsoleWindow::kNavigationVtable &&
        owner.SubscriptionCount()==18u && !owner.TemporaryAllocations() && !active.Size(),"Action shared constructor owns18 DLL subscriptions and zero temporary descriptors/activity");
    reject([&]{NativeCombatActionWindow duplicate(ui,root,active);});
    constexpr std::array offsets{0x5cu,0xb4u,0x110u};constexpr std::array counts{8u,6u,3u};
    for(unsigned i=0;i<3u;++i){auto& s=owner.Selector(static_cast<NativeCombatSelectorKind>(i));
        check(s.Address()==owner.Address()+offsets[i] && s.SubscriptionCount()==counts[i],"Action selectors occupy original embedded offsets with actual DLL subscriptions");}
    constexpr std::array<std::array<unsigned,2>,19> links{{{0x1b4,0x168},{0x1c0,0x168},{0x194,0x168},{0x1a8,0x1b8},{0x1c8,0x17c},{0x16c,0x1b8},{0x178,0x17c},{0x180,0x1b8},{0x188,0x1b8},{0x1d0,0x17c},{0x1ac,0x190},{0x1c4,0x1a4},{0x170,0x190},{0x174,0x1a4},{0x1a0,0x1cc},{0x19c,0x1a4},{0x184,0x1cc},{0x1d4,0x190},{0x1d8,0x190}}};
    for(auto link:links)check(owner.Word(link[0])==owner.Address()+link[1],"original six-button directional navigation graph");
    check(owner.Word(52u)-owner.Word(48u)==24u && !owner.Word(0x1e4u),"host navigation owns six windows and initial selection is null");
    for(unsigned i=0;i<6u;++i)check(id(reinterpret_cast<void*>(owner.Button(i)))==i && disabled(reinterpret_cast<void*>(owner.Button(i)),false) &&
        owner.Word(0x168u+20u*i)==owner.Button(i),"Action button IDs, disabled state and navigation records use actual DLL controls");
    check(!visible(reinterpret_cast<void*>(owner.Root()),false) && !visible(reinterpret_cast<void*>(owner.Word(0x1e0u)),false),"Action root/panel construction remains hidden");
    reject([&]{owner.SetVisible(0x1234u,true);});check(!active.Size(),"missing real availability rejects show without activity");
    unsigned properties{},magic{};bool has_magic{},fail_property{};
    owner.BindAvailability([&](std::uint32_t c,std::uint32_t i){check(c==0x1234u,"Action availability context retained");reject([&]{owner.Close();});++properties;
        if(fail_property && i==2u)throw std::runtime_error("Action availability failure fixture");return static_cast<std::uint8_t>(i==1u || i==3u);
    },[&]{++magic;return has_magic;});
    owner.SetVisible(0x1234u,true);check(properties==6u && magic==1u && active.GetLast()==owner.Address()+40u &&
        owner.Word(60u)==1u && owner.Word(0x1e4u)==owner.Address()+0x17cu && disabled(reinterpret_cast<void*>(owner.Button(3u)),false),"shared visibility selects first available button and preserves roster magic gate");
    owner.SetVisible(0u,false);has_magic=true;owner.SetVisible(0x1234u,true);
    check(!disabled(reinterpret_cast<void*>(owner.Button(3u)),false),"actual DLL Magic button follows roster availability");owner.SetVisible(0u,false);
    fail_property=true;bool failed{};try{owner.SetVisible(0x1234u,true);}catch(const std::runtime_error&){failed=true;}check(failed && !active.Size() && !visible(reinterpret_cast<void*>(owner.Word(0x1e0u)),false),"partial display failure hides/disables panel and removes borrowed activity");fail_property=false;
    fail_activation=true;failed=false;try{owner.SetVisible(0x1234u,true);}catch(const std::runtime_error&){failed=true;}check(failed && !active.Size(),"published activity is removed after actual Insert activation failure");fail_activation=false;
    unsigned callbacks{};std::uint32_t expected{},target{};
    owner.BindHandler([&](std::uint32_t method,std::uint32_t event){check(method==expected && reinterpret_cast<const CeguiMouseEvent*>(event)->window==target,"Action actual DLL callback ID and payload");reject([&]{owner.Close();});reject([&]{owner.BindHandler({});});++callbacks;return true;});
    const auto send=[&](unsigned i,const char* symbol,std::uint32_t method){expected=method;target=owner.Button(i);CeguiMouseEvent event{};event.vtable=reinterpret_cast<std::uint32_t>(detail::Export<const void*>(module,"??_7MouseEventArgs@CEGUI@@6B@"));event.window=target;
        detail::NativeString empty(module,"");fire(reinterpret_cast<void*>(target),detail::Export<const void*>(module,symbol),&event,empty.Data());};
    for(unsigned i=0;i<6u;++i){send(i,"?EventClicked@PushButton@CEGUI@@2VString@2@B",0x541480u);owner.RethrowEventError();
        send(i,"?EventMouseMove@Window@CEGUI@@2VString@2@B",0x4af760u);owner.RethrowEventError();
        send(i,"?EventMouseButtonUp@Window@CEGUI@@2VString@2@B",0x4af770u);owner.RethrowEventError();}
    check(callbacks==18u,"all eighteen Action DLL callbacks fire exactly once");
    owner.BindHandler({});send(0u,"?EventClicked@PushButton@CEGUI@@2VString@2@B",0x541480u);reject([&]{owner.RethrowEventError();});owner.RethrowEventError();
    owner.Close();check(!owner.Address() && !owner.SubscriptionCount() && !owner.TemporaryAllocations() && !active.Size(),"Action destructor retires embedded selectors, connections and navigation");
    NativeCombatActionWindow replacement(ui,root,active);current=&replacement;check(replacement.SubscriptionCount()==18u,"cached Action root and destroyed embedded selector names support reentry");replacement.Close();active.ReleaseStorage();
    check(!active.OutstandingAllocations() && activated,"actual activity storage cleanup");
    std::cout<<"Native Combat Action: shared full constructor, embedded3 selectors,6 button IDs/19 navigation links,18 actual DLL callbacks, shared display/magic gate, activation failure cleanup, guards/unbound errors and destructor/reentry pass; controller/game selection remain separate\n";
}
}
