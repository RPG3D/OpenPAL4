#include "pal4/gom_task_container.h"

#include "pal4/gom_task_base.h"
#if !defined(PAL4_PORTABLE_RUNTIME)
#include "pal4/comparison_flee_task_trace.h"
#endif

#include <new>
#include <exception>

namespace pal4 {

GomTaskContainerState& GomTaskContainer::Initialize(
    GomTaskContainerState& state, std::uint8_t source_byte) {
    state.opaque_byte = source_byte;
    const auto sentinel = runtime_.AllocateMemory(12u);
    if (sentinel == 0u) throw std::bad_alloc();
    runtime_.WriteWord(sentinel, sentinel);
    runtime_.WriteWord(sentinel + 4u, sentinel);
    state.sentinel = sentinel;
    state.count = 0u;
    state.ready = 1u;
    return state;
}

std::uint32_t GomTaskContainer::Destroy(GomTaskContainerState& state) {
    static_cast<void>(CleanupObjects(state));
    const auto sentinel = state.sentinel;
    auto node = runtime_.ReadWord(sentinel);
    while (node != sentinel) {
        const auto next = runtime_.ReadWord(node);
        const auto previous = runtime_.ReadWord(node + 4u);
        runtime_.WriteWord(previous, next);
        runtime_.WriteWord(next + 4u, previous);
        static_cast<void>(runtime_.FreeMemory(node));
        --state.count;
        node = next;
    }
    const auto result = runtime_.FreeMemory(sentinel);
    state.sentinel = 0u;
    state.count = 0u;
    return result;
}

std::uint32_t GomTaskContainer::AddTaskAndInitialize(
    GomTaskContainerState& state, std::uint32_t container_address,
    GomTaskBaseState& task, std::uint32_t task_address,
    const char* name, const GomTaskTimingConfig& timing) {
    std::uint32_t previous = 0u;
    if (state.count != 0u) {
        const auto tail = runtime_.ReadWord(state.sentinel + 4u);
        previous = runtime_.ReadWord(tail + 8u);
    }
    const auto sentinel = state.sentinel;
    const auto tail = runtime_.ReadWord(sentinel + 4u);
    const auto node = runtime_.AllocateMemory(12u);
    if (node == 0u) throw std::bad_alloc();
    try {
        static_cast<void>(base_.InitializeTimingAndLinks(
            task, name, timing, previous, 0u, container_address));
    } catch (...) {
        static_cast<void>(runtime_.FreeMemory(node));
        throw;
    }
    // Commit links only once both task-name and list-node allocations exist.
    // A failed append must not leave the previous task pointing at freed data.
    if (previous != 0u)
        runtime_.WriteWord(previous + 0x34u, task_address);
    runtime_.WriteWord(node, sentinel);
    runtime_.WriteWord(node + 4u, tail == 0u ? node : tail);
    runtime_.WriteWord(sentinel + 4u, node);
    runtime_.WriteWord(runtime_.ReadWord(node + 4u), node);
    runtime_.WriteWord(node + 8u, task_address);
    return ++state.count;
}

std::uint32_t GomTaskContainer::BeginWaiting(
    GomTaskContainerState& state) {
    const auto sentinel = state.sentinel;
    auto node = runtime_.ReadWord(sentinel);
    while (node != sentinel) {
        const auto task = runtime_.ReadWord(node + 8u);
        runtime_.WriteWord(task + 0x28u, 2u);
        node = runtime_.ReadWord(node);
    }
    if (state.count != 0u) state.ready = 0u;
    return state.count;
}

std::uint32_t GomTaskContainer::Update(GomTaskContainerState& state,
                                       float delta) {
#if !defined(PAL4_PORTABLE_RUNTIME)
    auto& trace=GetComparisonFleeTaskTrace();
    auto* record=trace.Begin(static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(&state)),delta,
        [this](std::uint32_t a){return runtime_.ReadWord(a);});
    struct TraceEnd {ComparisonFleeTaskTrace& trace;ComparisonFleeTaskTrace::Record* record;IGomTaskContainerRuntime& runtime;int errors=std::uncaught_exceptions();
        ~TraceEnd(){if(record && errors==std::uncaught_exceptions())try{trace.End(record,[this](std::uint32_t a){return runtime.ReadWord(a);});}catch(...){}}} trace_end{trace,record,runtime_};
#endif
    auto remaining = state.count;
    if (static_cast<std::int32_t>(remaining) <= 0) return remaining;
    const auto sentinel = state.sentinel;
    auto node = runtime_.ReadWord(sentinel);
    while (node != sentinel) {
        const auto task_address = runtime_.ReadWord(node + 8u);
        auto& task = runtime_.TaskAt(task_address);
        if (task.active != 0u) {
            switch (task.phase) {
            case 2u:
                base_.AccumulateTotal(task, delta);
                base_.AccumulateWait(task, delta);
                if (runtime_.IsWaitComplete(task_address)) {
                    runtime_.BeginRun(task_address);
                    task.phase = 3u;
                }
                break;
            case 3u:
                base_.AccumulateTotal(task, delta);
                base_.AccumulateRun(task, delta);
                runtime_.Tick(task_address, delta);
                if (runtime_.IsDurationComplete(task_address)) {
                    static_cast<void>(base_.LatchFinishTime(task));
                    runtime_.Finish(task_address);
                    task.phase = 4u;
                }
                break;
            case 4u:
                --remaining;
                break;
            default:
                break;
            }
        }
        node = runtime_.ReadWord(node);
    }
    if (static_cast<std::int32_t>(remaining) <= 0) {
        state.ready = 1u;
        return CleanupObjects(state);
    }
    return remaining;
}

std::uint32_t GomTaskContainer::CleanupObjects(
    GomTaskContainerState& state) {
    const auto sentinel = state.sentinel;
    auto node = runtime_.ReadWord(sentinel);
    while (node != sentinel) {
        const auto task = runtime_.ReadWord(node + 8u);
        if (task != 0u) runtime_.DeleteTask(task, 1u);
        node = runtime_.ReadWord(node);
    }

    node = runtime_.ReadWord(sentinel);
    std::uint32_t result = sentinel;
    while (node != sentinel) {
        const auto next = runtime_.ReadWord(node);
        const auto previous = runtime_.ReadWord(node + 4u);
        runtime_.WriteWord(previous, next);
        runtime_.WriteWord(next + 4u, previous);
        result = runtime_.FreeMemory(node);
        --state.count;
        node = next;
    }
    state.ready = 1u;
    return result;
}

std::uint32_t GomTaskContainer::CleanupAndReady(
    GomTaskContainerState& state) {
    const auto result = CleanupObjects(state);
    state.ready = 1u;
    return result;
}

}  // namespace pal4
