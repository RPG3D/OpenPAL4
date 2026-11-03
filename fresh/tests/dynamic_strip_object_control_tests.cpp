#include "pal4/dynamic_strip_object_control.h"

#include <cstdint>
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

struct Runtime final : pal4::IDynamicStripObjectControlRuntime {
    std::vector<std::string> calls;
    void CleanupResources(pal4::DynamicStripObjectState& state) override {
        Check((state.words[1u] & 0xffu) != 0u,
              "resource cleanup runs while active");
        calls.push_back("cleanup_resources");
    }
    void ProcessEvent(std::uint32_t system,
                      pal4::DynamicStripObjectState& state) override {
        Check(system == 0x5000u &&
                  (state.words[1u] & 0xffu) == 0u &&
                  ((state.words[1u] >> 16u) & 0xffffu) == 0u,
              "owner event follows activity and display clear");
        calls.push_back("event");
    }
    std::uint32_t DeleteSelf(pal4::DynamicStripObjectState& state,
                              std::uint32_t flags) override {
        Check(flags == 1u && (state.words[1u] & 0xffu) == 0u,
              "deleting thunk is unconditional with flag one");
        calls.push_back("delete_self");
        return 0xabcdef01u;
    }
};

void TestStop() {
    pal4::DynamicStripObjectState state{};
    state.words[1u] = 0x11223301u;
    state.words[4u] = 0x5000u;
    Runtime runtime;
    Check(pal4::DynamicStripObjectControl::StopAndDestroy(
              state, runtime) == 0xabcdef01u &&
              runtime.calls == std::vector<std::string>{
                  "cleanup_resources", "event", "delete_self"} &&
              state.words[1u] == 0x00003300u,
          "active stop cleans resources, flags and owner in order");
    runtime.calls.clear();
    state.words[1u] = 0x12340000u;
    Check(pal4::DynamicStripObjectControl::StopAndDestroy(
              state, runtime) == 0xabcdef01u &&
              runtime.calls == std::vector<std::string>{"delete_self"} &&
              state.words[1u] == 0x12340000u,
          "inactive stop directly invokes deleting thunk");
}

}  // namespace

int main() {
    TestStop();
}
