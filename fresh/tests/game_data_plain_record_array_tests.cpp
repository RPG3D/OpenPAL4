#include "pal4/game_data_plain_record_array.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace {

constexpr std::uint32_t kArray = 0x2000;
constexpr std::uint32_t kSource = 0x7000;

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Call {
    std::string name;
    std::uint32_t a = 0;
    std::uint32_t b = 0;
    std::uint32_t c = 0;
    bool operator==(const Call&) const = default;
};

struct Runtime final : pal4::IGameDataPlainRecordArrayRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<Call> calls;
    std::uint32_t allocation = 0xa000;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        calls.push_back({"write", address, value});
    }
    std::uint32_t Allocate(std::uint32_t size) override {
        calls.push_back({"allocate", size});
        return allocation;
    }
    void Free(std::uint32_t address) override {
        calls.push_back({"free", address});
    }
    void CopyRecord(std::uint32_t destination, std::uint32_t source,
                    std::uint32_t size) override {
        calls.push_back({"copy", destination, source, size});
        for (std::uint32_t i = 0; i < size; ++i)
            bytes[destination + i] = bytes[source + i];
    }

    void Fill(std::uint32_t address, std::uint32_t size,
              std::uint8_t seed) {
        for (std::uint32_t i = 0; i < size; ++i)
            bytes[address + i] = static_cast<std::uint8_t>(seed + i);
    }
    void CheckRecord(std::uint32_t address, std::uint32_t size,
                     std::uint8_t seed) {
        for (std::uint32_t i = 0; i < size; ++i)
            Check(bytes[address + i] ==
                      static_cast<std::uint8_t>(seed + i),
                  "full record copy preserves every byte");
    }
};

Runtime WithArray(std::uint32_t begin, std::uint32_t end,
                  std::uint32_t capacity) {
    Runtime runtime;
    runtime.words[kArray + 4] = begin;
    runtime.words[kArray + 8] = end;
    runtime.words[kArray + 12] = capacity;
    return runtime;
}

template <typename Array>
void CheckArray() {
    constexpr auto stride = Array::kRecordBytes;
    auto null_array = WithArray(0, 0, 0);
    Check(Array::Count(kArray, null_array) == 0,
          "null array has zero count");
    auto range = WithArray(0x4000, 0x4000 + 2 * stride,
                           0x4000 + 3 * stride);
    range.Fill(0x4000, stride, 10);
    range.Fill(0x4000 + stride, stride, 20);
    Check(Array::Count(kArray, range) == 2 &&
              Array::CopyRange(0x4000, 0x4000 + 2 * stride,
                               0x8000, range) == 0x8000 + 2 * stride &&
              range.calls == std::vector<Call>{
                  {"copy", 0x8000, 0x4000, stride},
                  {"copy", 0x8000 + stride, 0x4000 + stride, stride}},
          "count and half-open range copy use the native stride");
    range.CheckRecord(0x8000, stride, 10);
    range.CheckRecord(0x8000 + stride, stride, 20);
    range.calls.clear();
    Check(Array::CopyRange(0x4000, 0x4000, 0x8000, range) == 0x8000 &&
              range.calls.empty(),
          "empty range leaves destination unchanged");

    auto append = WithArray(0x4000, 0x4000 + stride,
                            0x4000 + 2 * stride);
    append.Fill(kSource, stride, 30);
    Check(Array::InsertOne(kArray, 0x4000 + stride,
                           kSource, append) == 0x4000 + stride &&
              append.words[kArray + 8] == 0x4000 + 2 * stride &&
              append.calls == std::vector<Call>{
                  {"copy", 0x4000 + stride, kSource, stride},
                  {"write", kArray + 8, 0x4000 + 2 * stride}},
          "spare-capacity append copies source at old end");
    append.CheckRecord(0x4000 + stride, stride, 30);

    auto middle = WithArray(0x4000, 0x4000 + 3 * stride,
                            0x4000 + 4 * stride);
    middle.Fill(0x4000, stride, 10);
    middle.Fill(0x4000 + stride, stride, 20);
    middle.Fill(0x4000 + 2 * stride, stride, 30);
    middle.Fill(kSource, stride, 40);
    Check(Array::InsertOne(kArray, 0x4000 + stride,
                           kSource, middle) == 0x4000 + stride &&
              middle.words[kArray + 8] == 0x4000 + 4 * stride &&
              middle.calls == std::vector<Call>{
                  {"copy", 0x4000 + 3 * stride,
                   0x4000 + 2 * stride, stride},
                  {"copy", 0x4000 + 2 * stride,
                   0x4000 + stride, stride},
                  {"copy", 0x4000 + stride, kSource, stride},
                  {"write", kArray + 8, 0x4000 + 4 * stride}},
          "middle insert shifts existing records backward");
    middle.CheckRecord(0x4000, stride, 10);
    middle.CheckRecord(0x4000 + stride, stride, 40);
    middle.CheckRecord(0x4000 + 2 * stride, stride, 20);
    middle.CheckRecord(0x4000 + 3 * stride, stride, 30);

    auto full = WithArray(0x4000, 0x4000 + 2 * stride,
                          0x4000 + 2 * stride);
    full.Fill(0x4000, stride, 10);
    full.Fill(0x4000 + stride, stride, 20);
    full.Fill(kSource, stride, 30);
    Check(Array::InsertOne(kArray, 0x4000 + stride,
                           kSource, full) == 0xa000 + stride &&
              full.words[kArray + 4] == 0xa000 &&
              full.words[kArray + 8] == 0xa000 + 3 * stride &&
              full.words[kArray + 12] == 0xa000 + 4 * stride &&
              full.calls == std::vector<Call>{
                  {"allocate", 4 * stride},
                  {"copy", 0xa000, 0x4000, stride},
                  {"copy", 0xa000 + stride, kSource, stride},
                  {"copy", 0xa000 + 2 * stride,
                   0x4000 + stride, stride},
                  {"free", 0x4000},
                  {"write", kArray + 12, 0xa000 + 4 * stride},
                  {"write", kArray + 4, 0xa000},
                  {"write", kArray + 8, 0xa000 + 3 * stride}},
          "full array doubles capacity and returns relocated insertion");
    full.CheckRecord(0xa000, stride, 10);
    full.CheckRecord(0xa000 + stride, stride, 30);
    full.CheckRecord(0xa000 + 2 * stride, stride, 20);

    auto empty = WithArray(0, 0, 0);
    empty.Fill(kSource, stride, 50);
    Check(Array::InsertOne(kArray, 0, kSource, empty) == 0xa000 &&
              empty.words[kArray + 8] == 0xa000 + stride &&
              empty.words[kArray + 12] == 0xa000 + stride &&
              empty.calls == std::vector<Call>{
                  {"allocate", stride},
                  {"copy", 0xa000, kSource, stride},
                  {"free", 0},
                  {"write", kArray + 12, 0xa000 + stride},
                  {"write", kArray + 4, 0xa000},
                  {"write", kArray + 8, 0xa000 + stride}},
          "empty array allocates one record");
    empty.CheckRecord(0xa000, stride, 50);
}

