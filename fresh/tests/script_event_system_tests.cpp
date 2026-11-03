#include "pal4/script_event_system.h"

#include <array>
#include <cstdlib>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IScriptEventSystemRuntime {
    std::uint32_t next = 0x1000u;
    std::vector<std::uint32_t> allocations;
    std::size_t allocation_index = 0u;
    bool throw_on_store = false;
    bool throw_on_assign = false;
    std::vector<std::string> calls;
    std::map<std::uint32_t, pal4::ScriptEventObjectImage> objects;
    std::map<std::uint32_t, pal4::ScriptIndexedAdapterImage> adapters;
    std::vector<std::uint32_t> pushed;

    void RegisterAtExit(std::uint32_t callback) override {
        calls.push_back("exit:" + std::to_string(callback));
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        calls.push_back("alloc:" + std::to_string(bytes));
        if (allocation_index < allocations.size())
            return allocations[allocation_index++];
        const auto address = next;
        next += bytes + 4u;
        return address;
    }
    void Free(std::uint32_t address) override {
        calls.push_back("free:" + std::to_string(address));
    }
    void StoreObject(std::uint32_t address,
                     const pal4::ScriptEventObjectImage& image) override {
        calls.push_back("object:" + std::to_string(address));
        if (throw_on_store) throw std::runtime_error("object constructor");
        objects[address] = image;
    }
    void AssignName(std::uint32_t object, std::uint32_t source,
                    bool error_string) override {
        calls.push_back("name:" + std::to_string(object) + ":" +
                        std::to_string(source) + ":" +
                        std::to_string(error_string));
    }
    void InitializeErrorString(pal4::StringBufferState& text,
                               const char* message,
                               std::uint8_t stack_byte) override {
        calls.push_back(std::string("error:") + message + ":" +
                        std::to_string(stack_byte));
        text = {0x6000u, 26u, 26u};
    }
    void StoreAdapter(std::uint32_t address,
                      const pal4::ScriptIndexedAdapterImage& image) override {
        calls.push_back("adapter:" + std::to_string(address));
        adapters[address] = image;
    }
    void AssignAdapter(std::uint32_t object, std::uint32_t offset,
                       std::uint32_t adapter, bool& owned) override {
        calls.push_back("assign:" + std::to_string(object) + ":" +
                        std::to_string(offset) + ":" +
                        std::to_string(adapter) + ":" +
                        std::to_string(owned));
        if (throw_on_assign) throw std::runtime_error("adapter assignment");
        owned = false;
    }
    void ReleaseTemporaryAdapter(std::uint32_t adapter,
                                 bool owned) override {
        calls.push_back("release:" + std::to_string(adapter) + ":" +
                        std::to_string(owned));
    }
    void PushBack(std::uint32_t object) override {
        calls.push_back("push:" + std::to_string(object));
        pushed.push_back(object);
    }
    void Sort() override { calls.push_back("sort"); }
    void CleanupCollection(std::uint32_t address) override {
        calls.push_back("cleanup:" + std::to_string(address));
    }
};

struct ErrorCleanupRuntime final : pal4::IScriptEventErrorCleanupRuntime {
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;

    std::uint8_t ReadByte(std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        return bytes.at(address);
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        calls.push_back("write:" + std::to_string(address) + ":" +
                        std::to_string(value));
        bytes[address] = value;
    }
    void Free(std::uint32_t address) override {
        calls.push_back("free:" + std::to_string(address));
    }
};

void TestObjectAndAdapterLayouts() {
    const auto image = pal4::ScriptEventSystem::MakeObject(0x7bu);
    Check(image.base_vtable == 0x842bbcu &&
              image.final_vtable == 0x842ba4u &&
              image.Dword(0u) == 0x842ba4u &&
              image.bytes[4] == 0u && image.bytes[12] == 0u &&
              image.bytes[20] == 0x7bu &&
              image.Dword(8u) == 0u && image.Dword(16u) == 0u &&
              image.Dword(24u) == 0u && image.Dword(28u) == 0u &&
              image.Dword(32u) == 0u &&
              !image.written[5] && !image.written[13] &&
              !image.written[21],
          "36-byte object records only the bytes written by its constructor");
    const auto reader = pal4::ScriptEventSystem::MakeAdapter(0x42f140u, 5u);
    const auto writer = pal4::ScriptEventSystem::MakeAdapter(0x42f160u, 5u);
    Check(reader.vtable == 0x842b9cu && reader.callback == 0x42f140u &&
              reader.index == 5u && writer.vtable == 0x842be4u &&
              writer.callback == 0x42f160u && writer.index == 5u,
          "read and write adapters retain distinct vtables and callbacks");
    pal4::ScriptIndexedAdapterImage reconstructed{};
    Check(pal4::ScriptEventSystem::ConstructWriteAdapter(
              reconstructed, 0x7000u, 0x42f160u, 3u) == 0x7000u &&
              reconstructed.vtable == 0x842be4u &&
              reconstructed.callback == 0x42f160u &&
              reconstructed.index == 3u,
          "12-byte write-adapter constructor returns its original address");
}

