#include "pal4/native_combat_fatal_children.h"
#include "pal4/combat_script_task_operations.h"
#include "pal4/combat_status_tasks.h"
#include "pal4/combat_action_task_operations.h"
#include "pal4/gom_task_base.h"
#include "pal4/gom_task_container.h"
#include "pal4/native_base_object_memory.h"
#include <functional>
#include <iostream>
#include <stdexcept>
#include <vector>
namespace {
void Check(bool v,const char* s){if(!v)throw std::runtime_error(s);}
template<class F> void Reject(F f){bool rejected=false;try{f();}catch(const std::exception&){rejected=true;}Check(rejected,"expected rejection");}
struct Services final : pal4::ICombatScriptTaskRuntime,pal4::ICombatStatusTaskRuntime,
    pal4::ICombatActionTaskRuntime {
    std::vector<unsigned> calls;
    std::function<void()> on_event;
    std::function<void()> on_action;
    std::string ReadCString(unsigned a) override{return reinterpret_cast<const char*>(a);}
    unsigned SkillSystem() override{throw std::logic_error("unexpected skill system");}
    unsigned GlobalSystem() override{throw std::logic_error("unexpected global system");}
    void SetActorTarget(unsigned actor,unsigned text,std::uint8_t mode) override {
        Check(actor==0x1000u && mode==7u,"ChangeAction actor564 and mode");
        Check(ReadCString(ReadWord(text+4u))=="Z12","native owned action text");
        if(on_action)on_action();
        calls.push_back(3u);
    }
    void StartActorAction(unsigned,unsigned,unsigned) override{throw std::logic_error("unexpected action start");}
    std::string GetAnimationName(unsigned) override{throw std::logic_error("unexpected animation wait");}
    std::int32_t GetAnimationFlag(unsigned) override{throw std::logic_error("unexpected animation flag");}
    int CompareInsensitive(std::string_view,std::string_view) override{throw std::logic_error("unexpected animation compare");}
    unsigned ReadWord(unsigned a) override{return *reinterpret_cast<unsigned*>(a);}
    std::uint8_t ReadByte(unsigned a) override{return *reinterpret_cast<std::uint8_t*>(a);}
    void WriteWord(unsigned a,unsigned v) override{*reinterpret_cast<unsigned*>(a)=v;}
    void FireEvent(unsigned event,unsigned actor,unsigned argument) override {
        Check(event==0x4300u && actor==0x1000u && argument==0x40400000u,"event payload preserves float3 bits");
        if(on_event)on_event();
        calls.push_back(1u);
    }
    void CallActor(unsigned actor,unsigned slot,unsigned target) override {
        Check(actor==0x1000u && slot==544u && target==0x2000u,"death end forwards actual actor544");
        calls.push_back(2u);
    }
    void SetProtectTarget(unsigned,unsigned,unsigned) override{throw std::logic_error("unexpected protect");}
    void ConstructBase(unsigned) override{throw std::logic_error("unexpected external base construction");}
    void SetStateOne(unsigned) override{throw std::logic_error("unexpected state1");}
};
void Run(){
    Services services;
    pal4::NativeCombatFatalChildren owner(
        static_cast<pal4::ICombatScriptTaskRuntime&>(services),
        static_cast<pal4::ICombatStatusTaskRuntime&>(services));
    auto& runtime=owner.ContainerRuntime();
    pal4::GomTaskBase base(owner.BaseRuntime());
    pal4::GomTaskContainer container(runtime,base);
    pal4::GomTaskContainerState state{};
    const auto address=static_cast<unsigned>(reinterpret_cast<std::uintptr_t>(&state));
    container.Initialize(state,0);
    Check(owner.OwnedAllocations()==1u,"real GOM sentinel allocation");
    Reject([&]{(void)owner.Create(0x845d34u);});
    Reject([&]{(void)owner.Create(0x845de4u);});
    Reject([&]{(void)owner.Create(0x845de8u);});
    Reject([&]{owner.Destroy(0x1234u);});
    for(unsigned cycle=0;cycle<40u;++cycle){
        const auto event=owner.Create(0x845de0u),end=owner.Create(0x845decu);
        auto& e=runtime.TaskAt(event);auto& d=runtime.TaskAt(end);
        Check(e.vtable==0x845df4u && d.vtable==0x845e48u && e.name==0u && e.context==0u,"original child layouts and base construction");
        Check(runtime.ReadWord(event+60u)==0u && runtime.ReadWord(event+64u)==0u,"event ctor clears operands");
        runtime.WriteWord(event+60u,0x1000u);runtime.WriteWord(event+64u,0x4300u);runtime.WriteWord(event+68u,0x40400000u);
        runtime.WriteWord(end+60u,0x1000u);runtime.WriteWord(end+64u,0x2000u);
        (void)container.AddTaskAndInitialize(state,address,e,event,"event",{1u,0,0});
        (void)container.AddTaskAndInitialize(state,address,d,end,"dead",{1u,0,0});
        Check(e.next==end && d.previous==event && e.context==address,"actual timing/link initialization");
        Check(owner.Count()==2u && owner.OwnedAllocations()==7u,"children names nodes sentinel owned");
        Reject([&]{owner.Destroy(event);});
        services.calls.clear();
        services.on_event=[&]{Reject([&]{owner.Destroy(event);});Reject([&]{(void)owner.Create(0x845de0u);});};
        (void)container.BeginWaiting(state);
        (void)container.Update(state,0.25f);
        Check(services.calls==std::vector<unsigned>{1u} && e.phase==3u && d.phase==2u,"event starts before dependent end");
        (void)container.Update(state,0.25f);
        Check(e.phase==4u && d.phase==2u,"end waits for original finish latch");
        for(unsigned tick=0;tick<12u && !state.ready;++tick)(void)container.Update(state,0.25f);
        Check(state.ready && state.count==0u && services.calls==std::vector<unsigned>({1u,2u}),"original scheduler completes in order exactly once");
        Check(owner.Count()==0u && owner.OwnedAllocations()==1u,"scheduled children names and list nodes released");
        Reject([&]{owner.Destroy(event);});
    }
    // Failed game callback remains phase2, propagates, and can retry. No fake completion.
    const auto failing=owner.Create(0x845de0u);
    runtime.WriteWord(failing+60u,0x1000u);runtime.WriteWord(failing+64u,0x4300u);runtime.WriteWord(failing+68u,0x40400000u);
    (void)container.AddTaskAndInitialize(state,address,runtime.TaskAt(failing),failing,"retry",{1u,0,0});
    (void)container.BeginWaiting(state);
    services.on_event=[]{throw std::runtime_error("unbound actual game event");};
    Reject([&]{(void)container.Update(state,0.1f);});
    Check(runtime.TaskAt(failing).phase==2u && !state.ready,"failed callback does not complete task");
    services.on_event={};
    for(unsigned tick=0;tick<8u && !state.ready;++tick)(void)container.Update(state,0.1f);
    Check(state.ready && owner.Count()==0u,"callback failure releases reentry guard for retry");
    // Null operands follow original virtual callback gates.
    for(unsigned factory:{0x845de0u,0x845decu}){
        const auto task=owner.Create(factory);
        runtime.WriteWord(task+60u,0u);runtime.WriteWord(task+64u,0u);
        const auto count=services.calls.size();runtime.BeginRun(task);
        Check(services.calls.size()==count,"null child operands skip service call");
        owner.Destroy(task);
    }
    (void)container.Destroy(state);
    Check(owner.Count()==0u && owner.OwnedAllocations()==0u,"balanced fatal child and GOM lifetime");
}
void BorrowedReactionDomain(){
    struct Impact final : pal4::ICombatStatusActorRuntime {
        unsigned calls{};
        unsigned ReadWord(unsigned a) override{return *reinterpret_cast<unsigned*>(a);}
        void CallActor(unsigned actor,unsigned slot,unsigned target) override{
            Check(actor==0x1000u && slot==508u && target==0x2000u,"borrowed impact receiver");
            ++calls;
        }
    } impact;
    Services action;
    pal4::NativeBaseObjectMemory domain;
    const auto sibling=domain.Allocate(16u);
    unsigned prepared{};
    {
        pal4::NativeCombatFatalChildren owner(impact,action,[&](unsigned actor,unsigned flag){
            Check(actor==0x2000u && flag==1u,"borrowed prepare receiver");
            ++prepared;
            Reject([&]{owner.ValidateIdle();});
        },domain);
        auto& runtime=owner.ContainerRuntime();
        pal4::GomTaskBase base(owner.BaseRuntime());
        pal4::GomTaskContainer container(runtime,base);
        pal4::GomTaskContainerState state{};
        const auto address=static_cast<unsigned>(reinterpret_cast<std::uintptr_t>(&state));
        container.Initialize(state,0u);
        const auto first=owner.Create(0x845d10u),second=owner.Create(0x845a80u);
        runtime.WriteWord(first+60u,0x1000u);runtime.WriteWord(first+64u,0x2000u);
        runtime.WriteWord(second+60u,0x2000u);
        (void)container.AddTaskAndInitialize(state,address,runtime.TaskAt(first),first,"impact",{1u,0,0});
        (void)container.AddTaskAndInitialize(state,address,runtime.TaskAt(second),second,"prepare",{1u,0,0});
        Check(owner.Owns(first) && owner.Owns(second) && !owner.Owns(sibling),"borrowed domain dispatch uses identity");
        Check(!owner.OwnedAllocations() && domain.OutstandingAllocations()==8u,"borrowed allocations counted only by domain");
        (void)container.BeginWaiting(state);
        for(unsigned tick=0;tick<16u && !state.ready;++tick)(void)container.Update(state,.016f);
        Check(state.ready && impact.calls==1u && prepared==1u && !owner.Count(),"borrowed reaction GOM dispatch");
        Check(!owner.Owns(first) && domain.OutstandingAllocations()==2u,"borrowed names nodes and children retired");
        (void)container.Destroy(state);
        owner.ValidateIdle();
    }
    Check(domain.OutstandingAllocations()==1u,"child owner preserves sibling allocation after destruction");
    (void)domain.Free(sibling);
    Check(!domain.OutstandingAllocations(),"caller releases borrowed domain");
}
void ChangeAction(){
    Services services;
    pal4::NativeCombatFatalChildren owner(services,services,services);
    auto& runtime=owner.ContainerRuntime();
    pal4::GomTaskBase base(owner.BaseRuntime());
    pal4::GomTaskContainer container(runtime,base);
    pal4::GomTaskContainerState state{};
    const auto address=static_cast<unsigned>(reinterpret_cast<std::uintptr_t>(&state));
    container.Initialize(state,0);
    for(unsigned cycle=0;cycle<40u;++cycle){
        const auto child=owner.Create(0x845d34u);
        Check(runtime.TaskAt(child).vtable==0x845d3cu,"original ChangeAction factory");
        owner.AssignAction(child,"temporary");owner.AssignAction(child,"Z12");
        Check(owner.OwnedActionTexts()==1u,"replacement retains single native string");
        runtime.WriteWord(child+60u,0x1000u);
        *reinterpret_cast<std::uint8_t*>(child+80u)=7u;
        (void)container.AddTaskAndInitialize(state,address,runtime.TaskAt(child),child,"ChangeActionTask",{1u,0,0});
        owner.AssignAction(child,"Z12"); // Original recipe assigns after linking.
        (void)container.BeginWaiting(state);
        Reject([&]{owner.AssignAction(child,"invalid");});
        services.on_action=[]{throw std::runtime_error("animation dependency unavailable");};
        Reject([&]{(void)container.Update(state,0.1f);});
        Check(runtime.TaskAt(child).phase==2u,"ChangeAction failure preserves retry phase");
        services.on_action=[&]{Reject([&]{owner.AssignAction(child,"reentry");});Reject([&]{owner.Destroy(child);});};
        const auto count=services.calls.size();
        for(unsigned tick=0;tick<8u && !state.ready;++tick)(void)container.Update(state,0.1f);
        Check(state.ready && services.calls.size()==count+1u,"ChangeAction scheduler dispatches exactly once without animation polling");
        Check(owner.Count()==0u && owner.OwnedActionTexts()==0u && owner.OwnedAllocations()==1u,"ChangeAction native text and task cleanup");
    }
    const auto child=owner.Create(0x845d34u);
    owner.AssignAction(child,"Z12");
    const auto count=services.calls.size();runtime.BeginRun(child);
    Check(services.calls.size()==count,"null actor gate");owner.Destroy(child);
    const auto event=owner.Create(0x845de0u);
    Reject([&]{owner.AssignAction(event,"Z12");});owner.Destroy(event);
    (void)container.Destroy(state);
    Check(owner.OwnedActionTexts()==0u && owner.OwnedAllocations()==0u,"balanced ChangeAction owner");
}
}
int main(){try{Run();ChangeAction();BorrowedReactionDomain();std::cout<<"Native fatal event/end/ChangeAction and borrowed reaction children: scheduling, callbacks, retry and ownership pass\n";return 0;}
catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
