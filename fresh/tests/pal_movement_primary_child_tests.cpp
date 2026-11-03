#include "pal4/pal_movement_primary_child.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IPalMovementPrimaryChildRuntime {
    unsigned calls = 0u;
    std::vector<std::string> cleanup_calls;
    void InitializeConfig(pal4::PalMovementPrimaryChildState& state) override {
        ++calls;
        Check(state.words[0] == 0xaaaaaaaau &&
                  state.words[27] == 0xaaaaaaaau &&
                  state.words[28] == 0xaaaaaaaau,
              "config initialization precedes constructor field writes");
        state.words[1] = 0x12345678u;
        state.words[27] = 0x11111111u;
    }
    std::uint32_t CleanupConfig2(
        pal4::PalMovementPrimaryChildState& state) override {
        Check(state.words[0] == 0x842760u,
              "destructor installs base vtable before config cleanup");
        cleanup_calls.emplace_back("cleanup2");
        return 0x1234u;
    }
    std::uint32_t CleanupConfig3(
        pal4::PalMovementPrimaryChildState& state) override {
        Check(state.words[0] == 0x84272cu,
              "alternate cleanup forwards without vtable reset");
        cleanup_calls.emplace_back("cleanup3");
        return 0x5678u;
    }
    void Free(std::uint32_t address) override {
        Check(address == 0x2000u,
              "deleting destructor frees original object address");
        cleanup_calls.emplace_back("free");
    }
};

}  // namespace

int main() {
    pal4::PalMovementPrimaryChildState state;
    state.words.fill(0xaaaaaaaau);
    Runtime runtime;
    auto& result = pal4::PalMovementPrimaryChild::Construct(state, runtime);
    Check(&result == &state && runtime.calls == 1u &&
              state.words[0] == 0x84272cu &&
              state.words[1] == 0x12345678u &&
              state.words[26] == 0xaaaaaaaau &&
              state.words[27] == 0xffffffffu &&
              state.words[28] == 0u,
          "constructor installs vtable and two tail fields after config init");
    state.words[13] = 0xabcdef01u;
    Check(pal4::PalMovementPrimaryChild::AlwaysReady(
              state, 0u, 0xffffffffu, 7u) == 1u &&
              pal4::PalMovementPrimaryChild::ReadField34(state) ==
                  0xabcdef01u &&
              pal4::PalMovementPrimaryChild::ReadField6C(state) ==
                  0xffffffffu &&
              pal4::PalMovementPrimaryChild::ReadField70(state) == 0u,
          "constant slot and three vtable accessors use confirmed fields");
    Check(pal4::PalMovementPrimaryChild::CleanupAlternate(
              state, runtime) == 0x5678u &&
              runtime.cleanup_calls ==
                  std::vector<std::string>{"cleanup3"},
          "alternate cleanup thunk forwards raw result");
    runtime.cleanup_calls.clear();
    Check(pal4::PalMovementPrimaryChild::Delete(
              state, 0x2000u, 2u, runtime) == 0x2000u &&
              state.words[0] == 0x842760u &&
              runtime.cleanup_calls ==
                  std::vector<std::string>{"cleanup2"},
          "deleting destructor skips free when low flag bit is clear");
    state.words[0] = 0x84272cu;
    runtime.cleanup_calls.clear();
    Check(pal4::PalMovementPrimaryChild::Delete(
              state, 0x2000u, 3u, runtime) == 0x2000u &&
              runtime.cleanup_calls ==
                  std::vector<std::string>{"cleanup2", "free"},
          "deleting destructor cleans before conditional free");

    state.words[0] = 0x84272cu;
    runtime.cleanup_calls.clear();
    Check(pal4::PalMovementPrimaryChild::Destruct(
              state, runtime) == 0x1234u &&
              runtime.cleanup_calls ==
                  std::vector<std::string>{"cleanup2"},
          "base destructor returns config cleanup value");

    state.words.fill(0x55555555u);
    pal4::PalMovementPrimaryChildRuntimeStub stub;
    static_cast<void>(pal4::PalMovementPrimaryChild::Construct(state, stub));
    Check(state.words[0] == 0x84272cu &&
              state.words[1] == 0x55555555u &&
              state.words[27] == 0xffffffffu &&
              state.words[28] == 0u,
          "linkable config stub preserves other child fields");
    Check(pal4::PalMovementPrimaryChild::Destruct(state, stub) == 0u &&
              state.words[0] == 0x842760u,
          "linkable cleanup stub preserves base vtable transition");

    state.words[0] = 0x84272cu;
    runtime.cleanup_calls.clear();
    Check(pal4::PalMovementBaseChild::Destruct(state, runtime) == 0x1234u &&
              runtime.cleanup_calls == std::vector<std::string>{"cleanup2"},
          "base child cleanup installs its vtable and forwards result");
    state.words[0] = 0x84272cu;
    runtime.cleanup_calls.clear();
    Check(pal4::PalMovementBaseChild::Delete(
              state, 0x2000u, 0u, runtime) == 0x2000u &&
              runtime.cleanup_calls == std::vector<std::string>{"cleanup2"},
          "base deleting destructor skips free when flag is clear");
    state.words[0] = 0x84272cu;
    runtime.cleanup_calls.clear();
    Check(pal4::PalMovementBaseChild::Delete(
              state, 0x2000u, 1u, runtime) == 0x2000u &&
              runtime.cleanup_calls ==
                  std::vector<std::string>{"cleanup2", "free"},
          "base deleting destructor frees after cleanup");
}
