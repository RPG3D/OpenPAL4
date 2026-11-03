#include "pal4/package_archive_file_entry.h"

#include <array>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Runtime final : public pal4::IPackageArchiveEntryRuntime {
public:
    std::uint32_t FindIndex(const char* path) override {
        events.push_back("find:" + std::string(path));
        return index;
    }
    pal4::PackageArchiveFileSlot* Slot(std::uint32_t address) override {
        const auto found = slots.find(address);
        return found == slots.end() ? nullptr : &found->second;
    }
    std::uint32_t MapView(std::uint32_t mapping, std::uint32_t access,
                          std::uint32_t aligned_offset,
                          std::uint32_t bytes) override {
        Check(mapping == 0x5000u && access == 4u &&
              aligned_offset == 0x2000u && bytes == 0x465u,
              "mapping uses aligned file offset, packed and extra lengths");
        events.push_back("map");
        return view;
    }
    bool UnmapView(std::uint32_t base) override {
        events.push_back("unmap:" + std::to_string(base));
        return unmap_success;
    }
    void EnsureDecoderAllocator() override {
        events.push_back("ensure-decoder");
    }
    std::uint32_t AllocateDecoded(std::uint32_t bytes) override {
        events.push_back("allocate:" + std::to_string(bytes));
        return decoded;
    }
    bool Decode(std::uint16_t type, std::uint32_t packed_bytes,
                std::uint32_t decoded_bytes, std::uint32_t source,
                std::uint32_t destination) override {
        Check(type == 2u && packed_bytes == 0x100u &&
              decoded_bytes == 0x200u && source == 0x9345u &&
              destination == decoded,
              "decode receives record type, sizes, mapped cursor and output");
        events.push_back("decode");
        return decode_success;
    }
    void FreeDecoded(std::uint32_t address, std::uint32_t bytes) override {
        events.push_back("free:" + std::to_string(address) + ":" +
                         std::to_string(bytes));
    }
    void SetFilePointer(std::uint32_t file, std::uint32_t offset) override {
        events.push_back("seek:" + std::to_string(file) + ":" +
                         std::to_string(offset));
    }
    void ReportError(pal4::PackageArchiveEntryError error) override {
        events.push_back("error:" + std::to_string(static_cast<int>(error)));
    }
    void ReportMapFailure(const char* archive_path, const char* member_path,
                          std::uint32_t aligned_offset,
                          std::uint32_t bytes) override {
        Check(std::string(archive_path) == "assets.cpk" &&
              std::string(member_path) == "music.mp3" &&
              aligned_offset == 0x2000u && bytes == 0x465u,
              "mapping diagnostic receives archive and member paths");
        events.push_back("map-report");
    }

    std::map<std::uint32_t, pal4::PackageArchiveFileSlot> slots;
    std::uint32_t index = 0u;
    std::uint32_t view = 0x9000u;
    std::uint32_t decoded = 0xa000u;
    bool decode_success = true;
    bool unmap_success = true;
    std::vector<std::string> events;
};

void Fixture(pal4::PackageArchiveState& archive, Runtime& runtime,
             std::uint16_t type = 1u, std::uint32_t mode = 2u) {
    archive.WriteWord(0u, 0x1000u);
    archive.WriteWord(4u, mode);
    archive.WriteByte(pal4::PackageArchiveState::kOpenFlag, 1u);
    archive.WriteWord(1048748u, 0x6000u);
    archive.WriteWord(1048752u, 0x5000u);
    constexpr char path[] = "assets.cpk";
    for (std::uint32_t i = 0u; i < sizeof(path); ++i)
        archive.WriteByte(1048756u + i, path[i]);
    constexpr std::uint32_t record = 136u;
    archive.WriteWord(record, 0xabu);
    archive.WriteByte(record + 6u, static_cast<std::uint8_t>(type));
    archive.WriteByte(record + 7u, static_cast<std::uint8_t>(type >> 8u));
    archive.WriteWord(record + 8u, 25u);
    archive.WriteWord(record + 12u, 0x2345u);
    archive.WriteWord(record + 16u, 0x100u);
    archive.WriteWord(record + 20u, 0x200u);
    archive.WriteWord(record + 24u, 0x20u);
    for (std::uint32_t i = 0u; i < 8u; ++i) {
        const auto address = 0x1000u + i * 0x100u;
        archive.WriteWord(pal4::PackageArchiveState::kSlotArray + i * 4u,
                          address);
        runtime.slots[address] = {};
    }
}

