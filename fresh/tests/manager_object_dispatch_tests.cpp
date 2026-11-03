#include "pal4/manager_object_dispatch.h"

#include <cstdlib>
#include <iostream>
#include <utility>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Runtime final : public pal4::IManagerObjectDispatchRuntime {
public:
    std::uint32_t Invoke(const std::uint32_t target,
                         const std::uint32_t slot_offset) override {
        calls.emplace_back(target, slot_offset);
        return result;
    }
    std::uint32_t result{0x123456abu};
    std::vector<std::pair<std::uint32_t, std::uint32_t>> calls;
};

pal4::ManagerObjectDispatchState InitialState() {
    pal4::ManagerObjectDispatchState state{};
    state.initialized = 1u;
    state.enabled = 1u;
    state.primary_target = 0x8100u;
    state.secondary_target = 0x9100u;
    return state;
}

void BothGuardsSkipVirtualDispatch() {
    auto state = InitialState();
    Runtime runtime;
    state.initialized = 0u;
    Check(pal4::ManagerObjectDispatch::Begin(state, runtime) == 0u &&
              pal4::ManagerObjectDispatch::End(state, runtime) == 0u &&
              pal4::ManagerObjectDispatch::BeginSecondary(state, runtime) == 0u &&
              pal4::ManagerObjectDispatch::EndSecondary(state, runtime) == 0u &&
              runtime.calls.empty(),
          "unset first flag skips target dereference on all four methods");
    state.initialized = 1u;
    state.enabled = 0u;
    Check(pal4::ManagerObjectDispatch::Begin(state, runtime) == 0u &&
              pal4::ManagerObjectDispatch::EndSecondary(state, runtime) == 0u &&
              runtime.calls.empty(),
          "unset second flag skips target dereference");
}

void VirtualSlotsAndTargetOffsets() {
    const auto state = InitialState();
    Runtime runtime;
    Check(pal4::ManagerObjectDispatch::Begin(state, runtime) == 0xabu &&
              pal4::ManagerObjectDispatch::End(state, runtime) == 0xabu &&
              pal4::ManagerObjectDispatch::BeginSecondary(state, runtime) == 0xabu &&
              pal4::ManagerObjectDispatch::EndSecondary(state, runtime) == 0xabu,
          "dispatch returns low byte of callee result");
    Check(runtime.calls == std::vector<std::pair<std::uint32_t, std::uint32_t>>({
              {0x8100u, 284u}, {0x8100u, 288u},
              {0x9100u, 284u}, {0x9100u, 288u}}),
          "common and key-14 variants use observed pointer fields and slots");
    pal4::ManagerObjectDispatchRuntimeStub stub;
    Check(pal4::ManagerObjectDispatch::Begin(state, stub) == 0u,
          "linkable virtual-call stub returns zero");
    Check(pal4::ManagerObjectDispatch::ReturnFalse(0u, 0u) == 0u &&
              pal4::ManagerObjectDispatch::ReturnFalse(0xffffffffu,
                                                         0x12345678u) == 0u,
          "shared manager virtual method ignores both arguments");
}

}  // namespace

int main() {
    BothGuardsSkipVirtualDispatch();
    VirtualSlotsAndTargetOffsets();
    std::cout << "2 manager object dispatch test cases passed\n";
}
