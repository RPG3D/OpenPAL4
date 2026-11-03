#pragma once
#include "pal4/native_script_object_effects_runtime.h"
#include "pal4/native_script_object_visibility_runtime.h"
#include "pal4/native_scene_gob_assets.h"
#include "pal4/native_angelscript_engine.h"
#include "pal4/native_game_script_callbacks.h"
#include "pal4/native_script_frame_runner.h"
#include "pal4/script_object_visibility.h"
#include "pal4/native_script_player_action_runtime.h"
#include "pal4/script_player_position.h"
#include <rpworld.h>
#include <cmath>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <utility>
#include <vector>

namespace pal4::test {
inline void VerifyNativeScriptObjectEffects(NativeSceneGobAssets& objects,
    NativeScriptObjectVisibilityRuntime& visibility, NativeScriptPlayerActionRuntime& actions,
    IScriptPlayerPositionRuntime& positions) {
    const auto check=[](bool ok,const char* message){if(!ok)throw std::runtime_error(message);};
    auto* object=objects.FindMutableObject("MO001");
    check(object && object->Bound(),"GOB effect probe requires real MO001");
    const auto address=object->Address();
    const auto word=[](std::uint32_t p){return *reinterpret_cast<const std::uint32_t*>(p);};
    const auto position=[&]{const auto* p=reinterpret_cast<const float*>(address+0xacu);return TimedMovePosition{p[0],p[1],p[2]};};
    const auto original=position();
    check(word(address+0x184u)==1u,"GOB effect fixture begins registered");
    auto* clump=reinterpret_cast<RpClump*>(word(address+20u));
    std::vector<std::pair<RpMaterial*,RwRGBA>> colors;
    RpClumpForAllAtomics(clump,[](RpAtomic* a,void* data)->RpAtomic*{
        auto& values=*static_cast<std::vector<std::pair<RpMaterial*,RwRGBA>>*>(data);
        auto* geometry=RpAtomicGetGeometry(a);
        for(int i=0;i<RpGeometryGetNumMaterials(geometry);++i){
            auto* material=RpGeometryGetMaterial(geometry,i);values.push_back({material,*RpMaterialGetColor(material)});
        }
        return a;
    },&colors);
    check(!colors.empty(),"GOB effect fixture has real SDK materials");
    const auto alpha=[&](unsigned value){for(auto& color:colors)if(RpMaterialGetColor(color.first)->alpha!=value)return false;return true;};
    NativeAngelScriptEngine engine;
    const char* source=
        "void Position(){giGOBSetPosition(\"MO001\",11.0f,12.0f,13.0f);}"
        "void MissingPosition(){giGOBSetPosition(\"missing-object-position-fixture\",1.0f,2.0f,3.0f);}"
        "void Out(){global_mvar=1;giOBJBlendOut(\"MO001\",1.0f,true);global_mvar=2;}"
        "void In(){global_mvar=3;giOBJBlendIn(\"MO001\",1.0f,true);global_mvar=4;}"
        "void Move(){global_mvar=5;giGOBMovment(\"MO001\",1.0f,1.0f,2.0f,3.0f,true);global_mvar=6;}"
        "void PlayerMove(){global_mvar=7;giGOBMovment(\"MO001\",1.0f,4.0f,5.0f,6.0f,false);"
        "giPlayerCurrentMovment(2.0f,10.0f,20.0f,30.0f,true);global_mvar=8;}"
        "void PlayerAsync(){giPlayerCurrentMovment(2.0f,100.0f,200.0f,300.0f,false);global_mvar=9;}";
    check(engine.Raw()->AddScriptSection("gob_effect_probe","probe",source,std::strlen(source))>=0 &&
          engine.Raw()->Build("gob_effect_probe")>=0,"compile GOB effect callbacks");
    NativeScriptFrameRunner runner(engine);
    NativeScriptObjectEffectsRuntime effects(objects,visibility,runner.Context());
    effects.BindCurrentPlayer(actions,positions);
    NativeGameScriptServices services;services.object_blend=&effects.Blend();services.object_move=&effects.Movement();
    services.current_player_move=&effects.CurrentPlayerMovement();
    NativeGameScriptCallbacks callbacks(services);engine.SetContextCallbacks(&runner.Context(),&callbacks);
    runner.Queue("gob_effect_probe","Out");
    check(runner.Tick(0.f,&effects)==NativeScriptFrameStatus::suspended && effects.BlocksScript() && alpha(255),
          "real SDK object fade-out suspends before changing story continuation");
    check(runner.Tick(.5f,&effects)==NativeScriptFrameStatus::suspended && alpha(127) && engine.GlobalStoryVariable()==1,
          "half object fade updates actual material alpha and retains SDK wait");
    check(runner.Tick(.5f,&effects)==NativeScriptFrameStatus::finished && !effects.Waiting() &&
          word(address+0x184u)==0u && engine.GlobalStoryVariable()==2,
          "completed fade-out removes actual scene object and resumes SDK");
    runner.Queue("gob_effect_probe","Position");
    check(runner.Tick(0.f,&effects)==NativeScriptFrameStatus::finished && !effects.Waiting() &&
          position()==TimedMovePosition{11,12,13} && word(address+0x184u)==0u,
          "actual SDK position callback updates a hidden owned clump without a wait or visibility change");
    const auto* positioned=RwFrameGetLTM(RpClumpGetFrame(clump));
    check(positioned->pos.x==11.f && positioned->pos.y==12.f && positioned->pos.z==13.f,
          "hidden GOB position setter synchronizes real RenderWare LTM");
    runner.Queue("gob_effect_probe","MissingPosition");
    check(runner.Tick(0.f,&effects)==NativeScriptFrameStatus::finished && !effects.Waiting() &&
          position()==TimedMovePosition{11,12,13},"missing named GOB position is a no-op");
    check(ScriptObjectTimedMove(effects.Movement()).Create("MO001",1.f,{1,2,3},0u)==address && !effects.Waiting(),
          "hidden object movement does not create a wait");
    runner.Queue("gob_effect_probe","In");
    check(runner.Tick(0.f,&effects)==NativeScriptFrameStatus::suspended && alpha(0) && word(address+0x184u)==1u,
          "fade-in re-registers actual GOB and starts transparent");
    (void)runner.Tick(.25f,&effects);runner.ClearExecution();effects.ClearHandlers();
    check(alpha(255) && word(address+0x184u)==1u && !effects.Waiting(),
          "cancelled fade-in restores colors and retains scene membership");
    runner.Queue("gob_effect_probe","Move");
    check(runner.Tick(0.f,&effects)==NativeScriptFrameStatus::suspended && effects.BlocksScript(),
          "closed-tail regression establishes an unfinished real GOB move");
    const auto before_tail=position();
    effects.AppendCompletedHandler(0x849544u,1,"player-Player-Movment");
    check(runner.Tick(0.f,&effects)==NativeScriptFrameStatus::finished && effects.Waiting() &&
          !effects.BlocksScript() && position()==before_tail && engine.GlobalStoryVariable()==6,
          "closed nonblocking tail clears the execution flag without completing an earlier GOB move");
    (void)effects.Tick(.25f);
    check(effects.Waiting() && !effects.BlocksScript() && position()!=before_tail,
          "earlier GOB move keeps progressing after the script execution flag clears");
    effects.ClearHandlers();
    runner.Queue("gob_effect_probe","Move");
    check(runner.Tick(0.f,&effects)==NativeScriptFrameStatus::suspended && engine.GlobalStoryVariable()==5,
          "actual timed movement callback suspends its context");
    check(runner.Tick(1.f,&effects)==NativeScriptFrameStatus::finished && position()==TimedMovePosition{1,2,3} &&
          engine.GlobalStoryVariable()==6,"timed GOB movement updates owned transform and completes SDK wait");
    const auto* frame=RwFrameGetLTM(RpClumpGetFrame(clump));
    check(frame->pos.x==1.f && frame->pos.y==2.f && frame->pos.z==3.f,
          "timed GOB movement also updates actual RenderWare root LTM");
    ScriptObjectTimedMove move(effects.Movement());
    (void)move.Create("MO001",2.f,{100,200,300},0u);(void)effects.Tick(.5f);
    (void)move.Create("MO001",1.f,{10,20,30},0u);(void)effects.Tick(1.f);(void)effects.Tick(2.f);
    check(!effects.Waiting() && position()==TimedMovePosition{10,20,30},
          "shared wait registration replaces only matching phase5 object name");
    (void)move.Create("MO001",2.f,{100,200,300},0u);(void)effects.Tick(.25f);
    runner.Queue("gob_effect_probe","Position");
    check(runner.Tick(0.f,&effects)==NativeScriptFrameStatus::finished && effects.Waiting() &&
          !effects.BlocksScript() && position()==TimedMovePosition{11,12,13},
          "position callback preserves existing asynchronous movement handler");
    (void)effects.Tick(.5f);
    check(position()==TimedMovePosition{43.75f,87.5f,131.25f},
          "existing move continues from its captured start after direct position callback");
    const auto partial=position();effects.ClearHandlers();(void)effects.Tick(2.f);
    check(position()==partial,"cancelled GOB movement preserves partial position");
    ScriptObjectBlend blend(effects.Blend());
    (void)blend.BlendOut("MO001",1.f,0u);(void)effects.Tick(.25f);effects.ClearHandlers();
    check(word(address+0x184u)==0u && alpha(255),"cancelled fade-out restores alpha and leaves scene");
    (void)blend.BlendIn("MO001",0.f,0u);(void)effects.Tick(0.f);
    check(!effects.Waiting() && alpha(255) && word(address+0x184u)==1u,"zero-duration fade-in completes on update");
    check(!blend.BlendOut("missing-object-effects-fixture",1.f,0u) && !effects.Waiting(),"absent GOB remains no-op");
    bool rejected{};try{(void)effects.Blend().FindByName(visibility.ObjectManager()^4u,"MO001");}
    catch(const std::invalid_argument&){rejected=true;}
    check(rejected,"object effects reject foreign manager");
    rejected=false;try{(void)effects.Tick(-.1f);}catch(const std::invalid_argument&){rejected=true;}
    check(rejected,"object effects reject negative frame time");
    const auto player=actions.CurrentPlayer(actions.PlayerManager());
    const auto player_pose=[&]{const auto* p=reinterpret_cast<const float*>(player+0xacu);return TimedMovePosition{p[0],p[1],p[2]};};
    const auto player_original=player_pose();const std::string player_clip(actions.CurrentRecordName(player));
    runner.Queue("gob_effect_probe","PlayerMove");
    check(runner.Tick(0.f,&effects)==NativeScriptFrameStatus::suspended && effects.BlocksScript() &&
          actions.CurrentRecordName(player)=="C01" && engine.GlobalStoryVariable()==7,
          "current-player move selects C01 and suspends actual SDK continuation");
    check(runner.Tick(1.f,&effects)==NativeScriptFrameStatus::suspended && position()==TimedMovePosition{4,5,6},
          "nonblocking GOB and blocking player moves share ordered updates without key replacement");
    const auto halfway=player_pose();
    for(unsigned i=0;i<3;++i)check(std::fabs(halfway[i]-(player_original[i]+TimedMovePosition{10,20,30}[i])*.5f)<.001f,
          "current-player movement interpolates actual owned position");
    check(runner.Tick(1.f,&effects)==NativeScriptFrameStatus::finished && player_pose()==TimedMovePosition{10,20,30} &&
          !effects.Waiting() && engine.GlobalStoryVariable()==8,"current-player duration closes and resumes SDK exactly at endpoint");
    auto* player_clump=reinterpret_cast<RpClump*>(word(player+20u));
    const auto* player_root=RwFrameGetLTM(RpClumpGetFrame(player_clump));
    check(player_root->pos.x==10.f && player_root->pos.y==20.f && player_root->pos.z==30.f,
          "current-player timed setter updates actual RenderWare LTM");
    runner.Queue("gob_effect_probe","PlayerAsync");
    check(runner.Tick(0.f,&effects)==NativeScriptFrameStatus::finished && effects.Waiting() && !effects.BlocksScript() &&
          engine.GlobalStoryVariable()==9,"async current-player move continues SDK while retaining its handler");
    (void)effects.Tick(.5f);const auto player_partial=player_pose();
    runner.ClearExecution();effects.ClearHandlers();(void)effects.Tick(2.f);
    check(player_pose()==player_partial && !effects.Waiting(),"cancelled current-player move keeps partial position without snapping");
    bool foreign_rejected=false;try{(void)effects.CurrentPlayerMovement().ReadPosition(address);}
    catch(const std::invalid_argument&){foreign_rejected=true;}
    check(foreign_rejected,"current-player movement rejects a GOB address");
    positions.SetPosition(player,{player_original[0],player_original[1],player_original[2]});
    if(!player_clip.empty())check(actions.SetAction(player,player_clip)!=0,"restore current-player clip after timed movement fixture");
    effects.ClearHandlers();runner.ClearExecution();engine.SetContextCallbacks(&runner.Context(),nullptr);
    object->SetPosition(original);for(auto& color:colors)(void)RpMaterialSetColor(color.first,&color.second);
    check(position()==original && word(address+0x184u)==1u,"GOB effect fixture restores pose and membership");
    std::cout<<"Native object effects: SDK GOB/player timed movement, real materials/LTM, shared queue, cancellation and foreign-player guards verified\n";
}
}
