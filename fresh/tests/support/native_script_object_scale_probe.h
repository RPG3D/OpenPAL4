#pragma once
#include "pal4/native_script_object_effects_runtime.h"
#include "pal4/native_script_object_visibility_runtime.h"
#include "pal4/native_scene_gob_assets.h"
#include "pal4/native_angelscript_engine.h"
#include "pal4/native_game_script_callbacks.h"
#include "pal4/native_script_frame_runner.h"
#include <rpworld.h>
#include <cmath>
#include <cstring>
#include <iostream>
#include <stdexcept>

namespace pal4::test {
inline void VerifyNativeScriptObjectScale(NativeSceneGobAssets& objects,
    NativeScriptObjectVisibilityRuntime& visibility) {
    const auto check=[](bool ok,const char* message){if(!ok)throw std::runtime_error(message);};
    auto* object=objects.FindMutableObject("MO001");
    check(object && object->Bound(),"GOB scale probe requires real MO001");
    const auto address=object->Address();
    const auto word=[](std::uint32_t p){return *reinterpret_cast<const std::uint32_t*>(p);};
    const auto vector=[&](unsigned offset){const auto* p=reinterpret_cast<const float*>(address+offset);
        return TimedMovePosition{p[0],p[1],p[2]};};
    const auto original_scale=vector(0xb8u),original_position=vector(0xacu);
    check(word(address+0x184u)!=0u,"GOB scale probe requires registered object");
    auto* clump=reinterpret_cast<RpClump*>(word(address+20u));
    check(clump!=nullptr,"GOB scale probe has real RenderWare clump");
    const auto matrix_scale=[&](TimedMovePosition expected){
        const auto* matrix=RwFrameGetLTM(RpClumpGetFrame(clump));
        const RwV3d axes[]{matrix->right,matrix->up,matrix->at};
        for(unsigned i=0;i<3;++i){
            const auto& axis=axes[i];
            const auto magnitude=std::sqrt(axis.x*axis.x+axis.y*axis.y+axis.z*axis.z);
            if(std::fabs(magnitude-expected[i])>.001f)return false;
        }
        return true;
    };
    NativeAngelScriptEngine engine;
    const char* source=
        "void Scale(){global_mvar=1;giGOBScale(\"MO001\",1.5f,2.0f,true);global_mvar=2;}"
        "void Async(){giGOBScale(\"MO001\",3.0f,2.0f,false);global_mvar=3;}"
        "void Mixed(){giGOBMovment(\"MO001\",2.0f,1.0f,2.0f,3.0f,false);"
        "giGOBScale(\"MO001\",2.0f,2.0f,true);global_mvar=4;}"
        "void Missing(){giGOBScale(\"missing-scale-probe\",2.0f,1.0f,true);global_mvar=5;}";
    check(engine.Raw()->AddScriptSection("gob_scale_probe","probe",source,std::strlen(source))>=0 &&
        engine.Raw()->Build("gob_scale_probe")>=0,"compile actual giGOBScale SDK callbacks");
    NativeScriptFrameRunner runner(engine);
    NativeScriptObjectEffectsRuntime effects(objects,visibility,runner.Context());
    NativeGameScriptServices services;services.object_scale=&effects.Scale();services.object_move=&effects.Movement();
    NativeGameScriptCallbacks callbacks(services);engine.SetContextCallbacks(&runner.Context(),&callbacks);
    object->SetScale({1.f,2.f,3.f});
    runner.Queue("gob_scale_probe","Scale");
    check(runner.Tick(0.f,&effects)==NativeScriptFrameStatus::suspended &&
        effects.BlocksScript() && vector(0xb8u)==TimedMovePosition{1.f,2.f,3.f} && engine.GlobalStoryVariable()==1,
        "actual giGOBScale suspends without advancing scale on creation");
    check(runner.Tick(1.f,&effects)==NativeScriptFrameStatus::suspended &&
        vector(0xb8u)==TimedMovePosition{1.25f,1.75f,2.25f} && matrix_scale({1.25f,1.75f,2.25f}) &&
        vector(0xacu)==original_position && engine.GlobalStoryVariable()==1,
        "half scale interpolates captured nonuniform axes and actual SDK frame without moving object");
    check(runner.Tick(1.f,&effects)==NativeScriptFrameStatus::finished &&
        !effects.Waiting() && !effects.BlocksScript() && vector(0xb8u)==TimedMovePosition{1.5f,1.5f,1.5f} &&
        matrix_scale({1.5f,1.5f,1.5f}) && engine.GlobalStoryVariable()==2,
        "scale completion writes uniform endpoint and resumes actual SDK continuation");
    runner.Queue("gob_scale_probe","Async");
    check(runner.Tick(0.f,&effects)==NativeScriptFrameStatus::finished && effects.Waiting() &&
        !effects.BlocksScript() && engine.GlobalStoryVariable()==3,"async scale retains handler without blocking SDK");
    (void)effects.Tick(.5f);const auto partial=vector(0xb8u);
    runner.ClearExecution();effects.ClearHandlers();(void)effects.Tick(3.f);
    check(vector(0xb8u)==partial && !effects.Waiting(),"cancelled scale keeps partial value without snapping");
    runner.Queue("gob_scale_probe","Mixed");
    check(runner.Tick(0.f,&effects)==NativeScriptFrameStatus::suspended,"mixed scale and movement suspend on scale");
    check(runner.Tick(2.f,&effects)==NativeScriptFrameStatus::finished && !effects.Waiting() &&
        vector(0xacu)==TimedMovePosition{1,2,3} && vector(0xb8u)==TimedMovePosition{2,2,2} &&
        matrix_scale({2,2,2}) && engine.GlobalStoryVariable()==4,
        "same-object scale and movement retain distinct shared wait handlers and transform facets");
    runner.Queue("gob_scale_probe","Missing");
    check(runner.Tick(0.f,&effects)==NativeScriptFrameStatus::finished && !effects.Waiting() &&
        engine.GlobalStoryVariable()==5,"absent named object does not suspend actual script");
    bool rejected{};try{(void)effects.Scale().ReadScale(address^4u);}catch(const std::invalid_argument&){rejected=true;}
    check(rejected,"scale adapter rejects a foreign GOB identity");
    runner.ClearExecution();effects.ClearHandlers();engine.SetContextCallbacks(&runner.Context(),nullptr);
    object->SetScale(original_scale);object->SetPosition(original_position);
    check(vector(0xb8u)==original_scale && vector(0xacu)==original_position,
        "scale probe restores fixture transform");
    std::cout<<"Native GOB scale: actual SDK callback, nonuniform XYZ/LTM interpolation, blocking/async completion, cancellation and mixed movement verified\n";
}
}
