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
        return 0x3000;
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

void CheckPropertySpare() {
    Runtime long_tail;
    Configure(long_tail, 56, 3, 5);
    pal4::GameDataShopRecordArray::InsertPropertyCopies(
        kArray, 0x1000 + 56, 1, kSource, long_tail);
    Check(long_tail.calls == std::vector<Call>{
              {"copy", 0x1000 + 168, 0x1000 + 112, 56},
              {"assign", 0x1000 + 112, 0x1000 + 56, 56},
              {"assign", 0x1000 + 56, kSource, 56},
              {"write", kArray + 8, 0x1000 + 224}},
          "long Property tail copies last record then shifts backward");

    Runtime short_tail;
    Configure(short_tail, 56, 3, 5);
    pal4::GameDataShopRecordArray::InsertPropertyCopies(
        kArray, 0x1000 + 112, 2, kSource, short_tail);
    Check(short_tail.calls == std::vector<Call>{
              {"copy", 0x1000 + 224, 0x1000 + 112, 56},
              {"copy", 0x1000 + 168, kSource, 56},
              {"assign", 0x1000 + 112, kSource, 56},
              {"write", kArray + 8, 0x1000 + 280}},
          "short Property tail moves past inserted copies before fill");

    Runtime zero;
    Configure(zero, 56, 3, 5);
    pal4::GameDataShopRecordArray::InsertPropertyCopies(
        kArray, 0x1000 + 56, 0, kSource, zero);
    Check(zero.calls.empty() && zero.words[kArray + 8] == 0x1000 + 168,
          "zero Property copies do not mutate storage");
}

void CheckPropertyGrowth() {
    Runtime runtime;
    Configure(runtime, 56, 3, 3);
    pal4::GameDataShopRecordArray::InsertPropertyCopies(
        kArray, 0x1000 + 56, 2, kSource, runtime);
    Check(runtime.calls == std::vector<Call>{
              {"allocate", 6 * 56},
              {"copy", 0x3000, 0x1000, 56},
              {"copy", 0x3000 + 56, kSource, 56},
              {"copy", 0x3000 + 112, kSource, 56},
              {"copy", 0x3000 + 168, 0x1000 + 56, 56},
              {"copy", 0x3000 + 224, 0x1000 + 112, 56},
              {"destroy-range", 0x1000, 0x1000 + 168, 56},
              {"free", 0x1000},
              {"write", kArray + 12, 0x3000 + 336},
              {"write", kArray + 4, 0x3000},
              {"write", kArray + 8, 0x3000 + 280}},
          "Property growth copies prefix, templates, suffix then releases old");

    Runtime empty;
    pal4::GameDataShopRecordArray::InsertPropertyCopies(
        kArray, 0, 3, kSource, empty);
    Check(empty.calls == std::vector<Call>{
              {"allocate", 3 * 56},
              {"copy", 0x3000, kSource, 56},
              {"copy", 0x3000 + 56, kSource, 56},
              {"copy", 0x3000 + 112, kSource, 56},
              {"destroy-range", 0, 0, 56}, {"free", 0},
              {"write", kArray + 12, 0x3000 + 168},
              {"write", kArray + 4, 0x3000},
              {"write", kArray + 8, 0x3000 + 168}},
          "empty Property array allocates exactly three templates");
}

void CheckGoodsCopies() {
    Runtime growth;
    Configure(growth, 36, 2, 2);
    pal4::GameDataShopRecordArray::InsertGoodsCopies(
        kArray, 0x1000 + 36, 3, kSource, growth);
    Check(growth.calls == std::vector<Call>{
              {"allocate", 5 * 36},
              {"copy", 0x3000, 0x1000, 36},
              {"copy", 0x3000 + 36, kSource, 36},
              {"copy", 0x3000 + 72, kSource, 36},
              {"copy", 0x3000 + 108, kSource, 36},
              {"copy", 0x3000 + 144, 0x1000 + 36, 36},
              {"destroy-range", 0x1000, 0x1000 + 72, 36},
              {"free", 0x1000},
              {"write", kArray + 12, 0x3000 + 180},
              {"write", kArray + 4, 0x3000},
              {"write", kArray + 8, 0x3000 + 180}},
          "Good growth uses max(requested, used) and 36-byte operations");

    Runtime spare;
    Configure(spare, 36, 2, 4);
    pal4::GameDataShopRecordArray::InsertGoodsCopies(
        kArray, 0x1000 + 36, 2, kSource, spare);
    Check(spare.calls == std::vector<Call>{
              {"copy", 0x1000 + 108, 0x1000 + 36, 36},
              {"copy", 0x1000 + 72, kSource, 36},
              {"assign", 0x1000 + 36, kSource, 36},
              {"write", kArray + 8, 0x1000 + 144}},
          "Good short tail constructs remaining template and assigns old slot");
}

void CheckFailure() {
    Runtime runtime;
    Configure(runtime, 56, 2, 2);
    runtime.fail_copy = 2;
    try {
        pal4::GameDataShopRecordArray::InsertPropertyCopies(
            kArray, 0x1000 + 56, 2, kSource, runtime);
        Check(false, "copy failure must propagate");
    } catch (const std::runtime_error& error) {
        Check(std::string(error.what()) == "copy failed" &&
                  runtime.calls.size() == 3 &&
                  runtime.words[kArray + 4] == 0x1000 &&
                  runtime.words[kArray + 8] == 0x1000 + 112,
              "copy failure stops before old storage destruction");
    }
}

void CheckFill() {
    Runtime runtime;
    pal4::GameDataShopRecordArray::FillProperty(
        0x5000, 3, kSource, runtime);
    Check(runtime.calls == std::vector<Call>{
              {"copy", 0x5000, kSource, 56},
              {"copy", 0x5000 + 56, kSource, 56},
              {"copy", 0x5000 + 112, kSource, 56}},
          "Property fill constructs count records at 56-byte intervals");
    runtime.calls.clear();
    pal4::GameDataShopRecordArray::FillGoods(
        0x6000, 2, kSource, runtime);
    Check(runtime.calls == std::vector<Call>{
              {"copy", 0x6000, kSource, 36},
              {"copy", 0x6000 + 36, kSource, 36}},
          "Good fill constructs count records at 36-byte intervals");
    runtime.calls.clear();
    pal4::GameDataShopRecordArray::FillProperty(
        0x5000, 0, kSource, runtime);
    pal4::GameDataShopRecordArray::FillGoods(
        0x6000, 0, kSource, runtime);
    Check(runtime.calls.empty(), "zero-count fills do not construct records");
}

}  // namespace

int main() {
    try {
        CheckPropertySpare();
        CheckPropertyGrowth();
        CheckGoodsCopies();
        CheckFailure();
        CheckFill();
        std::cout << "game_data_shop_record_array_copies_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
