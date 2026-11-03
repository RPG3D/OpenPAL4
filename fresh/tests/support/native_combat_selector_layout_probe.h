#pragma once
#include "pal4/native_combat_selector_layout.h"
#include "pal4/native_cegui_ui_session.h"
#include "pal4/combat_magic_select_window.h"
#include "pal4/combat_property_select_window.h"
#include "pal4/combat_stunt_select_window.h"
#include "pal4/cegui_window_pointer_events.h"
#include "../../native/ui/cegui_exports_native.h"
#include <algorithm>
#include <array>
#include <iostream>
#include <stdexcept>
#include <vector>
namespace pal4::test {
inline void VerifyNativeCombatSelectorLayouts(NativeCeguiUiSession& ui,const std::filesystem::path& root){
    const auto check=[](bool ok,const char* why){if(!ok)throw std::runtime_error(why);};
    const auto reject=[&](auto&& f){bool failed{};try{f();}catch(const std::logic_error&){failed=true;}check(failed,"Combat selector ownership/callback guard rejects");};
    const auto module=GetModuleHandleW(L"CEGUIBase.dll");
    const auto manager=detail::Export<void* (__cdecl*)()>(module,"?getSingletonPtr@WindowManager@CEGUI@@SAPAV12@XZ")();
    const auto present=detail::Export<bool (__thiscall*)(const void*,const void*)>(module,"?isWindowPresent@WindowManager@CEGUI@@QBE_NABVString@2@@Z");
    const auto fire=detail::Export<void (__thiscall*)(void*,const void*,void*,const void*)>(module,"?fireEvent@EventSet@CEGUI@@UAEXABVString@2@AAVEventArgs@2@0@Z");
    const auto visible=detail::Export<bool (__thiscall*)(const void*,bool)>(module,"?isVisible@Window@CEGUI@@QBE_N_N@Z");
    const auto sort=detail::Export<unsigned (__thiscall*)(const void*)>(module,"?getSortColumn@MultiColumnList@CEGUI@@QBEIXZ");
    const auto direction=detail::Export<unsigned (__thiscall*)(const void*)>(module,"?getSortDirection@MultiColumnList@CEGUI@@QBE?AW4SortDirection@ListHeaderSegment@2@XZ");
    const auto get_id=detail::Export<unsigned (__thiscall*)(const void*)>(module,"?getID@Window@CEGUI@@QBEIXZ");
    const auto disabled=detail::Export<bool (__thiscall*)(const void*,bool)>(module,"?isDisabled@Window@CEGUI@@QBE_N_N@Z");
    constexpr std::array kinds{NativeCombatSelectorKind::Magic,NativeCombatSelectorKind::Property,NativeCombatSelectorKind::Stunt};
    constexpr std::array sizes{22u,23u,16u},counts{8u,6u,3u};
    constexpr std::array vtables{CombatMagicSelectWindow::kVtable,CombatPropertySelectWindow::kVtable,CombatStuntSelectWindow::kVtable};
    constexpr std::array names{"MagicSelectWindowRoot","PropertySelectWindowRoot","StuntSelectWindowRoot"};
    std::array<std::array<std::uint32_t,25>,3> storage{};
    std::array<std::unique_ptr<NativeCombatSelectorLayout>,3> owners;
    for(unsigned i=0;i<3u;++i){
        storage[i][0]=0x12345678u;storage[i][sizes[i]+1u]=0x87654321u;
        owners[i]=std::make_unique<NativeCombatSelectorLayout>(kinds[i],ui,root,std::span(storage[i].data()+1u,sizes[i]));
    }
    unsigned callbacks{};
    for(unsigned i=0;i<3u;++i){auto& owner=*owners[i];
        check(owner.Address()==reinterpret_cast<std::uint32_t>(storage[i].data()+1u) && owner.Word(0u)==vtables[i] &&
            owner.Root() && owner.List() && owner.Description() && owner.SubscriptionCount()==counts[i] && !owner.TemporaryAllocations(),
            "actual shared selector construction publishes borrowed image, DLL controls and original subscriptions");
        check(!visible(reinterpret_cast<void*>(owner.Root()),false) && !owner.HasHandler(),"selector construction remains hidden with game services explicitly unbound");
        check(sort(reinterpret_cast<void*>(owner.List()))==0u && direction(reinterpret_cast<void*>(owner.List()))==0u,"actual selector list sort is initialized");
        for(const auto* symbol:{"?isUserColumnSizingEnabled@MultiColumnList@CEGUI@@QBE_NXZ",
            "?isUserColumnDraggingEnabled@MultiColumnList@CEGUI@@QBE_NXZ","?isUserSortControlEnabled@MultiColumnList@CEGUI@@QBE_NXZ"})
            check(!detail::Export<bool (__thiscall*)(const void*)>(module,symbol)(reinterpret_cast<void*>(owner.List())),"actual selector list locks manual header controls");
        reject([&]{NativeCombatSelectorLayout duplicate(kinds[i],ui,root);});
        check(owner.SubscriptionCount()==counts[i],"duplicate constructor failure preserves existing selector subscriptions");
        enum class Event{Mouse,Hover,Button};
        struct Route{std::uint32_t control,callback;Event event;};std::vector<Route> routes;
        if(i==0u){
            routes={{owner.Root(),0x552380u,Event::Mouse},{owner.List(),0x5530d0u,Event::Hover},{owner.List(),0x552ff0u,Event::Mouse}};
            unsigned id{};for(const auto* name:{"BtnWater","BtnFire","BtnThunder","BtnAir","BtnEarth"}){
                const auto button=owner.Control(name);check(get_id(reinterpret_cast<void*>(button))==id++,"Magic element buttons retain original IDs");
                routes.push_back({button,0x552fd0u,Event::Button});}
            check(owner.Word(56u)==5u,"Magic constructor keeps original initial element sentinel");
        }else if(i==1u){
            routes={{owner.Root(),0x552380u,Event::Mouse},{owner.List(),0x553e90u,Event::Hover},{owner.List(),0x553e30u,Event::Mouse}};
            unsigned id{};for(const auto* name:{"BtnCure","BtnAuxiliary","BtnAttackable"}){
                const auto button=owner.Control(name);check(get_id(reinterpret_cast<void*>(button))==id++ && disabled(reinterpret_cast<void*>(button),false),"Property category IDs and initial disabled state are real DLL properties");
                routes.push_back({button,0x553e10u,Event::Button});}
            check(!owner.Word(80u) && !owner.Word(84u) && !owner.Word(88u),"Property scratch vector starts empty");
        }else routes={{owner.Root(),0x555010u,Event::Mouse},{owner.List(),0x554d60u,Event::Hover},{owner.List(),0x554d00u,Event::Mouse}};
        const auto send=[&](const Route& route){
            const char* symbol=route.event==Event::Mouse?"?EventMouseClick@Window@CEGUI@@2VString@2@B":
                route.event==Event::Hover?"?EventHoverRowChanged@MultiColumnList@CEGUI@@2VString@2@B":"?EventClicked@PushButton@CEGUI@@2VString@2@B";
            CeguiMouseEvent event{};event.vtable=reinterpret_cast<std::uint32_t>(detail::Export<const void*>(module,"??_7MouseEventArgs@CEGUI@@6B@"));event.window=route.control;
            detail::NativeString empty(module,"");fire(reinterpret_cast<void*>(route.control),detail::Export<const void*>(module,symbol),&event,empty.Data());
        };
        std::uint32_t expected{};
        owner.BindHandler([&](std::uint32_t callback,std::uint32_t event){
            check(owner.InCallback() && callback==expected && event,"original selector callback ID reaches typed native transport once");
            reject([&]{owner.Close();});reject([&]{owner.BindHandler({});});++callbacks;return true;
        });
        for(const auto& route:routes){expected=route.callback;const auto before=callbacks;send(route);owner.RethrowEventError();check(callbacks==before+1u,"actual DLL selector callback fires exactly once");}
        owner.BindHandler([](std::uint32_t,std::uint32_t)->bool{throw std::runtime_error("selector transport failure fixture");});
        send(routes.front());bool failed{};try{owner.RethrowEventError();}catch(const std::runtime_error&){failed=true;}
        check(failed && !owner.InCallback(),"selector callback exception is captured and releases dispatch pin");owner.RethrowEventError();
        owner.BindHandler({});send(routes.front());reject([&]{owner.RethrowEventError();});
        owner.Close();check(!owner.Address() && !owner.Root() && !owner.SubscriptionCount() && !owner.TemporaryAllocations(),"selector closes DLL connections/layout and temporary allocations");
        check(storage[i][0]==0x12345678u && storage[i][sizes[i]+1u]==0x87654321u &&
            std::all_of(storage[i].begin()+1,storage[i].begin()+1+sizes[i],[](auto v){return v==0u;}),"borrowed selector image clears within exact embedded bounds");
        detail::NativeString name(module,names[i]);check(!present(manager,name.Data()),"actual selector DLL root is destroyed on Close");
        NativeCombatSelectorLayout replacement(kinds[i],ui,root);check(replacement.SubscriptionCount()==counts[i],"selector root names and callback claims can be reused after retirement");replacement.Close();
    }
    check(callbacks==17u,"all original selector constructor subscriptions verified through actual DLL");
    std::array<std::uint32_t,21> short_image{};
    reject([&]{NativeCombatSelectorLayout bad(NativeCombatSelectorKind::Magic,ui,root,short_image);});
    std::cout<<"Native Combat selectors: shared Magic/Property/Stunt constructors, original3 DLL layouts,17 callbacks, list/button state, embedded bounds, hidden staging, guards/errors, disconnect/destruction and reuse pass; game population/selection handlers remain separate\n";
}
}
