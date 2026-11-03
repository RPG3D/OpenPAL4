#include "pal4/script_flash_effect.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

class Runtime final : public pal4::IScriptFlashRuntime {
public:
    pal4::FlashWaitState state{};
    std::vector<std::string> calls;
    pal4::FlashColor expected_color{};
    std::uint32_t expected_hold = 0u;
    std::uint8_t expected_mode = 3u;
    bool expected_in = false;
    bool allocation_fails = false;
    std::uint32_t phase = 1u;

    std::uint32_t ScreenEffect() override {
        calls.emplace_back("screen effect");
        return 0x4000u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == 0x4030u, "screen effect phase at +48");
        calls.emplace_back("phase");
        return phase;
    }
    void StartFade(std::uint32_t effect, float duration,
                   std::uint32_t hold, pal4::FlashColor color,
                   float alpha_start, float alpha_end,
                   float alpha_on_finish) override {
        Check(effect == 0x4000u && duration == 1.25f &&
                  hold == expected_hold &&
                  color.red == expected_color.red &&
                  color.green == expected_color.green &&
                  color.blue == expected_color.blue &&
                  alpha_start == (expected_in ? 255.0f : 0.0f) &&
                  alpha_end == (expected_in ? 0.0f : 255.0f) &&
                  alpha_on_finish == (expected_in ? 0.0f : 255.0f),
              "flash color, hold and alpha setup");
        calls.emplace_back("start fade");
    }
    pal4::FlashWaitState* Allocate(std::uint32_t bytes) override {
        Check(bytes == 0x20u, "flash wait allocation size");
        calls.emplace_back("allocate");
        return allocation_fails ? nullptr : &state;
    }
    std::uint32_t ScriptExecutor() override {
        calls.emplace_back("executor");
        return 0x5000u;
    }
    void Construct(pal4::FlashWaitState& candidate,
                   std::uint8_t mode, std::uint32_t queue) override {
        Check(&candidate == &state && mode == expected_mode &&
                  queue == 0x5054u,
              "flash wait mode and queue");
        calls.emplace_back("construct");
    }
    void Flush(pal4::FlashWaitState& candidate) override {
        Check(&candidate == &state &&
                  state.vtable_address == 0x8495dcu,
              "flash wait vtable before flush");
        calls.emplace_back("flush");
    }
    void Close(pal4::FlashWaitState& candidate) override {
        Check(&candidate == &state, "flash wait close receiver");
        calls.emplace_back("close");
    }
    std::uint32_t AddHandler(
        std::uint32_t queue, pal4::FlashWaitState* candidate) override {
        Check(queue == 0x5054u &&
                  candidate == (allocation_fails ? nullptr : &state),
              "flash handler queue");
        calls.emplace_back("add");
        return 0x1234u;
    }
};

void ColorCases() {
    for (std::uint32_t variant = 0u; variant < 6u; ++variant) {
        Runtime runtime;
        const auto color = variant / 2u;
        runtime.expected_color = color == 0u ?
            pal4::FlashColor{0u, 0u, 0u} :
            color == 1u ? pal4::FlashColor{255u, 255u, 255u} :
                          pal4::FlashColor{229u, 10u, 10u};
        runtime.expected_in = (variant & 1u) != 0u;
        runtime.expected_hold = runtime.expected_in ? 0u : 7u;
        pal4::ScriptFlashEffect flash(runtime);
        std::uint32_t result{};
        switch (variant) {
        case 0u: result = flash.FlashOutBlack(1.25f, 7u, 3u); break;
        case 1u: result = flash.FlashInBlack(1.25f, 3u); break;
        case 2u: result = flash.FlashOutWhite(1.25f, 7u, 3u); break;
        case 3u: result = flash.FlashInWhite(1.25f, 3u); break;
        case 4u: result = flash.FlashOutRed(1.25f, 7u, 3u); break;
        default: result = flash.FlashInRed(1.25f, 0x9876u, 3u); break;
        }
        Check(result == 0x1234u &&
                  runtime.calls == std::vector<std::string>{
                      "screen effect", "phase", "screen effect",
                      "start fade", "allocate", "executor", "construct",
                      "flush", "executor", "add"},
              "all flash variants preserve fade and wait call order");
    }
}

void BoundaryCases() {
    Runtime disabled;
    disabled.phase = 0u;
    pal4::ScriptFlashEffect disabled_flash(disabled);
    Check(disabled_flash.FlashOutBlack(1.25f, 7u, 3u) == 0u &&
              disabled.calls == std::vector<std::string>{
                  "screen effect", "phase"},
          "zero phase does not start fade or allocate wait");

    Runtime failed;
    failed.allocation_fails = true;
    failed.expected_hold = 7u;
    pal4::ScriptFlashEffect failed_flash(failed);
    Check(failed_flash.FlashOutBlack(1.25f, 7u, 3u) == 0x1234u &&
              failed.calls == std::vector<std::string>{
                  "screen effect", "phase", "screen effect",
                  "start fade", "allocate", "executor", "add"},
          "allocation failure still queues null and forwards result");

    Runtime no_wait;
    no_wait.expected_mode = 0u;
    no_wait.expected_in = true;
    pal4::ScriptFlashEffect no_wait_flash(no_wait);
    Check(no_wait_flash.FlashInBlack(1.25f, 0u) == 0x1234u &&
              no_wait.calls.size() == 10u,
          "non-wait flash still creates and queues handler");

    for (const auto phase : {0u, 2u}) {
        Runtime callback;
        callback.phase = phase;
        pal4::ScriptFlashEffect effect(callback);
        effect.UpdateFlashOutBlack(callback.state);
        Check(callback.calls == (phase == 0u ?
                  std::vector<std::string>{"screen effect", "phase"} :
                  std::vector<std::string>{
                      "screen effect", "phase", "close"}),
              "flash-out callback closes only after nonzero phase");
    }
}

}  // namespace

int main() {
    try {
        ColorCases();
        BoundaryCases();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
