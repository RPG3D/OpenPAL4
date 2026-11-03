#pragma once
#include "pal4/native_script_monster_blend_runtime.h"
#include "pal4/native_script_monster_visibility_runtime.h"
#include "pal4/native_monster_manager_owner.h"
#include "pal4/native_game_script_callbacks.h"
#include "pal4/native_script_frame_runner.h"
#include <rpworld.h>
#include <iostream>
#include <stdexcept>
namespace pal4::test {
inline void VerifyNativeMonsterBlend(NativeMonsterManagerOwner& monsters,
    NativeScriptMonsterVisibilityRuntime& visibility) {
    const auto check=[](bool value,const char* why){if(!value)throw std::runtime_error(why);};
    const auto object=monsters.FindObject("m01-01-01");check(object!=0,"monster fade requires the actual M01 model");
    const auto was_visible=visibility.ReadWord(object+0x1f0u)!=0u;
    auto* clump=reinterpret_cast<RpClump*>(visibility.ReadWord(object+0x14u));
    const auto colors=[&](unsigned expected){
        struct State {unsigned expected,count{};bool equal=true;} state{expected};
        RpClumpForAllAtomics(clump,[](RpAtomic* a,void* p)->RpAtomic*{
            auto& s=*static_cast<State*>(p);auto* g=RpAtomicGetGeometry(a);
            for(int i=0;i<RpGeometryGetNumMaterials(g);++i){++s.count;s.equal=s.equal && RpMaterialGetColor(RpGeometryGetMaterial(g,i))->alpha==s.expected;}
            return a;
        },&state);
        check(state.count && state.equal,"monster fade changes all actual SDK material alphas");
    };
    NativeAngelScriptEngine engine;
    constexpr char source[]=
        "void WaitIn(){giMSTBlendIn(\"m01-01-01\",0.125,true);global_mvar=1;} "
        "void NowOut(){giMSTBlendOut(\"m01-01-01\",0.125,false);global_mvar=2;} "
        "void NowIn(){giMSTBlendIn(\"m01-01-01\",0.125,false);global_mvar=3;} "
        "void Missing(){giMSTBlendIn(\"missing-monster-blend\",0.125,true);global_mvar=4;}";
    check(engine.Raw()->AddScriptSection("monster_blend","probe.cs",source,sizeof(source)-1)>=0 && engine.Raw()->Build("monster_blend")>=0,"compile real monster fade callbacks");
    NativeScriptFrameRunner runner(engine);
    NativeScriptMonsterBlendRuntime fade(monsters,visibility,runner.Context());
    NativeGameScriptServices services{};services.monster_blend=&fade;
    NativeGameScriptCallbacks callbacks(services);engine.SetCallbacks(&callbacks);
    runner.Queue("monster_blend","WaitIn");
    check(runner.Tick(0,&fade)==NativeScriptFrameStatus::suspended && fade.BlocksScript(),"monster wait=true suspends its SDK context");colors(0);
    check(runner.Tick(.0625f,&fade)==NativeScriptFrameStatus::suspended,"monster fade waits through partial alpha");colors(127);
    check(runner.Tick(.0625f,&fade)==NativeScriptFrameStatus::finished && engine.GlobalStoryVariable()==1 && !fade.BlocksScript(),"monster fade completion resumes SDK context");colors(255);
    runner.Queue("monster_blend","NowOut");
    check(runner.Tick(0,&fade)==NativeScriptFrameStatus::finished && !fade.BlocksScript(),"monster wait=false retains asynchronous handler");
    (void)fade.Tick(.0625f);colors(127);fade.ClearHandlers();colors(255);
    check(visibility.ReadWord(object+0x1f0u)==0u,"fade-out cancellation leaves original scene entry");
    runner.Queue("monster_blend","NowIn");check(runner.Tick(0,&fade)==NativeScriptFrameStatus::finished,"nonblocking fade-in executes");
    (void)fade.Tick(.0625f);colors(127);fade.ClearHandlers();colors(255);
    check(visibility.ReadWord(object+0x1f0u)==1u,"fade-in cancellation preserves scene entry");
    const auto count=monsters.ObjectCount();runner.Queue("monster_blend","Missing");
    check(runner.Tick(0,&fade)==NativeScriptFrameStatus::finished && engine.GlobalStoryVariable()==4 && count==monsters.ObjectCount(),"missing fade target neither suspends nor creates a monster");
    if(!was_visible){const std::array<std::uint32_t,2> name{0,reinterpret_cast<std::uint32_t>("m01-01-01")};
        (void)ScriptMonsterVisibility::SetHide(reinterpret_cast<std::uint32_t>(name.data()),0,visibility);}
    std::cout<<"Native monster blend: compiled In/Out, blocking/nonblocking, alpha0/127/255, cancellation membership and missing target pass\n";
}
}

