#include "native_effect_track_probe.h"
#pragma once
#include "pal4/native_particle_effect_factory.h"
#include "pal4/native_particle_configuration.h"
#include "pal4/native_effect_texture_dictionary.h"
#include "pal4/native_effect_task_factory.h"
#include "pal4/native_dialog_object.h"
#include "pal4/native_dialog_task_runtime.h"
#include "pal4/native_dialog_particle_effects.h"
#include "pal4/native_effect_text_manager.h"
#include "pal4/native_script_object_runtime.h"
#include "pal4/native_base_object_memory.h"
#include "pal4/native_event_document_runtime.h"
#include "pal4/native_package_archive.h"
#include "pal4/particle_visual_object.h"
#include "pal4/native_particle_group_frames.h"
#include "pal4/native_model_material_runtime.h"
#include "pal4/effect_system_render_state.h"
#include "pal4/effect_system_object_creator.h"
#include "native_particle_probe_camera.h"
#include "native_visual_model_probe.h"
#include "native_effect_rotation_probe.h"
#include "native_effect_revolution_probe.h"
#include "native_effect_message_probe.h"
#include "native_rump_shadow_probe.h"
#include "native_sequence_anim_probe.h"
#include "native_thunder_probe.h"
#include <rwcore.h>
#include <array>
#include <bit>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <limits>
#include <source_location>
#include <stdexcept>

