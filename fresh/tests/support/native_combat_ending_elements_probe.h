#pragma once
#include "pal4/native_combat_ending_elements.h"
#include "pal4/native_cegui_ui_session.h"
#include "pal4/cegui_window_pointer_events.h"
#include "../../native/ui/cegui_exports_native.h"
#include <array>
#include <bit>
#include <cmath>
#include <iostream>
#include <stdexcept>
namespace pal4::test {
inline void VerifyNativeCombatEndingElements(NativeCeguiUiSession& ui){
    const auto check=[](bool b,const char* m){if(!b)throw std::runtime_error(m);};
    const auto reject=[&](auto f){bool b{};try{f();}catch(const std::logic_error&){b=true;}check(b,"Ending callback/lifecycle boundary rejection");};
    const auto module=GetModuleHandleW(L"CEGUIBase.dll");
    const auto root=ui.CombatLayoutRoot(NativeCombatLayout::Ending);
    std::array<std::uint32_t,180> image;image.fill(0xa1b2c3d4u);image[9]=root;
    auto bad=image;bad[9]=0u;reject([&]{NativeCombatEndingElements wrong(ui,bad);});
    NativeCombatEndingElements owner(ui,image);
    check(owner.ControlCount()==62u && owner.Subscribed() && !owner.TemporaryAllocations(),"Ending real62 controls and adopted DLL mouse connection");
    reject([&]{NativeCombatEndingElements duplicate(ui,image);});
    const auto y=detail::Export<float (__thiscall*)(const void*)>(module,"?getYPosition@Window@CEGUI@@QBEMXZ");
    const auto width=detail::Export<float (__thiscall*)(const void*)>(module,"?getWidth@Window@CEGUI@@QBEMXZ");
    const auto top=detail::Export<bool (__thiscall*)(const void*)>(module,"?isAlwaysOnTop@Window@CEGUI@@QBE_NXZ");
    const auto clipped=detail::Export<bool (__thiscall*)(const void*)>(module,"?isClippedByParent@Window@CEGUI@@QBE_NXZ");
    const auto visible=detail::Export<bool (__thiscall*)(const void*,bool)>(module,"?isVisible@Window@CEGUI@@QBE_N_N@Z");
    constexpr std::array<std::pair<unsigned,const char*>,14> fixed{{
        {80,"PanelScoreLv"},{84,"PanelScoreMark"},{64,"ImgScoreLv"},{88,"PanelPrize"},
        {104,"EditPrizeLeft"},{108,"EditPrizeRight"},{112,"StaticPrizeTitle"},{68,"StaticScoreMark"},
        {72,"PanelMoney"},{76,"PanelProp"},{92,"StaticMoney"},{96,"EditPropLeft"},{100,"EditPropRight"},
        {116,"PanelRole0"}}};
    for(auto [offset,name]:fixed)check(image[offset/4u]==ui.CombatLayoutControl(NativeCombatLayout::Ending,name),"Ending fixed original XML control mapping");
    for(unsigned i=0;i<4u;++i){const auto base=i*19u;
        check(image[29u+base]==ui.CombatLayoutControl(NativeCombatLayout::Ending,"PanelRole"+std::to_string(i)),"Ending actual row panel order");
        check(std::bit_cast<float>(image[44u+base])==y(reinterpret_cast<void*>(image[29u+base])),"Ending row original Y position");
        for(unsigned n=0;n<3u;++n)check(std::bit_cast<float>(image[41u+base+n])==width(reinterpret_cast<void*>(image[36u+base+n])),"Ending cached original HP/MP/rage widths");
        check(std::isfinite(std::bit_cast<float>(image[45u+base])) && std::isfinite(std::bit_cast<float>(image[46u+base])),"Ending hint anchor DLL screen/relative conversion finite");
    }
    check(top(reinterpret_cast<void*>(image[21])) && top(reinterpret_cast<void*>(image[23])) &&
        top(reinterpret_cast<void*>(image[24])) && top(reinterpret_cast<void*>(image[25])) &&
        !clipped(reinterpret_cast<void*>(image[24])) && !clipped(reinterpret_cast<void*>(image[25])),"Ending actual DLL z-order and clipping flags");
    check(!visible(reinterpret_cast<void*>(root),false) && image[0]==0xa1b2c3d4u && image[10]==0xa1b2c3d4u &&
        image[105]==0xa1b2c3d4u && image[179]==0xa1b2c3d4u,"Ending elements keep root hidden and preserve frame/report/task fields");
    const auto fire=detail::Export<void (__thiscall*)(void*,const void*,void*,const void*)>(module,"?fireEvent@EventSet@CEGUI@@UAEXABVString@2@AAVEventArgs@2@0@Z");
    const auto send=[&]{CeguiMouseEvent e{};e.vtable=reinterpret_cast<std::uint32_t>(detail::Export<const void*>(module,"??_7MouseEventArgs@CEGUI@@6B@"));e.window=root;
        detail::NativeString empty(module,"");fire(reinterpret_cast<void*>(root),detail::Export<const void*>(module,"?EventMouseButtonDown@Window@CEGUI@@2VString@2@B"),&e,empty.Data());};
    send();reject([&]{owner.RethrowMouseError();});owner.RethrowMouseError();
    unsigned calls{};owner.BindMouseHandler([&](std::uint32_t e){check(reinterpret_cast<const CeguiMouseEvent*>(e)->window==root,"Ending actual DLL event payload");
        reject([&]{owner.Close();});reject([&]{owner.BindMouseHandler({});});++calls;return true;});
    send();owner.RethrowMouseError();check(calls==1u,"Ending one actual mouse callback");
    owner.BindMouseHandler([](std::uint32_t)->bool{throw std::runtime_error("controlled ending DLL callback");});
    send();bool propagated{};try{owner.RethrowMouseError();}catch(const std::runtime_error& e){propagated=std::string_view(e.what())=="controlled ending DLL callback";}
    check(propagated,"Ending DLL callback catches and surfaces error on host boundary");owner.RethrowMouseError();
    owner.Close();owner.Close();check(!owner.Subscribed() && !owner.TemporaryAllocations() && !owner.ControlCount(),"Ending close retires connection/control ownership");
    send();owner.RethrowMouseError();check(calls==1u,"Ending disconnected root no longer calls retired owner");
    {NativeCombatEndingElements replacement(ui,image);unsigned n{};replacement.BindMouseHandler([&](std::uint32_t){++n;return true;});send();replacement.RethrowMouseError();check(n==1u,"Ending same cached root supports fresh binding without stale duplicate callback");replacement.Close();}
    std::cout<<"Native Combat Ending elements: original543F80/62 controls,4 row metrics/anchors, DLL flags/mouse subscription, exception handoff, callback pins, disconnect/rebind and untouched report/task fields pass; full frame/ending tasks remain pending\n";
}
}
