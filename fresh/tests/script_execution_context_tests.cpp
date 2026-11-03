#include "pal4/script_execution_context.h"

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <new>
#include <string>
#include <vector>

namespace {

using State = pal4::ScriptExecutionContextState;
static_assert(sizeof(State) == 48);
static_assert(offsetof(State, script_engine) == 4);
static_assert(offsetof(State, address_byte_16) == 16);
static_assert(offsetof(State, buffer_20) == 20);
static_assert(offsetof(State, address_byte_32) == 32);
static_assert(offsetof(State, buffer_36) == 36);
static_assert(offsetof(State, buffer_44) == 44);

void Check(bool value, const char* message) {
    if (!value) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IScriptExecutionContextRuntime {
    const void* base_vtable = nullptr;
    std::vector<std::string> events;
    std::vector<void*> freed;

    void Cleanup(State& state) override {
        Check(state.vtable == base_vtable,
              "base vtable installed before script cleanup");
        events.push_back("cleanup");
    }

    void Free(void* pointer) override {
        events.push_back("free");
        freed.push_back(pointer);
        std::free(pointer);
    }
};

}  // namespace

int main() {
    static const int base_marker = 1;
    static const int derived_marker = 2;
    Runtime runtime;
    runtime.base_vtable = &base_marker;
    State state{};
    state.reserved_17[0] = 0x11;
    state.reserved_17[1] = 0x22;
    state.reserved_17[2] = 0x33;
    state.reserved_33[0] = 0x44;
    state.reserved_33[1] = 0x55;
    state.reserved_33[2] = 0x66;
    state.script_engine = &state;
    state.word_8 = 7;
    state.current_stack_record = 8;
    state.buffer_20 = &state;
    state.buffer_36 = &state;

    const auto high_address_byte = static_cast<std::uint8_t>(
        reinterpret_cast<std::uintptr_t>(&state) >> 24);
    Check(pal4::ScriptExecutionContext::Initialize(
              state, &base_marker) == &state &&
              state.vtable == &base_marker && !state.script_engine &&
              state.word_8 == 0 && state.current_stack_record == 0 &&
              state.address_byte_16 == high_address_byte &&
              state.address_byte_32 == high_address_byte &&
              !state.buffer_20 && !state.buffer_24 && !state.buffer_28 &&
              !state.buffer_36 && !state.buffer_40 && !state.buffer_44,
          "constructor writes the 48-byte script context prefix");
    Check(state.reserved_17[0] == 0x11 && state.reserved_17[1] == 0x22 &&
              state.reserved_17[2] == 0x33 &&
              state.reserved_33[0] == 0x44 &&
              state.reserved_33[1] == 0x55 &&
              state.reserved_33[2] == 0x66,
          "constructor preserves bytes not written by IDA body");

    auto* second_buffer = std::malloc(8);
    auto* first_buffer = std::malloc(8);
    Check(second_buffer && first_buffer, "buffer allocation succeeds");
    state.vtable = &derived_marker;
    state.buffer_36 = second_buffer;
    state.buffer_40 = &state;
    state.buffer_44 = &state;
    state.buffer_20 = first_buffer;
    state.buffer_24 = &state;
    state.buffer_28 = &state;
    pal4::ScriptExecutionContext::ReleaseFields(state, &base_marker, runtime);
    Check(runtime.events == std::vector<std::string>({"cleanup", "free", "free"}) &&
              runtime.freed == std::vector<void*>({second_buffer, first_buffer}),
          "script cleanup precedes +36 then +20 frees");
    Check(state.vtable == &base_marker && !state.buffer_36 &&
              !state.buffer_40 && !state.buffer_44 && !state.buffer_20 &&
              !state.buffer_24 && !state.buffer_28,
          "release clears both three-pointer spans");

    runtime.events.clear();
    runtime.freed.clear();
    Check(pal4::ScriptExecutionContext::DestroyAt(
              &state, 2, &base_marker, runtime) == &state &&
              runtime.events ==
                  std::vector<std::string>({"cleanup", "free", "free"}) &&
              runtime.freed == std::vector<void*>({nullptr, nullptr}),
          "nondeleting flag still calls both MemFree boundaries with null roots");

    auto* heap = static_cast<State*>(std::malloc(sizeof(State)));
    Check(heap != nullptr, "context allocation succeeds");
    ::new (heap) State{};
    pal4::ScriptExecutionContext::Initialize(*heap, &derived_marker);
    runtime.events.clear();
    runtime.freed.clear();
    Check(pal4::ScriptExecutionContext::DestroyAt(
              heap, 3, &base_marker, runtime) == heap &&
              runtime.events ==
                  std::vector<std::string>({"cleanup", "free", "free", "free"}) &&
              runtime.freed == std::vector<void*>({nullptr, nullptr, heap}),
          "deleting flag frees the object after its two buffer roots");
}
