#include "pal4/effect_object_update.h"

namespace pal4 {
namespace {

std::uint8_t GetByte(const EffectObjectState& state,
                     const std::uint32_t offset) {
    return static_cast<std::uint8_t>(state.words[offset / 4u] >>
                                     ((offset % 4u) * 8u));
}

void SetByte(EffectObjectState& state, const std::uint32_t offset,
             const std::uint8_t value) {
    const auto shift = (offset % 4u) * 8u;
    const auto mask = 0xffu << shift;
    auto& word = state.words[offset / 4u];
    word = (word & ~mask) | (static_cast<std::uint32_t>(value) << shift);
}

std::uint32_t ReleaseFailedStart(EffectObjectState& state,
                                 const std::uint32_t context,
                                 const bool clear_flags_early,
                                 IEffectObjectUpdateRuntime& runtime) {
    const auto instance = state.words[23u];
    runtime.CleanupInstance(instance, context);
    runtime.DestroyInstance(instance, context);
    const auto frame = state.words[24u];
    state.words[23u] = 0u;
    if (clear_flags_early) {
        SetByte(state, 6u, 0u);
        SetByte(state, 7u, 0u);
    }
    auto result = frame;
    if (runtime.ReadWord(frame + 4u) != 0u)
        runtime.RemoveFrameChild(frame);
    if (frame != 0u) {
        result = runtime.DestroyFrame(frame);
        state.words[24u] = 0u;
    }
    SetByte(state, 4u, 0u);
    if (!clear_flags_early) {
        SetByte(state, 6u, 0u);
        SetByte(state, 7u, 0u);
    }
    return result;
}

}  // namespace

std::uint32_t EffectObjectUpdater::Update(
    EffectObjectState& state, const double elapsed,
    const std::uint32_t entry_eax, const std::uint32_t context,
    IEffectObjectUpdateRuntime& runtime) {
    if (GetByte(state, 4u) == 0u || GetByte(state, 6u) == 0u ||
        GetByte(state, 11u) != 0u)
        return entry_eax;

    const auto instance = state.words[23u];
    if (runtime.ReadWord(instance + 8u) != 0u)
        return runtime.Advance(instance, static_cast<float>(elapsed));

    if (GetByte(state, 88u) == 0u) {
        SetByte(state, 6u, 0u);
        SetByte(state, 88u, 0u);
        SetByte(state, 7u, 0u);
        return entry_eax;
    }

    runtime.WriteWord(instance + 12u, state.words[4u]);
    runtime.WriteByte(instance + 4u, 1u);
    runtime.WriteByte(instance + 80u, 0u);
    if ((runtime.Prepare(instance) & 0xffu) == 0u)
        return ReleaseFailedStart(state, context, true, runtime);

    const auto child = runtime.ReadWord(instance + 172u);
    if (child != 0u)
        runtime.AddFrameChild(state.words[24u], child);
    const auto owner = state.words[3u];
    if (owner != 0u)
        runtime.WriteByte(instance + 88u, runtime.ReadByte(owner + 88u));
    const auto result = runtime.Activate(instance);
    if ((result & 0xffu) == 0u)
        return ReleaseFailedStart(state, context, false, runtime);
    SetByte(state, 6u, 1u);
    SetByte(state, 7u, 1u);
    return result;
}

void EffectObjectUpdater::Start(
    EffectObjectState& state, const std::uint8_t mode,
    const std::uint32_t context, IEffectObjectUpdateRuntime& runtime) {
    if (GetByte(state, 4u) == 0u || GetByte(state, 6u) != 0u)
        return;

    const auto instance = state.words[23u];
    SetByte(state, 6u, 1u);
    SetByte(state, 7u, 1u);
    SetByte(state, 88u, mode);
    if (runtime.ReadWord(instance + 8u) == 0u &&
        (runtime.Prepare(instance) & 0xffu) == 0u) {
        (void)ReleaseFailedStart(state, context, true, runtime);
        return;
    }

    const auto child = runtime.ReadWord(instance + 172u);
    if (child != 0u)
        runtime.AddFrameChild(state.words[24u], child);
    const auto owner = state.words[3u];
    if (owner != 0u)
        runtime.WriteByte(instance + 88u, runtime.ReadByte(owner + 88u));
    if ((runtime.Activate(instance) & 0xffu) == 0u)
        (void)ReleaseFailedStart(state, context, true, runtime);
}

}  // namespace pal4
