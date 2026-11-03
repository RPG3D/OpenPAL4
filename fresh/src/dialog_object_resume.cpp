#include "pal4/dialog_object.h"

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
void ReleaseContainer(DialogObjectState& state,
                      IDialogObjectResumeRuntime& runtime) {
    if (state.words[2u] != 0u) {
        runtime.DestroyTaskContainer(state.words[2u]);
        runtime.Free(state.words[2u]);
        state.words[2u] = 0u;
    }
}
void StopAndReleaseFrame(DialogObjectState& state,
                         IDialogObjectResumeRuntime& runtime) {
    runtime.StopEffects(state);
    if (state.words[43u] != 0u) {
        runtime.DestroyFrame(state.words[43u]);
        state.words[43u] = 0u;
    }
    ReleaseContainer(state, runtime);
}
}  // namespace

bool DialogObject::ResumePlayback(
    DialogObjectState& state, std::uint32_t object,
    IDialogObjectResumeRuntime& runtime) {
    if (ReadByte(state, 4u) == 0u || state.words[3u] == 0u)
        return false;
    runtime.ResetBeforePlayback(state);
    ReleaseContainer(state, runtime);
    const auto memory = runtime.Allocate(0x10u);
    state.words[2u] = memory != 0u
        ? runtime.InitializeTaskContainer(memory) : 0u;

    for (auto record = state.words[28u]; record != state.words[29u];
         record += 20u) {
        const auto effect = runtime.ReadEffectHandle(record);
        if (effect == 0u) {
            runtime.StopEffects(state);
            runtime.ResetTaskSlots(state);
            runtime.DestroyFrame(state.words[43u]);
            state.words[43u] = 0u;
            ReleaseContainer(state, runtime);
            WriteByte(state, 80u, 0u);
            WriteByte(state, 4u, 0u);
            WriteByte(state, 81u, 0u);
            return false;
        }
        runtime.SetEffectMode(effect, 0u);
        const auto enabled = runtime.EffectsEnabled();
        if (!enabled && runtime.EffectIsActive(effect))
            continue;

        runtime.WriteEffectWord(effect, 16u, state.words[3u]);
        runtime.WriteEffectWord(effect, 12u, object);
        if (!runtime.RestartEffect(effect)) {
            if (enabled)
                ReleaseContainer(state, runtime);
            else
                StopAndReleaseFrame(state, runtime);
            return false;
        }
        runtime.ProcessEffectEvent(state.words[3u], effect);
        if (runtime.EffectFrame(effect) == 0u) {
            if (enabled)
                ReleaseContainer(state, runtime);
            else
                StopAndReleaseFrame(state, runtime);
            return false;
        }
        runtime.AddFrameChild(state.words[43u],
                              runtime.EffectFrame(effect));
    }

    if (!runtime.InitializeTasks(state)) {
        runtime.StopEffects(state);
        runtime.ResetTaskSlots(state);
        runtime.DestroyFrame(state.words[43u]);
        state.words[43u] = 0u;
        ReleaseContainer(state, runtime);
        return false;
    }
    WriteByte(state, 80u, 1u);
    WriteByte(state, 81u, 0u);
    runtime.RefreshFrame(state);
    return true;
}

}  // namespace pal4
