#include "pal4/uv_translate_task.h"

#include <bit>

namespace pal4 {

UVTranslateTaskState& UVTranslateTask::Construct(
    UVTranslateTaskState& state, IUVTranslateTaskRuntime& runtime) {
    runtime.ConstructBase(state);
    state.words[0u] = kVtable;
    for (std::size_t index = 18u; index <= 22u; ++index)
        state.words[index] = 0u;
    return state;
}

UVTranslateTaskState& UVTranslateTask::Destroy(
    UVTranslateTaskState& state, IUVTranslateTaskRuntime& runtime) {
    state.words[0u] = kVtable;
    runtime.DestroyBase(state);
    return state;
}

std::string_view UVTranslateTask::ClassName() {
    return "UVTranslateTask";
}

std::uint8_t UVTranslateTask::Start(UVTranslateTaskState& state,
                                    IUVTranslateTaskRuntime& runtime) {
    const auto facet = runtime.ResolveFacet(state.words[16u], ClassName());
    state.words[22u] = facet;
    runtime.StartTranslation(facet,
                             static_cast<std::int32_t>(state.words[21u]));
    return 1u;
}

std::uint8_t UVTranslateTask::Update(UVTranslateTaskState& state,
                                     float elapsed,
                                     IUVTranslateTaskRuntime& runtime) {
    const auto delta = std::array<float, 3u>{
        elapsed * std::bit_cast<float>(state.words[18u]),
        elapsed * std::bit_cast<float>(state.words[19u]),
        elapsed * std::bit_cast<float>(state.words[20u])};
    runtime.UpdateTranslation(state.words[22u],
        static_cast<std::int32_t>(state.words[21u]), delta);
    return 1u;
}

std::uint8_t UVTranslateTask::Stop(UVTranslateTaskState& state,
                                   IUVTranslateTaskRuntime& runtime) {
    runtime.StopTranslation(state.words[22u],
        static_cast<std::int32_t>(state.words[21u]));
    state.words[22u] = 0u;
    return 1u;
}

bool UVTranslateTask::LoadConfig(UVTranslateTaskState& state,
                                  std::uint32_t config,
                                  IUVTranslateTaskConfigRuntime& runtime) {
    if (config == 0u)
        return false;
    constexpr std::array<std::string_view, 3u> kKeys{
        "Attr_Task_UVTranslate_X", "Attr_Task_UVTranslate_Y",
        "Attr_Task_UVTranslate_Z"};
    for (std::size_t i = 0u; i < kKeys.size(); ++i)
        state.words[18u + i] = std::bit_cast<std::uint32_t>(
            runtime.ReadFloat(config, kKeys[i], 0.0f));
    state.words[21u] = static_cast<std::uint32_t>(runtime.ReadInt(
        config, "Attr_Task_UVTranslate_Texture_Layer", 0));
    return true;
}

}  // namespace pal4
