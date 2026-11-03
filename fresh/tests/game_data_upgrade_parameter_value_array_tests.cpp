#include "pal4/game_data_upgrade_parameter_value_array.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

using Record = pal4::GameDataUpgradeParameterRecord;
constexpr std::uint32_t kStride = 92;

struct Runtime final : pal4::IGameDataUpgradeParameterValueArrayRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, Record> records;
    std::vector<std::string> calls;
    std::uint32_t allocation = 0x6000;

    Runtime() {
        words[0x1004] = 0x2000;
        words[0x1008] = 0x2000 + 3 * kStride;
        words[0x100c] = 0x2000 + 4 * kStride;
        for (std::uint32_t i = 0; i < 3; ++i) {
            Record record{};
            record[0] = i + 1;
            record[22] = i + 100;
            records[0x2000 + i * kStride] = record;
        }
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        calls.push_back("word " + std::to_string(address));
    }
    Record ReadRecord(std::uint32_t address) override {
        return records.at(address);
    }
    void WriteRecord(std::uint32_t address,
                     const Record& record) override {
        records[address] = record;
        calls.push_back("record " + std::to_string(address));
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        calls.push_back("allocate " + std::to_string(bytes));
        return allocation;
    }
    void Free(std::uint32_t address) override {
        calls.push_back("free " + std::to_string(address));
    }
};

Record NewRecord() {
    Record record{};
    record[0] = 9;
    record[22] = 999;
    return record;
}

void CheckSizeAndCopy() {
    Runtime runtime;
    Check(pal4::GameDataUpgradeParameterValueArray::Size(0x1000, runtime) ==
              3,
          "size divides the live byte span by 92");
    runtime.words[0x1004] = 0;
    Check(pal4::GameDataUpgradeParameterValueArray::Size(0x1000, runtime) ==
              0,
          "null begin yields zero size");

    runtime = Runtime{};
    const auto end = 0x2000 + 3 * kStride;
    Check(pal4::GameDataUpgradeParameterValueArray::CopyRecords(
              0x2000, end, 0x8000, runtime) == 0x8000 + 3 * kStride &&
              runtime.records[0x8000][0] == 1 &&
              runtime.records[0x8000 + 2 * kStride][22] == 102,
          "range copy preserves complete first and last records");
    runtime.calls.clear();
    Check(pal4::GameDataUpgradeParameterValueArray::CopyRecords(
              0x2000, end, 0, runtime) == 3 * kStride &&
              runtime.records[kStride][0] == 2 &&
              runtime.records[2 * kStride][22] == 102 &&
              runtime.calls.size() == 2,
          "null destination skips only first write and advances address");
    Check(pal4::GameDataUpgradeParameterValueArray::CopyRecords(
              0x2000, 0x2000, 0x9000, runtime) == 0x9000,
          "empty range returns destination unchanged");
}

void CheckSingleInsert() {
    const auto record = NewRecord();
    const auto middle = 0x2000 + kStride;
    Runtime inplace;
    Check(pal4::GameDataUpgradeParameterValueArray::Insert(
              0x1000, middle, record, inplace) == middle &&
              inplace.words[0x1008] == 0x2000 + 4 * kStride &&
              inplace.records[middle][22] == 999 &&
              inplace.records[middle + kStride][0] == 2 &&
              inplace.records[middle + 2 * kStride][22] == 102,
          "spare-capacity insertion shifts whole records and returns position");

    Runtime growth;
    growth.words[0x100c] = growth.words[0x1008];
    Check(pal4::GameDataUpgradeParameterValueArray::Insert(
              0x1000, middle, record, growth) == 0x6000 + kStride &&
              growth.words[0x1004] == 0x6000 &&
              growth.words[0x1008] == 0x6000 + 4 * kStride &&
              growth.words[0x100c] == 0x6000 + 6 * kStride &&
              growth.records[0x6000][0] == 1 &&
              growth.records[0x6000 + kStride][22] == 999 &&
              growth.records[0x6000 + 3 * kStride][22] == 102 &&
              growth.calls.front() == "allocate 552",
          "full array doubles capacity and preserves full suffix");

    Runtime empty;
    empty.words[0x1004] = empty.words[0x1008] = empty.words[0x100c] = 0;
    Check(pal4::GameDataUpgradeParameterValueArray::Insert(
              0x1000, 0, record, empty) == 0x6000 &&
              empty.words[0x1008] == 0x6000 + kStride &&
              empty.records[0x6000][22] == 999,
          "null array allocates one record");

    Runtime failed;
    failed.words[0x100c] = failed.words[0x1008];
    failed.allocation = 0;
    Check(pal4::GameDataUpgradeParameterValueArray::Insert(
              0x1000, middle, record, failed) == kStride &&
              failed.words[0x1004] == 0 &&
              failed.words[0x1008] == 4 * kStride &&
              failed.records[kStride][22] == 999 &&
              failed.calls.front() == "allocate 552",
          "original null allocation path retains raw pointer arithmetic");
}

