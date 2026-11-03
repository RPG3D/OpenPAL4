#pragma once

#include "pal4/minimap_scene_ui.h"
#include "pal4/minimap_scene_ui_actions.h"

#include <cstdint>
#include <span>

namespace pal4 {

class IMinimapSceneUiPlayerRefreshRuntime {
public:
    virtual ~IMinimapSceneUiPlayerRefreshRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t PlayerManager() = 0;
    virtual void UpdatePlayerStates(std::uint32_t manager, std::uint32_t action) = 0;
    virtual void UpdatePlayerAndGameStates(std::uint32_t owner) = 0;
};

class IMinimapSceneUiSpecialKeyRuntime {
public:
    virtual ~IMinimapSceneUiSpecialKeyRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadEventWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ActorControl() = 0;
    [[nodiscard]] virtual std::int32_t GetControlWord(
        std::uint32_t control) = 0;
    virtual void SetControlWord(std::uint32_t control,
                                bool enabled) = 0;
};

// Menu entry can be supplied without implementing unrelated skill/team actions.
class IMinimapSceneUiMenuRuntime {
public:
    virtual ~IMinimapSceneUiMenuRuntime() = default;
    virtual void SwitchSystemState(std::uint32_t owner) = 0;
    [[nodiscard]] virtual std::uint32_t FrameManager() = 0;
    virtual void DispatchEvent(std::uint32_t dispatcher,
                               std::uint32_t key_cell,
                               MinimapSceneUiEventData data) = 0;
};

class IMinimapSceneUiInputRuntime : public IMinimapSceneUiMenuRuntime,
                                   public IMinimapSceneUiPlayerRefreshRuntime,
                                   public IMinimapSceneUiSpecialKeyRuntime {
public:
    virtual ~IMinimapSceneUiInputRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ArenaManager() = 0;
    virtual void AdvanceRunMode(std::uint32_t owner,
                                std::uint32_t adapter) = 0;
    virtual void ToggleState(std::uint32_t owner) = 0;
    [[nodiscard]] virtual bool WindowVisible(std::uint32_t window) = 0;
    virtual void ActivateSkill(std::uint32_t owner) = 0;
    virtual void UpdateSkillState(std::uint32_t owner) = 0;
    [[nodiscard]] virtual std::uint32_t PlayerManager() = 0;
    [[nodiscard]] virtual std::int32_t ActivePlayerCount(
        std::uint32_t manager) = 0;
    [[nodiscard]] virtual std::uint32_t PlayerByIndex(
        std::uint32_t manager, std::uint32_t index) = 0;
    virtual void SortPlayers(std::span<std::uint32_t> players) = 0;
    [[nodiscard]] virtual std::uint32_t PlayerState(
        std::uint32_t player) = 0;
    virtual void SetPlayerState(std::uint32_t player,
                                std::uint32_t value) = 0;
    virtual void UpdatePlayerStates(std::uint32_t manager,
                                    std::uint32_t action) = 0;
    virtual void UpdatePlayerAndGameStates(std::uint32_t owner) = 0;
};

class MinimapSceneUiInput final {
public:
    static constexpr std::uint32_t kSystemEventKeyCell = 0x8a80b8u;

    // HUD vtable 0x843AF0 +12/+16/+20: shared constant-true game callback.
    // This preserves its game role without reconstructing the shared CRT body.
    [[nodiscard]] static constexpr std::uint32_t ActiveCallback() noexcept {
        return 1u;
    }

    [[nodiscard]] static std::uint8_t HandleSpecialKey(
        std::uint32_t event_address,
        IMinimapSceneUiSpecialKeyRuntime& runtime);
    [[nodiscard]] static std::uint8_t HandlePlayerInput(
        const MinimapSceneUiState& state,
        std::uint32_t owner,
        std::uint32_t event_address,
        IMinimapSceneUiInputRuntime& runtime);
    static void RefreshPlayers(std::uint32_t owner,
                               std::uint32_t player,
                               IMinimapSceneUiPlayerRefreshRuntime& runtime);
};

}  // namespace pal4
