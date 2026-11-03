#include "pal4/game_data_monster_record_copy.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

constexpr std::uint32_t kStride = 596;
constexpr std::array<std::uint32_t, 11> kTextOffsets = {
    408, 424, 440, 456, 472, 488, 504, 520, 536, 552, 568};

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IGameDataMonsterRecordCopyRuntime {
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> text_assignments;
    std::vector<std::string> calls;

    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes[address];
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        bytes[address] = value;
        calls.push_back("byte " + std::to_string(address));
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        std::uint32_t result = 0;
        for (unsigned i = 0; i < 4; ++i)
            result |= static_cast<std::uint32_t>(ReadByte(address + i)) << (8 * i);
        return result;
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        for (unsigned i = 0; i < 4; ++i)
            bytes[address + i] = static_cast<std::uint8_t>(value >> (8 * i));
        calls.push_back("word " + std::to_string(address));
    }
    void CopyBytes(std::uint32_t destination, std::uint32_t source,
                   std::uint32_t count) override {
        std::vector<std::uint8_t> copy;
        for (std::uint32_t i = 0; i < count; ++i)
            copy.push_back(ReadByte(source + i));
        for (std::uint32_t i = 0; i < count; ++i)
            bytes[destination + i] = copy[i];
        calls.push_back("copy " + std::to_string(destination) + " " +
                        std::to_string(source) + " " + std::to_string(count));
    }
    void AssignText(std::uint32_t destination,
                    std::uint32_t source) override {
        text_assignments.emplace_back(destination, source);
        calls.push_back("text " + std::to_string(destination));
        CopyBytes(destination, source, 16);
    }
};

void Seed(Runtime& runtime, std::uint32_t base, std::uint8_t value) {
    for (std::uint32_t i = 0; i < kStride; ++i)
        runtime.bytes[base + i] = static_cast<std::uint8_t>(value + i);
}

void CheckRecord(const Runtime& runtime, std::uint32_t destination,
                 std::uint32_t source, const char* message) {
    for (std::uint32_t i = 0; i < kStride; ++i)
        Check(runtime.bytes.at(destination + i) == runtime.bytes.at(source + i),
              message);
}

void CheckTexts(const Runtime& runtime, std::uint32_t destination,
                std::uint32_t source, std::size_t start) {
    Check(runtime.text_assignments.size() >= start + kTextOffsets.size(),
          "eleven text fields assigned");
    for (std::size_t i = 0; i < kTextOffsets.size(); ++i)
        Check(runtime.text_assignments[start + i] ==
                  std::make_pair(destination + kTextOffsets[i],
                                 source + kTextOffsets[i]),
              "text field addresses and order");
}

void CheckConstruct() {
    Runtime runtime;
    Seed(runtime, 0x1000, 0x31);
    Seed(runtime, 0x2000, 0xe0);
    runtime.WriteWord(0x1000 + 584, 0x12345678);
    runtime.calls.clear();
    Check(pal4::GameDataMonsterRecordCopy::Construct(
              0x2000, 0x1000, runtime) == 0x12345678,
          "construct returns source tail first DWORD");
    CheckRecord(runtime, 0x2000, 0x1000, "construct copies entire record");
    CheckTexts(runtime, 0x2000, 0x1000, 0);
    Check(runtime.calls.front() == "copy 8192 4096 336" &&
              runtime.calls[1] == "copy 8528 4432 72",
          "construct copies scalar regions before text fields");
    for (const auto offset : kTextOffsets) {
        const auto byte = "byte " + std::to_string(0x2000 + offset);
        auto it = runtime.calls.begin();
        while (it != runtime.calls.end() && *it != byte) ++it;
        Check(it != runtime.calls.end() &&
                  *(it + 1) == "word " + std::to_string(0x2000 + offset + 4) &&
                  *(it + 2) == "word " + std::to_string(0x2000 + offset + 8) &&
                  *(it + 3) == "word " + std::to_string(0x2000 + offset + 12) &&
                  *(it + 4) == "text " + std::to_string(0x2000 + offset),
              "construct initializes each text object before assignment");
    }
    Check(runtime.calls[runtime.calls.size() - 1] ==
              "copy 8776 4680 12",
          "construct copies exactly three tail DWORDs last");

    Runtime null_runtime;
    Check(pal4::GameDataMonsterRecordCopy::Construct(
              0, 0x1000, null_runtime) == 0 && null_runtime.calls.empty(),
          "null destination performs no construction");
}