template <typename Array>
void CheckBulk() {
    constexpr auto stride = Array::kRecordBytes;
    constexpr bool upgrade =
        std::is_same_v<Array, pal4::GameDataUpgradeRecordArray>;

    auto zero = WithArray(0x4000, 0x4000 + 2 * stride,
                          0x4000 + 2 * stride);
    Check(Array::InsertCopies(kArray, 0x4000 + stride, 0,
                              kSource, zero) ==
              (upgrade ? 0u : 0x4000 + 2 * stride) &&
              zero.calls.empty(),
          "zero bulk count retains array and branch-specific return");

    auto long_tail = WithArray(0x4000, 0x4000 + 4 * stride,
                               0x4000 + 6 * stride);
    for (std::uint32_t i = 0; i < 4; ++i)
        long_tail.Fill(0x4000 + i * stride, stride,
                       static_cast<std::uint8_t>(10 + 10 * i));
    long_tail.Fill(kSource, stride, 60);
    Check(Array::InsertCopies(kArray, 0x4000 + stride, 2,
                              kSource, long_tail) ==
              (upgrade ? 0x4000 + 3 * stride
                       : 0x4000 + 6 * stride) &&
              long_tail.words[kArray + 8] == 0x4000 + 6 * stride &&
              long_tail.calls == std::vector<Call>{
                  {"copy", 0x4000 + 4 * stride,
                   0x4000 + 2 * stride, stride},
                  {"copy", 0x4000 + 5 * stride,
                   0x4000 + 3 * stride, stride},
                  {"copy", 0x4000 + 3 * stride,
                   0x4000 + stride, stride},
                  {"copy", 0x4000 + stride, kSource, stride},
                  {"copy", 0x4000 + 2 * stride, kSource, stride},
                  {"write", kArray + 8, 0x4000 + 6 * stride}},
          "long-tail bulk insert copies tail, shifts backward and fills gap");
    for (std::uint32_t i = 1; i <= 2; ++i)
        long_tail.CheckRecord(0x4000 + i * stride, stride, 60);
    long_tail.CheckRecord(0x4000 + 3 * stride, stride, 20);
    long_tail.CheckRecord(0x4000 + 4 * stride, stride, 30);
    long_tail.CheckRecord(0x4000 + 5 * stride, stride, 40);

    auto short_tail = WithArray(0x4000, 0x4000 + 3 * stride,
                                0x4000 + 6 * stride);
    short_tail.Fill(0x4000 + 2 * stride, stride, 30);
    short_tail.Fill(kSource, stride, 50);
    Check(Array::InsertCopies(kArray, 0x4000 + 2 * stride, 2,
                              kSource, short_tail) ==
              (upgrade ? 0x4000 + 5 * stride : 2 * stride) &&
              short_tail.words[kArray + 8] == 0x4000 + 5 * stride &&
              short_tail.calls == std::vector<Call>{
                  {"copy", 0x4000 + 4 * stride,
                   0x4000 + 2 * stride, stride},
                  {"copy", 0x4000 + 3 * stride, kSource, stride},
                  {"copy", 0x4000 + 2 * stride, kSource, stride},
                  {"write", kArray + 8, 0x4000 + 5 * stride}},
          "short-tail bulk insert moves tail then fills new and old slots");
    short_tail.CheckRecord(0x4000 + 2 * stride, stride, 50);
    short_tail.CheckRecord(0x4000 + 3 * stride, stride, 50);
    short_tail.CheckRecord(0x4000 + 4 * stride, stride, 30);

    auto full = WithArray(0x4000, 0x4000 + 2 * stride,
                          0x4000 + 2 * stride);
    full.Fill(0x4000, stride, 10);
    full.Fill(0x4000 + stride, stride, 20);
    full.Fill(kSource, stride, 70);
    const auto growth_return = upgrade
        ? static_cast<std::uint32_t>(0x66666667ull * (2 * stride))
        : 0xa000 + 5 * stride;
    Check(Array::InsertCopies(kArray, 0x4000 + stride, 3,
                              kSource, full) == growth_return &&
              full.words[kArray + 4] == 0xa000 &&
              full.words[kArray + 8] == 0xa000 + 5 * stride &&
              full.words[kArray + 12] == 0xa000 + 5 * stride &&
              full.calls == std::vector<Call>{
                  {"allocate", 5 * stride},
                  {"copy", 0xa000, 0x4000, stride},
                  {"copy", 0xa000 + stride, kSource, stride},
                  {"copy", 0xa000 + 2 * stride, kSource, stride},
                  {"copy", 0xa000 + 3 * stride, kSource, stride},
                  {"copy", 0xa000 + 4 * stride,
                   0x4000 + stride, stride},
                  {"free", 0x4000},
                  {"write", kArray + 12, 0xa000 + 5 * stride},
                  {"write", kArray + 4, 0xa000},
                  {"write", kArray + 8, 0xa000 + 5 * stride}},
          "bulk reallocation preserves prefix, repeated source and suffix");
    full.CheckRecord(0xa000, stride, 10);
    full.CheckRecord(0xa000 + 4 * stride, stride, 20);
    for (std::uint32_t i = 1; i <= 3; ++i)
        full.CheckRecord(0xa000 + i * stride, stride, 70);

    auto empty = WithArray(0, 0, 0);
    empty.Fill(kSource, stride, 90);
    Check(Array::InsertCopies(kArray, 0, 2, kSource, empty) ==
              (upgrade ? 0u : 0xa000 + 2 * stride) &&
              empty.words[kArray + 4] == 0xa000 &&
              empty.words[kArray + 8] == 0xa000 + 2 * stride &&
              empty.words[kArray + 12] == 0xa000 + 2 * stride &&
              empty.calls == std::vector<Call>{
                  {"allocate", 2 * stride},
                  {"copy", 0xa000, kSource, stride},
                  {"copy", 0xa000 + stride, kSource, stride},
                  {"free", 0},
                  {"write", kArray + 12, 0xa000 + 2 * stride},
                  {"write", kArray + 4, 0xa000},
                  {"write", kArray + 8, 0xa000 + 2 * stride}},
          "empty bulk insertion allocates exactly two records");
    empty.CheckRecord(0xa000, stride, 90);
    empty.CheckRecord(0xa000 + stride, stride, 90);
}

