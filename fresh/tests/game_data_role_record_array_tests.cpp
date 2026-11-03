#include "pal4/game_data_role_record_array.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
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

struct Runtime final : pal4::IGameDataRoleRecordArrayRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<Call> calls;
    std::uint32_t allocation = 0xa000;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
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
                       std::uint32_t source) override {
        calls.push_back({"copy", destination, source});
    }
    void FillConstruct(std::uint32_t destination,
                       std::uint32_t count,
                       std::uint32_t source) override {
        calls.push_back({"fill", destination, count, source});
    }
    void AssignBackward(std::uint32_t begin,
                        std::uint32_t source_end,
                        std::uint32_t destination_end) override {
        calls.push_back({"backward", begin, source_end, destination_end});
    }
    void AssignRange(std::uint32_t begin, std::uint32_t end,
                     std::uint32_t source) override {
        calls.push_back({"assign", begin, end, source});
    }
    void DestroyRange(std::uint32_t begin,
                      std::uint32_t end) override {
        calls.push_back({"destroy", begin, end});
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

void CheckCountAndRange() {
    auto empty = WithArray(0, 0, 0);
    Check(pal4::GameDataRoleRecordArray::Count(kArray, empty) == 0,
          "null Role record array has zero count");
    auto runtime = WithArray(0x4000, 0x4280, 0x43c0);
    Check(pal4::GameDataRoleRecordArray::Count(kArray, runtime) == 2,
          "Role count divides live bytes by 320");
    Check(pal4::GameDataRoleRecordArray::CopyConstructRange(
              0x4000, 0x4280, 0x8000, runtime) == 0x8280 &&
              runtime.calls == std::vector<Call>{
                  {"copy", 0x8000, 0x4000},
                  {"copy", 0x8140, 0x4140}},
          "range copy constructs in ascending 320-byte steps");
    runtime.calls.clear();
    Check(pal4::GameDataRoleRecordArray::CopyConstructRange(
              0x4140, 0x4140, 0x8000, runtime) == 0x8000 &&
              runtime.calls.empty(),
          "empty copy range returns unchanged destination");
}

void CheckInPlace() {
    auto append = WithArray(0x4000, 0x4140, 0x4280);
    Check(pal4::GameDataRoleRecordArray::InsertOne(
              kArray, 0x4140, kSource, append) == 0x4140 &&
              append.words[kArray + 8] == 0x4280 &&
              append.calls == std::vector<Call>{
                  {"fill", 0x4140, 1, kSource},
                  {"assign", 0x4140, 0x4140, kSource},
                  {"write", kArray + 8, 0x4280}},
          "spare-capacity append constructs one record at old end");

    auto middle = WithArray(0x4000, 0x4280, 0x43c0);
    Check(pal4::GameDataRoleRecordArray::InsertOne(
              kArray, 0x4140, kSource, middle) == 0x4140 &&
              middle.words[kArray + 8] == 0x43c0 &&
              middle.calls == std::vector<Call>{
                  {"copy", 0x4280, 0x4140},
                  {"backward", 0x4140, 0x4140, 0x4280},
                  {"assign", 0x4140, 0x4280, kSource},
                  {"write", kArray + 8, 0x43c0}},
          "spare-capacity middle insert copies tail, shifts and assigns");
}

void CheckReallocation() {
    auto full = WithArray(0x4000, 0x4280, 0x4280);
    Check(pal4::GameDataRoleRecordArray::InsertOne(
              kArray, 0x4140, kSource, full) == 0xa140 &&
              full.words[kArray + 4] == 0xa000 &&
              full.words[kArray + 8] == 0xa3c0 &&
              full.words[kArray + 12] == 0xa500 &&
              full.calls == std::vector<Call>{
                  {"allocate", 1280},
                  {"copy", 0xa000, 0x4000},
                  {"copy", 0xa140, kSource},
                  {"copy", 0xa280, 0x4140},
                  {"destroy", 0x4000, 0x4280},
                  {"free", 0x4000},
                  {"write", kArray + 12, 0xa500},
                  {"write", kArray + 4, 0xa000},
                  {"write", kArray + 8, 0xa3c0}},
          "full array doubles capacity and deep-copies around insertion");

    auto empty = WithArray(0, 0, 0);
    Check(pal4::GameDataRoleRecordArray::InsertOne(
              kArray, 0, kSource, empty) == 0xa000 &&
              empty.words[kArray + 4] == 0xa000 &&
              empty.words[kArray + 8] == 0xa140 &&
              empty.words[kArray + 12] == 0xa140 &&
              empty.calls == std::vector<Call>{
                  {"allocate", 320},
                  {"copy", 0xa000, kSource},
                  {"destroy", 0, 0},
                  {"free", 0},
                  {"write", kArray + 12, 0xa140},
                  {"write", kArray + 4, 0xa000},
                  {"write", kArray + 8, 0xa140}},
          "empty insertion allocates one record and returns new begin");
}

void CheckBulkInPlace() {
    auto zero = WithArray(0x4000, 0x4280, 0x4280);
    pal4::GameDataRoleRecordArray::InsertCopies(
        kArray, 0x4140, 0, kSource, zero);
    Check(zero.calls.empty() && zero.words[kArray + 8] == 0x4280,
          "zero-count bulk insert does no work even in a full array");

    auto long_tail = WithArray(0x4000, 0x4500, 0x4780);
    pal4::GameDataRoleRecordArray::InsertCopies(
        kArray, 0x4140, 2, kSource, long_tail);
    Check(long_tail.words[kArray + 8] == 0x4780 &&
              long_tail.calls == std::vector<Call>{
                  {"copy", 0x4500, 0x4280},
                  {"copy", 0x4640, 0x43c0},
                  {"backward", 0x4140, 0x4280, 0x4500},
                  {"assign", 0x4140, 0x43c0, kSource},
                  {"write", kArray + 8, 0x4780}},
          "bulk insert copies trailing records before backward assignment");

    auto exact_tail = WithArray(0x4000, 0x4280, 0x4500);
    pal4::GameDataRoleRecordArray::InsertCopies(
        kArray, 0x4140, 1, kSource, exact_tail);
    Check(exact_tail.calls == std::vector<Call>{
              {"copy", 0x4280, 0x4140},
              {"assign", 0x4140, 0x4280, kSource},
              {"write", kArray + 8, 0x43c0}},
          "equal tail and insertion counts skip empty backward move");

    auto short_tail = WithArray(0x4000, 0x43c0, 0x4780);
    pal4::GameDataRoleRecordArray::InsertCopies(
        kArray, 0x4280, 2, kSource, short_tail);
    Check(short_tail.words[kArray + 8] == 0x4640 &&
              short_tail.calls == std::vector<Call>{
                  {"copy", 0x4500, 0x4280},
                  {"fill", 0x43c0, 1, kSource},
                  {"assign", 0x4280, 0x43c0, kSource},
                  {"write", kArray + 8, 0x4640}},
          "short tail is constructed after insertion and remaining slots filled");
}

void CheckBulkGrowth() {
    auto full = WithArray(0x4000, 0x4280, 0x4280);
    pal4::GameDataRoleRecordArray::InsertCopies(
        kArray, 0x4140, 3, kSource, full);
    Check(full.words[kArray + 4] == 0xa000 &&
              full.words[kArray + 8] == 0xa640 &&
              full.words[kArray + 12] == 0xa640 &&
              full.calls == std::vector<Call>{
                  {"allocate", 1600},
                  {"copy", 0xa000, 0x4000},
                  {"fill", 0xa140, 3, kSource},
                  {"copy", 0xa500, 0x4140},
                  {"destroy", 0x4000, 0x4280},
                  {"free", 0x4000},
                  {"write", kArray + 12, 0xa640},
                  {"write", kArray + 4, 0xa000},
                  {"write", kArray + 8, 0xa640}},
          "bulk growth allocates max(old count, insert count) extra slots");

    auto empty = WithArray(0, 0, 0);
    pal4::GameDataRoleRecordArray::InsertCopies(
        kArray, 0, 2, kSource, empty);
    Check(empty.words[kArray + 4] == 0xa000 &&
              empty.words[kArray + 8] == 0xa280 &&
              empty.words[kArray + 12] == 0xa280 &&
              empty.calls == std::vector<Call>{
                  {"allocate", 640},
                  {"fill", 0xa000, 2, kSource},
                  {"destroy", 0, 0},
                  {"free", 0},
                  {"write", kArray + 12, 0xa280},
                  {"write", kArray + 4, 0xa000},
                  {"write", kArray + 8, 0xa280}},
          "empty bulk insert allocates exactly the requested record count");
}

void CheckFillConstructRange() {
    Runtime runtime;
    pal4::GameDataRoleRecordArray::FillConstructRange(
        0x8000, 3, kSource, runtime);
    Check(runtime.calls == std::vector<Call>{
              {"copy", 0x8000, kSource},
              {"copy", 0x8140, kSource},
              {"copy", 0x8280, kSource}},
          "Role repeated constructor advances in 320-byte steps");
    runtime.calls.clear();
    pal4::GameDataRoleRecordArray::FillConstructRange(
        0x8000, 0, kSource, runtime);
    Check(runtime.calls.empty(),
          "zero Role repeated-construction count does no work");
}

}  // namespace

int main() {
    try {
        CheckCountAndRange();
        CheckInPlace();
        CheckReallocation();
        CheckBulkInPlace();
        CheckBulkGrowth();
        CheckFillConstructRange();
        std::cout << "game_data_role_record_array_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
