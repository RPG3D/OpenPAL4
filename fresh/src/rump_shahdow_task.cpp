#include "pal4/rump_shahdow_task.h"

#include "rump_shahdow_task_state.h"

namespace pal4 {
using namespace rump_shahdow_detail;

RumpShahdowTaskState& RumpShahdowTask::Construct(
    RumpShahdowTaskState& state, const std::uint8_t stack_byte,
    IRumpShahdowTaskRuntime& runtime) {
    runtime.ConstructBase(state);
    state.words[18u] = 0u;
    state.words[20u] = 0u;
    state.words[21u] = 0u;
    state.words[23u] = 0u;
    WriteByte(state, 96u, stack_byte);
    state.words[25u] = 0u;
    state.words[26u] = 0u;
    state.words[27u] = 0u;
    state.words[0u] = kVtable;
    return state;
}

RumpShahdowTaskState& RumpShahdowTask::Destroy(
    RumpShahdowTaskState& state, IRumpShahdowTaskRuntime& runtime) {
    state.words[0u] = kVtable;
    (void)ClearShadows(state, runtime);
    runtime.FreeCollectionStorage(state);
    state.words[25u] = 0u;
    state.words[26u] = 0u;
    state.words[27u] = 0u;
    runtime.DestroyBase(state);
    return state;
}

std::string_view RumpShahdowTask::ClassName() {
    return "RumpShahdowTask";
}

std::uint32_t RumpShahdowTask::ClearShadows(
    RumpShahdowTaskState& state, IRumpShahdowTaskRuntime& runtime) {
    state.words[18u] = 0u;
    const auto count = runtime.RecordCount(state);
    for (std::uint32_t index = 0u; index < count; ++index) {
        const auto record_address = runtime.RecordAt(state, index);
        auto record = runtime.ReadRecord(record_address);
        const auto show = runtime.ResolveFacet(record.child, "ShowobjTask");
        runtime.SetShowFacet(show, false);
        runtime.DispatchChildEvent(record.child, true);
        record.child = 0u;
        runtime.WriteRecord(record_address, record);
        runtime.FreeRecord(record_address);
    }
    const auto old_end = state.words[26u];
    runtime.ClearRecords(state);
    return old_end;
}

std::uint8_t RumpShahdowTask::ActivateShadows(
    RumpShahdowTaskState& state, IRumpShahdowTaskRuntime& runtime) {
    WriteFlag(state, 113u, true);
    state.words[22u] = 0u;
    const auto count = runtime.RecordCount(state);
    for (std::uint32_t index = 0u; index < count; ++index) {
        const auto record = runtime.ReadRecord(runtime.RecordAt(state, index));
        const auto show = runtime.ResolveFacet(record.child, "ShowobjTask");
        runtime.SetShowFacet(show, true);
        runtime.DispatchChildEvent(record.child, false);
    }
    return 1u;
}

std::uint8_t RumpShahdowTask::Stop(
    RumpShahdowTaskState& state, IRumpShahdowTaskRuntime& runtime) {
    state.words[18u] = 0u;
    WriteFlag(state, 113u, false);
    const auto count = runtime.RecordCount(state);
    for (std::uint32_t index = 0u; index < count; ++index) {
        const auto record_address = runtime.RecordAt(state, index);
        auto record = runtime.ReadRecord(record_address);
        const auto show = runtime.ResolveFacet(record.child, "ShowobjTask");
        runtime.SetShowFacet(show, false);
        const auto frame = runtime.ChildFrame(record.child);
        if (runtime.FrameHasParent(frame))
            runtime.RemoveFrameChild(frame);
        runtime.DispatchChildEvent(record.child, true);
        record.child = 0u;
        runtime.WriteRecord(record_address, record);
        runtime.FreeRecord(record_address);
    }
    runtime.ClearRecords(state);
    return 1u;
}

}  // namespace pal4
