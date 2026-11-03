#pragma once
#include "native_cegui_test_click.h"
#include "pal4/native_menu_window.h"
#include "pal4/native_event_handler_array.h"
#include "pal4/native_cegui_ui_session.h"
#include "pal4/native_cegui_window_pointer_events.h"
#include "../../native/ui/cegui_exports_native.h"
#include <array>
#include <iostream>
#include <tuple>
#include <vector>
namespace pal4::test {
inline void VerifyNativeMenuWindow(NativeCeguiUiSession& ui,RwCamera* camera){
    using detail::Export;
    const auto check=[](bool ok,const char* why){if(!ok)throw std::runtime_error(why);};
    const auto ptr=[](std::uint32_t a){return reinterpret_cast<void*>(a);};
    const auto module=GetModuleHandleA("CEGUIBase.dll");
    NativeEventHandlerArray activity([](auto,auto){return 1u;},[](auto,auto,auto)->std::uint8_t{return 0u;});
    (void)activity.Insert(0xabcdu);
    std::vector<std::tuple<std::string,std::uint32_t,std::uint32_t>> calls;
    NativeMenuWindow* current{};bool reopen{};
    NativeMenuWindow menu(ui,activity,[&](std::string_view name,auto a,auto b){
        check(current && !current->Visible() && !current->ButtonCount(),"Menu closes and destroys labels before command");
        calls.emplace_back(name,a,b);
        if(reopen){current->AddButton("reopened",7u,8u,"Reopened",true);current->Show({320,200});}
    });current=&menu;
    auto* system=ptr(ui.SystemAddress());
    const auto render=[&]{RwRGBA black{0,0,0,255};check(RwCameraClear(camera,&black,rwCAMERACLEARIMAGE|rwCAMERACLEARZ) && RwCameraBeginUpdate(camera),"Menu camera");
        check(ui.Render(),"Menu actual DLL render");RwCameraEndUpdate(camera);};
    const auto key=[&](unsigned code){render();(void)Export<bool (__thiscall*)(void*,unsigned)>(module,"?injectKeyDown@System@CEGUI@@QAE_NI@Z")(system,code);
        (void)Export<bool (__thiscall*)(void*,unsigned)>(module,"?injectKeyUp@System@CEGUI@@QAE_NI@Z")(system,code);menu.RethrowEventError();};
    const auto click=[&](std::uint32_t window){render();const auto origin=NativeCeguiWindowPointerEvents(module).AbsolutePosition(window);
        const auto width=*reinterpret_cast<const float*>(window+268u),height=*reinterpret_cast<const float*>(window+272u);
        (void)Export<bool (__thiscall*)(void*,float,float)>(module,"?injectMousePosition@System@CEGUI@@QAE_NMM@Z")(system,origin.x+width/2,origin.y+height/2);
        (void)Export<bool (__thiscall*)(void*,int)>(module,"?injectMouseButtonDown@System@CEGUI@@QAE_NW4MouseButton@2@@Z")(system,0);
        (void)Export<bool (__thiscall*)(void*,int)>(module,"?injectMouseButtonUp@System@CEGUI@@QAE_NW4MouseButton@2@@Z")(system,0);menu.RethrowEventError();};
    for(unsigned cycle=0;cycle<3u;++cycle){
        const auto choices=[&]{menu.AddButton("disabled",0u,1u,"Disabled",false);
            menu.AddButton("use_property",0u,1234u,"Use",true);
            menu.AddButton("discard_property",0u,1234u,"Discard",true);};
        choices();
        menu.Show({400,100});render();check(menu.Visible() && menu.ButtonCount()==3u,"Menu real layout shown");
        const auto n=calls.size();click(menu.Button(0));check(calls.size()==n,"Disabled DLL button cannot dispatch");
        // Disabled controls pass the click to the root, which may dismiss it.
        if(!menu.Visible()){choices();menu.Show({400,100});}
        if(cycle==0u){key(208u);key(28u);check(calls.size()==n+1u && std::get<0>(calls.back())=="discard_property","Menu DLL modal keyboard navigation and Enter");}
        else if(cycle==1u){click(menu.Button(1));check(calls.size()==n+1u && std::get<0>(calls.back())=="use_property" && std::get<2>(calls.back())==1234u,"Menu mouse dispatch retains command payload after cleanup");}
        else {key(1u);check(calls.size()==n && !menu.Visible(),"Menu Esc closes without a command");}
        check(activity.Size()==1u && activity.GetLast()==0xabcdu,"Menu preserves parent activity");
    }
    for(unsigned i=0;i<12u;++i)menu.AddButton("grow",i,42u,std::string(180u,'x'),true);
    menu.Show({200,100});render();click(menu.Button(2));check(std::get<1>(calls.back())==2u,"Menu real DLL String copies survive record growth");
    menu.AddButton("disabled",0u,0u,"Unavailable",false);menu.Show({300,100});key(28u);check(!menu.Visible(),"Menu without navigable choices Enter closes");
    menu.AddButton("reentrant",1u,2u,"Again",true);menu.Show({300,100});reopen=true;click(menu.Button(0));reopen=false;
    check(menu.Visible() && menu.ButtonCount()==1u,"Menu command can reopen same owner after label destruction");
    menu.DispatchCommand("close_popup_menu");check(!menu.Visible() && !menu.ButtonCount(),"Menu registered close command");
    menu.AddButton("last",0u,0u,"Last",true);menu.Show({300,100});
    NativeMenuWindow sibling(ui,activity,[&](std::string_view n,auto a,auto b){calls.emplace_back(n,a,b);});
    check(sibling.FrameAddress()==menu.FrameAddress(),"one actual popup is shared by persistent page owners");
    sibling.AddButton("sibling",91u,92u,"Other page",true);sibling.Show({300,100});
    menu.HideAndCleanup();check(sibling.Visible() && !menu.Visible(),"inactive page hide preserves the active page popup");
    menu.Close();menu.Close();render();ClickNativeCeguiWindow(ui,sibling.Button(0u));sibling.RethrowEventError();
    check(std::get<0>(calls.back())=="sibling" && std::get<1>(calls.back())==91u && !sibling.Visible(),"closing inactive owner preserves live DLL popup and routes only its current command");
    sibling.Close();check(!menu.Visible() && !menu.ButtonCount() && activity.Size()==1u,"last menu owner teardown is idempotent and preserves parent");
    std::cout<<"Native Menu: actual DLL modal keys/mouse/disabled/reopen/growth/cleanup passed"<<std::endl;
}
}
