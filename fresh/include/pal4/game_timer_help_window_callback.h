#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class IGameTimerHelpWindowRuntime {
public:
    virtual ~IGameTimerHelpWindowRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadDword(std::uint32_t address) = 0;
    virtual void WriteDword(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t UiFrameManager() = 0;
    [[nodiscard]] virtual std::uint32_t FindFrame(
        std::uint32_t manager, std::string_view name) = 0;
    [[nodiscard]] virtual std::uint8_t UpdateClock(std::uint32_t frame) = 0;
};

class GameTimerHelpWindowCallback final {
public:
    explicit GameTimerHelpWindowCallback(std::uint32_t address)
        : address_(address) {}
    // IDA 0x5c3c10.
    [[nodiscard]] std::uint8_t Tick(IGameTimerHelpWindowRuntime& runtime) const;

private:
    std::uint32_t address_;
};

}  // namespace pal4
