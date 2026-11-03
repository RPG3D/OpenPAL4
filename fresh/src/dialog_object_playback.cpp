#include "pal4/dialog_object.h"

#include <cstddef>

namespace pal4 {
namespace {
std::uint8_t ReadByte(const DialogObjectState& state, std::size_t offset) {
    return static_cast<std::uint8_t>(
        state.words[offset / 4u] >> (8u * (offset % 4u)));
}
void WriteByte(DialogObjectState& state, std::size_t offset,
               std::uint8_t value) {
    const auto shift = static_cast<unsigned>(8u * (offset % 4u));
    auto& word = state.words[offset / 4u];
    word = (word & ~(0xffu << shift)) | (std::uint32_t{value} << shift);
}
void ReleasePlaybackResources(DialogObjectState& state,
                              IDialogObjectPlaybackRuntime& runtime) {
    if (state.words[43u] != 0u) {
        runtime.DestroyFrame(state.words[43u]);
        state.words[43u] = 0u;
    }
    if (state.words[2u] != 0u) {
        runtime.DestroyTaskContainer(state.words[2u]);
        runtime.Free(state.words[2u]);
        state.words[2u] = 0u;
    }
}
}  // namespace

bool DialogObject::PreparePlayback(
    DialogObjectState& state, IDialogObjectPlaybackRuntime& runtime) {
    if (ReadByte(state, 4u) != 0u || state.words[3u] == 0u)
        return false;
    if (ReadByte(state, 80u) != 0u)
        return true;

    runtime.ResetBeforePlayback(state);
    const auto task_begin = state.words[24u];
    if (task_begin == 0u ||
        ((state.words[25u] - task_begin) >> 3u) == 0u)
        return false;
    const auto effect_begin = state.words[28u];
    if (effect_begin == 0u ||
        (state.words[29u] - effect_begin) / 20u == 0u)
        return false;

    if (state.words[43u] != 0u) {
        runtime.DestroyFrame(state.words[43u]);
        state.words[43u] = 0u;
    }
    state.words[43u] = runtime.CreateFrame();
    runtime.RefreshFrame(state);

    if (state.words[2u] != 0u) {
        runtime.DestroyTaskContainer(state.words[2u]);
        runtime.Free(state.words[2u]);
        state.words[2u] = 0u;
    }
    const auto memory = runtime.Allocate(0x10u);
    state.words[2u] = memory != 0u
        ? runtime.InitializeTaskContainer(memory) : 0u;

    if (!runtime.ActivateEffects(state)) {
        runtime.StopEffects(state);
        ReleasePlaybackResources(state, runtime);
        return false;
    }
    if (!runtime.InitializeTasks(state)) {
        runtime.StopEffects(state);
        runtime.ResetTaskSlots(state);
        ReleasePlaybackResources(state, runtime);
        return false;
    }
    WriteByte(state, 80u, 1u);
    WriteByte(state, 81u, 0u);
    return true;
}

}  // namespace pal4
