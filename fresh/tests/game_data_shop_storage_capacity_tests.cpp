#include "pal4/game_data_shop_storage_capacity.h"

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
    bool operator==(const Call&) const = default;
};

struct Runtime final : pal4::IGameDataShopStorageCapacityRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<Call> calls;
    std::vector<std::uint32_t> allocations;
    std::size_t next_allocation = 0;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        calls.push_back({"write", address, value});
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        calls.push_back({"allocate", bytes});
        if (next_allocation >= allocations.size())
            throw std::runtime_error("unexpected allocation");
        return allocations[next_allocation++];
    }
    void CopyRecord(std::uint32_t destination,
                    std::uint32_t source) override {
        calls.push_back({"copy-record", destination, source});
    }
    void DestroyRecord(std::uint32_t address) override {
        calls.push_back({"destroy-record", address});
    }
    void CopyIndex(std::uint32_t destination,
                   std::uint32_t source) override {
        calls.push_back({"copy-index", destination, source});
    }
    void Free(std::uint32_t address) override {
        calls.push_back({"free", address});
    }
};

constexpr std::uint32_t kObject = 0x9000;

void CheckBothArraysGrow() {
    Runtime runtime;
    runtime.allocations = {0x3000, 0x4000};
    runtime.words[kObject + 96] = 0x1000;
    runtime.words[kObject + 100] = 0x1000 + 2 * 56;
    runtime.words[kObject + 104] = 0x1000 + 3 * 56;
    runtime.words[kObject + 8] = 0x2000;
    runtime.words[kObject + 12] = 0x2000 + 2 * 8;
    runtime.words[kObject + 16] = 0x2000 + 3 * 8;
    Check(pal4::GameDataShopStorageCapacity::Reserve(
              kObject, 5, runtime) == 0x4010,
          "index growth returns its new active end");
    Check(runtime.calls == std::vector<Call>{
              {"allocate", 5 * 56},
              {"copy-record", 0x3000, 0x1000},
              {"copy-record", 0x3000 + 56, 0x1000 + 56},
              {"destroy-record", 0x1000},
              {"destroy-record", 0x1000 + 56},
              {"free", 0x1000},
              {"write", kObject + 104, 0x3000 + 5 * 56},
              {"write", kObject + 96, 0x3000},
              {"write", kObject + 100, 0x3000 + 2 * 56},
              {"allocate", 5 * 8},
              {"copy-index", 0x4000, 0x2000},
              {"copy-index", 0x4008, 0x2008},
              {"free", 0x2000},
              {"write", kObject + 16, 0x4028},
              {"write", kObject + 8, 0x4000},
              {"write", kObject + 12, 0x4010}},
          "growth copies before destruction and updates both triplets");
}

void CheckIndependentCapacities() {
    Runtime runtime;
    runtime.words[kObject + 96] = 0x1000;
    runtime.words[kObject + 100] = 0x1038;
    runtime.words[kObject + 104] = 0x1000 + 6 * 56;
    runtime.words[kObject + 8] = 0x2000;
    runtime.words[kObject + 12] = 0x2008;
    runtime.words[kObject + 16] = 0x2030;
    Check(pal4::GameDataShopStorageCapacity::Reserve(
              kObject, 4, runtime) == 6 && runtime.calls.empty(),
          "no growth returns existing index capacity as a count");

    runtime.words[kObject + 16] = 0x2008;
    runtime.allocations = {0x4000};
    Check(pal4::GameDataShopStorageCapacity::Reserve(
              kObject, 4, runtime) == 0x4008 &&
              runtime.calls.front() == Call{"allocate", 32} &&
              runtime.calls[1] == Call{"copy-index", 0x4000, 0x2000},
          "index alone grows without touching record storage");

    Runtime records_only;
    records_only.words[kObject + 96] = 0x1000;
    records_only.words[kObject + 100] = 0x1000;
    records_only.words[kObject + 104] = 0x1000 + 56;
    records_only.words[kObject + 8] = 0x2000;
    records_only.words[kObject + 12] = 0x2000;
    records_only.words[kObject + 16] = 0x2000 + 5 * 8;
    records_only.allocations = {0x3000};
    Check(pal4::GameDataShopStorageCapacity::Reserve(
              kObject, 3, records_only) == 5 &&
              records_only.calls == std::vector<Call>{
                  {"allocate", 3 * 56}, {"free", 0x1000},
                  {"write", kObject + 104, 0x3000 + 3 * 56},
                  {"write", kObject + 96, 0x3000},
                  {"write", kObject + 100, 0x3000}},
          "record growth leaves sufficient index capacity intact");
}

