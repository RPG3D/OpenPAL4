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

class Runtime final : public pal4::IPackageArchiveRuntime {
public:
    std::uint32_t AllocationGranularity() override {
        events.push_back("granularity");
        return 0x10000u;
    }
    std::uint32_t AllocateSlot(std::uint32_t bytes) override {
        events.push_back("alloc:" + std::to_string(bytes));
        const auto current = allocation_index++;
        return current == failed_index ? 0u : 0x1000u + current * 0x100u;
    }
    void WriteSlotByte(std::uint32_t address, std::uint32_t offset,
                       std::uint8_t value) override {
        events.push_back("byte:" + std::to_string(address) + ":" +
                         std::to_string(offset) + ":" + std::to_string(value));
    }
    void WriteSlotWord(std::uint32_t address, std::uint32_t offset,
                       std::uint32_t value) override {
        events.push_back("word:" + std::to_string(address) + ":" +
                         std::to_string(offset) + ":" + std::to_string(value));
    }
    void ReportAllocationFailure() override {
        events.push_back("error");
    }
    std::uint32_t FreeSlot(std::uint32_t address) override {
        events.push_back("free:" + std::to_string(address));
        return address + 1u;
    }
    std::uint32_t allocation_index = 0u;
    std::uint32_t failed_index = 0xffffffffu;
    std::vector<std::string> events;
};

void CheckInitializeAndFailures() {
    pal4::PackageArchiveState state;
    state.WriteByte(0u, 0xffu);
    state.WriteByte(pal4::PackageArchiveState::kBytes - 1u, 0xffu);
    pal4::PackageNameCrc crc;
    Runtime runtime;
    runtime.failed_index = 2u;
    Check(&pal4::PackageArchive::Initialize(state, crc, runtime) == &state &&
          state.ReadWord(0u) == 0x10000u && state.ReadWord(4u) == 2u &&
          state.ReadByte(pal4::PackageArchiveState::kBytes - 1u) == 0u &&
          crc.Initialized() && runtime.allocation_index == 8u,
          "constructor clears entire archive, sets granularity/mode and CRC");
    Check(state.ReadWord(pal4::PackageArchiveState::kSlotArray) == 0x1000u &&
          state.ReadWord(pal4::PackageArchiveState::kSlotArray + 8u) == 0u &&
          state.ReadWord(pal4::PackageArchiveState::kSlotArray + 28u) ==
              0x1700u,
          "all eight slot positions are written despite one allocation failure");
    std::uint32_t allocated = 0u;
    std::uint32_t byte_writes = 0u;
    std::uint32_t word_writes = 0u;
    std::uint32_t errors = 0u;
    for (const auto& event : runtime.events) {
        allocated += event == "alloc:48";
        byte_writes += event.rfind("byte:", 0u) == 0u;
        word_writes += event.rfind("word:", 0u) == 0u;
        errors += event == "error";
    }
    Check(runtime.events.front() == "granularity" && allocated == 8u &&
          byte_writes == 14u && word_writes == 7u && errors == 1u,
          "constructor writes slot byte +0 twice and word +32 once");
}

void CheckReleaseAndReset() {
    pal4::PackageArchiveState state;
    pal4::PackageNameCrc crc;
    Runtime runtime;
    (void)pal4::PackageArchive::Initialize(state, crc, runtime);
    state.WriteByte(8u, 0xaau);
    state.WriteByte(136u, 0xbbu);
    state.WriteByte(136u + 0x100000u - 1u, 0xccu);
    state.WriteByte(pal4::PackageArchiveState::kOpenFlag, 1u);
    state.WriteWord(1048748u, 3u);
    state.WriteWord(1048752u, 4u);
    state.WriteByte(1048756u, 0xddu);
    state.WriteWord(1049016u, 5u);
    runtime.events.clear();
    Check(pal4::PackageArchive::ResetAfterClose(state, runtime) ==
              pal4::PackageArchiveState::kOpenFlag &&
          state.ReadByte(8u) == 0u && state.ReadByte(136u) == 0u &&
          state.ReadByte(136u + 0x100000u - 1u) == 0u &&
          state.ReadByte(pal4::PackageArchiveState::kOpenFlag) == 0u &&
          state.ReadWord(1048748u) == 0xffffffffu &&
          state.ReadWord(1048752u) == 0xffffffffu &&
          state.ReadByte(1048756u) == 0u &&
          state.ReadWord(1049016u) == 0u && runtime.events.size() == 8u,
          "close reset clears two ranges and resets all eight slot headers");
    runtime.events.clear();
    Check(pal4::PackageArchive::ReleaseSlots(state, runtime) == 0x1701u &&
          state.ReadWord(pal4::PackageArchiveState::kSlotArray) == 0u &&
          state.ReadWord(pal4::PackageArchiveState::kSlotArray + 28u) == 0u &&
          runtime.events.size() == 8u,
          "release frees every allocated slot and returns final free result");
    Check(pal4::PackageArchive::ReleaseSlots(state, runtime) == 0u &&
          runtime.events.size() == 8u,
          "repeat release skips empty slots");
}

void CheckModeGuard() {
    pal4::PackageArchiveState state;
    Check(pal4::PackageArchive::SetModeIfClosed(state, 0x12345678u) == 0x78u &&
          state.ReadWord(4u) == 0x12345678u,
          "closed archive stores full mode and returns its low byte");
    state.WriteByte(pal4::PackageArchiveState::kOpenFlag, 3u);
    Check(pal4::PackageArchive::SetModeIfClosed(state, 9u) == 3u &&
          state.ReadWord(4u) == 0x12345678u,
          "open archive returns flag without changing mode");
}

}  // namespace

int main() {
    CheckInitializeAndFailures();
    CheckReleaseAndReset();
    CheckModeGuard();
    std::cout << "3 package archive test cases passed\n";
}
