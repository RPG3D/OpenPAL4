#pragma once
#include "pal4/native_effect_scene_session.h"
#include "pal4/native_script_effect_runtime.h"
#include "pal4/native_angelscript_engine.h"
#include "pal4/native_game_script_callbacks.h"
#include "pal4/screen_effect_dispatcher.h"
#include "pal4/native_gob_object_image.h"
#include "pal4/native_base_object_memory.h"
#include "pal4/native_effect_visual_runtime.h"
#include "native_particle_probe_camera.h"
#include <rpworld.h>
#include <iostream>
#include <functional>
#include <source_location>

namespace pal4::test {
inline void VerifyNativeSceneCgFrameRoute(NativeEffectSceneSession& session,
    std::uint32_t game_camera) {
    const auto check=[](bool ok) {
        if(!ok) throw std::runtime_error("native Scene CG callback routing probe");
    };
    session.BindCamera(game_camera);
    check(!session.HasScreenEffectCapture());
    NativeGameScriptServices services{};services.effects=&session.Script();
    NativeGameScriptCallbacks callbacks(services);
    NativeAngelScriptEngine engine;engine.SetCallbacks(&callbacks);
    constexpr char source[]="void Play() { giCGEffPlay(11); } void Invert() { giCGEffPlay(1); } void Stop() { giCGEffStop(); }";
    check(engine.Raw()->AddScriptSection("cg_route","cg_route.cs",source,sizeof(source)-1)>=0);
    check(engine.Raw()->Build("cg_route")>=0);
    struct Context {
        asIScriptContext* value{};
        ~Context(){if(value)value->Release();}
    } context;
    check(engine.Raw()->CreateContext(&context.value)>=0 && context.value);
    const auto run=[&](const char* name) {
        const auto id=engine.FunctionId("cg_route",name);
        check(id>=0 && context.value->Prepare(id)>=0 && context.value->Execute()==asEXECUTION_FINISHED);
    };
    for(const auto* name : {"Play", "Invert", "Invert", "Play"}) {
        run(name);check(session.HasScreenEffectCapture());
        const auto& dispatcher=*reinterpret_cast<const ScreenEffectDispatcherState*>(session.SceneGroupAddress()+260u);
        const auto capture=dispatcher.frame_state;
        check(capture && dispatcher.active>0);
        run("Stop");
        check(!session.HasScreenEffectCapture() && !dispatcher.active &&
            dispatcher.enabled && dispatcher.frame_state==capture &&
            *reinterpret_cast<const std::uint8_t*>(capture)==1u);
        run("Stop");check(!session.HasScreenEffectCapture());
    }
    std::cout<<"Scene CG frame routing: compiled giCGEffPlay/Stop restores ordinary renderer, retains capture storage and replays\n";
}
inline void VerifyNativeBillboardVisual() {
    const auto check=[](bool ok) {
        if (!ok) throw std::runtime_error("native billboard visual assertion");
    };
    NativeBaseObjectMemory memory;
    NativeEffectVisualRuntime runtime(memory);
    EffectVisualObjectState visual{};
    visual.transform_mode=1; visual.property_flag=1;
    const std::array<float,2> size{32.0f,32.0f};
    check(EffectVisualObject::Initialize(visual,0,
        reinterpret_cast<std::uint32_t>(&size),1,1,1,runtime)!=0);
    auto& plane=*reinterpret_cast<GeoPlaneState*>(visual.geometry);
    check(plane.active && plane.width==32.0f && plane.height==32.0f &&
        RpAtomicGetGeometry(reinterpret_cast<RpAtomic*>(plane.atomic))->numVertices==4);
    runtime.SetPlaneColor(visual.geometry,0xffffffffu);
    check(plane.material_flags==0xfeffffffu);
    auto* parent=RwFrameCreate(); check(parent!=nullptr);
    const RwV3d parent_position{3.0f,4.0f,0.0f};
    (void)RwFrameTranslate(parent,&parent_position,rwCOMBINEREPLACE);
    (void)RwFrameAddChild(parent,reinterpret_cast<RwFrame*>(visual.frame));
    // Geometry mode uses its own rotation path; the default mode then faces
    // the real camera through the inherited parent transform.
    visual.geometry_mode=1;
    check(EffectVisualObject::Update(visual,0,0,0,runtime)!=0);
    visual.geometry_mode=0;
    NativeParticleProbeCamera camera;
    camera.Begin();
    check(EffectVisualObject::Update(visual,0,0,
        reinterpret_cast<std::uint32_t>(camera.camera),runtime)!=0);
    check(EffectVisualObject::Render(visual,0,0,runtime)!=0);
    const auto pixels=camera.EndPixels();
    check(pixels>0);
    EffectVisualObject::Deinitialize(visual,runtime);
    check(!visual.active && !visual.frame && !visual.geometry &&
        memory.OutstandingAllocations()==0 && !parent->child);
    (void)RwFrameDestroy(parent);
    std::cout<<"Native billboard geoPlane, camera-facing GPU pixels="<<pixels
        <<" and parent/geometry cleanup verified\n";
}
// Called with the application's actual effect session and original packages.
// The isolated camera makes pixel verification independent of scene occlusion.
inline void VerifyNativeEffectSceneSession(NativeEffectSceneSession& session,
    const std::function<void()>& scene_frame) {
    VerifyNativeBillboardVisual();
    const auto check=[&](bool ok,const std::source_location at=std::source_location::current()) {
        if(!ok) {
            std::cerr<<"Scene effect probe line="<<at.line()<<" active="<<session.ActiveCount()
                <<" pending="<<session.PendingCount()<<'\n';
            throw std::runtime_error("native scene effect session assertion");
        }
    };
    // Scene GOBs now own real effects. The fixture must preserve those owners.
    const auto baseline=session.ActiveCount();
    auto& gob=session.GobEffects();
    auto* parent=RwFrameCreate();check(parent!=nullptr);
    const RwV3d translation{10.0f,20.0f,30.0f};
    check(RwFrameTranslate(parent,&translation,rwCOMBINEREPLACE)!=nullptr);
    const auto attached=gob.Create("foot_2",0xffffffffu,false);
    check(gob.IsActive(attached.first,attached.second));
    gob.Attach(attached.first,attached.second,reinterpret_cast<std::uint32_t>(parent));
    RwFrame* child{};
    RwFrameForAllChildren(parent,[](RwFrame* frame,void* output)->RwFrame* {
        *static_cast<RwFrame**>(output)=frame;return frame;
    },&child);
    check(child && RwFrameGetParent(child)==parent && gob.Update(attached.first,attached.second));
    session.Update(0.05);
    const auto world=RwFrameGetLTM(child)->pos;
    const auto pending=session.PendingCount();
    gob.Stop(attached.first,attached.second);
    const auto detached=RwFrameGetLTM(child)->pos;
    check(!gob.IsActive(attached.first,attached.second) && !RwFrameGetParent(child) &&
        std::abs(world.x-detached.x)<0.001f && std::abs(world.y-detached.y)<0.001f &&
        std::abs(world.z-detached.z)<0.001f && session.PendingCount()==pending+1u);
    const auto replacement=gob.Create("foot_2",0xffffffffu,false);
    gob.Attach(replacement.first,replacement.second,reinterpret_cast<std::uint32_t>(parent));
    check(gob.Update(replacement.first,replacement.second)!=0u);
    gob.Release(replacement.first,replacement.second);
    unsigned children{};
    RwFrameForAllChildren(parent,[](RwFrame* frame,void* count)->RwFrame* {
        ++*static_cast<unsigned*>(count);return frame;
    },&children);
    check(!gob.IsActive(replacement.first,replacement.second) && !children);
    gob.Release(attached.first,attached.second); // stale active handle must not release pending payload
    for(unsigned tick=0u;tick<200u && session.PendingCount()>pending;++tick)
        session.Update(0.05);
    check(session.ActiveCount()==baseline && session.PendingCount()==pending);
    RwFrameDestroy(parent);
    // Exercise the complete original model-bearing EFT through the same arena
    // session used by giEffectPlay, with the application's actual world owner.
    const auto model_pending=session.PendingCount();
    const auto model=gob.Create("M_011",1u,false);
    check(gob.IsActive(model.first,model.second) && session.ActiveCount()==baseline+1u);
    bool premature_unbind{};
    try {session.UnbindArena();} catch(const std::logic_error&) {premature_unbind=true;}
    check(premature_unbind);
    auto* model_parent=RwFrameCreate();check(model_parent!=nullptr);
    gob.Attach(model.first,model.second,reinterpret_cast<std::uint32_t>(model_parent));
    check(gob.Update(model.first,model.second)!=0u);
    const auto find_atomic_frame=[](auto&& self,RwFrame* frame)->RwFrame* {
        RwFrame* result{};
        (void)RwFrameForAllObjects(frame,[](RwObject* object,void* output)->RwObject* {
            if(RwObjectGetType(object)==rpATOMIC) *static_cast<RwFrame**>(output)=RpAtomicGetFrame(reinterpret_cast<RpAtomic*>(object));
            return object;
        },&result);
        if(result) return result;
        struct Search {decltype(self)& visit;RwFrame* result{};} search{self};
        (void)RwFrameForAllChildren(frame,[](RwFrame* child,void* output)->RwFrame* {
            auto& search=*static_cast<Search*>(output);
            if(auto* found=search.visit(search.visit,child)) {search.result=found;return nullptr;}
            return child;
        },&search);
        return search.result;
    };
    RwFrame* model_frame{};
    for(unsigned tick=0;tick<20u && !model_frame;++tick) {
        session.Update(0.05);
        model_frame=find_atomic_frame(find_atomic_frame,model_parent);
    }
    if(!model_frame) {
        gob.Release(model.first,model.second);RwFrameDestroy(model_parent);
        check(false);
    }
    const auto model_start=RwFrameGetLTM(model_frame)->pos;
    bool model_moved{};
    for(unsigned tick=0;tick<40u;++tick) {
        session.Update(0.05);
        const auto current=RwFrameGetLTM(model_frame)->pos;
        model_moved|=std::abs(current.x-model_start.x)>0.001f ||
            std::abs(current.y-model_start.y)>0.001f || std::abs(current.z-model_start.z)>0.001f;
    }
    scene_frame();
    gob.Release(model.first,model.second);
    session.Update(0);
    check(model_moved && !gob.IsActive(model.first,model.second) &&
        session.ActiveCount()==baseline && session.PendingCount()==model_pending);
    unsigned model_children{};
    (void)RwFrameForAllChildren(model_parent,[](RwFrame* child,void* count)->RwFrame* {
        ++*static_cast<unsigned*>(count);return child;
    },&model_children);
    check(!model_children);RwFrameDestroy(model_parent);
    NativeGameScriptServices services{};services.effects=&session.Script();
    NativeGameScriptCallbacks callbacks(services);
    NativeAngelScriptEngine engine;engine.SetCallbacks(&callbacks);
    constexpr char source[]=
        "void Spawn() { giEffectPlay(\"foot_2\", 1, 3.0f, 4.0f, 5.0f); } "
        "void OnPlayer() { giEffectPlayWithCurrentPlayer(\"foot_2\", 1); } ";
    check(engine.Raw()->AddScriptSection("scene_effect_probe","scene_effect_probe.cs",source,sizeof(source)-1)>=0);
    check(engine.Raw()->Build("scene_effect_probe")>=0);
    struct Context {
        asIScriptContext* value{};
        ~Context(){if(value)value->Release();}
    } context;
    check(engine.Raw()->CreateContext(&context.value)>=0 && context.value);
    const auto run=[&](const char* name) {
        const auto id=engine.FunctionId("scene_effect_probe",name);
        check(id>=0 && context.value->Prepare(id)>=0 && context.value->Execute()==asEXECUTION_FINISHED);
    };
    run("Spawn");
    check(session.ActiveCount()==baseline+1u);
    unsigned colored{};
    for(unsigned tick=0u;tick<100u && session.ActiveCount()>baseline;++tick) {
        session.Update(0.05);
        if(tick==6u) {
            NativeParticleProbeCamera camera;camera.Begin();
            session.Draw(reinterpret_cast<std::uint32_t>(camera.camera));
            colored=camera.EndPixels();
        }
    }
    check(colored>0u && session.ActiveCount()==baseline);
    const auto loop=gob.Create("foot_2",0xffffffffu,false);
    check(gob.IsActive(loop.first,loop.second));
    session.Update(0.05);
    gob.Release(loop.first,loop.second);
    gob.Release(loop.first,loop.second);
    check(!gob.IsActive(loop.first,loop.second) && session.ActiveCount()==baseline);
    run("Spawn");session.Update(0.05);
    check(session.ActiveCount()==baseline+1u);
    scene_frame();
    check(session.ActiveCount()==baseline+1u);
    run("OnPlayer");
    check(session.ActiveCount()==baseline+2u);
    for(unsigned tick=0u;tick<100u && session.ActiveCount()>baseline;++tick)
        session.Update(0.05);
    check(session.ActiveCount()==baseline);
    std::cout<<"Native scene effect session: real giEffectPlay, original foot_2, "
        <<colored<<" GPU pixels, GOB parent attachment, world-preserving pending stop, "
        <<"forced release, stale handles, natural completion and actual scene frame; "
        <<"original M_011 model creation, nested world movement and active release; "
        <<baseline<<" scene GOB effects preserved\n";
}
}
