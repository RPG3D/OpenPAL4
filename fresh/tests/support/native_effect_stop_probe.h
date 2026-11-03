#pragma once

#include "pal4/binary_xml_config.h"
#include <array>
#include <bit>
#include <cmath>
#include <iostream>
#include <source_location>
#include <stdexcept>
#include <string>

namespace pal4::test {
// Uses an already-loaded original H_077 child and the actual group scheduler.
inline void VerifyNativeNestedEffectStop(NativeBaseObjectMemory& memory,
    NativeEventDocumentRuntime& documents,NativeParticleEffectFactory& objects,
    NativeEffectTaskFactory& factory,NativeScriptObjectRuntime& scheduler,
    NativeEffectObjectQueues& queues,NativeEffectGroupFrame& frames,
    std::uint32_t object,std::uint32_t child,std::uint32_t group) {
    const auto check=[](bool ok,const std::source_location at=std::source_location::current()) {
        if(!ok) {std::cerr<<"native effect Stop probe line="<<at.line()<<std::endl;
            throw std::runtime_error("native effect Stop assertion");}
    };
    const auto own_frame=objects.Frame(object),child_frame=memory.ReadDword(child+172u);
    const auto owners=objects.LiveObjects();
    for(const bool smooth:{false,true}) {
        objects.SetMode(object,0u);
        check(objects.ResetPlayback(object));queues.QueueEffect(group,object);
        objects.PlayEffect(objects.ResolveFacet(object,"PlayEffectTask"),1u);
        frames.Update(0.05);
        check(factory.LiveTasks()==10u && memory.ReadDword(child+8u) && memory.ReadByte(object+6u));
        std::array<std::uint32_t,15> xml{};
        const auto node=reinterpret_cast<std::uint32_t>(xml.data());
        (void)documents.InitializeDocument(node,"nested-stop-task",0u);
        BinaryXmlNode definition{"Task",{
            {"Attr_Class_Name",3u,std::string("StopEffectTask")},
            {"Attr_Task_StopEffect_Smooth",1u,std::int32_t(smooth)},
            {"Attr_Inherit_Pre_Task",1u,std::int32_t(0)}}, {}};
        const auto bytes=BinaryXmlConfig::Serialize(definition);
        check(bytes && documents.LoadBinaryDocument(node,*bytes));
        const auto task=factory.CreateTask(node);check(task);
        documents.DestroyDocument(node);memory.WriteDword(task+64u,object);
        GomTaskContainerState container{};
        const auto handle=reinterpret_cast<std::uint32_t>(&container);
        scheduler.InitializeTaskContainer(handle);
        // Smooth completion must outlive the shorter base expected duration.
        scheduler.AddTask(handle,task,"nested-stop",{2u,0.15f,0.1f});
        (void)scheduler.BeginWaiting(handle);
        (void)scheduler.UpdateTaskContainer(handle,0.05f);
        check(!memory.ReadDword(task+88u) && memory.ReadDword(child+8u));
        bool began{},draining{},finished{};
        for(unsigned tick=0;tick<400u && container.count;++tick) {
            (void)scheduler.UpdateTaskContainer(handle,0.05f);
            if(container.count && memory.ReadDword(task+88u)) {
                began=true;check(memory.ReadDword(task+88u)==object+36u);
                check(!memory.ReadDword(child+8u) && factory.LiveTasks()==1u);
                if(smooth && !memory.ReadByte(task+84u)) {
                    const auto duration=std::bit_cast<float>(memory.ReadDword(task+76u));
                    const auto elapsed=std::bit_cast<float>(memory.ReadDword(task+80u));
                    check(std::isfinite(duration) && duration>0.1f && elapsed<duration &&
                        memory.ReadByte(object+6u) && memory.ReadByte(object+11u));
                    if(elapsed>0.1f) {check(!scheduler.IsDurationComplete(task));draining=true;}
                } else check(!memory.ReadByte(object+6u) && !memory.ReadByte(object+7u));
            }
            if(container.count && memory.ReadDword(task+40u)==4u) {
                check(!memory.ReadDword(task+88u) && memory.ReadByte(task+84u));finished=true;
            }
            frames.Update(0.05);
        }
        check(began && finished && (!smooth || draining) && !container.count && !factory.LiveTasks());
        (void)scheduler.DestroyTaskContainer(handle);
        check(objects.Frame(object)==own_frame && memory.ReadDword(child+172u)==child_frame &&
            memory.ReadByte(object+4u) && !memory.ReadByte(object+6u) &&
            !memory.ReadDword(child+8u) && objects.LiveObjects()==owners);
    }
    objects.SetMode(object,0u);
}
}  // namespace pal4::test
