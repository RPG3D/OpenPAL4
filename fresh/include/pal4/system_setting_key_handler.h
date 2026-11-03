#pragma once

#include <cstdint>

namespace pal4 {

class ISystemSettingKeyRuntime {
public:
    virtual ~ISystemSettingKeyRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void HideFrame(std::uint32_t frame,
                           std::uint32_t first,
                           std::uint32_t second) = 0;
    [[nodiscard]] virtual std::uint32_t FrameManager() = 0;
    virtual void ProcessUiEvent(std::uint32_t dispatcher,
                                std::uint32_t event,
                                std::uint32_t first,
                                std::uint32_t second) = 0;
    [[nodiscard]] virtual std::uint8_t ForwardKey(
        std::uint32_t member, std::uint32_t event) = 0;
    virtual void HandleMouseEvent(std::uint32_t window) = 0;
    virtual void SelectLoadData(std::uint32_t frame) = 0;
    virtual void ShowSystemSettings(std::uint32_t frame) = 0;
    virtual void ConfirmQuit(std::uint32_t frame) = 0;
};

class SystemSettingKeyHandler final {
public:
    [[nodiscard]] static std::uint8_t OnExit(
        std::uint32_t member, std::uint32_t event,
        ISystemSettingKeyRuntime& runtime);
    [[nodiscard]] static std::uint8_t OnGameMenuKey(
        std::uint32_t member, std::uint32_t event,
        ISystemSettingKeyRuntime& runtime);
};

}  // namespace pal4
