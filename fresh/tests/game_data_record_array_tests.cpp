#include "pal4/game_data_record_array.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Call {
    std::string name;
    std::uint32_t first;
    std::uint32_t second = 0;
    std::uint32_t bytes = 0;
    bool operator==(const Call&) const = default;
};

struct Runtime final : pal4::IGameDataRecordArrayRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<Call> calls;
    std::uint32_t allocation = 0x3000;
    std::uint32_t copy_calls = 0;
    std::uint32_t fail_copy = 0;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        calls.push_back({"write", address, value});
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        calls.push_back({"allocate", bytes});
        return allocation;
    }
    void Free(std::uint32_t address) override {
        calls.push_back({"free", address});
    }
    void CopyConstruct(std::uint32_t destination,
                       std::uint32_t source,
                       std::uint32_t record_bytes) override {
        calls.push_back({"copy", destination, source, record_bytes});
        if (++copy_calls == fail_copy)
            throw std::runtime_error("copy failed");
    }
    void Assign(std::uint32_t destination,
                std::uint32_t source,
                std::uint32_t record_bytes) override {
        calls.push_back({"assign", destination, source, record_bytes});
    }
    void DestroyRange(std::uint32_t begin, std::uint32_t end,
                      std::uint32_t record_bytes) override {
        calls.push_back({"destroy-range", begin, end, record_bytes});
    }
};

using InsertFunction = std::uint32_t (*)(
    std::uint32_t, std::uint32_t, std::uint32_t,
    pal4::IGameDataRecordArrayRuntime&);
using CopyFunction = std::uint32_t (*)(
    std::uint32_t, std::uint32_t, std::uint32_t,
    pal4::IGameDataRecordArrayRuntime&);
using InsertCopiesFunction = void (*)(
    std::uint32_t, std::uint32_t, std::uint32_t,
    std::uint32_t, pal4::IGameDataRecordArrayRuntime&);
using FillFunction = void (*)(
    std::uint32_t, std::uint32_t, std::uint32_t,
    pal4::IGameDataRecordArrayRuntime&);

struct RecordCase {
    std::uint32_t bytes;
    InsertFunction insert;
    CopyFunction copy;
    InsertCopiesFunction insert_copies;
    FillFunction fill;
};

constexpr std::uint32_t kArray = 0x9000;
constexpr std::uint32_t kSource = 0xa000;

void Configure(Runtime& runtime, std::uint32_t bytes,
               std::uint32_t active, std::uint32_t capacity) {
    runtime.words[kArray + 4] = 0x1000;
    runtime.words[kArray + 8] = 0x1000 + active * bytes;
    runtime.words[kArray + 12] = 0x1000 + capacity * bytes;
}

void CheckGrowth(const RecordCase& kind) {
    const auto bytes = kind.bytes;
    Runtime runtime;
    Configure(runtime, bytes, 2, 2);
    Check(kind.insert(kArray, 0x1000 + bytes, kSource, runtime) ==
              0x3000 + bytes &&
              runtime.calls == std::vector<Call>{
                  {"allocate", 4 * bytes},
                  {"copy", 0x3000, 0x1000, bytes},
                  {"copy", 0x3000 + bytes, kSource, bytes},
                  {"copy", 0x3000 + 2 * bytes,
                   0x1000 + bytes, bytes},
                  {"destroy-range", 0x1000,
                   0x1000 + 2 * bytes, bytes},
                  {"free", 0x1000},
                  {"write", kArray + 12, 0x3000 + 4 * bytes},
                  {"write", kArray + 4, 0x3000},
                  {"write", kArray + 8, 0x3000 + 3 * bytes}},
          "growth copies prefix, inserted record and suffix before cleanup");

    Runtime empty;
    Check(kind.insert(kArray, 0, kSource, empty) == 0x3000 &&
              empty.calls == std::vector<Call>{
                  {"allocate", bytes},
                  {"copy", 0x3000, kSource, bytes},
                  {"destroy-range", 0, 0, bytes},
                  {"free", 0},
                  {"write", kArray + 12, 0x3000 + bytes},
                  {"write", kArray + 4, 0x3000},
                  {"write", kArray + 8, 0x3000 + bytes}},
          "empty array allocates one record slot");
}