namespace pal4::test {
inline void VerifyNativeModelMaterialColor() {
    const auto address=[](const void* p){return reinterpret_cast<std::uint32_t>(p);};
    auto* clump=RpClumpCreate();
    auto* geometry=RpGeometryCreate(3,1,rpGEOMETRYPOSITIONS|rpGEOMETRYPRELIT);
    auto* material=RpMaterialCreate();
    auto* atomic=RpAtomicCreate();
    if(!clump || !geometry || !material || !atomic) {
        if(atomic) RpAtomicDestroy(atomic);
        if(material) RpMaterialDestroy(material);
        if(geometry) RpGeometryDestroy(geometry);
        if(clump) RpClumpDestroy(clump);
        throw std::runtime_error("native material fixture allocation");
    }
    auto* triangle=RpGeometryGetTriangles(geometry);
    (void)RpGeometryTriangleSetVertexIndices(geometry,triangle,0,1,2);
    (void)RpGeometryTriangleSetMaterial(geometry,triangle,material);
    (void)RpGeometryUnlock(geometry);
    (void)RpAtomicSetGeometry(atomic,geometry,0);
    (void)RpClumpAddAtomic(clump,atomic);
    NativeModelMaterialRuntime runtime;
    const auto result=GameWorldMaterialOperations::SetClumpFlags(
        address(clump),0x7f563412u,runtime);
    const auto color=*RpMaterialGetColor(material);
    bool valid=result==address(clump) && color.red==0x12 &&
        color.green==0x34 && color.blue==0x56 && color.alpha==0x7f &&
        (RpGeometryGetFlags(geometry)&rpGEOMETRYMODULATEMATERIALCOLOR)!=0;
    // Repeat writes must preserve unrelated geometry flags and all four bytes.
    const auto flags=RpGeometryGetFlags(geometry);
    (void)GameWorldMaterialOperations::SetClumpFlags(address(clump),0xfeffffffu,runtime);
    valid=valid && RpGeometryGetFlags(geometry)==flags &&
        RpMaterialGetColor(material)->alpha==0xfe;
    (void)RpGeometrySetFlags(geometry,flags & ~rpGEOMETRYPRELIT &
                                                ~rpGEOMETRYMODULATEMATERIALCOLOR);
    (void)GameWorldMaterialOperations::SetClumpFlags(address(clump),0u,runtime);
    valid=valid && (RpGeometryGetFlags(geometry)&rpGEOMETRYMODULATEMATERIALCOLOR)==0 &&
        RpMaterialGetColor(material)->alpha==0;
    (void)RpClumpDestroy(clump);
    (void)RpGeometryDestroy(geometry);
    (void)RpMaterialDestroy(material);
    if(!valid) throw std::runtime_error("native model RGBA/material modulation");
}
inline void VerifyNativeParticleEffectFactory(const std::filesystem::path& game_root) {
    VerifyNativeModelMaterialColor();
    VerifyNativeVisualModel();
    const auto check=[](bool ok,const std::source_location at=std::source_location::current()) {
        if(!ok) {
            std::cerr << "FAIL: native particle effect factory line " << at.line() << std::endl;
            throw std::runtime_error("native particle effect factory line "+std::to_string(at.line()));
        }
    };
    const auto address=[](const void* p){return reinterpret_cast<std::uint32_t>(p);};
    NativeBaseObjectMemory memory;
    NativeEventDocumentRuntime documents(memory);
    NativeScriptObjectRuntime tasks(memory);
    std::array<std::uint32_t,15> xml{};
    const auto root=address(xml.data());(void)documents.InitializeDocument(root,"factory",0u);
    for(const auto& [value,expected]:std::array<std::pair<float,std::int32_t>,5>{{
        {360.0f,360},{-2.75f,-2},{4294967296.0f,0},
        {std::numeric_limits<float>::infinity(),0},{std::numeric_limits<float>::quiet_NaN(),0}}}) {
        documents.WriteFloatAttribute(root,"float-to-int",value);
        check(documents.AttributeToInt(documents.FindNumericAttribute(root,"float-to-int"),7)==expected);
    }
    {
        // Only these fixture status/counter/context fields are supplied by the
        // test. All object/resource/task lifecycles and updates are native.
        struct Status final : INativeEffectTaskStatus {
            bool EffectsEnabled() override{return true;}
            bool EffectIsActive(std::uint32_t) override{return false;}
        } status;
        std::array<std::uint32_t,42> system{};
        std::array<std::uint32_t,53> context{};context[1]=1u;context[12]=0x12345678u;
        NativeEffectTextureDictionary textures(game_root);
        NativeParticleEffectFactory objects(memory,documents,textures,address(system.data()),status);
        NativeEffectObjectQueues queues(memory,address(context.data()),objects);
        struct GeometryQueue final : IEffectSystemRenderQueueRuntime {
            NativeBaseObjectMemory& memory;
            explicit GeometryQueue(NativeBaseObjectMemory& m):memory(m){}
            std::uint8_t ReadByte(std::uint32_t a) override {return memory.ReadByte(a);}
            std::uint32_t ReadWord(std::uint32_t a) override {return memory.ReadDword(a);}
            void SortAtomicGeometries(std::uint32_t) override {throw std::logic_error("particle probe has no clump geometry queue");}
        } geometry(memory);
        std::array<std::uint32_t,3> gate{};gate[1]=1u;gate[2]=std::bit_cast<std::uint32_t>(10000.0f);
        NativeParticleGroupFrames frames(memory,objects,address(gate.data()),geometry);
        NativeEffectTaskFactory factory(memory,documents,tasks,objects);
        NativeDialogTaskRuntime scheduler(memory,documents,tasks,factory);
        NativeDialogObject dialog(memory,documents);
        NativeDialogParticleEffects activation(memory,dialog.Templates(),objects,queues);
        NativePackageArchive archive;check(archive.Open((game_root/"gamedata/Effect.cpk").string().c_str()));
        const auto data=archive.ReadMember("script\\foot_2.eft");
        check(data && documents.LoadBinaryDocument(root,*data) && dialog.Load(root));
        documents.CleanupDocument(root);
        const auto config=dialog.Templates().Find("lz.1");check(config);
        check(!objects.Create(0u,0u,0u));
        const auto baseline=memory.OutstandingAllocations();
        auto& state=dialog.State();state.words[3]=address(context.data());
        auto* dialog_frame=RwFrameCreate();check(dialog_frame);state.words[43]=address(dialog_frame);
        check(activation.Activate(state,dialog.Address()));
        const auto object=memory.ReadDword(state.words[28]+16u);
        check(object && objects.LiveObjects()==1u && system[41]==1u);
        check(queues.Count(176u)==1u && queues.Contains(176u,object) && RwFrameGetParent(reinterpret_cast<RwFrame*>(objects.Frame(object)))==dialog_frame);
        check(memory.ReadDword(object+12u)==dialog.Address() && memory.ReadDword(object+16u)==address(context.data()));
        auto& payload=objects.Configuration(object);
        VerifyNativeEffectMessage(memory,documents,objects,factory,tasks,object,dialog.Address());
        auto& particle=*reinterpret_cast<ParticleVisualState*>(payload.State().words[23]);
        check(objects.ResolveFacet(object,"ShowobjTask")==object+20u &&
            objects.ResolveFacet(object,"DecressTask")==object+48u && !objects.ResolveFacet(object,"unknown"));
        // Original particle color facet uses the existing shared properties,
        // including total-color mode and disabling animated color on write.
        const auto color_facet=objects.ResolveFacet(object,"ChangeColorTask");
        check(color_facet==object+44u);
        const auto resource=payload.State().words[23];
        const auto saved_mode=memory.ReadByte(resource+3u);
        const auto saved_animation=memory.ReadByte(resource+316u);
        std::array<std::uint32_t,4> saved_color{};
        for(unsigned i=0;i<4u;++i) saved_color[i]=memory.ReadDword(resource+212u+i*4u);
        for(const auto mode:{0u,1u}) {
            objects.SetColorFacetEnabled(color_facet,static_cast<std::uint8_t>(mode));
            check(memory.ReadByte(resource+3u)==mode);
            memory.WriteByte(resource+316u,1u);
            objects.SetColor(color_facet,{11u,22u,33u,128u});
            std::array<std::uint8_t,4> color{};objects.ReadColor(color_facet,color);
            check(color==std::array<std::uint8_t,4>{11u,22u,33u,128u} &&
                  memory.ReadByte(resource+316u)==0u);
        }
        memory.WriteByte(resource+3u,saved_mode);
        memory.WriteByte(resource+316u,saved_animation);
        for(unsigned i=0;i<4u;++i) memory.WriteDword(resource+212u+i*4u,saved_color[i]);
        check(!objects.Advance(object,0.05) && !particle.ReadWord(204u));
        const auto scale_facet=objects.ResolveFacet(object,"ScaleTask");
        const auto position_facet=objects.ResolveFacet(object,"PositionTask");
        const auto saved_position=objects.ReadPosition(position_facet);
        // Original614360 returns position+112 despite being the rotation getter.
        std::array<float,3> getter_rotation{};
        objects.ReadRotation(objects.ResolveFacet(object,"RotateTask"),getter_rotation);
        check(getter_rotation==saved_position);
        objects.Move(position_facet,{0,0,0},false);
        VerifyNativeEffectRotation(memory,documents,objects,factory,tasks,object,[&] {
            return std::array<float,3>{std::bit_cast<float>(particle.ReadWord(88u)),
                std::bit_cast<float>(particle.ReadWord(92u)),std::bit_cast<float>(particle.ReadWord(96u))};
        });
        VerifyNativeEffectRevolution(memory,documents,objects,factory,tasks,object);
        VerifyNativeEffectTrack(memory,documents,objects,factory,tasks,object);
        objects.Move(position_facet,saved_position,false);
        std::array<float,3> scale{};objects.ReadScale(scale_facet,scale);
        check(scale==std::array<float,3>{1.0f,1.0f,1.0f});
        objects.SetScale(scale_facet,{2.0f,3.0f,4.0f},0u);
        const auto* frame=reinterpret_cast<RwFrame*>(particle.ReadWord(44u));
        check(std::abs(RwV3dLength(&RwFrameGetMatrix(frame)->right)-2.0f)<0.00001f);
        objects.SetScale(scale_facet,{1.0f,1.0f,1.0f},0u);
        GomTaskContainerState container;
        const auto manager=address(&container);tasks.InitializeTaskContainer(manager);state.words[2]=manager;
        check(DialogObject::InitializeTasks(state,dialog.Address(),scheduler) && factory.LiveTasks()==3u);
        (void)tasks.BeginWaiting(manager);
        for(unsigned frame_number=1u;frame_number<=50u;++frame_number) {
            (void)tasks.UpdateTaskContainer(manager,0.05f);
            frames.Update(address(context.data()),0.05);
            if(frame_number==10u) {
                check(memory.ReadByte(object+6u) && particle.ReadWord(208u)>0u);
                const auto first=memory.ReadDword(particle.ReadWord(160u));
                const auto age=std::bit_cast<float>(memory.ReadDword(first+4u));
                payload.State().words[22]=std::bit_cast<std::uint32_t>(0.1f);
                (void)payload.PreRender(); // Correct own resource, arbitrary context argument.
                const auto after=std::bit_cast<float>(memory.ReadDword(first+4u));
                check(std::abs(after-age-0.1f)<0.00002f);
                payload.State().words[22]=0u;
                // Native list traversal -> owner draw gates -> shared proximity
                // -> SDK PTank callback, with actual image evidence.
                NativeParticleProbeCamera camera;
                std::array<std::uint32_t,2> source{};source[1]=address(camera.frame);
                memory.WriteByte(dialog.Address()+83u,1u);
                camera.Begin();frames.Draw(address(context.data()),address(source.data()));
                check(camera.EndPixels()>0u);
                camera.Begin();check(!camera.EndPixels());
                const RwV3d far_position{10000.0f,0.0f,0.0f};
                check(RwFrameTranslate(dialog_frame,&far_position,rwCOMBINEREPLACE));
                camera.Begin();
                frames.Draw(address(context.data()),address(source.data()));
                check(!camera.EndPixels());
                const RwV3d origin{};check(RwFrameTranslate(dialog_frame,&origin,rwCOMBINEREPLACE));
                memory.WriteByte(dialog.Address()+83u,0u);
                context[1]=0u;
                const auto frozen=particle.bytes;
                frames.Update(address(context.data()),0.1);check(frozen==particle.bytes);
                camera.Begin();frames.Draw(address(context.data()),0u);check(!camera.EndPixels());
                context[1]=1u;
            }
            if(frame_number==20u) check(!particle.ReadByte(1u));
        }
        check(!memory.ReadByte(object+6u) && !particle.ReadWord(208u) &&
            !factory.LiveTasks() && !tasks.RegisteredTasks() && !container.count && container.ready);
        const auto before=particle.bytes;
        frames.Update(address(context.data()),1.0);check(before==particle.bytes);
        frames.Draw(address(context.data()),0u); // Hidden owner needs no camera.
        (void)tasks.DestroyTaskContainer(manager);state.words[2]=0u;
        (void)DialogObject::ResetTaskSlots(state,scheduler);
        activation.Stop(state);activation.Stop(state);
        unsigned child_count{};
        (void)RwFrameForAllChildren(dialog_frame,[](RwFrame* f,void* p)->RwFrame* {++*static_cast<unsigned*>(p);return f;},&child_count);
        check(!memory.ReadDword(state.words[28]+16u) && !queues.Count(176u) && !child_count);
        check(RwFrameDestroy(dialog_frame));state.words[43]=0u;
        check(!objects.LiveObjects() && !system[41] && memory.OutstandingAllocations()==baseline);
        documents.WriteIntAttribute(config,"Attr_Prt_Is_Billboard",1);
        const auto collection=objects.Create(config,0u,address(context.data()));
        check(collection && objects.LiveObjects()==1u && system[41]==1u);
        objects.Destroy(collection);
        check(!objects.LiveObjects() && !system[41] && memory.OutstandingAllocations()==baseline);
        documents.WriteIntAttribute(config,"Attr_Prt_Is_Billboard",0);
        documents.WriteStringAttribute(config,"Attr_Class_Name","ClumpObject");
        bool rejected=false;try{(void)objects.Create(config,0u,address(context.data()));}catch(const std::logic_error&){rejected=true;}
        check(rejected && !objects.LiveObjects() && !system[41]);
        // IDA609C10: disabled optional effects + Attr_Dumy creates a genuine
        // dormant ClumpObject, with byte+5 set and no model/animation resources.
        documents.WriteIntAttribute(config,"Attr_Dumy",1);
        const auto dummy_baseline=memory.OutstandingAllocations();
        const auto dummy=objects.Create(config,0u,address(context.data()));
        check(dummy && system[41]==1u && objects.ObjectTypeName(dummy)=="ClumpObject" &&
            !memory.ReadByte(dummy+4u) && memory.ReadByte(dummy+5u)==1u && !objects.Frame(dummy));
        objects.SetMode(dummy,1u);
        check(!memory.ReadByte(dummy+11u) && !objects.ResetPlayback(dummy));
        objects.Show(objects.ResolveFacet(dummy,"ShowobjTask"));
        objects.Hide(objects.ResolveFacet(dummy,"HideobjTask"));
        check(objects.Duration(dummy)==static_cast<double>(0.8f));
        objects.Destroy(dummy);
        check(!system[41] && !objects.LiveObjects() && memory.OutstandingAllocations()==dummy_baseline);
        memory.WriteByte(address(system.data())+1u,1u);
        rejected=false;try{(void)objects.Create(config,0u,address(context.data()));}catch(const std::logic_error&){rejected=true;}
        check(rejected && !system[41] && !objects.LiveObjects() && memory.OutstandingAllocations()==dummy_baseline);
        memory.WriteByte(address(system.data())+1u,0u);
        documents.WriteIntAttribute(config,"Attr_Dumy",0);
        documents.WriteStringAttribute(config,"Attr_Class_Name","unknown");
        check(!objects.Create(config,0u,0u));
        documents.WriteStringAttribute(config,"Attr_Class_Name","RtkPrtStdAtomicObject");
        const auto recreated=objects.Create(config,0u,address(context.data()));check(recreated && system[41]==1u);
        // Native classification uses real +8/+9 bytes (IDA 606B90/606BA0),
        // including fallback precedence and the original disabled-group gate.
        check(objects.ObjectTypeName(recreated)=="RtkPrtStdAtomicObject" &&
            !objects.UseFallbackQueue(recreated) && !objects.UseCameraQueue(recreated));
        memory.WriteByte(recreated+9u,1u);
        queues.QueueEffect(address(context.data()),recreated);
        check(queues.Contains(152u,recreated) && !queues.Count(176u));
        queues.RemoveEffect(address(context.data()),recreated);check(!queues.Count(152u));
        memory.WriteByte(recreated+8u,1u);
        queues.QueueEffect(address(context.data()),recreated);
        check(queues.Contains(200u,recreated) && !queues.Count(152u));
        queues.RemoveEffect(address(context.data()),recreated);check(!queues.Count(200u));
        memory.WriteByte(recreated+8u,0u);memory.WriteByte(recreated+9u,0u);
        queues.QueueEffect(address(context.data()),recreated);
        queues.QueueEffect(address(context.data()),recreated);
        check(queues.Count(176u)==2u);
        queues.RemoveEffect(address(context.data()),recreated);check(queues.Count(176u)==1u);
        context[1]=0u;
        queues.QueueEffect(address(context.data()),recreated);
        queues.RemoveEffect(address(context.data()),recreated);check(queues.Count(176u)==1u);
        context[1]=1u;
        queues.RemoveEffect(address(context.data()),recreated);
        queues.RemoveEffect(address(context.data()),recreated);check(!queues.Count(176u));
        rejected=false;try{queues.QueueEffect(0u,recreated);}catch(const std::invalid_argument&){rejected=true;}
        check(rejected);
        rejected=false;
        const auto before_duplicate=memory.OutstandingAllocations();
        try{NativeEffectObjectQueues duplicate(memory,address(context.data()),objects);}
        catch(const std::logic_error&){rejected=true;}
        check(rejected && memory.OutstandingAllocations()==before_duplicate);
        objects.Destroy(recreated);check(!system[41]);
        {
            NativeParticleEffectFactory scoped(memory,documents,textures,address(system.data()),status);
            check(scoped.Create(config,0u,address(context.data())) && system[41]==1u);
        }
        check(!system[41]);
        {
            NativeDialogObject original(memory,documents);
            NativeDialogParticleEffects original_activation(memory,original.Templates(),objects,queues);
            const auto h077=archive.ReadMember("script\\H_077.eft");
            check(h077 && documents.LoadBinaryDocument(root,*h077) && original.Load(root));
            documents.CleanupDocument(root);
            const auto original_baseline=memory.OutstandingAllocations();
            auto& original_state=original.State();original_state.words[3]=address(context.data());
            auto* parent=RwFrameCreate();check(parent);original_state.words[43]=address(parent);
            check(original_activation.Activate(original_state,original.Address()));
            check(objects.LiveObjects()==3 && objects.LiveParticleObjects()==2 && system[41]==3 && queues.Count(140)==1);
            std::uint32_t cylinder{};
            for(auto p=original_state.words[28];p!=original_state.words[29];p+=20) {
                const auto value=memory.ReadDword(p+16);
                if(objects.ObjectTypeName(value)=="CylinderObject") cylinder=value;
            }
            check(cylinder && RwFrameGetParent(reinterpret_cast<RwFrame*>(objects.Frame(cylinder)))==parent);
            VerifyNativeEffectRotation(memory,documents,objects,factory,tasks,cylinder);
            VerifyNativeEffectRevolution(memory,documents,objects,factory,tasks,cylinder);
            const auto color_facet=objects.ResolveFacet(cylinder,"ChangeColorTask");
            std::array<std::uint8_t,4> color{};objects.ReadColor(color_facet,color);
            check(color==std::array<std::uint8_t,4>{251,238,0,254});
            const auto scale_facet=objects.ResolveFacet(cylinder,"ScaleTask");
            std::array<float,3> original_scale{};objects.ReadScale(scale_facet,original_scale);
            check(original_scale==std::array<float,3>{0.2f,0.2f,0.2f});
            GomTaskContainerState original_container{};const auto original_manager=address(&original_container);
            tasks.InitializeTaskContainer(original_manager);original_state.words[2]=original_manager;
            check(DialogObject::InitializeTasks(original_state,original.Address(),scheduler) && factory.LiveTasks()==10);
            (void)tasks.BeginWaiting(original_manager);
            bool faded{},scaled{};unsigned pixels{};
            for(unsigned i=0;i<160;++i) {
                (void)tasks.UpdateTaskContainer(original_manager,0.05f);
                frames.Update(address(context.data()),0.05);
                objects.ReadColor(color_facet,color);
                check(color[0]==251 && color[1]==238 && color[2]==0);
                faded=faded||(color[3]>0 && color[3]<254);
                std::array<float,3> scale{};objects.ReadScale(scale_facet,scale);scaled=scaled||scale[0]>original_scale[0];
                if(i==4) {
                    NativeParticleProbeCamera camera;camera.Begin();
                    std::array<std::uint32_t,2> source{};source[1]=address(camera.frame);
                    frames.Draw(address(context.data()),address(source.data()));pixels=camera.EndPixels();
                }
            }
            check(faded && scaled && pixels && color[3]==0 && !memory.ReadByte(cylinder+7) &&
                !factory.LiveTasks() && !tasks.RegisteredTasks() && !original_container.count);
            check(objects.ResetPlayback(cylinder));
            objects.Show(objects.ResolveFacet(cylinder,"ShowobjTask"));
            objects.SetMode(cylinder,1);(void)objects.Advance(cylinder,0.4);
            objects.ReadColor(color_facet,color);
            check(color[0]==251 && color[1]==238 && color[2]==0 && color[3]>=120 && color[3]<=130);
            (void)objects.Advance(cylinder,1.0);objects.ReadColor(color_facet,color);check(color[3]==0);
            objects.Hide(objects.ResolveFacet(cylinder,"HideobjTask"));
            (void)tasks.DestroyTaskContainer(original_manager);original_state.words[2]=0;
            (void)DialogObject::ResetTaskSlots(original_state,scheduler);
            objects.ReleasePlayback(cylinder,queues);
            check(!objects.Frame(cylinder) && !queues.Count(140) && system[41]==3);
            original_activation.Stop(original_state);original_activation.Stop(original_state);
            check(!objects.LiveObjects() && !system[41] && !queues.Count(140) && !queues.Count(176));
            check(RwFrameDestroy(parent));original_state.words[43]=0;
            check(memory.OutstandingAllocations()==original_baseline);
            const auto cylinder_config=original.Templates().Find("yzt3");check(cylinder_config);
            VerifyNativeRumpShadow(memory,documents,objects,factory,tasks,queues,cylinder_config,original.Address(),address(context.data()));
            documents.WriteIntAttribute(cylinder_config,"Attr_Cylinder_BorderNumb",2);
            bool bad_cylinder{};
            try {(void)objects.Create(cylinder_config,original.Address(),address(context.data()));}
            catch(const std::runtime_error&) {bad_cylinder=true;}
            check(bad_cylinder && !objects.LiveObjects() && !system[41] &&
                  memory.OutstandingAllocations()==original_baseline);
            std::cout << "Original H_077: real textured cylinder +2 particles,10 scheduled tasks,scale/alpha/hide; "
                      << pixels << " GPU pixels; timed fade and allocation balance verified\n";
        }
        VerifyNativeSequenceAnimFactory(memory,documents,objects,queues,frames,archive,root,system,context);
        VerifyNativeThunderFactory(memory,documents,objects,queues,frames,archive,textures,root,system,context);
        {
            // Exercise the production BillBord factory and group routing, not
            // only its visual primitive. Empty texture is the supported plain
            // material path; cold save loading separately supplies original CPK textures.
            documents.CleanupDocument(root);
            documents.WriteStringAttribute(root,EffectSystemObjectCreator::kClassAttribute,"BillBordObject");
            documents.WriteStringAttribute(root,"Attr_BillBord_Texture","");
            documents.WriteFloatAttribute(root,"Attr_BillBord_Width",32.0f);
            documents.WriteFloatAttribute(root,"Attr_BillBord_Height",32.0f);
            documents.WriteIntAttribute(root,"Attr_Render_Last",0);
            const auto before=memory.OutstandingAllocations();
            VerifyNativeRumpShadow(memory,documents,objects,factory,tasks,queues,root,0,address(context.data()));
            const auto billboard=objects.Create(root,0,address(context.data()));
            check(billboard && system[41]==1 && objects.LiveObjects()==1 &&
                objects.ObjectTypeName(billboard)=="BillBordObject");
            const auto show=objects.ResolveFacet(billboard,"ShowobjTask");
            const auto position=objects.ResolveFacet(billboard,"PositionTask");
            const auto scale=objects.ResolveFacet(billboard,"ScaleTask");
            const auto rotation=objects.ResolveFacet(billboard,"RotateTask");
            const auto tint=objects.ResolveFacet(billboard,"ChangeColorTask");
            check(show==billboard+20 && position==billboard+24 &&
                scale==billboard+40 && rotation==billboard+44 && tint==billboard+36);
            queues.QueueEffect(address(context.data()),billboard);
            check(queues.Count(116)==1 && queues.Contains(116,billboard));
            objects.Show(show);
            VerifyNativeEffectRevolution(memory,documents,objects,factory,tasks,billboard);
            objects.Move(position,{3,4,0},false);objects.Move(position,{-3,-4,0},true);
            check(objects.ReadPosition(position)==std::array<float,3>{0,0,0});
            objects.SetScale(scale,{2,3,0},0);objects.SetScale(scale,{1,1,0},1);
            std::array<float,3> values{};objects.ReadScale(scale,values);
            check(values==std::array<float,3>{3,4,0});
            objects.ApplyRotation(rotation,{15,0,0},false);
            objects.ApplyRotation(rotation,{5,0,0},true);objects.ReadRotation(rotation,values);
            check(values==std::array<float,3>{20,0,0});
            objects.SetColorFacetEnabled(tint,1);objects.SetColor(tint,{11,22,33,128});
            std::array<std::uint8_t,4> color{};objects.ReadColor(tint,color);
            check(color==std::array<std::uint8_t,4>{11,22,33,128});
            check(objects.ResetPlayback(billboard));objects.ReadScale(scale,values);
            check(values==std::array<float,3>{1,1,0});objects.ReadColor(tint,color);
            check(color==std::array<std::uint8_t,4>{255,255,255,254});
            NativeParticleProbeCamera camera;camera.Begin();
            const auto old_camera=context[12];context[12]=address(camera.camera);
            frames.Update(address(context.data()),0.05);
            frames.Draw(address(context.data()),address(camera.camera));
            const auto pixels=camera.EndPixels();check(pixels>0);
            objects.SetColor(tint,{11,22,33,200});objects.SetMode(billboard,1);
            frames.Update(address(context.data()),0.4);objects.ReadColor(tint,color);
            check(color[0]==11 && color[1]==22 && color[2]==33 && color[3]>=99 && color[3]<=100);
            frames.Update(address(context.data()),1.0);objects.ReadColor(tint,color);
            check(color==std::array<std::uint8_t,4>{11,22,33,0});
            objects.Hide(show);camera.Begin();
            frames.Draw(address(context.data()),address(camera.camera));check(!camera.EndPixels());
            objects.ReleasePlayback(billboard,queues);objects.Destroy(billboard);
            context[12]=old_camera;
            check(!objects.LiveObjects() && !system[41] && !queues.Count(116) &&
                memory.OutstandingAllocations()==before);
            documents.WriteStringAttribute(root,"Attr_BillBord_Texture","__missing_billboard_probe__");
            const auto failure_baseline=memory.OutstandingAllocations();
            bool rejected{};
            try {(void)objects.Create(root,0,address(context.data()));}
            catch(const std::runtime_error&) {rejected=true;}
            check(rejected && !objects.LiveObjects() && !system[41] &&
                memory.OutstandingAllocations()==failure_baseline);
            std::cout<<"Native BillBordObject: factory, real camera, group queue, transform/color/reset/hide, "
                <<pixels<<" GPU pixels, failed texture rollback and balanced teardown verified\n";
        }
        {
            const auto h077=archive.ReadMember("script\\H_077.eft");
            NativeDialogObject template_dialog(memory,documents);
            check(h077 && documents.LoadBinaryDocument(root,*h077) && template_dialog.Load(root));
            const auto source=template_dialog.Templates().Find("yzt3");check(source);
            const auto texture=documents.AttributeToString(documents.FindNumericAttribute(source,"Attr_Cylinder_Texture"));
            for(bool straight:{true,false}) {
                documents.CleanupDocument(root);
                documents.WriteStringAttribute(root,EffectSystemObjectCreator::kClassAttribute,"StripObject");
                documents.WriteStringAttribute(root,"Attr_Strip_Texture",texture);
                documents.WriteFloatAttribute(root,"Attr_Strip_Wdith",32);
                documents.WriteFloatAttribute(root,"Attr_Strip_Height",32);
                for(auto axis:{"x","y","z"})documents.WriteFloatAttribute(root,std::string("Attr_Strip_Scale_")+axis,1);
                for(auto channel:{"Red","Green","Blue","Alpha"})documents.WriteIntAttribute(root,std::string("Attr_Strip_Vertex_Col_")+channel,255);
                documents.WriteIntAttribute(root,"Attr_Render_Last",0);
                documents.WriteIntAttribute(root,"Attr_Strip_Cylinder_Creator",straight?0:1);
                documents.WriteIntAttribute(root,"Attr_Strip_Straight_Creator",straight?1:0);
                documents.WriteFloatAttribute(root,"Attr_Strip_Straight_Sequence",straight?1.0f:0.0f);
                const auto before=memory.OutstandingAllocations();
                VerifyNativeRumpShadow(memory,documents,objects,factory,tasks,queues,root,0,address(context.data()));
                const auto strip=objects.Create(root,0,address(context.data()));
                check(strip && system[41]==1 && objects.LiveObjects()==1 && objects.ObjectTypeName(strip)=="StripObject");
                const auto visual_address=memory.ReadDword(strip+(straight?152:148));
                const auto atomic_address=straight?memory.ReadDword(visual_address+12):memory.ReadDword(visual_address);
                auto* geometry=RpAtomicGetGeometry(reinterpret_cast<RpAtomic*>(atomic_address));
                check(geometry && geometry->refCount==1 && RpGeometryGetMaterial(geometry,0)->refCount==(straight?1:2));
                check(RpGeometryGetNumVertices(geometry)==(straight?4:20) && RpGeometryGetNumTriangles(geometry)==(straight?2:18));
                const auto show=objects.ResolveFacet(strip,"ShowobjTask");
                const auto pos=objects.ResolveFacet(strip,"PositionTask");
                const auto scale=objects.ResolveFacet(strip,"ScaleTask");
                const auto rotation=objects.ResolveFacet(strip,"RotateTask");
                const auto tint=objects.ResolveFacet(strip,"ChangeColorTask");
                check(show==strip+20 && pos==strip+32 && scale==strip+24 && rotation==strip+28 && tint==strip+44);
                queues.QueueEffect(address(context.data()),strip);check(queues.Contains(104,strip));
                objects.Show(show);VerifyNativeEffectRevolution(memory,documents,objects,factory,tasks,strip);objects.Move(pos,{3,4,0},false);objects.Move(pos,{-3,-4,0},true);
                check(objects.ReadPosition(pos)==std::array<float,3>{0,0,0});
                objects.SetScale(scale,{2,3,4},0);objects.SetScale(scale,{1,1,1},1);
                std::array<float,3> values{};objects.ReadScale(scale,values);check(values==std::array<float,3>{3,4,5});
                objects.ApplyRotation(rotation,{10,20,30},false);objects.ApplyRotation(rotation,{1,2,3},true);
                objects.ReadRotation(rotation,values);check(values==std::array<float,3>{11,22,33});
                objects.SetColor(tint,{11,22,33,200});check(objects.ResetPlayback(strip));
                std::array<std::uint8_t,4> color{};objects.ReadColor(tint,color);check(color==std::array<std::uint8_t,4>{255,255,255,254});
                auto* frame=reinterpret_cast<RwFrame*>(objects.Frame(strip));
                check(frame && RwFrameGetMatrix(frame)->right.x==1.0f);
                NativeParticleProbeCamera camera;
                // Strip cylinder geometry has normals + LIGHT (flags52), so
                // draw it in a genuine SDK world with an ambient light.
                struct Lighting {
                    RpWorld* world{};RpLight* light{};RpClump* clump{};RwCamera* camera{};
                    Lighting(RpClump* c,RwCamera* cam):clump(c),camera(cam) {
                        RwBBox bounds{{1000,1000,1000},{-1000,-1000,-1000}};
                        world=RpWorldCreate(&bounds);light=RpLightCreate(rpLIGHTAMBIENT);
                        if(!world || !light)throw std::runtime_error("strip lighting fixture allocation");
                        const RwRGBAReal white{1,1,1,1};(void)RpLightSetColor(light,&white);
                        (void)RpWorldAddLight(world,light);(void)RpWorldAddClump(world,clump);(void)RpWorldAddCamera(world,camera);
                    }
                    ~Lighting(){
                        (void)RpWorldRemoveCamera(world,camera);(void)RpWorldRemoveClump(world,clump);
                        (void)RpWorldRemoveLight(world,light);(void)RpLightDestroy(light);(void)RpWorldDestroy(world);
                    }
                };
                const auto old_camera=context[12];context[12]=address(camera.camera);
                const auto column=memory.ReadDword(strip+176);
                frames.Update(address(context.data()),0.05);
                if(straight)check(memory.ReadDword(strip+176)!=column);
                unsigned pixels{};
                {
                    Lighting lighting(reinterpret_cast<RpClump*>(memory.ReadDword(strip+136)),camera.camera);
                    camera.Begin();frames.Draw(address(context.data()),address(camera.camera));pixels=camera.EndPixels();
                }
                check(pixels>0);
                objects.SetColor(tint,{11,22,33,200});objects.SetMode(strip,1);
                frames.Update(address(context.data()),0.4);objects.ReadColor(tint,color);
                check(color[0]==11 && color[1]==22 && color[2]==33 && color[3]>=99 && color[3]<=100);
                frames.Update(address(context.data()),1.0);objects.ReadColor(tint,color);check(color==std::array<std::uint8_t,4>{11,22,33,0});
                objects.Hide(show);camera.Begin();frames.Draw(address(context.data()),address(camera.camera));check(!camera.EndPixels());
                objects.ReleasePlayback(strip,queues);objects.Destroy(strip);context[12]=old_camera;
                check(!objects.LiveObjects() && !system[41] && !queues.Count(104) && memory.OutstandingAllocations()==before);
                if(!straight) {
                    documents.WriteIntAttribute(root,"Attr_Strip_Cylinder_Border_Numb",1);
                    const auto invalid_before=memory.OutstandingAllocations();bool invalid{};
                    try{(void)objects.Create(root,0,address(context.data()));}catch(const std::runtime_error&){invalid=true;}
                    check(invalid && !objects.LiveObjects() && !system[41] && memory.OutstandingAllocations()==invalid_before);
                    documents.WriteIntAttribute(root,"Attr_Strip_Cylinder_Border_Numb",10);
                }
                documents.WriteStringAttribute(root,"Attr_Strip_Texture","__missing_strip_probe__");
                const auto fail_before=memory.OutstandingAllocations();bool rejected{};
                try{(void)objects.Create(root,0,address(context.data()));}catch(const std::exception&){rejected=true;}
                check(rejected && !objects.LiveObjects() && !system[41] && memory.OutstandingAllocations()==fail_before);
                std::cout<<"Native StripObject "<<(straight?"sequence plane":"cylinder mesh")<<": "<<pixels<<" GPU pixels; factory/facets/reset/timed alpha/cleanup verified\n";
            }
        }
        std::cout << "Native particle factory: original foot_2 tasks drive actual emission/expiry; "
                     "native group update/draw/proximity gates and pixel output; "
                     "500-step pre-render uses own resource; rollback/count/teardown verified\n";
    }
    documents.DestroyDocument(root);check(!documents.DocumentCount() && !memory.OutstandingAllocations());
}
}
