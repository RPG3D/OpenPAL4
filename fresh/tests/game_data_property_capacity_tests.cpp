#include "pal4/game_data_property_capacity.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool ok, const char* message) {
    if (!ok) throw std::runtime_error(message);
}

struct Runtime final : pal4::IGameDataPropertyCapacityRuntime,
                       pal4::IGameDataPropertyInsertRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::uint32_t> allocations;
    std::vector<std::string> calls;
    std::vector<std::uint32_t> destroyed;
    std::size_t next_allocation = 0;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        calls.push_back("write " + std::to_string(address));
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        calls.push_back("allocate " + std::to_string(bytes));
        return allocations.at(next_allocation++);
    }
    void Free(std::uint32_t address) override {
        calls.push_back("free " + std::to_string(address));
    }
    void CopyRecord(std::uint32_t dst, std::uint32_t src) override {
        calls.push_back("record " + std::to_string(dst) + " " +
                        std::to_string(src));
    }
    void AssignRecord(std::uint32_t dst, std::uint32_t src) override {
        calls.push_back("assign " + std::to_string(dst) + " " +
                        std::to_string(src));
    }
    void DestroyTextRecord(std::uint32_t address) override {
        destroyed.push_back(address);
        calls.push_back("text " + std::to_string(address));
    }
    void CopyIndex(std::uint32_t dst, std::uint32_t src) override {
        calls.push_back("index " + std::to_string(dst) + " " +
                        std::to_string(src));
    }
};

std::size_t At(const Runtime& r, const std::string& call) {
    const auto it = std::find(r.calls.begin(), r.calls.end(), call);
    Check(it != r.calls.end(), "expected operation missing");
    return static_cast<std::size_t>(it - r.calls.begin());
}

Runtime Populated() {
    Runtime r;
    r.words[0x1060] = 0x2000;
    r.words[0x1064] = 0x2000 + 168;
    r.words[0x1068] = 0x2000 + 2 * 168;
    r.words[0x1008] = 0x3000;
    r.words[0x100c] = 0x3008;
    r.words[0x1010] = 0x3018;
    return r;
}

void CheckNoGrowth() {
    auto r = Populated();
    Check(pal4::GameDataPropertyCapacity::Reserve(0x1000, 2, r) == 3 &&
              r.calls.empty(),
          "adequate record and index capacity returns index capacity");
}

void CheckRecordGrowth() {
    auto r = Populated();
    r.words[0x1068] = 0x2000 + 168;
    r.allocations = {0x4000};
    Check(pal4::GameDataPropertyCapacity::Reserve(0x1000, 2, r) == 3 &&
              r.words[0x1060] == 0x4000 &&
              r.words[0x1064] == 0x4000 + 168 &&
              r.words[0x1068] == 0x4000 + 336,
          "record growth preserves live count and extends capacity");
    Check(r.destroyed == std::vector<std::uint32_t>{
              0x2000 + 112, 0x2000 + 96, 0x2000 + 80,
              0x2000 + 64, 0x2000 + 48, 0x2000 + 32,
              0x2000 + 16},
          "old record destroys seven texts in reverse order");
    Check(At(r, "allocate 336") < At(r, "record 16384 8192") &&
              At(r, "record 16384 8192") < At(r, "text 8304") &&
              At(r, "text 8208") < At(r, "free 8192") &&
              At(r, "free 8192") < At(r, "write 4200"),
          "record copy, destruction and pointer replacement order");
}

void CheckBothGrowth() {
    auto r = Populated();
    r.words[0x1010] = 0x3008;
    r.allocations = {0x4000, 0x5000};
    Check(pal4::GameDataPropertyCapacity::Reserve(0x1000, 3, r) ==
              0x5008 && r.words[0x1060] == 0x4000 &&
              r.words[0x1064] == 0x40a8 && r.words[0x1068] == 0x41f8 &&
              r.words[0x1008] == 0x5000 &&
              r.words[0x100c] == 0x5008 && r.words[0x1010] == 0x5018,
          "both arrays grow independently and return new index end");
    Check(At(r, "allocate 504") < At(r, "allocate 24") &&
              At(r, "index 20480 12288") < At(r, "free 12288") &&
              At(r, "free 12288") < At(r, "write 4112"),
          "index is copied before old allocation is released");
}

