#include "pal4/archive_file_stream.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IArchiveFileStreamRuntime {
    std::vector<std::string> calls;
    std::vector<std::int32_t> tell_results;
    std::int32_t seek_result = -7;
    std::uint32_t read_result = 0u;
    std::uint32_t write_result = 0u;
    const void* expected_buffer = nullptr;
    std::uint32_t expected_size = 0u;

    std::int32_t Seek(std::uint32_t file, std::int32_t offset,
                      std::int32_t origin) override {
        Check(file == 0x5000u, "seek file handle");
        calls.push_back("seek:" + std::to_string(offset) + ":" +
                        std::to_string(origin));
        return seek_result;
    }
    std::int32_t Tell(std::uint32_t file) override {
        Check(file == 0x5000u && !tell_results.empty(), "tell file handle");
        calls.emplace_back("tell");
        const auto result = tell_results.front();
        tell_results.erase(tell_results.begin());
        return result;
    }
    std::uint32_t Read(void* buffer, std::uint32_t size,
                        std::uint32_t count,
                        std::uint32_t file) override {
        Check(buffer == expected_buffer && size == expected_size &&
                  count == 1u && file == 0x5000u,
              "read one exact element");
        calls.emplace_back("read");
        return read_result;
    }
    std::uint32_t Write(const void* buffer, std::uint32_t size,
                         std::uint32_t count,
                         std::uint32_t file) override {
        Check(buffer == expected_buffer && size == expected_size &&
                  count == 1u && file == 0x5000u,
              "write one exact element");
        calls.emplace_back("write");
        return write_result;
    }
};

void Cases() {
    Runtime runtime;
    pal4::ArchiveFileStream stream(runtime);
    pal4::ArchiveFileStreamState state{0xdeadbeefu, 0u};
    Check(&stream.Construct(state, 0x5000u) == &state &&
              state.vtable == pal4::ArchiveFileStream::kVtable &&
              state.file == 0x5000u &&
              runtime.calls == std::vector<std::string>{"seek:0:0"},
          "construct stores two words and seeks to start");

    runtime.calls.clear();
    runtime.tell_results = {17, 93};
    Check(stream.Size(state) == 93 &&
              runtime.calls == std::vector<std::string>{
                  "tell", "seek:0:2", "tell", "seek:17:0"},
          "size saves and restores current position");
    runtime.calls.clear();
    runtime.tell_results = {-1};
    Check(stream.Position(state) == -1 &&
              runtime.calls == std::vector<std::string>{"tell"},
          "position forwards signed CRT result");
    runtime.calls.clear();
    Check(stream.SetPosition(state, -20) == -7 &&
              runtime.calls == std::vector<std::string>{"seek:-20:0"},
          "set position forwards seek result and signed offset");

    std::uint32_t buffer = 0xabcdefu;
    runtime.expected_buffer = &buffer;
    runtime.expected_size = sizeof(buffer);
    runtime.calls.clear();
    Check(!stream.ReadElement(state, &buffer, sizeof(buffer)) &&
              runtime.calls == std::vector<std::string>{"read"},
          "zero elements read means failure");
    runtime.read_result = 2u;
    Check(stream.ReadElement(state, &buffer, sizeof(buffer)),
          "nonzero elements read means success");
    runtime.calls.clear();
    Check(!stream.WriteElement(state, &buffer, sizeof(buffer)) &&
              runtime.calls == std::vector<std::string>{"write"},
          "zero elements written means failure");
    runtime.write_result = 2u;
    Check(stream.WriteElement(state, &buffer, sizeof(buffer)),
          "nonzero elements written means success");

    pal4::ArchiveFileStreamRuntimeStub stub;
    Check(stub.Seek(0, 0, 0) == 0 && stub.Tell(0) == 0 &&
              stub.Read(nullptr, 0, 1, 0) == 0u &&
              stub.Write(nullptr, 0, 1, 0) == 0u,
          "file stream runtime stub links");
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
