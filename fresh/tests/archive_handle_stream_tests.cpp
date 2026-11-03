#include "pal4/archive_handle_stream.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

class Runtime final : public pal4::IArchiveHandleStreamRuntime {
public:
    std::vector<std::string> calls;
    std::vector<std::int32_t> positions;
    std::int32_t seek_result = -9;
    std::uint32_t read_result = 0u;
    std::uint32_t write_result = 0u;
    const void* buffer_expected = nullptr;
    std::uint32_t size_expected = 0u;

    std::int32_t Seek(std::uint32_t handle, std::int32_t offset,
                      std::int32_t origin) override {
        Check(handle == 0x6000u, "seek uses game file handle");
        calls.push_back("seek:" + std::to_string(offset) + ":" +
                        std::to_string(origin));
        return seek_result;
    }
    std::int32_t Position(std::uint32_t handle) override {
        Check(handle == 0x6000u && !positions.empty(),
              "position uses game file handle");
        calls.emplace_back("position");
        const auto result = positions.front();
        positions.erase(positions.begin());
        return result;
    }
    std::uint32_t Read(void* buffer, std::uint32_t size,
                        std::uint32_t count,
                        std::uint32_t handle) override {
        Check(buffer == buffer_expected && size == size_expected &&
                  count == 1u && handle == 0x6000u,
              "game file read arguments");
        calls.emplace_back("read");
        return read_result;
    }
    std::uint32_t Write(const void* buffer, std::uint32_t size,
                         std::uint32_t count,
                         std::uint32_t handle) override {
        Check(buffer == buffer_expected && size == size_expected &&
                  count == 1u && handle == 0x6000u,
              "game file write arguments");
        calls.emplace_back("write");
        return write_result;
    }
};

void Cases() {
    Runtime runtime;
    pal4::ArchiveHandleStream stream(runtime);
    pal4::ArchiveFileStreamState state{0xdeadu, 0xbeefu};
    Check(&stream.Construct(state, 0x6000u) == &state &&
              state.vtable == pal4::ArchiveHandleStream::kVtable &&
              state.file == 0x6000u &&
              runtime.calls == std::vector<std::string>{"seek:0:0"},
          "constructor installs handle vtable and seeks to start");

    runtime.calls.clear();
    runtime.positions = {23, 95};
    Check(stream.Size(state) == 95 &&
              runtime.calls == std::vector<std::string>{
                  "position", "seek:0:2", "position", "seek:23:0"},
          "size saves and restores the handle position field");
    runtime.calls.clear();
    Check(stream.SetPosition(state, -12) == -9 &&
              runtime.calls == std::vector<std::string>{"seek:-12:0"},
          "absolute seek forwards signed result");

    std::uint32_t data = 0x12345678u;
    runtime.buffer_expected = &data;
    runtime.size_expected = sizeof(data);
    runtime.calls.clear();
    Check(!stream.ReadElement(state, &data, sizeof(data)) &&
              runtime.calls == std::vector<std::string>{"read"},
          "zero read result fails");
    runtime.read_result = 3u;
    Check(stream.ReadElement(state, &data, sizeof(data)),
          "nonzero read result succeeds");
    runtime.calls.clear();
    Check(!stream.WriteElement(state, &data, sizeof(data)) &&
              runtime.calls == std::vector<std::string>{"write"},
          "zero write result fails");
    runtime.write_result = 3u;
    Check(stream.WriteElement(state, &data, sizeof(data)),
          "nonzero write result succeeds");

    pal4::ArchiveHandleStreamRuntimeStub stub;
    Check(stub.Seek(0u, 0, 0) == 0 && stub.Position(0u) == 0 &&
              stub.Read(nullptr, 0u, 1u, 0u) == 0u &&
              stub.Write(nullptr, 0u, 1u, 0u) == 0u,
          "native runtime stub links");
}

}  // namespace

int main() {
    try {
        Cases();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
