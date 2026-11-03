#pragma once
#include "pal4/native_combat_main_joint_hint.h"
#include "pal4/native_combat_main_window.h"
#include "pal4/native_combat_system_image.h"
#include "pal4/native_script_event_router.h"
#include "../../native/ui/cegui_exports_native.h"
#include <array>
#include <iostream>
#include <stdexcept>
namespace pal4::test {
inline void VerifyNativeCombatMainJointHint(NativeCombatMainJointHint& hint,NativeCombatMainWindow& main,
    std::uint32_t player,std::uint32_t monster,NativeCombatSystemImage& system,NativeScriptEventRouter& router){
    const auto check=[](bool b,const char* why){if(!b)throw std::runtime_error(why);};
    auto* image=reinterpret_cast<std::uint32_t*>(main.Address());
    const auto module=GetModuleHandleW(L"CEGUIBase.dll");
    const auto visible=detail::Export<bool (__thiscall*)(const void*,bool)>(module,"?isVisible@Window@CEGUI@@QBE_N_N@Z");
    const auto show=detail::Export<void (__thiscall*)(void*,bool)>(module,"?setVisible@Window@CEGUI@@QAEX_N@Z");
    const auto parent=detail::Export<void* (__thiscall*)(const void*)>(module,"?getParent@Window@CEGUI@@QBEPAV12@XZ");
    const auto root=reinterpret_cast<void*>(main.Root());
    const auto root_visible=visible(root,false);const auto root_parent=parent(root);
    struct Restore {
        std::uint32_t* image;NativeCombatSystemImage& system;
        std::array<std::uint32_t,4> words;decltype(show) show_fn;bool visible;
        ~Restore(){image[67]=words[0];image[68]=words[1];system.translator.words[2]=words[2];
            system.translator.words[3]=words[3];show_fn(reinterpret_cast<void*>(image[54]),visible);}
    } restore{image,system,{image[67],image[68],system.translator.words[2],system.translator.words[3]},show,
        visible(reinterpret_cast<void*>(image[54]),true)};
    const auto backend=system.translator.words[0],table=system.translator.words[1];
    const auto old_state=system.State();unsigned calls{};
    struct Route {NativeScriptEventRouter& router;std::uint32_t handle{};~Route(){if(handle)(void)router.Unsubscribe(handle);}} route{router};
    route.handle=router.Subscribe(router.Key("joint_attack_respond"),2u,[&](std::uint32_t p){hint.Handle(p);++calls;});
    // Actual source/target actors; temporary selected values prove both clears.
    for(const auto actor:{player,monster}){
        system.translator.words[2]=player;system.translator.words[3]=monster;
        const std::array payload{actor+716u,actor==player?monster:player};
        (void)router.Dispatch("joint_attack_respond",reinterpret_cast<std::uint32_t>(payload.data()));
        check(visible(reinterpret_cast<void*>(image[54]),true),"actual joint hint control visible");
        check(image[67]==actor && image[68]==payload[1],"joint hint preserves source and target");
        check(!system.translator.words[2] && !system.translator.words[3],"joint hint clears both Combat selections");
    }
    check(calls==2u && system.State()==old_state && system.translator.words[0]==backend && system.translator.words[1]==table,
        "joint hint route preserves scheduler and translator backend");
    const auto before=std::array{image[67],image[68]};
    bool rejected{};try{hint.Handle(0u);}catch(const std::invalid_argument&){rejected=true;}
    check(rejected,"null joint hint payload rejected");
    const std::array foreign{0x1000u,monster};rejected=false;
    try{hint.Handle(reinterpret_cast<std::uint32_t>(foreign.data()));}catch(const std::invalid_argument&){rejected=true;}
    check(rejected && before==std::array{image[67],image[68]},"foreign hint rejected before UI mutation");
    check(visible(root,false)==root_visible && parent(root)==root_parent,
        "joint hint preserves Main local visibility and actual attachment after prior Show/Close");
    std::cout<<"Native combat joint hint: actual Player/Monster route, DLL hint, pending pair, both selection clears and scoped cleanup pass\n";
}
}
