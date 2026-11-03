#include "pal4/script_trade_wait.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

class Runtime final : public pal4::IScriptTradeWaitRuntime {
public:
    pal4::TradeWaitState state{};
    std::vector<std::string> calls;
    bool allocation_fails = false;
    std::uint32_t ui = 0x7000u;
    std::int32_t completion = 0;
    std::string shop;
    std::string context;

    std::uint32_t TradeSystem() override {
        calls.emplace_back("trade");
        return 0x4000u;
    }
    void SetTradeItemInfo(std::uint32_t trade,
                          std::string_view shop_id,
                          std::string_view context_value) override {
        Check(trade == 0x4000u, "trade receiver");
        shop = shop_id;
        context = context_value;
        calls.emplace_back("set info");
    }
    pal4::TradeWaitState* AllocateWait(std::uint32_t bytes) override {
        Check(bytes == 0x20u, "trade wait size");
        calls.emplace_back("allocate");
        return allocation_fails ? nullptr : &state;
    }
    std::uint32_t ScriptExecutor() override {
        calls.emplace_back("executor");
        return 0x6000u;
    }
    void ConstructWait(pal4::TradeWaitState& handler,
                       std::uint8_t mode, std::uint32_t queue) override {
        Check(&handler == &state && mode == 1u && queue == 0x6054u,
              "trade wait constructor");
        calls.emplace_back("construct");
    }
    void FlushWait(pal4::TradeWaitState& handler) override {
        Check(&handler == &state && handler.vtable_address == 0x849600u,
              "trade wait flush state");
        calls.emplace_back("flush");
    }
    std::uint32_t AddHandler(
        std::uint32_t queue, pal4::TradeWaitState* handler) override {
        Check(queue == 0x6054u &&
                  handler == (allocation_fails ? nullptr : &state),
              "trade queue handler");
        calls.emplace_back("add");
        return 0x1234u;
    }
    std::uint32_t TradeUi() override {
        calls.emplace_back("ui");
        return ui;
    }
    std::int32_t TradeUiCompletion(std::uint32_t trade_ui) override {
        Check(trade_ui == ui && ui != 0u, "trade UI receiver");
        calls.emplace_back("completion");
        return completion;
    }
    void CloseWait(pal4::TradeWaitState& handler) override {
        Check(&handler == &state, "trade wait close receiver");
        calls.emplace_back("close");
    }
};

void StartCases() {
    Runtime runtime;
    pal4::ScriptTradeWait action(runtime);
    Check(action.Start("store01", "task02") == 0x1234u &&
              runtime.shop == "store01" && runtime.context == "task02" &&
              runtime.calls == std::vector<std::string>{
                  "trade", "set info", "allocate", "executor",
                  "construct", "flush", "executor", "add"},
          "trade startup configures UI then queues a blocking handler");

    Runtime empty;
    pal4::ScriptTradeWait empty_action(empty);
    Check(empty_action.Start({}, {}) == 0x1234u &&
              empty.shop.empty() && empty.context.empty(),
          "null script strings normalize to empty");

    Runtime failed;
    failed.allocation_fails = true;
    pal4::ScriptTradeWait failed_action(failed);
    Check(failed_action.Start("store01", "task02") == 0x1234u &&
              failed.calls == std::vector<std::string>{
                  "trade", "set info", "allocate", "executor", "add"},
          "allocation failure still queues null");
}

void UpdateCases() {
    Runtime runtime;
    pal4::ScriptTradeWait action(runtime);
    runtime.ui = 0u;
    action.Update(runtime.state);
    Check(runtime.calls == std::vector<std::string>{"ui", "close"},
          "absent trade UI closes wait without completion query");

    runtime.ui = 0x7000u;
    runtime.calls.clear();
    action.Update(runtime.state);
    Check(runtime.calls == std::vector<std::string>{
              "ui", "ui", "completion"},
          "active trade UI remains pending");

    runtime.completion = 2;
    runtime.calls.clear();
    action.Update(runtime.state);
    Check(runtime.calls == std::vector<std::string>{
              "ui", "ui", "completion"},
          "only exact completion value one closes");

    runtime.completion = 1;
    runtime.calls.clear();
    action.Update(runtime.state);
    Check(runtime.calls == std::vector<std::string>{
              "ui", "ui", "completion", "close"},
          "completed trade UI closes wait");
}

}  // namespace

int main() {
    try {
        StartCases();
        UpdateCases();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
