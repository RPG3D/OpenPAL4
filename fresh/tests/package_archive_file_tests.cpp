#include "pal4/package_archive.h"

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

class Runtime final : public pal4::IPackageArchiveFileRuntime {
public:
    std::uint32_t AllocationGranularity() override { return 0x10000u; }
    std::uint32_t AllocateSlot(std::uint32_t) override {
        return 0x1000u + allocation_index++ * 0x100u;
    }
    void WriteSlotByte(std::uint32_t address, std::uint32_t offset,
                       std::uint8_t value) override {
        events.push_back("slot-byte:" + std::to_string(address) + ":" +
                         std::to_string(offset) + ":" +
                         std::to_string(value));
    }
    void WriteSlotWord(std::uint32_t, std::uint32_t,
                       std::uint32_t) override {}
    void ReportAllocationFailure() override {}
    std::uint32_t FreeSlot(std::uint32_t) override { return 0u; }
    std::uint32_t OpenFile(const char* path, std::uint32_t access,
                           std::uint32_t share, std::uint32_t disposition,
                           std::uint32_t attributes) override {
        Check(access == 0x80000000u && share == 1u && disposition == 3u &&
              attributes == 0x10000080u, "original CreateFileA options");
        events.push_back("open:" + std::string(path));
        return file;
    }
    bool ReadFile(std::uint32_t handle, pal4::PackageArchiveState& state,
                  std::uint32_t offset, std::uint32_t bytes) override {
        Check(handle == file, "reads use opened package handle");
        events.push_back("read:" + std::to_string(offset) + ":" +
                         std::to_string(bytes));
        if (offset == 8u) {
            state.WriteWord(8u, magic);
            state.WriteWord(24u, entry_count);
            return header_read;
        }
        return table_read;
    }
    void DecryptTable(pal4::PackageArchiveState&,
                      std::uint32_t offset, std::uint32_t bytes) override {
        events.push_back("decrypt:" + std::to_string(offset) + ":" +
                         std::to_string(bytes));
    }
    std::uint32_t CreateMapping(std::uint32_t handle,
                                 std::uint32_t protection) override {
        Check(handle == file && protection == 2u,
              "mapping uses read-only protection and package file");
        events.push_back("map");
        return mapping;
    }
    std::uint32_t LastError() override {
        events.push_back("last-error");
        return last_error;
    }
    void CloseFile(std::uint32_t handle) override {
        events.push_back("close-file:" + std::to_string(handle));
    }
    void CloseMapping(std::uint32_t handle) override {
        events.push_back("close-map:" + std::to_string(handle));
    }
    void ReportError(pal4::PackageArchiveError error) override {
        events.push_back("error:" + std::to_string(static_cast<int>(error)));
    }
    void ReportPathError(pal4::PackageArchiveError error,
                         const char* path) override {
        events.push_back("path-error:" +
                         std::to_string(static_cast<int>(error)) + ":" + path);
    }
    std::uint32_t file = 0x2000u;
    std::uint32_t mapping = 0x3000u;
    std::uint32_t magic = 0x1a545352u;
    std::uint32_t entry_count = 2u;
    std::uint32_t last_error = 0u;
    std::uint32_t allocation_index = 0u;
    bool header_read = true;
    bool table_read = true;
    std::vector<std::string> events;
};

void Initialize(pal4::PackageArchiveState& state,
                pal4::PackageNameCrc& crc, Runtime& runtime) {
    (void)pal4::PackageArchive::Initialize(state, crc, runtime);
    runtime.events.clear();
}

