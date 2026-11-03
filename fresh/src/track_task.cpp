#include "pal4/track_task.h"

#include "track_task_state.h"

namespace pal4 {

TrackTaskState& TrackTask::Construct(TrackTaskState& state,
                                      ITrackTaskRuntime& runtime) {
    runtime.ConstructBase(state);
    track_task_detail::WriteByte(state, 92u,
                                 runtime.ConstructorStackByte());
    state.words[24u] = 0u;
    state.words[25u] = 0u;
    state.words[26u] = 0u;
    state.words[35u] = 0u;
    state.words[0u] = kVtable;
    return state;
}

TrackTaskState& TrackTask::Destroy(TrackTaskState& state,
                                    TrackPointArray& points,
                                    ITrackTaskRuntime& runtime) {
    state.words[0u] = kVtable;
    runtime.FreeNativePointBuffer(state.words[24u]);
    points.ClearAndRelease();
    state.words[24u] = 0u;
    state.words[25u] = 0u;
    state.words[26u] = 0u;
    runtime.DestroyBase(state);
    return state;
}

std::string_view TrackTask::ClassName() { return "TrackTask"; }

std::uint8_t TrackTask::Stop(TrackTaskState& state) {
    track_task_detail::WriteFlag(state, 72u, false);
    return 1u;
}

}  // namespace pal4