template <typename Array>
void CheckFillRange() {
    constexpr auto stride = Array::kRecordBytes;
    Runtime runtime;
    runtime.Fill(kSource, stride, 31);
    Check(Array::FillRange(0x8000, 3, kSource, runtime) ==
              0x8000 + 3 * stride &&
              runtime.calls == std::vector<Call>{
                  {"copy", 0x8000, kSource, stride},
                  {"copy", 0x8000 + stride, kSource, stride},
                  {"copy", 0x8000 + 2 * stride, kSource, stride}},
          "plain fill copies each full record and returns destination end");
    for (std::uint32_t i = 0; i < 3; ++i)
        runtime.CheckRecord(0x8000 + i * stride, stride, 31);
    runtime.calls.clear();
    Check(Array::FillRange(0x8000, 0, kSource, runtime) == 0 &&
              runtime.calls.empty(),
          "zero plain fill count returns zero without copying");
    Check(Array::FillRange(0, 2, kSource, runtime) ==
              2 * stride &&
              runtime.calls == std::vector<Call>{
                  {"copy", stride, kSource, stride}},
          "null first destination skips its copy but advances the cursor");
    runtime.CheckRecord(stride, stride, 31);
    runtime.calls.clear();
    Array::CopyOne(0x9000, kSource, runtime);
    Check(runtime.calls == std::vector<Call>{
              {"copy", 0x9000, kSource, stride}},
          "single-record helper copies the table-specific byte count");
    runtime.CheckRecord(0x9000, stride, 31);
    runtime.calls.clear();
    Array::CopyOne(0, kSource, runtime);
    Check(runtime.calls.empty(),
          "single-record helper skips null destination");
}