void CheckAssignment() {
    Runtime runtime;
    Seed(runtime, 0x1000, 0x21);
    Seed(runtime, 0x2000, 0xd1);
    pal4::GameDataMonsterRecordCopy::Assign(0x2000, 0x1000, runtime);
    CheckRecord(runtime, 0x2000, 0x1000, "assignment copies entire record");
    CheckTexts(runtime, 0x2000, 0x1000, 0);
    Check(runtime.calls.front() == "copy 8192 4096 336" &&
              runtime.calls[1] == "copy 8528 4432 72" &&
              runtime.calls.back() == "copy 8776 4680 12",
          "assignment copies scalar regions around text fields");

    runtime.calls.clear();
    runtime.text_assignments.clear();
    pal4::GameDataMonsterRecordCopy::Assign(0x1000, 0x1000, runtime);
    CheckTexts(runtime, 0x1000, 0x1000, 0);
    CheckRecord(runtime, 0x2000, 0x1000, "self assignment preserves bytes");
}

void CheckRanges() {
    Runtime runtime;
    Seed(runtime, 0x1000, 0x30);
    for (unsigned i = 0; i < 3; ++i)
        Seed(runtime, 0x3000 + i * kStride, static_cast<std::uint8_t>(0x80 + i));
    Check(pal4::GameDataMonsterRecordCopy::AssignRange(
              0x3000, 0x3000 + 3 * kStride, 0x1000, runtime) ==
              0x3000 + 3 * kStride,
          "forward range returns destination end");
    for (unsigned i = 0; i < 3; ++i) {
        CheckRecord(runtime, 0x3000 + i * kStride, 0x1000,
                    "forward range assigns all records");
        CheckTexts(runtime, 0x3000 + i * kStride, 0x1000,
                   i * kTextOffsets.size());
    }
    Check(runtime.text_assignments.size() == 3 * kTextOffsets.size(),
          "forward range assigns exactly eleven fields per record");
    runtime.calls.clear();
    Check(pal4::GameDataMonsterRecordCopy::AssignRange(
              0x3000, 0x3000, 0x1000, runtime) == 0x3000 &&
              runtime.calls.empty(), "empty forward range is inert");

    Runtime overlap;
    Seed(overlap, 0x4000, 0x10);
    Seed(overlap, 0x4000 + kStride, 0x40);
    Seed(overlap, 0x4000 + 2 * kStride, 0x70);
    Check(pal4::GameDataMonsterRecordCopy::AssignBackward(
              0x4000, 0x4000 + 2 * kStride,
              0x4000 + 3 * kStride, overlap) == 0x4000 + kStride,
          "backward range returns new destination begin");
    Check(overlap.bytes.at(0x4000 + kStride) == 0x10 &&
              overlap.bytes.at(0x4000 + 2 * kStride) == 0x40,
          "backward assignment preserves overlapping source records");
    CheckTexts(overlap, 0x4000 + 2 * kStride, 0x4000 + kStride, 0);
    CheckTexts(overlap, 0x4000 + kStride, 0x4000, kTextOffsets.size());
    Check(overlap.text_assignments.size() == 2 * kTextOffsets.size(),
          "backward range assigns exactly eleven fields per record");
    overlap.calls.clear();
    Check(pal4::GameDataMonsterRecordCopy::AssignBackward(
              0x4000, 0x4000, 0x5000, overlap) == 0x5000 &&
              overlap.calls.empty(), "empty backward range is inert");
}

