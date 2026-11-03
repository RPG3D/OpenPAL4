#include "pal4/uv_rotate_task.h"

#include <bit>

namespace pal4 {

UVRotateTaskState& UVRotateTask::Construct(UVRotateTaskState& state,
                                           IUVRotateTaskRuntime& runtime) {
    runtime.ConstructBase(state);
    state.words[0u] = kVtable;
    for (std::size_t index = 18u; index <= 23u; ++index)
        state.words[index] = 0u;
    return state;
}

UVRotateTaskState& UVRotateTask::Destroy(UVRotateTaskState& state,
                                         IUVRotateTaskRuntime& runtime) {
    state.words[0u] = kVtable;
    runtime.DestroyBase(state);
    return state;
}

std::string_view UVRotateTask::ClassName() { return "UVRotateTask"; }

std::uint8_t UVRotateTask::Start(UVRotateTaskState& state,
                                 IUVRotateTaskRuntime& runtime) {
    const auto facet = runtime.ResolveFacet(state.words[16u], ClassName());
    state.words[23u] = facet;
    runtime.StartRotation(facet, static_cast<std::int32_t>(state.words[22u]));
    return 1u;
}

std::uint8_t UVRotateTask::Update(UVRotateTaskState& state, float elapsed,
                                  IUVRotateTaskRuntime& runtime) {
    const auto axis = std::array<float, 3u>{
        std::bit_cast<float>(state.words[18u]),
        std::bit_cast<float>(state.words[19u]),
        std::bit_cast<float>(state.words[20u])};
    const auto step = elapsed * std::bit_cast<float>(state.words[21u]);
    runtime.UpdateRotation(state.words[23u],
                           static_cast<std::int32_t>(state.words[22u]),
                           axis, step);
    return 1u;
}

std::uint8_t UVRotateTask::Stop(UVRotateTaskState& state,
                                IUVRotateTaskRuntime& runtime) {
    runtime.StopRotation(state.words[23u],
                         static_cast<std::int32_t>(state.words[22u]));
    state.words[23u] = 0u;
    return 1u;
}

bool UVRotateTask::LoadConfig(UVRotateTaskState& state, std::uint32_t config,
                              IUVRotateTaskConfigRuntime& runtime) {
    if (config == 0u)
        return false;
    constexpr std::array<std::string_view, 4u> kFloatKeys{
        "Attr_Task_UVRotate_Axis_X", "Attr_Task_UVRotate_Axis_Y",
        "Attr_Task_UVRotate_Axis_Z", "Attr_Task_UVROtate_Angel"};
    for (std::size_t i = 0u; i < kFloatKeys.size(); ++i)
        state.words[18u + i] = std::bit_cast<std::uint32_t>(
            runtime.ReadFloat(config, kFloatKeys[i], 0.0f));
    state.words[22u] = static_cast<std::uint32_t>(runtime.ReadInt(
        config, "Attr_Task_UVROtate_Texture_Layer", 0));
    return true;
}

}  // namespace pal4
