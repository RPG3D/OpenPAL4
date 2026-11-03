#include "pal4/object_derived_lifecycle.h"

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
    pal4::ObjectMovementState* state = nullptr;
    std::vector<std::string> calls;
    std::uint32_t address = 0;
    std::uint32_t vtable = 0;
    std::uint32_t argument = 0;

    void ConstructBase(pal4::ObjectMovementState& object) override {
        calls.emplace_back("construct");
        Check(&object == state && object.words[0] == 0x1111u,
              "base constructor runs before derived vtable write");
    }
    void CleanupBase(pal4::ObjectMovementState& object) override {
        calls.emplace_back("cleanup");
        Check(&object == state &&
                  object.words[0] == pal4::ObjectDerivedLifecycle::kVtable,
              "derived vtable write precedes base cleanup");
    }
    void Free(std::uint32_t object_address) override {
        calls.emplace_back("free");
        address = object_address;
    }
    void CallCapturedSlotB0(std::uint32_t captured_vtable,
                            std::uint32_t value) override {
        calls.emplace_back("b0");
        vtable = captured_vtable;
        argument = value;
        state->words[0] = 0x2222u;
    }
    std::uint32_t CallCurrentSlotA0(
        pal4::ObjectMovementState& object) override {
        calls.emplace_back("a0");
        Check(&object == state && object.words[0] == 0x2222u,
              "second dispatch observes current vtable");
        return 0x3333u;
    }
};

void TestLifecycle() {
    pal4::ObjectMovementState state;
    state.words.fill(0x4444u);
    state.words[0] = 0x1111u;
    state.words[47] = 0x5555u;
    Runtime runtime;
    runtime.state = &state;
    Check(&pal4::ObjectDerivedLifecycle::Construct(state, runtime) == &state &&
              state.words[0] == pal4::ObjectDerivedLifecycle::kVtable &&
              state.words[47] == 0x5555u,
          "constructor returns object and only changes derived vtable");
    runtime.calls.clear();
    state.words[0] = 0x1111u;
    pal4::ObjectDerivedLifecycle::Cleanup(state, runtime);
    Check(runtime.calls == std::vector<std::string>{"cleanup"} &&
              state.words[47] == 0x5555u,
          "cleanup preserves extended object property");

    runtime.calls.clear();
    Check(pal4::ObjectDerivedLifecycle::Delete(
              state, 0x123400u, 0u, runtime) == 0x123400u &&
              runtime.calls == std::vector<std::string>{"cleanup"},
          "nondeleting destructor leaves allocation");
    runtime.calls.clear();
    Check(pal4::ObjectDerivedLifecycle::Delete(
              state, 0x123400u, 3u, runtime) == 0x123400u &&
              runtime.calls == std::vector<std::string>{"cleanup", "free"} &&
              runtime.address == 0x123400u,
          "deleting destructor frees after cleanup when bit zero is set");
}

void TestDispatch() {
    pal4::ObjectMovementState state;
    state.words[0] = 0x1111u;
    state.words[47] = 0x5555u;
    Runtime runtime;
    runtime.state = &state;
    Check(pal4::ObjectDerivedLifecycle::Dispatch(
              state, 0x6666u, 0x7777u, runtime) == 0x3333u &&
              runtime.calls == std::vector<std::string>{"b0", "a0"} &&
              runtime.vtable == 0x1111u && runtime.argument == 0x6666u &&
              state.words[47] == 0x5555u,
          "dispatch uses first argument and leaves second and property unused");
}

}  // namespace

int main() {
    TestLifecycle();
    TestDispatch();
    std::cout << "2 derived object lifecycle cases passed\n";
}
