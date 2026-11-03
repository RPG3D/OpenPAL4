#include "native_effect_track_probe.h"
#include "native_effect_facing_probe.h"
#pragma once
#include "native_effect_revolution_probe.h"
#include "native_rump_shadow_probe.h"
#include "pal4/native_nested_effect_dialogs.h"
#include "pal4/binary_xml_config.h"
#include "pal4/effect_object.h"
#include "pal4/native_clump_effect_object.h"
#include "pal4/native_rw_geometry_processor.h"
#include "pal4/game_world_object.h"
#include "native_effect_rotation_probe.h"
#include "native_effect_stop_probe.h"
#include <rwcore.h>
#include <rpworld.h>
#include <rphanim.h>
#include <rpskin.h>
#include <rtanim.h>
#include <source_location>

namespace pal4::test {
inline void VerifyNativeNestedEffectObjects(const std::filesystem::path& game_root,
    NativeBaseObjectMemory& memory,NativeEventDocumentRuntime& documents,
    NativeParticleEffectFactory& objects,NativeEffectObjectQueues& queues,
    NativeEffectDialogLoader& loader,
    NativeEffectGroupFrame& frames,NativeEffectTaskFactory& tasks,
    NativeScriptObjectRuntime& scheduler,
    NativePkFileManagerRuntime& packages,
    std::uint32_t owner,std::uint32_t group,std::uint32_t system,
    NativeArenaEffectServices& geometry,NativeEffectSupportObjects& support) {
    const auto check=[](bool ok,const std::source_location at=std::source_location::current()) {
        if(!ok) {std::cerr<<"nested effect probe line "<<at.line()<<std::endl;
            throw std::runtime_error("native nested effect assertion");}
    };
    const auto baseline=memory.OutstandingAllocations();
    const auto object_count=objects.LiveObjects(),dialog_count=loader.LiveDialogs();
    const auto instances=memory.ReadDword(system+164u);
    const auto cached=loader.CachedDocuments();
    const auto doc_count=documents.DocumentCount();
    NativePackageArchive archive;check(archive.Open((game_root/"gamedata/Effect.cpk").string().c_str()));
    const auto data=archive.ReadMember("script\\H_077.eft");BinaryXmlConfig original;
    check(data && original.Parse(*data));
    // Original leaf XML is preserved; its enclosing EffectObject is a fixture.
    BinaryXmlNode wrapper{"Object",{{"Attr_Class_Name",3u,std::string("EffectObject")}}, {original.Root()}};
    const auto bytes=BinaryXmlConfig::Serialize(wrapper);check(bytes.has_value());
    std::array<std::uint32_t,15> xml{};
    const auto root=reinterpret_cast<std::uint32_t>(xml.data());
    (void)documents.InitializeDocument(root,"nested-probe",0u);
    check(documents.LoadBinaryDocument(root,*bytes));
    const auto source_baseline=memory.OutstandingAllocations();
    const auto object=objects.Create(root,owner,group);
    check(object && objects.ObjectTypeName(object)=="EffectObject" && objects.LiveObjects()==object_count+4u);
    check(loader.LiveDialogs()==dialog_count+1u && loader.CachedDocuments()==cached && tasks.LiveTasks()==10u);
    const auto child=memory.ReadDword(object+92u),own_frame=objects.Frame(object);
    const auto child_frame=loader.Dialog(child).State().words[43];
    check(child && child_frame && own_frame && queues.Count(140u)==1u && queues.Count(176u)==2u);
    check(!RwFrameGetParent(reinterpret_cast<RwFrame*>(child_frame)));
    bool live_unbind_rejected{};
    try{objects.UnbindNestedDialogs();}catch(const std::logic_error&){live_unbind_rejected=true;}
    check(live_unbind_rejected);
    const auto scale=objects.ResolveFacet(object,"ScaleTask");
    objects.SetScale(scale,{2.0f,3.0f,4.0f},0u);
    std::array<float,3> actual{};objects.ReadScale(scale,actual);
    check(actual==std::array<float,3>{2,3,4});
    const auto* matrix=RwFrameGetMatrix(reinterpret_cast<RwFrame*>(own_frame));
    check(matrix->right.x==2 && matrix->up.y==3 && matrix->at.z==4);
    const auto rotation=objects.ResolveFacet(object,"RotateTask");
    const auto position=objects.ResolveFacet(object,"PositionTask");
    std::array<float,3> saved_rotation{},saved_position{};
    objects.ReadRotation(rotation,saved_rotation);saved_position=objects.ReadPosition(position);
    objects.Move(position,{4,5,6},false);
    objects.ApplyRotation(rotation,{0,90,0},false);
    objects.ReadRotation(rotation,actual);check(actual==std::array<float,3>{0,90,0});
    const auto close_angle=[](float a,float b){return std::abs(a-b)<0.001f;};
    check(close_angle(matrix->right.x,0) && close_angle(std::abs(matrix->right.z),2) &&
        close_angle(matrix->up.y,3) && close_angle(std::abs(matrix->at.x),4) &&
        close_angle(matrix->at.z,0) && close_angle(matrix->pos.x,4) &&
        close_angle(matrix->pos.y,5) && close_angle(matrix->pos.z,6));
    objects.ApplyRotation(rotation,{0,-90,0},true);
    objects.ReadRotation(rotation,actual);check(actual==std::array<float,3>{0,0,0});
    check(close_angle(matrix->right.x,2) && close_angle(matrix->up.y,3) && close_angle(matrix->at.z,4));
    objects.ApplyRotation(rotation,saved_rotation,false);
    objects.Move(position,saved_position,false);
    objects.SetScale(scale,{1,1,1},0);
    VerifyNativeNestedEffectFacing(memory,objects,object);
    VerifyNativeEffectRotation(memory,documents,objects,tasks,scheduler,object);
    VerifyNativeEffectRevolution(memory,documents,objects,tasks,scheduler,object);
    const auto play=[&](std::uint8_t loop) {
        std::array<std::uint32_t,15> task_xml{};
        const auto node=reinterpret_cast<std::uint32_t>(task_xml.data());
        (void)documents.InitializeDocument(node,"nested-play-task",0u);
        BinaryXmlNode definition{"Task",{
            {"Attr_Class_Name",3u,std::string("PlayEffectTask")},
            {"Attr_Task_PlayEffect_Loop",1u,std::int32_t(loop)},
            {"Attr_Inherit_Pre_Task",1u,std::int32_t(0)}}, {}};
        const auto serialized=BinaryXmlConfig::Serialize(definition);
        check(serialized && documents.LoadBinaryDocument(node,*serialized));
        const auto task=tasks.CreateTask(node);check(task);
        documents.DestroyDocument(node);
        check(memory.ReadDword(task+76u)==0u && memory.ReadByte(task+72u)==loop);
        memory.WriteDword(task+64u,object);
        GomTaskContainerState container{};
        const auto handle=reinterpret_cast<std::uint32_t>(&container);
        scheduler.InitializeTaskContainer(handle);
        scheduler.AddTask(handle,task,"nested-play",{2u,0.2f,0.0f});
        (void)scheduler.BeginWaiting(handle);
        (void)scheduler.UpdateTaskContainer(handle,0.05f);
        check(memory.ReadDword(task+40u)==2u && !memory.ReadByte(object+6u));
        bool reset_seen{};
        for(unsigned tick=0;tick<20u && container.count;++tick) {
            (void)scheduler.UpdateTaskContainer(handle,0.05f);
            if(container.count && memory.ReadDword(task+40u)==4u) {
                check(!memory.ReadDword(task+76u) && memory.ReadByte(task+72u)==loop);
                reset_seen=true;
            }
        }
        check(reset_seen && !container.count && tasks.LiveTasks()==10u && memory.ReadByte(object+6u));
        (void)scheduler.DestroyTaskContainer(handle);
    };
    queues.QueueEffect(group,object);
    play(0u);
    check(memory.ReadByte(object+6u) && RwFrameGetParent(reinterpret_cast<RwFrame*>(child_frame))==reinterpret_cast<RwFrame*>(own_frame));
    const auto playback_control=[&](bool rewind) {
        const auto allocations=memory.OutstandingAllocations();
        const auto live=tasks.LiveTasks(),child_manager=memory.ReadDword(child+8u);
        check(child_manager && !memory.ReadByte(child+82u));
        // Resume accepts an already-paused saved/runtime state. Pause on an
        // ordinary active child retains IDA610B00's early return unchanged.
        if(rewind) memory.WriteByte(child+82u,1u);
        std::array<std::uint32_t,15> task_xml{};
        const auto node=reinterpret_cast<std::uint32_t>(task_xml.data());
        (void)documents.InitializeDocument(node,"nested-playback-control",0u);
        BinaryXmlNode definition{"Task",{
            {"Attr_Class_Name",3u,std::string(rewind?"RewindEffectTask":"PauseEffectTask")},
            {"Attr_Inherit_Pre_Task",1u,std::int32_t(0)}}, {}};
        const auto serialized=BinaryXmlConfig::Serialize(definition);
        check(serialized && documents.LoadBinaryDocument(node,*serialized));
        const auto task=tasks.CreateTask(node);check(task);
        documents.DestroyDocument(node);memory.WriteDword(task+64u,object);
        GomTaskContainerState container{};
        const auto handle=reinterpret_cast<std::uint32_t>(&container);
        scheduler.InitializeTaskContainer(handle);
        scheduler.AddTask(handle,task,"nested-control",{2u,0.2f,0.15f});
        (void)scheduler.BeginWaiting(handle);
        (void)scheduler.UpdateTaskContainer(handle,0.05f);
        check(memory.ReadDword(task+72u)==0u &&
            memory.ReadByte(child+82u)==static_cast<std::uint8_t>(rewind));
        bool began{},finished{};
        for(unsigned tick=0;tick<20u && container.count;++tick) {
            (void)scheduler.UpdateTaskContainer(handle,0.05f);
            if(container.count && memory.ReadDword(task+72u)) {
                check(memory.ReadDword(task+72u)==object+36u && !memory.ReadByte(child+82u));
                began=true;
            }
            if(container.count && memory.ReadDword(task+40u)==4u) {
                check(!memory.ReadDword(task+72u));finished=true;
            }
        }
        check(began && finished && !container.count && !memory.ReadByte(child+82u) &&
            memory.ReadDword(child+8u)==child_manager && memory.ReadByte(object+6u) &&
            tasks.LiveTasks()==live);
        (void)scheduler.DestroyTaskContainer(handle);
        check(memory.OutstandingAllocations()==allocations);
    };
    playback_control(false);
    playback_control(true);
    unsigned pixels{};
    for(unsigned i=0;i<160;++i) {
        frames.Update(0.05);
        if(i==4) {
            NativeParticleProbeCamera camera;camera.Begin();
            std::array<std::uint32_t,2> source{};source[1]=reinterpret_cast<std::uint32_t>(camera.frame);
            frames.Draw(reinterpret_cast<std::uint32_t>(source.data()));pixels=camera.EndPixels();
        }
    }
    check(pixels && !memory.ReadByte(object+6u) && !memory.ReadByte(object+7u) &&
        !tasks.LiveTasks() && !memory.ReadDword(child+8u));
    // Reuse the same payload owners and XML templates after natural completion.
    check(objects.ResetPlayback(object));queues.QueueEffect(group,object);
    play(1u);
    check(tasks.LiveTasks()==10u);
    bool ended{},looped{};
    for(unsigned i=0;i<320 && !looped;++i) {
        frames.Update(0.05);
        if(!memory.ReadDword(child+8u)) ended=true;
        else if(ended) looped=true;
    }
    check(looped && tasks.LiveTasks()==10u && memory.ReadByte(object+6u) && objects.Frame(object)==own_frame);
    objects.SetMode(object,1u);
    check(objects.Duration(object)>0 && !tasks.LiveTasks() && !memory.ReadDword(child+8u));
    objects.SetMode(object,0u);
    VerifyNativeNestedEffectStop(memory,documents,objects,tasks,scheduler,queues,frames,object,child,group);
    check(objects.ResetPlayback(object));queues.QueueEffect(group,object);play(0);
    frames.Update(0.05);
    objects.ReleasePlayback(object,queues);objects.ReleasePlayback(object,queues);
    check(!objects.Frame(object) && loader.LiveDialogs()==dialog_count && !tasks.LiveTasks());
    // Original inactive facet calls are no-ops, including after frame release.
    std::array<std::uint32_t,7> released_aim{};
    for(unsigned i=0;i<released_aim.size();++i)released_aim[i]=memory.ReadDword(object+104u+4u*i);
    const auto released_facing=objects.ResolveFacet(object,"MoveToTargetTask");
    objects.Face(released_facing,{1.f,2.f,3.f});
    objects.SetFaceRotation(released_facing,{4.f,5.f,6.f});objects.ActivateFace(released_facing);
    for(unsigned i=0;i<released_aim.size();++i)check(memory.ReadDword(object+104u+4u*i)==released_aim[i]);

    objects.Destroy(object);
    // The normal frame phase owns deferred tree-node pruning.
    frames.Update(0);
    check(!memory.ReadDword(group+224u) && !queues.Count(140u) && !queues.Count(176u));
    check(objects.LiveObjects()==object_count && memory.ReadDword(system+164u)==instances &&
        memory.OutstandingAllocations()==source_baseline);
    // Drive the real M_011 nesting without replacing any model-bearing clump.
    // Preserve original nested XML, model and scheduled tasks; also retain the
    // explicit unbound-resource failure test after successful bound playback.
    const auto m011=archive.ReadMember("script\\M_011.eft");
    check(m011 && documents.LoadBinaryDocument(root,*m011));
    const auto config=documents.ChildAt(documents.FindChild(root,"OxnList"),0u);
    check(config);
    const auto find_clump=[&](auto&& self,std::uint32_t node)->std::uint32_t {
        const auto type=documents.FindNumericAttribute(node,"Attr_Class_Name");
        if(type && documents.AttributeToString(type)=="ClumpObject") return node;
        for(std::uint32_t i=0;i<documents.ChildCount(node);++i)
            if(const auto result=self(self,documents.ChildAt(node,i))) return result;
        return 0u;
    };
    const auto crane_config=find_clump(find_clump,config);check(crane_config);
    const auto model_baseline=memory.OutstandingAllocations();
    const auto package_count=packages.LoadedPackageCount();
    struct World {
            GameWorldObjectState arena{};
            RpWorld* world{};
            RpLight* ambient{};
            NativeArenaEffectServices& services;
            explicit World(NativeArenaEffectServices& s):services(s) {
                RwBBox bounds{{1000,1000,1000},{-1000,-1000,-1000}};
                world=RpWorldCreate(&bounds);
                if(!world) throw std::runtime_error("effect visibility test world");
                ambient=RpLightCreate(rpLIGHTAMBIENT);
                if(!ambient) throw std::runtime_error("effect probe ambient light");
                const RwRGBAReal white{1,1,1,1};RpLightSetColor(ambient,&white);RpWorldAddLight(world,ambient);
                arena.words[4]=reinterpret_cast<std::uint32_t>(world);arena.words[12]=1u;
                services.BindArena(reinterpret_cast<std::uint32_t>(&arena));
            }
            ~World(){services.UnbindArena();RpWorldRemoveLight(world,ambient);RpLightDestroy(ambient);RpWorldDestroy(world);}
    };
    for(unsigned pass=0;pass<2u;++pass) {
        World world(geometry);
        {
            NativeClumpEffectObject crane(memory,documents,system,game_root,&packages);
            crane.BindVisibility(support,geometry);
            const auto source=documents.CloneDocument(crane_config);
            check(crane.Load(source));
            documents.DestroyDocument(source);(void)memory.Free(source);
            auto& state=crane.State();
            state.words[4]=group;
            const auto manager=state.words[18];
            auto* clump=reinterpret_cast<RpClump*>(memory.ReadDword(manager+20u));
            check((state.words[1]&0xffu) && state.words[19] && crane.Frame() &&
                memory.ReadDword(manager+36u)==1u && state.words[17]==0xfeffffffu);
            auto* frame=reinterpret_cast<RwFrame*>(crane.Frame());
            check(std::abs(RwFrameGetMatrix(frame)->pos.y-20.0f)<0.001f);
            unsigned atomics{};
            (void)RpClumpForAllAtomics(clump,[](RpAtomic* a,void* p)->RpAtomic* {
                ++*static_cast<unsigned*>(p);
                for(int m=0;m<RpGeometryGetNumMaterials(RpAtomicGetGeometry(a));++m) {
                    const auto* color=RpMaterialGetColor(RpGeometryGetMaterial(RpAtomicGetGeometry(a),m));
                    if(color->red!=255 || color->green!=255 || color->blue!=255 || color->alpha!=254)
                        throw std::runtime_error("original crane material RGBA mismatch");
                }
                return a;
            },&atomics);check(atomics==1u);
            const auto embedded=reinterpret_cast<std::uint32_t>(&state)+108u;
            check(documents.AttributeToString(documents.FindNumericAttribute(embedded,"Attr_Clump_FileName"))=="HY\\xianhe\\XH");
            RwV3d moved{100,200,300};(void)RwFrameTranslate(frame,&moved,rwCOMBINEREPLACE);
            struct Hierarchy {
                static RwFrame* Visit(RwFrame* frame,void* output) {
                    auto*& found=*static_cast<RpHAnimHierarchy**>(output);
                    if(auto* h=RpHAnimFrameGetHierarchy(frame)) {found=h;return nullptr;}
                    (void)RwFrameForAllChildren(frame,Visit,output);return found?nullptr:frame;
                }
            };
            RpHAnimHierarchy* hierarchy{};
            (void)RwFrameForAllChildren(frame,Hierarchy::Visit,&hierarchy);
            check(hierarchy && hierarchy->currentAnim);
            (void)RtAnimInterpolatorSetCurrentTime(hierarchy->currentAnim,0.5f);
            RpAtomic* first{};
            (void)RpClumpForAllAtomics(clump,[](RpAtomic* atomic,void* output)->RpAtomic* {
                *static_cast<RpAtomic**>(output)=atomic;return nullptr;
            },&first);
            check(first && RpSkinAtomicGetHAnimHierarchy(first)==hierarchy);
            const auto original_render=RpAtomicGetRenderCallBack(first);
            (void)EffectSystem::ProcessClumpAtomics(group,reinterpret_cast<std::uint32_t>(clump),geometry);
            check(reinterpret_cast<std::uint32_t>(RpAtomicGetRenderCallBack(first))==
                NativeRwGeometryProcessor::RenderCallbackAddress());
            support.SetRadius(1200.0f);
            check(support.RegisterClump(reinterpret_cast<std::uint32_t>(clump)));
            check(reinterpret_cast<std::uint32_t>(RpAtomicGetRenderCallBack(first))!=
                NativeRwGeometryProcessor::RenderCallbackAddress());
            support.UnregisterClump(reinterpret_cast<std::uint32_t>(clump));
            check(reinterpret_cast<std::uint32_t>(RpAtomicGetRenderCallBack(first))==
                NativeRwGeometryProcessor::RenderCallbackAddress());
            (void)EffectSystem::ProcessClumpAtomicsWrapper(group,reinterpret_cast<std::uint32_t>(clump),geometry);
            check(RpAtomicGetRenderCallBack(first)==original_render);
            support.CleanupAuxiliary();
            (void)RpSkinAtomicSetHAnimHierarchy(first,nullptr);
            check(crane.Reset() && hierarchy->currentAnim->currentTime==0.5f);
            (void)RpSkinAtomicSetHAnimHierarchy(first,hierarchy);
            // Original reset locates descendants itself; animation time and
            // cached transforms must survive destruction of the source XML.
            check(crane.Reset() && std::abs(RwFrameGetMatrix(frame)->pos.y-20.0f)<0.001f);
            check(hierarchy->currentAnim->currentTime==0.0f);
            support.SetRadius(1200.0f);
            crane.Show(true);crane.Show(true);
            check(RpClumpGetWorld(clump)==world.world &&
                (state.words[1]&0x00ff0000u) && !(state.words[1]&0xff000000u) &&
                memory.ReadDword(system+144u) &&
                memory.ReadDword(memory.ReadDword(system+144u)+24u)==1u &&
                memory.ReadDword(memory.ReadDword(system+144u)+40u)==1u);
            crane.Advance(0.25);
            check(hierarchy->currentAnim->currentTime==0.25f);
            (void)RpSkinAtomicSetHAnimHierarchy(first,nullptr);
            crane.Advance(0.25);check(hierarchy->currentAnim->currentTime==0.25f);
            (void)RpSkinAtomicSetHAnimHierarchy(first,hierarchy);
            state.words[42]|=1u;crane.Advance(0.25);
            check(hierarchy->currentAnim->currentTime==0.25f);state.words[42]&=~1u;
            (void)RtAnimInterpolatorSetCurrentTime(hierarchy->currentAnim,hierarchy->currentAnim->pCurrentAnim->duration-0.05f);
            crane.Advance(0.1);check(hierarchy->currentAnim->currentTime==0.0f);
            crane.SetColor({31,53,79,200});crane.SetMode(1u);crane.Advance(0.25);
            std::array<std::uint8_t,4> color{};crane.ReadColor(color);
            check(color==std::array<std::uint8_t,4>{31,53,79,137});
            const auto* material_color=RpMaterialGetColor(RpGeometryGetMaterial(RpAtomicGetGeometry(first),0));
            check(material_color->red==31 && material_color->green==53 && material_color->blue==79 && material_color->alpha==137);
            crane.SetMode(0u);
            if(pass==0u) {
                crane.Show(false);world.arena.words[12]=0u;crane.Show(true);
                check(!RpClumpGetWorld(clump) && (state.words[1]&0xff000000u));
                const auto sphere=*RpAtomicGetWorldBoundingSphere(first);
                const auto distance=std::max(sphere.radius*3.0f,1.0f);
                NativeParticleProbeCamera camera;
                RpWorldAddCamera(world.world,camera.camera);
                camera.BeginAt({sphere.center.x,sphere.center.y,sphere.center.z-distance},distance*0.01f,distance*10.0f);
                geometry.InitializeGeometryData(group+52u);
                std::array<RwUInt32,5> before{},after{};
                constexpr std::array<RwUInt32,5> states{27u,19u,20u,7u,14u};
                for(unsigned i=0;i<states.size();++i) RwD3D9GetRenderState(states[i],&before[i]);
                RwUInt32 cull_before{},cull_after{};
                check(RwRenderStateGet(rwRENDERSTATECULLMODE,&cull_before));
                (void)crane.Draw(0u);
                for(unsigned i=0;i<states.size();++i) RwD3D9GetRenderState(states[i],&after[i]);
                check(RwRenderStateGet(rwRENDERSTATECULLMODE,&cull_after));
                check(before==after && cull_before==cull_after && !support.TakeRenderError());
                // The original scene route renders world members through the
                // world's camera/light context, then drains the effect queue.
                geometry.InitializeGeometryData(group+52u);
                crane.Show(false);world.arena.words[12]=1u;crane.Show(true);
                (void)RpClumpRender(clump);
                geometry.SortAtomicGeometries(group+52u);
                check(!NativeRwGeometryProcessor::TakeRenderError());
                const auto crane_pixels=camera.EndPixels();
                RpWorldRemoveCamera(world.world,camera.camera);
                std::cout<<"Native original M_011 crane: "<<crane_pixels<<" GPU pixels\n";
                check(crane_pixels>0u);
                geometry.InitializeGeometryData(group+52u);
                crane.Show(false);world.arena.words[12]=1u;crane.Show(true);
            }
            if(pass==0u) {
                crane.Show(false);crane.Show(false);
                check(!RpClumpGetWorld(clump) && !(state.words[1]&0xffff0000u) &&
                    RpAtomicGetRenderCallBack(first)==original_render);
                crane.Show(true);
                struct ReleaseQueue final : INativeDialogParticleQueues {
                    ClumpObjectState& state;std::uint32_t group;unsigned removals{};
                    ReleaseQueue(ClumpObjectState& s,std::uint32_t g):state(s),group(g){}
                    void QueueEffect(std::uint32_t,std::uint32_t) override {throw std::logic_error("release probe does not queue");}
                    void RemoveEffect(std::uint32_t owner,std::uint32_t object) override {
                        if(owner!=group || object!=reinterpret_cast<std::uint32_t>(&state) || state.words[18] || state.words[19])
                            throw std::runtime_error("ClumpObject release notification ordering");
                        ++removals;
                    }
                } released(state,group);
                crane.ReleasePlayback(released);crane.ReleasePlayback(released);
                check(released.removals==1u && !crane.Frame() && !state.words[18] && !state.words[19] && !(state.words[1]&0xffu));
                crane.Advance(0.1);check(!crane.Draw(0u));
            }
            // Pass1 destroys a still-visible model. Its callbacks and world
            // membership must be removed before freeing the SDK clump/frame.
        }
        check(!memory.ReadDword(memory.ReadDword(system+144u)+24u) &&
            !memory.ReadDword(memory.ReadDword(system+144u)+40u));
        support.CleanupAuxiliary();
        check(memory.OutstandingAllocations()==model_baseline &&
            memory.ReadDword(system+164u)==instances && packages.LoadedPackageCount()==package_count);
    }
    {
        NativeClumpEffectObject missing_model(memory,documents,system,game_root,&packages);
        const auto source=documents.CloneDocument(crane_config);
        documents.WriteStringAttribute(source,"Attr_Clump_FileName","HY\\xianhe\\NOSUCHMODEL");
        check(!missing_model.Load(source) && !missing_model.Frame() && !missing_model.State().words[18]);
        documents.DestroyDocument(source);(void)memory.Free(source);
    }
    check(memory.OutstandingAllocations()==model_baseline && memory.ReadDword(system+164u)==instances);
    {
        World world(geometry);
        objects.BindModelResources(game_root,packages,support,geometry);
        VerifyNativeRumpShadow(memory,documents,objects,tasks,scheduler,queues,crane_config,owner,group);
        const auto model=objects.Create(crane_config,owner,group);check(model && objects.Frame(model));
        const auto visibility=objects.ResolveFacet(model,"ShowobjTask");
        VerifyNativeEffectRotation(memory,documents,objects,tasks,scheduler,model);
        VerifyNativeEffectRevolution(memory,documents,objects,tasks,scheduler,model);
        VerifyNativeEffectTrack(memory,documents,objects,tasks,scheduler,model);
        support.SetRadius(1200.0f);objects.Show(visibility);
        const auto position=objects.ResolveFacet(model,"PositionTask");
        const auto initial=objects.ReadPosition(position);
        const auto aim_scale=objects.ResolveFacet(model,"ScaleTask");
        std::array<float,3> saved_aim_scale{};objects.ReadScale(aim_scale,saved_aim_scale);
        objects.SetScale(aim_scale,{2,3,4},0u);
        const auto aim=objects.ResolveFacet(model,"MoveToTargetTask");check(aim==model+56u);
        objects.Face(aim,{initial[0]+10.0f,initial[1],initial[2]});
        objects.SetFaceRotation(aim,{0,0,0});objects.ActivateFace(aim);
        const auto* aimed=RwFrameGetMatrix(reinterpret_cast<RwFrame*>(objects.Frame(model)));
        check(aimed->at.x>3.99f && std::abs(aimed->at.y)<0.01f && std::abs(aimed->at.z)<0.01f &&
            std::abs(RwV3dLength(&aimed->right)-2.0f)<0.01f &&
            std::abs(RwV3dLength(&aimed->up)-3.0f)<0.01f &&
            std::abs(aimed->pos.x-initial[0])<0.001f && std::abs(aimed->pos.y-initial[1])<0.001f &&
            std::abs(aimed->pos.z-initial[2])<0.001f && memory.ReadByte(model+104u)==1u);
        objects.SetFaceRotation(aim,{0,90,0});objects.ActivateFace(aim);
        const auto* offset_aim=RwFrameGetMatrix(reinterpret_cast<RwFrame*>(objects.Frame(model)));
        check(std::isfinite(offset_aim->at.x) && std::abs(offset_aim->at.x)<0.01f &&
            std::abs(offset_aim->at.z)>3.99f);
        objects.SetScale(aim_scale,saved_aim_scale,0u);
        const std::array<float,3> target{initial[0]+10.0f,initial[1]+4.0f,initial[2]-6.0f};
        std::array<std::uint32_t,15> position_xml{};
        const auto node=reinterpret_cast<std::uint32_t>(position_xml.data());
        (void)documents.InitializeDocument(node,"clump-position-task",0u);
        BinaryXmlNode definition{"Task",{
            {"Attr_Class_Name",3u,std::string("PositionTask")},
            {"Attr_Task_Position_Result",1u,std::int32_t(1)},
            {"Attr_Task_Position_Face_Target",1u,std::int32_t(1)},
            {"Attr_Task_Position_x",2u,target[0]},
            {"Attr_Task_Position_y",2u,target[1]},
            {"Attr_Task_Position_z",2u,target[2]}}, {}};
        const auto serialized=BinaryXmlConfig::Serialize(definition);
        check(serialized && documents.LoadBinaryDocument(node,*serialized));
        const auto motion=tasks.CreateTask(node);check(motion);
        documents.DestroyDocument(node);
        memory.WriteDword(motion+64u,model);
        GomTaskContainerState container{};
        const auto container_address=reinterpret_cast<std::uint32_t>(&container);
        scheduler.InitializeTaskContainer(container_address);
        scheduler.AddTask(container_address,motion,"crane-position",{2u,0.0f,0.5f});
        (void)scheduler.BeginWaiting(container_address);
        bool intermediate{},finished{};
        for(unsigned tick=0;tick<30u && container.count;++tick) {
            (void)scheduler.UpdateTaskContainer(container_address,0.05f);
            const auto value=objects.ReadPosition(position);
            intermediate|=value[0]>initial[0]+0.01f && value[0]<target[0]-0.01f;
            auto* frame=reinterpret_cast<RwFrame*>(objects.Frame(model));
            const auto* transform=RwFrameGetMatrix(frame);
            check(std::isfinite(transform->at.x) && std::isfinite(transform->at.y) &&
                std::isfinite(transform->at.z));
            check(std::abs(transform->pos.x-value[0])<0.001f &&
                  std::abs(transform->pos.y-value[1])<0.001f &&
                  std::abs(transform->pos.z-value[2])<0.001f);
            if(container.count && memory.ReadDword(motion+40u)==4u) {
                check(!memory.ReadDword(motion+152u) && !memory.ReadDword(motion+160u));
                finished=true;
            }
        }
        check(intermediate && finished && !container.count && !tasks.LiveTasks());
        const auto final_position=objects.ReadPosition(position);
        for(unsigned i=0;i<3u;++i) check(std::abs(final_position[i]-target[i])<0.001f);
        (void)scheduler.DestroyTaskContainer(container_address);
        objects.Move(position,initial,false);
        const auto model_scale=objects.ResolveFacet(model,"ScaleTask");
        std::array<float,3> original_scale{},scaled{};
        objects.ReadScale(model_scale,original_scale);
        objects.SetScale(model_scale,{2,3,4},0u);objects.ReadScale(model_scale,scaled);
        check(scaled==std::array<float,3>{2,3,4});
        objects.SetScale(model_scale,{-3,1,-5},1u);objects.ReadScale(model_scale,scaled);
        check(scaled==std::array<float,3>{0,4,0});
        objects.SetScale(model_scale,original_scale,0u);
        queues.QueueEffect(group,model);objects.PreparePlayback(model);
        objects.Advance(model,0.25);objects.SetMode(model,1u);objects.Advance(model,0.25);
        objects.ReleasePlayback(model,queues);objects.ReleasePlayback(model,queues);objects.Destroy(model);
        check(!memory.ReadDword(memory.ReadDword(system+144u)+24u) &&
            !memory.ReadDword(memory.ReadDword(system+144u)+40u));
        support.CleanupAuxiliary();
        const auto complete_model=objects.Create(config,owner,group);
        check(complete_model!=0u);
        std::cerr<<"Original M_011 created: objects="<<objects.LiveObjects()-object_count
            <<" tasks="<<tasks.LiveTasks()<<std::endl;
        const auto model_dialog=memory.ReadDword(complete_model+92u);
        const auto find_model=[&](auto&& self,std::uint32_t effect)->std::uint32_t {
            const auto type=objects.ObjectTypeName(effect);
            if(type=="ClumpObject") return effect;
            if(type!="EffectObject") return 0u;
            const auto dialog=memory.ReadDword(effect+92u);
            for(auto record=memory.ReadDword(dialog+112u);record!=memory.ReadDword(dialog+116u);record+=20u)
                if(const auto found=self(self,memory.ReadDword(record+16u))) return found;
            return 0u;
        };
        const auto first_model=find_model(find_model,complete_model);
        check(first_model && objects.ObjectTypeName(first_model)=="ClumpObject");
        const auto first_position=objects.ResolveFacet(first_model,"PositionTask");
        const auto initial_model_position=objects.ReadPosition(first_position);
        queues.QueueEffect(group,complete_model);
        objects.StartNestedEffect(complete_model,0u);
        bool original_moved{},original_ended{};
        try {
            for(unsigned tick=0;tick<1200u;++tick) {
                frames.Update(0.05);
                if(objects.ReadPosition(first_position)!=initial_model_position) original_moved=true;
                if(!memory.ReadDword(model_dialog+8u)) {original_ended=true;break;}
            }
        } catch(const std::exception& error) {
            std::cerr<<"Original M_011 playback: "<<error.what()<<std::endl;
            objects.Destroy(complete_model);frames.Update(0);throw;
        }
        std::cerr<<"Original M_011 playback: moved="<<original_moved<<" ended="<<original_ended<<std::endl;
        objects.Destroy(complete_model);
        frames.Update(0);
        check(original_moved);
        check(loader.LiveDialogs()==dialog_count && objects.LiveObjects()==object_count &&
            !tasks.LiveTasks() && memory.ReadDword(system+164u)==instances);
        objects.UnbindModelResources();
        support.CleanupAuxiliary();
    }
    check(memory.OutstandingAllocations()==model_baseline);
    const auto failure_baseline=memory.OutstandingAllocations();
    bool model_boundary{};
    try{(void)objects.Create(config,owner,group);}
    catch(const std::logic_error& error) {
        model_boundary=std::string(error.what())=="native ClumpObject model resource path is not bound";
    }
    check(model_boundary && loader.LiveDialogs()==dialog_count && objects.LiveObjects()==object_count &&
        !tasks.LiveTasks() && memory.ReadDword(system+164u)==instances &&
        memory.OutstandingAllocations()==failure_baseline);
    wrapper.children.clear();
    const auto missing=BinaryXmlConfig::Serialize(wrapper);
    check(missing && documents.LoadBinaryDocument(root,*missing));
    const auto missing_baseline=memory.OutstandingAllocations();
    check(!objects.Create(root,owner,group) && loader.LiveDialogs()==dialog_count &&
        objects.LiveObjects()==object_count && memory.ReadDword(system+164u)==instances &&
        memory.OutstandingAllocations()==missing_baseline);
    documents.DestroyDocument(root);
    check(documents.DocumentCount()==doc_count && memory.OutstandingAllocations()==baseline);
    std::cout<<"Native nested H_077: actual cylinder+2 particles,10 tasks,nested SDK frames,scale,"
        "scheduled PlayEffectTask,natural completion,replay,loop restart,measure,active release; "<<pixels<<" GPU pixels; original M_011 nested movement/active destruction and unbound-resource rollback verified\n";
}
} // namespace pal4::test