void CheckPlainAndSlotSelection() {
    pal4::PackageArchiveState archive;
    Runtime runtime;
    Fixture(archive, runtime);
    runtime.slots[0x1000u].active = 1u;
    Check(pal4::PackageArchiveFileEntry::Open(
              archive, "music.mp3", runtime) == 0x1100u &&
          archive.ReadWord(1049016u) == 1u &&
          runtime.events == std::vector<std::string>({
              "find:music.mp3", "map"}),
          "first inactive slot is selected and open count increases");
    const auto& slot = runtime.slots[0x1100u];
    Check(slot.active == 1u && slot.file_id == 0xabu &&
          slot.logical_bytes == 25u && slot.index == 0u &&
          slot.mapped_base == 0x9000u && slot.cursor == 0x9345u &&
          slot.mapping_delta == 0x345u && slot.needs_decode == 0u &&
          slot.data == 0x9345u && slot.decoded_bytes == 0x200u &&
          slot.record_offset == 136u,
          "plain record fields and cursor match IDA offsets");
}

void CheckEarlyAndMappingFailures() {
    pal4::PackageArchiveState archive;
    Runtime runtime;
    Fixture(archive, runtime);
    archive.WriteByte(pal4::PackageArchiveState::kOpenFlag, 0u);
    Check(pal4::PackageArchiveFileEntry::Open(
              archive, "music.mp3", runtime) == 0u &&
          runtime.events.empty(), "closed archive returns without lookup");
    archive.WriteByte(pal4::PackageArchiveState::kOpenFlag, 1u);
    archive.WriteWord(1049016u, 7u);
    Check(pal4::PackageArchiveFileEntry::Open(
              archive, "music.mp3", runtime) == 0u &&
          runtime.events == std::vector<std::string>({"error:0"}),
          "seven-open limit precedes lookup despite eight slots");
    archive.WriteWord(1049016u, 0u);
    runtime.events.clear();
    runtime.index = 0xffffffffu;
    Check(pal4::PackageArchiveFileEntry::Open(
              archive, "music.mp3", runtime) == 0u &&
          runtime.events == std::vector<std::string>({"find:music.mp3"}),
          "missing member returns without reserving a slot");
    runtime.index = 0u;
    runtime.view = 0u;
    runtime.events.clear();
    Check(pal4::PackageArchiveFileEntry::Open(
              archive, "music.mp3", runtime) == 0u &&
          runtime.slots[0x1000u].active == 0u &&
          archive.ReadWord(1049016u) == 0u &&
          runtime.events == std::vector<std::string>({
              "find:music.mp3", "map", "error:1", "map-report"}),
          "mapping failure reports and releases only active flag");
}

void CheckDecodeAndNonmapping() {
    {
        pal4::PackageArchiveState archive;
        Runtime runtime;
        Fixture(archive, runtime, 2u);
        Check(pal4::PackageArchiveFileEntry::Open(
                  archive, "music.mp3", runtime) == 0x1000u &&
              archive.ReadWord(1049016u) == 1u &&
              runtime.slots[0x1000u].active == 1u &&
              runtime.slots[0x1000u].needs_decode == 1u &&
              runtime.slots[0x1000u].data == 0xa000u &&
              runtime.events == std::vector<std::string>({
                  "find:music.mp3", "map", "ensure-decoder",
                  "allocate:512", "decode"}),
              "compressed success keeps mapping and decoded data");
    }
    for (int failure = 0; failure < 3; ++failure) {
        pal4::PackageArchiveState archive;
        Runtime runtime;
        Fixture(archive, runtime, 2u, failure == 0 ? 1u : 2u);
        if (failure == 1) runtime.decoded = 0u;
        if (failure == 2) runtime.decode_success = false;
        const auto result = pal4::PackageArchiveFileEntry::Open(
            archive, "music.mp3", runtime);
        Check(result == 0u && runtime.slots[0x1000u].active == 0u &&
              archive.ReadWord(1049016u) == 0u,
              "compressed failure leaves slot inactive and count unchanged");
        if (failure == 0)
            Check(runtime.events == std::vector<std::string>({
                      "find:music.mp3"}),
                  "nonmapping compressed entry fails without mapping");
        if (failure == 1)
            Check(runtime.events == std::vector<std::string>({
                      "find:music.mp3", "map", "ensure-decoder",
                      "allocate:512", "error:2", "unmap:36864"}),
                  "decoded-buffer allocation failure unmaps view");
        if (failure == 2)
            Check(runtime.events == std::vector<std::string>({
                      "find:music.mp3", "map", "ensure-decoder",
                      "allocate:512", "decode", "error:3",
                      "unmap:36864", "ensure-decoder", "free:40960:512"}),
                  "decode failure unmaps then frees decoded buffer");
    }
    pal4::PackageArchiveState archive;
    Runtime runtime;
    Fixture(archive, runtime, 1u, 1u);
    Check(pal4::PackageArchiveFileEntry::Open(
              archive, "music.mp3", runtime) == 0x1000u &&
          runtime.slots[0x1000u].mapped_base == 0u &&
          runtime.slots[0x1000u].data == 0u &&
          runtime.events == std::vector<std::string>({
              "find:music.mp3", "seek:24576:9029"}),
          "nonmapping plain entry seeks to record offset");
}