template <typename Array>
void CheckUniqueCopy() {
    constexpr auto stride = Array::kRecordBytes;
    constexpr std::uint32_t begin = 0x4000;
    constexpr std::uint32_t output = 0x8000;
    Runtime runtime;
    const std::uint8_t keys[] = {1, 1, 2, 2, 1};
    for (std::uint32_t i = 0; i < 5; ++i) {
        runtime.Fill(begin + i * stride, stride,
                     static_cast<std::uint8_t>(30 + i));
        runtime.bytes[begin + i * stride] = keys[i];
    }
    std::vector<std::pair<std::uint32_t, std::uint32_t>> comparisons;
    const auto equivalent = [&](std::uint32_t left, std::uint32_t right) {
        comparisons.emplace_back(left, right);
        return runtime.bytes[left] == runtime.bytes[right];
    };
    Check(Array::UniqueCopy(begin, begin + 5 * stride, output,
                            equivalent, runtime) == output + 3 * stride,
          "plain unique copy returns the compacted end");
    Check(runtime.calls == std::vector<Call>{
              {"copy", output, begin, stride},
              {"copy", output + stride, begin + 2 * stride, stride},
              {"copy", output + 2 * stride, begin + 4 * stride, stride}},
          "plain unique copy copies only retained complete records");
    Check(comparisons ==
              std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                  {begin, begin + stride},
                  {begin, begin + 2 * stride},
                  {begin + 2 * stride, begin + 3 * stride},
                  {begin + 2 * stride, begin + 4 * stride}},
          "plain unique copy compares against last retained source");
    for (std::uint32_t j = 1; j < stride; ++j) {
        Check(runtime.bytes[output + j] ==
                  static_cast<std::uint8_t>(30 + j) &&
                  runtime.bytes[output + stride + j] ==
                      static_cast<std::uint8_t>(32 + j) &&
                  runtime.bytes[output + 2 * stride + j] ==
                      static_cast<std::uint8_t>(34 + j),
              "plain unique copy preserves every retained byte");
    }
    runtime.calls.clear();
    comparisons.clear();
    Check(Array::UniqueCopy(begin, begin, output, equivalent, runtime) ==
              output && runtime.calls.empty() && comparisons.empty(),
          "empty plain unique copy is inert");
    Check(Array::UniqueCopy(begin, begin + stride, output,
                            equivalent, runtime) == output + stride &&
              runtime.calls == std::vector<Call>{
                  {"copy", output, begin, stride}} &&
              comparisons.empty(),
          "singleton plain unique copy never compares");
    runtime.calls.clear();
    try {
        const auto fail = [](std::uint32_t, std::uint32_t) -> bool {
            throw std::runtime_error("comparison failed");
        };
        static_cast<void>(Array::UniqueCopy(begin, begin + 2 * stride,
                                            output, fail, runtime));
        Check(false, "plain comparison failure must propagate");
    } catch (const std::runtime_error& error) {
        Check(std::string(error.what()) == "comparison failed" &&
                  runtime.calls == std::vector<Call>{
                      {"copy", output, begin, stride}},
              "failed plain comparison stops before second copy");
    }
}

}  // namespace

int main() {
    try {
        CheckArray<pal4::GameDataUpgradeRecordArray>();
        CheckArray<pal4::GameDataCombatPrizeRecordArray>();
        CheckBulk<pal4::GameDataUpgradeRecordArray>();
        CheckBulk<pal4::GameDataCombatPrizeRecordArray>();
        CheckFillRange<pal4::GameDataUpgradeRecordArray>();
        CheckFillRange<pal4::GameDataCombatPrizeRecordArray>();
        CheckUniqueCopy<pal4::GameDataUpgradeRecordArray>();
        CheckUniqueCopy<pal4::GameDataCombatPrizeRecordArray>();
        std::cout << "game_data_plain_record_array_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
