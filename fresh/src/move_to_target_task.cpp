#include "pal4/move_to_target_task.h"

#include <bit>

namespace pal4 {
namespace {

std::array<std::uint32_t, 3u> Triplet(const MoveToTargetTaskState& state,
                                      const std::size_t first) {
    return {state.words[first], state.words[first + 1u],
            state.words[first + 2u]};
}

}  // namespace

MoveToTargetTaskState& MoveToTargetTask::Construct(
    MoveToTargetTaskState& state, const std::uint8_t stack_byte,
    IMoveToTargetTaskRuntime& runtime) {
    runtime.ConstructBase(state);
    state.words[27u] = (state.words[27u] & 0xffffff00u) | stack_byte;
    for (std::size_t index = 28u; index <= 31u; ++index)
        state.words[index] = 0u;
    state.words[0u] = kVtable;
    return state;
}

std::string_view MoveToTargetTask::ClassName() {
    return "MoveToTargetTask";
}

MoveToTargetTaskState& MoveToTargetTask::Destroy(
    MoveToTargetTaskState& state, IMoveToTargetTaskRuntime& runtime) {
    state.words[0u] = kVtable;
    const auto data = state.words[28u];
    if (data != 0u) {
        const auto count = runtime.RefCount(data);
        if (count == 0u || count == 0xffu)
            runtime.Free(data - 1u);
        else
            runtime.SetRefCount(data, static_cast<std::uint8_t>(count - 1u));
    }
    state.words[28u] = 0u;
    state.words[29u] = 0u;
    state.words[30u] = 0u;
    runtime.DestroyBase(state);
    return state;
}

std::uint8_t MoveToTargetTask::Start(
    MoveToTargetTaskState& state, IMoveToTargetTaskRuntime& runtime) {
    state.words[33u] = 0u;
    std::uint32_t facet = 0u;
    if (runtime.ResolveFacet(state.words[16u], "LocalModelTask", facet)) {
        state.words[33u] = facet;
        const auto old_flag = runtime.ReadFacetFlag(facet);
        state.words[32u] = (state.words[32u] & 0xffffff00u) | old_flag;
        runtime.WriteFacetFlag(facet, 0u);
    }
    return 1u;
}

std::uint8_t MoveToTargetTask::Update(
    MoveToTargetTaskState& state, const float delta,
    IMoveToTargetTaskRuntime& runtime) {
    const auto facet = state.words[31u];
    runtime.SetTarget(facet, Triplet(state, 21u));
    runtime.SetOrigin(facet, Triplet(state, 24u));
    runtime.Enable(facet, true);
    const std::array<std::uint32_t, 3u> movement{
        std::bit_cast<std::uint32_t>(std::bit_cast<float>(state.words[18u]) * delta),
        std::bit_cast<std::uint32_t>(std::bit_cast<float>(state.words[19u]) * delta),
        std::bit_cast<std::uint32_t>(std::bit_cast<float>(state.words[20u]) * delta)};
    (void)runtime.SetPosition(facet, movement, true);
    return 1u;
}

std::uint8_t MoveToTargetTask::Stop(
    MoveToTargetTaskState& state, IMoveToTargetTaskRuntime& runtime) {
    const auto facet = state.words[31u];
    (void)runtime.SetPosition(facet, Triplet(state, 21u), false);
    runtime.Enable(facet, false);
    const auto local_model = state.words[33u];
    state.words[31u] = 0u;
    if (local_model != 0u) {
        runtime.WriteFacetFlag(local_model,
            static_cast<std::uint8_t>(state.words[32u]));
        state.words[33u] = 0u;
    }
    return 1u;
}

}  // namespace pal4
