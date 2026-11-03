#pragma once
#include "pal4/native_effect_rotation_facets.h"
#include "pal4/binary_xml_config.h"
#include <cmath>
#include <functional>
#include <iostream>
#include <source_location>

namespace pal4::test {
// Called on real resource owners, not a transform fixture.
inline void VerifyNativeEffectRotation(NativeBaseObjectMemory& memory,
    NativeEventDocumentRuntime& documents,NativeParticleEffectFactory& objects,
    NativeEffectTaskFactory& factory,NativeScriptObjectRuntime& scheduler,
    std::uint32_t object,const std::function<std::array<float,3>()>& read_actual={}) {
    const auto check=[](bool ok,const std::source_location at=std::source_location::current()) {
        if(!ok) {std::cerr<<"native rotation probe line="<<at.line()<<std::endl;
            throw std::runtime_error("native effect rotation assertion");}
    };
    const auto allocations=memory.OutstandingAllocations(),live=factory.LiveTasks();
    const auto facet=objects.ResolveFacet(object,"RotateTask");check(facet!=0u);
    const auto read=[&] {std::array<float,3> value{};
        if(read_actual) return read_actual();
        objects.ReadRotation(facet,value);return value;};
    const auto original=read();
    for(unsigned mode=0;mode<3;++mode) {
        objects.ApplyRotation(facet,{0,0,0},false);
        std::array<std::uint32_t,15> xml{};
        const auto root=reinterpret_cast<std::uint32_t>(xml.data());
        (void)documents.InitializeDocument(root,"native-rotate",0u);
        BinaryXmlNode definition{"Task",{
            {"Attr_Class_Name",3u,std::string("RotateTask")},
            {"Attr_Task_Rotate_Result",1u,std::int32_t(mode!=1)},
            {"Attr_Task_Rotate_y",2u,90.0f},
            {"Attr_Add_Vel_y",2u,30.0f},
            {"Attr_Task_Rotate_Rollback",1u,std::int32_t(mode==2)},
            {"Attr_Time_Interval",2u,0.1f},
            {"Attr_Time_Interval2",2u,0.15f}}, {}};
        const auto bytes=BinaryXmlConfig::Serialize(definition);
        check(bytes && documents.LoadBinaryDocument(root,*bytes));
        const auto task=factory.CreateTask(root);check(task);
        documents.DestroyDocument(root);memory.WriteDword(task+64u,object);
        GomTaskContainerState container{};const auto address=reinterpret_cast<std::uint32_t>(&container);
        scheduler.InitializeTaskContainer(address);
        scheduler.AddTask(address,task,"native-rotate",{2u,0.1f,0.5f});
        (void)scheduler.BeginWaiting(address);
        bool intermediate{},finish{},reverse{},first_wait{},second_wait{};
        for(unsigned tick=0;tick<(mode==2?100u:30u) && container.count;++tick) {
            (void)scheduler.UpdateTaskContainer(address,0.05f);
            const auto value=read();
            check(std::isfinite(value[0]) && std::isfinite(value[1]) && std::isfinite(value[2]));
            intermediate|=value[1]>1.0f && value[1]<80.0f;
            if(container.count) {
                if(mode==2) {
                    check(!scheduler.IsDurationComplete(task));
                    reverse|=memory.ReadByte(task+133u)!=0u;
                    first_wait|=memory.ReadByte(task+148u)!=0u;
                    second_wait|=memory.ReadByte(task+160u)!=0u;
                } else if(memory.ReadDword(task+40u)==4u) {
                    check(!memory.ReadDword(task+128u));finish=true;
                    if(mode==0) check(std::abs(value[1]-30.0f)<0.001f);
                }
            }
        }
        check(intermediate);
        if(mode==2) check(container.count && reverse && first_wait && second_wait);
        else {
            check(!container.count && finish);
            const auto final=read();
            check(std::abs(final[1]-(mode==0?30.0f:45.0f))<0.001f);
        }
        (void)scheduler.DestroyTaskContainer(address);
        check(factory.LiveTasks()==live && memory.OutstandingAllocations()==allocations);
    }
    objects.ApplyRotation(facet,original,false);
}
}
