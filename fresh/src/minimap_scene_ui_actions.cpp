#include "pal4/minimap_scene_ui_actions.h"

#include <bit>

namespace pal4 {

std::uint8_t MinimapSceneUiActions::SwitchToSystem(
    IMinimapSceneUiActionsRuntime& runtime) {
    runtime.InitializeStateAndUi(runtime.Game(), 2u, 0u);
    return 1u;
}

std::uint8_t MinimapSceneUiActions::SystemClick(
    const std::uint32_t ignored_event,
    IMinimapSceneUiActionsRuntime& runtime) {
    (void)ignored_event;
    (void)SwitchToSystem(runtime);
    runtime.ProcessEvent(runtime.FrameManager() + 0x104u,
                         kSystemEventKeyCell, {1u, 0u});
    return 1u;
}

std::uint8_t MinimapSceneUiActions::HelpClick(
    const std::uint32_t ignored_event,
    IMinimapSceneUiHelpRuntime& runtime) {
    (void)ignored_event;
    const auto value = runtime.ReadWord(runtime.FrameManager() + 0x144u);
    runtime.ProcessEvent(runtime.FrameManager() + 0x104u,
                         kHelpEventKeyCell, {1u, value});
    const auto document = runtime.InitializeDocument();
    try {
        const auto frame = runtime.FindFrame(runtime.FrameManager(),
                                             kHelpFrameKeyCell);
        runtime.BeginFrameName("frame");
        MinimapSceneUiAttributeLookup attribute;
        try {
            attribute = runtime.FindAttribute(document, "frame");
        } catch (...) {
            runtime.EndFrameName();
            throw;
        }
        runtime.EndFrameName();
        if (attribute.position == attribute.end ||
            attribute.position == 0xffffffe4u) {
            const auto created = runtime.CreateFrameAttribute("frame", frame);
            try {
                runtime.AddAttribute(document, created);
            } catch (...) {
                runtime.DestroyAttribute(created);
                throw;
            }
            runtime.DestroyAttribute(created);
        } else {
            runtime.SetExistingAttribute(attribute.position + 0x1cu, frame);
        }
        runtime.InitializeStateAndUi(runtime.Game(), 10u, document);
    } catch (...) {
        runtime.DestroyDocument(document);
        throw;
    }
    runtime.DestroyDocument(document);
    return 1u;
}

std::uint8_t MinimapSceneUiActions::ToggleState(
    MinimapSceneUiState& state, const std::uint32_t owner,
    const std::uint32_t ignored_event,
    IMinimapSceneUiToggleRuntime& runtime) {
    (void)ignored_event;
    (void)SetStateAndPlaySound(
        state, owner, static_cast<std::uint8_t>(state.ReadByte(0xf5u) == 0u),
        runtime);
    return 1u;
}

std::uint32_t MinimapSceneUiActions::SetStateAndPlaySound(
    MinimapSceneUiState& state, const std::uint32_t owner,
    const std::uint8_t enabled,
    IMinimapSceneUiToggleRuntime& runtime) {
    state.WriteByte(0xf5u, enabled);
    runtime.PlayMusic("WA22.wav", 1u);
    return UpdateVisibilityAndMinimapState(state, owner, runtime);
}

std::uint32_t MinimapSceneUiActions::SetFlagAndUpdate(
    MinimapSceneUiState& state, const std::uint32_t owner,
    const std::uint8_t value,
    IMinimapSceneUiMapVisibilityRuntime& runtime) {
    state.WriteByte(0xf4u, value);
    return UpdateVisibilityAndMinimapState(state, owner, runtime);
}

std::uint32_t MinimapSceneUiActions::UpdateVisibilityAndMinimapState(
    const MinimapSceneUiState& state, const std::uint32_t owner,
    IMinimapSceneUiMapVisibilityRuntime& runtime) {
    const bool visible =
        (state.ReadByte(0xf5u) &
         static_cast<std::uint8_t>(state.ReadByte(0xf4u) == 0u)) != 0u;
    runtime.SetWindowVisible(state.ReadWord(0x54u), visible);
    const auto minimap_enabled = visible && runtime.DialogIsVisible(owner);
    const auto manager = runtime.MinimapManager();
    runtime.WriteByte(manager + 0x54u,
                      static_cast<std::uint8_t>(minimap_enabled));
    return manager;
}

void MinimapSceneUiActions::AdvanceRunMode(
    const std::uint32_t owner, const std::uint32_t context,
    IMinimapSceneUiRunModeRuntime& runtime) {
    const auto first = runtime.ActorControl();
    const auto incremented =
        static_cast<std::uint32_t>(runtime.ActorControlMode(first)) + 1u;
    auto next = std::bit_cast<std::int32_t>(incremented);
    if (next >= 3) next = 0;
    runtime.SetActorControlMode(runtime.ActorControl(), context, next);
    runtime.UpdateCharacterMovementState(owner);
}

std::uint8_t MinimapSceneUiActions::RunClick(
    const std::uint32_t owner, const std::uint32_t context,
    const std::uint32_t ignored_event,
    IMinimapSceneUiActionsRuntime& runtime) {
    (void)ignored_event;
    AdvanceRunMode(owner, context, runtime);
    return 1u;
}

std::uint8_t MinimapSceneUiActions::SetFlagTrue(
    MinimapSceneUiState& state, const std::uint32_t ignored_event) {
    (void)ignored_event;
    state.WriteByte(0xf6u, 1u);
    return 1u;
}

std::uint8_t MinimapSceneUiActions::SetFlagFalseAndResetInput(
    MinimapSceneUiState& state, const std::uint32_t ignored_event,
    IMinimapSceneUiActionsRuntime& runtime) {
    (void)ignored_event;
    state.WriteByte(0xf6u, 0u);
    runtime.ResetInputState(runtime.InputManager());
    return 1u;
}

}  // namespace pal4
