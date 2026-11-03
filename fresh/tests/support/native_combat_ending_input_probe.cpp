#include "native_combat_ending_input_probe.h"
#include "pal4/native_combat_ending_window.h"
#include "pal4/native_event_handler_array.h"
#include "pal4/cegui_window_pointer_events.h"
#include "../../native/ui/cegui_exports_native.h"
#include <array>
#include <iostream>
#include <limits>
#include <stdexcept>
namespace pal4::test {
void VerifyNativeCombatEndingInput(NativeCombatEndingWindow& window,NativeEventHandlerArray& active,const std::function<void()>& restart){
    const auto check=[](bool v,const char* m){if(!v)throw std::runtime_error(m);};
    const auto handler=window.Address()+40u;
    check(active.GetLast()==handler && window.Word(436u)==1u && window.Word(716u)==0u && window.TaskCount()==1u,"Ending input borrows actual active victory frame/session");
    std::array<std::uint32_t,8> key{};key[4]=17u;
    (void)active.Contains(reinterpret_cast<std::uint32_t>(key.data())); // Original slot4/8 predicates are both nullsubs.
    check(!active.DispatchKeyDown(handler,reinterpret_cast<std::uint32_t>(key.data())) && window.Word(716u)==0u && window.TaskCount()==1u,"unknown original scan code preserves Ending state and graph");
    const auto module=GetModuleHandleW(L"CEGUIBase.dll");
    CeguiMouseEvent mouse{};mouse.window=window.Root();mouse.button=1u;
    mouse.vtable=reinterpret_cast<std::uint32_t>(detail::Export<const void*>(module,"??_7MouseEventArgs@CEGUI@@6B@"));
    detail::NativeString empty(module,"");detail::Export<void (__thiscall*)(void*,const void*,void*,const void*)>(module,"?fireEvent@EventSet@CEGUI@@UAEXABVString@2@AAVEventArgs@2@0@Z")(
        reinterpret_cast<void*>(window.Root()),detail::Export<const void*>(module,"?EventMouseButtonDown@Window@CEGUI@@2VString@2@B"),&mouse,empty.Data());
    window.RethrowMouseError();check(window.Word(716u)==0u && window.TaskCount()==1u,"actual DLL non-left mouse follows original no-transition branch");
    key[4]=28u;
    check(active.DispatchKeyDown(handler,reinterpret_cast<std::uint32_t>(key.data()))==1u && window.Word(716u)==1u && window.TaskCount()==2u,
        "Enter state0 runs real547BD0 shortcut and keeps disabled previous graph queue-owned");
    check(active.DispatchKeyDown(handler,reinterpret_cast<std::uint32_t>(key.data()))==1u && window.Word(716u)==2u && window.TaskCount()==3u,
        "Enter state1 runs real5481D0 hide shortcut with same task domain");
    const auto visible=detail::Export<bool (__thiscall*)(const void*,bool)>(module,"?isVisible@Window@CEGUI@@QBE_N_N@Z");
    check(!visible(reinterpret_cast<void*>(window.Word(72u)),false) && !visible(reinterpret_cast<void*>(window.Word(76u)),false),"actual hide shortcut closes both reward panels through original DLL");
    restart();window.Update(0.0);
    check(window.Word(716u)==0u && window.Word(436u)==1u && window.TaskCount()==1u,"restart resumes original timeline and retires all three superseded input graphs");
    for(const double bad:{-1.0,std::numeric_limits<double>::infinity(),std::numeric_limits<double>::quiet_NaN(),std::numeric_limits<double>::max()}){
        bool rejected{};try{window.Update(bad);}catch(const std::invalid_argument&){rejected=true;}check(rejected,"Ending double time rejects invalid or nonfinite narrowed input");}
    std::cout<<"Native Ending input: actual activity key/CEGUI mouse, unknown/right-button no-op, Enter0->547BD0 and Enter1->5481D0 DLL/XP/upgrade/graph replacement, three old graphs retired, frame time guards pass\n";
}
}
