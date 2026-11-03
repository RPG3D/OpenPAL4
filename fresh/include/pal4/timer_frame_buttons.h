#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace pal4 {

class ITimerFrameButtonsRuntime {
public:
    virtual ~ITimerFrameButtonsRuntime() = default;
    [[nodiscard]] virtual std::uint32_t BeginName(
        std::uint32_t frame) = 0;
    [[nodiscard]] virtual std::uint32_t ResetName(
        std::uint32_t buffer, std::uint32_t frame,
        std::string_view suffix) = 0;
    [[nodiscard]] virtual std::uint32_t EndName(
        std::uint32_t buffer) = 0;
    [[nodiscard]] virtual std::uint32_t FindWindow(
        std::uint32_t name) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(
        std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t ConstructCallback(
        std::uint32_t allocation, std::uint32_t function,
        std::uint32_t frame) = 0;
    virtual void Free(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t Subscribe(
        std::uint32_t window, std::uint32_t event,
        std::uint32_t callback) = 0;
    virtual void ReleaseConnection(std::uint32_t connection) = 0;
    [[nodiscard]] virtual std::uint32_t TriggerText(
        std::uint32_t trigger_id) = 0;
    [[nodiscard]] virtual std::string_view ToUtf8(
        std::uint32_t text) = 0;
    virtual void SetText(std::uint32_t window,
                         std::string_view text) = 0;
    [[nodiscard]] virtual std::array<std::uint32_t, 2> Position(
        std::uint32_t window) = 0;
    [[nodiscard]] virtual std::uint32_t HeightBits(
        std::uint32_t window) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
};

class TimerFrameButtons final {
public:
    static constexpr std::uint32_t kClickedEvent = 0x8411b8u;
    static constexpr std::uint32_t kKeyDownEvent = 0x84103cu;
    static constexpr std::uint32_t kEmptyText = 0x841964u;
    static constexpr std::uint32_t kCallbackVtable = 0x843b88u;
    static constexpr std::uint32_t kSettleClick = 0x47dbb0u;
    static constexpr std::uint32_t kDelegateClick = 0x47dbd0u;
    static constexpr std::uint32_t kExitClick = 0x47dbf0u;
    static constexpr std::uint32_t kKeyDown = 0x47db80u;

    [[nodiscard]] static std::uint32_t Initialize(
        std::uint32_t frame, ITimerFrameButtonsRuntime& runtime);
};

}  // namespace pal4
