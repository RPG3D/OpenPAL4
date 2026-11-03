#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace pal4 {

struct FlashWaitState {
    std::uint32_t vtable_address{};
    std::array<std::byte, 28> base{};
};
static_assert(sizeof(FlashWaitState) == 0x20u);

struct FlashColor {
    std::uint8_t red{};
    std::uint8_t green{};
    std::uint8_t blue{};
};

class IScriptFlashRuntime {
public:
    virtual ~IScriptFlashRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ScreenEffect() = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void StartFade(std::uint32_t effect, float duration,
                           std::uint32_t hold, FlashColor color,
                           float alpha_start, float alpha_end,
                           float alpha_on_finish) = 0;
    [[nodiscard]] virtual FlashWaitState* Allocate(std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t ScriptExecutor() = 0;
    virtual void Construct(FlashWaitState& state, std::uint8_t wait_mode,
                           std::uint32_t queue) = 0;
    virtual void Flush(FlashWaitState& state) = 0;
    virtual void Close(FlashWaitState& state) = 0;
    [[nodiscard]] virtual std::uint32_t AddHandler(
        std::uint32_t queue, FlashWaitState* state) = 0;
};

class ScriptFlashEffect final {
public:
    static constexpr std::uint32_t kWaitVtable = 0x8495dcu;
    static constexpr std::uint32_t kQueueOffset = 0x54u;

    explicit ScriptFlashEffect(IScriptFlashRuntime& runtime)
        : runtime_(runtime) {}

    // IDA 0x5e5490..0x5e5890.
    [[nodiscard]] std::uint32_t FlashOutBlack(
        float duration, std::uint8_t hold,
        std::uint8_t wait_mode);
    void UpdateFlashOutBlack(FlashWaitState& state);
    [[nodiscard]] std::uint32_t FlashInBlack(
        float duration, std::uint8_t wait_mode);
    [[nodiscard]] std::uint32_t FlashOutWhite(
        float duration, std::uint8_t hold,
        std::uint8_t wait_mode);
    [[nodiscard]] std::uint32_t FlashInWhite(
        float duration, std::uint8_t wait_mode);
    [[nodiscard]] std::uint32_t FlashOutRed(
        float duration, std::uint8_t hold,
        std::uint8_t wait_mode);
    [[nodiscard]] std::uint32_t FlashInRed(
        float duration, std::uint32_t unused,
        std::uint8_t wait_mode);

private:
    [[nodiscard]] std::uint32_t Start(float duration, FlashColor color,
                                       bool fade_in, std::uint32_t hold,
                                       std::uint8_t wait_mode);
    IScriptFlashRuntime& runtime_;
};

}  // namespace pal4
