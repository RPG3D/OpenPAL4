#include "pal4/message_task.h"

namespace pal4 {

std::uint8_t MessageTask::Start(MessageTaskState& state,
                                IMessageTaskRuntime& runtime) {
    const auto facet = runtime.ResolveFacet(state.words[16u], "MessageTask");
    state.words[22u] = facet;
    const auto id = runtime.ReadConfiguredId(state.words[16u], "ID", -1);
    const auto text = state.words[19u] != 0u
                          ? state.words[19u]
                          : kEmptyTextAddress;
    runtime.Send(facet, id, text);
    return 1u;
}

std::uint8_t MessageTask::Reset(MessageTaskState& state) {
    state.words[22u] = 0u;
    return 1u;
}

}  // namespace pal4