void CheckSuccessAndClose() {
    pal4::PackageArchiveState state;
    pal4::PackageNameCrc crc;
    Runtime runtime;
    Initialize(state, crc, runtime);
    runtime.header_read = false;
    Check(pal4::PackageArchive::Open(state, "music.smp", runtime) &&
          state.ReadByte(pal4::PackageArchiveState::kOpenFlag) == 1u &&
          state.ReadWord(1048748u) == 0x2000u &&
          state.ReadWord(1048752u) == 0x3000u &&
          state.ReadByte(1048756u) == 'm' &&
          runtime.events == std::vector<std::string>({
              "open:music.smp", "read:8:128", "read:136:64",
              "decrypt:136:1048576", "map", "last-error"}),
          "header read status is ignored; table decrypt precedes mapping");
    runtime.events.clear();
    Check(pal4::PackageArchive::Close(state, runtime) &&
          state.ReadByte(pal4::PackageArchiveState::kOpenFlag) == 0u &&
          state.ReadWord(1048748u) == 0xffffffffu &&
          state.ReadWord(1048752u) == 0xffffffffu &&
          state.ReadByte(1048756u) == 0u &&
          runtime.events[0] == "close-map:12288" &&
          runtime.events[1] == "close-file:8192" &&
          runtime.events.size() == 10u,
          "close releases mapping then file and resets archive/slots");
    runtime.events.clear();
    Check(!pal4::PackageArchive::Close(state, runtime) &&
          runtime.events.empty(), "closed archive has no close side effects");
    state.WriteWord(4u, 1u);
    Check(pal4::PackageArchive::Open(state, "script.cpk", runtime) &&
          state.ReadWord(1048752u) == 0xffffffffu &&
          runtime.events == std::vector<std::string>({
              "open:script.cpk", "read:8:128", "read:136:64",
              "decrypt:136:1048576"}),
          "non-mapping mode skips CreateFileMapping and LastError");
}

void CheckReadAndOpenFailures() {
    for (int failure = 0; failure < 3; ++failure) {
        pal4::PackageArchiveState state;
        pal4::PackageNameCrc crc;
        Runtime runtime;
        Initialize(state, crc, runtime);
        if (failure == 0) runtime.file = 0xffffffffu;
        if (failure == 1) runtime.magic = 0u;
        if (failure == 2) runtime.table_read = false;
        Check(!pal4::PackageArchive::Open(state, "bad.cpk", runtime) &&
              state.ReadByte(pal4::PackageArchiveState::kOpenFlag) == 0u &&
              state.ReadWord(1048748u) == runtime.file &&
              state.ReadByte(1048756u) == 'b' &&
              runtime.events.back() ==
                  "close-file:" + std::to_string(runtime.file),
              "early errors leave saved path and stale file handle");
        const std::string read = failure == 0 ? "read:8:128" :
                                 failure == 1 ? "read:136:64" :
                                                "decrypt:136:1048576";
        for (const auto& event : runtime.events)
            Check(event != read, "failed stage does not advance");
    }
}

void CheckMappingFailuresAndReopen() {
    for (int failure = 0; failure < 2; ++failure) {
        pal4::PackageArchiveState state;
        pal4::PackageNameCrc crc;
        Runtime runtime;
        Initialize(state, crc, runtime);
        if (failure == 0) runtime.mapping = 0u;
        else runtime.last_error = 183u;
        Check(!pal4::PackageArchive::Open(state, "map.cpk", runtime) &&
              state.ReadByte(pal4::PackageArchiveState::kOpenFlag) == 0u &&
              state.ReadWord(1048752u) == 0u &&
              runtime.events.back() == "close-file:8192",
              "mapping failure leaves archive closed");
        if (failure == 1)
            Check(runtime.events[runtime.events.size() - 3u] ==
                      "close-map:12288",
                  "already-existing mapping is closed before file");
    }
    pal4::PackageArchiveState state;
    pal4::PackageNameCrc crc;
    Runtime runtime;
    Initialize(state, crc, runtime);
    Check(pal4::PackageArchive::Open(state, "first.cpk", runtime),
          "initial open succeeds");
    runtime.events.clear();
    Check(pal4::PackageArchive::Open(state, "second.cpk", runtime) &&
          state.ReadByte(1048756u) == 0u &&
          runtime.events[0] == "error:0" &&
          runtime.events[1] == "close-map:12288" &&
          runtime.events[2] == "close-file:8192",
          "reopen copies path before close reset clears it");
}

}  // namespace

int main() {
    CheckSuccessAndClose();
    CheckReadAndOpenFailures();
    CheckMappingFailuresAndReopen();
    std::cout << "3 package archive file test cases passed\n";
}