void CheckSpare(const RecordCase& kind) {
    const auto bytes = kind.bytes;
    Runtime middle;
    Configure(middle, bytes, 3, 4);
    Check(kind.insert(kArray, 0x1000 + bytes, kSource, middle) ==
              0x1000 + bytes &&
              middle.calls == std::vector<Call>{
                  {"copy", 0x1000 + 3 * bytes,
                   0x1000 + 2 * bytes, bytes},
                  {"assign", 0x1000 + 2 * bytes,
                   0x1000 + bytes, bytes},
                  {"assign", 0x1000 + bytes, kSource, bytes},
                  {"write", kArray + 8, 0x1000 + 4 * bytes}},
          "spare insertion shifts tail backward before assignment");

    Runtime append;
    Configure(append, bytes, 1, 2);
    Check(kind.insert(kArray, 0x1000 + bytes, kSource, append) ==
              0x1000 + bytes &&
              append.calls == std::vector<Call>{
                  {"copy", 0x1000 + bytes, kSource, bytes},
                  {"write", kArray + 8, 0x1000 + 2 * bytes}},
          "spare append copy-constructs directly");
}

void CheckCopyAndFailure(const RecordCase& kind) {
    const auto bytes = kind.bytes;
    Runtime copied;
    Check(kind.copy(0x1000, 0x1000 + 2 * bytes,
                    0x3000, copied) == 0x3000 + 2 * bytes &&
              copied.calls == std::vector<Call>{
                  {"copy", 0x3000, 0x1000, bytes},
                  {"copy", 0x3000 + bytes,
                   0x1000 + bytes, bytes}},
          "range copy advances both ends by record stride");
    copied.calls.clear();
    Check(kind.copy(0x1000, 0x1000, 0x3000, copied) == 0x3000 &&
              copied.calls.empty(),
          "empty range returns its destination unchanged");

    Runtime failed;
    Configure(failed, bytes, 2, 2);
    failed.fail_copy = 2;
    try {
        static_cast<void>(kind.insert(
            kArray, 0x1000 + bytes, kSource, failed));
        Check(false, "copy failure must propagate");
    } catch (const std::runtime_error& error) {
        Check(std::string(error.what()) == "copy failed" &&
                  failed.calls == std::vector<Call>{
                      {"allocate", 4 * bytes},
                      {"copy", 0x3000, 0x1000, bytes},
                      {"copy", 0x3000 + bytes, kSource, bytes}} &&
                  failed.words[kArray + 4] == 0x1000 &&
                  failed.words[kArray + 8] == 0x1000 + 2 * bytes,
              "failed copy does not destroy old storage or update pointers");
    }
}

void CheckRepeatedSpare(const RecordCase& kind) {
    const auto bytes = kind.bytes;
    Runtime long_tail;
    Configure(long_tail, bytes, 3, 5);
    kind.insert_copies(kArray, 0x1000 + bytes, 1, kSource, long_tail);
    Check(long_tail.calls == std::vector<Call>{
              {"copy", 0x1000 + 3 * bytes,
               0x1000 + 2 * bytes, bytes},
              {"assign", 0x1000 + 2 * bytes,
               0x1000 + bytes, bytes},
              {"assign", 0x1000 + bytes, kSource, bytes},
              {"write", kArray + 8, 0x1000 + 4 * bytes}},
          "repeated spare insertion shifts a long tail backward");

    Runtime short_tail;
    Configure(short_tail, bytes, 2, 5);
    kind.insert_copies(kArray, 0x1000 + bytes, 2, kSource,
                       short_tail);
    Check(short_tail.calls == std::vector<Call>{
              {"copy", 0x1000 + 3 * bytes,
               0x1000 + bytes, bytes},
              {"copy", 0x1000 + 2 * bytes, kSource, bytes},
              {"assign", 0x1000 + bytes, kSource, bytes},
              {"write", kArray + 8, 0x1000 + 4 * bytes}},
          "repeated spare insertion handles a tail shorter than count");

    Runtime zero;
    Configure(zero, bytes, 2, 2);
    kind.insert_copies(kArray, 0x1000 + bytes, 0, kSource, zero);
    Check(zero.calls.empty(), "zero copies leave storage untouched");
}