void CheckEmptyAndNegativeRequests() {
    Runtime empty;
    empty.allocations = {0x3000, 0x4000};
    Check(pal4::GameDataShopStorageCapacity::Reserve(
              kObject, 2, empty) == 0x4000 &&
              empty.calls == std::vector<Call>{
                  {"allocate", 112}, {"free", 0},
                  {"write", kObject + 104, 0x3070},
                  {"write", kObject + 96, 0x3000},
                  {"write", kObject + 100, 0x3000},
                  {"allocate", 16}, {"free", 0},
                  {"write", kObject + 16, 0x4010},
                  {"write", kObject + 8, 0x4000},
                  {"write", kObject + 12, 0x4000}},
          "empty arrays reserve without copying and retain zero active size");

    Runtime negative;
    negative.allocations = {0x3000, 0x4000};
    Check(pal4::GameDataShopStorageCapacity::Reserve(
              kObject, -1, negative) == 0x4000 &&
              negative.calls[0] == Call{"allocate", 0} &&
              negative.calls[5] == Call{"allocate", 0} &&
              negative.words[kObject + 104] == 0x3000 - 56 &&
              negative.words[kObject + 16] == 0x4000 - 8,
          "negative request takes unsigned growth branches but clamps allocations");
}

void CheckGoodsReserve() {
    Runtime runtime;
    runtime.allocations = {0x3000, 0x4000};
    runtime.words[kObject + 96] = 0x1000;
    runtime.words[kObject + 100] = 0x1000 + 2 * 36;
    runtime.words[kObject + 104] = 0x1000 + 3 * 36;
    runtime.words[kObject + 8] = 0x2000;
    runtime.words[kObject + 12] = 0x2008;
    runtime.words[kObject + 16] = 0x2010;
    Check(pal4::GameDataShopStorageCapacity::ReserveGoods(
              kObject, 4, runtime) == 0x4008 &&
              runtime.calls == std::vector<Call>{
                  {"allocate", 4 * 36},
                  {"copy-record", 0x3000, 0x1000},
                  {"copy-record", 0x3000 + 36, 0x1000 + 36},
                  {"destroy-record", 0x1000},
                  {"destroy-record", 0x1000 + 36},
                  {"free", 0x1000},
                  {"write", kObject + 104, 0x3000 + 4 * 36},
                  {"write", kObject + 96, 0x3000},
                  {"write", kObject + 100, 0x3000 + 2 * 36},
                  {"allocate", 4 * 8},
                  {"copy-index", 0x4000, 0x2000},
                  {"free", 0x2000},
                  {"write", kObject + 16, 0x4020},
                  {"write", kObject + 8, 0x4000},
                  {"write", kObject + 12, 0x4008}},
          "Good reserve uses 36-byte records and eight-byte index entries");
    runtime.calls.clear();
    Check(pal4::GameDataShopStorageCapacity::ReserveGoods(
              kObject, 3, runtime) == 4 && runtime.calls.empty(),
          "Good reserve leaves sufficient capacity unchanged");

    Runtime empty;
    empty.allocations = {0x5000, 0x6000};
    Check(pal4::GameDataShopStorageCapacity::ReserveGoods(
              kObject, -1, empty) == 0x6000 &&
              empty.calls[0] == Call{"allocate", 0} &&
              empty.calls[5] == Call{"allocate", 0} &&
              empty.words[kObject + 104] == 0x5000 - 36 &&
              empty.words[kObject + 16] == 0x6000 - 8,
          "negative Good reserve request clamps allocation but wraps capacity");
}

}  // namespace

int main() {
    try {
        CheckBothArraysGrow();
        CheckIndependentCapacities();
        CheckEmptyAndNegativeRequests();
        CheckGoodsReserve();
        std::cout << "game_data_shop_storage_capacity_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
