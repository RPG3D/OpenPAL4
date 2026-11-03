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

struct Runtime final : pal4::ISelectDialogRecordRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::uint32_t> freed;
    std::vector<std::pair<std::uint32_t,
                          std::uint32_t>> copies;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes.at(address);
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        bytes[address] = value;
    }
    void Free(std::uint32_t address) override {
        freed.push_back(address);
    }
    void CopyRecord(std::uint32_t destination,
                    std::uint32_t source) override {
        copies.emplace_back(destination, source);
    }
};

void CheckCleanup() {
    Runtime runtime;
    pal4::SelectDialogOptionRecords::Cleanup(
        0x1000u, 0x1000u, runtime);
    Check(runtime.words.empty(), "empty cleanup has no writes");

    const std::vector<std::uint32_t> pointers{
        0x2001u, 0x2101u, 0x2201u, 0u};
    for (std::size_t index = 0u; index < pointers.size(); ++index) {
        const auto record = 0x1000u +
            static_cast<std::uint32_t>(index) * 16u;
        runtime.words[record + 4u] = pointers[index];
        runtime.words[record + 8u] = 5u;
        runtime.words[record + 12u] = 8u;
    }
    runtime.bytes[0x2000u] = 0u;
    runtime.bytes[0x2100u] = 4u;
    runtime.bytes[0x2200u] = 0xffu;
    pal4::SelectDialogOptionRecords::Cleanup(
        0x1000u, 0x1040u, runtime);
    Check(runtime.freed ==
              std::vector<std::uint32_t>{0x2000u, 0x2200u} &&
              runtime.bytes.at(0x2100u) == 3u,
          "reference zero/255 freed and shared buffer decremented");
    for (auto record = 0x1000u; record < 0x1040u;
         record += 16u) {
        Check(runtime.words.at(record + 4u) == 0u &&
                  runtime.words.at(record + 8u) == 0u &&
                  runtime.words.at(record + 12u) == 0u,
              "record buffer fields reset");
    }
}

void CheckCopies() {
    Runtime runtime;
    Check(pal4::SelectDialogOptionRecords::CopyRange(
              0x1000u, 0x1000u, 0x3000u,
              runtime) == 0x3000u &&
              runtime.copies.empty(),
          "empty range returns destination");
    Check(pal4::SelectDialogOptionRecords::CopyRange(
              0x1000u, 0x1030u, 0x3000u,
              runtime) == 0x3030u &&
              runtime.copies ==
                  std::vector<std::pair<std::uint32_t,
                                        std::uint32_t>>{
                      {0x3000u, 0x1000u},
                      {0x3010u, 0x1010u},
                      {0x3020u, 0x1020u}},
          "range copies 16-byte records");
    runtime.copies.clear();
    pal4::SelectDialogOptionRecords::CopyRepeated(
        0x5000u, 0u, 0x6000u, runtime);
    Check(runtime.copies.empty(), "zero repeats do nothing");
    pal4::SelectDialogOptionRecords::CopyRepeated(
        0x5000u, 3u, 0x6000u, runtime);
    Check(runtime.copies ==
              std::vector<std::pair<std::uint32_t,
                                    std::uint32_t>>{
                  {0x5000u, 0x6000u},
                  {0x5010u, 0x6000u},
                  {0x5020u, 0x6000u}},
          "repeated copy advances destination only");
}

struct ReplaceRuntime final : pal4::ISelectDialogReplaceRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::uint32_t> freed;
    std::vector<std::pair<std::uint32_t,
                          std::uint32_t>> allocations;
    std::vector<std::pair<std::uint32_t,
                          std::uint32_t>> copies;
    std::vector<std::uint32_t> detached;
    bool allocate_success = true;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes.at(address);
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        bytes[address] = value;
    }
    std::uint32_t EmptyStringPointer() override {
        return 0x9001u;
    }
    void Free(std::uint32_t address) override {
        freed.push_back(address);
    }
    bool AllocateBuffer(std::uint32_t record,
                        std::uint32_t length,
                        bool copy_mode) override {
        allocations.emplace_back(record, length);
        Check(copy_mode, "deep-copy allocation mode");
        if (allocate_success) words[record + 4u] = 0xa001u;
        return allocate_success;
    }
    void CopyBytes(std::uint32_t destination,
                   std::uint32_t source,
                   std::uint32_t length) override {
        Check(destination == 0xa001u,
              "deep-copy destination");
        copies.emplace_back(source, length);
    }
    void DetachAndReset(std::uint32_t record) override {
        detached.push_back(record);
    }
    void SetSize(std::uint32_t record,
                 std::uint32_t size) override {
        words[record + 8u] = size;
    }
    void ThrowRangeError() override {
        throw std::out_of_range("self replace range");
    }
};

