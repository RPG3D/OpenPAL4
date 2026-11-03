#pragma once

#include <cstdint>

namespace pal4 {

class ISystemSettingNavigationRuntime {
public:
    virtual ~ISystemSettingNavigationRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    [[nodiscard]] virtual float ReadFloat(std::uint32_t address) = 0;
    virtual void RequestRedraw(std::uint32_t window) = 0;
    virtual void SetSliderValue(std::uint32_t slider, float value) = 0;
    [[nodiscard]] virtual std::uint32_t FrameManager() = 0;
    virtual void RemoveActive(std::uint32_t collection,
                              std::uint32_t member) = 0;
    virtual void ShowSystemFrame(std::uint32_t frame) = 0;
    virtual void HandleMouseEvent(std::uint32_t window) = 0;
};

class SystemSettingNavigation final {
public:
    [[nodiscard]] static std::uint8_t HandleKey(
        std::uint32_t member, std::uint32_t event,
        ISystemSettingNavigationRuntime& runtime);
    [[nodiscard]] static std::uint8_t ResetHighlights(
        std::uint32_t member, ISystemSettingNavigationRuntime& runtime);
};

}  // namespace pal4
