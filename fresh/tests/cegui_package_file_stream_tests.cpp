#include "pal4/cegui_package_file_stream.h"

#include <cstdlib>
#include <cstring>
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

class Runtime final : public pal4::CeguiPackageFileRuntimeStub {
public:
    pal4::CeguiPackageFileStreamState* state = nullptr;
    std::vector<std::string> calls;
    std::vector<std::uint32_t> freed;
    std::uint32_t open_result = 0x6000u;
    bool read_result = true;
    std::uint32_t read_bytes = 0u;
    std::uint32_t read_buffer = 0u;
    std::uint32_t read_handle = 0u;
    std::int32_t seek_result = 27;
    std::uint32_t seek_origin = 0u;
    std::int32_t seek_offset = 0;
    std::uint32_t seek_handle = 0u;

    std::uint32_t Free(std::uint32_t address) override {
        Check(state->ReadDword(0u) == 0x12345678u,
              "object free follows base vtable transition");
        calls.emplace_back("free");
        freed.push_back(address);
        return 9u;
    }
    std::uint32_t BaseVtable() override {
        calls.emplace_back("base_vtable");
        return 0x12345678u;
    }
    void LoadUiScriptPath(std::uint32_t resource_address,
                          char* output, std::uint32_t capacity) override {
        Check(resource_address == 0x4000u && capacity == 256u,
              "path loader receives resource and fixed capacity");
        calls.emplace_back("load_path");
        std::memcpy(output, "ui/test.xml", sizeof("ui/test.xml"));
    }
    std::uint32_t InitializeManager() override {
        calls.emplace_back("manager");
        return 0x10u;
    }
    std::uint32_t OpenFile(std::uint32_t manager,
                           const char* path) override {
        Check(manager == 0x5009a0u &&
                  std::string(path) == "ui/test.xml",
              "package open receives adjusted manager and resolved path");
        calls.emplace_back("open");
        return open_result;
    }
    std::int32_t SeekFile(std::uint32_t manager,
                          std::uint32_t handle, std::int32_t offset,
                          std::uint32_t origin) override {
        Check(manager == 0x5009a0u,
              "seek receives adjusted manager address");
        calls.emplace_back("seek");
        seek_handle = handle;
        seek_offset = offset;
        seek_origin = origin;
        return seek_result;
    }
    void CloseFile(std::uint32_t manager,
                   std::uint32_t handle) override {
        Check(manager == 0x5009a0u && handle != 0u &&
                  state->ReadDword(4u) == handle,
              "close observes live handle before state reset");
        calls.emplace_back("close");
    }
    bool ReadFile(std::uint32_t manager, std::uint32_t buffer,
                  std::uint32_t bytes, std::uint32_t handle) override {
        Check(manager == 0x5009a0u,
              "read receives adjusted manager address");
        calls.emplace_back("read");
        read_buffer = buffer;
        read_bytes = bytes;
        read_handle = handle;
        return read_result;
    }
    std::int32_t ReadPosition(std::uint32_t handle) override {
        Check(handle == 0x6000u,
              "position reader receives current handle");
        calls.emplace_back("position");
        return 42;
    }
};

void ConstructorInitializesExactEightBytes() {
    pal4::CeguiPackageFileStreamState state;
    state.WriteDword(0u, 0xffffffffu);
    state.WriteDword(4u, 0xffffffffu);
    Check(&pal4::CeguiPackageFileStream::Construct(state) == &state &&
              state.ReadDword(0u) == state.kVtable &&
              state.ReadDword(4u) == 0u,
          "constructor installs derived vtable and clears handle");
}

void OpenClosesOldHandleAndSeeksNewFile() {
    pal4::CeguiPackageFileStreamState state;
    Runtime runtime;
    runtime.state = &state;
    state.WriteDword(4u, 0x5000u);
    Check(pal4::CeguiPackageFileStream::Open(
              state, 0x4000u, 0xdeadbeefu, runtime) &&
              state.ReadDword(4u) == 0x6000u &&
              runtime.calls == std::vector<std::string>{
                  "manager", "close", "load_path", "manager",
                  "open", "manager", "seek"} &&
              runtime.seek_handle == 0x6000u &&
              runtime.seek_offset == 0 && runtime.seek_origin == 0u,
          "open closes prior handle then resolves, opens and rewinds");
}

void FailedOpenStillCallsInitialSeek() {
    pal4::CeguiPackageFileStreamState state;
    Runtime runtime;
    runtime.state = &state;
    runtime.open_result = 0u;
    Check(!pal4::CeguiPackageFileStream::Open(
              state, 0x4000u, 0u, runtime) &&
              state.ReadDword(4u) == 0u &&
              runtime.calls == std::vector<std::string>{
                  "load_path", "manager", "open", "manager", "seek"} &&
              runtime.seek_handle == 0u,
          "failed open retains observed seek call with null handle");
}

