#include "pal4/strip_queue.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IStripQueueRuntime {
    std::vector<std::uint32_t> slots;
    std::uint32_t reset_count = 0u;
    bool throw_on_dispatch = false;
    bool throw_on_reset = false;

    void Dispatch(const std::uint32_t system, const std::uint32_t slot,
                  const pal4::EffectSystemHandleWords handle,
                  const std::uint32_t opaque) override {
        Check(system == 0x3456u && handle.id == 0x1234u &&
                  handle.system == system && opaque == 0x9000u,
              "copied owner system, id and opaque argument");
        slots.push_back(slot);
        if (throw_on_dispatch)
            throw std::runtime_error("dispatch");
    }

    std::uint32_t Reset(pal4::EffectSystemHandleWords& handle) override {
        Check(handle.id == 0x1234u && handle.system == 0x3456u,
              "temporary handle remains valid until reset");
        handle = {0xffffffffu, 0u};
        ++reset_count;
        if (throw_on_reset)
            throw std::runtime_error("reset");
        return 0x76543210u;
    }
};

}  // namespace

int main() {
    pal4::StripObjectState state{};
    state.words[4u] = 0x3456u;
    Runtime runtime;
    Check(pal4::StripQueue::Add(state, 0x1234u, 0x9000u, runtime) ==
                  0x76543210u && runtime.slots ==
                  std::vector<std::uint32_t>{12u} && runtime.reset_count == 1u,
          "add dispatches slot +12 then returns reset result");
    Check(pal4::StripQueue::Process(state, 0x1234u, 0x9000u, runtime) ==
                  0x76543210u && runtime.slots ==
                  std::vector<std::uint32_t>{12u, 16u} &&
                  runtime.reset_count == 2u,
          "process dispatches slot +16 then resets");

    runtime.throw_on_dispatch = true;
    for (const bool process : {false, true}) {
        const auto previous_resets = runtime.reset_count;
        bool caught = false;
        try {
            if (process)
                (void)pal4::StripQueue::Process(
                    state, 0x1234u, 0x9000u, runtime);
            else
                (void)pal4::StripQueue::Add(
                    state, 0x1234u, 0x9000u, runtime);
        } catch (const std::runtime_error&) {
            caught = true;
        }
        Check(caught && runtime.reset_count == previous_resets + 1u,
              "exception unwinds and resets the temporary handle");
    }
    runtime.throw_on_dispatch = false;
    runtime.throw_on_reset = true;
    const auto previous_resets = runtime.reset_count;
    bool caught = false;
    try {
        (void)pal4::StripQueue::Add(state, 0x1234u, 0x9000u, runtime);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && runtime.reset_count == previous_resets + 1u,
          "reset failure does not invoke reset twice");
}