void CheckEmptyGrowth() {
    Runtime r;
    r.allocations = {0x4000, 0x5000};
    Check(pal4::GameDataPropertyCapacity::Reserve(0x1000, 2, r) ==
              0x5000 && r.words[0x1060] == r.words[0x1064] &&
              r.words[0x1068] == 0x4000 + 336 &&
              r.words[0x1008] == r.words[0x100c] &&
              r.words[0x1010] == 0x5000 + 16 && r.destroyed.empty(),
          "empty arrays acquire capacity without live entries");
}

void CheckInsertEmpty() {
    Runtime r;
    r.allocations = {0x4000};
    Check(pal4::GameDataPropertyRecordArray::Insert(
              0x1000, 0, 0x9000, r) == 0x4000 &&
              r.words[0x1004] == 0x4000 &&
              r.words[0x1008] == 0x4000 + 168 &&
              r.words[0x100c] == 0x4000 + 168 &&
              r.destroyed.empty(),
          "first insert allocates one record and returns new begin");
    Check(At(r, "allocate 168") < At(r, "record 16384 36864") &&
              At(r, "record 16384 36864") < At(r, "free 0") &&
              At(r, "free 0") < At(r, "write 4108"),
          "empty allocation follows construct/free/pointer-update order");
}

void CheckInsertFull() {
    Runtime r;
    r.words[0x1004] = 0x2000;
    r.words[0x1008] = 0x2000 + 2 * 168;
    r.words[0x100c] = r.words[0x1008];
    r.allocations = {0x4000};
    Check(pal4::GameDataPropertyRecordArray::Insert(
              0x1000, 0x2000 + 168, 0x9000, r) == 0x4000 + 168 &&
              r.words[0x1004] == 0x4000 &&
              r.words[0x1008] == 0x4000 + 3 * 168 &&
              r.words[0x100c] == 0x4000 + 4 * 168,
          "full array doubles capacity and returns inserted address");
    Check(At(r, "record 16384 8192") <
              At(r, "record 16552 36864") &&
              At(r, "record 16552 36864") <
              At(r, "record 16720 8360") &&
              At(r, "record 16720 8360") < At(r, "text 8304") &&
              At(r, "text 8376") < At(r, "free 8192") &&
              r.destroyed.size() == 14,
          "prefix, inserted record and suffix copy before old text cleanup");
}

void CheckInsertWithCapacity() {
    Runtime r;
    r.words[0x1004] = 0x2000;
    r.words[0x1008] = 0x2000 + 3 * 168;
    r.words[0x100c] = 0x2000 + 4 * 168;
    Check(pal4::GameDataPropertyRecordArray::Insert(
              0x1000, 0x2000, 0x9000, r) == 0x2000 &&
              r.words[0x1008] == 0x2000 + 4 * 168 &&
              r.destroyed.empty(),
          "spare capacity inserts at begin without reallocating");
    Check(At(r, "record 8696 8528") <
              At(r, "assign 8528 8360") &&
              At(r, "assign 8528 8360") <
              At(r, "assign 8360 8192") &&
              At(r, "assign 8360 8192") <
              At(r, "assign 8192 36864") &&
              At(r, "assign 8192 36864") < At(r, "write 4104"),
          "live records shift backward before source assignment");

    Runtime tail;
    tail.words[0x1004] = 0x2000;
    tail.words[0x1008] = 0x2000 + 168;
    tail.words[0x100c] = 0x2000 + 2 * 168;
    Check(pal4::GameDataPropertyRecordArray::Insert(
              0x1000, 0x2000 + 168, 0x9000, tail) ==
              0x2000 + 168 &&
              At(tail, "record 8360 36864") < At(tail, "write 4104") &&
              std::none_of(tail.calls.begin(), tail.calls.end(),
                           [](const std::string& call) {
                               return call.starts_with("assign ");
                           }),
          "append with capacity copy-constructs without shifting");
}

