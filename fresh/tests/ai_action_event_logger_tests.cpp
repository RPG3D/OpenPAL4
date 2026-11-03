#include "pal4/ai_action_task.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

constexpr std::uint32_t kSource = 0x4000u;
constexpr std::uint32_t kKey = 0x5000u;

struct Runtime final : pal4::IAiActionEventLoggerRuntime {
    std::vector<std::string> calls;
    std::string formatted;
    std::vector<std::uint32_t> values;

    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address ==
                  pal4::AiActionEventLogger::kEventKeyPointer,
              "event key pointer lives at 0x8b8500");
        calls.emplace_back("key");
        return kKey;
    }
    void FormatToGlobalBuffer(
        std::uint32_t buffer, const char* format,
        const std::vector<std::uint32_t>& arguments) override {
        Check(buffer == pal4::AiActionEventLogger::kMessageBuffer &&
                  format != nullptr,
              "format writes global 0x8f9848 buffer");
        calls.emplace_back("format");
        formatted = format;
        values = arguments;
    }
    std::uint32_t DispatchEvent(
        std::uint32_t key, std::uint32_t source,
        std::uint32_t buffer, std::uint32_t mode) override {
        Check(key == kKey && source == kSource &&
                  buffer == pal4::AiActionEventLogger::kMessageBuffer &&
                  mode == 3u,
              "caae_message event uses source/buffer payload and mode 3");
        calls.emplace_back("dispatch");
        return 0x6000u;
    }
};

void FormatAndDispatch() {
    Runtime runtime;
    Check(pal4::AiActionEventLogger::Log(
              kSource, "selected:%d", {17u}, runtime) ==
              0x6000u &&
          runtime.calls == std::vector<std::string>{
              "format", "key", "dispatch"} &&
          runtime.formatted == "selected:%d" &&
          runtime.values == std::vector<std::uint32_t>{17u},
          "format precedes event-key read and dispatch");

    runtime = Runtime{};
    Check(pal4::AiActionEventLogger::Log(
              kSource, nullptr, {}, runtime) == 0x6000u &&
          runtime.calls == std::vector<std::string>{
              "key", "dispatch"},
          "null format dispatches previous global buffer content");
}

}  // namespace

int main() {
    try {
        FormatAndDispatch();
        std::cout << "ai_action_event_logger_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