void CheckEntryAssignments() {
    Runtime runtime;
    Seed(runtime, 0x1000, 0x28);
    Seed(runtime, 0x2000, 0x91);
    Check(pal4::GameDataMonsterRecordCopy::AssignRecord(
              0x2000, 0x1000, runtime) == 0x2000,
          "complete assignment returns destination");
    CheckRecord(runtime, 0x2000, 0x1000,
                "complete assignment copies all record fields");
    CheckTexts(runtime, 0x2000, 0x1000, 0);

    Runtime subrecord;
    Seed(subrecord, 0x3000, 0x42);
    Seed(subrecord, 0x4000, 0xa3);
    subrecord.bytes[0x4000 + kStride] = 0x5a;
    const auto prefix = subrecord.bytes.at(0x4000 + 335);
    const auto after = subrecord.bytes.at(0x4000 + kStride);
    Check(pal4::GameDataMonsterRecordCopy::AssignSubrecord(
              0x4000 + 336, 0x3000 + 336, subrecord) == 0x4000 + 336,
          "subrecord assignment returns destination subobject");
    for (std::uint32_t i = 336; i < kStride; ++i)
        Check(subrecord.bytes.at(0x4000 + i) ==
                  subrecord.bytes.at(0x3000 + i),
              "subrecord assignment copies all 260 bytes");
    Check(subrecord.bytes.at(0x4000 + 335) == prefix &&
              subrecord.bytes.at(0x4000 + kStride) == after,
          "subrecord assignment leaves adjacent bytes untouched");
    CheckTexts(subrecord, 0x4000, 0x3000, 0);
}

void CheckSelected() {
    Runtime runtime;
    for (unsigned i = 0; i < 4; ++i)
        Seed(runtime, 0x5000 + i * kStride,
             static_cast<std::uint8_t>(0x20 + 0x20 * i));
    std::vector<std::pair<std::uint32_t, std::uint32_t>> compared;
    const auto equivalent = [&](std::uint32_t previous,
                                std::uint32_t current) {
        compared.emplace_back(previous, current);
        return current == 0x5000 + 2 * kStride;
    };
    Check(pal4::GameDataMonsterRecordCopy::CopySelected(
              0x5000, 0x5000 + 4 * kStride, 0x7000,
              equivalent, runtime) == 0x7000 + 3 * kStride,
          "selection copies three distinct records");
    Check(compared ==
              std::vector<std::pair<std::uint32_t, std::uint32_t>>({
                  {0x5000, 0x5000 + kStride},
                  {0x5000 + kStride, 0x5000 + 2 * kStride},
                  {0x5000 + kStride, 0x5000 + 3 * kStride}}),
          "selection compares against last retained source address");
    CheckRecord(runtime, 0x7000, 0x5000, "selection copies first record");
    CheckRecord(runtime, 0x7000 + kStride, 0x5000 + kStride,
                "selection copies second record");
    CheckRecord(runtime, 0x7000 + 2 * kStride, 0x5000 + 3 * kStride,
                "selection copies fourth record");
    Check(runtime.text_assignments.size() == 3 * kTextOffsets.size(),
          "selection assigns eleven texts for each retained record");

    Runtime empty;
    Check(pal4::GameDataMonsterRecordCopy::CopySelected(
              0x5000, 0x5000, 0x7000, equivalent, empty) == 0x7000 &&
              empty.calls.empty() && compared.size() == 3,
          "empty selection performs no reads, writes or comparisons");
    Runtime single;
    Seed(single, 0x5000, 0x67);
    Check(pal4::GameDataMonsterRecordCopy::CopySelected(
              0x5000, 0x5000 + kStride, 0x7000,
              equivalent, single) == 0x7000 + kStride &&
              compared.size() == 3,
          "single selection copies without invoking predicate");
}

}  // namespace

int main() {
    try {
        CheckConstruct();
        CheckAssignment();
        CheckRanges();
        CheckEntryAssignments();
        CheckSelected();
        std::cout << "game_data_monster_record_copy_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
