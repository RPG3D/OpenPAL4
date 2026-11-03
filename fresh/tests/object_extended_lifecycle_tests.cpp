#include "pal4/object_extended_lifecycle.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IObjectExtendedRuntime {
    std::vector<std::string> calls;
    std::uint32_t saved_vtable = 0;
    std::uint32_t saved_argument = 0;
    std::uint32_t saved_address = 0;

    void ConstructBase(pal4::ObjectMovementState& state) override {
        calls.emplace_back("construct");
        Check(state.words[0] == 0x1234u && state.words[47] == 0x5678u,
              "base construct precedes derived writes");
    }
    void CleanupBase(pal4::ObjectMovementState& state) override {
        calls.emplace_back("cleanup");
        Check(state.words[0] == pal4::ObjectExtendedLifecycle::kVtable &&
                  state.words[47] == 0u,
              "derived cleanup precedes base cleanup");
    }
    void Free(std::uint32_t address) override {
        calls.emplace_back("free");
        saved_address = address;
    }
    void CallCapturedSlotB0(std::uint32_t vtable,
                            std::uint32_t value) override {
        calls.emplace_back("b0");
        saved_vtable = vtable;
        saved_argument = value;
    }
    std::uint32_t CallCurrentSlotA0(
        pal4::ObjectMovementState& state) override {
        calls.emplace_back("a0");
        Check(state.words[47] == 0xabcdef01u,
              "property is stored before virtual dispatch");
        return 0xfeedbeefu;
    }
};

void TestConstruction() {
    pal4::ObjectMovementState state;
    state.words.fill(0x88888888u);
    state.words[0] = 0x1234u;
    state.words[47] = 0x5678u;
    Runtime runtime;
    Check(&pal4::ObjectExtendedLifecycle::Construct(state, runtime) == &state,
          "construct returns the same object");
    Check(runtime.calls == std::vector<std::string>{"construct"} &&
              state.words[0] == pal4::ObjectExtendedLifecycle::kVtable &&
              state.words[47] == 0u && state.words[46] == 0x88888888u,
          "construct writes only the observed derived fields");
}

void TestCleanupAndDelete() {
    pal4::ObjectMovementState state;
    Runtime runtime;
    state.words[0] = 0x1234u;
    state.words[47] = 0x5678u;
    pal4::ObjectExtendedLifecycle::Cleanup(state, runtime);
    Check(runtime.calls == std::vector<std::string>{"cleanup"},
          "cleanup delegates once");

    runtime.calls.clear();
    Check(pal4::ObjectExtendedLifecycle::Delete(
              state, 0x7000u, 2u, runtime) == 0x7000u &&
              runtime.calls == std::vector<std::string>{"cleanup"},
          "delete without bit zero leaves allocation in place");
    runtime.calls.clear();
    Check(pal4::ObjectExtendedLifecycle::Delete(
              state, 0x7000u, 3u, runtime) == 0x7000u &&
              runtime.calls == std::vector<std::string>{"cleanup", "free"} &&
              runtime.saved_address == 0x7000u,
          "delete with bit zero frees after cleanup");
}

void TestPropertyDispatch() {
    pal4::ObjectMovementState state;
    state.words[0] = 0x9900u;
    Runtime runtime;
    Check(pal4::ObjectExtendedLifecycle::SetPropertyAndDispatch(
              state, 0x4567u, 0xabcdef01u, runtime) == 0xfeedbeefu &&
              state.words[47] == 0xabcdef01u &&
              runtime.saved_vtable == 0x9900u &&
              runtime.saved_argument == 0x4567u &&
              runtime.calls == std::vector<std::string>{"b0", "a0"},
          "set property calls captured b0 then current a0");

    pal4::ObjectExtendedRuntimeStub stub;
    Check(pal4::ObjectExtendedLifecycle::SetPropertyAndDispatch(
              state, 0u, 42u, stub) == 0u,
          "linkable default runtime returns zero");
}

}  // namespace

int main() {
    TestConstruction();
    TestCleanupAndDelete();
    TestPropertyDispatch();
    std::cout << "3 extended object lifecycle cases passed\n";
}
