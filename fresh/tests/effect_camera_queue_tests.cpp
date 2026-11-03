#include "pal4/effect_camera_queue.h"

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

struct Runtime final : pal4::IEffectSystemObjectFQueueRuntime {
    std::vector<std::uint32_t> slots;
    std::vector<std::uint32_t> opaque_values;
    std::uint32_t resets = 0u;
    bool fail = false;

    std::uint32_t Dispatch(std::uint32_t system, std::uint32_t slot,
                           pal4::EffectSystemHandleWords handle,
                           std::uint32_t opaque) override {
        Check(system == 0x12345678u && handle.system == system &&
                  handle.id == 0x99u,
              "camera facet copies id and EffectSystem word +16");
        slots.push_back(slot);
        opaque_values.push_back(opaque);
        if (fail)
            throw std::runtime_error("dispatch failure");
        return 0xaabbcc00u | slot;
    }
    void Reset(pal4::EffectSystemHandleWords& handle) override {
        Check(handle.id == 0x99u && handle.system == 0x12345678u,
              "reset sees the original local handle");
        handle = {0xffffffffu, 0u};
        ++resets;
    }
};

}  // namespace

int main() {
    pal4::EffectCameraObjectState state{};
    state.words[4u] = 0x12345678u;
    Runtime runtime;
    Check(pal4::EffectCameraQueue::Add(state, 0x99u, 0x111u, runtime) ==
              0xaabbcc0cu &&
              pal4::EffectCameraQueue::Process(
                  state, 0x99u, 0x222u, runtime) == 0xaabbcc10u &&
              runtime.slots == std::vector<std::uint32_t>{12u, 16u} &&
              runtime.opaque_values ==
                  std::vector<std::uint32_t>{0x111u, 0x222u} &&
              runtime.resets == 2u,
          "both queue wrappers preserve dispatch result and reset handle");
    runtime.fail = true;
    bool threw = false;
    try {
        static_cast<void>(pal4::EffectCameraQueue::Add(
            state, 0x99u, 0x333u, runtime));
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && runtime.resets == 3u &&
              runtime.opaque_values.back() == 0x333u,
          "SEH cleanup tail resets the local handle on failure");
}
