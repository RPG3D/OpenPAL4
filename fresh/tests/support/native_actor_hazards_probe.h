#pragma once
#include "pal4/native_actor_control_hazards.h"
#include "pal4/native_player_manager_session.h"
#include "pal4/native_player_hud_stats_data.h"
#include "pal4/native_player_script_property_runtime.h"
#include "pal4/native_ui_text_image_overlay.h"
#include "pal4/pal_actor_control.h"
#include "pal4/animation_event_queue.h"
#include <algorithm>
#include <bit>
#include <functional>
#include <iostream>
#include <stdexcept>

namespace pal4::test {
inline void VerifyNativeActorHazards(NativeActorControlHazards& hazards,PalActorControlState& control,
    NativePlayerManagerSession& players,NativeBaseObjectMemory& memory,NativeUiTextImageOverlay& overlay,
    const std::function<void(float)>& normal_timer,const std::function<std::uint32_t()>& zero_world_frame){
    const auto check=[](bool ok,const char* text){if(!ok)throw std::runtime_error(text);};
    check(control.words[19u]<4u && !control.words[188u] && !control.words[195u] && !control.words[204u],
        "hazard fixture starts with an owned current player and no pending statuses");
    const auto index=control.words[19u];auto& slot=players.State().slots[index];
    const auto owner=reinterpret_cast<std::uint32_t>(&slot),script=owner+0x654u,queue=owner+0xaa8u;
    check(!memory.ReadDword(queue+8u),"hazard fixture starts with empty actual player queue");
    const auto saved=control;NativePlayerHudStatsData stats(players.State());
    const auto hp=stats.PlayerStat(script,7u),maximum=stats.PlayerMaxStat(script,0u);
    check(maximum>2,"hazard actual original player maximum HP");
    const auto allocations=memory.AllocationAddresses();const auto texts=overlay.OwnedTexts();
    struct Restore {
        PalActorControlState& control;PalActorControlState saved;NativePlayerManagerSession& players;
        std::uint32_t script,queue;std::int32_t hp;
        ~Restore(){(void)AnimationEventQueue::Clear(queue,players.QueueRuntime());
            (void)NativePlayerScriptPropertyRuntime::Set(script,0x44u,7u,std::bit_cast<std::uint32_t>(hp));control=saved;}
    } restore{control,saved,players,script,queue,hp};
    const auto fill=[&]{(void)NativePlayerScriptPropertyRuntime::Set(script,0x44u,7u,static_cast<std::uint32_t>(maximum));};
    const auto drain=[&]{for(unsigned i=0;i<16u && memory.ReadDword(queue+8u);++i)
            (void)AnimationEventQueue::Update(queue,.25f,players.QueueRuntime());
        check(!memory.ReadDword(queue+8u) && overlay.OwnedTexts()==texts,"actual damage queue expires through its creating overlay");};
    fill();hazards.ApplyStatus(1000.f);
    check(stats.PlayerStat(script,7u)==1 && memory.ReadDword(queue+8u)==1u,"native hazard damage preserves original nonlethal floor");
    const auto node=memory.ReadDword(memory.ReadDword(queue+4u)),feedback=memory.ReadDword(node+12u);
    check(memory.ReadDword(feedback)==0x842698u && overlay.OwnedTexts()==texts+1u &&
        memory.ReadDword(feedback+56u)==std::bit_cast<std::uint32_t>(.5f) &&
        memory.ReadDword(feedback+60u)==std::bit_cast<std::uint32_t>(.5f),
        "native damage uses existing feedback type/text owner and exact timing setter");
    check(zero_world_frame()==1u && memory.ReadDword(queue+8u)==1u,
        "ordinary native player world frame accepts and updates the nonempty feedback queue");
    drain();fill();const auto damage=std::max(1,static_cast<int>(static_cast<long double>(maximum)*0.0099999998f*10.f));
    hazards.StartStatus(std::bit_cast<std::uint32_t>(1.f),std::bit_cast<std::uint32_t>(.5f),10.f);
    check(control.words[188u]==1u && stats.PlayerStat(script,7u)==std::max(1,maximum-damage),"native timed status applies immediate damage");
    normal_timer(.5f);
    check(control.words[188u]==1u && stats.PlayerStat(script,7u)==std::max(1,maximum-2*damage),
        "normal ActorControl timer entry applies the periodic damage");
    const auto before=stats.PlayerStat(script,7u);normal_timer(.5f);
    check(!control.words[188u] && stats.PlayerStat(script,7u)==before,"duration expiry precedes coincident damage interval");
    drain();const auto mode=PalActorControl::GetMode(control);
    hazards.BeginMovement(std::bit_cast<std::uint32_t>(.5f));
    check(control.words[195u]==1u && PalActorControl::GetMode(control)==0u,"native movement status enters conditional mode0");
    normal_timer(.25f);check(control.words[195u]==1u,"movement status remains before threshold");
    normal_timer(.25f);check(!control.words[195u] && PalActorControl::GetMode(control)==mode,"normal timer expiry restores the original movement mode");
    check(memory.AllocationAddresses()==allocations && overlay.OwnedTexts()==texts,"native hazard damage/status/feedback has no retained allocations");
    std::cout<<"Native actor hazards: real persistent HP/nonlethal clamp, existing feedback queue/text ownership/expiry, normal-frame periodic damage and movement status restore verified\n";
}
}
