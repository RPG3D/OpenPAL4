#pragma once

#include "pal4/minimap_scene_ui.h"

#include <cstdint>
#include <string_view>
#include <utility>

namespace pal4 {

struct MinimapUiPosition {
    float x{};
    float y{};
};

struct MinimapJumpHintArgs {
    std::int32_t visible_value{};
    std::uint32_t position_address{};
};

class IMinimapJumpHintRuntime {
public:
    virtual ~IMinimapJumpHintRuntime() = default;
    virtual void SetVisible(std::uint32_t window, bool visible) = 0;
    [[nodiscard]] virtual std::uint32_t SetPositionRaw(
        std::uint32_t window, std::uint32_t mode,
        std::uint32_t position_address) = 0;
};

class IMinimapSceneUiStateVisibilityRuntime {
public:
    virtual ~IMinimapSceneUiStateVisibilityRuntime() = default;
    virtual void SetVisible(std::uint32_t window, bool visible) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
};

class IMinimapSceneUiGameFlagRuntime : public IMinimapSceneUiStateVisibilityRuntime {
public:
    virtual void SetGameEntry(std::uint32_t index, std::uint8_t value) = 0;
};

class IMinimapSceneUiGeometryRuntime {
public:
    virtual ~IMinimapSceneUiGeometryRuntime() = default;
    virtual void SetClippedByParent(std::uint32_t window, bool clipped) = 0;
    [[nodiscard]] virtual std::pair<std::int32_t, std::int32_t> ConfigSize() = 0;
    [[nodiscard]] virtual MinimapUiPosition ConvertScreenCoordinates(
        MinimapUiPosition input) = 0;
    [[nodiscard]] virtual float Width(std::uint32_t window, std::uint32_t mode) = 0;
    [[nodiscard]] virtual float Height(std::uint32_t window, std::uint32_t mode) = 0;
    virtual void SetPosition(std::uint32_t window, std::uint32_t mode,
                             MinimapUiPosition position) = 0;
    virtual void SetAlwaysOnTop(std::uint32_t window, bool enabled) = 0;
};

class IMinimapSceneUiPlayerSelectionRuntime {
public:
    virtual ~IMinimapSceneUiPlayerSelectionRuntime() = default;
    [[nodiscard]] virtual std::uint32_t PlayerManager() = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t PlayerByIndex(std::uint32_t manager, std::uint32_t index) = 0;
    [[nodiscard]] virtual bool PlayerStateBlocked(std::uint32_t state) = 0;
    virtual void UpdatePlayerStates(std::uint32_t owner, std::uint32_t player) = 0;
    virtual void UpdatePlayerAndGameStates(std::uint32_t owner) = 0;
};

class IMinimapSceneUiStatsTimerRuntime {
public:
    virtual ~IMinimapSceneUiStatsTimerRuntime() = default;
    virtual void RemoveStatsTimer(bool exception_message) = 0;
    virtual void AddStatsTimer(std::uint32_t owner) = 0;
};

// Ordered show/hide operations, independent of legacy frame-manager storage.
class IMinimapSceneUiVisibilityFlowRuntime : public IMinimapSceneUiGameFlagRuntime,
                                            public IMinimapSceneUiStatsTimerRuntime {
public:
    virtual void SetParent(std::uint32_t owner, std::uint32_t visible, std::uint32_t parent) = 0;
    virtual void CloseUtf8Window(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::int32_t GameEntry(std::uint32_t index) = 0;
    virtual void SetGameEntry(std::uint32_t index, std::uint8_t value) = 0;
    virtual void SetComponentVisible(std::uint32_t component, std::uint32_t visible,
                                     std::uint32_t root) = 0;
    virtual void LayoutHudWindows(const MinimapSceneUiState& state, bool enabled) = 0;
    virtual void HideProgress() = 0;
    virtual void MarkSceneUiActive() = 0;
    virtual void UpdateCharacterMovement(std::uint32_t owner) = 0;
    virtual void RefreshHudPlayers(std::uint32_t owner) = 0;
    virtual void UpdateVisibilityAndMinimap(std::uint32_t owner) = 0;
    [[nodiscard]] virtual std::uint32_t InsertActiveComponent(std::uint32_t component) = 0;
    [[nodiscard]] virtual std::uint32_t RemoveActiveComponent(std::uint32_t component) = 0;
};

class IMinimapSceneUiVisibilityRuntime : public IMinimapSceneUiVisibilityFlowRuntime,
                                        public IMinimapJumpHintRuntime,
                                        public IMinimapSceneUiGeometryRuntime,
                                        public IMinimapSceneUiPlayerSelectionRuntime {
public:
    virtual ~IMinimapSceneUiVisibilityRuntime() = default;
    void LayoutHudWindows(const MinimapSceneUiState& state, bool enabled) override;
    void HideProgress() override;
    void MarkSceneUiActive() override;
    void RefreshHudPlayers(std::uint32_t owner) override;
    std::uint32_t InsertActiveComponent(std::uint32_t component) override;
    std::uint32_t RemoveActiveComponent(std::uint32_t component) override;
    void RemoveStatsTimer(bool exception_message) override;
    void AddStatsTimer(std::uint32_t owner) override;
    void SetVisible(std::uint32_t window, bool visible) override = 0;
    std::uint32_t ReadWord(std::uint32_t address) override = 0;
    virtual void SetParent(std::uint32_t owner, std::uint32_t visible,
                           std::uint32_t parent) = 0;
    virtual void CloseUtf8Window(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::int32_t GameEntry(std::uint32_t index) = 0;
    virtual void SetGameEntry(std::uint32_t index,
                              std::uint8_t value) = 0;
    virtual void SetComponentVisible(std::uint32_t component,
                                     std::uint32_t visible,
                                     std::uint32_t root_window) = 0;
    [[nodiscard]] virtual std::uint32_t FrameManager() = 0;
    [[nodiscard]] virtual MinimapUiPosition ConvertCoordinates(
        std::uint32_t manager, MinimapUiPosition input) = 0;
    MinimapUiPosition ConvertScreenCoordinates(MinimapUiPosition input) override {
        return ConvertCoordinates(FrameManager(), input);
    }
    [[nodiscard]] virtual std::uint32_t CreateProgressBar(
        std::uint32_t manager) = 0;
    virtual void SetProgressVisible(std::uint32_t progress,
                                    bool visible) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void UpdateCharacterMovement(std::uint32_t owner) = 0;
    [[nodiscard]] virtual std::uint32_t PlayerManager() = 0;
    [[nodiscard]] virtual std::uint32_t PlayerByIndex(
        std::uint32_t manager, std::uint32_t index) = 0;
    [[nodiscard]] virtual bool PlayerStateBlocked(
        std::uint32_t state_address) = 0;
    virtual void UpdatePlayerStates(std::uint32_t owner,
                                    std::uint32_t player) = 0;
    virtual void UpdatePlayerAndGameStates(std::uint32_t owner) = 0;
    virtual void UpdateVisibilityAndMinimap(std::uint32_t owner) = 0;
    virtual void BeginTimerName(std::string_view name,
                                bool exception_message) = 0;
    virtual void RemoveTimerByName(std::uint32_t queue,
                                   std::string_view name) = 0;
    virtual void EndTimerName() = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t CreateTimerEntry(
        std::string_view name, std::uint32_t period_bits,
        std::uint32_t callback) = 0;
    virtual void AddTimerEntry(std::uint32_t queue,
                               std::uint32_t entry) = 0;
    virtual void DestroyTimerEntry(std::uint32_t entry) = 0;
    [[nodiscard]] virtual std::uint32_t InsertActive(
        std::uint32_t array, std::uint32_t component) = 0;
    [[nodiscard]] virtual std::uint32_t RemoveActive(
        std::uint32_t array, std::uint32_t component) = 0;
};

class MinimapSceneUiVisibility final {
public:
    static void SelectAndRefreshPlayers(std::uint32_t owner,
                                       IMinimapSceneUiPlayerSelectionRuntime& runtime);
    static void UpdateStatsTimer(std::uint32_t owner, bool enabled,
                                IMinimapSceneUiStatsTimerRuntime& runtime);
    static void LayoutWindows(const MinimapSceneUiState& state, bool enabled,
                              IMinimapSceneUiGeometryRuntime& runtime);
    static constexpr std::string_view kTimerName = "FlashRoleButton";
    static constexpr std::uint32_t kTimerCallbackVtable = 0x843b20u;
    static constexpr std::uint32_t kUpdatePlayerStatsHandler = 0x479470u;
    static constexpr std::uint32_t kTimerPeriodBits = 0x3e99999au;

    static void StoreGameFlag(MinimapSceneUiState& state,
                              std::uint32_t address,
                              std::uint8_t value,
                              IMinimapSceneUiGameFlagRuntime& runtime);
    static void UpdateVisibilityBasedOnState(
        const MinimapSceneUiState& state,
        IMinimapSceneUiStateVisibilityRuntime& runtime);
    [[nodiscard]] static std::uint32_t SetJumpHint(
        const MinimapSceneUiState& state, bool visible,
        std::uint32_t position_address,
        IMinimapJumpHintRuntime& runtime);
    [[nodiscard]] static std::uint8_t SetJumpHintFromArgs(
        const MinimapSceneUiState& state,
        const MinimapJumpHintArgs& args,
        IMinimapJumpHintRuntime& runtime);

    [[nodiscard]] static std::uint32_t Update(
        MinimapSceneUiState& state, std::uint32_t address,
        std::uint32_t visibility, std::uint32_t parent,
        IMinimapSceneUiVisibilityFlowRuntime& runtime);
};

}  // namespace pal4
