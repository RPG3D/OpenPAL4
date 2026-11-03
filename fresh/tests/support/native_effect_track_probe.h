#pragma once
#include "pal4/binary_xml_config.h"
#include "pal4/native_base_object_memory.h"
#include "pal4/native_event_document_runtime.h"
#include "pal4/native_particle_effect_factory.h"
#include "pal4/native_effect_task_factory.h"
#include "pal4/native_script_object_runtime.h"
#include <rwcore.h>
#include <rpspline.h>
#include <cmath>
#include <iostream>
#include <stdexcept>
namespace pal4::test {
inline void VerifyNativeEffectTrack(NativeBaseObjectMemory& memory,NativeEventDocumentRuntime& documents,
    NativeParticleEffectFactory& objects,NativeEffectTaskFactory& factory,NativeScriptObjectRuntime& scheduler,std::uint32_t object) {
    const auto check=[](bool ok){if(!ok)throw std::runtime_error("native TrackTask SDK/scheduler assertion");};
    const auto address=[](const void* p){return reinterpret_cast<std::uint32_t>(p);};
    const auto baseline=memory.OutstandingAllocations(),live=factory.LiveTasks(),registered=scheduler.RegisteredTasks();
    const auto position=objects.ResolveFacet(object,"PositionTask");const auto original=objects.ReadPosition(position);
    auto* parent=RwFrameCreate();auto* frame=RwFrameCreate();check(parent && frame);
    const RwV3d parent_pos{100,200,300},local{10,20,30};
    (void)RwFrameTranslate(parent,&parent_pos,rwCOMBINEREPLACE);(void)RwFrameAddChild(parent,frame);
    (void)RwFrameTranslate(frame,&local,rwCOMBINEREPLACE);
    std::array<std::uint32_t,44> dialog{};dialog[43]=address(frame);
    for(bool sample:{false,true}) {
        std::array<std::uint32_t,15> xml{};const auto root=address(xml.data());
        (void)documents.InitializeDocument(root,"native-track",0);
        BinaryXmlNode definition{"Task",{
            {"Attr_Class_Name",3u,std::string("TrackTask")},
            {"Attr_Task_Track_Sample",1u,std::int32_t(sample)},
            {"Attr_Task_Track_Type",1u,std::int32_t(1)},
            {"Attr_Task_Track_Time",2u,1.0f},
            {"Attr_Task_Track_Loop",1u,std::int32_t(0)},
            {"Attr_Task_Track_Autoface",1u,std::int32_t(1)}}, {}};
        for(unsigned i=0;i<4;++i)definition.children.push_back({"Attr_Task_Track_Position",{
            {"Attr_Task_Track_Position_x",2u,static_cast<float>(i)},
            {"Attr_Task_Track_Position_y",2u,0.0f},
            {"Attr_Task_Track_Position_z",2u,0.0f}}, {}});
        const auto bytes=BinaryXmlConfig::Serialize(definition);check(bytes && documents.LoadBinaryDocument(root,*bytes));
        const auto task=factory.CreateTask(root);check(task);documents.DestroyDocument(root);
        memory.WriteDword(task+64u,object);factory.BindDialogObject(task,address(dialog.data()));
        const auto count=(memory.ReadDword(task+100u)-memory.ReadDword(task+96u))/12u;
        check(count==(sample?20u:4u));
        auto* points=reinterpret_cast<const std::array<float,3>*>(memory.ReadDword(task+96u));
        float expected_start=0.0f;
        if(sample){RwV3d control[]{{0,0,0},{1,0,0},{2,0,0},{3,0,0}},v{};
            auto* spline=RpSplineCreate(4,rpSPLINETYPEOPENLOOPBSPLINE,control);check(spline);
            check(RpSplineFindPosition(spline,rpSPLINEPATHSMOOTH,0,&v,nullptr)!=nullptr);expected_start=v.x;
            (void)RpSplineDestroy(spline);}
        check(std::abs(points[0][0]-expected_start)<0.0001f);
        scheduler.BeginRun(task);
        const auto initial=objects.ReadPosition(position);
        check(std::abs(points[0][0]-(expected_start+10))<0.0001f &&
            std::abs(initial[0]-points[0][0])<0.0001f && initial[1]==20 && initial[2]==30);
        scheduler.Tick(task,0.1f);const auto moved=objects.ReadPosition(position);
        check(moved[0]>initial[0] && moved[1]==20 && moved[2]==30);
        scheduler.Finish(task);scheduler.DeleteTask(task,1u);
        check(factory.LiveTasks()==live && scheduler.RegisteredTasks()==registered && memory.OutstandingAllocations()==baseline);
    }
    objects.Move(position,original,false);(void)RwFrameRemoveChild(frame);(void)RwFrameDestroy(frame);(void)RwFrameDestroy(parent);
    std::cout<<"Native TrackTask "<<objects.ObjectTypeName(object)<<": public spline samples, local matrix with transformed parent, real facet motion/autoface, lifecycle and balanced cleanup verified\n";
}
}
