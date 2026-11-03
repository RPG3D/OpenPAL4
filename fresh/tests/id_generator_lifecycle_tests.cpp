#include "pal4/id_generator_lifecycle.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

class Runtime final : public pal4::IIdGeneratorLifecycleRuntime {
public:
    std::vector<std::string> calls;
    bool throw_on_register = false;
    void RegisterAtExit(std::uint32_t callback) override {
        Check(callback == pal4::IdGeneratorLifecycle::kCleanupCallback,
              "registered cleanup callback address");
        calls.emplace_back("register");
        if (throw_on_register) throw std::runtime_error("registration failed");
    }
    void FreeMemory(std::uint32_t address) override {
        calls.push_back("free:" + std::to_string(address));
    }
};

void Cases() {
    Runtime runtime;
    pal4::IdGeneratorLifecycle lifecycle(runtime);
    pal4::IdGeneratorGlobalState state{};
    state.seed = 0x55u;
    state.padding = {1u, 2u, 3u};
    state.begin = 0x1000u;
    state.end = 0x2000u;
    state.capacity = 0x3000u;
    state.guard = 0x80u;
    state.tail_padding = {4u, 5u, 6u};

    Check(lifecycle.Initialize(state, 0x12345678u) ==
              pal4::IdGeneratorLifecycle::kGlobalAddress &&
              state.seed == 0x12u && state.guard == 0x81u &&
              state.begin == 0u && state.end == 0u &&
              state.capacity == 0u &&
              state.padding == std::array<std::uint8_t, 3>{1u, 2u, 3u} &&
              state.tail_padding ==
                  std::array<std::uint8_t, 3>{4u, 5u, 6u} &&
              runtime.calls == std::vector<std::string>{"register"},
          "first initialization extracts saved ECX high byte and clears pointers");
    state.begin = 0xa000u;
    state.end = 0xa010u;
    state.capacity = 0xa020u;
    Check(lifecycle.Initialize(state, 0xff000000u) ==
              pal4::IdGeneratorLifecycle::kGlobalAddress &&
              state.seed == 0x12u && state.begin == 0xa000u &&
              state.end == 0xa010u && state.capacity == 0xa020u &&
              runtime.calls.size() == 1u,
          "guarded repeat does not reset registry or register again");

    lifecycle.Cleanup(state);
    Check(state.begin == 0u && state.end == 0u &&
              state.capacity == 0u && state.seed == 0x12u &&
              state.guard == 0x81u &&
              runtime.calls == std::vector<std::string>{
                  "register", "free:40960"},
          "cleanup frees begin and preserves seed and guard");
    lifecycle.Cleanup(state);
    Check(runtime.calls.back() == "free:0",
          "cleanup calls allocator even for null begin");

    pal4::IdGeneratorGlobalState failed{};
    failed.begin = 0xbeefu;
    runtime.throw_on_register = true;
    bool threw = false;
    try {
        static_cast<void>(lifecycle.Initialize(failed, 0xab010203u));
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && failed.seed == 0xabu && failed.guard == 1u &&
              failed.begin == 0u && failed.end == 0u &&
              failed.capacity == 0u,
          "registration exception occurs after state initialization");

    pal4::IdGeneratorLifecycleRuntimeStub stub;
    pal4::IdGeneratorLifecycle stub_lifecycle(stub);
    pal4::IdGeneratorGlobalState empty{};
    Check(stub_lifecycle.Initialize(empty, 0u) ==
              pal4::IdGeneratorLifecycle::kGlobalAddress,
          "native runtime stub links");
    stub_lifecycle.Cleanup(empty);
}

}  // namespace

int main() {
    try {
        Cases();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
