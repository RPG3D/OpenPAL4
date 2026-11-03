#include "pal4/decress_task.h"

#include <bit>

namespace pal4 {

DecressTaskState& DecressTask::Construct(DecressTaskState& state,
                                        IDecressTaskRuntime& runtime) {
    runtime.ConstructBase(state);
    state.words[0u] = kVtable;
    state.words[18u] = 0u;
    state.words[19u] = (state.words[19u] & 0xffff0000u) | 1u;
    state.words[20u] = std::bit_cast<std::uint32_t>(1.0f);
    return state;
}

std::string_view DecressTask::ClassName() {
    return "DecressTask";
}

DecressTaskState& DecressTask::Destroy(DecressTaskState& state,
                                      IDecressTaskRuntime& runtime) {
    state.words[0u] = kVtable;
    runtime.DestroyBase(state);
    return state;
}

std::uint8_t DecressTask::Start(
    DecressTaskState& state, IDecressTaskFacetRuntime& runtime) {
    const auto facet = runtime.ResolveFacet(state.words[16u], ClassName());
    state.words[18u] = facet;
    if ((state.words[19u] & 0xffu) != 0u) {
        runtime.StartFacet(facet, std::bit_cast<float>(state.words[20u]),
                           static_cast<std::uint8_t>(state.words[19u] >> 8u));
    } else {
        runtime.StopFacet(facet);
    }
    return 1u;
}

std::uint8_t DecressTask::Reset(DecressTaskState& state) {
    state.words[18u] = 0u;
    return 1u;
}

bool DecressTask::LoadConfig(DecressTaskState& state,
                             const std::uint32_t config,
                             IDecressTaskConfigRuntime& runtime) {
    if (config == 0u)
        return false;

    const auto decress = runtime.ReadInt(
        config, "Attr_Task_Decress_Decress", 0);
    if (decress > 0) {
        state.words[19u] = (state.words[19u] & 0xffffff00u) | 1u;
        const auto increase = runtime.ReadInt(
            config, "Attr_Task_Decress_Incress", 0);
        state.words[19u] = (state.words[19u] & 0xffff00ffu) |
                           (static_cast<std::uint32_t>(increase > 0) << 8u);
        state.words[20u] = std::bit_cast<std::uint32_t>(runtime.ReadFloat(
            config, "Attr_Task_Decress_Time", 1.0f));
    } else {
        state.words[19u] &= 0xffff0000u;
        state.words[20u] = 0u;
    }

    const auto inherit = runtime.ReadInt(
        config, "Attr_Inherit_Pre_Task", 0);
    state.words[15u] = (state.words[15u] & 0xffff00ffu) |
                       (static_cast<std::uint32_t>(inherit > 0) << 8u);
    return true;
}

}  // namespace pal4
