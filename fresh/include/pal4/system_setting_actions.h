#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class ISystemSettingSelectionRuntime {
public:
    virtual ~ISystemSettingSelectionRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    virtual void ShowDataMode(std::uint32_t frame) = 0;
    virtual void ShowSave(std::uint32_t frame) = 0;
    virtual void SetFocusedButton(std::uint32_t member,
                                  std::uint32_t button) = 0;
    virtual void SetSelected(std::uint32_t button, bool selected) = 0;
    virtual void RequestRedraw(std::uint32_t window) = 0;
    virtual void SetVisible(std::uint32_t window, bool visible) = 0;
    virtual void SetImage(std::uint32_t window,
                          std::string_view image,
                          std::string_view imageset) = 0;
};

class ISystemSettingActionsRuntime : public ISystemSettingSelectionRuntime {
public:
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t TriggerText(
        std::uint32_t trigger_id) = 0;
    [[nodiscard]] virtual std::uint32_t FrameManager() = 0;
    virtual void CreateCommonDialog(std::uint32_t manager) = 0;
    [[nodiscard]] virtual std::uint32_t ShowDialog(
        std::uint32_t text, std::uint32_t x_bits,
        std::uint32_t y_bits, std::uint32_t mode,
        std::uint32_t callback) = 0;
    [[nodiscard]] virtual std::uint32_t ReleaseCallback(
        std::uint32_t callback) = 0;
    [[nodiscard]] virtual std::uint32_t ArenaManager() = 0;
    virtual void ResetArena(std::uint32_t arena) = 0;
    virtual void ProcessUiEvent(std::uint32_t dispatcher,
                                std::uint32_t event,
                                std::uint32_t first,
                                std::uint32_t second) = 0;
    [[nodiscard]] virtual std::uint32_t Game() = 0;
    virtual void InitializeGameState(std::uint32_t game,
                                     std::uint32_t first,
                                     std::uint32_t second) = 0;
};

class SystemSettingActions final {
public:
    [[nodiscard]] static std::uint8_t OnLoadData(
        std::uint32_t frame, std::uint32_t event,
        ISystemSettingActionsRuntime& runtime);
    static void SelectLoadData(std::uint32_t frame,
                               ISystemSettingSelectionRuntime& runtime);
    [[nodiscard]] static std::uint8_t OnStartNewGame(
        std::uint32_t frame, std::uint32_t event,
        ISystemSettingActionsRuntime& runtime);
    static void SelectStartNewGame(std::uint32_t frame,
                                   ISystemSettingSelectionRuntime& runtime);
    [[nodiscard]] static std::uint8_t OnSystemSettings(
        std::uint32_t frame, std::uint32_t event,
        ISystemSettingActionsRuntime& runtime);
    static void SelectSystemSettings(std::uint32_t frame,
                                     ISystemSettingSelectionRuntime& runtime);
    [[nodiscard]] static std::uint8_t OnQuitGame(
        std::uint32_t frame, std::uint32_t event,
        ISystemSettingActionsRuntime& runtime);
    [[nodiscard]] static std::uint32_t ConfirmQuit(
        std::uint32_t frame, ISystemSettingActionsRuntime& runtime);
    [[nodiscard]] static std::uint8_t OnQuitConfirmed(
        std::uint32_t frame, std::uint32_t event,
        ISystemSettingActionsRuntime& runtime);
};

}  // namespace pal4
