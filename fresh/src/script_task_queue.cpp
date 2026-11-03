#include "pal4/script_task_queue.h"
#include "pal4/script_task_queue_sequence.h"

namespace pal4 {

std::uint32_t ScriptTaskQueue::RegisterEvent(
    const std::uint32_t event, IScriptTaskQueueSequence& sequence,
    IScriptTaskQueueRuntime& runtime) {
    if (event == 0u) return 0u;
    const auto head = sequence.End();
    for (auto entry = sequence.First(); entry != head;
         entry = sequence.Next(entry)) {
        const auto current = sequence.Task(entry);
        if (runtime.ReadByte(current + 40u) == 0u) continue;
        const auto incoming_identity = runtime.EventIdentity(event);
        if (runtime.EventIdentity(current) != incoming_identity) continue;
        (void)runtime.EventIdentity(event);
        runtime.GetSkillSystem();
        runtime.RunNoOp();
        if (!runtime.EventProcessed(event) &&
            runtime.EventProcessed(current) &&
            runtime.EventIdentity(event) != 0u) {
            runtime.StartIdentityAction(runtime.EventIdentity(event));
        }
        runtime.WriteByte(current + 40u, 0u);
    }
    runtime.PrepareEvent(event);
    runtime.SetProcessedFlag(event + 4u);

    if (!sequence.AppendOwned(event)) {
        runtime.DeleteTask(event, 1u);
        return 0u;
    }
    return event;
}

std::uint32_t ScriptTaskQueue::Update(
    const float delta, IScriptTaskQueueSequence& sequence,
    IScriptTaskQueueRuntime& runtime) {
    const auto head = sequence.End();
    for (auto entry = sequence.First(); entry != head;
         entry = sequence.Next(entry)) {
        const auto task = sequence.Task(entry);
        if (runtime.ReadByte(task + 16u) == 0u &&
            runtime.ReadByte(task + 40u) != 0u) {
            runtime.UpdateContainer(task + 4u, delta);
        }
    }

    std::uint32_t result = head;
    for (auto entry = sequence.First(); entry != head;) {
        const auto task = sequence.Task(entry);
        const auto ready = runtime.ReadByte(task + 16u);
        if (ready == 0u) {
            const auto active = runtime.ReadByte(task + 40u);
            if (active != 0u) {
                result = (result & ~0xffu) | active;
                entry = sequence.Next(entry);
                continue;
            }
        }
        const auto enabled = runtime.ReadByte(task + 40u);
        std::uint32_t event = 0u;
        if (enabled != 0u) {
            runtime.StopTask(task);
            event = runtime.ResultEvent(task);
        }
        const auto next = sequence.Next(entry);
        result = sequence.EraseOwned(entry);
        if (event != 0u)
            result = RegisterEvent(event, sequence, runtime);
        entry = next;
    }
    return result;
}

std::uint32_t ScriptTaskQueue::ResetFlags(
    IScriptTaskQueueSequence& sequence,
    IScriptTaskQueueRuntime& runtime) {
    const auto head = sequence.End();
    for (auto entry = sequence.First(); entry != head;
         entry = sequence.Next(entry)) {
        const auto task = sequence.Task(entry);
        if (runtime.ReadByte(task + 16u) == 0u &&
            runtime.ReadByte(task + 40u) != 0u) {
            runtime.WriteByte(task + 40u, 0u);
        }
    }
    return head;
}

std::uint32_t ScriptTaskQueue::Cleanup(IScriptTaskQueueSequence& sequence) {
    const auto head=sequence.End();
    std::uint32_t result=0u;
    for(auto entry=sequence.First();entry!=head;) {
        const auto next=sequence.Next(entry);
        result=sequence.EraseOwned(entry);
        entry=next;
    }
    return result;
}

namespace {
class LegacySequence final : public IScriptTaskQueueSequence {
public:
    std::uint32_t queue;
    IScriptTaskQueueRuntime& runtime;
    LegacySequence(std::uint32_t q,IScriptTaskQueueRuntime& r):queue(q),runtime(r){}
    std::uint32_t End() override{return runtime.ReadWord(queue+4u);}
    std::uint32_t First() override{return runtime.ReadWord(End());}
    std::uint32_t Next(std::uint32_t e) override{return runtime.ReadWord(e);}
    std::uint32_t Task(std::uint32_t e) override{return runtime.ReadWord(e+12u);}
    bool AppendOwned(std::uint32_t task) override {
        const auto head=End(),tail=runtime.ReadWord(head+4u),entry=runtime.Allocate(16u);
        if(!entry)return false;
        runtime.WriteWord(entry,head);
        runtime.WriteWord(entry+4u,tail==0u?entry:tail);
        runtime.WriteWord(head+4u,entry);
        runtime.WriteWord(runtime.ReadWord(entry+4u),entry);
        runtime.WriteByte(entry+8u,1u);
        runtime.WriteWord(entry+12u,task);
        runtime.WriteWord(queue+8u,runtime.ReadWord(queue+8u)+1u);
        return true;
    }
    std::uint32_t EraseOwned(std::uint32_t e) override {
        const auto next=Next(e),previous=runtime.ReadWord(e+4u);
        runtime.WriteWord(previous,next);runtime.WriteWord(next+4u,previous);
        if(runtime.ReadByte(e+8u)!=0u) {
            const auto task=Task(e);
            if(task!=0u)runtime.DeleteTask(task,1u);
        }
        const auto result=runtime.Free(e);
        runtime.WriteWord(queue+8u,runtime.ReadWord(queue+8u)-1u);
        return result;
    }
};
}
std::uint32_t ScriptTaskQueue::RegisterEvent(std::uint32_t queue,std::uint32_t event,IScriptTaskQueueRuntime& runtime) {
    LegacySequence sequence(queue,runtime);return RegisterEvent(event,sequence,runtime);
}
std::uint32_t ScriptTaskQueue::Update(std::uint32_t queue,float delta,IScriptTaskQueueRuntime& runtime) {
    LegacySequence sequence(queue,runtime);return Update(delta,sequence,runtime);
}
std::uint32_t ScriptTaskQueue::ResetFlags(std::uint32_t queue,IScriptTaskQueueRuntime& runtime) {
    LegacySequence sequence(queue,runtime);return ResetFlags(sequence,runtime);
}
std::uint32_t ScriptTaskQueue::Cleanup(std::uint32_t queue,IScriptTaskQueueRuntime& runtime) {
    LegacySequence sequence(queue,runtime);return Cleanup(sequence);
}
} // namespace pal4