void TestFirstAndRepeatedInitialization() {
    pal4::ScriptEventSystemState state;
    for (auto& collection : state.collections)
        collection.fill(0xa5a5a5a5u);
    state.setup_guard = 0x80u;
    Runtime runtime;
    Check(pal4::ScriptEventSystem::Initialize(state, 0x4au, runtime) ==
              pal4::ScriptEventSystem::kCollectionAddress &&
              state.setup_guard == 0x81u && state.population_guard == 1u &&
              state.error_guard == 0u && runtime.pushed.size() == 6u &&
              runtime.objects.size() == 6u && runtime.adapters.size() == 12u,
          "initialization constructs all six descriptor objects once");
    for (const auto& collection : state.collections)
        Check(collection == std::array<std::uint32_t, 4>{
                    0xa5a5a54au, 0u, 0u, 0u},
              "collection initialization keeps unwritten high bytes");
    Check(runtime.calls.front() == "exit:4385856" &&
              runtime.calls[1] == "alloc:36" &&
              runtime.calls[3] == "name:4096:9364856:0" &&
              runtime.calls.back() == "sort",
          "cleanup registration, object naming and final sort follow IDA order");
    const auto& last = runtime.adapters.rbegin()->second;
    Check(last.index == 5u, "last adapter uses descriptor index five");
    const auto call_count = runtime.calls.size();
    Check(pal4::ScriptEventSystem::Initialize(state, 0x66u, runtime) ==
              pal4::ScriptEventSystem::kCollectionAddress &&
              runtime.calls.size() == call_count,
          "both guards suppress repeated work");
}

void TestFailedAllocationsAndErrorBoundary() {
    pal4::ScriptEventSystemState state;
    Runtime runtime;
    runtime.allocations = {0u, 0u, 0u};
    Check(pal4::ScriptEventSystem::Initialize(
              state, 0x33u, runtime, 0x2000u, 0x2010u) ==
              pal4::ScriptEventSystem::kCollectionAddress &&
              runtime.pushed == std::vector<std::uint32_t>{0u} &&
              runtime.objects.empty() && runtime.adapters.empty() &&
              runtime.calls.back() == "sort",
          "null allocations still flow through name, assignment and insertion");
    Check(runtime.calls[2] == "name:0:8192:0" &&
              runtime.calls[4] == "assign:0:4:0:0" &&
              runtime.calls[7] == "assign:0:12:0:0",
          "allocation failure preserves the original null arguments");

    pal4::ScriptEventSystemState empty_state;
    Runtime empty_runtime;
    (void)pal4::ScriptEventSystem::Initialize(
        empty_state, 0x55u, empty_runtime, 0x3000u, 0x3000u);
    Check(empty_state.error_guard == 1u &&
              empty_state.error_first_byte == 0x55u &&
              empty_state.error_text.data == 0x6000u &&
              empty_runtime.pushed.size() == 1u &&
              empty_runtime.calls[3] ==
                  "error:getEnumStringInArray Error:85" &&
              empty_runtime.calls[4] == "exit:4386992" &&
              empty_runtime.calls[5] == "name:4096:9931552:1",
          "do-loop error branch initializes and registers its global string");
}

void TestExceptionCleanup() {
    pal4::ScriptEventSystemState state;
    Runtime runtime;
    runtime.throw_on_store = true;
    bool threw = false;
    try {
        (void)pal4::ScriptEventSystem::Initialize(
            state, 0u, runtime, 0x1000u, 0x1010u);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && state.population_guard == 1u &&
              runtime.calls == std::vector<std::string>{
                  "exit:4385856", "alloc:36", "object:4096", "free:4096"},
          "constructor exception frees its allocation and retains guard");

    pal4::ScriptEventSystemState second_state;
    Runtime second_runtime;
    second_runtime.throw_on_assign = true;
    threw = false;
    try {
        (void)pal4::ScriptEventSystem::Initialize(
            second_state, 0u, second_runtime, 0x1000u, 0x1010u);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && second_runtime.calls.back() == "release:4136:1",
          "failed adapter assignment releases its still-owned temporary");
}

