#include "pal4/critical_section_object.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::ICriticalSectionObjectRuntime {
    std::vector<std::string> events;
    void InitializeNative(pal4::CriticalSectionState& state) override {
        Check(state.vtable == 0x84a014u && state.available == 1u &&
                  state.running == 0u && state.handle == 0u &&
                  state.trailing == 0u,
              "Win32 init follows native field writes");
        events.emplace_back("initialize-native");
    }
    void DeleteNative(pal4::CriticalSectionState& state) override {
        Check(state.vtable == 0x84a014u,
              "destructor restores native vtable before deletion");
        events.emplace_back("delete-native");
    }
    void WaitForSingleObject(const std::uint32_t handle,
                             const std::uint32_t milliseconds) override {
        events.emplace_back("wait:" + std::to_string(handle) + ":" +
                            std::to_string(milliseconds));
    }
    void CloseHandle(const std::uint32_t handle) override {
        events.emplace_back("close:" + std::to_string(handle));
    }
    void LeaveNative(pal4::CriticalSectionState& state) override {
        Check(state.available == 1u && state.running == 0u &&
                  state.handle == 0u,
              "leaves native section after clearing active handle");
        events.emplace_back("leave-native");
    }
    [[nodiscard]] std::uint32_t InitialReturnWord() override {
        events.emplace_back("initial-return");
        return 0x123456abU;
    }
};

}  // namespace

int main() {
    using Section = pal4::CriticalSectionObject;
    pal4::CriticalSectionState state;
    state.native_section.fill(0x5au);
    state.reserved = {0x77u, 0x88u};
    state.handle = 42u;
    state.trailing = 99u;
    state.running = 1u;
    Runtime runtime;
    Check(&Section::Initialize(state, runtime) == &state &&
              state.vtable == Section::kVtable &&
              state.available == 1u && state.running == 0u &&
              state.handle == 0u && state.trailing == 0u &&
              state.native_section[0] == 0x5au &&
              state.reserved[0] == 0x77u &&
              runtime.events == std::vector<std::string>({
                  "initialize-native"}),
          "constructor preserves native storage and reserved bytes");
    runtime.events.clear();
    Check(Section::WaitAndRelease(state, 100u, runtime) == 0x12345600u &&
              runtime.events == std::vector<std::string>({
                  "initial-return"}),
          "inactive wait only defines low return byte");
    runtime.events.clear();
    state.running = 1u;
    Check(Section::WaitAndRelease(state, 100u, runtime) == 0u &&
              state.running == 1u && runtime.events.empty(),
          "active flag with null handle skips wait and retains flag");
    state.handle = 0xabcdu;
    state.available = 0u;
    Check(Section::WaitAndRelease(state, 250u, runtime) == 0xabcdu &&
              state.handle == 0u && state.available == 1u &&
              state.running == 0u &&
              runtime.events == std::vector<std::string>({
                  "wait:43981:250", "close:43981", "leave-native"}),
          "wait closes handle and leaves critical section in native order");
    runtime.events.clear();
    state.vtable = 0u;
    Section::Destroy(state, runtime);
    Check(runtime.events == std::vector<std::string>({"delete-native"}) &&
              state.vtable == Section::kVtable,
          "inactive destructor only restores vtable and deletes section");
    runtime.events.clear();
    state.running = 1u;
    state.available = 0u;
    state.handle = 0x55u;
    state.vtable = 0u;
    Section::Destroy(state, runtime);
    Check(runtime.events == std::vector<std::string>({
              "wait:85:300", "close:85", "leave-native", "delete-native"}) &&
              state.handle == 0u && state.running == 0u,
          "active destructor waits 300 ms then deletes native section");
    pal4::CriticalSectionObjectRuntimeStub stub;
    pal4::CriticalSectionState empty;
    Check(&Section::Initialize(empty, stub) == &empty,
          "Win32 boundary stub links");
    return EXIT_SUCCESS;
}
