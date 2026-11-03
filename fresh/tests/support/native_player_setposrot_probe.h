#pragma once
#include "pal4/native_game_script_callbacks.h"
#include "pal4/native_script_entity_transform_runtime.h"
#include "pal4/native_script_player_position_runtime.h"
#include "pal4/native_script_player_rotation_runtime.h"
#include "pal4/native_player_transform_runtime.h"
#include "pal4/player_manager_object.h"
#include <rpworld.h>
#include <bit>
#include <memory>
#include <iostream>
#include <stdexcept>

namespace pal4::test {
inline void VerifyNativePlayerSetPosRot(PlayerManagerObjectState& players,
    NativeScriptPlayerPositionRuntime& positions, NativeScriptPlayerRotationRuntime& rotations,
    NativePlayerTransformRuntime& transforms){
    const auto check=[](bool ok,const char* text){if(!ok)throw std::runtime_error(text);};
    auto& player=players.slots[0].base;
    const auto actor=static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(&player));
    const auto old_position=std::array<float,3>{std::bit_cast<float>(player.Read(0xacu)),
        std::bit_cast<float>(player.Read(0xb0u)),std::bit_cast<float>(player.Read(0xb4u))};
    const auto old_rotation=PlayerRotation{rotations.ReadX(actor),std::bit_cast<float>(player.Read(0xa4u)),rotations.ReadZ(actor)};
    const auto position_waits=positions.WaitHandlerCount(),rotation_waits=rotations.WaitHandlerCount();
    NativeScriptEntityTransformRuntime restore(positions,rotations,transforms);
    check(restore.FindPlayer(restore.PlayerManager(),0)==actor,"SetPosRot actual player lookup");
    struct Restore {
        NativeScriptEntityTransformRuntime& runtime;std::uint32_t actor;
        std::array<float,3> position;PlayerRotation rotation;
        ~Restore(){runtime.SetPosition(actor,position,0u);(void)runtime.SetRotation(actor,{rotation.x,rotation.yaw,rotation.z},0u);}
    } restore_scope{restore,actor,old_position,old_rotation};
    rotations.SetRotation(actor,{3.f,old_rotation.yaw,5.f},0u);
    NativeAngelScriptEngine engine;
    NativeGameScriptServices services{};services.player_position=&positions;
    services.player_rotation=&rotations;services.entity_transforms=&transforms;
    NativeGameScriptCallbacks callbacks(services);engine.SetCallbacks(&callbacks);
    constexpr char source[]="void Probe(){giPlayerSetPosRot(0,126.9f,235.4f,-448.5f,274.f);"
        "giPlayerSetPosRot(\"__missing_entity__\",0.f,0.f,0.f,0.f);}";
    check(engine.Raw()->AddScriptSection("transform","transform.cs",source,sizeof(source)-1)>=0 &&
        engine.Raw()->Build("transform")>=0,"compile actual SetPosRot overloads");
    asIScriptContext* raw{};check(engine.Raw()->CreateContext(&raw)>=0 && raw,"SetPosRot context");
    const std::unique_ptr<asIScriptContext,void(*)(asIScriptContext*)> context(raw,[](auto* p){p->Release();});
    check(raw->Prepare(engine.FunctionId("transform","Probe"))>=0,"prepare SetPosRot callback");
    if(raw->Execute()!=asEXECUTION_FINISHED)
        throw std::runtime_error(raw->GetExceptionString()?raw->GetExceptionString():"SetPosRot script failed");
    check(player.Read(0xacu)==std::bit_cast<std::uint32_t>(126.9f) &&
        player.Read(0xb0u)==std::bit_cast<std::uint32_t>(235.4f) &&
        player.Read(0xb4u)==std::bit_cast<std::uint32_t>(-448.5f),"SetPosRot writes original position fields");
    check(rotations.ReadX(actor)==3.f && rotations.ReadZ(actor)==5.f &&
        player.Read(0xa4u)==std::bit_cast<std::uint32_t>(274.f),"SetPosRot preserves X/Z and replaces yaw");
    auto* clump=reinterpret_cast<RpClump*>(player.Read(20u));
    const auto* matrix=RwFrameGetLTM(RpClumpGetFrame(clump));
    check(matrix->pos.x==126.9f && matrix->pos.y==235.4f && matrix->pos.z==-448.5f,
        "SetPosRot reaches actual RenderWare root transform");
    check(position_waits==positions.WaitHandlerCount() && rotation_waits==rotations.WaitHandlerCount(),
        "immediate SetPosRot adds no queued script handlers");
    std::cout<<"Native SetPosRot: compiled overloads, real player/RwFrame, preserved XZ, missing NPC and no queued waits verified\n";
}
}