void CheckReplaceShareAndDeepCopy() {
    ReplaceRuntime runtime;
    runtime.words[0x1004u] = 0x2001u;
    runtime.words[0x1008u] = 3u;
    runtime.words[0x100cu] = 5u;
    runtime.bytes[0x2000u] = 1u;
    runtime.words[0x3004u] = 0x4001u;
    runtime.words[0x3008u] = 4u;
    runtime.words[0x300cu] = 8u;
    runtime.bytes[0x4000u] = 1u;
    Check(pal4::SelectDialogOptionRecords::ReplaceRange(
              0x1000u, 0x1010u, 0x3000u,
              runtime) == 0x1010u &&
              runtime.words.at(0x1004u) == 0x4001u &&
              runtime.words.at(0x1008u) == 4u &&
              runtime.words.at(0x100cu) == 8u &&
              runtime.bytes.at(0x2000u) == 0u &&
              runtime.bytes.at(0x4000u) == 2u &&
              runtime.allocations.empty(),
          "shared source releases old and increments new ref");

    ReplaceRuntime deep;
    deep.words[0x3004u] = 0x5001u;
    deep.words[0x3008u] = 3u;
    deep.words[0x300cu] = 4u;
    deep.words[0x1004u] = 0u;
    deep.words[0x1008u] = 0u;
    deep.words[0x100cu] = 0u;
    deep.bytes[0x5000u] = 0xfeu;
    Check(pal4::SelectDialogOptionRecords::ReplaceRange(
              0x1000u, 0x1010u, 0x3000u,
              deep) == 0x1010u &&
              deep.allocations ==
                  std::vector<std::pair<std::uint32_t,
                                        std::uint32_t>>{{0x1000u,
                                                         3u}} &&
              deep.copies ==
                  std::vector<std::pair<std::uint32_t,
                                        std::uint32_t>>{{0x5001u,
                                                         3u}} &&
              deep.words.at(0x1008u) == 3u &&
              deep.bytes.at(0xa004u) == 0u,
          "high-reference source allocates and terminates copy");
}

void CheckReplaceEmptyAndSelf() {
    ReplaceRuntime empty;
    empty.words[0x3004u] = 0u;
    empty.words[0x3008u] = 0u;
    empty.words[0x300cu] = 0u;
    empty.words[0x1004u] = 0u;
    empty.words[0x1008u] = 0u;
    empty.words[0x100cu] = 0u;
    Check(pal4::SelectDialogOptionRecords::ReplaceRange(
              0x1000u, 0x1010u, 0x3000u,
              empty) == 0x1010u &&
              empty.copies ==
                  std::vector<std::pair<std::uint32_t,
                                        std::uint32_t>>{{0x9001u,
                                                         0u}},
          "empty source allocates from global empty text");

    ReplaceRuntime self;
    self.words[0x1004u] = 0x5001u;
    self.words[0x1008u] = 4u;
    self.words[0x100cu] = 8u;
    Check(pal4::SelectDialogOptionRecords::ReplaceRange(
              0x1000u, 0x1010u, 0x1000u,
              self) == 0x1010u &&
              self.detached ==
                  std::vector<std::uint32_t>{0x1000u,
                                             0x1000u} &&
              self.allocations.empty(),
          "self source performs two detach operations");

    ReplaceRuntime failure;
    failure.allocate_success = false;
    failure.words[0x3004u] = 0x5001u;
    failure.words[0x3008u] = 2u;
    failure.words[0x300cu] = 3u;
    failure.words[0x1004u] = 0u;
    failure.words[0x1008u] = 0u;
    failure.words[0x100cu] = 0u;
    failure.bytes[0x5000u] = 0xffu;
    (void)pal4::SelectDialogOptionRecords::ReplaceRange(
        0x1000u, 0x1010u, 0x3000u, failure);
    Check(failure.allocations.size() == 1u &&
              failure.copies.empty(),
          "failed allocation does not copy");
}

void CheckReplaceBackward() {
    ReplaceRuntime runtime;
    runtime.words[0x1004u] = 0x5001u;
    runtime.words[0x1008u] = 2u;
    runtime.words[0x100cu] = 3u;
    runtime.words[0x1014u] = 0x5101u;
    runtime.words[0x1018u] = 3u;
    runtime.words[0x101cu] = 4u;
    runtime.words[0x1024u] = 0u;
    runtime.words[0x1028u] = 0u;
    runtime.words[0x102cu] = 0u;
    runtime.words[0x1034u] = 0u;
    runtime.words[0x1038u] = 0u;
    runtime.words[0x103cu] = 0u;
    runtime.bytes[0x5000u] = 1u;
    runtime.bytes[0x5100u] = 1u;
    Check(pal4::SelectDialogOptionRecords::ReplaceRangeBackward(
              0x1000u, 0x1020u, 0x1040u,
              runtime) == 0x1020u &&
              runtime.words.at(0x1024u) == 0x5001u &&
              runtime.words.at(0x1034u) == 0x5101u &&
              runtime.bytes.at(0x5000u) == 2u &&
              runtime.bytes.at(0x5100u) == 2u,
          "backward replacement preserves overlapping sources");
    Check(pal4::SelectDialogOptionRecords::ReplaceRangeBackward(
              0x1000u, 0x1000u, 0x1040u,
              runtime) == 0x1040u,
          "empty backward range returns destination end");
}

}  // namespace

int main() {
    try {
        CheckCleanup();
        CheckCopies();
        CheckReplaceShareAndDeepCopy();
        CheckReplaceEmptyAndSelf();
        CheckReplaceBackward();
        std::cout << "select_dialog_option_records_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
