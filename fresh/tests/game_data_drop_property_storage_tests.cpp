#include "pal4/game_data_drop_property_table.h"

#include <algorithm>
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

struct Runtime final : pal4::IGameDataDropPropertyStorageRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, pal4::GameDataAiKeyEntry> keys;
    std::vector<std::string> calls;
    std::vector<std::uint32_t> allocations{0x6000, 0x7000};

    Runtime() {
        words[0x1060] = 0x4000;
        words[0x1064] = 0x4010;
        words[0x1068] = 0x4014;
        words[0x4000] = 9;
        words[0x4004] = 9;
        words[0x4008] = 0xffffffff;
        words[0x400c] = 3;
        words[0x1008] = 0x5000;
        words[0x100c] = 0x5020;
        words[0x1010] = 0x5028;
        for (std::uint32_t i = 0; i < 4; ++i)
            keys[0x5000 + i * 8] = {words[0x4000 + i * 4], i};
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        calls.push_back("word " + std::to_string(address));
    }
    pal4::GameDataAiKeyEntry ReadKey(std::uint32_t address) override {
        return keys.at(address);
    }
    void WriteKey(std::uint32_t address,
                  const pal4::GameDataAiKeyEntry& entry) override {
        keys[address] = entry;
        calls.push_back("key " + std::to_string(address));
    }
    void AppendKey(std::uint32_t array,
                   const pal4::GameDataAiKeyEntry& entry) override {
        Check(array == 0x1004, "key array owner");
        keys[words[0x100c]] = entry;
        words[0x100c] += 8;
        calls.emplace_back("append key");
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        calls.push_back("allocate " + std::to_string(bytes));
        Check(!allocations.empty(), "unexpected allocation");
        const auto result = allocations.front();
        allocations.erase(allocations.begin());
        return result;
    }
    void Free(std::uint32_t address) override {
        calls.push_back("free " + std::to_string(address));
    }
};

std::size_t At(const Runtime& runtime, const std::string& call) {
    const auto found = std::find(runtime.calls.begin(), runtime.calls.end(),
                                 call);
    Check(found != runtime.calls.end(), "expected call missing");
    return static_cast<std::size_t>(found - runtime.calls.begin());
}

void CheckSort() {
    Runtime runtime;
    pal4::GameDataDropPropertyStorage::Sort(0x1000, runtime);
    Check(runtime.words[0x1064] == 0x400c &&
              runtime.words[0x4000] == 9 &&
              runtime.words[0x4004] == 0xffffffff &&
              runtime.words[0x4008] == 3,
          "adjacent equal values compact while first value survives");
    Check(runtime.words[0x100c] == 0x5018 &&
              runtime.keys[0x5000].key == 0xffffffff &&
              runtime.keys[0x5000].record_index == 1 &&
              runtime.keys[0x5008].key == 3 &&
              runtime.keys[0x5008].record_index == 2 &&
              runtime.keys[0x5010].key == 9 &&
              runtime.keys[0x5010].record_index == 0,
          "rebuilt map sorts signed keys and retains compacted indices");
    Check(At(runtime, "word 16388") < At(runtime, "append key"),
          "compaction precedes key-map reconstruction");

    Runtime empty;
    empty.words[0x1064] = empty.words[0x1060];
    pal4::GameDataDropPropertyStorage::Sort(0x1000, empty);
    Check(empty.words[0x100c] == 0x5000 &&
              empty.words[0x1064] == 0x4000,
          "empty value array resets the key end");
}

