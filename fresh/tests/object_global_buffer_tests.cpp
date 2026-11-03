#include "pal4/object_global_buffer.h"

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

struct Runtime final : pal4::IObjectGlobalBufferRuntime {
    pal4::ObjectGlobalBufferState* state = nullptr;
    std::vector<std::string> calls;
    std::uint32_t freed = 0;

    int RegisterCleanup(std::uint32_t callback) override {
        calls.emplace_back("register");
        Check(callback == pal4::ObjectGlobalBuffer::kCleanupAddress &&
                  state->source_byte == 0x12u &&
                  state->words == std::array<std::uint32_t, 3>{0u, 0u, 0u},
              "initialization precedes registration of the observed callback");
        return -37;
    }

    void Free(std::uint32_t address) override {
        calls.emplace_back("free");
        freed = address;
        Check((state->cleanup_guard & 1u) != 0u &&
                  state->words[0] == address,
              "cleanup guard is set before free, fields clear afterward");
    }
};

void TestInitializationAndRegistration() {
    pal4::ObjectGlobalBufferState state;
    state.cleanup_guard = 0x80u;
    state.reserved0.fill(0x55u);
    state.reserved1.fill(0x66u);
    state.words = {11u, 22u, 33u};
    Runtime runtime;
    runtime.state = &state;
    Check(pal4::ObjectGlobalBuffer::kStateAddress == 0x8e41f8u &&
              pal4::ObjectGlobalBuffer::InitializeAndRegister(
                  state, 0x12345678u, runtime) == -37 &&
              runtime.calls == std::vector<std::string>{"register"} &&
              state.cleanup_guard == 0x80u &&
              state.reserved0 == std::array<std::uint8_t, 3>{0x55u, 0x55u, 0x55u} &&
              state.reserved1 == std::array<std::uint8_t, 3>{0x66u, 0x66u, 0x66u},
          "initializer uses ECX high byte and preserves other global bytes");
}

void TestCleanupOnce() {
    pal4::ObjectGlobalBufferState state;
    state.cleanup_guard = 0x80u;
    state.source_byte = 0x77u;
    state.words = {0x4000u, 22u, 33u};
    Runtime runtime;
    runtime.state = &state;
    pal4::ObjectGlobalBuffer::CleanupOnce(state, runtime);
    Check(state.cleanup_guard == 0x81u && state.source_byte == 0x77u &&
              state.words == std::array<std::uint32_t, 3>{0u, 0u, 0u} &&
              runtime.freed == 0x4000u &&
              runtime.calls == std::vector<std::string>{"free"},
          "first cleanup frees the first word and clears all three words");
    pal4::ObjectGlobalBuffer::CleanupOnce(state, runtime);
    Check(runtime.calls == std::vector<std::string>{"free"},
          "set guard suppresses repeated cleanup");
}

void TestFreeZeroPointer() {
    pal4::ObjectGlobalBufferState state;
    Runtime runtime;
    runtime.state = &state;
    pal4::ObjectGlobalBuffer::CleanupOnce(state, runtime);
    Check(runtime.calls == std::vector<std::string>{"free"} &&
              runtime.freed == 0u,
          "first cleanup calls free even for a zero pointer");
}

}  // namespace

int main() {
    TestInitializationAndRegistration();
    TestCleanupOnce();
    TestFreeZeroPointer();
    pal4::ObjectGlobalBufferRuntimeStub stub;
    pal4::ObjectGlobalBufferState state;
    Check(pal4::ObjectGlobalBuffer::InitializeAndRegister(
              state, 0u, stub) == 0,
          "default runtime links");
    std::cout << "3 object global buffer cases passed\n";
}
