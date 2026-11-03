#pragma once
#include "pal4/native_actor_control_jump.h"
#include "pal4/native_actor_control_session.h"
#include "pal4/native_cegui_ui_session.h"
#include "pal4/native_scene_movement_asset.h"
#include "pal4/native_scene_player_movement.h"
#include "pal4/native_player_transform_runtime.h"
#include "pal4/native_script_player_action_runtime.h"
#include "pal4/native_script_player_position_runtime.h"
#include "pal4/native_actor_animation_state_binding.h"
#include "pal4/native_game_arena_manager_owner.h"
#include "pal4/native_arena_world_clump_binding.h"
#include "pal4/game_resource_manager.h"
#include "pal4/actor_control_database.h"
#include "pal4/input_manager.h"
#include <rwcore.h>
#include <rpworld.h>
#include <bit>
#include <cstring>
#include <iostream>
#include <stdexcept>
namespace pal4::test {
inline void VerifyNativeActorJump(const std::filesystem::path& root,NativeActorControlSession& control,
    InputManager& input,NativeCeguiUiSession& ui,NativeGameArenaManagerOwner& arena,NativePlayerTransformRuntime& transforms,
    NativeScriptPlayerActionRuntime& actions,NativeScriptPlayerPositionRuntime& positions,NativeActorAnimationStateBinding& animation,
    NativeMinimapManagerOwner& minimap,NativeCameraManagerOwner& cameras,NativeEffectSceneSession& effects,
    NativeActorControlJumpServices services,const std::function<void()>& restore_scene){
    const auto check=[](bool v,const char* text){if(!v)throw std::runtime_error(text);};
    check(control.Constructed() && control.State().words[1] && restore_scene,"native jump probe current player/cleanup services");
    const auto saved_control=control.State().words;const auto actor=saved_control[1];
    auto& resource=*reinterpret_cast<GameResourceManager*>(actor);const auto saved_resource=resource.words;
    const auto saved_action=std::string(animation.CurrentRecordName());const auto saved_time=animation.Time();const auto saved_flag=actions.GetFlag(actor);
    const std::array<unsigned,4> saved_input{input.ReadDword(0x18b8u),input.ReadDword(0x18bcu),input.ReadDword(0x18c0u),input.ReadDword(0x18c4u)};
    check(!ui.JumpHintProgressVisible() && !saved_control[35u+28u] && !saved_control[35u+29u] && !saved_control[35u+30u] &&
        !saved_control[35u+129u],"native jump probe starts without active jump/effect/progress");
    auto* dummy=reinterpret_cast<RpClump*>(saved_control[75]);check(dummy,"native jump actual retained dummy");
    auto* dummy_frame=RpClumpGetFrame(dummy);const auto saved_dummy_matrix=*RwFrameGetMatrix(dummy_frame);
    const auto saved_dummy_world=RpClumpGetWorld(dummy);
    const auto saved_player_matrix=*RwFrameGetMatrix(RpClumpGetFrame(reinterpret_cast<RpClump*>(resource.words[5])));
    NativeSceneMovementAsset collision;check(collision.Load(root,"M10","1"),"native jump actual M10 floor load");
    NativeScenePlayerMovement movement(collision);
    NativeClumpCollisionHit hit;check(collision.FindFloorCollision({542.8015137f,211.5036011f,-538.3237305f},1000.f,1000.f,hit),"native jump real M10 wine vicinity floor");
    unsigned ui_hides{},stops{},sounds{};
    const auto original_ui=services.ui_state;const auto original_stop=services.stop_monsters;const auto original_sound=services.play_sound;
    services.ui_state=[&](unsigned mode){if(mode==0u)++ui_hides;original_ui(mode);};
    services.stop_monsters=[&]{++stops;original_stop();};
    services.play_sound=[&](std::string_view path,unsigned mode){check(path=="gamedata\\PALSound\\WF027.wav" && mode==1u,"native jump actual launch sound request");++sounds;original_sound(path,mode);};
    // This isolated test must not execute a mainline danger script.
    services.script.initialize=[] {throw std::runtime_error("native jump initialization fixture must not execute danger");};
    NativeActorControlJump jump(control,input,ui,arena,movement,transforms,actions,positions,minimap,cameras,effects,std::move(services));
    bool cleanup_started=false;
    const auto cleanup=[&]{
        // A failing restoration is itself a fixture failure; never retry partially
        // restored owner operations (notably attaching an already attached clump).
        if(cleanup_started)return;
        cleanup_started=true;
        auto* db=control.State().words.data()+35u;
        jump.BindComponent(db,actor);
        check(ui.SetJumpHintProgressVisible(false),"native jump probe hides progress during restore");
        (void)NativeArenaWorldClumpBinding::Detach(arena.Address(),dummy);
        restore_scene();
        check(actions.SetAction(actor,saved_action)!=0 && animation.SetTime(saved_time),"native jump restores original action/time");actions.SetFlag(actor,saved_flag);
        for(unsigned i=0;i<4u;++i)input.WriteDword(0x18b8u+i*4u,saved_input[i]);
        (void)resource.UpdateClumpTransform({std::bit_cast<float>(saved_resource[43]),std::bit_cast<float>(saved_resource[44]),std::bit_cast<float>(saved_resource[45])},false,transforms);
        RwFrameTransform(RpClumpGetFrame(reinterpret_cast<RpClump*>(resource.words[5])),&saved_player_matrix,rwCOMBINEREPLACE);
        RwFrameUpdateObjects(RpClumpGetFrame(reinterpret_cast<RpClump*>(resource.words[5])));resource.words=saved_resource;
        RwFrameTransform(dummy_frame,&saved_dummy_matrix,rwCOMBINEREPLACE);RwFrameUpdateObjects(dummy_frame);
        if(saved_dummy_world && RpClumpGetWorld(dummy)!=saved_dummy_world)
            check(RpWorldAddClump(saved_dummy_world,dummy)==saved_dummy_world,"native jump restores dummy original world");
        control.State().words=saved_control;
    };
    try{
        bool rejected=false;try{(void)jump.ProcessDatabase(control.State().words.data()+34u,actor,.017f);}catch(const std::invalid_argument&){rejected=true;}
        check(rejected,"native jump rejects adjacent component");
        (void)resource.UpdateClumpTransform({hit.point[0],hit.point[1],hit.point[2]},false,transforms);
        auto* db=control.State().words.data()+35u;jump.BindComponent(db,actor);
        db[33]=1u; // Isolated fixture setup of the independently tested AlwaysJump word.
        check(!jump.DatabaseHasPriority(db),"native jump AlwaysJump passes original display gate");
        input.WriteDword(0x18b8u,1u);input.WriteDword(0x18c0u,0u);input.WriteDword(0x18c4u,1u);
        check(!jump.ProcessDatabase(db,actor,.017f) && db[29]==1u && !db[30] && ui.JumpHintProgressVisible(),"native jump actual held Space stages preview");
        check(ui.AdvanceJumpHintProgress(.25f),"native jump advances actual progress UI");
        check(!jump.ProcessDatabase(db,actor,.017f) && db[20]==std::bit_cast<unsigned>(100.f) && db[21]==std::bit_cast<unsigned>(15.f),"native jump shared progress range100/height15");
        input.WriteDword(0x18b8u,0u);input.WriteDword(0x18c0u,1u);input.WriteDword(0x18c4u,0u);
        check(jump.ProcessDatabase(db,actor,.017f) && db[28]==1u && db[29]==0u && db[30]==1u && !db[31] &&
            !ui.JumpHintProgressVisible() && jump.DatabaseBusy(db),"native jump release initializes persistent flight state");
        check(db[11]==std::bit_cast<unsigned>(588.f) && db[12]==saved_control[35u+12u] && db[2]!=0u && ui_hides==1u && stops==1u && sounds==1u,
            "native jump embedded gravity/velocity and actual UI/monster/sound dispatch once");
        check(RpClumpGetWorld(dummy)==nullptr,"native jump release detaches actual preview dummy");
        cleanup();
    }catch(...){cleanup();throw;}
    check(control.State().words==saved_control && resource.words==saved_resource && RpClumpGetWorld(dummy)==saved_dummy_world,
        "native jump restores borrowed control/player/dummy owners");
    std::cout<<"Native jump: actual M10 floor, held/released Space, real preview/progress, shared persistent physics initialization, delegated UI/monster/audio, foreign owner rejection and cleanup pass (no flight/pickup claim)\n";
}
}
