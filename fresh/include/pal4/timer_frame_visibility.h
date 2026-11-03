#pragma once

#include <cstdint>

namespace pal4 {

class ITimerFrameVisibilityRuntime {
public:
    virtual ~ITimerFrameVisibilityRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t FrameManager() = 0;
    [[nodiscard]] virtual std::uint32_t FindFrame(
        std::uint32_t manager, std::uint32_t name) = 0;
    virtual void RegisterFrame(std::uint32_t manager,
                               std::uint32_t name) = 0;
    virtual void SetFrameVisibility(std::uint32_t frame,
                                    std::uint8_t visible,
                                    std::uint32_t parent) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
};

class TimerFrameVisibility final {
public:
    static constexpr std::uint32_t kNameSource = 0x8b0660u;

    [[nodiscard]] static bool SetVisible(
        std::uint32_t enabled, std::uint32_t selection,
        ITimerFrameVisibilityRuntime& runtime);
};

}  // namespace pal4
