#pragma once
#include "pal4/binary_xml_config.h"
#include "pal4/native_base_object_memory.h"
#include "pal4/native_event_document_runtime.h"
#include "pal4/native_particle_effect_factory.h"
#include "pal4/native_effect_task_factory.h"
#include "pal4/native_script_object_runtime.h"
#include <rwcore.h>
#include <algorithm>
#include <bit>
#include <cmath>
#include <iostream>
#include <source_location>

namespace pal4::test {
// Uses production task creation/scheduler and real resource frame transforms.
inline void VerifyNativeEffectRevolution(NativeBaseObjectMemory& memory,
    NativeEventDocumentRuntime& documents,NativeParticleEffectFactory& objects,
    NativeEffectTaskFactory& factory,NativeScriptObjectRuntime& scheduler,std::uint32_t object) {
    const auto check=[](bool ok,const std::source_location at=std::source_location::current()) {
        if(!ok){std::cerr<<"native revolution probe line="<<at.line()<<std::endl;
            throw std::runtime_error("native effect revolution assertion");}
    };
    const auto allocations=memory.OutstandingAllocations(),live=factory.LiveTasks();
    const auto position=objects.ResolveFacet(object,"PositionTask");
    const auto revolute=objects.ResolveFacet(object,"RevoluteTask");check(position && revolute);
    const auto original=objects.ReadPosition(position);
    const auto actual=[&] {return RwFrameGetMatrix(reinterpret_cast<RwFrame*>(objects.Frame(object)))->pos;};
    objects.Move(position,{10,0,0},false);
    objects.ApplyRevolution(revolute,{0,0,0},{0,0,90,0,0,0});
    check(std::abs(actual().x)<0.001f && std::abs(actual().y-10)<0.001f);
    objects.ApplyRevolution(revolute,{0,0,0},{0,0,0,0,2,0});
    check(std::abs(actual().x)<0.001f && std::abs(actual().y-8)<0.001f);
    for(bool correct:{false,true}) {
        objects.Move(position,{10,0,0},false);
        std::array<std::uint32_t,15> xml{};const auto root=reinterpret_cast<std::uint32_t>(xml.data());
        (void)documents.InitializeDocument(root,"native-revolution",0);
        BinaryXmlNode definition{"Task",{
            {"Attr_Class_Name",3u,std::string("RevoluteTask")},
            {"Attr_Task_Revolute_Angle_z",2u,90.0f},
            {"Attr_Task_Revolute_Correct_Result",1u,std::int32_t(correct)},
            // These effect owners have no camera facet; optional lookup must
            // retain the configured origin rather than fabricate a target.
            {"Attr_Task_Revolute_Camera_Target",1u,std::int32_t(1)}}, {}};
        const auto bytes=BinaryXmlConfig::Serialize(definition);check(bytes && documents.LoadBinaryDocument(root,*bytes));
        const auto task=factory.CreateTask(root);check(task);documents.DestroyDocument(root);
        memory.WriteDword(task+64,object);
        GomTaskContainerState container{};const auto address=reinterpret_cast<std::uint32_t>(&container);
        scheduler.InitializeTaskContainer(address);scheduler.AddTask(address,task,"native-revolution",{2u,0.0f,0.5f});
        (void)scheduler.BeginWaiting(address);
        bool moved{},finished{};float elapsed{};
        for(unsigned i=0;i<40 && container.count;++i) {
            (void)scheduler.UpdateTaskContainer(address,0.07f);
            moved|=actual().y>0.1f && actual().y<6.0f;
            if(container.count) {
                elapsed=(std::max)(elapsed,std::bit_cast<float>(memory.ReadDword(task+32)));
                if(memory.ReadDword(task+40)==4) {
                    check(!memory.ReadDword(task+132) && !memory.ReadDword(task+128));finished=true;
                }
            }
        }
        const auto angle=(correct?0.5f:elapsed)*90.0f*0.01745329252f;
        if(!(moved && finished && !container.count && elapsed>0.5f &&
            std::abs(actual().x-10*std::cos(angle))<0.002f && std::abs(actual().y-10*std::sin(angle))<0.002f))
            std::cerr<<"Revolution result "<<objects.ObjectTypeName(object)<<" correct="<<correct<<" moved="<<moved<<" finished="<<finished<<" count="<<container.count<<" elapsed="<<elapsed<<" pos="<<actual().x<<","<<actual().y<<" expected="<<10*std::cos(angle)<<","<<10*std::sin(angle)<<std::endl;
        check(moved && finished && !container.count && elapsed>0.5f &&
            std::abs(actual().x-10*std::cos(angle))<0.002f && std::abs(actual().y-10*std::sin(angle))<0.002f);
        (void)scheduler.DestroyTaskContainer(address);
        check(factory.LiveTasks()==live && memory.OutstandingAllocations()==allocations);
    }
    objects.Move(position,original,false);
    std::cout<<"Native RevoluteTask "<<objects.ObjectTypeName(object)<<": real orbit/center approach, scheduled tick, overshoot correction, optional camera and balanced cleanup verified\n";
}
}
