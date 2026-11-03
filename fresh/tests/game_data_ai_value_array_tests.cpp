#include "pal4/game_data_ai_value_array.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <new>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

template <typename Record, typename Interface, std::uint32_t Stride>
struct Runtime final : Interface {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, Record> records;
    std::vector<std::string> calls;
    std::uint32_t allocation = 0x6000;

    Runtime() {
        words[0x1004] = 0x2000;
        words[0x1008] = 0x2000 + 3 * Stride;
        words[0x100c] = 0x2000 + 4 * Stride;
        for (std::uint32_t i = 0; i < 3; ++i) {
            Record record{};
            record[0] = i + 1;
            record.back() = i + 100;
            records[0x2000 + i * Stride] = record;
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

using Runtime56 = Runtime<pal4::GameDataAiRecord,
                          pal4::IGameDataAiValueArrayRuntime, 56>;
using Runtime68 = Runtime<pal4::GameDataAiPropertyRecord,
                          pal4::IGameDataAiPropertyValueArrayRuntime, 68>;

template <typename RuntimeType, typename Record, std::uint32_t Stride,
          typename Inserter>
void CheckInsertPaths(Inserter insert) {
    Record new_record{};
    new_record[0] = 9;
    new_record.back() = 999;

    RuntimeType inplace;
    const auto middle = 0x2000 + Stride;
    Check(insert(0x1000, middle, new_record, inplace) == middle &&
              inplace.words[0x1008] == 0x2000 + 4 * Stride &&
              inplace.records[0x2000][0] == 1 &&
              inplace.records[middle][0] == 9 &&
              inplace.records[middle].back() == 999 &&
              inplace.records[0x2000 + 2 * Stride][0] == 2 &&
              inplace.records[0x2000 + 3 * Stride][0] == 3 &&
              inplace.calls.back() == "word 4104",
          "in-place insertion moves full records and advances end");

    RuntimeType growth;
    growth.words[0x100c] = growth.words[0x1008];
    Check(insert(0x1000, middle, new_record, growth) ==
              0x6000 + Stride &&
              growth.words[0x1004] == 0x6000 &&
              growth.words[0x1008] == 0x6000 + 4 * Stride &&
              growth.words[0x100c] == 0x6000 + 6 * Stride &&
              growth.records[0x6000][0] == 1 &&
              growth.records[0x6000 + Stride][0] == 9 &&
              growth.records[0x6000 + 2 * Stride][0] == 2 &&
              growth.records[0x6000 + 3 * Stride][0] == 3 &&
              growth.records[0x6000 + 3 * Stride].back() == 102 &&
              growth.calls.front() ==
                  "allocate " + std::to_string(6 * Stride),
          "full array doubles capacity and preserves complete suffix records");

    RuntimeType empty;
    empty.words[0x1004] = empty.words[0x1008] = empty.words[0x100c] = 0;
    Check(insert(0x1000, 0, new_record, empty) == 0x6000 &&
              empty.words[0x1008] == empty.words[0x100c] &&
              empty.words[0x1008] == 0x6000 + Stride &&
              empty.records[0x6000].back() == 999,
          "null array allocates its first element");

    RuntimeType failed;
    failed.words[0x100c] = failed.words[0x1008];
    failed.allocation = 0;
    try {
        (void)insert(0x1000, middle, new_record, failed);
        Check(false, "null allocation should stop insertion");
    } catch (const std::bad_alloc&) {
        Check(failed.words[0x1004] == 0x2000 &&
                  failed.words[0x1008] == 0x2000 + 3 * Stride &&
                  failed.records[middle][0] == 2 &&
                  failed.calls == std::vector<std::string>{
                                      "allocate " +
                                      std::to_string(6 * Stride)},
              "failed allocation preserves old records");
    }
}

void CheckCopyHelper() {
    Runtime56 runtime;
    const auto end = 0x2000 + 3 * 56;
    Check(pal4::GameDataAiValueArray::CopyRecords(
              0x2000, end, 0x8000, runtime) == 0x8000 + 3 * 56 &&
              runtime.records[0x8000][0] == 1 &&
              runtime.records[0x8000 + 2 * 56].back() == 102,
          "56-byte copy helper copies full records and returns destination end");
    runtime.calls.clear();
    Check(pal4::GameDataAiValueArray::CopyRecords(
              0x2000, end, 0, runtime) == 3 * 56 &&
              runtime.records[56][0] == 2 &&
              runtime.records[112][0] == 3 &&
              runtime.calls.size() == 2,
          "null destination skips only the first write before advancing");
    Check(pal4::GameDataAiValueArray::CopyRecords(
              0x2000, 0x2000, 0x9000, runtime) == 0x9000,
          "empty copy returns the original destination");
}

template <typename RuntimeType, typename Record, std::uint32_t Stride,
          typename Inserter>
void CheckCopyInsertion(Inserter insert) {
    Record record{};
    record[0] = 9;
    record.back() = 999;

    RuntimeType inplace;
    inplace.words[0x100c] = 0x2000 + 5 * Stride;
    const auto middle = 0x2000 + Stride;
    Check(insert(0x1000, middle, 2, record, inplace) ==
              middle + 2 * Stride &&
              inplace.words[0x1008] == 0x2000 + 5 * Stride &&
              inplace.records[0x2000][0] == 1 &&
              inplace.records[middle][0] == 9 &&
              inplace.records[middle + Stride][0] == 9 &&
              inplace.records[middle + 2 * Stride][0] == 2 &&
              inplace.records[middle + 3 * Stride][0] == 3,
          "two-copy insertion shifts a two-record suffix in place");

    RuntimeType short_suffix;
    short_suffix.words[0x100c] = 0x2000 + 5 * Stride;
    Check(insert(0x1000, 0x2000 + 2 * Stride, 2, record,
                 short_suffix) == 0x2000 + 5 * Stride &&
              short_suffix.records[0x2000 + 4 * Stride][0] == 3,
          "short suffix returns new end after filling two copies");

    RuntimeType append;
    Check(insert(0x1000, append.words[0x1008], 1, record, append) ==
              0x2000 + 4 * Stride &&
              append.records[0x2000 + 3 * Stride].back() == 999,
          "append with spare capacity returns the new end");

    RuntimeType growth;
    growth.words[0x100c] = growth.words[0x1008];
    Check(insert(0x1000, middle, 2, record, growth) == 0x2000 &&
              growth.words[0x1004] == 0x6000 &&
              growth.words[0x1008] == 0x6000 + 5 * Stride &&
              growth.words[0x100c] == 0x6000 + 6 * Stride &&
              growth.records[0x6000][0] == 1 &&
              growth.records[0x6000 + Stride][0] == 9 &&
              growth.records[0x6000 + 2 * Stride][0] == 9 &&
              growth.records[0x6000 + 3 * Stride][0] == 2 &&
              growth.records[0x6000 + 4 * Stride][0] == 3,
          "growth path returns old begin while copying prefix and suffix");

    RuntimeType empty;
    empty.words[0x1004] = empty.words[0x1008] = empty.words[0x100c] = 0;
    Check(insert(0x1000, 0, 1, record, empty) == 0 &&
              empty.words[0x1004] == 0x6000 &&
              empty.words[0x1008] == 0x6000 + Stride &&
              empty.records[0x6000].back() == 999,
          "empty growth path returns old null begin");

    RuntimeType no_count;
    Check(insert(0x1000, middle, 0, record, no_count) == 0 &&
              no_count.words[0x1008] == 0x2000 + 3 * Stride &&
              no_count.calls.empty(),
          "zero-count insertion makes no writes");

    RuntimeType failed;
    failed.words[0x100c] = failed.words[0x1008];
    failed.allocation = 0;
    try {
        (void)insert(0x1000, middle, 2, record, failed);
        Check(false, "failed multi-insert allocation should throw");
    } catch (const std::bad_alloc&) {
        Check(failed.words[0x1004] == 0x2000 &&
                  failed.records[middle][0] == 2 &&
                  failed.calls == std::vector<std::string>{
                                      "allocate " +
                                      std::to_string(6 * Stride)},
              "null allocation leaves old multi-insert source intact");
    }
}

void CheckPropertyHelpers() {
    Runtime68 runtime;
    Check(pal4::GameDataAiPropertyValueArray::Size(0x1000, runtime) == 3,
          "68-byte size uses begin/end difference");
    runtime.words[0x1004] = 0;
    Check(pal4::GameDataAiPropertyValueArray::Size(0x1000, runtime) == 0,
          "null property begin reports zero size");
    runtime.words[0x1004] = 0x2000;
    Check(pal4::GameDataAiPropertyValueArray::CopyRecords(
              0x2000, 0x2000 + 3 * 68, 0x8000, runtime) ==
              0x8000 + 3 * 68 &&
              runtime.records[0x8000][0] == 1 &&
              runtime.records[0x8000 + 2 * 68].back() == 102,
          "68-byte copy helper preserves the final word of each record");
    runtime.calls.clear();
    Check(pal4::GameDataAiPropertyValueArray::CopyRecords(
              0x2000, 0x2000 + 3 * 68, 0, runtime) == 3 * 68 &&
              runtime.records[68][0] == 2 &&
              runtime.records[136][0] == 3 &&
              runtime.calls.size() == 2,
          "68-byte null destination skips only the first write");
}

void CheckSingleCopyHelpers() {
    Runtime56 values;
    pal4::GameDataAiValueArray::CopyOne(0x8000, 0x2000 + 56, values);
    Check(values.records[0x8000][0] == 2 &&
              values.records[0x8000].back() == 101 &&
              values.calls.size() == 1,
          "56-byte single copy preserves the last word");
    values.calls.clear();
    pal4::GameDataAiValueArray::CopyOne(0, 0x2000, values);
    Check(values.calls.empty(), "56-byte null destination skips source read");

    Runtime68 properties;
    pal4::GameDataAiPropertyValueArray::CopyOne(
        0x9000, 0x2000 + 68, properties);
    Check(properties.records[0x9000][0] == 2 &&
              properties.records[0x9000].back() == 101 &&
              properties.calls.size() == 1,
          "68-byte single copy preserves the last word");
    properties.calls.clear();
    pal4::GameDataAiPropertyValueArray::CopyOne(0, 0x2000, properties);
    Check(properties.calls.empty(),
          "68-byte null destination skips source read");
}

template <typename RuntimeType, typename Record, std::uint32_t Stride,
          typename Filler>
void CheckFillHelper(Filler fill) {
    Record record{};
    record[0] = 77;
    record.back() = 999;
    RuntimeType runtime;
    Check(fill(0x8000, 3, record, runtime) == 0x8000 + 3 * Stride &&
              runtime.records[0x8000][0] == 77 &&
              runtime.records[0x8000 + Stride].back() == 999 &&
              runtime.records[0x8000 + 2 * Stride][0] == 77 &&
              runtime.calls.size() == 3,
          "fill helper repeats a complete record and returns the end");
    runtime.calls.clear();
    Check(fill(0x8000, 0, record, runtime) == 0 &&
              runtime.calls.empty(),
          "zero count returns zero without writing");
    Check(fill(0, 3, record, runtime) == 3 * Stride &&
              runtime.records[Stride][0] == 77 &&
              runtime.records[2 * Stride][0] == 77 &&
              runtime.calls.size() == 2,
          "null start skips first write then advances normally");
}

}  // namespace

int main() {
    try {
        CheckInsertPaths<Runtime56, pal4::GameDataAiRecord, 56>(
            pal4::GameDataAiValueArray::Insert);
        CheckCopyHelper();
        CheckCopyInsertion<Runtime56, pal4::GameDataAiRecord, 56>(
            pal4::GameDataAiValueArray::InsertCopies);
        CheckFillHelper<Runtime56, pal4::GameDataAiRecord, 56>(
            pal4::GameDataAiValueArray::FillRecords);
        CheckInsertPaths<Runtime68, pal4::GameDataAiPropertyRecord, 68>(
            pal4::GameDataAiPropertyValueArray::Insert);
        CheckPropertyHelpers();
        CheckSingleCopyHelpers();
        CheckCopyInsertion<Runtime68, pal4::GameDataAiPropertyRecord, 68>(
            pal4::GameDataAiPropertyValueArray::InsertCopies);
        CheckFillHelper<Runtime68, pal4::GameDataAiPropertyRecord, 68>(
            pal4::GameDataAiPropertyValueArray::FillRecords);
        std::cout << "game_data_ai_value_array_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
