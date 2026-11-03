#pragma once

#include "pal4/timer_callback.h"

#include <cstdint>

namespace pal4 {

class ITimerFrameActionsRuntime {
public:
    virtual ~ITimerFrameActionsRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void SetFrameVisibility(std::uint32_t frame,
                                    std::uint8_t visible,
                                    std::uint32_t parent) = 0;
    virtual void ForwardMemberEvent(std::uint32_t member,
                                    std::uint32_t event) = 0;
};

class TimerFrameActions final {
public:
    [[nodiscard]] static std::uint8_t OnButtonKey(
        std::uint32_t frame, std::uint32_t event,
        ITimerFrameActionsRuntime& runtime);
    [[nodiscard]] static std::uint8_t OnSettle(
        std::uint32_t frame, ITimerFrameActionsRuntime& runtime);
    [[nodiscard]] static std::uint8_t OnDelegate(
        std::uint32_t frame, ITimerFrameActionsRuntime& runtime);
    [[nodiscard]] static std::uint8_t OnExit(
        std::uint32_t frame, ITimerFrameActionsRuntime& runtime);
};

class UiFrameEventCallback final {
public:
    static constexpr std::uint32_t kVtable = 0x843b88u;
    [[nodiscard]] static TimerCallbackState& Construct(
        TimerCallbackState& callback, std::uint32_t function,
        std::uint32_t context, std::uint32_t owner);
};

}  // namespace pal4
