#include "pal4/effect_camera_lifecycle.h"

#include <initializer_list>

namespace pal4 {
namespace {

std::uint8_t ReadByte(const EffectCameraObjectState& state,
                      const std::uint32_t offset) {
    return static_cast<std::uint8_t>(
        state.words[offset / 4u] >> (8u * (offset % 4u)));
}

void WriteByte(EffectCameraObjectState& state, const std::uint32_t offset,
               const std::uint8_t value) {
    const auto shift = 8u * (offset % 4u);
    const auto mask = 0xffu << shift;
    state.words[offset / 4u] =
        (state.words[offset / 4u] & ~mask) |
        (static_cast<std::uint32_t>(value) << shift);
}

}  // namespace

std::uint32_t EffectCameraLifecycle::Stop(
    EffectCameraObjectState& state,
    IEffectCameraLifecycleRuntime& runtime) {
    if (ReadByte(state, 4u) != 0u) {
        runtime.StopFacet(state, 24u);
        for (const auto offset : {4u, 6u, 7u, 44u, 47u})
            WriteByte(state, offset, 0u);
        for (std::uint32_t index = 22u; index <= 25u; ++index)
            state.words[index] = 0u;
        state.words[26u] = 0x3f800000u;
        state.words[27u] = 0x3f800000u;
        state.words[12u] = 0u;
        state.words[13u] = 0x3f800000u;
        state.words[14u] = 0x3f800000u;
        if (state.words[32u] != 0u) {
            runtime.DestroyFrame(state.words[32u]);
            state.words[32u] = 0u;
        }
        if (state.words[31u] != 0u) {
            runtime.DestroyFrame(state.words[31u]);
            state.words[31u] = 0u;
        }
        runtime.ProcessEvent2(state);
    }
    return runtime.Finalize(state, 1u);
}

std::uint8_t EffectCameraLifecycle::Update(
    EffectCameraObjectState& state,
    IEffectCameraLifecycleRuntime& runtime) {
    if (ReadByte(state, 4u) == 0u || ReadByte(state, 6u) == 0u)
        return 0u;
    static_cast<void>(runtime.UpdateTransform(state));
    const auto target_frame = runtime.TargetFrame(state);
    if (ReadByte(state, 44u) != 0u) {
        const std::array<std::uint32_t, 3> position{
            state.words[25u], state.words[26u], state.words[27u]};
        runtime.TranslateFrame(target_frame, position, 2u);
        state.words[25u] = 0u;
        state.words[26u] = 0x3f800000u;
        state.words[27u] = 0x3f800000u;
        return 0u;
    }
    runtime.SetFrameIdentity(target_frame);
    const auto transform = runtime.ObjectTransform(state.words[31u]);
    runtime.ApplyTransform(target_frame, transform, 0u);
    return runtime.UpdateFrameObjects(target_frame);
}

std::uint8_t EffectCameraLifecycle::Start(
    EffectCameraObjectState& state,
    IEffectCameraLifecycleRuntime& runtime) {
    if (ReadByte(state, 4u) == 0u)
        return 0u;
    const auto running = ReadByte(state, 6u);
    if (running != 0u)
        return running;
    runtime.ProcessEvent2(state);
    const auto ready = runtime.ContextReady(state);
    WriteByte(state, 80u, ready ? 1u : 0u);
    if (ready) {
        state.words[32u] = runtime.CloneHierarchy(runtime.TargetFrame(state));
        WriteByte(state, 6u, 1u);
        WriteByte(state, 7u, 0u);
        runtime.ProcessEvent(state);
    }
    const auto inherit = ReadByte(state, 44u);
    return inherit == 0u ? runtime.UpdateTransform(state) : inherit;
}

void EffectCameraLifecycle::Pause(
    EffectCameraObjectState& state,
    IEffectCameraLifecycleRuntime& runtime) {
    if (ReadByte(state, 4u) == 0u || ReadByte(state, 6u) == 0u ||
        ReadByte(state, 80u) == 0u)
        return;
    WriteByte(state, 6u, 0u);
    WriteByte(state, 7u, 0u);
    const auto clone = state.words[32u];
    if (runtime.HasTargetObject(state) && clone != 0u) {
        const auto target_frame = runtime.TargetFrame(state);
        runtime.SetFrameIdentity(target_frame);
        runtime.ApplyTransform(target_frame, clone + 16u, 0u);
        static_cast<void>(runtime.UpdateFrameObjects(target_frame));
    }
    if (clone != 0u) {
        runtime.DestroyFrame(clone);
        state.words[32u] = 0u;
    }
    WriteByte(state, 80u, 0u);
}

}  // namespace pal4
