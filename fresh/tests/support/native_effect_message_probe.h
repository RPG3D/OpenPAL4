#pragma once
#include "pal4/arena_effect_manager.h"
#include "pal4/message_task.h"
#include "pal4/native_combat_effect_notifications.h"
#include "pal4/native_script_event_router.h"
#include "pal4/native_combat_action_messages.h"

namespace pal4::test {
inline void VerifyNativeEffectMessage(NativeBaseObjectMemory& memory,
    NativeEventDocumentRuntime& documents,NativeParticleEffectFactory& objects,
    NativeEffectTaskFactory& factory,NativeScriptObjectRuntime& tasks,
    std::uint32_t object,std::uint32_t dialog) {
    const auto check=[](bool ok) {if(!ok) throw std::runtime_error("native MessageTask probe");};
    const auto baseline=memory.OutstandingAllocations();
    {
        NativeDialogObject config_owner(memory,documents);
        memory.WriteDword(object+12u,config_owner.Address());
        check(objects.ReadMessageId(object,"ID",-1)==-1);
        documents.WriteIntAttribute(config_owner.Address()+20u,"ID",123);
        check(objects.ReadMessageId(object,"ID",-1)==123);
        memory.WriteDword(object+12u,dialog);
    }
    const auto before=*reinterpret_cast<RtkPrtStdAtomicObjectState*>(object);
    std::array<std::uint32_t,15> config{};
    const auto xml=reinterpret_cast<std::uint32_t>(config.data());
    documents.InitializeDocument(xml,"message-probe",0u);
    documents.WriteStringAttribute(xml,"Attr_Class_Name","MessageTask");
    documents.WriteStringAttribute(xml,"Attr_Task_Message_What",std::string(257,'M'));
    const auto task=factory.CreateTask(xml);check(task && factory.LiveTasks()==1u);
    // Task text survives document replacement and uses host ownership.
    documents.WriteStringAttribute(xml,"Attr_Task_Message_What","changed");
    const auto text=memory.ReadDword(task+76u);
    check(text && std::string_view(reinterpret_cast<const char*>(text))==std::string(257,'M'));
    memory.WriteDword(task+64u,object);
    const auto owner=memory.ReadDword(object+16u);
    const auto owner_vtable=memory.ReadDword(owner);
    for(const auto vtable:{ArenaEffectManager::kVtable,0x84b19cu}) {
        memory.WriteDword(owner,vtable);
        tasks.BeginRun(task);
        check(memory.ReadDword(task+88u)==objects.ResolveFacet(object,"MessageTask"));
        tasks.Finish(task);check(!memory.ReadDword(task+88u));
        check(reinterpret_cast<RtkPrtStdAtomicObjectState*>(object)->words==before.words);
    }
    memory.WriteDword(owner,0u);
    bool rejected{};
    try {tasks.BeginRun(task);} catch(const std::logic_error&){rejected=true;}
    memory.WriteDword(owner,owner_vtable);check(rejected);
    {
        std::array<std::uint32_t,130> combat{};
        NativeScriptEventRouter router;
        NativeCombatEffectNotifications notifications(owner,
            reinterpret_cast<std::uint32_t>(combat.data()),router);
        struct Route final : INativeEffectOwnerMessages {
            NativeCombatEffectNotifications& notifications;
            explicit Route(NativeCombatEffectNotifications& n):notifications(n){}
            void EffectMessage(std::uint32_t o,const EffectSystemHandleWords& h,
                std::string_view text) override {notifications.Message(o,{h.id,h.system},text);}
        } route(notifications);
        struct Restore {
            NativeParticleEffectFactory& objects;NativeBaseObjectMemory& memory;
            std::uint32_t owner,vtable;
            ~Restore(){objects.BindOwnerMessages(nullptr);memory.WriteDword(owner,vtable);}
        } restore{objects,memory,owner,owner_vtable};
        objects.BindOwnerMessages(&route);memory.WriteDword(owner,0x8452c8u);
        unsigned calls{};bool fail{},nested{};
        const auto subscription=router.Subscribe(router.Key("effect_message"),1u,[&](std::uint32_t p){
            check(notifications.Busy());++calls;
            const auto* record=reinterpret_cast<const std::uint32_t*>(p);
            check(record[1]==owner && record[2]==1u);
            if(nested){check(record[0]==77u && !record[3] && !record[4]);return;}
            check(record[3]!=text && record[4]==257u && std::string_view(reinterpret_cast<const char*>(record[3]),record[4])==std::string(257,'M'));
            const auto snapshot=std::array{record[0],record[1],record[2],record[3],record[4],record[5]};
            nested=true;notifications.Message(owner,{77u,owner},{});nested=false;
            check(notifications.Busy() && snapshot==std::array{record[0],record[1],record[2],record[3],record[4],record[5]});
            if(fail)throw std::logic_error("test Combat message observer failure");
        });
        // Explicit receiving-task header; real particle MessageTask, facet,
        // notification and host text payload traverse the actual native route.
        std::array<std::uint32_t,24> receiver{};
        const auto receiving_task=reinterpret_cast<std::uint32_t>(receiver.data());
        NativeCombatActionMessages messages(router);unsigned received{};
        messages.Register(receiving_task,[&](auto,auto){throw std::logic_error("unexpected animation in effect probe");},
            [&](std::uint32_t target,std::uint32_t id,std::uint32_t system,std::uint32_t header){
                check(target==receiving_task && system==owner && notifications.Busy());++received;
                const auto* name=reinterpret_cast<const std::uint32_t*>(header);
                check(name[0]==1u);
                if(id==77u)check(!name[1] && !name[2]);
                else check(name[1]!=text && name[2]==257u && std::string_view(reinterpret_cast<const char*>(name[1]),name[2])==std::string(257,'M'));
            });
        tasks.BeginRun(task);tasks.Finish(task);
        check(calls==2u && received==2u && !notifications.Busy());
        fail=true;bool threw{};
        try{tasks.BeginRun(task);}catch(const std::logic_error&){threw=true;}
        check(threw && calls==4u && received==3u && !notifications.Busy());tasks.Finish(task);
        messages.Detach(receiving_task);fail=false;tasks.BeginRun(task);tasks.Finish(task);
        check(received==3u && calls==6u && !receiver[20] && !receiver[22]);
        check(reinterpret_cast<RtkPrtStdAtomicObjectState*>(object)->words==before.words);
        check(router.Unsubscribe(subscription));
        bool foreign{};try{notifications.Message(owner,{1u,owner+4u},"bad");}
        catch(const std::invalid_argument&){foreign=true;}check(foreign && !notifications.Busy());
        std::cout<<"Native Combat MessageTask: real particle facet and slot12 route, owned257-byte text, nested empty message, exception unwind and foreign owner rejection verified\n";
        std::cout<<"Native combat action messages: real particle MessageTask to receiving task, nested text views and cancellation pass\n";
    }
    tasks.DeleteTask(task,1u);
    // Missing text is still a valid empty message; scheduled completion owns
    // final deletion, and cancellation releases a second task before Begin.
    documents.CleanupDocument(xml);
    documents.WriteStringAttribute(xml,"Attr_Class_Name","MessageTask");
    const auto empty=factory.CreateTask(xml);
    check(std::string_view(reinterpret_cast<const char*>(memory.ReadDword(empty+76u))).empty());
    const auto cancelled=factory.CreateTask(xml);tasks.DeleteTask(cancelled,1u);
    GomTaskContainerState container;
    const auto manager=reinterpret_cast<std::uint32_t>(&container);
    tasks.InitializeTaskContainer(manager);
    memory.WriteDword(empty+64u,object);memory.WriteDword(owner,ArenaEffectManager::kVtable);
    tasks.AddTask(manager,empty,"MessageTask",{3u,0.0f,0.0f});
    (void)tasks.BeginWaiting(manager);
    for(unsigned i=0;i<12u && container.count;++i) (void)tasks.UpdateTaskContainer(manager,0.05f);
    memory.WriteDword(owner,owner_vtable);
    check(!container.count && !factory.LiveTasks() && !tasks.RegisteredTasks());
    (void)tasks.DestroyTaskContainer(manager);
    documents.DestroyDocument(xml);
    check(memory.OutstandingAllocations()==baseline);
    std::cout<<"native MessageTask: owned text, default/configured ID, verified owner callbacks, "
        "unknown override rejection, scheduling/cancellation and effect preservation verified\n";
}
}
