#include "pal4/pal_movement_secondary_child.h"

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

struct Runtime final : pal4::IPalMovementSecondaryChildRuntime {
    std::vector<std::string> calls;
    std::vector<std::uint32_t> freed;

    void InitializeConfig(pal4::PalMovementSecondaryChildState& state) override {
        calls.emplace_back("init");
        Check(state.words[0] == 0xaaaaaaaau &&
                  state.words[27] == 0xaaaaaaaau,
              "config initialization precedes constructor writes");
        state.words[1] = 0x12345678u;
        state.words[27] = 0xdeadbeefu;
    }
    void Free(std::uint32_t address) override {
        calls.emplace_back("free");
        freed.push_back(address);
    }
    std::uint32_t CleanupConfig(
        pal4::PalMovementSecondaryChildState& state) override {
        calls.emplace_back("cleanup");
        Check(state.words[0] == 0x842760u &&
                  state.words[28] == 0u && state.words[29] == 0u &&
                  state.words[30] == 0u && state.words[31] == 0u,
              "base cleanup follows array release and vtable transition");
        return 0xfeed1234u;
    }
    void CleanupAlternate(
        pal4::PalMovementSecondaryChildState& state) override {
        calls.emplace_back("alternate");
        Check(state.words[31] == 0x99u,
              "alternate cleanup precedes callback-state reset");
    }
    std::uint32_t ReadElement(std::uint32_t address) override {
        calls.emplace_back("read");
        return address + 0x1000u;
    }
    std::uint32_t GetClump(std::uint32_t element) override {
        calls.emplace_back("clump");
        return element + 0x100u;
    }
    void ResetAtomicCallbacks(std::uint32_t clump) override {
        calls.emplace_back("reset");
        freed.push_back(clump);
    }
};

void TestConstructorAndAccessors() {
    pal4::PalMovementSecondaryChildState state;
    state.words.fill(0xaaaaaaaau);
    Runtime runtime;
    auto& result = pal4::PalMovementSecondaryChild::Construct(
        state, 0x52u, runtime);
    Check(&result == &state &&
              runtime.calls == std::vector<std::string>{"init"} &&
              state.words[0] == 0x8427a0u &&
              state.words[1] == 0x12345678u &&
              state.words[27] == 0xdeadbe52u &&
              state.words[28] == 0u && state.words[29] == 0u &&
              state.words[30] == 0u && state.words[31] == 0u &&
              state.words[32] == 0xffffffffu && state.words[33] == 0u &&
              state.words[26] == 0xaaaaaaaau,
          "constructor writes confirmed offsets and preserves untouched bytes");
    Check(pal4::PalMovementSecondaryChild::ReadField80(state) ==
              0xffffffffu &&
              pal4::PalMovementSecondaryChild::ReadField84(state) == 0u,
          "virtual accessors read fields at 0x80 and 0x84");
    state.words[32] = 0x34567890u;
    state.words[33] = 0xabcdef01u;
    Check(pal4::PalMovementSecondaryChild::ReadField80(state) ==
              0x34567890u &&
              pal4::PalMovementSecondaryChild::ReadField84(state) ==
                  0xabcdef01u,
          "accessors return raw field values");
}

void TestDestructionOrder() {
    pal4::PalMovementSecondaryChildState state;
    state.words.fill(0x77777777u);
    state.words[28] = 0x4500u;
    Runtime runtime;
    Check(pal4::PalMovementSecondaryChild::Destruct(state, runtime) ==
              0xfeed1234u &&
              runtime.calls ==
                  std::vector<std::string>{"free", "cleanup"} &&
              runtime.freed == std::vector<std::uint32_t>{0x4500u} &&
              state.words[0] == 0x842760u &&
              state.words[27] == 0x77777777u &&
              state.words[32] == 0x77777777u &&
              state.words[33] == 0x77777777u,
          "destructor frees array, clears its fields and returns cleanup result");

    runtime.calls.clear();
    runtime.freed.clear();
    state.words[28] = 0x4600u;
    Check(pal4::PalMovementSecondaryChild::Delete(
              state, 0x2000u, 2u, runtime) == 0x2000u &&
              runtime.calls ==
                  std::vector<std::string>{"free", "cleanup"} &&
              runtime.freed == std::vector<std::uint32_t>{0x4600u},
          "deleting destructor skips object free when low flag bit is clear");

    runtime.calls.clear();
    runtime.freed.clear();
    state.words[28] = 0u;
    Check(pal4::PalMovementSecondaryChild::Delete(
              state, 0x2000u, 3u, runtime) == 0x2000u &&
              runtime.calls ==
                  std::vector<std::string>{"free", "cleanup", "free"} &&
              runtime.freed ==
                  std::vector<std::uint32_t>{0u, 0x2000u},
          "deleting destructor frees member even when null, then object");
}

void TestLinkableStub() {
    pal4::PalMovementSecondaryChildState state;
    state.words.fill(0xffffffffu);
    pal4::PalMovementSecondaryChildRuntimeStub runtime;
    static_cast<void>(pal4::PalMovementSecondaryChild::Construct(
        state, 0x19u, runtime));
    Check(state.words[27] == 0xffffff19u &&
              pal4::PalMovementSecondaryChild::Destruct(state, runtime) == 0u &&
              state.words[0] == 0x842760u,
          "secondary child composes with linkable config and free stubs");
}

void TestCallbackCleanup() {
    pal4::PalMovementSecondaryChildState state;
    state.words[28] = 0x5000u;
    state.words[29] = 0x5008u;
    state.words[30] = 0x5020u;
    state.words[31] = 0x99u;
    Runtime runtime;
    Check(pal4::PalMovementSecondaryChild::ClearClumpCallbacks(
              state, runtime) == 0x5008u &&
              runtime.calls == std::vector<std::string>{
                  "alternate", "read", "clump", "reset",
                  "read", "clump", "reset"} &&
              runtime.freed ==
                  std::vector<std::uint32_t>{0x6100u, 0x6104u} &&
              state.words[28] == 0x5000u &&
              state.words[29] == 0x5000u &&
              state.words[30] == 0x5020u && state.words[31] == 0u,
          "cleanup traverses each pointer, resets clumps and returns old end");

    runtime.calls.clear();
    runtime.freed.clear();
    state.words[31] = 0x99u;
    Check(pal4::PalMovementSecondaryChild::ClearClumpCallbacks(
              state, runtime) == 0x5000u &&
              runtime.calls == std::vector<std::string>{"alternate"} &&
              runtime.freed.empty() && state.words[31] == 0u,
          "empty collection still calls config cleanup and clears state");
}

}  // namespace

int main() {
    TestConstructorAndAccessors();
    TestDestructionOrder();
    TestCallbackCleanup();
    TestLinkableStub();
}
