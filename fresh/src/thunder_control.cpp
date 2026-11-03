#include "pal4/thunder_control.h"

namespace pal4 {
namespace {

void StoreByte(ThunderObjectState& state, const std::uint32_t offset,
               const std::uint8_t value) {
    const auto shift = (offset % 4u) * 8u;
    auto& word = state.words[offset / 4u];
    word = (word & ~(0xffu << shift)) |
           (static_cast<std::uint32_t>(value) << shift);
}

void CopyTriple(ThunderObjectState& state, const std::uint32_t source,
                const std::uint32_t destination) {
    for (std::uint32_t i = 0u; i < 3u; ++i)
        state.words[destination / 4u + i] = state.words[source / 4u + i];
}

}  // namespace

std::uint8_t ThunderControl::Reset(
    ThunderObjectState& state, IThunderResetRuntime& runtime) {
    if ((state.words[1u] & 0xffu) == 0u)
        return 0u;
    if ((state.words[58u] & 0xffu) == 0u && state.words[3u] != 0u)
        runtime.RefreshTarget(state, state.words[3u]);
    CopyTriple(state, 244u, 72u);
    runtime.UpdateFrame(state);
    CopyTriple(state, 256u, 84u);
    runtime.UpdateFrame(state);
    CopyTriple(state, 268u, 96u);
    runtime.UpdateFrame(state);
    state.words[27u] = state.words[60u];
    StoreByte(state, 11u, 0u);
    CopyTriple(state, 280u, 148u);
    return 1u;
}

std::uint32_t ThunderControl::Stop(
    ThunderObjectState& state, IThunderStopRuntime& runtime) {
    if ((state.words[1u] & 0xffu) != 0u) {
        runtime.ResetVisual(state);
        StoreByte(state, 4u, 0u);
        StoreByte(state, 6u, 0u);
        StoreByte(state, 7u, 0u);
        runtime.ClearTargetName(state);
        StoreByte(state, 232u, 0u);
        StoreByte(state, 56u, 0u);
        runtime.ProcessEffectEvent(state.words[4u], state);
    }
    return runtime.DeleteObject(state, 1u);
}

}  // namespace pal4
