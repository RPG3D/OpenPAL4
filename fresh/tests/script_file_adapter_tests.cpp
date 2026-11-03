#include "pal4/script_file_adapter.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <tuple>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IScriptFileAdapterRuntime {
    std::uint32_t script_data = 0u;
    std::uint32_t script_length = 91u;
    std::vector<std::uint32_t> positions;
    std::size_t position_index = 0u;
    std::vector<std::string> events;
    std::vector<std::tuple<std::uint32_t, std::int32_t,
                           std::uint32_t>> seeks;
    std::uint32_t read_result = 1u;
    std::uint32_t write_result = 1u;
    std::uint32_t last_buffer = 0u;
    std::uint32_t last_size = 0u;
    std::uint32_t last_count = 0u;
    std::uint32_t freed_object = 0u;
    const pal4::ScriptFileAdapterState* state = nullptr;

    std::uint32_t GetPackedScriptData() override {
        events.emplace_back("data");
        return script_data;
    }
    std::uint32_t GetPackedScriptLength(
        const std::uint32_t data, const std::uint32_t handle) override {
        Check(data == script_data && handle == 0x4000u,
              "packed script lookup receives data and handle");
        events.emplace_back("script_length");
        return script_length;
    }
    std::uint32_t GetPosition(const std::uint32_t handle) override {
        Check(handle == 0x4000u && position_index < positions.size(),
              "position query receives handle");
        events.emplace_back("tell");
        return positions[position_index++];
    }
    int Seek(const std::uint32_t handle, const std::int32_t offset,
             const std::uint32_t origin) override {
        Check(handle == 0x4000u, "seek receives handle");
        events.emplace_back("seek");
        seeks.emplace_back(handle, offset, origin);
        return -1;
    }
    std::uint32_t Read(const std::uint32_t buffer,
                       const std::uint32_t size,
                       const std::uint32_t count,
                       const std::uint32_t handle) override {
        Check(handle == 0x4000u, "read receives handle");
        events.emplace_back("read");
        last_buffer = buffer;
        last_size = size;
        last_count = count;
        return read_result;
    }
    std::uint32_t Write(const std::uint32_t buffer,
                        const std::uint32_t size,
                        const std::uint32_t count,
                        const std::uint32_t handle) override {
        Check(handle == 0x4000u, "write receives handle");
        events.emplace_back("write");
        last_buffer = buffer;
        last_size = size;
        last_count = count;
        return write_result;
    }
    void Free(const std::uint32_t object) override {
        Check(state != nullptr &&
                  state->vtable == pal4::ScriptFileAdapter::kBaseVtable &&
                  state->handle == 0u,
              "deleting destructor resets base before free");
        events.emplace_back("free");
        freed_object = object;
    }
};

void TestConstructionAndDestruction() {
    Runtime runtime;
    pal4::ScriptFileAdapter adapter(runtime);
    pal4::ScriptFileAdapterState state{0x1111u, 0x2222u};
    runtime.state = &state;
    Check(&pal4::ScriptFileAdapter::Construct(state, 0x4000u) == &state &&
              state.vtable == pal4::ScriptFileAdapter::kVtable &&
              state.handle == 0x4000u,
          "constructor writes derived vtable and file handle");
    Check(adapter.Destroy(state, 0x3000u, 0u) == 0x3000u &&
              state.vtable == pal4::ScriptFileAdapter::kBaseVtable &&
              state.handle == 0u && runtime.events.empty(),
          "nondeleting destructor resets state without freeing object");
    (void)pal4::ScriptFileAdapter::Construct(state, 0x4000u);
    Check(adapter.Destroy(state, 0x3000u, 3u) == 0x3000u &&
              runtime.freed_object == 0x3000u &&
              runtime.events == std::vector<std::string>{"free"},
          "deleting destructor frees after base reset for bit zero");
}

void TestPackedAndPhysicalSize() {
    Runtime runtime;
    pal4::ScriptFileAdapter adapter(runtime);
    pal4::ScriptFileAdapterState state{};
    (void)pal4::ScriptFileAdapter::Construct(state, 0x4000u);

    runtime.script_data = 0xa000u;
    runtime.script_length = 1234u;
    Check(adapter.GetSize(state) == 1234u &&
              runtime.events ==
                  (std::vector<std::string>{"data", "script_length"}) &&
              runtime.seeks.empty(),
          "packed script length bypasses physical file positioning");

    runtime.events.clear();
    runtime.script_data = 0u;
    runtime.positions = {17u, 482u, 17u};
    Check(adapter.GetSize(state) == 482u &&
              runtime.events ==
                  (std::vector<std::string>{
                      "data", "tell", "seek", "tell", "seek"}) &&
              runtime.seeks ==
                  (std::vector<std::tuple<std::uint32_t, std::int32_t,
                                          std::uint32_t>>{
                      {0x4000u, 0, 2u}, {0x4000u, 17, 0u}}),
          "physical size seeks to end and restores original position");
    Check(adapter.GetPosition(state) == 17u &&
              runtime.position_index == 3u,
          "position method forwards handle query result unchanged");
}

void TestReadAndWrite() {
    Runtime runtime;
    pal4::ScriptFileAdapter adapter(runtime);
    pal4::ScriptFileAdapterState state{};
    (void)pal4::ScriptFileAdapter::Construct(state, 0x4000u);
    Check(adapter.Read(state, 0x7000u, 12u) &&
              runtime.last_buffer == 0x7000u &&
              runtime.last_size == 12u && runtime.last_count == 1u,
          "read passes one item of requested byte size");
    runtime.read_result = 2u;
    Check(!adapter.Read(state, 0x7000u, 0u) &&
              runtime.last_size == 0u && runtime.last_count == 1u,
          "read succeeds only when native count equals one");
    Check(adapter.Write(state, 0x7100u, 9u) &&
              runtime.last_buffer == 0x7100u &&
              runtime.last_size == 9u && runtime.last_count == 1u,
          "write passes one item of requested byte size");
    runtime.write_result = 0u;
    Check(!adapter.Write(state, 0x7100u, 9u),
          "write rejects a short native item count");

    pal4::ScriptFileAdapterRuntimeStub stub;
    pal4::ScriptFileAdapter stub_adapter(stub);
    pal4::ScriptFileAdapterState empty{};
    (void)pal4::ScriptFileAdapter::Construct(empty, 0u);
    Check(stub_adapter.GetSize(empty) == 0u &&
              !stub_adapter.Read(empty, 0u, 0u),
          "runtime boundary stub links and returns empty results");
}

}  // namespace

int main() {
    TestConstructionAndDestruction();
    TestPackedAndPhysicalSize();
    TestReadAndWrite();
}
