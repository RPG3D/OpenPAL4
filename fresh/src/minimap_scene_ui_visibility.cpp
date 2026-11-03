#include "pal4/minimap_scene_ui_visibility.h"

namespace pal4 {
namespace {

void RemoveTimer(const bool exception_message,
                 IMinimapSceneUiVisibilityRuntime& runtime) {
    runtime.BeginTimerName(
        MinimapSceneUiVisibility::kTimerName, exception_message);
    try {
        const auto manager = runtime.FrameManager();
        const auto queue = runtime.ReadWord(manager + 0x1a0u);
        runtime.RemoveTimerByName(
            queue, MinimapSceneUiVisibility::kTimerName);
    } catch (...) {
        runtime.EndTimerName();
        throw;
    }
    runtime.EndTimerName();
}

void SelectPlayer(const std::uint32_t owner,
                  IMinimapSceneUiPlayerSelectionRuntime& runtime) {
    const auto manager = runtime.PlayerManager();
    const auto current_index = runtime.ReadWord(manager + 0x2c50u);
    const auto current_player = manager + 2836u * current_index;
    for (std::uint32_t index = 0u; index < 4u; ++index) {
        const auto candidate = runtime.PlayerByIndex(
            runtime.PlayerManager(), index);
        if (candidate != 0u &&
            runtime.ReadWord(candidate + 0xb08u) != 0u &&
            !runtime.PlayerStateBlocked(candidate + 0x654u) &&
            candidate != current_player) {
            runtime.UpdatePlayerStates(owner, candidate);
            break;
        }
    }
    runtime.UpdatePlayerAndGameStates(owner);
}

void AddTimer(const std::uint32_t owner,
              IMinimapSceneUiVisibilityRuntime& runtime) {
    runtime.BeginTimerName(MinimapSceneUiVisibility::kTimerName, false);
    try {
        const auto manager = runtime.FrameManager();
        const auto queue = runtime.ReadWord(manager + 0x1a0u);
        const auto storage = runtime.Allocate(0x10u);
        if (storage != 0u) {
            runtime.WriteWord(storage, MinimapSceneUiVisibility::kTimerCallbackVtable);
            runtime.WriteWord(storage + 8u,
                              MinimapSceneUiVisibility::kUpdatePlayerStatsHandler);
            runtime.WriteWord(storage + 4u, owner);
            runtime.WriteWord(storage + 12u, 0u);
        }
        const auto entry = runtime.CreateTimerEntry(
            MinimapSceneUiVisibility::kTimerName,
            MinimapSceneUiVisibility::kTimerPeriodBits, storage);
        try {
            runtime.AddTimerEntry(queue, entry);
        } catch (...) {
            runtime.DestroyTimerEntry(entry);
            throw;
        }
        runtime.DestroyTimerEntry(entry);
    } catch (...) {
        runtime.EndTimerName();
        throw;
    }
    runtime.EndTimerName();
}

}  // namespace

void IMinimapSceneUiVisibilityRuntime::LayoutHudWindows(
    const MinimapSceneUiState& state, bool enabled) {
    MinimapSceneUiVisibility::LayoutWindows(state, enabled, *this);
}
void IMinimapSceneUiVisibilityRuntime::HideProgress() {
    SetProgressVisible(CreateProgressBar(FrameManager()), false);
}
void IMinimapSceneUiVisibilityRuntime::MarkSceneUiActive() {
    WriteWord(FrameManager() + 0x144u, 1u);
}
void IMinimapSceneUiVisibilityRuntime::RefreshHudPlayers(std::uint32_t owner) {
    MinimapSceneUiVisibility::SelectAndRefreshPlayers(owner, *this);
}
std::uint32_t IMinimapSceneUiVisibilityRuntime::InsertActiveComponent(std::uint32_t component) {
    return InsertActive(FrameManager() + 0x134u, component);
}
std::uint32_t IMinimapSceneUiVisibilityRuntime::RemoveActiveComponent(std::uint32_t component) {
    return RemoveActive(FrameManager() + 0x134u, component);
}

void IMinimapSceneUiVisibilityRuntime::RemoveStatsTimer(bool exception_message) {
    RemoveTimer(exception_message, *this);
}

void IMinimapSceneUiVisibilityRuntime::AddStatsTimer(std::uint32_t owner) {
    AddTimer(owner, *this);
}

void MinimapSceneUiVisibility::UpdateStatsTimer(
    std::uint32_t owner, bool enabled, IMinimapSceneUiStatsTimerRuntime& runtime) {
    runtime.RemoveStatsTimer(!enabled);
    if (enabled) runtime.AddStatsTimer(owner);
}

void MinimapSceneUiVisibility::SelectAndRefreshPlayers(
    std::uint32_t owner, IMinimapSceneUiPlayerSelectionRuntime& runtime) {
    SelectPlayer(owner, runtime);
}

void MinimapSceneUiVisibility::StoreGameFlag(
    MinimapSceneUiState& state, const std::uint32_t address,
    const std::uint8_t value,
    IMinimapSceneUiGameFlagRuntime& runtime) {
    state.WriteByte(0xf7u, value);
    runtime.SetGameEntry(26u, state.ReadByte(0xf7u));
    (void)address;
    UpdateVisibilityBasedOnState(state, runtime);
}

void MinimapSceneUiVisibility::UpdateVisibilityBasedOnState(
    const MinimapSceneUiState& state,
    IMinimapSceneUiStateVisibilityRuntime& runtime) {
    const auto linked = state.ReadWord(0xccu);
    if (linked == 0u) return;
    const auto mode = runtime.ReadWord(linked + 0x34u);
    auto flag = state.ReadByte(0xf7u);
    if (mode <= 1u) {
        flag = 0u;
    } else if (mode == 2u) {
        flag &= 1u;
    }
    runtime.SetVisible(state.ReadWord(0x5cu), flag != 0u);
}

std::uint32_t MinimapSceneUiVisibility::SetJumpHint(
    const MinimapSceneUiState& state, const bool visible,
    const std::uint32_t position_address,
    IMinimapJumpHintRuntime& runtime) {
    const auto window = state.ReadWord(0xc0u);
    runtime.SetVisible(window, visible);
    return runtime.SetPositionRaw(window, 1u, position_address);
}

std::uint8_t MinimapSceneUiVisibility::SetJumpHintFromArgs(
    const MinimapSceneUiState& state, const MinimapJumpHintArgs& args,
    IMinimapJumpHintRuntime& runtime) {
    (void)SetJumpHint(state, args.visible_value > 0,
                      args.position_address, runtime);
    return 1u;
}

void MinimapSceneUiVisibility::LayoutWindows(
    const MinimapSceneUiState& state, const bool enabled,
    IMinimapSceneUiGeometryRuntime& runtime) {
    const auto root = state.ReadWord(0x24u);
    const auto overlay = state.ReadWord(0x50u);
    runtime.SetClippedByParent(overlay, false);
    runtime.SetClippedByParent(root, false);
    if (!enabled) return;
    const auto [screen_width, screen_height] = runtime.ConfigSize();
    const auto root_corner = runtime.ConvertScreenCoordinates(
        {static_cast<float>(screen_width), 0.0f});
    const MinimapUiPosition root_position{
        root_corner.x - runtime.Width(root, 1u), root_corner.y};
    runtime.SetPosition(root, 1u, root_position);
    const auto overlay_corner = runtime.ConvertScreenCoordinates(
        {0.0f, static_cast<float>(screen_height)});
    runtime.SetPosition(overlay, 1u,
        {overlay_corner.x - root_position.x,
         overlay_corner.y - runtime.Height(overlay, 1u)});
    runtime.SetAlwaysOnTop(overlay, true);
}

std::uint32_t MinimapSceneUiVisibility::Update(
    MinimapSceneUiState& state, const std::uint32_t address,
    const std::uint32_t visibility, const std::uint32_t parent,
    IMinimapSceneUiVisibilityFlowRuntime& runtime) {
    const auto enabled = static_cast<std::uint8_t>(visibility);
    runtime.SetParent(address, visibility, parent);
    runtime.CloseUtf8Window(address + 0x6cu);
    state.WriteByte(0xf6u, 0u);
    if (enabled != 0u) {
        StoreGameFlag(state, address,
                      static_cast<std::uint8_t>(runtime.GameEntry(26u) > 0),
                      runtime);
    }
    const auto root = state.ReadWord(0x24u);
    runtime.SetComponentVisible(address + 0x2cu, visibility, root);
    runtime.LayoutHudWindows(state, enabled != 0u);

    runtime.SetVisible(state.ReadWord(0xc0u), false);
    runtime.HideProgress();
    if (enabled != 0u) {
        runtime.MarkSceneUiActive();
        runtime.UpdateCharacterMovement(address);
        runtime.RefreshHudPlayers(address);
    }
    runtime.SetVisible(state.ReadWord(0x68u), true);
    runtime.UpdateVisibilityAndMinimap(address);

    UpdateStatsTimer(address, enabled != 0u, runtime);
    if (enabled == 0u) {
        return runtime.RemoveActiveComponent(address + 0x28u);
    }
    return runtime.InsertActiveComponent(address + 0x28u);
}

}  // namespace pal4
