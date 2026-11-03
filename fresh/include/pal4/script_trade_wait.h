#pragma once

#include "pal4/script_current_state_wait.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace pal4 {

// The 32-byte script wait object installed by 0x5e6e40.
struct TradeWaitState {
    std::uint32_t vtable_address{};
    std::array<std::byte, 28> base{};
};
static_assert(sizeof(TradeWaitState) == 0x20u);

// PALGameIV current-state and script-queue calls for the trade wait.
// The legacy TradeUi names below refer to game+18C's state entry: its virtual
// +14 completion query is the state ID, and Scene1 completes the wait.
class IScriptTradeWaitRuntime : public IScriptCurrentStateWaitRuntime {
public:
    virtual ~IScriptTradeWaitRuntime() = default;
    [[nodiscard]] virtual std::uint32_t TradeSystem() = 0;
    virtual void SetTradeItemInfo(std::uint32_t trade,
                                  std::string_view shop_id,
                                  std::string_view context) = 0;
    [[nodiscard]] virtual TradeWaitState* AllocateWait(
        std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t ScriptExecutor() = 0;
    virtual void ConstructWait(TradeWaitState& state,
                               std::uint8_t wait_mode,
                               std::uint32_t queue) = 0;
    virtual void FlushWait(TradeWaitState& state) = 0;
    [[nodiscard]] virtual std::uint32_t AddHandler(
        std::uint32_t queue, TradeWaitState* state) = 0;
    [[nodiscard]] virtual std::uint32_t TradeUi() = 0;
    [[nodiscard]] virtual std::int32_t TradeUiCompletion(
        std::uint32_t trade_ui) = 0;
    virtual void CloseWait(TradeWaitState& state) = 0;
    std::uint32_t CurrentState() override { return TradeUi(); }
    std::int32_t CurrentStateId(std::uint32_t state) override {
        return TradeUiCompletion(state);
    }
};

// Script entry and completion poll for the PAL4 trade screen.
class ScriptTradeWait final {
public:
    static constexpr std::uint32_t kWaitVtable = 0x849600u;
    static constexpr std::uint32_t kQueueOffset = 0x54u;

    explicit ScriptTradeWait(IScriptTradeWaitRuntime& runtime)
        : runtime_(runtime) {}

    // 0x5e6e40: configure trade and enqueue a blocking wait object.
    [[nodiscard]] std::uint32_t Start(std::string_view shop_id,
                                      std::string_view context);
    // 0x5e6ef0: close when trade UI is absent or signals completion.
    void Update(TradeWaitState& state);

private:
    IScriptTradeWaitRuntime& runtime_;
};

}  // namespace pal4
