#include "pal4/script_interpreter_base.h"

#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <new>
#include <string>
#include <vector>

namespace {

static_assert(sizeof(pal4::ScriptInterpreterBaseState) == 20);
static_assert(offsetof(pal4::ScriptInterpreterBaseState, full_path) == 4);
static_assert(offsetof(pal4::ScriptInterpreterBaseState, path) == 8);
static_assert(offsetof(pal4::ScriptInterpreterBaseState, name) == 12);
static_assert(offsetof(pal4::ScriptInterpreterBaseState, extra) == 16);

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IScriptInterpreterBaseRuntime {
    std::vector<std::string> calls;
    std::vector<void*> freed;
    pal4::ScriptInterpreterBaseState* state = nullptr;
    const void* base_vtable = nullptr;

    void* Allocate(std::size_t bytes) override {
        calls.push_back("allocate:" + std::to_string(bytes));
        return std::malloc(bytes);
    }
    void Free(void* pointer) override {
        Check(pointer != nullptr, "free receives nonnull pointer");
        if (state && base_vtable) {
            Check(state->vtable == base_vtable, "base vtable installed before free");
        }
        calls.push_back("free");
        freed.push_back(pointer);
        std::free(pointer);
    }
    std::size_t Length(const char* text) override {
        calls.push_back("length");
        return std::strlen(text);
    }
    void Copy(char* destination, const char* source) override {
        Check(destination != nullptr, "successful allocation required");
        calls.push_back("copy");
        std::strcpy(destination, source);
    }
};

}  // namespace

int main() {
    static const int base_marker = 1;
    static const int other_marker = 2;
    Runtime runtime;
    pal4::ScriptInterpreterBaseState state;
    state.vtable = &other_marker;
    state.full_path = reinterpret_cast<char*>(1);
    state.path = reinterpret_cast<char*>(1);
    state.name = reinterpret_cast<char*>(1);
    state.extra = reinterpret_cast<char*>(1);
    Check(pal4::ScriptInterpreterBase::Initialize(state, &base_marker) == &state,
          "initializer returns object identity");
    Check(state.vtable == &base_marker && !state.full_path && !state.path &&
              !state.name && !state.extra,
          "initializer clears four fields and installs base vtable");

    Check(!pal4::ScriptInterpreterBase::SetPath(state, nullptr, runtime) &&
              runtime.calls.empty(),
          "null input leaves state and runtime untouched");
    Check(pal4::ScriptInterpreterBase::SetFullPath(state, "world/a.cs", runtime) &&
              std::strcmp(state.full_path, "world/a.cs") == 0,
          "full-path slot receives copied text");
    Check(pal4::ScriptInterpreterBase::SetPath(state, "world", runtime) &&
              std::strcmp(state.path, "world") == 0,
          "path slot receives copied text");
    Check(pal4::ScriptInterpreterBase::SetName(state, "a", runtime) &&
              std::strcmp(state.name, "a") == 0,
          "name slot receives copied text");
    Check(pal4::ScriptInterpreterBase::SetName(state, "b", runtime) &&
              std::strcmp(state.name, "b") == 0,
          "replacement updates only the selected slot");
    const char empty[] = "";
    Check(pal4::ScriptInterpreterBase::GetFullPathOrEmpty(state, empty) ==
              state.full_path &&
              pal4::ScriptInterpreterBase::GetPathOrEmpty(state, empty) ==
                  state.path &&
              pal4::ScriptInterpreterBase::GetNameOrEmpty(state, empty) ==
                  state.name,
          "three getters select their own populated slot");
    const std::vector<std::string> set_calls{
        "length", "allocate:11", "copy", "length", "allocate:6", "copy",
        "length", "allocate:2", "copy", "free", "length", "allocate:2", "copy"};
    Check(runtime.calls == set_calls, "setters preserve free-length-allocate-copy order");

    runtime.calls.clear();
    runtime.freed.clear();
    runtime.state = &state;
    runtime.base_vtable = &base_marker;
    state.vtable = &other_marker;
    Check(pal4::ScriptInterpreterBase::SetPath(state, nullptr, runtime) == false &&
              state.path && runtime.calls.empty(),
          "null replacement preserves existing text");
    pal4::ScriptInterpreterBase::ReleaseFields(state, &base_marker, runtime);
    Check(state.vtable == &base_marker && !state.full_path && !state.path &&
              !state.name && !state.extra,
          "release resets vtable and nonnull slots");
    Check(runtime.calls == std::vector<std::string>({"free", "free", "free"}),
          "release frees populated slots in native order");
    runtime.calls.clear();
    pal4::ScriptInterpreterBase::ReleaseFields(state, &base_marker, runtime);
    Check(runtime.calls.empty(), "repeat release skips empty slots");
    Check(pal4::ScriptInterpreterBase::GetFullPathOrEmpty(state, empty) == empty &&
              pal4::ScriptInterpreterBase::GetPathOrEmpty(state, empty) == empty &&
              pal4::ScriptInterpreterBase::GetNameOrEmpty(state, empty) == empty,
          "three getters share the supplied empty-string identity");
    Check(!pal4::ScriptInterpreterBase::ReturnFalseTwoArgs(1, 0xffffffffu) &&
              pal4::ScriptInterpreterBase::GetEmptyString(empty) == empty &&
              pal4::ScriptInterpreterBase::ReturnMinusOne() == -1 &&
              pal4::ScriptInterpreterBase::ReturnTwoShared() == 2,
          "shared virtual defaults preserve constants and empty pointer");

    state.vtable = &other_marker;
    Check(pal4::ScriptInterpreterBase::DestroyAt(
              &state, 2, &base_marker, runtime) == &state &&
              state.vtable == &base_marker && runtime.calls.empty(),
          "deleting destructor ignores flags without bit zero");
    auto* heap = static_cast<pal4::ScriptInterpreterBaseState*>(
        std::malloc(sizeof(pal4::ScriptInterpreterBaseState)));
    Check(heap != nullptr, "heap object allocation");
    ::new (heap) pal4::ScriptInterpreterBaseState{};
    pal4::ScriptInterpreterBase::Initialize(*heap, &other_marker);
    Check(pal4::ScriptInterpreterBase::SetName(*heap, "heap", runtime),
          "heap object name setup");
    runtime.calls.clear();
    runtime.state = heap;
    Check(pal4::ScriptInterpreterBase::DestroyAt(
              heap, 3, &base_marker, runtime) == heap &&
              runtime.calls == std::vector<std::string>({"free", "free"}),
          "deleting destructor frees fields then object and returns original address");
}
