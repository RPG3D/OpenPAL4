#pragma once

#include <cstdint>

namespace pal4 {

class ISystemSettingOptionActionsRuntime {
public:
    virtual ~ISystemSettingOptionActionsRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void SetSelected(std::uint32_t button, bool selected) = 0;
    [[nodiscard]] virtual std::uint32_t GlobalData() = 0;
    virtual void ApplyClipDistance(std::uint32_t global_data,
                               std::uint32_t index) = 0;
};

class SystemSettingOptions final {
public:
    [[nodiscard]] static std::uint8_t SelectSequence(
        std::uint32_t frame, std::uint32_t event,
        ISystemSettingOptionActionsRuntime& runtime);
    [[nodiscard]] static std::uint8_t SelectClipDistance(
        std::uint32_t frame, std::uint32_t event,
        ISystemSettingOptionActionsRuntime& runtime);
    [[nodiscard]] static std::uint8_t SelectAuto(
        std::uint32_t frame, std::uint32_t event,
        ISystemSettingOptionActionsRuntime& runtime);
    [[nodiscard]] static std::uint8_t SelectIntro(
        std::uint32_t frame, std::uint32_t event,
        ISystemSettingOptionActionsRuntime& runtime);
};

}  // namespace pal4
