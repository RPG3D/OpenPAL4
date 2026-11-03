#pragma once
#include "pal4/native_combat_ending_window.h"
#include "pal4/native_combat_report.h"
#include "pal4/native_combat_hints.h"
#include "pal4/native_cegui_ui_session.h"
#include "pal4/native_event_handler_array.h"
#include "pal4/native_script_event_router.h"
#include "pal4/native_miles_sound_2d_runtime.h"
#include "pal4/cegui_window_pointer_events.h"
#include "../../native/ui/cegui_exports_native.h"
#include <array>
#include <iostream>
#include <set>
#include <stdexcept>
namespace pal4::test {
inline void VerifyNativeCombatEndingWindow(NativeCombatEndingWindow& owner,NativeCeguiUiSession& ui,
    NativeEventHandlerArray& active,NativeScriptEventRouter& router,NativeCombatHints& hints,NativeMilesSound2dRuntime& sounds){
    const auto check=[](bool b,const char* m){if(!b)throw std::runtime_error(m);};
    const auto reject=[&](auto f){bool b{};try{f();}catch(const std::logic_error&){b=true;}check(b,"Ending full-frame lifecycle/service guard");};
    const auto module=GetModuleHandleW(L"CEGUIBase.dll");
    const auto visible=detail::Export<bool (__thiscall*)(const void*,bool)>(module,"?isVisible@Window@CEGUI@@QBE_N_N@Z");
    check(owner.Word(0u)==0x845370u && owner.Word(40u)==0x845358u && owner.Root()==ui.CombatLayoutRoot(NativeCombatLayout::Ending) &&
        owner.ControlCount()==62u && !owner.TaskCount() && owner.Word(436u)==3u && hints.LevelUpCount()==4u,
        "Ending shared5433A0 constructor owns actual frame/report/queue/four named hints/controls");
    check(router.Contains(owner.Word(56u)) && owner.Word(60u)==router.Key("combat_prize"),"Ending original mode2 combat_prize subscription");
    std::set<std::uint32_t> hint_images,roots;
    for(unsigned i=0;i<4u;++i){check(owner.Word(632u+4u*i) && owner.Word(648u+4u*i),"Ending actual original portrait/name images");
        hint_images.insert(owner.Word(684u+4u*i));roots.insert(hints.LevelUpRoot(i));
        check(hints.LevelUpClock(i)=="CombatHintClockCombatEndingLevelUpHint"+std::to_string(i),"Ending original custom prefix is verbatim and clock keys remain distinct");
        reject([&]{(void)hints.ReserveLevelUp(i);});
        check(!owner.Word(700u+4u*i) && owner.Word(472u+40u*i+4u)==0u && owner.Word(472u+40u*i+36u)==0u,
            "Ending sound/learned vector/active flags initialized without private STL sentinel storage");
    }
    check(hint_images.size()==4u && roots.size()==4u && !roots.contains(0u),"Ending four independent real hint owner/root identities");
    for(unsigned i=0;i<5u;++i)check(owner.Word(664u+4u*i)!=0u,"Ending all five original rank images");
    const auto subscriptions=router.SubscriptionCount(),base=active.Size();
    reject([&]{(void)router.Dispatch("combat_prize",0u);});
    unsigned prizes{};owner.BindPrizeHandler([&](std::uint32_t p){check(p==0x12345678u,"Ending synchronous prize payload boundary");
        reject([&]{owner.Close();});reject([&]{owner.SetVisible(false);});reject([&]{owner.BindPrizeHandler({});});++prizes;});
    check(router.Dispatch("combat_prize",0x12345678u)==1u && prizes==1u,"Ending one native router callback");
    owner.BindPrizeHandler([](std::uint32_t){throw std::runtime_error("controlled ending prize observer");});
    bool threw{};try{(void)router.Dispatch("combat_prize",0u);}catch(const std::runtime_error& e){threw=std::string_view(e.what())=="controlled ending prize observer";}
    check(threw && router.Contains(owner.Word(56u)),"Ending throwing prize observer leaves subscription reusable");owner.BindPrizeHandler({});
    const auto sample=sounds.PlayNamedSound("WA22.wav",0);check(sample!=0u,"Ending actual stored sound sample");
    *reinterpret_cast<std::uint32_t*>(owner.Address()+700u)=sample;
    owner.SetVisible(true);
    check(visible(reinterpret_cast<void*>(owner.Root()),false) && active.Size()==base+1u && owner.OwnsActivity(owner.Address()+40u) &&
        !sounds.IsPlaying(sample) && owner.Word(700u)==sample && owner.Word(436u)==3u,"Ending original visibility activates handler, stops sample but preserves handle, resets result");
    for(unsigned i=0;i<4u;++i)check(!visible(reinterpret_cast<void*>(owner.Word(116u+76u*i)),false),"Ending visibility hides all four row panels");
    std::array<std::uint32_t,8> key{};key[4]=28u;
    reject([&]{(void)active.DispatchKeyDown(owner.Address()+40u,reinterpret_cast<std::uint32_t>(key.data()));});
    unsigned keys{},mouse{};owner.BindInputHandler([&](std::uint32_t e,bool m){reject([&]{owner.Close();});reject([&]{owner.BindInputHandler({});});
        if(m){check(reinterpret_cast<const CeguiMouseEvent*>(e)->window==owner.Root(),"Ending actual mouse payload reaches frame owner");++mouse;}
        else{check(e==reinterpret_cast<std::uint32_t>(key.data()),"Ending actual activity keyboard payload reaches frame owner");++keys;}return true;});
    check(active.DispatchKeyDown(owner.Address()+40u,reinterpret_cast<std::uint32_t>(key.data()))==1u && keys==1u,"Ending keyboard routes through actual activity owner");
    CeguiMouseEvent e{};e.window=owner.Root();e.vtable=reinterpret_cast<std::uint32_t>(detail::Export<const void*>(module,"??_7MouseEventArgs@CEGUI@@6B@"));
    detail::NativeString empty(module,"");detail::Export<void (__thiscall*)(void*,const void*,void*,const void*)>(module,"?fireEvent@EventSet@CEGUI@@UAEXABVString@2@AAVEventArgs@2@0@Z")(
        reinterpret_cast<void*>(owner.Root()),detail::Export<const void*>(module,"?EventMouseButtonDown@Window@CEGUI@@2VString@2@B"),&e,empty.Data());
    owner.RethrowMouseError();check(mouse==1u,"Ending DLL mouse passes through frame callback pin");
    {auto lease=owner.BorrowPresentation();owner.WritePresentationWord(436u,1u);}
    check(owner.Report().Word(16u)==1u,"Ending controlled presented result shares native report ownership");
    owner.SetVisible(false);check(active.Size()==base && !visible(reinterpret_cast<void*>(owner.Root()),false) && !owner.TaskCount() &&
        owner.Word(436u)==3u && owner.Report().Word(16u)==3u,"Ending hide removes actual activity, resets real task queue flags and keeps embedded report/result owner consistent");
    const auto token=owner.Word(56u);owner.Close();owner.Close();
    check(!owner.Address() && !owner.ControlCount() && !owner.TaskCount() && !router.Contains(token) && router.SubscriptionCount()+1u==subscriptions &&
        active.Size()==base && hints.LevelUpCount()==0u,"Ending frame/report/queue/prize/mouse/four named hints retire before shared owners");
    for(unsigned i=0;i<4u;++i)check(!hints.LevelUpRoot(i) && hints.LevelUpClock(i).empty(),"Ending named roots and clock owners released");
    std::cout<<"Native Combat Ending frame: shared5433A0/report/queue/4 named hints/5 rank images, mode2 prize callback, original543E90 visibility, actual sample stop/retained handle, activity keyboard/DLL mouse pins and complete scoped retirement pass; prize rendering/tasks remain pending\n";
}
}
