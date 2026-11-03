#pragma once
#include "pal4/native_combat_main_ui_events.h"
#include "pal4/native_combat_main_window.h"
#include "pal4/native_combat_player_manager.h"
#include "pal4/native_combat_player_actor_owner.h"
#include "pal4/native_combat_monster_manager.h"
#include "pal4/native_combat_actor_model_owner.h"
#include "pal4/native_combat_actor_skill_runtime.h"
#include "pal4/native_script_event_router.h"
#include "pal4/script_game_event_subscriptions.h"
#include "../../native/ui/cegui_exports_native.h"
#include <array>
#include <cmath>
#include <iostream>
#include <stdexcept>
namespace pal4::test {
inline void VerifyNativeCombatMainUiEvents(NativeCombatMainWindow& window,
    NativeCombatPlayerManager& players,NativeCombatMonsterManager& monsters,
    std::span<NativeCombatActorSkillRuntime* const> skills,NativeScriptEventRouter& router){
    const auto check=[](bool ok,const char* why){if(!ok)throw std::runtime_error(why);};
    // Only the game mode is a fixture; actors, notifications and UI are real.
    std::uint32_t mode=2u;NativeCombatMainUiEvents events(window,players,monsters,skills,[&]{return mode;});
    struct Routes {NativeScriptEventRouter& router;std::vector<std::uint32_t> handles;
        ~Routes(){for(auto h:handles)(void)router.Unsubscribe(h);}} routes{router};
    unsigned motions{};
    for(const auto& b:ScriptGameEventSubscriptions::Bindings())if(events.Supports(b.callback))
        routes.handles.push_back(router.Subscribe(router.Key(b.name),b.mode,[&,callback=b.callback](std::uint32_t payload){
            check(events.Handle(callback,payload),"actual Main event handled");
            if(callback==0x54c9f0u || callback==0x54ca30u)++motions;
        }));
    check(routes.handles.size()==14u,"fourteen supported Main routes");
    const auto word=[](std::uint32_t a){return *reinterpret_cast<const std::uint32_t*>(a);};
    const auto module=GetModuleHandleW(L"CEGUIBase.dll");
    const auto position=detail::Export<const void* (__thiscall*)(const void*,void*,std::uint32_t)>(module,"?getPosition@Window@CEGUI@@QBE?AVVector2@2@W4MetricsMode@2@@Z");
    const auto alpha=detail::Export<float (__thiscall*)(const void*)>(module,"?getAlpha@Window@CEGUI@@QBEMXZ");
    const auto set_alpha=detail::Export<void (__thiscall*)(void*,float)>(module,"?setAlpha@Window@CEGUI@@QAEXM@Z");
    const auto visible=detail::Export<bool (__thiscall*)(const void*,bool)>(module,"?isVisible@Window@CEGUI@@QBE_N_N@Z");
    const auto point=[&](std::uint32_t w){std::array<float,2> p{};position(reinterpret_cast<void*>(w),p.data(),1u);return p;};
    check(!skills.empty() && players.StageCount() && monsters.StageCount(),"real UI event actors available");
    auto& player=*skills.front();auto& monster=monsters.Skills(monsters.StageActorAt(0));
    const auto role_icon=word(window.Address()+224u+4u*players.Actor(player.ActorAddress()).Slot());
    const auto monster_icon=word(window.Address()+236u+4u*monsters.ModelOwner(monster.ActorAddress()).Slot());
    const auto bounds=window.SequenceBounds();
    const auto expect=[&](std::uint32_t icon,float progress){const auto p=point(icon);
        const auto x=static_cast<float>(static_cast<int>((bounds[2]-bounds[0])*progress/64.f+bounds[0]+0.5f));
        check(std::abs(p[0]-x)<0.01f && std::abs(p[1]-bounds[1])<0.01f,"real notification moves matching icon");};
    const auto old_player=player.ActionProgress(),old_monster=monster.ActionProgress();
    struct Restore {NativeCombatActorSkillRuntime& p;NativeCombatActorSkillRuntime& m;float a,b;
        ~Restore(){try{p.SetActionProgress(a);m.SetActionProgress(b);}catch(...){}}} restore{player,monster,old_player,old_monster};
    const auto before_monster=point(monster_icon);player.SetActionProgress(16.f);expect(role_icon,16.f);
    check(point(monster_icon)==before_monster,"Player motion preserves Monster position");
    const auto before_player=point(role_icon);monster.SetActionProgress(48.f);expect(monster_icon,48.f);
    check(point(role_icon)==before_player && motions==2u,"Monster motion preserves Player position and natural events fire once");
    mode=1u;player.SetActionProgress(24.f);expect(role_icon,16.f);
    mode=0u;monster.SetActionProgress(40.f);expect(monster_icon,48.f);mode=2u;
    std::uint32_t facet=player.ActorAddress()+716u;
    check(events.Handle(0x54d980u,reinterpret_cast<std::uint32_t>(&facet)) && word(window.Address()+308u)==role_icon,"select actual Player icon");
    set_alpha(reinterpret_cast<void*>(role_icon),0.25f);
    (void)events.Handle(0x54d9d0u,1u);
    check(!word(window.Address()+308u) && alpha(reinterpret_cast<void*>(role_icon))==1.f,"unselect restores actual alpha and target");
    (void)events.Handle(0x54da00u,1u);
    check(!visible(reinterpret_cast<void*>(word(window.Address()+260u)),true),"actual information panel hidden");
    check(!events.Handle(0x54d670u,1u),"unsupported callback ignores invalid payload");
    const auto reject=[&](std::uint32_t callback,std::uint32_t payload){bool failed{};try{(void)events.Handle(callback,payload);}catch(const std::invalid_argument&){failed=true;}check(failed,"invalid actor event rejected");};
    reject(0x54c9f0u,0u);facet=0x1000u;reject(0x54c9f0u,reinterpret_cast<std::uint32_t>(&facet));
    facet=monster.ActorAddress()+716u;reject(0x54c9f0u,reinterpret_cast<std::uint32_t>(&facet));
    (void)events.Handle(0x54ca70u,1u);
    check(!visible(reinterpret_cast<void*>(role_icon),true) && !visible(reinterpret_cast<void*>(monster_icon),true),"combat over hides actual icons");
    player.SetActionProgress(old_player);monster.SetActionProgress(old_monster);(void)events.Handle(0x54c9e0u,0u);
    std::cout<<"Native combat Main UI events: real14 routes, natural Player/Monster progress, isolated icons, mode gates, selection alpha, panel, invalid payload and cleanup pass\n";
}
}