void CheckClosePaths() {
    {
        pal4::PackageArchiveState archive;
        Runtime runtime;
        Fixture(archive, runtime, 2u);
        const auto slot = pal4::PackageArchiveFileEntry::Open(
            archive, "music.mp3", runtime);
        runtime.events.clear();
        Check(pal4::PackageArchiveFileEntry::Close(archive, slot, runtime) &&
              archive.ReadWord(1049016u) == 0u &&
              runtime.slots[slot].active == 0u &&
              runtime.events == std::vector<std::string>({
                  "unmap:36864", "ensure-decoder", "free:40960:512"}),
              "mapped compressed close unmaps, frees and decrements");
    }
    {
        pal4::PackageArchiveState archive;
        Runtime runtime;
        Fixture(archive, runtime);
        runtime.slots[0x1000u].active = 1u;
        archive.WriteWord(1049016u, 1u);
        Check(!pal4::PackageArchiveFileEntry::Close(
                  archive, 0x1000u, runtime) &&
              runtime.slots[0x1000u].active == 1u &&
              archive.ReadWord(1049016u) == 1u &&
              runtime.events == std::vector<std::string>({"error:4"}),
              "missing mapped address preserves slot and count");
        runtime.slots[0x1000u].mapped_base = 0x9000u;
        runtime.unmap_success = false;
        runtime.events.clear();
        Check(!pal4::PackageArchiveFileEntry::Close(
                  archive, 0x1000u, runtime) &&
              runtime.slots[0x1000u].active == 0u &&
              archive.ReadWord(1049016u) == 1u &&
              runtime.events == std::vector<std::string>({
                  "unmap:36864", "error:5"}),
              "failed unmap clears active without decrementing count");
    }
    {
        pal4::PackageArchiveState archive;
        Runtime runtime;
        Fixture(archive, runtime, 1u, 1u);
        const auto slot = pal4::PackageArchiveFileEntry::Open(
            archive, "music.mp3", runtime);
        runtime.events.clear();
        Check(pal4::PackageArchiveFileEntry::Close(archive, slot, runtime) &&
              archive.ReadWord(1049016u) == 0u &&
              runtime.slots[slot].active == 0u && runtime.events.empty(),
              "nonmapping close only clears active and count");
    }
}

void CheckSeekOrigins() {
    pal4::PackageArchiveState archive;
    Runtime runtime;
    Fixture(archive, runtime);
    pal4::PackageArchiveFileSlot slot;
    slot.record_offset = 136u;
    slot.decoded_bytes = 0x200u;
    slot.reserved40 = 10u;
    Check(pal4::PackageArchiveFileEntry::Seek(
              archive, slot, 20, 0u, runtime) == 20u &&
          pal4::PackageArchiveFileEntry::Seek(
              archive, slot, -5, 1u, runtime) == 15u &&
          pal4::PackageArchiveFileEntry::Seek(
              archive, slot, 5, 2u, runtime) == 0x1fbu &&
          runtime.events.empty(),
          "mapped seek handles absolute, signed relative and end-minus-offset");
    Check(pal4::PackageArchiveFileEntry::Seek(
              archive, slot, 99, 3u, runtime) == 0xffffffffu &&
          slot.reserved40 == 0x1fbu,
          "invalid origin leaves cursor unchanged");
    archive.WriteWord(4u, 1u);
    Check(pal4::PackageArchiveFileEntry::Seek(
              archive, slot, 7, 0u, runtime) == 7u &&
          runtime.events == std::vector<std::string>({
              "seek:24576:9036"}),
          "nonmapping seek adds record file offset");
}

}  // namespace

int main() {
    CheckPlainAndSlotSelection();
    CheckEarlyAndMappingFailures();
    CheckDecodeAndNonmapping();
    CheckClosePaths();
    CheckSeekOrigins();
    std::cout << "5 package archive file entry test cases passed\n";
}