void CheckSizeAndRangeCopy() {
    Runtime r;
    r.words[0x1004] = 0x2000;
    r.words[0x1008] = 0x2000 + 3 * 168;
    Check(pal4::GameDataPropertyRecordArray::Size(0x1000, r) == 3,
          "size divides live byte span by 168");
    r.words[0x1004] = 0;
    Check(pal4::GameDataPropertyRecordArray::Size(0x1000, r) == 0,
          "null begin yields zero size without reading end");
    r.calls.clear();
    Check(pal4::GameDataPropertyRecordArray::CopyConstructRange(
              0x2000, 0x2000 + 2 * 168, 0x4000, r) ==
              0x4000 + 2 * 168 &&
              r.calls == std::vector<std::string>{
                  "record 16384 8192", "record 16552 8360"},
          "range copy constructs each 168-byte record in order");
    r.calls.clear();
    Check(pal4::GameDataPropertyRecordArray::CopyConstructRange(
              0x2000, 0x2000, 0x4000, r) == 0x4000 &&
              r.calls.empty(),
          "empty range returns the unchanged destination");
    pal4::GameDataPropertyRecordArray::CopyConstructCopies(
        0x4000, 3, 0x9000, r);
    Check(r.calls == std::vector<std::string>{
              "record 16384 36864", "record 16552 36864",
              "record 16720 36864"},
          "repeated copy constructor advances destination by 168");
    r.calls.clear();
    pal4::GameDataPropertyRecordArray::CopyConstructCopies(
        0x4000, 0, 0x9000, r);
    Check(r.calls.empty(), "zero repeated copies perform no construction");
}

void CheckInsertCopiesWithSpareCapacity() {
    Runtime many_tail;
    many_tail.words[0x1004] = 0x2000;
    many_tail.words[0x1008] = 0x2000 + 4 * 168;
    many_tail.words[0x100c] = 0x2000 + 6 * 168;
    Check(pal4::GameDataPropertyRecordArray::InsertCopies(
              0x1000, 0x2000 + 168, 2, 0x9000, many_tail) ==
              0x2000 + 6 * 168 &&
              many_tail.words[0x1008] == 0x2000 + 6 * 168,
          "two copies fit in spare capacity with a longer tail");
    Check(many_tail.calls == std::vector<std::string>{
              "record 8864 8528", "record 9032 8696",
              "assign 8696 8360", "assign 8360 36864",
              "assign 8528 36864", "write 4104"},
          "long tail copies final records then shifts backward and fills gap");

    Runtime short_tail;
    short_tail.words[0x1004] = 0x2000;
    short_tail.words[0x1008] = 0x2000 + 2 * 168;
    short_tail.words[0x100c] = 0x2000 + 5 * 168;
    Check(pal4::GameDataPropertyRecordArray::InsertCopies(
              0x1000, 0x2000 + 168, 3, 0x9000, short_tail) ==
              0x2000 + 5 * 168 &&
              short_tail.words[0x1008] == 0x2000 + 5 * 168,
          "three copies fit with only one tail record");
    Check(short_tail.calls == std::vector<std::string>{
              "record 8864 8360", "record 8528 36864",
              "record 8696 36864", "assign 8360 36864",
              "write 4104"},
          "short tail constructs shifted and extra records before assignment");
}

void CheckInsertCopiesReallocation() {
    Runtime r;
    r.words[0x1004] = 0x2000;
    r.words[0x1008] = 0x2000 + 2 * 168;
    r.words[0x100c] = r.words[0x1008];
    r.allocations = {0x4000};
    Check(pal4::GameDataPropertyRecordArray::InsertCopies(
              0x1000, 0x2000 + 168, 3, 0x9000, r) ==
              0x4000 + 5 * 168 &&
              r.words[0x1004] == 0x4000 &&
              r.words[0x1008] == 0x4000 + 5 * 168 &&
              r.words[0x100c] == 0x4000 + 5 * 168,
          "reallocation grows by maximum of old size and inserted count");
    Check(At(r, "allocate 840") < At(r, "record 16384 8192") &&
              At(r, "record 16384 8192") <
              At(r, "record 16552 36864") &&
              At(r, "record 16888 36864") <
              At(r, "record 17056 8360") &&
              At(r, "record 17056 8360") < At(r, "text 8304") &&
              At(r, "text 8376") < At(r, "free 8192") &&
              r.destroyed.size() == 14,
          "prefix, repeated source, suffix, old text cleanup and free order");

    Runtime zero;
    zero.words[0x1004] = 0x2000;
    zero.words[0x1008] = 0x2000 + 168;
    Check(pal4::GameDataPropertyRecordArray::InsertCopies(
              0x1000, 0x2000, 0, 0x9000, zero) ==
              0x2000 + 168 && zero.calls.empty(),
          "zero inserted records leave storage untouched");
}

}  // namespace

int main() {
    try {
        CheckNoGrowth();
        CheckRecordGrowth();
        CheckBothGrowth();
        CheckEmptyGrowth();
        CheckInsertEmpty();
        CheckInsertFull();
        CheckInsertWithCapacity();
        CheckSizeAndRangeCopy();
        CheckInsertCopiesWithSpareCapacity();
        CheckInsertCopiesReallocation();
        std::cout << "game_data_property_capacity_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
