#pragma once

#include "pal4/minimap_scene_ui_extended_update.h"
#include "pal4/minimap_scene_ui_actions.h"
#include "pal4/minimap_scene_ui_visibility.h"
#include "pal4/utilities_scene_table.h"

#include <array>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace pal4 {
class NativeNewGameScriptSession;
struct NativeGameScriptServices;

class NativeCeguiUiSession;
class NativeGameArenaManagerOwner;
class NativeMinimapManagerOwner;
class NativeSceneGobAssets;
class NativeSceneRecordCatalog;
class NativeEventHandlerArray;
class NativePalGameIVTextureStorage;
class NativeGameTimerQueue;
class NativeUiCountdown;
struct NativeUiCountdownScriptServices;
struct PalActorControlState;
struct NativeSceneHudVisibilityServices;
class NativeSceneHudVisibilityRuntime;
class NativeSceneHudInputRuntime;
struct NativeSceneHudInputServices;

// Binds the original scene UI update to owned Q01/N01 game and CEGUI objects.
class NativeMinimapSceneUiUpdate final
    : private IMinimapSceneUiExtendedRuntime,
      private IMinimapSceneUiMapVisibilityRuntime,
      private IMinimapSceneUiStateVisibilityRuntime {
public:
    NativeMinimapSceneUiUpdate(
        std::filesystem::path game_root,
        NativeGameArenaManagerOwner& arena,
        NativeMinimapManagerOwner& minimap,
        NativeCeguiUiSession& ui,
        const NativeSceneRecordCatalog& scenes,
        const NativeSceneGobAssets& objects,
        std::int32_t screen_width,
        std::int32_t screen_height);
    ~NativeMinimapSceneUiUpdate() override;

    [[nodiscard]] bool Update();
    void StoreGameFlag(std::uint8_t value, NativePalGameIVTextureStorage& game);
    [[nodiscard]] bool SetHudVisible(bool visible, NativeSceneHudVisibilityServices& services,
                                     std::uint32_t parent = 0u);
    // Borrows the UI frame manager's shared handler array. The identity is
    // owner+0x28 in this same state image, not a separate simulated HUD.
    [[nodiscard]] std::uint32_t ActiveComponentAddress() const noexcept;
    // IDA4BFA40: this is the portrait owner registered with the borrowed manager.
    // Reads the live +F0 timer word without changing HUD or timer state.
    [[nodiscard]] std::uint32_t TimerFrameDuration(std::uint32_t manager) const;
    [[nodiscard]] NativeUiCountdown& BindCountdown(std::uint32_t manager,
        NativeGameTimerQueue& timers, std::function<void()> execute);
    // Session, endpoints and queue must outlive this registered HUD owner.
    [[nodiscard]] NativeUiCountdown& BindCountdown(std::uint32_t manager,
        NativeGameTimerQueue& timers, NativeNewGameScriptSession& scripts,
        std::function<NativeGameScriptServices()> services,
        std::function<void()> clear_handlers);
    void TickCountdown(std::uint32_t owner);
    void ExecuteCountdownScript(NativeUiCountdownScriptServices services);
    [[nodiscard]] std::uint32_t SetActive(NativeEventHandlerArray& handlers,
                                         bool enabled);
    [[nodiscard]] std::uint32_t DispatchActiveCallback(
        std::uint32_t component, std::uint32_t slot);
    [[nodiscard]] std::uint8_t DispatchActivePredicate(
        std::uint32_t component, std::uint32_t slot, std::uint32_t event,
        PalActorControlState& control);
    [[nodiscard]] std::uint8_t DispatchKeyDown(std::uint32_t component,
        std::uint32_t event, NativeSceneHudInputServices& services);
    [[nodiscard]] bool SetVisibilityFlags(std::uint8_t map,
        std::uint8_t suppressed, std::uint8_t button);
    [[nodiscard]] std::array<std::uint8_t, 3> VisibilityFlags() const noexcept {
        return {state_.ReadByte(0xf5u), state_.ReadByte(0xf4u), state_.ReadByte(0xf7u)};
    }
    [[nodiscard]] std::uint32_t SelectedSceneId() const noexcept;
    [[nodiscard]] std::uint32_t Mode() const noexcept;
    [[nodiscard]] const std::string& LastError() const noexcept {
        return last_error_;
    }

private:
    friend class NativeSceneHudVisibilityRuntime;
    friend class NativeSceneHudInputRuntime;
    void SetWindowVisible(std::uint32_t window, bool visible) override;
    void SetVisible(std::uint32_t window, bool visible) override;
    [[nodiscard]] bool DialogIsVisible(std::uint32_t owner) override;
    void WriteByte(std::uint32_t address, std::uint8_t value) override;
    [[nodiscard]] std::uint32_t ArenaManager() override;
    [[nodiscard]] std::uint32_t MinimapManager() override;
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t address) override;
    void WriteWord(std::uint32_t address, std::uint32_t value) override;
    void LoadScriptData(std::uint32_t minimap, std::uint32_t arena_path,
                        std::uint32_t arena_script) override;
    void InitializeDialogSystem() override;
    [[nodiscard]] MinimapSceneTriggerVariable QueryTriggerVariable(
        std::uint32_t arena_path, std::uint32_t variable_name) override;
    [[nodiscard]] std::uint32_t CopyFloorAttribute(
        std::uint32_t source) override;
    void ReleaseFloorAttribute(std::uint32_t copy) override;
    void ReleaseTriggerVariable(
        MinimapSceneTriggerVariable variable) override;
    [[nodiscard]] std::uint32_t ItemPropertyTable() override;
    [[nodiscard]] std::uint32_t ItemPropertyCount(
        std::uint32_t collection) override;
    [[nodiscard]] bool MatchesFloorAttribute(
        std::uint32_t key, std::uint32_t row) override;
    [[nodiscard]] std::uint32_t EmptyString() override;
    [[nodiscard]] std::uint32_t ConvertToUtf8(
        std::uint32_t text) override;
    void SetWindowText(std::uint32_t window,
                       std::uint32_t utf8_text) override;
    void SetFlagAndUpdate(std::uint32_t owner,
                          std::uint8_t value) override;
    void UpdateVisibilityAndMinimap(std::uint32_t owner) override;
    void UpdateVisibilityBasedOnState(std::uint32_t owner) override;
    [[nodiscard]] std::pair<std::int32_t, std::int32_t>
        ConfigSize() override;
    [[nodiscard]] std::int32_t FloatToLong(double value) override;
    void SetupMinimapTexture(std::uint32_t minimap,
                             std::int32_t x, std::int32_t y,
                             std::int32_t width,
                             std::int32_t height) override;

    std::filesystem::path game_root_;
    NativeGameArenaManagerOwner& arena_;
    NativeMinimapManagerOwner& minimap_;
    NativeCeguiUiSession& ui_;
    const NativeSceneGobAssets& objects_;
    std::int32_t screen_width_;
    std::int32_t screen_height_;
    MinimapSceneUiState state_{};
    std::vector<UtilitiesSceneRecord> rows_;
    std::array<std::uint32_t, 26> table_{};
    std::string world_key_;
    std::string scene_key_;
    std::string utf8_text_;
    std::string last_error_;
    // Retire timer callbacks before this owner's state and borrowed queue die.
    std::unique_ptr<NativeUiCountdown> countdown_;
    std::uint32_t countdown_manager_{};
    std::uint32_t countdown_queue_{};
    NativeNewGameScriptSession* countdown_session_{};
};

}  // namespace pal4
