#include "pal4/script_binary_interpreter.h"

#include <cstdlib>
#include <cstring>
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

struct Runtime final : pal4::IScriptBinaryDataRuntime {
    std::vector<std::string> events;
    std::vector<std::size_t> allocation_sizes;
    std::vector<void*> freed_pointers;
    std::vector<std::string> memory_events;
    void* expected_binary_buffer{};
    pal4::ScriptBinaryDataState* appended_to{};
    const void* appended_source{};
    std::uint32_t appended_size{};

    void* Allocate(std::size_t bytes) override {
        allocation_sizes.push_back(bytes);
        memory_events.push_back("allocate");
        return new char[bytes];
    }
    void Free(void* pointer) override {
        freed_pointers.push_back(pointer);
        memory_events.push_back("free");
        events.push_back(pointer == expected_binary_buffer ? "binary" : "base");
        delete[] static_cast<char*>(pointer);
    }
    std::size_t Length(const char* text) override { return std::strlen(text); }
    void Copy(char* destination, const char* source) override {
        std::strcpy(destination, source);
    }
    void AppendData(pal4::ScriptBinaryDataState& data, const void* source,
                    std::uint32_t size) override {
        events.push_back("append");
        appended_to = &data;
        appended_source = source;
        appended_size = size;
    }
};

void TestDataStream() {
    pal4::ScriptBinaryDataState state{};
    Runtime runtime;
    const auto* vtable = reinterpret_cast<const void*>(0x3000u);
    state.read_offset = 9u;
    state.write_offset = 8u;
    state.buffer_ptr = reinterpret_cast<void*>(0x5000u);
    state.total_size = 7u;
    Check(pal4::ScriptBinaryDataStream::Initialize(state, vtable) == &state &&
              state.vtable == vtable && state.read_offset == 0u &&
              state.write_offset == 0u && state.buffer_ptr == nullptr &&
              state.total_size == 0u,
          "binary data constructor initializes 20-byte stream state");

    const char first[] = {'a', 'b', 'c'};
    const char second[] = {'d', 'e'};
    Check(pal4::ScriptBinaryDataStream::Append(state, first, 3u, runtime) == 3u &&
              runtime.allocation_sizes == std::vector<std::size_t>{3u} &&
              std::memcmp(state.buffer_ptr, first, 3u) == 0 &&
              state.read_offset == 0u && state.total_size == 3u,
          "first append allocates exact input size and advances write offset");
    void* first_buffer = state.buffer_ptr;
    Check(pal4::ScriptBinaryDataStream::Append(state, second, 2u, runtime) == 5u &&
              runtime.allocation_sizes == std::vector<std::size_t>{3u, 3u, 5u} &&
              runtime.memory_events ==
                  std::vector<std::string>{"allocate", "allocate", "free",
                                           "allocate", "free"} &&
              runtime.freed_pointers.size() == 2u &&
              runtime.freed_pointers[0] == first_buffer &&
              state.write_offset == 5u && state.total_size == 5u &&
              std::memcmp(state.buffer_ptr, "abcde", 5u) == 0,
          "second append copies through temporary and preserves prior bytes");

    char read[4]{};
    pal4::ScriptBinaryDataStream::Read(state, read, 2u);
    pal4::ScriptBinaryDataStream::Read(state, read + 2, 2u);
    Check(std::memcmp(read, "abcd", 4u) == 0 && state.read_offset == 4u &&
              state.write_offset == 5u && state.total_size == 5u,
          "reads advance only the read offset");
    void* final_buffer = state.buffer_ptr;
    pal4::ScriptBinaryDataStream::ReleaseFields(state, vtable, runtime);
    Check(state.vtable == vtable && state.buffer_ptr == nullptr &&
              state.read_offset == 4u && state.write_offset == 5u &&
              state.total_size == 5u &&
              runtime.freed_pointers.back() == final_buffer,
          "data release frees buffer and preserves offsets and total size");
    const auto freed_count = runtime.freed_pointers.size();
    pal4::ScriptBinaryDataStream::ReleaseFields(state, vtable, runtime);
    Check(runtime.freed_pointers.size() == freed_count,
          "empty data release performs no free");
}

Runtime* callback_runtime{};
const void* callback_source{};
std::uint32_t callback_result{};
std::uint32_t Callback(const void* source, std::uint32_t* size,
                       void** buffer) {
    callback_runtime->events.push_back("callback");
    Check(source == callback_source, "loader receives source identity");
    Check(*size == 0u && *buffer == nullptr,
          "loader receives cleared size and buffer slots");
    *size = 19u;
    *buffer = reinterpret_cast<void*>(0x6900u);
    return callback_result;
}

