#include "pal4/save_ui_state.h"

namespace pal4 {

std::uint32_t SaveUiState::Construct(
    std::uint32_t object, ISaveUiConstructionRuntime& runtime) {
    runtime.WriteWord(object, kVtable);
    return object;
}

std::uint32_t SaveUiState::Enter(
    std::uint32_t, std::uint32_t, ISaveUiEntryRuntime& runtime) {
    runtime.SetUIFlag(runtime.UIFrameManager(), 1u);
    runtime.WriteWord(runtime.Console(), 0u);
    runtime.DispatchEvent(runtime.UIFrameManager(), kShowSaveUiEvent, 1u, 0u);
    return 1u;
}

std::uint32_t SaveUiState::Render(
    std::uint32_t frame, IWorldUiFrameRuntime& runtime) {
    const auto value = runtime.ReadWord(frame);
    runtime.ProcessWorldFrame(runtime.ArenaManager(), value);
    return runtime.RenderUI(runtime.UIFrameManager(), frame);
}

std::uint8_t SaveUiState::Update(ISaveUiUpdateRuntime& runtime) {
    const auto frame = runtime.FindFrame(runtime.UIFrameManager(), "SaveWindow");
    if (frame == 0u) return 0u;
    const auto visible = runtime.IsVisible(frame);
    if (visible != 0u) return visible;
    return runtime.InitializeStateAndUI(runtime.GameInstance(), 1u);
}

std::uint32_t SaveUiState::Finish(
    std::uint32_t, ISaveUiExitRuntime& runtime) {
    for (std::uint32_t index = 0; index < 4u; ++index) {
        const auto player = runtime.PlayerByIndex(runtime.PlayerManager(), index);
        if (player == 0u || runtime.ReadWord(player + 2824u) == 0u)
            continue;
        const auto component = player + 1620u;
        const auto first = runtime.GetComponentProperty(component, 0u);
        runtime.SetComponentProperty(component, 7u, first);
        const auto second = runtime.GetComponentProperty(component, 2u);
        runtime.SetComponentProperty(component, 9u, second);
    }
    const auto manager = runtime.PlayerManager();
    const auto selected = manager +
        2836u * runtime.ReadWord(manager + 11344u);
    if (selected != 0u) {
        const auto second_manager = runtime.PlayerManager();
        const auto effects = runtime.EffectManager();
        const auto position = second_manager +
            2836u * runtime.ReadWord(second_manager + 11344u) + 172u;
        runtime.CreateEffect(effects, "H_XS027", position, 1u, 0u);
        runtime.ResetTemporaryEffectHandle();
    }
    return 1u;
}

}  // namespace pal4
