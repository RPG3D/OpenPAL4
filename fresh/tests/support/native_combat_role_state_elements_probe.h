#pragma once
#include "pal4/native_combat_role_state_elements.h"
#include "pal4/native_cegui_ui_session.h"
#include "pal4/cegui_window_pointer_events.h"
#include "pal4/combat_role_state_window.h"
#include "../../native/ui/cegui_exports_native.h"
#include <array>
#include <bit>
#include <cmath>
#include <iostream>
#include <stdexcept>
namespace pal4::test {
inline void VerifyNativeCombatRoleStateElements(NativeCeguiUiSession& ui){
    const auto check=[](bool b,const char* message){if(!b)throw std::runtime_error(message);};
    const auto reject=[&](auto f){bool rejected{};try{f();}catch(const std::logic_error&){rejected=true;}check(rejected,"Role elements lifecycle/ownership rejection");};
    const auto module=GetModuleHandleW(L"CEGUIBase.dll");const auto root=ui.CombatLayoutRoot(NativeCombatLayout::Role);
    std::array<std::uint32_t,131> image;image.fill(0xa1b2c3d4u);image[9]=root;
    NativeCombatRoleStateElements owner(ui,image);
    check(owner.ControlCount()==61u && owner.SubscriptionCount()==4u && !owner.TemporaryAllocations(),"Role actual61 controls and four adopted DLL connections");
    reject([&]{NativeCombatRoleStateElements duplicate(ui,image);});
    for(unsigned i=0;i<28u;++i)if(i!=9u)check(image[i]==0xa1b2c3d4u,"Role elements preserve frame/subscription prefix");
    check(!image[72],"Role status image zero sentinel");
    for(unsigned i=68u;i<101u;++i)if(i!=72u)check(image[i] && image[i]!=0xa1b2c3d4u,"Role actual portrait/status DLL images");
    const auto width=detail::Export<float (__thiscall*)(const void*)>(module,"?getWidth@Window@CEGUI@@QBEMXZ");
    const auto font=detail::Export<const void* (__thiscall*)(const void*,bool)>(module,"?getFont@Window@CEGUI@@QBEPBVFont@2@_N@Z");
    for(unsigned role=0;role<3u;++role){
        for(auto base:{28u,31u,34u})check(std::isfinite(std::bit_cast<float>(image[base+role])) && std::bit_cast<float>(image[base+role])>0.f,"Role actual bar widths");
        check(width(reinterpret_cast<void*>(image[49u+role]))==std::bit_cast<float>(image[28u+role]),"Role original HP width captures DLL value");
        for(auto base:{37u,40u,43u})check(font(reinterpret_cast<void*>(image[base+role]),false)!=nullptr,"Role Figures5 font resolves in actual DLL");
        check(*reinterpret_cast<const std::uint32_t*>(image[46u+role]+280u)==role,"Role actual portrait user slot");
    }
    const auto fire=detail::Export<void (__thiscall*)(void*,const void*,void*,const void*)>(module,"?fireEvent@EventSet@CEGUI@@UAEXABVString@2@AAVEventArgs@2@0@Z");
    const auto send=[&](std::uint32_t control,bool button){
        CeguiMouseEvent event{};event.vtable=reinterpret_cast<std::uint32_t>(detail::Export<const void*>(module,"??_7MouseEventArgs@CEGUI@@6B@"));event.window=control;
        detail::NativeString empty(module,"");fire(reinterpret_cast<void*>(control),detail::Export<const void*>(module,button?"?EventClicked@PushButton@CEGUI@@2VString@2@B":"?EventMouseClick@Window@CEGUI@@2VString@2@B"),&event,empty.Data());
    };
    send(image[64],true);reject([&]{owner.RethrowError();});owner.RethrowError();
    unsigned calls{};owner.BindHandler([&](std::uint32_t method,std::uint32_t event){
        const auto control=reinterpret_cast<const CeguiMouseEvent*>(event)->window;
        check(method==CombatRoleStateWindow::kButtonCallback || method==CombatRoleStateWindow::kMouseCallback,"Role actual original click entrypoint");
        check(control==image[64] || control==image[65] || control==image[66] || control==image[49],"Role actual DLL payload control");
        reject([&]{owner.Close();});reject([&]{owner.BindHandler({});});++calls;return true;
    });
    for(auto i:{64u,65u,66u}){send(image[i],true);owner.RethrowError();}send(image[49],false);owner.RethrowError();
    check(calls==4u,"Role each original click source invokes once");
    owner.BindHandler([](std::uint32_t,std::uint32_t)->bool{throw std::runtime_error("intentional Role DLL observer failure");});
    send(image[64],true);bool failed{};try{owner.RethrowError();}catch(const std::runtime_error& e){failed=std::string_view(e.what())=="intentional Role DLL observer failure";}
    check(failed,"Role DLL boundary defers actual observer exception");owner.RethrowError();
    owner.Close();owner.Close();check(!owner.ControlCount() && !owner.SubscriptionCount() && !owner.TemporaryAllocations(),"Role disconnect and descriptor cleanup");
    send(image[64],true);owner.RethrowError();
    {NativeCombatRoleStateElements replacement(ui,image);unsigned n{};replacement.BindHandler([&](auto,auto){++n;return true;});send(image[64],true);replacement.RethrowError();check(n==1u,"Role cached root rebind has no stale callbacks");}
    std::cout<<"Native Combat Role elements: shared54F940/61 controls,28 status+4 portrait images,9 fonts/bar widths,3 slots,4 DLL click routes, ownership/reentry/deferred failure/disconnect/rebind pass; role frame actor/timer/events remain unbound\n";
}
}