void TestConstructAndRelease() {
    pal4::ScriptBinaryInterpreterState state;
    Runtime runtime;
    const auto* base_vtable = reinterpret_cast<const void*>(0x1000u);
    const auto* data_vtable = reinterpret_cast<const void*>(0x3000u);
    const auto* binary_vtable = reinterpret_cast<const void*>(0x2000u);
    state.base.path = reinterpret_cast<char*>(0x4000u);
    state.data.buffer_ptr = reinterpret_cast<void*>(0x5000u);
    state.data.total_size = 99u;
    Check(pal4::ScriptBinaryInterpreter::Initialize(
              state, base_vtable, data_vtable, binary_vtable) == &state &&
              state.base.vtable == binary_vtable &&
              state.base.full_path == nullptr && state.base.path == nullptr &&
              state.base.name == nullptr && state.base.extra == nullptr &&
              state.data.vtable == data_vtable &&
              state.data.read_offset == 0u &&
              state.data.write_offset == 0u &&
              state.data.buffer_ptr == nullptr && state.data.total_size == 0u,
          "binary constructor initializes base and 20-byte data subobject");

    state.data.buffer_ptr = runtime.Allocate(5u);
    runtime.expected_binary_buffer = state.data.buffer_ptr;
    state.data.write_offset = 2u;
    state.data.total_size = 5u;
    state.base.path = static_cast<char*>(runtime.Allocate(3u));
    pal4::ScriptBinaryInterpreter::ReleaseFields(
        state, base_vtable, data_vtable, binary_vtable, runtime);
    Check(runtime.events == std::vector<std::string>{"binary", "base"} &&
              state.data.buffer_ptr == nullptr &&
              state.data.write_offset == 2u && state.data.total_size == 5u &&
              state.data.vtable == data_vtable &&
              state.base.path == nullptr && state.base.vtable == base_vtable,
          "binary data cleanup precedes base and preserves other data words");
    runtime.events.clear();
    pal4::ScriptBinaryInterpreter::ReleaseFields(
        state, base_vtable, data_vtable, binary_vtable, runtime);
    Check(runtime.events.empty(), "empty binary release does not free");
}

void TestBufferMethods() {
    pal4::ScriptBinaryInterpreterState state{};
    Runtime runtime;
    const char source[] = "csb";
    Check(pal4::ScriptBinaryInterpreter::GetBuffer(state) == nullptr &&
              pal4::ScriptBinaryInterpreter::GetSize(state) == 0u,
          "empty binary state getters return null buffer and zero size");
    state.data.read_offset = 4u;
    state.data.write_offset = 8u;
    state.data.total_size = 12u;
    Check(!pal4::ScriptBinaryInterpreter::AppendBuffer(state, nullptr, 3u, runtime) &&
              runtime.events.empty(), "null append is rejected without dispatch");
    Check(pal4::ScriptBinaryInterpreter::AppendBuffer(state, source, 3u, runtime) &&
              runtime.events == std::vector<std::string>{"append"} &&
              runtime.appended_to == &state.data &&
              runtime.appended_source == source && runtime.appended_size == 3u,
          "nonnull append dispatches to data subobject and returns true");
    runtime.events.clear();
    Check(pal4::ScriptBinaryInterpreter::ResetAndLoad(
              state, nullptr, Callback, runtime) == 0u &&
              state.data.read_offset == 4u && state.data.write_offset == 8u &&
              state.data.total_size == 12u && runtime.events.empty(),
          "null loader source preserves data and skips callback");

    state.data.buffer_ptr = runtime.Allocate(3u);
    runtime.expected_binary_buffer = state.data.buffer_ptr;
    callback_runtime = &runtime;
    callback_source = source;
    callback_result = 0x101u;
    Check(pal4::ScriptBinaryInterpreter::ResetAndLoad(
              state, source, Callback, runtime) == 1u &&
              runtime.events == std::vector<std::string>{"binary", "callback"} &&
              state.data.read_offset == 0u && state.data.write_offset == 0u &&
              pal4::ScriptBinaryInterpreter::GetBuffer(state) ==
                  reinterpret_cast<void*>(0x6900u) &&
              pal4::ScriptBinaryInterpreter::GetSize(state) == 19u,
          "loader frees old buffer, clears fields, forwards slots and truncates result");
    runtime.events.clear();
    state.data.buffer_ptr = nullptr;
    callback_result = 0u;
    Check(pal4::ScriptBinaryInterpreter::ResetAndLoad(
              state, source, Callback, runtime) == 0u &&
              runtime.events == std::vector<std::string>{"callback"},
          "callback return is propagated and empty old buffer is not freed");
}

}  // namespace

int main() {
    TestDataStream();
    TestConstructAndRelease();
    TestBufferMethods();
}
