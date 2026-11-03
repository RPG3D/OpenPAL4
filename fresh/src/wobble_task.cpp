#include "pal4/wobble_task.h"

#include <bit>
#include <cmath>

namespace pal4 {

WobbleTaskState& WobbleTask::Construct(WobbleTaskState& state,
                                       IWobbleTaskRuntime& runtime) {
    runtime.ConstructBase(state);
    state.words[0u] = kVtable;
    state.words[20u] = 0u;
    return state;
}

WobbleTaskState& WobbleTask::Destroy(WobbleTaskState& state,
                                     IWobbleTaskRuntime& runtime) {
    state.words[0u] = kVtable;
    runtime.DestroyBase(state);
    return state;
}

std::string_view WobbleTask::ClassName() { return "WobbleTask"; }

std::uint8_t WobbleTask::Start(WobbleTaskState& state,
                               IWobbleTaskRuntime& runtime) {
    state.words[20u] = runtime.ResolvePositionFacet(
        state.words[16u], "PositionTask");
    return 1u;
}

std::uint8_t WobbleTask::Update(WobbleTaskState& state,
                                IWobbleTaskRuntime& runtime) {
    const auto frequency = std::bit_cast<float>(state.words[18u]);
    const auto range = std::bit_cast<float>(state.words[19u]);
    const auto phase = static_cast<double>(runtime.TickCountMilliseconds()) *
                       static_cast<double>(frequency) * 0.001;
    const auto displacement = static_cast<float>(std::sin(phase) * range);
    runtime.MovePosition(state.words[20u], 0.0f, displacement, 0.0f, true);
    return 1u;
}

std::uint8_t WobbleTask::Stop(WobbleTaskState& state) {
    state.words[20u] = 0u;
    return 1u;
}

bool WobbleTask::LoadConfig(WobbleTaskState& state, std::uint32_t config,
                            IWobbleTaskConfigRuntime& runtime) {
    if (config == 0u)
        return false;
    const auto inherit = runtime.ReadInt(
        config, "Attr_Inherit_Pre_Task", 0) > 0;
    const auto shift = 8u * (61u % 4u);
    auto& flag_word = state.words[61u / 4u];
    flag_word = (flag_word & ~(0xffu << shift)) |
                (std::uint32_t{inherit} << shift);
    state.words[19u] = std::bit_cast<std::uint32_t>(runtime.ReadFloat(
        config, "Attr_Task_Wobble_Range", 0.0f));
    const auto frequency = runtime.ReadFloat(
        config, "Attr_Task_Wobble_Fps", 0.0f);
    state.words[18u] = std::bit_cast<std::uint32_t>(
        frequency > 0.0f ? frequency : 10.0f);
    return true;
}

}  // namespace pal4
