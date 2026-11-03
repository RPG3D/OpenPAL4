#pragma once
#include "pal4/native_combat_main_action_ready.h"
#include "pal4/native_combat_main_window.h"
#include "pal4/native_combat_main_ui_events.h"
#include "pal4/native_combat_console.h"
#include "pal4/native_combat_player_manager.h"
#include "pal4/native_combat_monster_manager.h"
#include "pal4/native_effect_scene_session.h"
#include "pal4/native_game_timer_queue.h"
#include "pal4/native_ui_text_image_overlay.h"
#include "pal4/native_cegui_ui_session.h"
#include "pal4/ui_text_image_animation.h"
#include "../../native/ui/cegui_exports_native.h"
#include <bit>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
namespace pal4::test {
inline void VerifyNativeCombatMainActionReady(NativeCombatMainActionReady& ready,
    NativeCombatMainWindow& window,NativeCombatConsole& console,NativeCombatPlayerManager& players,
    NativeCombatMonsterManager& monsters,NativeEffectSceneSession& effects,NativeGameTimerQueue& timers,
    NativeCeguiUiSession& ui){
    const auto check=[](bool ok,const char* why){if(!ok)throw std::runtime_error(why);};
    const auto word=[](std::uint32_t a){return *reinterpret_cast<const std::uint32_t*>(a);};
    const auto f=[&](std::uint32_t a){return std::bit_cast<float>(word(a));};
    // Original ReleaseEventByName clears callbacks but keeps nodes until the
    // next nonzero Update. Repeated preparation may start before that frame.
    // Drain those canceled nodes before taking a baseline, retaining the exact
    // 0.4 threshold assertion below and without advancing live float periods.
    timers.Update(std::numeric_limits<float>::min());
    const auto before_events=timers.EventCount(),before_timers=timers.Count(),before_effects=effects.CombatActiveCount();
    const auto p=players.StageActorAt(0),m=monsters.StageActorAt(0);
    const auto visible=detail::Export<bool (__thiscall*)(const void*,bool)>(GetModuleHandleW(L"CEGUIBase.dll"),"?isVisible@Window@CEGUI@@QBE_N_N@Z");
    const auto parent=detail::Export<void* (__thiscall*)(const void*)>(GetModuleHandleW(L"CEGUIBase.dll"),"?getParent@Window@CEGUI@@QBEPAV12@XZ");
    const auto root=reinterpret_cast<void*>(window.Root());
    const auto root_visible=visible(root,true);const auto root_parent=parent(root);
    const auto roundtrip=ui.SceneScreenToUi(ui.UiToSceneScreen({750.f,110.f}));
    check(std::abs(roundtrip[0]-750.f)<0.001f && std::abs(roundtrip[1]-110.f)<0.001f,"ready UI transform inverse");
    for(const auto a:{p,m}){
        std::uint32_t facet=a+716u;ready.Handle(reinterpret_cast<std::uint32_t>(&facet));
    }
    check(ready.OutstandingAllocations()==2u && word(console.Address()+8u)==2u,"two actual owned console portraits");
    const auto head=word(console.Address()+4u);
    for(auto node=word(head);node!=head;node=word(node)){
        const auto portrait=word(node+12u);
        check(word(portrait)==UiTextImageAnimation::kPulseVtable && word(portrait+24u) &&
            f(portrait+28u)==750.f && f(portrait+32u)==50.f && f(portrait+36u)==1.f &&
            f(portrait+40u)==1.f && f(portrait+56u)==0.4f && f(portrait+60u)==0.4f && f(portrait+68u)==5.f && f(portrait+76u)==0.2f,
            "real atlas portrait and shared original pulse constants");
    }
    check(timers.EventCount()==before_events+2u && timers.Count()==before_timers &&
        effects.CombatActiveCount()==before_effects+2u,"two one-shot refreshes and actual jdg effects");
    timers.Update(0.4f);check(timers.EventCount()==before_events+2u,"refresh preserves strict original threshold");
    timers.Update(0.001f);timers.Update(0.001f);
    check(timers.EventCount()==before_events && timers.Count()==before_timers,"refresh fires once then unlinks");
    console.Update(0.1f);console.Update(2.1f);console.Update(0.01f);
    check(!ready.OutstandingAllocations() && !word(console.Address()+8u),"pulse portraits expire through real console queue");
    bool rejected{};try{ready.Handle(0u);}catch(const std::invalid_argument&){rejected=true;}check(rejected,"null ready record rejected");
    std::uint32_t foreign=0x1000u;rejected=false;try{ready.Handle(reinterpret_cast<std::uint32_t>(&foreign));}catch(const std::invalid_argument&){rejected=true;}check(rejected,"foreign ready actor rejected");
    check(!ready.HandleTimer(0u,0x1234u),"unrelated timer retained by caller");
    check(visible(root,true)==root_visible && parent(root)==root_parent,"ready presentation preserves Main visibility/attachment");
    std::cout<<"Native combat action ready: actual Player/Monster portraits=2 jdg=2 one_shot_refresh=2 pulse_cleanup=0 original54D670 pass\n";
}
}
