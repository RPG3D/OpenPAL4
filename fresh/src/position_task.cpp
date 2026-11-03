#include "pal4/position_task.h"

#include "position_task_state.h"

namespace pal4 {

PositionTaskState& PositionTask::Construct(PositionTaskState& state,
                                           IPositionTaskRuntime& runtime) {
    runtime.ConstructBase(state);
    state.words[0u] = kVtable;
    position_task_detail::WriteByte(state, 72u, 0u);
    state.words[21u] = 0u;
    state.words[38u] = 0u;
    return state;
}

PositionTaskState& PositionTask::Destroy(PositionTaskState& state,
                                         IPositionTaskRuntime& runtime) {
    state.words[0u] = kVtable;
    runtime.DestroyBase(state);
    return state;
}

PositionTaskState& PositionTask::Delete(PositionTaskState& state,
                                        std::uint8_t flags,
                                        IPositionTaskRuntime& runtime) {
    Destroy(state, runtime);
    if ((flags & 1u) != 0u)
        runtime.FreeObject(state);
    return state;
}

std::string_view PositionTask::ClassName() { return "PositionTask"; }

bool PositionTask::IsDurationComplete(const PositionTaskState& state,
                                      IPositionTaskRuntime& runtime) {
    return position_task_detail::ReadByte(state, 188u) == 0u &&
           runtime.BaseDurationComplete(state);
}

}  // namespace pal4
