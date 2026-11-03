#pragma once
#include "pal4/native_cegui_ui_session.h"
#include "pal4/native_cegui_window_pointer_events.h"
#include "../../native/ui/cegui_exports_native.h"
#include <iostream>
namespace pal4::test {
// Real DLL hit testing/input; does not move the desktop cursor or take focus.
inline void ClickNativeCeguiWindow(NativeCeguiUiSession& ui,std::uint32_t window){
    using detail::Export;const auto module=GetModuleHandleA("CEGUIBase.dll");
    auto* system=reinterpret_cast<void*>(ui.SystemAddress());
    auto* sheet=Export<void* (__thiscall*)(const void*)>(module,"?getGUISheet@System@CEGUI@@QBEPAVWindow@2@XZ")(system);
    const auto origin=NativeCeguiWindowPointerEvents(module).AbsolutePosition(window);
    const auto width=*reinterpret_cast<const float*>(window+268u),height=*reinterpret_cast<const float*>(window+272u);
    const auto move=Export<bool (__thiscall*)(void*,float,float)>(module,"?injectMousePosition@System@CEGUI@@QAE_NMM@Z");
    const auto mouse_target=Export<void* (__thiscall*)(const void*)>(module,"?getWindowContainingMouse@System@CEGUI@@QBEPAVWindow@2@XZ");
    const auto hit=Export<void* (__thiscall*)(const void*,const CeguiPoint*)>(module,"?getTargetWindow@System@CEGUI@@ABEPAVWindow@2@ABVVector2@2@@Z");
    CeguiPoint point{};bool found{};
    // Original buttons can enable PreciseHitTest with different normal/hover
    // alpha masks. Prefer the centre and re-hit-test after hover callbacks;
    // System's cached mouse window alone can still name the previous hit.
    constexpr unsigned samples[]{8,4,12,2,6,10,14,1,3,5,7,9,11,13,15};
    for(const auto y:samples){if(found)break;for(const auto x:samples){
        point={origin.x+width*x/16.0f,origin.y+height*y/16.0f};
        if(Export<void* (__thiscall*)(const void*,const CeguiPoint*)>(module,
            "?getChildAtPosition@Window@CEGUI@@QBEPAV12@ABVVector2@2@@Z")(sheet,&point)!=reinterpret_cast<void*>(window))continue;
        (void)move(system,point.x,point.y);
        if(hit(system,&point)==reinterpret_cast<void*>(window)){found=true;break;}
    }}
    if(!found)throw std::runtime_error("native CEGUI window has no stable input hit-test point");
    // This helper requests a single click. Fast repeated popup clicks must
    // not become DLL double/triple-click events instead of MouseButtonDown.
    const auto set_multi=Export<void (__thiscall*)(void*,bool)>(module,"?setWantsMultiClickEvents@Window@CEGUI@@QAEX_N@Z");
    const auto previous=Export<bool (__thiscall*)(const void*)>(module,"?wantsMultiClickEvents@Window@CEGUI@@QBE_NXZ")(reinterpret_cast<void*>(window));
    const auto manager=Export<void* (__cdecl*)()>(module,"?getSingletonPtr@WindowManager@CEGUI@@SAPAV12@XZ")();
    const auto native_name=Export<const void* (__thiscall*)(const void*)>(module,"?getName@Window@CEGUI@@QBEABVString@2@XZ")(reinterpret_cast<void*>(window));
    detail::NativeString name(module,Export<const char* (__thiscall*)(const void*)>(module,"?c_str@String@CEGUI@@QBEPBDXZ")(native_name));
    const auto present=Export<bool (__thiscall*)(const void*,const void*)>(module,"?isWindowPresent@WindowManager@CEGUI@@QBE_NABVString@2@@Z");
    const auto find=Export<void* (__thiscall*)(const void*,const void*)>(module,"?getWindow@WindowManager@CEGUI@@QBEPAVWindow@2@ABVString@2@@Z");
    struct RestoreClickMode {
        void* window;decltype(set_multi) setter;bool previous;void* manager;const void* name;decltype(present) exists;decltype(find) lookup;
        ~RestoreClickMode(){
            // A selection callback may retire the clicked layout synchronously.
            if(exists(manager,name) && lookup(manager,name)==window)setter(window,previous);
        }
    } restore{reinterpret_cast<void*>(window),set_multi,previous,manager,name.Data(),present,find};
    set_multi(reinterpret_cast<void*>(window),false);
    const auto trace=[&](const char* phase,bool handled){
        if(!GetEnvironmentVariableW(L"PAL4_UI_CLICK_DIAGNOSTICS",nullptr,0u))return;
        const auto fresh=Export<void* (__thiscall*)(const void*,const CeguiPoint*)>(module,"?getTargetWindow@System@CEGUI@@ABEPAVWindow@2@ABVVector2@2@@Z")(system,&point);
        const auto capture=Export<void* (__cdecl*)()>(module,"?getCaptureWindow@Window@CEGUI@@SAPAV12@XZ")();
        const auto modal=Export<void* (__thiscall*)(const void*)>(module,"?getModalTarget@System@CEGUI@@QBEPAVWindow@2@XZ")(system);
        const auto current_sheet=Export<void* (__thiscall*)(const void*)>(module,"?getGUISheet@System@CEGUI@@QBEPAVWindow@2@XZ")(system);
        std::cerr<<"DLL click "<<Export<const char* (__thiscall*)(const void*)>(module,"?c_str@String@CEGUI@@QBEPBDXZ")(name.Data())
            <<' '<<phase<<" handled="<<handled<<" expected="<<window<<" fresh="<<reinterpret_cast<unsigned>(fresh)
            <<" mouse="<<reinterpret_cast<unsigned>(mouse_target(system))<<" capture="<<reinterpret_cast<unsigned>(capture)
            <<" modal="<<reinterpret_cast<unsigned>(modal)<<" sheet="<<reinterpret_cast<unsigned>(current_sheet)
            <<" original_sheet="<<reinterpret_cast<unsigned>(sheet)<<std::endl;
    };
    trace("before",false);
    const auto down=Export<bool (__thiscall*)(void*,int)>(module,"?injectMouseButtonDown@System@CEGUI@@QAE_NW4MouseButton@2@@Z")(system,0);
    trace("down",down);
    const auto up=Export<bool (__thiscall*)(void*,int)>(module,"?injectMouseButtonUp@System@CEGUI@@QAE_NW4MouseButton@2@@Z")(system,0);
    trace("up",up);
}
}