void CheckMultipleInsert() {
    const auto record = NewRecord();
    const auto middle = 0x2000 + kStride;
    Runtime inplace;
    inplace.words[0x100c] = 0x2000 + 5 * kStride;
    Check(pal4::GameDataUpgradeParameterValueArray::InsertCopies(
              0x1000, middle, 2, record, inplace) == middle + 2 * kStride &&
              inplace.words[0x1008] == 0x2000 + 5 * kStride &&
              inplace.records[middle][22] == 999 &&
              inplace.records[middle + kStride][22] == 999 &&
              inplace.records[middle + 2 * kStride][0] == 2 &&
              inplace.records[middle + 3 * kStride][0] == 3,
          "two-copy insertion shifts a long suffix in place");

    Runtime short_suffix;
    short_suffix.words[0x100c] = 0x2000 + 5 * kStride;
    Check(pal4::GameDataUpgradeParameterValueArray::InsertCopies(
              0x1000, 0x2000 + 2 * kStride, 2, record,
              short_suffix) == 0x2000 + 5 * kStride &&
              short_suffix.records[0x2000 + 4 * kStride][0] == 3,
          "short suffix branch returns new end");

    Runtime growth;
    growth.words[0x100c] = growth.words[0x1008];
    Check(pal4::GameDataUpgradeParameterValueArray::InsertCopies(
              0x1000, middle, 2, record, growth) == 0x2000 &&
              growth.words[0x1004] == 0x6000 &&
              growth.words[0x1008] == 0x6000 + 5 * kStride &&
              growth.words[0x100c] == 0x6000 + 6 * kStride &&
              growth.records[0x6000 + 4 * kStride][22] == 102,
          "growth path returns old begin and retains suffix");

    Runtime no_count;
    Check(pal4::GameDataUpgradeParameterValueArray::InsertCopies(
              0x1000, middle, 0, record, no_count) == 0 &&
              no_count.calls.empty(),
          "zero copies make no writes");
}

void CheckFill() {
    Runtime runtime;
    const auto record = NewRecord();
    Check(pal4::GameDataUpgradeParameterValueArray::FillRecords(
              0x8000, 3, record, runtime) == 0x8000 + 3 * kStride &&
              runtime.records[0x8000][22] == 999 &&
              runtime.records[0x8000 + 2 * kStride][22] == 999,
          "fill writes full records and returns end");
    runtime.calls.clear();
    Check(pal4::GameDataUpgradeParameterValueArray::FillRecords(
              0, 3, record, runtime) == 3 * kStride &&
              runtime.records[kStride][22] == 999 &&
              runtime.calls.size() == 2,
          "null fill destination skips first write and advances");
    runtime.calls.clear();
    Check(pal4::GameDataUpgradeParameterValueArray::FillRecords(
              0x8000, 0, record, runtime) == 0 &&
              runtime.calls.empty(),
          "zero fill returns zero");
}

void CheckConditionalCopyAndCompaction() {
    Runtime runtime;
    runtime.records[0x2000][0] = 17;
    runtime.records[0x2000][22] = 7;
    runtime.calls.clear();
    pal4::GameDataUpgradeParameterValueArray::CopyRecordIfPresent(
        0, 0x2000, runtime);
    Check(runtime.calls.empty(), "null destination suppresses record copy");
    pal4::GameDataUpgradeParameterValueArray::CopyRecordIfPresent(
        0x8000, 0x2000, runtime);
    Check(runtime.records.at(0x8000) == runtime.records.at(0x2000),
          "conditional copy transfers the complete 92-byte record");

    const auto stride = kStride;
    for (std::uint32_t i = 0; i < 5; ++i) {
        Record record{};
        record[0] = 100 + i;
        record[22] = i == 0 || i == 1 || i == 3 ? 7 : 9;
        runtime.records[0x3000 + i * stride] = record;
    }
    std::vector<std::pair<std::uint32_t, std::uint32_t>> comparisons;
    const auto equal = [&](const Record& left, const Record& right) {
        comparisons.emplace_back(left[0], right[0]);
        return left[22] == right[22];
    };
    const auto end = pal4::GameDataUpgradeParameterValueArray::CompactAdjacent(
        0x3000, 0x3000 + 5 * stride, 0x9000, equal, runtime);
    Check(end == 0x9000 + 4 * stride &&
              runtime.records.at(0x9000)[0] == 100 &&
              runtime.records.at(0x9000 + stride)[0] == 102 &&
              runtime.records.at(0x9000 + 2 * stride)[0] == 103 &&
              runtime.records.at(0x9000 + 3 * stride)[0] == 104,
          "compaction preserves complete nonadjacent distinct records");
    Check(comparisons ==
              std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                  {100, 101}, {100, 102}, {102, 103}, {103, 104}},
          "comparator uses the last retained source record");
    Check(pal4::GameDataUpgradeParameterValueArray::CompactAdjacent(
              0x3000, 0x3000, 0xa000, equal, runtime) == 0xa000,
          "empty compaction returns destination unchanged");
    Check(pal4::GameDataUpgradeParameterValueArray::CompactAdjacent(
              0x3000, 0x3000 + 5 * stride, 0x3000, equal, runtime) ==
              0x3000 + 4 * stride &&
              runtime.records.at(0x3000 + stride)[0] == 102,
          "compaction supports an in-place destination");
}

}  // namespace

int main() {
    try {
        CheckSizeAndCopy();
        CheckSingleInsert();
        CheckMultipleInsert();
        CheckFill();
        CheckConditionalCopyAndCompaction();
        std::cout << "game_data_upgrade_parameter_value_array_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
