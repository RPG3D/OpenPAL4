#pragma once

#include <cstdint>

namespace pal4 {

class ITimerFrameLayoutRuntime {
public:
    virtual ~ITimerFrameLayoutRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void SetVisible(std::uint32_t window,
                            bool visible) = 0;
    virtual void SetPosition(std::uint32_t window,
                             std::uint32_t x_bits,
                             std::uint32_t y_bits) = 0;
    [[nodiscard]] virtual std::uint32_t HeightBits(
        std::uint32_t window) = 0;
    virtual void SetHeight(std::uint32_t window,
                           std::uint32_t height_bits) = 0;
    [[nodiscard]] virtual std::uint32_t SetupMouseHandler(
        std::uint32_t member, std::uint32_t window) = 0;
    virtual void SetParent(std::uint32_t frame,
                           std::uint8_t visible,
                           std::uint32_t parent) = 0;
    virtual void ResetMember(std::uint32_t member) = 0;
    virtual void SetModalState(std::uint32_t window,
                               std::uint8_t visible) = 0;
};

class TimerFrameLayout final {
public:
    [[nodiscard]] static std::uint32_t Apply(
        std::uint32_t frame, ITimerFrameLayoutRuntime& runtime);
    [[nodiscard]] static std::uint32_t SetWindowVisibility(
        std::uint32_t frame, std::uint8_t visible,
        std::uint32_t parent, ITimerFrameLayoutRuntime& runtime);
};

}  // namespace pal4