void TestEnumStringItem() {
    pal4::ScriptEventSystemState state;
    Runtime runtime;
    Check(pal4::ScriptEventSystem::GetEnumStringItem(
              0x1000u, 2, 3, state, runtime) == 0x1020u &&
              pal4::ScriptEventSystem::GetEnumStringItem(
                  0x1000u, -1, 3, state, runtime) == 0x0ff0u &&
              runtime.calls.empty(),
          "in-range and negative indices use 16-byte address arithmetic");

    state.error_guard = 0x80u;
    state.error_text = {0x7777u, 9u, 9u};
    Check(pal4::ScriptEventSystem::GetEnumStringItem(
              0x1000u, 3, 3, state, runtime) == 0x978b20u &&
              state.error_guard == 0x81u &&
              state.error_first_byte == 3u &&
              state.error_text.data == 0x6000u &&
              state.error_text.length == 26u &&
              runtime.calls == std::vector<std::string>{
                  "error:getEnumStringInArray Error:3",
                  "exit:4386992"},
          "first out-of-range index initializes global error text and exit hook");

    const auto call_count = runtime.calls.size();
    Check(pal4::ScriptEventSystem::GetEnumStringItem(
              0x2000u, 300, 3, state, runtime) == 0x978b20u &&
              state.error_first_byte == 3u &&
              runtime.calls.size() == call_count,
          "later failures keep the first index byte and do not register again");

    pal4::ScriptEventSystemState wide_index;
    Runtime wide_runtime;
    (void)pal4::ScriptEventSystem::GetEnumStringItem(
        0x1000u, 300, 3, wide_index, wide_runtime);
    Check(wide_index.error_first_byte == 44u,
          "error record stores only the index low byte");
}

void TestErrorStringCleanup() {
    pal4::ScriptEventSystemState state;
    state.error_guard = 1u;
    state.error_first_byte = 0x55u;
    state.error_text = {0x6000u, 4u, 8u};
    ErrorCleanupRuntime runtime;
    runtime.bytes[0x5fffu] = 2u;
    pal4::ScriptEventSystem::CleanupErrorString(state, runtime);
    Check(state.error_guard == 1u && state.error_first_byte == 0x55u &&
              state.error_text.data == 0u &&
              state.error_text.length == 0u &&
              state.error_text.capacity == 0u &&
              runtime.calls == std::vector<std::string>{
                  "read:24575", "write:24575:1"},
          "error-text cleanup decrements a shared header and keeps guards");

    state.error_text = {0x6000u, 4u, 8u};
    runtime.calls.clear();
    runtime.bytes[0x5fffu] = 0u;
    pal4::ScriptEventSystem::CleanupErrorString(state, runtime);
    Check(runtime.calls == std::vector<std::string>{
              "read:24575", "free:24575"},
          "zero reference frees the error-text allocation");

    runtime.calls.clear();
    pal4::ScriptEventSystem::CleanupErrorString(state, runtime);
    Check(runtime.calls.empty(), "null error-text cleanup only zeros fields");
}

}  // namespace

int main() {
    TestObjectAndAdapterLayouts();
    TestFirstAndRepeatedInitialization();
    TestFailedAllocationsAndErrorBoundary();
    TestExceptionCleanup();
    TestEnumStringItem();
    TestErrorStringCleanup();
    Runtime cleanup_runtime;
    pal4::ScriptEventSystem::Cleanup(cleanup_runtime);
    Check(cleanup_runtime.calls == std::vector<std::string>{
              "cleanup:9323044"},
          "exit callback forwards the fixed collection address");
    pal4::ScriptEventSystemState state;
    pal4::ScriptEventSystemRuntimeStub stub;
    Check(pal4::ScriptEventSystem::Initialize(state, 0u, stub) ==
              pal4::ScriptEventSystem::kCollectionAddress,
          "default runtime links");
    pal4::ScriptEventErrorCleanupRuntimeStub error_stub;
    pal4::ScriptEventSystem::CleanupErrorString(state, error_stub);
    std::cout << "6 script event system cases passed\n";
}
