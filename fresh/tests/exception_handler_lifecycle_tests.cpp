#include "pal4/exception_handler_lifecycle.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Runtime final : public pal4::IExceptionHandlerRuntime {
public:
    [[nodiscard]] std::uint32_t Read(
        const std::uint32_t address) const override {
        const auto found = words.find(address);
        return found == words.end() ? 0u : found->second;
    }

    void Write(const std::uint32_t address,
               const std::uint32_t value) override {
        words[address] = value;
        events.emplace_back("clear");
    }

    [[nodiscard]] std::uint32_t GetProcessHeap() override {
        events.emplace_back("heap");
        return 0x5000u;
    }

    [[nodiscard]] std::uint32_t HeapFree(
        const std::uint32_t heap, const std::uint32_t flags,
        const std::uint32_t address) override {
        Check(heap == 0x5000u && flags == 0u && address == 0x6000u,
              "HeapFree receives process heap, zero flags, and buffer");
        events.emplace_back("free");
        return free_result;
    }

    void SetUnhandledExceptionFilter(
        const std::uint32_t address) override {
        restored_filter = address;
        events.emplace_back("restore");
    }

    [[nodiscard]] std::uint32_t RegisterExitHandler(
        const std::uint32_t address) override {
        registered = address;
        events.emplace_back("register");
        return registration_result;
    }

    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> events;
    std::uint32_t restored_filter = 0u;
    std::uint32_t registered = 0u;
    std::uint32_t free_result = 1u;
    std::uint32_t registration_result = 23u;
};

void CheckStartupAndEmptyCleanup() {
    Runtime runtime;
    pal4::ExceptionHandlerLifecycle::InitializeEmpty();
    Check(runtime.words.empty() && runtime.events.empty(),
          "nullsub initializer has no observable effect");
    Check(pal4::ExceptionHandlerLifecycle::InitializeAndRegisterExit(runtime) ==
              23u &&
              runtime.registered ==
                  pal4::ExceptionHandlerLifecycle::kCleanupEntry,
          "startup registers cleanup callback and returns CRT result");
    runtime.events.clear();
    pal4::ExceptionHandlerLifecycle::Cleanup(runtime);
    Check(runtime.events.empty(),
          "empty cleanup neither frees nor changes exception filter");
}

void CheckCleanupSuccessAndFailure() {
    Runtime runtime;
    runtime.words[pal4::ExceptionHandlerLifecycle::kBuffer] = 0x6000u;
    runtime.words[pal4::ExceptionHandlerLifecycle::kPreviousFilter] = 0x7000u;
    pal4::ExceptionHandlerLifecycle::Cleanup(runtime);
    Check(runtime.Read(pal4::ExceptionHandlerLifecycle::kBuffer) == 0u &&
              runtime.restored_filter == 0x7000u &&
              runtime.events == std::vector<std::string>{
                  "heap", "free", "clear", "restore"},
          "cleanup frees before clearing buffer and restoring filter");

    Runtime failed;
    failed.free_result = 0u;
    failed.words[pal4::ExceptionHandlerLifecycle::kBuffer] = 0x6000u;
    pal4::ExceptionHandlerLifecycle::Cleanup(failed);
    Check(failed.Read(pal4::ExceptionHandlerLifecycle::kBuffer) == 0u &&
              failed.events == std::vector<std::string>{
                  "heap", "free", "clear"},
          "HeapFree failure still clears buffer without filter restoration");
}

}  // namespace

int main() {
    CheckStartupAndEmptyCleanup();
    CheckCleanupSuccessAndFailure();
    std::cout << "2 exception handler lifecycle test cases passed\n";
}
