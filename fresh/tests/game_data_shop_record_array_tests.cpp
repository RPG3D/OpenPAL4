#include "pal4/game_data_shop_record_array.h"

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

struct Runtime final : pal4::IGameDataShopRecordArrayRuntime {
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

constexpr std::uint32_t kArray = 0x9000;
constexpr std::uint32_t kSource = 0xa000;

void Configure(Runtime& runtime, std::uint32_t stride,
               std::uint32_t active, std::uint32_t capacity) {
    runtime.words[kArray + 4] = 0x1000;
    runtime.words[kArray + 8] = 0x1000 + active * stride;
    runtime.words[kArray + 12] = 0x1000 + capacity * stride;
}

void CheckPropertyGrowth() {
    Runtime runtime;
    Configure(runtime, 56, 2, 2);
    Check(pal4::GameDataShopRecordArray::InsertProperty(
              kArray, 0x1000 + 56, kSource, runtime) == 0x3000 + 56 &&
              runtime.calls == std::vector<Call>{
                  {"allocate", 4 * 56},
                  {"copy", 0x3000, 0x1000, 56},
                  {"copy", 0x3000 + 56, kSource, 56},
                  {"copy", 0x3000 + 112, 0x1000 + 56, 56},
                  {"destroy-range", 0x1000, 0x1000 + 112, 56},
                  {"free", 0x1000},
                  {"write", kArray + 12, 0x3000 + 224},
                  {"write", kArray + 4, 0x3000},
                  {"write", kArray + 8, 0x3000 + 168}},
          "Property growth copies prefix, insertion, suffix before cleanup");
    Check(runtime.words[kArray + 4] == 0x3000 &&
              runtime.words[kArray + 8] == 0x3000 + 168 &&
              runtime.words[kArray + 12] == 0x3000 + 224,
          "Property growth retains three active records and four slots");

    Runtime empty;
    empty.words[kArray + 4] = 0;
    empty.words[kArray + 8] = 0;
    empty.words[kArray + 12] = 0;
    Check(pal4::GameDataShopRecordArray::InsertProperty(
              kArray, 0, kSource, empty) == 0x3000 &&
              empty.calls == std::vector<Call>{
                  {"allocate", 56}, {"copy", 0x3000, kSource, 56},
                  {"destroy-range", 0, 0, 56}, {"free", 0},
                  {"write", kArray + 12, 0x3000 + 56},
                  {"write", kArray + 4, 0x3000},
                  {"write", kArray + 8, 0x3000 + 56}},
          "empty Property array grows to one record");
}

void CheckPropertySpare() {
    Runtime runtime;
    Configure(runtime, 56, 3, 4);
    Check(pal4::GameDataShopRecordArray::InsertProperty(
              kArray, 0x1000 + 56, kSource, runtime) == 0x1000 + 56 &&
              runtime.calls == std::vector<Call>{
                  {"copy", 0x1000 + 168, 0x1000 + 112, 56},
                  {"assign", 0x1000 + 112, 0x1000 + 56, 56},
                  {"assign", 0x1000 + 56, kSource, 56},
                  {"write", kArray + 8, 0x1000 + 224}},
          "spare Property insertion moves tail backward before assignment");

    Runtime append;
    Configure(append, 56, 1, 2);
    Check(pal4::GameDataShopRecordArray::InsertProperty(
              kArray, 0x1000 + 56, kSource, append) == 0x1000 + 56 &&
              append.calls == std::vector<Call>{
                  {"copy", 0x1000 + 56, kSource, 56},
                  {"write", kArray + 8, 0x1000 + 112}},
          "append into spare Property slot copy-constructs directly");
}

void CheckGoodsBranches() {
    Runtime growth;
    Configure(growth, 36, 2, 2);
    Check(pal4::GameDataShopRecordArray::InsertGoods(
              kArray, 0x1000 + 36, kSource, growth) == 0x3000 + 36 &&
              growth.calls == std::vector<Call>{
                  {"allocate", 4 * 36},
                  {"copy", 0x3000, 0x1000, 36},
                  {"copy", 0x3000 + 36, kSource, 36},
                  {"copy", 0x3000 + 72, 0x1000 + 36, 36},
                  {"destroy-range", 0x1000, 0x1000 + 72, 36},
                  {"free", 0x1000},
                  {"write", kArray + 12, 0x3000 + 144},
                  {"write", kArray + 4, 0x3000},
                  {"write", kArray + 8, 0x3000 + 108}},
          "Good growth uses 36-byte copy and cleanup strides");

    Runtime spare;
    Configure(spare, 36, 2, 3);
    Check(pal4::GameDataShopRecordArray::InsertGoods(
              kArray, 0x1000 + 36, kSource, spare) == 0x1000 + 36 &&
              spare.calls == std::vector<Call>{
                  {"copy", 0x1000 + 72, 0x1000 + 36, 36},
                  {"assign", 0x1000 + 36, kSource, 36},
                  {"write", kArray + 8, 0x1000 + 108}},
          "Good insertion into spare slot assigns displaced tail");
}

void CheckSizesRangesAndFailure() {
    Runtime runtime;
    Configure(runtime, 56, 2, 3);
    Check(pal4::GameDataShopRecordArray::SizeProperty(
              kArray, runtime) == 2,
          "Property size uses 56-byte active range");
    runtime.words[kArray + 4] = 0;
    Check(pal4::GameDataShopRecordArray::SizeProperty(
              kArray, runtime) == 0,
          "null Property storage has zero size");
    Check(pal4::GameDataShopRecordArray::CopyPropertyRange(
              0x1000, 0x1000 + 112, 0x3000, runtime) == 0x3000 + 112 &&
              runtime.calls == std::vector<Call>{
                  {"copy", 0x3000, 0x1000, 56},
                  {"copy", 0x3000 + 56, 0x1000 + 56, 56}},
          "Property range copy advances source and destination by 56");
    runtime.calls.clear();
    Check(pal4::GameDataShopRecordArray::CopyGoodsRange(
              0x1000, 0x1000 + 72, 0x4000, runtime) == 0x4000 + 72 &&
              runtime.calls == std::vector<Call>{
                  {"copy", 0x4000, 0x1000, 36},
                  {"copy", 0x4000 + 36, 0x1000 + 36, 36}},
          "Good range copy advances by 36");
    runtime.calls.clear();
    Check(pal4::GameDataShopRecordArray::CopyGoodsRange(
              0x1000, 0x1000, 0x4000, runtime) == 0x4000 &&
              runtime.calls.empty(),
          "empty copy range returns original destination");

    Runtime failed;
    Configure(failed, 56, 2, 2);
    failed.fail_copy = 2;
    try {
        static_cast<void>(pal4::GameDataShopRecordArray::InsertProperty(
            kArray, 0x1000 + 56, kSource, failed));
        Check(false, "copy failure must propagate");
    } catch (const std::runtime_error& error) {
        Check(std::string(error.what()) == "copy failed" &&
                  failed.calls.size() == 3 &&
                  failed.words[kArray + 4] == 0x1000 &&
                  failed.words[kArray + 8] == 0x1000 + 112,
              "copy failure stops before old storage destruction");
    }
}

}  // namespace

int main() {
    try {
        CheckPropertyGrowth();
        CheckPropertySpare();
        CheckGoodsBranches();
        CheckSizesRangesAndFailure();
        std::cout << "game_data_shop_record_array_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
