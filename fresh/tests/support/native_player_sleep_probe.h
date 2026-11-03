#pragma once

#include "pal4/native_game_script_callbacks.h"
#include "pal4/native_scene_movement_asset.h"
#include "pal4/player_manager_object.h"
#include "pal4/script_player_actions.h"
#include "pal4/script_player_material.h"
#include <rpworld.h>
#include <memory>
#include <stdexcept>

namespace pal4::test {
inline void VerifyNativePlayerSleepCallbacks(PlayerManagerObjectState& players,
    IScriptPlayerActionsRuntime& actions, IScriptPlayerTextureRuntime& materials,
    NativeSceneMovementAsset& movement) {
    const auto check=[](bool value,const char* message) {
        if(!value)throw std::runtime_error(message);
    };
    auto& player=players.slots[0].base;
    auto* clump=reinterpret_cast<RpClump*>(player.Read(20u));
    auto* material=reinterpret_cast<RpMaterial*>(player.Read(0x550u));
    check(clump && material,"sleep probe requires actual Player Clump/FACE material");
    check(!players.Word(2832u/4u) && !movement.IsObjectClumpRegistered(clump),
          "sleep collision probe requires detached Player");
    auto* original=RpMaterialGetTexture(material);
    check(original!=nullptr,"sleep probe requires original FACE texture");
    (void)RwTextureAddRef(original);
    struct Restore {
        RpMaterial* material;RwTexture* texture;
        ~Restore(){(void)RpMaterialSetTexture(material,texture);(void)RwTextureDestroy(texture);}
    } restore{material,original};
    NativeAngelScriptEngine engine;
    NativeGameScriptServices services{};services.player_actions=&actions;services.player_material=&materials;
    NativeGameScriptCallbacks callbacks(services);engine.SetCallbacks(&callbacks);
    constexpr char script[]="void Sleep(){giPlayerSetEmotion(0,\"101_by\");} "
        "void Awake(){giPlayerSetEmotion(0,\"101_zc\");} "
        "void Attach(){giPlayerAttachCollision(0);} "
        "void Detach(){giPlayerDetachCollision(0);}";
    check(engine.Raw()->AddScriptSection("sleep","sleep.cs",script,sizeof(script)-1)>=0 &&
          engine.Raw()->Build("sleep")>=0,"compile actual sleep callbacks");
    asIScriptContext* raw=nullptr;
    check(engine.Raw()->CreateContext(&raw)>=0 && raw,"sleep callback context");
    const std::unique_ptr<asIScriptContext,void(*)(asIScriptContext*)> context(raw,[](auto* p){p->Release();});
    const auto run=[&](const char* name){
        check(raw->Prepare(engine.FunctionId("sleep",name))>=0,"prepare compiled sleep callback");
        if(raw->Execute()!=asEXECUTION_FINISHED)
            throw std::runtime_error(std::string("compiled sleep callback ")+name+": "+
                (raw->GetExceptionString()?raw->GetExceptionString():"no script exception"));
    };
    run("Sleep");auto* sleeping=RpMaterialGetTexture(material);
    check(sleeping && sleeping!=original,"real101_by replaces the FACE texture");
    run("Sleep");check(RpMaterialGetTexture(material)==sleeping,"repeated emotion reuses texture owner");
    run("Awake");check(RpMaterialGetTexture(material) && RpMaterialGetTexture(material)!=sleeping,
                       "real101_zc restores awake FACE texture");
    const auto borrowers=movement.SceneBorrowerCount();
    run("Attach");
    check(players.Word(2832u/4u)==1u && movement.IsObjectClumpRegistered(clump) &&
          movement.SceneBorrowerCount()==borrowers+1u,"compiled collision attaches the real Player Clump");
    run("Attach");check(movement.SceneBorrowerCount()==borrowers+1u,"repeated attach does not duplicate borrowers");
    run("Detach");
    check(!players.Word(2832u/4u) && !movement.IsObjectClumpRegistered(clump) &&
          movement.SceneBorrowerCount()==borrowers,"collision detach retires the actual movement borrower");
}
}