void CheckReserve() {
    Runtime both;
    Check(pal4::GameDataDropPropertyStorage::Reserve(0x1000, 6, both) ==
              4 &&
              both.words[0x1060] == 0x6000 &&
              both.words[0x1064] == 0x6010 &&
              both.words[0x1068] == 0x6018 &&
              both.words[0x1008] == 0x7000 &&
              both.words[0x100c] == 0x7020 &&
              both.words[0x1010] == 0x7030,
          "both arrays grow and return used key count");
    Check(both.words[0x6000] == 9 &&
              both.words[0x6008] == 0xffffffff &&
              both.keys[0x7018].record_index == 3 &&
              both.calls.front() == "allocate 24" &&
              At(both, "free 16384") < At(both, "allocate 48") &&
              At(both, "free 20480") < both.calls.size(),
          "live values and keys copy before old arrays are freed");

    Runtime values_only;
    values_only.words[0x1010] = 0x5040;
    Check(pal4::GameDataDropPropertyStorage::Reserve(
              0x1000, 6, values_only) == 8 &&
              values_only.words[0x1060] == 0x6000 &&
              values_only.words[0x1008] == 0x5000,
          "value-only growth returns existing key capacity");

    Runtime keys_only;
    keys_only.words[0x1068] = 0x4020;
    keys_only.allocations = {0x7000};
    Check(pal4::GameDataDropPropertyStorage::Reserve(
              0x1000, 6, keys_only) == 4 &&
              keys_only.words[0x1060] == 0x4000 &&
              keys_only.calls.front() == "allocate 48",
          "key-only growth preserves value array and returns used count");

    Runtime enough;
    enough.words[0x1068] = 0x4020;
    enough.words[0x1010] = 0x5040;
    Check(pal4::GameDataDropPropertyStorage::Reserve(
              0x1000, 6, enough) == 8 && enough.calls.empty(),
          "sufficient capacities perform no writes");
}

void CheckNullAndFailedAllocation() {
    Runtime empty;
    empty.words[0x1060] = empty.words[0x1064] = empty.words[0x1068] = 0;
    empty.words[0x1008] = empty.words[0x100c] = empty.words[0x1010] = 0;
    Check(pal4::GameDataDropPropertyStorage::Reserve(0x1000, 3, empty) ==
              0 &&
              empty.words[0x1060] == empty.words[0x1064] &&
              empty.words[0x1068] == 0x600c &&
              empty.words[0x1008] == empty.words[0x100c] &&
              empty.words[0x1010] == 0x7018,
          "null arrays allocate requested capacities with zero used length");

    Runtime failed;
    failed.allocations = {0, 0};
    Check(pal4::GameDataDropPropertyStorage::Reserve(0x1000, 6, failed) ==
              4 &&
              failed.words[0x1060] == 0 &&
              failed.words[0x1064] == 16 &&
              failed.words[0x1008] == 0 &&
              failed.words[0x100c] == 32,
          "native null allocation still updates both array triplets");
}

void CheckScalarCompaction() {
    Runtime runtime;
    runtime.words[0x8000] = 7;
    runtime.words[0x8004] = 7;
    runtime.words[0x8008] = 9;
    runtime.words[0x800c] = 7;
    runtime.words[0x8010] = 7;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> comparisons;
    const auto equal = [&](std::uint32_t left, std::uint32_t right) {
        comparisons.emplace_back(left, right);
        return left == right;
    };
    Check(pal4::GameDataDropPropertyStorage::CompactAdjacent(
              0x8000, 0x8014, 0x9000, equal, runtime) == 0x900c &&
              runtime.words[0x9000] == 7 && runtime.words[0x9004] == 9 &&
              runtime.words[0x9008] == 7,
          "scalar compaction removes adjacent duplicates only");
    Check(comparisons ==
              std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                  {7, 7}, {7, 9}, {9, 7}, {7, 7}},
          "scalar comparator receives the last retained value");
    Check(pal4::GameDataDropPropertyStorage::CompactAdjacent(
              0x8000, 0x8000, 0xa000, equal, runtime) == 0xa000,
          "empty scalar input preserves destination");
    Check(pal4::GameDataDropPropertyStorage::CompactAdjacent(
              0x8000, 0x8014, 0x8000, equal, runtime) == 0x800c &&
              runtime.words[0x8004] == 9 && runtime.words[0x8008] == 7,
          "scalar compaction supports in-place output");
}

}  // namespace

int main() {
    try {
        CheckSort();
        CheckReserve();
        CheckNullAndFailedAllocation();
        CheckScalarCompaction();
        std::cout << "game_data_drop_property_storage_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
