#include "pal4/play_sound3d_task.h"

namespace pal4 {

PlaySound3DTaskState& PlaySound3DTask::Construct(
    PlaySound3DTaskState& state, IPlaySound3DTaskRuntime& runtime) {
    runtime.ConstructBase(state);
    state.words[0u] = kVtable;
    state.words[18u] &= 0xffffff00u;
    state.words[19u] = 0u;
    return state;
}

PlaySound3DTaskState& PlaySound3DTask::Destroy(
    PlaySound3DTaskState& state, IPlaySound3DTaskRuntime& runtime) {
    state.words[0u] = kVtable;
    runtime.DestroyBase(state);
    return state;
}

std::string_view PlaySound3DTask::ClassName() {
    return "PlaySound3DTask";
}

bool PlaySound3DTask::LoadConfig(
    PlaySound3DTaskState& state, const std::uint32_t config,
    IPlaySound3DConfigRuntime& runtime) {
    if (config == 0u)
        return false;
    const auto loop = runtime.ReadInt(
        config, "Attr_Task_PlaySound3D_Loop", 0);
    state.words[18u] = (state.words[18u] & 0xffffff00u) |
                       static_cast<std::uint32_t>(loop > 0);
    return true;
}

std::uint8_t PlaySound3DTask::Start(
    PlaySound3DTaskState& state, IPlaySound3DTaskRuntime& runtime) {
    const auto facet = runtime.ResolveFacet(state.words[16u], ClassName());
    state.words[19u] = facet;
    runtime.StartSound(facet,
        static_cast<std::uint8_t>((state.words[18u] & 0xffu) == 0u));
    return 1u;
}

}  // namespace pal4