void CloseClearsOnlyNonzeroHandle() {
    pal4::CeguiPackageFileStreamState state;
    Runtime runtime;
    runtime.state = &state;
    Check(pal4::CeguiPackageFileStream::Close(state, runtime) == 0 &&
              runtime.calls.empty(),
          "null close returns zero without package calls");
    state.WriteDword(4u, 0x6000u);
    Check(pal4::CeguiPackageFileStream::Close(state, runtime) == 0 &&
              state.ReadDword(4u) == 0u &&
              runtime.calls == std::vector<std::string>{
                  "manager", "close"},
          "live close initializes manager and clears handle");
}

void ReadUsesProductAndOnlyReturnsItOnSuccess() {
    pal4::CeguiPackageFileStreamState state;
    Runtime runtime;
    runtime.state = &state;
    Check(pal4::CeguiPackageFileStream::Read(
              state, 0x7000u, 5u, 7u, runtime) == 0u &&
              runtime.calls.empty(),
          "closed stream skips package read");
    state.WriteDword(4u, 0x6000u);
    Check(pal4::CeguiPackageFileStream::Read(
              state, 0x7000u, 5u, 7u, runtime) == 35u &&
              runtime.read_buffer == 0x7000u &&
              runtime.read_bytes == 35u &&
              runtime.read_handle == 0x6000u &&
              runtime.calls == std::vector<std::string>{
                  "manager", "read"},
          "successful read returns size times count");
    runtime.read_result = false;
    runtime.calls.clear();
    Check(pal4::CeguiPackageFileStream::Read(
              state, 0x7000u, 5u, 7u, runtime) == 0u &&
              runtime.calls == std::vector<std::string>{
                  "manager", "read"},
          "failed read discards requested byte count");
}

void SeekAcceptsOnlyThreeOrigins() {
    pal4::CeguiPackageFileStreamState state;
    Runtime runtime;
    runtime.state = &state;
    Check(pal4::CeguiPackageFileStream::Seek(
              state, -5, 0u, runtime) == -1 &&
              runtime.calls.empty(),
          "closed seek rejects without manager initialization");
    state.WriteDword(4u, 0x6000u);
    for (std::uint32_t origin = 0u; origin < 3u; ++origin) {
        runtime.calls.clear();
        Check(pal4::CeguiPackageFileStream::Seek(
                  state, -5, origin, runtime) == 27 &&
                  runtime.seek_handle == 0x6000u &&
                  runtime.seek_offset == -5 &&
                  runtime.seek_origin == origin &&
                  runtime.calls == std::vector<std::string>{
                      "manager", "seek"},
              "seek forwards each supported origin and result");
    }
    runtime.calls.clear();
    Check(pal4::CeguiPackageFileStream::Seek(
              state, -5, 3u, runtime) == -1 &&
              runtime.calls.empty(),
          "unsupported seek origin returns -1 without package call");
}

void PositionAndDestructionFollowHandleState() {
    pal4::CeguiPackageFileStreamState state;
    Runtime runtime;
    runtime.state = &state;
    Check(pal4::CeguiPackageFileStream::Position(state, runtime) == -1 &&
              runtime.calls.empty(),
          "closed position returns -1");
    state.WriteDword(4u, 0x6000u);
    Check(pal4::CeguiPackageFileStream::Position(state, runtime) == 42 &&
              runtime.calls == std::vector<std::string>{
                  "manager", "position"},
          "open position initializes manager then reads handle +40");
    runtime.calls.clear();
    Check(&pal4::CeguiPackageFileStream::Destroy(
              state, 2u, 0x8000u, runtime) == &state &&
              state.ReadDword(0u) == 0x12345678u &&
              state.ReadDword(4u) == 0u && runtime.freed.empty() &&
              runtime.calls == std::vector<std::string>{
                  "manager", "close", "base_vtable"},
          "destructor closes stream and preserves object for flag two");
    runtime.calls.clear();
    Check(&pal4::CeguiPackageFileStream::Destroy(
              state, 1u, 0x8000u, runtime) == &state &&
              runtime.freed == std::vector<std::uint32_t>{0x8000u} &&
              runtime.calls == std::vector<std::string>{
                  "base_vtable", "free"},
          "destructor flag one frees object after base vtable");
}

}  // namespace

int main() {
    ConstructorInitializesExactEightBytes();
    OpenClosesOldHandleAndSeeksNewFile();
    FailedOpenStillCallsInitialSeek();
    CloseClearsOnlyNonzeroHandle();
    ReadUsesProductAndOnlyReturnsItOnSuccess();
    SeekAcceptsOnlyThreeOrigins();
    PositionAndDestructionFollowHandleState();
    std::cout << "7 CEGUI package file-stream test cases passed\n";
}
