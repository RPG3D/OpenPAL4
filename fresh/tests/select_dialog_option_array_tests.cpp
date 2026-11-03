#include "pal4/select_dialog_option_records.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

constexpr std::uint32_t kArray = 0x5000u;
constexpr std::uint32_t kSource = 0x6000u;

struct Runtime final : pal4::ISelectDialogOptionArrayRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> copies;
    std::vector<std::uint32_t> frees;
    std::vector<std::uint32_t> allocations;
    std::uint32_t next_buffer = 0xf000u;

    Runtime() {
        words[kSource + 4u] = 0xa001u;
        words[kSource + 8u] = 1u;
        words[kSource + 12u] = 2u;
        bytes[0xa000u] = 1u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes[address];
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        bytes[address] = value;
    }
    void Free(std::uint32_t address) override {
        frees.push_back(address);
    }
    std::uint32_t Allocate(std::uint32_t bytes_count) override {
        allocations.push_back(bytes_count);
        return 0xb000u;
    }
    void CopyRecord(std::uint32_t destination,
                    std::uint32_t source) override {
        copies.emplace_back(destination, source);
        for (std::uint32_t offset = 0u; offset < 16u; offset += 4u)
            words[destination + offset] = words[source + offset];
    }
    std::uint32_t EmptyStringPointer() override { return 0xe000u; }
    bool AllocateBuffer(std::uint32_t record, std::uint32_t,
                        bool copy_mode) override {
        Check(copy_mode, "record replacement copy mode");
        words[record + 4u] = next_buffer++;
        return true;
    }
    void CopyBytes(std::uint32_t, std::uint32_t,
                   std::uint32_t) override {}
    void DetachAndReset(std::uint32_t) override {
        throw std::runtime_error("unexpected self replacement");
    }
    void SetSize(std::uint32_t record,
                 std::uint32_t size) override {
        words[record + 8u] = size;
    }
    void ThrowRangeError() override {
        throw std::out_of_range("option range");
    }
};

void EmptyAndGrowth() {
    Runtime empty;
    Check(pal4::SelectDialogOptionArray::Size(kArray, empty) == 0u,
          "null array size");
    Check(pal4::SelectDialogOptionArray::Insert(
              kArray, 0u, kSource, empty) == 0xb000u &&
              empty.allocations == std::vector<std::uint32_t>{16u} &&
              empty.words.at(kArray + 4u) == 0xb000u &&
              empty.words.at(kArray + 8u) == 0xb010u &&
              empty.words.at(kArray + 12u) == 0xb010u &&
              empty.copies ==
                  std::vector<std::pair<std::uint32_t,
                                        std::uint32_t>>{{0xb000u, kSource}} &&
              empty.frees == std::vector<std::uint32_t>{0u},
          "empty insert allocates one record and initializes all pointers");
    Check(pal4::SelectDialogOptionArray::Size(kArray, empty) == 1u,
          "grown array count");

    Runtime full;
    full.words[kArray + 4u] = 0x1000u;
    full.words[kArray + 8u] = 0x1020u;
    full.words[kArray + 12u] = 0x1020u;
    Check(pal4::SelectDialogOptionArray::Insert(
              kArray, 0x1010u, kSource, full) == 0xb010u &&
              full.allocations == std::vector<std::uint32_t>{64u} &&
              full.words.at(kArray + 4u) == 0xb000u &&
              full.words.at(kArray + 8u) == 0xb030u &&
              full.words.at(kArray + 12u) == 0xb040u &&
              full.copies ==
                  std::vector<std::pair<std::uint32_t,
                                        std::uint32_t>>{
                      {0xb000u, 0x1000u}, {0xb010u, kSource},
                      {0xb020u, 0x1010u}} &&
              full.frees == std::vector<std::uint32_t>{0x1000u},
          "full array doubles from two to four records around insertion");
    Check(pal4::SelectDialogOptionArray::Size(kArray, full) == 3u,
          "grown middle insertion count");
}

void AvailableCapacity() {
    Runtime middle;
    middle.words[kArray + 4u] = 0x1000u;
    middle.words[kArray + 8u] = 0x1030u;
    middle.words[kArray + 12u] = 0x1040u;
    Check(pal4::SelectDialogOptionArray::Insert(
              kArray, 0x1010u, kSource, middle) == 0x1010u &&
              middle.words.at(kArray + 8u) == 0x1040u &&
              middle.words.at(kArray + 4u) == 0x1000u &&
              middle.words.at(kArray + 12u) == 0x1040u &&
              middle.copies.front() ==
                  std::pair<std::uint32_t, std::uint32_t>{0x1030u, 0x1020u} &&
              middle.words.at(0x1010u + 4u) == 0xa001u &&
              middle.words.at(0x1010u + 8u) == 1u &&
              middle.allocations.empty(),
          "spare capacity shifts last record and replaces insertion slot");

    Runtime append;
    append.words[kArray + 4u] = 0x1000u;
    append.words[kArray + 8u] = 0x1010u;
    append.words[kArray + 12u] = 0x1030u;
    Check(pal4::SelectDialogOptionArray::Insert(
              kArray, 0x1010u, kSource, append) == 0x1010u &&
              append.words.at(kArray + 8u) == 0x1020u &&
              append.copies ==
                  std::vector<std::pair<std::uint32_t,
                                        std::uint32_t>>{{0x1010u, kSource}} &&
              append.allocations.empty(),
          "spare-capacity tail insert copies one record");
}

}  // namespace

int main() {
    try {
        EmptyAndGrowth();
        AvailableCapacity();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
