#pragma once
#include "pal4/native_script_player_position_runtime.h"
#include "pal4/native_arena_npc_scene_loader.h"
#include "pal4/native_angelscript_engine.h"
#include "pal4/native_game_script_callbacks.h"
#include "pal4/native_script_player_rotation_runtime.h"
#include "pal4/native_script_player_turn_runtime.h"
#include "pal4/native_script_player_action_runtime.h"
#include "pal4/game_object_embedded_animation.h"
#include <cmath>
#include <rwcore.h>
#include <rpworld.h>
#include <bit>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <source_location>
namespace pal4::test {
inline void VerifyNativeNpcPosition(NativeArenaNpcSceneLoader& npcs,
    NativePlayerManagerSession& players,NativePlayerTransformRuntime& transforms,
    NativeScriptPlayerActionRuntime& actions) {
    const auto check=[](bool ok,const std::source_location where=std::source_location::current()) {
        if(!ok) throw std::runtime_error("native NPC position probe line="+std::to_string(where.line()));
    };
    const auto npc=npcs.Manager().FindObject("MN001");check(npc!=0u);
    const auto word=[](std::uint32_t a) {return *reinterpret_cast<const std::uint32_t*>(a);};
    const PlayerPosition old{std::bit_cast<float>(word(npc+172u)),std::bit_cast<float>(word(npc+176u)),std::bit_cast<float>(word(npc+180u))};
    const auto old_action=std::string(reinterpret_cast<const char*>(word(word(npc+0x17cu)+20u)));
    const PlayerRotation old_rotation{std::bit_cast<float>(word(npc+0xa0u)),std::bit_cast<float>(word(npc+0xa4u)),std::bit_cast<float>(word(npc+0xa8u))};
    NativeScriptPlayerPositionRuntime runtime(players,transforms);runtime.BindNpcs(&npcs);
    NativeAngelScriptEngine engine;
    NativeGameScriptServices services{};services.player_position=&runtime;
    NativeGameScriptCallbacks callbacks(services);engine.SetCallbacks(&callbacks);
    constexpr char source[]="void Move() { giNpcSetPos(\"MN001\", 12.5f, -7.0f, 33.25f); } void Missing() { giNpcSetPos(\"missing\", 1.0f, 2.0f, 3.0f); } void Turn() { giNpcSetDir(\"MN001\",120.0f,false); } void TurnWait() { giNpcSetDir(\"MN001\",210.0f,true); } void Unhold() { giNpcUnHoldAct(\"MN001\"); giNpcUnHoldAct(\"missing\"); }";
    check(engine.Raw()->AddScriptSection("npcpos","npcpos.cs",source,sizeof(source)-1)>=0 && engine.Raw()->Build("npcpos")>=0);
    constexpr char concurrent_source[]=R"(void Concurrent() {
        giPlayerSetDir(0,90.0f,false);
        giNpcSetDir("MN001",40.0f,false);
        giNpcSetDir("MN001",150.0f,true);
    })";
    check(engine.Raw()->AddScriptSection("turns","turns.cs",concurrent_source,sizeof(concurrent_source)-1)>=0 && engine.Raw()->Build("turns")>=0);
    asIScriptContext* context{};check(engine.Raw()->CreateContext(&context)>=0 && context);
    NativeScriptPlayerRotationRuntime rotation(players,transforms);rotation.BindNpcs(&npcs);
    NativeScriptPlayerTurnRuntime turn(players,rotation,actions,*context);
    services.player_turn=&turn;services.action_hold=&actions;callbacks.SetServices(services);
    const auto restore=[&] {
        turn.ClearHandlers();rotation.SetRotation(npc,old_rotation,0u);
        npcs.SetNpcPosition(npc,old,0u);check(npcs.SelectAction(npc,old_action));runtime.ProcessWaitHandlers();
    };
    try {
        for(unsigned cycle=0;cycle<2;++cycle) {
            check(context->Prepare(engine.FunctionId("npcpos","Move"))>=0 && context->Execute()==asEXECUTION_FINISHED);
            check(std::bit_cast<float>(word(npc+172u))==12.5f && std::bit_cast<float>(word(npc+176u))==-7.0f && std::bit_cast<float>(word(npc+180u))==33.25f);
            auto* clump=reinterpret_cast<RpClump*>(word(npc+20u));
            const auto& pos=RwFrameGetLTM(RpClumpGetFrame(clump))->pos;
            check(pos.x==12.5f && pos.y==-7.0f && pos.z==33.25f);
            check(runtime.WaitHandlerCount()==1u && runtime.LastWaitHandlerName()=="MN001-Player-Movment");
        }
        check(context->Prepare(engine.FunctionId("npcpos","Missing"))>=0 && context->Execute()==asEXECUTION_FINISHED && runtime.WaitHandlerCount()==1u);
        bool rejected{};try {runtime.SetNpcPosition(1u,{0,0,0},0u);}catch(const std::invalid_argument&) {rejected=true;}
        check(rejected);
        runtime.ProcessWaitHandlers();check(runtime.WaitHandlerCount()==0u);
        for(const auto* name:{"Turn","TurnWait"}) {
            const bool blocking=std::strcmp(name,"TurnWait")==0;
            check(context->Prepare(engine.FunctionId("npcpos",name))>=0);
            check(context->Execute()==(blocking?asEXECUTION_SUSPENDED:asEXECUTION_FINISHED));
            check(turn.Waiting() && turn.BlocksScript()==blocking);
            bool resumed{};unsigned frames{};
            while(turn.Waiting() && frames++<200u) resumed=turn.Tick(0.05f)||resumed;
            check(!turn.Waiting() && resumed==blocking);
            if(blocking) check(context->Execute()==asEXECUTION_FINISHED);
            check(std::abs(std::bit_cast<float>(word(npc+0xa4u))-(blocking?210.0f:120.0f))<0.01f);
        }
        const auto player=turn.PlayerByIndex(turn.PlayerManager(),0);
        const PlayerRotation player_rotation{std::bit_cast<float>(word(player+0xa0u)),std::bit_cast<float>(word(player+0xa4u)),std::bit_cast<float>(word(player+0xa8u))};
        const auto player_action=std::string(reinterpret_cast<const char*>(word(word(player+0x17cu)+20u)));
        check(context->Prepare(engine.FunctionId("turns","Concurrent"))>=0 && context->Execute()==asEXECUTION_SUSPENDED);
        check(turn.Waiting() && turn.BlocksScript());
        unsigned frames{};bool resumed{};
        while(turn.Waiting() && frames++<200u) resumed=turn.Tick(0.05f)||resumed;
        check(!turn.Waiting() && resumed && context->Execute()==asEXECUTION_FINISHED);
        check(std::abs(std::bit_cast<float>(word(npc+0xa4u))-150.0f)<0.01f);
        check(std::abs(std::bit_cast<float>(word(player+0xa4u))-90.0f)<0.01f);
        // A world-clock owner already advances the blend in UpdateInterpolator.
        // Turning must only observe/rotate while this policy is enabled.
        const bool player_world_clock=actions.UsesPlayerWorldClock();
        const bool npc_world_clock=actions.UsesNpcWorldClock();
        turn.BindAnimation(actions.AnimationBinding(player));
        actions.UsePlayerWorldClock(true);actions.UseNpcWorldClock(true);
        try {
            check(turn.SetTimedAction(player,"C01",0.2f)!=0);
            check(turn.SetTimedAction(npc,"C01",0.2f)!=0 || turn.SetTimedAction(npc,"Z01",0.2f)!=0);
            auto& player_animation=*reinterpret_cast<GameObjectEmbeddedAnimationState*>(player+0x170u);
            auto& npc_animation=*reinterpret_cast<GameObjectEmbeddedAnimationState*>(npc+0x170u);
            const auto player_remaining=player_animation.words[14u];
            const auto npc_remaining=npc_animation.words[14u];
            (void)turn.Tick(0.017f);
            check(player_animation.words[14u]==player_remaining && npc_animation.words[14u]==npc_remaining);
            check(actions.AnimationBinding(player).AdvanceFrame(0.017f));
            check(npcs.AdvanceAction(npc,0.017f));
            const float expected=0.2f-0.017f;
            check(player_animation.words[14u]==std::bit_cast<std::uint32_t>(expected) &&
                  npc_animation.words[14u]==std::bit_cast<std::uint32_t>(expected));
        } catch(...) {
            actions.UsePlayerWorldClock(player_world_clock);actions.UseNpcWorldClock(npc_world_clock);throw;
        }
        actions.UsePlayerWorldClock(player_world_clock);actions.UseNpcWorldClock(npc_world_clock);
        (void)turn.Tick(1.0f);
        rotation.SetRotation(player,player_rotation,0u);check(actions.SetAction(player,player_action)!=0);
        auto& animation=*reinterpret_cast<GameObjectEmbeddedAnimationState*>(npc+0x170u);
        (void)GameObjectEmbeddedAnimation::SetPauseFlag(animation,1u);
        actions.UpdateEffectList3(npc);
        check(animation.words[19u]==1u && animation.words[7u]!=animation.words[8u]);
        check(context->Prepare(engine.FunctionId("npcpos","Unhold"))>=0 && context->Execute()==asEXECUTION_FINISHED);
        check(animation.words[19u]==0u && animation.words[7u]==animation.words[8u]);
        restore();context->Release();
    } catch(...) {restore();context->Release();throw;}
    check(std::bit_cast<float>(word(npc+172u))==old.x && std::bit_cast<float>(word(npc+176u))==old.y && std::bit_cast<float>(word(npc+180u))==old.z);
    std::cout<<"NPC position/turn/unhold: compiled model positioning, log replacement, missing/foreign owner, timed rotation/resume, concurrent player/NPC turns and same-key replacement, hold clear/listener removal and restore verified\n";
}
}
