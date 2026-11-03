#include "pal4/cylinder_object_queue.h"

#include <cstdint>
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
    std::vector<std::uint32_t> opaque_values;
    std::uint32_t resets = 0u;
    bool throw_on_dispatch = false;

    std::uint32_t Dispatch(std::uint32_t system,
                           std::uint32_t slot,
                           pal4::EffectSystemHandleWords handle,
                           std::uint32_t opaque) override {
        Check(system == 0x5000u, "system from object +16");
        slots.push_back(slot);
        handles.push_back(handle);
        opaque_values.push_back(opaque);
        if (throw_on_dispatch)
            throw std::runtime_error("dispatch failure");
        return 0x12345678u;
    }
    void Reset(pal4::EffectSystemHandleWords& handle) override {
        ++resets;
        Check(handle.id == 0x42u && handle.system == 0x5000u,
              "reset receives copied handle");
        handle = {0xffffffffu, 0u};
    }
};

void TestDispatch() {
    pal4::CylinderObjectState state{};
    state.words[4u] = 0x5000u;
    Runtime runtime;
    using pal4::CylinderObjectQueue;
    Check(CylinderObjectQueue::Add(
              state, 0x42u, 0xaaaau, runtime) == 0x12345678u &&
              CylinderObjectQueue::Process(
                  state, 0x42u, 0xbbbbu, runtime) == 0x12345678u &&
              runtime.slots == std::vector<std::uint32_t>{12u, 16u} &&
              runtime.opaque_values ==
                  std::vector<std::uint32_t>{0xaaaau, 0xbbbbu} &&
              runtime.handles.size() == 2u &&
              runtime.handles[0u].id == 0x42u &&
              runtime.handles[0u].system == 0x5000u &&
              runtime.resets == 2u,
          "queue paths select native slots and reset local handles");
    runtime.throw_on_dispatch = true;
    try {
        static_cast<void>(CylinderObjectQueue::Add(
            state, 0x42u, 0xccccu, runtime));
        Check(false, "dispatch must throw");
    } catch (const std::runtime_error&) {
        Check(runtime.resets == 3u && runtime.slots.back() == 12u,
              "exception path resets copied handle");
    }
}

}  // namespace

int main() {
    TestDispatch();
}
