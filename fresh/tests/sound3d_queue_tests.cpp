#include "pal4/sound3d_queue.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::ISound3DQueueRuntime {
    std::vector<std::string> events;
    std::uint32_t slot = 0u;
    std::uint32_t resets = 0u;
    bool fail_dispatch = false;
    void Dispatch(std::uint32_t system, std::uint32_t virtual_slot,
                  pal4::EffectSystemHandleWords handle,
                  std::uint32_t opaque) override {
        Check(system == 0x6000u && handle.id == 0x1234u &&
                  handle.system == 0x6000u && opaque == 0x9000u,
              "Sound3D copied handle and opaque queue argument");
        slot = virtual_slot;
        events.emplace_back("dispatch");
        if (fail_dispatch) throw std::runtime_error("dispatch");
    }
    std::uint32_t Reset(pal4::EffectSystemHandleWords& handle) override {
        Check(handle.id == 0x1234u && handle.system == 0x6000u,
              "temporary handle retained until reset");
        handle = {0xffffffffu, 0u};
        ++resets;
        events.emplace_back("reset");
        return 0x76543210u;
    }
};

}  // namespace

int main() {
    pal4::Sound3DObjectState state{};
    state.words[4u] = 0x6000u;
    Runtime runtime;
    Check(pal4::Sound3DQueue::Add(
              state, 0x1234u, 0x9000u, runtime) == 0x76543210u &&
              runtime.slot == 12u && runtime.resets == 1u &&
              runtime.events == std::vector<std::string>{"dispatch", "reset"},
          "add uses EffectSystem slot +12 and reset result");
    runtime.events.clear();
    Check(pal4::Sound3DQueue::Process(
              state, 0x1234u, 0x9000u, runtime) == 0x76543210u &&
              runtime.slot == 16u && runtime.resets == 2u &&
              runtime.events == std::vector<std::string>{"dispatch", "reset"},
          "process uses EffectSystem slot +16");
    for (const auto process : {false, true}) {
        runtime.events.clear();
        runtime.fail_dispatch = true;
        bool threw = false;
        try {
            if (process)
                (void)pal4::Sound3DQueue::Process(
                    state, 0x1234u, 0x9000u, runtime);
            else
                (void)pal4::Sound3DQueue::Add(
                    state, 0x1234u, 0x9000u, runtime);
        } catch (const std::runtime_error&) {
            threw = true;
        }
        Check(threw && runtime.events == std::vector<std::string>{
                  "dispatch", "reset"},
              "exception resets temporary handle for both slots");
    }
}
