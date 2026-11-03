#include "pal4/script_wait_handler_base.h"
#include "pal4/script_wait_handler.h"

#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <new>
#include <vector>

namespace {

using State = pal4::ScriptWaitHandlerBaseState;
static_assert(sizeof(State) == 32);
static_assert(offsetof(State, closed) == 4);
static_assert(offsetof(State, wait_mode) == 5);
static_assert(offsetof(State, script_context) == 8);
static_assert(offsetof(State, marker) == 12);
static_assert(offsetof(State, mode_copy) == 16);
static_assert(offsetof(State, text_data) == 20);
static_assert(offsetof(State, text_end) == 24);
static_assert(offsetof(State, text_capacity) == 28);

void Check(bool value, const char* message) {
    if (!value) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IScriptWaitHandlerBaseRuntime {
    std::vector<void*> freed;
    void* seen_context = nullptr;
    unsigned flag_calls = 0;

    void Free(void* pointer) override {
        Check(pointer != nullptr, "free uses nonnull pointer");
        freed.push_back(pointer);
        std::free(pointer);
    }

    std::uint8_t SetExecutionFlag(void* script_context) override {
        seen_context = script_context;
        ++flag_calls;
        return 7;
    }
};

std::uint8_t* MakeText(std::uint8_t references) {
    auto* header = static_cast<std::uint8_t*>(std::malloc(8));
    Check(header != nullptr, "text allocation succeeds");
    header[0] = references;
    return header + 1;
}

}  // namespace

int main() {
    static const int base_marker = 1;
    static const int derived_marker = 2;
    static int context = 3;
    Runtime runtime;
    State state{};
    state.reserved_6[0] = 0xa1;
    state.reserved_6[1] = 0xb2;
    state.reserved_17[0] = 0xc3;
    state.reserved_17[1] = 0xd4;
    state.reserved_17[2] = 0xe5;
    Check(pal4::ScriptWaitHandlerBase::Initialize(
              state, 1, &context, &base_marker) == &state &&
              state.vtable == &base_marker && state.closed == 0 &&
              state.wait_mode == 1 && state.script_context == &context &&
              state.marker == -1 && state.mode_copy == 1 &&
              !state.text_data && !state.text_end && !state.text_capacity,
          "constructor writes 32-byte game prefix");
    Check(state.reserved_6[0] == 0xa1 && state.reserved_6[1] == 0xb2 &&
              state.reserved_17[0] == 0xc3 &&
              state.reserved_17[1] == 0xd4 &&
              state.reserved_17[2] == 0xe5,
          "constructor preserves unwritten bytes");

    pal4::ScriptWaitHandlerBase::Close(state);
    Check(pal4::ScriptWaitHandlerBase::IsClosed(state) == 1,
          "close writes byte one");
    Check(pal4::ScriptWaitHandlerBase::Flush(state, runtime) == 7 &&
              state.closed == 0 && runtime.flag_calls == 1 &&
              runtime.seen_context == &context,
          "waiting flush clears closed and forwards script context");
    pal4::ScriptWaitHandlerBase::Close(state);
    state.wait_mode = 0;
    Check(pal4::ScriptWaitHandlerBase::Flush(state, runtime) == 0 &&
              state.closed == 0 && runtime.flag_calls == 1,
          "nonwaiting flush skips script flag");
    state.wait_mode = 3;
    state.script_context = nullptr;
    Check(pal4::ScriptWaitHandlerBase::Flush(state, runtime) == 3 &&
              runtime.flag_calls == 1,
          "wait mode without context returns original mode byte");

    auto* shared = MakeText(2);
    state.vtable = &derived_marker;
    state.script_context = &context;
    state.text_data = shared;
    state.text_end = shared + 2;
    state.text_capacity = shared + 7;
    pal4::ScriptWaitHandlerBase::ReleaseFields(state, &base_marker, runtime);
    Check(state.vtable == &base_marker && !state.script_context &&
              !state.text_data && !state.text_end && !state.text_capacity &&
              shared[-1] == 1 && runtime.freed.empty(),
          "shared text decrements header and clears three slots");
    std::free(shared - 1);

    for (std::uint8_t references : {std::uint8_t{0}, std::uint8_t{0xff}}) {
        auto* text = MakeText(references);
        state.text_data = text;
        state.text_end = text + 1;
        state.text_capacity = text + 7;
        pal4::ScriptWaitHandlerBase::ReleaseFields(state, &base_marker, runtime);
        Check(runtime.freed.back() == text - 1 && !state.text_data,
              "zero and sentinel headers free the prefix allocation");
    }
    Check(runtime.freed.size() == 2, "both owning header cases free once");

    State stack{};
    pal4::ScriptWaitHandlerBase::Initialize(stack, 0, nullptr, &derived_marker);
    Check(pal4::ScriptWaitHandlerBase::DestroyAt(
              &stack, 2, &base_marker, runtime) == &stack &&
              runtime.freed.size() == 2 && stack.vtable == &base_marker,
          "deleting destructor ignores flags without bit zero");
    auto* heap = static_cast<State*>(std::malloc(sizeof(State)));
    Check(heap != nullptr, "handler allocation succeeds");
    ::new (heap) State{};
    pal4::ScriptWaitHandlerBase::Initialize(
        *heap, 1, &context, &derived_marker);
    Check(pal4::ScriptWaitHandlerBase::DestroyAt(
              heap, 3, &base_marker, runtime) == heap &&
              runtime.freed.size() == 3 && runtime.freed.back() == heap,
          "delete flag frees the handler object after field cleanup");
}
