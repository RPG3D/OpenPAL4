#pragma once
#include "pal4/native_script_object_effects_runtime.h"
#include "pal4/native_script_object_visibility_runtime.h"
#include "pal4/native_scene_gob_assets.h"
#include "pal4/native_angelscript_engine.h"
#include "pal4/native_game_script_callbacks.h"
#include "pal4/native_script_frame_runner.h"
#include "pal4/game_object_embedded_animation.h"
#include "pal4/gob_action.h"
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace pal4::test {
inline void VerifyNativeScriptObjectAction(NativeSceneGobAssets& objects,
    NativeScriptObjectVisibilityRuntime& visibility){
    const auto check=[](bool ok,const char* text){if(!ok)throw std::runtime_error(text);};
    auto* object=objects.FindMutableObject("entrance1");
    check(object && object->Type()==6u && object->AnimationService(),"object action probe requires actual Q01 entrance1 animation");
    const auto address=object->Address();auto& animation=*object->AnimationService();
    const auto prior_visible=*reinterpret_cast<const std::uint32_t*>(address+0x184u);
    auto& state=*reinterpret_cast<GameObjectEmbeddedAnimationState*>(address+GobAction::kAnimationOffset);
    const auto prior_time=animation.Time();const auto restart=state.words[17],completed=state.words[18],pause=state.words[19];
    // The real manager updates only visible objects (IDA5A7350).
    // Preserve its prior state before entering that production update set.
    (void)object->SetVisible(1u);
    NativeAngelScriptEngine engine;NativeScriptFrameRunner runner(engine);
    NativeScriptObjectEffectsRuntime effects(objects,visibility,runner.Context());
    const std::string action(effects.Action().AnimationName(address));check(!action.empty(),"actual Action6 has current record name");
    const auto source=std::string("void Once(){global_mvar=1;giObjectDoAction(\"entrance1\",\"")+action+"\",-1,true);global_mvar=2;}"
        "void Twice(){global_mvar=3;giObjectDoAction(\"entrance1\",\""+action+"\",2,true);global_mvar=4;}"
        "void Background(){giObjectDoAction(\"entrance1\",\""+action+"\",0,false);global_mvar=5;}";
    check(engine.Raw()->AddScriptSection("object_action_probe","probe",source.data(),source.size())>=0 &&
        engine.Raw()->Build("object_action_probe")>=0,"compile actual giObjectDoAction signature");
    NativeGameScriptServices services;services.object_action=&effects.Action();
    NativeGameScriptCallbacks callbacks(services);engine.SetContextCallbacks(&runner.Context(),&callbacks);
    for(const bool twice:{false,true}){
        runner.Queue("object_action_probe",twice?"Twice":"Once");
        const auto first=runner.Tick(0.f,&effects);
        if(first==NativeScriptFrameStatus::failed)throw std::runtime_error(runner.Error());
        check(first==NativeScriptFrameStatus::suspended && effects.BlocksScript() && animation.Time()==0.f && !state.words[19] && state.words[17]==1u,"object action restarts actual clip, unpauses and suspends its SDK context");
        unsigned frames{};auto status=first;
        while(status==NativeScriptFrameStatus::suspended && frames++<3600u){
            (void)objects.AdvanceAnimations(1.f/60.f);status=runner.Tick(1.f/60.f,&effects);
        }
        if(status==NativeScriptFrameStatus::failed)throw std::runtime_error(runner.Error());
        check(status==NativeScriptFrameStatus::finished && !effects.Waiting() && state.words[19]==1u &&
            engine.GlobalStoryVariable()==(twice?4:2),"actual animation completion resumes same SDK and pauses object");
        check(std::fabs(animation.Time()-(twice?0.f:animation.Length()))<.0001f,"-1 action holds final frame; positive loop count resets to zero");
    }
    runner.Queue("object_action_probe","Background");
    check(runner.Tick(0.f,&effects)==NativeScriptFrameStatus::finished && effects.Waiting() && !effects.BlocksScript() && engine.GlobalStoryVariable()==5,"nonwaiting endless object action remains registered without suspending SDK");
    (void)effects.Tick(0.f);check(effects.Waiting(),"zero loop target is not treated as completed");
    (void)ScriptObjectAction(effects.Action()).Create("entrance1",action,0,0u);
    effects.ClearHandlers();
    check(!effects.Waiting() && !state.words[19],"named replacement and cancellation close handlers without pausing live animation");
    bool rejected{};try{(void)ScriptObjectAction(effects.Action()).Create("missing-object-action-fixture",action,-1,0u);}catch(const std::invalid_argument&){rejected=true;}
    check(rejected && !effects.Waiting(),"missing object action fails explicitly without retained wait");
    runner.ClearExecution();engine.SetContextCallbacks(&runner.Context(),nullptr);
    check(animation.SetTime(prior_time),"restore actual object action probe time");
    (void)object->SetVisible(prior_visible);
    state.words[17]=restart;state.words[18]=completed;state.words[19]=pause;
    std::cout<<"Native object action: SDK -298 actual Action6 clip, -1 final hold, two-loop reset, background zero-loop, named replacement/cancel verified\n";
}
}
