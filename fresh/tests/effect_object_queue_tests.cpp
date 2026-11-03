#include "pal4/effect_object_queue.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IEffectObjectQueueRuntime {
    std::vector<std::string> calls;
    bool throw_on_dispatch = false;
    void Dispatch(const std::uint32_t system, const std::uint32_t slot,
                  const pal4::EffectSystemHandleWords handle,
                  const std::uint32_t opaque) override {
        Check(system == 0x5000u && handle.id == 0x42u &&
                  handle.system == 0x5000u &&
                  (slot == 12u || slot == 16u) &&
                  (opaque == 0xaaaau || opaque == 0xbbbbu),
              "copied handle and native dispatch arguments");
        calls.emplace_back(slot == 12u ? "add" : "process");
        if (throw_on_dispatch)
            throw std::runtime_error("dispatch failed");
    }
    std::uint32_t Reset(pal4::EffectSystemHandleWords& handle) override {
        Check(handle.id == 0x42u && handle.system == 0x5000u,
              "reset receives original temporary handle");
        calls.emplace_back("reset");
        handle = {0xffffffffu, 0u};
        return 0x12345678u;
    }
};

void TestQueue() {
    pal4::EffectObjectState state{};
    state.words[4u] = 0x5000u;
    Runtime runtime;
    Check(pal4::EffectObjectQueue::Add(state, 0x42u, 0xaaaau, runtime) ==
              0x12345678u &&
              pal4::EffectObjectQueue::Process(
                  state, 0x42u, 0xbbbbu, runtime) == 0x12345678u &&
              runtime.calls == std::vector<std::string>{
                  "add", "reset", "process", "reset"},
          "queue returns reset result after each dispatch slot");
    runtime.calls.clear();
    runtime.throw_on_dispatch = true;
    bool threw = false;
    try {
        (void)pal4::EffectObjectQueue::Add(state, 0x42u, 0xaaaau, runtime);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && runtime.calls == std::vector<std::string>{"add", "reset"},
          "exception unwinds temporary handle");
}

}  // namespace

int main() {
    TestQueue();
}
