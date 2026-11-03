#pragma once
#include "native_effect_revolution_probe.h"
#include "pal4/native_effect_object_queues.h"
#include <source_location>
namespace pal4::test {
// Creates a separate production owner so the retained child pool can be
// checked through task reuse and then released with its original owner.
inline void VerifyNativeRumpShadow(NativeBaseObjectMemory& memory,
    NativeEventDocumentRuntime& documents,NativeParticleEffectFactory& objects,
    NativeEffectTaskFactory& factory,NativeScriptObjectRuntime& scheduler,
    NativeEffectObjectQueues& queues,std::uint32_t definition,
    std::uint32_t owner,std::uint32_t group){
    const auto check=[](bool ok,const std::source_location at=std::source_location::current()){
        if(!ok){std::cerr<<"native shadow probe line="<<at.line()<<std::endl;
            throw std::runtime_error("native shadow assertion");}
    };
    const auto allocations=memory.OutstandingAllocations(),live=objects.LiveObjects(),tasks=factory.LiveTasks();
    objects.BindShadowQueues(queues);
    const auto object=objects.Create(definition,owner,group);check(object);
    const auto facet=objects.ResolveFacet(object,"RumpShahdowTask");check(facet);
    const auto type=std::string(objects.ObjectTypeName(object));
    const auto queued=[&](std::uint32_t child){
        for(auto off:{68u,80u,92u,104u,116u,128u,140u,152u,164u,176u,188u,200u})
            if(queues.Contains(off,child))return true;
        return false;
    };
    std::array<std::uint32_t,3> children{};
    for(unsigned reuse=0;reuse<2;++reuse){
        std::array<std::uint32_t,15> xml{};const auto root=reinterpret_cast<std::uint32_t>(xml.data());
        (void)documents.InitializeDocument(root,"native-shadow",0);
        BinaryXmlNode definition{"Task",{
            {"Attr_Class_Name",3u,std::string("RumpShahdowTask")},
            {"Attr_Task_RumpShadow_Life",2u,0.2f},
            {"Attr_Task_RumpShadow_Fps",2u,20.0f},
            {"Attr_Task_RumpShadow_MaxCount",1u,std::int32_t(3)},
            {"Attr_Task_RumpShadow_CheckChange",1u,std::int32_t(1)}},{}};
        const auto bytes=BinaryXmlConfig::Serialize(definition);check(bytes && documents.LoadBinaryDocument(root,*bytes));
        const auto task=factory.CreateTask(root);check(task);documents.DestroyDocument(root);
        memory.WriteDword(task+64,object);factory.StartTask(task);
        check(objects.LiveObjects()==live+4 && memory.ReadDword(task+104)-memory.ReadDword(task+100)==12);
        for(unsigned i=0;i<3;++i){const auto child=objects.ShadowChild(facet,i);check(child);
            if(reuse)check(children[i]==child);else children[i]=child;
            std::array<std::uint8_t,4> c{};objects.ReadColor(objects.ResolveFacet(child,"ChangeColorTask"),c);check(c[3]==0 && !queued(child));
        }
        GomTaskContainerState container{};const auto address=reinterpret_cast<std::uint32_t>(&container);
        scheduler.InitializeTaskContainer(address);scheduler.AddTask(address,task,"native-shadow",{2u,0.0f,0.5f});
        (void)scheduler.BeginWaiting(address);
        bool copied{},faded{},was_queued{},finished{};
        const auto position=objects.ResolveFacet(object,"PositionTask");
        objects.SetColor(objects.ResolveFacet(object,"ChangeColorTask"),{11,22,33,200});
        for(unsigned tick=0;tick<40 && container.count;++tick){
            const float x=tick<5?10.0f+tick:14.0f; // Freeze to exercise CheckChange.
            objects.Move(position,{x,0,0},false);
            (void)scheduler.UpdateTaskContainer(address,0.05f);
            for(const auto child:children){
                was_queued|=queued(child);
                auto* frame=reinterpret_cast<RwFrame*>(objects.Frame(child));
                copied|=std::abs(RwFrameGetMatrix(frame)->pos.x-x)<0.001f;
                std::array<std::uint8_t,4> c{};objects.ReadColor(objects.ResolveFacet(child,"ChangeColorTask"),c);
                if(c[3]){check(c[0]==11 && c[1]==22 && c[2]==33);faded|=c[3]<200;}
            }
            if(container.count && memory.ReadDword(task+40)==4){
                check(!memory.ReadDword(task+72) && memory.ReadDword(task+100)==memory.ReadDword(task+104));finished=true;
            }
        }
        check(copied && faded && was_queued && finished && !container.count);
        for(const auto child:children){check(!queued(child) && !RwFrameGetParent(reinterpret_cast<RwFrame*>(objects.Frame(child))));}
        (void)scheduler.DestroyTaskContainer(address);check(factory.LiveTasks()==tasks);
    }
    objects.Destroy(object);
    check(objects.LiveObjects()==live && factory.LiveTasks()==tasks && memory.OutstandingAllocations()==allocations);
    std::cout<<"Native RumpShahdowTask "<<type<<": production child pool/reuse, scheduler, SDK transforms, RGB/alpha fade, queue retirement and balanced owner cleanup verified\n";
}
}