void CheckRepeatedGrowth(const RecordCase& kind) {
    const auto bytes = kind.bytes;
    Runtime runtime;
    Configure(runtime, bytes, 2, 2);
    kind.insert_copies(kArray, 0x1000 + bytes, 3, kSource, runtime);
    Check(runtime.calls == std::vector<Call>{
              {"allocate", 5 * bytes},
              {"copy", 0x3000, 0x1000, bytes},
              {"copy", 0x3000 + bytes, kSource, bytes},
              {"copy", 0x3000 + 2 * bytes, kSource, bytes},
              {"copy", 0x3000 + 3 * bytes, kSource, bytes},
              {"copy", 0x3000 + 4 * bytes,
               0x1000 + bytes, bytes},
              {"destroy-range", 0x1000,
               0x1000 + 2 * bytes, bytes},
              {"free", 0x1000},
              {"write", kArray + 12, 0x3000 + 5 * bytes},
              {"write", kArray + 4, 0x3000},
              {"write", kArray + 8, 0x3000 + 5 * bytes}},
          "growth copies prefix, repeated records and suffix before free");

    Runtime empty;
    kind.insert_copies(kArray, 0, 2, kSource, empty);
    Check(empty.calls == std::vector<Call>{
              {"allocate", 2 * bytes},
              {"copy", 0x3000, kSource, bytes},
              {"copy", 0x3000 + bytes, kSource, bytes},
              {"destroy-range", 0, 0, bytes},
              {"free", 0},
              {"write", kArray + 12, 0x3000 + 2 * bytes},
              {"write", kArray + 4, 0x3000},
              {"write", kArray + 8, 0x3000 + 2 * bytes}},
          "repeated growth initializes empty storage with exact capacity");

    Runtime failed;
    Configure(failed, bytes, 2, 2);
    failed.fail_copy = 2;
    try {
        kind.insert_copies(kArray, 0x1000 + bytes, 2,
                           kSource, failed);
        Check(false, "repeated copy failure must propagate");
    } catch (const std::runtime_error& error) {
        Check(std::string(error.what()) == "copy failed" &&
                  failed.calls == std::vector<Call>{
                      {"allocate", 4 * bytes},
                      {"copy", 0x3000, 0x1000, bytes},
                      {"copy", 0x3000 + bytes, kSource, bytes}} &&
                  failed.words[kArray + 4] == 0x1000,
              "repeated copy failure leaves old storage installed");
    }
}

void CheckFill(const RecordCase& kind) {
    Runtime runtime;
    kind.fill(0x3000, 2, kSource, runtime);
    Check(runtime.calls == std::vector<Call>{
              {"copy", 0x3000, kSource, kind.bytes},
              {"copy", 0x3000 + kind.bytes, kSource, kind.bytes}},
          "fill constructs each record with the correct stride");
    runtime.calls.clear();
    kind.fill(0x3000, 0, kSource, runtime);
    Check(runtime.calls.empty(), "zero-count fill does nothing");
}

void CheckSkillCount() {
    Runtime runtime;
    Configure(runtime, 132, 3, 5);
    Check(pal4::SkillDataRecordArray::Count(kArray, runtime) == 3,
          "Skill count uses 132-byte active range");
    runtime.words[kArray + 8] = 0x1000 - 132;
    Check(pal4::SkillDataRecordArray::Count(kArray, runtime) == -1,
          "Skill count divides signed negative span");
    runtime.words[kArray + 4] = 0;
    Check(pal4::SkillDataRecordArray::Count(kArray, runtime) == 0,
          "null Skill storage has zero count");
}

}  // namespace

int main() {
    try {
        for (const RecordCase kind : {
                 RecordCase{132, pal4::SkillDataRecordArray::Insert,
                            pal4::SkillDataRecordArray::CopyRange,
                            pal4::SkillDataRecordArray::InsertCopies,
                            pal4::SkillDataRecordArray::Fill},
                 RecordCase{208, pal4::StuntDataRecordArray::Insert,
                            pal4::StuntDataRecordArray::CopyRange,
                            pal4::StuntDataRecordArray::InsertCopies,
                            pal4::StuntDataRecordArray::Fill},
                 RecordCase{152, pal4::MagicDataRecordArray::Insert,
                            pal4::MagicDataRecordArray::CopyRange,
                            pal4::MagicDataRecordArray::InsertCopies,
                            pal4::MagicDataRecordArray::Fill}}) {
            CheckGrowth(kind);
            CheckSpare(kind);
            CheckCopyAndFailure(kind);
            CheckRepeatedSpare(kind);
            CheckRepeatedGrowth(kind);
            CheckFill(kind);
        }
        CheckSkillCount();
        std::cout << "game_data_record_array_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
