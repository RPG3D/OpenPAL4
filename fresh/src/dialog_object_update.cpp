#include "pal4/dialog_object.h"

#include <cstddef>

namespace pal4 {
namespace {
std::uint8_t ReadByte(const DialogObjectState& state, std::size_t offset) {
    return static_cast<std::uint8_t>(
        state.words[offset / 4u] >> (8u * (offset % 4u)));
}
}  // namespace

int DialogObject::UpdatePlayback(
    DialogObjectState& state, float elapsed,
    IDialogObjectUpdateRuntime& runtime) {
    const auto stopped = ReadByte(state, 82u);
    if (stopped != 0u)
        return stopped;
    if (ReadByte(state, 81u) == 0u || state.words[2u] == 0u)
        return 0;

    const auto update_result = runtime.UpdateTaskContainer(
        state.words[2u], elapsed);
    if (runtime.TaskContainerReady(state.words[2u]))
        return runtime.ResetRuntimeState(state);
    if (ReadByte(state, 212u) != 0u) {
        const auto position = runtime.ObjectPosition(state.words[54u]);
        return runtime.SetAbsolutePosition(state, position);
    }
    return update_result & ~0xff;
}

}  // namespace pal4
