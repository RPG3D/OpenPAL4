#include "pal4/ui_frame_transition_state.h"

namespace pal4 {

std::uint32_t UiFrameTransitionState::Construct(
    std::uint32_t object, IUiFrameTransitionRuntime& runtime) {
    runtime.WriteWord(object, kVtable);
    runtime.WriteWord(object + 4u, 0u);
    return object;
}

std::uint32_t UiFrameTransitionState::LoadConfig(
    std::uint32_t object, std::uint32_t,
    std::uint32_t config, IUiFrameTransitionRuntime& runtime) {
    runtime.SetUIFlag(runtime.UIFrameManager(), 1u);
    runtime.WriteWord(runtime.Console(), 0u);
    runtime.WriteWord(object + 4u,
                      runtime.ReadIntegerAttribute(config, "frame").value_or(0u));
    return 1u;
}

std::uint8_t UiFrameTransitionState::Update(
    std::uint32_t object, IUiFrameTransitionRuntime& runtime) {
    const auto frame = runtime.ReadWord(object + 4u);
    if (frame != 0u) {
        const auto visible = runtime.IsDialogVisible(frame);
        if (visible != 0u) return visible;
    }
    return runtime.InitializeStateAndUI(runtime.GameInstance(), 1u);
}

std::uint32_t UiFrameTransitionState::Reset(
    std::uint32_t object, std::uint32_t,
    IUiFrameTransitionRuntime& runtime) {
    runtime.WriteWord(object + 4u, 0u);
    return 1u;
}

}  // namespace pal4
