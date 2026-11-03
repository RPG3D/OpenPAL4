#include "pal4/effect_system_object_f_queue.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IEffectSystemObjectFQueueRuntime {
    std::vector<std::uint32_t> slots;
    std::vector<pal4::EffectSystemHandleWords> handles;
    std::vector<std::uint32_t> userdata;
    std::uint32_t resets = 0u;
    bool throw_on_dispatch = false;

    std::uint32_t Dispatch(std::uint32_t system, std::uint32_t slot,
                           pal4::EffectSystemHandleWords handle,
                           std::uint32_t opaque) override {
        Check(system == 0x12345678u && handle.system == system,
              "dispatch uses EffectSystem owner word at +16");
        slots.push_back(slot);
        handles.push_back(handle);
        userdata.push_back(opaque);
        if (throw_on_dispatch)
            throw std::runtime_error("dispatch failure");
        return 0xaabbcc00u | slot;
    }
    void Reset(pal4::EffectSystemHandleWords& handle) override {
        Check(handle.id == 0x99u && handle.system == 0x12345678u,
              "reset receives original local handle");
        handle = {0xffffffffu, 0u};
        ++resets;
    }
};

}  // namespace

int main() {
    Runtime runtime;
    pal4::EffectSystemObjectFState state{};
    state.words[4u] = 0x12345678u;
    Check(pal4::EffectSystemObjectFQueue::Add(
              state, 0x99u, 0x111u, runtime) == 0xaabbcc0cu &&
              pal4::EffectSystemObjectFQueue::Process(
                  state, 0x99u, 0x222u, runtime) == 0xaabbcc10u &&
              runtime.slots == std::vector<std::uint32_t>{12u, 16u} &&
              runtime.handles[0u].id == 0x99u &&
              runtime.handles[1u].id == 0x99u &&
              runtime.userdata == std::vector<std::uint32_t>{0x111u, 0x222u} &&
              runtime.resets == 2u,
          "two wrapper slots forward the handle and reset after dispatch");
    runtime.throw_on_dispatch = true;
    bool threw = false;
    try {
        static_cast<void>(pal4::EffectSystemObjectFQueue::Add(
            state, 0x99u, 0x333u, runtime));
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && runtime.resets == 3u &&
              runtime.userdata.back() == 0x333u,
          "exception unwind resets local handle");
}
