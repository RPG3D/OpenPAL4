#pragma once
#include "pal4/native_combat_mouse_input.h"
#include "pal4/native_combat_system_image.h"
#include "pal4/game_render_system.h"
#include "pal4/window_message_processor.h"
#include <array>
#include <cstring>
#include <iostream>
#include <stdexcept>
namespace pal4::test {
inline unsigned previous_mouse_calls{};
inline std::uint32_t previous_mouse_argument{};
inline std::uint32_t __cdecl PreviousMouseSlotProbe(std::uint32_t argument){
    if(argument!=previous_mouse_argument)throw std::logic_error("previous native mouse callback payload");
    ++previous_mouse_calls;return 23u;
}
inline void VerifyNativeCombatMouseSlotOwnership(GameRenderState& game,WindowMessageConfig& config,
    NativeCombatSystemImage& image,NativeCombatMouseTargets& mouse,NativeCombatPlayerManager& players){
    const auto check=[](bool ok,const char* m){if(!ok)throw std::runtime_error(m);};
    const auto reject=[&](auto&& f){bool rejected{};try{f();}catch(const std::logic_error&){rejected=true;}check(rejected,"mouse slot ownership guard");};
    const auto slot=game.Read(84u);const auto mode=image.Mode();auto& saved=*reinterpret_cast<std::uint32_t*>(reinterpret_cast<std::uint32_t>(&image)+428u);
    const auto old_saved=saved;
    struct Restore{GameRenderState& game;NativeCombatSystemImage& image;std::uint32_t& saved;std::uint32_t slot,value;std::uint8_t mode;
        ~Restore(){game.WriteShared(84u,slot);saved=value;image.Mode()=mode;}} restore{game,image,saved,slot,old_saved,static_cast<std::uint8_t>(mode)};
    const auto previous=reinterpret_cast<std::uint32_t>(&PreviousMouseSlotProbe);
    previous_mouse_calls=0u;previous_mouse_argument=reinterpret_cast<std::uint32_t>(config.data()+12u);
    game.WriteShared(84u,previous);image.Mode()=1u;
    NativeCombatMouseInput input(game,config,image,mouse,players,&PreviousMouseSlotProbe);
    check(input.DispatchMouse(12u,config.data()+12u)==23u && previous_mouse_calls==1u,"actual previous native callable receives forwarded float-pair address");input.RethrowError();
    input.Install();check(saved==previous && game.Read(84u)==input.CallbackAddress() && input.Installed(),"shared Start stores actual nonzero native callback before install");
    reject([&]{input.Install();});reject([&]{input.Restore(previous^4u);});
    const std::array<std::uint32_t,2> foreign{};
    check(input.DispatchMouse(12u,foreign.data())==2u,"WndProc ABI defers foreign payload error");reject([&]{input.RethrowError();});
    check(input.DispatchMouse(12u,config.data()+12u)==2u && input.DispatchCount()==1u,"installed real thunk invokes inactive Combat callback after deferred error recovery");input.RethrowError();
    game.WriteShared(84u,previous);reject([&]{input.Restore(previous);});game.WriteShared(84u,input.CallbackAddress());
    input.Restore(previous);
    input.Restore(previous);
    saved=previous^4u;reject([&]{input.Restore(previous);});saved=previous;
    game.WriteShared(84u,previous^4u);reject([&]{input.Restore(previous);});game.WriteShared(84u,previous);
    check(!input.Installed() && game.Read(84u)==previous && input.DispatchMouse(12u,config.data()+12u)==23u && previous_mouse_calls==2u,
          "restore publishes exact prior callable and forwards it again");input.RethrowError();input.Close();input.Close();
    NativeCombatMouseInput unknown(game,config,image,mouse,players);
    reject([&]{unknown.Install();});check(game.Read(84u)==previous,"foreign callback rejects before slot replacement");unknown.Close();
    game.WriteShared(84u,0u);
    saved=0u;
    NativeCombatMouseInput zero(game,config,image,mouse,players);
    reject([&]{zero.Restore(0u);});
    zero.Install();zero.Restore(0u);zero.Restore(0u);
    check(!zero.Installed() && game.Read(84u)==0u,
          "Ending input followed by state7 EndCombat can restore owned null callback twice");
    zero.Close();
    std::cout<<"Native combat mouse slot: real previous callable forwarding/capture/restore, concrete thunk, duplicate/foreign guards, deferred ABI error and recovery pass\n";
}
}
