#include "pal4/game_data_property_record_copy.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <utility>
#include <vector>

namespace {

void Check(bool ok, const char* message) {
    if (!ok) throw std::runtime_error(message);
}

using Record = std::array<std::uint32_t, 42>;

struct Runtime final : pal4::IGameDataPropertyRecordCopyRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> assigned;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> constructed;
    std::uint32_t reads = 0;
    std::uint32_t writes = 0;

    std::uint32_t ReadWord(std::uint32_t address) override {
        ++reads;
        return words.at(address);
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        ++writes;
        words[address] = value;
    }
    void CopyText(std::uint32_t destination, std::uint32_t source) {
        std::array<std::uint32_t, 4> words_to_copy{};
        for (std::uint32_t i = 0; i < 4; ++i)
            words_to_copy[i] = ReadWord(source + i * 4);
        for (std::uint32_t i = 0; i < 4; ++i)
            WriteWord(destination + i * 4, words_to_copy[i]);
    }
    void AssignText(std::uint32_t destination,
                    std::uint32_t source) override {
        assigned.emplace_back(destination, source);
        CopyText(destination, source);
    }
    void ConstructText(std::uint32_t destination,
                       std::uint32_t source) override {
        constructed.emplace_back(destination, source);
        CopyText(destination, source);
    }
};

void Fill(Runtime& r, std::uint32_t address, std::uint32_t marker) {
    for (std::uint32_t i = 0; i < 42; ++i)
        r.words[address + i * 4] = (marker << 16) | i;
}

Record Snapshot(const Runtime& r, std::uint32_t address) {
    Record result{};
    for (std::uint32_t i = 0; i < 42; ++i)
        result[i] = r.words.at(address + i * 4);
    return result;
}

void CheckAssignRange() {
    Runtime r;
    Fill(r, 0x2000, 7);
    const auto expected = Snapshot(r, 0x2000);
    for (std::uint32_t i = 0; i < 3; ++i)
        Fill(r, 0x4000 + i * 168, 30 + i);
    Check(pal4::GameDataPropertyRecordCopy::AssignRange(
              0x4000, 0x4000 + 3 * 168, 0x2000, r) ==
              0x4000 + 3 * 168,
          "range assignment returns destination end");
    for (std::uint32_t i = 0; i < 3; ++i)
        Check(Snapshot(r, 0x4000 + i * 168) == expected,
              "every 168-byte destination equals full source record");
    Check(r.assigned.size() == 21 && r.constructed.empty() &&
              r.assigned.front() ==
                  std::pair<std::uint32_t, std::uint32_t>{0x4010, 0x2010} &&
              r.assigned[6] ==
                  std::pair<std::uint32_t, std::uint32_t>{0x4070, 0x2070} &&
              r.assigned[7] ==
                  std::pair<std::uint32_t, std::uint32_t>{0x40b8, 0x2010},
          "seven text fields are assigned for each record in address order");

    r.assigned.clear();
    Check(pal4::GameDataPropertyRecordCopy::AssignRange(
              0x4000, 0x4000, 0x2000, r) == 0x4000 &&
              r.assigned.empty(),
          "empty range performs no text assignment");
}

void CheckBackwardOverlap() {
    Runtime r;
    std::array<Record, 3> original{};
    for (std::uint32_t i = 0; i < 3; ++i) {
        Fill(r, 0x2000 + i * 168, i + 1);
        original[i] = Snapshot(r, 0x2000 + i * 168);
    }
    Fill(r, 0x2000 + 3 * 168, 99);
    Check(pal4::GameDataPropertyRecordCopy::AssignBackward(
              0x2000, 0x2000 + 3 * 168,
              0x2000 + 4 * 168, r) == 0x2000 + 168,
          "backward assignment returns destination begin");
    for (std::uint32_t i = 0; i < 3; ++i)
        Check(Snapshot(r, 0x2000 + (i + 1) * 168) == original[i],
              "overlapping right shift preserves original source records");
    Check(r.assigned.size() == 21 &&
              r.assigned.front() ==
                  std::pair<std::uint32_t, std::uint32_t>{
                      0x2000 + 3 * 168 + 16,
                      0x2000 + 2 * 168 + 16} &&
              r.assigned[7] ==
                  std::pair<std::uint32_t, std::uint32_t>{
                      0x2000 + 2 * 168 + 16,
                      0x2000 + 168 + 16},
          "overlapping assignment visits last source first");
    r.assigned.clear();
    Check(pal4::GameDataPropertyRecordCopy::AssignBackward(
              0x2000, 0x2000, 0x4000, r) == 0x4000 &&
              r.assigned.empty(),
          "empty backward range leaves destination end unchanged");
}

void CheckConstruct() {
    Runtime r;
    Fill(r, 0x2000, 7);
    Fill(r, 0x4000, 42);
    Check(pal4::GameDataPropertyRecordCopy::Construct(
              0, 0x2000, r) == 0 &&
              r.reads == 0 && r.writes == 0 &&
              r.constructed.empty(),
          "null destination skips all record construction");
    Check(pal4::GameDataPropertyRecordCopy::Construct(
              0x4000, 0x2000, r) == 0x2000 &&
              Snapshot(r, 0x4000) == Snapshot(r, 0x2000) &&
              r.constructed.size() == 7 && r.assigned.empty() &&
              r.constructed.front() ==
                  std::pair<std::uint32_t, std::uint32_t>{0x4010, 0x2010} &&
              r.constructed.back() ==
                  std::pair<std::uint32_t, std::uint32_t>{0x4070, 0x2070},
          "copy constructor copies scalar head, seven texts and scalar tail");
}

void CheckAssignAndUniqueCopy() {
    Runtime single;
    Fill(single, 0x2000, 7);
    Fill(single, 0x4000, 9);
    Check(pal4::GameDataPropertyRecordCopy::Assign(
              0x4000, 0x2000, single) == 0x4000 &&
              Snapshot(single, 0x4000) == Snapshot(single, 0x2000) &&
              single.assigned.size() == 7 &&
              single.assigned.front() ==
                  std::pair<std::uint32_t, std::uint32_t>{0x4010, 0x2010},
          "single assignment copies all 42 words and seven text members");

    Runtime r;
    const std::array<std::int32_t, 6> keys{4, 4, -2, 4, 7, 7};
    for (std::uint32_t i = 0; i < keys.size(); ++i) {
        const auto address = 0x5000 + i * 168;
        Fill(r, address, i + 1);
        r.words[address] = static_cast<std::uint32_t>(keys[i]);
    }
    Check(pal4::GameDataPropertyRecordCopy::UniqueCopy(
              0x5000, 0x5000 + 6 * 168, 0x5000, r) ==
              0x5000 + 4 * 168,
          "adjacent duplicates compact to four records");
    Check(r.words[0x5000 + 164] == ((1u << 16) | 41) &&
              r.words[0x5000 + 168 + 164] == ((3u << 16) | 41) &&
              r.words[0x5000 + 336 + 164] == ((4u << 16) | 41) &&
              r.words[0x5000 + 504 + 164] == ((5u << 16) | 41) &&
              r.assigned.size() == 28,
          "first record in each run remains; separated duplicate survives");
    r.assigned.clear();
    Check(pal4::GameDataPropertyRecordCopy::UniqueCopy(
              0x6000, 0x6000, 0x7000, r) == 0x7000 &&
              r.assigned.empty(),
          "empty unique range leaves destination untouched");
}

}  // namespace

int main() {
    try {
        CheckAssignRange();
        CheckBackwardOverlap();
        CheckConstruct();
        CheckAssignAndUniqueCopy();
        std::cout << "game_data_property_record_copy_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
